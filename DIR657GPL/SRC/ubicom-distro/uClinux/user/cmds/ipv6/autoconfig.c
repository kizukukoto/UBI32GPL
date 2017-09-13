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
/*Joe Huang: This mode are call from rdnssd/icmp.c.
XXX 	m bit set means get Address & DNS from DHCPv6.
	o bit set means get Address from RA & DNS from DHCPv6.
	none bit set means get Address & DNS from RA.
	autoconfiguration and PPPoe mode would use this cmd.
*/

static int autoconfig_main_mset (int argc, char *argv[])
{
	if (argc != 2){
		printf("Invalid argument : cli ipv6 autoconfig mset [wan_if]\n");
		return 0;
	}

		sleep(1); //Joe Huang : sleep a while for waiting RA
                  /*TODO : Check IPv6 routing table before starting dhcp6c*/
	_system("cli ipv6 dhcp6c start %s mset", argv[1]);
        return 0;
}

static int autoconfig_main_oset (int argc, char *argv[])
{
	if (argc != 2){
		printf("Invalid argument : cli ipv6 autoconfig oset [wan_if]\n");
		return 0;
	}
	if(NVRAM_MATCH("ipv6_wan_specify_dns", "0") || NVRAM_MATCH("ipv6_dhcp_pd_enable", "1"))
		_system("cli ipv6 dhcp6c start %s oset", argv[1]);
	return 0;
}

static int autoconfig_main_noneset (int argc, char *argv[])
{
	if (argc != 2){
		printf("Invalid argument : cli ipv6 autoconfig noneset [wan_if]\n");
		return 0;
	}

	if(NVRAM_MATCH("ipv6_dhcp_pd_enable","1"))
		_system("cli ipv6 dhcp6c start %s noneset", argv[1]);
	return 0;
}

static int autoconfig_main_stop (int argc, char *argv[])
{
	char* wan_eth = nvram_safe_get("wan_eth");
	char* lan_bridge = nvram_safe_get("lan_bridge");

	if(NVRAM_MATCH("ipv6_wan_proto", "ipv6_pppoe")){
		if(NVRAM_MATCH("ipv6_pppoe_share", "1"))
			wan_eth = "ppp0";
		if(NVRAM_MATCH("ipv6_pppoe_share", "0"))
			wan_eth = "ppp6";
	}
        _system("ip -6 addr flush scope global dev %s",wan_eth);
	system("ip -6 addr flush scope global dev ath1");
        _system("ip -6 route flush root 2000::/3 dev %s", wan_eth);
        _system("ip -6 route flush root 2000::/3 dev %s", lan_bridge);
        system("ip -6 route flush root 2000::/3 dev lo");
        _system("ip -6 route flush proto kernel dev %s",wan_eth);
	_system("ip -6 neigh flush dev %s", lan_bridge);

	system("cli ipv6 dhcp6c stop"); //stop dhcp6c would stop radvd and dhcp6d too

	/*XXX Joe Huang : If dhcp-pd is disable, should not clean the static lan address.*/
	if(NVRAM_MATCH("ipv6_dhcp_pd_enable","1"))
       		_system("ip -6 addr flush scope global dev %s",lan_bridge);
	
	return 0;
}

int autoconfig_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "mset", autoconfig_main_mset, "start autoconfiguration mode with m bit set"},
                { "oset", autoconfig_main_oset, "start autoconfiguration mode with o bit set"},
		{ "noneset", autoconfig_main_noneset, "start autoconfiguration mode with no bit set"},
		{ "stop", autoconfig_main_stop, "stop autoconfiguration mode"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
