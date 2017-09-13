#ifdef WEBS
#include <webs.h>
#include <uemf.h>
#include <ej.h>
#else /* !WEBS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <httpd.h>
#include <log.h>
#include <linux_vct.h>
#include <sutil.h>
#include <shvar.h>
#include <syslog.h>
#include <time.h>
#endif /* WEBS */

#ifdef IPV6_6RD
#include <linux/if.h>
#include <linux/sockios.h>
#endif

#ifdef PURE_NETWORK_ENABLE
#include <pure.h>
#endif

/* Required for hash table headers*/
#if defined(linux)
/* Use SVID search */

#endif

#include <nvram.h>
#include <httpd_util.h>

/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
#include <sys/time.h>
#include "authgraph.h"
#endif

/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
#include        <fcntl.h>
#include "lp.h"
#include "project.h"
#endif

/* -------------------------- */
#ifdef HTTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#ifdef HTTPD_USED_MUTIL_AUTH
authorization_t auth_login[MAX_AUTH_NUMBER];
#else
authorization_t auth_login;
#endif
ping_ipaddr_t ping;
chklst_t chklst;
log_info_t logs_info;
wan_mac_clone_t con_info;
usb_test_t usb_test;
wps_pin_t enrollee;

void reset_dhcpd_resevation_rule(void);
static char wan_status[16];
static char wan_proto[8];
extern int reset_lan_tx_flag;
int inner_lan_tx_flag = 0;
extern int reset_wan_tx_flag;
int inner_wan_tx_flag = 0;
extern int reset_wlan_tx_flag;
int inner_wlan_tx_flag = 0;
extern int reset_lan_rx_flag;
int inner_lan_rx_flag = 0;
extern int reset_wan_rx_flag;
int inner_wan_rx_flag = 0;
extern int reset_wlan_rx_flag;
int inner_wlan_rx_flag = 0;
#define MAX_INPUT_LEN 1024

/* apply_XXX state flag */
int apply_config_flag = 0;
int gTotalClientNum = 0;

int webserver_reboot_needed = 0;

