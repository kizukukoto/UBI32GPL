#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "nvram_ext.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

void each_nvram_set(int max, char *key, char *val)
{
	int i = 0;
	char *p;
	
	foreach_nvram(i, max, p, key, g1, 1) {
		if (p == NULL)
			continue;
		nvram_set_i(key, i, g1, val);
	}
}

int net_restore_main()
{
	each_nvram_set(10, "zone_if_attached", "");
	each_nvram_set(10, "if_stop", NULL);
	each_nvram_set(10, "if_services_top", NULL);
	each_nvram_set(10, "dhcp_pid", "");
	each_nvram_set(10, "pppoe_pid", "");
	each_nvram_set(10, "dhcpc_addr", "");
	each_nvram_set(10, "dhcpc_netmask", "");
	each_nvram_set(99, "forward_port_stop", NULL);
	each_nvram_set(99, "adv_firewall_stop", NULL);
}

extern int zone_main(int argc, char *argv[]);
extern int dhcpc_main(int argc, char *argv[]);
extern int static_main(int argc, char *argv[]);
extern int dns_main(int argc, char *argv[]);
extern int pppoe_main(int argc, char *argv[]);
extern int ii_main(int argc, char *argv[]);
extern int proto_main(int argc, char *argv[]);
extern int wl_main(int argc, char *argv[]);
extern int group_main(int argc, char *argv[]);
int net_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
	//	{ "zone", zone_main, "interfaces grouping to zone"},
		{ "dhcpc", dhcpc_main, "interface up/down by dhcpc client, not use directly!"},
		{ "static", static_main, "interace up/down by static ip, not use directly!"},
		{ "pppoe", pppoe_main, "interace up/down by pppoe, not use directly!"},
		{ "dns", dns_main, "Domain servies: dns resolve subsystem by /etc/resolve.conf"},
		{ "ii", ii_main, "interfaces manipulater for static/dhcpc/pppXXX"},
		{ "device", ii_main, "interfaces manipulater for static/dhcpc/pppXXX, same as ii"},
		{ "proto", proto_main},
		{ "group", group_main},
		{ "wl", wl_main},		
		{ NULL, NULL}
	};
	int rev;
	
	lock_prog(argc, argv, 0);
	rev = lookup_subcmd_then_callout(argc, argv, cmds);
	unlock_prog();
	return rev;
}
