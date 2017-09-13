
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
	{ "admin_password", ""},
	{ "sel_time_zone", NULL},
	{ "wan0_proto", NULL},
	{ "mac_clone_addr", NULL},
	{ "wan0_hostname", ""},
	{ "wan0_static_ipaddr", NULL},
	{ "wan0_static_netmask", NULL},
	{ "wan0_static_gateway", NULL},
	{ "wan0_primary_dns", ""},
	{ "wan0_secondary_dns", NULL},
	{ "wan0_pppoe_dynamic", NULL},
        { "wan0_pppoe_username", NULL},
	{ "wan0_pppoe_passwd", NULL},
	{ "wan0_pppoe_service", ""},
	{ "wan0_pptp_dynamic", NULL},
	{ "wan0_pptp_server_ipaddr", NULL},
	{ "wan0_pptp_username", NULL},
	{ "wan0_pptp_passwd", NULL},
	{ "wan0_l2tp_dynamic", NULL},
	{ "wan0_l2tp_server_ipaddr", NULL},
	{ "wan0_l2tp_username", NULL},
	{ "wan0_l2tp_passwd", NULL},
	{ "wan0_bigpond_username", NULL},
	{ "wan0_bigpond_passwd", NULL},
	{ "wan0_bigpond_auth", "sm-server"},
	{ "wan0_bigpond_server", NULL},
	{ "wan0_ipaddr", "0.0.0.0"},
	{ "wan0_rupppoe_dynamic", NULL},
	{ "wan0_rustatic_ipaddr", NULL},
	{ "wan0_rupppoe_username", NULL},
	{ "wan0_rupppoe_passwd", NULL},
	{ "wan0_rupppoe_service", ""},
	{ "wan0_rupptp_dynamic", NULL},
	{ "wan0_rupptp_server_ipaddr", NULL},
	{ "wan0_rupptp_username", NULL},
	{ "wan0_rupptp_passwd", NULL},
	{ "wan0_russiapppoe_dynamic", NULL},
	{ NULL, NULL}
};

static struct items wan_static[] = {
	{ "mac_clone_addr", ""},
	{ "wan0_proto", NULL},
	{ "wan0_static_ipaddr", NULL},
	{ "wan0_static_netmask", NULL},
	{ "wan0_static_gateway", NULL},
	{ "wan0_primary_dns", ""},
	{ "wan0_secondary_dns", ""},
	{ "wan0_mtu", "1500"},
	{ "wan0_ipaddr", "0.0.0.0"},
	{ NULL, NULL }
};

static struct items wan_dhcp[] = {
	{ "mac_clone_addr", "" },
	{ "wan0_proto", NULL},
	{ "wan0_hostname", ""},
	{ "wan0_primary_dns", ""},
	{ "wan0_secondary_dns", ""},
	{ "wan0_mtu", "1500"},
	{ "wan0_ipaddr", "0.0.0.0"},
	{ NULL, NULL}
};

static struct items wan_poe[] = {
	{ "mac_clone_addr", "" },
	{ "wan0_proto", NULL },
	{ "wan0_pppoe_dynamic", NULL },
	{ "wan0_static_ipaddr", NULL },
	{ "wan0_pppoe_username", NULL },
	{ "wan0_pppoe_passwd", NULL },
	{ "wan0_pppoe_service", "" },
	{ "wan0_primary_dns", "" },
	{ "wan0_secondary_dns", "" },
	{ "wan0_pppoe_idletime", NULL },
	{ "wan0_pppoe_mtu", NULL },
	{ "wan0_pppoe_demand", NULL },
	{ "wan0_ipaddr", "0.0.0.0" },
	{ NULL, NULL }
};

static struct items wan_pptp[] = {
	{ "wan0_pptp_demand", NULL },
	{ "wan0_pptp_dynamic", NULL },
	{ "wan0_pptp_server_ipaddr", NULL },
	{ "wan0_pptp_username", NULL },
	{ "wan0_pptp_passwd", NULL },
	{ "wan0_pptp_idletime", NULL },
	{ "wan0_pptp_mtu", NULL },
	{ "wan0_pptp_encrypt", NULL },
	{ "wan0_proto", NULL },
	{ "wan0_static_ipaddr", NULL },
	{ "wan0_static_netmask", NULL },
	{ "wan0_static_gateway", NULL },
	{ "wan0_primary_dns", "" },
	{ "wan0_ipaddr", "0.0.0.0" },
	{ NULL, NULL }
};