/* used by set_basic_api() */
/**************************************************************/
/**************************************************************/
mapping_t ui_to_nvram[]={
        // D0 ~ D6
        {"hostname"},
        {"system_time"},
        {"countdown_time"},
        {"default_html"},
        {"admin_username"},
        {"admin_password"},
        {"user_username"},
        {"user_password"},
/*
*       Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
        {"sp_admin_username"},
#endif
/*
*       Date: 2009-08-04
*   Name: Ken Chiang
*   Reason: Modify using a nvram value to setting http timeout value.
*   Notice :
*/
#ifdef HTTP_TIMEOUT
        {"http_timeout_value"},
#endif
        {"default_html"},
        {"media_server_enable"},
        {"media_server_name"},
        {"blank_state"},
        {"lan_mac"},
        {"wan_mac"},
        {"lan_eth"},
        {"lan_bridge"},
        {"wan_eth"},
        {"lan_ipaddr"},
        {"lan_netmask"},
        {"lan_gateway"},
        {"lan_device_name"},
        {"dhcpd_enable"},
        {"dhcpd_start"},
        {"dhcpd_end"},
        {"dhcpd_lease_time"},
        {"dhcpd_domain_name"},
        {"dhcpd_reservation"},
        {"dhcpd_reserve_00"},
        {"dhcpd_reserve_01"},
        {"dhcpd_reserve_02"},
        {"dhcpd_reserve_03"},
        {"dhcpd_reserve_04"},
        {"dhcpd_reserve_05"},
        {"dhcpd_reserve_06"},
        {"dhcpd_reserve_07"},
        {"dhcpd_reserve_08"},
        {"dhcpd_reserve_09"},
        {"dhcpd_reserve_10"},
        {"dhcpd_reserve_11"},
        {"dhcpd_reserve_12"},
        {"dhcpd_reserve_13"},
        {"dhcpd_reserve_14"},
        {"dhcpd_reserve_15"},
        {"dhcpd_reserve_16"},
        {"dhcpd_reserve_17"},
        {"dhcpd_reserve_18"},
        {"dhcpd_reserve_19"},
        {"dhcpd_reserve_20"},
        {"dhcpd_reserve_21"},
        {"dhcpd_reserve_22"},
        {"dhcpd_reserve_23"},
        {"dhcpd_reserve_24"},
        {"dhcpd_netbios_enable"},
        {"dhcpd_netbios_learn"},
        {"dhcpd_static_wins_server"},
        {"dhcpd_static_node_type"},
        {"dhcpd_static_scope"},
        {"dhcpd_dynamic_wins_server"},
        {"dhcpd_dynamic_node_type"},
        {"dhcpd_dynamic_scope"},
        {"dhcpd_always_bcast"},
        {"dhcpc_use_ucast"},
        {"dns_relay"},
#ifdef OPENDNS
        {"opendns_enable"},
// 2010.11.15 new add
        {"opendns_mode"},
        {"opendns_deviceid"},
//
        {"opendns_dns1"},
        {"opendns_dns2"},
#endif
        {"dns_server"},
        {"wan_mtu"},
        {"wan_specify_dns"},
        {"wan_primary_dns"},
        {"wan_secondary_dns"},
        {"wan_third_dns"},
        {"wan_proto"},
        {"wan_static_ipaddr"},
        {"wan_static_netmask"},
        {"wan_static_gateway"},
        {"dhcpc_enable"},
        {"mac_clone_addr"},
        // D-7
#ifdef MPPPOE
        {"wan_pppoe_main_session"},
        {"wan_pppoe_mtu_00"},
#else
    {"wan_pppoe_mtu"},
#endif
        {"wan_pppoe_enable_00"},
        {"wan_pppoe_dynamic_00"},
        {"wan_pppoe_connect_mode_00"},
        {"wan_pppoe_username_00"},
        {"wan_pppoe_password_00"},
        {"wan_pppoe_service_00"},
        {"wan_pppoe_max_idle_time_00"},
        {"wan_pppoe_connect_mode_00"},
        {"wan_pppoe_ipaddr_00"},
        {"wan_pppoe_netmask_00"},
        {"wan_pppoe_gateway_00"},
        {"wan_pppoe_primary_dns_00"},
        {"wan_pppoe_secondary_dns_00"},
//Albert add
        {"wan_pppoe_ac_00"},
//
#ifdef MPPPOE
        {"wan_pppoe_enable_01"},
        {"wan_pppoe_dynamic_01"},
        {"wan_pppoe_username_01"},
        {"wan_pppoe_password_01"},
        {"wan_pppoe_service_01"},
        {"wan_pppoe_max_idle_time_01"},
        {"wan_pppoe_connect_mode_01"},
        {"wan_pppoe_ipaddr_01"},
        {"wan_pppoe_netmask_01"},
        {"wan_pppoe_gateway_01"},
        {"wan_pppoe_mtu_01"},
        {"wan_pppoe_primary_dns_01"},
    {"wan_pppoe_secondary_dns_01"},
        {"wan_pppoe_enable_02"},
        {"wan_pppoe_dynamic_02"},
        {"wan_pppoe_username_02"},
        {"wan_pppoe_password_02"},
        {"wan_pppoe_service_02"},
        {"wan_pppoe_max_idle_time_02"},
        {"wan_pppoe_connect_mode_02"},
        {"wan_pppoe_ipaddr_02"},
        {"wan_pppoe_netmask_02"},
        {"wan_pppoe_gateway_02"},
        {"wan_pppoe_mtu_02"},
    {"wan_pppoe_primary_dns_02"},
    {"wan_pppoe_secondary_dns_02"},
        {"wan_pppoe_enable_03"},
        {"wan_pppoe_dynamic_03"},
        {"wan_pppoe_username_03"},
        {"wan_pppoe_password_03"},
        {"wan_pppoe_service_03"},
        {"wan_pppoe_max_idle_time_03"},
        {"wan_pppoe_connect_mode_03"},
        {"wan_pppoe_ipaddr_03"},
        {"wan_pppoe_netmask_03"},
        {"wan_pppoe_gateway_03"},
        {"wan_pppoe_mtu_03"},
    {"wan_pppoe_primary_dns_03"},
    {"wan_pppoe_secondary_dns_03"},
        {"wan_pppoe_enable_04"},
        {"wan_pppoe_dynamic_04"},
        {"wan_pppoe_username_04"},
        {"wan_pppoe_password_04"},
        {"wan_pppoe_service_04"},
        {"wan_pppoe_max_idle_time_04"},
        {"wan_pppoe_connect_mode_04"},
        {"wan_pppoe_ipaddr_04"},
        {"wan_pppoe_netmask_04"},
        {"wan_pppoe_gateway_04"},
        {"wan_pppoe_mtu_04"},
    {"wan_pppoe_primary_dns_04"},
    {"wan_pppoe_secondary_dns_04"},
#endif
        {"wan_pppoe_russia_enable"},
        {"wan_pppoe_russia_dynamic"},
        {"wan_pppoe_russia_ipaddr"},
        {"wan_pppoe_russia_netmask"},
        {"wan_pppoe_russia_gateway"},
        {"wan_pppoe_russia_primary_dns"},
        {"wan_pppoe_russia_secondary_dns"},
#ifdef MPPPOE
        {"wan_pppoe_specify_dns_00"},
        {"wan_pppoe_specify_dns_01"},
        {"wan_pppoe_specify_dns_02"},
        {"wan_pppoe_specify_dns_03"},
        {"wan_pppoe_specify_dns_04"},
#endif
        // D-8
        {"wan_pptp_dynamic"},
        {"wan_pptp_username"},
        {"wan_pptp_password"},
        {"wan_pptp_max_idle_time"},
        {"wan_pptp_connect_mode"},
        {"wan_pptp_netmask"},
        {"wan_pptp_ipaddr"},
        {"wan_pptp_gateway"},
        {"wan_pptp_server_ip"},
        {"wan_pptp_mtu"},
        {"wan_pptp_connection_id"},
        {"support_pptp_mppe"},
#ifdef RPPPOE
        {"wan_pptp_russia_enable"},
#endif
        // D-9
        {"wan_l2tp_dynamic"},
        {"wan_l2tp_username"},
        {"wan_l2tp_password"},
        {"wan_l2tp_max_idle_time"},
        {"wan_l2tp_connect_mode"},
        {"wan_l2tp_ipaddr"},
        {"wan_l2tp_netmask"},
        {"wan_l2tp_gateway"},
        {"wan_l2tp_server_ip"},
        {"wan_l2tp_mtu"},
        {"support_l2tp_mppe"},
#ifdef RPPPOE
        {"wan_l2tp_russia_enable"},
#endif
/*
*       Date: 2009-1-19
*       Name: Ken_Chiang
*       Reason: modify for 3g usb client card to used.
*       Notice :
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
        //D-47
        {"usb3g_country"},
        {"usb3g_isp"},
        {"usb3g_username"},
        {"usb3g_password"},
        {"usb3g_dial_num"},
        {"usb3g_auth"},
        {"usb3g_apn_name"},
        {"usb3g_reconnect_mode"},
        {"usb3g_max_idle_time"},
        {"usb3g_wan_mtu"},
        {"usb3g_pin"},
#endif
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        {"usb3g_phone_reconnect_mode"},
#endif
        // D-11
        {"schedule_rule_00"},
        {"schedule_rule_01"},
        {"schedule_rule_02"},
        {"schedule_rule_03"},
        {"schedule_rule_04"},
        {"schedule_rule_05"},
        {"schedule_rule_06"},
        {"schedule_rule_07"},
        {"schedule_rule_08"},
        {"schedule_rule_09"},
        {"schedule_rule_10"},
        {"schedule_rule_11"},
        {"schedule_rule_12"},
        {"schedule_rule_13"},
        {"schedule_rule_14"},
        {"schedule_rule_15"},
        {"schedule_rule_16"},
        {"schedule_rule_17"},
        {"schedule_rule_18"},
        {"schedule_rule_19"},
        {"schedule_time_format"}, //NickChou 20100203, for Dlink 12/24 hour
        // d-12
        {"vs_rule_00"},
        {"vs_rule_01"},
        {"vs_rule_02"},
        {"vs_rule_03"},
        {"vs_rule_04"},
        {"vs_rule_05"},
        {"vs_rule_06"},
        {"vs_rule_07"},
        {"vs_rule_08"},
        {"vs_rule_09"},
        {"vs_rule_10"},
        {"vs_rule_11"},
        {"vs_rule_12"},
        {"vs_rule_13"},
        {"vs_rule_14"},
        {"vs_rule_15"},
        {"vs_rule_16"},
        {"vs_rule_17"},
        {"vs_rule_18"},
        {"vs_rule_19"},
        {"vs_rule_20"},
        {"vs_rule_21"},
        {"vs_rule_22"},
        {"vs_rule_23"},
        // D-13
        {"port_forward_both_00"},
        {"port_forward_both_01"},
        {"port_forward_both_02"},
        {"port_forward_both_03"},
        {"port_forward_both_04"},
        {"port_forward_both_05"},
        {"port_forward_both_06"},
        {"port_forward_both_07"},
        {"port_forward_both_08"},
        {"port_forward_both_09"},
        {"port_forward_both_10"},
        {"port_forward_both_11"},
        {"port_forward_both_12"},
        {"port_forward_both_13"},
        {"port_forward_both_14"},
        {"port_forward_both_15"},
        {"port_forward_both_16"},
        {"port_forward_both_17"},
        {"port_forward_both_18"},
        {"port_forward_both_19"},
        {"port_forward_both_20"},
        {"port_forward_both_21"},
        {"port_forward_both_22"},
        {"port_forward_both_23"},
        {"port_forward_both_24"},
        // D-14
        {"application_00"},
        {"application_01"},
        {"application_02"},
        {"application_03"},
        {"application_04"},
        {"application_05"},
        {"application_06"},
        {"application_07"},
        {"application_08"},
        {"application_09"},
        {"application_10"},
        {"application_11"},
        {"application_12"},
        {"application_13"},
        {"application_14"},
        {"application_15"},
        {"application_16"},
        {"application_17"},
        {"application_18"},
        {"application_19"},
        {"application_20"},
        {"application_21"},
        {"application_22"},
        {"application_23"},
        {"application_24"},
        // D-15
        {"mac_filter_type"},
        {"mac_filter_00"},
        {"mac_filter_01"},
        {"mac_filter_02"},
        {"mac_filter_03"},
        {"mac_filter_04"},
        {"mac_filter_05"},
        {"mac_filter_06"},
        {"mac_filter_07"},
        {"mac_filter_08"},
        {"mac_filter_09"},
        {"mac_filter_10"},
        {"mac_filter_11"},
        {"mac_filter_12"},
        {"mac_filter_13"},
        {"mac_filter_14"},
        {"mac_filter_15"},
        {"mac_filter_16"},
        {"mac_filter_17"},
        {"mac_filter_18"},
        {"mac_filter_19"},
        {"mac_filter_20"},
        {"mac_filter_21"},
        {"mac_filter_22"},
        {"mac_filter_23"},
        {"mac_filter_24"},

        {"wlan1_mac_filter_type"},
        {"wlan1_mac_filter_00"},
        {"wlan1_mac_filter_01"},
        {"wlan1_mac_filter_02"},
        {"wlan1_mac_filter_03"},
        {"wlan1_mac_filter_04"},
        {"wlan1_mac_filter_05"},
        {"wlan1_mac_filter_06"},
        {"wlan1_mac_filter_07"},
        {"wlan1_mac_filter_08"},
        {"wlan1_mac_filter_09"},
        {"wlan1_mac_filter_10"},
        {"wlan1_mac_filter_11"},
        {"wlan1_mac_filter_12"},
        {"wlan1_mac_filter_13"},
        {"wlan1_mac_filter_14"},
        {"wlan1_mac_filter_15"},
        {"wlan1_mac_filter_16"},
        {"wlan1_mac_filter_17"},
        {"wlan1_mac_filter_18"},
        {"wlan1_mac_filter_19"},
        {"wlan1_mac_filter_20"},
        {"wlan1_mac_filter_21"},
        {"wlan1_mac_filter_22"},
        {"wlan1_mac_filter_23"},
        {"wlan1_mac_filter_24"},

        // D-16
        {"filter_enable"},
        {"filter_00"},
        {"filter_01"},
        {"filter_02"},
        {"filter_03"},
        {"filter_04"},
        {"filter_05"},
        {"filter_06"},
        {"filter_07"},
        {"filter_08"},
        {"filter_09"},
        {"filter_10"},
        {"filter_11"},
        {"filter_12"},
        {"filter_13"},
        {"filter_14"},
        {"filter_15"},
        {"filter_16"},
        {"filter_17"},
        {"filter_18"},
        {"filter_19"},
        // D-17
        {"url_domain_filter_type"},
        {"url_domain_filter_00"},
        {"url_domain_filter_01"},
        {"url_domain_filter_02"},
        {"url_domain_filter_03"},
        {"url_domain_filter_04"},
        {"url_domain_filter_05"},
        {"url_domain_filter_06"},
        {"url_domain_filter_07"},
        {"url_domain_filter_08"},
        {"url_domain_filter_09"},
        {"url_domain_filter_10"},
        {"url_domain_filter_11"},
        {"url_domain_filter_12"},
        {"url_domain_filter_13"},
        {"url_domain_filter_14"},
        {"url_domain_filter_15"},
        {"url_domain_filter_16"},
        {"url_domain_filter_17"},
        {"url_domain_filter_18"},
        {"url_domain_filter_19"},
        {"url_domain_filter_20"},
        {"url_domain_filter_21"},
        {"url_domain_filter_22"},
        {"url_domain_filter_23"},
        {"url_domain_filter_24"},
        {"url_domain_filter_25"},
        {"url_domain_filter_26"},
        {"url_domain_filter_27"},
        {"url_domain_filter_28"},
        {"url_domain_filter_29"},
        {"url_domain_filter_30"},
        {"url_domain_filter_31"},
        {"url_domain_filter_32"},
        {"url_domain_filter_33"},
        {"url_domain_filter_34"},
        {"url_domain_filter_35"},
        {"url_domain_filter_36"},
        {"url_domain_filter_37"},
        {"url_domain_filter_38"},
        {"url_domain_filter_39"},
        {"url_domain_filter_enable_00"},
        {"url_domain_filter_enable_01"},
        {"url_domain_filter_enable_02"},
        {"url_domain_filter_enable_03"},
        {"url_domain_filter_enable_04"},
        {"url_domain_filter_enable_05"},
        {"url_domain_filter_enable_06"},
        {"url_domain_filter_enable_07"},
        {"url_domain_filter_enable_08"},
        {"url_domain_filter_enable_09"},
        {"url_domain_filter_enable_10"},
        {"url_domain_filter_enable_11"},
        {"url_domain_filter_enable_12"},
        {"url_domain_filter_enable_13"},
        {"url_domain_filter_enable_14"},
        {"url_domain_filter_enable_15"},
        {"url_domain_filter_enable_16"},
        {"url_domain_filter_enable_17"},
        {"url_domain_filter_enable_18"},
        {"url_domain_filter_enable_19"},
        {"url_domain_filter_enable_20"},
        {"url_domain_filter_enable_21"},
        {"url_domain_filter_enable_22"},
        {"url_domain_filter_enable_23"},
        {"url_domain_filter_enable_24"},
        {"url_domain_filter_enable_25"},
        {"url_domain_filter_enable_26"},
        {"url_domain_filter_enable_27"},
        {"url_domain_filter_enable_28"},
        {"url_domain_filter_enable_29"},
        {"url_domain_filter_enable_30"},
        {"url_domain_filter_enable_31"},
        {"url_domain_filter_enable_32"},
        {"url_domain_filter_enable_33"},
        {"url_domain_filter_enable_34"},
        {"url_domain_filter_enable_35"},
        {"url_domain_filter_enable_36"},
        {"url_domain_filter_enable_37"},
        {"url_domain_filter_enable_38"},
        {"url_domain_filter_enable_39"},
        {"url_domain_filter_schedule_00"},
        {"url_domain_filter_schedule_01"},
        {"url_domain_filter_schedule_02"},
        {"url_domain_filter_schedule_03"},
        {"url_domain_filter_schedule_04"},
        {"url_domain_filter_schedule_05"},
        {"url_domain_filter_schedule_06"},
        {"url_domain_filter_schedule_07"},
        {"url_domain_filter_schedule_08"},
        {"url_domain_filter_schedule_09"},
        {"url_domain_filter_schedule_10"},
        {"url_domain_filter_schedule_11"},
        {"url_domain_filter_schedule_12"},
        {"url_domain_filter_schedule_13"},
        {"url_domain_filter_schedule_14"},
        {"url_domain_filter_schedule_15"},
        {"url_domain_filter_schedule_16"},
        {"url_domain_filter_schedule_17"},
        {"url_domain_filter_schedule_18"},
        {"url_domain_filter_schedule_19"},
        {"url_domain_filter_schedule_20"},
        {"url_domain_filter_schedule_21"},
        {"url_domain_filter_schedule_22"},
        {"url_domain_filter_schedule_23"},
        {"url_domain_filter_schedule_24"},
        {"url_domain_filter_schedule_25"},
        {"url_domain_filter_schedule_26"},
        {"url_domain_filter_schedule_27"},
        {"url_domain_filter_schedule_28"},
        {"url_domain_filter_schedule_29"},
        {"url_domain_filter_schedule_30"},
        {"url_domain_filter_schedule_31"},
        {"url_domain_filter_schedule_32"},
        {"url_domain_filter_schedule_33"},
        {"url_domain_filter_schedule_34"},
        {"url_domain_filter_schedule_35"},
        {"url_domain_filter_schedule_36"},
        {"url_domain_filter_schedule_37"},
        {"url_domain_filter_schedule_38"},
        {"url_domain_filter_schedule_39"},
        {"url_domain_filter_trust_ip_00"},
        {"url_domain_filter_trust_ip_01"},
        {"url_domain_filter_trust_ip_02"},
        {"url_domain_filter_trust_ip_03"},
        {"url_domain_filter_trust_ip_04"},
        {"url_domain_filter_trust_ip_05"},
        {"url_domain_filter_trust_ip_06"},
        {"url_domain_filter_trust_ip_07"},
        {"url_domain_filter_trust_ip_08"},
        {"url_domain_filter_trust_ip_09"},
        {"url_domain_filter_trust_ip_10"},
        {"url_domain_filter_trust_ip_11"},
        {"url_domain_filter_trust_ip_12"},
        {"url_domain_filter_trust_ip_13"},
        {"url_domain_filter_trust_ip_14"},
        {"url_domain_filter_trust_ip_15"},
        {"url_domain_filter_trust_ip_16"},
        {"url_domain_filter_trust_ip_17"},
        {"url_domain_filter_trust_ip_18"},
        {"url_domain_filter_trust_ip_19"},
        {"url_domain_filter_trust_ip_20"},
        {"url_domain_filter_trust_ip_21"},
        {"url_domain_filter_trust_ip_22"},
        {"url_domain_filter_trust_ip_23"},
        {"url_domain_filter_trust_ip_24"},
        {"url_domain_filter_trust_ip_25"},
        {"url_domain_filter_trust_ip_26"},
        {"url_domain_filter_trust_ip_27"},
        {"url_domain_filter_trust_ip_28"},
        {"url_domain_filter_trust_ip_29"},
        {"url_domain_filter_trust_ip_30"},
        {"url_domain_filter_trust_ip_31"},
        {"url_domain_filter_trust_ip_32"},
        {"url_domain_filter_trust_ip_33"},
        {"url_domain_filter_trust_ip_34"},
        {"url_domain_filter_trust_ip_35"},
        {"url_domain_filter_trust_ip_36"},
        {"url_domain_filter_trust_ip_37"},
        {"url_domain_filter_trust_ip_38"},
        {"url_domain_filter_trust_ip_39"},
        // D-18
        {"rip_enable"},
        {"rip_rx_version"},
        {"rip_tx_version"},
        // D-19
/* jimmy added for dhcp option 249 enable/disable */
#if defined (UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
        {"classless_static_route"},
#endif
/* ---------------------------------------------- */
        {"static_routing_00"},
        {"static_routing_01"},
        {"static_routing_02"},
        {"static_routing_03"},
        {"static_routing_04"},
        {"static_routing_05"},
        {"static_routing_06"},
        {"static_routing_07"},
        {"static_routing_08"},
        {"static_routing_09"},
        {"static_routing_10"},
        {"static_routing_11"},
        {"static_routing_12"},
        {"static_routing_13"},
        {"static_routing_14"},
        {"static_routing_15"},
        {"static_routing_16"},
        {"static_routing_17"},
        {"static_routing_18"},
        {"static_routing_19"},
        {"static_routing_20"},
        {"static_routing_21"},
        {"static_routing_22"},
        {"static_routing_23"},
        {"static_routing_24"},
        {"static_routing_25"},
        {"static_routing_26"},
        {"static_routing_27"},
        {"static_routing_28"},
        {"static_routing_29"},
        {"static_routing_30"},
        {"static_routing_31"},
        {"static_routing_32"},
        {"static_routing_33"},
        {"static_routing_34"},
        {"static_routing_35"},
        {"static_routing_36"},
        {"static_routing_37"},
        {"static_routing_38"},
        {"static_routing_39"},
        {"static_routing_40"},
        {"static_routing_41"},
        {"static_routing_42"},
        {"static_routing_43"},
        {"static_routing_44"},
        {"static_routing_45"},
        {"static_routing_46"},
        {"static_routing_47"},
        {"static_routing_48"},
        {"static_routing_49"},
        {"static_routing_ipv6_00"},
        {"static_routing_ipv6_01"},
        {"static_routing_ipv6_02"},
        {"static_routing_ipv6_03"},
        {"static_routing_ipv6_04"},
        {"static_routing_ipv6_05"},
        {"static_routing_ipv6_06"},
        {"static_routing_ipv6_07"},
        {"static_routing_ipv6_08"},
        {"static_routing_ipv6_09"},
         // D-20
        {"time_zone"},
        {"time_daylight_saving_enable"},
        {"time_daylight_saving_start_month"},
        {"time_daylight_saving_start_week"},
        {"time_daylight_saving_start_day_of_week"},
        {"time_daylight_saving_start_time"},
        {"time_daylight_saving_end_month"},
        {"time_daylight_saving_end_week"},
        {"time_daylight_saving_end_day_of_week"},
        {"time_daylight_saving_end_time"},
        {"time_daylight_offset"},
        {"ntp_client_enable"},
        {"ntp_sync_interval"},
        {"ntp_server"},
        {"time_zone_area"},
        // D-21
        {"wan_port_ping_response_enable"},
        {"wan_port_ping_response_inbound_filter"},
        {"remote_http_management_inbound_filter"},
        {"wan_port_ping_response_start_ip"},
        {"wan_port_ping_response_end_ip"},
        {"remote_http_management_enable"},
        {"remote_http_management_port"},
        {"remote_http_management_ipaddr_range"},
        {"remote_http_management_schedule_name"},
#if defined(HAVE_HTTPS)
        {"https_config"},
#endif
        {"upnp_enable"},
        {"upnp_advertisement_period"},
        {"upnp_advertisement_ttl"},
        {"wan_port_speed"},
        {"multicast_stream_enable"},
//ipv6 multicast
        {"ipv6_multicast_stream_enable"},

        {"anti_spoof_check"},
        {"ident_enable"},
        // D-22
        {"ddns_enable"},
        {"ddns_type"},
        {"ddns_hostname"},
        {"ddns_username"},
        {"ddns_password"},
        {"ddns_wildcards_enable"},
        {"ddns_dyndns_kinds"},
        {"ddns_sync_interval"},
        {"ddns_wan_ip_old"},
        {"ddns_hostname_old"},
        // D-23
        {"dmz_enable"},
        {"dmz_ipaddr"},
        {"dmz_schedule"},
        // D-24
        {"pptp_pass_through"},
        {"ipsec_pass_through"},
        {"l2tp_pass_through"},
        {"pppoe_pass_through"},
        {"alg_sip"},
        {"alg_rtsp"},
        // D-25
        {"log_system_activity"},
        {"log_debug_information"},
        {"log_attacks"},
        {"log_dropped_packets"},
        {"log_notice"},
        {"log_email_sender"},
  {"log_email_recipien"},
        {"log_email_server"},
        {"log_email_server_port"},
        {"log_email_username"},
        {"log_email_password"},
        {"log_per_page"},
        {"log_current_page"},
        {"log_response_type"},
        {"log_email_block_enable"},
        {"log_email_enable"},
        {"log_email_schedule"},
  {"log_email_auth"},
  {"syslog_server"},
        // D-26
        {"wlan0_op_mode"},
        {"wlan0_enable"},
        // D-29
        {"model_name"},
        //Nickchou for USB option
        {"usb_type"},
        {"netusb_guest_zone"},
//5G
        {"wlan0_radio_mode"},
        {"wlan0_vap1_enable"},
        {"wlan0_vap2_enable"},
        {"wlan0_vap3_enable"},
        {"wlan0_vlan_enable"},
        {"wlan0_vlanid"},
        {"wlan0_vap1_vlanid"},
        {"wlan0_emi_test"},
        {"wlan0_ssid"},
        {"wlan0_vap1_ssid"},
        {"wlan0_vap2_ssid"},
        {"wlan0_vap3_ssid"},
        {"wlan0_domain"},
        {"wlan0_channel"},
        {"wlan0_auto_channel_enable"},
        {"wlan0_dot11_mode"},
        {"wlan0_xr_enable"},
        {"wlan0_adptive_radio_enable"},
        {"wlan0_ssid_broadcast"},
        {"wlan0_vap1_ssid_broadcast"},
        {"wlan0_vap2_ssid_broadcast"},
        {"wlan0_vap3_ssid_broadcast"},
        {"wlan0_idle_time"},
        {"wlan0_security"},
        {"wlan0_vap1_security"},
        {"wlan0_vap2_security"},
        {"wlan0_vap3_security"},
        {"wlan0_wep_auth"},
        {"wlan0_vap1_wep_auth"},
        {"wlan0_vap2_wep_auth"},
        {"wlan0_wep64_key_1"},
        {"wlan0_wep64_key_2"},
        {"wlan0_wep64_key_3"},
        {"wlan0_wep64_key_4"},
        {"wlan0_vap1_wep64_key_1"},
        {"wlan0_vap1_wep64_key_2"},
        {"wlan0_vap1_wep64_key_3"},
        {"wlan0_vap1_wep64_key_4"},
        {"wlan0_wep128_key_1"},
        {"wlan0_wep128_key_2"},
        {"wlan0_wep128_key_3"},
        {"wlan0_wep128_key_4"},
        {"wlan0_vap1_wep128_key_1"},
        {"wlan0_vap1_wep128_key_2"},
        {"wlan0_vap1_wep128_key_3"},
        {"wlan0_vap1_wep128_key_4"},
        {"wlan0_wep152_key_1"},
        {"wlan0_wep152_key_2"},
        {"wlan0_wep152_key_3"},
        {"wlan0_wep152_key_4"},
        {"wlan0_vap1_wep152_key_1"},
        {"wlan0_vap1_wep152_key_2"},
        {"wlan0_vap1_wep152_key_3"},
        {"wlan0_vap1_wep152_key_4"},
        {"wlan0_wep_display"},
        {"wlan0_vap1_wep_display"},
        {"wlan0_vap2_wep_display"},
        {"wlan0_vap3_wep_display"},
        {"wlan0_wep_default_key"},
        {"wlan0_vap1_wep_default_key"},
        {"wlan0_vap2_wep_default_key"},
        {"wlan0_vap3_wep_default_key"},
        {"wlan0_psk_cipher_type"},
        {"wlan0_vap1_psk_cipher_type"},
        {"wlan0_psk_pass_phrase"},
        {"wlan0_vap1_psk_pass_phrase"},
        {"wlan0_vap2_psk_pass_phrase"},
        {"wlan0_eap_radius_server_0"},
        {"wlan0_eap_radius_server_1"},
        {"wlan0_vap1_eap_radius_server_0"},
        {"wlan0_vap1_eap_radius_server_1"},
        {"wlan0_gkey_rekey_time"},
        {"wlan0_vap1_gkey_rekey_time"},
        {"wlan0_vap2_gkey_rekey_time"},
        {"wlan0_vap3_gkey_rekey_time"},
        {"wlan0_txpower"},
        {"wlan0_vap1_txpower"},
        {"wlan0_vap2_txpower"},
        {"wlan0_vap3_txpower"},
        {"wlan0_beacon_interval"},
        {"wlan0_vap1_beacon_interval"},
        {"wlan0_vap2_beacon_interval"},
        {"wlan0_vap3_beacon_interval"},
        {"wlan0_rts_threshold"},
        {"wlan0_vap1_rts_threshold"},
        {"wlan0_vap2_rts_threshold"},
        {"wlan0_vap3_rts_threshold"},
        {"wlan0_fragmentation"},
        {"wlan0_vap1_fragmentation"},
        {"wlan0_vap2_fragmentation"},
        {"wlan0_vap3_fragmentation"},
        {"wlan0_dtim"},
        {"wlan0_vap1_dtim"},
        {"wlan0_vap2_dtim"},
        {"wlan0_vap3_dtim"},
        {"wlan0_short_preamble"},
        {"wlan0_cts"},
        {"wlan0_wmm_enable"},
        {"wlan0_vap1_wmm_enable"},
        {"wlan0_vap2_wmm_enable"},
        {"wlan0_vap3_wmm_enable"},
        {"wlan0_wds_remote_mac"},
        {"wlan0_wds_remote_ssid"},
        {"wlan0_wmm_ap_0"},
        {"wlan0_wmm_ap_1"},
        {"wlan0_wmm_ap_2"},
        {"wlan0_wmm_ap_3"},
        {"wlan0_wmm_vap1_ap_0"},
        {"wlan0_wmm_vap1_ap_1"},
        {"wlan0_wmm_vap1_ap_2"},
        {"wlan0_wmm_vap1_ap_3"},
        {"wlan0_wmm_bss_0"},
        {"wlan0_wmm_bss_1"},
        {"wlan0_wmm_bss_2"},
        {"wlan0_wmm_bss_3"},
        {"wlan0_wmm_vap1_bss_0"},
        {"wlan0_wmm_vap1_bss_1"},
        {"wlan0_wmm_vap1_bss_2"},
        {"wlan0_wmm_vap1_bss_3"},
        {"wlan0_wmm_vap1_bss_0"},
        {"wlan0_protection"},
        {"wlan0_vap1_protection"},
        {"wlan0_auto_txrate"},
        {"wlan0_vap1_auto_txrate"},
        {"wlan0_11b_txrate"},
        {"wlan0_vap1_11b_txrate"},
        {"wlan0_11g_txrate"},
        {"wlan0_11n_txrate"},
        {"wlan0_vap1_11g_txrate"},
        {"wlan0_11bg_txrate"},
        {"wlan0_11gn_txrate"},
        {"wlan0_11bgn_txrate"},
        {"wlan0_vap1_11bg_txrate"},
        {"wlan0_super_g"},
        {"wlan0_vap1_super_g"},
        {"wlan0_wds_mode"},
        {"wlan0_remote_ssid"},
        {"wlan0_wds_enable"},
        {"wlan0_vap1_wds_enable"},
        {"wlan0_mode"},
        {"wlan0_eap_mac_auth"},
        {"wlan0_vap1_eap_mac_auth"},
        {"wlan0_countrycode"},
        {"wlan0_eap_reauth_period"},
        {"wlan0_vap1_eap_reauth_period"},
        {"wlan0_vap_guest_zone"},
        {"wlan0_mcast_rate"},
#ifdef ATHEROS_11N_DRIVER
        {"wlan0_11n_protection"},
        {"wlan0_partition"},
        {"wlan0_short_gi"},
        {"wlan0_11d_enable"},
#endif
#ifdef CONFIG_USER_WIRELESS_SCHEDULE
        {"wlan0_schedule"},
        {"wlan0_vap1_schedule"},
#endif //CONFIG_USER_WIRELESS_SCHEDULE
#if defined (USER_WL_ATH_5GHZ) || defined(USER_WL_RALINK_5GHZ)//5G interface 2
        {"wlan1_enable"},
        {"wlan1_op_mode"},
        {"wlan1_vap1_enable"},
        {"wlan1_vlan_enable"},
        {"wlan1_vlanid"},
        {"wlan1_vap1_vlanid"},
        {"wlan1_emi_test"},
        {"wlan1_ssid"},
        {"wlan1_vap1_ssid"},
        {"wlan1_vap2_ssid"},
        {"wlan1_vap3_ssid"},
        {"wlan1_domain"},
        {"wlan1_channel"},
        {"wlan1_auto_channel_enable"},
        {"wlan1_dot11_mode"},
        {"wlan1_xr_enable"},
        {"wlan1_adptive_radio_enable"},
        {"wlan1_ssid_broadcast"},
        {"wlan1_vap1_ssid_broadcast"},
        {"wlan1_idle_time"},
        {"wlan1_security"},
        {"wlan1_vap1_security"},
        {"wlan1_wep_auth"},
        {"wlan1_vap1_wep_auth"},
        {"wlan1_wep64_key_1"},
        {"wlan1_wep64_key_2"},
        {"wlan1_wep64_key_3"},
        {"wlan1_wep64_key_4"},
        {"wlan1_vap1_wep64_key_1"},
        {"wlan1_vap1_wep64_key_2"},
        {"wlan1_vap1_wep64_key_3"},
        {"wlan1_vap1_wep64_key_4"},
        {"wlan1_wep128_key_1"},
        {"wlan1_wep128_key_2"},
        {"wlan1_wep128_key_3"},
        {"wlan1_wep128_key_4"},
        {"wlan1_vap1_wep128_key_1"},
        {"wlan1_vap1_wep128_key_2"},
        {"wlan1_vap1_wep128_key_3"},
        {"wlan1_vap1_wep128_key_4"},
        {"wlan1_wep152_key_1"},
        {"wlan1_wep152_key_2"},
        {"wlan1_wep152_key_3"},
        {"wlan1_wep152_key_4"},
        {"wlan1_vap1_wep152_key_1"},
        {"wlan1_vap1_wep152_key_2"},
        {"wlan1_vap1_wep152_key_3"},
        {"wlan1_vap1_wep152_key_4"},
        {"wlan1_wep_display"},
        {"wlan1_vap1_wep_display"},
        {"wlan1_wep_default_key"},
        {"wlan1_vap1_wep_default_key"},
        {"wlan1_psk_cipher_type"},
        {"wlan1_vap1_psk_cipher_type"},
        {"wlan1_psk_pass_phrase"},
        {"wlan1_vap1_psk_pass_phrase"},
        {"wlan1_eap_radius_server_0"},
        {"wlan1_eap_radius_server_1"},
        {"wlan1_vap1_eap_radius_server_0"},
        {"wlan1_vap1_eap_radius_server_1"},
        {"wlan1_gkey_rekey_time"},
        {"wlan1_vap1_gkey_rekey_time"},
        {"wlan1_txpower"},
        {"wlan1_vap1_txpower"},
        {"wlan1_beacon_interval"},
        {"wlan1_vap1_beacon_interval"},
        {"wlan1_rts_threshold"},
        {"wlan1_vap1_rts_threshold"},
        {"wlan1_fragmentation"},
        {"wlan1_vap1_fragmentation"},
        {"wlan1_dtim"},
        {"wlan1_cts"},
        {"wlan1_vap1_dtim"},
        {"wlan1_short_preamble"},
        {"wlan1_cts_enable"},
        {"wlan1_wmm_enable"},
        {"wlan1_vap1_wmm_enable"},
        {"wlan1_wmm_ap_0"},
        {"wlan1_wmm_ap_1"},
        {"wlan1_wmm_ap_2"},
        {"wlan1_wmm_ap_3"},
        {"wlan1_wmm_bss_0"},
        {"wlan1_wmm_bss_1"},
        {"wlan1_wmm_bss_2"},
        {"wlan1_wmm_bss_3"},
        {"wlan1_protection"},
        {"wlan1_vap1_protection"},
        {"wlan1_auto_txrate"},
        {"wlan1_super_g"},
        {"wlan1_wds_mode"},
        {"wlan1_wds_remote_mac"},
		{"wlan1_wds_remote_ssid"},
        {"wlan1_wds_enable"},
        {"wlan1_vap1_wds_enable"},
        {"wlan1_mode"},
        {"wlan1_eap_mac_auth"},
		{"wlan1_vap1_eap_mac_auth"},       
        {"wlan1_countrycode"},
        {"wlan1_eap_reauth_period"},
        {"wlan1_vap1_eap_reauth_period"},
        {"wlan1_vap_guest_zone"},
        {"wlan1_mcast_rate"},
#endif
#ifdef CONFIG_USER_WIRELESS_SCHEDULE
        {"wlan1_schedule"},
        {"wlan1_vap1_schedule"},
#endif
#if defined (USER_WL_ATH_5GHZ) || defined(USER_WL_RALINK_5GHZ)//5G interface 2
        {"wlan1_11n_protection"},
        {"wlan1_partition"},
        {"wlan1_short_gi"},
        {"wlan1_11d_enable"},
#endif
#ifdef CONFIG_BRIDGE_MODE
/*      Date: 2009-04-10
        *       Name: Ken Chiang
        *       Reason: Add support for enable auto mode select(router mode/ap mode).
        *       Note:
*/
#ifdef AUTO_MODE_SELECT
        {"auto_mode_select"},
#endif
#endif
        // D-27
        {"blank_status"},
        // Multi-language GraceYang20090217
        {"language"},

#ifdef RADVD
        // IPv6
        {"ipv6_ra_status"},
        {"ipv6_ra_maxrtr_advinterval_l"},
        {"ipv6_ra_minrtr_advinterval_l"},
        {"ipv6_ra_adv_cur_hoplimit_l"},
        {"ipv6_ra_adv_managed_flag_l"},
        {"ipv6_ra_adv_other_config_flag_l"},
        {"ipv6_ra_adv_default_lifetime_l"},
        {"ipv6_ra_adv_reachable_time_l"},
        {"ipv6_ra_adv_retrans_timer_l"},
        {"ipv6_ra_adv_onlink_flag_l_one"},
        {"ipv6_ra_adv_autonomous_flag_l_one"},
        {"ipv6_ra_adv_valid_lifetime_l_one"},
        {"ipv6_ra_adv_prefer_lifetime_l_one"},
        {"ipv6_ra_adv_onlink_flag_l_two"},
        {"ipv6_ra_adv_autonomous_flag_l_two"},
        {"ipv6_ra_adv_valid_lifetime_l_two"},
        {"ipv6_ra_adv_prefer_lifetime_l_two"},
        {"ipv6_ra_adv_link_mtu_l"},
        {"ipv6_ra_prefix64_l_one"},
        {"ipv6_ra_prefix64_l_two"},
        {"ipv6_ra_maxrtr_advinterval_w"},
        {"ipv6_ra_minrtr_advinterval_w"},
        {"ipv6_ra_adv_cur_hoplimit_w"},
        {"ipv6_ra_adv_managed_flag_w"},
        {"ipv6_ra_adv_other_config_flag_w"},
        {"ipv6_ra_adv_default_lifetime_w"},
        {"ipv6_ra_adv_reachable_time_w"},
        {"ipv6_ra_adv_retrans_timer_w"},
        {"ipv6_ra_adv_onlink_flag_w_one"},
        {"ipv6_ra_adv_autonomous_flag_w_one"},
        {"ipv6_ra_adv_valid_lifetime_w_one"},
        {"ipv6_ra_adv_prefer_lifetime_w_one"},
        {"ipv6_ra_adv_onlink_flag_w_two"},
        {"ipv6_ra_adv_autonomous_flag_w_two"},
        {"ipv6_ra_adv_valid_lifetime_w_two"},
        {"ipv6_ra_adv_prefer_lifetime_w_two"},
        {"ipv6_ra_adv_link_mtu_w"},
        {"ipv6_ra_prefix64_w_one"},
        {"ipv6_ra_prefix64_w_two"},
        {"ipv6_ra_interface"},
        {"ipv6_wan_proto"},
        {"ipv6_use_link_local"},
        {"ipv6_autoconfig"},
      	{"ipv6_autodetect_lan_ip"},
      	{"ipv6_autodetect_secondary_dns"},
      	{"ipv6_autodetect_primary_dns"},
      	{"ipv6_autodetect_dns_enable"},
        {"ipv6_autoconfig_type"},
        {"ipv6_dhcpd_lifetime"},
        {"ipv6_dhcpd_prefix"},
        {"ipv6_static_prefix_length"},
        {"ipv6_static_default_gw"},
        {"ipv6_static_primary_dns"},
        {"ipv6_static_secondary_dns"},
        {"ipv6_dhcp_pd_enable"},
        { "ipv6_dhcp_pd_enable_l"},
	{ "ipv6_dhcpd_start"}, 
	{ "ipv6_dhcpd_end"}, 
        {"ipv6_6in4_dns_enable"},
        {"ipv6_dhcp_dns_enable"},
        {"ipv6_stateless_dns_enable"},
        {"ipv6_dhcppd_pppoe_enable"},
        {"ipv6_dhcppd_6in4_enable"},
        {"ipv6_dhcp_primary_dns"},
        {"ipv6_dhcp_secondary_dns"},
        {"ipv6_dhcp_lan_ip"},
        {"ipv6_autoconfig_dns_enable"}, 
        {"ipv6_autoconfig_primary_dns"},
        {"ipv6_autoconfig_secondary_dns"},
        {"ipv6_autoconfig_lan_ip"},
        {"ipv6_autoconfig_flag"},
        {"ipv6_pppoe_share"},
        {"ipv6_pppoe_dynamic"},
        {"ipv6_pppoe_ipaddr"},
        {"ipv6_pppoe_username"},
        {"ipv6_pppoe_password"},
        {"ipv6_pppoe_service"},
        {"ipv6_pppoe_connect_mode"},
        {"ipv6_pppoe_idle_time"},
        {"ipv6_pppoe_mtu"},
        {"ipv6_pppoe_dns_enable"},
        {"ipv6_pppoe_primary_dns"},
        {"ipv6_pppoe_secondary_dns"},
        {"ipv6_pppoe_lan_ip"},
        {"ipv4_6in4_remote_ip"},
        {"ipv6_6in4_remote_ip"},
        {"ipv4_6in4_wan_ip"},
        {"ipv6_6in4_wan_ip"},
        {"ipv6_6in4_primary_dns"},
        {"ipv6_6in4_secondary_dns"},
        {"ipv6_6in4_lan_ip"},
	{"ipv6_dhcp_pd_hint_enable"},
        {"ipv6_dhcp_pd_hint_prefix"}, 
        {"ipv6_dhcp_pd_hint_length"},
        {"ipv6_dhcp_pd_hint_pltime"},  
        {"ipv6_dhcp_pd_hint_vltime"}, 
        {"ipv6_6to4_primary_dns"},
        {"ipv6_6to4_secondary_dns"},
        {"ipv6_6to4_lan_ip"},
        {"ipv6_6to4_lan_ip_subnet"},
        {"ipv6_6to4_relay"},
#ifdef IPV6_STATELESS_WAN
        {"ipv6_stateless_primary_dns"},
        {"ipv6_stateless_secondary_dns"},
        {"ipv6_stateless_lan_ip"},
#endif
        {"ipv6_wan_specify_dns"},
#endif //RADVD
        {"ipv6_default_gateway_w"},
  {"ipv6_default_gateway_l"},
  {"ipv6_static_wan_ip"},
  {"ipv6_static_lan_ip"},
#ifdef MRD_ENABLE
        {"ipv6_mrd_status"},
#endif
        // D-28
        {"asp_temp_00"},
        {"asp_temp_01"},
        {"asp_temp_02"},
        {"asp_temp_03"},
        {"asp_temp_04"},
        {"asp_temp_05"},
        {"asp_temp_06"},
        {"asp_temp_07"},
        {"asp_temp_08"},
        {"asp_temp_09"},
        {"asp_temp_10"},
        {"asp_temp_11"},
        {"asp_temp_12"},
        {"asp_temp_13"},
        {"asp_temp_14"},
        {"asp_temp_15"},
        {"asp_temp_16"},
        {"asp_temp_17"},
        {"asp_temp_18"},
        {"asp_temp_19"},
        {"asp_temp_20"},
        {"asp_temp_21"},
        {"asp_temp_22"},
        {"asp_temp_23"},
        {"asp_temp_24"},
        {"asp_temp_25"},
        {"asp_temp_26"},
        {"asp_temp_27"},
        {"asp_temp_28"},
        {"asp_temp_29"},
        {"asp_temp_30"},
        {"asp_temp_31"},
        {"asp_temp_32"},
        {"asp_temp_33"},
        {"asp_temp_34"},
        {"asp_temp_35"},
        {"asp_temp_36"},
        {"asp_temp_37"},
        {"asp_temp_38"},
        {"asp_temp_39"},
        {"asp_temp_40"},
        {"asp_temp_41"},
        {"asp_temp_42"},
        {"asp_temp_43"},
        {"asp_temp_44"},
        {"asp_temp_45"},
        {"asp_temp_46"},
        {"asp_temp_47"},
        {"asp_temp_48"},
        {"asp_temp_49"},
        {"asp_temp_50"},
        {"asp_temp_51"},
        {"asp_temp_52"},
        {"asp_temp_53"},
        {"asp_temp_54"},
        {"asp_temp_55"},
        {"asp_temp_56"},
        {"asp_temp_57"},
        {"asp_temp_58"},
        {"asp_temp_59"},
        {"asp_temp_65"},
#ifdef USER_WL_ATH_5GHZ //add for WLAN Guest Zone
        {"asp_temp_60"},
        {"asp_temp_61"},
        {"asp_temp_62"},
        {"asp_temp_63"},
        {"asp_temp_64"},
        {"asp_temp_66"},
        {"asp_temp_67"},
        {"asp_temp_68"},
        {"asp_temp_69"},
        {"asp_temp_70"},
        {"asp_temp_71"},
#endif //DIR865
        {"asp_temp_72"},
        {"asp_temp_wep64_key_1"},
        {"asp_temp_wep64_key_2"},
        {"asp_temp_wep64_key_3"},
        {"asp_temp_wep64_key_4"},
        {"asp_temp_wep128_key_1"},
        {"asp_temp_wep128_key_2"},
        {"asp_temp_wep128_key_3"},
        {"asp_temp_wep128_key_4"},
        {"asp_temp_wep152_key_1"},
        {"asp_temp_wep152_key_2"},
        {"asp_temp_wep152_key_3"},
        {"asp_temp_wep152_key_4"},
        {"html_response_page"},
        {"html_response_message"},
        {"html_response_return_page"},
        // D-30
        {"pure_type"},
        {"pure_device_name"},
        {"pure_vendor_name"},
        {"pure_model_description"},
        // D-31
        {"block_service_00"},
        {"block_service_01"},
        {"block_service_02"},
        {"block_service_03"},
        {"block_service_04"},
        {"block_service_05"},
        {"block_service_06"},
        {"block_service_07"},
        {"block_service_08"},
        {"block_service_09"},
        // D-32 snmp
#ifdef CONFIG_USER_SNMP
        {"snmpv3_enable"},
        {"snmp_sys_loaction"},
        {"snmp_sys_contact"},
#ifdef CONFIG_USER_SNMP_TRAP
        {"snmp_trap_receiver_1"},
        {"snmp_trap_receiver_2"},
        {"snmp_trap_receiver_3"},
#endif
        ("snmpv3_security_user_name"),
        ("auth_protocol"),
        ("auth_password"),
        ("privacy_protocol"),
        ("privacy_password"),
#endif //CONFIG_USER_SNMP
         //D-35
#ifdef CONFIG_USER_TC
        {"qos_enable"},
        {"auto_uplink"},
        {"qos_uplink"},
        {"qos_downlink"},
        {"traffic_shaping"},
        {"auto_classification"},
        {"qos_connection_type"},
        {"qos_dyn_fragmentation"},
        {"qos_rule_00"},
        {"qos_rule_01"},
        {"qos_rule_02"},
        {"qos_rule_03"},
        {"qos_rule_04"},
        {"qos_rule_05"},
        {"qos_rule_06"},
        {"qos_rule_07"},
        {"qos_rule_08"},
        {"qos_rule_09"},
        {"qos_rule_10"},
        {"qos_rule_11"},
        {"qos_rule_12"},
        {"qos_rule_13"},
        {"qos_rule_14"},
        {"qos_rule_15"},
        {"qos_rule_16"},
        {"qos_rule_17"},
        {"qos_rule_18"},
        {"qos_rule_19"},
#endif
        //D-36
        {"tcp_nat_type"},
        {"udp_nat_type"},
        {"nat_enable"},
  //D-37
#ifdef CONFIG_USER_WISH
        {"wish_engine_enabled"},
        {"wish_http_enabled"},
        {"wish_media_enabled"},
        {"wish_auto_enabled"},
        {"wish_rule_00"},
        {"wish_rule_01"},
        {"wish_rule_02"},
        {"wish_rule_03"},
        {"wish_rule_04"},
        {"wish_rule_05"},
        {"wish_rule_06"},
        {"wish_rule_07"},
        {"wish_rule_08"},
        {"wish_rule_09"},
        {"wish_rule_10"},
        {"wish_rule_11"},
        {"wish_rule_12"},
        {"wish_rule_13"},
        {"wish_rule_14"},
        {"wish_rule_15"},
        {"wish_rule_16"},
        {"wish_rule_17"},
        {"wish_rule_18"},
  {"wish_rule_19"},
  {"wish_rule_20"},
        {"wish_rule_21"},
        {"wish_rule_22"},
        {"wish_rule_23"},
        {"wish_rule_24"},
        {"wish_rule_25"},
#endif //ifdef CONFIG_USER_WISH
        // D-39
        {"wps_enable"},
        {"wps_configured_mode"},
        {"wps_lock"},
        {"disable_wps_pin"},
        {"wps_pin"},
        {"wps_default_pin"},
        {"wps_sta_enrollee_pin"},
        // D-42
        {"firewall_rule_00"},
        {"firewall_rule_01"},
        {"firewall_rule_02"},
        {"firewall_rule_03"},
        {"firewall_rule_04"},
        {"firewall_rule_05"},
        {"firewall_rule_06"},
        {"firewall_rule_07"},
        {"firewall_rule_08"},
        {"firewall_rule_09"},
        {"firewall_rule_10"},
        {"firewall_rule_11"},
        {"firewall_rule_12"},
        {"firewall_rule_13"},
        {"firewall_rule_14"},
        {"firewall_rule_15"},
        {"firewall_rule_16"},
        {"firewall_rule_17"},
        {"firewall_rule_18"},
        {"firewall_rule_19"},
        {"firewall_rule_20"},
        {"firewall_rule_21"},
        {"firewall_rule_22"},
        {"firewall_rule_23"},
        {"firewall_rule_24"},
        {"firewall_rule_25"},
        {"firewall_rule_26"},
        {"firewall_rule_27"},
        {"firewall_rule_28"},
        {"firewall_rule_29"},
        {"firewall_rule_30"},
        {"firewall_rule_31"},
        {"firewall_rule_32"},
        {"firewall_rule_33"},
        {"firewall_rule_34"},
        {"firewall_rule_35"},
        {"firewall_rule_36"},
        {"firewall_rule_37"},
        {"firewall_rule_38"},
        {"firewall_rule_39"},
        {"firewall_rule_40"},
        {"firewall_rule_41"},
        {"firewall_rule_42"},
        {"firewall_rule_43"},
        {"firewall_rule_44"},
        {"firewall_rule_45"},
        {"firewall_rule_46"},
        {"firewall_rule_47"},
        {"firewall_rule_48"},
        {"firewall_rule_49"},
  {"firewall_rule_50"},
        {"firewall_rule_51"},
        {"firewall_rule_52"},
        {"firewall_rule_53"},
        {"firewall_rule_54"},
        {"firewall_rule_55"},
        {"firewall_rule_56"},
        {"firewall_rule_57"},
        {"firewall_rule_58"},
        {"firewall_rule_59"},
        {"firewall_rule_60"},
        {"firewall_rule_61"},
        {"firewall_rule_62"},
        {"firewall_rule_63"},
        {"firewall_rule_64"},
        {"firewall_rule_65"},
        {"firewall_rule_66"},
        {"firewall_rule_67"},
        {"firewall_rule_68"},
        {"firewall_rule_69"},
        {"firewall_rule_70"},
        {"firewall_rule_71"},
        {"firewall_rule_72"},
        {"firewall_rule_73"},
        {"firewall_rule_74"},
        {"firewall_rule_75"},
        {"firewall_rule_76"},
        {"firewall_rule_77"},
        {"firewall_rule_78"},
        {"firewall_rule_79"},
        {"firewall_rule_80"},
        {"firewall_rule_81"},
        {"firewall_rule_82"},
        {"firewall_rule_83"},
        {"firewall_rule_84"},
        {"firewall_rule_85"},
        {"firewall_rule_86"},
        {"firewall_rule_87"},
        {"firewall_rule_88"},
        {"firewall_rule_89"},
        {"firewall_rule_90"},
        {"firewall_rule_91"},
        {"firewall_rule_92"},
        {"firewall_rule_93"},
        {"firewall_rule_94"},
        {"firewall_rule_95"},
        {"firewall_rule_96"},
        {"firewall_rule_97"},
        {"firewall_rule_98"},
        {"firewall_rule_99"},
        {"firewall_rule_100"},
        {"firewall_rule_101"},
        {"firewall_rule_102"},
        {"firewall_rule_103"},
        {"firewall_rule_104"},
        {"firewall_rule_105"},
        {"firewall_rule_106"},
        {"firewall_rule_107"},
        {"firewall_rule_108"},
        {"firewall_rule_109"},
        {"firewall_rule_110"},
        {"firewall_rule_111"},
        {"firewall_rule_112"},
        {"firewall_rule_113"},
        {"firewall_rule_114"},
        {"firewall_rule_115"},
        {"firewall_rule_116"},
        {"firewall_rule_117"},
        {"firewall_rule_118"},
        {"firewall_rule_119"},
        {"ipv6_6rd_primary_dns"},
        {"ipv6_6rd_secondary_dns"},
        {"ipv6_6rd_prefix"},
        {"ipv6_6rd_prefix_length"},
        {"ipv6_6rd_lan_ip"},
        {"ipv6_6rd_relay"},
        {"ipv6_6rd_dhcp"},
        {"ipv6_6rd_ipv4_mask_length"},
				#ifdef IPV6_DSLITE
				{"wan_dslite_dhcp"},
				{"wan_dslite_aftr"},
				{"wan_dslite_ipv4"},
				#endif
        {"firewall_rule_ipv6_00"},
        {"firewall_rule_ipv6_01"},
        {"firewall_rule_ipv6_02"},
        {"firewall_rule_ipv6_03"},
        {"firewall_rule_ipv6_04"},
        {"firewall_rule_ipv6_05"},
        {"firewall_rule_ipv6_06"},
        {"firewall_rule_ipv6_07"},
        {"firewall_rule_ipv6_08"},
        {"firewall_rule_ipv6_09"},
        {"firewall_rule_ipv6_10"},   
        {"firewall_rule_ipv6_11"},   
        {"firewall_rule_ipv6_12"},   
        {"firewall_rule_ipv6_13"},   
        {"firewall_rule_ipv6_14"},   
        {"firewall_rule_ipv6_15"},   
        {"firewall_rule_ipv6_16"},   
        {"firewall_rule_ipv6_17"},   
        {"firewall_rule_ipv6_18"},   
        {"firewall_rule_ipv6_19"},
        {"ipv6_filter_type"}, 
        //D-43
        {"multiple_routing_table_00"},
        {"multiple_routing_table_01"},
        {"multiple_routing_table_02"},
        {"multiple_routing_table_03"},
        {"multiple_routing_table_04"},
        {"multiple_routing_table_05"},
        {"multiple_routing_table_06"},
        {"multiple_routing_table_07"},
        {"multiple_routing_table_08"},
        {"multiple_routing_table_09"},
        {"multiple_routing_table_10"},
        {"multiple_routing_table_11"},
        {"multiple_routing_table_12"},
        {"multiple_routing_table_13"},
        {"multiple_routing_table_14"},
        {"multiple_routing_table_15"},
        {"multiple_routing_table_16"},
        {"multiple_routing_table_17"},
        {"multiple_routing_table_18"},
        {"multiple_routing_table_19"},
        //D-30
        {"spi_enable"},
        {"CAMEO"},
  //D-34
        {"auto_check_newer_fw"},
        {"email_newer_fw"},
        // D-44
        {"inbound_filter_name_00"},
        {"inbound_filter_name_01"},
        {"inbound_filter_name_02"},
        {"inbound_filter_name_03"},
        {"inbound_filter_name_04"},
        {"inbound_filter_name_05"},
        {"inbound_filter_name_06"},
        {"inbound_filter_name_07"},
        {"inbound_filter_name_08"},
        {"inbound_filter_name_09"},
        {"inbound_filter_ip_00_A"},
        {"inbound_filter_ip_01_A"},
        {"inbound_filter_ip_02_A"},
        {"inbound_filter_ip_03_A"},
        {"inbound_filter_ip_04_A"},
        {"inbound_filter_ip_05_A"},
        {"inbound_filter_ip_06_A"},
        {"inbound_filter_ip_07_A"},
        {"inbound_filter_ip_08_A"},
        {"inbound_filter_ip_09_A"},
        {"inbound_filter_ip_00_B"},
        {"inbound_filter_ip_01_B"},
        {"inbound_filter_ip_02_B"},
        {"inbound_filter_ip_03_B"},
        {"inbound_filter_ip_04_B"},
        {"inbound_filter_ip_05_B"},
        {"inbound_filter_ip_06_B"},
        {"inbound_filter_ip_07_B"},
        {"inbound_filter_ip_08_B"},
        {"inbound_filter_ip_09_B"},
        // D-45
        {"access_control_filter_type"},
        {"access_control_00"},
        {"access_control_01"},
        {"access_control_02"},
        {"access_control_03"},
        {"access_control_04"},
        {"access_control_05"},
        {"access_control_06"},
        {"access_control_07"},
        {"access_control_08"},
        {"access_control_09"},
        {"access_control_10"},
        {"access_control_11"},
        {"access_control_12"},
        {"access_control_13"},
        {"access_control_14"},
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
        { "lingual"},
#endif
#ifdef MIII_BAR
        {"miiicasa_enabled"},
#endif
#ifdef USB_STORAGE_HTTP_ENABLE
        {"usb_storage_http_enable"},		
#endif//USB_STORAGE_HTTP_ENABLE 

#ifdef CONFIG_USER_WEB_FILE_ACCESS
//        {"usb_storage_http_enable"},		
				{"feature_storage"},
				{"file_access_enable"},
				{"file_access_remote"},
				{"file_access_remote_port"},
				{"file_access_ssl"},
				{"file_access_ssl_port"},
				{"storage_account"},
				{"storage_access"},
#endif//CONFIG_USER_WEB_FILE_ACCESS 
        //D-46
        {"graph_auth_enable"},
        {"nvram_default_value"},
/* Name: Norp Huang
 * Date: 2009-04-13
 * Description: configuration for openagent-2.0 (TR-069).
 */
#ifdef CONFIG_USER_OPENAGENT
// new add
// 2010.08.17 Aaron add
        {"tr069_ui"},
	{"tr069_cpe_usr"},
	{"tr069_cpe_pwd"},

        //D-48
        {"tr069_enable"},
        {"tr069_acs_usr"},
        {"tr069_acs_pwd"},
        {"tr069_acs_url"},
        {"tr069_acs_def_url"},
        {"tr069_cpe_url"},

        {"tr069_inform_enable"},
        {"tr069_inform_interval"},
        {"tr069_inform_time"},
#endif
/*
*       Date: 2009-5-21
*       Name: Macpaul Lin
*       Reason: Add 8021X support for WAN.
*       Notice :
*/
#ifdef CONFIG_USER_WAN_8021X
        {"wan_8021x_enable"},
        {"wan_8021x_auth_type"},
        {"wan_8021x_eap_method"},
        {"wan_8021x_eap_tls_key_passwd"},
        {"wan_8021x_eap_ttls_inner_proto"},
        {"wan_8021x_eap_ttls_username"},
        {"wan_8021x_eap_ttls_passwd"},
        {"wan_8021x_eap_peap_inner_proto"},
        {"wan_8021x_eap_peap_username"},
        {"wan_8021x_eap_peap_passwd"},
        /* the following is prepared for WiMAX */
        {"wan_8021x_auth_anon_user"},
        {"wan_8021x_auth_user"},
        {"wan_8021x_auth_passwd"},
        {"wan_8021x_auth_key_passwd"},
#endif
/*
*       Date: 2009-4-30
*   Name: Ken Chiang
*   Reason: modify for support heartbeat feature.
*   Notice :
*/
#ifdef CONFIG_USER_HEARTBEAT
        {"heartbeat_server"},
        {"heartbeat_interval"},
#endif
#ifdef CONFIG_USER_OPENSWAN
        {"ipsec_pt"},
        {"vpn_nat_traversal"},
        {"vpn_conn1"},
        {"vpn_conn2"},
        {"vpn_conn3"},
        {"vpn_conn4"},
        {"vpn_conn5"},
        {"vpn_conn6"},
        {"vpn_conn7"},
        {"vpn_conn8"},
        {"vpn_conn9"},
        {"vpn_conn10"},
        {"vpn_conn11"},
        {"vpn_conn12"},
        {"vpn_conn13"},
        {"vpn_conn14"},
        {"vpn_conn15"},
        {"vpn_conn16"},
        {"vpn_conn17"},
        {"vpn_conn18"},
        {"vpn_conn19"},
        {"vpn_conn20"},
        {"vpn_conn21"},
        {"vpn_conn22"},
        {"vpn_conn23"},
        {"vpn_conn24"},
        {"vpn_conn25"},
        {"vpn_conn26"},
        {"vpn_extra1"},
        {"vpn_extra2"},
        {"vpn_extra3"},
        {"vpn_extra4"},
        {"vpn_extra5"},
        {"vpn_extra6"},
        {"vpn_extra7"},
        {"vpn_extra8"},
        {"vpn_extra9"},
        {"vpn_extra10"},
        {"vpn_extra11"},
        {"vpn_extra12"},
        {"vpn_extra13"},
        {"vpn_extra14"},
        {"vpn_extra15"},
        {"vpn_extra16"},
        {"vpn_extra17"},
        {"vpn_extra18"},
        {"vpn_extra19"},
        {"vpn_extra20"},
        {"vpn_extra21"},
        {"vpn_extra22"},
        {"vpn_extra23"},
        {"vpn_extra24"},
        {"vpn_extra25"},
        {"vpn_extra26"},
        {"fw_vpn_conn0"},
        {"fw_vpn_conn1"},
        {"fw_vpn_conn2"},
        {"fw_vpn_conn3"},
        {"fw_vpn_conn4"},
        {"fw_vpn_conn5"},
        {"fw_vpn_conn6"},
        {"fw_vpn_conn7"},
        {"fw_vpn_conn8"},
        {"fw_vpn_conn9"},
        {"fw_vpn_conn10"},
        {"fw_vpn_conn11"},
        {"fw_vpn_conn12"},
        {"fw_vpn_conn13"},
        {"fw_vpn_conn14"},
        {"fw_vpn_conn15"},
        {"fw_vpn_conn16"},
        {"fw_vpn_conn17"},
        {"fw_vpn_conn18"},
        {"fw_vpn_conn19"},
        {"fw_vpn_conn20"},
        {"fw_vpn_conn21"},
        {"fw_vpn_conn22"},
        {"fw_vpn_conn23"},
        {"fw_vpn_conn24"},
        {"fw_vpn_conn25"},
        {"x509_1"},
        {"x509_2"},
        {"x509_3"},
        {"x509_4"},
        {"x509_5"},
        {"x509_6"},
        {"x509_7"},
        {"x509_8"},
        {"x509_9"},
        {"x509_10"},
        {"x509_11"},
        {"x509_12"},
        {"x509_13"},
        {"x509_14"},
        {"x509_15"},
        {"x509_16"},
        {"x509_17"},
        {"x509_18"},
        {"x509_19"},
        {"x509_20"},
        {"x509_21"},
        {"x509_22"},
        {"x509_23"},
        {"x509_24"},
        {"x509_ca_1"},
        {"x509_ca_2"},
        {"x509_ca_3"},
        {"x509_ca_4"},
        {"x509_ca_5"},
        {"x509_ca_6"},
        {"x509_ca_7"},
        {"x509_ca_8"},
        {"x509_ca_9"},
        {"x509_ca_10"},
        {"x509_ca_11"},
        {"x509_ca_12"},
        {"x509_ca_13"},
        {"x509_ca_14"},
        {"x509_ca_15"},
        {"x509_ca_16"},
        {"x509_ca_17"},
        {"x509_ca_18"},
        {"x509_ca_19"},
        {"x509_ca_20"},
        {"x509_ca_21"},
        {"x509_ca_22"},
        {"x509_ca_23"},
        {"x509_ca_24"},
        {"auth_group1"},
        {"auth_group2"},
        {"auth_group3"},
        {"auth_group4"},
        {"auth_group5"},
        {"auth_group6"},
        {"edit_vpn"},
        {"edit_type"},
        {"vpn_link"},
#endif
};

