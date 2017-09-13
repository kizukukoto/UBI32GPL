struct items_range iptables[] = {
	{ "app_port", "", NV_RESET, 0, 24},
	{ "forward_port", "", NV_RESET, 0, 99},
	{ "fw_forward_port", "", NV_RESET, 0, 24},
	{ "firewall_rule", "", NV_RESET, 0, 75},
	{ "filter_client0", "", NV_RESET},
	{ "filter_maclist", "", NV_RESET},
	{ "filter_macmode", "disabled", NV_RESET },
	{ "fw_vpn_conn", "", NV_RESET, 0, 25},
	{ "url_domain_filter_", "", NV_RESET, 0 , 49},
	{ "url_domain_filter_schedule_", "", NV_RESET, 0, 49},
	{ "url_domain_filter_type", "0", NV_RESET },    /*  */
	{ "mac_filter", "", NV_RESET, 0, 100},
	{ "vpn_conn", "", NV_RESET, 0, 99},     /* VPN connection rules */
        { "vpn_extra", "", NV_RESET, 0, 99},    /* VPN connection rules */
	{ NULL, NULL}
};
