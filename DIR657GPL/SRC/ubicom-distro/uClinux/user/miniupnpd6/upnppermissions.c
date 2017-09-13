/* $Id: upnppermissions.c,v 1.14 2009/12/22 17:21:43 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "config.h"
#include "upnppermissions.h"

int
read_permission_line(struct upnpperm * perm,
                     char * p)
{
	char * q;
	int n_bits;

	/* first token: (allow|deny) */
	while(isspace(*p))
		p++;
	if(0 == memcmp(p, "allow", 5))
	{
		perm->type = UPNPPERM_ALLOW;
		p += 5;
	}
	else if(0 == memcmp(p, "deny", 4))
	{
		perm->type = UPNPPERM_DENY;
		p += 4;
	}
	else
	{
	    fprintf(stderr, "Error no allow/deny ...\n");
		return -1;
	}

	/* second token: eport or eport_min-eport_max */
	while(isspace(*p))
		p++;
	if(!isdigit(*p))
	{
	    fprintf(stderr, "Not digit first eport...\n");
		return -1;
	}
	for(q = p; isdigit(*q); q++);
	if(*q=='-')
	{
		*q = '\0';
		perm->eport_min = (u_short)atoi(p);
		q++;
		p = q;
		while(isdigit(*q))
			q++;
		*q = '\0';
		perm->eport_max = (u_short)atoi(p);
	}
	else
	{
		*q = '\0';
		perm->eport_min = perm->eport_max = (u_short)atoi(p);
	}
	p = q + 1;
	while(isspace(*p))
		p++;

	/* third token:  ip/mask */
	if(!isdigit(*p)&&*p!=':')
	{
	    fprintf(stderr, "Not digit first ip...\n");
		return -1;
	}
	for(q = p; isdigit(*q) || (*q == ':'); q++); // IPv6 modification, ip separator ":"
	if(*q=='/')
	{
		*q = '\0';
		if(!inet_pton(AF_INET6, p, &perm->address)) // IPv6 modification
		{
			fprintf(stderr, "Error pton ipaddress...\n");
			return -1;
		}
		q++;
		p = q;
		while(isdigit(*q))
			q++;
		*q = '\0';
		n_bits = atoi(p);
		getv6AddressMask(&(perm->mask), n_bits); // IPv6 modification
		//perm->mask.s6_addr = htonl(n_bits ? (0xffffffffffffffffffffffffffffffffffffffff << (128 - n_bits)) : 0);
	}
	else
	{
		*q = '\0';
		if(!inet_pton(AF_INET6, p, &perm->address)) // IPv6 modification
		{
			fprintf(stderr, "Error pton ipaddress withoutport...\n");
			return -1;
		}
		getv6AddressMask(&(perm->mask), 128); // IPv6 modification
		//perm->mask.s6_addr = 0xffffffffffffffffffffffffffffffffffffffff;
	}
	p = q + 1;

	/* fourth token: iport or iport_min-iport_max */
	while(isspace(*p))
		p++;
	if(!isdigit(*p))
	{
	    fprintf(stderr, "Error no digit iport...\n");
		return -1;
	}
	for(q = p; isdigit(*q); q++);
	if(*q=='-')
	{
		*q = '\0';
		perm->iport_min = (u_short)atoi(p);
		q++;
		p = q;
		while(isdigit(*q))
			q++;
		*q = '\0';
		perm->iport_max = (u_short)atoi(p);
	}
	else
	{
		*q = '\0';
		perm->iport_min = perm->iport_max = (u_short)atoi(p);
	}
#ifdef DEBUG
	char addr[INET6_ADDRSTRLEN]="", msk[INET6_ADDRSTRLEN]="";
	inet_ntop(AF_INET6, &(perm->address), addr, INET6_ADDRSTRLEN);
	inet_ntop(AF_INET6, &(perm->mask), msk, INET6_ADDRSTRLEN);
	printf("perm rule added : %s %hu-%hu %s/%s %hu-%hu\n",
	       (perm->type==UPNPPERM_ALLOW)?"allow":"deny",
	       perm->eport_min, perm->eport_max, addr,// IPv6 modification (ntohl, equivalent?)
	       msk, perm->iport_min, perm->iport_max);// IPv6 modification
#endif
	return 0;
}

#ifdef USE_MINIUPNPDCTL
void
write_permlist(int fd, const struct upnpperm * permary,
               int nperms)
{
	int l;
	const struct upnpperm * perm;
	int i;
	char buf[128];
	char addr[INET6_ADDRSTRLEN]="", mask[INET6_ADDRSTRLEN]="";
	inet_ntop(AF_INET6, &(perm->address), addr, INET6_ADDRSTRLEN);
	inet_ntop(AF_INET6, &(perm->mask), mask, INET6_ADDRSTRLEN);
	write(fd, "Permissions :\n", 14);
	for(i = 0; i<nperms; i++)
	{
		perm = permary + i;
		l = snprintf(buf, sizeof(buf), "%02d %s %hu-%hu %s/%s %hu-%hu\n",
	       i,
    	   (perm->type==UPNPPERM_ALLOW)?"allow":"deny",
	       perm->eport_min, perm->eport_max, addr,// IPv6 modification (ntohl ?)
	       mask, perm->iport_min, perm->iport_max);// IPv6 modification
		if(l<0)
			return;
		write(fd, buf, l);
	}
}
#endif

/* match_permission()
 * returns: 1 if eport, address, iport matches the permission rule
 *          0 if no match */
static int
match_permission(const struct upnpperm * perm,
                 u_short eport, struct in6_addr address, u_short iport)// IPv6 modification
{
	int i;
	if( (eport < perm->eport_min) || (perm->eport_max < eport))
		return 0;
	if( (iport < perm->iport_min) || (perm->iport_max < iport))
		return 0;
	for(i=0; i<sizeof(struct in6_addr)/sizeof(int); i++)
	{
        	if( ( ((int) address.s6_addr[i]) & ((int) perm->mask.s6_addr[i]) )// IPv6 modification
        	!= ( ((int) perm->address.s6_addr[i]) & ((int) perm->mask.s6_addr[i]) ) )// IPv6 modification
            		return 0;
	}
	return 1;
}

int
check_upnp_rule_against_permissions(const struct upnpperm * permary,
                                    int n_perms,
                                    u_short eport, struct in6_addr address,// IPv6 modification
                                    u_short iport)
{
	int i;
	for(i=0; i<n_perms; i++)
	{
		if(match_permission(permary + i, eport, address, iport))
		{
			syslog(LOG_DEBUG,
			       "UPnP permission rule %d matched : port mapping %s",
			       i, (permary[i].type == UPNPPERM_ALLOW)?"accepted":"rejected"
			       );
			return (permary[i].type == UPNPPERM_ALLOW);
		}
	}
	syslog(LOG_DEBUG, "no permission rule matched : accept by default (n_perms=%d)", n_perms);
	return 1;	/* Default : accept */
}