#ifdef CONFIG_USER_WISH
void return_wish_sessions_table(webs_t wp,char *args);
#endif
static void set_ASCII_flag(void);
#ifdef USER_WL_ATH_5GHZ
static void set_a_ASCII_flag(void);
#endif
static void set_wep64_flag(void);
static void set_wep128_flag(void);
static void set_wep152_flag(void);
static void clean_all_wep_flag(void);
char *substitute_keyword(char *string,char *source_key,char *dest_key);
static char *checkQuot(char *name_t,char *value_t);
unsigned long lease_time_execute(int type);
int g_ASCII_flag = 0;
#ifdef USER_WL_ATH_5GHZ
int a_ASCII_flag = 0;
#endif

int g_wep64_flag = 0;
int g_wep128_flag = 0;
int g_wep152_flag = 0;
int dhcp_reset = 0;

wlan_wepkey_t wepkey_info[]=
{
/*    *wlan_name             wlan_value     (*set_wlan_flag)       */
        { "wlan0_wep_display", "ascii", set_ASCII_flag},
        { "wlan0_security", "wep_open_64", set_wep64_flag},
        { "wlan0_security", "wep_share_64", set_wep64_flag},
        { "wlan0_security", "wep_open_128", set_wep128_flag},
        { "wlan0_security", "wep_share_128", set_wep128_flag},
        { "wlan0_vap1_wep_display", "ascii", set_ASCII_flag},
        { "wlan0_vap1_security", "wep_open_64", set_wep64_flag},
        { "wlan0_vap1_security", "wep_share_64", set_wep64_flag},
        { "wlan0_vap1_security", "wep_open_128", set_wep128_flag},
        { "wlan0_vap1_security", "wep_share_128", set_wep128_flag},
#ifdef USER_WL_ATH_5GHZ
        { "wlan1_wep_display", "ascii", set_a_ASCII_flag},
        { "wlan1_security", "wep_open_64", set_wep64_flag},
        { "wlan1_security", "wep_share_64", set_wep64_flag},
        { "wlan1_security", "wep_open_128", set_wep128_flag},
        { "wlan1_security", "wep_share_128", set_wep128_flag},
        { "wlan1_vap1_wep_display", "ascii", set_a_ASCII_flag},
        { "wlan1_vap1_security", "wep_open_64", set_wep64_flag},
        { "wlan1_vap1_security", "wep_share_64", set_wep64_flag},
        { "wlan1_vap1_security", "wep_open_128", set_wep128_flag},
        { "wlan1_vap1_security", "wep_share_128", set_wep128_flag}
#endif
};
/* used by set_basic_api() */
/**************************************************************/
/**************************************************************/


#ifdef OPENDNS
void return_opendns_token(webs_t wp,char *args);
void return_opendns_token_get(webs_t wp,char *args);
void return_opendns_token_failcode(webs_t wp,char *args);
#endif

/* CmoGetStatus */
/**************************************************************/
/**************************************************************/
status_handler_t status_handlers[]={
        { "get_admin_password",return_admin_password},
        { "reboot_needed",return_reboot_needed},
        { "wlan0_wep_key1",return_wep_key1},
        { "wlan0_psk_pass_phrase",return_wpa_psk_ps},
        { "wlan0_radius0_ps",return_radius0_ps},  
        { "wlan0_radius1_ps",return_radius1_ps},
        { "wlan0_vap1_wep_key1",return_vap1_wep_key1},        
        { "wlan0_vap1_psk_pass_phrase",return_vap1_wpa_psk_ps},
        { "wlan0_vap1_radius0_ps",return_vap1_radius0_ps},  
        { "wlan0_vap1_radius1_ps",return_vap1_radius1_ps},       
        { "wan_current_ipaddr_00",return_wan_current_ipaddr_00},
#ifdef USER_WL_ATH_5GHZ
        { "wlan1_wep_key1",return_wlan1_wep_key1},
        { "wlan1_psk_pass_phrase",return_wlan1_wpa_psk_ps},
        { "wlan1_radius0_ps",return_wlan1_radius0_ps},  
        { "wlan1_radius1_ps",return_wlan1_radius1_ps},
        { "wlan1_vap1_wep_key1",return_wlan1_vap1_wep_key1},        
        { "wlan1_vap1_psk_pass_phrase",return_wlan1_vap1_wpa_psk_ps},
        { "wlan1_vap1_radius0_ps",return_wlan1_vap1_radius0_ps},  
        { "wlan1_vap1_radius1_ps",return_wlan1_vap1_radius1_ps},
#endif        
#ifdef MPPPOE
        { "wan_current_ipaddr_01",return_wan_current_ipaddr_01},
        { "wan_current_ipaddr_02",return_wan_current_ipaddr_02},
        { "wan_current_ipaddr_03",return_wan_current_ipaddr_03},
        { "wan_current_ipaddr_04",return_wan_current_ipaddr_04},
        { "lan_tx_bytes", return_lan_tx_bytes },
        { "lan_rx_bytes", return_lan_rx_bytes },
        { "wan_tx_bytes", return_wan_tx_bytes },
        { "wan_rx_bytes", return_wan_rx_bytes },
        { "wlan_tx_bytes", return_wlan_tx_bytes },
        { "wlan_rx_bytes", return_wlan_rx_bytes },
#endif
        { "ap_udhcpc_lan_ip_info",return_lan_ip_info},
        { "read_fw_success",return_read_fw_success},
        { "version",return_version},
        { "build",return_build},
        { "version_date",return_version_date},
        { "hw_version",return_hw_version},
#ifdef OPENDNS
        { "opendns_token",return_opendns_token},
        { "opendns_token_get",return_opendns_token_get},
        { "opendns_token_failcode",return_opendns_token_failcode},
#endif
        { "title",return_title},
        { "gui_logout",return_gui_logout},
        { "copyright",return_copyright},
        { "wireless_driver_info",return_wireless_driver_info},
        { "wireless_domain",return_wireless_domain},
        { "wireless_mac",return_wireless_mac},
        { "lan_tx_packets", return_lan_tx_packets },
        { "lan_rx_packets", return_lan_rx_packets },
        { "lan_lost_packets",return_lan_lost_packets},
        { "wan_tx_packets", return_wan_tx_packets },
        { "wan_rx_packets", return_wan_rx_packets },
        { "wan_lost_packets",return_wan_lost_packets},
        { "wlan_tx_packets", return_wlan_tx_packets },
        { "wlan_rx_packets", return_wlan_rx_packets },
        { "wlan_lost_packets",return_wlan_lost_packets},
        { "wlan_tx_frames", return_wlan_tx_frames },
        { "wlan_rx_frames", return_wlan_rx_frames },
        { "wan_ifconfig_info", return_wan_ifconfig_info},
        { "lan_ifconfig_info", return_lan_ifconfig_info},
        { "wlan_ifconfig_info", return_wlan_ifconfig_info},
//5G interface 2
        { "wlan1_ifconfig_info", return_wlan1_ifconfig_info},
#ifdef USER_WL_ATH_5GHZ
        { "wlan1_mac_addr", return_wlan1_mac_addr},//AG Concurrent
#endif
#if defined(DIR865) || defined(EXVERSION_NNA)
        { "language", return_language}, //for GUI Multi-language
#endif
        { "guestzone_mac", return_guestzone_mac},
        { "fixip_connection_status", return_fixip_connection_status},
        { "dhcpc_connection_status",return_dhcpc_connection_status},
        { "pppoe_00_connection_status",return_pppoe_00_connection_status},
        { "pppoe_01_connection_status",return_pppoe_01_connection_status},
        { "pppoe_02_connection_status",return_pppoe_02_connection_status},
        { "pppoe_03_connection_status",return_pppoe_03_connection_status},
        { "pppoe_04_connection_status",return_pppoe_04_connection_status},
        { "l2tp_connection_status",return_l2tp_connection_status},
        { "pptp_connection_status",return_pptp_connection_status},
#ifdef CONFIG_USER_BIGPOND//no used
    { "bigpond_connection",return_bigpond_connection_status},
#endif
        { "get_current_user",return_get_current_user},
        { "get_error_message",return_get_error_message},
        { "get_error_setting",return_get_error_setting},
        { "mac_clone_addr",return_mac_clone_addr},
        { "mac_default_addr",return_mac_default_addr},
        { "ping_ipaddr",return_ping_ipaddr},
        { "ping_result",return_ping_result},
        { "ping_result_display",return_ping_result_display},
#ifdef RADVD
        { "ping6_ipaddr",return_ping6_ipaddr},
        { "ping6_interface",return_ping6_interface},
        { "ping6_size",return_ping6_size},
        { "ping6_count",return_ping6_count},
        { "ping6_result",return_ping6_result},
        { "ping6_test_result",return_ping6_test_result},
        { "6to4_tunnel_address",return_6to4_tunnel_address},
        { "6to4_lan_address",return_6to4_lan_address},
#endif
        { "system_time",return_system_time},
        { "wlan0_channel_list",return_wlan0_channel_list},
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
        { "language_mtdblock_check",return_language_mtdblock_check},
        { "lp_version", return_lp_version},
        { "lp_date", return_lp_date},
        { "online_lp_check", return_online_lp_check},
#endif
//5G interface 2
        { "wlan1_channel_list",return_wlan1_channel_list},
        { "wan_port_status",return_wan_port_status},
        { "wan_port_speed_duplex",return_wan_port_speed_duplex},
        { "lan_port_status_00",return_lan_port_status_00},
        { "lan_port_status_01",return_lan_port_status_01},
        { "lan_port_status_02",return_lan_port_status_02},
        { "lan_port_status_03",return_lan_port_status_03},
        { "lan_port_speed_duplex_00",return_lan_port_speed_duplex_00},
        { "lan_port_speed_duplex_01",return_lan_port_speed_duplex_01},
        { "lan_port_speed_duplex_02",return_lan_port_speed_duplex_02},
        { "lan_port_speed_duplex_03",return_lan_port_speed_duplex_03},
        { "log_total_page",return_log_total_page},
        { "log_current_page", return_log_current_page},
        { "current_channel", return_current_channel},
        { "usb_test_result", return_usb_test_result},
        { "lan_collisions", return_lan_collisions},
        { "wan_collisions", return_wan_collisions},
        { "wlan_collisions", return_wlan_collisions},
        { "dhcpc_obtain_time", return_dhcpc_obtain_time},
        { "dhcpc_expire_time", return_dhcpc_expire_time},
        { "lan_uptime", return_lan_uptime},
        { "wan_uptime", return_wan_uptime},
        { "wlan_uptime", return_wlan_uptime},
        { "pppoe_00_uptime",return_pppoe_00_uptime},
        { "internet_connect_type", return_internet_connect_type},
        { "internet_connect",return_internet_connect},
        { "ddns_status",return_ddns_status},
        { "wps_generate_pin",return_wps_generate_pin_by_random},
        { "wps_generate_default_pin",return_wps_generate_pin_by_mac},
        { "wps_sta_enrollee_pin",return_wps_sta_enrollee_pin},
        { "wps_push_button_result",return_wps_push_button_result},
        { "online_firmware_check",return_online_firmware_check},
        { "internetonline_check",return_internetonline_check},
#ifdef CONFIG_USER_TC
        {"qos_detected_line_type", return_qos_detected_line_type},
        { "measured_uplink_speed",return_measured_uplink_speed},
        { "if_measuring_uplink_now",return_if_measuring_uplink_now},
        { "if_wan_ip_obtained",return_if_wan_ip_obtained},
#endif
        { "average_bytes",return_average_bytes},
        { "dns_query_result",return_dns_query_result},
#ifdef IPv6_SUPPORT
        { "default_gateway",return_default_gateway},
        { "get_ipv6_dns",return_get_ipv6_dns},
        { "global_ip_w",return_global_ip_w},
        { "global_ip_l",return_global_ip_l},
        { "link_local_ip_w",return_link_local_ip_w},
        { "link_local_ip_l",return_link_local_ip_l},
        { "radvd_status",return_radvd_status},
        { "ipv6_tunnel_lifetime", return_ipv6_tunnel_lifetime},
#ifdef IPV6_DSLITE
        { "aftr_address",return_aftr_address},
#endif
#endif
#ifdef XML_AGENT
        { "login_salt",return_login_salt_random},
#endif
#ifdef GUI_LOGIN_ALERT
        /* NickChou 20090723
        If user login fail at login.asp,
        GUI redirect to login.asp again and show alert messages
        */
        { "gui_login_alert",return_gui_login_alert},
#endif
#ifdef AUTHGRAPH/* jimmy added 20081121, for graphic auth */
        { "graph_auth_id",return_graph_auth_id},
#endif
#if defined(HAVE_HTTPS)
        { "support_https",return_support_https},
#endif
    { "russia_wan_phy_ipaddr", return_russia_wan_phy_ipaddr},
#ifdef CONFIG_USER_WEB_FILE_ACCESS
				{ "storage_get_number_of_dev", return_storage_get_number_of_dev},
				{ "storage_get_space", return_storage_get_space},
				{ "storage_wan_ip", return_storage_wan_ip},
#endif
        { NULL,NULL}
};
/* CmoGetStatus */
/**************************************************************/
/**************************************************************/

/* CmoGetList */
/**************************************************************/
/**************************************************************/
list_handler_t list_handlers[]={
        { "routing_table", return_routing_table },
        { "dhcpd_leased_table", return_dhcpd_leased_table },
        { "wireless_ap_scan_list",return_wireless_ap_scan_list},
        { "upnp_portmap_table", return_upnp_portmap_table},
        { "vlan_list", return_vlan_table},
        { "napt_session_table", return_napt_session_table },
        { "internet_session_table", return_internet_session_table },
        { "igmp_table", return_igmp_table }
#ifdef CONFIG_USER_WISH
        ,{ "wish_sessions_table", return_wish_sessions_table }
#endif
#ifdef CONFIG_USER_IP_LOOKUP
        ,{ "local_lan_ip", return_client_list_table }
#endif
#ifdef IPv6_SUPPORT
        ,{ "ipv6_client_list",return_ipv6_client_list_table}
        ,{ "ipv6_routing_table",return_ipv6_routing_table}
#endif
#ifdef USB_STORAGE_HTTP_ENABLE
        ,{ "usb_storage_list", return_usb_storage_list }		
#endif//USB_STORAGE_HTTP_ENABLE	
};
/* CmoGetList */
/**************************************************************/
/**************************************************************/

/* no find used */
/**************************************************************/
/**************************************************************/
dhcpd_leased_table_t dhcpd_leased_table[] ={
        {"localhost","192.168.0.1","00:00:00:00:00:00","expired_at"},\
        {"localhost","192.168.0.100","00:00:00:00:00:00","expired_at"}
};

wireless_stations_table_t wireless_stations_table[] ={
        {"00:00:00:00:00:00","100","2.4G"},\
        {"11:00:22:00:33:00","200","5G"}
};

condition_status_t condition = {"connected","connecting","disconnected"};
/* no find used */
/**************************************************************/
/**************************************************************/

#ifdef MIII_BAR
void return_getFileList(webs_t wp,char *args)
{
	return;			
}	
void return_uploadFile(webs_t wp,char *args)
{
	return;			
}	
void return_downloadFile(webs_t wp,char *args)
{
	return;			
}	
void return_createDirectory(webs_t wp,char *args)
{
	return;			
}	
void return_getRouter(webs_t wp,char *args)
{
	websWrite(wp,"%s,", "_getData_xxxx()");		
	return;			
}	
void return_listDevices(webs_t wp,char *args)
{
	return;			
}	
void return_deleteFiles(webs_t wp,char *args)
{
	return;			
}	
void return_renameFile(webs_t wp,char *args)
{
	return;			
}	
void return_copyFilesTo(webs_t wp,char *args)
{
	return;			
}	
void return_moveFilesTo(webs_t wp,char *args)
{
	return;			
}	
void return_getVersion(webs_t wp,char *args)
{
	return;			
}	
void return_uploadTo(webs_t wp,char *args)
{
	return;			
}	
void return_updateRid(webs_t wp,char *args)
{
	return;			
}	
void return_updateStatusTo(webs_t wp,char *args)
{
	return;			
}	
#endif

/* for GPL used */
/**************************************************************/
/**************************************************************/
/*
    Albert add for GPL customer request
    2009-04-15
*/
/* CmoGetCfg */
/**************************************************************/
/**************************************************************/
extern mapping_t customer_ui_to_nvram[];
extern int customer_variable_num;
void check_customer_variable(){
        char *value=NULL;
        mapping_t *customer = NULL;

        char *value_buf =NULL;
        char value_tmp[MAX_INPUT_LEN]={};

        for (customer = customer_ui_to_nvram; customer < &customer_ui_to_nvram[customer_variable_num]; customer++)
        {
                if (customer == NULL)
                        break;
                if((value = websGetVar(wp, customer->nvram_mapping_name, NULL)))
                {

                        value_buf = value;

                        /* protect the vlaue error */
                        if(value_buf && strcmp(value_buf,"WDB8WvbXdHtZyM8Ms2RENgHlacJghQyG"))
                        {
                                sprintf(value_tmp,"%s",value_buf);
                                /* ----------------------------------- */
                                nvram_set(customer->nvram_mapping_name, value_tmp);
                                printf("%s = %s \n",customer->nvram_mapping_name, value_tmp);
                        }

                }
        }
}
/* CmoGetCfg */
/**************************************************************/
/**************************************************************/

/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Added for GPL CmoGetStatus.
*   Notice :
*/
/* CmoGetStatus */
/**************************************************************/
/**************************************************************/
extern status_handler_t customer_status_handlers[];
void check_customer_status_handler(webs_t wp, char *name, char *value)
{
        status_handler_t *status_handler = customer_status_handlers;

        for (; status_handler && status_handler->pattern; status_handler++) {
                if (strcmp(status_handler->pattern, name) == 0) {
                        status_handler->output(wp, value);
                        break;
                }
        }

        return;
}
/* CmoGetStatus */
/**************************************************************/
/**************************************************************/

/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Added for GPL CmoGetList.
*   Notice :
*/
/* CmoGetList */
/**************************************************************/
/**************************************************************/
extern list_handler_t customer_list_handlers[];
void check_customer_list_handler(webs_t wp, char *name, char *value)
{
        list_handler_t *list_handler = customer_list_handlers;

        for (; list_handler && list_handler->pattern; list_handler++) {
                if (strcmp(list_handler->pattern, name) == 0) {
                        list_handler->output(wp, value);
                        break;
                }
        }

        return;
}
/* CmoGetList */
/**************************************************************/
/**************************************************************/
/* for GPL used */
/**************************************************************/
/**************************************************************/


