#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"

static unsigned long pow(int m, int n)
{
	if (m == 1)
		return n;
	else if (m == 0)
		return 1;
	return pow(m - 1, n) * n ;
}

static int dhcp6c_main_start (int argc, char *argv[])
{
	char *ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");
	char *dhcp6c_if;
	int is_ipv6_dhcp_pd = (nvram_match("ipv6_dhcp_pd_enable", "1") == 0 ? 1 : 0);
        unsigned long iaid = 0;
        char *mac, *lan_bridge;
        char iaid_str[9] = {0};
        int i;
        static int j = 0;
        char str_buf[200];
	char *ipv6_autoconfig_flag = NULL;

	if (argc != 3){
		printf("Invalid argument : cli ipv6 dhcp6c start [wan_if] [mset/oset/noneset]\n");
		return 0;
	}
	dhcp6c_if = argv[1];
	ipv6_autoconfig_flag = argv[2];

        if (access(DHCP6C_PID, F_OK) == 0) {
                printf("dhcp6c is already active !\n");
                return 0;
        }
	printf("Start IPv6 dhclient\n");

	lan_bridge = nvram_safe_get("lan_bridge");
              /*
               * produce IAID from WAN MAC(use last 7 chars)
               */
	mac = nvram_safe_get("wan_mac");
	for (i = 7; i < 17; i++){
		if (mac[i] != 0x3A){ // escape :
			iaid_str[j] = mac[i];
			j++;
		}
	}
	j = 0;
	for(i = 0; i < 7; i++){
	unsigned long var = 0;
	if (iaid_str[i] >= 0x30 && iaid_str[i] <= 0x39) //0-9
		var = iaid_str[i] - 0x30;
	else if (iaid_str[i] >= 0x41 && iaid_str[i] <= 0x46) //A-F
		var = 10 + (iaid_str[i] - 0x41);
	else if (iaid_str[i] >= 0x61 && iaid_str[i] <= 0x66) //a-f
		var = 10 + (iaid_str[i] - 0x61);
		var = var * pow(6 - i, 16);
		iaid += var;
	}
	init_file(DHCP6C_CONF_FILE);
	save2file(DHCP6C_CONF_FILE, "interface %s {\n", dhcp6c_if );

	if (strcmp(ipv6_autoconfig_flag, "mset") == 0)
		save2file(DHCP6C_CONF_FILE, "send ia-na %d;\n", iaid);

	if (is_ipv6_dhcp_pd)
		save2file(DHCP6C_CONF_FILE, "send ia-pd %d;\n", iaid);

	if(NVRAM_MATCH("wan_proto", "dslite") && NVRAM_MATCH("wan_dslite_dhcp", "1"))
		save2file(DHCP6C_CONF_FILE, "request ds-lite;\n");

	if (NVRAM_MATCH("ipv6_wan_specify_dns", "0") && strcmp(ipv6_autoconfig_flag, "noneset") != 0) {
		save2file(DHCP6C_CONF_FILE, "request domain-name-servers;\n");
		save2file(DHCP6C_CONF_FILE, "request domain-name;\n");
	}

	save2file(DHCP6C_CONF_FILE, "script \"%s\";\n", DHCP6C_SCRIPT_FILE);
	save2file(DHCP6C_CONF_FILE, "};\n");

	if (strcmp(ipv6_autoconfig_flag, "mset") == 0)
		save2file(DHCP6C_CONF_FILE, "id-assoc na %d { };\n", iaid);

	if (is_ipv6_dhcp_pd) {
		save2file(DHCP6C_CONF_FILE, "id-assoc pd %d {", iaid);
		save2file(DHCP6C_CONF_FILE, "  prefix-interface %s {", lan_bridge);
		save2file(DHCP6C_CONF_FILE, "    sla-id 1;");
		save2file(DHCP6C_CONF_FILE, "  };");
		save2file(DHCP6C_CONF_FILE, "};");
		strcpy(str_buf, ipv6_wan_proto);
		strcat(str_buf, "_lan_ip");
		nvram_set(str_buf, "");
		sprintf(str_buf, "ip -6 addr flush scope global dev %s; "
			"ip -6 route flush root 2000::/3 dev %s", lan_bridge, lan_bridge);
		system(str_buf);
	}

// 2011.03.22
	if( strcmp(nvram_safe_get("tr069_enable"),"1")==0) {
		//_system("dhcp6c -z -f -c %s %s %s &", DHCP6C_CONF_FILE, dhcp6c_if, dhcp6c_if);
	        _system("dhcp6c -z -f -c %s %s %s &", DHCP6C_CONF_FILE, nvram_safe_get("wan_eth"), dhcp6c_if);
	}
	else
	/*XXX Joe Huang:The first argv should be wan eth, use for getting wan mac */
	_system("dhcp6c -f -c %s %s %s &", DHCP6C_CONF_FILE, nvram_safe_get("wan_eth"), dhcp6c_if);

	return 0;
}

static int dhcp6c_main_stop (int argc, char *argv[])
{
	printf("Stop IPv6 dhclient\n");
	system ("killall dhcp6c &");
        if (access(DHCP6C_PID, F_OK) == 0)
                system("killall -9 dhcp6c");
                unlink(DHCP6C_PID);

	// rename DHCP_PD to DHCP_PD_OLD for release DHCP_PD
//	rename(DHCP_PD, DHCP_PD_OLD);
	unlink(DHCP_PD);

        /*
	XXX by Joe Huang
	If enable dhcp-pd, stop dhcp6c alos shoulde stop radvd because pd is disappearing
        At ipv6_pppoe on demand mode, radvd should NOT killed
        because user need default route in this mode.
	*/
        if ( NVRAM_MATCH("ipv6_dhcp_pd_enable","1") && (
                !NVRAM_MATCH("ipv6_wan_proto","ipv6_static") ||
                !NVRAM_MATCH("ipv6_wan_proto","ipv6_6rd") ||
                !NVRAM_MATCH("ipv6_wan_proto","ipv6_6to4")) &&
                (!NVRAM_MATCH("ipv6_wan_proto","ipv6_pppoe") ||
                !NVRAM_MATCH("ipv6_pppoe_connect_mode","on_demand") ||
		!NVRAM_MATCH("ipv6_pppoe_share","0")))
        {
                system("cli ipv6 radvd stop");
                system("cli ipv6 dhcp6d stop");
        }

	return 0;
}

int dhcp6c_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", dhcp6c_main_start, "start DHCP6c"},
		{ "stop", dhcp6c_main_stop, "stop DHCP6c"},
		{ NULL, NULL}
	};
	int rev;
	rev = lookup_subcmd_then_callout(argc, argv, cmds);
	return rev;
}
