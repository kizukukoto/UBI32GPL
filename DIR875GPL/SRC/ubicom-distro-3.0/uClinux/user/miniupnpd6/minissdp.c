/* $Id: minissdp.c,v 1.18 2009/11/06 20:18:17 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2009 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <net/if.h>
#include "config.h"
#include "upnpdescstrings.h"
#include "miniupnpdpath.h"
#include "upnphttp.h"
#include "upnpglobalvars.h"
#include "minissdp.h"
#include "codelength.h"
#include "addressManip.h"

#define MAXHOSTNAMELEN 64

#ifdef MINIUPNPD_DEBUG_SSDP
#define DEBUG_MSG(fmt, arg...)       printf("%s (%s) [%d]: "fmt, __FILE__, __func__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/* SSDP ip/port */
#define SSDP_PORT (1900)
#define SSDP_MCAST_ADDR ("FF05::C")    // IPv6 Modification
#define LL_SSDP_MCAST_ADDR ("FF02::C") // IPv6 Modification
#define SL_SSDP_MCAST_ADDR ("FF05::C") // IPv6 Modification
#define SLEEP_VALUE 0.8

static int
AddMulticastMembership(int s, int ifindex, int linklocal) // IPv6 Modification
{
	struct ipv6_mreq LL_ipv6mr;	/* Ip multicast membership */ // IPv6 Modification
	struct ipv6_mreq SL_ipv6mr;

	/* setting up imr structure */
	inet_pton(AF_INET6, LL_SSDP_MCAST_ADDR, &(LL_ipv6mr.ipv6mr_multiaddr)); // IPv6 Modification

	/*imr.imr_interface.s_addr = htonl(INADDR_ANY);*/
	LL_ipv6mr.ipv6mr_interface = ifindex; //v6 use the ifindex not the ifaddr // IPv6 Modification

	inet_pton(AF_INET6, SL_SSDP_MCAST_ADDR, &(SL_ipv6mr.ipv6mr_multiaddr)); // IPv6 Modification
	SL_ipv6mr.ipv6mr_interface = ifindex; 

	if(!linklocal)
	{
		if (setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (void *)&SL_ipv6mr, sizeof(struct ipv6_mreq)) < 0)
		{
			syslog(LOG_ERR, "setsockopt(udp, IPV6_ADD_MEMBERSHIP): %m");
			return -1;
		}
		else
			syslog(LOG_INFO, "Listening on SiteLocal Multicast Address (%s)\n", SL_SSDP_MCAST_ADDR);
	}
	else
	{
		if (setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (void *)&LL_ipv6mr, sizeof(struct ipv6_mreq)) < 0)
		{
			syslog(LOG_ERR, "setsockopt(udp, IPV6_ADD_MEMBERSHIP): %m");
			return -1;
		}
		else
			syslog(LOG_INFO, "Listening on LinkLocal Multicast Address (%s)\n", LL_SSDP_MCAST_ADDR);
	}

	return 0;
}

/* Open and configure the socket listening for
 * SSDP v4 udp packets sent on 239.255.255.250 port 1900 
 * SSDP v6 udp packets sent on FF02::C, or FF05::C, port 1900*/
