/* $Id: commonrdr.h,v 1.1 2009/04/21 11:14:48 jimmy_huang Exp $ */
/* MiniUPnP project
 * (c) 2006-2007 Thomas Bernard
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#ifndef __COMMONRDR_H__
#define __COMMONRDR_H__

#include "config.h"

/* init and shutdown functions */
int
init_redirect(void);

void
shutdown_redirect(void);

/* get_redirect_rule() gets internal IP and port from
 * interface, external port and protocl
 */
int
get_redirect_rule(const char * ifname, unsigned short eport, int proto,
                  char * iaddr, int iaddrlen, unsigned short * iport,
                  char * desc, int desclen,
                  u_int64_t * packets, u_int64_t * bytes, long int *expire_time);

int
get_redirect_rule_by_index(int index_local,
                           char * ifname, unsigned short * eport,
                           char * iaddr, int iaddrlen, unsigned short * iport,
                           int * proto, char * desc, int desclen,
                           u_int64_t * packets, u_int64_t * bytes, long int *expire_time);

#endif

