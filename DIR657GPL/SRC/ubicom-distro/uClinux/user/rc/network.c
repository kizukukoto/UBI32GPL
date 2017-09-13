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

#include <sutil.h>
#include <shvar.h>
#include <nvram.h>
#include <rc.h>


#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define IFDOWN	0

int ifconfig(char *ifname, int op, char *addr, char *netmask)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr, in_netmask, in_broadaddr;

	if (!ifname)
		return -1;
	else if (0==strcmp(ifname,""))
		return -1;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		goto err;

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	
	ifr.ifr_flags = op;
	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
		goto err;
	
	if (addr) {
		if (0!=strcmp(addr,"")) {
			inet_aton(addr, &in_addr);
			sin_addr(&ifr.ifr_addr).s_addr = in_addr.s_addr;
			ifr.ifr_addr.sa_family = AF_INET;
			if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0)
				goto err;
		}
	} 

	if (addr && netmask) {
		if ((0!=strcmp(addr,"")) && (0!=strcmp(netmask,""))) {
			inet_aton(netmask, &in_netmask);
			sin_addr(&ifr.ifr_netmask).s_addr = in_netmask.s_addr;
			ifr.ifr_netmask.sa_family = AF_INET;
			if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0)
				goto err;

			in_broadaddr.s_addr = (in_addr.s_addr & in_netmask.s_addr) | ~in_netmask.s_addr;
			sin_addr(&ifr.ifr_broadaddr).s_addr = in_broadaddr.s_addr;
			ifr.ifr_broadaddr.sa_family = AF_INET;
			if (ioctl(sockfd, SIOCSIFBRDADDR, &ifr) < 0)
				goto err;
		}
	}

	close(sockfd);
	return 0;

 err:
	close(sockfd);
	perror(ifname);
	return errno;
}	


int ifconfig_up(char *ifname, char *addr, char *netmask)
{
	_system("ifconfig eth0 txqueuelen 750");
	return ifconfig(ifname, IFUP, addr, netmask);
}

int ifconfig_down(char *ifname, char *addr, char *netmask)
{
	return ifconfig(ifname, 0, addr, netmask);
}


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


char *get_ipaddr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr;
	char *ip_tmp=NULL;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return NULL;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		return NULL;
	}

	close(sockfd);
	
	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
		
/*	Date: 2010-01-13
*	Name: Jimmy Huang
*	Reason:	Add support for Windows Mobile 5
*	Note:
*		Because Windows Mobile Phone's feature Internet Sharing
*		If we plug in the device but not activate the feature
*		udhcpc will acquire 169.254.x.x
*		In order to check if we should keep trying udhcpc discover
*		We consider 169.254.x.x as invalid IP, thus wantimer will keep
*		discovering dhcp server to acquire valid IP
*/
	ip_tmp = inet_ntoa(in_addr);
	if(ip_tmp && strncmp(ip_tmp,"169.254",7) == 0){//APIPA, Automatic Private IP Addressing, we should not consider it as valid IP
		return NULL;
	}else
		return ip_tmp;
	//return inet_ntoa(in_addr);
}

char *get_broadcast_addr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFBRDADDR, &ifr) < 0) {
		close(sockfd);
		return 0;
	}

	close(sockfd);
	
	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	return inet_ntoa(in_addr);
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


int get_single_ip(char *ipaddr, int order)
{
    int ip[4]={0,0,0,0};
    int ret;

    ret = sscanf(ipaddr,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);

    return ip[order];	
}



char *get_domain(void)
{
	FILE *fp;
	char *buff;
	static char domain[50] ={};
	int num, len;

	memset(domain, 0, sizeof(domain) );
	fp = fopen (RESOLVFILE, "r");
	if(!fp) {
		perror(RESOLVFILE);
		return 0;
	}
	buff = (char *) malloc(1024);
	memset(buff, 0, 1024);
	while( fgets(buff, 1024, fp)) {
		if (strstr(buff, "domain") ) 
		{
			num = strspn(buff+ 6, " ");
			len = strlen( (buff + 6 + num) );
			strncpy(domain, (buff + 6 + num), len-1 );
			strcat(domain, "\0");
		}
		else
			strcpy(domain, "\0");
	}
	free(buff);
	fclose(fp);
	return domain;

}

