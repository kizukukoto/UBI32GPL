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

char *schedule2iptables(char *schedule_name)
{
	static char  schedule_value[100]={0};
	char value[128] = {0}, week[32]={0};
	char *nvram_ptr=NULL;
	char *name = NULL, *weekdays = NULL, *start_time = NULL, *end_time = NULL;
	char schedule_list[]="schedule_rule_00";
	char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	int i=0, day, count=0;

	if ( (schedule_name == NULL) || 
		(strcmp ("Always", schedule_name ) == 0) || 
		(strcmp(schedule_name, "") == 0) )
		return "";

	/*	Schedule format
	 *	name/weekdays/start_time/end_time
	 *	Test/1101011/08:00/18:00
	 *  only valid for PRE_ROUTING, LOCAL_IN, FORWARD and OUTPUT
	 */
	for(i=0; i<MAX_SCHEDULE_NUMBER; i++)
	{
		snprintf(schedule_list, sizeof(schedule_list), "schedule_rule_%02d", i);
		nvram_ptr = nvram_safe_get(schedule_list);
		if ( nvram_ptr == NULL || strcmp(nvram_ptr, "") == 0)
			continue;
		strncpy(value,nvram_ptr,sizeof(value));
		if( getStrtok(value, "/", "%s %s %s %s", 
			&name, &weekdays, &start_time, &end_time) !=4)
			continue;

		/* Add , symbol between date */
		if ( strcmp(name, schedule_name) == 0) {

			for(day=0; day<7; day++) {
				if ( *(weekdays + day) == '1') {
					count += 1;

					if( count >1 ) {
						strcat(week, ",");
						count -=1;
					}
					strcat(week, weekday[day] );
				}
			}

			/* If start_time = end_time or start_tiem=00:00 and end_time=24:00, it means all day */
			if((strcmp(start_time, end_time) == 0) || 
				( !strcmp(start_time,"00:00") && 
				!strcmp(end_time, "24:00")))
				sprintf(schedule_value, "-m time --weekdays %s", week);
			else
				sprintf(schedule_value, 
					"-m time --timestart %s --timestop %s --weekdays %s", 
					start_time, end_time, week);
			return schedule_value;
		}
	}
	return "";
}

void set_ipv6_tables_policy(void)
{
	char *ipv6_filter_type=nvram_safe_get("ipv6_filter_type");
	init_file(FW_IPV6_FILTER);
	save2file(FW_IPV6_FILTER,
		"*filter\n"
		":INPUT ACCEPT [0:0]\n"
		":%s\n"
		":OUTPUT ACCEPT [0:0]\n"
		,((strcmp(ipv6_filter_type, "list_deny") == 0)||
			(strcmp(ipv6_filter_type, "disable") == 0)) ? 
			"FORWARD ACCEPT [0:0]" : "FORWARD DROP [0:0]"
	);
}


