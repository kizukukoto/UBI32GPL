/* $Id: miniupnpdtypes.h,v 1.2 2008/01/27 22:24:39 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2007 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#ifndef __MINIUPNPDTYPES_H__
#define __MINIUPNPDTYPES_H__

#include "config.h"
#include <netinet/in.h>

/* structure for storing lan addresses
 * with ascii representation and mask */
struct lan_addr_s {
	char str[40];	/* example: fe80:0000:0000:0000:0000:0000:0000:0001 */ //IPv6 Modification
	struct in6_addr addr, mask;	/* ip/mask */ //IPv6 Modification
#ifdef MULTIPLE_EXTERNAL_IP
	char ext_ip_str[40]; //IPv6 Modification
	struct in6_addr ext_ip_addr; //IPv6 Modification
#endif
};

#endif
