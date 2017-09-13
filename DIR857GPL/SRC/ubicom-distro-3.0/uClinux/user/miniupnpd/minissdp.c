/* $Id: minissdp.c,v 1.5 2009/06/11 03:46:38 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <stdlib.h>
#include "config.h"
#include "upnpdescstrings.h"
#include "miniupnpdpath.h"
#include "upnphttp.h"
#include "upnpglobalvars.h"
#include "minissdp.h"
#include "codelength.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef MINIUPNPD_DEBUG_SSDP
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/* SSDP ip/port */
#define SSDP_PORT (1900)
#define SSDP_MCAST_ADDR ("239.255.255.250")


static int
AddMulticastMembership(int s, in_addr_t ifaddr)
{
	struct ip_mreq imr;	/* Ip multicast membership */

    /* setting up imr structure */
    imr.imr_multiaddr.s_addr = inet_addr(SSDP_MCAST_ADDR);
    /*imr.imr_interface.s_addr = htonl(INADDR_ANY);*/
    imr.imr_interface.s_addr = ifaddr;	/*inet_addr(ifaddr);*/
	
	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&imr, sizeof(struct ip_mreq)) < 0)
	{
        syslog(LOG_ERR, "setsockopt(udp, IP_ADD_MEMBERSHIP): %m");
		return -1;
    }

	return 0;
}

/* Open and configure the socket listening for 
 * SSDP udp packets sent on 239.255.255.250 port 1900 */
int
OpenAndConfSSDPReceiveSocket(void)
{
	int s;
	int i;
	int j=1;
	struct sockaddr_in sockname;
	
	if( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		syslog(LOG_ERR, "socket(udp): %m");
		return -1;
	}	

	
#if 1
	// set SO_REUSEADDR on a socket to true (1):
	int optval = 1;
	int ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	printf("SO_REUSEADDR=%d\n",ret);
#endif

	memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(SSDP_PORT);
	/* NOTE : it seems it doesnt work when binding on the specific address */
    /*sockname.sin_addr.s_addr = inet_addr(UPNP_MCAST_ADDR);*/
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);
    /*sockname.sin_addr.s_addr = inet_addr(ifaddr);*/

	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &j, sizeof(j)) < 0)
	{
		syslog(LOG_WARNING, "SSDPRecv: setsockopt(udp, SO_REUSEADDR): %m");
	}

    if(bind(s, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in)) < 0)
	{
		syslog(LOG_ERR, "bind(udp): %m");
		close(s);
		return -1;
    }

	i = n_lan_addr;
	while(i>0)
	{
		i--;
		if(AddMulticastMembership(s, lan_addr[i].addr.s_addr) < 0)
		{
			syslog(LOG_WARNING,
			       "Failed to add multicast membership for address %s", 
			       lan_addr[i].str );
		}
	}

	return s;
}

/* open the UDP socket used to send SSDP notifications to
 * the multicast group reserved for them */
