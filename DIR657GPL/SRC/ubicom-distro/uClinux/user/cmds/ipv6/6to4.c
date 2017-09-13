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
#include <netdb.h>
#include "cmds.h"
#include "shutils.h"
#include "netinf.h"
#include <nvramcmd.h>
#include "ipv6.h"

/*Use in app.c*/
/*When IPv4 address changed would do start_app*/
static int v6to4_main_diagnose (int argc, char *argv[])
{
        char wan_ipv4_address[16] = {};
        char ip_token_1[8] = {};
        char ip_token_2[8] = {};
        char ip_token_3[8] = {};
        char ip_token_4[8] = {};
        char ipv6_nvram_lan_ip[64] = {};
        char ipv6_6to4_lan_ip[64] = {};
        char *wan_eth = NULL;
        char *instr = "";    //insert pointer
        char *index = "";
        char *next;
        int colon=0,i;

	if(!NVRAM_MATCH("ipv6_wan_proto","ipv6_6to4")){
                return 0;
        }

        if(NVRAM_MATCH("wan_proto","pppoe") || 
		NVRAM_MATCH("wan_proto","pptp") || 
		NVRAM_MATCH("wan_proto","l2tp"))
                wan_eth = "ppp0";
        else
                wan_eth = nvram_safe_get("wan_eth");

        // Make 6to4 tunnel change Prefix automatically
        if(get_ip(wan_eth, wan_ipv4_address) == -1)
                strcpy(wan_ipv4_address,"0.0.0.0");
        strcpy(ip_token_1,strtok(wan_ipv4_address,"."));
        strcpy(ip_token_2,strtok(NULL,"."));
        strcpy(ip_token_3,strtok(NULL,"."));
        strcpy(ip_token_4,strtok(NULL,"."));
        //Get suffix from old ipv6_6to4_lan_ip
	strcpy(ipv6_nvram_lan_ip, nvram_safe_get("ipv6_6to4_lan_ip"));

        index = ipv6_nvram_lan_ip;
        while((index = strstr(index, ":")))
        {
                ++colon;
                if(next != NULL)
                {
                        if(index-next ==1) instr = next;
                }
                next = index;
                index++;
        }

        if(colon !=7){
                for(i=0; i< 8-colon; i++){
                        if(i==0) string_insert(instr,"0", 1);
                        else string_insert(instr,"0:", 1);
                }
        }

        strtok(ipv6_nvram_lan_ip,":");
        strtok(NULL,":");
        strtok(NULL,":");
        sprintf(ipv6_6to4_lan_ip,"2002:%02x%02x:%02x%02x:%s",
		atoi(ip_token_1),atoi(ip_token_2),atoi(ip_token_3),
		atoi(ip_token_4),strtok(NULL,"\0"));
	//Remove old lan ip while ip lease time out
	_system("ip -6 addr flush scope global dev %s", nvram_safe_get("lan_bridge"));
        _system("ip -6 addr add %s/64 dev %s", ipv6_6to4_lan_ip, nvram_safe_get("lan_bridge"));
	nvram_set("ipv6_6to4_lan_ip", ipv6_6to4_lan_ip);
/*XXX Joe Huang : using busybox httpd do not need synchronize*/
#ifndef DIR652V 
        _system("killall -SIGUSR2 httpd");
#endif
        return 0;
}

static int v6to4_main_start (int argc, char *argv[])
{
        char *wan_if;
        char wan_ipv4_address[20]={};
        char ip_token_1[8] = {};
        char ip_token_2[8] = {};
        char ip_token_3[8] = {};
        char ip_token_4[8] = {};
        char ipv6_6to4_prefix[32] = {};
	char *relay_ptr;
	struct in_addr v4addr_in;
	char relay_addr[200] = {};

        if (access(TUN6TO4_PID, F_OK) == 0)
        {
                printf("tun6to4.pid is already active !\n");
                return 0;
        } else
                fclose(fopen(TUN6TO4_PID, "w+"));          // create empty TUN6TO4_PID file

        if(NVRAM_MATCH("wan_proto","pppoe") || 
		NVRAM_MATCH("wan_proto","pptp") || 
		NVRAM_MATCH("wan_proto","l2tp"))
                wan_if = "ppp0";
        else
                wan_if = nvram_safe_get("wan_eth");

	relay_ptr = nvram_safe_get("ipv6_6to4_relay");

	if (inet_aton(relay_ptr, &v4addr_in))
		strcpy(relay_addr, relay_ptr);
	else{
		struct hostent *relay_h = NULL;
		if (relay_h = gethostbyname(relay_ptr))
			strcpy(relay_addr, inet_ntoa(*(struct in_addr *)(relay_h->h_addr)));
	}

        if(get_ip(wan_if,wan_ipv4_address) == 0)
        {
                _system("ip tunnel add tun6to4 mode sit ttl 64 remote any local %s",
			wan_ipv4_address);
                _system("ip link set dev tun6to4 up");
                strcpy(ip_token_1,strtok(wan_ipv4_address,"."));
                strcpy(ip_token_2,strtok(NULL,"."));
                strcpy(ip_token_3,strtok(NULL,"."));
                strcpy(ip_token_4,strtok(NULL,"."));
                sprintf(ipv6_6to4_prefix,"%02x%02x:%02x%02x",
			atoi(ip_token_1),atoi(ip_token_2),atoi(ip_token_3),atoi(ip_token_4));
                _system("ip -6 addr add 2002:%s::%s/16 dev tun6to4",
			ipv6_6to4_prefix,ipv6_6to4_prefix);
                _system("ip -6 route add ::/0 via ::%s dev tun6to4 metric 1", relay_addr);
        }
	else
		unlink(TUN6TO4_PID);

	return 0;
}
static int v6to4_main_stop (int argc, char *argv[])
{
        _system("ip link set dev tun6to4 down");
        _system("ip tunnel del tun6to4");
	unlink(TUN6TO4_PID);
	return 0;
}

int v6to4_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", v6to4_main_start, "start 6to4 tunnel"},
                { "stop", v6to4_main_stop, "stop 6to4 tunnel"},
		{ "diagnose", v6to4_main_diagnose, "Change 6to4 LAN IP base on IPv4 current IP"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
