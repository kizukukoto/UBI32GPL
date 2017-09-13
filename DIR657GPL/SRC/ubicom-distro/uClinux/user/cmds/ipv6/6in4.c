#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"
static int v6in4_main_start (int argc, char *argv[])
{
        char *wan_if;
        char wan_ipv4_address[20]={};

        if (access(SIT1_PID, F_OK) == 0)
        {
                printf("sit1.pid is already active !\n");
                return 0;
        } else
                fclose(fopen(SIT1_PID, "w+"));
	
        if(NVRAM_MATCH("wan_proto","pppoe") || 
		NVRAM_MATCH("wan_proto","pptp") || 
		NVRAM_MATCH("wan_proto","l2tp"))
                wan_if = "ppp0";
        else
                wan_if = nvram_safe_get("wan_eth");

	getIPbyDevice(wan_if, wan_ipv4_address);

        if(strlen(wan_ipv4_address) != 0){
		nvram_set("ipv4_6in4_wan_ip", wan_ipv4_address);
	        _system("ip tunnel add sit1 remote %s local %s ttl 64", 
			nvram_safe_get("ipv4_6in4_remote_ip"), 
			nvram_safe_get("ipv4_6in4_wan_ip"));
        	_system("ip link set dev sit1 up");
	        _system("ip -6 addr add %s/64 dev sit1", nvram_safe_get("ipv6_6in4_wan_ip"));
        	_system("route -A inet6 add ::/0 gw %s dev sit1 metric 256", 
			nvram_safe_get("ipv6_6in4_remote_ip"));
	}
	else 
		unlink(SIT1_PID);
	return 0;
}
static int v6in4_main_stop (int argc, char *argv[])
{
        _system("ip link set dev sit1 down");
        _system("ip tunnel del sit1");
	unlink(SIT1_PID);
	return 0;
}

int v6in4_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", v6in4_main_start, "start 6 over 4 tunnel"},
                { "stop", v6in4_main_stop, "stop 6 over 4 tunnel"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
