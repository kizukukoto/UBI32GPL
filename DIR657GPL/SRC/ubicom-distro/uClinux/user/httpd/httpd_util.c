#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <getopt.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux_vct.h>
#include <nvram.h>
#include <sutil.h>
#include <httpd_util.h>
#include <shvar.h>
#include <leases.h>


#define SMTP_MAX_ADDR_NUM 10
#define HOST_NAME                                       "WBR-3310"
#define _HTTPD_UTIL_C_SRC
#define RTF_UP                  0x0001          /* route usable                 */
#define RTF_GATEWAY     0x0002          /* destination is a gateway     */
#define PATH_PROCNET_ROUTE      "/proc/net/route"
#define RESOLVCONF                              "/var/etc/resolv.conf"

//#define HTTPD_UTIL_DEBUG 1
#ifdef HTTPD_UTIL_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

typedef struct _smtpb_t {                                       // smtp for log mail
        int sock;
        char host_name[40];
        char *server;
        char *addr_buf;
        char *addr_p[SMTP_MAX_ADDR_NUM];
        char *wan_ip;
        int addr_num;
} _SMTPB_s;
struct hostent * he;
_SMTPB_s *smtpb = NULL;

T_WLAN_WEP_FLAG_INFO wlan_wep_flag_info = {{0},{0},{0},{0}};
T_WLAN_WEP_FLAG_INFO *p_wlan_wep_flag_info = &wlan_wep_flag_info;
T_WLAN_STAION_INFO wlan_staion_info;
wireless_stations_table_t wlan_sta_table[M_MAX_STATION];
#ifdef USER_WL_ATH_5GHZ
T_WLAN_STAION_INFO wlan_staion_5G_info;
wireless_stations_table_t wlan_sta_5G_table[M_MAX_STATION];
#endif
T_WLAN_MAC_LEN wlan_mac_addr[M_MAX_STATION];

char nvram_wepkey_value[256] ={0};

dhcpd_leased_table_t client_list[M_MAX_DHCP_LIST];
struct html_leases_s leases[M_MAX_DHCP_LIST];
T_DHCPD_LIST_INFO dhcpd_list;

igmp_table_t igmp_return_member[M_MAX_IGMP_LIST];
struct igmp_list_s igmp_member[M_MAX_IGMP_LIST];
T_IGMP_LIST_INFO igmp_list;

routing_table_t routing_table[];

/*      Date: 2009-01-20
*       Name: jimmy huang
*       Reason: for usb-3g detect status
*       Note:
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
usb_3g_device_table_t usb_3g_device_table[]={
        {"1186","3e04","Dlink","DWM-652"},
        {"NULL","NULL","NULL","NULL"}
};
#endif

napt_session_table_t session_list[0];
T_NAPT_LIST_INFO napt_session_list;

//unsigned char returnVal[256]; //15
/* common interface for get bytes and packets */
#ifndef MPPPOE
static char* display_bytes(eByteType byte_type,char *interface);
#endif
static char* display_pkts(ePktType packet_type, char *interface);
static char* display_real_pkts(ePktType packet_type, char *interface);
/* CmoGetStatus */
static char* display_real_wan_pkts(ePktType packet_type);
static char* display_real_lan_pkts(ePktType packet_type);
static char* display_real_wlan_pkts(ePktType packet_type);
/* common variable for get bytes and packets */
char p_pkts[sizeof(long)*8+1] ;
char p_real_pkts[sizeof(long)*8+1] ;
char p_byte_bytes[sizeof(long)*8+1] ;
char p_frames[sizeof(long)*8+1] ;
char p_collision[sizeof(long)] ;
char p_ifconfig_info[64];

/* common variable for initial packets*/
static char p_wlan_txbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_lan_txbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wan_txbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wlan_rxbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_lan_rxbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wan_rxbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wlan_lossbytes_on_resetting[sizeof(long)*8+1]={0};
static char p_lan_lossbytes_on_resetting[sizeof(long)*8+1] ={0};
static char p_wan_lossbytes_on_resetting[sizeof(long)*8+1] ={0};

unsigned long rx_packets_on_resetting[4] = {0, 0, 0, 0}; /* wan/lan/wlan/wlan1 */
unsigned long tx_packets_on_resetting[4] = {0, 0, 0, 0};
unsigned long rx_dropped_on_resetting[4] = {0, 0, 0, 0};
unsigned long tx_dropped_on_resetting[4] = {0, 0, 0, 0};
unsigned long collisions_on_resetting[4] = {0, 0, 0, 0};
unsigned long error_on_resetting[4]      = {0, 0, 0, 0};

/* check port connection time */
void check_wan_connection_time(void);
void check_pppoe_connection_time(void);

static char* pWepKeyIndex[37] =
{
        "wlan0_wep64_key_1",
        "wlan0_wep64_key_2",
        "wlan0_wep64_key_3",
        "wlan0_wep64_key_4",
        "wlan1_wep64_key_1" ,
        "wlan1_wep64_key_2",
        "wlan1_wep64_key_3",
        "wlan1_wep64_key_4",
        "wlan0_vap1_wep64_key_1",
        "wlan0_vap1_wep64_key_2",
        "wlan0_vap1_wep64_key_3",
        "wlan0_vap1_wep64_key_4",
        "wlan1_vap1_wep64_key_1",
        "wlan1_vap1_wep64_key_2",
        "wlan1_vap1_wep64_key_3",
        "wlan1_vap1_wep64_key_4",
        "wlan0_wep128_key_1",
        "wlan0_wep128_key_2",
        "wlan0_wep128_key_3",
        "wlan0_wep128_key_4",
        "wlan1_wep128_key_1",
        "wlan1_wep128_key_2",
        "wlan1_wep128_key_3",
        "wlan1_wep128_key_4",
        "wlan0_vap1_wep128_key_1",
        "wlan0_vap1_wep128_key_2",
        "wlan0_vap1_wep128_key_3",
        "wlan0_vap1_wep128_key_4",
        "wlan1_vap1_wep128_key_1",
        "wlan1_vap1_wep128_key_2",
        "wlan1_vap1_wep128_key_3",
        "wlan1_vap1_wep128_key_4",
        "wlan0_wep152_key_1",
        "wlan0_wep152_key_2",
        "wlan0_wep152_key_3",
        "wlan0_wep152_key_4"
};

static char* pTempWepKeyIndex[13] =
{
        "asp_temp_wep64_key_1",
        "asp_temp_wep64_key_2",
        "asp_temp_wep64_key_3",
        "asp_temp_wep64_key_4",
        "asp_temp_wep128_key_1",
        "asp_temp_wep128_key_2",
        "asp_temp_wep128_key_3",
        "asp_temp_wep128_key_4",
        "asp_temp_wep152_key_1",
        "asp_temp_wep152_key_2",
        "asp_temp_wep152_key_3",
        "asp_temp_wep152_key_4"
};

#ifndef MPPPOE
static char* display_bytes(eByteType byte_type,char *interface){
        FILE *fp = NULL;
        char *data = NULL;
        char *p_bytes_s = NULL;
        char *p_bytes_e = NULL;
        int file_len = 0;

        memset(p_byte_bytes, 0, sizeof(long)*8+1);
        if( (nvram_match("wlan0_enable", "0") == 0) && (strcmp(interface, WLAN0_ETH) == 0) )
                return NULL;
        else
        _system("ifconfig %s > /var/misc/bytes.txt",interface);

        fp = fopen("/var/misc/bytes.txt","rb");
        if (fp == NULL){
                return NULL;
        }

        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        data = (char*)malloc(file_len);
        if(data == NULL || file_len == 0){
                fclose(fp);
                DEBUG_MSG("display bytes memory allocate failed\n");
                return NULL;
        }
        fseek(fp, 0, SEEK_SET);
        fread(data, file_len, 1, fp);

        switch (byte_type){
                case TXBYTES:
                        p_bytes_s = strstr(data, "TX bytes:") + 9;      //9 is len of TX bytess:
                        p_bytes_e = strstr(p_bytes_s, "(") ;
                        if(p_bytes_s == NULL || p_bytes_e == NULL){
                                DEBUG_MSG("tx bytes not found\n");
                                return NULL;
                        }
                        memcpy(p_byte_bytes, p_bytes_s, p_bytes_e - p_bytes_s);
                        break;
                case RXBYTES:
                        p_bytes_s = strstr(data, "RX bytes:") + 9;      //11 is len of RX packets:
                        p_bytes_e = strstr(p_bytes_s, "(") ;
                        if(p_bytes_s == NULL || p_bytes_e == NULL)
                        {
                                DEBUG_MSG("rx bytes not found\n");
                                return NULL;
                        }
                        memcpy(p_byte_bytes, p_bytes_s, p_bytes_e - p_bytes_s);
                        break;
                default :
                        DEBUG_MSG("bytes not found\n");
                        fclose(fp);
                        return NULL;
        }

        free(data);
        fclose(fp);
        return p_byte_bytes;
}

char* display_lan_bytes(eByteType byte_type){
        return display_bytes(byte_type,nvram_safe_get("lan_eth"));
        }

char* display_wan_bytes(eByteType byte_type){
        return display_bytes(byte_type,nvram_safe_get("wan_eth"));
                        }

char* display_wlan_bytes(eByteType byte_type){
        return display_bytes(byte_type, WLAN0_ETH);
        }
#endif

