/* $Id: minissdp.c,v 1.18 2009/11/06 20:18:17 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2009 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <syslog.h>
#include <net/if.h>
#include "config.h"
#include "upnpdescstrings.h"
#include "miniupnpdpath.h"
#include "upnphttp.h"
#include "upnpglobalvars.h"
#include "minissdp.h"
#include "codelength.h"

#define MAXHOSTNAMELEN 64
/* GENA ip/port */
#define GENA_PORT (7900) // GENA Modificationn
#define GENA_MCAST_ADDR ("FF05::130") // IPv6 Modification // GENA Modification
#define ADDR_LEN INET6_ADDRSTRLEN

static volatile int quitting = 0;

static void
sigterm(int sig) {
	/*int save_errno = errno;*/
	signal(sig, SIG_IGN);	/* Ignore this signal while we are quitting */

	syslog(LOG_NOTICE, "received signal %d, Unsubscibe", sig);

	quitting = 1;
	/*errno = save_errno;*/
}

struct subscriber {
	time_t timeout;
	uint32_t seq;
	int num_event;
	char uuid[42];
	/* "uuid:00000000-0000-0000-0000-000000000000"; 5+36+1=42bytes */
	char callback[];
};

/* Adding 0's when needed to have an 32 xdigit IPv6 address */
void
paddingAddr(char *str, int *index, int lim)
{
    int i=0, id=*index;
    for(i=1; i<lim; i++)
    {
        str[id]='0';
        id++;
    }
    *index=id;

}

/* Specific case of adding 0's when faced with an '::' in an IPv6 address */
void
longPaddingAddr(char *str, int *index, int max, int mis)
{
    int u=0, v=0;
    u=39-max-mis;
    u=u/4;
    v=u*4;
    paddingAddr(str, index, v+1);
}

/* Formating the IPv6 address accordingly to the address stored in the /proc/net/if_inet6 file */
void
del_char(char *str)
{
    int id_read=0, id_write=0;
    char tmp[ADDR_LEN]="";

    while (str[id_read]!='\0')
    {
        if(isxdigit(str[id_read]))
        {
            tmp[id_write]=str[id_read];
            id_write++;
        }
        else
        {
            if(!isxdigit(str[id_read+1]))
            {
                char *p;
                int max=0;
                p=str;
                while(*p!='\0')
                {
                    max++;
                    p++;
                }
                if(max<=34 && max>=30)
                {
                    longPaddingAddr(tmp, &id_write, max, 1);
                }
                else if(max<=29 && max>=25)
                {
                    longPaddingAddr(tmp, &id_write, max, 2);
                }
                else if(max<=24 && max>=20)
                {
                    longPaddingAddr(tmp, &id_write, max, 3);
                }
                else if(max<=19 && max>=15)
                {
                    longPaddingAddr(tmp, &id_write, max, 4);
                }
                else if(max<=14 && max>=10)
                {
                    longPaddingAddr(tmp, &id_write, max, 5);
                }
                else if(max<=9)
                {
                    longPaddingAddr(tmp, &id_write, max, 6);
                }
            }
            else if(!isxdigit(str[id_read+2]))
            {
                paddingAddr(tmp, &id_write, 4);
            }
            else if(!isxdigit(str[id_read+3]))
            {
                paddingAddr(tmp, &id_write, 3);
            }
            else if(!isxdigit(str[id_read+4]))
            {
                paddingAddr(tmp, &id_write, 2);
            }
        }
        id_read++;
    }
    tmp[id_write]='\0';
    strcpy(str, tmp);
}

/* Retrieve the index of an interface from its address */
int
getifindex(const char * addr, int * ifindex)
{
    FILE *f;//int s;
	char devname[IFNAMSIZ];
	char addr6p[ADDR_LEN]="", buf[ADDR_LEN]="";
	int has_index=0, plen, scope, dad_status, if_idx;
	strcpy(buf, addr);
	del_char(buf);

	f=fopen("/proc/net/if_inet6", "r");

	if(f == NULL)
	{
		printf("fopen(\"/proc/net/if_inet6\", \"r\")");
		return -1;
	}
	else
	{
	    while(fscanf(f, "%32s %02x %02x %02x %02x %20s\n", addr6p, &if_idx, &plen, &scope, &dad_status, devname) !=EOF)
	    {
	            //printf("\naddr6p: %s\n", addr6p);
		    //printf("  addr: %s\n", buf);
	            if(strcmp(addr6p,buf)==0)
	            {
	                has_index=1;
	                *ifindex=if_idx;
#ifdef DEBUG
	                printf("Address index: %d\n", *ifindex);
#endif
	                break;
	            }
	    }
	    if(has_index==0)
        {
            printf("No match found for this address! No index retreived\n");
            return -1;
        }
        fclose(f);
	}
	return 0;
}

