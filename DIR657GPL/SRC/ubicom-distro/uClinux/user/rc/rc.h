/* rc.h 
 *
 * $Id: rc.h,v 1.17 2009/05/23 09:13:07 macpaul_lin Exp $
 */

#ifndef _RC_H
#define _RC_H
#include "nvram.h"
#include "autoconf.h"
/* WAN */
int start_wan(void);
int stop_wan(void);
int start_pptp(void);
int stop_pptp(void);
int start_l2tp(void);
int stop_l2tp(void);
#ifdef MPPPOE
int start_pppoe(int ppp_ifunit);
#else
//Albert modiyf 20081024
//--
//int start_pppoe(int ppp_ifunit, char* wan_eth, char* service_name);
int start_pppoe(int ppp_ifunit, char* wan_eth, char* service_name, char *ac_name);
//--
#endif
int stop_pppoe(int ppp_ifunit);
int start_bigpond(void);
int stop_bigpond(void);
void modify_ppp_options(void);
int modify_sercets(char *file);
//Albert modify //2008/10/21
//-----
//int modify_l2tp_conf(void);
int modify_l2tp_conf(char *ip);
//----
int ipup_main(int argc, char **argv);
int ipdown_main(int argc, char **argv);

/*
 * WISH
 */
extern int ubicom_wish_enabled(void);
extern int ubicom_wish_media_enabled(void);
extern int ubicom_wish_http_enabled(void);
extern void ubicom_wish_start(void);
extern void ubicom_wish_stop(void);

/* LAN */
int start_lan(void);
int stop_lan(void);

/*DHCP RELAY*/
#define DHCPRELAY_CONFIG "/tmp/dhcp-fwd.conf"
#define DHCPRELAY_PID "/var/run/dhcp-fwd.pid"

/* IPv6 */
#ifdef IPv6_SUPPORT
#define IPV6ADDR_LOOPBACK      0x0010U
#define IPV6ADDR_LINKLOCAL     0x0020U
#define IPV6ADDR_SITELOCAL     0x0040U
#define IPV6ADDR_COMPATv4      0x0080U
#define IPV6ADDR_SCOPE_MASK    0x00f0U
#define _PROCNET_IFINET6       "/proc/net/if_inet6"
extern int inet6_br0_status;
extern int inet6_eth1_status;
extern void get_inet6_status(void);
#endif //IPv6_SUPPORT

/* APP */
int start_app(void);
int stop_app(void);
int start_dnsmasq(void);
int stop_dnsmasq(int flag);
int start_ntp(void);
int stop_ntp(int flag);
int start_upnp(void);
int stop_upnp(int flag);
int start_mcast(void);
int stop_mcast(int flag);
int start_psmon(void);
int stop_psmon(int flag);
int start_dhcpd(void);
int stop_dhcpd(int flag);
int start_dhcp_relay(void);
int stop_dhcp_releay(int flag);
int start_ddns(void);
int stop_ddns(int flag);
int start_tftpd(void);
int stop_tftpd(int flag);
int start_maillog(void);
int stop_maillog(int flag);
int start_wantimer(void);
int stop_wantimer(int flag);
int start_lanmon(void);
int stop_lanmon(int flag);
int start_syslogd(void);
int stop_syslogd(int flag);
int start_klogd(void);
int stop_klogd(int flag);
int start_dhcpd6 (void);
int stop_dhcpd6 (int flag);

#ifdef CONFIG_USER_OWERA	
int start_owera(void);
int stop_owera(void);
#endif

#ifdef CONFIG_USER_OPENAGENT	
int start_tr069(void);
int stop_tr069(void);
#endif

#ifdef CONFIG_USER_NMBD			
int start_nmbd(void);
int stop_nmbd(void);
#endif				

#ifdef DLINK_FW_QUERY
int start_fwqd(void);
int stop_fwqd(void);
#endif

/* Firewall */
int start_firewall(void);
int stop_firewall(void);

/* WLAN */
int start_wlan(void);
int stop_wlan(void);
int start_ap(void);
void set_op_mode(void);

/* Network */
char *get_ipaddr(char *if_name);
char *get_macaddr(char *if_name);
int get_single_ip(char *ipaddr, int which);
char *get_dns(void);
int route_add(char *ifname, char *rt_destaddr, char *rt_netmask, char *rt_gateway, int rt_metric);
int route_del(char *ifname, char *rt_destaddr, char *rt_netmask, char *rt_gateway, int rt_metric);
int ifconfig_up(char *ifname, char *addr, char *netmask);
int ifconfig_down(char *ifname, char *addr, char *netmask);
int ifconfig_mac(char *eth, char *chmac);