char* display_pkts(ePktType packet_type, char *interface){
        FILE *fp = NULL;
        char *data = NULL, *p_pkts_s = NULL, *p_pkts_e = NULL;
        int file_len = 0;
        char p_tx_lossbytes[sizeof(long)*8+1] ={0};
        char p_rx_lossbytes[sizeof(long)*8+1] ={0};
        unsigned long total_loss_bytes = 0;
        unsigned long after_reset_bytes = 0;

        memset(p_pkts, 0, sizeof(long)*8+1);
        if( (nvram_match("wlan0_enable", "0") == 0) && (strcmp(interface, WLAN0_ETH) == 0) )
                return NULL;
        else
        _system("ifconfig %s > /var/misc/pkts.txt",interface);

        fp = fopen("/var/misc/pkts.txt","rb");
        if (fp == NULL)
                return NULL;

        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        data = (char*)malloc(file_len);
        if(data == NULL || file_len == 0){
                fclose(fp);
                DEBUG_MSG("display pkts memory allocate failed\n");
                return NULL;
        }
        fseek(fp, 0, SEEK_SET);
        fread(data, file_len, 1, fp);


        switch (packet_type){
                case TXPACKETS:
                        p_pkts_s = strstr(data, "TX packets:") + 11;    //11 is len of TX packets:
                        p_pkts_e = strstr(p_pkts_s, "errors:") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("tx pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_pkts, p_pkts_s, p_pkts_e - p_pkts_s);
#ifndef DEVICE_NO_ROUTING
                        if(strcmp(interface,nvram_safe_get("wan_eth")) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_wan_txbytes_on_resetting);
                        else
#endif
                         if(strcmp(interface,nvram_safe_get("lan_eth")) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_lan_txbytes_on_resetting);
                        else if(strcmp(interface, WLAN0_ETH) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_wlan_txbytes_on_resetting);
                        memset(p_pkts, 0, sizeof(long)*8+1);
                        sprintf(p_pkts, "%lu", after_reset_bytes);
                        break;
                case RXPACKETS:
                        p_pkts_s = strstr(data, "RX packets:") + 11;    //11 is len of RX packets:
                        p_pkts_e = strstr(p_pkts_s, "errors:") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("rx pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_pkts, p_pkts_s, p_pkts_e - p_pkts_s);
#ifndef DEVICE_NO_ROUTING
                        if(strcmp(interface,nvram_safe_get("wan_eth")) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_wan_rxbytes_on_resetting);
                        else
#endif
                        if(strcmp(interface,nvram_safe_get("lan_eth")) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_lan_rxbytes_on_resetting);
                        else if(strcmp(interface, WLAN0_ETH) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_wlan_rxbytes_on_resetting);
                        memset(p_pkts, 0, sizeof(long)*8+1);
                        sprintf(p_pkts, "%lu", after_reset_bytes);
                        break;
                case LOSSPACKETS:
                        p_pkts_s = strstr(data, "dropped") + 8;                 //8 is len of dropped:
                        p_pkts_e = strstr(p_pkts_s, "overruns") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("tx loss pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_tx_lossbytes, p_pkts_s, p_pkts_e - p_pkts_s);
                        p_pkts_s = strstr(p_pkts_e, "dropped") + 8;                     //8 is len of dropped:
                        p_pkts_e = strstr(p_pkts_s, "overruns") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("rx loss pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_rx_lossbytes, p_pkts_s, p_pkts_e - p_pkts_s);
                        total_loss_bytes = atoi(p_tx_lossbytes)+atoi(p_rx_lossbytes);
                        sprintf(p_pkts, "%lu", total_loss_bytes);
#ifndef DEVICE_NO_ROUTING
                        if(strcmp(interface,nvram_safe_get("wan_eth")) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_wan_lossbytes_on_resetting);
                        else
#endif
                        if(strcmp(interface,nvram_safe_get("lan_eth")) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_lan_lossbytes_on_resetting);
                        else if(strcmp(interface, WLAN0_ETH) == 0)
                                after_reset_bytes = atoi(p_pkts) - atoi(p_wlan_lossbytes_on_resetting);
                        memset(p_pkts, 0, sizeof(long)*8+1);
                        sprintf(p_pkts, "%lu", after_reset_bytes);
                        break;
                default :
                        DEBUG_MSG("pkts not found\n");
                        fclose(fp);
                        return NULL;
        }

        free(data);
        fclose(fp);
        return p_pkts;
}

char* display_wan_pkts(ePktType packet_type){
        return display_pkts(packet_type,nvram_safe_get("wan_eth"));
}
char* display_lan_pkts(ePktType packet_type){
        return display_pkts(packet_type,nvram_safe_get("lan_eth"));
}

char* display_wlan_pkts(ePktType packet_type){
        return display_pkts(packet_type, WLAN0_ETH);
}

static char* display_real_pkts(ePktType packet_type, char *interface){
        FILE *fp = NULL;
        char *data = NULL;
        char *p_pkts_s = NULL;
        char *p_pkts_e = NULL;
        int file_len = 0;
        char p_tx_lossbytes[sizeof(long)*8+1] ={0};
        char p_rx_lossbytes[sizeof(long)*8+1] ={0};
        unsigned long total_loss_bytes = 0;

        memset(p_real_pkts, 0, sizeof(long)*8+1);
        _system("ifconfig %s > /var/misc/pkts.txt",interface);
        fp = fopen("/var/misc/pkts.txt","rb");
        if (fp == NULL)
                return NULL;

        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        data = (char*)malloc(file_len);

  /* display pkts memory allocate failed */
        if(data == NULL || file_len == 0){
                fclose(fp);
                DEBUG_MSG("display pkts memory allocate failed\n");
                return NULL;
        }
        fseek(fp, 0, SEEK_SET);
        fread(data, file_len, 1, fp);


        switch (packet_type){
                case TXPACKETS:
                        p_pkts_s = strstr(data, "TX packets:") + 11;    //11 is len of TX packets:
                        p_pkts_e = strstr(p_pkts_s, "errors:") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("tx pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_real_pkts, p_pkts_s, p_pkts_e - p_pkts_s);
                        break;
                case RXPACKETS:
                        p_pkts_s = strstr(data, "RX packets:") + 11;    //11 is len of RX packets:
                        p_pkts_e = strstr(p_pkts_s, "errors:") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("rx pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_real_pkts, p_pkts_s, p_pkts_e - p_pkts_s);
                        break;
                case LOSSPACKETS:
                        p_pkts_s = strstr(data, "dropped") + 8;                 //8 is len of dropped:
                        p_pkts_e = strstr(p_pkts_s, "overruns") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("tx loss pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_tx_lossbytes, p_pkts_s, p_pkts_e - p_pkts_s);
                        p_pkts_s = strstr(p_pkts_e, "dropped") + 8;                     //8 is len of dropped:
                        p_pkts_e = strstr(p_pkts_s, "overruns") ;
                        if(p_pkts_s == NULL || p_pkts_e == NULL){
                                DEBUG_MSG("rx loss pkts not found\n");
                                return NULL;
                        }
                        memcpy(p_rx_lossbytes, p_pkts_s, p_pkts_e - p_pkts_s);
                        total_loss_bytes = atoi(p_tx_lossbytes)+atoi(p_rx_lossbytes);
                        sprintf(p_real_pkts, "%lu", total_loss_bytes);
                        break;
                default :
                        DEBUG_MSG("pkts not found\n");
                        fclose(fp);
                        return NULL;
                        break;
        }

        printf("display real p_real_pkts =%s\n", p_real_pkts);
        free(data);
        fclose(fp);
        return p_real_pkts;
}

static char* display_real_wan_pkts(ePktType packet_type){
        return display_real_pkts(packet_type,nvram_safe_get("wan_eth"));
}

static char* display_real_lan_pkts(ePktType packet_type){
        return display_real_pkts(packet_type,nvram_safe_get("lan_eth"));
}

static char* display_real_wlan_pkts(ePktType packet_type){
        return display_real_pkts(packet_type, WLAN0_ETH);
}

char* display_collisions(eCollisionType collision_type){
        FILE *fp = NULL;
        char *data = NULL;
        char *p_collision_s = NULL;
        char *p_collision_e = NULL;
        int file_len = 0;

        memset(p_collision, 0, sizeof(long));
        switch (collision_type){
                case LanCollision:
                        _system("ifconfig %s > /var/misc/pkts.txt", nvram_safe_get("lan_eth"));
                        break;
                case WanCollision:
                        _system("ifconfig %s > /var/misc/pkts.txt",nvram_safe_get("wan_eth"));
                        break;
                case WlanCollision:
                  _system("ifconfig %s > /var/misc/pkts.txt", WLAN0_ETH);
                        break;
                default:
                        return NULL;
        }

        fp = fopen("/var/misc/pkts.txt","rb");
        if (fp == NULL)
                return NULL;

        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        data = (char*)malloc(file_len);

        if(data == NULL || file_len == 0)
        {
                fclose(fp);
                return NULL;
        }
        fseek(fp, 0, SEEK_SET);
        fread(data, file_len, 1, fp);

        p_collision_s = strstr(data, "collisions:") + 11;       //11 is len of "collisions:" string:
        p_collision_e = strstr(p_collision_s, "txqueuelen:") ;

        if(p_collision_s == NULL || p_collision_e == NULL)
                return NULL;

        memcpy(p_collision, p_collision_s, p_collision_e - p_collision_s);
        free(data);
        fclose(fp);

        return p_collision;
}

char* display_ifconfig_info(eIfType IfType){
        FILE *fp = NULL;
        char buf[128] = {0},collision[16] = {0};
        char rx_packets[16] = {0},rx_error[16] = {0},rx_drop[16] = {0};
        char tx_packets[16] = {0},tx_error[16] = {0},tx_drop[16] = {0};
        char *packets, *error, *drop, *overrun, *collisions, *txqueuelen;
        int type=0;

        switch (IfType){
                case IFTYPE_ETH_WAN:
                        DEBUG_MSG("IFTYPE_ETH_WAN\n");
#ifdef CONFIG_USER_3G_USB_CLIENT
/*  Date: 2009-3-9
*   Name: Ken Chiang
*   Reason: modify to fix when deined CONFIG_USER_3G_USB_CLIENT can't show wan ifconfig_info.
*   Notice :
*/
                        if( nvram_match("wan_proto", "usb3g") == 0){
                                DEBUG_MSG("usb3g\n");
                                system("ifconfig ppp0 > /var/misc/ifconfig_info.txt");
                        }
                        else{
                                DEBUG_MSG("ethernet\n");
                                _system("ifconfig %s > /var/misc/ifconfig_info.txt", nvram_safe_get("wan_eth"));
                        }
#else
                        _system("ifconfig %s > /var/misc/ifconfig_info.txt", nvram_safe_get("wan_eth"));
#endif
                        type = 0;
                        break;
                case IFTYPE_ETH_LAN:
                        DEBUG_MSG("IFTYPE_ETH_LAN\n");
                        _system("ifconfig %s > /var/misc/ifconfig_info.txt",nvram_safe_get("lan_eth"));
                        type = 1;
                        break;
                //wireless interface 1(2.4G)
                case IFTYPE_WLAN:
                        DEBUG_MSG("IFTYPE_WLAN\n");
                        if(nvram_match("wlan0_enable","0") == 0)
                                return NULL;
                        else
                        {
#ifdef DIR865
                                _system("iwpriv %s stat > /var/misc/ifconfig_info.txt", WLAN0_ETH);
#else
                                _system("ifconfig %s > /var/misc/ifconfig_info.txt", WLAN0_ETH);
#endif
                                type = 2;
                        }
                        break;
                //wireless interface 2(5G)
                case IFTYPE_WLAN1:
                        DEBUG_MSG("IFTYPE_WLAN1\n");
#ifdef DIR865
                        if(nvram_match("wlan1_enable","0") == 0)
                                return NULL;
                        else
#endif
                        {
#ifdef DIR865
                                if(nvram_match("wlan0_enable", "1")==0)
                                        _system("iwpriv %s stat > /var/misc/ifconfig_info.txt", WLAN1_ETH);
                                else
                                        _system("iwpriv %s stat > /var/misc/ifconfig_info.txt", WLAN0_ETH);

#else
                                if(nvram_match("wlan0_enable", "1")==0)
                                        _system("ifconfig %s > /var/misc/ifconfig_info.txt", "ath1");
                                else
                                        _system("ifconfig %s > /var/misc/ifconfig_info.txt", "ath0");
#endif
                                type = 3;
                        }
                        break;
                default:
                        return NULL;
        }

        fp = fopen("/var/misc/ifconfig_info.txt","rb");
        if (fp == NULL){
                DEBUG_MSG("ifconfig_info.txt open fail.\n");
                return NULL;
        }
        while(fgets(buf,sizeof(buf), fp))
        {
                if(packets = strstr(buf,"RX packets"))
                {
                        DEBUG_MSG("RX packets\n");
                        if(error = strstr(buf,"errors"))
                        {
                                DEBUG_MSG("error\n");
                                packets = packets + 11;
                                error = error - 1;
                                strncpy(rx_packets,packets,error - packets);
                                DEBUG_MSG("rx_packets=%s\n",rx_packets);
                                if(drop = strstr(buf,"dropped"))
                                {
                                        DEBUG_MSG("dropped\n");
                                        error = error + 8;
                                        drop = drop - 1;
                                        strncpy(rx_error,error,drop - error);
                                        DEBUG_MSG("rx_error=%s\n",rx_error);
                                        if(overrun = strstr(buf,"overruns"))
                                        {
                                                DEBUG_MSG("overruns\n");
                                                drop = drop + 9;
                                                overrun = overrun - 1;
                                                strncpy(rx_drop,drop,overrun - drop);
                                                DEBUG_MSG("rx_drop=%s\n",rx_drop);
                                        }
                                }
                        }
                }

                if(packets = strstr(buf,"TX packets"))
        {
                        if(error = strstr(buf,"errors"))
                        {
                                packets = packets + 11;
                                error = error - 1;
                                strncpy(tx_packets,packets,error - packets);
                                if(drop = strstr(buf,"dropped"))
                                {
                                        error = error + 8;
                                        drop = drop - 1;
                                        strncpy(tx_error,error,drop - error);
                                        if(overrun = strstr(buf,"overruns"))
                                        {
                                                drop = drop + 9;
                                                overrun = overrun - 1;
                                                strncpy(tx_drop,drop,overrun - drop);
                                        }
                                }
                        }
        }

                if(collisions = strstr(buf,"collisions"))
                {
                        if(txqueuelen = strstr(buf,"txqueuelen"))
                        {
                                collisions = collisions + 11;
                                txqueuelen = txqueuelen - 1;
                                strncpy(collision,collisions,txqueuelen - collisions);
                        }
                }
#ifdef DIR865
                if(packets = strstr(buf,"Rx success"))
                {
                        DEBUG_MSG("Rx success\n");
                        if(packets = strstr(packets,"="))
                        {
                                strcpy(rx_packets,packets+1);
                                DEBUG_MSG("rx_packets=%s\n",rx_packets);
                        }
                }

                if(packets = strstr(buf,"Tx success"))
        {
             DEBUG_MSG("Tx success\n");
             if(packets = strstr(packets,"="))
             {
                  strcpy(tx_packets,packets+1);
                  DEBUG_MSG("tx_packets=%s\n",tx_packets);
             }
        }
#endif

        }

        sprintf(p_ifconfig_info, "%lu/%lu/%lu/%lu/%lu/%lu",
                                                        atol(rx_packets) - rx_packets_on_resetting[type],
                                                        atol(tx_packets) - tx_packets_on_resetting[type],
                                                        atol(rx_drop) - rx_dropped_on_resetting[type],
                                                        atol(tx_drop) - tx_dropped_on_resetting[type],
                                                        atol(collision) - collisions_on_resetting[type],
                                                        atol(rx_error) + atol(tx_error) - error_on_resetting[type]);

        DEBUG_MSG("p_ifconfig_info=%s\n",p_ifconfig_info);
        fclose(fp);
        return p_ifconfig_info;
}

char* display_wlan_frames(eFrameType frame_type){
        FILE *fp = NULL;
        char *data = NULL;
        char *p_frames_s = NULL;
        char *p_frames_e = NULL;
        int file_len = 0;

        _system("ifconfig %s > /var/misc/frame.txt", WLAN0_ETH);
        fp = fopen("/var/misc/frame.txt","rb");
        if (fp == NULL)
                return NULL;

  /* get data stream && check */
        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        data = (char*)malloc(file_len);
        if(data == NULL || file_len == 0)
                return NULL;
        fseek(fp, 0, SEEK_SET);
        fread(data, file_len, 1, fp);
  /* parse tx || rx keyword */
        switch (frame_type){
                case TXFRAMES:
                        /* not implemented */
                        break;
                case RXFRAMES:
                        p_frames_s = strstr(data, "frame:") + 6;        //6 is len of RX packets:
                        p_frames_e = strstr(p_frames_s, "TX");
                        if(p_frames_s != NULL && p_frames_e != NULL)
                          memcpy(p_frames,p_frames_s,p_frames_e - p_frames_s - 10);     //10 is the space length in front of TX packets:
                        break;
                default :
                        break;
        }
        free(data);
        fclose(fp);
        if(p_frames_s == NULL || p_frames_e == NULL)
                return NULL;
        return p_frames;
}

char *ether_etoa(const unsigned char *e, char *a)
{
        char *c = a;
        int i;

        for (i = 0; i < 6; i++) {
                if (i)
                        *c++ = ':';
                c += sprintf(c, "%02X", e[i] & 0xff);
        }
        return a;
}


int get_hw_addr(char *if_name, char *hwaddr)
{
        int sockfd;
        struct ifreq ifr;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0){
                strcpy(hwaddr, "00:00:00:00:00:00");
                return;
        }

        strcpy(ifr.ifr_name, if_name);

        if( ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
                close(sockfd);
                strcpy(hwaddr, "00:00:00:00:00:00");
                return;
        }
        close(sockfd);

        ether_etoa(ifr.ifr_hwaddr.sa_data, hwaddr);
        return 1;
}

int get_if_macaddr(int iftype, char *macaddr)
{
        FILE *fp;
        char *data, *hw_addr;
        int file_len;

        memset(p_pkts, 0, sizeof(long)*8+1);
        switch (iftype)
        {
                case IFTYPE_BRIDGE:
                        _system("ifconfig br0 > /var/tmp/mac.txt");
                        break;
                case IFTYPE_ETH_LAN:
                        _system("ifconfig %s > /var/tmp/mac.txt",nvram_safe_get("lan_eth"));
                        break;
                case IFTYPE_WLAN:
                        _system("ifconfig %s > /var/tmp/mac.txt", WLAN0_ETH);
                        break;
                case IFTYPE_ETH_WAN:
                        _system("ifconfig %s > /var/tmp/mac.txt", nvram_safe_get("wan_eth"));
                        break;
        }

        fp = fopen("/var/tmp/mac.txt","rb");
        if (fp == NULL)
        {
                strcpy(macaddr, "00:00:00:00:00:00");
                return -1;
        }

        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        data = (char*)malloc(file_len);
        if(data == NULL || file_len == 0)
        {
                strcpy(macaddr, "00:00:00:00:00:00");
                return -1;
        }
        fseek(fp, 0, SEEK_SET);
        fread(data, file_len, 1, fp);

        hw_addr = strstr(data, "HWaddr ") + strlen("HWaddr ");
        if(hw_addr == NULL)
        {
                DEBUG_MSG("HWaddr not found\n");
                strcpy(macaddr, "00:00:00:00:00:00");
                return -1;
        }

        memcpy(macaddr, hw_addr, strlen("00:00:00:00:00:00"));
        macaddr[strlen("00:00:00:00:00:00")] = 0;

        printf("macaddr =%s\n", macaddr);

        free(data);
        fclose(fp);
        return atoi(p_pkts);
}

/*NickChou, 20090806
Typically, "wlanconfig ath0 sta" shows clients associated to the AP and reports RSSI info.
The RSSI % can be derived in similar fashion as Atheros ACU client utility.
You can adjust the ratio if you like !!
*/
int rssi_to_ratio(char *rssi)
{
        int nRSSI=atoi(rssi);
        int signalStrength=0;

    if (nRSSI >= 40)
    {
        signalStrength = 100;// 40 dB SNR or greater is 100%
    }
    else if (nRSSI >= 20)// 20 dB SNR is 80%
    {
        signalStrength = nRSSI - 20;
        signalStrength = (signalStrength * 26) / 30;
        signalStrength += 80; // Between 20 and 40 dB, linearize between 80 and 100
    }
    else
    {
        signalStrength = (nRSSI * 75) / 20;
    }

    return signalStrength;
}

static int qcom(const int *a, const int *b)
{
        if ((*a) > (*b))
                return 1;
        else if ((*a) == (*b))
                return 0;
        else
                return -1;
}

/* Convert signal degree for Ralink */
static int convert_signal(char *rssi0, char *rssi1, char *rssi2)
{
        int annta[3], nRssi, i;

        annta[0] = atoi(rssi0);
        annta[1] = atoi(rssi1);
        annta[2] = atoi(rssi2);

        qsort(annta, 3, sizeof(int), qcom);

        nRssi = (annta[2] != 0) ? annta[2] : annta[1];

        if ( nRssi >= -50)
                return 100;
         else if (nRssi <= -51 && nRssi >= -80)
                return (int)((24 + (nRssi + 80) * 2.6)+ 0.5);
         else if (nRssi <= -81 && nRssi >= -90)
                return (int)(((nRssi + 90) * 2.6) + 0.5);
         else
                return 0;
}

T_DHCPD_LIST_INFO* dhcpd_status;

T_WLAN_STAION_INFO* get_stalist()
{
    FILE *fp,*fp1;
    char *ptr, *rssi_ptr, *rate_ptr,*nego_rate,*ptr1;
    char buff[1024], addr[20], rssi[4], mode[16], time[16],rate[8], ip_addr[20];
    int i;
    char *ptr_htcaps;
    int sta_is_n_mode;

    dhcpd_status = get_dhcp_client_list();
    
    if (nvram_match("wlan0_enable", "0") == 0)
        return NULL;

    wlan_staion_info.num_sta = 0;
    memset(wlan_sta_table, 0, sizeof(wireless_stations_table_t)*M_MAX_STATION);

    //wlanconfig ath0 list sta
    _system("wlanconfig %s list sta > /var/misc/stalist.txt", WLAN0_ETH);
    if (nvram_match("wlan0_vap1_enable", "1") == 0)
    {
#ifdef USER_WL_ATH_5GHZ
        if (nvram_match("wlan1_enable", "1") == 0)
            system("wlanconfig ath2 list sta >> /var/misc/stalist.txt"); //2.4GHz GuestZone
        else
#endif
            system("wlanconfig ath1 list sta >> /var/misc/stalist.txt"); //2.4GHz GuestZone
    }

    fp = fopen("/var/misc/stalist.txt","rb");
    if (fp == NULL) {
        DEBUG_MSG("stalist file open failed (No station connected). \n");
        return NULL;
    }

    while (fgets(buff, 1024, fp))
    {
        if (strstr(buff, "ID CHAN RATE RSSI IDLE"))
            continue;

		ptr_htcaps = NULL;
		sta_is_n_mode = 0;

        ptr = buff;
        if (strstr(buff, "UAPSD QoSInfo")) {
            DEBUG_MSG("get_stalist(): Having VISTA Client QoSInfo.\n");
            continue;
        }

        memset(addr, 0, 20);
        memset(rssi, 0, 4);
        memset(mode, 0, 16);
        memset(time, 0, 16);
        memset(rate, 0, 8);

        strncpy(addr, ptr, 17);
        ptr += strlen(addr);
        ptr += 5; /* skip one space + AID */
        ptr += 5; /* skip one space + CHAN */

        ptr += 1; /* skip one space */
        rate_ptr = ptr;
        while (*rate_ptr == ' ')
            rate_ptr++;
        strncpy(rate, rate_ptr, 4-(rate_ptr-ptr) );

    	ptr += 4; /* skip RATE */
        /* check rate value is valid */
        char rate_val[10];
        strcpy(rate_val, rate);
        rate_val[strlen(rate) - 1] = 0;   /* del 'M' */
        if (atoi(rate_val) >= 300)
                strcpy(rate, "300M");
        else if (atof(rate_val) <= 5)
                strcpy(rate, "5.5M");

        ptr += 1; /* skip one space */
        rssi_ptr = ptr;

        while (*rssi_ptr == ' ')
            rssi_ptr++;
        strncpy(rssi, rssi_ptr, 4-(rssi_ptr-ptr) );

        ptr += 4; /* skip RSSI */
        ptr += 5; /* skip one space + IDLE */
        ptr += 7; /* skip one space + TXSEQ */
        ptr += 7; /* skip one space + RXSEQ */
        ptr += 5; /* skip one space + CAPS */
        ptr += 6; /* skip one space + ACAPS */
        ptr += 4; /* skip one space + ERP */
        ptr += 9; /* skip one space + STATE */

		/*	Date: 2010-02-22
		*	Name: Jimmy Huang
		*	Reason:	Fixed the bug that sometimes wireless sta mode showing in WEB GUI is not correct
		*	Note:	We using HTCAPS to determine the mode
		*			If has any one of G,M,P,S,W, we consider it as n-mode client
		*/
		ptr_htcaps = ptr + 1;
		ptr += 7; /* skip one space + HTCAPS */

        ptr += 1; /* skip one space */
        strncpy(time, ptr, 8);

        ptr += strlen(time);
        ptr += 1; /* skip one space */
        if (strlen(time) == 0)
            strcpy(time, "0");

        nego_rate = ptr;
        while (*nego_rate != ' ')
            nego_rate++;

    	if (nego_rate - ptr == 2) /*NEGO_RATES = 12*/
    	{
			/*	Date: 2010-02-22
			*	Name: Jimmy Huang
			*	Reason:	Fixed the bug that sometimes wireless sta mode showing in WEB GUI is not correct
			*	Note:	We using HTCAPS to determine the mode
			*			If has any one of G,M,P,S,W, we consider it as n-mode client
			*/
			strcpy(mode, "802.11g");
			while (*ptr_htcaps != ' ') {
				switch (ptr_htcaps[0]) {
					case 'G':
					case 'M':
					case 'P':
					case 'S':
					case 'W':
						memset(mode,'\0',sizeof(mode));
						strcpy(mode, "802.11n");
						sta_is_n_mode = 1;
						break;
					default:
						break;
				}

				if (sta_is_n_mode) {
					break;
				}
				ptr_htcaps++;
			}
    	}
    	else if (nego_rate - ptr == 1) /*NEGO_RATES = 4*/
    		strcpy(mode, "802.11b");

        strcpy(ip_addr, "0.0.0.0");
        _system("/bin/cat /proc/net/arp | grep '%s'| cut -d ' ' -f 1  > /tmp/var/arpinfo.txt", addr);

        fp1 = fopen("/tmp/var/arpinfo.txt", "rb");
        if (fp1)
        {
            fgets(ip_addr, 20, fp1);
            fclose(fp1);
            _system("rm -rf /tmp/var/arpinfo.txt");
        }
        else {
            if (dhcpd_status != NULL)
            {
                for(i = 0 ; i < dhcpd_status->num_list; i++)
                {
                    if(strcmp(dhcpd_status->p_st_dhcpd_list[i].mac_addr, addr) == 0)
                    {
                        strcpy(ip_addr, dhcpd_status->p_st_dhcpd_list[i].ip_addr);
                        break;
                    }
                }
            }
        }

        sprintf(wlan_sta_table[wlan_staion_info.num_sta].list, "%s/%d/%s/%s/%s/%s", addr, rssi_to_ratio(rssi), mode, time, rate, ip_addr);   // for GUI
        sprintf(wlan_sta_table[wlan_staion_info.num_sta].info, "%s/%d/%s/%s/%s/%s", addr, rssi_to_ratio(rssi), mode, time, rate, ip_addr);
                wlan_sta_table[wlan_staion_info.num_sta].mac_addr = strtok(wlan_sta_table[wlan_staion_info.num_sta].info, "/");
                wlan_sta_table[wlan_staion_info.num_sta].rssi = strtok(NULL,"/");
                wlan_sta_table[wlan_staion_info.num_sta].mode = strtok(NULL,"/");
                wlan_sta_table[wlan_staion_info.num_sta].connected_time = strtok(NULL,"/");
                wlan_sta_table[wlan_staion_info.num_sta].rate = strtok(NULL,"/");
                wlan_sta_table[wlan_staion_info.num_sta].ip_addr = strtok(NULL,"/");
                wlan_staion_info.num_sta++;
        }

        fclose(fp);
        wlan_staion_info.p_st_wlan_sta = &wlan_sta_table[0];

        return &wlan_staion_info;
}

#ifdef USER_WL_ATH_5GHZ
//5G interface 2
T_WLAN_STAION_INFO* get_5g_stalist()
{
    FILE *fp, *fp1;
    char *ptr, *rssi_ptr, *rate_ptr;
    char buff[1024], addr[20], rssi[4], mode[16], time[16], rate[8], ip_addr[20];
    int i;

    if (nvram_match("wlan1_enable", "0") == 0)
        return NULL;

    wlan_staion_5G_info.num_sta = 0;
    memset(wlan_sta_5G_table, 0, sizeof(wireless_stations_table_t)*M_MAX_STATION);

    if (nvram_match("wlan0_enable", "1") == 0) {
        system("wlanconfig ath1 list sta > /var/misc/stalist5g.txt");
        if (nvram_match("wlan1_vap1_enable", "1") == 0)  { /* Add 5GHz Wireless GuestZone STA List */  
            if (nvram_match("wlan0_vap1_enable", "1") == 0)
                system("wlanconfig ath3 list sta >> /var/misc/stalist5g.txt"); //5GHz GuestZone
            else
                system("wlanconfig ath2 list sta >> /var/misc/stalist5g.txt"); //5GHz GuestZone
        }               
    }       
    else {
        system("wlanconfig ath0 list sta > /var/misc/stalist5g.txt");   
        if (nvram_match("wlan1_vap1_enable", "1") == 0)
            system("wlanconfig ath1 list sta >> /var/misc/stalist5g.txt");//5GHz GuestZone
    }

    fp = fopen("/var/misc/stalist5g.txt","rb");
    if (fp == NULL) {
        DEBUG_MSG("stalist5g file open failed (No station connected). \n");
        return NULL;
    }

    while (fgets(buff, 1024, fp))
    {
        if (strstr(buff, "ID CHAN RATE RSSI IDLE"))
            continue;
        
        ptr = buff;
        if (strstr(buff, "UAPSD QoSInfo")) {
            DEBUG_MSG("get_5g_stalist(): Having VISTA Client QoSInfo.\n");
            continue;
        }

        memset(addr, 0, 20);
        memset(rssi, 0, 4);
        memset(mode, 0, 16);
        memset(time, 0, 16);
        memset(rate, 0, 8);

        strncpy(addr, ptr, 17);
        ptr += strlen(addr);
        ptr += 5; /* skip one space + AID */
        ptr += 5; /* skip one space + CHAN */

        ptr += 1; /* skip one space */
        rate_ptr = ptr;
        while (*rate_ptr == ' ')
            rate_ptr++;
        strncpy(rate, rate_ptr, 4-(rate_ptr-ptr) );

        ptr += 4; /* skip RATE */
        /* check rate value is valid */
        char rate_str[10];
        int  rate_val;
        strcpy(rate_str, rate);
        rate_str[strlen(rate) - 1] = 0;   /* del 'M' */
        rate_val = atoi(rate_str);

        if (rate_val >= 300)
            strcpy(rate, "300M");
        else if (atof(rate_str) <= 5)
            strcpy(rate, "5.5M");
        else if ((rate_val >= 50) && (rate_val <= 53))
            strcpy(rate, "54M");

        ptr += 1; /* skip one space */
        rssi_ptr = ptr;
        while (*rssi_ptr == ' ')
            rssi_ptr++;
        strncpy(rssi, rssi_ptr, 4-(rssi_ptr-ptr) );

        ptr += 4; /* skip RSSI */
        ptr += 5; /* skip one space + IDLE */
        ptr += 7; /* skip one space + TXSEQ */
        ptr += 7; /* skip one space + RXSEQ */
        ptr += 5; /* skip one space + CAPS */
        ptr += 6; /* skip one space + ACAPS */
        ptr += 4; /* skip one space + ERP */
        ptr += 9; /* skip one space + STATE */

		/*	Date: 2010-02-22
		*	Name: Jimmy Huang
		*	Reason:	Fixed the bug that sometimes wireless sta mode showing in WEB GUI is not correct
		*	Note:	We using HTCAPS to determine the mode
		*			If has any one of G,M,P,S,W, we consider it as n-mode client
		*/
		ptr += 7; /* skip one space + HTCAPS */

        ptr += 1; /* skip one space */
        strncpy(time, ptr, 8);

        ptr += strlen(time);
        ptr += 1; /* skip one space */
        if (strlen(time) == 0)
            strcpy(time, "0");

        ptr += 11; /* skip one space + NEGO_RATES */

        if (strcmp(rate, "54M") == 0)
            strcpy(mode, "802.11a");
        else
            strcpy(mode, "802.11n");
        
        strcpy(ip_addr, "0.0.0.0");
        _system("/bin/cat /proc/net/arp | grep '%s'| cut -d ' ' -f 1  > /tmp/var/arpinfo_5g.txt", addr);

        fp1 = fopen("/tmp/var/arpinfo_5g.txt", "rb");
        if (fp1) {
            fgets(ip_addr, 20, fp1);
            fclose(fp1);
            _system("rm -rf /tmp/var/arpinfo_5g.txt");
        }
        else {
            if (dhcpd_status != NULL) {
                for (i = 0 ; i < dhcpd_status->num_list; i++) {
                    if (strcmp(dhcpd_status->p_st_dhcpd_list[i].mac_addr, addr) == 0) {
                        strcpy(ip_addr, dhcpd_status->p_st_dhcpd_list[i].ip_addr);
                        break;
                    }
                }
            }
        }

        sprintf(wlan_sta_5G_table[wlan_staion_5G_info.num_sta].list, "%s/%d/%s/%s/%s/%s", addr, rssi_to_ratio(rssi), mode, time, rate, ip_addr);   // for GUI
        sprintf(wlan_sta_5G_table[wlan_staion_5G_info.num_sta].info, "%s/%d/%s/%s/%s/%s", addr, rssi_to_ratio(rssi), mode, time, rate, ip_addr);
                wlan_sta_5G_table[wlan_staion_5G_info.num_sta].mac_addr = strtok(wlan_sta_5G_table[wlan_staion_5G_info.num_sta].info, "/");
                wlan_sta_5G_table[wlan_staion_5G_info.num_sta].rssi = strtok(NULL,"/");
                wlan_sta_5G_table[wlan_staion_5G_info.num_sta].mode = strtok(NULL,"/");
                wlan_sta_5G_table[wlan_staion_5G_info.num_sta].connected_time = strtok(NULL,"/");
                wlan_sta_5G_table[wlan_staion_5G_info.num_sta].rate = strtok(NULL,"/");
                wlan_sta_5G_table[wlan_staion_5G_info.num_sta].ip_addr = strtok(NULL,"/");
                wlan_staion_5G_info.num_sta++;
    }

    fclose(fp);
    wlan_staion_5G_info.p_st_wlan_sta = &wlan_sta_5G_table[0];

    return &wlan_staion_5G_info;
}
#endif

T_DHCPD_LIST_INFO* get_dhcp_client_list(void)
{
        FILE *fp;
        FILE *fp_pid;
        int i;

        // Jery Lin Added for Fix DHCP Reservations issue 2010/02/04
        char res_rule[] = "dhcpd_reserve_XX";
        char *enable="", *name="", *ip="", *mac="";
        char reserve_ip[MAX_DHCPD_RESERVATION_NUMBER][16];
        char *p_dhcp_reserve;
        char fw_value[192]={0};
        int j;

        if(!nvram_match("wlan0_mode","ap") || !nvram_match("dhcpd_enable","0"))
                return NULL;

        dhcpd_list.num_list = 0;
        memset(leases, 0, sizeof(struct html_leases_s)*M_MAX_DHCP_LIST);

        fp_pid = fopen(UDHCPD_PID,"r");
        if(fp_pid)
        {
        /*write dhcp list into file /var/misc/udhcpd.leases & html.leases */
                kill(read_pid(UDHCPD_PID), SIGUSR1);
                fclose(fp_pid);
        }
        else
        {
                DEBUG_MSG("get_dhcp_client_list /var/run/udhcpd.pid error\n");
                return NULL;
        }

        DEBUG_MSG("get_dhcp_client_list: wait for  open udhcpd.leases\n");
        /*Albert: take off sleep for GUI get information lag */
//      sleep(1);
        usleep(1000);
        fp = fopen("/var/misc/html.leases","r");
        if(fp == NULL)
        {
                DEBUG_MSG("get_dhcp_client_list: open html.leases error\n");
                return NULL;
        }

        // Jery Lin Added for save to reserve_ip array from nvram dhcpd_reserve_XX
        memset(reserve_ip,0,sizeof(reserve_ip));
        for (j=0; j<MAX_DHCPD_RESERVATION_NUMBER; j++){
                snprintf(res_rule, sizeof(res_rule), "dhcpd_reserve_%02d",j);
                p_dhcp_reserve = nvram_safe_get(res_rule);
                if (p_dhcp_reserve == NULL || (strlen(p_dhcp_reserve) == 0))
                        continue;
                memcpy(fw_value, p_dhcp_reserve, strlen(p_dhcp_reserve));
                getStrtok(fw_value, "/", "%s %s %s %s ", &enable, &name, &ip, &mac);
                if (atoi(enable)==0)
                        continue;
                strcpy(reserve_ip[j],ip);
                DEBUG_MSG("get_dhcp_client_list: reserve_ip[%d]=%s\n",j,reserve_ip[j]);
        }


        for( i = 0; i < M_MAX_DHCP_LIST; i++)
        {
                fread(&leases[i], sizeof(struct html_leases_s), 1, fp);
                if(leases[i].ip_addr[0] != 0x00)
                {
                        dhcpd_list.num_list++;
                        client_list[i].hostname = leases[i].hostname;
                        client_list[i].ip_addr = leases[i].ip_addr;
                        client_list[i].mac_addr = leases[i].mac_addr;

                        // Jery Lin Added for check client_list[i] whether is in reserve_ip array
                        for (j=0; j<MAX_DHCPD_RESERVATION_NUMBER; j++)
                                if (strcmp(reserve_ip[j], client_list[i].ip_addr)==0)
                                        break;

                        client_list[i].expired_at = (j<MAX_DHCPD_RESERVATION_NUMBER) ?
                                                    "Never":
                                                    leases[i].expires;

                        DEBUG_MSG("%s",leases[i].hostname);
                        DEBUG_MSG("%s",client_list[i].ip_addr);
                        DEBUG_MSG("%s",leases[i].mac_addr);
                        DEBUG_MSG("%s",client_list[i].expired_at);
                        DEBUG_MSG("\n");
                }
                else
                        break;
        }
        DEBUG_MSG("dhcpd_list.num_list = %d \n",dhcpd_list.num_list);
        fclose(fp);
        dhcpd_list.p_st_dhcpd_list = &client_list[0];
        return &dhcpd_list;
}

T_IGMP_LIST_INFO* get_igmp_list(void)
{
        FILE *fp;
        FILE *fp_pid;
        int i;

        igmp_list.num_list = 0;
        memset(igmp_member, 0, sizeof(struct igmp_list_s)*M_MAX_IGMP_LIST);


        unlink(IGMP_GROUP_FILE);

        fp_pid = fopen(IGMPPROXY_PID,"r");
        if(fp_pid)
        {
                /*write igmp member into file*/
                kill(read_pid(IGMPPROXY_PID), SIGUSR1);
                fclose(fp_pid);
        }
        else
        {
                DEBUG_MSG("get_igmp_list %s error\n",IGMPPROXY_PID);
                return NULL;
        }
        sleep(1);

        fp = fopen(IGMP_GROUP_FILE,"r");
        if(fp == NULL)
        {
                DEBUG_MSG("get_igmp_list: %s error\n",IGMP_GROUP_FILE);
                return NULL;
        }

        for( i = 0; i < M_MAX_IGMP_LIST; i++)
        {
                fread(&igmp_member[i], sizeof(struct igmp_list_s), 1, fp);
                if(strlen(igmp_member[i].member))
                {
                        igmp_list.num_list++;
                        igmp_return_member[i].member = igmp_member[i].member;
                }
                else
                        break;
        }
        fclose(fp);
        igmp_list.p_igmp_list = &igmp_return_member[0];
        return &igmp_list;
}

unsigned char *get_ping_app_stat(unsigned char *host)
{
        FILE *fp;
        char *ping_result;
        unsigned char returnVal[256] = {}; //15

        //memset(returnVal, 0, sizeof(returnVal));
        parse_special_char(host);
        _system("ping -c 1 \"%s\" > /var/misc/ping_app.txt", host);
        fp = fopen("/var/misc/ping_app.txt", "r");
        if(fp == NULL)
        {
                DEBUG_MSG("open /var/misc/ping_app.txt error\n");
                return NULL;
        }
        fread(returnVal, sizeof(returnVal), 1, fp);
        DEBUG_MSG("%s", returnVal);

        if(strlen(returnVal) > 1)
        {
                if(strstr(returnVal, "transmitted")){//transmitted
                if(strstr(returnVal, "100%")) //100% packet loss
                        ping_result = "Fail";
                else
                        ping_result = "Success";
            }
            else
                    ping_result = "Unknown Host";
        }
        else
                ping_result = "Unknown Host";

        fclose(fp);
        return ping_result;
}

unsigned char *get_ping_app_display(unsigned char *host)
{
        FILE *fp;
        char ping_result[1024] = {0};
        unsigned char returnVal[1024] = {}; //15
        int len = 0;
        char *p_result;
        int fd;

        parse_special_char(host);
        _system("ping -c 5 \"%s\" > /var/misc/ping_display.txt", host);

        fp = fopen("/var/misc/ping_display.txt", "r");

        if(fp == NULL)
        {
                DEBUG_MSG("open /var/misc/ping_display.txt error\n");
                return NULL;
        }
        p_result = ping_result;

        while( fgets(returnVal, 1024, fp)) {
                len = sprintf(p_result, "%s<br>", returnVal);
                p_result += len;
        }


        fclose(fp);

        return ping_result;

}


#ifdef RADVD
unsigned char *get_ping6_app_stat(unsigned char *host,char *interface,int size,int count)
{
        FILE *fp;
        char *ping_result;
        unsigned char returnVal[256] = {}; //15

        //memset(returnVal, 0, sizeof(returnVal));
        parse_special_char(host);
        _system("ping6 -c %d -s %d -I %s \"%s\" > /var/misc/ping_app.txt", count, size, interface, host);
        fp = fopen("/var/misc/ping_app.txt", "r");
        if(fp == NULL)
        {
                DEBUG_MSG("open /var/misc/ping_app.txt error\n");
                return NULL;
        }
        fread(returnVal, sizeof(returnVal), 1, fp);
        DEBUG_MSG("%s", returnVal);

        if(strlen(returnVal) > 1)
        {
        if(strstr(returnVal, "100%")) //100% packet loss
                ping_result = "Fail";
        else
                ping_result = "Success";
        }
        else
                ping_result = "Unknown Host";

        fclose(fp);
        return ping_result;
}

unsigned char *get_ping6_app_test(unsigned char *host)
{
        FILE *fp;
        char *ping_result;
        unsigned char returnVal[256] = {}; //15

        //memset(returnVal, 0, sizeof(returnVal));
        parse_special_char(host);
        _system("ping6 -c 1 \"%s\" > /var/misc/ping_app.txt", host);
        fp = fopen("/var/misc/ping_app.txt", "r");
        if(fp == NULL)
        {
                DEBUG_MSG("open /var/misc/ping_app.txt error\n");
                return NULL;
        }
        fread(returnVal, sizeof(returnVal), 1, fp);
        DEBUG_MSG("%s", returnVal);

        if(strlen(returnVal) > 1)
        {
        if(strstr(returnVal, "100%")) //100% packet loss
                ping_result = "Fail";
        else
                ping_result = "Success";
        }
        else
                ping_result = "Unknown Host";

        fclose(fp);
        return ping_result;
}

#endif

char *proc_gen_fmt(char *name, int more, FILE * fh,...)
{
        char buf[512], format[512] = "";
        char *title, *head, *hdr;
        va_list ap;

        if (!fgets(buf, (sizeof buf) - 1, fh))
                return NULL;
        strcat(buf, " ");

        va_start(ap, fh);
        title = va_arg(ap, char *);
        for (hdr = buf; hdr;) {
                while (isspace(*hdr) || *hdr == '|')
                        hdr++;
                head = hdr;
                hdr = strpbrk(hdr, "| \t\n");
                if (hdr)
                        *hdr++ = 0;

                if (!strcmp(title, head)) {
                        strcat(format, va_arg(ap, char *));
                        title = va_arg(ap, char *);
                        if (!title || !head)
                                break;
                } else {
                        strcat(format, "%*s");  /* XXX */
                }
                strcat(format, " ");
        }
        va_end(ap);

        if (!more && title) {
                fprintf(stderr, "warning: %s does not contain required field %s\n",
                                name, title);
                return NULL;
        }
        return strdup(format);
}

#ifdef MPPPOE
char *return_ppp_gateway(void)
{
        FILE *fp;
        char buf[128] = {};
        fp = fopen(PPP_GATEWAY_IP,"r");

        if(fp)
        {
                if(fgets(buf,128,fp))
                {
                        fclose(fp);
                        return buf;
                }
                else
                {
                        fclose(fp);
                        return "0.0.0.0";
                }
        }
        else
                return "0.0.0.0";
}
#endif

char *read_gatewayaddr(char *if_name)
{
        FILE *fp;
        char buff[1024], iface[16];
        char gate_addr[128], net_addr[128];
        char mask_addr[128], *fmt;
        int num, iflags, metric, refcnt, use, mss, window, irtt;
        struct in_addr gateway, dest, route, mask;
        int success=0, count =0;

        int i = 0, under_static_routing_area = 0;
        char *ptr = NULL,*ptr_tmp = NULL;
        char useless[16];
        int already_found_in_nvramORdhcp = 0;
        char lan_bridge[5];

        typedef struct static_routing_table{
        char dest_addr[16];
        char dest_mask[16];
        char gateway[16];
        char interface[8];
        char metric[3];
        }static_routing_table;

        static_routing_table *static_routing_lists_nvram[MAX_STATIC_ROUTING_NUMBER];//shvra.h
#define NVRAM_FILE "/var/etc/nvram.conf"


#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
        // definition need to be identical with the file
        // Eagle/apps/udhcp/dhcpc.c OptionMicroSoftClasslessStaticRoute()
#define MAX_STATIC_ROUTE_NUM_DHCP 25
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
#define MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL "/tmp/ms_classes_static_route_add.sh"
#define MS_CLASSLESS_STATIC_ROUTE_DEL_SHELL "/tmp/ms_classes_static_route_del.sh"
#endif

#ifdef UDHCPC_RFC_CLASSLESS_STATIC_ROUTE
#define RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL "/tmp/classes_static_route_add.sh"
#define RFC_CLASSLESS_STATIC_STATIC_ROUTE_DEL_SHELL "/tmp/classes_static_route_del.sh"
#endif

static_routing_table *static_routing_lists_dhcp[MAX_STATIC_ROUTE_NUM_DHCP];

#endif

#ifdef MPPPOE
        if(!nvram_match("wan_proto","pppoe"))
        {
                if(read_ipaddr_no_mask(if_name))
                {
                        _system("ifconfig %s",if_name);
                        return  return_ppp_gateway();
                }
                else
                        return "0.0.0.0";
        }
#endif

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

        //read nvram routing info
        DEBUG_MSG("%s, reading nvram info from %s\n",__FUNCTION__,NVRAM_FILE);
        memset(lan_bridge,'\0',sizeof(lan_bridge));
        for(i = 0;i < MAX_STATIC_ROUTING_NUMBER; i++)
                static_routing_lists_nvram[i]=NULL;

        if((fp = fopen(NVRAM_FILE,"r"))!=NULL){
                i = 0;
                while(!feof(fp) && i < MAX_STATIC_ROUTING_NUMBER){
                        memset(buff,'\0',sizeof(buff));
                        fgets(buff,sizeof(buff),fp);
                        if(buff[0] != '\n' && buff[0] != '\0' && buff[0] != '#'){       //ignore #!/bin.sh
                                if( strncmp(buff,"static_routing_",strlen("static_routing_")) == 0 && isdigit(buff[strlen("static_routing_")])){
                                        DEBUG_MSG("%s, %s",__FUNCTION__,buff);
                                        under_static_routing_area = 1;
                                        //static_routing_xx     enable/name/dest_addr/dest_mask/gw/interface/metric
                                        //static_routing_05=1/test_2/192.168.220.0/255.255.0.0/192.168.100.220/WAN/1
                                        ptr = strchr(buff,'/');
                                        if(ptr && (*(ptr-1) == '0')){//disable
                                                goto next_round;
                                        }
                                        DEBUG_MSG("%s, parsing %s",__FUNCTION__,buff);
                                        static_routing_lists_nvram[i] = malloc(sizeof(static_routing_table));
                                        /*      Date: 2009-02-05
                                        *       Name: jimmy huang
                                        *       Reason: in case memory alloc failed
                                        *       Note:   Add new codes below
                                        */
                                        if(static_routing_lists_nvram[i] == NULL){
                                                printf("%s , line %d, memory alloc failed !\n",__FUNCTION__,__LINE__);
                                                break;
                                        }
                                        if(NULL == (ptr_tmp = strtok(ptr,"/")))
                                                goto next_round;
                                        if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
                                                goto next_round;
                                        strcpy(static_routing_lists_nvram[i]->dest_addr,ptr_tmp);
                                        if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
                                                goto next_round;
                                        strcpy(static_routing_lists_nvram[i]->dest_mask,ptr_tmp);
                                        if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
                                                goto next_round;
                                        strcpy(static_routing_lists_nvram[i]->gateway,ptr_tmp);
                                        if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
                                                goto next_round;
                                        strcpy(static_routing_lists_nvram[i]->interface,ptr_tmp);
                                        if(NULL ==  (ptr_tmp = strtok(NULL,"/")))
                                                goto next_round;
                                        strcpy(static_routing_lists_nvram[i]->metric,ptr_tmp);
                                        DEBUG_MSG("%s, read from nvram : dest_addr = %s, dest_mask = %s, gw = %s, interface = %s, metric = %s\n" \
                                                ,__FUNCTION__ \
                                                ,static_routing_lists_nvram[i]->dest_addr \
                                                ,static_routing_lists_nvram[i]->dest_mask \
                                                ,static_routing_lists_nvram[i]->gateway \
                                                ,static_routing_lists_nvram[i]->interface \
                                                ,static_routing_lists_nvram[i]->metric \
                                        );
next_round:
                                        if(static_routing_lists_nvram[i] != NULL)
                                                i++;
                                }
                                else
                                {
                                        if(strncmp(buff,"lan_bridge",strlen("lan_bridge")) == 0){
                                                ptr=strchr(buff,'=');
                                                if(ptr){
                                                        ptr++;
                                                        strcpy(lan_bridge,ptr);
                                                        if(lan_bridge[strlen(lan_bridge)-1] == '\n'){
                                                                lan_bridge[strlen(lan_bridge)-1]='\0';
                                                        }
                                                }
                                        }
                                        if(under_static_routing_area)
                                                break;
                                }
                        }
                }
                fclose(fp);
        }
        //read dhcp routing info
#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
        for(i = 0;i < MAX_STATIC_ROUTE_NUM_DHCP; i++)
                static_routing_lists_dhcp[i]=NULL;
        i = 0;
#ifdef UDHCPC_MS_CLASSLESS_STATIC_ROUTE
        if((fp = fopen(MS_CLASSLESS_STATIC_ROUTE_ADD_SHELL,"r")) != NULL){
#else
        if((fp = fopen(RFC_CLASSLESS_STATIC_STATIC_ROUTE_ADD_SHELL,"r")) != NULL){
#endif
                while(!feof(fp)){
                        memset(buff,'\0',sizeof(buff));
                        fgets(buff,sizeof(buff),fp);
                        if(buff[0] != '\n' && buff[0] != '\0' && buff[0] != '#'){       //ignore #!/bin.sh
                                static_routing_lists_dhcp[i] = malloc(sizeof(static_routing_table));
                                if(static_routing_lists_dhcp[i] == NULL){
                                        printf("%s memory alloc failed !\n",__FUNCTION__);
                                        break;
                                }
                                        //route add -net 192.168.100.100 netmask 255.255.255.255 gw 192.168.0.10 metric 1
                                sscanf(buff,"%s %s %s %s %s %s %s %s %s %s"
                                                ,useless,useless,useless,static_routing_lists_dhcp[i]->dest_addr
                                                ,useless,static_routing_lists_dhcp[i]->dest_mask,useless
                                                ,static_routing_lists_dhcp[i]->gateway,useless
                                                ,static_routing_lists_dhcp[i]->metric);
                                DEBUG_MSG("%s, read from dhcp : dest_addr = %s, dest_mask = %s, gw = %s, metric = %s\n" \
                                                ,__FUNCTION__,static_routing_lists_dhcp[i]->dest_addr \
                                                ,static_routing_lists_dhcp[i]->dest_mask \
                                                ,static_routing_lists_dhcp[i]->gateway \
                                                ,static_routing_lists_dhcp[i]->metric);
                                i++;
                        }
                }
                fclose(fp);
        }else{
                DEBUG_MSG("%s, dhcp option 121/249 shell file not exists\n",__FUNCTION__);
        }
#endif

        fp = fopen (PATH_PROCNET_ROUTE, "r");
        if(!fp) {
                perror(PATH_PROCNET_ROUTE);
                return 0;
        }

        fmt = proc_gen_fmt(PATH_PROCNET_ROUTE, 0, fp,
                        "Iface", "%16s",
                        "Destination", "%128s",
                        "Gateway", "%128s",
                        "Flags", "%X",
                        "RefCnt", "%d",
                        "Use", "%d",
                        "Metric", "%d",
                        "Mask", "%128s",
                        "MTU", "%d",
                        "Window", "%d",
                        "IRTT", "%d",
                        NULL);
        /* "%16s %128s %128s %X %d %d %d %128s %d %d %d\n" */

        if (!fmt)
                return 0;

        /* jimmy marked 20080505 */
        //printf("sizeof routing_table: %d\n", sizeof(&routing_table));
        /* --------------------- */
        for(count =0;  strlen(routing_table[count].dest_addr) >0; count++ ) {
                strcpy(routing_table[count].dest_addr, "") ;
        }
        count =0;

        while( fgets(buff, 1024, fp)) {
                num = sscanf(buff, fmt, iface, net_addr, gate_addr, &iflags, &refcnt, &use, &metric, mask_addr, &mss, &window, &irtt);

                if(iflags & RTF_UP) {
                        dest.s_addr = strtoul(net_addr, NULL, 16);
                        route.s_addr = strtoul(gate_addr, NULL, 16);
                        mask.s_addr = strtoul(mask_addr, NULL, 16);

                        /* for route_table */
                        strcpy(routing_table[count].dest_addr, inet_ntoa(dest));
                        strcpy(routing_table[count].dest_mask, inet_ntoa(mask));
                        strcpy(routing_table[count].gateway, inet_ntoa(route));
                        strcpy(routing_table[count].interface, iface);
                        routing_table[count].metric = metric;
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
                        *               3:INTRANET      :       The table is used for Intranet
                        *               4:LOCAL         :       Local loop back
                        *
                        *       Creator:
                        *               0:System:               Learned automatically
                        *               1:ADMIN:                Learned via user's operation in Web GUI
                        */
                        if(strncmp(routing_table[count].interface,lan_bridge,strlen(lan_bridge)) == 0)//br0
                                routing_table[count].type = 3;
                        else if(strcmp(routing_table[count].interface,"lo") == 0)
                                routing_table[count].type = 4;
                        else
                                routing_table[count].type = 0;
                        routing_table[count].creator = 0;
                        already_found_in_nvramORdhcp = 0;
                        //DEBUG_MSG("%s, reading in dest_addr = %s, dest_mask = %s, gw = %s, interface = %s, type = %d(%s), creator = %d(%s), metric = %d\n"
                        //      ,__FUNCTION__
                        //      ,routing_table[count].dest_addr
                        //      ,routing_table[count].dest_mask
                        //      ,routing_table[count].gateway
                        //      ,routing_table[count].interface
                        //      ,routing_table[count].type
                        //      ,(routing_table[count].type == 0) ? "Internet" : ((routing_table[count].type == 1) ? "DHCP Option" : "STATIC")
                        //      ,routing_table[count].creator
                        //      ,routing_table[count].creator ? "ADMIN" : "System"
                        //      ,routing_table[count].metric
                        //      );

                        // check if this route is added via rfc 3442 (option 121/249)
#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
                        if((already_found_in_nvramORdhcp != 1) && (static_routing_lists_dhcp[0] != NULL)){
                                /*
                                for(i=0; static_routing_lists_dhcp[i] != NULL && strlen(static_routing_lists_dhcp[i]->dest_addr) > 0; i++ ){
                                */
                                /*      Date: 2009-02-04
                                *       Name: jimmy huang
                                *       Reason: original condition rule maybe cause unstable memory access
                                *                       and will let httpd crash
                                *       Note:   Modified the codes above to new belows
                                */
                                for(i=0; i < MAX_STATIC_ROUTING_NUMBER && static_routing_lists_dhcp[i] != NULL && strlen(static_routing_lists_dhcp[i]->dest_addr) > 0; i++ ){
                                        if((strcmp(routing_table[count].dest_addr,static_routing_lists_dhcp[i]->dest_addr) == 0)
                                                && (strcmp(routing_table[count].dest_mask,static_routing_lists_dhcp[i]->dest_mask) == 0)
                                                && (strcmp(routing_table[count].gateway,static_routing_lists_dhcp[i]->gateway) == 0))
                                        {
                                                routing_table[count].type = 1;//DHCP Option
                                                routing_table[count].creator = 0; //System
                                                already_found_in_nvramORdhcp = 1;
                                                break;
                                        }
                                }
                        }
#endif

                        // check if this route is added by user via Web function "static route"
                        if((already_found_in_nvramORdhcp != 1) && (static_routing_lists_nvram[0] != NULL)){
                                /*
                                for(i=0; static_routing_lists_nvram[i] != NULL && strlen(static_routing_lists_nvram[i]->dest_addr) > 0; i++ ){
                                */
                                /*      Date: 2009-02-04
                                *       Name: jimmy huang
                                *       Reason: original condition rule maybe cause unstable memory access
                                *                       and will let httpd crash
                                *       Note:   Modified the codes above to new belows
                                */
                                for(i=0; i < MAX_STATIC_ROUTING_NUMBER && static_routing_lists_nvram[i] != NULL && strlen(static_routing_lists_nvram[i]->dest_addr) > 0; i++ ){
                                        if((strcmp(routing_table[count].dest_addr,static_routing_lists_nvram[i]->dest_addr) == 0)
                                                && (strcmp(routing_table[count].dest_mask,static_routing_lists_nvram[i]->dest_mask) == 0)
                                                && (strcmp(routing_table[count].gateway,static_routing_lists_nvram[i]->gateway) == 0)
                                                )
                                        {
                                                routing_table[count].type = 2;//STATIC
                                                routing_table[count].creator = 1;//Admin
                                                already_found_in_nvramORdhcp = 1;
                                                break;
                                        }
                                }
                        }

                        DEBUG_MSG("%s, table[%d] dest_addr = %s, dest_mask = %s, gw = %s, interface = %s, type = %d (%s), creator = %d (%s), metric = %d\n"
                                ,__FUNCTION__,count
                                ,routing_table[count].dest_addr
                                ,routing_table[count].dest_mask
                                ,routing_table[count].gateway
                                ,routing_table[count].interface
                                ,routing_table[count].type
                                ,(routing_table[count].type == 0) ? "Internet" : ((routing_table[count].type == 1) ? "DHCP Option" : ((routing_table[count].type == 2) ? "STATIC" : "INTRANET"))
                                ,routing_table[count].creator
                                ,routing_table[count].creator ? "ADMIN" : "System"
                                ,routing_table[count].metric
                                );


                        count +=1;
                        /* end */

                        if ((iflags & RTF_GATEWAY) && !strcmp("0.0.0.0", inet_ntoa(dest)) && !strcmp(iface, if_name) ) {
                                gateway.s_addr = strtoul(gate_addr,NULL,16);
                                success =1;
                        }
                }
        }

        free(fmt);
        fclose(fp);

        for(i = 0;i < MAX_STATIC_ROUTING_NUMBER; i++){
                if(static_routing_lists_nvram[i] != NULL)
                        free(static_routing_lists_nvram[i]);
        }
#if defined(CLASSLESS_STATIC_ROUTE_SHOW_UI) && (defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE))
        for(i = 0;i < MAX_STATIC_ROUTE_NUM_DHCP; i++){
                if(static_routing_lists_dhcp[i] != NULL)
                        free(static_routing_lists_dhcp[i]);
        }
#endif

        if(success)
                return inet_ntoa(gateway);
        else
                return "0.0.0.0";
}

routing_table_t *read_route_table(void)
{
        read_gatewayaddr(nvram_safe_get("wan_eth"));

        return &routing_table;
}

#ifdef IPv6_SUPPORT
char *read_ipv6defaultgateway(const char *if_name, char *ipv6addr){
        //current support static and 6in4  only
        char nvram_temp[30]={0};
        if(!ipv6addr)
                return NULL;
        strcpy(nvram_temp,nvram_safe_get("ipv6_wan_proto"));
        if(strncmp(nvram_temp,"ipv6_static",11)==0){
                strcpy(ipv6addr,nvram_safe_get("ipv6_static_default_gw"));
        }else if(strncmp(nvram_temp,"ipv6_6in4",9)==0){
                strcpy(ipv6addr,nvram_safe_get("ipv6_6in4_remote_ip"));
	}else {
		char buf[200], *pos;
		FILE *pp;

		sprintf(buf,"ip -6 route show match ::/0 dev %s", if_name);
		if( (pp = popen(buf,"r")) != NULL )	{
			fgets(buf, sizeof buf, pp);
			pclose(pp);

			if (strstr(buf, "default via")==buf) { // if start with "default via"
				pos = strstr(buf+12, " "); // find next " "
				strncpy(ipv6addr, buf+12, pos-buf-12);
			}
		} else {
			DEBUG_MSG("popen() error!\n");
			return NULL;
		}
        }

        DEBUG_MSG("ipv6 gateway:%s\n",ipv6addr);
        return ipv6addr;
}
char *read_ipv6addr(const char *if_name,const int scope, char *ipv6addr,const int length){
        FILE *fp;
        char buf[1024]={};
        char *start=NULL,*end=NULL;
	char cmd[50] = {};

        if (!ipv6addr)
                return NULL;

        sprintf(cmd, "ifconfig %s 2>&-", if_name);
        int num_flag=0;
	
        fp = popen(cmd,"r");
        if(!fp)
        return NULL;

        while (start = fgets(buf, 1024, fp)) {
                if (scope == SCOPE_GLOBAL)
                        end = strstr(buf, "Scope:Global");
                else if (scope == SCOPE_LOCAL)
                        end = strstr(buf, "Scope:Link");

                if (end) {
                        end = end - 1;
                        if (start = strstr(start,"addr: ")) {
                                DEBUG_MSG("strlen(ipv6addr):%d,(end-start) %d, length:%d\n",strlen(ipv6addr),end-start,length);
				start = start + 5;
                                //handle string overflow
                                if (strlen(ipv6addr)+(end - start) >= length) {
                                        DEBUG_MSG("Overflow:Extend the MAX_IPV6_IP_LEN to hold more ip\n");
                                        break;
                                }
                                if (num_flag == 0) {
                                        strncpy(ipv6addr,start,end - start);
                                        num_flag=1;
				}else{
                                        strcat(ipv6addr,",");
                                        strncat(ipv6addr,start,end - start);
                                }
                        }
                }
                DEBUG_MSG("size:%i\n",strlen((char *)ipv6addr));
        }
	pclose(fp);

	if (num_flag)
        return ipv6addr;
	else
		return NULL;
}
#endif
char *read_ipaddr(char *if_name)
{
        int sockfd;
        struct ifreq ifr;
        struct in_addr in_addr, netmask;
        int valid=1;
        static char ipaddr[64]={};

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return 0;

        strcpy(ifr.ifr_name, if_name);
        if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
                valid = 0;

        in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
        sprintf(ipaddr, "%s", inet_ntoa(in_addr));

        if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0)
                valid = 0;

        netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
        sprintf(ipaddr, "%s/%s", ipaddr, inet_ntoa(netmask));

        close(sockfd);

        if(valid == 0)
                return NULL;
        else
                return ipaddr;
}

char *read_ipaddr_no_mask(char *if_name)
{
        int sockfd;
        struct ifreq ifr;
        struct in_addr in_addr;
        int valid=1;
        static char ipaddr[64]={};

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return 0;

        strcpy(ifr.ifr_name, if_name);
        if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
                valid = 0;

        in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
        sprintf(ipaddr, "%s", inet_ntoa(in_addr));

        close(sockfd);

        if(valid == 0)
                return NULL;
        else
                return ipaddr;
}

char *read_dns(int ifnum)
{
        FILE *fp;
        char *buff;
#ifdef MPPPOE
        char *resolve_file;
#endif
#ifndef IPv6_SUPPORT
        char dns[3][20] = {};
        static char return_dns[64] ="";
#else
        char dns[3][MAX_IPV6_IP_LEN]={};
        static char return_dns[3*MAX_IPV6_IP_LEN] ="";
#endif
        int num, i=0, len=0;

#ifndef MPPPOE
#ifdef IPv6_SUPPORT
        if(ifnum == 16)
                fp = fopen (RESOLVFILE_IPV6, "r");
        else
#endif
        fp = fopen (RESOLVCONF, "r");
        if(!fp) {
#ifdef IPv6_SUPPORT
                if(ifnum == 16)
                        perror(RESOLVFILE_IPV6);
                else
#endif
                perror(RESOLVCONF);
                return 0;
        }
#else
        if(ifnum == 0)
                resolve_file = DNS_FILE_00;
    else if(ifnum == 1)
        resolve_file = DNS_FILE_01;
    else if(ifnum == 2)
        resolve_file = DNS_FILE_02;
    else if(ifnum == 3)
        resolve_file = DNS_FILE_03;
    else if(ifnum == 4)
        resolve_file = DNS_FILE_04;
#ifdef IPv6_SUPPORT
   else if(ifnum == 16)
        resolve_file = RESOLVFILE_IPV6;
#endif
    else
                resolve_file = RESOLVCONF;

        fp = fopen (resolve_file, "r");
    if(!fp)
    {
                if(ifnum ==5)
                        sprintf(return_dns, "%s/%s/%s", "0.0.0.0", "0.0.0.0","0.0.0.0");
#ifdef IPv6_SUPPORT
                else if(ifnum ==16)
                        sprintf(return_dns, "%s/%s", "", "");
#endif
                else
                        sprintf(return_dns, "%s/%s", "0.0.0.0", "0.0.0.0");

        return return_dns;
        }
#endif

        buff = (char *) malloc(1024);
        if (!buff)
        {       fclose(fp);
                return;
        }

        memset(buff, 0, 1024);

#ifndef MPPPOE
        while( fgets(buff, 1024, fp))
        {
                if (strstr(buff, "nameserver") )
                {
                        num = strspn(buff+10, " ");
                        len = strlen( (buff + 10 + num) );

                        if(strchr(buff, '\n'))/*Line Feed*/
                                strncpy(dns[i], (buff+10+num), len-1);
                        else
                                strncpy(dns[i], (buff+10+num), len);

                        DEBUG_MSG("strlen(buff)=%d, len=%d, dns[%d]=%s\n",
                                strlen(buff), len, i, dns[i]);
                        /* jimmy marked 20080505 */
                        //printf("strlen(buff)=%d, len=%d, dns[%d]=%s\n",
                        //      strlen(buff), len, i, dns[i]);
                        /* --------------------- */

                        i++;
                }
                if(3<=i)
                {
                        strcat(dns[2], "\0");
                        break;
                }
        }
        free(buff);
        sprintf(return_dns, "%s/%s", dns[0], dns[1]);
#else
        if(ifnum == 5){
                while( fgets(buff, 1024, fp))
                {
                        if (strstr(buff, "nameserver") )
                        {
                                num = strspn(buff+10, " ");
                                len = strlen( (buff + 10 + num) );
                                strncpy(dns[i], (buff + 10 + num), len-1);  // for \n case
                                i++;
                        }
                        if(3<=i)
                        {
                                strcat(dns[2], "\0");
                                break;
                        }
                }
                free(buff);
                sprintf(return_dns, "%s/%s/%s", dns[0], dns[1], dns[2]);
        }else{
                while( fgets(buff, 1024, fp))
                {
                        if (strstr(buff, "nameserver") )
                        {
                                num = strspn(buff+10, " ");
                                len = strlen( (buff + 10 + num) );
                                strncpy(dns[i], (buff + 10 + num), len-1);  // for \n case
                                i++;
                        }
                        if(2<=i)
                        {
                                strcat(dns[1], "\0");
                                break;
                        }
                }
                free(buff);
                sprintf(return_dns, "%s/%s", dns[0], dns[1]);
        }
#endif
        fclose(fp);
        return return_dns;
}

/* NickChou, 071217, add for set static routing in RUSSIA Env.
 * return WANPHY_IP/WANPHY_SubnetMask/WANPHY_Gateway/WANPHY_DNS
 */
char *read_current_wanphy_ipaddr()
{
        static char wan_current_ipaddr[128]={};
        char eth[5];
        char status[15];
        char *eth_ipaddr=NULL;
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        int usb_type = 0;
	char cmd[128]={'\0'};
        struct stat usbVendorID;
        usb_type = atoi(nvram_safe_get("usb_type"));
#endif
        strcpy(eth, nvram_safe_get("wan_eth"));

        /* Check phy connect statue */
/*      Date:   2010-01-22
*       Name:   jimmy huang
*       Reason: for Windows Mobile 5, We do not use VCTGetPortConnectState
*                       to get wan port conn state
*/
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
/*      Date:   2010-02-05
 *      Name:   Cosmo Chang
 *      Reason: add usb_type=5 for Android Phone RNDIS feature
 */
        if((usb_type == 3)  || (usb_type == 4) || (usb_type == 5)){
                sprintf(cmd,"[ -n \"`cat %s`\" ] && echo 1 > /var/tmp/usb_test.tmp",USB_DEVICE_VendorID);
                system(cmd);
                /* Check USB Port connection */
                if(stat("/var/tmp/usb_test.tmp", &usbVendorID) != 0){
                        return "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0";
                }
                unlink("/var/tmp/usb_test.tmp");
        }else
#endif
        VCTGetPortConnectState( eth, VCTWANPORT0, status);
        if( !strncmp("disconnect", status, 10) )
                return "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0";

        eth_ipaddr=read_ipaddr(eth);

        /* if wan eth ipaddr in not setting or ipaddr equal 10.64.64.64 (pppd demand ipaddr) */
        if ( eth_ipaddr == NULL )
                sprintf(wan_current_ipaddr, "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0");
        else if(!strncmp(eth_ipaddr, "10.64.64.64", 11))
                sprintf(wan_current_ipaddr, "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0");
        else
                sprintf(wan_current_ipaddr, "%s/%s/%s", eth_ipaddr, read_gatewayaddr(eth), read_dns(5));

        return wan_current_ipaddr;
}

char *read_lan_ip_info() //for ap mode with udhcpc in status page
{
        char *eth_ipaddr;
        char *gateway;
        char result[128] = {};

        if(!nvram_match("dhcpc_enable","1"))
        {
                eth_ipaddr = read_ipaddr(nvram_safe_get("lan_bridge"));

                if(eth_ipaddr == NULL)
                        return "0.0.0.0/0.0.0.0/0.0.0.0";
                else if(!strncmp(eth_ipaddr, "10.64.64.64", 11) || strlen(eth_ipaddr)==0)
                        return "0.0.0.0/0.0.0.0/0.0.0.0";

                gateway = read_gatewayaddr(nvram_safe_get("lan_bridge"));
                sprintf(result,"%s/%s",eth_ipaddr,gateway);
        }
        else
                sprintf(result,"%s/%s/%s",nvram_safe_get("lan_ipaddr"),nvram_safe_get("lan_netmask"),nvram_safe_get("lan_gateway"));
        return result;
}

char *read_current_ipaddr(int ifnum)
{
        static char wan_current_ipaddr[128]={0};
        char eth[5] = {0};
        char status[15] = {0};
        char *wan_proto=NULL;
        char *eth_ipaddr=NULL;
#if defined(CONFIG_USER_3G_USB_CLIENT) || defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        int usb_type = 0;
	char cmd[128]={'\0'};
        struct stat usbVendorID;
#endif

#ifdef  CONFIG_BRIDGE_MODE
        char *wlan0_mode=NULL;

        wlan0_mode = nvram_safe_get("wlan0_mode");
#endif

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        usb_type = atoi(nvram_safe_get("usb_type"));
#endif

        wan_proto = nvram_safe_get("wan_proto");

#ifdef LAN_SUPPORT_DHCPC
        char *dhcpc_enable=NULL;
  dhcpc_enable = nvram_safe_get("dhcpc_enable");
        if(strcmp(dhcpc_enable,"1")==0)
#else
        if( strcmp(wan_proto, "dhcpc") ==0 || strcmp(wan_proto, "static") ==0)
#endif
    {
#ifdef  CONFIG_BRIDGE_MODE
                if(strcmp(wlan0_mode, "ap") == 0)
                        strcpy(eth, nvram_safe_get("lan_bridge"));
                else
#endif
                {
                ifnum= 5;               // for read_dns() to differentiate wan_proto. (if wan_proto==dhcpc | static, it has 3 dns servers)
                        strcpy(eth, nvram_safe_get("wan_eth"));
                }
        }
        else
                sprintf(eth, "ppp%d", ifnum);

#ifdef CONFIG_USER_3G_USB_CLIENT
        if( strcmp(wan_proto, "usb3g") == 0)
        {
                /* Check USB Port connection */
                if(stat(PPP_3G_VID_PATH, &usbVendorID) != 0)
                        return "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0";
        }
/*      Date:   2010-01-22
*       Name:   jimmy huang
*       Reason: for Windows Mobile 5, We do not use VCTGetPortConnectState
*                       to get wan port conn state
*/
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
/*      Date:   2010-02-05
 *      Name:   Cosmo Chang
 *      Reason: add usb_type=5 for Android Phone RNDIS feature
 */
        else if((usb_type == 3) || (usb_type == 4) || (usb_type == 5)){
                sprintf(cmd,"[ -n \"`cat %s`\" ] && echo 1 > /var/tmp/usb_test.tmp",USB_DEVICE_VendorID);
                system(cmd);
                /* Check USB Port connection */
                if(stat("/var/tmp/usb_test.tmp", &usbVendorID) != 0){
                        return "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0";
                }
                unlink("/var/tmp/usb_test.tmp");
                strcpy(eth, nvram_safe_get("wan_eth"));
        }
#endif
        else
        {
#endif //CONFIG_USER_3G_USB_CLIENT

#ifdef  CONFIG_BRIDGE_MODE
        if(strcmp(wlan0_mode, "rt") == 0)
#endif
        {
                /* Check phy connect statue */
                VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
                if( !strncmp("disconnect", status, 10) )
                        return "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0";
        }
#ifdef CONFIG_USER_3G_USB_CLIENT
        }
#endif

        eth_ipaddr = read_ipaddr(eth);
        if(eth_ipaddr == NULL)
                sprintf(wan_current_ipaddr, "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0");
        else if(!strncmp(eth_ipaddr, "10.64.64.64", 11) || strlen(eth_ipaddr)==0)
                sprintf(wan_current_ipaddr, "0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0/0.0.0.0");
        else
        {
                sprintf(wan_current_ipaddr, "%s", eth_ipaddr);

#ifdef  CONFIG_BRIDGE_MODE
            if((strcmp(wlan0_mode, "ap") == 0) && (strcmp(wan_proto, "static") == 0))
                sprintf(wan_current_ipaddr + strlen(wan_current_ipaddr), "/%s", nvram_safe_get("wan_static_gateway"));
            else
#endif
                 sprintf(wan_current_ipaddr + strlen(wan_current_ipaddr), "/%s", read_gatewayaddr(eth));

            sprintf(wan_current_ipaddr + strlen(wan_current_ipaddr), "/%s", read_dns(ifnum));
        }
        return wan_current_ipaddr;

}


char *wan_statue(const char *proto)
{
        char pid_file[40]="";
        char mpppoe_mode[40]="";
        char eth[5], ifnum='0';
        char status[15];
        int demand = 0, alwayson = 0;
        struct stat filest;
        char *ConnStatue[] = {"Connected", "Connecting", "Disconnected"};
        enum {Connected, Connecting, Disconnected};
        char *wan_eth = NULL;
        char *ip_addr = NULL;
#if defined(CONFIG_USER_3G_USB_CLIENT) || defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        int usb_type = 0;
	char cmd[128]={'\0'};
        struct stat usbVendorID;
        usb_type = atoi(nvram_safe_get("usb_type"));
#endif

#ifdef CONFIG_USER_3G_USB_CLIENT
        if(strcmp(proto, "usb3g")==0)
        {
                /* Check USB Port connection */
                if(stat(PPP_3G_VID_PATH, &usbVendorID) != 0)
                        return ConnStatue[Disconnected];
        }
/*      Date:   2010-01-22
*       Name:   jimmy huang
*       Reason: for Windows Mobile 5, We do not use VCTGetPortConnectState
*                       to get wan port conn state
*/
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
/*      Date:   2010-02-05
 *      Name:   Cosmo Chang
 *      Reason: add usb_type=5 for Android Phone RNDIS feature
 */
        else if( (strcmp(proto, "usb3g_phone") == 0) && ((usb_type == 3) || (usb_type == 4) || (usb_type == 5)) ){
                wan_eth = nvram_safe_get("wan_eth");
                sprintf(cmd,"[ -n \"`cat %s`\" ] && echo 1 > /var/tmp/usb_test.tmp",USB_DEVICE_VendorID);
                system(cmd);
                /* Check USB Port connection */
                if(stat("/var/tmp/usb_test.tmp", &usbVendorID) != 0){
                        return ConnStatue[Disconnected];
                }
                unlink("/var/tmp/usb_test.tmp");
        }
#endif
        else
        {
#endif //CONFIG_USER_3G_USB_CLIENT

        wan_eth = nvram_safe_get("wan_eth");

#ifdef CONFIG_BRIDGE_MODE
        if(!nvram_match("wlan0_mode","rt"))
#endif
        {
                /* Check WAN eth phy connect */
                VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);
                if( !strncmp("disconnect", status, 10) )
                        return ConnStatue[Disconnected];
        }
#ifdef CONFIG_USER_3G_USB_CLIENT
        }
#endif //CONFIG_USER_3G_USB_CLIENT

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        if( strcmp(proto, "dhcpc") ==0 || strcmp(proto, "static") ==0 || strcmp(proto, "usb3g_phone") ==0)
#else
        if( strcmp(proto, "dhcpc") ==0 || strcmp(proto, "static") ==0)
#endif
        {
#ifdef CONFIG_BRIDGE_MODE
                if(!nvram_match("wlan0_mode","ap"))
                        strcpy(eth, nvram_safe_get("lan_bridge"));
                else
#endif
                        strcpy(eth, wan_eth);
        }
        else
    {
#ifdef MPPPOE
            ifnum = *(proto + 7);
        sprintf(eth, "ppp%c", ifnum);
#else
        strcpy(eth, "ppp0");
#endif
    }

    ip_addr = read_ipaddr(eth);

    if(ip_addr)
    {
                if( strncmp(proto, "pppoe", 5)==0
#ifdef CONFIG_USER_3G_USB_CLIENT
                 || strncmp(proto, "usb3g", 5)==0
#endif
                )
        {
                        if(!strncmp(ip_addr, "10.64.64.64", 11))
                                return ConnStatue[Disconnected];
        }
            if( read_ipaddr_no_mask(eth))
                    return ConnStatue[Connected];
    }

        if(strncmp(proto, "pppoe", 5) ==0) {
                sprintf( pid_file, "/var/run/ppp-0%c.pid", ifnum );
                sprintf( mpppoe_mode, "wan_pppoe_connect_mode_0%c", ifnum);
                if(nvram_match(mpppoe_mode,"on_demand")==0)
                        demand = 1;
                else if(nvram_match(mpppoe_mode,"always_on")==0)
                        alwayson = 1;
        } else if(strcmp(proto, "pptp") ==0 ) {
                strcpy(pid_file, "/var/run/ppp-pptp.pid");
                if( nvram_match("wan_pptp_connect_mode", "on_demand")==0 )
                        demand = 1;
                else if(nvram_match("wan_pptp_connect_mode", "always_on")==0)
                        alwayson = 1;
        } else if(strcmp(proto, "l2tp") ==0){
                strcpy(pid_file, "/var/run/l2tp.pid");
                if( nvram_match("wan_l2tp_connect_mode", "on_demand")==0 )
                        demand = 1;
                else if( nvram_match("wan_l2tp_connect_mode", "always_on")==0 )
                        alwayson = 1;
        }
#ifdef IPV6_DSLITE
	else if (strcmp(proto, "dslite") == 0 && stat(DSLITE_PID, &filest) == 0) {
		return ConnStatue[Connected];
	}
#endif
#ifdef CONFIG_USER_3G_USB_CLIENT
        else if(strncmp(proto, "usb3g", 5) ==0) {
                sprintf(pid_file, "/var/run/ppp0.pid");
                if(nvram_match("usb3g_reconnect_mode","on_demand")==0)
                        demand = 1;
                else if(nvram_match("usb3g_reconnect_mode","always_on")==0)
                        alwayson = 1;
        }
#endif

        if( stat(pid_file, &filest) == 0 || alwayson == 1)
                return ConnStatue[Connecting];

        return ConnStatue[Disconnected];
}

/*  Date: 2009-2-17
*   Name: Ken Chiang
*   Reason: added to change wireless region from show Domain name to Country name for version.txt.
*   Notice :
*/
char *GetCountryName(unsigned short number)
{
        int n=0;
        DEBUG_MSG("%s:\n",__func__);
        while (1)
        {
                if (country_name[n].number == 0xffff)
                        break;    /* not found */
                if (country_name[n].number == number)
                        break;    /* found */
                n++;
        }

        return (country_name[n].name);
}

char *GetDomainName(unsigned short number)
{
        int n=0;

        while (1)
        {
                if (domain_name[n].number == 0xffff)
                        break;    /* not found */
                if (domain_name[n].number == number)
                        break;    /* found */
                n++;
        }

        return (domain_name[n].name);
}

char *GetDomainNameByRegion(char *region_s){
        int n = 0;
        char *wlan_domain = NULL;

        while (1){
                if (strcmp(region[n].name,region_s) == 0){
                        wlan_domain = GetDomainName(region[n].number);
                        if(!strcmp(wlan_domain,"unknow") == 0)
                                return "unknow";
                        else
                                return wlan_domain              ;
                }
                n++;
        }

        return "unknow";
}

int iw_freq2channel(iwfreq *    in)
{
        int     freq = in->m;
        int channel;

        channel = (freq/100000 - 2412)/5 + 1;
        if(channel > 0 && channel < 14)
                return channel;
        else
                return 0;
}

void get_channel(char *wlan_channel)
{
#ifdef DIR865   //using cmd and parse file to get channel
        FILE *fp;
        char cmds[128], tmp_channel[4];

        memset(tmp_channel, '\0', sizeof(tmp_channel));
        sprintf(cmds, "%s", "cmds wlan utility channel ra01_0 channel");

        fp = popen(cmds, "r");
        if (fp == NULL) {
                strcpy(wlan_channel, "unknown");
                return;
        }

        fgets(tmp_channel, sizeof(tmp_channel), fp);
        pclose(fp);
        strcpy(wlan_channel, tmp_channel);

        return;

#else ////using ioctl to get channel

        int skfd;
        struct iwreq wrq;
        int channel;

        /* Create a channel to the NET kernel. */
        skfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(skfd < 0)
        {
                perror("get_channel");
                strcpy(wlan_channel, "unknown");
                close(skfd);
                return;
        }

        /* Set device name */
        strncpy(wrq.ifr_name, WLAN0_ETH, IFNAMSIZ);

        /* Do the request */
        if( ioctl(skfd, 0x8B05/*SIOCGIWFREQ*/, &wrq) >= 0)
        {
                channel=iw_freq2channel(&(wrq.u.freq));
                if(channel>0)
                        sprintf(wlan_channel,"%d",channel);
                else
                        strcpy(wlan_channel, "unknown");
        }
        else
        {
                printf("get_channel: ioctl error\n");
                strcpy(wlan_channel, "unknown");
        }

        close(skfd);
        return;
#endif
}

//NickChou add for AP94
int iw_freq2channel_5g(iwfreq * in)
{
        int     freq = in->m;
        int channel;

        //printf("NickChou: iw_freq2channel_5g: freq=%d\n", freq);
        channel = (freq/100000-5000)/5;

        return channel;
}

//5G interface 2
void get_5g_channel(char *wlan_channel)
{
#ifdef DIR865   //using cmd and parse file to get channel
        FILE *fp;
        char cmds[128], tmp_channel[4];

        memset(tmp_channel, '\0', sizeof(tmp_channel));
        sprintf(cmds, "%s", "cmds wlan utility  channel ra00_0 channel");

        fp = popen(cmds, "r");
        if (fp == NULL) {
                strcpy(wlan_channel, "unknown");
                return;
        }

        fgets(tmp_channel, sizeof(tmp_channel), fp);
        pclose(fp);
        strcpy(wlan_channel, tmp_channel);
        return;
#else ////using ioctl to get channel
        int skfd;
        struct iwreq wrq;
        int channel;

        /* Create a channel to the NET kernel. */
        skfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(skfd < 0)
        {
                perror("get_5g_channel");
                strcpy(wlan_channel, "unknown");
                close(skfd);
                return;
        }

        /* Set device name */
        if(nvram_match("wlan0_enable", "1")==0){
                strncpy(wrq.ifr_name, WLAN1_ETH, IFNAMSIZ);
        }
        else{
                strncpy(wrq.ifr_name, WLAN0_ETH, IFNAMSIZ);
        }

        /* Do the request */
        if( ioctl(skfd, 0x8B05/*SIOCGIWFREQ*/, &wrq) >= 0)
        {
                channel=iw_freq2channel_5g(&(wrq.u.freq));
                if(channel>0)
                        sprintf(wlan_channel,"%d",channel);
                else
                        strcpy(wlan_channel, "unknown");
        }
        else
        {
//                printf("get_5g_channel: ioctl error\n");
                strcpy(wlan_channel, "unknown");
        }

        close(skfd);
        return;
#endif
}

void get_resetting_bytes()
{
        memset(p_wan_txbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_wan_rxbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_wan_lossbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_lan_txbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_lan_rxbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_lan_lossbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_wlan_txbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_wlan_rxbytes_on_resetting, 0, sizeof(long)*8+1);
        memset(p_wlan_lossbytes_on_resetting, 0, sizeof(long)*8+1);

        if(display_real_wan_pkts(TXPACKETS))
                memcpy(p_wan_txbytes_on_resetting, display_real_wan_pkts(TXPACKETS), sizeof(long)*8);
        if(display_real_wan_pkts(RXPACKETS))
                memcpy(p_wan_rxbytes_on_resetting, display_real_wan_pkts(RXPACKETS), sizeof(long)*8);
        if(display_real_wan_pkts(LOSSPACKETS))
                memcpy(p_wan_lossbytes_on_resetting, display_real_wan_pkts(LOSSPACKETS), sizeof(long)*8);
        if(display_real_lan_pkts(TXPACKETS))
                memcpy(p_lan_txbytes_on_resetting, display_real_lan_pkts(TXPACKETS), sizeof(long)*8);
        if(display_real_lan_pkts(RXPACKETS))
                memcpy(p_lan_rxbytes_on_resetting, display_real_lan_pkts(RXPACKETS), sizeof(long)*8);
        if(display_real_lan_pkts(LOSSPACKETS))
                memcpy(p_lan_lossbytes_on_resetting, display_real_lan_pkts(LOSSPACKETS), sizeof(long)*8);
        if(display_real_wlan_pkts(TXPACKETS))
                memcpy(p_wlan_txbytes_on_resetting, display_real_wlan_pkts(TXPACKETS), sizeof(long)*8);
        if(display_real_wlan_pkts(RXPACKETS))
                memcpy(p_wlan_rxbytes_on_resetting, display_real_wlan_pkts(RXPACKETS), sizeof(long)*8);
        if(display_real_wlan_pkts(LOSSPACKETS))
                memcpy(p_wlan_lossbytes_on_resetting, display_real_wlan_pkts(LOSSPACKETS), sizeof(long)*8);

}

void get_ifconfig_resetting_bytes() //for display_ifconfig_info()
{
        FILE *fp = NULL;
        char buf[80] = {};
        unsigned long rx_error, tx_error;
        int if_type = 0;

        for(if_type = 0; if_type < 4; if_type++)
        {
                switch (if_type){
                        case 0:
                                DEBUG_MSG("%s: Lan\n",__func__);
                                _system("ifconfig %s > /var/misc/ifconfig_info.txt", nvram_safe_get("wan_eth"));
                                break;
                        case 1:
                                DEBUG_MSG("%s: Wan\n",__func__);
                                _system("ifconfig %s > /var/misc/ifconfig_info.txt",nvram_safe_get("lan_eth"));
                                break;
#ifdef DIR865
                        //wireless interface 1(2.4G)
                        case 2:
                                DEBUG_MSG("%s: Wlan\n",__func__);
                                if(nvram_match("wlan0_enable","0") == 0)
                                        return NULL;
                                else
                                        _system("iwpriv %s stat > /var/misc/ifconfig_info.txt", WLAN0_ETH);
                                break;
                        //wireless interface 2(5G)
                        case 3:
                                DEBUG_MSG("%s: Wlan1\n",__func__);
                                if(nvram_match("wlan1_enable","0") == 0)
                                        return NULL;
                                else{
                                        if(nvram_match("wlan0_enable", "1")==0)
                                                _system("iwpriv %s stat > /var/misc/ifconfig_info.txt", WLAN1_ETH);
                                        else
                                                _system("iwpriv %s stat > /var/misc/ifconfig_info.txt", WLAN0_ETH);
                                }
                                break;
#else//DIR865
                        //wireless interface 1(2.4G)
                        case 2:
                                if(nvram_match("wlan0_enable","0") == 0)
                                        return NULL;
                                else
                                        _system("ifconfig %s > /var/misc/ifconfig_info.txt", WLAN0_ETH);
                                break;
                        //wireless interface 2(5G)
                        case 3:
#ifdef DIR865
                                if(nvram_match("wlan1_enable","0") == 0)
                                        return NULL;
                                else
#endif
                                {
                                        if(nvram_match("wlan0_enable", "1")==0)
                                                _system("ifconfig %s > /var/misc/ifconfig_info.txt", "ath1");
                                        else
                                                _system("ifconfig %s > /var/misc/ifconfig_info.txt", "ath0");
                                }
                                break;
#endif//DIR865
                }

                fp = fopen("/var/misc/ifconfig_info.txt","rb");
                if (fp == NULL)
                        return NULL;

                while(fgets(buf, sizeof(buf),fp))
                {
                        if(strstr(buf,"RX packets"))
                        {
                                if( sscanf(buf, "           RX packets:%lu errors:%lu dropped:%lu overruns:%*lu frame:%*lu",
                                        &rx_packets_on_resetting[if_type], &rx_error, &rx_dropped_on_resetting[if_type]) != 3)
                                        printf("sscanf: error 1\n");
                        }
                    if(strstr(buf,"TX packets"))
            {
                                if( sscanf(buf, "           TX packets:%lu errors:%lu dropped:%lu overruns:%*lu carrier:%*lu",
                                        &tx_packets_on_resetting[if_type], &tx_error, &tx_dropped_on_resetting[if_type]) != 3)
                                        printf("sscanf: error 2\n");
                        }
                        if(strstr(buf,"collisions"))
                        {
                                if( sscanf(buf, "           collisions:%lu txqueuelen:%*lu",
                                        &collisions_on_resetting[if_type]) != 1)
                                        printf("sscanf: error 3\n");
                        }
#ifdef DIR865
                        char rx_packets[16] = {0};//,rx_error[16] = {0},rx_drop[16] = {0};
                        char tx_packets[16] = {0};//,tx_error[16] = {0},tx_drop[16] = {0};
                        char *packets;//, *error, *drop, *overrun, *collisions, *txqueuelen;
                        if(packets = strstr(buf,"Rx success"))
                        {
                                DEBUG_MSG("%s: Rx success\n",__func__);
                                if(packets = strstr(packets,"="))
                                {
                                        strcpy(rx_packets,packets+1);
                                        rx_packets_on_resetting[if_type]= atol(rx_packets);
                                        DEBUG_MSG("rx_packets_on_resetting[%d]=%lu\n",if_type,rx_packets_on_resetting[if_type]);
                                }
                        }
                        if(packets = strstr(buf,"Tx success"))
                        {
                                DEBUG_MSG("%s: Tx success\n",__func__);
                                if(packets = strstr(packets,"="))
                                {
                                        strcpy(tx_packets,packets+1);
                                        tx_packets_on_resetting[if_type]= atol(tx_packets);
                                        DEBUG_MSG("tx_packets_on_resetting[%d]=%lu\n",if_type,tx_packets_on_resetting[if_type]);
                                }
                        }
#endif
                        memset(buf,0 , sizeof(buf));
                }
                error_on_resetting[if_type] = rx_error + tx_error;
                fclose(fp);
        }

#if 0
        printf("rx_packets_on_resetting: %u, %u, %u, %u\n", rx_packets_on_resetting[0], rx_packets_on_resetting[1], rx_packets_on_resetting[2], rx_packets_on_resetting[3]);
        printf("tx_packets_on_resetting: %u, %u, %u, %u\n", tx_packets_on_resetting[0], tx_packets_on_resetting[1], tx_packets_on_resetting[2], tx_packets_on_resetting[3]);
        printf("rx_dropped_on_resetting: %u, %u, %u, %u\n", rx_dropped_on_resetting[0], rx_dropped_on_resetting[1], rx_dropped_on_resetting[2], rx_dropped_on_resetting[3]);
        printf("tx_dropped_on_resetting: %u, %u, %u, %u\n", tx_dropped_on_resetting[0], tx_dropped_on_resetting[1], tx_dropped_on_resetting[2], tx_dropped_on_resetting[3]);
        printf("collisions_on_resetting: %u, %u, %u, %u\n", collisions_on_resetting[0], collisions_on_resetting[1], collisions_on_resetting[2], collisions_on_resetting[3]);
        printf("error_on_resetting     : %u, %u, %u, %u\n", error_on_resetting[0], error_on_resetting[1], error_on_resetting[2], error_on_resetting[3]);
#endif

}

#if 0
/* start of smtp function */
static int reset_smtpd(void){
        if(smtpb->sock != -1){
                close(smtpb->sock);
                smtpb->sock = -1;
        }
        if(smtpb){
                free(smtpb);
                smtpb = NULL;
        }
        return 0;
}

        static int smtp_send_data(int s, char *buf, int len){
                if(send(s, (void *)buf, len, 0) < 0)
                        return -1;
                return 0;
        }

static int smtp_get_response(void){
        fd_set afdset;
        int ret = 0, r = 0;
        char *tag = NULL;
        char buf[5000] = {};
        struct timeval timeval;

        if(smtpb->sock < 0)
                return -1;

        memset(&timeval, 0, sizeof(timeval));
        timeval.tv_sec = 15;
        timeval.tv_usec = 0;

        while(1){
                FD_ZERO(&afdset);
                FD_SET(smtpb->sock, &afdset);

                /* timeout orror */
                ret = select(smtpb->sock + 1, &afdset, NULL, NULL, &timeval);
                if(ret <= 0)
                        return -1;

                /* receive fail */
                r = recv(smtpb->sock, buf, 500, 0);
                if(r <= 0)
                        return -1;

                /* end of data */
                if((tag = strstr(buf, "\r\n")) != (char *)0){
                        *tag = 0;
                        break;
                }
        }

        /* unexpected reply */
        if((!isdigit(buf[0])) || (buf[0] > '3'))
                return -1;

        return 1;
}

        static int chat(int s, char *buf, int len){
                if(smtp_send_data(s, buf, len) < 0)
                        return -1;
                return (smtp_get_response());
        }

static int send_messag_body(void){
        FILE *fp = NULL;
        char *buf = NULL;
        struct stat file_buf;

        /* access file fail */
        if(access(LOGFILE_PATH, F_OK) < 0)
                return reset_smtpd();

        stat(LOGFILE_PATH, &file_buf);

        /* malloc fail */
        if((buf = (char *)malloc(file_buf.st_size + 1)) == NULL)
                return reset_smtpd();

        /* open file */
        if((fp = fopen(LOGFILE_PATH, "r")) == NULL){
                if(buf)
                        free(buf);
                return reset_smtpd();
        }

        fread((void *)buf, file_buf.st_size, 1, fp);
        fclose(fp);

        /* send_data fail */
        if(smtp_send_data(smtpb->sock, buf, file_buf.st_size) < 0){
                free(buf);
                return reset_smtpd();
        }
        free(buf);

        return 1;
}

static int send_message_header(void){
        char buf[500] = {};
        int i = 0, len = 0;

        len = 0;
        len += sprintf(buf + len, "From: %s\r\n", smtpb->addr_p[0]);
        len += sprintf(buf + len, "Subject: Log Manual(from: %s)\r\n", smtpb->wan_ip);
        len += sprintf(buf + len ,"Sender: %s\r\n", smtpb->host_name);
        len += sprintf(buf + len, "To: %s", smtpb->addr_p[0]);

        for(i = 1; i < smtpb->addr_num; i++){
                len += sprintf(buf + len, ",%s", smtpb->addr_p[i]);
        }
        len += sprintf(buf + len, "\r\n\r\n");

        /* send data fail */
        if(smtp_send_data(smtpb->sock, buf, len) < 0)
                return reset_smtpd();

        return 1;
}

static int send_smtp_header(void){
        char buf[500] = {};
        int i = 0, len = 0;

        len = 0;
        len += sprintf(buf + len, "HELO %s\r\n", smtpb->host_name);
        if(chat(smtpb->sock, buf, len) < 0)
                return reset_smtpd();

        len = 0;
        len += sprintf(buf + len, "MAIL FROM: <%s>\r\n", smtpb->addr_p[0]);
        if(chat(smtpb->sock, buf, len) < 0)
                return reset_smtpd();

        len = 0;
        for(i = 0; i < smtpb->addr_num; i++){
                len += sprintf(buf + len, "RCPT TO: <%s>\r\n", smtpb->addr_p[i]);
                if(chat(smtpb->sock, buf, len) < 0)
                        return reset_smtpd();
        }

        len = 0;
        len += sprintf(buf + len, "DATA\r\n");
        if(chat(smtpb->sock, buf, len) < 0)
                return reset_smtpd();

        return 1;
}

static int send_smtp_end(void){
        char buf[500] = {};
        int len = 0;

        len = 0;
        len += sprintf(buf + len, "\r\n.\r\n");
        if(chat(smtpb->sock, buf, len) < 0)
                return reset_smtpd();

        len = 0;
        len += sprintf(buf + len, "QUIT\r\n");
        if(chat(smtpb->sock, buf, len))
                return reset_smtpd();

        return 1;
}

static int smtp_update_addr(_SMTPB_s *smtp_var){
        int i = 0;
        char *buf = NULL, *found = NULL;

        if((strlen(smtp_var->server) == 0) || (strlen(smtp_var->addr_buf) == 0))
                return reset_smtpd();

        if (strlen(nvram_safe_get("hostname")) > 1)
                strcpy(smtp_var->host_name, nvram_safe_get("hostname"));
        else
                strcpy(smtp_var->host_name, nvram_safe_get("model_number"));

        buf = smtp_var->addr_buf;
        for(i = 0; i < SMTP_MAX_ADDR_NUM; i++){
                for(; *buf == ' ';)
                        buf++;

                smtp_var->addr_p[i] = buf;
                if((found = strchr(buf, ',')) || (found = strchr(buf, ';'))){
                        buf = found;
                        *buf = 0;
                        buf++;
                }else{
                        break;
                }
        }

        smtp_var->addr_num = i + 1;
        return 1;
}

static int do_connect(int fd,struct sockaddr *remote, int len,struct timeval *timeout, int *err)
{
        int saveflags = -1,ret = -1 ,back_err = -1;
        fd_set fd_w;

        saveflags=fcntl(fd,F_GETFL,0);                                                          /* save flags*/
        if(saveflags<0)
                return -1;
        if(fcntl(fd,F_SETFL,saveflags|O_NONBLOCK)<0)    /* Set non blocking */
                return -1;

        *err=connect(fd,remote,len);
        back_err=errno;

        if(fcntl(fd,F_SETFL,saveflags)<0)                                                       /* restore flags */
                return -1;
        if(*err<0 && back_err!=EINPROGRESS)                             /* connect error & not in progress */
                return -1;

        FD_ZERO(&fd_w);
        FD_SET(fd,&fd_w);

        *err=select(smtpb->sock+1,NULL,&fd_w,NULL,timeout);
        if(*err<0)                                                                                                                                      /* select error*/
                return -1;
        if(*err==0) {                                                                                                                                   /* 0 means timeout out & no fds changed */
                close(fd);*err=ETIMEDOUT;return -1;}

        len=sizeof(ret);
        *err=getsockopt(fd,SOL_SOCKET,SO_ERROR,&ret,&len);
        if(*err<0)                                                                                                      /* get the return */
                return -1;
        if(ret)                                                                                                                                 /* ret = 0 means success */
                return -1;

        return 0;
}

static int init_socket(struct sockaddr_in s_sin,struct sockaddr_in s_socka){
        int err = 0;
        struct timeval timeval_t;

        /* set time out of connecting smtp server */
        memset(&timeval_t, 0, sizeof(timeval_t));
        timeval_t.tv_sec = 5;
        timeval_t.tv_usec = 0;

        /* initial sockaddr_in */
        memset(&s_sin, 0, sizeof(s_sin));
        memset(&s_socka, 0, sizeof(s_sin));

        s_sin.sin_addr.s_addr = *((int*)(he->h_addr_list[0]));
        s_sin.sin_family = AF_INET;
        s_sin.sin_port = htons(25);

        /* pen socket fail*/
        if((smtpb->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
                return reset_smtpd();

        s_socka.sin_family = AF_INET;
        s_socka.sin_port = 0;
        s_socka.sin_addr.s_addr = INADDR_ANY;

        /* bind socket fail */
        err = bind(smtpb->sock, (struct sockaddr *)&s_socka, sizeof(struct sockaddr_in));
        if(err)
                return reset_smtpd();

        /* connect to server fail */
        err = do_connect(smtpb->sock,(struct sockaddr *)&s_sin, sizeof(struct sockaddr_in),&timeval_t, &err);
        if(err)
                return reset_smtpd();

        return 1;
}

static int init_smtp(char *s_server,char *s_account,char *s_wan_ip){

        if((smtpb = (struct _smtpb_t *)malloc(sizeof(struct _smtpb_t))) == NULL)
                return reset_smtpd();

        memset(smtpb, 0, sizeof(struct _smtpb_t));              //initial smtpd
        smtpb->server = s_server;
        smtpb->addr_buf = s_account;
        smtpb->wan_ip = s_wan_ip;
        smtpb->sock = -1;

        /* gethostbyname fail */
        he = gethostbyname(smtpb->server);                                              // get host by name
        if(he == NULL)
                return reset_smtpd();

        return 1;
}

/* smtp entry */
int send_log_mail(char *server,char *account,char *wan_ip){
        struct sockaddr_in sin;
        struct sockaddr_in socka;

        if(!init_smtp(server,account,wan_ip))   // initial smtp
                return 0;
        if(!smtp_update_addr(smtpb))            // update address
                return 0;
        if(!init_socket(sin,socka))             // initial socket
                return 0;
        if(!smtp_get_response())                            // get response
                return 0;
        if(!send_smtp_header())                             // send smtp header
                return 0;
        if(!send_message_header())                  // send message header
                return 0;
        if(!send_messag_body())                             // send message body
                return 0;
        if(!send_smtp_end())                                        // send SMTP end
                return 1;
        return 1;
}
/* end of smtp function */
#endif

char* convert_webvalue_to_nvramvalue(char *p_nvrma_name, char *web_value)
{
        char *security_value,*temp_security_value;
        int i, j;

        memset(nvram_wepkey_value, 0, 256);
        strcpy(nvram_wepkey_value, web_value);
        //printf("p_nvrma_name=%s,web_value=%s\n",p_nvrma_name,web_value);

        for(i = 0; i < 16 ; i++)
        {
                if (strcmp(p_nvrma_name, pWepKeyIndex[i] ) == 0)
                {
#ifdef USER_WL_ATH_5GHZ
                        if ((p_wlan_wep_flag_info->ASCII_flag[0] == 1 && strstr(p_nvrma_name, "wlan0")) ||
                                (p_wlan_wep_flag_info->a_ASCII_flag[0] == 1 && strstr(p_nvrma_name, "wlan1"))
                        )
#else
                        if ((p_wlan_wep_flag_info->ASCII_flag[0] == 1) && strstr(p_nvrma_name, "wlan0"))
#endif
                        {
                                if (strlen(web_value) > 5)
                                        web_value[5] ='\0';
                                memset(nvram_wepkey_value , 0, 256);
                                for (j =0 ; j < strlen(web_value); j++)
                                        sprintf(nvram_wepkey_value, "%s%x", nvram_wepkey_value,*(web_value+j));
                        }
                        if (p_wlan_wep_flag_info->wep64_flag[0] != 1)
                        {
                                memset(nvram_wepkey_value , 0, 256);
                                memcpy(nvram_wepkey_value, "0000000000", 10);
                        }
                        return nvram_wepkey_value;
                }
        }
        //wep 64bits
        for (i = 0; i < 4 ; i++)
        {
                if (strcmp(p_nvrma_name, pTempWepKeyIndex[i] ) == 0)
                {
                        if (p_wlan_wep_flag_info->temp_ASCII_flag[0] == 1)
                        {
                                if (strlen(web_value) > 5)
                                        web_value[5] ='\0';
                                memset(nvram_wepkey_value , 0, 256);
                                for (j =0 ; j < strlen(web_value); j++)
                                        sprintf(nvram_wepkey_value, "%s%x", nvram_wepkey_value,*(web_value+j));
                        }
/*
*       Date: 2009-7-6
*   Name: Ken Chiang
*   Reason: modify for wizard can set web key used the aep_temp_52 replace the wlan0_security.
*   Notice :
*/
                        security_value = nvram_get("wlan0_security");
                        temp_security_value = nvram_get("asp_temp_52");
                        if (security_value == NULL || strcmp(security_value, "") == 0)
                        {
                                if (temp_security_value == NULL || strcmp(temp_security_value, "") == 0)
                                {
                                        memset(nvram_wepkey_value , 0, 256);
                                        memcpy(nvram_wepkey_value, "0000000000", 10);
                                        return nvram_wepkey_value;
                                }
                                else{
                                        if(!(strcmp(temp_security_value, "wep_open_64" ) == 0 || strcmp(temp_security_value, "wep_share_64" ) == 0))
                                        {
                                                memset(nvram_wepkey_value , 0, 256);
                                                memcpy(nvram_wepkey_value, "0000000000", 10);
                                        }
                                }
                        }

                        if (!(strcmp(security_value, "wep_open_64" ) == 0 || strcmp(security_value, "wep_share_64" ) == 0))
                        {
                                if (!(strcmp(temp_security_value, "wep_open_64" ) == 0 || strcmp(temp_security_value, "wep_share_64" ) == 0))
                                {
                                        memset(nvram_wepkey_value , 0, 256);
                                        memcpy(nvram_wepkey_value, "0000000000", 10);
                                }
                        }
                        return nvram_wepkey_value;
                }
        }

        for (i = 16; i < 32 ; i++)
        {
                if (strcmp(p_nvrma_name, pWepKeyIndex[i] ) == 0)
                {
#ifdef USER_WL_ATH_5GHZ
                        if ((p_wlan_wep_flag_info->ASCII_flag[0] == 1 && strstr(p_nvrma_name, "wlan0")) ||
                                (p_wlan_wep_flag_info->a_ASCII_flag[0] == 1 && strstr(p_nvrma_name, "wlan1"))
                        )
#else
                        if ((p_wlan_wep_flag_info->ASCII_flag[0] == 1) && strstr(p_nvrma_name, "wlan0"))
#endif
                        {
                                if(strlen(web_value) > 13)
                                        web_value[13] ='\0';
                                memset(nvram_wepkey_value , 0, 256);
                                for(j =0 ; j < strlen(web_value); j++)
                                        sprintf(nvram_wepkey_value, "%s%x", nvram_wepkey_value,*(web_value+j));
                        }
                        if (p_wlan_wep_flag_info->wep128_flag[0] != 1)
                        {
                                memset(nvram_wepkey_value , 0, 256);
                                memcpy(nvram_wepkey_value, "00000000000000000000000000", 26);
                        }
                        return nvram_wepkey_value;
                }
        }
        //wep 128bits
        for (i = 4; i < 8 ; i++)
        {
                if (strcmp(p_nvrma_name, pTempWepKeyIndex[i] ) == 0)
                {
                        if (p_wlan_wep_flag_info->temp_ASCII_flag[0] == 1)
                        {
                                if (strlen(web_value) > 13)
                                        web_value[13] ='\0';
                                memset(nvram_wepkey_value , 0, 256);
                                for (j =0 ; j < strlen(web_value); j++)
                                        sprintf(nvram_wepkey_value, "%s%x", nvram_wepkey_value,*(web_value+j));
                        }
/*
*       Date: 2009-7-6
*   Name: Ken Chiang
*   Reason: modify for wizard can set web key used the aep_temp_52 replace the wlan0_security.
*   Notice :
*/
                        security_value = nvram_get("wlan0_security");
                        temp_security_value = nvram_get("asp_temp_52");
                        if (security_value == NULL || strcmp(security_value, "") == 0)
                        {
                                if (temp_security_value == NULL || strcmp(temp_security_value, "") == 0)
                                {
                                        memset(nvram_wepkey_value , 0, 256);
                                        memcpy(nvram_wepkey_value, "00000000000000000000000000", 26);
                                        return nvram_wepkey_value;
                                }
                                else {
                                        if (!(strcmp(temp_security_value, "wep_open_128" ) == 0 || strcmp(temp_security_value, "wep_share_128" ) == 0))
                                        {
                                                memset(nvram_wepkey_value , 0, 256);
                                                memcpy(nvram_wepkey_value, "00000000000000000000000000", 26);
                                        }
                                }
                        }

                        if (!(strcmp(security_value, "wep_open_128" ) == 0 || strcmp(security_value, "wep_share_128" ) == 0))
                        {
                                if (!(strcmp(temp_security_value, "wep_open_128" ) == 0 || strcmp(temp_security_value, "wep_share_128" ) == 0))
                                {
                                        memset(nvram_wepkey_value , 0, 256);
                                        memcpy(nvram_wepkey_value, "00000000000000000000000000", 26);
                                }
                        }
                        return nvram_wepkey_value;
                }
        }

        for (i = 32; i < 36 ; i++)
        {
                if (strcmp(p_nvrma_name, pWepKeyIndex[i] ) == 0)
                {
#ifdef USER_WL_ATH_5GHZ
                        if ((p_wlan_wep_flag_info->ASCII_flag[0] == 1 && strstr(p_nvrma_name, "wlan0")) ||
                                (p_wlan_wep_flag_info->a_ASCII_flag[0] == 1 && strstr(p_nvrma_name, "wlan1"))
                        )
#else
                        if ((p_wlan_wep_flag_info->ASCII_flag[0] == 1) && strstr(p_nvrma_name, "wlan0"))
#endif
                        {
                                if (strlen(web_value) > 16)
                                        web_value[16] ='\0';
                                memset(nvram_wepkey_value , 0, 256);
                                for (j =0 ; j < strlen(web_value); j++)
                                        sprintf(nvram_wepkey_value, "%s%x", nvram_wepkey_value,*(web_value+j));
                        }
                        if (p_wlan_wep_flag_info->wep152_flag[0] != 1)
                        {
                                memset(nvram_wepkey_value , 0, 256);
                                memcpy(nvram_wepkey_value, "00000000000000000000000000000000", 32);
                        }

                        return nvram_wepkey_value;
                }
        }
        //wep 152bits
        for (i = 8; i < 12 ; i++)
        {
                if (strcmp(p_nvrma_name, pTempWepKeyIndex[i] ) == 0)
                {
                        if (p_wlan_wep_flag_info->temp_ASCII_flag[0] == 1)
                        {
                                if (strlen(web_value) > 16)
                                        web_value[16] ='\0';
                                memset(nvram_wepkey_value , 0, 256);
                                for (j =0 ; j < strlen(web_value); j++)
                                        sprintf(nvram_wepkey_value, "%s%x", nvram_wepkey_value,*(web_value+j));
                        }
/*
*       Date: 2009-7-6
*   Name: Ken Chiang
*   Reason: modify for wizard can set web key used the aep_temp_52 replace the wlan0_security.
*   Notice :
*/
                        security_value = nvram_get("wlan0_security");
                        temp_security_value = nvram_get("asp_temp_52");
                        if (security_value == NULL || strcmp(security_value, "") == 0)
                        {
                                if (temp_security_value == NULL || strcmp(temp_security_value, "") == 0)
                                {
                                        memset(nvram_wepkey_value , 0, 256);
                                        memcpy(nvram_wepkey_value, "00000000000000000000000000000000", 32);
                                        return nvram_wepkey_value;
                                }
                                else {
                                        if (!(strcmp(temp_security_value, "wep_open_152" ) == 0 || strcmp(temp_security_value, "wep_share_152" ) == 0))
                                        {
                                                memset(nvram_wepkey_value , 0, 256);
                                                memcpy(nvram_wepkey_value, "00000000000000000000000000000000", 32);
                                        }
                                }
                        }

                        if (!(strcmp(security_value, "wep_open_152" ) == 0 || strcmp(security_value, "wep_share_152" ) == 0))
                        {
                                if (!(strcmp(temp_security_value, "wep_open_152" ) == 0 || strcmp(temp_security_value, "wep_share_152" ) == 0))
                                {
                                        memset(nvram_wepkey_value , 0, 256);
                                        memcpy(nvram_wepkey_value, "00000000000000000000000000000000", 32);
                                }
                        }
                        return nvram_wepkey_value;
                }
        }
        return nvram_wepkey_value;
}

unsigned long uptime(void){
        struct sysinfo info;
        sysinfo(&info);
        return info.uptime;
}

/* check if internet connection is established */
void check_internet_connection(char *ip_addr){
                _system("rc restart");
                sleep(3);
                get_ping_app_stat(ip_addr);
        }

/* get the wan port connection time */
unsigned long get_wan_connect_time(char *wan_proto, int check_wan_status){
        FILE *fp = NULL;
        char tmp[10] = {0};
        unsigned long connect_time  = 0;
        char wan_status[16]={0};

        if(check_wan_status)
        {
                memset(wan_status, 0 , sizeof(wan_status));
                if(strcmp(wan_proto, "pppoe") == 0)
                        strcpy(wan_status, wan_statue("pppoe-00"));
                else
                        strcpy(wan_status, wan_statue(wan_proto));
                if(strncmp(wan_status, "Connected", 9) == 0)
                {
                        /* open read-only file to get wan connect time stamp */
                        fp = fopen(WAN_CONNECT_FILE,"r+");
                        if(fp == NULL)
                                return 0;

                if( fgets(tmp, sizeof(tmp), fp) )
                        connect_time = atol(tmp);
                        fclose(fp);
                        if( uptime() == connect_time )
                                return 1;
                        else
                    return connect_time ? uptime() - connect_time : 0;
        }
                else
                        return 0;
        }
        else
        {
                /* open read-only file to get wan connect time stamp */
                fp = fopen(WAN_CONNECT_FILE,"r+");
                if(fp == NULL)
                        return 0;

                if( fgets(tmp, sizeof(tmp), fp) )
                        connect_time = atol(tmp);

                fclose(fp);

                if( uptime() == connect_time )
                        return 1;
                else
                        return connect_time ? uptime() - connect_time : 0;
        }
}

/* get the pppoe connection time */
unsigned long get_pppoe00_connect_time(int type)
{
        FILE *fp = NULL;
        char tmp[10] = {};
        unsigned long reset  = 0;      // reset time stamp
        unsigned long finial = 0;      // time after calculating

        /* open read-only file to get pppoe connect time stamp */
        fp = fopen(PPPoE_CONNECT_FILE,"r+");
        if(fp == NULL)
                return 0;
        memset(tmp,0,sizeof(tmp))       ;
        fgets(tmp,sizeof(tmp),fp);
        reset = atol(tmp);
        fclose(fp);

        /* open writable file to set pppoe connect time stamp */
        fp = fopen(PPPoE_CONNECT_FILE,"w+");
        if(fp == NULL)
                return 0;

        /* update new time */
        switch(type){
                case PPPoE_DISCONNECT_INIT:
                        fprintf(fp,"%lu%s",reset,"\n");
                        finial = 0;
                        break;
                case PPPoE_DISCONNECT:
                        fprintf(fp,"%lu%s",reset,"\n");
                        finial = uptime() - reset;
        }
        fclose(fp);
        return finial;
}

int send_log_by_smtp(void)
{
/*
        msmtp -d test_recipient@testingsmtp.com     //recipient
        --auth=login                                    //authentication method
        --user=test                                             //username
        --host=testing.smtp.com                                 //smtp server
        --from=test_sender@testingsmtp.com              //sender
        --passwd test                                           //password
        --port => NickChou Add for setting SMTP Server Port
*/
        FILE *fp;
        char log_email_server[64]={0}, log_email_username[64]={0}, log_email_password[64]={0};
        char log_email_recipien[64]={0}, log_email_sender[64]={0}, log_email_auth[64]={0};
        char log_email_server_port[6]={0};
        char buffer[256];
        char *ptr;

        fp=fopen(SMTP_CONF, "r");
        if(fp==NULL)
                return 0;

        while(fgets(buffer, 256, fp))
        {
                ptr = strstr(buffer, "log_email_auth=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_auth=");
                        strncpy(log_email_auth, ptr, strlen(buffer)-strlen("log_email_auth=")-1);
                        continue;
                }

                ptr = strstr(buffer, "log_email_recipien=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_recipien=");
                        strncpy(log_email_recipien, ptr, strlen(buffer)-strlen("log_email_recipien=")-1);
                        if(strlen(log_email_recipien) == 0)
                                return 0;
                        continue;
                }

                ptr = strstr(buffer, "log_email_username=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_username=");
                        strncpy(log_email_username, ptr, strlen(buffer)-strlen("log_email_username=")-1);
                        continue;
                }

                ptr = strstr(buffer, "log_email_password=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_password=");
                        strncpy(log_email_password, ptr, strlen(buffer)-strlen("log_email_password=")-1);
                        continue;
                }

                ptr = strstr(buffer, "log_email_server=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_server=");
                        strncpy(log_email_server, ptr, strlen(buffer)-strlen("log_email_server=")-1);
                        if(strlen(log_email_server) == 0)
                                return 0;
                        continue;
                }

                ptr = strstr(buffer, "log_email_sender=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_sender=");
                        strncpy(log_email_sender, ptr, strlen(buffer)-strlen("log_email_sender=")-1);
                        if(strlen(log_email_sender) == 0)
                                return 0;
                        continue;
                }

                ptr = strstr(buffer, "log_email_server_port=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_server_port=");
                        strncpy(log_email_server_port, ptr, strlen(buffer)-strlen("log_email_server_port=")-1);
                        if(strlen(log_email_server_port) == 0)
                                return 0;
                        continue;
                }

        }

        fclose(fp);
        DEBUG_MSG("send_log_by_smtp\n");

        if( (strcmp(log_email_auth, "1") == 0) )
        {
                _system("msmtp %s --auth=  --user=%s  --passwd %s --host=%s --from=%s --port=%s &",
                                log_email_recipien, log_email_username, log_email_password, log_email_server, log_email_sender, log_email_server_port);
        }
        else
        {
                _system("msmtp %s --host=%s --from=%s --port=%s &",
                                log_email_recipien, log_email_server, log_email_sender, log_email_server_port);
        }

        return 1;
}

