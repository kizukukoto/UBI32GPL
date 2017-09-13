/* $Id: iptcrdr.c,v 1.6 2009/07/09 07:38:11 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2008 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <xtables.h>
#include <libiptc/libiptc.h>
#include <iptables.h>
#include <sutil.h>

#include <linux/version.h>

#ifdef MINIUPNPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef IPTABLES_143
/* IPTABLES API version >= 1.4.3 */
#include <net/netfilter/nf_nat.h>
#define ip_nat_multi_range	nf_nat_multi_range
#define ip_nat_range		nf_nat_range
#define IPTC_HANDLE		struct iptc_handle *
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter_ipv4/ipt_state.h>
#include <linux/netfilter/xt_multiport.h>
#else
/* IPTABLES API version < 1.4.3 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
#include <linux/netfilter_ipv4/ip_nat.h>
/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
*	Note:	
*			1. only test with libipt iptables-1.3.3
*			2. Let compiler knows the structure knows struct ipt_state_info and NEW
*/
#include <linux/netfilter_ipv4/ipt_state.h>  // for struct ipt_state_info, "-m --state" used
#include <linux/netfilter_ipv4/ip_conntrack.h>  // for IPT_STATE_BIT(IP_CT_NEW) "NEW" used
// ----------
#else
#include <linux/netfilter/nf_nat.h>
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter_ipv4/ipt_state.h>  // for struct ipt_state_info, "-m --state" used
#include <linux/netfilter/xt_multiport.h>
#endif
#define IPTC_HANDLE		iptc_handle_t
#endif

#include "iptcrdr.h"
#include "../upnpglobalvars.h"
/*	Date:	2009-04-20
*	Name:	jimmy huang
*	Reason:	for after remove expired port mapping, call upnp_event_var_change_notify()
*/
#include "../upnpevents.h"
#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
#include "../nvram_funs.h"
#endif
//------

/* dummy init and shutdown functions */
int init_redirect(void)
{
	return 0;
}

void shutdown_redirect(void)
{
	return;
}

/* convert an ip address to string */
static int snprintip(char * dst, size_t size, uint32_t ip)
{
	return snprintf(dst, size,
	       "%u.%u.%u.%u", ip >> 24, (ip >> 16) & 0xff,
	       (ip >> 8) & 0xff, ip & 0xff);
}

/* netfilter cannot store redirection descriptions, so we use our
 * own structure to store them */
/*	Date:	2009-04-20
*	Name:	jimmy huang
*	Reason:	to let this structure can be seen by each one
*	Note:	move to iptcrdr.h
*/
/*
struct rdr_desc {
	struct rdr_desc * next;
	unsigned short eport;
	short proto;
	char str[];
};
*/


/* pointer to the chained list where descriptions are stored */

//static struct rdr_desc * rdr_desc_list = 0;
struct rdr_desc * rdr_desc_list = 0;
// ----------


