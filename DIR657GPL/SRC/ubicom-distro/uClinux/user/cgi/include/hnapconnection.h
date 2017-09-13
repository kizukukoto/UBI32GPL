#ifndef __HNAPCONNECTION_H__
#define __HNAPCONNECTION_H__
#include "libdhcpd.h"
#include "hnapfirewall.h"

#define MAX_RES_LEN_XML 4096 
#define MAX_RES_LEN         10000
#define MAX_DHCPD_RESERVATION_NUMBER  25
#define M_MAX_STATION 30
#define M_MAX_DHCP_LIST 254
#define UDHCPD_PID                    "/var/run/udhcpd.pid"

typedef struct wireless_stations_table_s{
        char *mac_addr;
        char *rate;
        char *rssi;
        char *mode;
        char *connected_time;
        char *ip_addr;
        char list[64];
        char info[64];
}wireless_stations_table_t;


typedef struct
{
        int num_list;
        struct dhcpd_leased_table_s *p_st_dhcpd_list;          //array of dhcpd_leased_table_s info
} T_DHCPD_LIST_INFO;


typedef struct
{
        int num_sta;
        wireless_stations_table_t *p_st_wlan_sta;               //array of wireless_stations_table_t info
}T_WLAN_STAION_INFO;

typedef struct cnted_client_list_s{
        char *cnted_time;
        char *mac_add;
        char *dev_name;
        char *port_name;
        char *wireless;
        char *active;
}cnted_client_list_t;

extern int do_xml_response(const char *);
extern int do_xml_mapping(const char *, hnap_entry_s *);

#endif
