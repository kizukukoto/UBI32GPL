#ifndef _cmoapi_h
#define _cmoapi_h
#include <httpd.h>
#include <netinet/in.h>
#include "autoconf.h"


/* nvram default tmp path , for apply_XXX type */
#define NVRAM_CONFIG_PATH "/var/tmp/nvram.tmp"
#define NVRAM_CONFIG_SIZE 50000
#define WEB_TITLE "D-LINK CORPORATION, INC | WIRELESS ROUTER | HOME"
#define WEB_COPYRIGHT "Copyright &copy; 2004-2011 D-Link Corporation, Inc."

typedef struct chklst_s{
	char *FW;
	char *Build;
	char *Date;
	char *LINUX;
	char *LINUX_BUILD;
	char *LINUX_BUILD_DATE;
	char *SDK;
	char *SDK_Build;
	char *SDK_Date;
	char *WLAN_Version;
	char *WLAN_Build;
	char *WLAN_Date;
	char *WAN_MAC;
	char *LAN_MAC;
	char *WLAN_MAC;
#ifdef USER_WL_ATH_5GHZ
	char *WLAN1_MAC;
	char *SSID1;		//5.0GHz
	char *WLAN_Domain1;	//5.0GHz
#endif
	char *WLAN_Domain;
	char *SSID;
	char *HW_Version;
	int DEFAULT_VALUE;
#ifdef NVRAM_CONF_VER_CTL	
	char *Config_Version;
#endif
}chklst_t;

typedef struct mapping_s{
	char *nvram_mapping_name;
}mapping_t;

typedef struct ping_ipaddr_s{
        char *ping_ipaddr;
#ifdef IPv6_SUPPORT
	char *ping_ipaddr_v6;
	char *interface;
	int ping6_size;
	int ping6_count;
#endif
}ping_ipaddr_t;

typedef struct usb_test_s{
        char *result;
}usb_test_t;

typedef struct wps_pin_s{
        char *pin;
}wps_pin_t;

struct page_table{
	char *mainPage;
	char *subPage;
};

/*
*	Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetStatus can used the args.
*   Notice :
*/
/*
typedef struct status_handler_s {
	char *pattern;
	void (*output)(webs_t wp);
}status_handler_t;
*/
typedef struct status_handler_s {
	char *pattern;
	void (*output)(webs_t wp,char *args);
}status_handler_t;

/*
*	Date: 2009-05-07
*   Name: Ken Chiang
*   Reason: support mutil user login by MAC address
*	to fixed all on-line account will be change to last login account.
*   Notice :
*/
#ifdef HTTPD_USED_MUTIL_AUTH
typedef struct authorization_s {
        char curr_user[24];
        char curr_passwd[24];
        char mac_addr[18];
        int logintime;
}authorization_t;
#else
typedef struct authorization_s {
        char *curr_user;
        char *curr_passwd;
}authorization_t;
#endif

/*
*	Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetList can used the args.
*   Notice :
*/
/*
typedef struct list_handler_s {
	char *pattern;
	void (*output)(webs_t wp);
}list_handler_t;
*/
typedef struct list_handler_s {
	char *pattern;
	void (*output)(webs_t wp,char *args);
}list_handler_t;

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

typedef struct dhcpd_leased_table_s{
	char *hostname;
	char *ip_addr;
	char *mac_addr;
	char *expired_at;
}dhcpd_leased_table_t;

typedef struct igmp_table_s{
	char *member;
}igmp_table_t;

struct igmp_list_s{
     unsigned char member[16];
};

typedef struct wireless_stations_table_s{
	char *mac_addr;
	char *rate;
	char *rssi;
	char *mode;
	char *connected_time;
	char *ip_addr;
	char list[80];
	char info[80];
}wireless_stations_table_t;

typedef struct condition_status_s{
	char *status_1;
	char *status_2;
	char *status_3;
}condition_status_t;

typedef struct log_msg_table_s{
	char *time;
	char *type;
	char *message;
}log_msg_table_t;

