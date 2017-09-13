/* $Id: miniupnpd.c,v 1.5 2009/07/08 11:18:33 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2008 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <syslog.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <sys/param.h>
#if defined(sun)
#include <kstat.h>
#else
/* for BSD's sysctl */
#include <sys/sysctl.h>
#endif

/* unix sockets */
#include "config.h"
#ifdef USE_MINIUPNPDCTL
#include <sys/un.h>
#endif

#include "upnpglobalvars.h"
#include "upnphttp.h"
#include "upnpdescgen.h"
#include "miniupnpdpath.h"
#include "getifaddr.h"
#include "upnpsoap.h"
#include "options.h"
#include "minissdp.h"
#include "upnpredirect.h"
#include "miniupnpdtypes.h"
#include "daemonize.h"
#include "upnpevents.h"
#ifdef ENABLE_NATPMP
#include "natpmp.h"
#endif
#include "commonrdr.h"

#include <sys/signal.h>
#include <errno.h>
#include "nvram_funs.h"

#ifdef MINIUPNPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#ifdef USE_MINIUPNPDCTL
struct ctlelem {
        int socket;
        LIST_ENTRY(ctlelem) entries;
};
#endif

/* MAX_LAN_ADDR : maximum number of interfaces
 * to listen to SSDP traffic */
/*#define MAX_LAN_ADDR (4)*/

static volatile int quitting = 0;

/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: For wps enable / disable
*/
extern char *known_service_types[];


/* OpenAndConfHTTPSocket() :
 * setup the socket used to handle incoming HTTP connections. */
static int
OpenAndConfHTTPSocket(unsigned short port)
{
        int s;
        int i = 1;
        struct sockaddr_in listenname;

        if( (s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        {
                syslog(LOG_ERR, "socket(http): %m");
                return -1;
        }

        if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0)
        {
                syslog(LOG_WARNING, "setsockopt(http, SO_REUSEADDR): %m");
        }

        memset(&listenname, 0, sizeof(struct sockaddr_in));
        listenname.sin_family = AF_INET;
        listenname.sin_port = htons(port);
        listenname.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(s, (struct sockaddr *)&listenname, sizeof(struct sockaddr_in)) < 0)
        {
                syslog(LOG_ERR, "bind(http): %m");
                close(s);
                return -1;
        }

        if(listen(s, 6) < 0)
        {
                syslog(LOG_ERR, "listen(http): %m");
                close(s);
                return -1;
        }

        return s;
}

/* Functions used to communicate with miniupnpdctl */
#ifdef USE_MINIUPNPDCTL
static int
OpenAndConfCtlUnixSocket(const char * path)
{
        struct sockaddr_un localun;
        int s;
        s = socket(AF_UNIX, SOCK_STREAM, 0);
        localun.sun_family = AF_UNIX;
        strncpy(localun.sun_path, path,
                  sizeof(localun.sun_path));
        if(bind(s, (struct sockaddr *)&localun,
                sizeof(struct sockaddr_un)) < 0)
        {
                syslog(LOG_ERR, "bind(sctl): %m");
                close(s);
                s = -1;
        }
        else if(listen(s, 5) < 0)
        {
                syslog(LOG_ERR, "listen(sctl): %m");
                close(s);
                s = -1;
        }
        return s;
}

static void
write_upnphttp_details(int fd, struct upnphttp * e)
{
        char buffer[256];
        int len;
        while(e)
        {
                len = snprintf(buffer, sizeof(buffer),
                               "%d %d %s req_buf=%p(%dbytes) res_buf=%p(%dbytes alloc)\n",
                               e->socket, e->state, e->HttpVer,
                               e->req_buf, e->req_buflen,
                               e->res_buf, e->res_buf_alloclen);
                write(fd, buffer, len);
                e = e->entries.le_next;
        }
}

static void
write_ctlsockets_list(int fd, struct ctlelem * e)
{
        char buffer[256];
        int len;
        while(e)
        {
                len = snprintf(buffer, sizeof(buffer),
                               "struct ctlelem: socket=%d\n", e->socket);
                write(fd, buffer, len);
                e = e->entries.le_next;
        }
}

static void
write_option_list(int fd)
{
        char buffer[256];
        int len;
        int i;
        for(i=0; i<num_options; i++)
        {
                len = snprintf(buffer, sizeof(buffer),
                               "opt=%02d %s\n",
                               ary_options[i].id, ary_options[i].value);
                write(fd, buffer, len);
        }
}

#endif

/* Handler for the SIGTERM signal (kill)
 * SIGINT is also handled */
static void
sigterm(int sig)
{
        /*int save_errno = errno;*/
        signal(sig, SIG_IGN);   /* Ignore this signal while we are quitting */

        syslog(LOG_NOTICE, "received signal %d, good-bye", sig);

        quitting = 1;
        /*errno = save_errno;*/
}

/* record the startup time, for returning uptime */
static void
set_startup_time(int sysuptime)
{
        startup_time = time(NULL);
        if(sysuptime)
        {
                /* use system uptime instead of daemon uptime */
#if defined(__linux__)
                char buff[64];
		int uptime = 0, fd;
                fd = open("/proc/uptime", O_RDONLY);
                if(fd < 0)
                {
                        syslog(LOG_ERR, "open(\"/proc/uptime\" : %m");
                }
                else
                {
                        memset(buff, 0, sizeof(buff));
			if(read(fd, buff, sizeof(buff) - 1) < 0){
				syslog(LOG_ERR, "read(\"/proc/uptime\" : %m");
			}else{
				uptime = atoi(buff);
				syslog(LOG_INFO, "system uptime is %d seconds", uptime);
			}
                        close(fd);
                        startup_time -= uptime;
                }
#elif defined(SOLARIS_KSTATS)
                kstat_ctl_t *kc;
                kc = kstat_open();
                if(kc != NULL)
                {
                        kstat_t *ksp;
                        ksp = kstat_lookup(kc, "unix", 0, "system_misc");
                        if(ksp && (kstat_read(kc, ksp, NULL) != -1))
                        {
                                void *ptr = kstat_data_lookup(ksp, "boot_time");
                                if(ptr)
                                        memcpy(&startup_time, ptr, sizeof(startup_time));
				else
                                        syslog(LOG_ERR, "cannot find boot_time kstat");
                                }
			else
                                syslog(LOG_ERR, "cannot open kstats for unix/0/system_misc: %m");
                        kstat_close(kc);
                }
#else
                struct timeval boottime;
                size_t size = sizeof(boottime);
                int name[2] = { CTL_KERN, KERN_BOOTTIME };
                if(sysctl(name, 2, &boottime, &size, NULL, 0) < 0)
                {
                        syslog(LOG_ERR, "sysctl(\"kern.boottime\") failed");
                }
                else
                {
                        startup_time = boottime.tv_sec;
                }
#endif
        }
}

/* structure containing variables used during "main loop"
 * that are filled during the init */
struct runtime_vars {
        /* LAN IP addresses for SSDP traffic and HTTP */
        /* moved to global vars */
        /*int n_lan_addr;*/
        /*struct lan_addr_s lan_addr[MAX_LAN_ADDR];*/
        int port;       /* HTTP Port */
        int notify_interval;    /* seconds between SSDP announces */
        /* unused rules cleaning related variables : */
        int clean_ruleset_threshold;    /* threshold for removing unused rules */
        int clean_ruleset_interval;             /* (minimum) interval between checks */
};

