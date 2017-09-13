/* $Id: upnpevents.c,v 1.4 2009/06/11 03:46:38 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2008 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "config.h"
#include "upnpevents.h"
#include "miniupnpdpath.h"
#include "upnpglobalvars.h"
#include "upnpdescgen.h"

//for detect wan connection status
#include "getifaddr.h"
// ----------

#ifdef MINIUPNPD_DEBUG_EVENTS
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef ENABLE_EVENTS
/*enum subscriber_service_enum {
 EWanCFG = 1,
 EWanIPC,
 EL3F
};*/

/* stuctures definitions */
struct subscriber {
	LIST_ENTRY(subscriber) entries;
	struct upnp_event_notify * notify;
	time_t timeout;
	uint32_t seq;
	/*enum { EWanCFG = 1, EWanIPC, EL3F } service;*/
	enum subscriber_service_enum service;
	char uuid[42];
	char callback[];
};

struct upnp_event_notify {
	LIST_ENTRY(upnp_event_notify) entries;
    int s;  /* socket */
    enum { ECreated=1,
	       EConnecting,
	       ESending,
	       EWaitingForResponse,
	       EFinished,
	       EError } state;
    struct subscriber * sub;
    char * buffer;
    int buffersize;
	int tosend;
    int sent;
	const char * path;
	char addrstr[16];
	char portstr[8];
};

/* prototypes */
static void
upnp_event_create_notify(struct subscriber * sub);

/* Subscriber list */
LIST_HEAD(listhead, subscriber) subscriberlist = { NULL };

/* notify list */
LIST_HEAD(listheadnotif, upnp_event_notify) notifylist = { NULL };

/* create a new subscriber */
static struct subscriber *
newSubscriber(const char * eventurl, const char * callback, int callbacklen)
{
	struct subscriber * tmp;
	//jimmy added
	char *tmp_uuid_value=NULL;// Chun add for WPS-COMPATIBLE
	//----------
	
	DEBUG_MSG("%s (%d): begin\n",__FUNCTION__,__LINE__);

	if(!eventurl || !callback || !callbacklen){
		DEBUG_MSG("%s (%d): eventurl or callback or backlen (%d) is null, return NULL\n",__FUNCTION__,__LINE__,callbacklen);
		return NULL;
	}

	tmp = calloc(1, sizeof(struct subscriber)+callbacklen+1);
	if(strcmp(eventurl, WANCFG_EVENTURL)==0){
		DEBUG_MSG("%s (%d): %s, uuid = %s\n",__FUNCTION__,__LINE__,WANCFG_EVENTURL,uuidvalue_2);
		tmp_uuid_value = uuidvalue_2;
		tmp->service = EWanCFG;
	}else if(strcmp(eventurl, WANIPC_EVENTURL)==0){
		DEBUG_MSG("%s (%d): %s, uuid = %s\n",__FUNCTION__,__LINE__,WANIPC_EVENTURL,uuidvalue_3);
		tmp_uuid_value = uuidvalue_3;
		tmp->service = EWanIPC;
	}
#ifdef ENABLE_L3F_SERVICE
	else if(strcmp(eventurl, L3F_EVENTURL)==0){
		DEBUG_MSG("%s (%d): %s, L3F_EVENTURL, uuid = %s\n",__FUNCTION__,__LINE__,WANIPC_EVENTURL,uuidvalue_1);
		tmp_uuid_value = uuidvalue_1;
		tmp->service = EL3F;
	}
#endif
	else {
		free(tmp);
		return NULL;
	}
	memcpy(tmp->callback, callback, callbacklen);
	tmp->callback[callbacklen] = '\0';
	/* make a dummy uuid */
	/* TODO: improve that */

	//strncpy(tmp->uuid, uuidvalue, sizeof(tmp->uuid));
	strncpy(tmp->uuid, tmp_uuid_value, sizeof(tmp->uuid));
	// ----------
	tmp->uuid[sizeof(tmp->uuid)-1] = '\0';
	DEBUG_MSG("%s (%d): uuid = %s\n",__FUNCTION__,__LINE__,tmp->uuid);
	snprintf(tmp->uuid+37, 5, "%04lx", random() & 0xffff);
	
	DEBUG_MSG("%s (%d): End \n",__FUNCTION__,__LINE__);
	return tmp;
}

