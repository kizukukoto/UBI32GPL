#include <stdio.h>
#include <stdlib.h>
#include "cmds.h"
#include "shutils.h"
#include "interface.h"
#include "nvram.h"

/*
 * Enable;Name;Source_if;\
 * 	Source_IP/Source_mask/Source_sport-Source_eport;Protocol;
 * 	Dest_IP/Dest_mask/Dest_sport-Dest_eport;Forward;Gateway
 * 	
 * EX: "policy_rule1=1;qwer;br0;192.168.0.0/255.255.255.0/0-65535;"
 * 		"tcp;61.222.188.0/24/0-65535;eth1;172.21.46.136"
 **/
static int policy_route_start(int argc, char *argv[])
{
	cprintf("Start Policy Route\n");

	char *tmp[8], *src[3], *dest[3], *dest_port[2];
	char *p;
	char str[512]={0}, strKey[16]={0}, s[512]={0}, str_rule[512]={0};
	int n = 0, i = 0, flag = 0;

	for( i = 0; i < 25; i++ ){
		memset(strKey, sizeof(strKey), 0);
		memset(tmp, sizeof(tmp), 0);
		memset(str, sizeof(str), 0);
		memset(s, sizeof(s), 0);
	
		sprintf(strKey, "policy_rule%d", i);
		//cprintf("Key=[%s]\n", strKey);

		p = nvram_safe_get(strKey);
		strcpy(str, p);
		//cprintf(str);

		if (strlen(str) == 0)
			goto out;
		
		cprintf("\nStart Parsing:[%d]\n", i);
		if(split(str, ";", tmp, 1)!=8){
			err_msg("nvram:%s is invalid\n");
			return 1;
		}
		
		tmp[2] = __nvram_map("if_alias", tmp[2], "if_dev");
		tmp[6] = __nvram_map("if_alias", tmp[6], "if_dev");
		if (tmp[2] == NULL || tmp[6] == NULL) {
			err_msg("No interface mapped");
			return 2;
		}
		/*for (n = 0; n < 8; n++)
			cprintf("tmp[%d]=%s\n", n, tmp[n]);*/

		if (split(tmp[3], "/", src, 0)!=3){
			cprintf("nvram:%s source ip is invalid\n");
			return 2;
		}
		//cprintf("src:[%s], ip=[%s], mask=[%s], port=[%s]\n", s, src[0], src[1], src[2]);

		if (split(tmp[5], "/", dest, 0)!=3) {
			cprintf("nvram:%s source ip is invalid\n");
			return 2;
		}
		//cprintf("dest:[%s],[%s] ip=[%s], mask=[%s], port=[%s]\n", s, src[0], dest[0], dest[1], dest[2]);

		if ( split(dest[2], "-", dest_port, 0)!=2 ) {
			cprintf("nvram:%s dest port is invalid\n");
			return 3;
		}

		if (strcmp(tmp[0], "1")!= 0)
			goto out;

		memset(str_rule, sizeof(str_rule), 0);
		if( strcmp(tmp[4], "tcp") == 0 || strcmp(tmp[4], "udp") == 0 ){
			sprintf(str_rule, "iptables -t mangle -I PREROUTING -i %s -p %s -s %s/%s -d %s/%s -m mport --dport %s:%s -j MARK --set-mark 10",
				tmp[2], tmp[4], src[0], src[1], dest[0], dest[1], dest_port[0], dest_port[1] );
		} else{
			sprintf(str_rule, "iptables -t mangle -I PREROUTING -i %s -s %s/%s -d %s/%s -j MARK --set-mark 10",
				tmp[2], src[0], src[1], dest[0], dest[1] );
		}
		system(str_rule);
		cprintf("\n%s\n", str_rule);
	
		if( flag == 0){
			memset(str_rule, sizeof(str_rule), 0);
			sprintf(str_rule, "ip rule add fwmark 10 table 5");
			system(str_rule);
			flag = 1;
			cprintf("%s\n\n", str_rule);
		}

		memset(str_rule, sizeof(str_rule), 0);
		sprintf(str_rule, "ip route add %s/%s via %s table 5 dev %s", dest[0], dest[1], tmp[7], tmp[6]);
		system(str_rule);
		cprintf("%s\n\n", str_rule);
	}

out:
	return 0;
}

static int policy_route_stop(int argc, char *argv[]){
	char str[512];

	cprintf("Stop Policy Route\n");
	
	sprintf(str, "ip route flush table 5");
	system(str);
	cprintf("%s\n", str);

	sprintf(str, "ip rule delete table 5");
	system(str);
	cprintf("%s\n", str);

	sprintf(str, "iptables -F -t mangle");
	system(str);
	cprintf("%s\n", str);
	return 0;
}

int policy_route_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", policy_route_start},
		{ "stop", policy_route_stop},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
