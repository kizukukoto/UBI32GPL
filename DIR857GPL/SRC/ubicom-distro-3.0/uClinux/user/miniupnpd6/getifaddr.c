/* $Id: getifaddr.c,v 1.9 2008/10/15 10:16:28 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2008 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#if defined(sun)
#include <sys/sockio.h>
#endif

#include "getifaddr.h"
/*
int
getifaddr(const char * ifname, char * buf, int len)
{
	// SIOCGIFADDR struct ifreq
	int s;
	struct ifreq ifr;
	int ifrlen;
	struct sockaddr_in6 * addr; // IPv6 modification, IPv6 only
	ifrlen = sizeof(ifr);
	if(!ifname || ifname[0]=='\0')
		return -1;
	s = socket(PF_INET6, SOCK_DGRAM, 0); // IPv6 modification, IPv6 only
	if(s < 0)
	{
		syslog(LOG_ERR, "socket(PF_INET6, SOCK_DGRAM): %m"); // IPv6 modification, IPv6 only
		return -1;
	}
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if(ioctl(s, SIOCGIFADDR, &ifr, &ifrlen) < 0)
	{
		syslog(LOG_ERR, "ioctl(s, SIOCGIFADDR, ...): %m");
		close(s);
		return -1;
	}
	addr = (struct sockaddr_in6 *)&ifr.ifr_addr; // IPv6 modification, IPv6 only
	if(!inet_ntop(AF_INET6, &addr->sin6_addr, buf, len)) // IPv6 modification, IPv6 only
	{
		syslog(LOG_ERR, "inet_ntop(): %m");
		close(s);
		return -1;
	}
	close(s);
	return 0;
} */

int
getifaddr(const char * ifname, char * buf, int len)
{
	FILE *f;
	char devname[IFNAMSIZ];
	char addr6p[8][5]={0};
	int has_ip=0, plen, scope, dad_status, if_idx;

	if(!ifname || ifname[0]=='\0')
		return -1;
	f=fopen("/proc/net/if_inet6", "r"); 
	//fprintf(stderr, "Openning file\n");
	if(f == NULL)
	{
		printf("fopen(\"/proc/net/if_inet6\", \"r\")"); /* IPv6 modification, IPv6 only */
		fprintf(stderr, "Cannot open /proc/net/if_inet6 file for interface %s.\n", ifname);
		return -1;
	}
	else
	{
	    while(has_ip==0 && fscanf(f, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %20s\n", addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4], addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope, &dad_status, devname) !=EOF)
	    {
	        //printf("\naddr: %s\n", addr6p);
	        //printf("scope: %d\n", scope);
	        //printf("Current ifname: %s\n", devname);
	        if(!strcmp(devname, ifname))
	        {
	            switch(scope)
	            {
	                case 0:
			//fprintf(stderr, "Entering case0...\n");
	                has_ip=1;
	                sprintf(buf, "%s:%s:%s:%s:%s:%s:%s:%s", addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
	                break;
	                case 32:
			//fprintf(stderr, "Entering case1...\n");
	                has_ip=1;
	                sprintf(buf, "%s:%s:%s:%s:%s:%s:%s:%s", addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
	                break;
	                default:
			//fprintf(stderr, "Entering default...\n");
	                has_ip=0;
	                sprintf(buf, "::");
	                break;
	            }
	        }
	        //fprintf(stderr, "IPv6 addr: %s, has_ip=%d.\n", buf, has_ip);
	    }
	    if(has_ip==0)
	    {
	        fprintf(stderr, "No IPv6 address found for interface %s.\n", ifname);
	        return -1;
	    }
        fclose(f);
	}
	return 0;
}
