
//"25-110>192.168.0.101:25-110,tcp,on,PortForward,02"
//"25-110>192.168.0.101:25-110,tcp,on,PortForward,02"
//	"enable,device,DIP1,proto,port>device,DIP2,proto,port,name,schedule"
//	"1,WAN,192.168.0.1,tcp,22:66,77:99:200>LAN,192.168.0.2,11:22-44,Ex_virtual,always"
//

static struct items_range adv_virtual[] = {
	{ "vs_rule_0", 0, 9, ""},
	{ "vs_rule_", 10, 23, ""},
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_portforward[] = {
	{ "port_forward_both_0", 0, 9, ""},
	{ "port_forward_both_", 10, 20, ""},
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_firewall[] = {
	{ "spi_enable", -1, -1, "1"},
	{ "udp_nat_type", -1, -1, "2"},
	{ "tcp_nat_type", -1, -1, "1"},
	{ "anti_spoof_check", -1, -1, "0"},
	{ "dmz_enable", -1, -1, "0"},
	{ "dmz_ipaddr", -1, -1, "0.0.0.0"},
	{ "pptp_pass_through", -1, -1, "1"},
	{ "ipsec_pass_through", -1, -1, "1"},
	{ "alg_rtsp", -1, -1, "1"},
	{ "alg_sip", -1, -1, "1"},
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
	{ "application_0", 0, 9, NULL},
	{ "application_", 10, 23, NULL},
	{ NULL, -1, -1, NULL}
};

static struct items adv_filters_mac[] = {
	{ "mac_filter_type", "0"},
	{ "mac_filter_00", NULL},
	{ "mac_filter_01", NULL},
	{ "mac_filter_02", NULL},
	{ "mac_filter_03", NULL},
	{ "mac_filter_04", NULL},
	{ "mac_filter_05", NULL},
	{ "mac_filter_06", NULL},
	{ "mac_filter_07", NULL},
	{ "mac_filter_08", NULL},
	{ "mac_filter_09", NULL},
	{ "mac_filter_10", NULL},
	{ "mac_filter_11", NULL},
	{ "mac_filter_12", NULL},
	{ "mac_filter_13", NULL},
	{ "mac_filter_14", NULL},
	{ "mac_filter_15", NULL},
	{ "mac_filter_16", NULL},
	{ "mac_filter_17", NULL},
	{ "mac_filter_18", NULL},
	{ "mac_filter_19", NULL},
	{ "mac_filter_20", NULL},
	{ "mac_filter_21", NULL},
	{ "mac_filter_22", NULL},
	{ "mac_filter_23", NULL},
	{ "mac_filter_24", NULL},
	{ NULL, NULL}
};

static struct items_range adv_filters_url[] = {
	{ "url_domain_filter_type", -1, -1, ""},
	{ "url_domain_filter_0", 0, 9, ""},
	{ "url_domain_filter_", 10, 39, ""},
	{ NULL, NULL}
};

static struct items static_route[] = {
	{ "static_routing_00", ""},
	{ "static_routing_01", ""},
	{ "static_routing_02", ""},
	{ "static_routing_03", ""},
	{ "static_routing_04", ""},
	{ "static_routing_05", ""},
	{ "static_routing_06", ""},
	{ "static_routing_07", ""},
	{ "static_routing_08", ""},
	{ "static_routing_09", ""},
	{ "static_routing_10", ""},
	{ "static_routing_11", ""},
	{ "static_routing_12", ""},
	{ "static_routing_13", ""},
	{ "static_routing_14", ""},
	{ "static_routing_15", ""},
	{ "static_routing_16", ""},
	{ "static_routing_17", ""},
	{ "static_routing_18", ""},
	{ "static_routing_19", ""},
	{ "static_routing_20", ""},
	{ "static_routing_21", ""},
	{ "static_routing_22", ""},
	{ "static_routing_23", ""},
	{ "static_routing_24", ""},
	{ "static_routing_25", ""},
	{ "static_routing_26", ""},
	{ "static_routing_27", ""},
	{ "static_routing_28", ""},
	{ "static_routing_29", ""},
	{ "static_routing_30", ""},
	{ "static_routing_31", ""},
	{ "static_routing_32", ""},
	{ "static_routing_33", ""},
	{ "static_routing_34", ""},
	{ "static_routing_35", ""},
	{ "static_routing_36", ""},
	{ "static_routing_37", ""},
	{ "static_routing_38", ""},
	{ "static_routing_39", ""},
	{ "static_routing_40", ""},
	{ "static_routing_41", ""},
	{ "static_routing_42", ""},
	{ "static_routing_43", ""},
	{ "static_routing_44", ""},
	{ "static_routing_45", ""},
	{ "static_routing_46", ""},
	{ "static_routing_47", ""},
	{ "static_routing_48", ""},
	{ "static_routing_49", ""},
	{ NULL, NULL}
};

static struct items_range policy_route[] = {
	{ "policy_rule", 0, 24, NULL},
	{ NULL, -1, -1, NULL}
};

static struct items_range url_domain_filter[] = {
	{ "url_domain_filter_0", 0, 9, ""},
	{ "url_domain_filter_", 10, 39, ""},
	{ "url_domain_filter_type", -1, -1, "0"},
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

static struct items adv_network[] = {
	{ "upnp_enable", "0"},
	{ "wan_port_ping_response_enable", "0"},
	{ "wan_port_ping_response_inbound_filter", NULL},
	{ "multicast_stream_enable", "0"},
	{ "wan_port_speed", ""},
	{ NULL, NULL}
};

static struct items_range user_group_edit[] = {
	{ "auth_group", 1, 6, NULL},
	{ NULL, -1, -1, NULL}
};

static struct items_range inbound_filter[] = {
	{ "inbound_filter_name_0", 0, 9, NULL},
	{ "inbound_filter_name_", 10, 23, NULL},
	{ "inbound_filter_ip_00_A", -1, -1, ""},
	{ "inbound_filter_ip_01_A", -1, -1, ""},
	{ "inbound_filter_ip_02_A", -1, -1, ""},
	{ "inbound_filter_ip_03_A", -1, -1, ""},
	{ "inbound_filter_ip_04_A", -1, -1, ""},
	{ "inbound_filter_ip_05_A", -1, -1, ""},
	{ "inbound_filter_ip_06_A", -1, -1, ""},
	{ "inbound_filter_ip_07_A", -1, -1, ""},
	{ "inbound_filter_ip_08_A", -1, -1, ""},
	{ "inbound_filter_ip_09_A", -1, -1, ""},
	{ "inbound_filter_ip_11_A", -1, -1, ""},
	{ "inbound_filter_ip_11_A", -1, -1, ""},
	{ "inbound_filter_ip_12_A", -1, -1, ""},
	{ "inbound_filter_ip_13_A", -1, -1, ""},
	{ "inbound_filter_ip_14_A", -1, -1, ""},
	{ "inbound_filter_ip_15_A", -1, -1, ""},
	{ "inbound_filter_ip_16_A", -1, -1, ""},
	{ "inbound_filter_ip_17_A", -1, -1, ""},
	{ "inbound_filter_ip_18_A", -1, -1, ""},
	{ "inbound_filter_ip_19_A", -1, -1, ""},
	{ "inbound_filter_ip_20_A", -1, -1, ""},
	{ "inbound_filter_ip_21_A", -1, -1, ""},
	{ "inbound_filter_ip_22_A", -1, -1, ""},
	{ "inbound_filter_ip_23_A", -1, -1, ""},
	{ "inbound_filter_ip_00_B", -1, -1, ""},
	{ "inbound_filter_ip_01_B", -1, -1, ""},
	{ "inbound_filter_ip_02_B", -1, -1, ""},
	{ "inbound_filter_ip_03_B", -1, -1, ""},
	{ "inbound_filter_ip_04_B", -1, -1, ""},
	{ "inbound_filter_ip_05_B", -1, -1, ""},
	{ "inbound_filter_ip_06_B", -1, -1, ""},
	{ "inbound_filter_ip_07_B", -1, -1, ""},
	{ "inbound_filter_ip_08_B", -1, -1, ""},
	{ "inbound_filter_ip_09_B", -1, -1, ""},
	{ "inbound_filter_ip_11_B", -1, -1, ""},
	{ "inbound_filter_ip_11_B", -1, -1, ""},
	{ "inbound_filter_ip_12_B", -1, -1, ""},
	{ "inbound_filter_ip_13_B", -1, -1, ""},
	{ "inbound_filter_ip_14_B", -1, -1, ""},
	{ "inbound_filter_ip_15_B", -1, -1, ""},
	{ "inbound_filter_ip_16_B", -1, -1, ""},
	{ "inbound_filter_ip_17_B", -1, -1, ""},
	{ "inbound_filter_ip_18_B", -1, -1, ""},
	{ "inbound_filter_ip_19_B", -1, -1, ""},
	{ "inbound_filter_ip_20_B", -1, -1, ""},
	{ "inbound_filter_ip_21_B", -1, -1, ""},
	{ "inbound_filter_ip_22_B", -1, -1, ""},
	{ "inbound_filter_ip_23_B", -1, -1, ""},
	{ NULL, -1, -1, NULL}
};

static struct items_range qos_rule[] = {
	{ "qos_rule_0", 0, 9, ""},
	{ "qos_rule_", 10, 10, ""},
	{ "traffic_shaping", -1, -1, "1"},
	{ "auto_uplink", -1, -1, "1"},
	{ "qos_enable", -1, -1, "1"},
	{ "qos_uplink", -1, -1, ""},
	{ "auto_classification", -1, -1, "1"},
	{ "qos_dyn_fragmentation", -1, -1, "1"},
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_wish[] = {
	{ "wish_rule_0", 0, 9, ""},
	{ "wish_rule_", 10, 23, ""},
	{ "wish_engine_enabled", -1, -1, "1"},
	{ "wish_http_enabled", -1, -1, "1"},
	{ "wish_media_enabled", -1, -1, "1"},
	{ "wish_auto_enabled", -1, -1, "1"},
	{ NULL, -1, -1, NULL}
};

static struct items_range adv_access_control[] = {
	{ "access_control_filter_type", -1, -1, "disable"},
	{ "access_control_0", 0, 9, ""},
	{ "access_control_", 10, 14, ""},
	{ "firewall_rule_0", 0, 9, ""},
	{ "firewall_rule_", 10, 119, ""},
	{ NULL, -1, -1, NULL}
};

static struct items adv_ipv6_static[] = {
	{"ipv6_autoconfig_type", NULL},
	{"ipv6_static_wan_ip", NULL},
	{"ipv6_static_lan_ip", ""},
        {"ipv6_static_prefix_length", NULL},
        {"ipv6_static_default_gw", ""},
        {"ipv6_static_primary_dns", ""},
        {"ipv6_static_secondary_dns", ""},
	{ "ipv6_autoconfig", "1"},
	{ "ipv6_dhcpd_start", NULL},
	{ "ipv6_dhcpd_end", NULL},
	{ "ipv6_wan_proto", NULL},
	{ "ipv6_ra_adv_valid_lifetime_l_one", NULL},
	{ "ipv6_ra_adv_prefer_lifetime_l_one", NULL},
	{ "ipv6_dhcpd_lifetime", NULL},
	{ "ipv6_wan_specify_dns", NULL},
	{ "ipv6_use_link_local", NULL},
	{ NULL, NULL}
};

static struct items adv_ipv6_6in4[] = {
	{ "ipv6_autoconfig_type", NULL},
	{ "ipv6_autoconfig", "1"},
	{ "ipv6_dhcp_pd_enable", "1"},
	{ "ipv6_6in4_dns_enable", "1"},
	{ "ipv6_dhcpd_start", NULL},
	{ "ipv6_dhcpd_end", NULL},
	{ "ipv6_wan_proto", NULL},
	{ "ipv6_ra_adv_valid_lifetime_l_one", NULL},
	{ "ipv6_ra_adv_prefer_lifetime_l_one", NULL},
	{ "ipv6_dhcpd_lifetime", NULL},
	{ "ipv6_wan_specify_dns", ""},
        { "ipv4_6in4_remote_ip", ""},
        { "ipv6_6in4_remote_ip", ""},
        { "ipv4_6in4_wan_ip", ""},
        { "ipv6_6in4_wan_ip", ""},
        { "ipv6_6in4_primary_dns", ""},
        { "ipv6_6in4_secondary_dns", ""},
        { "ipv6_6in4_lan_ip", ""},
	{ NULL, NULL}
};

static struct items adv_ipv6_6to4[] = {
	{ "ipv6_autoconfig_type", NULL},
	{ "ipv6_autoconfig", "1"},
	{ "ipv6_dhcpd_start", NULL},
	{ "ipv6_dhcpd_end", NULL},
	{ "ipv6_wan_proto", NULL},
	{ "ipv6_ra_adv_valid_lifetime_l_one", NULL},
	{ "ipv6_ra_adv_prefer_lifetime_l_one", NULL},
	{ "ipv6_dhcpd_lifetime", NULL},
	{ "ipv6_wan_specify_dns", ""},
	{ "ipv6_6to4_primary_dns", ""},
        { "ipv6_6to4_secondary_dns", ""},
        { "ipv6_6to4_lan_ip", ""},
        { "ipv6_6to4_lan_ip_subnet", ""},
        { "ipv6_6to4_relay", ""},
	{ NULL, NULL}
};

static struct items adv_link_local[] = {
	{ "ipv6_wan_proto", ""},
	{ NULL, NULL}
};

static struct items adv_ipv6_poe[] = {
	{ "ipv6_autoconfig_type", NULL},
	{ "ipv6_autoconfig", ""},
	{ "ipv6_dhcp_pd_enable", ""},
	{ "ipv6_dhcp_pd_enable_l", ""},
	{ "ipv6_pppoe_dns_enable", ""},
	{ "ipv6_pppoe_password", NULL},
	{ "ipv6_dhcpd_start", NULL},
	{ "ipv6_dhcpd_end", NULL},
	{ "ipv6_wan_proto", ""},
	{ "ipv6_ra_adv_valid_lifetime_l_one", NULL},
	{ "ipv6_ra_adv_prefer_lifetime_l_one", NULL},
	{ "ipv6_dhcpd_lifetime", NULL},
	{ "ipv6_wan_specify_dns", ""},
	{ "ipv6_pppoe_idle_time", NULL},
	{ "ipv6_pppoe_share", ""},
        { "ipv6_pppoe_dynamic", ""},
        { "ipv6_pppoe_ipaddr", NULL},
        { "ipv6_pppoe_username", NULL},
        { "ipv6_pppoe_service", NULL},
        { "ipv6_pppoe_connect_mode", NULL},
        { "ipv6_pppoe_mtu", NULL},
        { "ipv6_pppoe_primary_dns", NULL},
        { "ipv6_pppoe_secondary_dns", NULL},
        { "ipv6_pppoe_lan_ip", ""},
	{ NULL, NULL}
};

static struct items adv_ipv6_autodetect[] = {
	{ "ipv6_pppoe_share", NULL},
	{ "ipv6_autoconfig_type", NULL},
	{ "ipv6_autoconfig", ""},
   	{ "ipv6_dhcp_pd_enable", ""},
	{ "ipv6_dhcp_pd_enable_l", ""},
	{ "ipv6_autodetect_dns_enable", ""},
	{ "ipv6_dhcpd_start", NULL},
	{ "ipv6_dhcpd_end", NULL},
	{ "ipv6_wan_proto", ""},
	{ "ipv6_ra_adv_valid_lifetime_l_one", NULL},
	{ "ipv6_ra_adv_prefer_lifetime_l_one", NULL},
	{ "ipv6_dhcpd_lifetime", NULL},
	{ "ipv6_wan_specify_dns", ""},
        { "ipv6_autodetect_primary_dns", ""},
        { "ipv6_autodetect_secondary_dns", ""},
        { "ipv6_autodetect_lan_ip", ""},
	{ NULL, NULL}
};

static struct items adv_ipv6_autoconfig[] = {
	{ "ipv6_autoconfig_type", NULL},
	{ "ipv6_autoconfig", ""},
   	{ "ipv6_dhcp_pd_enable", ""},
	{ "ipv6_dhcp_pd_enable_l", ""},
	{ "ipv6_autoconfig_dns_enable", ""},
	{ "ipv6_dhcpd_start", NULL},
	{ "ipv6_dhcpd_end", NULL},
	{ "ipv6_wan_proto", ""},
	{ "ipv6_ra_adv_valid_lifetime_l_one", NULL},
	{ "ipv6_ra_adv_prefer_lifetime_l_one", NULL},
	{ "ipv6_dhcpd_lifetime", NULL},
	{ "ipv6_wan_specify_dns", ""},
        { "ipv6_autoconfig_primary_dns", ""},
        { "ipv6_autoconfig_secondary_dns", ""},
        { "ipv6_autoconfig_lan_ip", ""},
	{ NULL, NULL}
};

static struct items adv_ipv6_6rd[] = {
	{ "ipv6_autoconfig_type", NULL},
	{ "ipv6_6rd_primary_dns", ""},
	{ "ipv6_6rd_secondary_dns", ""},
	{ "ipv6_6rd_prefix", ""},
	{ "ipv6_6rd_prefix_length", ""},
	{ "ipv6_6rd_lan_ip", ""},
	{ "ipv6_6rd_relay", ""},
	{ "ipv6_6rd_dhcp", ""},
        { "ipv6_autoconfig", ""},
        { "ipv6_dhcpd_start", NULL},
        { "ipv6_dhcpd_end", NULL},
        { "ipv6_wan_proto", ""},
        { "ipv6_6rd_ipv4_mask_length", ""},
	{ "ipv6_ra_adv_valid_lifetime_l_one", NULL},
	{ "ipv6_ra_adv_prefer_lifetime_l_one", NULL},
	{ "ipv6_dhcpd_lifetime", NULL},
        { "ipv6_wan_specify_dns", ""},
        { NULL, NULL}
};

static struct items adv_ipv6_firewall[] = {
	{"firewall_rule_ipv6_00", ""},
	{"firewall_rule_ipv6_01", ""},
	{"firewall_rule_ipv6_02", ""},
	{"firewall_rule_ipv6_03", ""},
	{"firewall_rule_ipv6_04", ""},
	{"firewall_rule_ipv6_05", ""},
	{"firewall_rule_ipv6_06", ""},
	{"firewall_rule_ipv6_07", ""},
	{"firewall_rule_ipv6_08", ""},
	{"firewall_rule_ipv6_09", ""},
	{"firewall_rule_ipv6_10", ""},
	{"firewall_rule_ipv6_11", ""},
	{"firewall_rule_ipv6_12", ""},
	{"firewall_rule_ipv6_13", ""},
	{"firewall_rule_ipv6_14", ""},
	{"firewall_rule_ipv6_15", ""},
	{"firewall_rule_ipv6_16", ""},
	{"firewall_rule_ipv6_17", ""},
	{"firewall_rule_ipv6_18", ""},
	{"firewall_rule_ipv6_19", ""},
	{"ipv6_filter_type", ""},
        { NULL, NULL}
};

static struct items adv_ipv6_routing[] = {
	{"static_routing_ipv6_00", ""},
	{"static_routing_ipv6_01", ""},
	{"static_routing_ipv6_02", ""},
	{"static_routing_ipv6_03", ""},
	{"static_routing_ipv6_04", ""},
	{"static_routing_ipv6_05", ""},
	{"static_routing_ipv6_06", ""},
	{"static_routing_ipv6_07", ""},
	{"static_routing_ipv6_08", ""},
	{"static_routing_ipv6_09", ""},
        { NULL, NULL}
};

static struct items adv_opendns[] = {
	{ "en_opendns", "1"},
	{ "device_id", NULL},
	{ NULL, NULL}
};