/*	Date:	2009-04-20
*	Name:	jimmy huang
*	Reason:	For periodcially update port forward mapping rules expiration time
*/
void update_redirect_desc(unsigned short eport, int proto, const char * desc
		, const char *RemoteHost, unsigned short InternalPort, const char *InternalClient
		, int Enabled, long int LeaseDuration, long int *out_expire_time)
{
	struct rdr_desc * p, * last;
	p = rdr_desc_list;
	last = 0;

	DEBUG_MSG("\n%s:%s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);
	DEBUG_MSG("%s:%s (%d), NewRemoteHost = %s\n",__FILE__,__FUNCTION__,__LINE__,RemoteHost ? RemoteHost : "");
	DEBUG_MSG("%s:%s (%d), NewExternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,eport);
	DEBUG_MSG("%s:%s (%d), NewProtocol = %s (%d)\n",__FILE__,__FUNCTION__,__LINE__
		,proto == 6 ? "TCP" : (proto == 17 ? "UDP" : "Other Protocol")
		,proto
		);
	DEBUG_MSG("%s:%s (%d), NewInternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,InternalPort);
	DEBUG_MSG("%s:%s (%d), NewInternalClient = %s\n",__FILE__,__FUNCTION__,__LINE__,InternalClient ? InternalClient : "");
//	DEBUG_MSG("%s:%s (%d), NewEnabled = %d\n",__FILE__,__FUNCTION__,__LINE__,Enabled);
	DEBUG_MSG("%s:%s (%d), NewPortMappingDesc = %s\n",__FILE__,__FUNCTION__,__LINE__,desc ? desc : "");
	DEBUG_MSG("%s:%s (%d), NewLeaseDuration = %ld\n",__FILE__,__FUNCTION__,__LINE__,LeaseDuration);
	
	while(p)
	{
		if(p->eport == eport && p->proto == proto)
		{
			p->LeaseDuration = LeaseDuration;
			p->expire_time = time(NULL) + LeaseDuration;
			*out_expire_time = p->expire_time;
			return;
		}
		last = p;
		p = p->next;
	}
}
// ----------

static void
/*
add_redirect_desc(unsigned short eport, int proto, const char * desc)
*/
add_redirect_desc(unsigned short eport, int proto, const char * desc
		, char *RemoteHost, unsigned short InternalPort, char *InternalClient
		, int Enabled, long int LeaseDuration, long int *out_expire_time)
//------------------
{
	struct rdr_desc * p;
	size_t l;
	DEBUG_MSG("\n%s:%s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);
	DEBUG_MSG("%s:%s (%d), NewRemoteHost = %s\n",__FILE__,__FUNCTION__,__LINE__,RemoteHost ? RemoteHost : "");
	DEBUG_MSG("%s:%s (%d), NewExternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,eport);
	DEBUG_MSG("%s:%s (%d), NewProtocol = %s (%d)\n",__FILE__,__FUNCTION__,__LINE__
		,proto == 6 ? "TCP" : (proto == 17 ? "UDP" : "Other Protocol")
		,proto
		);
	DEBUG_MSG("%s:%s (%d), NewInternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,InternalPort);
	DEBUG_MSG("%s:%s (%d), NewInternalClient = %s\n",__FILE__,__FUNCTION__,__LINE__,InternalClient ? InternalClient : "");
//	DEBUG_MSG("%s:%s (%d), NewEnabled = %d\n",__FILE__,__FUNCTION__,__LINE__,Enabled);
	DEBUG_MSG("%s:%s (%d), NewPortMappingDesc = %s\n",__FILE__,__FUNCTION__,__LINE__,desc ? desc : "");
	DEBUG_MSG("%s:%s (%d), NewLeaseDuration = %ld\n",__FILE__,__FUNCTION__,__LINE__,LeaseDuration);
	if(desc)
	{
		l = strlen(desc) + 1;
		p = malloc(sizeof(struct rdr_desc) + l);
		//p = malloc(sizeof(struct rdr_desc));
		if(p)
		{
			p->next = rdr_desc_list;
			p->eport = eport;
			p->proto = (short)proto;
			memcpy(p->str, desc, l);
			//strncpy(p->str, desc, PORTMAPPING_DESC_LEN);
			
			if(RemoteHost){
				strncpy(p->RemoteHost,RemoteHost,INET_ADDRSTRLEN);
				p->RemoteHost[INET_ADDRSTRLEN-1] = '\0';
			}else{
				memset(p->RemoteHost,'\0',INET_ADDRSTRLEN);
			}
			p->InternalPort = InternalPort;
			if(InternalClient){
				strncpy(p->InternalClient,InternalClient,INET_ADDRSTRLEN);
				p->InternalClient[INET_ADDRSTRLEN-1] = '\0';
			}else{
				memset(p->InternalClient,'\0',INET_ADDRSTRLEN);
			}
			p->Enabled = Enabled;
			p->LeaseDuration = LeaseDuration;
			p->expire_time = time(NULL) + LeaseDuration;
			*out_expire_time = p->expire_time;
			// --------
			rdr_desc_list = p;
		}
	}
	DEBUG_MSG("%s:%s (%d): End\n",__FILE__,__FUNCTION__,__LINE__);
}


//static void
void
del_redirect_desc(unsigned short eport, int proto)
{
	struct rdr_desc * p, * last;
	p = rdr_desc_list;
	last = 0;
	while(p)
	{
		if(p->eport == eport && p->proto == proto)
		{
			if(!last)
				rdr_desc_list = p->next;
			else
				last->next = p->next;                             
			free(p);
			return;
		}
		last = p;
		p = p->next;
	}
}


/*
static void
get_redirect_desc(unsigned short eport, int proto,
                  char * desc, int desclen)
*/
int get_redirect_desc(unsigned short eport, int proto
                  , char * desc, int desclen
				  , char *RemoteHost
				  , unsigned short *InternalPort, char *InternalClient
				  , int *Enabled, long int *LeaseDuration, long int *out_expire_time)
//------------------
{
	struct rdr_desc * p;
	if(!desc || (desclen == 0))
		return 0;
	for(p = rdr_desc_list; p; p = p->next)
	{
		if(p->eport == eport && p->proto == (short)proto)
		{
			strncpy(desc, p->str, desclen);

			if(p->RemoteHost && (strlen(p->RemoteHost) > 6)){
				strncpy(RemoteHost,p->RemoteHost,INET_ADDRSTRLEN);
			}else{
				RemoteHost[0] = '\0';
			}
			if(p->InternalPort){
				*InternalPort = p->InternalPort;
			}else{
				*InternalPort = 0;
			}
			if(p->InternalClient && (strlen(p->InternalClient) > 6)){
				strncpy(InternalClient,p->InternalClient,INET_ADDRSTRLEN);
			}else{
				InternalClient[0] = '\0';
			}

			if(p->Enabled){
				*Enabled = p->Enabled;
			}else{
				*Enabled = 0;
			}
			*Enabled = 1;

			if(p->LeaseDuration > 0){
				// finite
				*LeaseDuration = p->LeaseDuration;
				*out_expire_time = p->expire_time;
			}else{
				// infinite
				*LeaseDuration = 0;
				*out_expire_time = 0;
			}
			
			// ---------
			return 1;
		}
	}
	/* if no description was found, return miniupnpd as default */
	strncpy(desc, "miniupnpd", desclen);
	return 0;
}

/* add_redirect_rule2() */
int
/*
add_redirect_rule2(const char * ifname, unsigned short eport,
                   const char * iaddr, unsigned short iport, int proto,
				   const char * desc,
				   const char * eaddr)// Chun add: for CD_ROUTER
*/
add_redirect_rule2(const char * ifname, unsigned short eport,
					const char * iaddr, unsigned short iport, int proto,
					const char * desc,
					const char * eaddr,
					int Enabled, long int LeaseDuration,
					long int *out_expire_time)
// -----------------
{
    //struct rdr_desc p;
	
	/* Chun modify: for CD_ROUTER */
	//int r = addnatrule(proto, eport, iaddr, iport);
	// compile and run through here
	DEBUG_MSG("\n%s:%s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);
	DEBUG_MSG("%s:%s (%d), NewRemoteHost = %s\n",__FILE__,__FUNCTION__,__LINE__,eaddr ? eaddr : "");
	DEBUG_MSG("%s:%s (%d), NewExternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,eport);
	DEBUG_MSG("%s:%s (%d), NewProtocol = %s (%d)\n",__FILE__,__FUNCTION__,__LINE__
		,proto == 6 ? "TCP" : (proto == 17 ? "UDP" : "Other Protocol")
		,proto
		);
	DEBUG_MSG("%s:%s (%d), NewInternalPort = %hu\n",__FILE__,__FUNCTION__,__LINE__,iport);
	DEBUG_MSG("%s:%s (%d), NewInternalClient = %s\n",__FILE__,__FUNCTION__,__LINE__,iaddr ? iaddr : "");
	DEBUG_MSG("%s:%s (%d), NewEnabled = %d\n",__FILE__,__FUNCTION__,__LINE__,Enabled);
	DEBUG_MSG("%s:%s (%d), NewPortMappingDesc = %s\n",__FILE__,__FUNCTION__,__LINE__,desc ? desc : "");
	DEBUG_MSG("%s:%s (%d), NewLeaseDuration = %ld\n",__FILE__,__FUNCTION__,__LINE__,LeaseDuration);
	int r = addnatrule(proto, eport, iaddr, iport, eaddr); 
	if(r >= 0){
		//add_redirect_desc(eport, proto, desc);
		add_redirect_desc(eport, proto, desc
			, (char *) eaddr, iport, (char *) iaddr
			, Enabled, LeaseDuration, out_expire_time);
	// -------------
	}
	return r;
}

int
add_filter_rule2(const char * ifname, const char * iaddr,
                 unsigned short eport, unsigned short iport,
                 int proto, const char * desc)
{
	return add_filter_rule(proto, iaddr, iport);
}

/* get_redirect_rule() 
 * returns -1 if the rule is not found */
int
get_redirect_rule(const char * ifname, unsigned short eport, int proto,
                  char * iaddr, int iaddrlen, unsigned short * iport,
                  char * desc, int desclen,
                  u_int64_t * packets, u_int64_t * bytes, long int *expire_time)
{
	int r = -1;
	IPTC_HANDLE h;
	const struct ipt_entry * e;
	const struct ipt_entry_target * target;
	const struct ip_nat_multi_range * mr;
	const struct ipt_entry_match *match;
	
	char RemoteHost[INET_ADDRSTRLEN] = {0};
	unsigned short InternalPort = 0;
	char InternalClient[INET_ADDRSTRLEN] = {0};
	int Enabled = 0;
	long int LeaseDuration = 0;

	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	
/*	Date:	2009-04-23
*	Name:	jimmy huang
*	Reason:	For support UPnP can see static port forward rules
*			added via WEB GUI
*/
#ifdef CHECK_VS_NVRAM

// this fucntion gets virtual server rules from nvram for specific proto + port
	r = nvram_GetSpecificPortMappingEntry(eport, proto
			, iport, iaddr
			, &Enabled , desc , &LeaseDuration);

	if(r == 1){
		return 0;
	}
	r = -1;

#endif

	h = iptc_init("nat");
	if(!h)
	{
		syslog(LOG_ERR, "get_redirect_rule() : "
		                "iptc_init() failed : %s",
		       iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain(miniupnpd_nat_chain, h))
	{
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = iptc_first_rule(miniupnpd_nat_chain, h);
		    e;
			e = iptc_next_rule(e, h))
#else
		for(e = iptc_first_rule(miniupnpd_nat_chain, &h);
		    e;
			e = iptc_next_rule(e, &h))
#endif
		{
			if(proto==e->ip.proto)
			{
				match = (const struct ipt_entry_match *)&e->elems;
				if(0 == strncmp(match->u.user.name, "tcp", IPT_FUNCTION_MAXNAMELEN))
				{
					const struct ipt_tcp * info;
					info = (const struct ipt_tcp *)match->data;
					if(eport != info->dpts[0])
						continue;
				}
				else
				{
					const struct ipt_udp * info;
					info = (const struct ipt_udp *)match->data;
					if(eport != info->dpts[0])
						continue;
				}
				target = (void *)e + e->target_offset;
				//target = ipt_get_target(e);
				mr = (const struct ip_nat_multi_range *)&target->data[0];
				snprintip(iaddr, iaddrlen, ntohl(mr->range[0].min_ip));
				*iport = ntohs(mr->range[0].min.all);
				/*if(desc)
					strncpy(desc, "miniupnpd", desclen);*/

				//get_redirect_desc(eport, proto, desc, desclen);
				get_redirect_desc(eport, proto, desc, desclen
							, RemoteHost, &InternalPort, InternalClient
							, &Enabled, &LeaseDuration, expire_time
							);
				// ------------
				if(packets)
					*packets = e->counters.pcnt;
				if(bytes)
					*bytes = e->counters.bcnt;
				r = 0;
				break;
			}
		}
	}
/*	Date:	2009-04-20
*	Name:	jimmy huang
*	Reason:	For also checking PREROUTING, whcih may added via WEB GUI
*/

	DEBUG_MSG("%s (%d): getting PREROUTING Chain\n",__FUNCTION__,__LINE__);
	if(!iptc_is_chain("PREROUTING", h))
	{
		DEBUG_MSG("%s (%d): not find PREROUTING Chain !\n",__FUNCTION__,__LINE__);
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = iptc_first_rule("PREROUTING", h);
		    e;
			e = iptc_next_rule(e, h))
#else
		for(e = iptc_first_rule("PREROUTING", &h);
		    e;
			e = iptc_next_rule(e, &h))
#endif
		{
			
			DEBUG_MSG("%s (%d): iptc_get_target(%s)\n", iptc_get_target(e, &h));
			if( strcmp(iptc_get_target(e, &h), "DNAT") !=0 )
				continue;
				
			if(proto==e->ip.proto)
			{
				DEBUG_MSG("%s (%d): find same proto !\n",__FUNCTION__,__LINE__);
				match = (const struct ipt_entry_match *)&e->elems;
				if(0 == strncmp(match->u.user.name, "tcp", IPT_FUNCTION_MAXNAMELEN))
				{
					const struct ipt_tcp * info;
					info = (const struct ipt_tcp *)match->data;
					DEBUG_MSG("%s (%d): eport(%d) info->dpts[0](%d)\n",__FUNCTION__,__LINE__, eport, info->dpts[0]);
					if(eport != info->dpts[0]){
						DEBUG_MSG("%s (%d): tcp port is not match !\n",__FUNCTION__,__LINE__);
						continue;
					}
					DEBUG_MSG("%s (%d): find same tcp port ! \n",__FUNCTION__,__LINE__);
				}
				else if(0 == strncmp(match->u.user.name, "udp", IPT_FUNCTION_MAXNAMELEN))
				{
					const struct ipt_udp * info;
					info = (const struct ipt_udp *)match->data;
					DEBUG_MSG("%s (%d): eport(%d) info->dpts[0](%d)\n",__FUNCTION__,__LINE__, eport, info->dpts[0]);
					//if(1){
					if(eport != info->dpts[0]){
						DEBUG_MSG("%s (%d): udp port is not match !\n",__FUNCTION__,__LINE__);
						continue;
					}
					DEBUG_MSG("%s (%d): find same udp port ! \n",__FUNCTION__,__LINE__);
				}
				else if(0 == strncmp(match->u.user.name, "multiport", IPT_FUNCTION_MAXNAMELEN))
				{
					const struct xt_multiport_v1 * minfo_v1;
					minfo_v1 = (const struct xt_multiport_v1 *)match->data;
					int i, impact_flag = 0;
					for(i=0; i< minfo_v1->count; i++){
						DEBUG_MSG("%s (%d): eport(%d) flags(%d) ports[%d](%d) pflags(%d) invert(%d)\n",
											__FUNCTION__,__LINE__, eport, minfo_v1->flags, i, minfo_v1->ports[i], minfo_v1->pflags[i], minfo_v1->invert);
						if(minfo_v1->pflags[i] == 1){
							if((minfo_v1->ports[i] < eport ) && (eport < minfo_v1->ports[i+1])){
								impact_flag = 1;
								DEBUG_MSG("%s (%d): find same multiport port range ! \n",__FUNCTION__,__LINE__);
								continue;
						  }
						}
						else if(eport == minfo_v1->ports[i])
						{
							impact_flag = 1;
							DEBUG_MSG("%s (%d): find same multiport port ! \n",__FUNCTION__,__LINE__);
							continue;
						}
					}
					if(impact_flag == 0){
						DEBUG_MSG("%s (%d): multiport is not match !\n",__FUNCTION__,__LINE__);
						continue;
					}
					DEBUG_MSG("%s (%d): find same multiport port or range! \n",__FUNCTION__,__LINE__);
				}
				target = (void *)e + e->target_offset;
				mr = (const struct ip_nat_multi_range *)&target->data[0];
				snprintip(iaddr, iaddrlen, ntohl(mr->range[0].min_ip));
				*iport = ntohs(mr->range[0].min.all);

				// cause here is added via WEB GUI so set desc as "WEB GUI"
				// Note: desc is a pointer, not alloc memory !!! 
				//desclen = strlen("WEB GUI");
				//strncpy(desc,"WEB GUI",desclen);
				// ----------
				if(packets)
					*packets = e->counters.pcnt;
				if(bytes)
					*bytes = e->counters.bcnt;
				DEBUG_MSG("%s (%d): This rule has same added to iptables !!!\n",__FUNCTION__,__LINE__);
				r = 1;
				break;
			}
		}
	}
// -------------------------------------------
#ifdef IPTABLES_143
	iptc_free(h);
#else
	iptc_free(&h);
#endif
	DEBUG_MSG("%s (%d): End\n",__FUNCTION__,__LINE__);
	return r;
}

/* get_redirect_rule_by_index() 
 * return -1 when the rule was not found */
int
get_redirect_rule_by_index(int index_local,
                           char * ifname, unsigned short * eport,
                           char * iaddr, int iaddrlen, unsigned short * iport,
                           int * proto, char * desc, int desclen,
                           u_int64_t * packets, u_int64_t * bytes
						   ,long int *expire_time
						   )
{
	int r = -1;
	int i = 0;
	IPTC_HANDLE h;
	const struct ipt_entry * e;
	const struct ipt_entry_target * target;
	const struct ip_nat_multi_range * mr;
	const struct ipt_entry_match *match;

	char RemoteHost[INET_ADDRSTRLEN] = {0};
	unsigned short InternalPort = 0;
	char InternalClient[INET_ADDRSTRLEN] = {0};
	int Enabled = 0;
	long int LeaseDuration = 0;
#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
	int num_in_nvram = 0;
#endif

	DEBUG_MSG("\n%s, %s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);

/*	Date:	2009-04-23
*	Name:	jimmy huang
*	Reason:	For support UPnP can see static port forward rules
*			added via WEB GUI
*/
#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
/* this fucntion gets virtual server rules from nvram for specific number */
	DEBUG_MSG("%s (%d): find nvram with index_local = %d\n",__FUNCTION__,__LINE__,index_local);
	r = nvram_GetGenericPortMappingEntry(index_local
			, eport, proto
			, iport, iaddr
			, &Enabled, desc, &LeaseDuration);
	if(r == 1){
		return 0;
	}
	r = -1;

	/*
	TODO: improve this
	*/
	num_in_nvram = nvram_GetVsRules_Total_Number();
	index_local = index_local - num_in_nvram;
	if(index_local < 0){
		fprintf(stderr,"miniupnpd: %s(), Error, index_local = %d should not happen !!\n",__FUNCTION__,index_local);
		return -1;
	}
#endif
	DEBUG_MSG("%s (%d): index_local = %d\n",__FUNCTION__,__LINE__,index_local);

	h = iptc_init("nat");
	if(!h)
	{
		DEBUG_MSG("get_redirect_rule_by_index() : "
				"iptc_init() failed : %s",
				 iptc_strerror(errno));
		syslog(LOG_ERR, "get_redirect_rule_by_index() : "
		                "iptc_init() failed : %s",
		       iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain(miniupnpd_nat_chain, h))
	{
		DEBUG_MSG("chain %s not found", miniupnpd_nat_chain);
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = iptc_first_rule(miniupnpd_nat_chain, h);
		    e;
			e = iptc_next_rule(e, h))
#else
		for(e = iptc_first_rule(miniupnpd_nat_chain, &h);
		    e;
			e = iptc_next_rule(e, &h))
#endif
		{
			if(i==index_local)
			{
				*proto = e->ip.proto;
				DEBUG_MSG("%s (%d): proto(%s)\n",__FUNCTION__,__LINE__, proto);
				match = (const struct ipt_entry_match *)&e->elems;
				if(0 == strncmp(match->u.user.name, "tcp", IPT_FUNCTION_MAXNAMELEN))
				{
					const struct ipt_tcp * info;
					info = (const struct ipt_tcp *)match->data;
					*eport = info->dpts[0];
				}
				else
				{
					const struct ipt_udp * info;
					info = (const struct ipt_udp *)match->data;
					*eport = info->dpts[0];
				}
				target = (void *)e + e->target_offset;
				mr = (const struct ip_nat_multi_range *)&target->data[0];
				snprintip(iaddr, iaddrlen, ntohl(mr->range[0].min_ip));
				*iport = ntohs(mr->range[0].min.all);
                /*if(desc)
				    strncpy(desc, "miniupnpd", desclen);*/

				//get_redirect_desc(*eport, *proto, desc, desclen);
				DEBUG_MSG("%s (%d): go get_redirect_desc()\n",__FUNCTION__,__LINE__);
				get_redirect_desc(*eport, *proto, desc, desclen
							, RemoteHost, &InternalPort, InternalClient
							, &Enabled, &LeaseDuration, expire_time
							);
				// -------------
				if(packets)
					*packets = e->counters.pcnt;
				if(bytes)
					*bytes = e->counters.bcnt;
				r = 0;
				break;
			}
			i++;
		}
	}
#ifdef IPTABLES_143
	iptc_free(h);
#else
	iptc_free(&h);
#endif
	DEBUG_MSG("%s (%d): End, return r = %d\n",__FUNCTION__,__LINE__,r);
	return r;
}

/*	Date:	2009-04-29
*	Name:	jimmy huang
*	Reason:	To reduce the counting time when control point
*			ask our numbers of PortMapping rules
*	Note:	Return numbers of rules
*/
int get_redirect_rule_nums(void)
{
	int i = 0;
	IPTC_HANDLE h;
	const struct ipt_entry * e;

#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
	int num_in_nvram = 0;
#endif

	DEBUG_MSG("\n%s, %s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);

#if defined(CHECK_VS_NVRAM) || defined(CHECK_PF_NVRAM)
	/* this fucntion gets virtual server rules nums from nvram */
	num_in_nvram = nvram_GetVsRules_Total_Number();
	DEBUG_MSG("%s (%d): num_in_nvram = %d\n",__FUNCTION__,__LINE__,num_in_nvram);
	i = i + num_in_nvram;
#endif

	h = iptc_init("nat");
	if(!h)
	{
		syslog(LOG_ERR, "get_redirect_rule_nums() : "
		                "iptc_init() failed : %s",
		       iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain(miniupnpd_nat_chain, h))
	{
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = iptc_first_rule(miniupnpd_nat_chain, h);
		    e;
			e = iptc_next_rule(e, h))
#else
		for(e = iptc_first_rule(miniupnpd_nat_chain, &h);
		    e;
			e = iptc_next_rule(e, &h))
#endif
		{
			i++;
		}
	}
#ifdef IPTABLES_143
	iptc_free(h);
#else
	iptc_free(&h);
#endif
	DEBUG_MSG("%s (%d): End, return num = %d\n",__FUNCTION__,__LINE__,i);
	return i;
}


/* delete_rule_and_commit() :
 * subfunction used in delete_redirect_and_filter_rules() */
static int
delete_rule_and_commit(unsigned int index_local, IPTC_HANDLE h,
                       const char * miniupnpd_chain,
                       const char * logcaller)
{
	int r = 0;
	DEBUG_MSG("\n%s (%d): begin\n",__FUNCTION__,__LINE__);
#ifdef IPTABLES_143
	if(!iptc_delete_num_entry(miniupnpd_chain, index_local, h))
#else
	if(!iptc_delete_num_entry(miniupnpd_chain, index_local, &h))
#endif
	{
		syslog(LOG_ERR, "%s() : iptc_delete_num_entry(): %s\n",
	    	   logcaller, iptc_strerror(errno));
		r = -1;
	}
#ifdef IPTABLES_143
	else if(!iptc_commit(h))
#else
	else if(!iptc_commit(&h))
#endif
	{
		syslog(LOG_ERR, "%s() : iptc_commit(): %s\n",
	    	   logcaller, iptc_strerror(errno));
		r = -1;
	}
	DEBUG_MSG("%s (%d): end\n",__FUNCTION__,__LINE__);
	return r;
}

/* delete_redirect_and_filter_rules()
 */
int
delete_redirect_and_filter_rules(unsigned short eport, int proto)
{
	int r = -1;
	unsigned index_local = 0;
	unsigned i = 0;
	IPTC_HANDLE h;
	const struct ipt_entry * e;
	const struct ipt_entry_match *match;
	
	DEBUG_MSG("\n%s (%d): begin \n",__FUNCTION__,__LINE__);

	h = iptc_init("nat");
	if(!h)
	{
		syslog(LOG_ERR, "delete_redirect_and_filter_rules() : "
		                "iptc_init() failed : %s",
		       iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain(miniupnpd_nat_chain, h))
	{
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = iptc_first_rule(miniupnpd_nat_chain, h);
		    e;
			e = iptc_next_rule(e, h), i++)
#else
		for(e = iptc_first_rule(miniupnpd_nat_chain, &h);
		    e;
			e = iptc_next_rule(e, &h), i++)
#endif
		{
			if(proto==e->ip.proto)
			{
				match = (const struct ipt_entry_match *)&e->elems;
				if(0 == strncmp(match->u.user.name, "tcp", IPT_FUNCTION_MAXNAMELEN))
				{
					const struct ipt_tcp * info;
					info = (const struct ipt_tcp *)match->data;
					if(eport != info->dpts[0])
						continue;
				}
				else
				{
					const struct ipt_udp * info;
					info = (const struct ipt_udp *)match->data;
					if(eport != info->dpts[0])
						continue;
				}
				index_local = i;
				r = 0;
				break;
			}
		}
	}
#ifdef IPTABLES_143
	iptc_free(h);
#else
	iptc_free(&h);
#endif
	if(r == 0)
	{
		syslog(LOG_INFO, "Trying to delete rules at index_local %u", index_local);
		/* Now delete both rules */
		h = iptc_init("nat");
		DEBUG_MSG("%s (%d): iptc_init (nat) success\n",__FUNCTION__,__LINE__);
		if(h)
		{
			unsigned int num_rules;

			DEBUG_MSG("%s (%d): go delete_rule_and_commit(miniupnpd_nat_chain)\n",__FUNCTION__,__LINE__);
			r = delete_rule_and_commit(index_local, h, miniupnpd_nat_chain, "delete_redirect_rule");
			DEBUG_MSG("%s (%d): delete_rule_and_commit() return r = %d\n",__FUNCTION__,__LINE__,r);

			/*
			 * Gareth Williams (gwilliams@ubicom.com) 14/Dec/2009. When last rule is deleted remove jump from main table (performance optimisation).
			 */
#ifdef IPTABLES_143
			for (num_rules = 0, e = iptc_first_rule(miniupnpd_nat_chain, h); e; e = iptc_next_rule(e, h))
#else
			for (num_rules = 0, e = iptc_first_rule(miniupnpd_nat_chain, &h); e; e = iptc_next_rule(e, &h))
#endif
			{
				num_rules++;
			}
			if (num_rules != 0) {
				syslog(LOG_DEBUG, "miniupnpd : still rules remain, link not removed\n");
			} else {
				syslog(LOG_INFO, "miniupnpd : link to nat miniupnpd table removed\n");
				_system("iptables -t nat -D PREROUTING -j MINIUPNPD");	/* Delete it */
			}

			/*	Date:	2009-09-24
			*	Name:	jimmy huang
			*	Reason:	Fixing unclosed raw sockets bug with netfilter code
			*	Note:	Patched from miniupnpd-2009-09-21
			*			for iptables older version < 1.4.3, when iptc_commit() success, will also call iptc_free()
			*			so older iptable no need to iptc_free after iptc_commt() succeed.
			*/
#ifdef IPTABLES_143
			iptc_free(h);
// #else
// 			iptc_free(&h);
#endif
		}else{
			DEBUG_MSG("%s (%d): iptc_handle_t h Not has values\n",__FUNCTION__,__LINE__);
		}
		h = iptc_init("filter");
		DEBUG_MSG("%s (%d): iptc_init (filter) success\n",__FUNCTION__,__LINE__);
		if(h && (r == 0))
		{
			unsigned int num_rules;

			DEBUG_MSG("%s (%d): go delete_rule_and_commit(miniupnpd_forward_chain)\n",__FUNCTION__,__LINE__);
			r = delete_rule_and_commit(index_local, h, miniupnpd_forward_chain, "delete_filter_rule");
			DEBUG_MSG("%s (%d): delete_rule_and_commit() return r = %d\n",__FUNCTION__,__LINE__,r);

			/*
			 * Gareth Williams (gwilliams@ubicom.com) 14/Dec/2009. When last rule is deleted remove jump from main table (performance optimisation).
			 */
#ifdef IPTABLES_143
			for (num_rules = 0, e = iptc_first_rule(miniupnpd_forward_chain, h); e; e = iptc_next_rule(e, h))
#else
			for (num_rules = 0, e = iptc_first_rule(miniupnpd_forward_chain, &h); e; e = iptc_next_rule(e, &h))
#endif
			{
				num_rules++;
			}
			if (num_rules != 0) {
				syslog(LOG_DEBUG, "miniupnpd : still rules remain, link not removed\n");
			} else {
				syslog(LOG_INFO, "miniupnpd : link to filter miniupnpd table removed\n");
				_system("iptables -t filter -D FORWARD -j MINIUPNPD");	/* Delete it */
			}

			/*	Date:	2009-09-24
			*	Name:	jimmy huang
			*	Reason:	Fixing unclosed raw sockets bug with netfilter code
			*	Note:	Patched from miniupnpd-2009-09-21
			*			for iptables older version < 1.4.3, when iptc_commit() success, will also call iptc_free()
			*			so older iptable no need to iptc_free after iptc_commt() succeed.
			*/
#ifdef IPTABLES_143
			iptc_free(h);
// #else
// 			iptc_free(&h);
#endif
		}else{
			DEBUG_MSG("%s (%d): iptc_handle_t h Not has values, r = %d\n",__FUNCTION__,__LINE__,r);
		}
	}
	/*	Date:	2009-04-20
	*	Name:	jimmy huang
	*	Reason:	mark this, when after call delete_redirect_and_filter_rules()
	*			we need to manually call del_redirect_desc()
	*/
	//del_redirect_desc(eport, proto);
	DEBUG_MSG("%s (%d): end ,return r = %d\n",__FUNCTION__,__LINE__,r);
	return r;
}

/* ==================================== */
/* TODO : add the -m state --state NEW,ESTABLISHED,RELATED 
 * only for the filter rule */
static struct ipt_entry_match *
get_tcp_match(unsigned short dport)
{
	struct ipt_entry_match *match;
	struct ipt_tcp * tcpinfo;
	size_t size;
	size =   IPT_ALIGN(sizeof(struct ipt_entry_match))
	       + IPT_ALIGN(sizeof(struct ipt_tcp));
	match = calloc(1, size);
	if (!match) {
		return NULL;
	}
	match->u.match_size = size;
	strncpy(match->u.user.name, "tcp", IPT_FUNCTION_MAXNAMELEN);
	tcpinfo = (struct ipt_tcp *)match->data;
	tcpinfo->spts[0] = 0;		/* all source ports */
	tcpinfo->spts[1] = 0xFFFF;
	tcpinfo->dpts[0] = dport;	/* specified destination port */
	tcpinfo->dpts[1] = dport;
	return match;
}

static struct ipt_entry_match *
get_udp_match(unsigned short dport)
{
	struct ipt_entry_match *match;
	struct ipt_udp * udpinfo;
	size_t size;
	size =   IPT_ALIGN(sizeof(struct ipt_entry_match))
	       + IPT_ALIGN(sizeof(struct ipt_udp));
	match = calloc(1, size);
	if (!match) {
		return NULL;
	}
	match->u.match_size = size;
	strncpy(match->u.user.name, "udp", IPT_FUNCTION_MAXNAMELEN);
	udpinfo = (struct ipt_udp *)match->data;
	udpinfo->spts[0] = 0;		/* all source ports */
	udpinfo->spts[1] = 0xFFFF;
	udpinfo->dpts[0] = dport;	/* specified destination port */
	udpinfo->dpts[1] = dport;
	return match;
}


/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
*	Note:	
*			1. only test with libipt iptables-1.3.3
*			2. This function is for set new rules match state "-m --state NEW"
*	TODO:		Use parameters for state "NEW", "RELATED", "ESTABLISHED" ...
*/
static struct ipt_entry_match *
get_state_match(void)
{
	struct ipt_entry_match *match;
	//struct ip_nat_multi_range * mr;
	//struct ip_nat_range * range;
	struct ipt_state_info *sinfo;
	size_t size;

	size =   IPT_ALIGN(sizeof(struct ipt_entry_match))
	       + IPT_ALIGN(sizeof(struct ipt_state_info));
	match = calloc(1, size);
	if (!match) {
		return NULL;
	}
	match->u.match_size = size;
	strncpy(match->u.user.name, "state", IPT_FUNCTION_MAXNAMELEN);

	// refer to iptables_2_6/extensions/libipt_state.c
	sinfo = (struct ipt_state_info *)&match->data[0];
	sinfo->statemask = 0;
	sinfo->statemask |= IPT_STATE_BIT(IP_CT_NEW);
	
	return match;
}

static struct ipt_entry_target *
get_dnat_target(const char * daddr, unsigned short dport)
{
	struct ipt_entry_target * target;
	struct ip_nat_multi_range * mr;
	struct ip_nat_range * range;
	size_t size;

	size =   IPT_ALIGN(sizeof(struct ipt_entry_target))
	       + IPT_ALIGN(sizeof(struct ip_nat_multi_range));
	target = calloc(1, size);
	if (!target) {
		return NULL;
	}
	target->u.target_size = size;
	strncpy(target->u.user.name, "DNAT", IPT_FUNCTION_MAXNAMELEN);
	/* one ip_nat_range already included in ip_nat_multi_range */
	mr = (struct ip_nat_multi_range *)&target->data[0];
	mr->rangesize = 1;
	range = &mr->range[0];
	range->min_ip = range->max_ip = inet_addr(daddr);
	range->flags |= IP_NAT_RANGE_MAP_IPS;
	range->min.all = range->max.all = htons(dport);
	range->flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	return target;
}

/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
*	Note:	
*			1. only test with libipt iptables-1.3.3
*			2. This function is SNAT
*/
static struct ipt_entry_target *
get_snat_target(const char * daddr, unsigned short dport)
{
	struct ipt_entry_target * target;
	struct ip_nat_multi_range * mr;
	struct ip_nat_range * range;
	size_t size;

	size =   IPT_ALIGN(sizeof(struct ipt_entry_target))
	       + IPT_ALIGN(sizeof(struct ip_nat_multi_range));
	target = calloc(1, size);
	if (!target) {
		return NULL;
	}
	target->u.target_size = size;
	strncpy(target->u.user.name, "SNAT", IPT_FUNCTION_MAXNAMELEN);
	/* one ip_nat_range already included in ip_nat_multi_range */
	mr = (struct ip_nat_multi_range *)&target->data[0];
	mr->rangesize = 1;
	range = &mr->range[0];
	range->min_ip = range->max_ip = inet_addr(daddr);
	range->flags |= IP_NAT_RANGE_MAP_IPS;
	range->min.all = htons(0);
	range->max.all = htons(65535);
	
	range->flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	return target;
}
// -------------

/* iptc_init_verify_and_append()
 * return 0 on success, -1 on failure */
static int
iptc_init_verify_and_append(const char * table, const char * miniupnpd_chain, struct ipt_entry * e,
                            const char * logcaller)
{
	IPTC_HANDLE h;
	h = iptc_init(table);
	/*	Date:	2009-07-08
	*	Name:	jimmy huang
	*	Reason:	To debug
	*	Note:	
	*/
#if 0
	printf("==================================================\n");
	printf("***  %s, tabe = %s  *** \n",__FUNCTION__,table);
	printf("***  %s, chain = %s  *** \n",__FUNCTION__,miniupnpd_chain);
	printf("%s, ipt_entry->ip.src = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.src))[0]
				,((unsigned char *)&(e->ip.src))[1]
				,((unsigned char *)&(e->ip.src))[2]
				,((unsigned char *)&(e->ip.src))[3]
				);
	printf("%s, ipt_entry->ip.dst = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.dst))[0]
				,((unsigned char *)&(e->ip.dst))[1]
				,((unsigned char *)&(e->ip.dst))[2]
				,((unsigned char *)&(e->ip.dst))[3]
				);
	printf("%s, ipt_entry->ip.smsk = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.smsk))[0]
				,((unsigned char *)&(e->ip.smsk))[1]
				,((unsigned char *)&(e->ip.smsk))[2]
				,((unsigned char *)&(e->ip.smsk))[3]
				);
	printf("%s, ipt_entry->ip.dmsk = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.dmsk))[0]
				,((unsigned char *)&(e->ip.dmsk))[1]
				,((unsigned char *)&(e->ip.dmsk))[2]
				,((unsigned char *)&(e->ip.dmsk))[3]
				);
	printf("%s, ipt_entry->ip.iniface = %s \n",__FUNCTION__
				,e->ip.iniface
				);
	printf("%s, ipt_entry->ip.outiface = %s \n",__FUNCTION__
				,e->ip.outiface
				);
	printf("%s, ipt_entry->ip.iniface_mask = %s \n",__FUNCTION__
				,e->ip.iniface_mask
				);
	printf("%s, ipt_entry->ip.outiface_mask = %s  \n",__FUNCTION__
				,e->ip.outiface_mask
				);
	printf("%s, ipt_entry->ip.proto = %hu \n",__FUNCTION__
				,e->ip.proto
				);
	printf("%s, ipt_entry->nfcache = %x \n",__FUNCTION__
				,e->nfcache
				);
	printf("--------------------------------------------------\n");
#endif
	if(!h)
	{
		syslog(LOG_ERR, "%s : iptc_init() error : %s\n",
		       logcaller, iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain(miniupnpd_chain, h))
	{
		syslog(LOG_ERR, "%s : iptc_is_chain() error : %s\n",
		       logcaller, iptc_strerror(errno));
		/*	Date:	2009-09-24
		*	Name:	jimmy huang
		*	Reason:	Fixing unclosed raw sockets bug with netfilter code
		*	Note:	Patched from miniupnpd-2009-09-21
		*/
#ifdef IPTABLES_143
		iptc_free(h);
#else
		iptc_free(&h);
#endif
		return -1;
	}
#ifdef IPTABLES_143
	if(!iptc_append_entry(miniupnpd_chain, e, h))
#else
	if(!iptc_append_entry(miniupnpd_chain, e, &h))
#endif
	{
		syslog(LOG_ERR, "%s : iptc_append_entry() error : %s\n",
		       logcaller, iptc_strerror(errno));
		/*	Date:	2009-09-24
		*	Name:	jimmy huang
		*	Reason:	Fixing unclosed raw sockets bug with netfilter code
		*	Note:	Patched from miniupnpd-2009-09-21
		*/
#ifdef IPTABLES_143
		iptc_free(h);
#else
		iptc_free(&h);
#endif
		return -1;
	}
#ifdef IPTABLES_143
	if(!iptc_commit(h))
#else
	if(!iptc_commit(&h))
#endif
	{
		syslog(LOG_ERR, "%s : iptc_commit() error : %s\n",
		       logcaller, iptc_strerror(errno));
		/*	Date:	2009-09-24
		*	Name:	jimmy huang
		*	Reason:	Fixing unclosed raw sockets bug with netfilter code
		*	Note:	Patched from miniupnpd-2009-09-21
		*/
#ifdef IPTABLES_143
		iptc_free(h);
#else
		iptc_free(&h);
#endif
		return -1;
	}

	/*	Date:	2009-09-24
	*	Name:	jimmy huang
	*	Reason:	Fixing unclosed raw sockets bug with netfilter code
	*	Note:	Patched from miniupnpd-2009-09-21
	*			for iptables older version < 1.4.3, when iptc_commit() success, will also call iptc_free()
	*			so older iptable no need to iptc_free after iptc_commt() succeed.
	*/
#ifdef IPTABLES_143
	iptc_free(h);
// #else
// 	iptc_free(&h);
#endif
	return 0;
}

/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
*	Note:	
*			1. only test with libipt iptables-1.3.3
*			2. This function is "insert" iptable rules
*/
/* iptc_init_verify_and_insert()
 * return 0 on success, -1 on failure */
static int
iptc_init_verify_and_insert(const char * table, const char * miniupnpd_chain, struct ipt_entry * e,
                            const char * logcaller)
{
	IPTC_HANDLE h;
	h = iptc_init(table);
#if 0
	printf("%s, tabe = %s \n",__FUNCTION__,table);
	printf("%s, chain = %s \n",__FUNCTION__,miniupnpd_chain);
	printf("==================================================\n");
	printf("%s, ipt_entry->ip.src = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.src))[0]
				,((unsigned char *)&(e->ip.src))[1]
				,((unsigned char *)&(e->ip.src))[2]
				,((unsigned char *)&(e->ip.src))[3]
				);
	printf("%s, ipt_entry->ip.dst = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.dst))[0]
				,((unsigned char *)&(e->ip.dst))[1]
				,((unsigned char *)&(e->ip.dst))[2]
				,((unsigned char *)&(e->ip.dst))[3]
				);
	printf("%s, ipt_entry->ip.smsk = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.smsk))[0]
				,((unsigned char *)&(e->ip.smsk))[1]
				,((unsigned char *)&(e->ip.smsk))[2]
				,((unsigned char *)&(e->ip.smsk))[3]
				);
	printf("%s, ipt_entry->ip.dmsk = %u.%u.%u.%u \n",__FUNCTION__
				,((unsigned char *)&(e->ip.dmsk))[0]
				,((unsigned char *)&(e->ip.dmsk))[1]
				,((unsigned char *)&(e->ip.dmsk))[2]
				,((unsigned char *)&(e->ip.dmsk))[3]
				);
	printf("%s, ipt_entry->ip.iniface = %s \n",__FUNCTION__
				,e->ip.iniface
				);
	printf("%s, ipt_entry->ip.outiface = %s \n",__FUNCTION__
				,e->ip.outiface
				);
	printf("%s, ipt_entry->ip.iniface_mask = %s \n",__FUNCTION__
				,e->ip.iniface_mask
				);
	printf("%s, ipt_entry->ip.outiface_mask = %s \n",__FUNCTION__
				,e->ip.outiface_mask
				);
	printf("%s, ipt_entry->ip.proto = %hu \n",__FUNCTION__
				,e->ip.proto
				);
	printf("%s, ipt_entry->nfcache = %x \n",__FUNCTION__
				,e->nfcache
				);
	printf("--------------------------------------------------\n");
#endif
	if(!h)
	{
		syslog(LOG_ERR, "%s : iptc_init() error : %s\n",
		       logcaller, iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain(miniupnpd_chain, h))
	{
		syslog(LOG_ERR, "%s : iptc_is_chain() error : %s\n",
		       logcaller, iptc_strerror(errno));
		return -1;
	}
#ifdef IPTABLES_143
	if(!iptc_insert_entry(miniupnpd_chain, e , 0 , h)) // 0:rule number
#else
	if(!iptc_insert_entry(miniupnpd_chain, e , 0 , &h))
#endif
	{
		syslog(LOG_ERR, "%s : iptc_insert_entry() error : %s\n",
		       logcaller, iptc_strerror(errno));
		return -1;
	}
#ifdef IPTABLES_143
	if(!iptc_commit(h))
#else
	if(!iptc_commit(&h))
#endif
	{
		syslog(LOG_ERR, "%s : iptc_commit() error : %s\n",
		       logcaller, iptc_strerror(errno));
		return -1;
	}
	return 0;
}


/* add nat rule 
 * iptables -t nat -A MINIUPNPD -p proto --dport eport -j DNAT --to iaddr:iport
 * */
int
addnatrule(int proto, unsigned short eport,
           const char * iaddr, unsigned short iport,
		   const char * eaddr) /* Chun add: for CD_ROUTER */
{
	int r = 0;
	struct ipt_entry *e;
	struct ipt_entry *new_e;
	struct ipt_entry_match *match = NULL;
	struct ipt_entry_target *target = NULL;
	IPTC_HANDLE h;

	/* 
		For debug, we can use this to see the detail structure info 
		when we add rule via using command "iptables ..."
	*/
	// list_redirect_rule_forward();

	e = calloc(1, sizeof(struct ipt_entry));
	if (!e) {
		return -1;
	}
	e->ip.proto = proto;
	if(proto == IPPROTO_TCP)
	{
		match = get_tcp_match(eport);
	}
	else
	{
		match = get_udp_match(eport);
	}
	if (!match) {
		free(e);
		return -1;
	}

	// NFC_IP_DST_PT: destination port, define in /include/linux/netfilter_ipv4.h
	e->nfcache = NFC_IP_DST_PT;
	// Chun add: for CD_ROUTER
	//TODO: use better way to check if eaddr is valid
	/*
	stelen(eaddr) > 3
	It's due to when eaddr has no value, ex: desc//ext_port/proto/...
	whithin the function we to get each value get_element() in upnpredirect.c
	when no valuem, I force to set it to "" <===  *(va_arg(ap, char **)) = "";
	*/
	if((eaddr) && (strlen(eaddr) > 3)) // in case eaddr read from lease_file is empty
	{
		e->ip.src.s_addr = inet_addr(eaddr);
		e->ip.smsk.s_addr = INADDR_NONE;
	}
	/*	Date:	2009-07-08
	*	Name:	jimmy huang
	*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
	*	Note:	
	*			1. only test with libipt iptables-1.3.3
	*			2. used for "match" part of iptable sturcture
	*				-d lan_pc_ip/lan_netmask (-d 192.168.0.180 / 255.255.255.255)
	*/
	//
#ifdef HAIRPIN_SUPPORT
	if(last_ext_ip_addr && (strlen(last_ext_ip_addr) > 3)){
		// -d lan_pc_ip/lan_netmask (-d 192.168.0.180 / 255.255.255.255)
		e->ip.dst.s_addr = inet_addr(last_ext_ip_addr);
		e->ip.dmsk.s_addr = inet_addr("255.255.255.255");
	}
#endif
	//----------
	target = get_dnat_target(iaddr, iport);
	if (!target) {
		free(match);
		free(e);
		return -1;
	}
	e->nfcache |= NFC_UNKNOWN;
	new_e = realloc(e, sizeof(struct ipt_entry)
	               + match->u.match_size
				   + target->u.target_size);
	if (!new_e) {
		free(target);
		free(match);
		free(e);
		return -1;
	}
	e = new_e;
	memcpy(e->elems, match, match->u.match_size);
	memcpy(e->elems + match->u.match_size, target, target->u.target_size);
	e->target_offset = sizeof(struct ipt_entry)
	                   + match->u.match_size;
	e->next_offset = sizeof(struct ipt_entry)
	                 + match->u.match_size
					 + target->u.target_size;
	
	r = iptc_init_verify_and_append("nat", miniupnpd_nat_chain, e, "addnatrule()");
	free(target);
	free(match);
	free(e);

	/*	Date:	2009-07-08
	*	Name:	jimmy huang
	*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
	*	Note:	
	*			1. only test with libipt iptables-1.3.3
	*			2. Add rules to POSTROUTING

format: iptables -I POSTROUTING -p TCP -s lan_ipaddr/lan_netmask -d lan_pc_ip --dport private_port_range_1 -j SNAT --to wan_ipaddr
ex: iptables -I POSTROUTING -s 192.168.0.0/255.255.255.0 -d 192.168.0.180 -p tcp -m tcp --dport 3333 -j SNAT --to-source 172.12.33.142
		struct ipt_entry : include/linux/netfilter_ipv4/ip_tables.h
		struct ipt_entry : -s 192.168.0.0/255.255.255.0 -d 192.168.0.180 -p tcp -m tcp
		struct ipt_entry_match : -d 192.168.0.180 -m --state NEW
		struct ipt_entry_target: -j SNAT --to wan_ipaddr
	*/
#ifdef HAIRPIN_SUPPORT
	e = calloc(1, sizeof(struct ipt_entry));
	if (!e) {
		return -1;
	}
	e->ip.proto = proto;
	if(proto == IPPROTO_TCP)
	{
		match = get_tcp_match(iport);
	}
	else
	{
		match = get_udp_match(iport);
	}
	if (!match) {
		free(e);
		return -1;
	}
	
	// NFC_IP_DST_PT: destination port, define in /include/linux/netfilter_ipv4.h
	e->nfcache = NFC_IP_DST_PT;
	
	e->ip.src.s_addr = lan_addr[0].addr.s_addr & lan_addr[0].mask.s_addr; //
	e->ip.smsk.s_addr = lan_addr[0].mask.s_addr;

	// -d lan_pc_ip/lan_netmask (-d 192.168.0.180 / 255.255.255.255)
	e->ip.dst.s_addr = inet_addr(iaddr);
	e->ip.dmsk.s_addr = inet_addr("255.255.255.255");
	
	//-j SNAT --to wan_ipaddr
	target = get_snat_target(last_ext_ip_addr, 0);
	if (!target) {
		free(match);
		free(e);
		return -1;
	}
	e->nfcache |= NFC_UNKNOWN;
	new_e = realloc(e, sizeof(struct ipt_entry)
	               + match->u.match_size
				   + target->u.target_size);
	if (!new_e) {
		free(target);
		free(match);
		free(e);
		return -1;
	}
	e = new_e;
		
	memcpy(e->elems, match, match->u.match_size);
	memcpy(e->elems + match->u.match_size, target, target->u.target_size);
	e->target_offset = sizeof(struct ipt_entry)
	                   + match->u.match_size;
	e->next_offset = sizeof(struct ipt_entry)
	                 + match->u.match_size
					 + target->u.target_size;
	r=0;
	r = iptc_init_verify_and_insert("nat", "POSTROUTING", e, "addnatrule()");
	if(r != 0){
		syslog(LOG_ERR, "miniupnpd : %s, iptc_init_verify_and_insert failed !\n",__FUNCTION__);
	}

	free(target);
	free(match);
	free(e);
#endif

	/*
	 * Gareth Williams Ubicom Inc. 11/Dec/2009.
	 * If this is the first rule added then we need to add the jump from the POSTROUTING chain to this table.
	 */
	h = iptc_init("nat");
	if (!h) {
		syslog(LOG_ERR, "miniupnpd : %s jump to table in postrouting failed !\n", __FUNCTION__);
	} else {
		unsigned int num_rules;
#ifdef IPTABLES_143
		for (num_rules = 0, e = iptc_first_rule(miniupnpd_nat_chain, h); e; e = iptc_next_rule(e, h))
#else
		for (num_rules = 0, e = iptc_first_rule(miniupnpd_nat_chain, &h); e; e = iptc_next_rule(e, &h))
#endif
		{
			num_rules++;
		}
		if (num_rules != 1) {
			syslog(LOG_DEBUG, "miniupnpd : linked to nat miniupnpd table already added\n");
		} else {
			syslog(LOG_INFO, "miniupnpd : link to nat miniupnpd table added\n");
			_system("iptables -t nat -I PREROUTING 1 -j %s", miniupnpd_nat_chain);	/* Create it */
		}
#ifdef IPTABLES_143
		iptc_free(h);
#else
		iptc_free(&h);
#endif
	}

	return r;
}
/* ================================= */
static struct ipt_entry_target *
get_accept_target(void)
{
	struct ipt_entry_target * target = NULL;
	size_t size;
	size =   IPT_ALIGN(sizeof(struct ipt_entry_target))
	       + IPT_ALIGN(sizeof(int));
	target = calloc(1, size);
	if (!target) {
		return NULL;
	}
	target->u.user.target_size = size;
	strncpy(target->u.user.name, "ACCEPT", IPT_FUNCTION_MAXNAMELEN);
	return target;
}

/* add_filter_rule()
 * */
int
add_filter_rule(int proto, const char * iaddr, unsigned short iport)
{
	int r = 0;
	struct ipt_entry *e;
	struct ipt_entry *new_e;
	struct ipt_entry_match *match = NULL;
	struct ipt_entry_target *target = NULL;
	IPTC_HANDLE h;

#ifdef HAIRPIN_SUPPORT
	struct ipt_entry_match *match_state = NULL;
#endif

	DEBUG_MSG("\n%s (%d): begin \n",__FUNCTION__,__LINE__);

	/*	Date:	2009-07-08
	*	Name:	jimmy huang
	*	Reason:	To add match rules -o br0 for filter FORWARD
	*			And "-m --state NEW"
	*	Note:	
	*			1. only test with libipt iptables-1.3.3
	*			2. Within define "HAIRPIN_SUPPORT"
	*/
#ifdef HAIRPIN_SUPPORT
	e = calloc(1, sizeof(struct ipt_entry));
	if (!e) {
		return -1;
	}
	e->ip.proto = proto;
	if(proto == IPPROTO_TCP)
	{
		match = get_tcp_match(iport);
	}
	else
	{
		match = get_udp_match(iport);
	}
	if (!match) {
		free(e);
		return -1;
	}

	// NFC_IP_DST_PT: destination port, define in /include/linux/netfilter_ipv4.h
	e->nfcache = NFC_IP_DST_PT;
	e->ip.dst.s_addr = inet_addr(iaddr);
	e->ip.dmsk.s_addr = INADDR_NONE;
	target = get_accept_target();
	if (!target) {
		free(match);
		free(e);
		return -1;
	}
	e->nfcache |= NFC_UNKNOWN;

	if(int_if_name){
		// To add match rules -o br0 for filter FORWARD
		strncpy(e->ip.outiface , int_if_name,IFNAMSIZ); // -o br0
		memset(e->ip.outiface_mask , 0xff , sizeof(e->ip.outiface_mask)); //must be set, or libipt will ignore ip.outiface
	}
	// To add match rules And "-m --state NEW"
	match_state = get_state_match();
	if (!match_state) {
		free(target);
		free(match);
		free(e);
		return -1;
	}
	new_e = realloc(e, sizeof(struct ipt_entry)
	               + match->u.match_size
				   + match_state->u.match_size
				   + target->u.target_size);
	if (!new_e) {
		free(match_state);
		free(target);
		free(match);
		free(e);
		return -1;
	}
	e = new_e;
	memcpy(e->elems, match, match->u.match_size);

	// To add match rules And "-m --state NEW"
	memcpy(e->elems + match->u.match_size , match_state, match_state->u.match_size);
	
	memcpy(e->elems + match->u.match_size + match_state->u.match_size, target, target->u.target_size);
	e->target_offset = sizeof(struct ipt_entry)
	                   + match->u.match_size
					   + match_state->u.match_size
					   ;

	e->next_offset = sizeof(struct ipt_entry)
	                 + match->u.match_size
					 + match_state->u.match_size
					 + target->u.target_size;
#else
	e = calloc(1, sizeof(struct ipt_entry));
	if (!e) {
		return -1;
	}
	e->ip.proto = proto;
	if(proto == IPPROTO_TCP)
	{
		match = get_tcp_match(iport);
	}
	else
	{
		match = get_udp_match(iport);
	}
	if (!match) {
		free(e);
		return -1;
	}

	e->nfcache = NFC_IP_DST_PT;
	e->ip.dst.s_addr = inet_addr(iaddr);
	e->ip.dmsk.s_addr = INADDR_NONE;
	target = get_accept_target();
	if (!target) {
		free(match);
		free(e);
		return -1;
	}
	e->nfcache |= NFC_UNKNOWN;

	new_e = realloc(e, sizeof(struct ipt_entry)
	               + match->u.match_size
				   + target->u.target_size);
	if (!new_e) {
		free(target);
		free(match);
		free(e);
		return -1;
	}
	e = new_e;

	memcpy(e->elems, match, match->u.match_size);
	memcpy(e->elems + match->u.match_size, target, target->u.target_size);

	e->target_offset = sizeof(struct ipt_entry)
	                   + match->u.match_size;

	e->next_offset = sizeof(struct ipt_entry)
	                 + match->u.match_size
					 + target->u.target_size;

#endif
	r = iptc_init_verify_and_append("filter", miniupnpd_forward_chain, e, "add_filter_rule()");

	/*
	 * Gareth Williams Ubicom Inc. 11/Dec/2009.
	 * If this is the first rule added then we need to add the jump from the FORWARD chain to this table.
	 */
	h = iptc_init("filter");
	if (!h) {
		syslog(LOG_ERR, "miniupnpd : %s jump to table in forward failed !\n", __FUNCTION__);
	} else {
		unsigned int num_rules;
#ifdef IPTABLES_143
		for (num_rules = 0, e = iptc_first_rule(miniupnpd_forward_chain, h); e; e = iptc_next_rule(e, h))
#else
		for (num_rules = 0, e = iptc_first_rule(miniupnpd_forward_chain, &h); e; e = iptc_next_rule(e, &h))
#endif
		{
			num_rules++;
		}
		if (num_rules != 1) {
			syslog(LOG_DEBUG, "miniupnpd : linked to filter miniupnpd table already added\n");
		} else {
			syslog(LOG_INFO, "miniupnpd : link to filter miniupnpd table added\n");
			_system("iptables -t filter -I FORWARD 1 -j %s", miniupnpd_forward_chain);	/* Create it */
		}
#ifdef IPTABLES_143
		iptc_free(h);
#else
		iptc_free(&h);
#endif
	}

	free(target);
	free(match);
#ifdef HAIRPIN_SUPPORT
	free(match_state);
#endif
	free(e);
	DEBUG_MSG("%s (%d): end, return r = %d\n",__FUNCTION__,__LINE__,r);
	return r;
}

/* ================================ */
static int
print_match(const struct ipt_entry_match *match)
{
	printf("match %s\n", match->u.user.name);
	if(0 == strncmp(match->u.user.name, "tcp", IPT_FUNCTION_MAXNAMELEN))
	{
		struct ipt_tcp * tcpinfo;
		tcpinfo = (struct ipt_tcp *)match->data;
		printf("srcport = %hu:%hu dstport = %hu:%hu\n",
		       tcpinfo->spts[0], tcpinfo->spts[1],
			   tcpinfo->dpts[0], tcpinfo->dpts[1]);
	}
	else if(0 == strncmp(match->u.user.name, "udp", IPT_FUNCTION_MAXNAMELEN))
	{
		struct ipt_udp * udpinfo;
		udpinfo = (struct ipt_udp *)match->data;
		printf("srcport = %hu:%hu dstport = %hu:%hu\n",
		       udpinfo->spts[0], udpinfo->spts[1],
			   udpinfo->dpts[0], udpinfo->dpts[1]);
	}
	return 0;
}

static void
print_iface(const char * iface, const unsigned char * mask, int invert)
{
	unsigned i;
	if(mask[0] == 0)
		return;
	if(invert)
		printf("! ");
	for(i=0; i<IFNAMSIZ; i++)
	{
		if(mask[i])
		{
			if(iface[i])
				putchar(iface[i]);
		}
		else
		{
			if(iface[i-1])
				putchar('+');
			break;
		}
	}
}

static void
printip(uint32_t ip)
{
	printf("%u.%u.%u.%u", ip >> 24, (ip >> 16) & 0xff,
	       (ip >> 8) & 0xff, ip & 0xff);
}

/* for debug */
/* read the "filter" and "nat" tables */
int
list_redirect_rule(const char * ifname)
{
	IPTC_HANDLE h;
	const struct ipt_entry * e;
	const struct ipt_entry_target * target;
	const struct ip_nat_multi_range * mr;
	const char * target_str;

	h = iptc_init("nat");
	if(!h)
	{
		printf("iptc_init() error : %s\n", iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain(miniupnpd_nat_chain, h))
	{
		printf("chain %s not found\n", miniupnpd_nat_chain);
		return -1;
	}
#ifdef IPTABLES_143
	for(e = iptc_first_rule(miniupnpd_nat_chain, h);
		e;
		e = iptc_next_rule(e, h))
	{
		target_str = iptc_get_target(e, h);
#else
	for(e = iptc_first_rule(miniupnpd_nat_chain, &h);
		e;
		e = iptc_next_rule(e, &h))
	{
		target_str = iptc_get_target(e, &h);
#endif
		printf("===\n");
		printf("src = %s%s/%s\n", (e->ip.invflags & IPT_INV_SRCIP)?"! ":"",
		       inet_ntoa(e->ip.src), inet_ntoa(e->ip.smsk));
		printf("dst = %s%s/%s\n", (e->ip.invflags & IPT_INV_DSTIP)?"! ":"",
		       inet_ntoa(e->ip.dst), inet_ntoa(e->ip.dmsk));
		/*printf("in_if = %s  out_if = %s\n", e->ip.iniface, e->ip.outiface);*/
		printf("in_if = ");
		print_iface(e->ip.iniface, e->ip.iniface_mask,
		            e->ip.invflags & IPT_INV_VIA_IN);
		printf(" out_if = ");
		print_iface(e->ip.outiface, e->ip.outiface_mask,
		            e->ip.invflags & IPT_INV_VIA_OUT);
		printf("\n");
		printf("ip.proto = %s%d\n", (e->ip.invflags & IPT_INV_PROTO)?"! ":"",
		       e->ip.proto);
		/* display matches stuff */
		if(e->target_offset)
		{
			IPT_MATCH_ITERATE(e, print_match);
			/*printf("\n");*/
		}
		printf("target = %s\n", target_str);
		target = (void *)e + e->target_offset;
		mr = (const struct ip_nat_multi_range *)&target->data[0];
		printf("ips ");
		printip(ntohl(mr->range[0].min_ip));
		printf(" ");
		printip(ntohl(mr->range[0].max_ip));
		printf("\nports %hu %hu\n", ntohs(mr->range[0].min.all),
		          ntohs(mr->range[0].max.all));
		printf("flags = %x\n", mr->range[0].flags);
	}
#ifdef IPTABLES_143
	iptc_free(h);
#else
	iptc_free(&h);
#endif
	return 0;
}

/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
*	Note:	
*			1. only test with libipt iptables-1.3.3
*			2. This function is for debug used, list all rules added in filter table, forward chain
*/
int
list_redirect_rule_forward(void)
{
	IPTC_HANDLE h;
	const struct ipt_entry * e;
	const struct ipt_entry_target * target;
	const struct ip_nat_multi_range * mr;
	const char * target_str;
	printf(" List all rules in filter table, FORWARD chain \n");
	h = iptc_init("filter");
	if(!h)
	{
		printf("iptc_init() error : %s\n", iptc_strerror(errno));
		return -1;
	}
	if(!iptc_is_chain("FORWARD", h))
	{
		printf("chain %s not found\n", "FORWARD");
		return -1;
	}
#ifdef IPTABLES_143
	for(e = iptc_first_rule("FORWARD", h);
		e;
		e = iptc_next_rule(e, h))
	{
		target_str = iptc_get_target(e, h);
#else
	for(e = iptc_first_rule("FORWARD", &h);
		e;
		e = iptc_next_rule(e, &h))
	{
		target_str = iptc_get_target(e, &h);
#endif
		printf("===================================\n");
		printf("src = %s%s/%s\n", (e->ip.invflags & IPT_INV_SRCIP)?"! ":"",
		       inet_ntoa(e->ip.src), inet_ntoa(e->ip.smsk));
		printf("dst = %s%s/%s\n", (e->ip.invflags & IPT_INV_DSTIP)?"! ":"",
		       inet_ntoa(e->ip.dst), inet_ntoa(e->ip.dmsk));
		/*printf("in_if = %s  out_if = %s\n", e->ip.iniface, e->ip.outiface);*/
		printf("in_if = ");
		print_iface(e->ip.iniface, e->ip.iniface_mask,
		            e->ip.invflags & IPT_INV_VIA_IN);
		printf("   iniface_mask = %s \n",e->ip.iniface_mask);
		printf(" out_if = ");
		print_iface(e->ip.outiface, e->ip.outiface_mask,
		            e->ip.invflags & IPT_INV_VIA_OUT);
		printf("   outiface_mask = %s \n",e->ip.outiface_mask);
		printf("\n");
		printf("ip.proto = %s%d\n", (e->ip.invflags & IPT_INV_PROTO)?"! ":"",
		       e->ip.proto);
		/* display matches stuff */
		if(e->target_offset)
		{
			IPT_MATCH_ITERATE(e, print_match);
			/*printf("\n");*/
		}
		printf("target = %s\n", target_str);
		target = (void *)e + e->target_offset;
		mr = (const struct ip_nat_multi_range *)&target->data[0];
		printf("ips ");
		printip(ntohl(mr->range[0].min_ip));
		printf(" ");
		printip(ntohl(mr->range[0].max_ip));
		printf("\nports %hu %hu\n", ntohs(mr->range[0].min.all),
		          ntohs(mr->range[0].max.all));
		printf("flags = %x\n", mr->range[0].flags);
	}
#ifdef IPTABLES_143
	iptc_free(h);
#else
	iptc_free(&h);
#endif
	printf("\n\n");
	return 0;
}


/*	Date:	2009-04-20
*	Name:	jimmy huang
*	Reason:	For remove expired portfoward mapping rule
*/
#ifdef ENABLE_LEASEFILE
extern int lease_file_remove( unsigned short eport, int proto);//upnpredirect.c
#endif

void ScanAndCleanPortMappingExpiration(void){
	struct rdr_desc * current = NULL;
	struct rdr_desc * previous = NULL;
	time_t current_time;
	int changed = 0;
	
	DEBUG_MSG("\n%s, %s (%d): begin\n",__FILE__,__FUNCTION__,__LINE__);
	
	current_time = time(NULL);
	DEBUG_MSG("%s (%d), current_time = %ld\n",__FUNCTION__,__LINE__,current_time);
	
	for(current = rdr_desc_list, previous = rdr_desc_list; current; current = current->next)
	{
		DEBUG_MSG("\n");
		DEBUG_MSG("%s (%d), RemoteHost = %s\n",__FUNCTION__,__LINE__
							,current->RemoteHost ? current->RemoteHost : "");
		DEBUG_MSG("%s (%d), ExternalPort = %hu\n",__FUNCTION__,__LINE__,current->eport);
		DEBUG_MSG("%s (%d), Protocol = %s (%d)\n",__FUNCTION__,__LINE__
							,current->proto == IPPROTO_TCP ? "TCP" : 
								(current->proto == IPPROTO_UDP ? "UDP" : "Unknown Proto")
							,current->proto );
		DEBUG_MSG("%s (%d), InternalPort = %hu\n",__FUNCTION__,__LINE__,current->InternalPort);
		DEBUG_MSG("%s (%d), InternalClient = %s\n",__FUNCTION__,__LINE__
							,current->InternalClient ? current->InternalClient : "");
		//DEBUG_MSG("%s (%d), Enabled = %d\n",__FUNCTION__,__LINE__,Enabled);
		DEBUG_MSG("%s (%d), PortMappingDesc = %s\n",__FUNCTION__,__LINE__
							,current->str ? current->str : "");
		DEBUG_MSG("%s (%d), LeaseDuration = %ld\n",__FUNCTION__,__LINE__,current->LeaseDuration);
		DEBUG_MSG("%s (%d), expire_time = %ld\n",__FUNCTION__,__LINE__,current->expire_time);
		if((current->LeaseDuration != 0) && (current->expire_time <= current_time)){
			DEBUG_MSG("%s (%d): expire !\n",__FUNCTION__,__LINE__);
			changed = 1;
		// remove iptable rules
			delete_redirect_and_filter_rules(current->eport, current->proto);
			
#ifdef ENABLE_LEASEFILE
		// remove lease files
			lease_file_remove( current->eport, current->proto);
#endif
		// remove rdr_desc_list 
			if(previous == rdr_desc_list){
				//This is the first record in link list
				rdr_desc_list = rdr_desc_list->next;
				free(current);
			}else{
				previous->next = current->next;
				free(current);
			}
		}else{
			previous = current;
		}
	}
	// Send Notify Event
#ifdef ENABLE_EVENTS
	if(changed){
		DEBUG_MSG("%s, %s (%d): call upnp_event_var_change_notify()\n",__FILE__,__FUNCTION__,__LINE__);
		upnp_event_var_change_notify(EWanIPC);
	}
#endif
}

// -------------






