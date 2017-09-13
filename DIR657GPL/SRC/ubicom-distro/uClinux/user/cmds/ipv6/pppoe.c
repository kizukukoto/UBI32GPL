#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
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

int start_ipv6_redial(char *ipv6_wan_proto, char *wan_eth)
{
        if(strncmp(ipv6_wan_proto, "ipv6_pppoe", 10) == 0)
        {
                _system("/var/sbin/red6 5 %s %s &",
                	ipv6_wan_proto, wan_eth);
        }
        return 0;
}

int stop_ipv6_redial(void)
{
        _system("killall -9 red6"); 
        unlink(RED6_PID);
        return 0;
}

int start_ipv6_monitor( char *ipv6_wan_proto, char *wan_eth)
{
        char ipv6_wan_info[128] = {0};
        char ipv6_ppp_info[64] = {0};
        int  dial_on_demand_mode = 0;
        char ipv6_lan_lla[MAX_IPV6_IP_LEN]={};

        read_ipv6addr(nvram_safe_get("lan_bridge"), 
		SCOPE_LOCAL, ipv6_lan_lla, sizeof(ipv6_lan_lla));
        sprintf(ipv6_wan_info, "%s/%s/%s/%s", ipv6_wan_proto, wan_eth,
        strtok(ipv6_lan_lla,"/"), nvram_safe_get("lan_bridge"));

        dial_on_demand_mode = 1;

        sprintf(ipv6_ppp_info, "%d", dial_on_demand_mode);
        _system("/var/sbin/mon6 5 %s %s &",
                ipv6_wan_info, ipv6_ppp_info);

        return 0;
}

int stop_ipv6_monitor(void)
{
        _system("killall -9 mon6");
        unlink(MON6_PID);
        return 0;
}

static int pppoe_main_start (int argc, char *argv[])
{
        char *ipv6_wan_proto = NULL;
	char *wan_eth = NULL;
	char *ipv6_pppoe_connect_mode = NULL;
	if(NVRAM_MATCH("ipv6_pppoe_share", "0"))
	{
        	ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");

		wan_eth = nvram_safe_get("wan_eth");
		ipv6_pppoe_connect_mode = nvram_safe_get("ipv6_pppoe_connect_mode");

		if( strcmp(ipv6_pppoe_connect_mode, "always_on") == 0 )
		{
			start_ipv6_redial(ipv6_wan_proto, wan_eth);
			printf("IPv6 PPPoE : Always On Mode\n");
		}
		else if( strcmp(ipv6_pppoe_connect_mode, "on_demand") == 0 )
		{
			start_ipv6_monitor(ipv6_wan_proto, wan_eth);
			printf("IPv6 PPPoE : On Demand Mode\n");
		}
		else //manual mode
		{
			printf("IPv6 PPPoE : Manual Mode\n");
		}
	}
	return 0;
}

static int pppoe_main_stop (int argc, char *argv[])
{
	system("cli ipv6 pppoe disdial");
	stop_ipv6_redial();
	stop_ipv6_monitor();
	return 0;
}

static int pppoe_main_dial (int argc, char *argv[])
{
/*XXX Joe Huang : When pppd6 dial successful, pppd6 would start rdnssd.*/
	char *ipv6_service_name;
	char ipv6_sn[50] = {};

        if (access(PPP6_PID, F_OK) == 0 ) {
                cprintf("IPv6 PPPoE is already connecting\n");
                return 0;
        } else {
                fclose(fopen(PPP6_PID, "w+"));
        }

        write_ipv6_pppoe_sercets(PAP_SECRETS_V6);
        write_ipv6_pppoe_sercets(CHAP_SECRETS_V6);
        write_ipv6_pppoe_options();

        ipv6_service_name = nvram_safe_get("ipv6_pppoe_service");

        if(strcmp(ipv6_service_name, "") == 0)
                strncpy(ipv6_sn, "null_sn", strlen("null_sn"));
        else
                strncpy(ipv6_sn, ipv6_service_name, strlen(ipv6_service_name));

        parse_special_char(ipv6_sn);

        symlink("/sbin/pppd","/var/sbin/pppd6");

	cprintf("IPv6 PPPoE : Dial\n");
	if( (strcmp(ipv6_sn, "null_sn") == 0)){
		_system("/var/sbin/pppd6 unit 6 linkname 06 file %s %s &",
			IPv6PPPOE_OPTIONS_FILE, nvram_safe_get("wan_eth"));
	}
	else{
		_system("/var/sbin/pppd6 unit 6 linkname 06 file %s rp_pppoe_service \"%s\" %s &",
			IPv6PPPOE_OPTIONS_FILE, ipv6_sn, nvram_safe_get("wan_eth"));
	}

        return 0;
}

static void pppoe_main_disdial (int argc, char *argv[])
{
	char *connect_mode = nvram_safe_get("ipv6_pppoe_connect_mode");
	char *share = nvram_safe_get("ipv6_pppoe_share");
	cprintf("IPv6 PPPoE : Disdial\n");
	system("cli ipv6 autoconfig stop");
	system("cli ipv6 rdnssd stop");
        system("killall pppd6");
        unlink(PPP6_PID);
	/*XXX Joe Huang : on demand mode should restart radvd for users gateway*/
	if(strcmp(connect_mode, "on_demand") == 0 && strcmp(share, "0") == 0 ){
		if(access(RADVD_PID, F_OK) == 0)
		        system("cli ipv6 radvd stop");

        	system("cli ipv6 radvd start");
        }

}

int pppoe_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", pppoe_main_start, "start IPv6 PPPoE"},
                { "stop", pppoe_main_stop, "stop IPv6 PPPoE"},
		{ "dial", pppoe_main_dial, "dial IPv6 PPPoE"},
		{ "disdial", pppoe_main_disdial, "disdial IPv6 PPPoE"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
