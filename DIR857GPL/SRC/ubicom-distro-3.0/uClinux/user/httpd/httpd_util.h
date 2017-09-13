#ifndef _HTTPD_UTIL_INCLUDED
#define _HTTPD_UTIL_INCLUDED

#include <cmoapi.h>
#include <sys/sysinfo.h>

#define M_WLAN_MAC_LEN 17
#define M_MAX_STATION 30
#define M_MAX_DHCP_LIST 254
#define WIRELESS_BAND_24G   0
#define WIRELESS_BAND_50G   1
#define M_MAX_IGMP_LIST 500

/* wan connect time */
#define WAN_CONNECT_FILE      "/var/tmp/wan_connect_time.tmp"
#define WAN_DISCONNECT_INIT   0
#define WAN_DISCONNECT        1

/* pppoe connect time */
#define PPPoE_CONNECT_FILE    "/var/tmp/pppoe00_connect_time.tmp"
#define PPPoE_DISCONNECT_INIT 0
#define PPPoE_DISCONNECT      1

/* smtp log mail*/
#define LOGFILE_PATH                    "/var/tmp/logmail.txt"

/* active session file */
#define IP_CONNTRACK_FILE               "/proc/net/ip_conntrack"
#define MAX_NUMBER_FIELD                14
#define IPV6_WAN_CONNECT_FILE           "/var/tmp/ipv6_wan_connect_time.tmp"

typedef enum IfType
{
        IFTYPE_BRIDGE = 0,
        IFTYPE_ETH_LAN = 1,
        IFTYPE_WLAN = 2,
        IFTYPE_ETH_WAN = 3,
//5G interface 2
        IFTYPE_WLAN1 = 4
}eIfType;

typedef enum PktType
{
        TXPACKETS =1,
        RXPACKETS,
        LOSSPACKETS
}ePktType;

#ifndef MPPPOE
typedef enum ByteType
{
        TXBYTES =1,
        RXBYTES,
}eByteType;
#endif

typedef enum FrameType
{
        TXFRAMES =1,
        RXFRAMES,
}eFrameType;

typedef enum CollisionType
{
        LanCollision =1,
        WanCollision,
        WlanCollision
}eCollisionType;

typedef struct
{
        char pMac_addr[M_WLAN_MAC_LEN + 1];
} T_WLAN_MAC_LEN;


typedef struct
{
        int num_sta;
        wireless_stations_table_t *p_st_wlan_sta;               //array of wireless_stations_table_t info
}T_WLAN_STAION_INFO;

typedef struct
{
        int num_list;
        dhcpd_leased_table_t *p_st_dhcpd_list;          //array of dhcpd_leased_table_t info
}T_DHCPD_LIST_INFO;

typedef struct
{
        int num_list;
        igmp_table_t *p_igmp_list;
}T_IGMP_LIST_INFO;

typedef struct
{
        int num_list;
        mssid_vlan_table_t *p_st_vlan_list;             //array of dhcpd_leased_table_t info
}T_VLAN_LIST_INFO;

typedef struct {
    int ASCII_flag[1];
#ifdef USER_WL_ATH_5GHZ
        int a_ASCII_flag[1]; //for Dual Concurrent Wireless
#endif       
    int temp_ASCII_flag[1];
    int wep64_flag[1]; 
    int wep128_flag[1]; 
    int wep152_flag[1];
}T_WLAN_WEP_FLAG_INFO;

typedef struct
{
        int num_list;
        napt_session_table_t *p_st_napt_list;
}T_NAPT_LIST_INFO;

#ifndef _HTTPD_UTIL_C_SRC

extern T_WLAN_STAION_INFO wlan_staion_info;
extern wireless_stations_table_t wlan_sta_table[M_MAX_STATION];
extern T_WLAN_MAC_LEN wlan_mac_addr[M_MAX_STATION];
extern T_DHCPD_LIST_INFO* get_dhcp_client_list(void);
extern char p_wlan_bytes[sizeof(long)*8+1] ;
extern char p_wlan_byte_bytes[sizeof(long)*8+1] ;
extern char p_lan_bytes[sizeof(long)*8+1] ;
extern char p_lan_byte_bytes[sizeof(long)*8+1] ;
extern char p_collision[sizeof(long)] ;

extern char nvram_wepkey_value[256] ;
extern T_WLAN_WEP_FLAG_INFO *p_wlan_wep_flag_info;
#endif

