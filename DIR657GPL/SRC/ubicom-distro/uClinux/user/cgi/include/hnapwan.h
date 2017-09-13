#ifndef __HNAPWAN_H__
#define __HNAPWAN_H__

#define IPV4_IP_LEN     16
#define RTF_UP  0x0001  /* route usable */
#define RTF_GATEWAY     0x0002
#define PATH_PROCNET_ROUTE       "/proc/net/route"
#define NVRAM_FILE "/var/etc/nvram.conf"
#define MAC_LENGTH  17
#define POE_NAME_LEN  64
#define POE_PASS_LEN 32

typedef struct wan_settings_s{
        char *protocol;
        char *name;
        char *password;
        char *idle_time;
        char *service;
        char *mode;                             // always_on | manual | on_demand , not support on_demand
        char *ip;
        char *netmask;
        char *gateway;
        char *primary_dns;
        char *secondary_dns;
        char *mtu;
        char *mac;
        char *dynamic;
#ifdef OPENDNS
        char *opendns;
#endif
} wan_settings_t;

wan_settings_t wan_settings;

typedef struct pppoe_pptp_l2tp_s{
        char *mode;
        char *name;
        char *password;
        char *idle_time;
        char *service;    // in pptp,l2tp is server ip 
        char *ip;
        char *netmask;
        char *gateway;
        char *dynamic;
} pppoe_pptp_l2tp_t;

typedef struct routing_table_s{
	char dest_addr[16];
	char dest_mask[16];
	char gateway[16];
	char interface[10];
	int metric;
	int type;
	int creator;
} routing_table_t;
#endif
