
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2007 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#ifndef __MINISSDP_H__
#define __MINISSDP_H__

/*#include "miniupnpdtypes.h"*/

int
OpenAndConfSSDPReceiveSocket(void);
/* OpenAndConfSSDPReceiveSocket(int n_lan_addr, struct lan_addr_s * lan_addr);*/

/*int
OpenAndConfSSDPNotifySocket(const char * addr);*/

int
OpenAndConfSSDPNotifySockets(int * sockets);
/*OpenAndConfSSDPNotifySockets(int * sockets,
                             struct lan_addr_s * lan_addr, int n_lan_addr);*/

/*void
SendSSDPNotifies(int s, const char * host, unsigned short port,
                 unsigned int lifetime);*/
void
SendSSDPNotifies2(int * sockets,
                  unsigned short port,
                  unsigned int lifetime);
/*SendSSDPNotifies2(int * sockets, struct lan_addr_s * lan_addr, int n_lan_addr,
                  unsigned short port,
                  unsigned int lifetime);*/

void
ProcessSSDPRequest(int s, unsigned short port);
/*ProcessSSDPRequest(int s, struct lan_addr_s * lan_addr, int n_lan_addr,
                   unsigned short port);*/

int
SendSSDPGoodbye(int * sockets, int n);

/*	Date:	2010-09-17
*	Name:	jimmy huang
*	Reason:	To fixed the bug, When rc ask hostpad restart, hostapd will lose all subscription records
*			Thus we need to tell all control points that WFA services are dead
*			Then when we detect hostpad is alive again, we will send NOTIFY to tell all CPs
*			that WFA services are ready, CPs will subscribe information with hostapd again
*/
int SendSSDPGoodbye_WFAService(int * sockets, int n_sockets);
void stop_wfa_service(int sig);

int
SubmitServicesToMiniSSDPD(const char * host, unsigned short port);

#endif

