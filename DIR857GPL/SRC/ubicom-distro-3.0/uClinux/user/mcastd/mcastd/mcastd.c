#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <errno.h>
#include <err.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <fcntl.h>
#include <stdint.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cmds.h"
#include "hal.h"
struct uinet_addr{
	union {
		struct in6_addr in6;
		struct in_addr in4;
	}addr;
	int type;
};
/*
 * transform @group_str to hw ether addr @e
 * e.g. - @group_str "ff0e::1235" to "333300001235"
 * */
int ether_mtoe(const char *group_str, int type, unsigned char *e)
{
	struct uinet_addr a;
	char *p;

	if (inet_pton(type, group_str, &a.addr) <= 0)
		return -1;
	p = (char *)&a.addr;
	if (type == AF_INET6)
		p += 12;
	else
		return -1;	//v4 not support
	e[0] = 0x33;
	e[1] = 0x33;
	memcpy(e + 2, p, 4);
	return 0;
}
/*
mkdir /tmp/rootfs;mount -o nolock 172.21.49.156:/share/rootfs /tmp/rootfs
cp /tmp/rootfs/mcastd /tmp/
sleep 3;LD_LIBRARY_PATH=/tmp/rootfs/ /tmp/rootfs/mcastd join inet6 ff0e::1234 00:11:22:33:44:55 1 & 


 */
static int ether_igmp_join_leave(int join, unsigned char *ether_mcast, unsigned char *ether_from, int vid)
{
	unsigned int mcast_pmap, from_pmap;
	
	mcast_pmap = atu_search(ether_mcast, vid);
	from_pmap = atu_search(ether_from, vid);
	fprintf(stderr, "%s: %02X, %02X\n", join ? "join":"leave", mcast_pmap, from_pmap);
	if (join) {
		if (mcast_pmap & from_pmap)
			return 0; //already exist
		atu_add(ether_mcast, vid, mcast_pmap|from_pmap);
	} else {
		if (mcast_pmap == -1)
			return 0;
		if (!(mcast_pmap & from_pmap)) {
			printf("not exit in atu");
			atu_purge(ether_mcast, 1);
			return 0; //not exit
		}
		mcast_pmap &= ~from_pmap;
		fprintf(stderr, "%s %d: to portmap: %02X\n", __FUNCTION__, __LINE__, mcast_pmap);
		if (mcast_pmap)
			atu_add(ether_mcast, vid, mcast_pmap);
		else
			atu_purge(ether_mcast, vid);
	}
	return 0;
}
/*
 * used by /sbin/igmp.event
 *
 * SRC='fe80::224:e8ff:feb1:77fd'
 * ACTION='join'
 * GROUP='ff0e::1235'
 * e.g.
 * 	join inet6 ff0e::1235 00:12:34:56:78:9a
 * */
static int igmp_join_main(int argc ,char *argv[])
{
	int type, vid = 1, portmap, client_portmap;
	unsigned char e[ETHER_ADDR_LEN], e2[ETHER_ADDR_LEN];

	if (argc < 4)
		exit(0);

	if (argc > 4)
		vid = atoi(argv[4]);

	if (strcmp("inet6", argv[1]) != 0)
		return -1;	//ipv6 only
	type = strcmp("inet6", argv[1]) == 0 ? AF_INET6 : AF_INET;

	if (ether_mtoe(argv[2], type, e) == -1)
		return -1;
	printf("%02X:%02X:%02X:%02X:%02X:%02X\n", e[0], e[1], e[2], e[3], e[4], e[5]);

	/* 
 	 * get ether multicast portmap.
	 * I assume LAN ports VID is 1 in WASP design 
	 * */
	if ((portmap = atu_search(e, vid)) == -1)
		portmap = 0;
	printf("port map %02X\n", portmap);

	/* get port client portmap */
	__ether_atoe(argv[3], e2);
	if ((client_portmap = atu_search(e2, vid)) == -1)
		return 0;	/* client srouce mac not exist in ATU */
	printf("clinet portmap %02X\n", client_portmap);

	if (client_portmap & portmap)
		return 0;	/* already in ATU */
	printf("=============add new atu = to portmap %X\n", portmap |client_portmap);
	atu_add(e, vid, portmap |client_portmap);
	return 0;
}