void set_basic_api(webs_t wp)
{
        char *value=NULL;
        mapping_t *m = NULL;

        char *webs_buf=NULL;
        int webs_buf_offset=0;
        char *value_buf =NULL;
        int i;
    char value_tmp[MAX_INPUT_LEN]={};
        int year = 2006,month = 4, date = 12, hour = 12, min = 24, sec = 12;
#if defined(CONFIG_USER_3G_USB_CLIENT) && (defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE))
        char new_lan_ip[18]={'\0'};
        char *cur_lan_ipaddr = NULL;
#endif
#ifndef DHCPD_CHECK_RESERV_BY_GUI
/*
*       Date: 2009-7-3
*       Name: NickChou
*       Modify: 1. If Answer Y => Check dhcpd reservation table IP setting by GUI.
*                  If Answer N => When device's IP change,
*                  then force to clean dhcpd reservation table automatically.
*/
        int dhcpResevationClearFlag = 0;
#endif

#ifdef CONFIG_USER_DROPBEAR
        char *username, *password;
        int logout_event = 0;
#endif /* CONFIG_USER_DROPBEAR */

#ifdef PORT_FORWARD_TO_LOG
        char *enable="", *name="", *protocol_type="", *tcp_port_range="", *udp_port_range="", *ip="", *sch="", *inbound="";
        char *public_port_start="", *public_port_end="", *private_port_start="", *private_port_end="", *getValue;
#endif
        websBufferInit(wp);
        if (!webs_buf) {
                websWrite(wp, "out of memory<br>");
                websDone(wp, 0);
                return;
        }

        clean_all_wep_flag();

#ifdef USER_WL_ATH_5GHZ
        for(i = 0 ; i < 23 ;i++)
#else
        for(i = 0 ; i < 13 ;i++)
#endif
        {
                value = websGetVar(wp, wepkey_info[i].wlan_name, NULL);
                if(value)
                        if (strcmp(value, wepkey_info[i].wlan_value) == 0)
                                wepkey_info[i].set_wlan_flag();
        }

        p_wlan_wep_flag_info->ASCII_flag[0] = g_ASCII_flag;
#ifdef USER_WL_ATH_5GHZ
        p_wlan_wep_flag_info->a_ASCII_flag[0] = a_ASCII_flag;
#endif

        p_wlan_wep_flag_info->wep64_flag[0] = g_wep64_flag;
        p_wlan_wep_flag_info->wep128_flag[0] = g_wep128_flag;
        p_wlan_wep_flag_info->wep152_flag[0] = g_wep152_flag;

        for (m = ui_to_nvram; m < &ui_to_nvram[ARRAYSIZE(ui_to_nvram)]; m++)
        {
                if((value = websGetVar(wp, m->nvram_mapping_name, NULL)))
                {
                        /* As long as this statment be true, we set the value that we got into nvram. */
                        /* system setting */
                        if(!strcmp(m->nvram_mapping_name,"system_time")) {
                                sscanf(value,"%d/%d/%d/%d/%d/%d",&year,&month,&date,&hour,&min,&sec);
                                sprintf(value_tmp,"%02d%02d%02d%02d%04d",month,date,hour,min+1,year);
                                _system("date -s %s ",value_tmp);
                        }

                        if(!strcmp(value,"0.0.0.0") && strncmp(m->nvram_mapping_name,"asp_temp_",9))
                        {
                                if((strstr(m->nvram_mapping_name,"_dns") == 0)
#ifdef CONFIG_USER_SNMP_TRAP
                           || (strstr(m->nvram_mapping_name,"snmp_trap_receiver_") == 0)
#endif
)
                                   continue;
                        }

                        if(strstr(m->nvram_mapping_name, "wep")) // ex. wlan0_wep64_key_1 = value
                                value_buf = convert_webvalue_to_nvramvalue(m->nvram_mapping_name, value);
                        else
                                value_buf = value;

                        /* protect the vlaue error */
                        if (value_buf && strcmp(value_buf,"WDB8WvbXdHtZyM8Ms2RENgHlacJghQyG"))
                        {
                                /* replace anti-single-quote (`) to (\`) */
                                /* jimmy modified 20080721 */
                                //sprintf(value_tmp,"%s",substitute_keyword(value_buf,"`","\\`"));
                                sprintf(value_tmp,"%s",value_buf);
                                /* ----------------------------------- */

                        extern int change_admin_password_flag;
                        if(!strcmp(m->nvram_mapping_name,"admin_username"))
                        {
                                if(nvram_match("admin_username",value) !=0){
                                        change_admin_password_flag = 1;
#ifdef CONFIG_USER_DROPBEAR
                                        logout_event = 1;
#endif
                                }
                        }

                        if(!strcmp(m->nvram_mapping_name,"admin_password"))
                        {
                                if(nvram_match("admin_password",value) !=0){
                                        change_admin_password_flag = 1;
#ifdef CONFIG_USER_DROPBEAR
                                        logout_event = 1;
#endif
                                }
                        }


                                nvram_set(m->nvram_mapping_name, value_tmp);
                                printf("%s = %s \n",m->nvram_mapping_name, value_tmp);
/*      Date:   2009-06-09
*       Name:   jimmy huang
*       Reason: Avoid miniupnpd heavy loading cause not response some UPnP soap
*                       We decrease the frequency for open nvram.conf
*                       When http change nvram value, send signal to miniupnpd-1.3
*/
#ifdef UPNP_CHECK_VS_NVRAM
                                if(strncmp(m->nvram_mapping_name,"vs_rule",7) == 0){
                                        system("killall -SIGUSR2 miniupnpd");
                                }
#endif
/*  Date: 2009-3-31
*   Name: Ken Chiang
*   Reason: Added the port forward rule to syslog when port forword is save the rule.
*   Notice :
*/
#ifdef PORT_FORWARD_TO_LOG
                                if(strstr(m->nvram_mapping_name,"port_forward")){
                                        if(strlen(value_tmp)){//NULL
                                                if(strcmp(value_tmp, "0//0.0.0.0/0/0/Always/Allow_All")){//No Default
                                                        getStrtok(value_tmp, "/", "%s %s %s %s %s %s %s %s",&enable,&name,&ip,&tcp_port_range,&udp_port_range,&sch,&inbound);
                                                        printf("Enable=%s,Name=%s,IP=%s,TCP=%s,UDP=%s,Schedule=%s,Inbound=%s\n",enable,name,ip,tcp_port_range,udp_port_range,sch,inbound);
                                                        syslog(LOG_INFO,"PORT FORWARD:Enable=%s,Name=%s,IP=%s,TCP=%s,UDP=%s,Schedule=%s,Inbound=%s\n",enable,name,ip,tcp_port_range,udp_port_range,sch,inbound);
                                                }
                                        }
                                }

#endif
/*      Date:   2010-01-22
*       Name:   jimmy huang
*       Reason: Add support for Windows Mobile 5
*                       Because WM5 will OFFER 192.168.0.0/24 which is confilict with our lan ip range
*                       iphone iwll OFFER 192.168.20.0/24
*                       Thus we have to force to change our lan ip to 192.168.99.0/24
*       Note:   Maybe we can move the mechanism to WEB GUI and using javascript to implement ?
*/
#if defined(CONFIG_USER_3G_USB_CLIENT) && (defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE))
                                if(strncmp(m->nvram_mapping_name,"usb_type",8) == 0){
                                        cur_lan_ipaddr = nvram_safe_get("lan_ipaddr");
                                        switch(atoi(value_tmp)){
                                                case 0:
                                                case 1:
                                                case 2:
#ifdef UBICOM_TWOMII
                                                        nvram_set("wan_eth","eth1");
#else
                                                        nvram_set("wan_eth","eth0.1");
#endif
                                                        break;
                                                case 3:
                                                /*      Date:   2010-02-05
                                                 *      Name:   Cosmo Chang
                                                 *      Reason: add usb_type=5 for Android Phone RNDIS feature
                                                 */
                                                case 5:
                                                        /* case 3 -> windows mobile phone 5 */
                                                        /* case 5 -> android phone */
                                                        // windows mobile phone 5, OFFER 192.168.0.x
                                                        //if(strncmp(cur_lan_ipaddr,"192.168.99.1",12) != 0){
                                                        if(strncmp(cur_lan_ipaddr,"192.168.0.",10) == 0){
                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("lan_ipaddr", new_lan_ip);

                                                                cur_lan_ipaddr = nvram_safe_get("dhcpd_start");
                                                                memset(new_lan_ip,'\0',sizeof(new_lan_ip));
                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("dhcpd_start", new_lan_ip);

                                                                cur_lan_ipaddr = nvram_safe_get("dhcpd_end");
                                                                memset(new_lan_ip,'\0',sizeof(new_lan_ip));
                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("dhcpd_end", new_lan_ip);

                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("dhcpd_start", "192.168.99.100");
                                                                nvram_set("dhcpd_end", "192.168.99.199");
                                                        }
                                                        nvram_set("wan_eth","rndis0");
                                                        break;
                                                case 4: // iphone, OFFER 192.168.20.x
                                                        //if(strncmp(cur_lan_ipaddr,"192.168.99.1",12) != 0){
                                                        if(strncmp(cur_lan_ipaddr,"192.168.20.",11) == 0){
                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("lan_ipaddr", new_lan_ip);

                                                                cur_lan_ipaddr = nvram_safe_get("dhcpd_start");
                                                                memset(new_lan_ip,'\0',sizeof(new_lan_ip));
                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("dhcpd_start", new_lan_ip);

                                                                cur_lan_ipaddr = nvram_safe_get("dhcpd_end");
                                                                memset(new_lan_ip,'\0',sizeof(new_lan_ip));
                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("dhcpd_end", new_lan_ip);

                                                                change_ip_class_c(cur_lan_ipaddr,99,new_lan_ip);
                                                                nvram_set("dhcpd_start", "192.168.99.100");
                                                                nvram_set("dhcpd_end", "192.168.99.199");
                                                        }
                                                        nvram_set("wan_eth","eth2");
                                                        break;
                                                default:
                                                        break;
                                        }
                                }
#endif
                        }

#ifndef DHCPD_CHECK_RESERV_BY_GUI
                        if(!strcmp(m->nvram_mapping_name,"lan_ipaddr"))
                        {
                            printf("current ip = %s , nvram ip = %s \n", value, read_ipaddr_no_mask(nvram_safe_get("lan_bridge")));
                            if(strcmp(value, read_ipaddr_no_mask(nvram_safe_get("lan_bridge"))))
                               dhcpResevationClearFlag = 1;
                        }
#endif

#ifdef SET_GET_FROM_ARTBLOCK
//Albert modify : i could not allow user can modify ART block
//                      if(!strcmp(m->nvram_mapping_name,"wan_mac"))
//                      {
//                              printf("current wanmac = %s \n", value);
//                              if(!artblock_set("wan_mac",value)){
//                                      printf("artblock_set set wanmac = %s fail\n", value);
//                              }
//                      }
#endif
                }
        }

/*
        *       Date: 2010-02-02
        *   Name: Jimmy Huang
        *   Reason: When wan_proto is not usb3g_phone Make sure wan_eth is eth0.1
        *   Notice :
        */
#if defined(CONFIG_USER_3G_USB_CLIENT) && (defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE))
        if(nvram_match("wan_proto","usb3g_phone") != 0){
#ifdef UBICOM_TWOMII
                nvram_set("wan_eth","eth1");
#else
                nvram_set("wan_eth","eth0.1");
#endif
        }
#endif

/*
    Albert add for GPL customer request
    2009-04-15
*/
        check_customer_variable();

#ifndef DHCPD_CHECK_RESERV_BY_GUI
    if (dhcpResevationClearFlag)
    {
                unlink("/var/misc/udhcpd.leases");
                unlink("/var/misc/html.leases");
                /* remove old dhcpd reservation list */
                reset_dhcpd_resevation_rule();
    }
#endif

        nvram_flag_set();
        websBufferFlush(wp);

#ifdef CONFIG_USER_DROPBEAR
        if (logout_event)
        {
                _system("killall dropbear &");
                username = nvram_safe_get("admin_username");
                password = nvram_safe_get("admin_password");
#if 1
                /* /etc/passwd should have Admin entry with root permission */
                if (strcmp("Admin", username) )
                        username = "Admin";
                if (strlen(password) == 0) /* empty password */
                        password = "password";
#endif
                _system("dropbearkey -t rsa -f /tmp/dropbear_key");
                _system("dropbear -r /tmp/dropbear_key -U %s -u %s",
                        username,
                        password);
        }
#endif /* CONFIG_USER_DROPBEAR */
}

char *StrEncode(char *to, char *from, int en_nbsp)
{
        char *p = to;

        while (*from)
        {
                switch (*from)
                {
                        case '<':
                                strcpy(p, "&lt;");
                                p += 4;
                                break;
                        case '>':
                                strcpy(p, "&gt;");
                                p += 4;
                                break;
                        case '&':
                                strcpy(p, "&amp;");
                                p += 5;
                                break;
                        case '"':
                                strcpy(p, "&quot;");
                                p += 6;
                                break;
                        case ' ':
                                if (en_nbsp)
                                {
                                        strcpy(p, "&nbsp;");
                                        p += 6;
                                        break;
                                }
                                // fall through to default
                        default:
                                *p++ = *from;
                }

                from++;
        }
        *p = '\0';

        return to;
}

#ifdef HTTPD_USED_MUTIL_AUTH
int auth_login_find(char *mac_addr)
{
        int i;
        DEBUG_MSG("mac_addr=%s\n",mac_addr);
        for(i=0;i<MAX_AUTH_NUMBER;i++){
                if(strlen(auth_login[i].mac_addr)){
                        DEBUG_MSG("auth_login[%d].mac_addr=%s\n",i,auth_login[i].mac_addr);
                        if(strstr(auth_login[i].mac_addr,mac_addr)){
                                DEBUG_MSG("index=%d\n",i);
                                return i;
                        }
                }
        }
        return -1;
}

int auth_login_search(void)
{
        int i;
        for(i=0;i<MAX_AUTH_NUMBER;i++){
                if(!strlen(auth_login[i].mac_addr)){
                        DEBUG_MSG("index=%d\n",i);
                        return i;
                }
        }
        return -1;
}

int auth_login_search_long(void)
{
        int i,minidx=0,mintime=0;
        for(i=0;i<(MAX_AUTH_NUMBER-1);i++){
                DEBUG_MSG("mintime=%d\n",mintime);
                if(auth_login[i].logintime <= auth_login[i+1].logintime){
                        if(mintime){
                                if(auth_login[i].logintime < mintime){
                                        mintime=auth_login[i].logintime;
                                        minidx=i;
                                }
                        }else{
                                mintime=auth_login[i].logintime;
                                minidx=i;
                        }
                        DEBUG_MSG("< auth_login[%d].logintime=%d,minidx=%d,mintime=%d\n",i,auth_login[i].logintime,minidx,mintime);
                }
                else{
                        if(mintime){
                                if(auth_login[i+1].logintime < mintime){
                                        mintime=auth_login[i+1].logintime;
                                        minidx=i+1;
                                }
                        }else{
                                mintime=auth_login[i+1].logintime;
                                minidx=i+1;
                        }
                        DEBUG_MSG("> auth_login[%d].logintime=%d,minidx=%d,mintime=%d\n",i,auth_login[i].logintime,minidx,mintime);
                }
        }
        return minidx;
}
#endif//#ifdef HTTPD_USED_MUTIL_AUTH

#ifdef USB_STORAGE_HTTP_ENABLE
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file can uesd https.
*   Notice :
*/
#if defined(HAVE_HTTPS)
extern int openssl_request;
#endif
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
extern int lan_request;
extern void usb_storge_http_get_wanip(char *wan_ipaddr);
void return_usb_storage_list(webs_t wp,char *args){
	int list_flag=0;
	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0},listtemp[512]={0};
	char wan_ipaddr[34]={0}, wan_ipaddrport[40]={0};
	FILE *fp=NULL;
	char *delim = "*5#x";

	if(nvram_match("usb_storage_http_enable", "1")){
		printf("usb_storage_http_disable\n");
		websWrite(wp,"");
		return;
	}	
	system("df -k |grep /tmp/sd > /var/df.txt");	
	system("df -k |grep /tmp/mm >> /var/df.txt");

	fp = fopen("/var/df.txt","r");

	if(fp == NULL){
		DEBUG_MSG("\"%s\" does not exist\n","/var/df.txt");
		printf("\"%s\" does not exist\n","/var/df.txt");
		websWrite(wp,"");
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{			
			list_flag++;
			DEBUG_MSG("temp=%s\n",temp);
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			if(!lan_request){		
				DEBUG_MSG("%s: lan_request=0\n",__func__);	
				usb_storge_http_get_wanip(wan_ipaddr);	
				sprintf(wan_ipaddrport,"%s:%s",wan_ipaddr, nvram_safe_get("remote_http_management_port"));
#if defined(HAVE_HTTPS)
				if(openssl_request)
					sprintf(listtemp,"https://%s%s%s%s%s%s%s%s",wan_ipaddrport, mount, delim, filesystem, delim, totalsize, delim, freesize);
				else	
#endif				
					sprintf(listtemp,"http://%s%s%s%s%s%s%s%s",wan_ipaddrport, mount, delim, filesystem, delim, totalsize, delim, freesize);
			}else{
				DEBUG_MSG("%s: lan_request=1\n",__func__);
			#if defined(HAVE_HTTPS)
							if(openssl_request)
								sprintf(listtemp,"https://%s%s%s%s%s%s%s%s",nvram_safe_get("lan_ipaddr"), mount, delim, filesystem, delim, totalsize, delim, freesize);
							else	
			#endif	
					sprintf(listtemp,"http://%s%s%s%s%s%s%s%s",nvram_safe_get("lan_ipaddr"), mount, delim, filesystem, delim, totalsize, delim, freesize);
			}	
			DEBUG_MSG("listtemp=(%s)\n", listtemp);
			websWrite(wp,"%s,",listtemp);
			memset(temp,0,sizeof(temp));
		}
		if(!list_flag)
			websWrite(wp,"");
		fclose(fp);		
	}		
}		
#endif//USB_STORAGE_HTTP_ENABLE	 

/* compare admin or user */
static char *user_define(char *name,char *value){
        int index;
        //printf(":user_define:name=%s,value=%s\n",name,value);
        if(value != NULL){
                if(strcmp(name,"admin_username") == 0){
#ifdef HTTPD_USED_MUTIL_AUTH
                        DEBUG_MSG("user_define: MUTIL_AUTH is admin_username\n");
                        if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 ){
                                DEBUG_MSG("user_define: index=%d\n",index);
                                if(strcmp(value,auth_login[index].curr_user) == 0){
                                        DEBUG_MSG("user_define: curr_user=%s\n",auth_login[index].curr_user);
                                        value = "admin";}
                                }
                        }
#else
                        if(strcmp(value,auth_login.curr_user) == 0){
                                value = "admin";}}
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
                else if(strcmp(name,"user_username") == 0){
#ifdef HTTPD_USED_MUTIL_AUTH
                        DEBUG_MSG("user_define: MUTIL_AUTH is user_username\n");
                        if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 ){
                                DEBUG_MSG("user_define: index=%d\n",index);
                                if(strcmp(value,auth_login[index].curr_user) == 0){
                                        DEBUG_MSG("user_define: curr_user=%s\n",auth_login[index].curr_user);
                                        value = "user";}
                                }
                        }
#else
                        if(strcmp(value,auth_login.curr_user) == 0){
                                value = "user";}}
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
                else{
/*
*       Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
                        if(strcmp(name,"sp_admin_username") == 0){
#ifdef HTTPD_USED_MUTIL_AUTH
                        DEBUG_MSG("user_define: MUTIL_AUTH is sp_admin_username\n");
                        if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 ){
                                DEBUG_MSG("user_define: index=%d\n",index);
                                if(strcmp(value,auth_login[index].curr_user) == 0){
                                        DEBUG_MSG("user_define: curr_user=%s\n",auth_login[index].curr_user);
                                        value = "sp_admin";}
                                }
                        }
#else
                                if(strcmp(value,auth_login.curr_user) == 0){
                                        value = "sp_admin";}}
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
#endif
                        return NULL;
                }
        }else
                return NULL;

        return value;
}

/*      Date: 2009-02-12
*       Name: jimmy huang
*       Reason: for, if any model doesn't enable AuthGraph support
*                       we need to return nvram graph_auth_enable value is 0
*                       to UI, so that login.asp won't display AuthGraph
*                       related info in UI
*       Note:   Add the codes below
*/
#ifndef AUTHGRAPH
static char graph_auth_is_disable[2]="0";
#endif

/* CmoGetCfg */
/**************************************************************/
/**************************************************************/
/* transfer hex to 10 */
int hexTO10(char *hex_1,char *hex_0)
{
        int hexType_c;
        char    hexType_lower[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
        char    hexType_upper[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
        int hexTypeMap[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        char    *hexTypeLower_ptr = hexType_lower;
        char    *hexTypeUpper_ptr = hexType_upper;
        int     h_1 = -1;
        int     h_0 = -1;

        for(hexType_c = 0;hexType_c < 16;hexType_c++)
        {
                if(*(hexTypeLower_ptr+hexType_c) == *hex_1 || *(hexTypeUpper_ptr+hexType_c) == *hex_1)
                {
                        h_1 = hexTypeMap[hexType_c];
                        break;
                }
        }

        for(hexType_c = 0;hexType_c < 16;hexType_c++)
        {
                if(*(hexTypeLower_ptr+hexType_c) == *hex_0 || *(hexTypeUpper_ptr+hexType_c) == *hex_0)
                {
                        h_0 = hexTypeMap[hexType_c];
                        break;
                }
        }

        return (h_1*16 + h_0);
}

/* transfer nvram value to specified type */
void transfer_nvram_value(const char *start,char *mode,char *result){
        int i;
        int c;
        char value_tmp[64] = {};
        int _10code;

        switch(*mode){
                case 'h':
                        c=1;break;
                case 'a':
                        c=2;break;
                default:
                        return;
        }
        for(i=0;i<strlen(start);i=i+c){
                switch(*mode){
                        case 'h':
                                sprintf(value_tmp,"%s%02x",value_tmp,*(start+i));
                                break;
                        case 'a':
                                _10code = hexTO10(start+i,start+i+1);
                                if(_10code < 0 || _10code > 128)
                                {
                                        DEBUG_MSG("hexto10 exceeds ASCII code\n");
                                        _10code =0;
                                }
                                sprintf(value_tmp,"%s%c",value_tmp,_10code);
                                break;
                        default:
                                return;
                }
        }
        strcpy(result,value_tmp);
}

int ej_cmo_exec_cgi(int eid, webs_t wp, int argc, char_t **argv)
{
	int i;
	FILE *fp;
	char cmds[256] = "", buf[512];
	
	for (i = 0; i < argc; i++) {
		strcat(cmds, argv[i]);
		strcat(cmds, " ");
	}
	/* ignored a space char end of @cmds eg: "exe arg0 arg1 " */
	//fprintf(stderr, "### [%s]\n", cmds);

	if ((fp = popen(cmds, "r")) == NULL)
		goto err;	/* output "" */
	while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
		websWrite(wp, "%s", buf);
	}
	pclose(fp);
	//websWrite(wp, "1.2.2.3.4.5.6");
	
err:
	return 0;

}
/* Function name : ej_cmo_get_cfg
 *  API name : CmoGetCfg
 *  Function descryption: This function is used to get related configuration of the nvram in UI interface.
 *  Input: apiName which is defined in structure mapping.
 *  Output: return corresponding value according to input apiname.
 *  Author: Roy Tseng
 */
int ej_cmo_get_cfg(int eid, webs_t wp, int argc, char_t **argv)
{
        char *name=NULL;
        char *value_type=NULL;
        static char *value = NULL;
        char result[64];
        int args = 0;
        int ret = 0;
        int none_type = 0;
        int i;

    char str_encode_buf[256]={0};
        char value_tmp[MAX_INPUT_LEN]={};
        char value_tmp_lt[MAX_INPUT_LEN]={};
        char value_tmp_gt[MAX_INPUT_LEN]={};
        char value_tmp_amp[MAX_INPUT_LEN]={};

  /* format error : CmoGetCfg() , no arg */
        if ((args=ejArgs(argc, argv, "%s %s", &name, &value_type)) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }/* CmoGetCfg("name") */
        else if(args==1){
                DEBUG_MSG("%s: args==1\n",__func__);
                assert(name);
                value =  user_define(name,nvram_get(name)) ? : nvram_get(name) ;
                none_type =1;
        }/* CmoGetCfg("name","none"),CmoGetCfg("name","ascii"),CmoGetCfg("name","hex") */
        else if(args==2){
                DEBUG_MSG("%s: args==2\n",__func__);
                assert(name);
                assert(value_type);
                //value =  user_define(name,nvram_get(name)) ? : nvram_get(name) ;
#ifdef SET_GET_FROM_ARTBLOCK
                if(!strcmp(name,"wan_mac"))     {
                        //printf("artblock_get wanmac\n");
                        if(!(value =artblock_get("wan_mac"))){
                                value = nvram_get(name);
                        }
                }
                else if(!strcmp(name,"lan_mac")){
                        //printf("artblock_get lanmac\n");
                        if(!(value =artblock_get("lan_mac"))){
                                value = nvram_get(name);
                        }
                }
                /*      Date: 2009-02-12
                *       Name: jimmy huang
                *       Reason: for, if any model doesn't enable AuthGraph support
                *                       we need to return nvram graph_auth_enable value is 0
                *                       to UI, so that login.asp won't display AuthGraph
                *                       related info in UI
                *       Note:   Add the codes below
                */
#ifndef AUTHGRAPH
                else if(strcmp(name,"graph_auth_enable") == 0){
                        value=graph_auth_is_disable;
                }
#endif //AuthGraph
                else
                        value = nvram_get(name);
#else
                /*      Date: 2009-02-12
                *       Name: jimmy huang
                *       Reason: for, if any model doesn't enable AuthGraph support
                *                       we need to return nvram graph_auth_enable value is 0
                *                       to UI, so that login.asp won't display AuthGraph
                *                       related info in UI
                *       Note:   Add the codes below
                */
#ifndef AUTHGRAPH
                if(strcmp(name,"graph_auth_enable") == 0){
                        value=graph_auth_is_disable;
                }else
#endif // AuthGraph
                value = nvram_get(name);

#endif // SET_GET_FROM_ARTBLOCK

                if( value!= NULL && strlen(value) != 0){
                        if (strcmp(value_type, "none") ==0 ) {
                                none_type =1;
                        } else if(strncmp(value_type,"ascii",5) == 0){
                                DEBUG_MSG("hex to ascii: %s\n", value);
                                DEBUG_MSG("before transform %s\n", value);
                                if((strlen(value))%2 == 1)
                                {
                                        /* transform to ascii */
                                        DEBUG_MSG("add zero for odd nvram value\n");
                                        *(value + strlen(value)) = '0';

                                }

                                transfer_nvram_value(value,"a",result);
                                value =result;
                                DEBUG_MSG("after transform to ascii %s\n", value);
                                if(strstr(name, "wep64") != NULL)
                                {
                                        value[5] ='\0';
                                        if(strlen(value) < 5);
                                        {
                                                DEBUG_MSG("add zero for wlan0_wep64_key\n");
                                                for(i =strlen(value) ; i < 5 ; i++)
                                                        *(value+i) = '0';
                                        }
                                }
                                if(strstr(name, "wep128") != NULL)
                                {
                                        value[13] ='\0';
                                        if(strlen(value) < 13);
                                        {
                                                DEBUG_MSG("add zero for wlan0_wep128_key\n");
                                                for(i =strlen(value) ; i < 13 ; i++)
                                                        *(value+i) = '0';
                                        }

                                }

                                if(strstr(name, "wep152") != NULL)
                                {
                                        value[16] ='\0';
                                        if(strlen(value) < 16);
                                        {
                                                DEBUG_MSG("add zero for wlan0_wep152_key\n");
                                                for(i =strlen(value) ; i < 16 ; i++)
                                                        *(value+i) = '0';
                                        }

                                }

                        }
                }
        }
        else{
                websError(wp, 400, "Too much args\n");
                return -1;
        }

        if (value && none_type == 0){
                StrEncode(str_encode_buf, value, 1);
                ret += websWrite(wp, "%s", str_encode_buf);
        }
        else
        {
            if (value != NULL)
            {
                /* restore (\`) to anti-single-quote (`) && check quote && check (<) && check (>) && check (&) */

                        /* jimmy modified 20080721 */
                        //sprintf(value_tmp,"%s",substitute_keyword(value,"\\`","`"));

                        sprintf(value_tmp,"%s",value);
                        /* ------------------------- */
                        sprintf(value_tmp_amp,"%s",substitute_keyword(value_tmp,"\&","&amp;"));
                        sprintf(value_tmp_lt,"%s",substitute_keyword(value_tmp_amp,"\<","&lt;"));
                        sprintf(value_tmp_gt,"%s",substitute_keyword(value_tmp_lt,"\>","&gt;"));
                        ret += websWrite(wp, "%s", checkQuot(name,value_tmp_gt));
                }else{
                        return -1;
                }
        }

        return ret;
}
/* CmoGetCfg */
/**************************************************************/
/**************************************************************/


/* no find used */
/**************************************************************/
/**************************************************************/
extern int read_fw_success;
extern const char VER_FIRMWARE[];
extern const char VER_DATE[];
extern const char VER_BUILD[];
extern const char HW_VERSION[];

extern const unsigned char __CAMEO_WIRELESS_VERSION__[];
extern const unsigned char __CAMEO_WIRELESS_BUILD__[];
extern const unsigned char __CAMEO_WIRELESS_DATE_[];

extern int firmware_upgrade;
extern int configuration_upload;

void return_firmware_upgrade(webs_t wp) {
        websWrite(wp, "%d",  firmware_upgrade);
}

void return_configuration_upload(webs_t wp) {
        websWrite(wp, "%d",  configuration_upload);
}

void return_wlan_sta(webs_t wp){
                websWrite(wp, "%s", "100");
}
/* no find used */
/**************************************************************/
/**************************************************************/

/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetStatus can used the args.
*   Notice :
*/
/* CmoGetStatus */
/**************************************************************/
/**************************************************************/
void return_wan_current_ipaddr_00(webs_t wp,char *args){
        websWrite(wp, "%s", read_current_ipaddr(0) );
}

#ifdef MPPPOE
void return_wan_current_ipaddr_01(webs_t wp,char *args){
        websWrite(wp, "%s", read_current_ipaddr(1) );
}
void return_wan_current_ipaddr_02(webs_t wp,char *args){
        websWrite(wp, "%s", read_current_ipaddr(2) );
}
void return_wan_current_ipaddr_03(webs_t wp,char *args){
        websWrite(wp, "%s", read_current_ipaddr(3) );
}
void return_wan_current_ipaddr_04(webs_t wp,char *args){
        websWrite(wp, "%s", read_current_ipaddr(4) );
}
void return_lan_tx_bytes (webs_t wp,char *args){
        eByteType lanBytes = TXBYTES;
        char lanTxBytes[128] = {};
        FILE *fp;
        char *buf[128] = {};
        int result = 0;

        strcpy(lanTxBytes,display_lan_bytes(lanBytes,LAN_TX_BYTES));
        if(reset_lan_tx_flag)
        {
                reset_lan_tx_flag = 0;
                inner_lan_tx_flag = 1;
                websWrite(wp, "0");
        }
        else
        {
                if(inner_lan_tx_flag)
                {
                        fp = fopen(TX_BYTES,"r");
                        if(fp)
                        {
                                fgets(buf,128,fp);
                                fclose(fp);
                                result = atoi(lanTxBytes) - atoi(buf);
                                printf("LAN BEFORE BYTES = %s\nAFTERS BYTES = %s\nRESULT BYTES = %d\n",buf,lanTxBytes,result);
                                memset(buf,0,128);
                                if(result > 0)
                                {
                                        sprintf(buf,"%d",result);
                                        websWrite(wp, "%s", buf);
                                }
                                else
                                        websWrite(wp, "0");
                        }
                }
                else
                {
                        printf("NOT PRESS RESET BUTTON , TOTAL BYTES = %s\n",lanTxBytes);
                        websWrite(wp, "%s", lanTxBytes);
                }
        }

}
void return_lan_rx_bytes (webs_t wp,char *args){
        eByteType lanBytes = RXBYTES;
        char lanRxBytes[128] = {};
        FILE *fp;
        char *buf[128] = {};
        int result = 0;

        strcpy(lanRxBytes,display_lan_bytes(lanBytes,LAN_RX_BYTES));
        if(reset_lan_rx_flag)
        {
                reset_lan_rx_flag = 0;
                inner_lan_rx_flag = 1;
                websWrite(wp, "0");
        }
        else
        {
                if(inner_lan_rx_flag)
                {
                        fp = fopen(RX_BYTES,"r");
                        if(fp)
                        {
                                fgets(buf,128,fp);
                                fclose(fp);
                                result = atoi(lanRxBytes) - atoi(buf);
                                printf("LAN BEFORE BYTES = %s\nAFTERS BYTES = %s\nRESULT BYTES = %d\n",buf,lanRxBytes,result);
                                memset(buf,0,128);
                                if(result > 0)
                                {
                                        sprintf(buf,"%d",result);
                                        websWrite(wp, "%s", buf);
                                }
                                else
                                        websWrite(wp, "0");
                        }
                }
                else
                {
                        printf("NOT PRESS RESET BUTTON , TOTAL BYTES = %s\n",lanRxBytes);
                        websWrite(wp, "%s", lanRxBytes);
                }
        }


}
void return_wan_tx_bytes (webs_t wp,char *args){
        eByteType wanBytes = TXBYTES;
        char wanTxBytes[128] = {};
        FILE *fp;
        char *buf[128] = {};
        int result = 0,i=0;
        strcpy(wanTxBytes,display_wan_bytes(wanBytes,WAN_TX_BYTES));
        if(reset_wan_tx_flag)
        {
                reset_wan_tx_flag = 0;
                inner_wan_tx_flag = 1;
                websWrite(wp, "0");
        }
        else
        {
                if(inner_wan_tx_flag)
                {
                        fp = fopen(TX_BYTES,"r");
                        if(fp)
                        {
                                for(i=0;i<2;i++)
                                {
                                        memset(buf,0,128);
                                        fgets(buf,128,fp);
                                }

                                fclose(fp);
                                result = atoi(wanTxBytes) - atoi(buf);
                                printf("WAN   BEFORE BYTES = %s\nAFTERS BYTES = %s\nRESULT BYTES = %d\n",buf,wanTxBytes,result);
                                memset(buf,0,128);
                                if(result > 0)
                                {
                                        sprintf(buf,"%d",result);
                                        websWrite(wp, "%s", buf);
                                }
                                else
                                        websWrite(wp, "0");
                        }
                }
                else
                {
                        printf("NOT PRESS RESET BUTTON , TOTAL BYTES = %s\n",wanTxBytes);
                        websWrite(wp, "%s", wanTxBytes);
                }
        }


}
void return_wan_rx_bytes (webs_t wp,char *args){
        eByteType wanBytes = RXBYTES;
        char wanRxBytes[128] = {};
        FILE *fp;
        char *buf[128] = {};
        int result = 0,i=0;
        strcpy(wanRxBytes,display_wan_bytes(wanBytes,WAN_RX_BYTES));
        if(reset_wan_rx_flag)
        {
                reset_wan_rx_flag = 0;
                inner_wan_rx_flag = 1;
                websWrite(wp, "0");
        }
        else
        {
                if(inner_wan_rx_flag)
                {
                        fp = fopen(RX_BYTES,"r");
                        if(fp)
                        {
                                for(i=0;i<2;i++)
                                {
                                        memset(buf,0,128);
                                        fgets(buf,128,fp);
                                }

                                fclose(fp);
                                result = atoi(wanRxBytes) - atoi(buf);
                                printf("WAN   BEFORE BYTES = %s\nAFTERS BYTES = %s\nRESULT BYTES = %d\n",buf,wanRxBytes,result);
                                memset(buf,0,128);
                                 if(result > 0)
                                {
                                        sprintf(buf,"%d",result);
                                        websWrite(wp, "%s", buf);
                                }
                                else
                                        websWrite(wp, "0");
                        }
                }
                else
                {
                        printf("NOT PRESS RESET BUTTON , TOTAL BYTES = %s\n",wanRxBytes);
                        websWrite(wp, "%s", wanRxBytes);
                }
        }

}
void return_wlan_tx_bytes (webs_t wp,char *args){
        eByteType wlanBytes = TXBYTES;
        char wlanTxBytes[128] = {};
        FILE *fp;
        char *buf[128] = {};
        int result = 0,i = 0;

        if(nvram_match("wlan0_enable","1") == 0)
                strcpy(wlanTxBytes,display_wlan_bytes(wlanBytes,WLAN_TX_BYTES));
        else
                strcpy(wlanTxBytes,"0");
        if(reset_wlan_tx_flag)
        {
                reset_wlan_tx_flag = 0;
                inner_wlan_tx_flag = 1;
                websWrite(wp, "0");
        }
        else
        {
                if(inner_wlan_tx_flag)
                {
                        fp = fopen(TX_BYTES,"r");
                        if(fp)
                        {
                                for(i=0;i<3;i++)
                                {
                                        memset(buf,0,128);
                                        fgets(buf,128,fp);
                                }

                                fclose(fp);
                                result = atoi(wlanTxBytes) - atoi(buf);
                                printf("WLAN   BEFORE BYTES = %s\nAFTERS BYTES = %s\nRESULT BYTES = %d\n",buf,wlanTxBytes,result);
                                memset(buf,0,128);
                                 if(result > 0)
                                {
                                        sprintf(buf,"%d",result);
                                        websWrite(wp, "%s", buf);
                                }
                                else
                                        websWrite(wp, "0");

                        }
                }
                else
                {
                        printf("NOT PRESS RESET BUTTON , TOTAL BYTES = %s\n",wlanTxBytes);
                        websWrite(wp, "%s", wlanTxBytes);
                }
        }

}
void return_wlan_rx_bytes (webs_t wp,char *args){
        eByteType wlanBytes = RXBYTES;
        char wlanRxBytes[128] = {};
        FILE *fp;
        char *buf[128] = {};
        int result = 0,i = 0;

      if(nvram_match("wlan0_enable","1") == 0)
                strcpy(wlanRxBytes,display_wlan_bytes(wlanBytes,WLAN_RX_BYTES));
        else
                strcpy(wlanRxBytes,"0");

        if(reset_wlan_rx_flag)
        {
                reset_wlan_rx_flag = 0;
                inner_wlan_rx_flag = 1;
                websWrite(wp, "0");
        }
        else
        {
                if(inner_wlan_rx_flag)
                {
                        fp = fopen(RX_BYTES,"r");
                        if(fp)
                        {
                                for(i=0;i<3;i++)
                                {
                                        memset(buf,0,128);
                                        fgets(buf,128,fp);
                                }

                                fclose(fp);
                                result = atoi(wlanRxBytes) - atoi(buf);
                                printf("WLAN   BEFORE BYTES = %s\nAFTERS BYTES = %s\nRESULT BYTES = %d\n",buf,wlanRxBytes,result);
                                memset(buf,0,128);
                                 if(result > 0)
                                {
                                        sprintf(buf,"%d",result);
                                        websWrite(wp, "%s", buf);
                                }
                                else
                                        websWrite(wp, "0");

                        }
                }
                else
                {
                        printf("NOT PRESS RESET BUTTON , TOTAL BYTES = %s\n",wlanRxBytes);
                        websWrite(wp, "%s", wlanRxBytes);
                }
        }


}
#endif //#ifdef MPPPOE

void return_reboot_needed(webs_t wp,char *args)
{
	websWrite(wp, "%d", webserver_reboot_needed);
}

void return_lan_ip_info(webs_t wp,char *args)
{
        websWrite(wp, "%s", read_lan_ip_info());
}

void return_read_fw_success(webs_t wp,char *args)
{
        websWrite(wp, "%d", read_fw_success);
}

void return_version(webs_t wp,char *args){
        websWrite(wp, "%s", VER_FIRMWARE);
}

void return_build(webs_t wp,char *args){
        websWrite(wp, "%s", VER_BUILD);
}

void return_version_date(webs_t wp,char *args){
        websWrite(wp, "%s", VER_DATE);
}

void return_hw_version(webs_t wp,char *args){
#ifdef SET_GET_FROM_ARTBLOCK
        if(artblock_get("hw_version"))
                websWrite(wp, "%s", artblock_get("hw_version"));
        else
                websWrite(wp, "%s", HW_VERSION);
#else
        websWrite(wp, "%s", HW_VERSION);
#endif
}

#ifdef OPENDNS

char opendns_token[32+1]; // 32 bytes
int opendns_token_time;
int opendns_token_failcode;

void return_opendns_token_failcode(webs_t wp,char *args){
    websWrite(wp, "%d", opendns_token_failcode);

}

void return_opendns_token(webs_t wp,char *args){
	//

#if 0
	opendns_token_time = time((time_t *) NULL);

	char tmp[16];

	sprintf(tmp,"%08x",rand());
	strcpy(opendns_token,tmp);
	sprintf(tmp,"%08x",rand());
	strcat(opendns_token,tmp);

	opendns_token_failcode = 0;
#endif

	int now_time;

	now_time = time((time_t *) NULL);

	int create_new;

	create_new = 0;
	
	if( now_time > (opendns_token_time + (30 * 60)) ) {
		// timeout 30*60 seconds
		create_new = 1;
	}

	// check token
	if( strlen(opendns_token) < 8) {
		// token fail
		create_new = 1;
	}


	if( create_new ) {
		char tmp[16];
DEBUG_MSG("create_new\n");
		sprintf(tmp,"%08x",rand());
		strcpy(opendns_token,tmp);
		sprintf(tmp,"%08x",rand());
		strcat(opendns_token,tmp);
	}

	
	opendns_token_failcode = 0;

	opendns_token_time = time((time_t *) NULL);




    websWrite(wp, "%s", opendns_token);
}

void return_opendns_token_get(webs_t wp,char *args){
	//

    websWrite(wp, "%s", opendns_token);
}

#endif

void return_title(webs_t wp,char *args){
        websWrite(wp, "%s", WEB_TITLE);
}

void return_gui_logout(webs_t wp,char *args){
        websWrite(wp, "%s", "180;URL=logout.cgi");
}

void return_copyright(webs_t wp,char *args){
        websWrite(wp, "%s", WEB_COPYRIGHT);
}

void return_wireless_driver_info(webs_t wp,char *args){
        websWrite(wp, "%s, %s, %s",
                        __CAMEO_WIRELESS_VERSION__,
                        __CAMEO_WIRELESS_BUILD__,
                        __CAMEO_WIRELESS_DATE_);
}

void return_wireless_domain(webs_t wp,char *args){
        char *value = NULL;
        char buf[64]={0};
        int domain=0x3a;

#ifdef SET_GET_FROM_ARTBLOCK
        if(!(value = artblock_get("wlan0_domain")))
                value = nvram_get("wlan0_domain");
#else
                value = nvram_get("wlan0_domain");
#endif

        if(value != NULL)
        {
                sscanf(value, "%x", &domain);
                sprintf(buf, "%s, %s", value, (char *) GetDomainName(domain));
        }
        else
        {
                strcpy(buf, "0x10, FCC1_FCCA");
        }
        websWrite(wp, "%s", buf);
}

void return_wireless_mac(webs_t wp,char *args){

        unsigned char macaddr[32];
        get_if_macaddr(IFTYPE_WLAN, macaddr);
        websWrite(wp, "%s", macaddr);
}

void return_lan_tx_packets (webs_t wp,char *args){
        ePktType lanPkt = TXPACKETS;
        char *lanTxPkts = display_lan_pkts(lanPkt) ? : "0";
        websWrite(wp, "%s", lanTxPkts);
}

void return_lan_rx_packets (webs_t wp,char *args){
        ePktType lanPkt = RXPACKETS;
        char *lanRxPkts = display_lan_pkts(lanPkt) ? : "0";
        websWrite(wp, "%s", lanRxPkts);
}

void return_lan_lost_packets(webs_t wp,char *args){
        ePktType lanPkt = LOSSPACKETS;
        char *lanLossPkts = display_lan_pkts(lanPkt) ? : "0";
        websWrite(wp, "%s", lanLossPkts);
}

void return_wan_tx_packets (webs_t wp,char *args){
        ePktType wanPkt = TXPACKETS;
        char *wanTxPkts = display_wan_pkts(wanPkt) ? : "0";
        websWrite(wp, "%s", wanTxPkts);
}

void return_wan_rx_packets (webs_t wp,char *args){
        ePktType wanPkt = RXPACKETS;
        char *wanRxPkts = display_wan_pkts(wanPkt) ? : "0";
        websWrite(wp, "%s", wanRxPkts);
}

void return_wan_lost_packets(webs_t wp,char *args){
        ePktType wanPkt = LOSSPACKETS;
        char *wanLossPkts = display_wan_pkts(wanPkt) ? : "0";
        websWrite(wp, "%s", wanLossPkts);
}

void return_wlan_tx_packets (webs_t wp,char *args){
        ePktType wlanPkt = TXPACKETS;
        char *wlanTxPkts = display_wlan_pkts(wlanPkt) ? : "0";
        websWrite(wp, "%s", wlanTxPkts);
}

void return_wlan_rx_packets (webs_t wp,char *args){
        ePktType wlanPkt = RXPACKETS;
        char *wlanRxPkts = display_wlan_pkts(wlanPkt) ? : "0";
        websWrite(wp, "%s", wlanRxPkts);
}

void return_wlan_lost_packets(webs_t wp,char *args){
        ePktType wlanPkt = LOSSPACKETS;
        char *wlanLossPkts = display_wlan_pkts(wlanPkt) ? : "0";
        websWrite(wp, "%s", wlanLossPkts);
}

void return_wlan_tx_frames (webs_t wp,char *args){
        eFrameType wlanFrames = TXFRAMES;
        char *wlanTxFrames = display_wlan_frames(wlanFrames) ? : "0";
        websWrite(wp, "%s", wlanTxFrames);
}

void return_wlan_rx_frames (webs_t wp,char *args){
        eFrameType wlanFrames = RXFRAMES;
        char *wlanRxFrames = display_wlan_frames(wlanFrames) ? : "0";
        websWrite(wp, "%s", wlanRxFrames);
}

void return_wan_ifconfig_info(webs_t wp,char *args){
        eIfType IfType = IFTYPE_ETH_WAN;
        char *wan_info = display_ifconfig_info(IfType) ? : "0/0/0/0/0/0";
        websWrite(wp, "%s", wan_info);
}

void return_lan_ifconfig_info(webs_t wp,char *args){
        eIfType IfType = IFTYPE_ETH_LAN;
        char *lan_info = display_ifconfig_info(IfType) ? : "0/0/0/0/0/0";
        websWrite(wp, "%s", lan_info);
}

void return_wlan_ifconfig_info(webs_t wp,char *args){
        eIfType IfType = IFTYPE_WLAN;
        char *wlan_info = display_ifconfig_info(IfType) ? : "0/0/0/0/0/0";
        websWrite(wp, "%s", wlan_info);
}

//5G interface 2
void return_wlan1_ifconfig_info(webs_t wp,char *args){
        eIfType IfType = IFTYPE_WLAN1;
        char *wlan_info = display_ifconfig_info(IfType) ? : "0/0/0/0/0/0";
        websWrite(wp, "%s", wlan_info);
}

#ifdef USER_WL_ATH_5GHZ
void return_wlan1_mac_addr(webs_t wp,char *args)
{
    char wlan1_mac[32];

    get_ath_5g_macaddr(wlan1_mac);

    websWrite(wp, "%s", wlan1_mac);
}
#endif

void return_guestzone_mac(webs_t wp,char *args)
{
        static unsigned char guest_mac[20]={0};
        char *ath_if="ath0";

#ifdef USER_WL_ATH_5GHZ 
        if(atoi(args) == 1){// 5GHz Guest Zone
                if(atoi(nvram_safe_get("wlan1_enable")) == 0){
                        websWrite(wp, "%s", guest_mac);        
                        return;
                }
                
                if( atoi(nvram_safe_get("wlan0_enable")) == 1 ){
                        if(atoi(nvram_safe_get("wlan0_vap1_enable")) == 1)
                                ath_if = "ath3";
                        else
                                ath_if = "ath2";
                }
                else
                        ath_if = "ath1";
        }
        else //24GHz Guest Zone
#endif
        { 
                if(atoi(nvram_safe_get("wlan0_enable")) == 0){
                        websWrite(wp, "%s", guest_mac);        
                        return;
                }
                
                if(atoi(nvram_safe_get("wlan1_enable")) == 1)
                        ath_if = "ath2";
                else
                        ath_if = "ath1";        
        }
        
        get_hw_addr(ath_if, guest_mac);
        websWrite(wp, "%s", guest_mac);                
}


#if defined(DIR865) || defined(EXVERSION_NNA)
void return_language(webs_t wp,char *args){
                websWrite(wp, "%s", nvram_safe_get("language"));
}
#endif //#ifdef DIR865

/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
void return_language_mtdblock_check(webs_t wp,char *args){
        int no_art=0;
  char cmd[256];
  FILE *fp = NULL;
        fp = fopen("/proc/mtd","r");
        if(fp)
        {
                while(!feof(fp)){
                        memset(cmd,'\0',sizeof(cmd));
                        fgets(cmd,sizeof(cmd),fp);
                        if(strstr(cmd, "mtd5"))
                        {
                                no_art = 1;
                                break;
                        }
                }
                fclose(fp);
        }
        else
                no_art = 0;

        websWrite(wp, "%d", no_art);
}
#endif
void return_fixip_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "static") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("fixip"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_dhcpc_connection_status(webs_t wp,char *args){

        memset(wan_proto, 0 , sizeof(wan_proto));
        strcpy(wan_proto, nvram_safe_get("wan_proto"));

        if(strcmp(wan_proto, "dhcpc") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("dhcpc"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_pppoe_00_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "pppoe") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("pppoe-00"));
                websWrite(wp, "%s", wan_status);
}
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_pppoe_01_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "pppoe") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("pppoe-01"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_pppoe_02_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "pppoe") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("pppoe-02"));
                websWrite(wp, "%s", wan_status);
}
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_pppoe_03_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "pppoe") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("pppoe-03"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_pppoe_04_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "pppoe") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("pppoe-04"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_l2tp_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "l2tp") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("l2tp"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_pptp_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "pptp") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("pptp"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}