static struct items wan_l2tp[] = {
	{ "wan0_l2tp_demand", NULL},
	{ "wan0_l2tp_dynamic", NULL},
	{ "wan0_l2tp_server_ipaddr", NULL},
	{ "wan0_l2tp_username", NULL},
	{ "wan0_l2tp_passwd", NULL},
	{ "wan0_l2tp_idletime", NULL},
	{ "wan0_l2tp_mtu", NULL},
	{ "wan0_proto", NULL},
	{ "wan0_static_ipaddr", NULL},
	{ "wan0_static_netmask", NULL},
	{ "wan0_static_gateway", NULL},
	{ "wan0_primary_dns", ""},
	{ "wan0_ipaddr", "0.0.0.0"},
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

static struct items wan_rupptp[] = {
	{ "mac_clone_addr", ""},
	{ "wan0_rupptp_demand", NULL},
	{ "wan0_rupptp_dynamic", NULL},
	{ "wan0_rupptp_server_ipaddr", NULL},
	{ "wan0_rupptp_username", NULL},
	{ "wan0_rupptp_passwd", NULL},
	{ "wan0_rupptp_idletime", NULL},
	{ "wan0_rupptp_mtu", NULL},
	{ "wan0_rupptp_encrypt", NULL},
	{ "wan0_proto", NULL},
	{ "wan0_static_ipaddr", NULL},
	{ "wan0_static_netmask", NULL},
	{ "wan0_static_gateway", NULL},
	{ "wan0_primary_dns", ""},
	{ "wan0_secondary_dns", ""},
	{ "wan0_ipaddr", "0.0.0.0"},
	{ NULL, NULL}
};

static struct items_range wan_multipoe[] = {
	{ "zone_if_proto2", -1, -1, "pppoe"},
	{ "zone_if_list2", -1, -1, "WAN_primary;WAN_slave"},
	//{ "mac_clone_addr", ""},
	{ "if_proto2", -1, -1, "pppoe"},
	{ "if_proto_alias2", -1, -1, "multi_pppoe_primary;multi_pppoe_secondary"},
	//{ "pppoe_hostname4", -1, -1, "multi_pppoe_primary"},
	{ "pppoe_mac4", -1, -1, ""},
	{ "pppoe_mode4", -1, -1, NULL},
	{ "pppoe_user4", -1, -1, NULL},
	{ "pppoe_pass4", -1, -1, NULL},
	{ "pppoe_servicename4", -1, -1, "multi_pppoe_primary"},
	{ "pppoe_addr4", -1, -1, NULL},
	//{ "pppoe_netmask4", -1, -1, NULL},
	{ "pppoe_primary_dns4", -1, -1, ""},
	{ "pppoe_secondary_dns4", -1, -1, ""},
	{ "pppoe_idle4", -1, -1, NULL},
	{ "pppoe_mtu4", -1, -1, NULL},
	{ "pppoe_dial4", -1, -1, NULL},
	{ "pppoe_unnum_enable4", -1, -1, "0"},
	{ "pppoe_unnum_ip4", -1, -1, ""},
	{ "pppoe_unnum_netmask4", -1, -1, ""},

	//{ "pppoe_hostname5", -1, -1, "multi_pppoe_primary"},
	{ "pppoe_mac5", -1, -1, ""},
	{ "pppoe_mode5", -1, -1, NULL},
	{ "pppoe_user5", -1, -1, NULL},
	{ "pppoe_pass5", -1, -1, NULL},
	{ "pppoe_servicename5", -1, -1, "multi_pppoe_secondary"},
	{ "pppoe_addr5", -1, -1, NULL},
	//{ "pppoe_netmask5", -1, -1, NULL},
	{ "pppoe_primary_dns5", -1, -1, ""},
	{ "pppoe_secondary_dns5", -1, -1, ""},
	{ "pppoe_idle5", -1, -1, NULL},
	{ "pppoe_mtu5", -1, -1, NULL},
	{ "pppoe_dial5", -1, -1, NULL},
	{ "pppoe_ntt5", -1, -1, NULL},
	{ "pppoe_policy_type5", -1, -1, NULL},

	{ "if_dev2", -1, -1, "ppp4"},		/* auto add */
	{ "if_services2", -1, -1, ""},
	{ "mpoe_policy_dest", 1, 10, ""},	/* Enable;Name;DestIP or Domain*/
	{ "mpoe_policy_proto", 1, 10, ""},	/* Enable;NAme;StartPort-EndPort;Proto(any,tcp,udp)*/
	{ NULL, -1, -1, NULL}
};

static struct items wan_usb3g[] = {
	{ "usb3g_alias2", "WAN_usb3g"},
        { "usb3g_hostname2", "WAN_l2tp"},
        { "usb3g_addr2", ""},
        { "usb3g_netmask2", ""},
        { "usb3g_gateway2", ""},
        { "usb3g_dns2", ""},
        { "usb3g_mtu2", "1492"},
        { "usb3g_mru2", "1496"},
        /* USB3G PRPTO PPPXXX */
        { "usb3g_user2", ""},
        { "usb3g_pass2", ""},
        { "usb3g_unit2", "0"},
        { "usb3g_dial2", "1"},        /* "0" manual, "1" always, "2" demand, "3" */
        { "usb3g_mode2", "1"},        /* "1" dynamic, "0" static*/
        { "usb3g_mac2", ""},
        { "usb3g_idle", "5"},
        { "usb3g_mppe2", "nomppe"},   /* require-mppe|nomppe */
        /* USB3G EXTRA PARAMS */
        { "usb3g_country2", ""},
        { "usb3g_isp2", ""},
        { "usb3g_dial_num2", ""},
        { "usb3g_apn_name2", ""},
	{ "usb3g_auth", "XXX"},
	{ "zone_if_proto2", "3G"},
	{ "if_dev2", "ppp2"},
	{ "if_proto2", "usb3g"},
	{ "if_proto_alias2", "WAN_usb3g"},
	{ "if_services2", PROTO_SINGLE_SERVICES},
	{ NULL, NULL}
};

static struct items wan_usb3gadapter[] = {
	{ NULL, NULL}
};

static struct items wan_usb3gphone[] = {
	{ NULL, NULL}
};
