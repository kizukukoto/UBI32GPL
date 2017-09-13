#ifndef __IPV6_H
#define __IPV6_H

#define CLIENT_LIST_FILE			"/var/tmp/local_lan_ip"
#define IPV6_CLIENT_LIST			"/var/tmp/ipv6_client_list"             
#define IPV6_CLIENT_INFO			"/var/tmp/ipv6_client_info"            
#define IPV6_CLIENT_RESULT			"/var/tmp/ipv6_client_result"  
#define MAC					1
#define HOSTNAME				2
#define IPV6_ADDRESS				3
#define IPV6_BUFFER_SIZE			4096
#define RDNSSD_PID				"/var/run/rdnssd.pid"
#define RDNSSD_SCRIPT_FILE			"/etc/rdnssd-script"    
#define RDNSSD_SCRIPT_FILE_NOLANRDNSS		"/etc/rdnssd-script.nolanrdnss" 
#define RDNSSD_RESOLV_FILE			"/var/etc/resolv_ipv6.conf"     
#define IPv6PPPOE_OPTIONS_FILE			"/var/etc/options_06"           
#define PPP6_PID				"/var/run/ppp-06.pid"           
#define MON6_PID				"/var/run/mon6.pid"             
#define RED6_PID				"/var/run/red6.pid"             
#define SIT1_PID				"/var/run/sit1.pid"             
#define TUN6TO4_PID				"/var/run/tun6to4.pid"  
#define SIT2_PID				"/var/run/sit2.pid"             
#define FW_IPV6_FILTER				"/var/tmp/fw_ipv6_filter"       
#define FW_IPV6_FLUSH				"/var/tmp/fw_ipv6_flush"        
#define RESOLVFILE_IPV6				"/var/etc/resolv_ipv6.conf"     
#define RESOLVFILE_DUAL				"/var/etc/resolv_dual.conf"     
#define MAX_IPV6_IP_LEN				45
#define SCOPE_LOCAL				0
#define SCOPE_GLOBAL				1
#define CHAP_REMOTE_NAME			"CAMEO"
#define RADVD_PID				"/var/run/radvd.pid"
#define RADVD_CONF_FILE				"/etc/radvd.conf"                       
#define LINK_LOCAL_INFO				"/var/etc/link_local_info"              
#define DHCPD6_CONF_FILE			"/var/etc/dhcpd6.conf"                  
#define DHCPD6_GUEST_CONF_FILE			"/var/etc/dhcpd6_guest.conf"                  
#define DHCPD6_LEASE_FILE			"/var/misc/dhcpd6.lease"                
#define DHCPD6_PID				"/var/run/dhcp6s.pid"           
#define DHCP_PD					"/var/tmp/dhcp_pd"
#define DHCP_PD_OLD				"/var/tmp/dhcp_pd.old"
#define DHCP6C_CONF_FILE			"/var/etc/dhcp6c.conf"                  
#define DHCP6C_SCRIPT_FILE			"/etc/dhcp6c-script"                    
#define DHCP6C_PID				"/var/run/dhcp6c.pid"                  
#define DHCP6C_SCRIPT_FILE_NOLANRDNSS		"/etc/dhcp6c-script.nolanrdnss"
#define RESOLVFILE				"/var/etc/resolv.conf"
#define CHAP_SECRETS_V6				"/var/etc/chap-secrets-v6"
#define PAP_SECRETS_V6				"/var/etc/pap-secrets-v6"
#define IPV6_ROUTING_INFO			"/var/tmp/routingv6"
#define MAX_STATIC_ROUTINGV6_NUMBER		10
#define WIZARD_IPV6				"/var/tmp/wizard_ipv6"
#define DSLITE_PID				"/var/run/dslite.pid"             

#ifdef CONFIG_USER_SCHEDULE_NUMBER
#define MAX_SCHEDULE_NUMBER			CONFIG_USER_SCHEDULE_NUMBER
#else
#define MAX_SCHEDULE_NUMBER			10             
#endif

enum{
	AUTODETECT,
	STATIC,
	AUTOCONFIG,
	PPPOE,
	_6IN4,
	_6TO4,
	_6RD,
	LINKLOCAL,
};

extern int init_file(char *file);
extern void save2file(const char *file, const char *fmt, ...);
extern void string_insert(char *st, char insert[], int start);
extern char *get_ipv6_prefix (char *ipv6_addr, int length);
extern char *read_dns(void);
extern void write_dhcpd6_conf (int pd_len);
extern char *get_link_local_ip_l(char *interface_name);
#ifdef MRD_ENABLE
extern void write_mrd_conf(void);
#endif
extern void write_radvd_conf(int pd_len);
extern void write_ipv6_pppoe_options(void);
extern char *parse_special_char(char *p_string);
extern void write_ipv6_pppoe_sercets(char *file);
extern char *read_ipv6addr(const char *if_name,const int scope, char *ipv6addr,const int length);
extern void copy_mac_to_eui64(char *mac, char *ifid);
extern void trigger_send_rs(char *interface);
extern void set_default_accept_ra(int flag);
extern int set_ipv6_specify_dns(void);
extern int ifconfirm(char *if_name);
extern int write_static_pd();







#endif //__IPV6_H