#ifdef CONFIG_USER_BIGPOND//no used
void return_bigpond_connection_status(webs_t wp,char *args){
        if(strcmp(wan_proto, "bigpond") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("bigpond"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}
#endif

void return_get_current_user(webs_t wp,char *args){
#ifdef HTTPD_USED_MUTIL_AUTH
                int index;
                DEBUG_MSG("return_get_current_user: MUTIL_AUTH=%s\n",con_info.mac_addr);
                if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 ){
                        DEBUG_MSG("user_define: index=%d\n",index);
                        DEBUG_MSG("user_define: curr_user=%s\n",auth_login[index].curr_user);
                        websWrite(wp, "%s", auth_login[index].curr_user);
                }
                else{
                        DEBUG_MSG("user_define: nofind\n");
                        websWrite(wp, "%s", "admin");
                }
#else
        websWrite(wp, "%s", auth_login.curr_user);
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
}

void return_admin_password(webs_t wp,char *args)
{
	char admin_ps[20],user_ps[20],admin_en_ps[40],user_en_ps[40];
	memset(admin_ps,0,20);
	memset(user_ps,0,20);
	strcpy(admin_ps, nvram_safe_get("admin_password"));
	strcpy(user_ps, nvram_safe_get("admin_password"));
#ifdef HTTPD_USED_MUTIL_AUTH
                int index;
                if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 ){
                                        if(strcmp(auth_login[index].curr_user,"admin") == 0){
                                        	if(strlen(admin_ps) == 0)
                                        		websWrite(wp, "");
                                        	else
                                                	websWrite(wp, "%s/%d", b64_encode( admin_ps ,strlen(admin_ps), admin_en_ps, 40),strlen(admin_ps));
                                        }
                                        else{
                                                websWrite(wp, "%s", nvram_safe_get("user_password"));
                                        }
                }
                else{
                       	if(strlen(admin_ps) == 0)
                       		websWrite(wp, "");
                       	else
                               	websWrite(wp, "%s/%d", b64_encode( admin_ps ,strlen(admin_ps), admin_en_ps, 40),strlen(admin_ps));
                }
#else
{
                if(strcmp(auth_login.curr_user,"admin") == 0){
                       	if(strlen(admin_ps) == 0)
                       		websWrite(wp, "");
                       	else
                               	websWrite(wp, "%s/%d", b64_encode( admin_ps ,strlen(admin_ps), admin_en_ps, 40),strlen(admin_ps));
                }
                else{
                        websWrite(wp, "%s", nvram_safe_get("user_password"));
                }
}
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
}

void return_wep_key1(webs_t wp,char *args)
{
	char wep64_key1_ps[64],wep64_key1_en_ps[64];
	char wep128_key1_ps[64],wep128_key1_en_ps[64];
	
	memset(wep64_key1_ps, 0, 64);
	memset(wep64_key1_en_ps, 0, 64);
	strcpy(wep64_key1_ps, nvram_safe_get("wlan0_wep64_key_1"));
  		
	memset(wep128_key1_ps, 0, 64);
	memset(wep128_key1_en_ps, 0, 64);
	strcpy(wep128_key1_ps, nvram_safe_get("wlan0_wep128_key_1"));

	websWrite(wp, "%s/%s",  b64_encode( wep64_key1_ps ,strlen(wep64_key1_ps), wep64_key1_en_ps, 40), 
							b64_encode( wep128_key1_ps ,strlen(wep128_key1_ps), wep128_key1_en_ps, 40));
}
void return_wlan1_wep_key1(webs_t wp,char *args)
{
	char wep64_key1_ps[64],wep64_key1_en_ps[64];
	char wep128_key1_ps[64],wep128_key1_en_ps[64];
	
	memset(wep64_key1_ps, 0, 64);
	memset(wep64_key1_en_ps, 0, 64);
	strcpy(wep64_key1_ps, nvram_safe_get("wlan1_wep64_key_1"));
  		
	memset(wep128_key1_ps, 0, 64);
	memset(wep128_key1_en_ps, 0, 64);
	strcpy(wep128_key1_ps, nvram_safe_get("wlan1_wep128_key_1"));

	websWrite(wp, "%s/%s",  b64_encode( wep64_key1_ps ,strlen(wep64_key1_ps), wep64_key1_en_ps, 40), 
							b64_encode( wep128_key1_ps ,strlen(wep128_key1_ps), wep128_key1_en_ps, 40));
}

void return_wpa_psk_ps(webs_t wp,char *args)
{
	char wpa_psk_ps[120],wpa_en_ps[120], buf[120];
 	memset(wpa_psk_ps, 0, 120);
    memset(wpa_en_ps, 0, 120);
    
	strcpy(wpa_psk_ps, nvram_safe_get("wlan0_psk_pass_phrase"));
	strcpy(buf, b64_encode(wpa_psk_ps ,strlen(wpa_psk_ps), wpa_en_ps, 100));
	
    if (strlen(wpa_psk_ps) == 0)
        websWrite(wp, "");
    else
        websWrite(wp, "%s/%d", buf, strlen(wpa_psk_ps));
}

void return_wlan1_wpa_psk_ps(webs_t wp,char *args)
{
	char wpa_psk_ps[120],wpa_en_ps[120], buf[120];
 	memset(wpa_psk_ps, 0, 120);
    memset(wpa_en_ps, 0, 120);
    
	strcpy(wpa_psk_ps, nvram_safe_get("wlan1_psk_pass_phrase"));
	strcpy(buf, b64_encode(wpa_psk_ps ,strlen(wpa_psk_ps), wpa_en_ps, 100));
	
    if (strlen(wpa_psk_ps) == 0)
        websWrite(wp, "");
    else
        websWrite(wp, "%s/%d", buf, strlen(wpa_psk_ps));
}

void return_radius0_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan0_eap_radius_server_0"));
    strcpy(radius_server, strtok(radius_value, "/"));
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }	
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}

void return_wlan1_radius0_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan1_eap_radius_server_0"));
    strcpy(radius_server, strtok(radius_value, "/"));
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }	
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}

void return_radius1_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan0_eap_radius_server_1"));
    strcpy(radius_server, strtok(radius_value, "/"));
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}

void return_wlan1_radius1_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan1_eap_radius_server_1"));
    strcpy(radius_server, strtok(radius_value, "/"));
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}

//
void return_vap1_wep_key1(webs_t wp,char *args)
{
	char wep64_key1_ps[64],wep64_key1_en_ps[64];
	char wep128_key1_ps[64],wep128_key1_en_ps[64];
	
	memset(wep64_key1_ps, 0, 64);
	memset(wep64_key1_en_ps, 0, 64);
	strcpy(wep64_key1_ps, nvram_safe_get("wlan0_vap1_wep64_key_1"));
  		
	memset(wep128_key1_ps, 0, 64);
	memset(wep128_key1_en_ps, 0, 64);
	strcpy(wep128_key1_ps, nvram_safe_get("wlan0_vap1_wep128_key_1"));

	websWrite(wp, "%s/%s",  b64_encode( wep64_key1_ps ,strlen(wep64_key1_ps), wep64_key1_en_ps, 40), 
							b64_encode( wep128_key1_ps ,strlen(wep128_key1_ps), wep128_key1_en_ps, 40));
}

void return_wlan1_vap1_wep_key1(webs_t wp,char *args)
{
	char wep64_key1_ps[64],wep64_key1_en_ps[64];
	char wep128_key1_ps[64],wep128_key1_en_ps[64];
	
	memset(wep64_key1_ps, 0, 64);
	memset(wep64_key1_en_ps, 0, 64);
	strcpy(wep64_key1_ps, nvram_safe_get("wlan1_vap1_wep64_key_1"));
  		
	memset(wep128_key1_ps, 0, 64);
	memset(wep128_key1_en_ps, 0, 64);
	strcpy(wep128_key1_ps, nvram_safe_get("wlan1_vap1_wep128_key_1"));

	websWrite(wp, "%s/%s",  b64_encode( wep64_key1_ps ,strlen(wep64_key1_ps), wep64_key1_en_ps, 40), 
							b64_encode( wep128_key1_ps ,strlen(wep128_key1_ps), wep128_key1_en_ps, 40));
}

void return_vap1_wpa_psk_ps(webs_t wp,char *args)
{
	char wpa_psk_ps[120],wpa_en_ps[120], buf[120];
 	memset(wpa_psk_ps, 0, 120);
    memset(wpa_en_ps, 0, 120);
    
	strcpy(wpa_psk_ps, nvram_safe_get("wlan0_vap1_psk_pass_phrase"));
	strcpy(buf, b64_encode(wpa_psk_ps ,strlen(wpa_psk_ps), wpa_en_ps, 100));
	
    if (strlen(wpa_psk_ps) == 0)
        websWrite(wp, "");
    else
        websWrite(wp, "%s/%d", buf, strlen(wpa_psk_ps));
}

void return_wlan1_vap1_wpa_psk_ps(webs_t wp,char *args)
{
	char wpa_psk_ps[120],wpa_en_ps[120], buf[120];
 	memset(wpa_psk_ps, 0, 120);
    memset(wpa_en_ps, 0, 120);
    
	strcpy(wpa_psk_ps, nvram_safe_get("wlan1_vap1_psk_pass_phrase"));
	strcpy(buf, b64_encode(wpa_psk_ps ,strlen(wpa_psk_ps), wpa_en_ps, 100));
	
    if (strlen(wpa_psk_ps) == 0)
        websWrite(wp, "");
    else
        websWrite(wp, "%s/%d", buf, strlen(wpa_psk_ps));
}

void return_vap1_radius0_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan0_vap1_eap_radius_server_0"));
    strcpy(radius_server, strtok(radius_value, "/"));

    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }

    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}

void return_wlan1_vap1_radius0_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan1_vap1_eap_radius_server_0"));
    strcpy(radius_server, strtok(radius_value, "/"));

    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }

    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}

void return_vap1_radius1_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan0_vap1_eap_radius_server_1"));
    strcpy(radius_server, strtok(radius_value, "/"));
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }

    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}

void return_wlan1_vap1_radius1_ps(webs_t wp,char *args)
{
	char radius_value[120], radius_server[20],radius_port[8],radius_ps[120],radius_en_ps[120];
	char *ptr;
	memset(radius_ps, 0, 120);
    memset(radius_en_ps, 0, 120);
    
	strcpy(radius_value, nvram_safe_get("wlan1_vap1_eap_radius_server_1"));
    strcpy(radius_server, strtok(radius_value, "/"));
    
    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_port, ptr);
    }
    else {
    	websWrite(wp, "%s/1812/0/0", radius_server);
    	return;
    }

    ptr = strtok(NULL, "/");
    if (ptr) {
    	strcpy(radius_ps, ptr);
    	websWrite(wp, "%s/%s/%s/%d", radius_server, radius_port, b64_encode( radius_ps ,strlen(radius_ps), radius_en_ps, 100), strlen(radius_ps));
    }
    else {
    	websWrite(wp, "%s/%s/0/0", radius_server, radius_port);
    }
}


char error_message[128];
char error_setting[128];
void return_get_error_message(webs_t wp,char *args){
        websWrite(wp, "%s", error_message);
        memset(error_message, 0, sizeof(error_message));
}

void return_get_error_setting(webs_t wp,char *args){
        websWrite(wp, "%s", error_setting);
        memset(error_setting, 0, sizeof(error_setting));
}

void return_mac_clone_addr(webs_t wp,char *args){
        unsigned char mac[17];
        strcpy(mac, con_info.mac_addr);
        websWrite(wp, "%s", mac);
}

void return_mac_default_addr(webs_t wp,char *args)
{
    unsigned char mac[17];

#ifdef SET_GET_FROM_ARTBLOCK
    if (artblock_get("art_wan_mac"))
        strcpy(mac, artblock_safe_get("art_wan_mac"));
    else
#endif
        strcpy(mac, nvram_default_search("wan_mac"));

    websWrite(wp, "%s", mac);
}

void return_ping_ipaddr(webs_t wp,char *args){
        if( ping.ping_ipaddr != NULL && strcmp(ping.ping_ipaddr,"") != 0)
                websWrite(wp, "%s", ping.ping_ipaddr);
}

void return_ping_result(webs_t wp,char *args){
        if( ping.ping_ipaddr != NULL && strcmp(ping.ping_ipaddr,"") != 0){
                websWrite(wp, "%s", get_ping_app_stat(ping.ping_ipaddr));
                ping.ping_ipaddr = NULL;
        }
}

void return_ping_result_display(webs_t wp,char *args){
        if( ping.ping_ipaddr != NULL && strcmp(ping.ping_ipaddr,"") != 0){
                websWrite(wp, "%s", get_ping_app_display(ping.ping_ipaddr));
                ping.ping_ipaddr = NULL;
        }
}

#ifdef RADVD
void return_ping6_ipaddr(webs_t wp,char *args){
        if( ping.ping_ipaddr_v6 != NULL && strcmp(ping.ping_ipaddr_v6,"") != 0)
                websWrite(wp, "%s", ping.ping_ipaddr_v6);
}
void return_ping6_interface(webs_t wp,char *args){
        if( ping.interface != NULL && strcmp(ping.interface,"") != 0)
                websWrite(wp, "%s", ping.interface);
}
void return_ping6_size(webs_t wp,char *args){
                websWrite(wp, "%d", ping.ping6_size);
}
void return_ping6_count(webs_t wp,char *args){
                websWrite(wp, "%d", ping.ping6_count);
}
void return_ping6_result(webs_t wp,char *args){
        if( ping.ping_ipaddr_v6 != NULL && strcmp(ping.ping_ipaddr_v6,"") != 0 && ping.interface != NULL && strcmp(ping.interface,"") != 0)
        {
                if(strcmp(ping.interface,"LAN") == 0)
                websWrite(wp, "%s", get_ping6_app_stat(ping.ping_ipaddr_v6,nvram_safe_get("lan_bridge"),ping.ping6_size,ping.ping6_count));
                else if(strcmp(ping.interface,"WAN") == 0)
                        websWrite(wp, "%s", get_ping6_app_stat(ping.ping_ipaddr_v6,nvram_safe_get("wan_eth"),ping.ping6_size,ping.ping6_count));

                ping.ping_ipaddr_v6 = NULL;
                ping.interface = NULL;
                ping.ping6_size = 1500;
                ping.ping6_count = 1;
        }
}
void return_ping6_test_result(webs_t wp,char *args){
        if( ping.ping_ipaddr_v6 != NULL && strcmp(ping.ping_ipaddr_v6,"") != 0)
        {
               websWrite(wp, "%s", get_ping6_app_test(ping.ping_ipaddr_v6));
               ping.ping_ipaddr_v6 = NULL;
        }
}
void return_6to4_tunnel_address(webs_t wp,char *args)
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
                websWrite(wp, "2002:%s::%s",ipv6_6to4_prefix,ipv6_6to4_prefix);
        }
        else
        {
                printf("WAN not ready\n");
                websWrite(wp, "0:0:0:0:0:0:0:0");
        }
}
void return_6to4_lan_address(webs_t wp,char *args)
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
                websWrite(wp, "2002:%s:",ipv6_6to4_prefix);
        }
        else
        {
                printf("WAN not ready\n");
                websWrite(wp, "2002:0:0:");
        }
}
#endif//#ifdef RADVD

#ifdef IPV6_6RD
//copy from wide-dhcpv6/prefixconf.c
void copy_mac_to_eui64(char *mac, char *ifid)
{
	ifid[8] = mac[0];
	ifid[8] ^= 0x02; /* reverse the u/l bit*/
	ifid[9] = mac[1];
	ifid[10] = mac[2];
	ifid[11] = 0xff;
	ifid[12] = 0xfe;
	ifid[13] = mac[3];
	ifid[14] = mac[4];
	ifid[15] = mac[5];
}
//copy from wide-dhcpv6/prefixconf.c
int get_macaddr(char *if_name, char *mac)
{
	int sockfd;
	struct ifreq ifr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return -1;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
		close(sockfd);
		return -1;
	}   
	close(sockfd);
	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

	return 0;
}

