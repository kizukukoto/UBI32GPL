#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ssi.h"
#include "libdb.h"
#include "debug.h"
#include "nvram.h"
#include "shvar.h"
#include "sutil.h"

extern char *get_ipaddr(char *);
int total_lines = 0;

struct __ipv6_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int ipv6_client_numbers(void)
{
        FILE *fp;
        int numbers = 0;
        char temp[IPV6_BUFFER_SIZE] = {};
        fp = fopen(IPV6_CLIENT_RESULT,"r");
        if(fp)
        {
                while(fgets(temp,IPV6_BUFFER_SIZE,fp))
                        numbers++;
                fclose(fp);
                return numbers;
        }
        return 0;
}

void get_ipv6_client_list_table(void)
{
        int i = 0;
        FILE *fp;
        char temp[IPV6_BUFFER_SIZE] = {};
        total_lines = 0;
	system("cli ipv6 clientlist");
//	get_ipv6_client_list();
        usleep(500);

        fp = fopen(IPV6_CLIENT_RESULT,"r");
        if(fp)
        {
                for(i=0;i<ipv6_client_numbers();i++)
                {
                        fgets(temp,IPV6_BUFFER_SIZE,fp);
                        temp[strlen(temp) - 1] = '\0';
                        printf("%s@",temp);
                        memset(temp,0,sizeof(temp));
                }
                fclose(fp);
        }
}

void get_link_local_ip_w(void)
{
        FILE *fp;
        char buf[1024]={},link_local_ip[128]={};
        char *start,*end;
        init_file(LINK_LOCAL_INFO);
        _system("ifconfig %s > %s",nvram_safe_get("wan_eth"),LINK_LOCAL_INFO);

        fp = fopen(LINK_LOCAL_INFO,"r+");
        if(fp)
        {
                while(start = fgets(buf,1024,fp))
                {
                        if(end = strstr(buf,"Scope:Link"))
                        {
                                end = end - 1;
                                if(start = strstr(start,"addr:"))
                                {
                                        start = start + 5 + 1;
                                        strncpy(link_local_ip,start,end - start);
                                        printf("%s", link_local_ip);
                                        break;
                                }
                        }
                }
                fclose(fp);
        }
}

void get_link_local_ip_l(void)
{
        FILE *fp;
        char buf[1024]={},link_local_ip[128]={};
        char *start,*end;
        init_file(LINK_LOCAL_INFO);
        _system("ifconfig %s > %s",nvram_safe_get("lan_bridge"),LINK_LOCAL_INFO);

        fp = fopen(LINK_LOCAL_INFO,"r+");
        if(fp)
        {
                while(start = fgets(buf,1024,fp))
                {
                        if(end = strstr(buf,"Scope:Link"))
                        {
                                end = end - 1;
                                if(start = strstr(start,"addr:"))
                                {
                                        start = start + 5 + 1; 
                                        strncpy(link_local_ip,start,end - start);
                                        printf("%s", link_local_ip);
                                        break;
                                }
                        }
                }
                fclose(fp);
        }
}

