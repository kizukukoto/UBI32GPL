
//"25-110>192.168.0.101:25-110,tcp,on,PortForward,02"
//"25-110>192.168.0.101:25-110,tcp,on,PortForward,02"
//	"enable,device,DIP1,proto,port>device,DIP2,proto,port,name,schedule"
//	"1,WAN,192.168.0.1,tcp,22:66,77:99:200>LAN,192.168.0.2,11:22-44,Ex_virtual,always"
//	

/* adv_ipv6_firewall and adv_ipv6_6rd don't need on DIR-130/330.
 * we must give an empty structure here for main/public.c
 *
 * 2010/7/15 FredHung */

static struct items adv_ipv6_firewall[] = {
	{ NULL, NULL }
};

static struct items adv_ipv6_6rd[] = {
	{ NULL, NULL }
};

static struct items_range adv_virtual[] = {
	{ NULL, -1, -1, NULL}

};
static struct items_range adv_portforward[] = {
	{ "forward_port", 0, 47, "" },
	{ "fw_forward_port", 0, 24, "" },
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_firewall[] = {
	{ "dmz_ipaddr", -1, -1, "0.0.0.0"},
	{ "dmz_enable", -1, -1, "0"},
	{ "dmz_display", -1, -1, "0.0.0.0"},
	{ "dmz_schedule", -1, -1, "always"},
	{ "pptp_pt", -1, -1, "0"},
	{ "l2tp_pt", -1, -1, "0"},
	{ "ipsec_pt", -1, -1, "0"},
	{ "firewall_rule", 0, 74, ""},
	{ "fw_forward_port", 0, 24, ""},
	{ "fw_vpn_conn", 0, 24, ""},
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_bandwidth[] = {
        { "bw_enable", -1, -1, "0"},
        { "bw_uplink", -1, -1, "10000"},
        { "bw_downlink", -1, -1, "10000"},
        { "bw_ctrl_", 0, 24, ""},
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_appl[] = {
	{ "app_port", 0, 24, NULL},
	{ NULL, -1, -1, NULL}
};

static struct items adv_filters_mac[] = {
	{ "filter_macmode", NULL},
	{ "filter_maclist", ""},
	{ NULL, NULL}
};

static struct items_range adv_filters_url[] = {
	{ NULL, NULL}
};

static struct items static_route[] = {
	{ "static_route", ""},
	{ NULL, NULL}
};

static struct items_range policy_route[] = {
	{ "policy_rule", 0, 24, NULL},
	{ NULL, -1, -1, NULL}
};

static struct items_range url_domain_filter[] = {
	{ "url_domain_filter_type", -1, -1, "0"},
	{ "url_domain_filter_", 0, 49, ""},
	{ "url_domain_filter_schedule_", 0, 49, ""},
	{ NULL, -1, -1, NULL}
};

static struct items dynamic_route[] = {
	{"rip_enable", NULL},	
	{"rip_tx_version", NULL},
	{"rip_rx_version", NULL},
	{"ospf_enable", NULL},	
	{"ospf_if_area0", NULL},
	{"ospf_if_area1", NULL},
	{"ospf_if_area2", NULL},
	{"ospf_if_cost0", NULL},	
	{"ospf_if_cost1", NULL},	
	{"ospf_if_cost2", NULL},	
	{"ospf_if_priority0", NULL},
	{"ospf_if_priority1", NULL},
	{"ospf_if_priority2", NULL},
	{ NULL, NULL}
};
static struct items_range qos_rule[] = {
	/* Qos Engine Setup*/
	{ "is_enable_qos", -1, -1, "1"},
	{ "qos_auto_classif", -1, -1, "1"},
	{ "qos_dynamic_frag", -1, -1, "1"},
	/* Qos Weight Setting */
	{ "qos_weight_rule", -1, -1, NULL},
	/* Qos Rule Setting */
	{ "qos_rule", 1, 10, "0"},
	{ NULL, -1, -1, NULL}
};
static struct items adv_network[] = {
	{ "wan0_ping_enable", "0"},
	{ "upnp_enable", "0"},
	{ "igmpsnooping", "0"},
	{ "wan_port_speed", "0"},
	{ NULL, NULL}
};

static struct items_range adv_access_control[] = {
	{ NULL, -1, -1, NULL}
};

static struct items_range user_group_edit[] = {
	{ "auth_group", 1, 6, NULL},
	{ NULL, -1, -1, NULL}
};

static struct items_range inbound_filter[] = {
	{ "inbound_list_", 0, 10, NULL,},
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_wish[] = {
	{ NULL, -1, -1, NULL}
};

static struct items guest_zone[] = {
	{ NULL, NULL}
};

static struct items adv_ipv6_static[] = {
	{ NULL, NULL}
};

static struct items adv_ipv6_autodetect[] = {
	{ NULL, NULL}
};

static struct items adv_ipv6_autoconfig[] = {
	{ NULL, NULL}
};

static struct items adv_ipv6_6in4[] = {
	{ NULL, NULL}
};

static struct items adv_ipv6_6to4[] = {
	{ NULL, NULL}
};

static struct items adv_link_local[] = {
	{ NULL, NULL}
};

static struct items adv_ipv6_poe[] = {
	{ NULL, NULL}
};

static struct items adv_ipv6_routing[] = {
	{ NULL, NULL}
};
static struct items adv_opendns[] = {
	{ NULL, NULL}
};