void return_6rd_tunnel_address(webs_t wp,char *args)
{
	char *wan_if;
	char *wan_ipv4_ptr;
	char wan_ipv4_addr[20] = {};
	struct in_addr wan_ipv4_addr_in;

	char *tmp;
	char prefix[50];
	struct in6_addr prefix_in6;
	int prefix_len;

	char _6rd_addr[50];
	struct in6_addr _6rd_addr_in6;
	u_int32_t mask;

	char wan_mac[6]={};

	if(nvram_match("wan_proto","pppoe") == 0 || nvram_match("wan_proto","pptp") == 0 || nvram_match("wan_proto","l2tp") == 0 )
		wan_if = "ppp0";
	else
		wan_if = nvram_safe_get("wan_eth");

	wan_ipv4_ptr = get_ip_addr(wan_if);
	if(wan_ipv4_ptr)
	{
		strcpy(wan_ipv4_addr, wan_ipv4_ptr);
		inet_pton(AF_INET, wan_ipv4_addr, &wan_ipv4_addr_in);

		tmp = nvram_safe_get("ipv6_6rd_prefix");
		strcpy(prefix, tmp);
		strcat(prefix, "::");

		inet_pton(AF_INET6, prefix, &prefix_in6);

		prefix_len = atoi(nvram_safe_get("ipv6_6rd_prefix_length"));

		mask = 0xFFFFFFFF << (32-prefix_len);

		_6rd_addr_in6.s6_addr32[0] = (prefix_in6.s6_addr32[0] & mask);
		_6rd_addr_in6.s6_addr32[0] = _6rd_addr_in6.s6_addr32[0] | ((unsigned long long)wan_ipv4_addr_in.s_addr >> prefix_len);
		_6rd_addr_in6.s6_addr32[1] = wan_ipv4_addr_in.s_addr << (32-prefix_len);

		get_macaddr(wan_if, wan_mac);
		copy_mac_to_eui64(wan_mac, _6rd_addr_in6.s6_addr);
		inet_ntop(AF_INET6, &_6rd_addr_in6, _6rd_addr, sizeof(_6rd_addr));

		websWrite(wp, "%s", _6rd_addr);
	}
	else
	{
		printf("WAN not ready\n");
		websWrite(wp, "0:0:0:0:0:0:0:0");
	}
}

void return_6rd_lan_address(webs_t wp,char *args)
{
	char *lan_if;
	struct in_addr lan_ipv4_addr_in;

	char *tmp;
	char prefix[50];
	struct in6_addr prefix_in6;
	int prefix_len;

	char _6rd_addr[50];
	struct in6_addr _6rd_addr_in6;
	u_int32_t mask;

	char lan_mac[6]={};

	lan_if = nvram_safe_get("lan_bridge");
	inet_pton(AF_INET, nvram_safe_get("lan_ipaddr"), &lan_ipv4_addr_in);

	tmp = nvram_safe_get("ipv6_6rd_prefix");
	strcpy(prefix, tmp);
	strcat(prefix, "::");
	inet_pton(AF_INET6, prefix, &prefix_in6);

	prefix_len = atoi(nvram_safe_get("ipv6_6rd_prefix_length"));

	mask = 0xFFFFFFFF << (32-prefix_len);

	_6rd_addr_in6.s6_addr32[0] = (prefix_in6.s6_addr32[0] & mask);
	_6rd_addr_in6.s6_addr32[0] = _6rd_addr_in6.s6_addr32[0] | ((unsigned long long)lan_ipv4_addr_in.s_addr >> prefix_len);
	_6rd_addr_in6.s6_addr32[1] = lan_ipv4_addr_in.s_addr << (32-prefix_len);

	get_macaddr(lan_if, lan_mac);
	copy_mac_to_eui64(lan_mac, _6rd_addr_in6.s6_addr);
	inet_ntop(AF_INET6, &_6rd_addr_in6, _6rd_addr, sizeof(_6rd_addr));

	websWrite(wp, "%s", _6rd_addr);
}
#endif

#if 0//old systime
/* transfer system time format, ex: Tue Mar 28 10:37:00 UTC 2006 -> 2006/03/28/10/37 */
char *trnasfer_system_time(char *sysTime,char *sys_time){
        const int YEAR_START = 24;
        const int MONTH_START = 4;
        const int DATE_START = 9;
        const int TIME_START = 11;

        int i;
        int offset = 1;
        char sys_time_t[2] = {};
        char *month[] = {"Jan","Feb","Mar","Apr","May","Jun",\
                "Jul","Aug","Sep","Oct","Nov","Dec"};

        if(!(strncmp(sysTime+DATE_START-1," ",1)))              // if offset == 1 , date is more than 10
                offset = 0;
        else
                offset = 1;

        memcpy(sys_time,sysTime+YEAR_START,4);
        strcat(sys_time,"/");

        for(i=0;i<ARRAYSIZE(month);i++){                                                                            // match month
                if(!(strncmp(sysTime+MONTH_START,month[i],3))){
                        sprintf(sys_time,"%s%d%s",sys_time,i+1,"/");
                        break;
                }
        }

        if(!offset){                                                                                                                     // for case date is less than 10
                memcpy(sys_time_t,sysTime+DATE_START,1);
                sprintf(sys_time,"%s%s%s",sys_time,sys_time_t,"/");
        }else{                                                                                                                                                 // for case date is more than 10
                memcpy(sys_time_t,sysTime+DATE_START-1,1);
                sprintf(sys_time,"%s%s",sys_time,sys_time_t);
                memcpy(sys_time_t,sysTime+DATE_START,1);
                sprintf(sys_time,"%s%s%s",sys_time,sys_time_t,"/");
        }

        for(i=TIME_START;i<=TIME_START+6;i=i+3){
                memcpy(sys_time_t,sysTime+i,1);
                sprintf(sys_time,"%s%s",sys_time,sys_time_t);
                memcpy(sys_time_t,sysTime+i+1,1);
                if(i<TIME_START+6)
                        sprintf(sys_time,"%s%s%s",sys_time,sys_time_t,"/");
                else
                        sprintf(sys_time,"%s%s",sys_time,sys_time_t);
        }

        return sys_time;
}
#endif

void return_system_time(webs_t wp,char *args){
        char sys_time[50] = {};
#if 0//old systime
        FILE *fp;
        char systemTime[64]="2006/1/1/1/1/1";
        _system("date > /var/tmp/time.tmp &");
        fp = fopen("/var/tmp/time.tmp","r");

        if(fp) {
                fgets(systemTime, 64, fp);
                fclose(fp);
        }
        websWrite(wp, "%s", trnasfer_system_time(systemTime,sys_time));
#else
        time_t timep;
        struct tm *p;
        time(&timep);
//      p = gmtime(&timep);
        p = localtime(&timep);

        DEBUG_MSG("return_system_time: %d/%d/%d/%d/%d/%d\n",
                (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

        sprintf(sys_time, "%d/%d/%d/%d/%d/%d",
                (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

        websWrite(wp, "%s", sys_time);
#endif
}

void return_wlan0_channel_list(webs_t wp,char *args){
        char *value=NULL;
        char channel_list[128]={0};
        int domain=0x3a;

#ifdef SET_GET_FROM_ARTBLOCK
        value = artblock_get("wlan0_domain");
        if(value == NULL)
                value = nvram_get("wlan0_domain");
#else
        value = nvram_get("wlan0_domain");
#endif

        if (value != NULL)
                sscanf(value, "%x", &domain);
        else
                domain = 0x10;
/*
        Date: 2009-3-5
        Name: Ken_Chiang
        Reason: modify for show chanel list used the space gap.
        Notice :
*/
/*
        GetDomainChannelList(domain, WIRELESS_BAND_24G, channel_list);
*/
        GetDomainChannelList(domain, WIRELESS_BAND_24G, channel_list, 0);
        websWrite(wp, "%s", channel_list);
}

//5G interface 2
void return_wlan1_channel_list(webs_t wp,char *args){
        char *value=NULL;
        char channel_list[512]={0};
        int domain;

        /*
        *       Reason: DIR865 support different 5GHz Domain,
        *                       we also need to modify GUI for matching channel list
        */
#ifdef DIR865
        char *GHz_channel_ptr=NULL;
        char GHz_channel_list[512]={0};
        int GHz_channel=0;
#endif

#ifdef SET_GET_FROM_ARTBLOCK
        value = artblock_get("wlan0_domain");
        if(value==NULL)
                value = nvram_get("wlan0_domain");
#else
        value = nvram_get("wlan0_domain");
#endif

        if (value != NULL)
                sscanf(value, "%x", &domain);
        else
                domain = 0x37;
/*
        Date: 2009-3-5
        Name: Ken_Chiang
        Reason: modify for show chanel list used the space gap.
        Notice :
*/
/*
        GetDomainChannelList(domain, WIRELESS_BAND_50G, channel_list);
*/
        GetDomainChannelList(domain, WIRELESS_BAND_50G, channel_list ,0);

#ifdef DIR865
        DEBUG_MSG("channel_list=%s\n", channel_list);

        GHz_channel_ptr=strtok(channel_list, ",");
        GHz_channel=atoi(GHz_channel_ptr);
        if(GHz_channel>35 && GHz_channel<65)
                sprintf( GHz_channel_list, "5.%d GHz - CH %d,", 180+((GHz_channel-36)/4)*20,  GHz_channel);
        else if(GHz_channel>99 && GHz_channel<105)
                sprintf( GHz_channel_list, "5.%d GHz - CH %d,", 500+((GHz_channel-100)/4)*20,  GHz_channel);
        else if(GHz_channel>148 && GHz_channel<166)
                sprintf( GHz_channel_list, "5.%d GHz - CH %d,", 45+((GHz_channel-149)/4)*20,  GHz_channel);

        while((GHz_channel_ptr = strtok(NULL, ",")))
        {
                GHz_channel=atoi(GHz_channel_ptr);
                if(GHz_channel>35 && GHz_channel<65)
                        sprintf( GHz_channel_list, "%s5.%d GHz - CH %d,", GHz_channel_list, 180+((GHz_channel-36)/4)*20,  GHz_channel);
                else if(GHz_channel>99 && GHz_channel<105)
                        sprintf( GHz_channel_list, "%s5.%d GHz - CH %d,", GHz_channel_list, 500+((GHz_channel-100)/4)*20,  GHz_channel);
                else if(GHz_channel>148 && GHz_channel<166)
                        sprintf( GHz_channel_list, "%s5.%d GHz - CH %d,", GHz_channel_list, 745+((GHz_channel-149)/4)*20,  GHz_channel);
        }

        websWrite(wp, "%s", GHz_channel_list);
#else
        websWrite(wp, "%s", channel_list);
#endif
}

void return_wan_port_status(webs_t wp,char *args){
        char status[15];
        VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
        websWrite(wp, "%s", status);
}

void return_wan_port_speed_duplex(webs_t wp,char *args){
        char speed[5];
        char duplex[5];
        VCTGetPortSpeedState( nvram_safe_get("wan_eth"), VCTWANPORT0, speed, duplex);
        websWrite(wp, "%s|%s", speed, duplex);
}

void return_lan_port_status_00(webs_t wp,char *args){
        char status[15];
        VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT0, status);
        websWrite(wp, "%s", status);
}

void return_lan_port_status_01(webs_t wp,char *args){
        char status[15];
        VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT1, status);
        websWrite(wp, "%s", status);
}

void return_lan_port_status_02(webs_t wp,char *args){
        char status[15];
        VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT2, status);
        websWrite(wp, "%s", status);
}

void return_lan_port_status_03(webs_t wp,char *args){
        char status[15];
        VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT3, status);
        websWrite(wp, "%s", status);
}

void return_lan_port_speed_duplex_00(webs_t wp,char *args){
        char speed[5];
        char duplex[5];
        VCTGetPortSpeedState( nvram_get("lan_eth"), VCTLANPORT0, speed, duplex);
        websWrite(wp, "%s|%s", speed, duplex);
}

void return_lan_port_speed_duplex_01(webs_t wp,char *args){
        char speed[5];
        char duplex[5];
        VCTGetPortSpeedState( nvram_get("lan_eth"), VCTLANPORT1, speed, duplex);
        websWrite(wp, "%s|%s", speed, duplex);
}

void return_lan_port_speed_duplex_02(webs_t wp,char *args){
        char speed[5];
        char duplex[5];
        VCTGetPortSpeedState( nvram_get("lan_eth"), VCTLANPORT2, speed, duplex);
        websWrite(wp, "%s|%s", speed, duplex);
}

void return_lan_port_speed_duplex_03(webs_t wp,char *args){
        char speed[5];
        char duplex[5];
        VCTGetPortSpeedState( nvram_get("lan_eth"), VCTLANPORT3, speed, duplex);
        websWrite(wp, "%s|%s", speed, duplex);
}

void return_log_current_page(webs_t wp,char *args){
        websWrite(wp, "%d", logs_info.cur_page);
}

void return_log_total_page(webs_t wp,char *args){
        websWrite(wp, "%d", logs_info.total_page);
}

void return_current_channel(webs_t wp,char *args){
        char pchannel[20]={};
        if(nvram_match("wlan0_enable", "0")==0)
                websWrite(wp, "%s", pchannel);
        else
        {
                get_channel(pchannel);
                websWrite(wp, "%s", pchannel);
        }
}

void return_usb_test_result(webs_t wp,char *args){
                if(usb_test.result == NULL)
                        usb_test.result = "";
                websWrite(wp, "%s", usb_test.result);
                usb_test.result = NULL;
}

void return_lan_collisions(webs_t wp,char *args){
        eCollisionType collisions = LanCollision;
        char *lanCollisions = display_collisions(collisions) ? : "0";
        websWrite(wp, "%s", lanCollisions);
}

void return_wan_collisions(webs_t wp,char *args){
        eCollisionType collisions = WanCollision;
        char *wanCollisions = display_collisions(collisions) ? : "0";
        websWrite(wp, "%s", wanCollisions);
}

void return_wlan_collisions(webs_t wp,char *args){
        eCollisionType collisions = WlanCollision;
        char *wlanCollisions = display_collisions(collisions) ? : "0";
        websWrite(wp, "%s", wlanCollisions);
}

void return_dhcpc_obtain_time(webs_t wp,char *args){
        websWrite(wp, "%lu", lease_time_execute(2));
}

void return_dhcpc_expire_time(webs_t wp,char *args){
        websWrite(wp, "%lu", lease_time_execute(0));
}

void return_lan_uptime(webs_t wp,char *args){
                websWrite(wp, "%lu", uptime());
}

void return_wan_uptime(webs_t wp,char *args){
        if(strlen(wan_status) > 8)//{"Connected", "Connecting", "Disconnected"};
        {
                //if(strncmp(wan_status, "Connected", 9) == 0)
                        //websWrite(wp, "%lu", get_wan_connect_time(wan_proto, 0));
                //else
                        //websWrite(wp, "%s", "0");
                        websWrite(wp, "%lu", get_wan_connect_time(wan_proto, 1));
}
        else
        {
                memset(wan_proto, 0 , sizeof(wan_proto));
                strcpy(wan_proto, nvram_safe_get("wan_proto"));
                websWrite(wp, "%lu", get_wan_connect_time(wan_proto, 1));
        }
}

void return_wlan_uptime(webs_t wp,char *args){
                websWrite(wp, "%lu", uptime());
}

void return_pppoe_00_uptime(webs_t wp,char *args){
        char *status = NULL;
        status = wan_statue("pppoe-00");
        printf("status = %s\n",status);
        if(strncmp(status,"Connected",9) == 0)
                        websWrite(wp, "%lu", get_pppoe00_connect_time(PPPoE_DISCONNECT));
        else
                websWrite(wp, "%s", "Disconnected");
}

void return_internet_connect_type(webs_t wp,char *args)
{
        char status[15];
        char pppoeRes[5]={0}, dhcpRes[5]={0};
        FILE *fp;

        memset(status, 0, 15);
        VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
        if(strcmp(status, "disconnect") == 0)
        {
                printf("return not connected\n");
                websWrite(wp,"%s", "notconnected");
                return ;
        }
        else /* do pppoe and dhcp test */
        {
                /*pppoe server test*/
                system("pppoe-discovery");
                fp=fopen("/var/tmp/test_pppoe_res.txt","rb");
                if (fp == NULL)
                {
                        printf("test_pppoe_res.txt open failed. \n");
                        return ;
                }
                fread(pppoeRes, 1, 1, fp);
                printf("detect pppoe res = %s\n", pppoeRes);
                if(strcmp(pppoeRes, "1")==0)
                {
                        fclose(fp);
                        unlink("/var/tmp/test_pppoe_res.txt");
                        printf("return pppoe\n");
                        websWrite(wp,"%s", "pppoe");
                        return ;
                }
                /*pppoe server not exist, do dhcp server test*/
                system("killall -SIGALRM udhcpc");
                fp=fopen("/var/tmp/test_dhcp_res.txt","rb");
                if (fp == NULL)
                {
                        printf("test_dhcp_res.txt open failed. \n");
                        return ;
                }
                fread(dhcpRes, 1, 1, fp);
                if(strcmp(dhcpRes, "1")==0)
                {
                        fclose(fp);
                        unlink("/var/tmp/test_dhcp_res.txt");
                        printf("return dhcp\n");
                        websWrite(wp,"%s", "dhcp");
                        return ;
                }
                printf("return staticip\n");
                websWrite(wp,"%s", "staticip");
                return ;
        }
}

void return_internet_connect(webs_t wp,char *args){
        FILE *fp = NULL;
        unsigned char returnVal[15] = {};
        unsigned char ret[15] = {};
        char *tmp = NULL;

        fp = fopen("var/misc/ping_app.txt", "r");
        if(fp == NULL){
                websWrite(wp, "%s", "ERROR ACCESS");
                return;
        }
        fread(returnVal, sizeof(returnVal), 1, fp);
        fclose(fp);
        /* take of space and chang row */
        tmp = &returnVal[0];
        strncpy(ret,tmp,strlen(tmp)-2);
        websWrite(wp, "%s", ret);
}

void return_ddns_status(webs_t wp,char *args)
{
        FILE *fp;
        FILE *fp2;
        char wan_interface[8] = {};
        char ddns_status[64] = {0};
        char web[] = "168.95.1.1";
        char buf[256]={0}, host_ip[16]={0};
        char *ip_s=NULL, *ip_e=NULL;
        char *wan_proto=nvram_safe_get("wan_proto");
        char *ddns_type = nvram_safe_get("ddns_type");

        if( strcmp(wan_proto,"pppoe") == 0
                || strcmp(wan_proto,"pptp") == 0
                || strcmp(wan_proto,"l2tp") == 0
#ifdef CONFIG_USER_3G_USB_CLIENT
                || strcmp(wan_proto,"usb3g") == 0
#endif
                )
                strcpy(wan_interface,"ppp0");
        else
                strcpy(wan_interface,nvram_safe_get("wan_eth"));

        if(nvram_match("ddns_enable","1") != 0)
                websWrite(wp, "%s","Dynamic DNS service is not enabled.");
        else
        {
                if(!read_ipaddr(wan_interface))
                        websWrite(wp, "%s","No update action.There is no WAN IP address.");
                else
                {
                        if(strcmp(get_ping_app_stat(web), "Success") != 0)
                                websWrite(wp, "%s","Internet Offline");
                        else
                        {
                                fp = fopen("/var/tmp/ddns_status","r"); /*if we have updated, then this file is exist*/
                                if(fp)
                                {
                                        fgets(ddns_status, sizeof(ddns_status), fp);
                        fclose(fp);
                                if(strstr(ddns_status, "SUCCESS"))
                                                websWrite(wp, "%s", "success");
                                else if(strstr(ddns_status, "Authentication failed"))
                                        websWrite(wp, "%s", "Update Fail!");
                                else
                                        websWrite(wp, "%s", "The Host Name is invalid");
                          }
        else
        {
                                _system("ping -c 1 \"%s\" > /var/etc/ddns_check.txt 2>&1", nvram_safe_get("ddns_hostname"));
                                        fp2 = fopen("/var/etc/ddns_check.txt", "r");
                                        if(fp2)
                                        {
                                                memset(buf, 0, 80);
                                                fgets(buf, sizeof(buf), fp2);
                                                ip_s = strstr(buf, "(");
            ip_e = strstr(buf, ")");
                                                if(ip_s && ip_e)
                                                {
                                                          strncpy(host_ip, ip_s+1, ip_e-(ip_s+1));

                                                        if (strcasecmp( ddns_type, "DynDns.org" ) == 0 || strncasecmp(ddns_type, "dyndns.com", 10) ==0 )
                                                        {
                                                                FILE *fp_checkip = fopen("/var/etc/dyndns_checkip", "r");
                                                                char buf_checkip[20]={0};
                                                                if(fp_checkip)
                                                                {
                                                                        fgets(buf_checkip, sizeof(buf_checkip), fp_checkip);
                                                                        DEBUG_MSG("dyndns_checkip=%s=, host_ip=%s=\n", buf_checkip, host_ip);
                                                                        if(strncmp(buf_checkip, host_ip, strlen(host_ip))==0)
                                                                                websWrite(wp, "%s", "success");
                                                                        fclose(fp_checkip);
                                                                }
                                                                else
                                                                        websWrite(wp, "%s", "fail");
                                                        }
                                                        else if(strcmp(get_ipaddr(wan_interface), host_ip)==0)
                                                websWrite(wp, "%s", "success");
                                                }
                                                else
                                                        websWrite(wp, "%s", "fail");    //NickChou

                                                fclose(fp2);
                                        }
                                        else
                                        {
                                                printf("open /var/tmp/ddns_check.txt fail");
                                                websWrite(wp, "%s", "fail");    //NickChou
                                        }
        }
                        }
                }
        }
}

void return_wps_generate_pin_by_random (webs_t wp,char *args){
        char wps_pin[9];
        generate_pin_by_random(wps_pin);
        websWrite(wp, "%s", wps_pin);
}

/*
*       Date: 2009-5-20
*       Name: Ken_Chiang
*       Reason: modify for New WPS algorithm.
*       Notice :
*
*       Date: 2009-7-3
*       Name: Jerry_Shyu
*       Modify: 1. Use NVRAM entry "wps_pin_code_gen_interface"
*                  to select the interface to generate PIN code.
*/
void generate_pin_by_wan_mac(char *wps_pin)
{
        unsigned long int rnumber = 0;
        unsigned long int pin1 = 0,pin2 = 0;
        unsigned long int accum = 0;
        int i,digit = 0;
        char wan_mac[32]={0};
        char *mac_ptr = NULL;
        char *pin_if=nvram_get("wps_pin_code_gen_interface");

        //printf("%s:\n",__func__);
        //get mac
#ifdef SET_GET_FROM_ARTBLOCK
        if (!pin_if) {
                if(artblock_get("wan_mac"))
                        mac_ptr = artblock_safe_get("wan_mac");
                else
                        mac_ptr = nvram_safe_get("wan_mac");
        }else {
                if (0==nvram_match("wan_eth", pin_if)) {
                        if(artblock_get("wan_mac"))
                                mac_ptr = artblock_safe_get("wan_mac");
                        else
                                mac_ptr = nvram_safe_get("wan_mac");
                }else { //if (0==nvram_match("lan_eth", pin_if)) {
                        if(artblock_get("lan_mac"))
                                mac_ptr = artblock_safe_get("lan_mac");
                        else
                                mac_ptr = nvram_safe_get("lan_mac");
                }
        }
#else
        if (!pin_if) {
                mac_ptr = nvram_safe_get("wan_mac");
        }else {
                if (0==nvram_match("wan_eth", pin_if))
                        mac_ptr = nvram_safe_get("wan_mac");
                else //if (0==nvram_match("lan_eth", pin_if))
                        mac_ptr = nvram_safe_get("lan_mac");
        }
#endif
        printf("%s:mac_ptr=%s\n",__func__,mac_ptr);
        strncpy(wan_mac, mac_ptr, 18);
        mac_ptr = wan_mac;
        //remove ':'
        for(i =0 ; i< 5; i++ )
        {
                memmove(wan_mac+2+(i*2), wan_mac+3+(i*3), 2);
        }
        memset(wan_mac+12, 0, strlen(wan_mac)-12);
        //printf("%s:wan_mac=%x%x%x%x%x%x%x%x%x%x%x%x\n",__func__,
                        //wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5],
                        //wan_mac[6],wan_mac[7],wan_mac[8],wan_mac[9],wan_mac[10],wan_mac[11]);
        sscanf(wan_mac,"%06X%06X",&pin1,&pin2);

        pin2 ^= 0x55aa55;
        pin1 = pin2 & 0x00000F;
        pin1 = (pin1<<4) + (pin1<<8) + (pin1<<12) + (pin1<<16) + (pin1<<20) ;
        pin2 ^= pin1;
        pin2 = pin2 % 10000000; // 10^7 to get 7 number

        if( pin2 < 1000000 )
                pin2 = pin2 + (1000000*((pin2%9)+1)) ;

        //ComputeChecksum
        rnumber = pin2*10;
        accum += 3 * ((rnumber / 10000000) % 10);
        accum += 1 * ((rnumber / 1000000) % 10);
        accum += 3 * ((rnumber / 100000) % 10);
        accum += 1 * ((rnumber / 10000) % 10);
        accum += 3 * ((rnumber / 1000) % 10);
        accum += 1 * ((rnumber / 100) % 10);
        accum += 3 * ((rnumber / 10) % 10);
        digit = (accum % 10);
        pin1 = (10 - digit) % 10;

        pin2 = pin2 * 10;
        pin2 = pin2 + pin1;
        sprintf( wps_pin, "%08d", pin2 );
        //printf("%s:wps_pin=%s\n",__func__,wps_pin);
        return;
}

void return_wps_generate_pin_by_mac (webs_t wp,char *args){
        char wps_pin[10];
/*
*       Date: 2009-5-20
*       Name: Ken_Chiang
*       Reason: modify for New WPS algorithm.
*       Notice :
*/
/*
        generate_pin_by_mac(wps_pin);
*/
        generate_pin_by_wan_mac(wps_pin);
        websWrite(wp, "%s", wps_pin);
}

void return_wps_sta_enrollee_pin(webs_t wp,char *args){
        if( enrollee.pin != NULL ){
                set_sta_enrollee_pin(enrollee.pin);
                enrollee.pin = NULL;
        }
}

void return_wps_push_button_result (webs_t wp,char *args){
        int  result = 0;
        result = get_push_button_result();
        websWrite(wp, "%d", result);
}


char* online_firmware_check(char *iface, char *fw_url, char *result_file){
        FILE *fp;
        char result[1024]={0};
        _system("fwqd -i %s -u %s -r %s", iface, fw_url, result_file);
        if((fp = fopen(result_file,"r+"))==NULL){
                perror("online_firmware_check");
                return "ERROR";
        }
        if(!fgets(result, sizeof(result), fp)){
                printf("fw_check: fgets error\n");
                return "ERROR";
        }
        DEBUG_MSG("fw_check: %s", result);
        fclose(fp);
        return result;
}

void return_online_firmware_check(webs_t wp,char *args)
{
        char result[1024]={'\0'};
        char *check_fw_url = NULL;
        char *wan_eth = NULL;

        check_fw_url = nvram_safe_get("check_fw_url");
        wan_eth = nvram_safe_get("wan_eth");

        sprintf(result, "%s",online_firmware_check(wan_eth, check_fw_url, "/var/tmp/fw_check_result.txt"));
        DEBUG_MSG("%s\n",result);
                        websWrite(wp, "%s", result);
                        }

void return_internetonline_check(webs_t wp,char *args)
{
        char wan_interface[8] = {};
        char status[15] = {};
        char *wan_eth = NULL;
        char *wan_proto = NULL;

                wan_eth = nvram_safe_get("wan_eth");
        wan_proto = nvram_safe_get("wan_proto");

        if(strcmp(wan_proto, "dhcpc") == 0 || strcmp(wan_proto, "static") == 0)
                strcpy(wan_interface, wan_eth);
        else
                strcpy(wan_interface, "ppp0");

        /* Check phy connect statue */
        VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);

        if( strncmp("disconnect", status, 10) == 0)
                websWrite(wp, "%s","0"); /*Internet Offline*/
        else if(read_ipaddr(wan_interface) == NULL)
                websWrite(wp, "%s","0"); /*Internet Offline*/
        else
                        websWrite(wp, "%s","1"); /*Internet Online*/
}

#ifdef CONFIG_USER_TC
/*
 * return_qos_detected_line_type()
 *	(Ubicom Streamengine) Return the line type detected
 */
void return_qos_detected_line_type(webs_t wp,char *args){
	int type;
	type = detected_line_type();
	switch(type) {
	case 0:
		websWrite(wp, "Cable Or Other Broadband Network");
		return;
	case 1:
		websWrite(wp, "xDSL Or Other Frame Relay Network");
		return;
	}

	websWrite(wp, "Not detected");

}

void return_measured_uplink_speed (webs_t wp,char *args){
        char bandwidth[32];
#if CONFIG_USER_STREAMENGINE
        FILE *fp;
#endif
        measure_uplink_bandwidth(bandwidth);
        if(!strcmp(bandwidth,"0"))
        {
#if CONFIG_USER_STREAMENGINE
                fp = fopen("/var/tmp/ubicom_streamengine_calculated_rate.tmp","r");
                if (fp)
                {
                        fclose(fp);
                        websWrite(wp, "Your broadband internet connection has surpassed<br>&nbsp; the uplink measurement requirement.");
                }
                else
#endif
                websWrite(wp, "Not Estimated");
        }
        else
                websWrite(wp, "%s kbps", bandwidth);
}
void return_if_measuring_uplink_now (webs_t wp,char *args){
        int  measure_now = 0;
        measure_now = if_measure_now();
        websWrite(wp, "%d", measure_now);
}
void return_if_wan_ip_obtained (webs_t wp,char *args){
        int  obtain = 0;
        obtain = wan_ip_is_obtained();
        websWrite(wp, "%d", obtain);
}
#endif//#ifdef CONFIG_USER_TC

void return_average_bytes(webs_t wp,char *args)
{
        FILE *fp;
        char content[128] = {};
        fp = fopen(AVERAGE_BYTES,"r");
        if(fp)
        {
                fgets(content,128,fp);
                content[strlen(content)] = '\0';
                websWrite(wp, "%s", content);
        }
        fclose(fp);
}

void return_dns_query_result(webs_t wp,char *args)
{
        FILE *fp;
        char temp[256] = {};
        int total_line = 0;
        fp = fopen(DNS_QUERY_RESULT,"r");
        if(fp)
        {
                while(fgets(temp,256,fp))
                {
                        total_line++;
                        websWrite(wp,"%s",temp);
                        memset(temp,0,sizeof(temp));
                }
                if(total_line <= 3)
                        websWrite(wp,"UnKnow Host");
                fclose(fp);
        }
        init_file(DNS_QUERY_RESULT);
}

