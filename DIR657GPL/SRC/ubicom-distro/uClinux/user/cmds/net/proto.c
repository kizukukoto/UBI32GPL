#include <stdio.h>
#include <stdlib.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "proto.h"
#include "interface.h"
#include "libcameo.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

struct proto_proto {
	char *proto;
	int (*call_main)(int argc, char *argv[]);
};
struct proto_proto *
lookup_proto_proto(struct proto_proto *pp, const char *proto)
{
	DEBUG_MSG("lookup_proto_proto: looking for proto=%s\n",proto);
	for (;pp->proto != NULL; pp++) {
		DEBUG_MSG("lookup_proto_proto: current pp->proto=%s\n",pp->proto);
		if (strcmp(pp->proto, proto) != 0)
			continue;
		DEBUG_MSG("%s: found matched proto=%s, address at %p\n",__FUNCTION__,pp->proto,pp ? pp : "NULL_Address");
		return pp;
	}
	debug( "No proto: %s in this method", proto);
	return NULL;
}

extern int dhcpc_start_main(int argc, char *argv[]);
extern int static_start_main(int argc, char *argv[]);
extern int pppoe_start_main(int argc, char *argv[]);
extern int pptp_start_main(int argc, char *argv[]);
extern int l2tp_start_main(int argc, char *argv[]);
extern int usb3g_start_main(int argc, char *argv[]);
static struct proto_proto proto_start[] = {
	{ PROTO_DHCPC_STR, dhcpc_start_main},
	{ PROTO_STATIC_STR, static_start_main},
	{ PROTO_PPPOE_STR, pppoe_start_main},
	{ PROTO_PPTP_STR, pptp_start_main},
	{ PROTO_L2TP_STR, l2tp_start_main},
	{ PROTO_USB3G_STR, usb3g_start_main},
	{ NULL, NULL},
};

extern int static_stop_main(int argc, char *argv[]);
extern int dhcpc_stop_main(int argc, char *argv[]);
extern int pppoe_stop_main(int argc, char *argv[]);
extern int pptp_stop_main(int argc, char *argv[]);
extern int l2tp_stop_main(int argc, char *argv[]);
extern int usb3g_stop_main(int argc, char *argv[]);

static struct proto_proto proto_stop[] = {
	{ PROTO_DHCPC_STR, dhcpc_stop_main},
	{ PROTO_STATIC_STR, static_stop_main},
	{ PROTO_PPPOE_STR, pppoe_stop_main},
	{ PROTO_PPTP_STR, pptp_stop_main},
	{ PROTO_L2TP_STR, l2tp_stop_main},
	{ PROTO_USB3G_STR, usb3g_stop_main},
	{ NULL, NULL},
};

extern int static_attach_main(int argc, char *argv[]);
extern int dhcpc_attach_main(int argc, char *argv[]);
extern int pppoe_attach_main(int argc, char *argv[]);
extern int pptp_attach_main(int argc, char *argv[]);
extern int l2tp_attach_main(int argc, char *argv[]);
extern int usb3g_attach_main(int argc, char *argv[]);
static struct proto_proto proto_attach[] = {
	{ PROTO_STATIC_STR, static_attach_main},
	{ PROTO_DHCPC_STR, dhcpc_attach_main},
	{ PROTO_PPPOE_STR, pppoe_attach_main},
	{ PROTO_PPTP_STR, pptp_attach_main},
	{ PROTO_L2TP_STR, l2tp_attach_main},
	{ PROTO_USB3G_STR, usb3g_attach_main},
	{ NULL, NULL}
};
extern int static_detach_main(int argc, char *argv[]);
extern int dhcpc_detach_main(int argc, char *argv[]);
extern int pppoe_detach_main(int argc, char *argv[]);
extern int pptp_detach_main(int argc, char *argv[]);
extern int l2tp_detach_main(int argc, char *argv[]);
extern int usb3g_detach_main(int argc, char *argv[]);
static struct proto_proto proto_detach[] = {
	{ PROTO_STATIC_STR, static_detach_main},
	{ PROTO_DHCPC_STR, dhcpc_detach_main},
	{ PROTO_PPPOE_STR, pppoe_detach_main},
	{ PROTO_PPTP_STR, pptp_detach_main},
	{ PROTO_L2TP_STR, l2tp_detach_main},
	{ PROTO_USB3G_STR, usb3g_detach_main},
	{ NULL, NULL}
};

