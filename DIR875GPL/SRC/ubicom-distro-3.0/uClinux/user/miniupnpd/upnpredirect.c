/* $Id: upnpredirect.c,v 1.6 2009/07/08 11:18:33 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2007 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "upnpredirect.h"
#include "upnpglobalvars.h"
#include "upnpevents.h"
#if defined(USE_NETFILTER)
#include "netfilter/iptcrdr.h"
#endif
#if defined(USE_PF)
#include "pf/obsdrdr.h"
#endif
#if defined(USE_IPF)
#include "ipf/ipfrdr.h"
#endif
#ifdef USE_MINIUPNPDCTL
#include <stdio.h>
#include <unistd.h>
#endif
#ifdef ENABLE_LEASEFILE
#include <sys/stat.h>

#include <stdarg.h>

#endif

#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
#include "nvram_funs.h"
#endif

#ifdef MINIUPNPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/* proto_atoi() 
 * convert the string "UDP" or "TCP" to IPPROTO_UDP and IPPROTO_UDP */
static int
proto_atoi(const char * protocol)
{
        int proto = IPPROTO_TCP;
        if(strcmp(protocol, "UDP") == 0)
                proto = IPPROTO_UDP;
        return proto;
}

#ifdef ENABLE_LEASEFILE
/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: For updating expire time
*/
static int lease_file_update(unsigned short eport, const char * iaddr, unsigned short iport
                        , int proto, const char * desc
                        , const char * eaddr, int NewEnabled, long int LeaseDuration){
        FILE* fd, *fdt;
        int tmp;
        char buf[512];
        char str[32];
        char tmpfilename[128];
        int str_size = 0, buf_size = 0;
	/*	Date:	2009-10-05
	*	Name:	jimmy huang
	*	Reason:	to fixed the bug, if there is no rule in lease file,
	*			we should write down the new rules to lease file
	*/
	int found_record = 0;

        DEBUG_MSG("\n%s, %s (%d): begin \n",__FILE__,__FUNCTION__,__LINE__);
        if (lease_file == NULL){
                DEBUG_MSG("%s (%d): lease_file not specified ! \n",__FUNCTION__,__LINE__);
                return 0;
        }

        DEBUG_MSG("%s (%d): Update eport = %hu , protocol = %s (%d) with new expire_time = %ld, NewEnabled = %d\n"
                        ,__FUNCTION__,__LINE__
                        , eport
                        , proto == IPPROTO_TCP ? "TCP" :
                                (proto == IPPROTO_UDP ? "DUP" : "Unknown Protocol")
                        , proto
                        , LeaseDuration + time(NULL)
                        , NewEnabled
                        );

        if (strlen( lease_file) + 7 > sizeof(tmpfilename)) {
                syslog(LOG_ERR, "Lease filename is too long");
                return -1;
        }

        strncpy( tmpfilename, lease_file, sizeof(tmpfilename) );
        strncat( tmpfilename, "XXXXXX", sizeof(tmpfilename) - strlen(tmpfilename));

RETRY_OPEN:
        fd = fopen( lease_file, "r");
        if (fd==NULL) {
                DEBUG_MSG("%s (%d): can not open %s \n",__FUNCTION__,__LINE__,lease_file);
		/*	Date:	2009-10-05
		*	Name:	jimmy huang
		*	Reason:	to fixed the bug, When rc only restart apps, (ex: save log_seting in status page)
		*			firewall will not restart
		*			Thus, when miniupnpd are reloaded port mapping rules,
		*			- move lease_file to temp file
		*			- the rules still exists in firewall
		*			- so the code will go through here, and the lease_file is not exist (already move to temp file)
		*			- so we need create a new one to record, if we do not, we can not remember any port mapping rules
		*				any more, cause lease file is not exist
		*/
		if( (fd = fopen( lease_file, "w")) != NULL ){
			DEBUG_MSG("%s (%d): Create a new \"%s\" for updating new record \n",__FUNCTION__,__LINE__,lease_file);
			fclose(fd);
			goto RETRY_OPEN;
		}

                return 0;
        }
/*
        snprintf( str, sizeof(str), "%s:%u", ((proto==IPPROTO_TCP)?"TCP":"UDP"), eport);
        str_size = strlen(str);
*/
        snprintf( str, sizeof(str), "/%hu/%s/", eport , ((proto==IPPROTO_TCP)?"TCP":"UDP"));
        str_size = strlen(str);
        DEBUG_MSG("%s (%d): find and updae [%s] \n",__FUNCTION__,__LINE__,str);

        tmp = mkstemp(tmpfilename);
        if (tmp==-1) {
                fclose(fd);
                syslog(LOG_ERR, "could not open temporary lease file");
                return -1;
        }
        fchmod(tmp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        fdt = fdopen(tmp, "a");

        buf[sizeof(buf)-1] = 0;
        while( fgets( buf, sizeof(buf)-1, fd) != NULL) {
                buf_size = strlen(buf);
                DEBUG_MSG("%s (%d): buf = [%s] ",__FUNCTION__,__LINE__,buf);
                // jimmy test, jimmy care maybe here need to improve ?
                //if (buf_size < str_size || strncmp( str, buf, str_size)!=0) {
                if (strstr( buf, str) == 0) {
                // ----------
                        fwrite(buf, buf_size, 1, fdt);
                }else{
                /*
                        Format for lease file
                        description / remote ip / ex-port / proto / int-ip / int-port / leasetime / expire time / "Reserved"
                */
			/*	Date:	2009-10-05
			*	Name:	jimmy huang
			*	Reason:	to fixed the bug, if there is no rule in lease file,
			*			we should write down the new rules to lease file
			*/
			found_record = 1;
                        memset(buf,'\0',sizeof(buf));
                        sprintf(buf,"%s/%s/%hu/%s/%s/%hu/%ld/%ld/%s\n"
                                        , desc ? desc : ""
                                        , eaddr ? eaddr : ""
                                        , eport
                                        , ((proto==IPPROTO_TCP) ? "TCP":"UDP")
                                        , iaddr, iport
                                        , LeaseDuration, LeaseDuration + time(NULL)
                                        , "Reserved"
                                        );
                        fwrite(buf, strlen(buf), 1, fdt);
                }
        }

	/*	Date:	2009-10-05
	*	Name:	jimmy huang
	*	Reason:	to fixed the bug, if there is no rule in lease file,
	*			we should write down the new rules to lease file
	*/
	if(found_record == 0){
		memset(buf,'\0',sizeof(buf));
			sprintf(buf,"%s/%s/%hu/%s/%s/%hu/%ld/%ld/%s\n"
					, desc ? desc : ""
					, eaddr ? eaddr : "" 
					, eport
					, ((proto==IPPROTO_TCP) ? "TCP":"UDP")
					, iaddr, iport
					, LeaseDuration, LeaseDuration + time(NULL)
					, "Reserved"
					);
			fwrite(buf, strlen(buf), 1, fdt);
	}

        fclose(fdt);
        fclose(fd);

        if (rename( tmpfilename, lease_file) < 0) {
                syslog(LOG_ERR, "could not rename temporary lease file to %s", lease_file);
                remove( tmpfilename);
        }

        DEBUG_MSG("%s, %s (%d): End \n",__FILE__,__FUNCTION__,__LINE__);
        return 0;
}
// ----------

/*
static int lease_file_add( unsigned short eport, const char * iaddr, unsigned short iport, int proto, const char * desc)
*/
static int lease_file_add( unsigned short eport, const char * iaddr, unsigned short iport
                        , int proto, const char * desc
                        , const char * eaddr, int Enabled, long int LeaseDuration , long int expire_time)
{
        FILE* fd;
        char buf[128];
        char str[32];


        if (lease_file == NULL){
                DEBUG_MSG("\n%s (%d): begin, lease_file not been specified !!\n",__FUNCTION__,__LINE__);
                return 0;
        }

        DEBUG_MSG("\n%s (%d): begin, with lease_file %s\n",__FUNCTION__,__LINE__,lease_file);

        memset(str,'\0',sizeof(str));
        snprintf( str, sizeof(str), "/%hu/%s/", eport , ((proto==IPPROTO_TCP)?"TCP":"UDP"));
        // search first, in case duplicated rules are re-add in this file
        // ex: firewall has been clean because Web GUI add new virtual or other setting
        //              rc will only clear firewall then re-add new rules without miniupnpd's rules
        if((fd = fopen(lease_file,"r")) != NULL){
                memset(buf,'\0',sizeof(buf));
                while(!feof(fd)){
                        fgets(buf,sizeof(buf),fd);
                        if(buf[0] == '\n'){
                                break;
                        }

                        if(strstr(buf,str) != 0){
                                // this rule already exists in the file
                                DEBUG_MSG("%s (%d): port %hu , proto %s, already exists, ignore it ! \n"
                                                ,__FUNCTION__,__LINE__, eport,((proto==IPPROTO_TCP)?"TCP":"UDP"));
                                fclose(fd);
                                return 0;
                        }
                }
                fclose(fd);
        }

        fd = fopen( lease_file, "a");
        if (fd==NULL) {
                syslog(LOG_ERR, "could not open lease file: %s", lease_file);
                return -1;
        }


/*
        fprintf( fd, "%s:%hu:%s:%hu:%s\n", ((proto==IPPROTO_TCP)?"TCP":"UDP"), eport, iaddr, iport, desc);
*/
// libupnp used
//      fprintf(fd,"%s/%s/%s/%s/%s/%s/%ld/%ld/%s\n",desc ? desc : ""
//                                      ,protocol
//                                      ,remoteHost ? remoteHost : "",externalPort
//                                      ,internalClient,internalPort
//                                      ,duration,duration + time(NULL)
//                                      ,devudn
//                                      );
        /*
                Format for lease file
                description / remote ip / ex-port / proto / int-ip / int-port / leasetime / expire time / "Reserved"
        */
        fprintf(fd,"%s/%s/%hu/%s/%s/%hu/%ld/%ld/%s\n"
                                        , desc ? desc : ""
                                        , eaddr ? eaddr : ""
                                        , eport
                                        , ((proto==IPPROTO_TCP) ? "TCP":"UDP")
                                        , iaddr, iport
                                        , LeaseDuration,expire_time
                                        , "Reserved"
                                        );
        fclose(fd);

        return 0;
}


//static int lease_file_remove( unsigned short eport, int proto)
int lease_file_remove( unsigned short eport, int proto)
// -----------
{
        FILE* fd, *fdt;
        int tmp;
        char buf[512];
        char str[32];
        char tmpfilename[128];
        int str_size, buf_size;

        DEBUG_MSG("\n%s, %s (%d): begin \n",__FILE__,__FUNCTION__,__LINE__);
        if (lease_file == NULL){
                DEBUG_MSG("%s (%d): lease_file not specified ! \n",__FUNCTION__,__LINE__);
                return 0;
        }

        DEBUG_MSG("%s (%d): eport = %hu , protocol = %s (%d)\n"
                        ,__FUNCTION__,__LINE__
                        , eport
                        , proto == IPPROTO_TCP ? "TCP" :
                                (proto == IPPROTO_UDP ? "DUP" : "Unknown Protocol")
                        , proto
                        );

        if (strlen( lease_file) + 7 > sizeof(tmpfilename)) {
                syslog(LOG_ERR, "Lease filename is too long");
                return -1;
        }

        strncpy( tmpfilename, lease_file, sizeof(tmpfilename) );
        strncat( tmpfilename, "XXXXXX", sizeof(tmpfilename) - strlen(tmpfilename));

        fd = fopen( lease_file, "r");
        if (fd==NULL) {
                DEBUG_MSG("%s (%d): can not open %s \n",__FUNCTION__,__LINE__,lease_file);
                return 0;
        }
// libupnp used
//      fprintf(fd,"%s/%s/%s/%s/%s/%s/%ld/%ld/%s\n",desc ? desc : ""
//                                      ,protocol
//                                      ,remoteHost ? remoteHost : "",externalPort
//                                      ,internalClient,internalPort
//                                      ,duration,duration + time(NULL)
//                                      ,devudn
//                                      );
/*
        snprintf( str, sizeof(str), "%s:%u", ((proto==IPPROTO_TCP)?"TCP":"UDP"), eport);
        str_size = strlen(str);
*/
        snprintf( str, sizeof(str), "/%hu/%s/", eport , ((proto==IPPROTO_TCP)?"TCP":"UDP"));
        str_size = strlen(str);
        DEBUG_MSG("%s (%d): find and del [%s] \n",__FUNCTION__,__LINE__,str);

        tmp = mkstemp(tmpfilename);
        if (tmp==-1) {
                fclose(fd);
                syslog(LOG_ERR, "could not open temporary lease file");
                return -1;
        }
        fchmod(tmp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        fdt = fdopen(tmp, "a");

        buf[sizeof(buf)-1] = 0;
        while( fgets( buf, sizeof(buf)-1, fd) != NULL) {
                buf_size = strlen(buf);
                DEBUG_MSG("%s (%d): buf = [%s] ",__FUNCTION__,__LINE__,buf);

                //if (buf_size < str_size || strncmp( str, buf, str_size)!=0) {
                if (strstr( buf, str) == 0) {
                // ----------
                        fwrite(buf, buf_size, 1, fdt);
                }
        }
        fclose(fdt);
        fclose(fd);

        if (rename( tmpfilename, lease_file) < 0) {
                syslog(LOG_ERR, "could not rename temporary lease file to %s", lease_file);
                remove( tmpfilename);
        }

        DEBUG_MSG("%s, %s (%d): End \n",__FILE__,__FUNCTION__,__LINE__);
        return 0;

}


/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: Add for parse lease file
*/
int get_element(char *input, char *token, char *fmt, ...){
    //desc, protocol, remoteHost, externalPort, internalClient, internalPort,duration, desc,devudn
        va_list  ap;
        int arg, count = 0;
        char *c, *tmp;
        char *p_head = NULL, *p_tail = NULL;
        char *input_end = NULL;
        p_head = input;

        input_end = input + strlen(input);

        if (!input)
                return 0;
        tmp = input;
        for(count=0; (tmp = strpbrk(tmp, token)); tmp+=1, count++);
        count +=1;
        va_start(ap, fmt);
        for (arg = 0, c = fmt; c && *c && arg < count;) {
                if (*c++ != '%'){
                        continue;
                }
                switch (*c) {
                        case 'd':
                                if(!arg)
                                        *(va_arg(ap, int *)) = atoi(strtok(input, token));
                                else
                                        *(va_arg(ap, int *)) = atoi(strtok(NULL, token));
                                break;
                        case 's':
                                p_tail = strchr(p_head,'/');

                                if((p_tail == NULL) || (p_tail > input_end) ){
                                        return 0;
                                }

                                if(p_head == p_tail){
                                        p_head = p_tail;
                                        //*(va_arg(ap, char **)) = NULL;
                                        *(va_arg(ap, char **)) = "";
                                }else{
                                        *p_tail='\0';
                                        *(va_arg(ap, char **)) = p_head;
                                }
                                p_head = p_tail + 1;
                                break;
                }
                arg++;
        }

        va_end(ap);

        return arg;
}
// ----------

/* reload_from_lease_file()
 * read lease_file and add the rules contained
 */
int reload_from_lease_file(int sig)
{
        FILE * fd;

        unsigned short eport = 0, iport = 0;
        char * proto;
        char * iaddr;
        char * desc;
        char line[128];

        char * eaddr = NULL;
        char * char_iport = NULL;
        char * char_eport = NULL;
        char * char_LeaseDuration= NULL;
        char * char_expireTime = NULL;
        char * char_reserved = NULL;
        int Enabled = 1;
        int expire_time_tmp = 0;
        char tmp_path[32] = {0};

    struct rdr_desc * p = NULL, * last = NULL;
        p = rdr_desc_list;

        DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);

        if(!lease_file) return -1;

        sprintf(tmp_path,"%s_tmp",lease_file);
        rename(lease_file,tmp_path);
        unlink(lease_file);

        //fd = fopen( lease_file, "r");
        fd = fopen( tmp_path, "r");
        if (fd==NULL) {
                syslog(LOG_ERR, "could not open lease file: %s", lease_file);
                return -1;
        }
    /*  Date:   2009-04-21
        *       Name:   jimmy huang
        *       Reason:
        *                       when reload rules , if rdr_desc link list already has something, clear it all
        *       Note:   currently, we can directly remove the link list and reload without considering
        *                       the rules already been added on iptable, because in our usage
        *                       1.booting -> start upnpd
        *                       2.rc restart -> start upnpd
        *                       3. UI add new iptables rule -> send signal to miniupnpd to reload_file
        *                       in those scenario, iptable rules will be cleared first
        *                       so we don't need to take care of that
        */

        while(p)
        {
                DEBUG_MSG("%s (%d): rdr_desc_list is not null, clear it\n",__FUNCTION__,__LINE__);
                last = p;
                p = p->next;
                free(last);
        }
        rdr_desc_list = NULL;

        while(fgets(line, sizeof(line), fd)) {
                syslog(LOG_DEBUG, "parsing lease file line '%s'", line);
        /*
                proto = line;
                p = strchr(line, ':');
                if(!p) {
                        syslog(LOG_ERR, "unrecognized data in lease file");
                        continue;
                }
                *(p++) = '\0';
                iaddr = strchr(p, ':');
                if(!iaddr) {
                        syslog(LOG_ERR, "unrecognized data in lease file");
                        continue;
                }
                *(iaddr++) = '\0';
                eport = (unsigned short)atoi(p);
                p = strchr(iaddr, ':');
                if(!p) {
                        syslog(LOG_ERR, "unrecognized data in lease file");
                        continue;
                }
                *(p++) = '\0';
                desc = strchr(p, ':');
                if(!desc) {
                        syslog(LOG_ERR, "unrecognized data in lease file");
                        continue;
                }
                *(desc++) = '\0';
                iport = (unsigned short)atoi(p);
        */
                /*
                        Format for lease file
                        description / remote ip / ex-port / proto / int-ip / int-port / leasetime / expire time / "Reserved"
                */
                get_element(line,"/","%s %s %s %s %s %s %s %s %s"
                                                        , &desc
                                                        , &eaddr
                                                        , &char_eport
                                                        , &proto
                                                        , &iaddr
                                                        , &char_iport
                                                        , &char_LeaseDuration
                                                        , &char_expireTime
                                                        , &char_reserved);

                //if(upnp_redirect(eport, iaddr, iport, proto, desc) == -1) {
                if(sig){
                        if(atoi(char_LeaseDuration) == 0){
                                if(upnp_redirect((unsigned short)atoi(char_eport), iaddr
                                                , (unsigned short)atoi(char_iport), proto, desc
                                                , eaddr, Enabled , 0 ) == -1)
                                {
                                        syslog(LOG_ERR, "Failed to redirect %hu -> %s:%hu protocol %s",
                                                eport, iaddr, iport, proto);
                                }
                        }else{
                                expire_time_tmp = atoi(char_expireTime) - time(NULL);
                                if(expire_time_tmp > 0){
                                        if(upnp_redirect((unsigned short)atoi(char_eport), iaddr
                                                        , (unsigned short)atoi(char_iport), proto, desc
                                                        , eaddr, Enabled , (long int)expire_time_tmp ) == -1)
                                        {
                                                syslog(LOG_ERR, "Failed to redirect %hu -> %s:%hu protocol %s",
                                                        eport, iaddr, iport, proto);
                                        }
                                }
                        }
                }else{
                        if(upnp_redirect((unsigned short)atoi(char_eport), iaddr
                                                , (unsigned short)atoi(char_iport), proto, desc
                                                , eaddr, Enabled , (long int)atoi(char_LeaseDuration) ) == -1)
                        {
                                syslog(LOG_ERR, "Failed to redirect %hu -> %s:%hu protocol %s",
                                        eport, iaddr, iport, proto);
                        }
                }

        }
        fclose(fd);

        return 0;
}
#endif

/* upnp_redirect() 
 * calls OS/fw dependant implementation of the redirection.
 * protocol should be the string "TCP" or "UDP"
 * returns: 0 on success
 *          -1 failed to redirect
 *          -2 already redirected
 *          -3 permission check failed
 */

/*
int 
upnp_redirect(unsigned short eport, 
              const char * iaddr, unsigned short iport,
              const char * protocol, const char * desc,
                          const char * eaddr)// Chun add: for CD_ROUTER
*/
int upnp_redirect(unsigned short eport, 
              const char * iaddr, unsigned short iport,
              const char * protocol, const char * desc,
                          const char * eaddr
                          , int Enabled, long int LeaseDuration)
// ---------------
{
        int proto, r;
        char iaddr_old[32];
        unsigned short iport_old;
        struct in_addr address;
        long int new_expire_time = 0;
        proto = proto_atoi(protocol);
        if(inet_aton(iaddr, &address) < 0)
        {
                syslog(LOG_ERR, "inet_aton(%s) : %m", iaddr);
                return -1;
        }
        DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
        DEBUG_MSG("%s:%s (%d), NewRemoteHost = %s\n",__FILE__,__FUNCTION__,__LINE__,eaddr ? eaddr : "");
        DEBUG_MSG("%s:%s (%d), NewExternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,eport);
        DEBUG_MSG("%s:%s (%d), NewProtocol = %s\n",__FILE__,__FUNCTION__,__LINE__,protocol);
        DEBUG_MSG("%s:%s (%d), NewInternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,iport);
        DEBUG_MSG("%s:%s (%d), NewInternalClient = %s\n",__FILE__,__FUNCTION__,__LINE__,iaddr ? iaddr : "");
        DEBUG_MSG("%s:%s (%d), NewEnabled = %d\n",__FILE__,__FUNCTION__,__LINE__,Enabled);
        DEBUG_MSG("%s:%s (%d), NewPortMappingDesc = %s\n",__FILE__,__FUNCTION__,__LINE__,desc ? desc : "");
        DEBUG_MSG("%s:%s (%d), NewLeaseDuration = %ld\n",__FILE__,__FUNCTION__,__LINE__,LeaseDuration);
        if(!check_upnp_rule_against_permissions(upnppermlist, num_upnpperm,
                                                eport, address, iport))
        {
                syslog(LOG_INFO, "redirection permission check failed for "
                                 "%hu->%s:%hu %s", eport, iaddr, iport, protocol);
                return -3;
        }
        r = get_redirect_rule(ext_if_name, eport, proto,
                              iaddr_old, sizeof(iaddr_old), &iport_old, 0, 0, 0, 0, &new_expire_time);
        //if(r == 0)
        if(r >= 0)
        {
                // 0, already added by miniupnpd previously
                // 1, already added by WEB GUI previously
                /* if existing redirect rule matches redirect request return success
                 * xbox 360 does not keep track of the port it redirects and will
                 * redirect another port when receiving ConflictInMappingEntry */

                //if(strcmp(iaddr,iaddr_old)==0 && iport==iport_old)
                if(r == 0 && strcmp(iaddr,iaddr_old)==0 && iport==iport_old)
                {
                        /*      Date:   2009-04-20
                        *       Name:   jimmy huang
                        *       Reason: For updating expire time
                        */
                        //syslog(LOG_INFO, "ignoring redirect request as it matches existing redirect");
                        DEBUG_MSG("%s (%d): redirect rules already existing in MINIUPNPD chain\n",__FUNCTION__,__LINE__);
                        if(Enabled){
                                DEBUG_MSG("%s (%d): go update expire_time\n",__FUNCTION__,__LINE__);
#ifdef ENABLE_LEASEFILE
                                lease_file_update( eport, iaddr, iport, proto, desc, eaddr, Enabled, LeaseDuration);
#endif
                                update_redirect_desc(eport, proto, desc, eaddr,iport, iaddr
                                                                ,Enabled,LeaseDuration,&new_expire_time);
                        }else{
                                DEBUG_MSG("%s (%d): Enabled = %d, so ignore it\n",__FUNCTION__,__LINE__,Enabled);
                        }
                }
                else
                {
                        if(r == 1){
                                DEBUG_MSG("%s (%d): port %hu protocol %s redirected to %s:%hu, already added via WEB GUI previously\n"
                                                ,__FUNCTION__,__LINE__,eport, protocol, iaddr_old, iport_old);
                        }else{
                                DEBUG_MSG("%s (%d): port %hu protocol %s already redirected to %s:%hu\n"
                                                ,__FUNCTION__,__LINE__,eport, protocol, iaddr_old, iport_old);
                                syslog(LOG_INFO, "port %hu protocol %s already redirected to %s:%hu",
                                                eport, protocol, iaddr_old, iport_old);
                        }
                        return -2;
                }
        }
        else
        {
                syslog(LOG_INFO, "redirecting port %hu to %s:%hu protocol %s for: %s",
                        eport, iaddr, iport, protocol, desc);

                //return upnp_redirect_internal(eport, iaddr, iport, proto, desc);
                //return upnp_redirect_internal(eport, iaddr, iport, proto, desc, eaddr);
                return upnp_redirect_internal(eport, iaddr, iport, proto, desc, eaddr
                                                , Enabled , LeaseDuration
                                                );
                // -------------
#if 0
                if(add_redirect_rule2(ext_if_name, eport, iaddr, iport, proto, desc) < 0)
                {
                        return -1;
                }

                syslog(LOG_INFO, "creating pass rule to %s:%hu protocol %s for: %s",
                        iaddr, iport, protocol, desc);
                if(add_filter_rule2(ext_if_name, iaddr, eport, iport, proto, desc) < 0)
                {
                        /* clean up the redirect rule */
#if !defined(__linux__)
                        delete_redirect_rule(ext_if_name, eport, proto);
#endif
                        return -1;
                }
#endif
        }

        return 0;
}

int
/*
upnp_redirect_internal(unsigned short eport,
                       const char * iaddr, unsigned short iport,
                       int proto, const char * desc,
                                           const char * eaddr)// Chun add: for CD_ROUTER
*/
upnp_redirect_internal(unsigned short eport,
                       const char * iaddr, unsigned short iport,
                       int proto, const char * desc,
                                           const char * eaddr, int Enabled, long int LeaseDuration)// Chun add: for CD_ROUTER
// ----------------------------------------
{
        long int out_expire_time = 0;
        /*syslog(LOG_INFO, "redirecting port %hu to %s:%hu protocol %s for: %s",
                eport, iaddr, iport, protocol, desc);                   */
        DEBUG_MSG("\n%s (%d): begin, go add_redirect_rule2()\n",__FUNCTION__,__LINE__);

        // NAT PREROUTE
        //if(add_redirect_rule2(ext_if_name, eport, iaddr, iport, proto, desc) < 0)
        if(add_redirect_rule2(ext_if_name, eport, iaddr, iport
                        , proto, desc, eaddr, Enabled, LeaseDuration, &out_expire_time) < 0)
        // ---------------------
        {
                DEBUG_MSG("%s (%d): go add_redirect_rule2() failed\n",__FUNCTION__,__LINE__);
                return -1;
        }

/*      syslog(LOG_INFO, "creating pass rule to %s:%hu protocol %s for: %s",
                iaddr, iport, protocol, desc);*/
        // Filter Forward
        DEBUG_MSG("%s (%d): go add_filter_rule2() \n",__FUNCTION__,__LINE__);
        if(add_filter_rule2(ext_if_name, iaddr, eport, iport, proto, desc) < 0)
        {
                DEBUG_MSG("%s (%d): go add_filter_rule2() failed\n",__FUNCTION__,__LINE__);
                /* clean up the redirect rule */
#if !defined(__linux__)
                delete_redirect_rule(ext_if_name, eport, proto);
#endif
                return -1;
        }
#ifdef ENABLE_EVENTS
/*      Date:   2009-04-29
*       Name:   jimmy huang
*       Reason: Currently, due to miniupnpd's architecture(select,noblock),
*                       we may not immediately send notify after variable changed (Add / Del PortMapping...)
*                       This definition is used to force when after soap, we use sock with block
*                       to send notify right after AddPortMapping, search this definition within codes
*                       for more detail
*       Note:   Not test this feature well
*/
#ifdef IMMEDIATELY_SEND_NOTIFY
        // we use check_notify_right_now(), in upnpsoap.c AddPortMapping() to send notify
#else
        DEBUG_MSG("%s (%d): go upnp_event_var_change_notify() \n",__FUNCTION__,__LINE__);
        upnp_event_var_change_notify(EWanIPC);
#endif
#endif

#ifdef ENABLE_LEASEFILE
        /*
        lease_file_add( eport, iaddr, iport, proto, desc);
        */
        lease_file_add( eport, iaddr, iport, proto, desc, eaddr, Enabled, LeaseDuration, out_expire_time);
#endif
        DEBUG_MSG("%s (%d): end \n",__FUNCTION__,__LINE__);
        return 0;
}



int
upnp_get_redirection_infos(unsigned short eport, const char * protocol,
                           unsigned short * iport,
                           char * iaddr, int iaddrlen,
                           char * desc, int desclen, long int *expire_time)
{
        DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
        if(desc && (desclen > 0))
                desc[0] = '\0';
        return get_redirect_rule(ext_if_name, eport, proto_atoi(protocol),
                                 iaddr, iaddrlen, iport, desc, desclen, 0, 0, expire_time);
}

int
upnp_get_redirection_infos_by_index(int index_local,
                                    unsigned short * eport, char * protocol,
                                    unsigned short * iport, 
                                    char * iaddr, int iaddrlen,
                                    char * desc, int desclen
                                                                        ,long int *exprie_time
                                                                        )
{
        /*char ifname[IFNAMSIZ];*/
        int proto = 0;

        DEBUG_MSG("\n%s (%d): begin, index_local = %d\n",__FUNCTION__,__LINE__,index_local);

        if(desc && (desclen > 0))
                desc[0] = '\0';

        if(get_redirect_rule_by_index(index_local, 0/*ifname*/, eport, iaddr, iaddrlen,
                                      iport, &proto, desc, desclen, 0, 0, exprie_time) < 0){
                DEBUG_MSG("%s (%d): get_redirect_rule_by_index() failed\n",__FUNCTION__,__LINE__);
                return -1;
        }
        else
        {
                if(proto == IPPROTO_TCP)
                        memcpy(protocol, "TCP", 4);
                else
                        memcpy(protocol, "UDP", 4);
                return 0;
        }
        DEBUG_MSG("%s (%d): end",__FUNCTION__,__LINE__);
}

int
_upnp_delete_redir(unsigned short eport, int proto)
{
        int r;
        DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
#if defined(__linux__)
        DEBUG_MSG("%s (%d): go delete_redirect_and_filter_rules()\n",__FUNCTION__,__LINE__);
        r = delete_redirect_and_filter_rules(eport, proto);
        DEBUG_MSG("%s (%d): delete_redirect_and_filter_rules() return r = %d\n",__FUNCTION__,__LINE__,r);
        /*      Date:   2009-04-20
        *       Name:   jimmy huang
        *       Reason:
        *       Note:   move out from delete_redirect_and_filter_rules()
        */
        del_redirect_desc(eport, proto);
        // ----------
#else
        r = delete_redirect_rule(ext_if_name, eport, proto);
        delete_filter_rule(ext_if_name, eport, proto);
#endif
#ifdef ENABLE_LEASEFILE
        DEBUG_MSG("%s (%d): go lease_file_remove()\n",__FUNCTION__,__LINE__);
        lease_file_remove( eport, proto);
#endif

#ifdef ENABLE_EVENTS
        upnp_event_var_change_notify(EWanIPC);
#endif
        DEBUG_MSG("%s (%d): end\n",__FUNCTION__,__LINE__);
        return r;
}

int
upnp_delete_redirection(unsigned short eport, const char * protocol)
{
        syslog(LOG_INFO, "removing redirect rule port %hu %s", eport, protocol);
        return _upnp_delete_redir(eport, proto_atoi(protocol));
}

/* upnp_get_portmapping_number_of_entries()
 * TODO: improve this code */
int
upnp_get_portmapping_number_of_entries()
{
/*      Date:   2009-04-29
*       Name:   jimmy huang
*       Reason: To reduce the counting time when control point
*                       ask our numbers of PortMapping rules
*                       We use get_redirect_rule_nums() instead of original implements
*       Note:   see the TODO above ? It's author's note
*/
/*
        int n = 0, r = 0;
        unsigned short eport, iport;
        char protocol[4], iaddr[32], desc[64];
        long int expire_time = 0;
        DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);

        do {
                protocol[0] = '\0'; iaddr[0] = '\0'; desc[0] = '\0';
                r = upnp_get_redirection_infos_by_index(n, &eport, protocol, &iport,
                                                        iaddr, sizeof(iaddr),
                                                        desc, sizeof(desc) , &expire_time);
                n++;
        } while(r==0);

        DEBUG_MSG("%s (%d): end, we have %d rules\n",__FUNCTION__,__LINE__,n-1);
        return (n-1);
*/
        return get_redirect_rule_nums();
}

/* functions used to remove unused rules */
struct rule_state *
get_upnp_rules_state_list(int max_rules_number_target)
{
        char ifname[IFNAMSIZ];
        int proto;
        unsigned short iport;
        struct rule_state * tmp;
        struct rule_state * list = 0;
        int i = 0;
        long int expire_time = 0;
        DEBUG_MSG("\n%s, %s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);
        tmp = malloc(sizeof(struct rule_state));
        if(!tmp)
                return 0;
        DEBUG_MSG("%s (%d): call get_redirect_rule_by_index()\n",__FUNCTION__,__LINE__);
        while(get_redirect_rule_by_index(i, ifname, &tmp->eport, 0, 0,
                                      &iport, &proto, 0, 0,
                                                                  &tmp->packets, &tmp->bytes, &expire_time) >= 0)
        {
                tmp->proto = (short)proto;
                /* add tmp to list */
                tmp->next = list;
                list = tmp;
                /* prepare next iteration */
                i++;
                tmp = malloc(sizeof(struct rule_state));
                if(!tmp)
                        break;
        }
        free(tmp);
        /* return empty list if not enough redirections */
        if(i<=max_rules_number_target)
                while(list)
                {
                        tmp = list;
                        list = tmp->next;
                        free(tmp);
                }
        /* return list */
        return list;
}

void
remove_unused_rules(struct rule_state * list)
{
        char ifname[IFNAMSIZ];
        unsigned short iport;
        struct rule_state * tmp;
        u_int64_t packets;
        u_int64_t bytes;
        int n = 0;
        long int expire_time = 0;

        DEBUG_MSG("%s (%d): begin\n",__FUNCTION__,__LINE__);

        while(list)
        {
                /* remove the rule if no traffic has used it */
                if(get_redirect_rule(ifname, list->eport, list->proto,
                                 0, 0, &iport, 0, 0, &packets, &bytes, &expire_time) >= 0)
                {
                        if(packets == list->packets && bytes == list->bytes)
                        {
                                _upnp_delete_redir(list->eport, list->proto);
                                n++;
                        }
                }
                tmp = list;
                list = tmp->next;
                free(tmp);
        }
        if(n>0)
                syslog(LOG_NOTICE, "removed %d unused rules", n);
}


/* stuff for miniupnpdctl */
#ifdef USE_MINIUPNPDCTL
void
write_ruleset_details(int s)
{
        char ifname[IFNAMSIZ];
        int proto = 0;
        unsigned short eport, iport;
        char desc[64];
        char iaddr[32];
        u_int64_t packets;
        u_int64_t bytes;
        int i = 0;
        char buffer[256];
        int n;
        long int expire_time = 0;
        DEBUG_MSG("\n%s, %s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);

        while(get_redirect_rule_by_index(i, ifname, &eport, iaddr, sizeof(iaddr),
                                         &iport, &proto, desc, sizeof(desc),
                                         &packets, &bytes, &expire_time) >= 0)
        {
                n = snprintf(buffer, sizeof(buffer), "%2d %s %s %hu->%s:%hu "
                                                     "'%s' %llu %llu\n",
                             i, ifname, proto==IPPROTO_TCP?"TCP":"UDP",
                             eport, iaddr, iport, desc, packets, bytes);
                write(s, buffer, n);
                i++;
        }
}
#endif