/* creates a new subscriber and adds it to the subscriber list
 * also initiate 1st notify */
const char *
upnpevents_addSubscriber(const char * eventurl,
                         const char * callback, int callbacklen,
                         int timeout)
{
	struct subscriber * tmp;
	/*static char uuid[42];*/
	/* "uuid:00000000-0000-0000-0000-000000000000"; 5+36+1=42bytes */

	DEBUG_MSG("\n");
	syslog(LOG_DEBUG, "upnpevents_addSubscriber(%s, %.*s, %d)",
	       eventurl, callbacklen, callback, timeout);
	/*	Date:	2009-04-20
	*	Name:	jimmy huang
	*	Reason:	For debug message
	*/
	{
		struct subscriber * sub;
		DEBUG_MSG("%s (%d): Before added, list are \n",__FUNCTION__,__LINE__);
		for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
			DEBUG_MSG("uuid = %s, seq = %d, timeout = %ld, service = %s (%d)\n"
				,sub->uuid,sub->seq,sub->timeout
				,sub->service == 1 ? "EWanCFG" : 
					(sub->service == 2 ? "EWanIPC" : 
					(sub->service == 3 ? "EL3F" : "Unknown"))
				,sub->service );
		}
		DEBUG_MSG("-------------------------------\n");
	}
	/*strncpy(uuid, uuidvalue, sizeof(uuid));
	uuid[sizeof(uuid)-1] = '\0';*/
	DEBUG_MSG("%s (%d): go newSubscriber() \n",__FUNCTION__,__LINE__);
	tmp = newSubscriber(eventurl, callback, callbacklen);
	if(!tmp){
		DEBUG_MSG("%s (%d): newSubscriber() failed\n",__FUNCTION__,__LINE__);
		return NULL;
	}else{
		DEBUG_MSG("%s (%d): newSubscriber() suceess\n",__FUNCTION__,__LINE__);
	}
	if(timeout)
		tmp->timeout = time(NULL) + timeout;
	LIST_INSERT_HEAD(&subscriberlist, tmp, entries);
	DEBUG_MSG("%s (%d): add to list with\n",__FUNCTION__,__LINE__);
	DEBUG_MSG("uuid = %s, seq = %d, timeout = %ld, service = %s (%d)\n"
			,tmp->uuid, tmp->seq, tmp->timeout
			,tmp->service == 1 ? "EWanCFG" : 
				(tmp->service == 2 ? "EWanIPC" : 
				(tmp->service == 3 ? "EL3F" : "Unknown"))
			,tmp->service );
	/*	Date:	2009-04-20
	*	Name:	jimmy huang
	*	Reason:	For debug message
	*/
	{
		struct subscriber * sub;
		DEBUG_MSG("%s (%d): after added, list are \n",__FUNCTION__,__LINE__);
		for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
			DEBUG_MSG("uuid = %s, seq = %d, timeout = %ld, service = %s (%d)\n"
				,sub->uuid,sub->seq,sub->timeout
				,sub->service == 1 ? "EWanCFG" : 
					(sub->service == 2 ? "EWanIPC" : 
					(sub->service == 3 ? "EL3F" : "Unknown"))
				,sub->service );
		}
		DEBUG_MSG("-------------------------------\n");
	}
	//----------------
	DEBUG_MSG("%s (%d): go upnp_event_create_notify()\n",__FUNCTION__,__LINE__);