static int
OpenAndConfSSDPNotifySocket(in_addr_t addr)
{
	int s;
	unsigned char loopchar = 0;
	int bcast = 1;
	struct in_addr mc_if;
	struct sockaddr_in sockname;
	
	if( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		syslog(LOG_ERR, "socket(udp_notify): %m");
		return -1;
	}

	mc_if.s_addr = addr;	/*inet_addr(addr);*/

	if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopchar, sizeof(loopchar)) < 0)
	{
		syslog(LOG_ERR, "setsockopt(udp_notify, IP_MULTICAST_LOOP): %m");
		close(s);
		return -1;
	}

	if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&mc_if, sizeof(mc_if)) < 0)
	{
		syslog(LOG_ERR, "setsockopt(udp_notify, IP_MULTICAST_IF): %m");
		close(s);
		return -1;
	}
	
	if(setsockopt(s, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)) < 0)
	{
		syslog(LOG_ERR, "setsockopt(udp_notify, SO_BROADCAST): %m");
		close(s);
		return -1;
	}

	memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_addr.s_addr = addr;	/*inet_addr(addr);*/

    if (bind(s, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in)) < 0)
	{
		syslog(LOG_ERR, "bind(udp_notify): %m");
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
		sockets[i] = OpenAndConfSSDPNotifySocket(lan_addr[i].addr.s_addr);
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


/* Chun add for WPS-COMPATIBLE */
//void GetUUIDValue(const char * st,char **uuid_value)
int GetUUIDValue(const char * st,char **uuid_value)
{
	/*	Date:	2009-05-12
	*	Name:	jimmy huang
	*	Reason:	For Intel Device Validator
	*			we need to no if this st is device or uuid
	*			device : WANCommonInterfaceConfig...
	*			uuid: uuid:75802409-bccb-40e7-8e6c-fa095ecce13e..
	*/
/*
	if( strcmp(st,"urn:schemas-upnp-org:device:WANDevice:")==0 ||
	 		strcmp(st,"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:")==0 )
	{
		*uuid_value=uuidvalue_2;
	}
	else if( strcmp(st,"urn:schemas-upnp-org:device:WANConnectionDevice:")==0 ||
	 		strcmp(st,"urn:schemas-upnp-org:service:WANPPPConnection:")==0 ||
	 		strcmp(st,"urn:schemas-upnp-org:service:WANIPConnection:")==0 )
	{
		*uuid_value=uuidvalue_3;
	}
	else if( strcmp(st,"urn:schemas-wifialliance-org:device:WFADevice:")==0 ||
	 		strcmp(st,"urn:schemas-wifialliance-org:service:WFAWLANConfig:")==0 )
	{
		*uuid_value=uuidvalue_4;
	}
	else  //rootdevice
	{
		*uuid_value=uuidvalue_1;
	}
*/
	int st_is_uuid = 0;
	int len_dev = strlen("urn:schemas-upnp-org:device:InternetGatewayDevice:");
	int len_srv = strlen("urn:schemas-upnp-org:service:Layer3Forwarding:");
	if (strncmp(st, "urn:schemas-upnp-org:device:InternetGatewayDevice:", len_dev) == 0 ||
			strncmp(st, "urn:schemas-upnp-org:service:Layer3Forwarding:", len_srv) == 0) {
		*uuid_value=uuidvalue_1;
		return st_is_uuid;
	}

	len_dev = strlen("urn:schemas-upnp-org:device:WANDevice:");
	len_srv = strlen("urn:schemas-upnp-org:service:WANCommonInterfaceConfig:");
	if (strncmp(st, "urn:schemas-upnp-org:device:WANDevice:", len_dev) == 0 ||
			strncmp(st, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:", len_srv) == 0) {
		*uuid_value=uuidvalue_2;
		return st_is_uuid;
	}

	len_dev = strlen("urn:schemas-upnp-org:device:WANConnectionDevice:");
	len_srv = strlen("urn:schemas-upnp-org:service:WANIPConnection:");
	if (strncmp(st, "urn:schemas-upnp-org:device:WANConnectionDevice:", len_dev) == 0 ||
			strncmp(st, "urn:schemas-upnp-org:service:WANIPConnection:", len_srv) == 0) {
		*uuid_value=uuidvalue_3;
		return st_is_uuid;
	}

	len_dev = strlen("urn:schemas-wifialliance-org:device:WFADevice:");
	len_srv = strlen("urn:schemas-wifialliance-org:service:WFAWLANConfig:");
	if (strncmp(st, "urn:schemas-wifialliance-org:device:WFADevice:", len_dev) == 0 ||
			strncmp(st, "urn:schemas-wifialliance-org:service:WFAWLANConfig:", len_srv) == 0) {
		*uuid_value=uuidvalue_4;
		return st_is_uuid;
	}

	if(strncmp(st,uuidvalue_1,strlen(uuidvalue_1))==0)
	{
		*uuid_value=uuidvalue_1;
		st_is_uuid = 1;
	}else if(strncmp(st,uuidvalue_2,strlen(uuidvalue_2))==0)
	{
		*uuid_value=uuidvalue_2;
		st_is_uuid = 1;
	}else if(strncmp(st,uuidvalue_3,strlen(uuidvalue_3))==0)
	{
		*uuid_value=uuidvalue_3;
		st_is_uuid = 1;
	}else if(strncmp(st,uuidvalue_4,strlen(uuidvalue_4))==0)
	{
		*uuid_value=uuidvalue_4;
		st_is_uuid = 1;
	}else{
		//rootdevice
		*uuid_value=uuidvalue_1;
	}
	return st_is_uuid;

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
SendSSDPAnnounce2(int s, struct sockaddr_in sockname,
                  const char * st, int st_len, const char * suffix,
                  const char * host, unsigned short port)
{
	int l, n;
	char buf[512];
	//jimm test
	char *tmp_uuid_value=NULL;// Chun add for WPS-COMPATIBLE
	//GetUUIDValue(st,&tmp_uuid_value);
	
	int st_is_uuid = 0;
	st_is_uuid = GetUUIDValue(st,&tmp_uuid_value);
	/* 
	 * follow guideline from document "UPnP Device Architecture 1.0"
	 * uppercase is recommended.
	 * DATE: is recommended
	 * SERVER: OS/ver UPnP/1.0 miniupnpd/1.0
	 * - check what to put in the 'Cache-Control' header 
	 * */
#if 0
	l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
		"CACHE-CONTROL: max-age=120\r\n"
		/*"DATE: ...\r\n"*/
		"ST: %.*s%s\r\n"
		"USN: %s::%.*s%s\r\n"
		"EXT:\r\n"
		"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
		"LOCATION: http://%s:%u" ROOTDESC_PATH "\r\n"
		"\r\n",
		st_len, st, suffix,
		uuidvalue, st_len, st, suffix,
		host, (unsigned int)port);
#endif
	/*	Date:	2009-05-12
	*	Name:	jimmy huang
	*	Reason:	For Intel Device Validator
	*/
	/*
	l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
		"Cache-Control: max-age=120\r\n"
		"EXT:\r\n"
		"Location: http://%s:%u" ROOTDESC_PATH "\r\n"
		"Server: " MINIUPNPD_SERVER_STRING "\r\n"
		"ST: %.*s\r\n"
		"USN: %s::%.*s\r\n"
		"\r\n",
		host, (unsigned int)port,
		st_len, st,
		tmp_uuid_value, st_len, st
		);
	*/
	if(st_is_uuid == 0){
		if((st[st_len-1] == ':') && (st[st_len] != '1')){
			// lack of 1
			//ex urn:schemas-wifialliance-org:service:WFAWLANConfig:
			// should be urn:schemas-wifialliance-org:service:WFAWLANConfig:1
			l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
				"Cache-Control: max-age=120\r\n"
				"EXT:\r\n"
				"Location: http://%s:%u" ROOTDESC_PATH "\r\n"
				"Server: " MINIUPNPD_SERVER_STRING "\r\n"
				"ST: %.*s1\r\n"
				"USN: %s::%.*s1\r\n"
				"\r\n",
				host, (unsigned int)port,
				st_len, st,
				tmp_uuid_value, st_len, st
				);
		}else{
			l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
				"Cache-Control: max-age=120\r\n"
				"EXT:\r\n"
				"Location: http://%s:%u" ROOTDESC_PATH "\r\n"
				"Server: " MINIUPNPD_SERVER_STRING "\r\n"
				"ST: %.*s\r\n"
				"USN: %s::%.*s\r\n"
				// we need this ??
				//"Content-Length: 0\r\n"
				"\r\n",
				host, (unsigned int)port,
				st_len, st,
				tmp_uuid_value, st_len, st
				);
		}
	}else{
		l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
			"Cache-Control: max-age=120\r\n"
			"EXT:\r\n"
			"Location: http://%s:%u" ROOTDESC_PATH "\r\n"
			"Server: " MINIUPNPD_SERVER_STRING "\r\n"
			"ST: %.*s\r\n"
			"USN: %s\r\n"
			"\r\n",
			host, (unsigned int)port,
			st_len, st,
			tmp_uuid_value
			);
	}

	n = sendto(s, buf, l, 0,
	           (struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );
	if(n < 0)
	{
		syslog(LOG_ERR, "sendto(udp): %m");
	}
}