/* Platform */
int wan_port_type_setting(void);

/* Ppp */
int start_monitor(int ppp_ifunit, char *wan_proto, char *wan_eth);
int stop_monitor(void);
//Albert modify 20081024
//--
//int start_redial(char *wan_proto, char *wan_eth, char *service_name);
int start_redial(char *wan_proto, char *wan_eth, char *service_name, char *ac_name);
#ifdef IPV6_PPPoE
int start_ipv6_redial(char *ipv6_wan_proto, char *wan_eth, char *ipv6_service_name, char *ipv6_ac_name);
int stop_ipv6_redial(void);
int ipv6_redial_main(int argc, char **argv);
int start_ipv6_monitor( char *ipv6_wan_proto, char *wan_eth);
int ipv6_monitor_main(int argc, char** argv);
int stop_ipv6_monitor(void);
//--
#endif
int stop_redial(void);
int start_ppp_timer(void);
int stop_ppp_timer(void);
int redial_main(int argc, char **argv);
int monitor_main(int argc, char** argv);
int ppptimer_main(int argc, char **argv);
int check_timer(void);

/* Route */
int start_route(void);
int stop_route(void);

/* Process monitor */
int psmon_main(int argc, char** argv);
int psmon_upnp(void);

/* Wan connection time mointor */
int wantimer_main(int argc, char** argv);

/* MTD */
int fwupgrade_main(int argc, char **argv);


/* process */
int checkServiceAlivebyName(char *service);

#ifdef CONFIG_USER_TC
int start_qos(void);
int stop_qos(void);
/* Measure Bandwidth */
/* jimmy marked 20080602 */
#if 0
int start_mbandwidth(void);
int stop_mbandwidth(void);
int check_ping_result(char*,char*);
int mbandwidth_main(int argc, char**argv);
int measure_bandwidth(char*,char*,char*,char*);
float transmit_test_packet(int, int, char*, char*, char*);
#define STRING_LENGTH 100
#endif
/* ----------------------- */
void get_measure_bandwidth(char *bandwidth);
#endif
   
struct action_flag {
	int all;
	int wan;
	int lan;
	int wlan;
	int app;
	int firewall;
	int lan_app;
#ifdef CONFIG_USER_TC
	int qos;
#endif
};

extern struct action_flag action_flags;

extern const unsigned char __CAMEO_LINUX_VERSION__[];

/*	Date: 2009-2-1
*	Name: Nick Chou
*	Reason: check 3G USB Device Type 
*	Note:
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
typedef struct usb_3g_device_table_s{
	char vendorID[6];
	char productID[6];
	char manufactor[16];
	char model[16];
}usb_3g_device_table_t;

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
typedef struct usb_device_table_s{
        char vendorID[6];
        char productID[6];
        char manufactor[16];
        char model[16];
        char os[16];
        //char wan_protocol[12];
        char wan_proto[12];
}usb_device_table_t;
#endif // end defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)

#endif

#ifdef CONFIG_USER_WAN_8021X
#define WAN_WPA_CONFIG_TPL "/etc/wpa.conf"
#define WAN_WPA_CONFIG "/var/etc/wpa.conf"
#define WAN_WPA_PID "/var/run/wan_wpa.pid"
int start_wan_supplicant(void);
int stop_wan_supplicant(void);
#endif

/*
 * Useful defines
 */
#define KILL_APP_SYNCH(app_name)  _system("killall " app_name " > /dev/null 2>&1")
#define KILL_APP_ASYNCH(app_name) _system("killall " app_name " > /dev/null 2>&1 &")

struct pkt_counter_s 
{
	unsigned long rx_bytes;
	unsigned long rx_packets;
	unsigned long rx_errs;
	unsigned long rx_drop;
	unsigned long rx_fifo;
	unsigned long rx_frame;
	unsigned long rx_compressed; 
	unsigned long rx_multicast;

	unsigned long tx_bytes;
	unsigned long tx_packets; 
	unsigned long tx_errs;
	unsigned long tx_drop; 
	unsigned long tx_fifo;
	unsigned long tx_colls;
	unsigned long tx_carrier;
	unsigned long tx_compressed;

};

extern int RC_get_if_pkt_counters(const char if_name[], struct pkt_counter_s *pkt_count);


#endif