int
OpenAndConfSSDPReceiveSocket()
{
	int s;
	int i;
	int j = 1;
	int if_index=0, linklocal=-1;// IPv6 add
	//extern const struct in6_addr in6addr_any; // IPv6 add
	struct sockaddr_in6 sockname; // IPv6 Modification
	struct ifreq IfReq; // IPv6 Modification


	if( (s = socket(PF_INET6, SOCK_DGRAM, 0)) < 0) // IPv6 Modification
	{
		syslog(LOG_ERR, "SSDPRecv: socket(udp): %m");
		return -1;
	}

	memset(&sockname, 0, sizeof(struct sockaddr_in6)); // IPv6 Modification
	sockname.sin6_family = AF_INET6; // IPv6 Modification
	sockname.sin6_port = htons(SSDP_PORT); // IPv6 Modification
	/* NOTE : it seems it doesnt work when binding on the specific address */
	/*sockname.sin_addr.s_addr = inet_addr(UPNP_MCAST_ADDR);*/
	sockname.sin6_addr = in6addr_any; // IPv6 Modification
	/*sockname.sin_addr.s_addr = inet_addr(ifaddr);*/

	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &j, sizeof(j)) < 0)
	{
		syslog(LOG_WARNING, "SSDPRecv: setsockopt(udp, SO_REUSEADDR): %m");
	}

	if(bind(s, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in6)) < 0) // IPv6 Modification
	{
		syslog(LOG_ERR, "SSDPRecv: bind(udp): %m");
		close(s);
		return -1;
	}

	i = n_lan_addr;
	while(i>0)
	{
		i--;
		DEBUG_MSG("lan_addr[i].str (%s)\n",lan_addr[i].str);
		if(!strncmp(lan_addr[i].str, "fe80", 4) && strncmp(lan_addr[i].str, "::", 2))
			linklocal = 1;
		else
			linklocal = 0;
#ifdef DEBUG
		fprintf(stderr, "\tSSDPRecv: inet, str: %s\n", lan_addr[i].str);// IPv6 add
#endif
//		getifindex(lan_addr[i].str, &if_index);// IPv6 Modification
		
		memset(&IfReq,0,sizeof(IfReq));
		memcpy( IfReq.ifr_name, "br0", 3 );

 		if (ioctl(s, SIOCGIFINDEX, &IfReq ) < 0) {
			syslog(LOG_ERR, "ioctl SIOCGIFINDEX for %s", IfReq.ifr_name);
			return -1;
		}
		if_index = IfReq.ifr_ifindex;
		DEBUG_MSG("if_index=(%d)========linklocal=(%d)============\n", if_index, linklocal);
		if(AddMulticastMembership(s, if_index, linklocal) < 0) // IPv6 Modification
		{
			syslog(LOG_WARNING,
			       "SSDPRecv: Failed to add multicast membership for address %s",
			       lan_addr[i].str );
		}
	}

	return s;
}

/* open the UDP socket used to send SSDP notifications to
 * the multicast group reserved for them */
static int
OpenAndConfSSDPNotifySocket(struct in6_addr addr)
{
	int s;
	unsigned int on = 0; // IPv6 Modification
	char str[40]=""; // IPv6 Add
	int ifindex = 0; // IPv6 Add
	int bcast = 1;
	struct in6_addr ipv6mc_if; // IPv6 Modification
	struct sockaddr_in6 sockname; // IPv6 Modification
	struct ifreq IfReq; // IPv6 Modification

	if( (s = socket(PF_INET6, SOCK_DGRAM, 0)) < 0) // IPv6 Modification
	{
		syslog(LOG_ERR, "SSDPNotif: socket(udp_notify): %m");
		return -1;
	}

	ipv6mc_if = addr;	/*inet_addr(addr);*/ // IPv6 Modification

	if(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &on, sizeof(on)) < 0) // IPv6 Modification
	{
		syslog(LOG_ERR, "SSDPNotif: setsockopt(udp_notify, IPV6_MULTICAST_LOOP): %m");
		close(s);
		return -1;
	}

	inet_ntop(AF_INET6, &addr, str, INET6_ADDRSTRLEN);// IPv6 Add
	DEBUG_MSG("SSDPNotif: inet, str: %s\n", str);	
#ifdef DEBUG
	fprintf(stderr, "\tSSDPNotif: inet, str: %s\n", str);// IPv6 add
#endif
//	getifindex(str, &ifindex);// IPv6 Add
	memset(&IfReq,0,sizeof(IfReq));
	memcpy( IfReq.ifr_name, "br0", 3 );
 
 	if (ioctl(s, SIOCGIFINDEX, &IfReq ) < 0) {
		syslog(LOG_ERR, "ioctl SIOCGIFINDEX for %s", IfReq.ifr_name);
		return -1;
	}
	ifindex = IfReq.ifr_ifindex;
	DEBUG_MSG("ifindex = (%d)\n", ifindex);

	if(IN6_IS_ADDR_LINKLOCAL(&addr))
	{
		DEBUG_MSG("\tFind an linklocal address remenber its index.\n");
		linklocal_index = ifindex;
	}

	if(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) // IPv6 Modification
	{
		syslog(LOG_ERR, "SSDPNotif: setsockopt(udp_notify, IPV6_MULTICAST_IF): %m");
		close(s);
		return -1;
	}

	if(setsockopt(s, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)) < 0)
	{
		syslog(LOG_ERR, "SSDPNotif: setsockopt(udp_notify, SO_BROADCAST): %m");
		close(s);
		return -1;
	}

	memset(&sockname, 0, sizeof(struct sockaddr_in6)); // IPv6 Modification
	sockname.sin6_family = AF_INET6; // IPv6 Modification
	sockname.sin6_addr = addr;	/*inet_addr(addr);*/ // IPv6 Modification
	if(!strncmp(str, "fe80", 4))
		sockname.sin6_scope_id = ifindex;

	if (bind(s, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in6)) < 0) // IPv6 Modification
	{
		syslog(LOG_ERR, "SSDPNotif: bind(udp_notify): %m");
		close(s);
		return -1;
	}

	return s;
}