extern int static_status_main(int argc, char *argv[]);
extern int dhcpc_status_main(int argc, char *argv[]);
extern int pppoe_status_main(int argc, char *argv[]);
extern int pptp_status_main(int argc, char *argv[]);
extern int l2tp_status_main(int argc, char *argv[]);
extern int usb3g_status_main(int argc, char *argv[]);

static struct proto_proto proto_status[] = {
	{ PROTO_STATIC_STR, static_status_main},
	{ PROTO_DHCPC_STR, dhcpc_status_main},
	{ PROTO_PPPOE_STR, pppoe_status_main},
	{ PROTO_PPTP_STR, pptp_status_main},
	{ PROTO_L2TP_STR, l2tp_status_main},
	{ PROTO_USB3G_STR, usb3g_status_main},
	{ NULL, NULL}
};

static struct protos_ops {
	char *action;
	struct proto_proto *proto_action;
} protos_ops[] = {
	{ "start", proto_start},
	{ "stop", proto_stop},
	{ "attach", proto_attach},
	{ "status", proto_status},
	{ "detach", proto_detach},
	{ NULL, NULL}
};

/*
 * Pass @proto, @proto_alias, @dev to protocol operators.
 * */
int proto_operator_main(int argc, char *argv[])
{
	struct protos_ops *p;
	struct proto_proto *pp;
	char *alias, *dev, *proto, *phy, *act;
	int rev = -1;
	
	act = argv[0];
	args(argc - 1, argv + 1, &alias, &dev, &proto, &phy);
	if (alias == NULL || dev == NULL || proto == NULL)
		goto out;
	
	
	for (p = protos_ops; p->action != NULL;p++) {
		if (strcmp(p->action, act) != 0)
			continue;
		//debug("proto_operator_main: p->action=%s\n", p->action);
		if ((pp = lookup_proto_proto(p->proto_action, proto)) == NULL){
			err_msg( ": lookup_proto_proto() fail!");
			goto out;
		}
		//debug( ": call %s->%s %s %s %s", proto, act, alias, dev, phy);
		
		if (pp->call_main) {
			rev = pp->call_main(phy == NULL ? 4 : 5,
				(char *[]){ act, alias, dev, phy});
			rd(rev, "error return from :%s->%s %s %s %s, %d",
					proto, act, alias, dev, phy, rev);
		} else
			err_msg( "%s->%s not implement!", proto, act);
			
		break;
	}
out:
	rd(rev, "#####alias:%s, dev:%s, proto:%s phy:%s####\n",
			alias, dev, proto, phy ? phy : "");
	return rev;
}

static proto_show_main(int argc, char *argv[])
{
	proto_showconfig_main(PROTO_DHCPC, argc, argv);
	proto_showconfig_main(PROTO_STATIC, argc, argv);
	proto_showconfig_main(PROTO_PPPOE, argc, argv);
	proto_showconfig_main(PROTO_PPTP, argc, argv);
	proto_showconfig_main(PROTO_L2TP, argc, argv);
	proto_showconfig_main(PROTO_USB3G, argc, argv);
}

int proto_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", proto_operator_main}, /* @proto @proto_alias @dev */
		{ "stop", proto_operator_main},
		{ "status", proto_operator_main},
		{ "showconfig", proto_show_main},
		{ "attach", proto_operator_main},
		{ "release", NULL},
		{ "renew", NULL},
		{ "deconfig", NULL},
		{ NULL, NULL}
	};
	DEBUG_MSG("%s, Begin, args :\n", __FUNCTION__);
	{
		int i = 0;
		for(i = 0; i < argc ; i++){
			DEBUG_MSG("%s ", argv[i] ? argv[i] : "");
		}
	}

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