typedef struct log_info_s{
	int cur_page;
	int total_page;
	int total_logs;
	int remainder;
}log_info_t;

typedef struct wan_mac_clone_s{
	unsigned int ip_addr;
	char mac_addr[17];
#ifdef IPv6_SUPPORT
	struct in6_addr ip6_addr;
#endif
	int conn_fd;
}wan_mac_clone_t;

typedef struct wlan_wepkey_s {
    char *wlan_name;
    char *wlan_value;
    void (*set_wlan_flag)(void);
}wlan_wepkey_t;

typedef struct mssid_vlan_table_s{
	char enable;
	char index;
	char *ssid;
	char *Encryption;
	char vlan_id;
}mssid_vlan_table_t;

typedef struct napt_session_info_s{
	char protocol[8];
	char local_ip[16];
	char remote_ip[16];
	char local_port[8];
	char remote_port[8];
	char public_port[8];
	char timeout[8];
	char state[8];
	char direction[8];
	struct napt_session_info_s *next;
}napt_session_info_t;

typedef struct napt_session_table_s{
	char local_ip[16];
	int tcp_count;
	int udp_count;
	struct napt_session_info_s *next;
	struct napt_session_info_s *last;
}napt_session_table_t;

/*	Date: 2009-01-20
*	Name: jimmy huang
*	Reason: for usb-3g detect status
*	Note:
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
typedef struct usb_3g_device_table_s{
	char vendorID[6];
	char productID[6];
	char manufactor[16];
	char model[16];
}usb_3g_device_table_t;
#endif

#ifdef HTTPD_USED_MUTIL_AUTH
#define MAX_AUTH_NUMBER 100
extern authorization_t auth_login[MAX_AUTH_NUMBER];
#else
extern authorization_t auth_login;
#endif
extern ping_ipaddr_t ping;
extern chklst_t chklst;
extern log_info_t logs_info;
extern wan_mac_clone_t con_info;
extern usb_test_t usb_test;
extern wps_pin_t enrollee;

/* for Basic setting */
extern void set_basic_api(webs_t wp);
extern void set_multi_pppoe(webs_t wp);
extern int ej_cmo_get_cfg(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_cmo_get_status(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_cmo_get_list(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_cmo_list_match(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_cmo_get_client_count(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_cmo_get_client_list(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_cmo_get_log(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_get_listcfg(int eid, webs_t wp, int argc, char_t **argv);
extern int ej_cmo_exec_cgi(int eid, webs_t wp, int argc, char_t **argv);

/* CmoGetCfg */
/**************************************************************/
/**************************************************************/
extern int hexTO10(char *hex_1,char *hex_0);
extern void transfer_nvram_value(const char *start,char *mode,char *result);
/* for Advanced setting */
//extern void set_adv_api(webs_t wp);
/* CmoGetCfg */
/**************************************************************/
/**************************************************************/

/*
*	Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetStatus can used the args.
*   Notice :
*/
/* CmoGetStatus */
/**************************************************************/
/**************************************************************/
extern void return_wan_current_ipaddr_00(webs_t wp,char *args);
extern void return_wan_current_ipaddr_01(webs_t wp,char *args);
extern void return_wan_current_ipaddr_02(webs_t wp,char *args);
extern void return_wan_current_ipaddr_03(webs_t wp,char *args);
extern void return_wan_current_ipaddr_04(webs_t wp,char *args);
extern void return_lan_tx_bytes(webs_t wp,char *args);
extern void return_lan_rx_bytes(webs_t wp,char *args);
extern void return_wan_tx_bytes(webs_t wp,char *args);
extern void return_wan_rx_bytes(webs_t wp,char *args);
extern void return_wlan_tx_bytes(webs_t wp,char *args);
extern void return_wlan_rx_bytes(webs_t wp,char *args);
extern void return_lan_ip_info(webs_t wp,char *args);
extern void return_read_fw_success(webs_t wp,char *args);
extern void return_version(webs_t wp,char *args);
extern void return_build(webs_t wp,char *args);
extern void return_version_date(webs_t wp,char *args);
extern void return_hw_version(webs_t wp,char *args);
extern void return_title(webs_t wp,char *args);
extern void return_gui_logout(webs_t wp,char *args);
extern void return_copyright(webs_t wp,char *args);
extern void return_wireless_driver_info(webs_t wp,char *args);
extern void return_wireless_domain(webs_t wp,char *args);
extern void return_wireless_mac(webs_t wp,char *args);
extern void return_lan_tx_packets (webs_t wp,char *args);
extern void return_lan_rx_packets (webs_t wp,char *args);
extern void return_lan_lost_packets(webs_t wp,char *args);
extern void return_wan_tx_packets (webs_t wp,char *args);
extern void return_wan_rx_packets (webs_t wp,char *args);
extern void return_wan_lost_packets(webs_t wp,char *args);
extern void return_wlan_tx_packets (webs_t wp,char *args);
extern void return_wlan_rx_packets (webs_t wp,char *args);
extern void return_wlan_lost_packets(webs_t wp,char *args);
extern void return_wlan_tx_frames (webs_t wp,char *args);
extern void return_wlan_rx_frames (webs_t wp,char *args);
extern void return_wan_ifconfig_info(webs_t wp,char *args);
extern void return_lan_ifconfig_info(webs_t wp,char *args);
extern void return_wlan_ifconfig_info(webs_t wp,char *args);
//5G interface 2
extern void return_wlan1_ifconfig_info(webs_t wp,char *args);
#ifdef USER_WL_ATH_5GHZ
extern void return_wlan1_mac_addr(webs_t wp,char *args);
#endif
#if defined(DIR865) || defined(EXVERSION_NNA)
extern void return_language(webs_t wp,char *args);
#endif
//#ifdef ATH_GUEST_ZONE
extern void return_guestzone_mac(webs_t wp,char *args);
//#endif
extern void return_fixip_connection_status(webs_t wp,char *args);
extern void return_dhcpc_connection_status(webs_t wp,char *args);
extern void return_pppoe_00_connection_status(webs_t wp,char *args);
extern void return_pppoe_01_connection_status(webs_t wp,char *args);
extern void return_pppoe_02_connection_status(webs_t wp,char *args);
extern void return_pppoe_03_connection_status(webs_t wp,char *args);
extern void return_pppoe_04_connection_status(webs_t wp,char *args);
extern void return_l2tp_connection_status(webs_t wp,char *args);
extern void return_pptp_connection_status(webs_t wp,char *args);
#ifdef CONFIG_USER_BIGPOND//no used
extern void return_bigpond_connection_status(webs_t wp,char *args);
#endif
extern void return_get_current_user(webs_t wp,char *args);
extern void return_get_error_message(webs_t wp,char *args);
extern void return_get_error_setting(webs_t wp,char *args);
extern void return_mac_clone_addr(webs_t wp,char *args);
extern void return_mac_default_addr(webs_t wp,char *args);
extern void return_ping_ipaddr(webs_t wp,char *args);
extern void return_ping_result(webs_t wp,char *args);
extern void return_ping_result_display(webs_t wp,char *args);
#ifdef RADVD
extern void return_ping6_ipaddr(webs_t wp,char *args);
extern void return_ping6_interface(webs_t wp,char *args);
extern void return_ping6_size(webs_t wp,char *args);
extern void return_ping6_count(webs_t wp,char *args);
extern void return_ping6_result(webs_t wp,char *args);
extern void return_ping6_test_result(webs_t wp,char *args);
extern void return_6to4_tunnel_address(webs_t wp,char *args);
extern void return_6to4_lan_address(webs_t wp,char *args);
#endif
#ifdef IPV6_6RD
extern void return_6rd_tunnel_address(webs_t wp,char *args);
extern void return_6rd_lan_address(webs_t wp,char *args);
#endif
#if 0//old systime
extern char *trnasfer_system_time(char *sysTime,char *sys_time);
#endif
extern void return_system_time(webs_t wp,char *args);
extern void return_wlan0_channel_list(webs_t wp,char *args);
//5G interface 2
extern void return_wlan1_channel_list(webs_t wp,char *args);
extern void return_wan_port_status(webs_t wp,char *args);
extern void return_wan_port_speed_duplex(webs_t wp,char *args);
extern void return_lan_port_status_00(webs_t wp,char *args);
extern void return_lan_port_status_01(webs_t wp,char *args);
extern void return_lan_port_status_02(webs_t wp,char *args);
extern void return_lan_port_status_03(webs_t wp,char *args);
extern void return_lan_port_speed_duplex_00(webs_t wp,char *args);
extern void return_lan_port_speed_duplex_01(webs_t wp,char *args);
extern void return_lan_port_speed_duplex_02(webs_t wp,char *args);
extern void return_lan_port_speed_duplex_03(webs_t wp,char *args);
extern void return_log_total_page(webs_t wp,char *args);
extern void return_log_current_page(webs_t wp,char *args);
extern void return_current_channel(webs_t wp,char *args);
extern void return_usb_test_result(webs_t wp,char *args);
extern void return_lan_collisions(webs_t wp,char *args);
extern void return_wan_collisions(webs_t wp,char *args);
extern void return_wlan_collisions(webs_t wp,char *args);
extern void return_dhcpc_obtain_time(webs_t wp,char *args);
extern void return_dhcpc_expire_time(webs_t wp,char *args);
extern void return_lan_uptime(webs_t wp,char *args);
extern void return_wan_uptime(webs_t wp,char *args);
extern void return_wlan_uptime(webs_t wp,char *args);
extern void return_pppoe_00_uptime(webs_t wp,char *args);
extern void return_internet_connect_type(webs_t wp,char *args);
extern void return_internet_connect(webs_t wp,char *args);
extern void return_ddns_status(webs_t wp,char *args);
extern void return_wps_generate_pin_by_random (webs_t wp,char *args);
extern void return_wps_generate_pin_by_mac (webs_t wp,char *args);
extern void return_wps_push_button_result (webs_t wp,char *args);
extern void return_wps_sta_enrollee_pin (webs_t wp,char *args);
extern void return_online_firmware_check(webs_t wp,char *args);
extern void return_internetonline_check(webs_t wp,char *args);
#ifdef CONFIG_USER_TC
extern void return_qos_detected_line_type(webs_t wp,char *args);
extern void return_measured_uplink_speed (webs_t wp,char *args);
extern void return_if_measuring_uplink_now (webs_t wp,char *args);
extern void return_if_wan_ip_obtained (webs_t wp,char *args);
#endif
extern void return_average_bytes(webs_t wp,char *args);
extern void return_dns_query_result(webs_t wp,char *args);
#ifdef IPv6_SUPPORT
extern void return_default_gateway(webs_t wp,char *args);
extern void return_get_ipv6_dns(webs_t wp,char *args);
extern void return_link_local_ip_w(webs_t wp,char *args);
extern void return_link_local_ip_l(webs_t wp,char *args);
extern void return_global_ip_w(webs_t wp,char *args);
extern void return_global_ip_l(webs_t wp,char *args);
extern void return_radvd_status(webs_t wp,char *args);
extern void return_ipv6_tunnel_lifetime(webs_t wp,char *args);
#ifdef IPV6_DSLITE
extern void return_aftr_address(webs_t wp,char *args);
#endif
#endif
#ifdef XML_AGENT
extern void return_login_salt_random(webs_t wp,char *args);
#endif
#ifdef GUI_LOGIN_ALERT
extern void return_gui_login_alert(webs_t wp,char *args);
#endif
/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
extern void return_graph_auth_id(webs_t wp,char *args);
#endif
#if defined(HAVE_HTTPS)
extern void return_support_https(webs_t wp,char *args);
#endif
void return_admin_password(webs_t wp,char *args);
extern void return_wep_key1(webs_t wp,char *args);
extern void return_wpa_psk_ps(webs_t wp,char *args);
extern void return_radius0_ps(webs_t wp,char *args);
extern void return_radius1_ps(webs_t wp,char *args);
extern void return_vap1_wep_key1(webs_t wp,char *args);
extern void return_vap1_wpa_psk_ps(webs_t wp,char *args);
extern void return_vap1_radius0_ps(webs_t wp,char *args);
extern void return_vap1_radius1_ps(webs_t wp,char *args);
extern void return_russia_wan_phy_ipaddr(webs_t wp,char *args);
#ifdef CONFIG_USER_WEB_FILE_ACCESS
extern void return_storage_get_number_of_dev(webs_t wp,char *args);
extern void return_storage_get_space(webs_t wp,char *args);
extern void return_storage_wan_ip(webs_t wp,char *args);
#endif
#ifdef USER_WL_ATH_5GHZ
extern void return_wlan1_wep_key1(webs_t wp,char *args);
extern void return_wlan1_wpa_psk_ps(webs_t wp,char *args);
extern void return_wlan1_radius0_ps(webs_t wp,char *args);
extern void return_wlan1_radius1_ps(webs_t wp,char *args);
extern void return_wlan1_vap1_wep_key1(webs_t wp,char *args);
extern void return_wlan1_vap1_wpa_psk_ps(webs_t wp,char *args);
extern void return_wlan1_vap1_radius0_ps(webs_t wp,char *args);
extern void return_wlan1_vap1_radius1_ps(webs_t wp,char *args);
#endif
extern void return_reboot_needed(webs_t wp,char *args);
/* CmoGetStatus */
/**************************************************************/
/**************************************************************/

/*
*	Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetList can used the args.
*   Notice :
*/
/* CmoGetList */
/**************************************************************/
/**************************************************************/
extern void return_routing_table(webs_t wp,char *args);
extern void return_dhcpd_leased_table(webs_t wp,char *args);
extern void return_wireless_ap_scan_list(webs_t wp,char *args);
extern void return_upnp_portmap_table(webs_t wp,char *args);
extern void return_vlan_table(webs_t wp,char *args);
extern void return_napt_session_table(webs_t wp,char *args);
extern void return_internet_session_table(webs_t wp,char *args);
extern void return_igmp_table(webs_t wp,char *args);
#ifdef CONFIG_USER_IP_LOOKUP
extern void return_client_list_table(webs_t wp,char *args);
#endif
#ifdef IPv6_SUPPORT
extern void return_ipv6_client_list_table(webs_t wp,char *args);
extern void return_ipv6_routing_table(webs_t wp,char *args);
#endif

/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
extern void return_lp_version(webs_t wp);
extern void return_language_mtdblock_check(webs_t wp,char *args);
extern void return_lp_date(webs_t wp);
extern void return_online_lp_check(webs_t wp);
#endif
#ifdef USB_STORAGE_HTTP_ENABLE
extern void return_usb_storage_list(webs_t wp,char *args);		
#endif//USB_STORAGE_HTTP_ENABLE	 
/* CmoGetList */
/**************************************************************/
/**************************************************************/

/* no find used */
/**************************************************************/
/**************************************************************/
//extern void return_wireless_status(webs_t wp);
extern void return_firmware_upgrade(webs_t wp);
extern void return_configuration_upload(webs_t wp);
extern void return_wlan_sta(webs_t wp);
/* no find used */
/**************************************************************/
/**************************************************************/


/* export for pure network restart.cgi */
int redirect_count_down_page(FILE *stream,int type,char *count_down_sec,int ret_s);

/* apply_XXX state */
/**************************************************************/
/**************************************************************/
extern void set_nvram_config(void);
extern void commit_nvram_config(void);
extern void discard_nvram_config(void);
extern char *get_old_value_by_file(char *path,char *name);
//extern char *get_old_lan_ipaddr_by_file(char *path);
extern int get_apply_config_flag(void);
/* apply_XXX state */
/**************************************************************/
/**************************************************************/
#endif