//	/*	Date:	2009-05-04
//	*	Name:	jimmy huang
//	*	Reason:	For Intel Device Spy Validator, item "Subscribe"
//	*			Subscribe (timeout 15) -> Subscribe (renew) -> UnSubscribe -> Subscribe (timeout 120)
//	*			Refer to Ubicom DIR-655 A2, The first subscribe (timeout 15) does not send notify for initial Subscribe
//	*/
//	//	upnp_event_create_notify(tmp);
//	if(timeout > 15){
//		upnp_event_create_notify(tmp);
//	}
//	//-----------
	/*
	 * What are the reasons of the above lines but in the
	 * UPnP Device Architecture 1.0 page 63, As soon as possible
	 * after the subscription is accepted, the publisher also sends the first,
	 * or initial event message to the subscriber. This initial event message
	 * is always sent...
	 */
	upnp_event_create_notify(tmp);

	// Send the initial notify event
	upnp_event_var_change_notify(EWanIPC);

	DEBUG_MSG("%s (%d): end \n",__FUNCTION__,__LINE__);
	return tmp->uuid;
}

/* renew a subscription (update the timeout) */
int
renewSubscription(const char * sid, int sidlen, int timeout)
{
	struct subscriber * sub;
	DEBUG_MSG("\n%s (%d): Begin with new sid = %s\n",__FUNCTION__,__LINE__,sid);
	for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
		DEBUG_MSG("%s (%d): searching [ %s ]\n",__FUNCTION__,__LINE__,sub->uuid);
		if(memcmp(sid, sub->uuid, 41) == 0) {
			sub->timeout = (timeout ? time(NULL) + timeout : 0);
			DEBUG_MSG("%s (%d): found matched sid!!, new timeout = %ld\n",__FUNCTION__,__LINE__,sub->timeout);
			return 0;
		}
	}
	DEBUG_MSG("%s (%d): end, No matched !!!\n",__FUNCTION__,__LINE__);
	return -1;
}

int
upnpevents_removeSubscriber(const char * sid, int sidlen)
{
	struct subscriber * sub;
	DEBUG_MSG("\n%s (%d): beign, sid = [ %s ]\n",__FUNCTION__,__LINE__,sid);
	if(!sid){
		DEBUG_MSG("%s (%d): sid is null, return -1\n",__FUNCTION__,__LINE__);
		return -1;
	}
	for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
		DEBUG_MSG("%s (%d): searching [ %s ]\n",__FUNCTION__,__LINE__,sub->uuid);
		//miniupnpd's bug
		//if(memcmp(sid, sub->uuid, 41)) {
		if(memcmp(sid, sub->uuid, 41) == 0) {
			DEBUG_MSG("%s (%d): found matched sid !!\n",__FUNCTION__,__LINE__);
			if(sub->notify) {
				sub->notify->sub = NULL;
			}
			LIST_REMOVE(sub, entries);
			free(sub);
			return 0;
		}
	}
	DEBUG_MSG("%s (%d): No matched !!!\n",__FUNCTION__,__LINE__);
	return -1;
}

/* notifies all subscriber of a number of port mapping change
 * or external ip address change */
void
upnp_event_var_change_notify(enum subscriber_service_enum service)
{
	struct subscriber * sub;
	DEBUG_MSG("\n%s (%d): beign, service = %s (%d)\n"
			,__FUNCTION__,__LINE__
			,service == 1 ? "EWanCFG" : 
				(service == 2 ? "EWanIPC" : 
				(service == 3 ? "EL3F" : "Unknown"))
				,service
			);
	for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
		if(sub->service == service && sub->notify == NULL){
			DEBUG_MSG("%s (%d): go upnp_event_create_notify() for sub->service = %s (%d) \n"
					,__FUNCTION__,__LINE__
					,sub->service == 1 ? "EWanCFG" : 
						(sub->service == 2 ? "EWanIPC" : 
						(sub->service == 3 ? "EL3F" : "Unknown"))
					,sub->service
					);
			upnp_event_create_notify(sub);
		}else{
			DEBUG_MSG("%s (%d): sub->notify != NULL, do nothing\n",__FUNCTION__,__LINE__);
		}
	}
	DEBUG_MSG("%s (%d): End\n",__FUNCTION__,__LINE__);
}

