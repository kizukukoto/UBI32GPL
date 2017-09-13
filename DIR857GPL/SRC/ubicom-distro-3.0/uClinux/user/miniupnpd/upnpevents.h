/* $Id: upnpevents.h,v 1.2 2009/04/29 03:58:04 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2008 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __UPNPEVENTS_H__
#define __UPNPEVENTS_H__
#ifdef ENABLE_EVENTS
enum subscriber_service_enum {
 EWanCFG = 1,
 EWanIPC,
 EL3F
};

void
upnp_event_var_change_notify(enum subscriber_service_enum service);

const char *
upnpevents_addSubscriber(const char * eventurl,
                         const char * callback, int callbacklen,
                         int timeout);

int
upnpevents_removeSubscriber(const char * sid, int sidlen);

int
renewSubscription(const char * sid, int sidlen, int timeout);

void upnpevents_selectfds(fd_set *readset, fd_set *writeset, int * max_fd);
void upnpevents_processfds(fd_set *readset, fd_set *writeset);

#ifdef USE_MINIUPNPDCTL
void write_events_details(int s);
#endif


void check_if_SubscriberNeedNotify(void);
void ScanAndCleanPortMappingExpiration(void);


/*	Date:	2009-04-29
*	Name:	jimmy huang
*	Reason:	Currently, due to miniupnpd's architecture(select,noblock),
*			we may not immediately send notify after variable changed (Add / Del PortMapping...)
*			This definition is used to force when after soap, we use sock with block
*			to send notify right after AddPortMapping, search this definition within codes
*			for more detail
*	Note:	Not test this feature well
*/
#ifdef IMMEDIATELY_SEND_NOTIFY
int check_notify_right_now(enum subscriber_service_enum service);
#endif

#endif //#ifdef ENABLE_EVENTS
#endif
