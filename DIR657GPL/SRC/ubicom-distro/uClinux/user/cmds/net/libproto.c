#include <stdio.h>
#include <stdlib.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "proto.h"
#include "interface.h"
#include "libcameo.h"
#include "nvram_ext.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static struct proto_map_struct {
	int proto;
	char *proto_str;
} proto_struct[] = {
	{ 0,		""},
	{ PROTO_STATIC,	PROTO_STATIC_STR},
	{ PROTO_DHCPC,	PROTO_DHCPC_STR},
	{ PROTO_PPPOE,	PROTO_PPPOE_STR},
	{ PROTO_PPTP,	PROTO_PPTP_STR},
	{ PROTO_L2TP,	PROTO_L2TP_STR},
	{ PROTO_USB3G,	PROTO_USB3G_STR},
};

/* beware the order of elements */
char *PROTO_STR[] = {
	"",
	PROTO_STATIC_STR,
	PROTO_DHCPC_STR,
	PROTO_PPPOE_STR,
	PROTO_PPTP_STR,
	PROTO_L2TP_STR,
	PROTO_USB3G_STR,
};
static inline int proto_ok(int proto)
{
	return (0 <= proto && proto <= PROTO_MAX) ? 1 : 0;
}

static inline void 
__init_proto_base(const char *proto_str, struct proto_base *pb, int idx)
{
	struct {
		char *map;
   		char **key;		
	} base_key[] = {
		{"_alias", &pb->alias},
		{"_hostname", &pb->hostname},
		{"_addr", &pb->addr},
		{"_netmask", &pb->netmask},
		{"_gateway", &pb->gateway},
		{"_dns", &pb->dns},
		{"_mac", &pb->mac},
		{"_mtu", &pb->mtu},
		{NULL, NULL}
	}, *m;
	char tmp[512];
	
	pb->index = idx;
	for (m = base_key; m->map != NULL; m++) {
		/* ex @tmp = "static_alias2" */
		sprintf(tmp, "%s%s%d", proto_str, m->map, idx);
		*m->key = nvram_safe_get(tmp);
		//DEBUG_MSG("%s=%p:%s\n", tmp, *m->key, *m->key);
	}
	return;
}
static void dump_proto_base(struct proto_base *pb, FILE *fd)
{
	fprintf(fd, "*** Base info ***\n"
		"index: %d, alias: %s, addr: %s, mask: %s, gw: %s\n"
		"dns: %s, mtu: %s, mac: %s\n",
		pb->index, pb->alias, pb->addr, pb->netmask, pb->gateway,
		pb->dns, pb->mtu, pb->mac);
}

