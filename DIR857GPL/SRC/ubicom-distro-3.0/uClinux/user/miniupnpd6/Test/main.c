/* $Id: getifaddr.c,v 1.9 2008/10/15 10:16:28 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2008 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h> //
#include <stdlib.h> //
#include <netdb.h> //
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h> //

#define NI_MAXHOST 100
#define NI_NUMERICHOST 0

int
getifaddr(const char * ifname, char * buf, int len)
{
	/* SIOCGIFADDR struct ifreq *  */
	int s;
	struct ifreq ifr;
	int ifrlen;
	struct sockaddr_in * addr; /* IPv6 modification, IPv6 only */
	ifrlen = sizeof(ifr);
	printf("test ifname\n");
	if(!ifname || ifname[0]=='\0')
		return -1;
    printf("creationsocket\n");
	s = socket(PF_INET, SOCK_DGRAM, 0); /* IPv6 modification, IPv6 only */
	if(s < 0)
	{
		syslog(LOG_ERR, "socket(PF_INET, SOCK_DGRAM): %m"); /* IPv6 modification, IPv6 only */
		return -1;
	}
	printf("test strncpy\n");
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	printf("test ioctl1:%s\n", ifr.ifr_name);
	if(ioctl(s, SIOCGIFADDR, &ifr, &ifrlen) < 0)
	{
	    printf("erreurioctl\n");
		//syslog(LOG_ERR, "ioctl(s, SIOCGIFADDR, ...): %m");
		close(s);
		return -1;
	}
	printf("test recupaddr\n");
	addr = (struct sockaddr_in *)&ifr.ifr_addr; /* IPv6 modification, IPv6 only */
	printf("test inet_ntop\n");
	if(!inet_ntop(AF_INET, &addr->sin_addr, buf, len)) /* IPv6 modification, IPv6 only */
	{
	    printf("erreur\n");
        //syslog(LOG_ERR, "inet_ntop(): %m");
		close(s);
		return -1;
	}
	printf("buf:%s",buf);
	close(s);
	return 0;
}

int PrintAddr()
{

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        family = ifa->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic form of the latter for the common families) */

        printf("%s  address family: %d%s\n",
                       ifa->ifa_name, family,
                       (family == AF_PACKET) ? " (AF_PACKET)" :
                       (family == AF_INET) ?   " (AF_INET)" :
                       (family == AF_INET6) ?  " (AF_INET6)" : "");

        /* For an AF_INET* interface address, display the address */

        if (family == AF_INET || family == AF_INET6)
        {
            s = getnameinfo(ifa->ifa_addr,
                           (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                 sizeof(struct sockaddr_in6),
                                                  host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return -1;
            }
            printf("\taddress: <%s>\n", host);
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}

int
lookup_host (const char *host)
{
  struct addrinfo hints;
  struct addrinfo *res;
  int errcode;
  char addrstr[100];
  void *ptr;

  memset (&hints, 0, sizeof (hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;

  errcode = getaddrinfo (host, NULL, &hints, &res);
  if (errcode != 0)
    {
      perror ("getaddrinfo");
      return -1;
    }

  printf ("Host: %s\n", host);
  while (res)
    {
      //inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);

      switch (res->ai_family)
        {
        case AF_INET:
          ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
          break;
        case AF_INET6:
          ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
          break;
        }
      inet_ntop (res->ai_family, ptr, addrstr, 100);
      printf ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4,
              addrstr, res->ai_canonname);
      res = res->ai_next;
    }

  return 0;
}


int main()
{
    printf("test declaration\n");
    char buf[1024]="";
    int len = 1024;
    printf("test getifaddr\n");
    /*if(getifaddr("eth0", &buf, len)<0)
    {
        printf("getifaddr n'a pas terminée\n");
        printf("l'adresse est : %s\n", &buf);
    }
    else
    {
        printf("l'adresse est : %s\n", &buf);
    }*/
    //struct ifaddrs **ifap;
    //getifaddrs(ifap);
    if(PrintAddr()==0)
    {
        printf("Succes");
    }
    else
    {
        printf("Failure");
    }

    lookup_host("labo4g-laptop");

    return 0;
}