/* Get the value of config_checksum in configuration file */
char *get_value_from_upload_config(char *path , char *name , char *value){
        FILE *fp = NULL;
        char *head = NULL;
        char *tail = NULL;
        char match_name[256] = {};
        char name_tmp[256] = {};

        /* read upload file */
        fp = fopen(path, "r");
        if(fp == NULL){
                return NULL;
        }

        /* get value of special name*/
        sprintf(match_name,"%s%s",name,"=");
        while ( fgets( name_tmp, sizeof(name_tmp) , fp ) != NULL ){
          /* initial varibles */
          memset(value,0,sizeof(value));
          tail = &name_tmp[0] + strlen(&name_tmp[0]);
          head = strstr(name_tmp,match_name);
          /* find keyword */
          if(head != NULL){
                head = head + strlen(&match_name[0]);
                /* cpoy B from A=B */
        memcpy(value , head ,tail - head);
        value[strlen(head) - strlen(tail) - 1] = '\0';
        return value;
    }
  }

  return NULL;
}

/* get nvram nvame from upload configuration file(exclude config_checksum) */
char *upload_configuration_init(char *path , char *name_string, char *boundry, int conf_ver_ctl){
        FILE *fp = NULL;
        char name_tmp[1024] = {};

        /* read upload file */
        fp = fopen(path, "r");
        if(fp == NULL){
                return NULL;
        }

        /* combine nvram with format A=B\nC=D\nE=F\n..... */
        while ( fgets( name_tmp, sizeof(name_tmp) , fp ) != NULL ){
                /* keyword at the end of file */
                if(strstr(name_tmp,boundry) != NULL)
                    break;
		
		/*
		configuration version control => never update from config.bin
		Only for whether add new nvram item from config.bin to NVRAM
		*/
		if(strstr(name_tmp, "configuration_version") != NULL){
			printf("Configuration Version Control: Do not update configuration_version to %s\n"
			, path);
			continue;
		}	

#ifdef NVRAM_CONF_VER_CTL/*configuration version control*/
		if(conf_ver_ctl==1){
			char *tmp_item;
			char name_tmp_item[256];
			memset(name_tmp_item,0,sizeof(name_tmp_item));
			tmp_item = NULL;
			strcpy(name_tmp_item, name_tmp);
			tmp_item = strtok(name_tmp_item, "=");
		
			/*	Do not use nvram_safe_get() => return "" when items un-exist and empty vlaue  
				Use nvram_get to check tmp_item in nvram.conf/nvram.default or not
			*/
			if(nvram_get(tmp_item)==NULL){	
				printf("Configuration Version Control: Do not add %s to %s\n"
					, tmp_item, path);
				continue;
			}	
		}
#endif				

                strcat(name_string,name_tmp);
                memset(name_tmp,0,sizeof(name_tmp));
        }
        fclose(fp);
        return name_string;
}

