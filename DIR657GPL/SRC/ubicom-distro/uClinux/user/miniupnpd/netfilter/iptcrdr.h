/* $Id: iptcrdr.h,v 1.3 2009/07/08 11:18:33 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __IPTCRDR_H__
#define __IPTCRDR_H__

#include "../commonrdr.h"


//#define PORTMAPPING_DESC_LEN 31

struct rdr_desc {
	struct rdr_desc * next;
	char RemoteHost[INET_ADDRSTRLEN]; // can be empty string (wildcard)
	unsigned short eport; // can be 0, wildcard
	short proto; // TCP or UDP
	unsigned short InternalPort; // can not be 0
	char InternalClient[INET_ADDRSTRLEN]; // can not be wildcard (ex: empty string)
	int Enabled;
	long int LeaseDuration; // can be 0 or non-zero
	long int expire_time; // used to check when will expire
	char str[];
	//char str[PORTMAPPING_DESC_LEN+1];
};

void del_redirect_desc(unsigned short eport, int proto);
// ----------


/*
int
add_redirect_rule2(const char * ifname, unsigned short eport,
                   const char * iaddr, unsigned short iport, int proto,
				   const char * desc,
				   const char * eaddr);// Chun add: for CD_ROUTER
*/
int
add_redirect_rule2(const char * ifname, unsigned short eport,
                   const char * iaddr, unsigned short iport, int proto,
				   const char * desc,
				   const char * eaddr
				   , int Enabled, long int LeaseDuration, long int *out_expire_time);
// -------------------------------

int
add_filter_rule2(const char * ifname, const char * iaddr,
                 unsigned short eport, unsigned short iport,
                 int proto, const char * desc);

int
delete_redirect_and_filter_rules(unsigned short eport, int proto);

/* for debug */
int
list_redirect_rule(const char * ifname);

int
addnatrule(int proto, unsigned short eport,
               const char * iaddr, unsigned short iport,
			   const char * eaddr);/* Chun add: for CD_ROUTER */

int
add_filter_rule(int proto, const char * iaddr, unsigned short iport);

/*	Date:	2009-04-29
*	Name:	jimmy huang
*	Reason:	To reduce the counting time when control point
*			ask our numbers of PortMapping rules
*	Note:	Return numbers of rules
*/
int get_redirect_rule_nums(void /* char * ifname , unsigned short * eport,
                           char * iaddr, int iaddrlen, unsigned short * iport,
                           int * proto, char * desc, int desclen*/
						   );

/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To support hairpin, LAN A call LAN B with router's wan ip and port
*	Note:	
*			1. only test with libipt iptables-1.3.3
*			2. This function is for debug used, list all rules added in filter table, forward chain
*/
int
list_redirect_rule_forward(void);
//-----------
#endif

