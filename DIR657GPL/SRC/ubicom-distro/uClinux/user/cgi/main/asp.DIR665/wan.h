
/*
 * Define servies the order of services
 * */
#define LAN_SERVICES						\
	"httpd,httpd_LAN,@if_dev,@Z_LOCALIP "			\
	"dhcpd,LAN_dhcpd,@if_dev,@Z_LOCALIP,@Z_LOCALMASK "	\
	"upnp,@if_dev,@Z_LOCALIP route,@if_dev,@if_route "	\
	"tftpd webfilter,@if_dev,@Z_LOCALIP,@Z_LOCALMASK "	\
	"wps inbound_filter"

#define DMZ_SERVICES						\
	"dhcpd,DMZ_dhcpd,@if_dev,@Z_LOCALIP,@Z_LOCALMASK "	\
	"route,@if_dev,@if_route inbound_filter"
/* STATIC DHCP PPPOE */
#define PROTO_SINGLE_SERVICES					\
	"ntp igmp ddns https,https_REMOTE,@if_dev,@Z_LOCALIP "	\
	"route,@if_dev,@if_route policy_route "			\
	"xl2tpd pptpd ipsec qos ips,@if_dev inbound_filter"	\
/* PPTP/L2TP, RU_PPTP/RU_PPPOE */
#define PROTO_DUAL0_SERVICES					\
	"https,https_REMOTE,@if_dev,@Z_LOCALIP "		\
	"netdevice,WAN_slave inbound_filter"

#define PROTO_DUAL1_SERVICES	PROTO_SINGLE_SERVICES

static struct items wizard_wan[] = {
	{ "time_zone_area", NULL},
	{ "wan_mac", NULL},
	{ "wan_proto", NULL},
	{ "wan_specify_dns", NULL},
	{ "admin_password", ""},
	{ "time_zone", NULL},
	{ "mac_clone_addr", NULL},
	{ "hostname", NULL},
	{ "wan_pppoe_ipaddr_00", NULL},
	{ "wan_pppoe_dynamic_00", NULL},
	{ "wan_pppoe_username_00", NULL},
	{ "wan_pppoe_password_00", NULL},
	{ "wan_pppoe_service_00", NULL},
	{ "wan_pptp_ipaddr", NULL},
	{ "wan_pptp_netmask", NULL},
	{ "wan_pptp_gateway", NULL},
	{ "wan_pptp_dynamic", NULL},
	{ "wan_pptp_server_ip", NULL},
	{ "wan_pptp_username", NULL},
	{ "wan_pptp_password", NULL},
	{ "wan_l2tp_ipaddr", NULL},
	{ "wan_l2tp_netmask", NULL},
	{ "wan_l2tp_gateway", NULL},
	{ "wan_l2tp_dynamic", NULL},
	{ "wan_l2tp_server_ip", NULL},
	{ "wan_l2tp_username", NULL},
	{ "wan_l2tp_password", NULL},
	{ "wan_static_ipaddr", NULL},
	{ "wan_static_netmask", NULL},
	{ "wan_static_gateway", NULL},
	{ "wan_primary_dns", NULL},
	{ "wan_secondary_dns", NULL},
	{ NULL, NULL}
};

static struct items wan_static[] = {
	{ "mac_clone_addr", ""},
	{ "wan_specify_dns", NULL},
	{ "lan_ipaddr", NULL},
	{ "lan_netmask", NULL},
	{ "usb_type", NULL},
	{ "from_usb3g", NULL},
	{ "wan_proto", NULL},
	{ "opendns_enable", NULL},
	{ "dns_relay", NULL},
	{ "wan_static_ipaddr", NULL},
	{ "wan_static_netmask", NULL},
	{ "wan_static_gateway", NULL},
	{ "wan_primary_dns", ""},
	{ "wan_secondary_dns", ""},
	{ "wan_mtu", "1500"},
	{ "wan_mac", NULL},
	{ NULL, NULL}
};

static struct items wan_dhcp[] = {
	{ "mac_clone_addr", ""},
	{ "wan_specify_dns", NULL},
	{ "dhcpc_use_ucast", NULL},
	{ "classless_static_route", NULL},
	{ "usb_type", NULL},
	{ "from_usb3g", NULL},
	{ "wan_proto", NULL},
	{ "opendns_enable", NULL},
	{ "dns_relay", NULL},
	{ "hostname", NULL},
	{ "dhcpc_use_ucast_sel", NULL},
	{ "wan_primary_dns", NULL},
	{ "wan_secondary_dns", NULL},
	{ "wan_mtu", "1500"},
	{ "wan_mac", NULL},
	{ NULL, NULL}
};

static struct items wan_poe[] = {
	{ "mac_clone_addr", ""},
	{ "wan_specify_dns", NULL},
	{ "wan_pppoe_password_00", NULL},
	{ "usb_type", NULL},
	{ "from_usb3g", NULL},
	{ "reboot_type", NULL},
	{ "wan_proto", NULL},
	{ "opendns_enable", NULL},
	{ "dns_relay", NULL},
	{ "wan_pppoe_ipaddr_00", NULL},
	{ "wan_pppoe_dynamic_00", NULL},
	{ "wan_pppoe_username_00", NULL},
	{ "poe_pass_s", NULL},
	{ "poe_pass_v", NULL},
	{ "wan_pppoe_service_00", NULL},
	{ "wan_pppoe_connect_mode_00", NULL},
	{ "wan_pppoe_max_idle_time_00", NULL},
	{ "wan_primary_dns", ""},
	{ "wan_secondary_dns", ""},
	{ "wan_pppoe_mtu", "1492"},
	{ "wan_mac", NULL},
	{ NULL, NULL }
};

