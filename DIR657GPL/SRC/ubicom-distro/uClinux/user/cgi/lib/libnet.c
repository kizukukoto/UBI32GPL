#include <unistd.h>
#include "libdb.h"
#include "netinf.h"
#define PATH_PROC_ROUTE "/proc/net/route"
#define PATH_RESOLVECONF "/var/etc/resolv.conf"
/* Get IP from interface @ifname
 * Input:
 *	@ifname: name of interface
 * Output:
 *	@buf is converted by inet_ntoa()
 */

int get_ip(const char *ifname, char *buf)
{
	int s, r, ret = -1;
	struct ifreq ifr;
	struct sockaddr_in in;
	
	s = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifname);
	r = ioctl(s, SIOCGIFADDR, &ifr);
	close(s);
	if (r < 0){
		return ret;
	}
	memcpy(&in, &ifr.ifr_addr, sizeof(struct sockaddr_in));
	strcpy(buf, (char *)inet_ntoa(in.sin_addr));
	ret = 0;
	return ret;
}

int get_mac(const char *ifname, char *buf)
{
	int s, r, ret = -1;
	struct ifreq ifr;
	unsigned char *mac;
	
	s = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifname);
	r = ioctl(s, SIOCGIFHWADDR,&ifr);
	close(s);
	if (r < 0){
		return ret;
	}
	mac = (unsigned char *)ifr.ifr_netmask.sa_data;
	sprintf(buf, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	ret = 0;
	return ret;
}

int get_netmask(const char *ifname, char *buf)
{
	int s, r, ret = -1;
	struct ifreq ifr;
	struct sockaddr_in in;
	
	s = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ifr,0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifname);
	r = ioctl(s, SIOCGIFNETMASK,&ifr);
	close(s);
	if (r < 0){
		return ret;
	}
	memcpy(&in, &ifr.ifr_netmask, sizeof(struct sockaddr_in));
	strcpy(buf, (char *)inet_ntoa(in.sin_addr));
	ret = 0;
	return ret;
}

/*
 * Get default gateway from @PATH_PROC_ROUTE
 */
void get_def_gw(char *out)
{
	FILE *fd;
	char buf[256];
	//unsigned long int dst = -1;
	unsigned int dst = -1;
	struct in_addr in;

	if (NVRAM_MATCH("wan_proto", "pppoe")) {
		strcpy(out, nvram_safe_get("pppoe_remoteip"));
		return;
	}
	if ((fd = fopen(PATH_PROC_ROUTE, "r")) == NULL)
		return;

	/*XXX:Are we sure last line is default gw? */
	while(fgets(buf, sizeof(buf), fd) > 0) {
		sscanf(buf, "%*s\t%x\t%x\t%*s", &dst, &in.s_addr);
		if (dst == 0) {
			sprintf(out, "%s", inet_ntoa(in));
			break;
		}
	}

	fclose(fd);
	return;
}

static void chop(char *io)
{
	char *p;

	p = io + strlen(io) - 1;
	while ( *p == '\n' || *p == '\r') {
		*p = '\0';
		p--;
	}
	return;
}

/*
 * Get name server ip from @PATH_RESOLVECONF
 */
void get_dns_srv(char *out)
{
	FILE *fd;
	char buf[256];
	
	if((fd = fopen(PATH_RESOLVECONF, "r")) == NULL)
		return;
	
	while(fgets(buf, sizeof(buf), fd)) {
		char *p;
		p = buf;
		
		if(strncmp(buf, "nameserver", strlen("nameserver")) != 0)
			continue;
		
		p += strlen("nameserver");
		while (*p == ' ' || *p == '\t')
			p++;
		chop(p);
		if (strcmp(p, "0.0.0.0") == 0)
			continue;
		sprintf(out+strlen(out), "%s ", p);
	}

	fclose(fd);
}

