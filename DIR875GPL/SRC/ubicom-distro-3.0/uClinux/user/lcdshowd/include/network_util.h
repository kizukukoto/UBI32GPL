#ifndef __NETWORK_UTIL_H__
#define __NETWORK_UTIL_H__

#define WAN_CONNECT_FILE      "/var/tmp/wan_connect_time.tmp"
#define PACKET_LOG_PATH       "/var/tmp/packet_log_tmp"

typedef struct routing_table_s{
	char dest_addr[16];
	char dest_mask[16];
	char gateway[16];
	char interface[10];
	int metric;
	/*	Date: 2008-12-22
	*	Name: jimmy huang
	*	Reason: for the feature routing information can show "Type" and "Creator"
	*			so we add 2 more fileds in routing_table_s structure
	*	Note:
	*	  Type:
	*		0:Internet: 	The table is learned from Internet
	*		1:DHCP Option:	The table is learned from DHCP server in Internet
	*		2:STATIC:		The table is learned from "Static Route" for Internet
	*						Port of DIR-Series
	*		3:INTRANET:	    The table is used for Intranet
	*		4:LOCAL		:	Local loop back
	*
	*	  Creator:
	*		0:System:		Learned automatically
	*		1:ADMIN:		Learned via user's operation in Web GUI
	*		
	*/
	int type;
	int creator;
}routing_table_t;

typedef struct performance_t{
	unsigned long rx;
	unsigned long tx;
	long throughput_rx;
	long throughput_tx;
	long last_update_time;
}performance_info;

extern char *read_ipaddr(char *if_name);
extern char *read_ipaddr_no_mask(char *if_name);
extern char *read_gatewayaddr(char *if_name);
extern char *read_netmask_addr(char *if_name);
extern unsigned long get_wan_connect_time(char *wan_proto, int check_wan_status);
extern char *wan_statue(const char *proto);
extern void reset_performance(performance_info *info);
extern int get_net_stats(char *if_name, performance_info *info);
extern int get_wlan_stats(char *if_name, performance_info *info);
extern void get_ath_macaddr(char *strMac);

#endif