#ifdef IPv6_SUPPORT
void return_default_gateway(webs_t wp,char *args)
{
	char gateway[MAX_IPV6_IP_LEN] = {};
	char wan_interface[20] = {};
	get_ipv6_wanif(wan_interface);
	read_ipv6defaultgateway(wan_interface, gateway);
	if(strlen(gateway) > 0)
		websWrite(wp, "%s", gateway);
	else
		websWrite(wp, "");
}

void return_get_ipv6_dns(webs_t wp,char *args){
        websWrite(wp, "%s", read_dns(16) );
}
void return_global_ip_w(webs_t wp,char *args)
{
        FILE *fp;
        char buf[1024]={},link_local_ip[128]={};
        char *start,*end;
        init_file(LINK_LOCAL_INFO);
        char ipv6_wan_proto[16];
        strcpy(ipv6_wan_proto, nvram_safe_get("ipv6_wan_proto"));
        if(strncmp(ipv6_wan_proto, "ipv6_6to4",9) == 0){
                 _system("ifconfig tun6to4 > %s", LINK_LOCAL_INFO);
        }else if(strncmp(ipv6_wan_proto, "ipv6_6in4",9) == 0){
                 _system("ifconfig sit1 > %s", LINK_LOCAL_INFO);
        }else{
                 _system("ifconfig %s > %s",nvram_safe_get("wan_eth"),LINK_LOCAL_INFO);
        }

        fp = fopen(LINK_LOCAL_INFO,"r+");
        if(fp)
        {
                while(start = fgets(buf,1024,fp))
                {
                        if(end = strstr(buf,"Scope:Global"))
                        {
                                end = end - 1;
                                if(start = strstr(start,"addr:"))
                                {
                                        start = start + 5;
                                        strncpy(link_local_ip,start,end - start);
                                        websWrite(wp, "%s", link_local_ip);
                                        break;
                                }
                        }
                }
                fclose(fp);
        }

}
void return_global_ip_l(webs_t wp,char *args)
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
                        if(end = strstr(buf,"Scope:Global"))
                        {
                                end = end - 1;
                                if(start = strstr(start,"addr:"))
                                {
                                        start = start + 5;
                                        strncpy(link_local_ip,start,end - start);
                                        websWrite(wp, "%s", link_local_ip);
                                        break;
                                }
                        }
                }
                fclose(fp);
        }

}
void return_link_local_ip_w(webs_t wp,char *args)
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
                                        start = start + 5;
                                        strncpy(link_local_ip,start,end - start);
                                        websWrite(wp, "%s", link_local_ip);
                                        break;
                                }
                        }
                }
                fclose(fp);
        }
}
void return_link_local_ip_l(webs_t wp,char *args)
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
                                        start = start + 5;
                                        strncpy(link_local_ip,start,end - start);
                                        websWrite(wp, "%s", link_local_ip);
                                        break;
                                }
                        }
                }
                fclose(fp);
        }
}
void return_radvd_status(webs_t wp,char *args)
{
        if(checkServiceAlivebyName("radvd"))
                websWrite(wp, "On");
        else
                websWrite(wp, "Off");
}
void return_ipv6_tunnel_lifetime(webs_t wp,char *args)
{
	unsigned int lease=0;
	if(nvram_match("wan_proto","dhcpc") == 0){
		lease = read_dhcpc_lease_time();
		}
		if (lease==0)
			lease = 604800; // default lifetime is 7 days

	websWrite(wp, "%u", lease);
}
#ifdef IPV6_DSLITE
void return_aftr_address(webs_t wp,char *args)
{
 		        FILE *fp; 
 		        char temp[256] = {}; 
 		        fp = fopen("/var/tmp/aftr_address","r"); 
 		        if(fp){ 
 		                if(fgets(temp, 256, fp)){ 
 		                        temp[strlen(temp) - 1] = '\0'; 
			websWrite(wp, "%s", temp);
 		                } 
 		                fclose(fp); 
 		        }else 
		websWrite(wp,"");
} 
#endif

#endif//#ifdef IPv6_SUPPORT

#ifdef XML_AGENT
char login_salt[9];
void return_login_salt_random (webs_t wp,char *args){
        unsigned int randsalt;
        srand(time(0));
        randsalt = random();
        sprintf(login_salt, "%08x", randsalt);
        websWrite(wp, "%s", login_salt);
}
#endif//#ifdef XML_AGENT

/*
*       Date: 2009-7-23
*       Name: Nick Chou
*       Reason:
If user login fail at login.asp,
GUI redirect to login.asp again and show alert messages.
( GUI originally redirect to login_fail.asp or login_auth_fail.asp )
If you want to use this feature,
you also need to modify GUI to show alert messages.
*/
#ifdef GUI_LOGIN_ALERT
extern int gui_loginfail_alert; //apps\httpd\core.c
void return_gui_login_alert (webs_t wp,char *args){
        websWrite(wp, "%d", gui_loginfail_alert);
        gui_loginfail_alert = 0;
}
#endif//#ifdef GUI_LOGIN_ALERT

/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
void return_graph_auth_id(webs_t wp,char *args){
        unsigned long auth_id;
        /* jimmy modified , modify auth codes from 4 to 5 */
        //unsigned short auth_code;
        unsigned long auth_code;
        /* ----------------------------------------------- */
        char AuthID[12];
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv,&tz);
        //cticks = time( (time_t*) 0 ) ;  /* clock ticks since startup */
        cticks = tv.tv_usec;
        //DEBUG_MSG("%s cticks = %lu !!!\n",__FUNCTION__,cticks);
        AuthGraph_GenerateIdCode();
        auth_id   = AuthGraph_GetId();

        /* jimmy modified , modify auth codes from 4 to 5 */
        /* in AuthGraph_ValidateIdCodeByStr(), all use %4X to compare ... */
        //sprintf(AuthID,"%04X",(unsigned int)auth_id);
        /*
        unsigned long 4 bytes:
           1 byte    1 byte    1 byte    1 byte
        |----|----|----|----|----|----|----|----|
           1   c    1    5    5    b    4    2
                we don't need the first 3 bytes
                so we shift the first 3 bytes to
        |----|----|----|----|----|----|----|----|
          5    5    b    4    2    0    0    0
        */
        auth_id = auth_id << 12;
        sprintf(AuthID,"%x%x%x%x%x",(unsigned char )((auth_id >> 28)  & 0x000f)
                ,(unsigned char )((auth_id >> 24)  & 0x000f)
                ,(unsigned char )((auth_id >> 20)  & 0x000f)
                ,(unsigned char )((auth_id >> 16)  & 0x000f)
                ,(unsigned char )((auth_id >> 12)  & 0x000f));
        /* ----------------------------------------------- */
        DEBUG_MSG("%s, AuthID = %s\n",__FUNCTION__,AuthID);
        websWrite(wp,"%s",AuthID);
}
#endif//#ifdef AUTHGRAPH

#if defined(HAVE_HTTPS)
void return_support_https(webs_t wp,char *args)
{
        websWrite(wp, "%d", HAVE_HTTPS);
}
#endif

void return_russia_wan_phy_ipaddr(webs_t wp,char *args){
        websWrite(wp, "%s", read_current_wanphy_ipaddr() );
}

void return_storage_get_number_of_dev(webs_t wp,char *args){
	FILE *fp;
	char space[128];
	int count = 0;

	fp = popen("df |grep /mnt/shared |sed 's/ */ /g' |cut -d ' ' -f 7", "r");			

	if(fp){
		while (fgets(space, sizeof(space), fp))
			count++;
		pclose(fp);	
	}
	
	websWrite(wp, "%d", count);
}

static char *convert_unit(double st_size, int count, char *final_size){
	char *unit[] = {"Bytes", "KB", "MB", "GB", "TB", "PB", "EB"};

	while (st_size > 1024){
		st_size = st_size/1024;
		count++;
	}

	if (count == 0)
		sprintf(final_size, "%.0f %s", st_size, unit[count]);
	else 
		sprintf(final_size, "%.2f %s", st_size, unit[count]);

	return final_size;
}

void return_storage_get_space(webs_t wp,char *args){
	FILE *fp;
	char space[128], *delim = " ";
	char total_sp[32], free_sp[32], *device, *space_tmp;
	char html[1024];

	fp = popen("df |grep /mnt/shared |sed 's/ */ /g' |cut -d ' ' -f 3,5,7", "r");			
	if(fp){
		while (fgets(space, sizeof(space), fp)) {
			DEBUG_MSG("##### SPACE:%s\n", space);
			space_tmp = strtok(space, delim);
			convert_unit(atoi(space_tmp), 1, total_sp);
			space_tmp = strtok(NULL, delim);
			convert_unit(atoi(space_tmp), 1, free_sp);
			device = strtok(NULL, delim);
			device[strlen(device) - 1] = '\0';
	
			sprintf(html, "<tr align=\"center\"><td>%s</td><td>%s</td><td>%s</td></tr>", 
					device + strlen("/mnt/shared/"), total_sp, free_sp);
			DEBUG_MSG("##### SPACE:\n%s\n", html);
	
			websWrite(wp, "%s", html);
		}
		pclose(fp);
	}
}

char *get_wan_ip(char *wan_ip_1)
{
        char *tmp_ip_1 = NULL;

        if ((nvram_match("wan_proto", "dhcpc") != 0) ||
            (nvram_match("wan_proto", "static") != 0)) {
                tmp_ip_1 = get_ipaddr(nvram_safe_get("wan_eth"));
                if (tmp_ip_1)
                        strncpy(wan_ip_1, tmp_ip_1, strlen(tmp_ip_1));
        } else {
                tmp_ip_1 = get_ipaddr("ppp0");
                if(tmp_ip_1)
                        strncpy(wan_ip_1, tmp_ip_1, strlen(tmp_ip_1));
        }
}

void return_storage_wan_ip(webs_t wp,char *args)
{
	char ip[16] = {0};

	get_wan_ip(ip);
	websWrite(wp, "%s", ip);

}

int ej_cmo_get_status(int eid, webs_t wp, int argc, char_t **argv)
{
        char *name=NULL,*value=NULL;
        int i,args = 0;
        status_handler_t *status_handler;

/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetStatus can used the args.
*   Notice :
*/
/*
        if (ejArgs(argc, argv, "%s", &name) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }
*/
        if ((args=ejArgs(argc, argv, "%s %s", &name, &value)) < 1) {
                DEBUG_MSG("%s: Insufficient args\n",__func__);
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }/* CmoGetStatus("name") */
        else if(args==1){
                assert(name);
        }/* CmoGetStatus("name","args") */
        else if(args==2){
                assert(name);
                assert(value);
                DEBUG_MSG("%s: value=%s\n",__func__,value);
        }
        else{
                DEBUG_MSG("%s: Too much args\n",__func__);
                websError(wp, 400, "Too much args\n");
                return -1;
        }
    DEBUG_MSG("%s: name=%s\n",__func__,name);

        status_handler = &status_handlers[0];
        for(i = 0;i < sizeof(status_handlers)/sizeof(status_handler_t);i++)
        {
                if(status_handler->pattern){
                        if (strcmp(status_handler->pattern, name) == 0){
                                status_handler->output(wp,value);
                                break;
                        }
                        if(i != (sizeof(status_handlers)/sizeof(status_handler_t) -1))
                                status_handler = status_handler + 1;
                }
        }
/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Added for GPL CmoGetStatus.
*   Notice :
*/
        check_customer_status_handler(wp,name,value);
        return 0;
}
/* CmoGetStatus */
/**************************************************************/
/**************************************************************/

/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetList can used the args.
*   Notice :
*/
/* CmoGetList */
/**************************************************************/
/**************************************************************/
#ifdef CONFIG_USER_WISH
void return_wish_sessions_table(webs_t wp,char *args)
{
}
#endif
void return_routing_table(webs_t wp,char *args){
        int i = 0;
        routing_table_t *routing_table_list;

        routing_table_list = read_route_table();

/*
        for(i=0; strlen(routing_table_list[i].dest_addr) >0; i++ ){
                websWrite(wp,"%s/%s/%s/%s/%d,",\
                                routing_table_list[i].dest_addr,\
                                routing_table_list[i].dest_mask,\
                                routing_table_list[i].gateway,\
                                routing_table_list[i].interface,\
                                routing_table_list[i].metric);
        }
*/
        /*      Date: 2008-12-22
        *       Name: jimmy huang
        *       Reason: for the feature routing information can show "Type" and "Creator"
        *                       so we add 2 more fileds in routing_table_s structure
        *       Note:
        *         Type:
        *               0:INTERNET:     The table is learned from Internet
        *               1:DHCP Option:  The table is learned from DHCP server in Internet
        *               2:STATIC:               The table is learned from "Static Route" for Internet
        *                                               Port of DIR-Series
        *               3:INTRANET:         The table is used for Intranet
        *               4:LOCAL         :       Local loop back
        *
        *       Creator:
        *               0:System:               Learned automatically
        *               1:ADMIN:                Learned via user's operation in Web GUI
        */
        for(i=0; strlen(routing_table_list[i].dest_addr) >0; i++ ){
                printf("%s/%s/%s/%s/%d/%d/%d," \
                        ,routing_table_list[i].dest_addr \
                        ,routing_table_list[i].dest_mask \
                        ,routing_table_list[i].gateway \
                        ,routing_table_list[i].interface \
                        ,routing_table_list[i].metric \
                        ,routing_table_list[i].creator \
                        ,routing_table_list[i].type \
                        );
                websWrite(wp,"%s/%s/%s/%s/%d/%d/%d," \
                        ,routing_table_list[i].dest_addr \
                        ,routing_table_list[i].dest_mask \
                        ,routing_table_list[i].gateway \
                        ,routing_table_list[i].interface \
                        ,routing_table_list[i].metric \
                        ,routing_table_list[i].creator \
                        ,routing_table_list[i].type \
                        );
        }

}

void return_dhcpd_leased_table(webs_t wp,char *args){
        int i ;
        T_DHCPD_LIST_INFO* dhcpd_status = get_dhcp_client_list();

        if(dhcpd_status == NULL)
                return;

        for(i = 0 ; i < dhcpd_status->num_list; i++)
        {
                websWrite(wp,"%s/%s/%s/%s,",\
                                dhcpd_status->p_st_dhcpd_list[i].hostname,\
                                dhcpd_status->p_st_dhcpd_list[i].ip_addr,\
                                dhcpd_status->p_st_dhcpd_list[i].mac_addr,\
                                dhcpd_status->p_st_dhcpd_list[i].expired_at);
        }
}

/*
*   Date: 2009-08-04
*   Name: Vic chang
*   Reason: Fixed site Survery status error
*   Notice :
*/
void return_wireless_ap_scan_list(webs_t wp,char *args)
{
#ifdef CONFIG_USER_WL_ATH
        FILE *fp;
        char buf[256],tmp[80],scanning_result[64*256],final_result[64*256];
        char *ap_mac,*ap_ssid,*ap_channel,*ap_security,*op_mode,*error_check;

        if(nvram_match("wlan0_radio_mode","0") == 0) //2.4 GHz
                op_mode = nvram_safe_get("wlan0_op_mode");
        else if(nvram_match("wlan0_radio_mode","1") == 0) //5 GHz
                op_mode = nvram_safe_get("wlan1_op_mode");
        if(strcmp(op_mode,"rpt_wds") == 0)
                _system("iwlist ath1 scanning > %s",AP_SCAN_LIST);
        else if(0==strcmp(op_mode,"br_wds") || 0==strcmp(op_mode,"APC"))
                _system("iwlist ath0 scanning > %s",AP_SCAN_LIST);
        sleep(10);
        memset(buf,0,sizeof(buf));
        memset(tmp,0,sizeof(tmp));
        memset(scanning_result,0,sizeof(scanning_result));
        memset(final_result,0,sizeof(final_result));

        #define AUTH_TYPE_NO    0
        #define AUTH_TYPE_WEP   1
        #define AUTH_TYPE_WPA   2
        #define AUTH_TYPE_WPA2  3
        #define AUTH_TYPE_AUTO  4
        #define CIPHER_TYPE_NO  5
    #define CIPHER_TYPE_TIKP  6
    #define CIPHER_TYPE_AES  7
    #define CIPHER_TYPE_AUTO  8

        int first_flag = 0;
        int auth_type = -1;
        int cipher_type = CIPHER_TYPE_NO;

        fp = fopen(AP_SCAN_LIST,"r");
        if(fp)
        {
                while(fgets(buf,256,fp))
                {
                        //DEBUG_MSG("buf: %s\n", buf);
                        memset(tmp,0,sizeof(tmp));

                        if(ap_mac = strstr(buf,"Cell"))
                        {
                                if (first_flag == 0) {
                                        first_flag = 1;
                                }
                                else {

                                        DEBUG_MSG("auth_type: %d\n", auth_type);
                                        DEBUG_MSG("cipher_type: %d\n", cipher_type);
                                        if (auth_type == AUTH_TYPE_NO && cipher_type == CIPHER_TYPE_NO)
                                                strcat(scanning_result, "OFF/NO/NO/");
                                        else if (auth_type == AUTH_TYPE_WEP && cipher_type == CIPHER_TYPE_NO)
                                                strcat(scanning_result, "ON/WEP/NO/");
                                        else if (auth_type == AUTH_TYPE_WPA) {
                                                    if (cipher_type == CIPHER_TYPE_TIKP)
                                                        strcat(scanning_result, "ON/WPA-PSK/TIKP/");
                                                else if (cipher_type == CIPHER_TYPE_AES)
                                                        strcat(scanning_result, "ON/WPA-PSK/AES/");
                                                else
                                                        strcat(scanning_result, "ON/WPA-PSK/AUTO/");
                                        }
                                        else if (auth_type == AUTH_TYPE_WPA2) {
                                                        if(cipher_type == CIPHER_TYPE_TIKP)
                                                                strcat(scanning_result, "ON/WPA2-PSK/TIKP/");
                                                    else if (cipher_type == CIPHER_TYPE_AES)
                                                                strcat(scanning_result, "ON/WPA2-PSK/AES/");
                                                    else
                                                                strcat(scanning_result, "ON/WPA2-PSK/AUTO/");
                                        }
                                        else if (auth_type == AUTH_TYPE_AUTO){
                                                        if( cipher_type == CIPHER_TYPE_TIKP)
                                                             strcat(scanning_result, "ON/AUTO/TIKP/");
                                                        else if (cipher_type == CIPHER_TYPE_AES)
                                                             strcat(scanning_result, "ON/AUTO/AES/");
                                                        else
                                                            strcat(scanning_result, "ON/AUTO/AUTO/");
                                        }
                                        DEBUG_MSG("scanning_result6: %s\n", scanning_result);
                                                strcat(scanning_result, ",");
                                }

                                auth_type = -1;
                                cipher_type = CIPHER_TYPE_NO;

                                if(ap_mac = strstr(ap_mac,"Address"))
                                {
                                        ap_mac = ap_mac + 9;
                                        strcpy(tmp, ap_mac);
                                        //tmp[strlen(tmp) - 3] = '/';
                                        //tmp[strlen(tmp) - 2] = 0;
                                        strcat(tmp, "/");
                                        //DEBUG_MSG("tmp2: %s\n", tmp);
                                        strcat(scanning_result, tmp);
                                //      DEBUG_MSG("scanning_result1: %s\n", scanning_result);
                                }
                        }
                        else if(ap_ssid = strstr(buf,"ESSID"))
                        {
                                ap_ssid = ap_ssid + 7;
                                //strncpy(tmp, ap_ssid, strlen(ap_ssid) - 1);
                                strcpy(tmp,remove_backslash(ap_ssid,strlen(ap_ssid)));
                                tmp[strlen(tmp) - 4] = '/';
                                tmp[strlen(tmp) - 3] = 0;
                                //strcat(scanning_result,remove_backslash(tmp,strlen(tmp)));
                                strcat(scanning_result, tmp);
                                //DEBUG_MSG("scanning_result2: %s\n", scanning_result);
                                //strcat(scanning_result,"/");
                        }
                        else if(ap_channel = strstr(buf,"Channel"))
                        {
                                ap_channel = ap_channel + 8;
                                //strncpy(tmp,ap_channel,strlen(ap_channel) - 1);
                                strcpy(tmp, ap_channel);
                                tmp[strlen(tmp) - 2] = '/';
                                tmp[strlen(tmp) - 1] = 0;
                                //strcat(tmp,"/");
                                strcat(scanning_result,tmp);
                                //DEBUG_MSG("scanning_result3: %s\n", scanning_result);
                        }
                                        else if (strstr(buf,"Encryption key:off"))
                                        {
                                                auth_type = AUTH_TYPE_NO;
                        }
                        else if (strstr(buf,"Encryption key:on"))
                        {
                                                if(auth_type != AUTH_TYPE_NO)
                                auth_type = AUTH_TYPE_WEP;
                        }
                                        else if(strstr(buf,"IE: IEEE 802.11i/WPA2 Version")&&(auth_type != AUTH_TYPE_NO)) {
                                if (auth_type == AUTH_TYPE_WPA)
                                        auth_type = AUTH_TYPE_AUTO;
                                else
                                        auth_type = AUTH_TYPE_WPA2;
                }
                                        else if(strstr(buf,"IE: WPA Version")&&(auth_type != AUTH_TYPE_NO)) {
                                if (auth_type == AUTH_TYPE_WPA2)
                                        auth_type = AUTH_TYPE_AUTO;
                                else
                                        auth_type = AUTH_TYPE_WPA;
                        }
                                        else if(strstr(buf,"Pairwise Ciphers (1) : TKIP")&&(auth_type != AUTH_TYPE_NO))
                                cipher_type = CIPHER_TYPE_TIKP;
                                        else if(strstr(buf,"Pairwise Ciphers (1) : CCMP")&&(auth_type != AUTH_TYPE_NO))
                                cipher_type = CIPHER_TYPE_AES;
                                        else if(strstr(buf,"Pairwise Ciphers (2) : TKIP CCMP")&&(auth_type != AUTH_TYPE_NO))
                                cipher_type = CIPHER_TYPE_AUTO;
                        DEBUG_MSG("scanning_result4: %s\n", scanning_result);

                }

                fclose(fp);



                //iwlist has a bug that can not show info completely.
                error_check = strrchr(scanning_result,',');
                if(strlen(error_check) > 1)
                {
                        strncpy(final_result,scanning_result,error_check - scanning_result);
                        strcat(final_result,",");
                        websWrite(wp,"%s",final_result);
                }
                else
                        websWrite(wp,"%s",scanning_result);
        }
        //_system("rm -f %s",AP_SCAN_LIST);
#endif
}

void return_upnp_portmap_table(webs_t wp,char *args){

        int i = 0;
/*      Date:   2009-04-17
*       Name:   jimmy huang
*       Reason: Add definition for miniupnpd-1.3 to use same features
*       Note:   This feature only works for libupnp or miniupnpd-1.3 (not include miniupnpd-1.0-RC7)
*                       For libupnp:
*                               These codes and definitions are related to
*                                       - libupnp-1.6.3/upnp/sample/linuxigd-1.0/
*                                               main.c
*                                               gatedevice.c
*                                               gatedevice.h
*                                       - httpd
*                                               cmobasicapi.c
*                                               ajax.c
*                       For miniupnpd-1.3:
*                               These codes and definitions are related to
*                                       miniupnpd-1.3/config.h
*/
//#if defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)
#if (defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)) || \
                (defined(UPNP_ATH_MINIUPNPD_1_3) && defined(UPNP_ATH_MINIUPNPD_1_3_LEASE_FILES))
// -------------
#ifdef UPNP_ATH_MINIUPNPD_RC
#error UPNP_ATH_MINIUPNPD_RC and UPNP_ATH_LIBUPNP can not be selected simultaneously
#endif
#if defined(UPNP_ATH_LIBUPNP) && defined(UPNP_ATH_MINIUPNPD_1_3)
#error UPNP_ATH_LIBUPNP and UPNP_ATH_MINIUPNPD_1_3 can not be selected simultaneously
#endif
#define UPNP_FORWARD_RULES_FILE "/tmp/upnp_portmapping_rules"
#define UPNP_RULES_SHOW_UI_MAX 200
#define UPNP_FORWARD_RULES_FILE "/tmp/upnp_portmapping_rules"
#define UPNP_FORWARD_RULES_FILE_TMP "/tmp/upnp_portmapping_rules.tmp"
#define PRO_LEN 5
#define HOST_LEN 16
#define PORT_LEN 6
#define DESC_LEN 64
#define DURATION_LEN 12
#define NAME_SIZE  256
#define FILE_LINE_LEN (PRO_LEN + 2*HOST_LEN + 2*PORT_LEN + DESC_LEN + 2*DURATION_LEN + NAME_SIZE)

        char buf[FILE_LINE_LEN] = {0};
        char *protocol_in = NULL, *desc_in = NULL;
        char *remoteHost_in = NULL, *externalPort_in = NULL;
        char *internalPort_in = NULL, *internalClient_in = NULL;
        char *duration_in = NULL, *expireTime_in = NULL;
        char *devudn_in = NULL;
        char *p_head = NULL, *p_tail = NULL;
        long int shown_time = 0;
/*
Formats for
miniupnpd-1.3:          (upnpredirect.c, lease_file_add())
description / remote ip / ex-port / proto / int-ip / int-port / leasetime / expire time / "Reserved"

libupnp+linux-igd:
description / proto / remote ip  / ex-port/ int-ip     / int-port / leasetime / expire time / udev
test1      / TCP   / 61.61.1.61 / 3000   / 10.10.0.10  /  2000   /  3600    / 123456789 / uuid:33333333
*/
        FILE *fp;
        if((fp = fopen(UPNP_FORWARD_RULES_FILE,"r"))!=NULL){
                while(!feof(fp)){
                        fgets(buf,sizeof(buf),fp);
                        if(!feof(fp) && (buf[0] != '\n')){
                                p_head = buf;
                            p_tail = strchr(buf,'/');

                                if(p_head == p_tail){
                                        p_tail = p_head;
                                        desc_in = NULL;
                                }else{
                                        p_tail[0]='\0';
                                        desc_in = p_head;
                                }
#ifdef UPNP_ATH_LIBUPNP
                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        protocol_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0]='\0';
                                        protocol_in = p_head;
                                }
#endif
                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        remoteHost_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0]='\0';
                                        remoteHost_in= p_head;
                                }

                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        externalPort_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0] = '\0';
                                        externalPort_in = p_head;
                                }
#ifdef UPNP_ATH_MINIUPNPD_1_3
                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        protocol_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0]='\0';
                                        protocol_in = p_head;
                                }
#endif
                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        internalClient_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0] = '\0';
                                        internalClient_in = p_head;
                                }

                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        internalPort_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0] = '\0';
                                        internalPort_in = p_head;
                                }

                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        duration_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0] = '\0';
                                        duration_in = p_head;
                                }

                        /*      Date:   2009-04-22
                                *       Name:   jimmy huang
                                *       Reason: If the rules is dynamic rule (lease duration time != 0)
                                *                       Time shown on Web GUI should be remain time,
                                *                       not the Duration Time
                                *       Note:   Refer to UPnP_IGD_WANIPConnection 1.0.pdf
                                *                       page 14, section 2.2.21
                                */
                                p_head = p_tail + 1;
                                if(p_head[0] == '/'){
                                        p_tail = p_head;
                                        expireTime_in = NULL;
                                }else{
                                        p_tail = strchr(p_head,'/');
                                        p_tail[0] = '\0';
                                        expireTime_in = p_head;
                                }

                                if((duration_in != NULL) && (atoi(duration_in) > 0)){
                                        // this is a dynamic rule (lease duration time > 0)
                                        if(expireTime_in && (atoi(expireTime_in) > 0)){
                                                // remain time > 0 (rule still valid)
                                                shown_time = atol(expireTime_in) - time(NULL);
                                                if(shown_time < 0)
                                                        shown_time = 0;
                                        }else{
                                                // remain time <= 0 (rule is expired)
                                                shown_time = 0;
                                        }
                                }else{
                                        // this is static rule (lease duration time = 0)
                                        shown_time = atol(duration_in);
                                }

/*
These codes can not handle the situation
/TCP/172.21.35.200/3000/192.168.0.101/2000/3600/123456789/uuid:33333333
Formal format:
test1/TCP/172.21.35.200/3000/192.168.0.101/2000/3600/123456789/uuid:33333333
                                getStrtok(buf,"/","%s %s %s %s %s %s %s %s %s"
                                                        ,&desc_in,&protocol_in
                                                        ,&remoteHost_in,&externalPort_in
                                                        ,&internalClient_in,&internalPort_in
                                                        ,&duration_in,&expireTime_in,&devudn_in);
*/
                                        /*      Date:   2009-04-22
                                        *       Name:   jimmy huang
                                        *       Reason: If the rules is a dynamic rule (lease duration time != 0)
                                        *                       Time shown on Web GUI should be remain time,
                                        *                       not the Duration Time
                                        *       Note:   Refer to UPnP_IGD_WANIPConnection 1.0.pdf
                                        *                       page 14, section 2.2.21
                                        */
                                /*
                                websWrite(wp,"%s/%s/%s/%s/%s/%s/%s/%s/%s,"
                                                        ,desc_in ? desc_in : ""
                                                        ,protocol_in
                                                        ,remoteHost_in ? remoteHost_in : ""
                                                        ,externalPort_in
                                                        ,internalClient_in
                                                        ,internalPort_in ? internalPort_in : ""
                                                        ,duration_in ? duration_in : ""
                                                );
                                */
                                websWrite(wp,"%s/%s/%s/%s/%s/%s/%s/%s/%ld,"
                                                        ,desc_in ? desc_in : ""
                                                        ,protocol_in
                                                        ,remoteHost_in ? remoteHost_in : ""
                                                        ,externalPort_in
                                                        ,internalClient_in
                                                        ,internalPort_in ? internalPort_in : ""
                                                        ,shown_time
                                                );
                        }
                }
                //desc, protocol, remoteHost, externalPort, internalClient, internalPort,duration, desc,devudn
                //we need to show
                fclose(fp);
        }else{
                websWrite(wp,"No rules");
        }


#else
        char vs_rule_name[64];
        char vs_rule_value[256];

        for(i = 0; i < 20; i++){
                memset(vs_rule_name, 0, 64);
                memset(vs_rule_value, 0, 256);
                sprintf(vs_rule_name, "vs_rule_%02d", i);
                strcpy(vs_rule_value, nvram_get(vs_rule_name));
                if(strcmp(vs_rule_value, "") != 0)
                        strcat(vs_rule_value, ",");
                websWrite(wp,"%s", vs_rule_value);
        }
#endif
/* --------------------------------- */

}