/* return checksum value */
int in_cksum(unsigned short *buf, int sz)
{
        int nleft = sz;
        int sum = 0;
        unsigned short *w = buf;
        unsigned short ans = 0;

        while (nleft > 1) {
                sum += *w++;
                nleft -= 2;
        }

        if (nleft == 1) {
                *(unsigned char *) (&ans) = *(unsigned char *) w;
                sum += ans;
        }

        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);
        ans = ~sum;
        return (ans);
}

void generate_pin_by_random(char *wps_pin)
{
        unsigned long int rnumber = 0;
        unsigned long int pin = 0;
        unsigned long int accum = 0;
        int digit = 0;
        int checkSumValue =0 ;

        //generate random 7 digits
        srand((unsigned int)time(NULL));
        rnumber = rand()%10000000;
        pin = rnumber;

        //Compute the checksum
        rnumber *= 10;
        accum += 3 * ((rnumber / 10000000) % 10);
        accum += 1 * ((rnumber / 1000000) % 10);
        accum += 3 * ((rnumber / 100000) % 10);
        accum += 1 * ((rnumber / 10000) % 10);
        accum += 3 * ((rnumber / 1000) % 10);
        accum += 1 * ((rnumber / 100) % 10);
        accum += 3 * ((rnumber / 10) % 10);
        digit = (accum % 10);
        checkSumValue = (10 - digit) % 10;

        pin = pin*10 + checkSumValue;
        sprintf( wps_pin, "%08d", pin );
        return;

}