/* create and add the notify object to the list */
static void
upnp_event_create_notify(struct subscriber * sub)
{
	struct upnp_event_notify * obj;
	int flags;
	obj = calloc(1, sizeof(struct upnp_event_notify));
	if(!obj) {
		syslog(LOG_ERR, "%s: calloc(): %m", "upnp_event_create_notify");
		return;
	}
	DEBUG_MSG("\n%s (%d): beign, uuid = %s, seq = %d, service = %d \n"
			,__FUNCTION__,__LINE__,sub->uuid,sub->seq,sub->service);
	obj->sub = sub;
	obj->state = ECreated;
	DEBUG_MSG("%s (%d): create with state = ECreated (%d)\n",__FUNCTION__,__LINE__,ECreated);
	obj->s = socket(PF_INET, SOCK_STREAM, 0);
	if(obj->s<0) {
		syslog(LOG_ERR, "%s: socket(): %m", "upnp_event_create_notify");
		goto error;
	}
	if((flags = fcntl(obj->s, F_GETFL, 0)) < 0) {
		syslog(LOG_ERR, "%s: fcntl(..F_GETFL..): %m",
		       "upnp_event_create_notify");
		goto error;
	}
	if(fcntl(obj->s, F_SETFL, flags | O_NONBLOCK) < 0) {
		syslog(LOG_ERR, "%s: fcntl(..F_SETFL..): %m",
		       "upnp_event_create_notify");
		goto error;
	}
	if(sub)
		sub->notify = obj;
	DEBUG_MSG("%s (%d): Add to notifylist\n",__FUNCTION__,__LINE__);
	LIST_INSERT_HEAD(&notifylist, obj, entries);
	DEBUG_MSG("%s (%d): end \n",__FUNCTION__,__LINE__);
	return;
error:
	DEBUG_MSG("%s (%d): Error !\n",__FUNCTION__,__LINE__);
	if(obj->s >= 0)
		close(obj->s);
	free(obj);
}

