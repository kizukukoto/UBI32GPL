/*
 * Linux network interface code
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: interface.c,v 1.1 2009/03/11 04:06:28 peter_pan Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include "shutils.h"
#include "nvram.h"
#include "interface.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif
int
set_dev_mtu(char *name, int flags, int mtu)
{
	int s;
	struct ifreq ifr;

	DEBUG_MSG("%s, mtu: %d\n", __func__, mtu);
	if (mtu <= 0)
		return -1;

	/* Open a raw socket to the kernel */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		goto err;

	/* Set interface name */
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	/* Set mtu */
	ifr.ifr_mtu = mtu;

	if (ioctl(s, SIOCSIFMTU, &ifr) < 0)
		goto err;

	close(s);
	return 0;
 err:
	close(s);
	perror(name);
	return errno;
}

int
ifconfig(char *name, int flags, char *addr, char *netmask)
{
	int s;
	struct ifreq ifr;
	struct in_addr in_addr, in_netmask, in_broadaddr;

	/* Open a raw socket to the kernel */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		goto err;

	/* Set interface name */
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	
	/* Set interface flags */
	ifr.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
		goto err;
	
	/* Set IP address */
	if (addr) {
		inet_aton(addr, &in_addr);
		sin_addr(&ifr.ifr_addr).s_addr = in_addr.s_addr;
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(s, SIOCSIFADDR, &ifr) < 0)
			goto err;
	}

	/* Set IP netmask and broadcast */
	if (addr && netmask) {
		inet_aton(netmask, &in_netmask);
		sin_addr(&ifr.ifr_netmask).s_addr = in_netmask.s_addr;
		ifr.ifr_netmask.sa_family = AF_INET;
		if (ioctl(s, SIOCSIFNETMASK, &ifr) < 0)
			goto err;

		in_broadaddr.s_addr = (in_addr.s_addr & in_netmask.s_addr) | ~in_netmask.s_addr;
		sin_addr(&ifr.ifr_broadaddr).s_addr = in_broadaddr.s_addr;
		ifr.ifr_broadaddr.sa_family = AF_INET;
		if (ioctl(s, SIOCSIFBRDADDR, &ifr) < 0)
			goto err;
	}

	close(s);

	return 0;

 err:
	close(s);
	perror(name);
	return errno;
}	

static int
route_manip(int cmd, char *name, int metric, char *dst, char *gateway, char *genmask)
{
	int s;
	struct rtentry rt;

	/* Open a raw socket to the kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		goto err;

	/* Fill in rtentry */
	memset(&rt, 0, sizeof(rt));
	if (dst)
		inet_aton(dst, &sin_addr(&rt.rt_dst));
	if (gateway)
		inet_aton(gateway, &sin_addr(&rt.rt_gateway));
	if (genmask)
		inet_aton(genmask, &sin_addr(&rt.rt_genmask));
	rt.rt_metric = metric;
	rt.rt_flags = RTF_UP;
	if (sin_addr(&rt.rt_gateway).s_addr)
		rt.rt_flags |= RTF_GATEWAY;
	if (sin_addr(&rt.rt_genmask).s_addr == INADDR_BROADCAST)
		rt.rt_flags |= RTF_HOST;
	rt.rt_dev = name;

	/* Force address family to AF_INET */
	rt.rt_dst.sa_family = AF_INET;
	rt.rt_gateway.sa_family = AF_INET;
	rt.rt_genmask.sa_family = AF_INET;
		
	if (ioctl(s, cmd, &rt) < 0)
		goto err;

	close(s);
	return 0;

 err:
	close(s);
	perror(name);
	return errno;
}

int
route_add(char *name, int metric, char *dst, char *gateway, char *genmask)
{
	DEBUG_MSG("%s:\n",__func__);
	return route_manip(SIOCADDRT, name, metric, dst, gateway, genmask);
}

int
route_del(char *name, int metric, char *dst, char *gateway, char *genmask)
{
	DEBUG_MSG("%s:\n",__func__);
	return route_manip(SIOCDELRT, name, metric, dst, gateway, genmask);
}

/* configure loopback interface */
void
config_loopback(void)
{
	/* Bring up loopback interface */
	ifconfig("lo", IFUP, "127.0.0.1", "255.0.0.0");

	/* Add to routing table */
	route_add("lo", 0, "127.0.0.0", "0.0.0.0", "255.0.0.0");
}

/*
 * set_ethmac : set ethernet MAC addr
 * 	@IN:
 * 		int s - raw network socket
 * 		mac - ascii format 
 * 		struct ifreq *ifr - ifreq
 *
 * 	@NVRAM:
 * 		mac_clone_addr - cloned MAC
 *
 * 	@OUT
 * 		success - setting mac value(ascii), using orginal mac buffer
 */
char *set_ethmac(int s, char *mac, struct ifreq *ifr)
{
	DEBUG_MSG("%s, mac: %s\n", __func__, mac);

	memset(ifr->ifr_hwaddr.sa_data, 0, ETHER_ADDR_LEN);

	if (!mac || mac[0] == '\0' || !ether_atoe(mac, ifr->ifr_hwaddr.sa_data) || 
			memcmp(ifr->ifr_hwaddr.sa_data, "\0\0\0\0\0\0", 
				ETHER_ADDR_LEN) == 0) {
		mac = NULL;
		goto out;
	}
	/* set hw mac addr */
	ifr->ifr_hwaddr.sa_family = ARPHRD_ETHER;
	if (ioctl(s, SIOCSIFHWADDR, ifr)) {
		mac = NULL;
	}

	close(s);
out:
	return mac;
}