static int igmp_leave_main(int argc ,char *argv[])
{
	int type, vid = 1;
	unsigned char e[ETHER_ADDR_LEN], e2[ETHER_ADDR_LEN];

	if (argc < 4)
		exit(0);
	if (argc > 4)
		vid = atoi(argv[4]);

	if (strcmp("inet6", argv[1]) != 0)
		return -1;	//ipv6 only
	type = strcmp("inet6", argv[1]) == 0 ? AF_INET6 : AF_INET;

	if (ether_mtoe(argv[2], type, e) == -1)
		return -1;
	__ether_atoe(argv[3], e2);
	//fprintf(stderr, "%02X:%02X:%02X:%02X:%02X:%02X\n", e[0], e[1], e[2], e[3], e[4], e[5]);
	//fprintf(stderr, "%02X:%02X:%02X:%02X:%02X:%02X\n", e2[0], e2[1], e2[2], e2[3], e2[4], e2[5]);
	ether_igmp_join_leave(0, e, e2, vid);
	return 0;
}
static int atu_add_main(int argc, char *argv[])
{
	unsigned char e[128];
	
	if (argc < 4)
		exit(0);
	__ether_atoe(argv[1], e);
	return atu_add(e, atoi(argv[2]), atoi(argv[3]));
}

static int atu_del_main(int argc, char *argv[])
{
	unsigned char e[128];
	if (argc < 3)
		exit(0);
	__ether_atoe(argv[1], e);

	return atu_purge(e, atoi(argv[2]));
}
static int atu_search_main(int argc, char *argv[])
{
	unsigned char e[128];
	if (argc < 3)
		exit(0);
	__ether_atoe(argv[1], e);
	atu_search(e, atoi(argv[2]));
	return 0;
}
static int atu_dump_main(int argc, char *argv[])
{
	atu_get_next();
	return 0;	
}
static int atu_flush_main()
{
	atu_flushall();
	return 0;
}

static int igmp_atu_main(int argc ,char *argv[])
{
	struct subcmd cmds[] = {
	 	{"add", atu_add_main, "add [mac] [vlan] [portmap]"},
		{"del", atu_del_main, "del [mac] [vlan]"},
		{"search", atu_search_main, "search [mac] [vlan]"},
		{"dump", atu_dump_main, "dump atu"},
		{"flush", atu_flush_main, "flush atu"},
		{NULL, NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

static int acl_dump_main(int argc, char *argv[])
{
	int debug = 0;

	if (argc > 1)
		if (strcmp(argv[1], "-D") == 0)
			debug = 1;
	acl_dump(debug);
	return 0;	
}
static int acl_disable_entry_main(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	acl_disable_entry(atoi(argv[1]));
	return 0;	
}
static int acl_enable_entry_main(int argc, char *argv[])
{
	if (argc < 3)
		return -1;
	acl_enable_entry(atoi(argv[1]), argv[2]);
	return 0;	
}

static int acl_add_entry_main(int argc ,char *argv[])
{
	return acl_add_mac_entry(argc, argv);
}

extern int acl_ipv4_add_entry(int argc, char *argv[]);
static int acl_ipv4_main(int argc ,char *argv[])
{
	struct subcmd cmds[] = {
		{"add", acl_ipv4_add_entry, "add ipv4 rule entry"},
		{NULL, NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
static int acl_main(int argc ,char *argv[])
{
	struct subcmd cmds[] = {
	 	{"dump", acl_dump_main, "dump acl, -D for debug"},
	 	{"disable", acl_disable_entry_main, "disable entry [RuleNumber]"},
	 	{"enable", acl_enable_entry_main, "enable entry [RuleNumber] [MAC|IPv4|IPv6_Rule1|IPv6_Rule2|IPv6_Rule3|Window|EnhanceMAC]"},
		{"add", acl_add_entry_main, "add MAC rule entry"},
		{"ipv4", acl_ipv4_main, "acl ipv4 entry"},
		{NULL, NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
int main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"join", igmp_join_main, "join [inet6|inet4] [group_addr] [src_mac] <vid>"},
		{"leave", igmp_leave_main, "leave [inet6|inet4] [group_addr] [src_mac] <vid>"},
		{"hwarp", igmp_atu_main, "address table"},
		{"acl", acl_main, "acl"},
		{NULL, NULL, NULL}
	};
	reg_init();
	//acl_add_mac_entry(12);
	//acl_dump();
	return lookup_subcmd_then_callout(argc, argv, cmds);
	return 0;
}