static void
upnp_event_notify_connect(struct upnp_event_notify * obj)
{
	int i;
	const char * p;
	unsigned short port;
	struct sockaddr_in addr;
	
	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	
	if(!obj)
		return;

	memset(&addr, 0, sizeof(addr));
	i = 0;
	if(obj->sub == NULL) {
		obj->state = EError;
		return;
	}
	p = obj->sub->callback;
	p += 7;	/* http:// */
	while(*p != '/' && *p != ':')
		obj->addrstr[i++] = *(p++);
	obj->addrstr[i] = '\0';
	if(*p == ':') {
		obj->portstr[0] = *p;
		i = 1;
		p++;
		port = (unsigned short)atoi(p);
		while(*p != '/') {
			if(i<7) obj->portstr[i++] = *p;
			p++;
		}
		obj->portstr[i] = 0;
	} else {
		port = 80;
		obj->portstr[0] = '\0';
	}
	obj->path = p;
	addr.sin_family = AF_INET;
	inet_aton(obj->addrstr, &addr.sin_addr);
	addr.sin_port = htons(port);
	syslog(LOG_DEBUG, "%s: '%s' %hu '%s'", "upnp_event_notify_connect",
	       obj->addrstr, port, obj->path);
	obj->state = EConnecting;
	if(connect(obj->s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		if(errno != EINPROGRESS && errno != EWOULDBLOCK) {
			syslog(LOG_ERR, "%s: connect(): %m", "upnp_event_notify_connect");
			obj->state = EError;
		}
	}
}

static void upnp_event_prepare(struct upnp_event_notify * obj)
{
	static const char notifymsg[] = 
		"NOTIFY %s HTTP/1.1\r\n"
		"Host: %s%s\r\n"
		"Content-Type: text/xml\r\n"
		"Content-Length: %d\r\n"
		"NT: upnp:event\r\n"
		"NTS: upnp:propchange\r\n"
		"SID: %s\r\n"
		"SEQ: %u\r\n"
		"Connection: close\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n"
		"%.*s\r\n";
	char * xml;
	int l;
	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	if(obj->sub == NULL) {
		obj->state = EError;
		return;
	}
	switch(obj->sub->service) {
	case EWanCFG:
		DEBUG_MSG("%s (%d): Service is EWanCFG, go getVarsWANCfg()\n",__FUNCTION__,__LINE__);
		xml = getVarsWANCfg(&l);
		break;
	case EWanIPC:
		DEBUG_MSG("%s (%d): Service is EWanIPC, go getVarsWANIPCn()\n",__FUNCTION__,__LINE__);
		xml = getVarsWANIPCn(&l);
		break;
#ifdef ENABLE_L3F_SERVICE
	case EL3F:
		xml = getVarsL3F(&l);
		break;
#endif
	default:
		xml = NULL;
		l = 0;
	}
	obj->buffersize = 1024;
	obj->buffer = malloc(obj->buffersize);
	/*if(!obj->buffer) {
	}*/
	obj->tosend = snprintf(obj->buffer, obj->buffersize, notifymsg,
	                       obj->path, obj->addrstr, obj->portstr, l+2,
	                       obj->sub->uuid, obj->sub->seq,
	                       l, xml);
	if(xml) {
		free(xml);
		xml = NULL;
	}
	obj->state = ESending;
	DEBUG_MSG("\n============================================\n");
	DEBUG_MSG("%s (%d): Send Notify with\n",__FUNCTION__,__LINE__);
	DEBUG_MSG("%s\n",obj->buffer);
	DEBUG_MSG("\n============================================\n");
}

static void upnp_event_send(struct upnp_event_notify * obj)
{
	int i;
	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	DEBUG_MSG("%s (%d): sent at %ld \n",__FUNCTION__,__LINE__,time(NULL));
	i = send(obj->s, obj->buffer + obj->sent, obj->tosend - obj->sent, 0);
	if(i<0) {
		syslog(LOG_NOTICE, "%s: send(): %m", "upnp_event_send");
		obj->state = EError;
		return;
	}
	else if(i != (obj->tosend - obj->sent))
		syslog(LOG_NOTICE, "%s: %d bytes send out of %d",
		       "upnp_event_send", i, obj->tosend - obj->sent);
	obj->sent += i;
	if(obj->sent == obj->tosend)
		obj->state = EWaitingForResponse;
}

static void upnp_event_recv(struct upnp_event_notify * obj)
{
	int n;
	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	n = recv(obj->s, obj->buffer, obj->buffersize, 0);
	if(n<0) {
		syslog(LOG_ERR, "%s: recv(): %m", "upnp_event_recv");
		obj->state = EError;
		return;
	}
	syslog(LOG_DEBUG, "%s: (%dbytes) %.*s", "upnp_event_recv",
	       n, n, obj->buffer);
	obj->state = EFinished;
	if(obj->sub)
		obj->sub->seq++;
}

static void
upnp_event_process_notify(struct upnp_event_notify * obj)
{
	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	switch(obj->state) {
	case EConnecting:
		/* now connected or failed to connect */
		upnp_event_prepare(obj);
		upnp_event_send(obj);
		break;
	case ESending:
		upnp_event_send(obj);
		break;
	case EWaitingForResponse:
		upnp_event_recv(obj);
		break;
	case EFinished:
		close(obj->s);
		obj->s = -1;
		break;
	default:
		syslog(LOG_ERR, "upnp_event_process_notify: unknown state");
	}
	DEBUG_MSG("%s (%d): End\n",__FUNCTION__,__LINE__);
}

void upnpevents_selectfds(fd_set *readset, fd_set *writeset, int * max_fd)
{
	struct upnp_event_notify * obj;
	DEBUG_MSG("\n");
	for(obj = notifylist.lh_first; obj != NULL; obj = obj->entries.le_next) {
		syslog(LOG_DEBUG, "upnpevents_selectfds: %p %d %d",
		       obj, obj->state, obj->s);
		if(obj->s >= 0) {
			DEBUG_MSG("%s (%d): obj->s >= 0\n",__FUNCTION__,__LINE__);
			switch(obj->state) {
				case ECreated:
					DEBUG_MSG("%s (%d): obj->state = ECreated, go upnp_event_notify_connect()\n",__FUNCTION__,__LINE__);
					upnp_event_notify_connect(obj);
					if(obj->state != EConnecting)
						break;
				case EConnecting:
				case ESending:
					FD_SET(obj->s, writeset);
					if(obj->s > *max_fd)
						*max_fd = obj->s;
					break;
				case EWaitingForResponse:
					FD_SET(obj->s, readset);
					if(obj->s > *max_fd)
						*max_fd = obj->s;
					break;
				// added to avoid warning message
				default:
					break;
			// ----------
			}
		}
	}
}

void upnpevents_processfds(fd_set *readset, fd_set *writeset)
{
	struct upnp_event_notify * obj = NULL;
	struct upnp_event_notify * next = NULL;
	struct subscriber * sub = NULL;
	struct subscriber * subnext = NULL;
	time_t curtime;
	DEBUG_MSG("\n%s (%d): Begin\n",__FUNCTION__,__LINE__);
	for(obj = notifylist.lh_first; obj != NULL; obj = obj->entries.le_next) {
		syslog(LOG_DEBUG, "%s: %p %d %d %d %d",
		       "upnpevents_processfds", obj, obj->state, obj->s,
		       FD_ISSET(obj->s, readset), FD_ISSET(obj->s, writeset));
		if(obj->s >= 0) {
			if(FD_ISSET(obj->s, readset) || FD_ISSET(obj->s, writeset)){
				DEBUG_MSG("%s (%d): go upnp_event_process_notify()\n",__FUNCTION__,__LINE__);
				upnp_event_process_notify(obj);
		}
	}
	}
	obj = notifylist.lh_first;
	while(obj != NULL) {
		next = obj->entries.le_next;
		if(obj->state == EError || obj->state == EFinished) {
			if(obj->s >= 0) {
				close(obj->s);
			}
			if(obj->sub)
				obj->sub->notify = NULL;
			/* remove also the subscriber from the list if there was an error */
			if(obj->state == EError && obj->sub) {
				DEBUG_MSG("%s (%d): Removing subscribers happened errors, uuid = %s,\n"
					,__FUNCTION__,__LINE__,obj->sub->uuid ? obj->sub->uuid : "none");
				LIST_REMOVE(obj->sub, entries);
				free(obj->sub);
			}
			if(obj->buffer) {
				free(obj->buffer);
			}
			LIST_REMOVE(obj, entries);
			free(obj);
		}
		obj = next;
	}
	/* remove timeouted subscribers */
	curtime = time(NULL);
	for(sub = subscriberlist.lh_first; sub != NULL; ) {
		subnext = sub->entries.le_next;
		if(sub->timeout && curtime > sub->timeout && sub->notify == NULL) {
			DEBUG_MSG("%s (%d): Removing timeouted subscribers, uuid = %s,\n"
					,__FUNCTION__,__LINE__,sub->uuid);
			DEBUG_MSG("%s (%d): sub->timeout = %ld , curtime = %ld\n"
					,__FUNCTION__,__LINE__,sub->timeout,curtime);
			LIST_REMOVE(sub, entries);
			free(sub);
		}
		sub = subnext;
	}
	DEBUG_MSG("%s (%d): End\n",__FUNCTION__,__LINE__);
}

#ifdef USE_MINIUPNPDCTL
void write_events_details(int s) {
	int n;
	char buff[80];
	struct upnp_event_notify * obj;
	struct subscriber * sub;
	write(s, "Events details\n", 15);
	for(obj = notifylist.lh_first; obj != NULL; obj = obj->entries.le_next) {
		n = snprintf(buff, sizeof(buff), " %p sub=%p state=%d s=%d\n",
		             obj, obj->sub, obj->state, obj->s);
		write(s, buff, n);
	}
	write(s, "Subscribers :\n", 14);
	for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
		n = snprintf(buff, sizeof(buff), " %p timeout=%d seq=%u service=%d\n",
		             sub, sub->timeout, sub->seq, sub->service);
		write(s, buff, n);
		n = snprintf(buff, sizeof(buff), "   notify=%p %s\n",
		             sub->notify, sub->uuid);
		write(s, buff, n);
		n = snprintf(buff, sizeof(buff), "   %s\n",
		             sub->callback);
		write(s, buff, n);
	}
}
#endif