/* jimmy modified for WFADevice support */
/*
static const char * const known_service_types[] =
{
	"upnp:rootdevice",
	"urn:schemas-upnp-org:device:InternetGatewayDevice:",
	"urn:schemas-upnp-org:device:WANConnectionDevice:",
	"urn:schemas-upnp-org:device:WANDevice:",
	"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:",
	"urn:schemas-upnp-org:service:WANIPConnection:",
	"urn:schemas-upnp-org:service:WANPPPConnection:",
	"urn:schemas-upnp-org:service:Layer3Forwarding:",
	0
};
*/

/*
char *known_service_types[11] =
{
	"upnp:rootdevice",
	"urn:schemas-upnp-org:device:InternetGatewayDevice:",
	"urn:schemas-upnp-org:device:WANConnectionDevice:",
	"urn:schemas-upnp-org:device:WANDevice:",
	"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:",
	"urn:schemas-upnp-org:service:WANIPConnection:",
	"urn:schemas-upnp-org:service:WANPPPConnection:",
// use ifdef ENABLE_L3F_SERVICE to seperate ??
	"urn:schemas-upnp-org:service:Layer3Forwarding:",
	0,//"urn:schemas-wifialliance-org:device:WFADevice:",//Chun
	0,//"urn:schemas-wifialliance-org:service:WFAWLANConfig:",//Chun
	0
};
*/
char *known_service_types[10] =
{
	"upnp:rootdevice",
	"urn:schemas-upnp-org:device:InternetGatewayDevice:",
	"urn:schemas-upnp-org:device:WANConnectionDevice:",
	"urn:schemas-upnp-org:device:WANDevice:",
	"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:",
	"urn:schemas-upnp-org:service:WANIPConnection:",
#if defined(ENABLE_L3F_SERVICE)
	"urn:schemas-upnp-org:service:Layer3Forwarding:",
#endif
	0,		//"urn:schemas-wifialliance-org:device:WFADevice:",//Chun
	0,		//"urn:schemas-wifialliance-org:service:WFAWLANConfig:",//Chun
	0
};

