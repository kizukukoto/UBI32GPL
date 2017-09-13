/* $Id: iptcrdr.c,v 1.32 2010/03/03 11:23:52 nanard Exp $ */
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
#include <libiptc/libiptc.h>
#include <libiptc/libip6tc.h> // IPv6 Add
#include <iptables.h>

#include <linux/version.h>

#if IPTABLES_143
/* IPTABLES API version >= 1.4.3 */
#include <linux/netfilter_ipv6/ip6_tables.h>
#include <net/netfilter/nf_nat.h>
#define ip_nat_multi_range	nf_nat_multi_range
#define ip_nat_range		nf_nat_range
#define IP6TC_HANDLE		struct ip6tc_handle *
#else
/* ip6tABLES API version < 1.4.3 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
#include <linux/netfilter_ipv4/ip_nat.h> // IPv6 modification
#else
#include <linux/netfilter/nf_nat.h>
#endif
#define IP6TC_HANDLE		ip6tc_handle_t
#endif

#include "iptcrdr.h"
#include "../upnpglobalvars.h"

union ip6_conntrack_manip_proto
{
	u_int16_t all;
	struct {
		u_int16_t port;
	} tcp;
	struct {
		u_int16_t port;
	} udp;
	struct {
		u_int16_t port;
	} icmp;
	struct {
		u_int16_t port;
	} sctp;
};

struct
ip6_entry_target
{
	union {
		struct {
			u_int16_t target_size;
			char name[IP6T_FUNCTION_MAXNAMELEN];
		} user;
		struct {
			u_int16_t target_size;
			struct ip6t_target * target;
		} kernel;
		u_int16_t target_size;
	} u;
	unsigned char data[0];
};
	
struct
ip6_entry_match
{
	union {
		struct {
			u_int16_t match_size;
			char name[IP6T_FUNCTION_MAXNAMELEN];
		} user;
		struct {
			u_int16_t match_size;
			struct ip6t_match * target;
		} kernel;
		u_int16_t match_size;
	} u;
	unsigned char data[0];
};

struct
ip6_nat_range
{
	unsigned int flags;
	struct in6_addr min_ip, max_ip;
	union ip6_conntrack_manip_proto min, max;
};

struct
ip6_nat_multi_range
{
	unsigned int rangesize;
	struct ip6_nat_range range[1];
};

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
static int snprintip(char * dst, size_t size, struct in6_addr ip)
{
	char addr[INET6_ADDRSTRLEN]=""; // IPv6 Add
	inet_ntop(AF_INET6, &ip, addr, INET6_ADDRSTRLEN); // IPv6 Add
	//fprintf(stderr, "ipt_snprint received ntop request for the address: %s", addr);
	return snprintf(dst, size, "%s", addr); // IPv6 Modification
}

/* netfilter cannot store redirection descriptions, so we use our
 * own structure to store them */
struct rdr_desc {
	struct rdr_desc * next;
	unsigned short eport;
	short proto;
	char str[];
};

/* pointer to the chained list where descriptions are stored */
static struct rdr_desc * rdr_desc_list = 0;

static void
add_redirect_desc(unsigned short eport, int proto, const char * desc)
{
	struct rdr_desc * p;
	size_t l;
	if(desc)
	{
		l = strlen(desc) + 1;
		p = malloc(sizeof(struct rdr_desc) + l);
		if(p)
		{
			p->next = rdr_desc_list;
			p->eport = eport;
			p->proto = (short)proto;
			memcpy(p->str, desc, l);
			rdr_desc_list = p;
		}
	}
}

static void
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

static void
get_redirect_desc(unsigned short eport, int proto,
                  char * desc, int desclen)
{
	struct rdr_desc * p;
	if(!desc || (desclen == 0))
		return;
	for(p = rdr_desc_list; p; p = p->next)
	{
		if(p->eport == eport && p->proto == (short)proto)
		{
			strncpy(desc, p->str, desclen);
			return;
		}
	}
	/* if no description was found, return miniupnpd as default */
	strncpy(desc, "miniupnpd", desclen);
}