int
OpenAndConfSSDPNotifySockets(int * sockets)
/*OpenAndConfSSDPNotifySockets(int * sockets,
                             struct lan_addr_s * lan_addr, int n_lan_addr)*/
{
	int i, j;

	for(i=0; i<n_lan_addr; i++)
	{
		sockets[i] = OpenAndConfSSDPNotifySocket(lan_addr[i].addr);
		if(sockets[i] < 0)
		{
			for(j=0; j<i; j++)
			{
				close(sockets[j]);
				sockets[j] = -1;
			}
			return -1;
		}
	}
	return 0;
}

/*
 * response from a LiveBox (Wanadoo)
HTTP/1.1 200 OK
CACHE-CONTROL: max-age=1800
DATE: Thu, 01 Jan 1970 04:03:23 GMT
EXT:
LOCATION: http://192.168.0.1:49152/gatedesc.xml
SERVER: Linux/2.4.17, UPnP/1.0, Intel SDK for UPnP devices /1.2
ST: upnp:rootdevice
USN: uuid:75802409-bccb-40e7-8e6c-fa095ecce13e::upnp:rootdevice

 * response from a Linksys 802.11b :
HTTP/1.1 200 OK
Cache-Control:max-age=120
Location:http://192.168.5.1:5678/rootDesc.xml
Server:NT/5.0 UPnP/1.0
ST:upnp:rootdevice
USN:uuid:upnp-InternetGatewayDevice-1_0-0090a2777777::upnp:rootdevice
EXT:
 */

/* not really an SSDP "announce" as it is the response
 * to a SSDP "M-SEARCH" */