/*      Date:   2009-02-27
*       Name:   jimmy huang
*       Reason: for Vista Logo, win 7, dtm 1.3
*                       miniupnpd must tell each upnp client that we have WFA Device service
*                       only when the daemon (wsccmd) who take care of WFA Device service
*                       is activated on the serive port, so that upnp client will show up
*                       WFA Device icon
*       Note:   1.      This function is used to check if wsccmd is already activated or not
*                               If any project is using different daemon to handle WFA Device service
*                               The definition WFADAEMON should be modified
*                       2.      Add flags " WPS_WITH_WSCCMD " to decide if we need to check
*                               deamon "wsccmd"'s status
*                               This flag is added in Makefile, and depends on flag "CONFIG_USER_WL_ATH"
*                       3.      Added the function below
*                       4.      WPS_WITH_WSCCMD is export in Makefile
*/
int is_WFAdevice_daemon_ready(void){

#ifdef WPS_WITH_WSCCMD
#define WFADAEMON "hostapd"
#define WFADAEMON_NUM 0
#define WFAFAEMON_CHECK "/tmp/wfa_daemon_check"
        char cmd[100];
        int result = 0;
        int count = 0;
        FILE *fp;
        unlink(WFAFAEMON_CHECK);
        sprintf(cmd,"ps |grep %s |grep -v grep > %s",WFADAEMON,WFAFAEMON_CHECK);
        system(cmd);

        //DEBUG_MSG("%s (%d): checking is_WFAdevice_daemon_ready \n",__FUNCTION__,__LINE__);

        if((fp = fopen(WFAFAEMON_CHECK,"r"))!=NULL){
                while(!feof(fp)){
                        memset(cmd,'\0',sizeof(cmd));
                        fgets(cmd,sizeof(cmd),fp);
                        if(cmd[0]!='\0' && cmd[0]!='\n' && (strstr(cmd,WFADAEMON)!=NULL) ){
                                count++;
                        }
                }
                fclose(fp);
		
		/* Because wsccmd will sequentially fork 11 process, WFA service is about on 6th process */
		if(count > WFADAEMON_NUM){
                        DEBUG_MSG("%s (%d): WFAdevice daemon %s is ready\n",__FUNCTION__,__LINE__,WFADAEMON);
                        return 1;
                }
        }
        //DEBUG_MSG("%s (%d): WFAdevice daemon %s is not ready\n",__FUNCTION__,__LINE__,WFADAEMON);
        return result;
#else
        return 1;
#endif
}

/* parselanaddr()
 * parse address with mask
 * ex: 192.168.1.1/24
 * return value :
 *    0 : ok
 *   -1 : error */
static int
parselanaddr(struct lan_addr_s * lan_addr_local, const char * str)
{
        const char * p;
        int nbits = 24;
        int n;
        p = str;
        while(*p && *p != '/' && !isspace(*p))
                p++;
        n = p - str;
        if(*p == '/')
        {
                nbits = atoi(++p);
                while(*p && !isspace(*p))
                        p++;
        }
        if(n>15)
        {
                fprintf(stderr, "Error parsing address/mask : %s\n", str);
                return -1;
        }
        memcpy(lan_addr_local->str, str, n);
        lan_addr_local->str[n] = '\0';
        if(!inet_aton(lan_addr_local->str, &lan_addr_local->addr))
        {
                fprintf(stderr, "Error parsing address/mask : %s\n", str);
                return -1;
        }
        lan_addr_local->mask.s_addr = htonl(nbits ? (0xffffffff << (32 - nbits)) : 0);
#ifdef MULTIPLE_EXTERNAL_IP
        while(*p && isspace(*p))
                p++;
        if(*p) {
                n = 0;
                while(p[n] && !isspace(*p))
                        n++;
                if(n<=15) {
                        memcpy(lan_addr_local->ext_ip_str, p, n);
                        lan_addr_local->ext_ip_str[n] = '\0';
                        if(!inet_aton(lan_addr_local->ext_ip_str, &lan_addr_local->ext_ip_addr)) {
                                /* error */
                                fprintf(stderr, "Error parsing address : %s\n", lan_addr_local->ext_ip_str);
                        }
                }
        }
#endif
        return 0;
}

/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: For generate uuid
*/
int gen_uuid(void){
        char cmd[128]={0};
        FILE *fp = NULL;
        // we generate new uuid file "only" when /var/tmp/uuid_file not exist
        // if we generate each time when rc start, if may due to subscribe/unscribe failed
        // ex: subscribe with uuid 11111111-1111-1111-1111-111111111111
        // once we lose wan IP, get new wan ip, rc restart, gen new uuid 22222222-xxxx...
        // now, we can't handle all related events for old uuid 11111111-1111-1111-1111-111111111111

        if((fp = fopen(UUID_GEN_FILE, "r")) == NULL){
                sprintf(cmd,"uuidgen > %s ;  uuidgen >> %s ;  uuidgen >> %s ;  uuidgen >> %s"
                ,UUID_GEN_FILE,UUID_GEN_FILE,UUID_GEN_FILE,UUID_GEN_FILE);
                system(cmd);
        }else{
                fclose(fp);
        }
        //unlink(UUID_GEN_FILE);
        return 0;
}


/*  Date:   2009-03-31
*   Name:   Jackey Chen
*   Reason: Fixed UUID format
*   Note:   Not hard code in miniupnpd, it follows MircoSoft Spec
*/
int read_in_uuid(void)
{
        FILE *fp = NULL;

//1:InternetGatewayDevice
//2:WANDevice
//3:WANConnectionDevice
//4:WFADevice
        memset(uuidvalue_1,'\0',sizeof(uuidvalue_1));
        memset(uuidvalue_2,'\0',sizeof(uuidvalue_2));
        memset(uuidvalue_3,'\0',sizeof(uuidvalue_3));
        memset(uuidvalue_4,'\0',sizeof(uuidvalue_4));
        if((fp = fopen(UUID_GEN_FILE,"r"))!=NULL){              // /var/tmp/uuid_file
                fgets(uuidvalue_1,sizeof(uuidvalue_1),fp);
                //if(uuidvalue_1[0] == ' ')
                uuidvalue_1[strlen(uuidvalue_1) - 1] = '\0';
                fgets(uuidvalue_2,sizeof(uuidvalue_2),fp);
                if(strlen(uuidvalue_2) < 5){
                        strcpy(uuidvalue_2,uuidvalue);
                }else{
                        uuidvalue_2[strlen(uuidvalue_2) - 1] = '\0';
                }
                fgets(uuidvalue_3,sizeof(uuidvalue_3),fp);
                if(strlen(uuidvalue_3) < 5){
                        strcpy(uuidvalue_3,uuidvalue);
                }else{
                        uuidvalue_3[strlen(uuidvalue_3) - 1] = '\0';
                }
                fgets(uuidvalue_4,sizeof(uuidvalue_4),fp);
                if(strlen(uuidvalue_4) < 5){
                        strcpy(uuidvalue_4,uuidvalue);
                }else{
                        uuidvalue_4[strlen(uuidvalue_4) - 1] = '\0';
                }
                /*      Date:   2009-05-12
                *       Name:   jimmy huang
                *       Reason: fixed bug here, forget to fclose(fp)
                */
                fclose(fp);
        }
        else{
                DEBUG_MSG("%s (%d): %s not exist, using default uuidvalue \n",__FUNCTION__,__LINE__,UUID_GEN_FILE);
        }
        DEBUG_MSG("%s (%d): uuidvalue_1 = %s\n",__FUNCTION__,__LINE__,uuidvalue_1);
        DEBUG_MSG("%s (%d): uuidvalue_2 = %s\n",__FUNCTION__,__LINE__,uuidvalue_2);
        DEBUG_MSG("%s (%d): uuidvalue_3 = %s\n",__FUNCTION__,__LINE__,uuidvalue_3);
        DEBUG_MSG("%s (%d): uuidvalue_4 = %s\n",__FUNCTION__,__LINE__,uuidvalue_4);
        /*      Date:   2009-05-12
        *       Name:   jimmy huang
        *       Reason: For debug
        */
        {
                fp = fopen("/var/tmp/upnp_uuid_info","w");
                if(fp){
                        fprintf(fp,"%s: InternetGatewayDevice\n",uuidvalue_1);
                        fprintf(fp,"%s: WANDevice\n",uuidvalue_2);
                        fprintf(fp,"%s: WANConnectionDevice\n",uuidvalue_3);
                        fprintf(fp,"%s: WFADevice\n",uuidvalue_4);
                        fclose(fp);
                }
        }

        return 0;
}
// ----------