void set_sta_enrollee_pin(unsigned char *pin)
{
        char cmd[32];
        printf("pin=%s\n",pin);
        if((pin != NULL)&& strcmp(pin," "))
        {
                sprintf(cmd,"wsc_cfg pin %s",pin);
                printf("cmd=%s\n",cmd);
                _system(cmd);
        }
        return ;
}

int push_button_result = 0;
void set_push_button_result(int result)
{
        printf("SET:push_button_result=%d\n",result);
        push_button_result = result;
        return;
}
int get_push_button_result(void)
{
        printf("GET:push_button_result=%d\n",push_button_result);
        return push_button_result;
}

void add_new_node(char* timeout, char *state,int direction_out, char *local_ip, char *remote_ip, char *local_port, char *remote_port, char *public_port, char *protocol, napt_session_info_t **n_node)
{
        napt_session_info_t *new_node = NULL;

        /* Allocate memory for new node */
        new_node = (napt_session_info_t*)malloc( sizeof(napt_session_info_t));
        if( new_node == NULL)
                printf("Allocate memory FAIL!!!");

        /* Copy the content of ip_contrack to new_node.
         * TCP: all fileds
         * UDP: all fileds except for state
         * ICMP: all fileds except for state ,local_port and remote_port
         */
        strcpy(new_node->timeout,timeout);
        strcpy(new_node->remote_ip,remote_ip);
        strcpy(new_node->local_ip,local_ip);
        strcpy(new_node->protocol,protocol);
        if(direction_out==1)
                strcpy(new_node->direction,"OUT");
        else
                strcpy(new_node->direction,"IN");
        if(state)
                strcpy(new_node->state,state);
        else
                strcpy(new_node->state,"-");
        if(local_port)
                strcpy(new_node->local_port,local_port);
        else
                strcpy(new_node->local_port,"");
        if(remote_port)
                strcpy(new_node->remote_port,remote_port);
        else
                strcpy(new_node->remote_port,"");
        if(public_port)
                strcpy(new_node->public_port,public_port);
        else
                strcpy(new_node->public_port,"-");
        new_node->next = NULL;
        *n_node=new_node;

        return;
}