static int misc_ipv6_help(struct __ipv6_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

void get_6to4_tunnel_address(void)
{
        char *wan_if;
        char *wan_ipv4_address;
        char ip_token_1[8] = {};
        char ip_token_2[8] = {};
        char ip_token_3[8] = {};
        char ip_token_4[8] = {};
        char ipv6_6to4_prefix[32] = {};

        if(nvram_match("wan_proto","pppoe") == 0 || nvram_match("wan_proto","pptp") == 0 || nvram_match("wan_proto","l2tp") == 0 )
                wan_if = "ppp0";
        else
                wan_if = nvram_safe_get("wan_eth");
        if(get_ip_addr(wan_if))
        {
                wan_ipv4_address = get_ip_addr(wan_if);
                strcpy(ip_token_1,strtok(wan_ipv4_address,"."));
                strcpy(ip_token_2,strtok(NULL,"."));
                strcpy(ip_token_3,strtok(NULL,"."));
                strcpy(ip_token_4,strtok(NULL,"."));
                sprintf(ipv6_6to4_prefix,"%02x%02x:%02x%02x",atoi(ip_token_1),atoi(ip_token_2),atoi(ip_token_3),atoi(ip_token_4));
                printf("2002:%s::%s",ipv6_6to4_prefix,ipv6_6to4_prefix);
        }
        else
        {
                cprintf("WAN not ready\n");
                printf("0:0:0:0:0:0:0:0");
        }
}

void get_6to4_lan_address(void)
{
        char *wan_if;
        char *wan_ipv4_address;
        char ip_token_1[8] = {};
        char ip_token_2[8] = {};
        char ip_token_3[8] = {};
        char ip_token_4[8] = {};
        char ipv6_6to4_prefix[32] = {};

        if(nvram_match("wan_proto","pppoe") == 0 || nvram_match("wan_proto","pptp") == 0 || nvram_match("wan_proto","l2tp") == 0 )
                wan_if = "ppp0";
        else
                wan_if = nvram_safe_get("wan_eth");
        if(get_ip_addr(wan_if))
        {
                wan_ipv4_address = get_ip_addr(wan_if);
                strcpy(ip_token_1,strtok(wan_ipv4_address,"."));
                strcpy(ip_token_2,strtok(NULL,"."));
                strcpy(ip_token_3,strtok(NULL,"."));
                strcpy(ip_token_4,strtok(NULL,"."));
                sprintf(ipv6_6to4_prefix,"%02x%02x:%02x%02x",atoi(ip_token_1),atoi(ip_token_2),atoi(ip_token_3),atoi(ip_token_4));
                printf("2002:%s:",ipv6_6to4_prefix);
        }
        else
        {
                cprintf("WAN not ready\n");
                printf("2002:0:0:");
        }
}

void get_wan_current_ipaddr(void)
{
	char *ipaddr, *wan_if;
        if(nvram_match("wan_proto","pppoe") == 0 || nvram_match("wan_proto","pptp") == 0 || nvram_match("wan_proto","l2tp") == 0 )
                wan_if = "ppp0";
        else
                wan_if = nvram_safe_get("wan_eth");

	ipaddr = get_ipaddr(wan_if);
	printf("%s",ipaddr);
}

static void set_percent(int value)
{
	if(value >= -1 && value <= 100)
		_system("echo %d > %s", value, WIZARD_IPV6);
}

void wizard_autodetect_start(void)
{
	char ppp_ip[64] = {};
	char wan_ip[64] = {};
	char lan_ip[64] = {};
	int count = 0;
	if(access(WIZARD_IPV6, F_OK) == 0)
		return; //wizard is aleardy running

	read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp_ip, sizeof(ppp_ip));
	read_ipv6addr(nvram_safe_get("wan_eth"), SCOPE_GLOBAL, wan_ip, sizeof(wan_ip));
	read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));

	if((strlen(wan_ip) != 0 || strlen(ppp_ip) != 0) && strlen(lan_ip) != 0){
		set_percent(100);
		sleep(3);	//Confirm GUI can get "-1" or "100" value
		unlink(WIZARD_IPV6);
		return;
	}

	nvram_set("ipv6_dhcp_pd_enable","1");
	nvram_set("autoconfig_type","stateless_dhcp");
	nvram_set("ipv6_wan_specify_dns", "0");
	nvram_set("ipv6_autoconfig", "1");
	set_percent(10);

	if(nvram_match("wan_proto", "pppoe") == 0 && access("/var/run/ppp-00.pid", F_OK) != 0){
		nvram_set("ipv6_wan_proto","ipv6_autodetect");
		nvram_set("ipv6_pppoe_share","1");
		nvram_set("ipv6_pppoe_dynamic","1");
		system("cli ipv6 autoconfig stop");
		system("cli ipv6 rdnssd stop");
		modify_ppp_options();
		system("/bin/wan_conn pppoe_00_connect &");
		while(count < 25){
			read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp_ip, sizeof(ppp_ip));
			read_ipv6addr(nvram_safe_get("wan_eth"), SCOPE_GLOBAL, wan_ip, sizeof(wan_ip));
			read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));
			if((strlen(wan_ip) != 0 || strlen(ppp_ip) != 0) && strlen(lan_ip) != 0)
				break;
			else{
				sleep(1);
				count++;
				set_percent(15 + count * 3);
			}
		}

		system("/bin/wan_conn pppoe_00_disconnect &");
		system("cli ipv6 autoconfig stop");
		system("cli ipv6 rdnssd stop");

		if(strlen(ppp_ip) != 0 && strlen(lan_ip) != 0){
			nvram_set("ipv6_wan_proto", "ipv6_pppoe");
			nvram_set("ipv6_pppoe_dns_enable", "0");
			nvram_commit();
			set_percent(100);
		}
		else if(strlen(wan_ip) != 0 && strlen(lan_ip) != 0){
			nvram_set("ipv6_wan_proto", "ipv6_autoconfig");
			nvram_set("ipv6_autoconfig_dns_enable", "0");
			nvram_commit();
			set_percent(100);
		}else
			set_percent(-1);


	}else{
		nvram_set("ipv6_wan_proto","ipv6_autoconfig");
		nvram_commit();
		system("cli ipv6 autoconfig stop");
		system("cli ipv6 rdnssd stop");
		_system("cli ipv6 rdnssd start %s &", nvram_safe_get("wan_eth"));

		while(count < 10){
			read_ipv6addr(nvram_safe_get("wan_eth"), SCOPE_GLOBAL, wan_ip, sizeof(wan_ip));
			read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));
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

		if(strlen(wan_ip) == 0 || strlen(lan_ip) == 0)
			set_percent(-1);
		else
			set_percent(100);
	}
	sleep(3);	//Confirm GUI can get "-1" or "100" value
	unlink(WIZARD_IPV6);
        return ;
}

