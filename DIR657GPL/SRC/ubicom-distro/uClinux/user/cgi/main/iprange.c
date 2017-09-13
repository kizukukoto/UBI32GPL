#include <stdio.h>
#include <tgmath.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ssi.h"

#if 1
#define DEBUG_MSG	cprintf
#else
#define DEBUG_MSG 
#endif

static int singal_ctrl(const char *ip, const char *userip)
{
	int ret = -1;

	if (strcmp(ip, userip))
		goto out;

	DEBUG_MSG("SINGAL [%s] -> [%s]\n", ip, userip);

	ret = 0;
out:
	return ret;
}

static int range_ctrl(char *iprange, const char *userip)
{
	int ret = -1;
	char *ipstart = strsep(&iprange, "-");
	char *ipend = iprange;

	DEBUG_MSG("RANGE [%s]-[%s] -> [%s]\n", ipstart, ipend, userip);

	if (ntohl(inet_addr(userip)) < ntohl(inet_addr(ipstart)) ||
		ntohl(inet_addr(userip)) > ntohl(inet_addr(ipend)))
		goto out;

	ret = 0;
out:
	return ret;
}

static unsigned long bit_to_inetaddr(int bit)
{
	int i = 0;
	char ipaddr[16], maskaddr[33];

	bzero(ipaddr, sizeof(ipaddr));
	memset(maskaddr, '0', 32);
	memset(maskaddr, '1', bit);

	for (; i < 4; i++) {
		int j = 7, subip_addr = 0;
		char subip[9];

		bzero(subip, sizeof(subip));
		memcpy(subip, maskaddr + 8 * i, 8);

		for (; j >= 0; j--) {
			if (subip[j] == '1')
				subip_addr += pow(2, 7 - j);
		}
		sprintf(ipaddr, "%s%d.", ipaddr, subip_addr);
	}

	ipaddr[strlen(ipaddr) - 1] = '\0';

	return ntohl(inet_addr(ipaddr));
}

static int netmask_ctrl(char *ipnetwork, const char *userip)
{
	int ret = -1;
	char *ip = strsep(&ipnetwork, "/");
	char *mask = ipnetwork;
	unsigned long userip_inetaddr = ntohl(inet_addr(userip));
	unsigned long ip_inetaddr = ntohl(inet_addr(ip));
	unsigned long mask_inetaddr = bit_to_inetaddr(atoi(mask));

	DEBUG_MSG("MASK [%s/%s] -> [%s]\n", ip, mask, userip);
	DEBUG_MSG("ip_inetaddr [%s] -> [%u]\n", ip, ip_inetaddr);
	DEBUG_MSG("mask [%s] -> [%u]\n", mask, mask_inetaddr);
	DEBUG_MSG("userip [%s] -> [%u]\n", userip, userip_inetaddr);
	DEBUG_MSG("userip & mask -> [%u]\n", userip_inetaddr & mask_inetaddr);

	if ((userip_inetaddr & mask_inetaddr) != ip_inetaddr)
		goto out;

	ret = 0;
out:
	return ret;
}

/*
 * @multi_addr_fmt := "172.21.1.1;172.21.1.3-172.21.1.10,192.168.0.0/24";
 * @userip := "192.168.0.100";
 * */
int iprange_ctrl(const char *multi_addr_fmt, const char *userip)
{
	int ret = -1;
	char *s, *p, multi_addr_array[1024];

	DEBUG_MSG("XXX iprange_ctrl.multi_addr_fmt [%s]\n", multi_addr_fmt);
	DEBUG_MSG("XXX iprange_ctrl.userip [%s]\n", userip);

	if (multi_addr_fmt == NULL || *multi_addr_fmt == '\0')
		goto out;
	if (userip == NULL || *userip == '\0')
		goto out;

	DEBUG_MSG("XXX iprange_ctrl.parameters ok\n");

	p = multi_addr_array;
	strcpy(multi_addr_array, multi_addr_fmt);

	DEBUG_MSG("XXX iprange_ctrl.multi_addr_array [%s]\n", multi_addr_array);

	while ((s = strsep(&p, ",;")) != NULL) {
		if (strchr(s, '-') == NULL && strchr(s, '/') == NULL)
			ret = singal_ctrl(s, userip);
		else if (strchr(s, '-') != NULL)
			ret = range_ctrl(s, userip);
		else
			ret = netmask_ctrl(s, userip);

		if (ret == 0)
			goto out;
	}
out:
	DEBUG_MSG("XXX iprange_ctrl.ret [%d]\n", ret);
	return ret;
}