void add_session_to_table(char *timeout, char *state, int direction_out, char *local_ip, char *remote_ip, char *local_port, char *remote_port, char *public_port, char *protocol)
{
        napt_session_info_t *new_node = NULL;


        add_new_node(timeout,state,direction_out,local_ip,remote_ip,local_port,remote_port,public_port,protocol,&new_node);

        if(session_list[0].next == NULL)
        {
                session_list[0].next = new_node;
                session_list[0].last = new_node;
        }
        else
        {
                session_list[0].last->next = new_node;
                session_list[0].last = new_node;
        }

        return;
}
char *char_token(char *src_str, char sub_char)
{
    char *current_ptr,*tmp_ptr;
    int len=0;
    static char  *next_ptr;


    if (src_str != NULL)
        current_ptr = src_str;
    else
        current_ptr = next_ptr;

        tmp_ptr=current_ptr;

    while (1)
    {
        if (*tmp_ptr == sub_char)
        {
            *tmp_ptr='\0';
            tmp_ptr++;
            next_ptr=tmp_ptr;

            if( *next_ptr != sub_char )
                break;
        }
        else if ( *tmp_ptr == '\0')
        {
                break;
        }
        else
        {
                tmp_ptr++;
                len++;
        }
    }
    return current_ptr;
}

