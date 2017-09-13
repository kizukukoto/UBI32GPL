/*
 * This file is derived from rc/network.c and will be used temporarily
 * for demo purposes!
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <net/ethernet.h>
#include <net/route.h>

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)
#define RESOLVFILE	"/var/etc/resolv.conf"

/*
char *ether_etoa(const unsigned char *e, char *a)
{
	char *c = a;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (i)
			*c++ = ':';
		c += sprintf(c, "%02X", e[i] & 0xff);
	}
	return a;
}

char *get_macaddr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	static unsigned char mac[20];

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return "";

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
		close(sockfd);
		return "";
	}
	close(sockfd);

	ether_etoa(ifr.ifr_hwaddr.sa_data, mac);

	return mac;
}
*/
char *get_ipaddr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr addr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		return 0;
	}

	close(sockfd);
	
	addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	return inet_ntoa(addr);
}

char *get_netmask(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr netmask;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
		close(sockfd);
		return 0;
	}

	close(sockfd);
	
	netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
	return inet_ntoa(netmask);
}

char *get_dns(int index)
{
	FILE *fp;
	char buff[1024] = {};
	char dns[4][20] = {};
	int i=0;
	char *dns_s=NULL;

	memset(dns, 0, sizeof(dns));
	memset(buff, 0, sizeof(buff));

	fp = fopen (RESOLVFILE, "r");
	if(!fp) {
		perror(RESOLVFILE);
		return 0;
	}

	/*ex. nameserver 168.95.1.1 */
	while( fgets(buff, 1024, fp))
	{
		if (strstr(buff, "nameserver") )
		{
			dns_s = buff;
			dns_s = buff + strlen("nameserver ");
			if(strchr(buff, '\n')) /*Line Feed*/
				strncpy(dns[i], dns_s, strlen(buff) - strlen("nameserver ") - 1);
			else
				strncpy(dns[i], dns_s, strlen(buff) - strlen("nameserver "));
			i++;
		}
		if(4<=i)
			break;
	}

	/* add end char "\0" */
	if(4<=i)
		strcat(dns[3], "\0");
	else
		strcat(dns[i], "\0");

	fclose(fp);
	return dns[index];
}