static int
AddMulticastMembership(int s, int ifindex)
{
	struct ipv6_mreq ipv6mr;	/* Ip multicast membership */

	/* setting up ipv6mr structure */
	inet_pton(AF_INET6, GENA_MCAST_ADDR, &(ipv6mr.ipv6mr_multiaddr));

	ipv6mr.ipv6mr_interface = ifindex;

	if (setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (void *)&ipv6mr, sizeof(struct ipv6_mreq)) < 0)
	{
        syslog(LOG_ERR, "setsockopt(gena, IPV6_ADD_MEMBERSHIP): %m");
		return -1;
	}

	return 0;
}

/* Open and configure the socket listening for
 * GENA v4 udp packets sent on 239.255.255.246 port 7900 
 * GENA v6 udp packets sent on FF05::130 port 7900*/
int
OpenAndConfReceiveEventSocket(struct in6_addr addr)
{
	int s;
	int i;
	int j = 1;
	int if_index=0;
	struct sockaddr_in6 sockname;
	char str[MAXHOSTNAMELEN]="";

	if( (s = socket(PF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		syslog(LOG_ERR, "socket(gena): %m");
		return -1;
	}

	memset(&sockname, 0, sizeof(struct sockaddr_in6));
	sockname.sin6_family = AF_INET6; 
	sockname.sin6_port = htons(GENA_PORT); 
	sockname.sin6_addr = in6addr_any;

	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &j, sizeof(j)) < 0)
	{
		syslog(LOG_WARNING, "setsockopt(gena, SO_REUSEADDR): %m");
	}


	if(bind(s, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in6)) < 0) // IPv6 Modification
	{
		syslog(LOG_ERR, "bind(gena): %m");
		close(s);
		return -1;
	}

	inet_ntop(AF_INET6, &(addr), str, INET6_ADDRSTRLEN);
#ifdef DEBUG
	fprintf(stderr, "gena: inet, str: %s\n", str);
#endif
	getifindex(str, &if_index);
	if(AddMulticastMembership(s, if_index) < 0)
	{
		syslog(LOG_WARNING,
		       "Failed to add GENA multicast membership for address %s",
		       str );
	}

	return s;
}

/* GENA Init Response */
static void
SendGENAInitResponse(int s, struct sockaddr_in6 sockname) // IPv6 Modification
{
	int l, n;
	char buf[512];
	/*
	 * follow guideline from document "UPnP Device Architecture 1.0"
	 * uppercase is recommended.
	 * DATE: is recommended
	 * SERVER: OS/ver UPnP/1.0 miniupnpd/1.0
	 * - check what to put in the 'Cache-Control' header
	 * */
	l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
		"\r\n");
	n = sendto(s, buf, l, 0,
	           (struct sockaddr *)&sockname, sizeof(struct sockaddr_in6) ); // IPv6 Modification
	if(n < 0)
	{
		syslog(LOG_ERR, "sendto(udp): %m");
	}
}


static int
SendGENAMessages(const char * publisherHost, unsigned short publisherPort, const char * genaCmd,
                 const char * callback, const char * publisherPath, char * sid, unsigned int lifetime, unsigned int * timeout)
{
	int opt = 1, sub=0;
	static const char GENASubMsgFmt[] =
	"SUBSCRIBE %s HTTP/1.1\r\n"
	"HOST: [%s]:%u \r\n"
	"USER-AGENT: " OS_STRING " UPnP/1.1 MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"
	"CALLBACK: %s\r\n" 
	"NT: upnp:event\r\n"
	"TIMEOUT: Second-%u\r\n"
	"\r\n";

	static const char GENARenMsgFmt[] =
	"SUBSCRIBE %s HTTP/1.1\r\n"
	"HOST: [%s]:%u \r\n"
	"SID: \r\n"
	"TIMEOUT: Second-%u\r\n"
	"\r\n";

	static const char GENAUnsMsgFmt[] =
	"UNSUBSCRIBE %s HTTP/1.1\r\n"
	"HOST: [%s]:%u \r\n"
	"SID: \r\n"
	"\r\n";

	char bufr[1536];	/* reception and emission buffer */
	int sgena;
	int n;
	struct sockaddr_in6 sockudp_r, sockudp_w; /* IPv6 modification, IPv6 only */
	unsigned int mx;

	/* fallback to direct discovery */
#ifdef WIN32
	sgena = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP); /* IPv6 modification, IPv6 only */
