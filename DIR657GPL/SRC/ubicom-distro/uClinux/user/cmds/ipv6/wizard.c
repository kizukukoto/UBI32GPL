#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"

static void set_percent(int value)
{
	if(value >= -1 && value <= 100)
		_system("echo %d > %s", value, WIZARD_IPV6);
}

// waitting ajax.c create_ipv6_wizard_status_xml remove WIZARD_IPV6
static void waitting_remove_WIZARD_IPV6()
{
	int count = 0;
	
	do {
		sleep(1);
		count++;
	} while (count<7 && access(WIZARD_IPV6, F_OK)==0);
	
	if (count>=7)
		unlink(WIZARD_IPV6);
}

static void wizard_start(int argc, char *argv[])
{
	char ppp_ip[64] = {};
	char wan_ip[64] = {};
	char lan_ip[64] = {};
	int count = 0;
	char *wan_eth;
	char *lan_bridge;
	
	if(access(WIZARD_IPV6, F_OK) == 0)
		return; //wizard is aleardy running

	wan_eth = nvram_safe_get("wan_eth");
	lan_bridge = nvram_safe_get("lan_bridge");
	
	read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp_ip, sizeof(ppp_ip));
	if (strlen(ppp_ip)<1)
		read_ipv6addr("ppp6", SCOPE_GLOBAL, ppp_ip, sizeof(ppp_ip));
	
	read_ipv6addr(wan_eth, SCOPE_GLOBAL, wan_ip, sizeof(wan_ip));
	read_ipv6addr(lan_bridge, SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));


	printf("ipv6_wizard: now state wan_ip=%s, ppp_ip=%s, lan_ip=%s\n",wan_ip, ppp_ip, lan_ip);

	if((strlen(ppp_ip) != 0 || strlen(wan_ip) != 0) && strlen(lan_ip) != 0){
		set_percent(100);
		waitting_remove_WIZARD_IPV6();	//Confirm GUI can get "-1" or "100" value
		return;
	}

	nvram_set("ipv6_dhcp_pd_enable","1");
	nvram_set("ipv6_autoconfig_type","stateless_dhcp");
	nvram_set("ipv6_wan_specify_dns", "0");
	nvram_set("ipv6_autoconfig", "1");
	set_percent(5);
	
	if(NVRAM_MATCH("wan_proto", "pppoe"))
	{
		printf("ipv6_wizard: wan_ip=%s, ppp_ip=%s, lan_ip=%s try to detect ipv6_pppoe\n",wan_ip, ppp_ip, lan_ip);
		
		system("cli ipv6 pppoe disdial");
		sleep(2);
		set_percent(10);
		
		nvram_set("ipv6_wan_proto","ipv6_pppoe");
		nvram_set("ipv6_pppoe_share","0");
		nvram_set("ipv6_pppoe_dynamic","1");
		nvram_set("ipv6_pppoe_dns_enable", "0");
		nvram_set("ipv6_pppoe_username", nvram_safe_get("wan_pppoe_username_00"));
		nvram_set("ipv6_pppoe_password", nvram_safe_get("wan_pppoe_password_00"));
		nvram_set("ipv6_pppoe_service", nvram_safe_get("wan_pppoe_service_00"));
		
		set_percent(15);
		
		//------------- Try IPv6 PPPoE Mode --------------------
		system("cli ipv6 pppoe dial");
		sleep(3);
		set_percent(20);
		
		count = 0;
		while(count < 10){
			read_ipv6addr("ppp6", SCOPE_GLOBAL, ppp_ip, sizeof(ppp_ip));
			read_ipv6addr(lan_bridge, SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));
			if( strlen(ppp_ip) != 0 && strlen(lan_ip) != 0 )
				break;
			else{
				sleep(1);
				count++;
				set_percent(30 + count * 3);
			}
		}
		system("cli ipv6 pppoe disdial");
		
		//------------- Try Autoconfiguration Mode --------------------
		if (count >=10) {
			printf("IPv6 PPPoE if Failed, Change to Autoconfiguration Mode\n");
			nvram_set("ipv6_wan_proto","ipv6_autoconfig");
			nvram_set("ipv6_autoconfig_dns_enable", "0");
			_system("cli ipv6 rdnssd start %s", wan_eth);
			
			sleep(2);
			
			while(count < 20){
				read_ipv6addr(wan_eth, SCOPE_GLOBAL, wan_ip, sizeof(wan_ip));
				read_ipv6addr(lan_bridge, SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));
				if( strlen(wan_ip) != 0 && strlen(lan_ip) != 0 )
					break;
				else{
					sleep(1);
					count++;
					set_percent(30 + count * 3);
				}
			}
		}
		
		set_percent(95);
		
		system("cli ipv6 autoconfig stop");
		system("cli ipv6 rdnssd stop");
		
		if(strlen(ppp_ip) != 0 && strlen(lan_ip) != 0){
			nvram_set("ipv6_pppoe_share","1");
			nvram_commit();
			system("killall -SIGUSR2 httpd");
			set_percent(100);
			printf("ipv6_wizard: wan_ip=%s, ppp_ip=%s, lan_ip=%s detect_type=ipv6_pppoe\n",wan_ip, ppp_ip, lan_ip);
		}
		else if(strlen(wan_ip) != 0 && strlen(lan_ip) != 0){
			nvram_set("ipv6_wan_proto", "ipv6_autoconfig");
			nvram_commit();
			system("killall -SIGUSR2 httpd");
			set_percent(100);
			printf("ipv6_wizard: wan_ip=%s, ppp_ip=%s, lan_ip=%s detect_type=ipv6_autoconfig\n",wan_ip, ppp_ip, lan_ip);
		}else {
			set_percent(-1);
			printf("ipv6_wizard: wan_ip=%s, ppp_ip=%s, lan_ip=%s can't detect ipv6 type\n",wan_ip, ppp_ip, lan_ip);
		}

	}else{
		printf("ipv6_wizard: wan_ip=%s, ppp_ip=%s, lan_ip=%s try to detect ipv6_autoconfig\n",wan_ip, ppp_ip, lan_ip);
		system("cli ipv6 autoconfig stop");
		system("cli ipv6 rdnssd stop");
		_system("cli ipv6 rdnssd start %s &", wan_eth);

		while(count < 10){
			read_ipv6addr(wan_eth, SCOPE_GLOBAL, wan_ip, sizeof(wan_ip));
			read_ipv6addr(lan_bridge, SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));
			if(strlen(wan_ip) != 0 && strlen(lan_ip) != 0)
				break;
			else{
				sleep(1);
				count++;
				set_percent(15 + count * 8);
			}
		}
		system("cli ipv6 autoconfig stop");
		system("cli ipv6 rdnssd stop");

		if(strlen(wan_ip) == 0 || strlen(lan_ip) == 0) {
			set_percent(-1);
			printf("ipv6_wizard: wan_ip=%s, ppp_ip=%s, lan_ip=%s can't detect ipv6 type\n",wan_ip, ppp_ip, lan_ip);
		} else {
			printf("ipv6_wizard: wan_ip=%s, ppp_ip=%s, lan_ip=%s detect_type=ipv6_autoconfig\n",wan_ip, ppp_ip, lan_ip);
			sleep(5);
			nvram_set("ipv6_wan_proto","ipv6_autoconfig");
			nvram_commit();
			system("killall -SIGUSR2 httpd");
			set_percent(100);
		}
	}
	
	waitting_remove_WIZARD_IPV6();	//Confirm GUI can get "-1" or "100" value
        return ;
}

static void wizard_stop(int argc, char *argv[])
{
	cprintf("User click cancel button, STOP Autodetect\n");
	system("cli ipv6 pppoe disdial");
	unlink(WIZARD_IPV6);
	system("killall cli");
}

int wizard_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", wizard_start, "wizard start"},
		{ "stop", wizard_stop, "wizard stop"},
		{ NULL, NULL}
	};
	int rev;
	rev = lookup_subcmd_then_callout(argc, argv, cmds);
	return rev;
}