static void
SendSSDPNotifies(int s, const char * host, unsigned short port,
                 unsigned int lifetime)
{
	struct sockaddr_in sockname;
	int l, n, i=0;
	char bufr[512];

	char *uuid_value = NULL;// Chun add for WPS-COMPATIBLE

	DEBUG_MSG("%s (%d), SendSSDPNotifies \n\n",__FUNCTION__,__LINE__);
	memset(&sockname, 0, sizeof(struct sockaddr_in));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(SSDP_PORT);
	sockname.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);

	while(known_service_types[i])
	{
		/* jimmy added */
		GetUUIDValue(known_service_types[i],&uuid_value);

		l = snprintf(bufr, sizeof(bufr), 
				"NOTIFY * HTTP/1.1\r\n"
				"HOST:%s:%d\r\n"
				"Cache-Control:max-age=%u\r\n"
				"Location:http://%s:%d" ROOTDESC_PATH"\r\n"
				/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
				/*	Date:	2009-06-03
				*	Name:	jimmy huang
				*	Reason:	remove useless space character
				*/
				//"Server: " MINIUPNPD_SERVER_STRING "\r\n"
				"Server: " MINIUPNPD_SERVER_STRING "\r\n"
				// ----------
				"NT:%s%s\r\n"
				"USN:%s::%s%s\r\n"
				"NTS:ssdp:alive\r\n"
				"\r\n",
				SSDP_MCAST_ADDR, SSDP_PORT,
				lifetime,
				host, port,
				known_service_types[i], (i==0?"":"1"),
				
				//uuidvalue, known_service_types[i], (i==0?"":"1") );
				uuid_value, known_service_types[i], (i==0?"":"1") );
				//-------------------------
		if(l>=sizeof(bufr))
		{
			syslog(LOG_WARNING, "SendSSDPNotifies(): truncated output");
			l = sizeof(bufr);
		}
		n = sendto(s, bufr, l, 0,
			(struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );
		if(n < 0)
		{
			syslog(LOG_ERR, "sendto(udp_notify=%d, %s): %m", s, host);
		}
		i++;
	}
#ifdef UPNP_FULLLY_FEATURE
	/*	Date:	2009-05-12
	*	Name:	jimmy huang
	*	Reason:	For Intel Device Validator
	*			DIR-855 will not only send notify for
	*			NT: service
	*			but also send notify for
	*			NT: uuid
	*/
	l = snprintf(bufr, sizeof(bufr), 
				"NOTIFY * HTTP/1.1\r\n"
				"HOST:%s:%d\r\n"
				"Cache-Control:max-age=%u\r\n"
				"Location:http://%s:%d" ROOTDESC_PATH"\r\n"
				/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
				"Server: " MINIUPNPD_SERVER_STRING "\r\n"
				"NT: %s\r\n"
				"USN: %s\r\n"
				"NTS:ssdp:alive\r\n"
				"\r\n",
				SSDP_MCAST_ADDR, SSDP_PORT,
				lifetime,
				host, port,
				uuidvalue_1,uuidvalue_1);
	n = sendto(s, bufr, l, 0,
			(struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );
	l = snprintf(bufr, sizeof(bufr), 
				"NOTIFY * HTTP/1.1\r\n"
				"HOST:%s:%d\r\n"
				"Cache-Control:max-age=%u\r\n"
				"Location:http://%s:%d" ROOTDESC_PATH"\r\n"
				/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
				"Server: " MINIUPNPD_SERVER_STRING "\r\n"
				"NT: %s\r\n"
				"USN: %s\r\n"
				"NTS:ssdp:alive\r\n"
				"\r\n",
				SSDP_MCAST_ADDR, SSDP_PORT,
				lifetime,
				host, port,
				uuidvalue_2,uuidvalue_2);
	n = sendto(s, bufr, l, 0,
			(struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );
	l = snprintf(bufr, sizeof(bufr), 
				"NOTIFY * HTTP/1.1\r\n"
				"HOST:%s:%d\r\n"
				"Cache-Control:max-age=%u\r\n"
				"Location:http://%s:%d" ROOTDESC_PATH"\r\n"
				/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
				"Server: " MINIUPNPD_SERVER_STRING "\r\n"
				"NT: %s\r\n"
				"USN: %s\r\n"
				"NTS:ssdp:alive\r\n"
				"\r\n",
				SSDP_MCAST_ADDR, SSDP_PORT,
				lifetime,
				host, port,
				uuidvalue_3,uuidvalue_3);
	n = sendto(s, bufr, l, 0,
			(struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );

	/*
	 * If wsccmd daemon is not ready, it must not advertised.
	 */
	if (wsccmd_ready_flag == 1) {
		l = snprintf(bufr, sizeof(bufr),
					"NOTIFY * HTTP/1.1\r\n"
					"HOST:%s:%d\r\n"
					"Cache-Control:max-age=%u\r\n"
					"Location:http://%s:%d" ROOTDESC_PATH"\r\n"
					/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
					"Server: " MINIUPNPD_SERVER_STRING "\r\n"
					"NT: %s\r\n"
					"USN: %s\r\n"
					"NTS:ssdp:alive\r\n"
					"\r\n",
					SSDP_MCAST_ADDR, SSDP_PORT,
					lifetime,
					host, port,
					uuidvalue_4,uuidvalue_4);
		n = sendto(s, bufr, l, 0,
				(struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );
	}
#endif
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
	char bufr[1500];
	socklen_t len_r;
	struct sockaddr_in sendername;
	int i, l;
	int lan_addr_index = 0;
	char * st = 0;
	char * mx = 0;
	int mx_len = 0;
	int invalid = 0;
	int has_man = 0;
	int child_pid = 0;

	int st_len = 0;
	len_r = sizeof(struct sockaddr_in);

	n = recvfrom(s, bufr, sizeof(bufr), 0,
	             (struct sockaddr *)&sendername, &len_r);
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
	else if(memcmp(bufr, "M-SEARCH", 8) == 0)
	{
		DEBUG_MSG("%s (%d): SSDP M-SEARCH from %s:%d\n"
				,__FUNCTION__,__LINE__
				,inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
		
		i = 0;
		while(i < n)
		{
			/*	Date:	2009-06-03
			*	Name:	jimmy huang
			*	Reason:	In case buffer overflow
			*	Note:	Patched by miniupnpd-20090516
			*/
			//while(bufr[i] != '\r' || bufr[i+1] != '\n')
			while((i < n - 1) && (bufr[i] != '\r' || bufr[i+1] != '\n'))
				i++;
			i += 2;
			/*	Date:	2009-06-03
			*	Name:	jimmy huang
			*	Reason:	In case buffer overflow
			*	Note:	Patched by miniupnpd-20090516
			*/
			//if(strncasecmp(bufr+i, "st:", 3) == 0)
			if((i < n - 3) && strncasecmp(bufr+i, "st:", 3) == 0)
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
		}
		/*syslog(LOG_INFO, "SSDP M-SEARCH packet received from %s:%d",
	           inet_ntoa(sendername.sin_addr),
	           ntohs(sendername.sin_port) );*/
		/*	Date:	2009-05-07
		*	Name:	jimmy huang
		*	Reason:	UPnP-arch-DeviceArchitecture-v1.1-20081015.pdf, page 40
		*			for multicast M-SEARCH request, if the search does not contain
		*			and MX header field, the device MUST sitently discard and ignore the request
		*/
#ifdef UPNP_FULLLY_FEATURE
		i = 0;
		while(i < n)
		{
			if(strncasecmp(bufr+i, "M-SEARCH", 8) == 0)
			{
			// check M-SEARCH format, valid format:
			// M-SEARCH * HTTP/1.1\r\n
				//if((bufr[i-2] != '*') || (bufr[i+4] != '/') || (bufr[i+5] != '1') || (bufr[i+6] != '.') || (bufr[i+7] != '1')){
				if((bufr[i+9] != '*') || (bufr[i+15] != '/') || (bufr[i+16] != '1') || (bufr[i+17] != '.') || (bufr[i+18] != '1')){
					invalid = 1;
					break;
				}
			}
			/*	Date:	2009-06-03
			*	Name:	jimmy huang
			*	Reason:	In case buffer overflow
			*	Note:	Patched by miniupnpd-20090516
			*/
			//while(bufr[i] != '\r' || bufr[i+1] != '\n')
			while((i < n - 1) && (bufr[i] != '\r' || bufr[i+1] != '\n'))
				i++;
			i += 2;
			/*	Date:	2009-06-03
			*	Name:	jimmy huang
			*	Reason:	In case buffer overflow
			*	Note:	Patched by miniupnpd-20090516
			*			Added if(i < n - 4){
			*/
			if(i < n - 4){
			if(strncasecmp(bufr+i, "mx:", 3) == 0)
			{
				mx = bufr+i+3;
				mx_len = 0;
				while((*mx == ' ' || *mx == '\t') && (mx < bufr + n))
					mx++;
				while(mx[mx_len]!='\r' && mx[mx_len]!='\n' && (mx + mx_len < bufr + n))
					mx_len++;
			}
			else if(strncasecmp(bufr+i, "MAN:", 4) == 0)
			{
			//check MAN existence
				has_man = 1;
			}
			}
		}

		if(invalid || !has_man){
			syslog(LOG_INFO, "Invalid SSDP M-SEARCH from %s:%d ",
	        	   inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
			return ;
		}

		if(!mx){
			syslog(LOG_INFO, "Invalid SSDP M-SEARCH from %s:%d , no mx value been specified",
	        	   inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
			return ;
		}

		if(atoi(mx) <= 0){
			syslog(LOG_INFO, "Invalid SSDP M-SEARCH from %s:%d , mx value not valid",
	        	   inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
			return ;
		}
#endif
		/*	Date:	2009-06-03
		*	Name:	jimmy huang
		*	Reason:	In case buffer overflow
		*	Note:	Patched by miniupnpd-20090516
		*/
		//if(st)
		if(st && (st_len > 0))
		{
			/* TODO : doesnt answer at once but wait for a random time */
			syslog(LOG_INFO, "SSDP M-SEARCH from %s:%d ST: %.*s",
	        	   inet_ntoa(sendername.sin_addr),
	           	   ntohs(sendername.sin_port),
				   st_len, st);
			/* find in which sub network the client is */
			for(i = 0; i<n_lan_addr; i++)
			{
				if( (sendername.sin_addr.s_addr & lan_addr[i].mask.s_addr)
				   == (lan_addr[i].addr.s_addr & lan_addr[i].mask.s_addr))
				{
					lan_addr_index = i;
					break;
				}
                                //jacky add for disable not sub network the client start
                                else {
                                        return;
                                     }
                                //jacky add for disable not sub network the client end
			}
			/*	Date:	2009-05-07
			*	Name:	jimmy huang
			*	Reason:	UPnP-arch-DevsignaliceArchitecture-v1.1-20081015.pdf, page 40
			*			1.
			*			device should wait a random period of time between 0 seconds
			*			and the number of seconds specified in the MX field
			*			2.
			*			for multicast M-SEARCH request, 
			*			if specifies MX value greater than 5, device should assume that it contained
			*			the value 5 or less
			*			3. so we use fork()
			*	Note:	1.
			*			usleep => msec = 10^-6
			*			ex: usleep(3600) = 3.6 sec
			*			2.
			*			Due the the miniupnpd arch (select)
			*			If we enable this feature, we may hang miniupnpd for a while ?
			*			then cause some unexpect issue ???
			*			3.
			*			in our system (DIR-615, SR-300, Atheros based) fork() twice 
			*			would still cause zombie process, so we use signal(SIGCHLD,SIG_IGN)
			*			plus double fork()
			*/
#ifdef UPNP_FULLLY_FEATURE
			signal(SIGCHLD,SIG_IGN);
#if defined(__uClinux__) && !defined(IP8000)
			child_pid = vfork();
#else
			child_pid = fork();
#endif
			if(child_pid == 0){
					//child
#if defined(__uClinux__) && !defined(IP8000)
				if(vfork()){
#else
				if(fork()){
#endif
					//child exists
					DEBUG_MSG("%s (%d): child (%d) exit\n",__FUNCTION__,__LINE__,getpid());
#if defined(__uClinux__) && !defined(IP8000)
					_exit(0);
#else
					exit(0);
#endif
				}else{
					//grandchild
					DEBUG_MSG("%s (%d): child (%d) handling M-SEARCH\n",__FUNCTION__,__LINE__,getpid());
					srand(time(NULL));

					// (rand()%(maxmum-minimum+1))+minimum
					if((atoi(mx) > MX_MAXVALUE)  || (atoi(mx) < 0)){
						DEBUG_MSG("%s (%d): mx value = %s\n",__FUNCTION__,__LINE__,mx);
						sleep((rand()%(MX_MAXVALUE-0+1))+0);
					}else{
						DEBUG_MSG("%s (%d): mx value = %s\n",__FUNCTION__,__LINE__,mx);
						/*	Date:	2009-06-03
						*	Name:	jimmy huang
						*	Reason:	do not really sleep upto the max value control specified
						*/
						//sleep((rand()%(atoi(mx)-0+1))+0);
						sleep((rand()%( (atoi(mx)-1)  -  0  +  1))+0);
					}

					for(i = 0; known_service_types[i]; i++)
					{
						l = (int)strlen(known_service_types[i]);
						if(l<=st_len && (0 == memcmp(st, known_service_types[i], l)))
						{
							/*	Date:	2009-05-12
							*	Name:	jimmy huang
							*	Reason:	For Intel Device Validator
							*			Responds to request with a device type as ST header
							*			existence device type, and correct version
							*			ex :
							*			good:	WANCommonInterfaceConfig:1
							*			bad:	WANCommonInterfaceConfig:6
							*/
                            if((st[st_len-2] == ':') && (st[st_len-1] != '1')){
								break;
							}
							DEBUG_MSG("%s (%d): send back response for M-Search [ %s ]\n",__FUNCTION__,__LINE__,known_service_types[i]);
							SendSSDPAnnounce2(s, sendername,
											st, st_len, "",
											lan_addr[lan_addr_index].str, port);
						}
					}

					/* Responds to request with ST: ssdp:all */
					/* strlen("ssdp:all") == 8 */
					if(st_len==8 && (0 == memcmp(st, "ssdp:all", 8)))
					{
						for(i=0; known_service_types[i]; i++)
						{
							l = (int)strlen(known_service_types[i]);
							DEBUG_MSG("%s (%d): send back response for M-Search [ %s ]\n",__FUNCTION__,__LINE__,known_service_types[i]);
							SendSSDPAnnounce2(s, sendername,
											known_service_types[i], l, i==0?"":"1",
											lan_addr[lan_addr_index].str, port);
						}
						/*	Date:	2009-05-12
						*	Name:	jimmy huang
						*	Reason:	For Intel Device Validator
						*			we should not only send service's 200 OK
						*			but also send uuid's 200 OK
						*/
						SendSSDPAnnounce2(s, sendername, uuidvalue_1 , strlen(uuidvalue_1), "", lan_addr[lan_addr_index].str, port);
						SendSSDPAnnounce2(s, sendername, uuidvalue_2 , strlen(uuidvalue_1), "", lan_addr[lan_addr_index].str, port);
						SendSSDPAnnounce2(s, sendername, uuidvalue_3 , strlen(uuidvalue_1), "", lan_addr[lan_addr_index].str, port);
						/*
						 * If wsccmd daemon is not ready, it must not responded.
						 */
						if(wsccmd_ready_flag == 1) {
							SendSSDPAnnounce2(s, sendername, uuidvalue_4 , strlen(uuidvalue_1), "", lan_addr[lan_addr_index].str, port);
						}
					}
					/* Chun add : process the icon request to surf web page*/
					l = (int)strlen(uuidvalue_1);
					/*	Date:	2009-05-12
					*	Name:	jimmy huang
					*	Reason:	For Intel Validator
					*			when M-SEARCH with ST specified with UUID not string,
					*			We should response that request
					*	Note:	
					*/
					/*
					if(l==st_len && (0 == memcmp(st, uuidvalue_1, l)))
					{	
						syslog(LOG_INFO, "333 SSDP M-SEARCH ");
						SendSSDPAnnounce2(s, sendername, st, st_len, "", lan_addr[lan_addr_index].str, port);
					}
					*/
					if(l==st_len){
						if( (0 == memcmp(st, uuidvalue_1, l)) ||
							(0 == memcmp(st, uuidvalue_2, l)) ||
							(0 == memcmp(st, uuidvalue_3, l)) ||
							(0 == memcmp(st, uuidvalue_4, l))
						){
						DEBUG_MSG("%s (%d): send back response for M-Search [ %s ]\n",__FUNCTION__,__LINE__,st);
							SendSSDPAnnounce2(s, sendername, st, st_len, "", lan_addr[lan_addr_index].str, port);
						}
					}
					//grandchild exits
					DEBUG_MSG("%s (%d): grandchild (%d) exit\n",__FUNCTION__,__LINE__,getpid());
#if defined(__uClinux__) && !defined(IP8000)
					_exit(0);
#else
					exit(0);
#endif
				}
			}
			// father
			waitpid(child_pid,NULL,WUNTRACED);
			DEBUG_MSG("%s (%d): father (%d) keep handling following stuff \n",__FUNCTION__,__LINE__,getpid());
#else
			/* Responds to request with a device as ST header */
			for(i = 0; known_service_types[i]; i++)
			{
				l = (int)strlen(known_service_types[i]);
				if(l<=st_len && (0 == memcmp(st, known_service_types[i], l)))
				{
					SendSSDPAnnounce2(s, sendername,
					                  st, st_len, "",
					                  lan_addr[lan_addr_index].str, port);
					break;
				}
			}
			/* Responds to request with ST: ssdp:all */
			/* strlen("ssdp:all") == 8 */
			if(st_len==8 && (0 == memcmp(st, "ssdp:all", 8)))
			{
				for(i=0; known_service_types[i]; i++)
				{
					l = (int)strlen(known_service_types[i]);
					SendSSDPAnnounce2(s, sendername,
					                  known_service_types[i], l, i==0?"":"1",
					                  lan_addr[lan_addr_index].str, port);
				}
			}
			/* responds to request by UUID value */
			/*	Date:	2009-04-20
			*	Name:	jimmy huang
			*	Reason:	Using unigue uuid-value
			*/
			/*
			l = (int)strlen(uuidvalue);
			if(l==st_len && (0 == memcmp(st, uuidvalue, l)))
			{
				SendSSDPAnnounce2(s, sendername, st, st_len, "",
				                  lan_addr[lan_addr_index].str, port);
			}
			*/
			/* Chun add : process the icon request to surf web page*/
			l = (int)strlen(uuidvalue_1);
            if(l==st_len && (0 == memcmp(st, uuidvalue_1, l)))
			{	
				syslog(LOG_INFO, "333 SSDP M-SEARCH ");
				SendSSDPAnnounce2(s, sendername, st, st_len, "", lan_addr[lan_addr_index].str, port);
			}
			//-----------
#endif
		}
		else
		{
			syslog(LOG_INFO, "Invalid SSDP M-SEARCH from %s:%d",
	        	   inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
		}
	}
	else
	{
		syslog(LOG_NOTICE, "Unknown udp packet received from %s:%d",
		       inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
	}
}



/*	Date:	2010-09-17
*	Name:	jimmy huang
*	Reason:	To fixed the bug, When rc ask hostpad restart, hostapd will lose all subscription records
*			Thus we need to tell all control points that WFA services are dead
*			Then when we detect hostpad is alive again, we will send NOTIFY to tell all CPs
*			that WFA services are ready, CPs will subscribe information with hostapd again
*/
extern int snotify[MAX_LAN_ADDR];

void stop_wfa_service(int sig){
	SendSSDPGoodbye_WFAService(snotify, n_lan_addr);
	known_service_types[8]=0;
	known_service_types[9]=0;
	wsccmd_ready_flag = 0;
	return;
}

int SendSSDPGoodbye_WFAService(int * sockets, int n_sockets)
{
	struct sockaddr_in sockname;
	int n, l;
	int i, j;
	char bufr[512];
	char *tmp_uuid_value=NULL; // Chun modify for WPS-COMPATIBLE

    memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(SSDP_PORT);
    sockname.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);

	for(j=0; j<n_sockets; j++){
		for(i=0; known_service_types[i]; i++){
	    	if(strstr(known_service_types[i],"WFADevice") || strstr(known_service_types[i],"WFAWLANConfig")){
				/* Chun modify for WPS-COMPATIBLE */
				GetUUIDValue(known_service_types[i],&tmp_uuid_value); 
				l = snprintf(bufr, sizeof(bufr),
						"NOTIFY * HTTP/1.1\r\n"
						"HOST:%s:%d\r\n"
						"NT:%s%s\r\n"
						"USN:%s::%s%s\r\n"
						"NTS:ssdp:byebye\r\n"
						"\r\n",
						SSDP_MCAST_ADDR, SSDP_PORT,
						known_service_types[i], (i==0?"":"1"),
						//uuidvalue, known_service_types[i], (i==0?"":"1"));
						tmp_uuid_value, known_service_types[i], (i==0?"":"1"));
						//--------
				n = sendto(sockets[j], bufr, l, 0,
						(struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );
				if(n < 0){
					syslog(LOG_ERR, "sendto(udp_shutdown=%d): %m", sockets[j]);
					return -1;
				}
			}
    	}
	}
	return 0;
}

/* This will broadcast ssdp:byebye notifications to inform 
 * the network that UPnP is going down. */
int
SendSSDPGoodbye(int * sockets, int n_sockets)
{
	struct sockaddr_in sockname;
	int n, l;
	int i, j;
	char bufr[512];


	char *tmp_uuid_value=NULL; // Chun modify for WPS-COMPATIBLE


    memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(SSDP_PORT);
    sockname.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);

	for(j=0; j<n_sockets; j++)
	{
	    for(i=0; known_service_types[i]; i++)
	    {
			/* Chun modify for WPS-COMPATIBLE */
			GetUUIDValue(known_service_types[i],&tmp_uuid_value); 

	        l = snprintf(bufr, sizeof(bufr),
	                 "NOTIFY * HTTP/1.1\r\n"
	                 "HOST:%s:%d\r\n"
	                 "NT:%s%s\r\n"
	                 "USN:%s::%s%s\r\n"
	                 "NTS:ssdp:byebye\r\n"
	                 "\r\n",
	                 SSDP_MCAST_ADDR, SSDP_PORT,
					 known_service_types[i], (i==0?"":"1"),

	                 //uuidvalue, known_service_types[i], (i==0?"":"1"));
					 tmp_uuid_value, known_service_types[i], (i==0?"":"1"));
					 //--------
	        n = sendto(sockets[j], bufr, l, 0,
	                   (struct sockaddr *)&sockname, sizeof(struct sockaddr_in) );
			if(n < 0)
			{
				syslog(LOG_ERR, "sendto(udp_shutdown=%d): %m", sockets[j]);
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

	char *tmp_uuid_value=NULL;// Chun add for WPS-COMPATIBLE

	
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s < 0) {
		syslog(LOG_ERR, "socket(unix): %m");
		return -1;
	}
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, minissdpdsocketpath, sizeof(addr.sun_path));
	if(connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
		syslog(LOG_ERR, "connect(\"%s\"): %m", minissdpdsocketpath);
		return -1;
	}
	for(i = 0; known_service_types[i]; i++) {
		buffer[0] = 4;
		p = buffer + 1;
		l = (int)strlen(known_service_types[i]);
		if(i > 0)
			l++;
		CODELENGTH(l, p);
		memcpy(p, known_service_types[i], l);
		if(i > 0)
			p[l-1] = '1';
		p += l;

// 		l = snprintf(strbuf, sizeof(strbuf), "%s::%s%s", 
// 		             uuidvalue, known_service_types[i], (i==0)?"":"1");
		GetUUIDValue(known_service_types[i],&tmp_uuid_value);
		l = snprintf(strbuf, sizeof(strbuf), "%s::%s%s", 
		             tmp_uuid_value, known_service_types[i], (i==0)?"":"1");
		// -----------
		CODELENGTH(l, p);
		memcpy(p, strbuf, l);
		p += l;
		l = (int)strlen(MINIUPNPD_SERVER_STRING);
		CODELENGTH(l, p);
		memcpy(p, MINIUPNPD_SERVER_STRING, l);
		p += l;
		l = snprintf(strbuf, sizeof(strbuf), "http://%s:%u" ROOTDESC_PATH,
		             host, (unsigned int)port);
		CODELENGTH(l, p);
		memcpy(p, strbuf, l);
		p += l;
		if(write(s, buffer, p - buffer) < 0) {
			syslog(LOG_ERR, "write(): %m");
			return -1;
		}
	}
 	close(s);
	return 0;
}