void split(char *buf, char delim, char *tok[])
{
        int i = 0;
        char *p;

        if((p = char_token(buf, delim))!= NULL )
                tok[0] = p;

        for(i=1 ; i<MAX_NUMBER_FIELD ; i++)
        {
                if ((p = char_token(NULL, delim))!= NULL)
                        tok[i] = p;
        }
        return ;
}

char *get_ipaddr(char *if_name)
{
        int sockfd;
        struct ifreq ifr;
        struct in_addr in_addr;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return 0;

        strcpy(ifr.ifr_name, if_name);
        if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
                close(sockfd);
                return 0;
        }

        close(sockfd);

        in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
        return inet_ntoa(in_addr);
}


#ifndef CONFIG_WEB_404_REDIRECT
#ifndef USB_STORAGE_HTTP_ENABLE
char *get_netmask(char *if_name)
{
        int sockfd;
        struct ifreq ifr;
        struct in_addr netmask;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return 0;

        strcpy(ifr.ifr_name, if_name);
        if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
                close(sockfd);
                return 0;
        }

        close(sockfd);

        netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
        return inet_ntoa(netmask);
}
#endif
#endif


int check_valid(char *local_ip,char *remote_ip, char *lan_ip,char *wan_ip_1,char *wan_ip_2)
{
        /* Active session doesn't include the packets
         * a) from/to 127.0.0.1 or 0.0.0.0
         * b) from/to lan_ip of the router.
         * c) from/to wan_ip of the router.
         * d) from/to physical wan_ip of the router in russia mode.
         */

        if( !strcmp(local_ip,"127.0.0.1") || !strcmp(local_ip,"0.0.0.0") || !strcmp(remote_ip,"127.0.0.1")||!strcmp(remote_ip,"0.0.0.0"))
                return 1;

        else if( !strcmp(local_ip,lan_ip)  || !strcmp(remote_ip,lan_ip) )
                return 1;

        else if( strlen(wan_ip_1) && (!strcmp(local_ip,wan_ip_1) || !strcmp(remote_ip,wan_ip_1)))
                return 1;

#ifdef RPPPOE

        else if( strlen(wan_ip_2) && (!strcmp(local_ip,wan_ip_2) || !strcmp(remote_ip,wan_ip_2)))
                return 1;
#endif

        else
                return 0;
}

char* get_protocol_name(char *pro_number)
{
        switch (atoi(pro_number))
        {
                case 1:
                        return "ICMP";
                case 2:
                        return "IGMP";
                case 3:
                        return "GGP";
                case 4:
                        return "IP";
                case 5:
                        return "ST";
                case 6:
                        return "TCP";
                case 7:
                        return "CBT";
                case 8:
                        return "EGP";
                case 9:
                        return "IGP";
                case 17:
                        return "UDP";
                case 47:
                        return "GRE";
                default:
                        return "UNKONWN";
        }
}
/*
 * If the ip has been in igmp table, return 0
 * Otherwise, return 1.
 */
int add_igmp_table(char igmp_list[][16],char *igmp_ip,int *number)
{
        int i=0;
        int index=*number;

        for(i=0;i<index;i++)
                if(!strcmp(igmp_list[i],igmp_ip))
                        return 0;
        strcpy(igmp_list[index],igmp_ip);
        *number = ++index;

        return 1;
}
/*
 * All session is based on /proc/net/ip_conntrack. All packets sending to the router will be recorded in the file.
 * INPUT PARAM:
 *      proto_flag      -       Which kind of protocol we need to gather information about?
 *                                      1, we need to gather information for ONLY TCP and UDP.
 *                                      2, we need to gather information for ALL.
 *      packet_type     -       What status of packets we need to gather information about?
 *                                      ALL: all kinds of session excepts for [UNREPLIED] session
 *                              ESTABLISHED: only ESTABLISHED session for TCP and all session but [UNREPLIED] for UDP and ICMP
 *
 *
 * linux 2.4 ************************************************************************************************************
 *      st[0]  st[1]  st[2]      st[3]            st[4]             st[5]       st[6]                   st[7]            st[8]            st[9]        st[10]
 * [ name |proto |time  |src_ip          |dst_ip           |sport      |dport      |reply      |src_ip2          |dst_ip2        |sport2      |dport2 ]
 *   udp   17     40     src=192.168.0.3  dst=172.21.3.250      sport=2000  dport=1900  [UNREPLIED] src=172.21.3.250  dst=172.21.3.1  sport=1900   dport=1111
 *
 *      st[0]  st[1]  st[2]     st[3]        st[4]              st[5]           st[6]       st[7]                    st[8]            st[9]             st[10]       st[11]
 * [ name |proto |time |state       |src_ip            |dst_ip         |sport      |dport       |reply      |src_ip2         |dst_ip2          |sport2      |dport2 ]
 *   tcp   6      7500  ESTABLISHED      src=172.21.3.250       dst=172.21.3.1  sport=1900  dport=1111   [UNREPLIED] src=192.168.0.3  dst=172.21.3.250  sport=2000   dport=1900
 *
 *
 * linux 2.6 ************************************************************************************************************
 *      st[0]  st[1]  st[2]      st[3]            st[4]             st[5]       st[6]       st[7]      st[8]                  st[9]             st[10]          st[11]       st[12]
 * [ name |proto |time  |src_ip          |dst_ip           |sport      |dport      |packets   |bytes    |reply       |src_ip2          |dst_ip2        |sport2      |dport2 ]
 *   udp   17     40     src=192.168.0.3  dst=172.21.3.250      sport=2000  dport=1900  packets=1  bytes=72 [UNREPLIED]   src=172.21.3.250  dst=172.21.3.1  sport=1900   dport=1111
 *
 *      st[0]  st[1]  st[2]     st[3]        st[4]              st[5]           st[6]       st[7]        st[8]      st[9]                  st[10]           st[11]            st[12]       st[13]
 * [ name |proto |time |state       |src_ip            |dst_ip         |sport      |dport       |packets   |bytes     |reply      |src_ip2         |dst_ip2          |sport2      |dport2 ]
 *   tcp   6      7500  ESTABLISHED      src=172.21.3.250       dst=172.21.3.1  sport=1900  dport=1111   packets=5  bytes=883 [UNREPLIED]  src=192.168.0.3  dst=172.21.3.250  sport=2000   dport=1900
 *
 **************************************************************************************************************************
 */