#else
	sgena = socket(PF_INET6, SOCK_DGRAM, 0); /* IPv6 modification, IPv6 only */
#endif
	if(sgena < 0)
	{
		PRINT_SOCKET_ERROR("socket");
		return 1;
	}
	/* reception */
	memset(&sockudp_r, 0, sizeof(struct sockaddr_in6)); // IPv6 modification, IPv6 only 
	sockudp_r.sin6_family = AF_INET6; // IPv6 modification, IPv6 only
	sockudp_r.sin6_addr = in6addr_any; // IPv6 modification, IPv6 only 
	/* emission */
	memset(&sockudp_w, 0, sizeof(struct sockaddr_in6)); /* IPv6 modification, IPv6 only */
	sockudp_w.sin6_family = AF_INET6; /* IPv6 modification, IPv6 only */
	sockudp_w.sin6_port = htons(port); /* IPv6 modification, IPv6 only */
	inet_pton(AF_INET6, host, &(sockudp_w.sin6_addr)); /* IPv6 modification, IPv6 only */

#ifdef WIN32
	if (setsockopt(sgena, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof (opt)) < 0)
#else
	if (setsockopt(sgena, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt)) < 0)
#endif
	{
		PRINT_SOCKET_ERROR("setsockopt sgena");
		return 1;
	}

	/* Avant d'envoyer le paquet on bind pour recevoir la reponse */
	if (bind(sgena, (struct sockaddr *)&sockudp_r, sizeof(struct sockaddr_in6)) != 0) /* IPv6 modification, IPv6 only */
	{
		PRINT_SOCKET_ERROR("bind");
		closesocket(sudp);
		return 1;
	}

	/* receiving GENA response packet */
	for(n = 0;;)
	{
		if(n == 0)
		{
			/* sending the GENA packet */
			if(strcmp(genaCmd, "subscribe")==0)
			{
				n = snprintf(bufr, sizeof(bufr),
				             GENASubMsgFmt, publisherPath, publisherHost, publisherPort,
						callback, lifetime);
			}
			else if(strcmp(genaCmd, "renew")==0)
			{
				n = snprintf(bufr, sizeof(bufr),
				             GENASubMsgFmt, publisherPath, publisherHost, publisherPort,
						uuid, sid, lifetime);
			}
			else if(strcmp(genaCmd, "unsubscribe")==0)
			{
				n = snprintf(bufr, sizeof(bufr),
				             GENASubMsgFmt, publisherPath, publisherHost, publisherPort,
						uuid, sid);
			}
			else
			{
				syslog(LOG_INFOS, "Unsupported GENA Command: %s", genaCmd);
			}
			printf("Sending GENA %s message\n", genaCmd);
			n = sendto(sgena, bufr, n, 0,
			           (struct sockaddr *)&sockudp_w, sizeof(struct sockaddr_in6)); /* IPv6 modification, IPv6 only */
			if (n < 0)
			{
				PRINT_SOCKET_ERROR("sendto");
				closesocket(sgena);
				return 1;
			}
		}
		/* Waiting for GENA Reply packet to GENA Command */
		n = ReceiveData(sgena, bufr, sizeof(bufr), delay);
		if (n < 0)
		{
			/* error */
			closesocket(sgena);
			return 1;
		}
		else if (n == 0)
		{
			closesocket(sgena);
			return 1;
		}
		else
		{
			/* Parse la réponse pour récupérer le sid et le timeout */
		}
	}
}


/* ProcessGENAEvent()
 * process GENA Event messages and responds to the initial one */