T_WLAN_STAION_INFO* get_stalist(void);
char* display_lan_pkts(ePktType packet_type);

#ifndef MPPPOE
char* display_lan_bytes(eByteType byte_type);
char* display_wan_bytes(eByteType byte_type);
char* display_wlan_bytes(eByteType byte_type);
#endif

char* display_wlan_pkts(ePktType packet_type);
char* display_wan_pkts(ePktType packet_type);
char* display_collisions(eCollisionType collision_type);
char* display_wlan_frames(eFrameType frame_type);
/*  Date: 2009-2-17
*   Name: Ken Chiang
*   Reason: added to change wireless region from show Domain name to Country name for version.txt.
*   Notice :
*/
char *GetCountryName(unsigned short number);
char *GetDomainName(unsigned short number);
extern char *GetDomainNameByRegion(char *region_s);
void get_channel(char *wlan_channel);
void get_resetting_bytes(void);
void get_ifconfig_resetting_bytes(void);
char *read_current_ipaddr(int ifnum);
char *read_current_wanphy_ipaddr(void);
char *wan_statue(const char *proto);
routing_table_t *read_route_table(void);
extern char *upload_configuration_init(char *path , char *name_string, char *boundry, int conf_ver_ctl);

extern int get_if_macaddr(int iftype, char *macaddr);
extern char *read_ipaddr(char *if_name);
extern unsigned char *get_ping_app_stat(unsigned char *host);
extern unsigned char *get_ping_app_stat_display(unsigned char *host);
#ifdef IPV6_SUPPORT
extern char *read_ipv6addr(const char *if_name,const int scope, char *ipv6addr,const int length);
extern char *read_ipv6defaultgateway(const char *if_name, char *ipv6addr);
unsigned char *get_ping6_app_stat(unsigned char *host,char *interface,int size,int count);
unsigned char *get_ping6_app_test(unsigned char *host);
#endif
extern char* convert_webvalue_to_nvramvalue(char *p_nvrma_name, char *web_value);
#endif

/* smtp log mail*/
//extern int send_log_mail(char *server,char *account,char *wan_ip);
/* system counter from booting */
extern unsigned long uptime(void);
/* wan connect time */
extern unsigned long get_wan_connect_time(char *wan_proto, int check_wan_status);
extern unsigned long get_ipv6_wan_connect_time(void);
/* pppoe connect time */
extern unsigned long get_pppoe00_connect_time(int type);
/* check internet connection */
extern void check_internet_connection(char *ip_addr);
/* define for logout */

void set_push_button_result(int result);
int get_push_button_result(void);
extern int push_button_result;

/*      Date: 2009-01-20
*       Name: jimmy huang
*       Reason: for usb-3g detect status
*       Note:   0: disconnected
                        1: connected
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
int is_3g_device_connect(void);
/*	Date:	2010-01-22
*	Name:	jimmy huang
*	Reason:	Add support for Windows Mobile 5
*			Because WM5 will OFFER 192.168.0.0/24 which is confilict with our lan ip range
*			Thus we have to force to change our lan ip to 192.168.99.0/24
*/
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
int change_ip_class_c(char *old_ip,int class_c_new_value,char *new_ip);
#endif

#endif // end CONFIG_USER_3G_USB_CLIENT

#define ADD_MAC        0
#define GET_FLG        1
#define SET_FLG        2
#define GET_PAG        3
#define SET_PAG        4
#define MAX_TMP        32
#define DEL_MAC        5
#define NULL_LIST      6
/*
#define MAX_TMP        64
*/
/*      Date: 2009-01-20
*       Name: jimmy huang
*       Reason: Marked to avoid compiler warning
*       Note: redefinition of MAT_TMP, marked the smaller one
*/

/*NickChou copy from wireless.h, for using ioctl getting wlan channel*/

struct  iw_freq
{
        int     m;              /* Mantissa */
        int     e;              /* Exponent */
        int     i;              /* List index (when in range struct) */
        int     flags;  /* Flags (fixed/auto) */
};

union   iwreq_data
{
        struct iw_freq  freq;   /* frequency or channel*/
};

struct  iwreq
{
        union
        {
                char    ifrn_name[16];  /* if name, e.g. "eth0" */
        } ifr_ifrn;

        union   iwreq_data      u;
};

typedef struct  iw_freq iwfreq;