/* add_redirect_rule2() */
int
add_redirect_rule2(const char * ifname, unsigned short eport,
                   const char * iaddr, unsigned short iport, int proto,
				   const char * desc)	
{
	fprintf(stderr, "add_redirect_rule2: proto=%d, eport=%hu, addr=%s, iport=%hu, desc=%s\n", proto, eport, iaddr, iport, desc);
	int r = addnatrule(proto, eport, iaddr, iport);
	//int r = 1;
	if(r >= 0)
		add_redirect_desc(eport, proto, desc);
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
                  u_int64_t * packets, u_int64_t * bytes)
{
	int r = -1;
	IP6TC_HANDLE h; // IPv6 Modification
	const struct ip6t_entry * e; // IPv6 Modification
	const struct ip6t_entry_target * target; // IPv6 Modification
	const struct ip6_nat_multi_range * mr; // IPv6 modification
	const struct ip6t_entry_match *match; // IPv6 Modification

	h = ip6tc_init("filter"); // IPv6 Modification **mangle/nat
	if(!h)
	{
		syslog(LOG_ERR, "get_redirect_rule() : "
		                "ip6tc_init() failed : %s",
		       ip6tc_strerror(errno));
		return -1;
	}
	if(!ip6tc_is_chain(miniupnpd_nat_chain, h)) // IPv6 Modification
	{
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = ip6tc_first_rule(miniupnpd_nat_chain, h); // IPv6 Modification
		    e;
			e = ip6tc_next_rule(e, h)) // IPv6 Modification
#else
		for(e = ip6tc_first_rule(miniupnpd_nat_chain, &h); // IPv6 Modification
		    e;
			e = ip6tc_next_rule(e, &h)) // IPv6 Modification
#endif
		{
			if(proto==e->ipv6.proto) // IPv6 Modification
			{
				match = (const struct ip6t_entry_match *)&e->elems; // IPv6 modification
				if(0 == strncmp(match->u.user.name, "tcp", IP6T_FUNCTION_MAXNAMELEN)) // IPv6 modification
				{
					const struct ip6t_tcp * info; // IPv6 Modification
					info = (const struct ip6t_tcp *)match->data; // IPv6 Modification
					if(eport != info->dpts[0])
						continue;
				}
				else
				{
					const struct ip6t_udp * info; // IPv6 Modification
					info = (const struct ip6t_udp *)match->data; // IPv6 Modification
					if(eport != info->dpts[0])
						continue;
				}
				target = (void *)e + e->target_offset;
				//target = ip6t_get_target(e);
				mr = (const struct ip6_nat_multi_range *)&target->data[0];
				snprintip(iaddr, iaddrlen, mr->range[0].min_ip); // IPv6 Modification
				*iport = ntohs(mr->range[0].min.all);
				/*if(desc)
					strncpy(desc, "miniupnpd", desclen);*/
				get_redirect_desc(eport, proto, desc, desclen);
				if(packets)
					*packets = e->counters.pcnt;
				if(bytes)
					*bytes = e->counters.bcnt;
				r = 0;
				break;
			}
		}
	}
	if(h)
#ifdef IPTABLES_143
		ip6tc_free(h); // IPv6 Modification
#else
		ip6tc_free(&h); // IPv6 Modification
#endif
	return r;
}

/* get_redirect_rule_by_index() 
 * return -1 when the rule was not found */