void
ProcessGENAEvent(int s, unsigned short port, int * init)
{
	int n;
	char bufr[1500];
	socklen_t len_r;
	struct sockaddr_in6 sendername;
	char formatedAddr[INET6_ADDRSTRLEN]="";
	int i, j, l;
	char * seq = 0;
	int seq_len = 0;
	len_r = sizeof(struct sockaddr_in6);

	n = recvfrom(s, bufr, sizeof(bufr), 0,
	             (struct sockaddr *)&sendername, &len_r);
	if(n < 0)
	{
		syslog(LOG_ERR, "recvfrom(udp): %m");
		return;
	}

	if(memcmp(bufr, "NOTIFY", 6) == 0)
	{
		i = 0;
		while(i < n)
		{
			while((i < n - 1) && (bufr[i] != '\r' || bufr[i+1] != '\n'))
				i++;
			i += 2;
			if((i < n - 3) && (strncasecmp(bufr+i, "SEQ:", 4) == 0))
			{
				seq = bufr+i+4;
				seq_len = 0;
				while((*seq == ' ' || *seq == '\t') && (seq < bufr + n))
					seq++;
				while(seq[seq_len]!='\r' && seq[seq_len]!='\n'
				     && (seq + seq_len < bufr + n))
					seq_len++;
			}
		}
		inet_ntop(AF_INET6, &(sendername.sin6_addr), formatedAddr, INET6_ADDRSTRLEN);
		if(seq && (seq_len > *init))
		{
			syslog(LOG_INFO, "GENA NOTIFY from [%s]:%d SEQ: %.*s",
	        	   formatedAddr,
	           	   ntohs(sendername.sin6_port),
				   seq_len, seq);
			if(!init)
			{
				/* Responds to GENA INIT message with a 200 OK */
				SendGENAInitResponse(s, sendername);
			}
			/* Create the base for comparaison (INIT) or Compare the new value received with the base */
		}
		else
		{
			syslog(LOG_INFO, "Invalid GENA NOTIFY from [%s]:%d",
	        	   formatedAddr, ntohs(sendername.sin6_port)); // IPv6 Modification
		}
	}
	else
	{
		syslog(LOG_NOTICE, "Unknown udp packet received from [%s]:%d",
		       formatedAddr, ntohs(sendername.sin6_port)); // IPv6 Modification
	}
}

void
retrievePublisherInfos(const char * descURL, char * host, int * port)
{
	int i;
	const char * p;
	unsigned short port;
	if(!descURL)
		return;
	i = 0;
	p = descURL;
	p += 8;	/* http://[ */  
	while(*p != '/' && *p != ']')
		*host[i++] = *(p++);
	*host[i] = '\0';
	if(*p == ']')
	{
		p+=2;
		*port = (unsigned short)atoi(p);
	}
}


int
eventingProcess(const char * descURL, const char * addr)
{
	int init=0, numRenew=0;
	int sudp = -1;
	int lifetime = 600;
	char publisherHost[ADDR_LEN]="";
	unsigned int publisherPort[8];

	struct subscriber subscriber;
	struct in6_addr subscriberAddr;

	fd_set readset;	/* for select() */
	int max_fd = -1;

	strcat(subscriber.callback, "http://[");
	strcat(subscriber.callback, addr);
	strcat(subscriber.callback, "]/"); 
	inet_pton(AF_INET6, addr, &subscriberAddr);
	retrievePublisherInfos(descURL, &publisherHost, &publisherPort);

	/* open socket for GENA connections */
	sudp = OpenAndConfGENAReceiveSocket(subscriberAddr);
	if(sudp < 0)
	{
		syslog(LOG_INFO, "Failed to open socket for receiving GENA message.");
		return 1;
	}

	SendGENAMessages(publisherHost, publisherPort, "subscribe", subscriber.callback, descURL, subscriber.uuid, lifetime, &(subscriber.timeout));
	/* main loop */
	while(!quitting)
	{
		if(timeout==0 && numRenew ==10)
		{
			printf("Timeout reached, need to subscribe again");
			return 0;
		}
		else
		{
			numRenew++;
			SendGENAMessages(publisherHost, publisherPort, "renew", 0, descURL, subscriber.uuid, lifetime, &(subscriber.timeout));
		}

		/* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
		FD_ZERO(&readset);

		if (sudp >= 0)
		{
			FD_SET(sudp, &readset);
			max_fd = MAX( max_fd, sudp);
		}

		if(select(max_fd+1, &readset, 0, 0, &timeout) < 0)
		{
			if(quitting) goto shutdown;
			if(errno == EINTR) continue; /* interrupted by a signal, start again */
			syslog(LOG_ERR, "select(all): %m");
			syslog(LOG_ERR, "Failed to select open sockets. EXITING");
			return 1;	/* very serious cause of error */
		}

		/* process GENA packets */
		if(sudp >= 0 && FD_ISSET(sudp, &readset))
		{
			/*syslog(LOG_INFO, "Received UDP Packet");*/
			ProcessGENAEvent(sudp, (unsigned short)v.port, &init);
			init++;
		}
	}	/* end of main loop */

shutdown:
	if (sudp >= 0) close(sudp);
	SendGENAMessages(publisherHost, publisherPort, "unsubscribe", 0, publisherPath, subscriber.uuid, 0, 0);

	return 0;
}