void return_vlan_table(webs_t wp,char *args)
{
        int i ;
        char enable[20];
        char ssid[20];
        char security[20];
        char vlanid[20];
        char index[20];


        for(i = 1 ; i < 4; i++){
                memset(enable, 0, 20);
                memset(ssid, 0, 20);
                memset(security, 0, 20);
                memset(vlanid, 0, 20);
                memset(index, 0, 20);
                sprintf(enable,"wlan0_vap%d_enable", i);
                sprintf(ssid,"wlan0_vap%d_ssid", i);
                sprintf(security,"wlan0_vap%d_security", i);
                sprintf(vlanid,"wlan0_vap%d_vlanid", i);
                sprintf(index, "%d",i);
                websWrite(wp,"%s/%s/%s/%s/%s,",\
                                nvram_get(enable),\
                                index,\
                                nvram_get(ssid),\
                                nvram_get(security),\
                                nvram_get(vlanid));
        }

}

void return_napt_session_table(webs_t wp,char *args){
        T_NAPT_LIST_INFO* napt_status = get_napt_session(1,"ESTABLISHED");
        napt_session_info_t* current_node = NULL;

        if(napt_status == NULL)
                return;

        current_node = napt_status->p_st_napt_list[0].next;
        while( current_node != NULL )
        {
                if(!strcmp(current_node->direction,"OUT"))
                {
                        websWrite(wp,"%s/%s/%s/%s/%s,",\
                                current_node->local_ip,\
                                current_node->protocol,\
                                current_node->local_port,\
                                current_node->remote_ip,\
                                current_node->remote_port);
                }
                else
                {
                        websWrite(wp,"%s/%s/%s/%s/%s,",\
                                current_node->remote_ip,\
                                current_node->protocol,\
                                current_node->remote_port,\
                                current_node->local_ip,\
                                current_node->local_port);
                }
                current_node = current_node->next;
        }

        free_session_node();
}

void return_internet_session_table(webs_t wp,char *args){
        T_NAPT_LIST_INFO* napt_status = get_napt_session(2,"ALL");
        napt_session_info_t* current_node = NULL;

        if(napt_status == NULL)
                return;

        current_node = napt_status->p_st_napt_list[0].next;
        while( current_node != NULL )
        {
                websWrite(wp,"%s/%s/%s/%s/%s/%s/%s/%s/%s,",\
                        current_node->protocol,\
                        current_node->timeout,\
                        current_node->state,\
                        current_node->direction,\
                        current_node->local_ip,\
                        current_node->local_port,\
                        current_node->remote_ip,\
                        current_node->remote_port,\
                        current_node->public_port);

                current_node = current_node->next;
        }

        free_session_node();
}

void return_igmp_table(webs_t wp,char *args){
        int i ;
        T_IGMP_LIST_INFO* igmp_status = get_igmp_list();

        if(igmp_status == NULL)
                return;

        for(i = 0 ; i < igmp_status->num_list; i++)
        {
                websWrite(wp,"%s,",igmp_status->p_igmp_list[i].member);
        }
}

#ifdef CONFIG_USER_IP_LOOKUP
void lookup_name_at_dhcp_table(char *host_data)
{
	char value[128];
	char *ip_addr, *hostname, *mac_addr;
	T_DHCPD_LIST_INFO* dhcpd_status = get_dhcp_client_list();
	int i;

    strcpy(value, host_data);
    ip_addr = strtok(value, "/");
    hostname = strtok(NULL, "/");
    mac_addr = strtok(NULL, "/");

    if (mac_addr == NULL) {
    	mac_addr = hostname;
    	hostname = NULL;
    }
	
    DEBUG_MSG("ip_addr = %s, hostname = %s, mac_addr = %s\n", ip_addr, hostname, mac_addr);
    if ((hostname == NULL) && (dhcpd_status != NULL)) {
		for (i = 0 ;i < dhcpd_status->num_list; i++) {
			if (strcmp(dhcpd_status->p_st_dhcpd_list[i].ip_addr, ip_addr) == 0) {
				sprintf(host_data, "%s/%s/%s", ip_addr, dhcpd_status->p_st_dhcpd_list[i].hostname, mac_addr);
				DEBUG_MSG("host_data = %s\n", host_data);
			}
		}
    }
}

/*
* 	Date: 2009-6-17
* 	Name: Ken_Chiang
* 	Reason: modify to fixed sometime heartbeat can't get DHCP client list and DHCP client list can¡¦t display issue.
* 	Notice :
*/
void return_client_list_table(webs_t wp, char *args){
	int i=0;
	FILE *fp,*fp1;
	char temp[128]={0};

	fp = fopen(CLIENT_LIST_FILE, "r");
	if (fp == NULL) {
		printf("list file does not exist\n");
		websWrite(wp, "");
	}
	else {
		while (fgets(temp, sizeof(temp), fp)) {
			i++;
			lookup_name_at_dhcp_table(temp);
			websWrite(wp, "%s,", temp);
			memset(temp, 0, sizeof(temp));
		}
		fclose(fp);

		if (!i) {
			fp1 = fopen(CLIENT_LIST_TMP_FILE, "r");
			if (fp1 == NULL) {
				printf("list tmp file does not exist\n");
				websWrite(wp, "");
			}
			else {
				while (fgets(temp, sizeof(temp), fp1)) {
					lookup_name_at_dhcp_table(temp);
					websWrite(wp, "%s,", temp);
					memset(temp, 0, sizeof(temp));
				}
				fclose(fp);
			}
		}
	}
}
#endif//#ifdef CONFIG_USER_IP_LOOKUP

#ifdef IPv6_SUPPORT
#define MAC                     1
#define HOSTNAME                2
#define IPV6_ADDRESS            3
#define IPV6_BUFFER_SIZE        4096
int total_lines = 0;

char *value(FILE *fp,int line,int token)
{
        int i;
        static char temp[IPV6_BUFFER_SIZE],buffer[IPV6_BUFFER_SIZE];
        fseek(fp,0,SEEK_SET);
        for(i = 0;i < line;i++)
        {
                memset(temp,0,sizeof(temp));
                fgets(temp,sizeof(temp),fp);
        }
        memset(buffer,0,sizeof(buffer));
        switch(token)
        {
                case MAC:
                        strcpy(buffer,strtok(temp,"/"));
                        break;
                case HOSTNAME:
                        strtok(temp,"/");
                        strcpy(buffer,strtok(NULL,"/"));
                        break;
                case IPV6_ADDRESS:
                        strtok(temp,"/");
                        strtok(NULL,"/");
                        strcpy(buffer,strtok(NULL,"/"));
                        buffer[strlen(buffer) -1] = '\0';
                        break;
                default:
                        printf("error option\n");
                        strcpy(buffer,"ERROR");
                        break;
        }
        return buffer;
}
int check_mac_previous(char *mac)
{
        FILE *fp;
        char temp[IPV6_BUFFER_SIZE];
        memset(temp,0,sizeof(temp));
        fp = fopen(IPV6_CLIENT_RESULT,"r");
        if(fp)
        {
                while(fgets(temp,IPV6_BUFFER_SIZE,fp))
                {
                        if(strstr(temp,mac))
                        {
                                fclose(fp);
                                return 1;
                        }
                }
                fclose(fp);
        }
        return 0;
}

/*
int compare_back(FILE *fp,int current_line,char *buffer)
{
        int i=0;
        char mac[32] = {},compare_mac[32] = {};

        buffer[strlen(buffer) -1] = '\0';
        strcpy(mac,value(fp,current_line,MAC));
        if(check_mac_previous(mac))
                return 0;
        for(i=0;i<(total_lines - current_line);i++)
        {
                strcpy(compare_mac,value(fp,current_line+1+i,MAC));
                if(strcmp(mac,compare_mac) == 0)
                {
                        strcat(buffer,",");
                        strcat(buffer,value(fp,current_line+1+i,IPV6_ADDRESS));
                }
        }
        save2file(IPV6_CLIENT_RESULT,"%s\n",buffer);
        return 0;
}
/*
/*
void get_ipv6_client_list(void)
{
        FILE *fp;
        int i = 0,line_index = 1;
        char temp[IPV6_BUFFER_SIZE],*buffer[IPV6_BUFFER_SIZE];
        static char *compare_mac;
        memset(temp,0,sizeof(temp));
        fp = fopen(IPV6_CLIENT_INFO,"r");
        init_file(IPV6_CLIENT_RESULT);
        if(fp)
        {
                while(fgets(temp,IPV6_BUFFER_SIZE,fp))
                        total_lines++;
                fseek(fp,0,SEEK_SET);
                memset(temp,0,sizeof(temp));
                memset(buffer,0,sizeof(buffer));

                while(fgets(temp,IPV6_BUFFER_SIZE,fp))
                {
                        compare_back(fp,line_index,temp);
                        value(fp,line_index,MAC);
                        line_index++;
                }
                fclose(fp);
        }
        line_index = 1;
}
*/

int ipv6_client_numbers(void)
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

void return_ipv6_routing_table(webs_t wp,char *value)
{
	FILE *fp;
	char temp[256] = {};
	char dest[64] = {};
	char gw[64] = {};
	char metric[64] = {};
	char iface[64] = {};
	char cmd[128] = {};
	sprintf(cmd, "route -A inet6 | sed 's/  */%c//g' | cut -d '/' -f 1,2,3,5,8 | \
			grep -v '^fe80::/64' | grep -v '^ff0.::' | \
			grep -v 'lo$'| grep -v '^[KD]'",0x5c);
	fp = popen(cmd,"r");
	if(fp){
		while(fgets(temp, 256, fp)){
			temp[strlen(temp)-1] = ',';
			websWrite(wp, "%s", temp);
			memset(temp,0,sizeof(temp));
		}
		pclose(fp);
	}else
		websWrite(wp, "%s","");
}

void return_ipv6_client_list_table(webs_t wp,char *value)
{
        int i = 0;
        FILE *fp;
        char temp[IPV6_BUFFER_SIZE] = {};
        total_lines = 0;

    //get_ipv6_client_list();
    system("cli ipv6 clientlist"); 
        usleep(500);

        fp = fopen(IPV6_CLIENT_RESULT,"r");
        if(fp)
        {
                for(i=0;i<ipv6_client_numbers();i++)
                {
                        fgets(temp,IPV6_BUFFER_SIZE,fp);
                        temp[strlen(temp) - 1] = '\0';
                        websWrite(wp, "%s@",temp);
                        memset(temp,0,sizeof(temp));
                }
                fclose(fp);
        }
}
#endif//#ifdef IPv6_SUPPORT

int ej_cmo_get_list(int eid, webs_t wp, int argc, char_t **argv)
{
        char *name=NULL,*value=NULL;
        int i,args = 0;
        list_handler_t *list_handler;

/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Modify for CmoGetList can used the args.
*   Notice :
*/
/*
        if (ejArgs(argc, argv, "%s", &name) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }
*/
        if ((args=ejArgs(argc, argv, "%s %s", &name, &value)) < 1) {
                DEBUG_MSG("%s: Insufficient args\n",__func__);
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }/* CmoGetList("name") */
        else if(args==1){
                assert(name);
        }/* CmoGetList("name","args") */
        else if(args==2){
                assert(name);
                assert(value);
                DEBUG_MSG("%s: value=%s\n",__func__,value);
        }
        else{
                DEBUG_MSG("%s: Too much args\n",__func__);
                websError(wp, 400, "Too much args\n");
                return -1;
        }
    DEBUG_MSG("%s: name=%s\n",__func__,name);

        list_handler = &list_handlers[0];
        for(i = 0;i < sizeof(list_handlers)/sizeof(list_handler_t);i++)
        {
                if(list_handler->pattern){
                        if (strncmp(list_handler->pattern, name,strlen(list_handler->pattern)) == 0)
                                list_handler->output(wp,value);
                        if(i != (sizeof(list_handlers)/sizeof(list_handler_t) -1))
                                list_handler = list_handler + 1;
                }
        }
/*
*       Date: 2009-08-05
*   Name: Ken Chiang
*   Reason: Added for GPL CmoGetList.
*   Notice :
*/
        check_customer_list_handler(wp,name,value);
        return 0;
}
/* CmoGetList */
/**************************************************************/
/**************************************************************/

#ifdef CONFIG_USER_3G_USB_CLIENT
void return_usb3g_connection_status(webs_t wp){
        if(strcmp(wan_proto, "usb3g") == 0)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                strcpy(wan_status, wan_statue("usb3g"));
                websWrite(wp, "%s", wan_status);
        }
        else
                websWrite(wp, "%s", "Disconnected");
}
#endif//CONFIG_USER_3G_USB_CLIENT

/* CmoGetLog */
/**************************************************************/
/**************************************************************/
/* jimmy modified for IPC syslog */
#define NEW_LOG
#ifdef NEW_LOG
int ej_cmo_get_log(int eid, webs_t wp, int argc, char_t **argv)
{
        int i, j, log_system_activity, log_debug_information, log_attacks, log_dropped_packets, log_notice;
        int per_page, quotient, flag_quotient_equal_0;
        flag_quotient_equal_0 = 0;

        log_system_activity = atoi(nvram_get("log_system_activity"));
        log_debug_information = atoi(nvram_get("log_debug_information"));
        log_attacks = atoi(nvram_get("log_attacks"));
        log_dropped_packets = atoi(nvram_get("log_dropped_packets"));
        log_notice = atoi(nvram_get("log_notice"));
        logs_info.total_logs = update_log_table(log_system_activity, log_debug_information, log_attacks, log_dropped_packets, log_notice);

        per_page = atoi(nvram_get("log_per_page"));
        if(per_page <= 0)
                per_page = 10;

        quotient = logs_info.total_logs / per_page;
        logs_info.remainder = logs_info.total_logs % per_page;

        if (quotient == 0)
                flag_quotient_equal_0 = 1;

        if(logs_info.remainder != 0)
                quotient += 1;

        if(quotient != 0)
                logs_info.total_page = quotient;
        else
                logs_info.total_page = 1;

        if(logs_info.cur_page == 0)
                logs_info.cur_page = 1;

        if(logs_info.cur_page > logs_info.total_page)
                logs_info.cur_page = 1;

        if(logs_info.cur_page == 1){    //current page is the first page
                for(i = 0 ; i < per_page && i < logs_info.total_logs ; i++){
                        websWrite(wp,"|syslog|%s|%s|%s ",
                                                log_dynamic_table[i].time,
                                                log_dynamic_table[i].type,
                                                log_dynamic_table[i].message);
                }
        }else if(logs_info.cur_page == -1){     //clear all log messages

                logs_info.cur_page = 1;
                logs_info.total_page = 1;
        /* Note:
                the code below is copy from rc/app.c, start_syslogd()
                The codes below and in start_syslogd() must be identically !!
        */
                char *nvram_syslog_server = NULL, *syslog_enable = NULL, *syslog_ip = NULL;
                char tmp_syslog_server[20]={0};
                nvram_syslog_server = nvram_safe_get("syslog_server");
                strcpy(tmp_syslog_server, nvram_syslog_server);
                _system("killall syslogd klogd");
                /* jimmy added , remove the file generated by syslogd */
                unlink("/var/log/message_die_bak");
                /* --------------------------------------- */

                if( strlen(nvram_syslog_server) ){
                    syslog_enable = strtok(tmp_syslog_server, "/");
                    syslog_ip = strtok(NULL, "/");

                    if(syslog_enable == NULL || syslog_ip == NULL){
                        printf("start_syslogd: syslog_server error\n");
                                _system("syslogd -O %s -b 0 &", LOG_FILE_BAK);
                    }else{
                            if( strcmp(syslog_enable, "1") == 0 )
                                        _system("syslogd -O %s -L -R %s:514 &", LOG_FILE_BAK, syslog_ip);
                            else
                                        _system("syslogd -O %s &", LOG_FILE_BAK);
                        }
                }else{
                        _system("syslogd -O %s -b 0 &", LOG_FILE_BAK);
                }
                _system("klogd &");
                syslog(LOG_INFO, "Log cleared by Administrator");
                printf("log clear page\n");
        }else{
                j = 0;
                for(i = (logs_info.cur_page - 1) * per_page ; (j < per_page) && (i < logs_info.total_logs) ; i++ , j++){
                        websWrite(wp,"|syslog|%s|%s|%s ",
                                        log_dynamic_table[i].time,
                                        log_dynamic_table[i].type,
                                        log_dynamic_table[i].message);
                }
        }

        return 0;
}
#else
/* older log version */
int ej_cmo_get_log(int eid, webs_t wp, int argc, char_t **argv)
{
        int i, j, log_system_activity, log_debug_information, log_attacks, log_dropped_packets, log_notice;
        int per_page, quotient, flag_quotient_equal_0;
        flag_quotient_equal_0 = 0;

        log_system_activity = atoi(nvram_get("log_system_activity"));
        log_debug_information = atoi(nvram_get("log_debug_information"));
        log_attacks = atoi(nvram_get("log_attacks"));
        log_dropped_packets = atoi(nvram_get("log_dropped_packets"));
        log_notice = atoi(nvram_get("log_notice"));
        logs_info.total_logs = update_log_table(log_system_activity, log_debug_information, log_attacks, log_dropped_packets, log_notice);

        per_page = atoi(nvram_get("log_per_page"));
        if(per_page <= 0)
                per_page = 10;

        quotient = logs_info.total_logs / per_page;
        logs_info.remainder = logs_info.total_logs % per_page;

        if (quotient == 0)
                flag_quotient_equal_0 = 1;

        if(logs_info.remainder != 0)
                quotient += 1;

        if(quotient != 0)
                logs_info.total_page = quotient;
        else
                logs_info.total_page = 1;

        if(logs_info.cur_page == 0)
                logs_info.cur_page = 1;

        if(logs_info.cur_page > logs_info.total_page)
                logs_info.cur_page = 1;

        if(logs_info.cur_page == 1){    //current page is first page
                if(flag_quotient_equal_0 == 1){
                        for(i = logs_info.total_logs - 1; i >= 0; i--){
                                websWrite(wp,"|syslog|%s|%s|%s ",\
                                                log_dynamic_table[i].time,\
                                                log_dynamic_table[i].type,\
                                                log_dynamic_table[i].message);
                        }
                }
                else{
                        if(logs_info.remainder == 0){
                                for(i = (per_page - 1); i >= 0 ; i--){
                                        websWrite(wp,"|syslog|%s|%s|%s ",\
                                                        log_dynamic_table[i].time,\
                                                        log_dynamic_table[i].type,\
                                                        log_dynamic_table[i].message);
                                }
                        }else{
                                for(i = logs_info.total_logs - 1; i > (logs_info.total_logs - (per_page + 1)); i--){
                                        websWrite(wp,"|syslog|%s|%s|%s ",\
                                                        log_dynamic_table[i].time,\
                                                        log_dynamic_table[i].type,\
                                                        log_dynamic_table[i].message);
                                }
                        }
                }
        }else if(logs_info.cur_page == logs_info.total_page){   //current page is last page
                if(logs_info.remainder != 0){
                        for(i = (logs_info.remainder - 1); i >= 0 ; i--){
                                websWrite(wp,"|syslog|%s|%s|%s ",\
                                                log_dynamic_table[i].time,\
                                                log_dynamic_table[i].type,\
                                                log_dynamic_table[i].message);
                        }
                }else{
                        for(i = (per_page - 1); i >= 0 ; i--){
                                websWrite(wp,"|syslog|%s|%s|%s ",\
                                                log_dynamic_table[i].time,\
                                                log_dynamic_table[i].type,\
                                                log_dynamic_table[i].message);
                        }
                }
        }else if(logs_info.cur_page == -1){     //clear all log messages
                logs_info.cur_page = 1;
                logs_info.total_page = 1;
                init_file(LOG_FILE);
                syslog(LOG_INFO, "Log cleared by Administrator");
                printf("log clear page\n");
        }else{
                j = (logs_info.total_logs - ((logs_info.cur_page - 1) * per_page) - 1);
                for(i = per_page; i > 0; i--){
                        websWrite(wp,"|syslog|%s|%s|%s ",\
                                        log_dynamic_table[j].time,\
                                        log_dynamic_table[j].type,\
                                        log_dynamic_table[j].message);
                        j -= 1;
                        if(j < 0)
                                break;
                }
        }

        return 0;
}
#endif
/* --------------------------------------------------------------------------------- */


static void set_ASCII_flag()
{
        g_ASCII_flag = 1 ;
}

#ifdef USER_WL_ATH_5GHZ
static void set_a_ASCII_flag()
{
        a_ASCII_flag = 1 ;
}
#endif

static void set_wep64_flag()
{
        g_wep64_flag = 1 ;
}
static void set_wep128_flag()
{
        g_wep128_flag = 1 ;
}

static void set_wep152_flag()
{
        g_wep152_flag = 1 ;
}

static void clean_all_wep_flag()
{
        g_ASCII_flag = 0 ;
#ifdef USER_WL_ATH_5GHZ
        a_ASCII_flag = 0 ;
#endif
        //g_wep64_flag = 0 ;
        //g_wep128_flag = 0 ;
        //g_wep152_flag = 0 ;
}

/* replace keyword in string */
char *substitute_keyword(char *string,char *source_key,char *dest_key){
        char *find = NULL;
        char head[MAX_INPUT_LEN] = {0};
        char result[MAX_INPUT_LEN] = {0};
        int flag=0;

        while(1){
          if((find = strstr(string,source_key)) != NULL){
                /* substitute source_key for dest_key */

                if(flag==0)
                snprintf(head,find-string+1,"%s%s",head,string);
                else
                        snprintf(head,find-string+1+strlen(result),"%s%s",result,string);

                if(flag==0)
                sprintf(result,"%s%s%s",result,head,dest_key);
                else
                        sprintf(result,"%s%s",head,dest_key);

                string = find + strlen(source_key);
                flag++;
                continue;
          }else{
                /* remainder part */
                sprintf(result,"%s%s",result,string);
                break;
                }
        }
        string = &result[0];
        return string;
}
/* check if (dhcpd_domain_name || hostname || wlan0_ssid) is with quot
   if yes, subsitute that with &quot; , or it will cause UI page error
*/
static char *checkQuot(char *name_t,char *value_t){
        char *tmp = NULL;
        static char value_tmp[100] = {};
        char value_head[100] = {};
        char value_body[100] = {};

        if(name_t == NULL)
                return;
        if(     strcmp(name_t,"dhcpd_domain_name") == 0 ||
                strcmp(name_t,"hostname") == 0          ||
                strcmp(name_t,"wlan0_ssid") == 0        ||
                strcmp(name_t,"wlan1_ssid") == 0        ||
                strcmp(name_t,"wlan0_vap1_ssid") == 0   ||
                strcmp(name_t,"wlan1_vap1_ssid") == 0   ||
                strcmp(name_t,"wlan0_vap2_ssid") == 0   ||
                strcmp(name_t,"wlan0_vap3_ssid") == 0   ||
                strcmp(name_t,"wlan0_psk_pass_phrase") == 0     ||
                strcmp(name_t,"wlan1_psk_pass_phrase") == 0     ||
                strcmp(name_t,"asp_temp_34") == 0               ||   /*ssid in wizard*/
                strcmp(name_t,"wan_pppoe_service_00") == 0
          )
        {
                if(value_t == NULL)
                        return;
                strcpy(value_tmp, value_t);
                while(1)
                {
                        tmp = strstr(value_tmp,"\"");
                if(tmp == NULL)
                        break;
                memset(value_head,0,100);
                memset(value_body,0,100);
                        memcpy(value_head, value_tmp, tmp - value_tmp);
                strcat(value_body,tmp+1);
                sprintf(value_tmp,"%s%s%s",value_head,"&quot;",value_body);
          }
                return value_tmp;
        }
        return value_t;
}

/* execute udhcp client (obtain && lease) time */
unsigned long lease_time_execute(int type){
        char string[10] = {};
        unsigned long lease = 0;
        unsigned long reset = 0;
        unsigned long obtain = 0;
        unsigned long now = 0;
        FILE *fp = NULL;

        fp = fopen("/var/tmp/dhcpc.tmp","r");
        if(fp == NULL)
                return 0;

        memset(string,0,sizeof(string));
        fgets(string,sizeof(string),fp);
        lease = atol(string);

        memset(string,0,sizeof(string));
        fgets(string,sizeof(string),fp);
        reset = atol(string);

        now = uptime();
        obtain = (now - reset ) % lease;

        fclose(fp);

        switch(type){
                case 0:
                        return obtain;
                case 1:
                        return lease - obtain;
                case 2:
                        return lease;
                default:
                        return 0;
        }
}

/* remove old dhcpd reservation list */
void reset_dhcpd_resevation_rule(void){
        int i = 0;
        char res_rule[] = "dhcpd_reserve_XX";

        /*if DUT GUI don't have item to enable it, dhcpd reservation function can't work */
        //nvram_set("dhcpd_reservation","0");

        for(i=0; i<MAX_DHCPD_RESERVATION_NUMBER; i++){
                snprintf(res_rule, sizeof(res_rule), "dhcpd_reserve_%02d",i);
                nvram_set(res_rule,"");
        }
        nvram_commit();
}


/* copy nvram value for discarding (discard_nvram_config) */
void set_nvram_config(void){
        char tmp[100] = {};
        FILE *fp = NULL;
        mapping_t *m = NULL;

        /* create a file to save default value if flag = 0 */
        /* flag will be changed when commit and discard */
        if(apply_config_flag == 0)
                fp = fopen(NVRAM_CONFIG_PATH,"w+");
        if(fp == NULL || apply_config_flag)
                return;
        /* set the nvram value to html query format */
        /* a=1&b=2&c=3&......*/
        for (m = ui_to_nvram; m < &ui_to_nvram[ARRAYSIZE(ui_to_nvram)]; m++){
                if(strncmp(m->nvram_mapping_name,"asp_temp_",9) == 0)
                        continue;
                memset(tmp,0,sizeof(tmp));
                sprintf(tmp,"%s%s%s%s",m->nvram_mapping_name,"=",nvram_safe_get(m->nvram_mapping_name),"&");
                fseek(fp,0,SEEK_END);
                fputs(tmp,fp);
        }
        fclose(fp);
        /* set flag for just doing once */
        apply_config_flag = 1;
}

/* initial apply state */
void commit_nvram_config(void){
        /* flag for press save & act */
        apply_config_flag = 0;
        /* compare old lan ip with new lan ip after rc restart */
        if( strcmp(get_old_value_by_file(NVRAM_CONFIG_PATH,"lan_ipaddr"),nvram_safe_get("lan_ipaddr")) != 0){
                  /* remove old dhcp client list */
                        unlink("/var/misc/udhcpd.leases");
                        unlink("/var/misc/html.leases");
                        /* remove old dhcpd reservation list */
                        reset_dhcpd_resevation_rule();
        }
        /* remove nvram.tmp file (that is for nvram discard)*/
        unlink(NVRAM_CONFIG_PATH);
}

void discard_nvram_config(void){
        FILE *fp = fopen(NVRAM_CONFIG_PATH,"r");
        mapping_t *m = NULL;
        char tmp[NVRAM_CONFIG_SIZE] = {};
        char *value_n = NULL;   // from nvram
        char *value_f = NULL;     // from file

        if(fp == NULL)
                return;
        if(fgets(tmp,sizeof(tmp),fp) != NULL){
        /* send fake query(html format) to initial request */
                init_cgi(tmp);
                fclose(fp);
        }
        /* compare to default value and courrent nvram value*/
        for (m = ui_to_nvram; m < &ui_to_nvram[ARRAYSIZE(ui_to_nvram)]; m++){
                if(strncmp(m->nvram_mapping_name,"asp_temp_",9) == 0)
                        continue;
                value_n = nvram_safe_get(m->nvram_mapping_name);
                value_f = get_cgi(m->nvram_mapping_name);
                if(strcmp(value_n,value_f) != 0)
                        nvram_set(m->nvram_mapping_name,value_f);
        }

        nvram_commit();
        /* initial apply state */
        commit_nvram_config();
}

/* get the old lan_ip before saving && acting (rc restart)*/
char *get_old_value_by_file(char *path,char *name){
        FILE *fp = fopen(path,"r");
        char line[MAX_INPUT_LEN] = {};
        static char old_value[MAX_INPUT_LEN] = {};
        char *start = NULL;
        char *end = NULL;

  /* in the case, user just do rc restart */
        if(fp == NULL)
                return nvram_safe_get(name);

        /* search string lan_ipaddr=XXX */
        if( fgets(line,sizeof(line),fp) != NULL ){
                start = strstr(line,name);
                if( start != NULL ){
                        start = start + strlen(name)+1;       // 1 for length of "="
                        end = strstr(start,"&");
                        memset(old_value,0,sizeof(old_value));
                        memcpy(old_value,start,end - start);
                }
        }
        fclose(fp);
        return old_value;
}

/* get apply_config_flag for recognizing if rc restart (save & act button)*/
int get_apply_config_flag(void){
        return apply_config_flag;
}

#ifdef CONFIG_LP
void return_lp_version(webs_t wp){
        FILE *fp;
        char lp_ver[5]={"NA"};
        char lp_reg[3]={'\0'};
        if((fp = fopen(LP_REG_PATH,"r"))){
                fread( lp_reg, 2, 1, fp);
                fclose(fp);
        }
        if((fp = fopen(LP_VER_PATH,"r"))){
                fread( lp_ver, 4, 1, fp);
                fclose(fp);
                if(strlen(lp_ver)==0)
                        websWrite(wp, "%s", lp_reg);
                else if(strlen(lp_reg)==0)
                        websWrite(wp, "%s", lp_ver);
                else
                        websWrite(wp, "%s%s", lp_ver, lp_reg);
        }
        else
                websWrite(wp, "%s", lp_reg);
}
void return_lp_date(webs_t wp){
        FILE *fp;
        char lp_date[20]={'\0'};
        if((fp = fopen(LP_DATE_PATH,"r"))){
                fread( lp_date, 20, 1, fp);
                fclose(fp);
        }
        websWrite(wp, "%s", lp_date);
}

void return_online_lp_check(webs_t wp)
{
    char result[1024]={"ERROR"};
    char *check_fw_url = NULL;
    char *wan_eth = NULL;
    FILE *fp;
    char *ptr=NULL;
    char lp_reg[4]={'\0'};
    char check_lp_url[1024]={'\0'};

    check_fw_url = nvram_safe_get("check_fw_url");
    wan_eth = nvram_safe_get("wan_eth");


    if((fp = fopen(LP_REG_PATH,"r"))){
        ptr= strrchr( check_fw_url,'_');
        fread( lp_reg, 2, 1, fp);
        fclose(fp);
        snprintf(check_lp_url, ptr - check_fw_url + 1,"%s", check_fw_url);
        sprintf( check_lp_url, "%s_%sLP", check_lp_url, lp_reg);
        printf("\n%s\n",check_lp_url);
                sprintf(result,"%s", online_firmware_check(wan_eth, check_lp_url, "/var/tmp/lp_check_result.txt"));
    }
    DEBUG_MSG("%s\n",result);
    websWrite(wp, "%s", result);
}
#endif

/* apply_XXX state */
/**************************************************************/
/**************************************************************/