char *get_dns(void)
{
	FILE *fp;
	char buff[1024] = {};
	char dns[4][20] = {};
	static char return_dns[64] ="";
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
	sprintf(return_dns, "%s %s %s %s", dns[0], dns[1], dns[2], dns[3]);
	return return_dns;
}


int route_man(const int op, char *ifname, const char *rt_destaddr, const char *rt_netmask, const char *rt_gateway, const int rt_metric)
{
	struct rtentry rt;
	int sock_fd = -1;
	DEBUG_MSG("route_add: ifname=%s,rt_destaddr=%s,rt_netmask=%s,rt_gateway=%s,rt_metric=%d\n",ifname,rt_destaddr,rt_netmask,rt_gateway,rt_metric);

	memset(&rt, 0, sizeof(rt));	
	rt.rt_dst.sa_family = AF_INET;
	rt.rt_gateway.sa_family = AF_INET;
	rt.rt_genmask.sa_family = AF_INET;
	if (rt_destaddr)
		inet_aton(rt_destaddr, &sin_addr(&rt.rt_dst));
	if (rt_gateway)
		inet_aton(rt_gateway, &sin_addr(&rt.rt_gateway));
	if (rt_netmask)
		inet_aton(rt_netmask, &sin_addr(&rt.rt_genmask));	
	rt.rt_metric = rt_metric +1;
	rt.rt_dev = ifname;

	rt.rt_flags = RTF_UP;
	if (sin_addr(&rt.rt_gateway).s_addr)
		rt.rt_flags |= RTF_GATEWAY;
	if (sin_addr(&rt.rt_genmask).s_addr == INADDR_BROADCAST)
		rt.rt_flags |= RTF_HOST;
	
	if( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) <0) 
		goto err;

	if (ioctl(sock_fd, op, &rt) < 0 ) 
		goto err;

	close(sock_fd);
	/* return 0 if success, return -1 if fail */
	return 0;
err:
	close(sock_fd);
	perror(ifname);
	return errno;
		
}

int route_add(char *ifname, char *rt_destaddr, char *rt_netmask, char *rt_gateway, int rt_metric)
{
	return route_man(SIOCADDRT, ifname, rt_destaddr, rt_netmask, rt_gateway, rt_metric);
}


int route_del(char *ifname, char *rt_destaddr, char *rt_netmask, char *rt_gateway, int rt_metric)
{
	return route_man(SIOCDELRT, ifname, rt_destaddr, rt_netmask, rt_gateway, rt_metric);
}