static int init_proto_base(int proto, struct proto_base *pb, int idx)
{
	int rev = -1;

	if (!proto_ok(proto))
		goto out;
	__init_proto_base(proto_struct[proto].proto_str, pb, idx);
	//dump_proto_base(pb, stdout);
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int init_proto_static(struct proto_base *pb, int size, int idx)
{
	return init_proto_base(PROTO_STATIC, pb, idx);
}

int init_proto_dhcpc(struct proto_base *pb, int size, int idx)
{
	return init_proto_base(PROTO_DHCPC, pb, idx);
}

static void init_proto_ppp_base(const char *proto_str, struct proto_pppoe *pb, int idx)
{
/* 
#define PPP_STUFF struct {			\
	char *mru, *mtu, *idle, *unit, *dial;	\
	char *user, *pass, *servicename;	\
	char *ip, *dns1, *dns2, *mode, *mppe;	\
}
*/
	struct {
		char *map;
   		char **key;		
	} base_key[] = {
		{"_mru", &pb->mru},
		{"_mtu", &pb->mtu},
		{"_idle", &pb->idle},
		{"_unit", &pb->unit},
		{"_dial", &pb->dial},
		{"_user", &pb->user},
		{"_pass", &pb->pass},
		{"_servicename", &pb->servicename},
		{"_primary_dns", &pb->dns1},
		{"_secondary_dns", &pb->dns2},
		{"_mode", &pb->mode},
		{"_mppe", &pb->mppe},
		{NULL, NULL}
	}, *m;
	char tmp[512];
	
	for (m = base_key; m->map != NULL; m++) {
		/* ex @tmp = "static_alias2" */
		sprintf(tmp, "%s%s%d", proto_str, m->map, idx);
		*m->key = nvram_safe_get(tmp);
		//printf("%s=%p:%s\n", tmp, *m->key, *m->key);
	}
	return;
}

int init_proto_pppoe(struct proto_base *pb, int size, int idx)
{
	int rev = -1;
	struct proto_pppoe *poe;
	if (size < sizeof(struct proto_pppoe))
		goto out;
	
	init_proto_base(PROTO_PPPOE, pb, idx);
	poe = (struct proto_pppoe *)pb;
	init_proto_ppp_base(PROTO_PPPOE_STR, poe, idx);
	rev = 0;
out:
	return rev;
}

int init_proto_pptp(struct proto_base *pb, int size, int idx)
{
	int rev = -1;
	struct proto_pptp *pptp;
	
	if (size < sizeof(struct proto_pptp))
		goto out;
	
	init_proto_base(PROTO_PPTP, pb, idx);
	pptp = (struct proto_pptp *)pb;
	init_proto_ppp_base(PROTO_PPTP_STR, (void *)pptp, idx);
	pptp->serverip =  nvram_safe_get_i("pptp_serverip", idx, g1);
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int init_proto_l2tp(struct proto_base *pb, int size, int idx)
{
	int rev = -1;
	struct proto_l2tp *l2tp;
	
	if (size < sizeof(struct proto_l2tp))
		goto out;
	
	init_proto_base(PROTO_L2TP, pb, idx);
	l2tp = (struct proto_l2tp *)pb;
	init_proto_ppp_base(PROTO_L2TP_STR, (void *)l2tp, idx);
	l2tp->serverip =  nvram_safe_get_i("l2tp_serverip", idx, g1);
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int init_proto_usb3g(struct proto_base *pb, int size, int idx)
{
	int rev = -1;
	struct proto_usb3g *usb3g;
	
	if (size < sizeof(struct proto_usb3g))
		goto out;
	
	init_proto_base(PROTO_USB3G, pb, idx);
	usb3g = (struct proto_usb3g *)pb;
	init_proto_ppp_base(PROTO_USB3G_STR, (void *)usb3g, idx);
	
	usb3g->dial_num =  nvram_safe_get_i("usb3g_dial_num", idx, g1);
	usb3g->apn_name =  nvram_safe_get_i("usb3g_apn_name", idx, g1);
	/* TODO: extra params ...*/
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

void dump_proto_struct(int proto, struct proto_base *pb, FILE *fd)
{
	dump_proto_base(pb, fd);

	switch (proto) {
	case PROTO_STATIC:
		break;
	case PROTO_DHCPC:
		break;
	case PROTO_PPTP:
	case PROTO_L2TP:
		{
			struct proto_pppoe *l2tp = (struct proto_l2tp *)pb;
			fprintf(fd, "serverip:%s ,", l2tp->serverip);
		}
			
	case PROTO_PPPOE:
		{
			struct proto_pppoe *poe = (struct proto_pppoe *)pb;
			fprintf(fd, "user: %s, pass: %s, mru: %s, mtu: %s, idle: %s\n"
				" unit: %s, dail: %s, mode: %s, servicename: %s\n"
				" dns1: %s, dns2: %s, mppe: %s\n",
				poe->user, poe->pass, poe->mru, poe->mtu, poe->idle,
			       	poe->unit, poe->dial, poe->mode, poe->servicename,
				poe->dns1, poe->dns2, poe->mppe);
		}
		break;
	case PROTO_USB3G:
		{
			struct proto_pppoe *poe = (struct proto_pppoe *)pb;
			struct proto_usb3g *usb3g = (struct proto_usb3g *)pb;
			fprintf(fd, "user: %s, pass: %s, mru: %s, mtu: %s, idle: %s\n"
				" unit: %s, dail: %s, mode: %s, servicename: %s\n"
				" dns1: %s, dns2: %s, mppe: %s\n",
				poe->user, poe->pass, poe->mru, poe->mtu, poe->idle,
			       	poe->unit, poe->dial, poe->mode, poe->servicename,
				poe->dns1, poe->dns2, poe->mppe);
			fprintf(fd, "apn: %s, dial_num: %s\n",
				usb3g->dial_num, usb3g->apn_name);

		}
		break;
	defualt:
		debug( "Unknow protocol\n");
		break;
	}
}

int proto_xxx_status(const char *dev, const char *phy)
{
	int rev = 0;
	char ip[32] = "0.0.0.0", mask[24] = "255.255.255.255", mac[32] = "";
	char *st = "unknow";

	if (get_ip(dev, ip) == -1) rev = -1;
	if (get_netmask(dev, mask) == -1) rev = -1;
	if (get_mac(phy, mac) == -1) rev = -1;

	printf("%s/%s %s\n", ip, mask, mac);
out:
	return rev;
}

int proto_xxx_status2(int proto, const char *alias, const char *dev, const char *phy)
{
	int rev = -1, i;
	char buf[128], *st;
	
	sprintf(buf, "%s_alias", PROTO_STR[proto]);
	if ((i = nvram_find_index(buf, alias)) == -1)
		goto out;
	sprintf(buf, "%s_status", PROTO_STR[proto]);
	
	if ((st = nvram_get_i(buf, i, g1)) == NULL)
		st = "disconnected";
	
	printf("%s %s %s ", alias, PROTO_STR[proto], st);
	proto_xxx_status(dev, phy);
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int init_proto_struct(int proto, struct proto_base *pb, int size, int idx)
{
	int rev = -1;

	switch (proto) {
	case PROTO_STATIC:
		rev = init_proto_static(pb, size, idx);
		break;
	case PROTO_DHCPC:
		rev = init_proto_dhcpc(pb, size, idx);
		break;
	case PROTO_PPPOE:
		rev = init_proto_pppoe(pb, size, idx);
		break;
	case PROTO_PPTP:
		rev = init_proto_pptp(pb, size, idx);
		break;
	case PROTO_L2TP:
		rev = init_proto_l2tp(pb, size, idx);
		break;
	case PROTO_USB3G:
		rev = init_proto_usb3g(pb, size, idx);
		break;
	}
out:
	return rev;
}
/*
 * helper: show configure from nvram by @proto
 * */
int proto_showconfig_main(int proto, int argc, char **argv)
{
	int rev = -1, i = 0;
	char *alias = NULL, *dev = NULL, *proto_str;
	char proto_alias[128], buf[512];

	if (proto >= PROTO_MAX)
		goto out;

	proto_str = PROTO_STR[proto];
	sprintf(proto_alias, "%s_alias", proto_str);
	
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1) {
		char tmp[512], *p;

		printf("miss [%s] [device]\n", proto_alias);
		foreach_nvram(i, 20, p, proto_alias, tmp, 1) {
			if (p == NULL)
				continue;
			printf("alias = %s\n", p);
		}
		goto out;
	}
	if ((i = nvram_find_index(proto_alias, alias)) == -1) {
		goto out;
	}
	init_proto_struct(proto, (struct proto_base *)buf, sizeof(buf), i);
	
	dump_proto_struct(proto, (struct proto_base *)buf , stdout);
	rev = 0;
	
out:
	return rev;
}
/*
 * This only set dns info to nvarm. not write to /etc/resolv.conf yet!
 * */
static void nameservice_attach_detach(int attach, int proto, int index)
{
	char buf[64], *p, *dns1 = NULL, *dns2 = NULL;

	if (!attach) {
		strcat_r(PROTO_STR[proto], "_dns1_actived", buf);
		nvram_set_i(buf, index, g1, "");
		strcat_r(PROTO_STR[proto], "_dns2_actived", buf);
		nvram_set_i(buf, index, g1, "");
		return;
	}
	/* xxx_primary_dns xxx_secndory_dns ? */
	/* set to xxx_actived_dns */
	strcat_r(PROTO_STR[proto], "_primary_dns", buf);
	
	/* ex: @buf: "static_primary_dns" */
	p = nvram_safe_get_i(buf, index, g1);
	if (*p != '\0') {	/* any string in @p ?, user define */
		dns1 = p;
		strcat_r(PROTO_STR[proto], "_secondary_dns", buf);
		dns2 = nvram_safe_get_i(buf, index, g1);
	} else {	
		/* Z_DNS1? Z_DNS2 ? ex: get info from dhcpc */
		dns1 = getenv("Z_DNS1");
		dns2 = getenv("Z_DNS2");
	}
	strcat_r(PROTO_STR[proto], "_dns1_actived", buf);
	nvram_set_i(buf, index, g1, dns1 == NULL ? "":dns1);
	strcat_r(PROTO_STR[proto], "_dns2_actived", buf);
	nvram_set_i(buf, index, g1, dns2 == NULL ? "":dns2); 
	return;
}

int proto_xxx_attach_detach(int attach, int proto, const char *alias)
{
	int rev = -1, i;
	char buf[128], *e;
	
	if (!proto_ok(proto))
		goto out;
	
	sprintf(buf, "%s_alias", PROTO_STR[proto]);
	if ((i = nvram_find_index(buf, alias)) == -1)
		goto out;
	if (proto != PROTO_STATIC) {
		sprintf(buf, "%s_pid", PROTO_STR[proto]);
		if (attach) {
			if ((e = getenv("Z_PID")) == NULL)
				goto out;
			nvram_set(strcat_i(buf, i, g1), e);
		} else
			nvram_set(buf, "");
	}
	nameservice_attach_detach(attach, proto, i);
	sprintf(buf, "%s_status%d", PROTO_STR[proto], i);
	nvram_set(buf, attach?"connected":"disconnected");
	rev = 0;
out:
	rd(rev, "error");
	return rev;	
}