/* init phase :
 * 1) read configuration file
 * 2) read command line arguments
 * 3) daemonize
 * 4) open syslog
 * 5) check and write pid file
 * 6) set startup time stamp
 * 7) compute presentation URL
 * 8) set signal handlers */

static int
init(int argc, char * * argv, struct runtime_vars * v)
{
        int i;
        int pid;
        int debug_flag = 0;
        int options_flag = 0;
        int openlog_option;
        struct sigaction sa;
        /*const char * logfilename = 0;*/
        const char * presurl = 0;
        //const char * optionsfile = "/etc/miniupnpd.conf";
        const char * optionsfile = MINIUPNPD_CONF;
#ifdef ENABLE_LEASEFILE
        lease_file = MINIUPNPD_LEASE_FILE; // defined in config.h
#endif
        // ------------------------------------

        DEBUG_MSG("%s (%d): begin\n",__FUNCTION__,__LINE__);
#ifdef ENABLE_LEASEFILE
        DEBUG_MSG("%s (%d): lease_file = %s\n",__FUNCTION__,__LINE__,lease_file);
#endif

        /* first check if "-f" option is used */
        for(i=2; i<argc; i++)
        {
                if(0 == strcmp(argv[i-1], "-f"))
                {
                        optionsfile = argv[i];
                        options_flag = 1;
                        break;
                }
        }

        DEBUG_MSG("%s (%d): set initial variables\n",__FUNCTION__,__LINE__);

        /* set initial values */
        SETFLAG(ENABLEUPNPMASK);

        /*v->n_lan_addr = 0;*/
        v->port = -1;
        v->notify_interval = 30;        /* seconds between SSDP announces */
        v->clean_ruleset_threshold = 20;
        v->clean_ruleset_interval = 0;  /* interval between ruleset check. 0=disabled */

        DEBUG_MSG("%s (%d): set notify_interval = 30 sec\n",__FUNCTION__,__LINE__);

        DEBUG_MSG("%s (%d): read in configuration from %s\n",__FUNCTION__,__LINE__,optionsfile);
        /* read options file first since
         * command line arguments have final say */
        if(readoptionsfile(optionsfile) < 0)
        {
                DEBUG_MSG("%s (%d): read in configuration failed\n",__FUNCTION__,__LINE__);
                /* only error if file exists or using -f */
                if(access(optionsfile, F_OK) == 0 || options_flag)
                        fprintf(stderr, "Error reading configuration file %s\n", optionsfile);
                DEBUG_MSG("%s (%d): can't access configuration %s\n",__FUNCTION__,__LINE__,optionsfile);
        }
        else
        {
                for(i=0; i<num_options; i++)
                {
                        switch(ary_options[i].id)
                        {
                        case UPNPEXT_IFNAME:
                                ext_if_name = ary_options[i].value;
                                DEBUG_MSG("%s (%d): ext_if_name = %s\n",__FUNCTION__,__LINE__,ext_if_name);
                                break;
                        /*      Date:   2009-07-08
                        *       Name:   jimmy huang
                        *       Reason: To add rules with "-o br0"
                        */
                        case UPNPINT_IFNAME:
                                int_if_name = ary_options[i].value;
                                DEBUG_MSG("%s (%d): int_if_name = %s\n",__FUNCTION__,__LINE__,int_if_name);
                                break;
                        case UPNPEXT_IP:
                                use_ext_ip_addr = ary_options[i].value;
                                DEBUG_MSG("%s (%d): use_ext_ip_addr = %s\n",__FUNCTION__,__LINE__,use_ext_ip_addr);
                                break;
                        case UPNPLISTENING_IP:
                                if(n_lan_addr < MAX_LAN_ADDR)/* if(v->n_lan_addr < MAX_LAN_ADDR)*/
                                {
                                        /*if(parselanaddr(&v->lan_addr[v->n_lan_addr],*/
                                        if(parselanaddr(&lan_addr[n_lan_addr],
                                                     ary_options[i].value) == 0)
                                                n_lan_addr++; /*v->n_lan_addr++; */
                                }
                                else
                                {
                                        fprintf(stderr, "Too many listening ips (max: %d), ignoring %s\n",
                                            MAX_LAN_ADDR, ary_options[i].value);
                                }
                                DEBUG_MSG("%s (%d): listening_ip = %s\n",__FUNCTION__,__LINE__,lan_addr[0].str);
                                break;
                        case UPNPPORT:
                                v->port = atoi(ary_options[i].value);
                                DEBUG_MSG("%s (%d): listening port = %d\n",__FUNCTION__,__LINE__,v->port);
                                break;
                        case UPNPBITRATE_UP:
                                upstream_bitrate = strtoul(ary_options[i].value, 0, 0);
                                DEBUG_MSG("%s (%d): upstream_bitrate = %lu\n",__FUNCTION__,__LINE__,upstream_bitrate);
                                break;
                        case UPNPBITRATE_DOWN:
                                downstream_bitrate = strtoul(ary_options[i].value, 0, 0);
                                DEBUG_MSG("%s (%d): downstream_bitrate = %lu\n",__FUNCTION__,__LINE__,downstream_bitrate);
                                break;
                        case UPNPPRESENTATIONURL:
                                presurl = ary_options[i].value;
                                break;
#ifdef USE_NETFILTER
                        case UPNPFORWARDCHAIN:
                                miniupnpd_forward_chain = ary_options[i].value;
                                break;
                        case UPNPNATCHAIN:
                                miniupnpd_nat_chain = ary_options[i].value;
                                break;
#endif
                        case UPNPNOTIFY_INTERVAL:
                                v->notify_interval = atoi(ary_options[i].value);
                                DEBUG_MSG("%s (%d): NOTIFY interval = %d\n",__FUNCTION__,__LINE__,v->notify_interval);
                                break;
                        case UPNPSYSTEM_UPTIME:
                                if(strcmp(ary_options[i].value, "yes") == 0){
                                        DEBUG_MSG("%s (%d): using system up time as UPTIME\n",__FUNCTION__,__LINE__);
                                        SETFLAG(SYSUPTIMEMASK); /*sysuptime = 1;*/
                                }else{
                                        DEBUG_MSG("%s (%d): using miniupnpd up time as UPTIME\n",__FUNCTION__,__LINE__);
                                }
                                break;
                        case UPNPPACKET_LOG:
                                if(strcmp(ary_options[i].value, "yes") == 0)
                                        SETFLAG(LOGPACKETSMASK);        /*logpackets = 1;*/
                                break;
                        case UPNPUUID:
                                strncpy(uuidvalue+5, ary_options[i].value,
                                        strlen(uuidvalue+5) + 1);
                                /*      Date:   2009-04-20
                                *       Name:   jimmy huang
                                *       Reason: For debug message
                                */
                                strcpy(uuidvalue_1,uuidvalue);
                                DEBUG_MSG("%s (%d): using %s as uuidvalue_1\n",__FUNCTION__,__LINE__,uuidvalue);
                                //------------
                                break;
                        case UPNPSERIAL:
                                strncpy(serialnumber, ary_options[i].value, SERIALNUMBER_MAX_LEN);
                                serialnumber[SERIALNUMBER_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): serialnumber = %s\n",__FUNCTION__,__LINE__,serialnumber);
                                break;
                        case UPNPMODEL_NUMBER:
                                strncpy(modelnumber, ary_options[i].value, MODELNUMBER_MAX_LEN);
                                modelnumber[MODELNUMBER_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): modelnumber = %s\n",__FUNCTION__,__LINE__,modelnumber);
                                break;
                        case UPNPCLEANTHRESHOLD:
                                v->clean_ruleset_threshold = atoi(ary_options[i].value);
                                break;
                        case UPNPCLEANINTERVAL:
                                v->clean_ruleset_interval = atoi(ary_options[i].value);
                                DEBUG_MSG("%s (%d): clean_ruleset_interval = %d\n",__FUNCTION__,__LINE__,v->clean_ruleset_interval);
                                break;
#ifdef USE_PF
                        case UPNPQUEUE:
                                queue = ary_options[i].value;
                                break;
                        case UPNPTAG:
                                tag = ary_options[i].value;
                                break;
#endif
#ifdef ENABLE_NATPMP
                        case UPNPENABLENATPMP:
                                if(strcmp(ary_options[i].value, "yes") == 0)
                                        SETFLAG(ENABLENATPMPMASK);      /*enablenatpmp = 1;*/
                                else
                                        if(atoi(ary_options[i].value))
                                                SETFLAG(ENABLENATPMPMASK);
                                        /*enablenatpmp = atoi(ary_options[i].value);*/
                                break;
#endif
#ifdef PF_ENABLE_FILTER_RULES
                        case UPNPQUICKRULES:
                                if(strcmp(ary_options[i].value, "no") == 0)
                                        SETFLAG(PFNOQUICKRULESMASK);
                                break;
#endif
                        case UPNPENABLE:
                                if(strcmp(ary_options[i].value, "yes") != 0)
                                        CLEARFLAG(ENABLEUPNPMASK);
                                DEBUG_MSG("%s (%d): enable_upnp = %s\n",__FUNCTION__,__LINE__,ary_options[i].value);
                                break;
                        case UPNPSECUREMODE:
                                if(strcmp(ary_options[i].value, "yes") == 0)
                                        SETFLAG(SECUREMODEMASK);
                                break;
#ifdef ENABLE_LEASEFILE
                        case UPNPLEASEFILE:
                                lease_file = ary_options[i].value;
                                DEBUG_MSG("%s (%d): lease_file = %s\n",__FUNCTION__,__LINE__,ary_options[i].value);
                                break;
#endif
                        case UPNPMINISSDPDSOCKET:
                                minissdpdsocketpath = ary_options[i].value;
                                break;
                /*      Date:   2009-04-20
                *       Name:   jimmy huang
                *       Reason: For wps enable and read in UPnP related variables from config
                */
                        case UPNPWPS_ENABLE:
                                strncpy(wpsenable, ary_options[i].value, WPSENABLE_MAX_LEN);
                                wpsenable[WPSENABLE_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): wpsenable = %s\n",__FUNCTION__,__LINE__,wpsenable);
                                break;
                        case UPNPFRIENDLY_NAME:
                                strncpy(friendlyname, ary_options[i].value, FRIENDLYNAME_MAX_LEN);
                                friendlyname[FRIENDLYNAME_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): friendlyname = %s\n",__FUNCTION__,__LINE__,friendlyname);
                                break;
                        case UPNPMANUFACTURER_NAME:
                                strncpy(manufacturername, ary_options[i].value, MANUFACTURERNAME_MAX_LEN);
                                manufacturername[FRIENDLYNAME_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): manufacturername = %s\n",__FUNCTION__,__LINE__,manufacturername);
                                break;
                        case UPNPMANUFACTURER_URL:
                                strncpy(manufacturerurl, ary_options[i].value, MANUFACTURERURL_MAX_LEN);
                                manufacturerurl[MANUFACTURERURL_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): manufacturerurl = %s\n",__FUNCTION__,__LINE__,manufacturerurl);
                                break;
                        case UPNPMODEL_NAME:
                                strncpy(modelname, ary_options[i].value, MODELNAME_MAX_LEN);
                                modelname[MODELNAME_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): modelname = %s\n",__FUNCTION__,__LINE__,modelname);
                                break;
                        case UPNPMODEL_URL:
                                strncpy(modelurl, ary_options[i].value, MODELURL_MAX_LEN);
                                modelurl[MODELURL_MAX_LEN-1] = '\0';
                                DEBUG_MSG("%s (%d): modelurl = %s\n",__FUNCTION__,__LINE__,modelurl);
                                break;
                //----------
                        default:
                                fprintf(stderr, "Unknown option in file %s\n",
                                        optionsfile);
                        }
                }
        }

        DEBUG_MSG("%s (%d): read in command line arguments\n",__FUNCTION__,__LINE__);

        /* command line arguments processing */
        for(i=1; i<argc; i++)
        {
                if(argv[i][0]!='-')
                {
                        fprintf(stderr, "Unknown option: %s\n", argv[i]);
                }
                else switch(argv[i][1])
                {
                case 'o':
                        if(i+1 < argc)
                                use_ext_ip_addr = argv[++i];
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 't':
                        if(i+1 < argc)
                                v->notify_interval = atoi(argv[++i]);
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 'u':
                        if(i+1 < argc)
                                strncpy(uuidvalue+5, argv[++i], strlen(uuidvalue+5) + 1);
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 's':
                        if(i+1 < argc)
                                strncpy(serialnumber, argv[++i], SERIALNUMBER_MAX_LEN);
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        serialnumber[SERIALNUMBER_MAX_LEN-1] = '\0';
                        break;
                case 'm':
                        if(i+1 < argc)
                                strncpy(modelnumber, argv[++i], MODELNUMBER_MAX_LEN);
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        modelnumber[MODELNUMBER_MAX_LEN-1] = '\0';
                        break;
#ifdef ENABLE_NATPMP
                case 'N':
                        /*enablenatpmp = 1;*/
                        SETFLAG(ENABLENATPMPMASK);
                        break;
#endif
                case 'U':
                        /*sysuptime = 1;*/
                        SETFLAG(SYSUPTIMEMASK);
                        break;
                /*case 'l':
                        logfilename = argv[++i];
                        break;*/
                case 'L':
                        /*logpackets = 1;*/
                        SETFLAG(LOGPACKETSMASK);
                        break;
                case 'S':
                        SETFLAG(SECUREMODEMASK);
                        break;
                case 'i':
                        if(i+1 < argc)
                                ext_if_name = argv[++i];
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
#ifdef USE_PF
                case 'q':
                        if(i+1 < argc)
                                queue = argv[++i];
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 'T':
                        if(i+1 < argc)
                                tag = argv[++i];
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
#endif
                case 'p':
                        if(i+1 < argc)
                                v->port = atoi(argv[++i]);
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 'P':
                        if(i+1 < argc)
                                pidfilename = argv[++i];
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 'd':
                        debug_flag = 1;
                        break;
                case 'w':
                        if(i+1 < argc)
                                presurl = argv[++i];
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 'B':
                        if(i+2<argc)
                        {
                                downstream_bitrate = strtoul(argv[++i], 0, 0);
                                upstream_bitrate = strtoul(argv[++i], 0, 0);
                        }
                        else
                                fprintf(stderr, "Option -%c takes two arguments.\n", argv[i][1]);
                        break;
                case 'a':
                        if(i+1 < argc)
                        {
                                int address_already_there = 0;
                                int j;
                                i++;
                                for(j=0; j<n_lan_addr; j++)/* for(j=0; j<v->n_lan_addr; j++)*/
                                {
                                        struct lan_addr_s tmpaddr;
                                        parselanaddr(&tmpaddr, argv[i]);
                                        /*if(0 == strcmp(v->lan_addr[j].str, tmpaddr.str))*/
                                        if(0 == strcmp(lan_addr[j].str, tmpaddr.str))
                                                address_already_there = 1;
                                }
                                if(address_already_there)
                                        break;
                                if(n_lan_addr < MAX_LAN_ADDR) /*if(v->n_lan_addr < MAX_LAN_ADDR)*/
                                {
                                        /*v->lan_addr[v->n_lan_addr++] = argv[i];*/
                                        /*if(parselanaddr(&v->lan_addr[v->n_lan_addr], argv[i]) == 0)*/
                                        if(parselanaddr(&lan_addr[n_lan_addr], argv[i]) == 0)
                                                n_lan_addr++; /*v->n_lan_addr++;*/
                                }
                                else
                                {
                                        fprintf(stderr, "Too many listening ips (max: %d), ignoring %s\n",
                                            MAX_LAN_ADDR, argv[i]);
                                }
                        }
                        else
                                fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                        break;
                case 'f':
                        i++;    /* discarding, the config file is already read */
                        break;
                default:
                        fprintf(stderr, "Unknown option: %s\n", argv[i]);
                }
        }

        DEBUG_MSG("%s (%d): \n",__FUNCTION__,__LINE__);

/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: Marked the because we want miniupnpd activate whatever wan has IP or Not
*/

        if(!ext_if_name || (/*v->*/n_lan_addr==0))
        {
                if(!ext_if_name){
                        fprintf(stderr," miniupnpd: error, ext_if_name is NULL !!!\n");
                }
                if(n_lan_addr==0){
                        fprintf(stderr," miniupnpd: error, n_lan_addr is 0 !!!\n");
                }
                fprintf(stderr, "Usage:\n\t"
                        "%s [-f config_file] [-i ext_ifname] [-o ext_ip]\n"
#ifndef ENABLE_NATPMP
                                "\t\t[-a listening_ip] [-p port] [-d] [-L] [-U] [-S]\n"
#else
                                "\t\t[-a listening_ip] [-p port] [-d] [-L] [-U] [-S] [-N]\n"
#endif
                                /*"[-l logfile] " not functionnal */
                                "\t\t[-u uuid] [-s serial] [-m model_number] \n"
                                "\t\t[-t notify_interval] [-P pid_filename]\n"
#ifdef USE_PF
                                "\t\t[-B down up] [-w url] [-q queue] [-T tag]\n"
#else
                                "\t\t[-B down up] [-w url]\n"
#endif
                        "\nNotes:\n\tThere can be one or several listening_ips.\n"
                        "\tNotify interval is in seconds. Default is 30 seconds.\n"
                                "\tDefault pid file is %s.\n"
                                "\tWith -d miniupnpd will run as a standard program.\n"
                                "\t-L sets packet log in pf and ipf on.\n"
                                "\t-S sets \"secure\" mode : clients can only add mappings to their own ip\n"
                                "\t-U causes miniupnpd to report system uptime instead "
                                "of daemon uptime.\n"
                                "\t-B sets bitrates reported by daemon in bits per second.\n"
                                "\t-w sets the presentation url. Default is http address on port 80\n"
#ifdef USE_PF
                                "\t-q sets the ALTQ queue in pf.\n"
                                "\t-T sets the tag name in pf.\n"
#endif
                        "", argv[0], pidfilename);
                return 1;
        }


        if(debug_flag)
        {
                pid = getpid();
        }
        else
        {

#ifdef USE_DAEMON
                DEBUG_MSG("%s (%d): go daemon(0,0)\n",__FUNCTION__,__LINE__);
                if(daemon(0, 0)<0) {
                        perror("daemon()");
                }
                pid = getpid();
#else
                DEBUG_MSG("%s (%d): go daemonize()\n",__FUNCTION__,__LINE__);
                pid = daemonize();
#endif
        }

        openlog_option = LOG_PID|LOG_CONS;
        if(debug_flag)
        {
                openlog_option |= LOG_PERROR;   /* also log on stderr */
        }

        openlog("miniupnpd", openlog_option, LOG_MINIUPNPD);

        if(!debug_flag)
        {
                /* speed things up and ignore LOG_INFO and LOG_DEBUG */
                setlogmask(LOG_UPTO(LOG_NOTICE));
        }

        if(checkforrunning(pidfilename) < 0)
        {
                syslog(LOG_ERR, "MiniUPnPd is already running. EXITING");
                return 1;
        }
        set_startup_time(GETFLAG(SYSUPTIMEMASK)/*sysuptime*/);

        /* Chun add for WPS-COMPATIBLE */
        /* use different xml file according to if wps enable or not */
        if( !strcmp(wpsenable, "1")  ) {
		if(debug_flag)syslog(LOG_ERR, "wps_enable ");
                /*      Date:   2009-02-27
                *       Name:   jimmy huang
                *       Reason: for Vista Logo, win 7, dtm 1.3
                *                       miniupnpd must tell each upnp client that we have WFA Device service
                *                       only when the daemon (wsccmd) who take care of WFA Device service
                *                       is activated on the serive port, so that upnp client will show up
                *                       WFA Device icon
                *       Note:   Modified the codes below
                */
        /*
                known_service_types[8]="urn:schemas-wifialliance-org:device:WFADevice:";
                known_service_types[9]="urn:schemas-wifialliance-org:service:WFAWLANConfig:";
        */
                if(is_WFAdevice_daemon_ready()){//wsccmd_ready_flag
			if(debug_flag)syslog(LOG_ERR, "wsccmd is ready ... ");
                        known_service_types[WFA_DEV_KNOWN_SERVICE_INDEX]="urn:schemas-wifialliance-org:device:WFADevice:";
                        known_service_types[WFA_SRV_KNOWN_SERVICE_INDEX]="urn:schemas-wifialliance-org:service:WFAWLANConfig:";
                }else{
			if(debug_flag)syslog(LOG_ERR, "wsccmd is not ready !!! ");
                }

                wps_enable_set(1);
        }
        else
        {
		if(debug_flag) syslog(LOG_ERR, "wps_disable");
                wps_enable_set(0);
        }

/* Chun add for WFA-XML-IP */
        if(lan_addr[0].str)
        {
                sprintf(wfadev_path, "http://%s:60001/WFAWLANConfig/scpd.xml", lan_addr[0].str);
                sprintf(wfadev_control, "http://%s:60001/WFAWLANConfig/control", lan_addr[0].str);
                sprintf(wfadev_eventual, "http://%s:60001/WFAWLANConfig/event", lan_addr[0].str);
        }
/*Chun end*/

        /* presentation url */
        if(presurl)
        {
                strncpy(presentationurl, presurl, PRESENTATIONURL_MAX_LEN);
                presentationurl[PRESENTATIONURL_MAX_LEN-1] = '\0';

        }
        else
        {
                snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
                        /*      Date:   2009-04-20
                        *       Name:   jimmy huang
                        *       Reason: For control point can access our web
                        */
                         //"http://%s/", lan_addr[0].str);
                                 "http://%s:80/", lan_addr[0].str);
                        //-----------
                         /*"http://%s:%d/", lan_addr[0].str, 80);*/
                         /*"http://%s:%d/", v->lan_addr[0].str, 80);*/
        }
        /* set signal handler */
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = sigterm;

        if (sigaction(SIGTERM, &sa, NULL))
        {
                syslog(LOG_ERR, "Failed to set %s handler. EXITING", "SIGTERM");
                return 1;
        }
        if (sigaction(SIGINT, &sa, NULL))
        {
                syslog(LOG_ERR, "Failed to set %s handler. EXITING", "SIGINT");
                return 1;
        }

        if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
                syslog(LOG_ERR, "Failed to ignore SIGPIPE signals");
        }

        if(init_redirect() < 0)
        {
                syslog(LOG_ERR, "Failed to init redirection engine. EXITING");
                return 1;
        }

        writepidfile(pidfilename, pid);

