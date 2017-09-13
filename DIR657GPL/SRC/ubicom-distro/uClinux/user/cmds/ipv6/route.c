#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <nvram.h>
#include "ipv6.h"
#include "cmds.h"
#include "shutils.h"
/*XXX Joe Huang: This cmd is use for ipv6 static routing*/
/*route start would add the routing rule form nvram*/
/*route stop would delete the routing rule that we add before*/
/* nvram: enable/name/dest_addr/prefix/gateway/interface/metric */

static int route_main_start (int argc, char *argv[])
{
	char static_routing[]="static_ipv6_routing_XYXXXXX";
	char *enable = NULL, *name = NULL, *value = NULL, *dest_addr = NULL;
	char *dest_prefix = NULL, *gateway = NULL, *ifname = NULL, *metric = NULL;
	char route_value[256] = {0};
	char interface[8] = {0};
	char routing_buf[1024] = {0};
	char *lan_bridge, *wan_eth;
	lan_bridge = nvram_safe_get("lan_bridge");
	wan_eth = nvram_safe_get("wan_eth");
	FILE *fp ;
	int i;
	char *wan_proto=NULL;

	printf("start_routev6\n");
	init_file(IPV6_ROUTING_INFO);	
	chmod(IPV6_ROUTING_INFO, 0755); 

	for(i = 0; i < MAX_STATIC_ROUTINGV6_NUMBER; i++) {
		memset(route_value,0,sizeof(route_value));
		sprintf(static_routing, "static_routing_ipv6_%02d", i);
		value = nvram_safe_get(static_routing);		
	
		if ( strlen(value) == 0 ) 
			continue;
		else 
			strcpy(route_value, value);

		enable = strtok(route_value, "/");
		name = strtok(NULL, "/");
		dest_addr = strtok(NULL, "/");
		dest_prefix = strtok(NULL, "/");
		gateway = strtok(NULL, "/");
		ifname = strtok(NULL, "/");
		metric = strtok(NULL, "/");
	
		if( enable == NULL || name == NULL || dest_addr == NULL || 
			dest_prefix == NULL || gateway == NULL || ifname == NULL )
			continue;	
		/*XXX Joe Huang : Add this condition to avoid the gateway is empty would made token error*/
		else if ( (strcmp(gateway, "NULL") == 0 || strcmp(gateway, "DHCPPD") == 0) && 
			metric == NULL ){
			metric = ifname;
			ifname = gateway;
		}

		memset(interface,0,sizeof(interface));
			
		if( strcmp(ifname, "LAN") == 0)
			strcpy(interface,nvram_get("lan_bridge"));					
		else if( strcmp(ifname, "WAN") == 0)
			strcpy(interface,nvram_get("wan_eth"));	
		else if(strcmp(ifname,"DHCPPD") == 0)
			continue;
		else
			strcpy(interface,"lo");
	
		if( !strcmp(enable, "1" ) ){
			if(strcmp(ifname, "NULL") != 0){
				save2file(IPV6_ROUTING_INFO,"route -A inet6 add %s/%s gw %s dev %s metric %s\n", \
					dest_addr,dest_prefix, gateway, \
					interface, metric );
			}else{  
				save2file(IPV6_ROUTING_INFO,"route -A inet6 add %s/%s dev %s metric %s\n", \
					dest_addr,dest_prefix, \
					interface, metric );   
			}                                                 
		}
	}
	_system("/bin/sh %s",IPV6_ROUTING_INFO);
	return 0;
}

static int route_main_stop (int argc, char *argv[])
{
	FILE *fp;
	char *action = NULL;
	char tmp[512] = {};
	char buf[1024] = {};

        fp = fopen(IPV6_ROUTING_INFO,"r");
        if(!fp)
                return ;

        while(action = fgets(buf,1024,fp)){
		if(action = strstr(action, "add")){
			strcpy(tmp,action + 3);
			strcpy(action, "del");
			strcat(action,tmp);
			system(buf);
			memset(buf,0,sizeof(buf));
		}
	}
	fclose(fp);
}


int route_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", route_main_start, "start IPv6 Static Route"},
                { "stop", route_main_stop, "stop IPv6 Static Route"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