/*	Date:	2009-04-20
*	Name:	jimmy huang
*	Reason:	For subscribe WANIPCON, wan ip has changed
*/
int WanConnectionStatus_changed(int last_status){
	char ext_ip_addr[INET_ADDRSTRLEN]={0};

	DEBUG_MSG("\n\n%s (%d): last_status is %s (%d)\n"
			,__FUNCTION__,__LINE__
			,last_status == 1 ? "Connected" : "Disconnected",last_status);
	int changed = 0;
	int cur_status = 0;
	
	//get current status and IP
	cur_status = getifaddr(ext_if_name, ext_ip_addr, INET_ADDRSTRLEN);
	
	//check connection status
	if(cur_status < 0){
		// Disconnected
		DEBUG_MSG("%s (%d): current status is Disconnected\n",__FUNCTION__,__LINE__);
		if(last_status == 1){
			DEBUG_MSG("%s (%d): WanConnectionStatus changed to %s \n"
						,__FUNCTION__,__LINE__
						,(!last_status) == 1 ? "Connected" : "Disconnected");
			changed = 1;
		}
		strncpy(ext_ip_addr,"0.0.0.0\0",strlen("0.0.0.0\0"));
	}else{
		//Connected
		DEBUG_MSG("%s (%d): current status is Connected\n",__FUNCTION__,__LINE__);
		if(last_status == 0){
			DEBUG_MSG("%s (%d): WanConnectionStatus changed to %s \n"
						,__FUNCTION__,__LINE__
						,(!last_status) == 1 ? "Connected" : "Disconnected");
			changed = 1;
		}
	}
	
	//check ip
	DEBUG_MSG("%s (%d): current IP is %s\n",__FUNCTION__,__LINE__,ext_ip_addr);
	DEBUG_MSG("%s (%d): last_ext_ip_addr is %s\n",__FUNCTION__,__LINE__,last_ext_ip_addr);
	if(changed == 0){
		if(strncmp(last_ext_ip_addr,ext_ip_addr, INET_ADDRSTRLEN) != 0){
			DEBUG_MSG("%s (%d): WAN IP is Connected\n",__FUNCTION__,__LINE__);
			changed = 1;
		}
	}
	strncpy(last_ext_ip_addr,ext_ip_addr,INET_ADDRSTRLEN);

	return changed;
}