static int firewall_main_start (int argc, char *argv[])
{
        char *lan_bridge, *wan_eth;
        char *ipv6_filter_type = nvram_safe_get("ipv6_filter_type");
        char ip_range_cmd[256] = {0}, port_range_cmd[20] = {0}, iface_src_cmd[15] = {0};
        char iface_dest_cmd[15] = {0}, ip_range_src[128], ip_range_dst[128];
        int i = 0, ipv6_rule_number = 20;
        char *getValue, *enable, *name, *schedule_name, *iface_src;
        char *ip_start, *iface_dest, *ip_end, *protocol, *port_start, *port_end;
        char fw_value[512] = {0};
        char fw_rule[32] = {0};
        char action[50] = {0};
	char *ipv6_wan_proto = nvram_safe_get("ipv6_wan_proto");

	/*XXX Expect for these three mode, do not support ipv6 firewall*/
	if(strcmp(ipv6_filter_type, "disable") != 0 && 
		strcmp(ipv6_filter_type, "list_allow") != 0 && 
		strcmp(ipv6_filter_type, "list_deny") != 0)
		return 0;

        lan_bridge = nvram_safe_get("lan_bridge");
        wan_eth = nvram_safe_get("wan_eth");

	if(strcmp(ipv6_wan_proto, "ipv6_pppoe") == 0){
                if(NVRAM_MATCH("ipv6_pppoe_share", "1"))
                        wan_eth = "ppp0";
                if(NVRAM_MATCH("ipv6_pppoe_share", "0"))
                        wan_eth = "ppp6";
	}else if(strcmp(ipv6_wan_proto, "ipv6_6to4") == 0)
		wan_eth = "tun6to4";
	else if(strcmp(ipv6_wan_proto, "ipv6_6in4") == 0)
		wan_eth = "sit1";
	else if(strcmp(ipv6_wan_proto, "ipv6_6rd") == 0)
		wan_eth = "sit2";

        printf("Start IPv6 Firewall\n");

        system("cli ipv6 firewall stop");

        set_ipv6_tables_policy();

        if(strcmp(ipv6_filter_type, "list_allow") == 0 ){
		save2file(FW_IPV6_FILTER,
			"-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n");
                strcpy(action,"ACCEPT -m state --state NEW");
	}
        else
                strcpy(action,"DROP");

        if( (strcmp(ipv6_filter_type, "list_allow") == 0)  ||   \
                (strcmp(ipv6_filter_type, "list_deny") == 0) ){
                for( i = 0 ; i < ipv6_rule_number ; i++ ){
                        snprintf(fw_rule, sizeof(fw_rule), "firewall_rule_ipv6_%02d", i);
                        getValue = nvram_safe_get(fw_rule);
                        if( strlen(getValue) == 0 || strlen(getValue) > sizeof(fw_value) )
                                continue;
                        else{
                                memset(fw_value, 0, sizeof(fw_value));
                                strncpy(fw_value, getValue, strlen(getValue));
                        }
                        if( getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s %s %s", 
				&enable, &name, &schedule_name, &iface_src, &ip_start, 
				&iface_dest, &ip_end, &protocol ,&port_start ,&port_end) != 10 )
                                continue;

                        if(strcmp(schedule_name, "Never") == 0)
                                continue;

                        if(strcmp("1", enable) == 0){
                                if( strcmp (iface_src,"0") == 0)
                                        sprintf (iface_src_cmd, "-i %s", lan_bridge );
                                else if (!strcmp (iface_src, "1"))
                                        sprintf (iface_src_cmd, "-i %s", wan_eth );
                                else
                                        sprintf (iface_src_cmd,"%s", "" );
                                if( strcmp(iface_dest, "0") == 0)
                                        sprintf (iface_dest_cmd, "-o %s", lan_bridge );
                                else if (!strcmp(iface_dest, "1"))
                                        sprintf (iface_dest_cmd, "-o %s", wan_eth );
                                else
                                        sprintf (iface_dest_cmd, "%s", "" );
                                /*Have "-", means user type range address*/
                                if(strstr(ip_start,"-") != NULL)
                                        sprintf (ip_range_src, "-m iprange --src-range %s" , ip_start);
                                else if(strcmp(ip_start,"::") == 0)
                                        sprintf (ip_range_src, "-s ::/0");
				else
                                        sprintf (ip_range_src, "-s %s", ip_start);
                                if(strstr(ip_end,"-") != NULL)
                                        sprintf (ip_range_dst, "-m iprange --dst-range %s" , ip_end);
                                else if(strcmp(ip_end,"::") == 0)
                                        sprintf (ip_range_dst, "-d ::/0");
                                else
                                        sprintf (ip_range_dst, "-d %s", ip_end);
                                sprintf(ip_range_cmd, "%s %s", ip_range_src ,ip_range_dst);

                                if(strcmp(protocol,"ICMPv6") != 0 && ( strcmp(port_start,"1") != 0 || 
					strcmp(port_end,"65535") != 0))
                                        sprintf(port_range_cmd,"--dport %s:%s",port_start,port_end);

                                if( (strcmp(protocol,"TCP") == 0 ) || (strcmp(protocol,"Any") == 0) ){
                                        save2file(FW_IPV6_FILTER,"-A FORWARD -p TCP %s %s %s %s %s -j %s\n", \
                                                iface_src_cmd,iface_dest_cmd, ip_range_cmd, port_range_cmd, \
                                                schedule2iptables(schedule_name), action );
                                }
                                if( (strcmp(protocol,"UDP") == 0) || (strcmp(protocol,"Any") == 0) ){
                                        save2file(FW_IPV6_FILTER,"-A FORWARD -p UDP %s %s %s %s %s -j %s\n", \
                                                iface_src_cmd,iface_dest_cmd, ip_range_cmd, port_range_cmd, \
                                                schedule2iptables(schedule_name), action );
                                }
                                if( (strcmp(protocol,"ICMPv6") == 0) || (strcmp(protocol,"Any") == 0) ){
                                        save2file(FW_IPV6_FILTER,"-A FORWARD -p ICMPv6 %s %s %s %s -j %s\n", \
                                                iface_src_cmd,iface_dest_cmd, ip_range_cmd, \
                                              schedule2iptables(schedule_name), action );
                                }
                        }
                }
        }
                 save2file(FW_IPV6_FILTER, "COMMIT\n");
                _system("ip6tables-restore %s", FW_IPV6_FILTER);
                _system("cp %s %s_LAST", FW_IPV6_FILTER, FW_IPV6_FILTER);
                unlink(FW_IPV6_FILTER);

	return 0;
}

static int firewall_main_stop (int argc, char *argv[])
{
	char *ipv6_filter_type = nvram_safe_get("ipv6_filter_type");
        /*XXX Expect these three mode, do not support ipv6 firewall*/
        if(strcmp(ipv6_filter_type, "disable") != 0 &&
                strcmp(ipv6_filter_type, "list_allow") != 0 &&
                strcmp(ipv6_filter_type, "list_deny") != 0)
		return 0;

	init_file(FW_IPV6_FLUSH);
	printf("flush_ip6tables\n");
	save2file(FW_IPV6_FLUSH,
		"*filter\n"
		":INPUT ACCEPT [0:0]\n"
		":FORWARD ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		"COMMIT\n"
	);      
	_system("ip6tables-restore %s", FW_IPV6_FLUSH);
	return 0;
}

int firewall_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", firewall_main_start, "start IPv6 firewall"},
                { "stop", firewall_main_stop, "stop IPv6 firewall"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