int
get_redirect_rule_by_index(int index,
                           char * ifname, unsigned short * eport,
                           char * iaddr, int iaddrlen, unsigned short * iport,
                           int * proto, char * desc, int desclen,
                           u_int64_t * packets, u_int64_t * bytes)
{
	int r = -1;
	int i = 0;
	IP6TC_HANDLE h; // IPv6 modification
	const struct ip6t_entry * e; // IPv6 modification
	const struct ip6t_entry_target * target; // IPv6 modification
	const struct ip6_nat_multi_range * mr; // IPv6 modification
	const struct ip6t_entry_match *match; // IPv6 modification

	h = ip6tc_init("filter"); // IPv6 modification **mangle/nat
	if(!h)
	{
		syslog(LOG_ERR, "get_redirect_rule_by_index() : "
		                "ip6tc_init() failed : %s", // IPv6 modification
		       ip6tc_strerror(errno)); // IPv6 modification
		return -1;
	}
	if(!ip6tc_is_chain(miniupnpd_nat_chain, h)) // IPv6 modification
	{
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = ip6tc_first_rule(miniupnpd_nat_chain, h); // IPv6 modification
		    e;
			e = ip6tc_next_rule(e, h)) // IPv6 modification
#else
		for(e = ip6tc_first_rule(miniupnpd_nat_chain, &h); // IPv6 modification
		    e;
			e = ip6tc_next_rule(e, &h)) // IPv6 modification
#endif
		{
			if(i==index)
			{
				*proto = e->ipv6.proto;
				match = (const struct ip6t_entry_match *)&e->elems; // IPv6 modification
				if(0 == strncmp(match->u.user.name, "tcp", IP6T_FUNCTION_MAXNAMELEN)) // IPv6 modification
				{
					const struct ip6t_tcp * info; // IPv6 modification
					info = (const struct ip6t_tcp *)match->data; // IPv6 modification
					*eport = info->dpts[0];
				}
				else
				{
					const struct ip6t_udp * info; // IPv6 modification
					info = (const struct ip6t_udp *)match->data; // IPv6 modification
					*eport = info->dpts[0];
				}
				target = (void *)e + e->target_offset;
				mr = (const struct ip6_nat_multi_range *)&target->data[0];
				snprintip(iaddr, iaddrlen, mr->range[0].min_ip); // IPv6 modification
				*iport = ntohs(mr->range[0].min.all);
                /*if(desc)
				    strncpy(desc, "miniupnpd", desclen);*/
				get_redirect_desc(*eport, *proto, desc, desclen);
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
	if(h)
#ifdef IPTABLES_143
		ip6tc_free(h); // IPv6 modification
#else
		ip6tc_free(&h); // IPv6 modification
#endif
	return r;
}

/* delete_rule_and_commit() :
 * subfunction used in delete_redirect_and_filter_rules() */
static int
delete_rule_and_commit(unsigned int index, IP6TC_HANDLE h, // IPv6 modification
                       const char * miniupnpd_chain,
                       const char * logcaller)
{
	int r = 0;
#ifdef IPTABLES_143
	if(!ip6tc_delete_num_entry(miniupnpd_chain, index, h)) // IPv6 modification
#else
	if(!ip6tc_delete_num_entry(miniupnpd_chain, index, &h)) // IPv6 modification
#endif
	{
		syslog(LOG_ERR, "%s() : ip6tc_delete_num_entry(): %s\n", // IPv6 modification
	    	   logcaller, ip6tc_strerror(errno)); // IPv6 modification
		r = -1;
	}
#ifdef IPTABLES_143
	else if(!ip6tc_commit(h)) // IPv6 modification
#else
	else if(!ip6tc_commit(&h)) // IPv6 modification
#endif
	{
		syslog(LOG_ERR, "%s() : ip6tc_commit(): %s\n", // IPv6 modification
	    	   logcaller, ip6tc_strerror(errno)); // IPv6 modification
		r = -1;
	}
	if(h)
#ifdef IPTABLES_143
		ip6tc_free(h); // IPv6 modification
#else
		ip6tc_free(&h); // IPv6 modification
#endif
	return r;
}

/* delete_redirect_and_filter_rules()
 */
int
delete_redirect_and_filter_rules(unsigned short eport, int proto)
{
	int r = -1;
	unsigned index = 0;
	unsigned i = 0;
	IP6TC_HANDLE h; // IPv6 modification
	const struct ip6t_entry * e; // IPv6 modification
	const struct ip6t_entry_match *match; // IPv6 modification

	h = ip6tc_init("filter"); // IPv6 modification **mangle/nat
	if(!h)
	{
		syslog(LOG_ERR, "delete_redirect_and_filter_rules() : "
		                "ip6tc_init() failed : %s", // IPv6 modification
		       ip6tc_strerror(errno)); // IPv6 modification
		return -1;
	}
	if(!ip6tc_is_chain(miniupnpd_nat_chain, h)) // IPv6 modification
	{
		syslog(LOG_ERR, "chain %s not found", miniupnpd_nat_chain);
	}
	else
	{
#ifdef IPTABLES_143
		for(e = ip6tc_first_rule(miniupnpd_nat_chain, h); // IPv6 modification
		    e;
			e = ip6tc_next_rule(e, h), i++) // IPv6 modification
#else
		for(e = ip6tc_first_rule(miniupnpd_nat_chain, &h); // IPv6 modification
		    e;
			e = ip6tc_next_rule(e, &h), i++) // IPv6 modification
#endif
		{
			if(proto==e->ipv6.proto)
			{
				match = (const struct ip6t_entry_match *)&e->elems; // IPv6 modification
				if(0 == strncmp(match->u.user.name, "tcp", IP6T_FUNCTION_MAXNAMELEN)) // IPv6 modification
				{
					const struct ip6t_tcp * info; // IPv6 modification
					info = (const struct ip6t_tcp *)match->data; // IPv6 modification
					if(eport != info->dpts[0])
						continue;
				}
				else
				{
					const struct ip6t_udp * info; // IPv6 modification
					info = (const struct ip6t_udp *)match->data; // IPv6 modification
					if(eport != info->dpts[0])
						continue;
				}
				index = i;
				r = 0;
				break;
			}
		}
	}
	if(h)
#ifdef IPTABLES_143
		ip6tc_free(h); // IPv6 modification
#else
		ip6tc_free(&h); // IPv6 modification
#endif
	if(r == 0)
	{
		syslog(LOG_INFO, "Trying to delete rules at index %u", index);
		/* Now delete both rules */
		h = ip6tc_init("filter"); // IPv6 modification **mangle/nat
		if(h)
		{
			r = delete_rule_and_commit(index, h, miniupnpd_nat_chain, "delete_redirect_rule");
		}
		/*if((r == 0) && (h = ip6tc_init("filter"))) // IPv6 modification
		{
			r = delete_rule_and_commit(index, h, miniupnpd_forward_chain, "delete_filter_rule");
		}*/
	}
	del_redirect_desc(eport, proto);
	return r;
}

/* ==================================== */
/* TODO : add the -m state --state NEW,ESTABLISHED,RELATED 
 * only for the filter rule */
static struct ip6t_entry_match *
get_tcp_match(unsigned short dport) // IPv6 modification
{
	struct ip6t_entry_match *match; // IPv6 modification
	struct ip6t_tcp * tcpinfo; // IPv6 modification
	size_t size;
	size =   IP6T_ALIGN(sizeof(struct ip6t_entry_match)) // IPv6 modification
	       + IP6T_ALIGN(sizeof(struct ip6t_tcp)); // IPv6 modification
	match = calloc(1, size);
	match->u.match_size = size;
	strncpy(match->u.user.name, "tcp", IP6T_FUNCTION_MAXNAMELEN); // IPv6 modification
	tcpinfo = (struct ip6t_tcp *)match->data; // IPv6 modification
	tcpinfo->spts[0] = 0;		/* all source ports */
	tcpinfo->spts[1] = 0xFFFF;
	tcpinfo->dpts[0] = dport;	/* specified destination port */
	tcpinfo->dpts[1] = dport;
	return match;
}

static struct ip6t_entry_match * // IPv6 modification
get_udp_match(unsigned short dport)
{
	struct ip6t_entry_match *match; // IPv6 modification
	struct ip6t_udp * udpinfo; // IPv6 modification
	size_t size;
	size =   IP6T_ALIGN(sizeof(struct ip6t_entry_match)) // IPv6 modification
	       + IP6T_ALIGN(sizeof(struct ip6t_udp)); // IPv6 modification
	match = calloc(1, size);
	match->u.match_size = size;
	strncpy(match->u.user.name, "udp", IP6T_FUNCTION_MAXNAMELEN); // IPv6 modification
	udpinfo = (struct ip6t_udp *)match->data; // IPv6 modification
	udpinfo->spts[0] = 0;		/* all source ports */
	udpinfo->spts[1] = 0xFFFF;
	udpinfo->dpts[0] = dport;	/* specified destination port */
	udpinfo->dpts[1] = dport;
	return match;
}

static struct ip6t_entry_target *
get_dnat_target(const char * daddr, unsigned short dport) // IPv6 modification
{
	struct ip6t_entry_target * target; // IPv6 modification
	struct ip6_nat_multi_range * mr; // IPv6 modification
	struct ip6_nat_range * range; // IPv6 modification
	size_t size;

	size =   IP6T_ALIGN(sizeof(struct ip6t_entry_target)) // IPv6 modification
	       + IP6T_ALIGN(sizeof(struct ip6_nat_multi_range)); // IPv6 modification
	target = calloc(1, size);
	target->u.target_size = size;
	strncpy(target->u.user.name, "DNAT", IP6T_FUNCTION_MAXNAMELEN); // IPv6 modification
	/* one ip_nat_range already included in ip_nat_multi_range */
	mr = (struct ip6_nat_multi_range *)&target->data[0];
	mr->rangesize = 1;
	range = &(mr->range[0]);
	inet_pton(AF_INET6, daddr, &(range->max_ip)); // IPv6 modification
	printip(range->max_ip);
	inet_pton(AF_INET6, daddr, &(range->min_ip)); // IPv6 modification
	printip(range->min_ip);
	range->flags |= IP_NAT_RANGE_MAP_IPS;
	range->min.all = range->max.all = htons(dport);
	range->flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	return target;
}

/* iptc_init_verify_and_append()
 * return 0 on success, -1 on failure */
static int
ip6tc_init_verify_and_append(const char * table, const char * miniupnpd_chain, struct ip6t_entry * e,
                            const char * logcaller) // IPv6 modification
{
	IP6TC_HANDLE h; // IPv6 modification
	h = ip6tc_init(table); // IPv6 modification
	fprintf(stderr, "Checking entry\n");
	printip(e->ipv6.dst);
	printip(e->ipv6.src);
	printentryinfos(e);
	if(!h)
	{
		syslog(LOG_ERR, "%s : ip6tc_init() error : %s\n", // IPv6 modification
		       logcaller, ip6tc_strerror(errno)); // IPv6 modification
		return -1;
	}
	//fprintf(stderr, "ip6tc_init: %s", h);
	if(!ip6tc_is_chain(miniupnpd_chain, h)) // IPv6 modification
	{
		syslog(LOG_ERR, "%s : ip6tc_is_chain() error : %s\n", // IPv6 modification
		       logcaller, ip6tc_strerror(errno)); // IPv6 modification
		if(h)
#ifdef IPTABLES_143
			ip6tc_free(h); // IPv6 modification
#else
			ip6tc_free(&h); // IPv6 modification
#endif
		return -1;
	}
	/* ip6tc_insert_entry(miniupnpd_chain, e, n, h/&h) could also be used */
#ifdef IPTABLES_143
	if(!ip6tc_append_entry(miniupnpd_chain, e, h)) // IPv6 modification
#else
	if(!ip6tc_append_entry(miniupnpd_chain, e, &h)) // IPv6 modification
#endif
	{
		syslog(LOG_ERR, "%s : ip6tc_append_entry() error : %s\n", // IPv6 modification
		       logcaller, ip6tc_strerror(errno)); // IPv6 modification
		if(h)
#ifdef IPTABLES_143
			ip6tc_free(h); // IPv6 modification
#else
			ip6tc_free(&h); // IPv6 modification
#endif
		return -1;
	}
#ifdef IPTABLES_143
	if(!ip6tc_commit(h)) // IPv6 modification
#else
	if(!ip6tc_commit(&h)) // IPv6 modification
#endif
	{
		syslog(LOG_ERR, "%s : ip6tc_commit() error : %s & err=%d\n", // IPv6 modification
		       logcaller, ip6tc_strerror(errno), errno); // IPv6 modification
		if(h)
#ifdef IPTABLES_143
			ip6tc_free(h); // IPv6 modification
#else
			ip6tc_free(&h); // IPv6 modification
#endif
		return -1;
	}
	if(h)
#ifdef IPTABLES_143
		ip6tc_free(h); // IPv6 modification
#else
		ip6tc_free(&h); // IPv6 modification
#endif
	return 0;
}

/* add nat rule 
 * ip6tables -t nat -A MINIUPNPD -p proto --dport eport -j DNAT --to iaddr:iport
 * */
int
addnatrule(int proto, unsigned short eport,
           const char * iaddr, unsigned short iport)
{
	int r = 0;
	struct ip6t_entry * e; // IPv6 modification
	struct ip6t_entry_match *match = NULL; // IPv6 modification
	struct ip6t_entry_target *target = NULL; // IPv6 modification

	e = calloc(1, sizeof(struct ip6t_entry)); // IPv6 modification
	e->ipv6.proto = proto; // IPv6 modification
	fprintf(stderr, "addnatrule: protocol=%d / eport=%d\n", e->ipv6.proto, eport);
	if(proto == IPPROTO_TCP)
	{
		match = get_tcp_match(eport);
	}
	else
	{
		match = get_udp_match(eport);
	}
	e->nfcache = NFC_IP6_DST_PT;
	fprintf(stderr, "addnatrule: address=%s\n", iaddr);
	target = get_dnat_target(iaddr, iport);
	//printf(stderr, "addnatrule: target=%s", target);
	e->nfcache |= NFC_UNKNOWN;
	e = realloc(e, sizeof(struct ip6t_entry) // IPv6 modification
	               + match->u.match_size
				   + target->u.target_size);
	memcpy(e->elems, match, match->u.match_size);
	memcpy(e->elems + match->u.match_size, target, target->u.target_size);
	e->target_offset = sizeof(struct ip6t_entry) // IPv6 modification
	                   + match->u.match_size;
	e->next_offset = sizeof(struct ip6t_entry) // IPv6 modification
	                 + match->u.match_size
					 + target->u.target_size;
	r = ip6tc_init_verify_and_append("filter", miniupnpd_nat_chain, e, "addnatrule()"); // IPv6 modification **mangle/nat
	free(target);
	free(match);
	free(e);
	return r;
}
/* ================================= */
static struct ip6t_entry_target *
get_accept_target(void) // IPv6 modification
{
	struct ip6t_entry_target * target = NULL; // IPv6 modification
	size_t size;
	size =   IP6T_ALIGN(sizeof(struct ip6t_entry_target)) // IPv6 modification
	       + IP6T_ALIGN(sizeof(int)); // IPv6 modification
	target = calloc(1, size);
	target->u.user.target_size = size;
	strncpy(target->u.user.name, "ACCEPT", IP6T_FUNCTION_MAXNAMELEN); // IPv6 modification
	return target;
}

/* add_filter_rule()
 * */
int
add_filter_rule(int proto, const char * iaddr, unsigned short iport)
{
	int r = 0;
	struct ip6t_entry * e; // IPv6 modification
	struct ip6t_entry_match *match = NULL; // IPv6 modification
	struct ip6t_entry_target *target = NULL; // IPv6 modification

	e = calloc(1, sizeof(struct ip6t_entry)); // IPv6 modification
	e->ipv6.proto = proto;
	fprintf(stderr, "add_filter_rule: protocol=%d", e->ipv6.proto);
	if(proto == IPPROTO_TCP)
	{
		match = get_tcp_match(iport);
	}
	else
	{
		match = get_udp_match(iport);
	}
	e->nfcache = NFC_IP6_DST_PT;
	inet_pton(AF_INET6, iaddr, &(e->ipv6.dst)); // IPv6 Modification
	e->ipv6.dmsk = in6addr_any; // IPv6 Modification
	target = get_accept_target();
	e->nfcache |= NFC_UNKNOWN;
	e = realloc(e, sizeof(struct ip6t_entry) // IPv6 modification
	               + match->u.match_size
				   + target->u.target_size);
	memcpy(e->elems, match, match->u.match_size);
	memcpy(e->elems + match->u.match_size, target, target->u.target_size);
	e->target_offset = sizeof(struct ip6t_entry) // IPv6 modification
	                   + match->u.match_size;
	e->next_offset = sizeof(struct ip6t_entry) // IPv6 modification
	                 + match->u.match_size
					 + target->u.target_size;
	r = ip6tc_init_verify_and_append("filter", miniupnpd_forward_chain, e, "add_filter_rule()"); // IPv6 modification
	free(target);
	free(match);
	free(e);
	return r;
}

/* ================================ */
static int
print_match(const struct ip6t_entry_match *match) // IPv6 modification
{
	printf("match %s\n", match->u.user.name);
	if(0 == strncmp(match->u.user.name, "tcp", IP6T_FUNCTION_MAXNAMELEN)) // IPv6 modification
	{
		struct ip6t_tcp * tcpinfo; // IPv6 modification
		tcpinfo = (struct ip6t_tcp *)match->data; // IPv6 modification
		printf("srcport = %hu:%hu dstport = %hu:%hu\n",
		       tcpinfo->spts[0], tcpinfo->spts[1],
			   tcpinfo->dpts[0], tcpinfo->dpts[1]);
	}
	else if(0 == strncmp(match->u.user.name, "udp", IP6T_FUNCTION_MAXNAMELEN)) // IPv6 modification
	{
		struct ip6t_udp * udpinfo; // IPv6 modification
		udpinfo = (struct ip6t_udp *)match->data; // IPv6 modification
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
printip(struct in6_addr ip)
{
	char addr[INET6_ADDRSTRLEN]=""; // IPv6 Add
	inet_ntop(AF_INET6, &ip, addr, INET6_ADDRSTRLEN); // IPv6 Add
	fprintf(stderr, "ipt_print received ntop request for the address: %s\n", addr);
	//printf("%s", addr);
}

void
printentryinfos(struct ip6t_entry *e)
{
	char dst[INET6_ADDRSTRLEN]="", src[INET6_ADDRSTRLEN]="", dmsk[INET6_ADDRSTRLEN]="", smsk[INET6_ADDRSTRLEN]=""; // IPv6 Add
	printf("===\n");
	inet_ntop(AF_INET6, &(e->ipv6.src), src, INET6_ADDRSTRLEN); // IPv6 Add
	inet_ntop(AF_INET6, &(e->ipv6.smsk), smsk, INET6_ADDRSTRLEN); // IPv6 Add
	inet_ntop(AF_INET6, &(e->ipv6.dst), dst, INET6_ADDRSTRLEN); // IPv6 Add
	inet_ntop(AF_INET6, &(e->ipv6.dmsk), dmsk, INET6_ADDRSTRLEN); // IPv6 Add
	printf("src = %s%s/%s\n", (e->ipv6.invflags & IP6T_INV_SRCIP)?"! ":"", // IPv6 modification
	       src, smsk); // IPv6 Modification
	printf("dst = %s%s/%s\n", (e->ipv6.invflags & IP6T_INV_DSTIP)?"! ":"", // IPv6 modification
	       dst, dmsk); // IPv6 Modification
	/*printf("in_if = %s  out_if = %s\n", e->ipv6.iniface, e->ipv6.outiface);*/
	printf("in_if = ");
	print_iface(e->ipv6.iniface, e->ipv6.iniface_mask,
	            e->ipv6.invflags & IP6T_INV_VIA_IN); // IPv6 modification
	printf(" out_if = ");
	print_iface(e->ipv6.outiface, e->ipv6.outiface_mask,
	            e->ipv6.invflags & IP6T_INV_VIA_OUT); // IPv6 modification
	printf("\n");
	printf("ipv6.proto = %s%d\n", (e->ipv6.invflags & IP6T_INV_PROTO)?"! ":"", // IPv6 modification
	       e->ipv6.proto);
}

/* for debug */
/* read the "filter" and "nat" tables */
int
list_redirect_rule(const char * ifname)
{
	IP6TC_HANDLE h; // IPv6 modification
	const struct ip6t_entry * e; // IPv6 modification
	const struct ip6t_entry_target * target; // IPv6 modification
	const struct ip6_nat_multi_range * mr; // IPv6 modification
	const char * target_str;
	char dst[INET6_ADDRSTRLEN]="", src[INET6_ADDRSTRLEN]="", dmsk[INET6_ADDRSTRLEN]="", smsk[INET6_ADDRSTRLEN]=""; // IPv6 Add

	h = ip6tc_init("filter"); // IPv6 modification **mangle/nat
	if(!h)
	{
		printf("ip6tc_init() error : %s\n", ip6tc_strerror(errno)); // IPv6 modification
		return -1;
	}
	if(!ip6tc_is_chain(miniupnpd_nat_chain, h)) // IPv6 modification
	{
		printf("chain %s not found\n", miniupnpd_nat_chain);
#ifdef IPTABLES_143
		ip6tc_free(h); // IPv6 modification
#else
		ip6tc_free(&h); // IPv6 modification
#endif
		return -1;
	}
#ifdef IPTABLES_143
	for(e = ip6tc_first_rule(miniupnpd_nat_chain, h); // IPv6 modification
		e;
		e = ip6tc_next_rule(e, h)) // IPv6 modification
	{
		target_str = ip6tc_get_target(e, h); // IPv6 modification
#else
	for(e = ip6tc_first_rule(miniupnpd_nat_chain, &h); // IPv6 modification
		e;
		e = ip6tc_next_rule(e, &h)) // IPv6 modification
	{
		target_str = ip6tc_get_target(e, &h); // IPv6 modification
#endif
		printf("===\n");
		inet_ntop(AF_INET6, &(e->ipv6.src), src, INET6_ADDRSTRLEN); // IPv6 Add
		inet_ntop(AF_INET6, &(e->ipv6.smsk), smsk, INET6_ADDRSTRLEN); // IPv6 Add
		inet_ntop(AF_INET6, &(e->ipv6.dst), dst, INET6_ADDRSTRLEN); // IPv6 Add
		inet_ntop(AF_INET6, &(e->ipv6.dmsk), dmsk, INET6_ADDRSTRLEN); // IPv6 Add
		printf("src = %s%s/%s\n", (e->ipv6.invflags & IP6T_INV_SRCIP)?"! ":"", // IPv6 modification
		       src, smsk); // IPv6 Modification
		printf("dst = %s%s/%s\n", (e->ipv6.invflags & IP6T_INV_DSTIP)?"! ":"", // IPv6 modification
		       dst, dmsk); // IPv6 Modification
		/*printf("in_if = %s  out_if = %s\n", e->ipv6.iniface, e->ipv6.outiface);*/
		printf("in_if = ");
		print_iface(e->ipv6.iniface, e->ipv6.iniface_mask,
		            e->ipv6.invflags & IP6T_INV_VIA_IN); // IPv6 modification
		printf(" out_if = ");
		print_iface(e->ipv6.outiface, e->ipv6.outiface_mask,
		            e->ipv6.invflags & IP6T_INV_VIA_OUT); // IPv6 modification
		printf("\n");
		printf("ipv6.proto = %s%d\n", (e->ipv6.invflags & IP6T_INV_PROTO)?"! ":"", // IPv6 modification
		       e->ipv6.proto);
		/* display matches stuff */
		if(e->target_offset)
		{
			IP6T_MATCH_ITERATE(e, print_match); // IPv6 modification
			/*printf("\n");*/
		}
		printf("target = %s\n", target_str);
		target = (void *)e + e->target_offset;
		mr = (const struct ip6_nat_multi_range *)&target->data[0]; // IPv6 modification
		printf("ips ");
		printip(mr->range[0].min_ip); // IPv6 modification
		printf(" ");
		printip(mr->range[0].max_ip); // IPv6 modification
		printf("\nports %hu %hu\n", ntohs(mr->range[0].min.all),
		          ntohs(mr->range[0].max.all));
		printf("flags = %x\n", mr->range[0].flags);
	}
	if(h)
#ifdef IPTABLES_143
		ip6tc_free(h); // IPv6 modification
#else
		ip6tc_free(&h); // IPv6 modification
#endif
	return 0;
}