void check_if_SubscriberNeedNotify(void){
	struct subscriber * sub = NULL;
	int wan_changed = 0;

	DEBUG_MSG("\n------------------------------------------\n");
	DEBUG_MSG("\n\n%s (%d): Begin \n",__FUNCTION__,__LINE__);
	DEBUG_MSG("%s (%d): last_WanConnectionStatus is %s (%d) \n"
			,__FUNCTION__,__LINE__
			,last_WanConnectionStatus == 1 ? "Connected" : "Disconnected"
			,last_WanConnectionStatus);

	wan_changed = WanConnectionStatus_changed(last_WanConnectionStatus);
	if(wan_changed){
		last_WanConnectionStatus = !last_WanConnectionStatus;
	}
			
	DEBUG_MSG("%s (%d): subscriber list are \n",__FUNCTION__,__LINE__);
	for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
		
		DEBUG_MSG("uuid = %s, seq = %d, timeout = %ld, service = %s (%d)\n"
				,sub->uuid,sub->seq,sub->timeout
				,sub->service == 1 ? "EWanCFG" : 
					(sub->service == 2 ? "EWanIPC" : 
					(sub->service == 3 ? "EL3F" : "Unknown"))
				,sub->service
				);
		if(sub->service == EWanIPC){ // 2
			if(wan_changed){
				DEBUG_MSG("%s (%d): WanConnectionStatus chagned, go upnp_event_var_change_notify()\n",__FUNCTION__,__LINE__);
				upnp_event_var_change_notify(EWanIPC);
			}
		}
	}
	DEBUG_MSG("%s (%d): End \n",__FUNCTION__,__LINE__);
}

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
int check_notify_right_now(enum subscriber_service_enum service){
	struct subscriber * sub;
	struct upnp_event_notify * obj = NULL;
	int flags;
	int i = 0;
	const char * p;
	unsigned short port;
	struct sockaddr_in addr;
	
	memset(&addr, 0, sizeof(addr));
		
	DEBUG_MSG("\n%s (%d): check notify for AddPortMapping\n"
				,__FUNCTION__,__LINE__);
	for(sub = subscriberlist.lh_first; sub != NULL; sub = sub->entries.le_next) {
		if(sub->service == service && sub->notify == NULL){ //EWanIPC
			//upnp_event_create_notify
			obj = calloc(1, sizeof(struct upnp_event_notify));
			if(!obj) {
				syslog(LOG_ERR, "%s: calloc(): %m", "upnp_event_create_notify");
				return -1;
			}
			obj->sub = sub;
			//obj->state = ECreated;
			obj->s = socket(PF_INET, SOCK_STREAM, 0);
		// if we want here non-block to wait recv(), unmark these
		// recommend: block (so we won't send back TCP RST when control porint return 200 OK)
//  			if((flags = fcntl(obj->s, F_GETFL, 0)) < 0) {
//  				syslog(LOG_ERR, "%s: fcntl(..F_GETFL..): %m",
//  					"upnp_event_create_notify");
//  				free(obj);
//  				return -1;
//  			}
//  			if(fcntl(obj->s, F_SETFL, flags | O_NONBLOCK) < 0) {
//  				syslog(LOG_ERR, "%s: fcntl(..F_SETFL..): %m",
//  					"upnp_event_create_notify");
//  				free(obj);
//  				return -1;
//  			}
			if(sub)
				sub->notify = obj;
			
			//upnp_event_notify_connect
			p = obj->sub->callback;
			p += 7;	/* http:// */
			while(*p != '/' && *p != ':')
				obj->addrstr[i++] = *(p++);
			obj->addrstr[i] = '\0';
			if(*p == ':') {
				obj->portstr[0] = *p;
				i = 1;
				p++;
				port = (unsigned short)atoi(p);
				while(*p != '/') {
					if(i<7) obj->portstr[i++] = *p;
					p++;
				}
				obj->portstr[i] = 0;
			} else {
				port = 80;
				obj->portstr[0] = '\0';
			}
			obj->path = p;
			addr.sin_family = AF_INET;
			inet_aton(obj->addrstr, &addr.sin_addr);
			addr.sin_port = htons(port);
			syslog(LOG_DEBUG, "%s: '%s' %hu '%s'", "upnp_event_notify_connect",
						obj->addrstr, port, obj->path);
			//obj->state = EConnecting;
			if(connect(obj->s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				if(errno != EINPROGRESS && errno != EWOULDBLOCK) {
					syslog(LOG_ERR, "%s: connect(): %m", "upnp_event_notify_connect");
					obj->state = EError;
				}
			}

			//upnp_event_process_notify
			DEBUG_MSG("%s (%d): go upnp_event_prepare()\n",__FUNCTION__,__LINE__);
			upnp_event_prepare(obj);
			//ESending
			DEBUG_MSG("%s (%d): go upnp_event_send()\n",__FUNCTION__,__LINE__);
			upnp_event_send(obj);
			//EWaitingForResponse
 			upnp_event_recv(obj);
			//EFinished
			close(obj->s);
			obj->sub->notify = NULL;
			obj->s = -1;
			free(obj);
			obj = NULL;
		}
	}
	DEBUG_MSG("%s (%d): End\n",__FUNCTION__,__LINE__);
	return 1;
}
#endif
// -----------

#endif // end #ifdef ENABLE_EVENTS