static struct items wan_pptp[] = {
	{ "mac_clone_addr", ""},
	{ "wan_specify_dns", NULL},
	{ "wan_pptp_password", NULL},
	{ "usb_type", NULL},
	{ "from_usb3g", NULL},
	{ "reboot_type", NULL},
	{ "wan_proto", NULL},
	{ "opendns_enable", NULL},
	{ "dns_relay", NULL},
	{ "wan_pptp_ipaddr", NULL},
	{ "wan_pptp_netmask", NULL},
	{ "wan_pptp_gateway", NULL},
	{ "wan_pptp_dynamic", NULL},
	{ "wan_pptp_server_ip", NULL},
	{ "wan_pptp_username", NULL},
	{ "wan_pptp_connect_mode", NULL},
	{ "wan_pptp_max_idle_time", NULL},
	{ "wan_primary_dns", NULL},
	{ "wan_secondary_dns", NULL},
	{ "wan_pptp_mtu", NULL},
	{ "wan_mac", NULL},
	{ "wan_pptp_proto_client", NULL},
	{ "wan_pptp_encryption_client", NULL},
	{ NULL, NULL }
};

static struct items wan_l2tp[] = {
	{ "mac_clone_addr", ""},
	{ "wan_specify_dns", NULL},
	{ "wan_l2tp_password", NULL},
	{ "usb_type", NULL},
	{ "from_usb3g", NULL},
	{ "reboot_type", NULL},
	{ "wan_proto", NULL},
	{ "opendns_enable", NULL},
	{ "dns_relay", NULL},
	{ "wan_l2tp_ipaddr", NULL},
	{ "wan_l2tp_netmask", NULL},
	{ "wan_l2tp_gateway", NULL},
	{ "wan_l2tp_dynamic", NULL},
	{ "wan_l2tp_server_ip", NULL},
	{ "wan_l2tp_username", NULL},
	{ "wan_l2tp_connect_mode", NULL},
	{ "wan_l2tp_max_idle_time", NULL},
	{ "wan_primary_dns", NULL},
	{ "wan_secondary_dns", NULL},
	{ "wan_l2tp_mtu", NULL},
	{ "wan_mac", NULL},
	{ NULL, NULL}
};

static struct items wan_rupoe[] = {
	{ "mac_clone_addr", ""},
	{ "wan0_proto", NULL},
	{ "wan0_rupppoe_dynamic", NULL},
	{ "wan0_rustatic_ipaddr", NULL},
	{ "wan0_rupppoe_username", NULL},
	{ "wan0_rupppoe_passwd", NULL},
	{ "wan0_rupppoe_service", ""},
	{ "wan0_rupppoe_idletime", NULL},
	{ "wan0_rupppoe_mtu", NULL},
	{ "wan0_rupppoe_demand", NULL},
	{ "wan0_russiapppoe_dynamic", NULL},
	{ "wan0_static_ipaddr", NULL},
	{ "wan0_static_netmask", NULL},
	{ "wan0_static_gateway", NULL},
	{ "wan0_primary_dns", ""},
	{ "wan0_secondary_dns", ""},
	{ "wan0_ipaddr", "0.0.0.0"},
	{ NULL, NULL}
};

static struct items wan_usb3gadapter[] = {
	{ "usb3g_password", ""},
	{ "reboot_type", NULL},
	{ "usb3g_isp", NULL},
	{ "usb3g_country", NULL},
	{ "usb3g_auth", NULL},
	{ "usb_type", NULL},
	{ "from_usb3g", NULL},
	{ "wan_specify_dns", NULL},
	{ "qos_enable", NULL},
	{ "traffic_shaping", NULL},
	{ "wan_proto", NULL},
	{ "opendns_enable", NULL},
	{ "dns_relay", NULL},
	{ "get_data", NULL},
	{ "temp_country", NULL},
	{ "country", NULL},
	{ "ispList", NULL},
	{ "usb3g_username", NULL},
	{ "password", NULL},
	{ "usb3g_dial_num", NULL},
	{ "usb3g_apn_name", NULL},
	{ "usb3g_max_idle_time", NULL},
	{ "wan_primary_dns", NULL},
	{ "wan_secondary_dns", NULL},
	{ "usb3g_wan_mtu", NULL},
	{ NULL, NULL}
};

static struct items wan_usb3gphone[] = {
	{ "wan_specify_dns", NULL},
	{ "dhcpc_use_ucast", NULL},
	{ "asp_temp_52", NULL},
	{ "reboot_type", NULL},
	{ "from_usb3g", NULL},
	{ "wan_proto", NULL},
	{ "opendns_enable", NULL},
	{ "dns_relay", NULL},
	{ "usb_type", NULL},
	{ "hostname", NULL},
	{ "dhcpc_use_ucast_sel", NULL},
	{ "wan_primary_dns", NULL},
	{ "wan_secondary_dns", NULL},
	{ NULL, NULL}
};

static struct items_range wan_multipoe[] = {
	{ NULL, NULL}
};

static struct items_range wan_rupptp[] = {
	{ NULL, NULL}
};

static struct items_range wan_usb3g[] = {
	{ NULL, NULL}
};
