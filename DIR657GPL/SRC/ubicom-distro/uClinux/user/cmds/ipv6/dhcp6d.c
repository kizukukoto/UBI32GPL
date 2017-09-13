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
#include <nvramcmd.h>
#include "cmds.h"
#include "shutils.h"
#include "ipv6.h"

static int dhcp6d_main_start (int argc, char *argv[])
{
        char dhcpd_prefix[32] = {};
        char dhcpd_start[64] = {};
        char dhcpd_end[64] = {};
        char dhcpd_guest_prefix[32] = {};
        char dhcpd_guest_start[64] = {};
        char dhcpd_guest_end[64] = {};
        char start[50] = {};
        char *lan_interface;
	char guest_prefix[64] = {};
	struct in6_addr guest_pf;
	FILE *fp_dhcppd;
	int pd_len = 128;
	char buffer[128] = {};

	lan_interface = nvram_safe_get("lan_bridge");

	fp_dhcppd = fopen ( DHCP_PD , "r" );
	if ( fp_dhcppd ){
		fgets(buffer, sizeof(buffer), fp_dhcppd);
		if(fgets(buffer, sizeof(buffer), fp_dhcppd))
			buffer[strlen(buffer)-1] = '\0'; /*replace '\n'*/
		pd_len = atoi(buffer);
		fclose(fp_dhcppd);
	}

        if (access(DHCPD6_PID, F_OK) == 0){
                printf("dhcp6s.pid is already active !\n");
                return 1;
        }
        if(NVRAM_MATCH("ipv6_autoconfig","1") && 
		(!NVRAM_MATCH("ipv6_autoconfig_type","stateless") || NVRAM_MATCH("ipv6_dhcp_pd_enable_l", "1"))){

        	printf("Start dhcpdv6\n");
                strcpy( start, nvram_safe_get("ipv6_wan_proto"));
                strcat (start, "_lan_ip");
	
			char lan_ipv6_addr[50]={};
                        char *tmp;
                        // read lan ipv6 global address
                        if ( read_ipv6addr(lan_interface, SCOPE_GLOBAL, lan_ipv6_addr, sizeof(lan_ipv6_addr)) ) {
                                if ( tmp = strchr(lan_ipv6_addr, '/') )
                                        *tmp = '\0';
                                strcpy(start, lan_ipv6_addr);
                        } else 
				perror("not have lan ipv6 global address");

                        // If can't get dhcp-pd
                        if (strstr(start, "ipv6")) {
                                strcpy(start, "");
                        }

                if(strlen(start)){
                        strcpy(dhcpd_prefix,get_ipv6_prefix (start, 64));
			sprintf(dhcpd_guest_prefix, "%s::", dhcpd_prefix);
			inet_pton(AF_INET6, dhcpd_guest_prefix, &guest_pf);
			guest_pf.s6_addr[7] -= 1;
			inet_ntop(AF_INET6, &guest_pf, dhcpd_guest_prefix, sizeof(dhcpd_guest_prefix));
                        strcpy(dhcpd_guest_prefix,get_ipv6_prefix (dhcpd_guest_prefix, 64));
                } else {
                        printf("Current ipv6_wan_proto:%s_lan_ip nvram value is NULL\n", nvram_safe_get("ipv6_wan_proto"));
                        if(start) free (start);
//                        return 1;
                }

                if(strlen(dhcpd_prefix) != 0){
                        nvram_set ("ipv6_dhcpd_prefix", dhcpd_prefix);

                        // Reason: Fix that ipv6 address poll range will show "null"
                        // after configuration it at first time.
                        if (strstr(dhcpd_prefix, "::")) {
                                sprintf(dhcpd_start, "%s:", dhcpd_prefix);
                                sprintf(dhcpd_end, "%s:", dhcpd_prefix);
                        } else {
                                sprintf(dhcpd_start, "%s::", dhcpd_prefix);
                                sprintf(dhcpd_end, "%s::", dhcpd_prefix);
                        }

                        if (strstr(dhcpd_guest_prefix, "::")) {
       	                        sprintf(dhcpd_guest_start, "%s:", dhcpd_guest_prefix);
               	                sprintf(dhcpd_guest_end, "%s:", dhcpd_guest_prefix);
			} else {
       	                        sprintf(dhcpd_guest_start, "%s::", dhcpd_guest_prefix);
                                sprintf(dhcpd_guest_end, "%s::", dhcpd_guest_prefix);
               	        }

                        char *token1 = NULL, *token2 = NULL;
                        char tmp[256], *t = NULL;

                        //---------ipv6_dhcpd_start
                        sprintf(tmp, "%s", nvram_safe_get("ipv6_dhcpd_start"));
                        t = tmp;
                        token2 = strtok(t, ":");
                        while ((token1 = strtok(NULL, ":")) != NULL )
                                token2 = token1;
                        strcat(dhcpd_start, ((token2 != NULL) ? token2 : "1"));
	                strcat(dhcpd_guest_start, ((token2 != NULL) ? token2 : "1"));

                        //---------ipv6_dhcpd_end
                        sprintf(tmp, "%s", nvram_safe_get("ipv6_dhcpd_end"));
                        t = tmp;
                        token2 = strtok(t, ":");
                        while ((token1 = strtok(NULL, ":")) != NULL )
                                token2 = token1;
                        strcat(dhcpd_end, ((token2 != NULL) ? token2 : "200"));
			strcat(dhcpd_guest_end, ((token2 != NULL) ? token2 : "200"));

                        nvram_set("ipv6_dhcpd_start", dhcpd_start);
                        nvram_set("ipv6_dhcpd_end", dhcpd_end);

			if(pd_len < 64){
                        	nvram_set ("ipv6_guest_dhcpd_prefix", dhcpd_guest_prefix);
	                        nvram_set("ipv6_guest_dhcpd_start", dhcpd_guest_start);
        	                nvram_set("ipv6_guest_dhcpd_end", dhcpd_guest_end);
			}

                        write_dhcpd6_conf(pd_len);

//			_system("/sbin/dhcp6s -c %s %s &", DHCPD6_CONF_FILE, lan_interface);
			if(pd_len < 64 && !NVRAM_MATCH("ipv6_autoconfig_type", "stateless") && ifconfirm("ath1"))
	                        _system("/sbin/dhcp6s -c %s ath1 &", DHCPD6_GUEST_CONF_FILE);
                } else
                        unlink(DHCPD6_PID);
        }

	if(write_static_pd() || (NVRAM_MATCH("ipv6_autoconfig","1") && 
		(!NVRAM_MATCH("ipv6_autoconfig_type","stateless") || NVRAM_MATCH("ipv6_dhcp_pd_enable_l", "1"))))
		_system("/sbin/dhcp6s -c %s %s &", DHCPD6_CONF_FILE, lan_interface);

/*XXX Joe Huang : using busybox httpd do not need synchronize*/
#ifndef DIR652V
        _system("killall -SIGUSR2 httpd");
#endif
        return 0;
}



static int dhcp6d_main_stop (int argc, char *argv[])
{
		printf("Stop IPv6 dhcpd\n");
		system("killall dhcp6s");
		if (access(DHCPD6_PID, F_OK) == 0){
			system("killall -9 dhcp6s"); //sometime dhcp6s can not killed
			unlink(DHCPD6_PID);
		}
	return 0;
}

int dhcp6d_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", dhcp6d_main_start, "start DHCP6d"},
                { "stop", dhcp6d_main_stop, "stop DHCP6d"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}