char *set_clone_mac(const char *dev, const char *mac)
{
	int s;
	char tmac[18];
	int if_idx;
	struct ifreq ifr;

	bzero(tmac, sizeof(tmac));
	if (!mac || *mac == '\0' || strcmp(mac, "00:00:00:00:00:00") == 0) {
		char _mac[18];

		if_idx = atoi(dev + 3) + 1;

		DEBUG_MSG("if_idx: %d\n", if_idx);

		bzero(_mac, sizeof(_mac));
		getmac(if_idx, _mac);
		sprintf(tmac, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
			_mac[0], _mac[1], _mac[2], _mac[3],
			_mac[4], _mac[5], _mac[6], _mac[7],
			_mac[8], _mac[9], _mac[10], _mac[11]);
		mac = tmac;
	}

	DEBUG_MSG("%s, dev: %s, mac: %s\n", __func__, dev, mac);

	bzero(&ifr, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

	if ((s = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		DEBUG_MSG("socket error\n");
		return NULL;
	}

	return set_ethmac(s, mac, &ifr);
}

#if 0
/* configure/start vlan interface(s) based on nvram settings */
int start_vlan(void)
{
	int s;
	struct ifreq ifr;
	int i, j;

	/* set vlan i/f name to style "vlan<ID>" */
	eval("vconfig", "set_name_type", "VLAN_PLUS_VID_NO_PAD");

	/* create vlan interfaces */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return errno;

	for (i = 0; i <= VLAN_MAXVID; i ++) {
		char nvvar_name[16];
		char vlan_id[16];
		char *emac;
		char ebuf[64];
		char prio[8];
#if 0
		/* get the address of the EMAC on which the VLAN sits */
		snprintf(nvvar_name, sizeof(nvvar_name), "vlan%dhwname", i);
		if (!(hwname = nvram_get(nvvar_name)))
			continue;
		snprintf(nvvar_name, sizeof(nvvar_name), "%smacaddr", hwname);
		if (!(hwaddr = nvram_get(nvvar_name)))
			continue;
		strcpy(ebuf, hwaddr);
		ether_atoe(hwaddr, ea);
		/* find the interface name to which the address is assigned */
		for (j = 1; j <= DEV_NUMIFS; j ++) {
			ifr.ifr_ifindex = j;
			if (ioctl(s, SIOCGIFNAME, &ifr))
				continue;
			if (ioctl(s, SIOCGIFHWADDR, &ifr))
				continue;
			if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
				continue;
			if (!bcmp(ifr.ifr_hwaddr.sa_data, ea, ETHER_ADDR_LEN))
				break;
		}
		if (j > DEV_NUMIFS)
			continue;
#endif
		snprintf(nvvar_name, sizeof(nvvar_name), "et%dmacaddr", i);
		sprintf(ebuf, nvram_safe_get(nvvar_name));
		snprintf(nvvar_name, sizeof(nvvar_name), "vlan%dhwdev", i);
		sprintf(ifr.ifr_name, nvram_safe_get(nvvar_name));
		if (ioctl(s, SIOCGIFFLAGS, &ifr))
			continue;
		if (!(ifr.ifr_flags & IFF_UP)) {
			if (!(emac = set_ethmac(s, ebuf, &ifr)))
				continue;
			
			snprintf(nvvar_name, sizeof(nvvar_name), 
					"wan%d_hwaddr", i);
			nvram_set(nvvar_name, emac);
			ifconfig(ifr.ifr_name, IFUP, 0, 0);
		}
		/* create the VLAN interface */
		snprintf(vlan_id, sizeof(vlan_id), "%d", i);
		eval("vconfig", "add", ifr.ifr_name, vlan_id);
		/* setup ingress map (vlan->priority => skb->priority) */
		snprintf(vlan_id, sizeof(vlan_id), "vlan%d", i);
		for (j = 0; j < VLAN_NUMPRIS; j ++) {
			snprintf(prio, sizeof(prio), "%d", j);
			eval("vconfig", "set_ingress_map", vlan_id, prio, prio);
		}
	}

	close(s);

	return 0;
}

/* stop/rem vlan interface(s) based on nvram settings */
int
stop_vlan(void)
{
	int i;
	char nvvar_name[16];
	char vlan_id[16];
	char *hwname;

	for (i = 0; i <= VLAN_MAXVID; i ++) {
		/* get the address of the EMAC on which the VLAN sits */
		snprintf(nvvar_name, sizeof(nvvar_name), "vlan%dhwname", i);
		if (!(hwname = nvram_get(nvvar_name)))
			continue;

		/* remove the VLAN interface */
		snprintf(vlan_id, sizeof(vlan_id), "vlan%d", i);
		eval("vconfig", "rem", vlan_id);
	}

	return 0;
}
#endif
