#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"


static int radvd_main_stop (int argc, char *argv[])
{
        printf("Stop radvd\n");
	system("ip -6 addr flush scope global dev ath1");
        system ("killall radvd");
        return 0;
}

static int radvd_main_start (int argc, char *argv[])
{
        char radvd_prefix[32] = {};
        char tmp_buf[128] = {};
        char *ipv6_wan_proto=nvram_safe_get("ipv6_wan_proto");
        char ipv6_wan_proto_lan_ip[40];
	char lan_ipv6_addr[50]={};
	char *tmp;
        int pd_len = 128;
        char buffer[128] = {};
        char guest_prefix[64] = {};
	char guest_ipaddr[50]={};
        struct in6_addr guest_ip;
        FILE *fp_dhcppd;
        struct in6_addr ifid;
	char guest_ifid[50]={};

        fp_dhcppd = fopen ( DHCP_PD , "r" );
        if ( fp_dhcppd ){
                fgets(buffer, sizeof(buffer), fp_dhcppd);
                if(fgets(buffer, sizeof(buffer), fp_dhcppd))
                        buffer[strlen(buffer)-1] = '\0'; /*replace '\n'*/
                pd_len = atoi(buffer);
                fclose(fp_dhcppd);
        }

	if(NVRAM_MATCH("ipv6_wan_proto","link_local"))
		return 0;

        strcpy(ipv6_wan_proto_lan_ip, ipv6_wan_proto);
        strcat(ipv6_wan_proto_lan_ip, "_lan_ip");

        if(NVRAM_MATCH("ipv6_autoconfig_type","stateful")){
                nvram_set("ipv6_ra_adv_managed_flag_l","on");
                nvram_set("ipv6_ra_adv_autonomous_flag_l_one","off");
        }else {
                nvram_set("ipv6_ra_adv_managed_flag_l","off");
                nvram_set("ipv6_ra_adv_autonomous_flag_l_one","on");
        }

	if ( read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, lan_ipv6_addr, sizeof(lan_ipv6_addr)) ) {
        	if ( tmp = strchr(lan_ipv6_addr, '/') )
	        	*tmp = '\0';
		strcpy(tmp_buf, lan_ipv6_addr);
		nvram_set(ipv6_wan_proto_lan_ip, lan_ipv6_addr);
	} else perror("not have lan ipv6 global address");

	if(pd_len < 64 && !read_ipv6addr("ath1", SCOPE_GLOBAL, guest_ifid, sizeof(guest_ifid)) && ifconfirm("ath1")){
		read_ipv6addr("ath1", SCOPE_LOCAL, guest_ifid, sizeof(guest_ifid));
        	if (tmp = strchr(guest_ifid, '/'))
	        	*tmp = '\0';
		inet_pton(AF_INET6, guest_ifid, &ifid);
		inet_pton(AF_INET6, lan_ipv6_addr, &guest_ip);
		guest_ip.s6_addr[7] -= 1;
		guest_ip.s6_addr32[2] = ifid.s6_addr32[2];
		guest_ip.s6_addr32[3] = ifid.s6_addr32[3];
		inet_ntop(AF_INET6, &guest_ip, guest_ipaddr, sizeof(guest_ipaddr));
		_system("ip -6 addr add %s/64 dev ath1", guest_ipaddr);
		nvram_set("ipv6_guest_ra_prefix64_l_one", get_ipv6_prefix(guest_ipaddr,64));
	}

        if(strlen(tmp_buf) != 0){
                strcpy(radvd_prefix,get_ipv6_prefix(tmp_buf,64));
	}
        else if (strcmp(ipv6_wan_proto, "ipv6_pppoe") == 0 &&
		NVRAM_MATCH("ipv6_pppoe_share", "0") &&
		NVRAM_MATCH("ipv6_pppoe_connect_mode", "on_demand")) {
		strcpy(radvd_prefix, "fe80");
        }else
		return 0;

        if(strlen(radvd_prefix) != 0){
               nvram_set("ipv6_ra_prefix64_l_one",radvd_prefix);
	}

	if (NVRAM_MATCH("ipv6_autoconfig","1")){
		write_radvd_conf(pd_len);
		_system("radvd -C %s &",RADVD_CONF_FILE);
	}

        return 0;
}

int radvd_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", radvd_main_start, "start RADVD"},
                { "stop", radvd_main_stop, "stop RADVD"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