#ifdef ENABLE_LEASEFILE
/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: to fixed the bug, When rc restart, firewall restart, miniupnpd not
*                       re-add previous rules, cause firewall will clean all rules first
*                       in rc, start_app() => start_firewall()
*                       wait 5 sec to relad rules after rc firewall finished
*/
        //sleep(5);
        // -----------
        /*remove(lease_file);*/
        syslog(LOG_INFO, "miniupnpd: Reloading port mapping rules from lease file");
        fprintf(stderr,"miniupnpd: Reloading port forward mapping rules from lease file\n");
        reload_from_lease_file(0);
#endif
        DEBUG_MSG("%s (%d): End\n",__FUNCTION__,__LINE__);

        return 0;
}

/*	Date:	2010-09-17
*	Name:	jimmy huang
*	Reason:	To fixed the bug, When rc ask hostpad restart, hostapd will lose all subscription records
*			Thus we need to tell all control points that WFA services are dead
*			Then when we detect hostpad is alive again, we will send NOTIFY to tell all CPs
*			that WFA services are ready, CPs will subscribe information with hostapd again
*			So move variable snotify to blobal
*/
int snotify[MAX_LAN_ADDR];

/* === main === */
/* process HTTP or SSDP requests */
int
main(int argc, char * * argv)
{
        int i;
        int sudp = -1, shttpl = -1;
#ifdef ENABLE_NATPMP
        int snatpmp = -1;
#endif
//	int snotify[MAX_LAN_ADDR];
        LIST_HEAD(httplisthead, upnphttp) upnphttphead;
        struct upnphttp * e = 0;
        struct upnphttp * next;
        fd_set readset; /* for select() */
#ifdef ENABLE_EVENTS
        fd_set writeset;
#endif
        struct timeval timeout, timeofday, lasttimeofday = {0, 0};
        int max_fd = -1;
#ifdef USE_MINIUPNPDCTL
        int sctl = -1;
        LIST_HEAD(ctlstructhead, ctlelem) ctllisthead;
        struct ctlelem * ectl;
        struct ctlelem * ectlnext;
#endif
        struct runtime_vars v;
        /* variables used for the unused-rule cleanup process */
        struct rule_state * rule_list = 0;
        struct timeval checktime = {0, 0};

        DEBUG_MSG("%s (%d): begin\n",__FUNCTION__,__LINE__);

/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: To generate / Set uuid we used
*/
        gen_uuid();
        read_in_uuid();


        if(init(argc, argv, &v) != 0){
                DEBUG_MSG("%s (%d): Initial failed !\n",__FUNCTION__,__LINE__);
                return 1;
        }

/*      Date:   2009-06-09
*       Name:   jimmy huang
*       Reason: Avoid miniupnpd heavy loading cause not response some UPnP soap
*                       We decrease the frequency for open nvram.conf
*                       we update vs rule in nvram only when
*                       1. first start
*                       2. receive signal from httpd
*/
#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
        global_vs_rule_num = 0;
        init_vs_struct();
        update_nvram_vs_rules();
#endif


        LIST_INIT(&upnphttphead);
#ifdef USE_MINIUPNPDCTL
        LIST_INIT(&ctllisthead);
#endif

        if(
#ifdef ENABLE_NATPMP
        (!(GETFLAG(ENABLENATPMPMASK))) &&
#endif
        (!(GETFLAG(ENABLEUPNPMASK))) ) {
                syslog(LOG_ERR, "Why did you run me anyway?");
                return 0;
        }

        if(GETFLAG(ENABLEUPNPMASK)/*enableupnp*/)
        {

                /* open socket for HTTP connections. Listen on the 1st LAN address */
                shttpl = OpenAndConfHTTPSocket((v.port > 0) ? v.port : 0);
                if(shttpl < 0)
                {
                        syslog(LOG_ERR, "Failed to open socket for HTTP. EXITING");
                        return 1;
                }
                if(v.port <= 0) {
                        struct sockaddr_in sockinfo;
                        socklen_t len = sizeof(struct sockaddr_in);
                        if (getsockname(shttpl, (struct sockaddr *)&sockinfo, &len) < 0) {
                                syslog(LOG_ERR, "getsockname(): %m");
                                return 1;
                        }
                        v.port = ntohs(sockinfo.sin_port);
                }
                syslog(LOG_NOTICE, "HTTP listening on port %d", v.port);

                /* open socket for SSDP connections */
                //n_lan_addr, lan_addr are global seen, declared in upnpglobalvars.c
                //sudp = OpenAndConfSSDPReceiveSocket(n_lan_addr, lan_addr);
                sudp = OpenAndConfSSDPReceiveSocket();
                if(sudp < 0)
                {
                        /*syslog(LOG_ERR, "Failed to open socket for receiving SSDP. EXITING");
                        return 1;*/
                        syslog(LOG_INFO, "Failed to open socket for receiving SSDP. Trying to use MiniSSDPd");
                        if(SubmitServicesToMiniSSDPD(lan_addr[0].str, v.port) < 0) {
                                syslog(LOG_ERR, "Failed to connect to MiniSSDPd. EXITING");
                                return 1;
                        }
                }

                /* open socket for sending notifications */
                if(OpenAndConfSSDPNotifySockets(snotify) < 0)
                {
                        syslog(LOG_ERR, "Failed to open sockets for sending SSDP notify "
                                "messages. EXITING");
                        return 1;
                }
        }

#ifdef ENABLE_NATPMP
        /* open socket for NAT PMP traffic */
        if(GETFLAG(ENABLENATPMPMASK))
        {
                snatpmp = OpenAndConfNATPMPSocket();
                if(snatpmp < 0)
                {
                        syslog(LOG_ERR, "Failed to open socket for NAT PMP.");
                        /*syslog(LOG_ERR, "Failed to open socket for NAT PMP. EXITING");
                        return 1;*/
                } else {
                        syslog(LOG_NOTICE, "Listening for NAT-PMP traffic on port %u",
                               NATPMP_PORT);
                }
                ScanNATPMPforExpiration();
        }
#endif

        /* for miniupnpdctl */
#ifdef USE_MINIUPNPDCTL
        sctl = OpenAndConfCtlUnixSocket("/var/run/miniupnpd.ctl");
#endif

#ifdef ENABLE_LEASEFILE
/*      Date:   2009-04-20
*       Name:   jimmy huang
*       Reason: to fixed the bug, When rc restart, firewall restart, miniupnpd not
*                       re-add previous rules, cause firewall will clean all rules first
*                       in rc, start_app() => start_firewall()
*                       Now, we let rc to send signal to miniupnpd to reload rules
*/
        signal(SIGUSR1, (void *) reload_from_lease_file);
#else
        // ignore it
        signal(SIGUSR1, SIG_IGN);
#endif

	/*	Date:	2010-09-17
	*	Name:	jimmy huang
	*	Reason:	To fixed the bug, When rc ask hostpad restart, hostapd will lose all subscription records
	*			Thus we need to tell all control points that WFA services are dead
	*			Then when we detect hostpad is alive again, we will send NOTIFY to tell all CPs
	*			that WFA services are ready, CPs will subscribe information with hostapd again
	*/
	signal(SIGTSTP, (void *) stop_wfa_service);


/*      Date:   2009-06-09
*       Name:   jimmy huang
*       Reason: Avoid miniupnpd heavy loading cause not response some UPnP soap
*                       We decrease the frequency for open nvram.conf
*                       we update vs rule in nvram only when
*                       1. first start
*                       2. receive signal from httpd
*/
#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
#ifdef CHECK_VS_NVRAM
        signal(SIGUSR2, (void *) update_nvram_vs_rules);
#endif
#else
        // ignore it
        signal(SIGUSR2, SIG_IGN);
#endif

        /* main loop */
        while(!quitting)
        {
                /*      Date:   2009-02-27
                *       Name:   jimmy huang
                *       Reason: for Vista Logo, win 7, dtm 1.3
                *                       miniupnpd must tell each upnp client that we have WFA Device service
                *                       only when the daemon (wsccmd) who take care of WFA Device service
                *                       is activated on the serive port, so that upnp client will show up
                *                       WFA Device icon
                *       Note:   Added the codes below
                */
                if(wsccmd_ready_flag == 0 && is_WFAdevice_daemon_ready()){//wsccmd_ready_flag
                        syslog(LOG_ERR, "wsccmd is ready (after init upnp stack) ... ");
                        known_service_types[WFA_DEV_KNOWN_SERVICE_INDEX]="urn:schemas-wifialliance-org:device:WFADevice:";
                        known_service_types[WFA_SRV_KNOWN_SERVICE_INDEX]="urn:schemas-wifialliance-org:service:WFAWLANConfig:";
                        wsccmd_ready_flag = 1;
                        //SendSSDPNotifies(snotify, listen_addr, (unsigned short)v.port);
                }

		/* Correct startup_time if it was set with a RTC close to 0 */
		// http://miniupnp.tuxfamily.org/forum/viewtopic.php?t=617
		if((startup_time<60*60*24) && (time(NULL)>60*60*24)){
			set_startup_time(GETFLAG(SYSUPTIMEMASK));
		}

                /* Check if we need to send SSDP NOTIFY messages and do it if
                 * needed */
                if(gettimeofday(&timeofday, 0) < 0)
                {
                        syslog(LOG_ERR, "gettimeofday(): %m");
			timeout.tv_sec = v.notify_interval;
                        timeout.tv_usec = 0;
                }
                else
                {
                        DEBUG_MSG("%s (%d): timeofday.tv_sec = %ld\n",__FUNCTION__,__LINE__,timeofday.tv_sec);
                        DEBUG_MSG("%s (%d): lasttimeofday.tv_sec (%ld) + v.notify_interval(%d) = %ld\n"
                                        ,__FUNCTION__,__LINE__
                                        ,lasttimeofday.tv_sec
                                        ,v.notify_interval
                                        ,lasttimeofday.tv_sec+v.notify_interval);
                        /*  Date:   2009-02-27
            *   Name:   jimmy huang
            *   Reason: for some reason, some special case (ex:on dhcp mode, wan is connected but no ip)
            *           after booting, lasttimeofday is bigger than timeofday,
            *           so miniupnpd won't sends Notify to upnp client for a
            *           period of time after booting
            *   Note:   Modified the codes below
            */
                        /* the comparaison is not very precise but who cares ? */
                        //if(timeofday.tv_sec >= (lasttimeofday.tv_sec + v.notify_interval))
                        if((timeofday.tv_sec >= (lasttimeofday.tv_sec + v.notify_interval))
                    || lasttimeofday.tv_sec > timeofday.tv_sec)
                        {
                                if (GETFLAG(ENABLEUPNPMASK)){
                                        DEBUG_MSG("%s (%d): go SendSSDPNotifies2()\n",__FUNCTION__,__LINE__);
                                        SendSSDPNotifies2(snotify,
                                                  (unsigned short)v.port,
                                                  v.notify_interval << 1);
                                }else{
                                        DEBUG_MSG("%s (%d): Do Not go SendSSDPNotifies2() !!!\n",__FUNCTION__,__LINE__);
                                }
                                memcpy(&lasttimeofday, &timeofday, sizeof(struct timeval));
				timeout.tv_sec = v.notify_interval;
                                timeout.tv_usec = 0;
                        }
                        else
                        {
                                timeout.tv_sec = lasttimeofday.tv_sec + v.notify_interval
                                                 - timeofday.tv_sec;
                                if(timeofday.tv_usec > lasttimeofday.tv_usec)
                                {
                                        timeout.tv_usec = 1000000 + lasttimeofday.tv_usec
                                                          - timeofday.tv_usec;
                                        timeout.tv_sec--;
                                }
                                else
                                {
                                        timeout.tv_usec = lasttimeofday.tv_usec - timeofday.tv_usec;
                                }
                        }
                }
                /* remove unused rules */
                if( v.clean_ruleset_interval
                  && (timeofday.tv_sec >= checktime.tv_sec + v.clean_ruleset_interval))
                {
                        DEBUG_MSG("%s (%d): checking useless rules \n",__FUNCTION__,__LINE__);
                        if(rule_list)
                        {
                                remove_unused_rules(rule_list);
                                rule_list = NULL;
                        }
                        else
                        {
                                rule_list = get_upnp_rules_state_list(v.clean_ruleset_threshold);
                        }
                        memcpy(&checktime, &timeofday, sizeof(struct timeval));
                }
#ifdef ENABLE_NATPMP
                /* Remove expired NAT-PMP mappings */
                while( nextnatpmptoclean_timestamp && (timeofday.tv_sec >= nextnatpmptoclean_timestamp + startup_time))
                {
                        /*syslog(LOG_DEBUG, "cleaning expired NAT-PMP mappings");*/
                        if(CleanExpiredNATPMP() < 0) {
                                syslog(LOG_ERR, "CleanExpiredNATPMP() failed");
                                break;
                        }
                }
                if(nextnatpmptoclean_timestamp && timeout.tv_sec >= (nextnatpmptoclean_timestamp + startup_time - timeofday.tv_sec))
                {
                        /*syslog(LOG_DEBUG, "setting timeout to %d sec", nextnatpmptoclean_timestamp + startup_time - timeofday.tv_sec);*/
                        timeout.tv_sec = nextnatpmptoclean_timestamp + startup_time - timeofday.tv_sec;
                        timeout.tv_usec = 0;
                }
#endif

                /* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
                FD_ZERO(&readset);

                if (sudp >= 0)
                {
                        FD_SET(sudp, &readset);
                        max_fd = MAX( max_fd, sudp);
                }

                if (shttpl >= 0)
                {
                        FD_SET(shttpl, &readset);
                        max_fd = MAX( max_fd, shttpl);
                }

                i = 0;  /* active HTTP connections count */
                for(e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
                {
                        if((e->socket >= 0) && (e->state <= 2))
                        {
                                FD_SET(e->socket, &readset);
                                max_fd = MAX( max_fd, e->socket);
                                i++;
                        }
                }
                /* for debug */
#ifdef DEBUG
                if(i > 1)
                {
                        syslog(LOG_DEBUG, "%d active incoming HTTP connections", i);
                }
#endif
#ifdef ENABLE_NATPMP
                if(snatpmp >= 0) {
                        FD_SET(snatpmp, &readset);
                        max_fd = MAX( max_fd, snatpmp);
                }
#endif
#ifdef USE_MINIUPNPDCTL
                if(sctl >= 0) {
                        FD_SET(sctl, &readset);
                        max_fd = MAX( max_fd, sctl);
                }

                for(ectl = ctllisthead.lh_first; ectl; ectl = ectl->entries.le_next)
                {
                        if(ectl->socket >= 0) {
                                FD_SET(ectl->socket, &readset);
                                max_fd = MAX( max_fd, ectl->socket);
                        }
                }
#endif

#ifdef ENABLE_EVENTS
                FD_ZERO(&writeset);
                upnpevents_selectfds(&readset, &writeset, &max_fd);
#endif

#ifdef ENABLE_EVENTS
                if(select(max_fd+1, &readset, &writeset, 0, &timeout) < 0)
#else
                if(select(max_fd+1, &readset, 0, 0, &timeout) < 0)
#endif
                {
                        if(quitting) goto shutdown;
                        /*      Date:   2009-04-21
                        *       Name:   jimmy huang
                        *       Reason: we wan reload port forward rules after firewal re-run
                        *                       So we will send SIGUSR1 from rc/firewall.c to miniupnpd
                        *                       But When receiving signal from rc (firewall.c),
                        *                       select think there is a interrupt,
                        *                       (
                        *                               select (all): Interrupted system call
                        *                               Failed to select open sockets, EXITING
                        *                       )
                        *                       and it is not his design originally,
                        *                       so miniupnpd will take this
                        *                       as a mistake, thus miniupnpd exit anyway
                        *                       No we ask miniupnpd to ignore this situation (EINTR)
                        *       Note:   If this way has any side effect, the backup solution
                        *                       is maybe we need adjust start_upnp()'s calling
                        *                       sequence within start_app() ?
                        */
                        if(errno == EINTR){
                                syslog(LOG_ERR, "receive Interrupted system signal");
                                continue;
                        }
                        // -----------
                        syslog(LOG_ERR, "select(all): %m");
                        syslog(LOG_ERR, "Failed to select open sockets. EXITING");

                        return 1;       /* very serious cause of error */
                }
#ifdef USE_MINIUPNPDCTL
                for(ectl = ctllisthead.lh_first; ectl;)
                {
                        ectlnext =  ectl->entries.le_next;
                        if((ectl->socket >= 0) && FD_ISSET(ectl->socket, &readset))
                        {
                                char buf[256];
                                int l;
                                l = read(ectl->socket, buf, sizeof(buf));
                                if(l > 0)
                                {
                                        /*write(ectl->socket, buf, l);*/
                                        write_option_list(ectl->socket);
                                        write_permlist(ectl->socket, upnppermlist, num_upnpperm);
                                        write_upnphttp_details(ectl->socket, upnphttphead.lh_first);
                                        write_ctlsockets_list(ectl->socket, ctllisthead.lh_first);
                                        write_ruleset_details(ectl->socket);
#ifdef ENABLE_EVENTS
                                        write_events_details(ectl->socket);
#endif
                                        /* close the socket */
                                        close(ectl->socket);
                                        ectl->socket = -1;
                                }
                                else
                                {
                                        close(ectl->socket);
                                        ectl->socket = -1;
                                }
                        }
                        if(ectl->socket < 0)
                        {
                                LIST_REMOVE(ectl, entries);
                                free(ectl);
                        }
                        ectl = ectlnext;
                }
                if((sctl >= 0) && FD_ISSET(sctl, &readset))
                {
                        int s;
                        struct sockaddr_un clientname;
                        struct ctlelem * tmp;
                        socklen_t clientnamelen = sizeof(struct sockaddr_un);
                        //syslog(LOG_DEBUG, "sctl!");
                        s = accept(sctl, (struct sockaddr *)&clientname,
                                   &clientnamelen);
                        syslog(LOG_DEBUG, "sctl! : '%s'", clientname.sun_path);
                        tmp = malloc(sizeof(struct ctlelem));
                        if (!tmp) {
                                return 1;
                        }
                        tmp->socket = s;
                        LIST_INSERT_HEAD(&ctllisthead, tmp, entries);
                }
#endif
#ifdef ENABLE_EVENTS
                /*      Date:   2009-04-20
                *       Name:   jimmy huang
                *       Reason:
                *                       1. For Event Notify (Wan IP Update)
                *                       2. Checking PortForward rules expiration
                */
                check_if_SubscriberNeedNotify();
                ScanAndCleanPortMappingExpiration();
                // ------------
                upnpevents_processfds(&readset, &writeset);
#endif
#ifdef ENABLE_NATPMP
                /* process NAT-PMP packets */
                if((snatpmp >= 0) && FD_ISSET(snatpmp, &readset))
                {
                        ProcessIncomingNATPMPPacket(snatpmp);
                }
#endif
                /* process SSDP packets */
                if(sudp >= 0 && FD_ISSET(sudp, &readset))
                {
                        /*syslog(LOG_INFO, "Received UDP Packet");*/
                        ProcessSSDPRequest(sudp, (unsigned short)v.port);
                }
                /* process active HTTP connections */
                /* LIST_FOREACH macro is not available under linux */
                for(e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
                {
                        if(  (e->socket >= 0) && (e->state <= 2)
                                &&(FD_ISSET(e->socket, &readset)) )
                        {
                                Process_upnphttp(e);
                        }
                }
                /* process incoming HTTP connections */
                if(shttpl >= 0 && FD_ISSET(shttpl, &readset))
                {
                        int shttp;
                        socklen_t clientnamelen;
                        struct sockaddr_in clientname;
                        clientnamelen = sizeof(struct sockaddr_in);
                        shttp = accept(shttpl, (struct sockaddr *)&clientname, &clientnamelen);
                        if(shttp<0)
                        {
                                syslog(LOG_ERR, "accept(http): %m");
                        }
                        else
                        {
                                struct upnphttp * tmp = 0;
                                DEBUG_MSG("\n\nHTTP connection from %s:%d\n\n"
                                                ,inet_ntoa(clientname.sin_addr)
                                                ,ntohs(clientname.sin_port));
                                syslog(LOG_INFO, "HTTP connection from %s:%d",
                                        inet_ntoa(clientname.sin_addr),
                                        ntohs(clientname.sin_port) );
                                /*if (fcntl(shttp, F_SETFL, O_NONBLOCK) < 0) {
                                        syslog(LOG_ERR, "fcntl F_SETFL, O_NONBLOCK");
                                }*/
                                /* Create a new upnphttp object and add it to
                                 * the active upnphttp object list */
                                tmp = New_upnphttp(shttp);
                                if(tmp)
                                {
                                        tmp->clientaddr = clientname.sin_addr;
                                        LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
                                }
                                else
                                {
                                        syslog(LOG_ERR, "New_upnphttp() failed");
                                        close(shttp);
                                }
                        }
                }
                /* delete finished HTTP connections */
                for(e = upnphttphead.lh_first; e != NULL; )
                {
                        next = e->entries.le_next;
                        if(e->state >= 100)
                        {
                                LIST_REMOVE(e, entries);
                                Delete_upnphttp(e);
                        }
                        e = next;
                }
	} /* end of main loop */

shutdown:
        /* close out open sockets */
        while(upnphttphead.lh_first != NULL)
        {
                e = upnphttphead.lh_first;
                LIST_REMOVE(e, entries);
                Delete_upnphttp(e);
        }

        if (sudp >= 0) close(sudp);
        if (shttpl >= 0) close(shttpl);
#ifdef ENABLE_NATPMP
        if(snatpmp>=0)
        {
                close(snatpmp);
                snatpmp = -1;
        }
#endif
#ifdef USE_MINIUPNPDCTL
        if(sctl>=0)
        {
                close(sctl);
                sctl = -1;
                if(unlink("/var/run/miniupnpd.ctl") < 0)
                {
                        syslog(LOG_ERR, "unlink() %m");
                }
        }
#endif

        /*if(SendSSDPGoodbye(snotify, v.n_lan_addr) < 0)*/
        if (GETFLAG(ENABLEUPNPMASK))
        {
                if(SendSSDPGoodbye(snotify, n_lan_addr) < 0)
                {
                        syslog(LOG_ERR, "Failed to broadcast good-bye notifications");
                }
                for(i=0; i<n_lan_addr; i++)/* for(i=0; i<v.n_lan_addr; i++)*/
                        close(snotify[i]);
        }

        if(unlink(pidfilename) < 0)
        {
                syslog(LOG_ERR, "Failed to remove pidfile %s: %m", pidfilename);
        }

        closelog();
        freeoptions();

        return 0;
}

