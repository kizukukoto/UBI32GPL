#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmds.h"
#include "shutils.h"
#include "ipv6.h"
extern int dhcp6c_main(int argc, char *argv[]);
extern int dhcp6d_main(int argc, char *argv[]);
extern int rdnssd_main(int argc, char *argv[]);
extern int v6to4_main(int argc, char *argv[]);
extern int v6in4_main(int argc, char *argv[]);
extern int v6rd_main(int argc, char *argv[]);
extern int pppoe_main(int argc, char *argv[]);
extern int autoconfig_main(int argc, char *argv[]);
extern int firewall_main(int argc, char *argv[]);
extern int ipv6_start_main(int argc, char *argv[]);
extern int ipv6_stop_main(int argc, char *argv[]);
extern int radvd_main(int argc, char *argv[]);
extern int clientlist_main(int argc, char *argv[]);
extern int route_main(int argc, char *argv[]);
extern int wizard_main(int argc, char *argv[]);

/*XXX Joe Huang : The code in cmds/ipv6 was from rc 						*/
/*There are still some ipv6 code in rc, because these code should run in a daemond		*/
/*I made a daemond (ex.dhcp6c) and a mode that can product a new interface (ex.6in4) be a cmd	*/
/*rc start -> cli ipv6 start -> configure according to the different modes			*/

int ipv6_subcmd(int argc, char *argv[])
{
        struct subcmd cmds[] = {
		{ "dhcp6c", dhcp6c_main, "dhcp6c command"},
		{ "dhcp6d", dhcp6d_main, "dhcp6s command"},
		{ "rdnssd", rdnssd_main, "rdnssd command"},
		{ "radvd", radvd_main, "radvd command"},
		{ "6to4", v6to4_main, "6 to 4 mode"},
		{ "6in4", v6in4_main, "6 in 4 mode"},
		{ "6rd", v6rd_main, "6RD mode"},
		{ "pppoe", pppoe_main, "PPPoE mode"},
		{ "autoconfig", autoconfig_main, "Autoconfiguration mode"},
		{ "firewall", firewall_main, "IPv6 Firewall"},
		{ "clientlist", clientlist_main, "Create IPv6 client list"},
		{ "route", route_main, "IPv6 Static Route"},
		{ "start", ipv6_start_main, "Start IPv6 setting"},
		{ "stop", ipv6_stop_main, "Clean IPv6 configuration"},
		{ "wizard", wizard_main, "wizard command"},
		{ NULL, NULL}
        };
        return lookup_subcmd_then_callout(argc, argv, cmds);
}