static void
SendSSDPAnnounce2(int s, struct sockaddr_in6 sockname,
                  const char * st, int st_len, const char * suffix,
                  const char * usn, const char * host, unsigned short port) // IPv6 Modification
{
	int l, n;
	char buf[512], expire_time[32];
	time_t expire = time(NULL);
	int strlen;

	strlen = strftime(expire_time, sizeof(expire_time), "%a, %d %b %Y %H:%M:%S \"GMT\"", gmtime(&expire));

	/*
	 * follow guideline from document "UPnP Device Architecture 1.0"
	 * uppercase is recommended.
	 * DATE: is recommended
	 * SERVER: OS/ver UPnP/1.0 miniupnpd/1.0
	 * - check what to put in the 'Cache-Control' header
	 * */
	l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
		// According to the UDA should be greater than or equal to 1800 but for test purpose have set it to 120
		"CACHE-CONTROL: max-age=120\r\n"
		"DATE: %s\r\n"
		"EXT:\r\n"
		"LOCATION: http://[%s]:%u" ROOTDESC_PATH "\r\n"//Ipv6 modification
		"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
		"ST: %.*s%s\r\n"
		"USN: %s%s\r\n"//%s::%.*s%s
		// Following Header are needed in the UDA 1.1
		"BOOTID.UPNP.ORG: %d\r\n"
		"CONFIGID.UPNP.ORG: %d\r\n"
		//"SEARCHPORT.UPNP.ORG: %d\r\n"
		"\r\n",
		expire_time, host, (unsigned int)port,
		st_len, st, suffix,
		usn, suffix, upnp_bootid, upnp_configid);
	DEBUG_MSG("\tSending M-SEARCH response:\n %.*s", l, buf);
	n = sendto(s, buf, l, 0,
	           (struct sockaddr *)&sockname, sizeof(struct sockaddr_in6) ); // IPv6 Modification
	if(n < 0)
	{
		syslog(LOG_ERR, "sendto(udp): %m");
	}
}
static const struct
{
	const char * uid;
	const char * name;
}
known_service_types[] =
{
	{ uuidvalue, "upnp:rootdevice"},
	{ uuidvalue, "urn:schemas-upnp-org:device:InternetGatewayDevice:2"},
	{ wanconuuid, "urn:schemas-upnp-org:device:WANConnectionDevice:2"},
	{ wandevuuid, "urn:schemas-upnp-org:device:WANDevice:2"},
	{ wandevuuid, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"},
	{ wanconuuid, "urn:schemas-upnp-org:service:WANIPConnection:2"},
	//{ wanconuuid, "urn:schemas-upnp-org:service:WANPPPConnection:2"},
#ifdef ENABLE_L3F_SERVICE
	{ uuidvalue, "urn:schemas-upnp-org:service:Layer3Forwarding:1"},
#endif
	{ wanconuuid, "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1"},
	{ uuidvalue, 0},
	{ wanconuuid, 0},
	{ wandevuuid, 0},
	{ 0, 0}
};

static void
SendSSDPNotifies(int s, const char * host, unsigned short port,
                 unsigned int lifetime)
{
	struct sockaddr_in6 sockname; // IPv6 Modification
	int l, n, i=0, linklocal=-1;
	char bufr[512];


	if(!strncmp(host, "fe80", 4)){
		linklocal = 1;
	}
	else{
		linklocal = 0;
	}
	DEBUG_MSG("\tNotify Send on %s (%d)\n", (linklocal)? "Linklocal":"Global", linklocal);

	memset(&sockname, 0, sizeof(struct sockaddr_in6)); // IPv6 Modification
	sockname.sin6_family = AF_INET6; // IPv6 Modification
	sockname.sin6_port = htons(SSDP_PORT); // IPv6 Modification
	if(linklocal)
		inet_pton(AF_INET6, LL_SSDP_MCAST_ADDR, &(sockname.sin6_addr)); // IPv6 Modification
	else
		inet_pton(AF_INET6, SL_SSDP_MCAST_ADDR, &(sockname.sin6_addr));

	while(known_service_types[i].uid)
	{
		if(known_service_types[i].name)
			l = snprintf(bufr, sizeof(bufr),
					"NOTIFY * HTTP/1.1\r\n"
					"HOST: [%s]:%d\r\n"//Ipv6 modification
					"CACHE-CONTROL: max-age=%u\r\n"
					"LOCATION: http://[%s]:%d" ROOTDESC_PATH"\r\n"//Ipv6 modification
					"NT: %s\r\n"
					"NTS: ssdp:alive\r\n"
					/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
					"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
					"USN: %s::%s\r\n"
					// Following Header are needed in the UDA 1.1
					"BOOTID.UPNP.ORG: %d\r\n"
					"CONFIGID.UPNP.ORG: %d\r\n"
					//"SEARCHPORT.UPNP.ORG: %d\r\n"
					"\r\n",
					(linklocal? LL_SSDP_MCAST_ADDR:SL_SSDP_MCAST_ADDR), SSDP_PORT,
					lifetime,
					host, port,
					known_service_types[i].name,
					known_service_types[i].uid, known_service_types[i].name, upnp_bootid, upnp_configid);
		else
			l = snprintf(bufr, sizeof(bufr),
					"NOTIFY * HTTP/1.1\r\n"
					"HOST: [%s]:%d\r\n"//Ipv6 modification
					"CACHE-CONTROL: max-age=%u\r\n"
					"LOCATION: http://[%s]:%d" ROOTDESC_PATH"\r\n"//Ipv6 modification
					"NT: %s\r\n"
					"NTS: ssdp:alive\r\n"
					/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
					"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
					"USN: %s\r\n"
					// Following Header are needed in the UDA 1.1
					"BOOTID.UPNP.ORG: %d\r\n"
					"CONFIGID.UPNP.ORG: %d\r\n"
					//"SEARCHPORT.UPNP.ORG: %d\r\n"
					"\r\n",
					(linklocal? LL_SSDP_MCAST_ADDR:SL_SSDP_MCAST_ADDR), SSDP_PORT,
					lifetime,
					host, port,
					known_service_types[i].uid,
					known_service_types[i].uid, upnp_bootid, upnp_configid);
		if(l>=sizeof(bufr))
		{
			syslog(LOG_WARNING, "SendSSDPNotifies(): truncated output");
			l = sizeof(bufr);
		}
		sleep(SLEEP_VALUE);
		DEBUG_MSG("\tSending SendSSDPNotifies: %s\n", bufr);
		n = sendto(s, bufr, l, 0,
			(struct sockaddr *)&sockname, sizeof(struct sockaddr_in6) ); // IPv6 Modification
		if(n < 0)
		{
			DEBUG_MSG("err sendto(udp_notify=%d, %s): %m", s, host);
			syslog(LOG_ERR, "sendto(udp_notify=%d, %s): %m", s, host);
		}
		i++;
	}
}

void
SendSSDPNotifies2(int * sockets,
                  unsigned short port,
                  unsigned int lifetime)
/*SendSSDPNotifies2(int * sockets, struct lan_addr_s * lan_addr, int n_lan_addr,
                  unsigned short port,
                  unsigned int lifetime)*/
{
	int i;

	for(i=0; i<n_lan_addr; i++)
	{
		DEBUG_MSG("lan_addr[i].str (%s)\n", lan_addr[i].str);	
		SendSSDPNotifies(sockets[i], lan_addr[i].str, port, lifetime);
	}
}

/* ProcessSSDPRequest()
 * process SSDP M-SEARCH requests and responds to them */
void
ProcessSSDPRequest(int s, unsigned short port)
/*ProcessSSDPRequest(int s, struct lan_addr_s * lan_addr, int n_lan_addr,
                   unsigned short port)*/
{
	int n;
	char bufr[1500], usn[128]="";
	socklen_t len_r;
	struct sockaddr_in6 sendername;
	char senderAddr[INET6_ADDRSTRLEN]="", hostAddr[INET6_ADDRSTRLEN]=""; // IPv6 Modification
	char senderMask[INET6_ADDRSTRLEN]="", hostMask[INET6_ADDRSTRLEN]=""; // IPv6 Modification
	int i, j, k, l;
	int lan_addr_index = 0;
	char * st = 0, * host = 0, * man = 0, * mx = 0;
	int st_len = 0, host_len = 0, man_len = 0, mx_len = 0;
	len_r = sizeof(struct sockaddr_in6); // IPv6 Modification

	n = recvfrom(s, bufr, sizeof(bufr), 0,
	             (struct sockaddr *)&sendername, &len_r); // IPv6 Modification
DEBUG_MSG("BUG! return ProcessSSDPRequest \n");
return;
	if(n < 0)
	{
		syslog(LOG_ERR, "recvfrom(udp): %m");
		return;
	}

	if(memcmp(bufr, "NOTIFY", 6) == 0)
	{
		/* ignore NOTIFY packets. We could log the sender and device type */
		return;
	}
	else if(memcmp(bufr, "M-SEARCH * HTTP/1.1", 19) == 0)
	// Modification to be able to pass certification, do not respond to not well-formed M-SEARCH request
	{
		DEBUG_MSG("\n");
		i = 0;
		while(i < n)
		{
			while((i < n - 1) && (bufr[i] != '\r' || bufr[i+1] != '\n'))
				i++;
			i += 2;
			if((i < n - 3) && (strncasecmp(bufr+i, "st:", 3) == 0))
			{
				st = bufr+i+3;
				st_len = 0;
				while((*st == ' ' || *st == '\t') && (st < bufr + n))
					st++;
				while(st[st_len]!='\r' && st[st_len]!='\n'
				     && (st + st_len < bufr + n))
					st_len++;
				/*syslog(LOG_INFO, "ST: %.*s", st_len, st);*/
				/*j = 0;*/
				/*while(bufr[i+j]!='\r') j++;*/
				/*syslog(LOG_INFO, "%.*s", j, bufr+i);*/
			}
			if((i < n - 5) && (strncasecmp(bufr+i, "host:", 5) == 0))
			{
				host = bufr+i+5;
				host_len = 0;
				while((*host == ' ' || *host == '\t') && (host < bufr + n))
					host++;
				while(host[host_len]!='\r' && host[host_len]!='\n'
				     && (host + host_len < bufr + n))
					host_len++;
			}
			if((i < n - 4) && (strncasecmp(bufr+i, "man:", 4) == 0))
			{
				man = bufr+i+4;
				man_len = 0;
				while((*man == ' ' || *man == '\t') && (man < bufr + n))
					man++;
				while(man[man_len]!='\r' && man[man_len]!='\n'
				     && (man + man_len < bufr + n))
					man_len++;
			}
			if((i < n - 3) && (strncasecmp(bufr+i, "mx:", 3) == 0))
			{
				mx = bufr+i+3;
				mx_len = 0;
				while((*mx == ' ' || *mx == '\t') && (mx < bufr + n))
					mx++;
				while(mx[mx_len]!='\r' && mx[mx_len]!='\n'
				     && (mx + mx_len < bufr + n))
					mx_len++;
			}
		}
		DEBUG_MSG("\n");
		/*syslog(LOG_INFO, "SSDP M-SEARCH packet received from %s:%d",
	           inet_ntoa(sendername.sin_addr),
	           ntohs(sendername.sin_port) );*/
		inet_ntop(AF_INET6, &(sendername.sin6_addr), senderAddr, INET6_ADDRSTRLEN);
		sleep(SLEEP_VALUE);
		if(st && (st_len > 0) && man && mx && atoi(mx)>0)
		{
			DEBUG_MSG("\n");
			/* TODO : doesnt answer at once but wait for a random time */
			//fprintf(stderr, "Addr Source: %s\n", senderAddr);
			syslog(LOG_INFO, "SSDP M-SEARCH from [%s]:%d ST: %.*s on %.*s",
	        	   senderAddr, // IPv6 Modification
	           	   ntohs(sendername.sin6_port),
				   st_len, st, host_len, host);
			/* find in which sub network the client is */
			for(i = 0; i<n_lan_addr; i++)
			{
			    DEBUG_MSG("\n");
			    int count = 0;
			    DEBUG_MSG("(%s)\n",senderAddr);
			    if(strncmp(senderAddr, ":", 1)==0)
			    	break;
			    del_char(senderAddr);
			    DEBUG_MSG("\n");
			    inet_ntop(AF_INET6, &(lan_addr[i].addr.s6_addr), hostAddr, INET6_ADDRSTRLEN);
			    DEBUG_MSG("\n");
			    inet_ntop(AF_INET6, &(lan_addr[i].mask.s6_addr), hostMask, INET6_ADDRSTRLEN);
			    DEBUG_MSG("\n");
			    del_char(hostAddr);
			    DEBUG_MSG("\n");
			    del_char(hostMask);
			    DEBUG_MSG("\n");
			    char * p;
			    DEBUG_MSG("\n");
			    p=hostMask;
			    DEBUG_MSG("\n");
			    while(*p!='0')
			    {
			    	DEBUG_MSG("\n");
			    	p++;count++;
			    }
			    /*for(j=0; j<sizeof(struct in6_addr)/sizeof(int); j++) // IPv6 modification
			    {
			    printf("Mask comparison:\nsender: %s\n  host: %s - Mask: %s\nsender + mask: %u\n  host + mask: %u\n", senderAddr, hostAddr, hostMask, (((long) sendername.sin6_addr.s6_addr) & ((long) lan_addr[i].mask.s6_addr)), ( ((long) lan_addr[i].addr.s6_addr) & ((long) lan_addr[i].mask.s6_addr) ));
			        if( ( ((int) sendername.sin6_addr.s6_addr) & ((int) lan_addr[i].mask.s6_addr) ) 
				   == ( ((int) lan_addr[i].addr.s6_addr) & ((int) lan_addr[i].mask.s6_addr) ) )
				   {
				       //count++;
				       printf("find a match\n");
				       lan_addr_index = i;
				       break;
				   }*/
				/*else
				{
				       break;
				}
			    }
			    if(count==sizeof(struct in6_addr)/sizeof(int))
			    {
			    	lan_addr_index = i;
			    	break;
			    }*/
			    if(!strncmp(senderAddr, hostAddr, count))
			    {
			    	DEBUG_MSG("\n");
			    	lan_addr_index = i;
			    }
			    DEBUG_MSG("\n");
			}
			DEBUG_MSG("\n");
			/* Responds to request with a device as ST header */
			for(i = 0; known_service_types[i].name; i++)
			{
				l = (int)strlen(known_service_types[i].name);
				DEBUG_MSG("\n");
				if(l==st_len && (0 >= memcmp(st, known_service_types[i].name, l)))
				{
					if(strcmp(tmp_char_1, "")==0)
					{
						strcpy(tmp_char_1, senderAddr);
					}
					else if(strcmp(tmp_char_2, "")==0 && strcmp(tmp_char_1, senderAddr)!=0)
					{
						strcpy(tmp_char_2, senderAddr);
					}
					
					if(strcmp(tmp_char_1, senderAddr)==0)
					{
						count_1 ++;
						DEBUG_MSG("\n");
						printf("\tReceived %d m-search from %s\n", count_1, tmp_char_1);
					}
					else if(strcmp(tmp_char_2, senderAddr)==0)
					{
						count_2 ++;
						DEBUG_MSG("\n");
						printf("\tReceived %d m-search from %s\n", count_2, tmp_char_2);
					}
					k = snprintf(usn, sizeof(usn), "%s::%.*s", known_service_types[i].uid, l, known_service_types[i].name);
					DEBUG_MSG("\n");
					SendSSDPAnnounce2(s, sendername,
					                  known_service_types[i].name, st_len, "",
					                  usn, lan_addr[lan_addr_index].str, port);
					DEBUG_MSG("\n");
					break;
				}
			}
			DEBUG_MSG("\n");
			/* Responds to request with ST: ssdp:all */
			/* strlen("ssdp:all") == 8 */
			if(st_len==8 && (0 == memcmp(st, "ssdp:all", 8)))
			{
				DEBUG_MSG("\n");
				for(i=0; known_service_types[i].uid; i++)
				{
					if(known_service_types[i].name)
					{
						l = (int)strlen(known_service_types[i].name);
						k = snprintf(usn, sizeof(usn), "%s::%.*s", known_service_types[i].uid, l, known_service_types[i].name);
						DEBUG_MSG("\n");
						SendSSDPAnnounce2(s, sendername,
						                  known_service_types[i].name, l, "",
						                  usn, lan_addr[lan_addr_index].str, port);
					}
					else
					{
						l = (int)strlen(known_service_types[i].uid);
						k = snprintf(usn, sizeof(usn), "%s", known_service_types[i].uid);
						DEBUG_MSG("\n");
						SendSSDPAnnounce2(s, sendername,
						                  known_service_types[i].uid, l, "",
						                  usn, lan_addr[lan_addr_index].str, port);
					}
				}
			}
			DEBUG_MSG("\n");
			/* responds to request by UUID value */
			if(strncmp(st, "uuid:", 5)==0)
			{
				l = (int)strlen(uuidvalue);
				DEBUG_MSG("\n");
				if(l==st_len && (0 == memcmp(st, uuidvalue, l))){
					DEBUG_MSG("\n");
					SendSSDPAnnounce2(s, sendername, st, st_len, "",
					                  uuidvalue, lan_addr[lan_addr_index].str, port);
				}
				else if(l==st_len && (0 == memcmp(st, wanconuuid, l))){
					DEBUG_MSG("\n");
					SendSSDPAnnounce2(s, sendername, st, st_len, "",
					                  wanconuuid, lan_addr[lan_addr_index].str, port);
				}
				else if(l==st_len && (0 == memcmp(st, wandevuuid, l))){
					DEBUG_MSG("\n");
					SendSSDPAnnounce2(s, sendername, st, st_len, "",
					                  wandevuuid, lan_addr[lan_addr_index].str, port);
				}
				else
					printf("\tUnknown uuid value %.*s\n", st_len, st);
			}
			DEBUG_MSG("\n");
		}
		else
		{
			syslog(LOG_INFO, "Invalid SSDP M-SEARCH from [%s]:%d",
	        	   senderAddr, ntohs(sendername.sin6_port)); // IPv6 Modification
		}
		DEBUG_MSG("\n");
	}
	else
	{
		syslog(LOG_NOTICE, "Unknown udp packet received from [%s]:%d",
		       senderAddr, ntohs(sendername.sin6_port)); // IPv6 Modification
	}
	DEBUG_MSG("\n");
}

/* This will broadcast ssdp:byebye notifications to inform
 * the network that UPnP is going down. */
int
SendSSDPGoodbye(int * sockets, int n_sockets)
{
	struct sockaddr_in6 sockname; // IPv6 Modification
	int n, l;
	int i, j;
	char bufr[512];

	memset(&sockname, 0, sizeof(struct sockaddr_in6)); // IPv6 Modification
	sockname.sin6_family = AF_INET6; // IPv6 Modification
	sockname.sin6_port = htons(SSDP_PORT); // IPv6 Modification
	inet_pton(AF_INET6, SSDP_MCAST_ADDR, &(sockname.sin6_addr)); // IPv6 Modification

	for(j=0; j<n_sockets; j++)
	{
	    for(i=0; known_service_types[i].uid; i++)
	    {
		if(known_service_types[i].name)
		        l = snprintf(bufr, sizeof(bufr),
		                 "NOTIFY * HTTP/1.1\r\n"
		                 "HOST: [%s]:%d\r\n"//Ipv6 modification
		                 "NT: %s\r\n"
		                 "NTS: ssdp:byebye\r\n"
		                 "USN: %s::%s\r\n"
		                 // Following Header are needed in the UDA 1.1
		                 "BOOTID.UPNP.ORG: %d\r\n"
		                 "CONFIGID.UPNP.ORG: %d\r\n"
		                 "\r\n",
		                 SSDP_MCAST_ADDR, SSDP_PORT, known_service_types[i].name,
		                 known_service_types[i].uid, known_service_types[i].name, upnp_bootid, upnp_configid);
		else
		        l = snprintf(bufr, sizeof(bufr),
		                 "NOTIFY * HTTP/1.1\r\n"
		                 "HOST: [%s]:%d\r\n"//Ipv6 modification//v6 []
		                 "NT: %s\r\n"
		                 "NTS: ssdp:byebye\r\n"
		                 "USN: %s\r\n"
		                 // Following Header are needed in the UDA 1.1
		                 "BOOTID.UPNP.ORG: %d\r\n"
		                 "CONFIGID.UPNP.ORG: %d\r\n"
		                 "\r\n",
		                 SSDP_MCAST_ADDR, SSDP_PORT, known_service_types[i].uid,
		                 known_service_types[i].uid, upnp_bootid, upnp_configid);
		sleep(SLEEP_VALUE);
		DEBUG_MSG("\tSending bye-bye message: %s\n", known_service_types[i].name?known_service_types[i].name:known_service_types[i].uid);
	        n = sendto(sockets[j], bufr, l, 0,
	                   (struct sockaddr *)&sockname, sizeof(struct sockaddr_in6) );
			if(n < 0)
			{
				syslog(LOG_ERR, "SendSSDPGoodbye: sendto(udp_shutdown=%d): %m",
				       sockets[j]);
				return -1;
			}
    	    }
	}
	return 0;
}

/* SubmitServicesToMiniSSDPD() :
 * register services offered by MiniUPnPd to a running instance of
 * MiniSSDPd */
int
SubmitServicesToMiniSSDPD(const char * host, unsigned short port) {
	struct sockaddr_un addr;
	int s;
	unsigned char buffer[2048];
	char strbuf[256];
	unsigned char * p;
	int i, l;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s < 0)
	{
		syslog(LOG_ERR, "socket(unix): %m");
		return -1;
	}
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, minissdpdsocketpath, sizeof(addr.sun_path));
	if(connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0)
	{
		syslog(LOG_ERR, "connect(\"%s\"): %m", minissdpdsocketpath);
		return -1;
	}
	for(i = 0; known_service_types[i].name; i++)
	{
		buffer[0] = 4;
		p = buffer + 1;
		l = (int)strlen(known_service_types[i].name);
		if(i > 0)
			l++;
		CODELENGTH(l, p);
		memcpy(p, known_service_types[i].name, l);
		if(i > 0)
			p[l-1] = '1';
		p += l;
		l = snprintf(strbuf, sizeof(strbuf), "%s::%s%s",
		             known_service_types[i].uid, known_service_types[i].name, "");
		CODELENGTH(l, p);
		memcpy(p, strbuf, l);
		p += l;
		l = (int)strlen(MINIUPNPD_SERVER_STRING);
		CODELENGTH(l, p);
		memcpy(p, MINIUPNPD_SERVER_STRING, l);
		p += l;
		l = snprintf(strbuf, sizeof(strbuf), "http://[%s]:%u" ROOTDESC_PATH,//Ipv6 modification
		             host, (unsigned int)port);
		CODELENGTH(l, p);
		memcpy(p, strbuf, l);
		p += l;
		if(write(s, buffer, p - buffer) < 0)
		{
			syslog(LOG_ERR, "write(): %m");
			return -1;
		}
	}
 	close(s);
	return 0;
}