int ifconfig_mac(char *eth, char *chmac)
{
	int mac[6] ={0};

	if( chmac == NULL || eth == NULL)
		return -1;
	else if ((0==strcmp(eth,"")) || (0==strcmp(chmac,"")))
		return -1;

	if ( strcasecmp(chmac, get_macaddr( eth )) !=0 ) 
	{
		sscanf(chmac,"%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		_system("ifconfig %s down", eth);
		_system("ifconfig %s hw ether %02x%02x%02x%02x%02x%02x", eth, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		_system("ifconfig eth0 txqueuelen 750");
		_system("ifconfig %s up", eth);
	}
}


int RC_get_if_pkt_counters(const char if_name[], struct pkt_counter_s *pkt_count)
{
	#define PATH_NET_DEV "/proc/net/dev"
	
	FILE *fp;
	char devname[IFNAMSIZ];
	char buf[256];
	int len;

	if (if_name == NULL) return (-1);
	if (pkt_count == NULL) return (-1);
		
	memset(pkt_count, 0, sizeof(pkt_count));
	
#if 1
	len = strlen(if_name);
	strcpy(devname, if_name);
	devname[len] = ':';
	devname[len+1] = 0;
#else	/* test */
	strcpy(devname, "eth0:");
#endif
	
	DEBUG_MSG("devname = %s\n", devname);
	
	if ((fp = fopen(PATH_NET_DEV, "r")) == NULL)
	{
		printf("can't open %s: %s", PATH_NET_DEV, strerror(errno));
		return (-1);	
	}

	while (fscanf(fp, "%s", buf) != EOF)
	{
		DEBUG_MSG("%s\t", buf);
		if(strcmp(devname, buf) == 0)
			goto found_label;
	}
	fclose (fp);
	return (-2);

found_label:	
	
	DEBUG_MSG("found %s\n", devname);	

	/* format */
	/* bytes packets errs drop fifo frame compressed multicast|bytes packets errs drop fifo colls carrier compressed */

	/* rx bytes */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_bytes = atoi(buf);
	DEBUG_MSG("%s: rx bytes = %d\n", if_name, pkt_count->rx_bytes);
	
	/* rx pkts */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_packets = atoi(buf);
	DEBUG_MSG("%s: rx packets = %d\n", if_name, pkt_count->rx_packets);
	
	/* rx errs */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_errs = atoi(buf);
	DEBUG_MSG("%s: rx errs = %d\n", if_name, pkt_count->rx_errs);
	
	/* rx drop */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_drop = atoi(buf);
	DEBUG_MSG("%s: rx drop = %d\n", if_name, pkt_count->rx_drop);
	
	/* rx fifo */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_fifo = atoi(buf);
	DEBUG_MSG("%s: rx fifo = %d\n", if_name, pkt_count->rx_fifo);
	
	/* rx frame */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_frame = atoi(buf);
	DEBUG_MSG("%s: rx frame = %d\n", if_name, pkt_count->rx_frame);
	
	/* rx compressed */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_compressed = atoi(buf);
	DEBUG_MSG("%s: rx compressed = %d\n", if_name, pkt_count->rx_compressed);
	
	/* rx multicast */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->rx_multicast = atoi(buf);
	DEBUG_MSG("%s: rx multicast = %d\n", if_name, pkt_count->rx_multicast);
	
	/* tx bytes */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_bytes = atoi(buf);
	DEBUG_MSG("%s: tx bytes = %d\n", if_name, pkt_count->tx_bytes);
	
	/* tx pkts */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_packets = atoi(buf);
	DEBUG_MSG("%s: tx pkts = %d\n", if_name, pkt_count->tx_packets);
	
	/* tx errs */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_errs = atoi(buf);
	DEBUG_MSG("%s: tx errs = %d\n", if_name, pkt_count->tx_errs);
	
	/* tx drop */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_drop = atoi(buf);
	DEBUG_MSG("%s: tx drop = %d\n", if_name, pkt_count->tx_drop);
	
	/* tx fifo */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_fifo = atoi(buf);
	DEBUG_MSG("%s: tx fifo = %d\n", if_name, pkt_count->tx_fifo);
	
	/* tx colls */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_colls = atoi(buf);
	DEBUG_MSG("%s: tx colls = %d\n", if_name, pkt_count->tx_colls);
	
	/* tx carrier */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_carrier = atoi(buf);
	DEBUG_MSG("%s: tx carrier = %d\n", if_name, pkt_count->tx_carrier);
	
	/* tx compressed */
	if (fscanf(fp, "%s", buf) == EOF) goto error_label;
	pkt_count->tx_compressed = atoi(buf);
	DEBUG_MSG("%s: tx compressed = %d\n", if_name, pkt_count->tx_compressed);
		
	fclose (fp);
	return (0);
	
error_label:
	fclose (fp);
	return (-1);
	
}