T_NAPT_LIST_INFO* get_napt_session(int proto_flag, char *packet_type)
{
        FILE *fp;
        char buf[256]={0}, state[4]={0}, lan_ip[16]={0}, lan_mask[16]={0}, wan_ip_1[16]={0},wan_ip_2[16]={0};
        char *st[MAX_NUMBER_FIELD];
        char *local_ip=NULL, *remote_ip=NULL, *local_port=NULL, *remote_port=NULL, *public_port=NULL;
        char *tmp_ip=NULL, *tmp_ip_1=NULL, *tmp_ip_2=NULL, *tmp_mask=NULL;
        int direction_out=0, i=0;
        struct in_addr netmask, myaddr, saddr, igmp_reserve_group, igmp_reserve_mask;
        char *p=NULL,*src_ip=NULL,*dst_ip=NULL,*protocol=NULL;
        int has_src_ip=0,has_dst_ip=0;

        /*      Date:   2009-08-03
         *      Name:   jimmy huang
         *      Reason: Avoid rc crashed due to invalid memory access ()
         *      Note:   due to not initialized to NULL, add_session_to_table() won't malloc for the very first entry
         *                      But when free_session_node(), will free the very first entry...
         */
        session_list[0].next=NULL;

        inet_aton("224.0.0.1", &igmp_reserve_group);
        inet_aton("255.255.255.0", &igmp_reserve_mask);

        napt_session_list.num_list=0;


        if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 ))
        {
                tmp_ip_1 = get_ipaddr( nvram_safe_get("wan_eth") );
                if(tmp_ip_1)
                        strncpy(wan_ip_1,tmp_ip_1,strlen(tmp_ip_1));
        }
        else
        {
                tmp_ip_1 = get_ipaddr( "ppp0" );
                if(tmp_ip_1)
                        strncpy(wan_ip_1,tmp_ip_1,strlen(tmp_ip_1));

#ifdef RPPPOE
                if ((( nvram_match("wan_proto", "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 ))
                        ||(( nvram_match("wan_proto", "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 ))
                        ||(( nvram_match("wan_proto", "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 ))
                        )
                {
                        tmp_ip_2 = get_ipaddr( nvram_safe_get("wan_eth") );
                        if(tmp_ip_2)
                                strncpy(wan_ip_2,tmp_ip_2,strlen(tmp_ip_2));
                }
#endif
        }

        if (!(strlen(wan_ip_1)) && !(strlen(wan_ip_2)))
        {
                //printf("wan_ipaddr == NULL, ACTIVE session don't start\n");
                return NULL;
        }


        /* lan_ip=NULL implies that there is no active session */
        if(tmp_ip=nvram_safe_get("lan_ipaddr"))
        {
                strcpy(lan_ip,tmp_ip);
                inet_aton(lan_ip, &myaddr);
        }
        else
                return NULL;

        if(tmp_mask=nvram_safe_get("lan_netmask"))
        {
                strcpy(lan_mask,tmp_mask);
                inet_aton(lan_mask, &netmask);
        }
        else
                return NULL;

        /* open file */
    fp = fopen(IP_CONNTRACK_FILE, "r");
    if (fp == (FILE *) 0)
    {
                printf("Open %s record file FAIL\n", IP_CONNTRACK_FILE);
                return NULL;
    }

    /* parse file */
        while (fgets(buf, sizeof(buf),  fp) != NULL)
        {
                split(buf, 32, st);
                switch (atoi(st[1]))
                {
                        case 6:         /* TCP ptotocol */
                                if ((!strcmp(packet_type,"ESTABLISHED") && !strcmp(st[3], "ESTABLISHED"))
                                        || !strcmp(packet_type,"ALL"))
                                {
                                        if(!strcmp(st[3],"[UNREPLIED]"))
                                        {
                                                //printf("DEBUG:st[3]=[UNREPLIED], so BREAK\n");
                                                break;
                                        }
                                        if (!strcmp(st[3], "NONE"))
                                                strcpy(state,"NO");
                                        else if (!strcmp(st[3], "SYN_SENT"))
                                                strcpy(state,"SS");
                                        else if (!strcmp(st[3], "SYN_RECV"))
                                                strcpy(state,"SR");
                                        else if (!strcmp(st[3], "ESTABLISHED"))
                                                strcpy(state,"EST");
                                        else if (!strcmp(st[3], "FIN_WAIT"))
                                                strcpy(state,"FW");
                                        else if (!strcmp(st[3], "CLOSE_WAIT"))
                                                strcpy(state,"CW");
                                        else if (!strcmp(st[3], "TIME_WAIT"))
                                                strcpy(state,"TW");
                                        else if (!strcmp(st[3], "LAST_ACK"))
                                                strcpy(state,"LA");
                                        else if (!strcmp(st[3], "CLOSE"))
                                                strcpy(state,"CL");

                                        tmp_ip = strchr(st[4], '=');
                                        tmp_ip++;
                                        inet_aton(tmp_ip, &saddr);

                                        /* from LAN */
                                        if( (netmask.s_addr & myaddr.s_addr) == (netmask.s_addr & saddr.s_addr))
                                        {
                                                direction_out = 1;
                                                local_ip = strchr(st[4], '=');
                                                remote_ip = strchr(st[5], '=');
                                                local_port = strchr(st[6], '=');
                                                remote_port = strchr(st[7], '=');
                                                public_port = strchr(st[11], '=');
                                        }
                                        /* from WAN */
                                        else
                                        {
                                                direction_out = 0;
                                                local_ip = strchr(st[8], '=');
                                                local_port = strchr(st[10], '=');
                                                remote_ip = strchr(st[4], '=');
                                                remote_port = strchr(st[6], '=');
                                                public_port = strchr(st[7], '=');
                                        }

                                        if (local_ip == NULL || remote_ip == NULL || lan_ip == NULL  || wan_ip_1 == NULL || wan_ip_2 == NULL)
                                                break;

                                        if (check_valid(local_ip+1,remote_ip+1,lan_ip,wan_ip_1,wan_ip_2) )
                                                break;

                                        protocol=get_protocol_name(st[1]);
                                        add_session_to_table( st[2], state, direction_out, local_ip+1, remote_ip+1, local_port+1, remote_port+1, public_port+1, protocol);
                                }
                                break;

                        case 17:        /* UDP ptotocol */
                                if (strcmp(st[7],"[UNREPLIED]"))
                                {
                                        tmp_ip = strchr(st[3], '=');
                                        tmp_ip++;
                                        inet_aton(tmp_ip, &saddr);

                                        /* from LAN */
                                        if ((netmask.s_addr & myaddr.s_addr) == (netmask.s_addr & saddr.s_addr))
                                        {
                                                direction_out = 1;
                                                local_ip = strchr(st[3], '=');
                                                remote_ip = strchr(st[4], '=');
                                                local_port = strchr(st[5], '=');
                                                remote_port = strchr(st[6], '=');
                                                public_port = strchr(st[10], '=');
                                        }
                                        /* from WAN */
                                        else
                                        {
                                                direction_out = 0;
                                                local_ip = strchr(st[7], '=');
                                                local_port = strchr(st[9], '=');
                                                remote_ip = strchr(st[3], '=');
                                                remote_port = strchr(st[5], '=');
                                                public_port = strchr(st[6], '=');
                                        }

                                        if (local_ip == NULL || remote_ip == NULL || lan_ip == NULL  || wan_ip_1 == NULL || wan_ip_2 == NULL)
                                                break;

                                        if (check_valid(local_ip+1,remote_ip+1,lan_ip,wan_ip_1,wan_ip_2))
                                                break;

                                        protocol=get_protocol_name(st[1]);
                                        add_session_to_table( st[2], NULL, direction_out, local_ip+1, remote_ip+1, local_port+1, remote_port+1, public_port+1, protocol);
                                }
                                break;

                        default:
                                if (proto_flag == 2)
                                {
                                        has_src_ip=0;
                                        has_dst_ip=0;

                                        /* Get src IP and dst IP*/
                                        for(i=0;i<MAX_NUMBER_FIELD;i++)
                                        {
                                                if(strlen(st[i]))
                                                {
                                                        if(has_src_ip==0 && (p=strstr(st[i],"src")))
                                                        {
                                                                src_ip = p+4;
                                                                has_src_ip=1;
                                                                continue;
                                                        }
                                                        if(has_dst_ip==0 && (p=strstr(st[i],"dst")))
                                                        {
                                                                dst_ip = p+4;
                                                                has_dst_ip=1;
                                                                continue;
                                                        }
                                                }
                                        }

                                        if (has_src_ip && has_dst_ip)
                                        {
                                                tmp_ip = src_ip;
                                                inet_aton(tmp_ip, &saddr);

                                                /* from LAN */
                                                if( (netmask.s_addr & myaddr.s_addr) == (netmask.s_addr & saddr.s_addr))
                                                {

                                                        direction_out = 1;
                                                        local_ip = src_ip;
                                                        remote_ip = dst_ip;
                                                }
                                                /* from WAN */
                                                else
                                                {
                                                        direction_out = 0;
                                                        local_ip = dst_ip;
                                                        remote_ip = src_ip;
                                                }

                                                if (check_valid(local_ip,remote_ip,lan_ip,wan_ip_1,wan_ip_2))
                                                {
                                                        break;
                                                }
                                                protocol=get_protocol_name(st[1]);
                                                add_session_to_table( st[2], NULL, direction_out, local_ip, remote_ip , NULL, NULL, NULL, protocol);
                                        }
                                }
                                break;
                }
        }

        /* close file */
        fclose(fp);
        napt_session_list.p_st_napt_list = session_list;
        return &napt_session_list;
}


void free_session_node(void)
{
        napt_session_info_t* current_node = NULL;
        napt_session_info_t* next_node = NULL;

                current_node = session_list[0].next;
                strcpy(session_list[0].local_ip," ");
                session_list[0].tcp_count = 0;
                session_list[0].udp_count = 0;
                session_list[0].last = NULL;
                session_list[0].next = NULL;

                while( current_node != NULL )
                {
                        //printf("CHUN:free_session_node\n");
                        next_node = current_node->next;
                        free(current_node);
                        current_node = next_node;
                }

}

#ifdef CONFIG_USER_TC
/*
 * detected_line_type()
 *	(Ubicom Streamengine) Return detected line type, 1 for frame relay, 0 for no frame relay. -1 for error/not detected
 */
int detected_line_type(void)
{
	FILE *fp;
	int type = -1;
	char returnVal[32] = {0};

#if CONFIG_USER_STREAMENGINE
	fp = fopen("/sys/devices/system/ubicom_streamengine/ubicom_streamengine0/ubicom_streamengine_frame_relay","r");
	if (fp) {
		fread(returnVal, sizeof(returnVal), 1, fp);
		DEBUG_MSG("%s", returnVal);
		if(strlen(returnVal) >= 1) {
			sscanf(returnVal, "%d", &type);
		}

		fclose(fp);
	}
#endif
	DEBUG_MSG("%d", type);
	return type;
}

unsigned int read_dhcpc_lease_time(void){
	char string[20] = {};
	unsigned int lease = 0;
	FILE *fp = NULL;

	fp = fopen(DHCPC_LEASE_FILE,"r");
	if(fp == NULL)
		return 0;

	if (fgets(string,sizeof(string),fp)){
		lease = atol(string);
	}

	fclose(fp);

	return lease;
}

void measure_uplink_bandwidth(char *bandwidth)
{
        FILE *fp;
        char returnVal[32] = {0};

/*
 *      Date: 2009-9-25
 *      Name: Gareth Williams <gareth.williams@ubicom.com>
 *      Reason: Streamengine support
 */
#if CONFIG_USER_STREAMENGINE
        fp = fopen("/sys/devices/system/ubicom_streamengine/ubicom_streamengine0/ubicom_streamengine_calculated_rate","r");
#else
        fp = fopen("/var/tmp/bandwidth_result.txt","r");
#endif
        if (fp)
        {
                fread(returnVal, sizeof(returnVal), 1, fp);
                DEBUG_MSG("%s", returnVal);
                if(strlen(returnVal) > 1)
/*
 *      Date: 2009-9-25
 *      Name: Gareth Williams <gareth.williams@ubicom.com>
 *      Reason: Streamengine support
 */
#if CONFIG_USER_STREAMENGINE
                {
                        unsigned long int kbps;
                        sscanf(returnVal, "%lu", &kbps);
                        kbps /= 1000;           /* bps to kbps */
                        sprintf(bandwidth, "%lu", kbps);
                }
#else
                        strcpy(bandwidth, returnVal);
#endif
                else
                        strcpy(bandwidth, "0");
                fclose(fp);
        }
        else
        {
                strcpy(bandwidth, "0");
        }

        return ;
}

/* return 1 if we have wan_ip, otherwise return 0 */
int wan_ip_is_obtained(void)
{
        char *tmp_wan_ip1=NULL, *tmp_wan_ip2=NULL;
        char wan_ipaddr[2][16]={0};
        char status[16];
        char *wan_proto=NULL;

        wan_proto = nvram_safe_get("wan_proto");

        /* Check phy connect statue */
        VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
        if( !strncmp("disconnect", status, 10) )
                return 0;

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 ) || ( nvram_match("wan_proto", "usb3g_phone") ==0 ))
#else
        if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 ))
#endif
        {
                tmp_wan_ip1 = get_ipaddr( nvram_safe_get("wan_eth") );
                if(tmp_wan_ip1)
                        strncpy(wan_ipaddr[0],tmp_wan_ip1,strlen(tmp_wan_ip1));
        }
        else
        {
                tmp_wan_ip1 = get_ipaddr( "ppp0" );
                if(tmp_wan_ip1)
                        strncpy(wan_ipaddr[0],tmp_wan_ip1,strlen(tmp_wan_ip1));

                /* if Russia is enabled */
                if ((( nvram_match("wan_proto", "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 ))
                ||(( nvram_match("wan_proto", "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 ))
                ||(( nvram_match("wan_proto", "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 )))
                {
                        tmp_wan_ip2 = get_ipaddr( nvram_safe_get("wan_eth") );
                        if(tmp_wan_ip2)
                                strncpy(wan_ipaddr[1],tmp_wan_ip2,strlen(tmp_wan_ip2));
                }
        }

        if(!(strlen(wan_ipaddr[0])) && !(strlen(wan_ipaddr[1])))
        {
                printf("wan_ipaddr == NULL, can't measure bandwidth\n");
                return 0;
        }
        else
                return 1;
}

/* return 1 if we have not measure uplink before, otherwise return 0 */
int not_measure_yet()
{
        FILE *fp = NULL;

#if CONFIG_USER_STREAMENGINE
        char rate[32] = {0};
        char code[32] = {0};

        fp = fopen("/sys/devices/system/ubicom_streamengine/ubicom_streamengine0/ubicom_streamengine_start_estimation","r");
        if (!fp) {
                /*
                 * Module not loaded or failed: We cannot say whether we have measured or not
                 * with any certainty so we return 0 to say we have - it avoids trying to re-run in the event of failures.
                 */
                return 0;
        }

        /*
         * If we have a non-zero measured bandwidth them we have reasured.
         */
        measure_uplink_bandwidth(rate);
        if (atoi(rate) != 0) {
                fclose(fp);
                return 0;       /* Measured */
        }

        /*
         * No measured bandwidth but if ubicom_streamengine_start_estimation reports error then we have tried and failed.
         */
        fread(code, sizeof(code), 1, fp);
        fclose(fp);
        if (atoi(code) < 0) {
                return 0;               /* We tried and failed so we have attempted a measurement */
        }

        /*
         * No measured rate and not attempted before
         */
        return 1;
#else
        fp = fopen("/var/tmp/bandwidth_result.txt","r");
        if(!fp) {
                return 1;
        }
        fclose(fp);
        return 0;
#endif
}

int if_measure_now(void)
{
/* jimmy modified 20080722, when wan is disconned , UI do not show router is measuring bandwidth now */
        char status[15] = {0};
        VCTGetPortConnectState(nvram_safe_get("wan_eth"),VCTWANPORT0,status);
/* ------------------------------------------------------------------------------------------ */

        if((!nvram_match("traffic_shaping", "1") && !nvram_match("auto_uplink", "1"))
/* jimmy modified 20080722, when wan is disconned , UI do not show router is measuring bandwidth now */
//              && not_measure_yet() )
                && not_measure_yet() && (strncmp("disconnect", status, 10) != 0) )
/* ------------------------------------------------------------------------------------------ */
                return 1;
        else
                return 0;

}
#endif

/*      Date: 2009-01-20
*       Name: jimmy huang
*       Reason: for usb-3g detect status
*       Note:   0: disconnected
*                       1: connected
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
int is_3g_device_connect(void){
        FILE *fp = NULL;
        char usbProductID[10];
        char usbVendorID[10];
        //usb_3g_device_table_t *p = NULL;
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        int usb_type = 0;
#endif
        char *pid_file_path = NULL;
        char *vid_file_path = NULL;
        DEBUG_MSG("Start %s\n",__FUNCTION__);

        memset(usbProductID,'\0',sizeof(usbProductID));
        memset(usbVendorID,'\0',sizeof(usbVendorID));

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        usb_type = atoi(nvram_safe_get("usb_type"));
/*      Date:   2010-02-05
 *      Name:   Cosmo Chang
 *      Reason: add usb_type=5 for Android Phone RNDIS feature
 */
        if(usb_type == 3 || usb_type == 4 || usb_type == 5){
                pid_file_path = USB_DEVICE_ProductID;
                vid_file_path = USB_DEVICE_VendorID;
        }else{
#endif
                pid_file_path = PPP_3G_PID_PATH;
                vid_file_path = PPP_3G_VID_PATH;
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
        }
#endif
        if((fp=fopen(vid_file_path,"r"))!=NULL){
                fgets(usbVendorID,sizeof(usbVendorID),fp);
                fclose(fp);
        }
        else
        {
                printf("%s: open %s fail\n",__FUNCTION__,PPP_3G_VID_PATH);
                return 0;
        }
        if((fp = fopen(pid_file_path,"r"))!=NULL){
                fgets(usbProductID,sizeof(usbProductID),fp);
                fclose(fp);
        }
        else
        {
                printf("%s: open %s fail\n",__FUNCTION__,PPP_3G_PID_PATH);
                return 0;
        }

        if((strlen(usbProductID) < 2) || (strlen(usbVendorID) < 2)){
                return 0;
        }

        DEBUG_MSG("%s, usb device connected, vid = %s, pid = %s\n",__FUNCTION__,usbVendorID,usbProductID);
/*
        for(p = usb_3g_device_table; strncmp(p->vendorID,"NULL",4) ;p++)
        {
                DEBUG_MSG("%s, supported vid = %s, pid = %s\n",__FUNCTION__,p->vendorID,p->productID);
                if((strncmp(usbVendorID,p->vendorID,strlen(p->vendorID))==0)
                                && (strncmp(usbProductID,p->productID,strlen(p->productID))==0))
                {
                        DEBUG_MSG("%s, Found !\n",__FUNCTION__);
                                return 1;
                }
        }
*/
/*      Date: 2009-01-22
*       Name: jimmy huang
*       Reason: Marked the codes below  because we only chcek if any USB device is plugged in,
*                       do not check supported vid and pid
*       Note:

*/
        return 1;
}

/*      Date:   2010-01-22
*       Name:   jimmy huang
*       Reason: Add support for Windows Mobile 5
*                       Because WM5 will OFFER 192.168.0.0/24 which is confilict with our lan ip range
*                       Thus we have to force to change our lan ip to 192.168.99.0/24
*/
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
int change_ip_class_c(char *old_ip,int class_c_new_value,char *new_ip){
        int ip_a,ip_b,ip_c,ip_d;
        if ( old_ip )
        {
                sscanf ( old_ip,"%d.%d.%d.%d",&ip_a,&ip_b,&ip_c,&ip_d );
        }
        else
        {
                return 0;
        }
        sprintf ( new_ip,"%d.%d.%d.%d",ip_a,ip_b,class_c_new_value,ip_d );
        return 1;
}
#endif

#endif //CONFIG_USER_3G_USB_CLIENT