void wizard_autodetect_stop(void)
{
	cprintf("User click cancel button, STOP Autodetect\n");
	system("cli ipv6 rdnssd stop");
	system("cli ipv6 autoconfig stop");
	system("killall -9 pppd");
	unlink("/var/run/ppp-00.pid");
	system("killall wan_conn");
	unlink(WIZARD_IPV6);
	system("killall ipv6");
}

int ipv6_main(int argc, char *argv[])
{
	int ret = -1;
	struct __ipv6_struct *p, list[] = {
		{ "link_local_ip_w", "WAN Link Local IP" , get_link_local_ip_w},
		{ "link_local_ip_l", "LAN Link Local IP" , get_link_local_ip_l},
		{ "wan_current_ipaddr_00", "Get wan current IPv4 address", get_wan_current_ipaddr},
		{ "6to4_tunnel_address", "Get 6to4 tunnel address", get_6to4_tunnel_address},
		{ "6to4_lan_address", "Get 6to4_lan_address", get_6to4_lan_address},
		{ "ipv6_client_list", "Get IPv6 client list" , get_ipv6_client_list_table},
		{ "wizard_autodetect_start", "Start Auto Detection in wizard" , wizard_autodetect_start },
		{ "wizard_autodetect_stop", "Stop Auto Detection in wizard" , wizard_autodetect_stop },
		{ NULL, NULL, NULL }
	};

	if (argc == 1 || strcmp(argv[1], "help") == 0)
		goto help;

	for (p = list; p && p->param; p++) {
		if (strcmp(argv[1], p->param) == 0) {
			ret = p->fn(argc - 1, argv + 1);
			goto out;
		}
	}
help:
	ret = misc_ipv6_help(list);
out:
	return ret;
}
