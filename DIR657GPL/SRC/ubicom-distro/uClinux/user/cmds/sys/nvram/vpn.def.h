struct items_range vpn[] = { 
	{ "vpn_nat_traversal", "1", NV_RESET},
	
	{ "vpn_conn", "", NV_RESET, 0, 100},
	{ "vpn_extra", "", NV_RESET, 0, 100},
	{ "sslvpn", "", NV_RESET, 1, 6},
	
	{ "l2tpd_enable", "0", NV_RESET},
	{ "l2tpd_servername", "", NV_RESET},
	{ "l2tpd_local_ip", "", NV_RESET},
	{ "l2tpd_remote_ip", "", NV_RESET},
	{ "l2tpd_auth_proto", "pap", NV_RESET},
	{ "l2tpd_auth_source", "local", NV_RESET},
	{ "l2tpd_authmethod", "LDAP", NV_RESET},
	{ "l2tpd_conn_", "", NV_RESET, 0, 25},
	{ "l2tpd_overipsec_enable", "0", NV_RESET},
	
	{ "pptpd_enable", "0", NV_RESET},
	{ "pptpd_servername", "", NV_RESET},
	{ "pptpd_local_ip", "", NV_RESET},
	{ "pptpd_remote_ip", "", NV_RESET},
	{ "pptpd_auth_proto", "pap", NV_RESET},
	{ "pptpd_encryption", "mppe-40", NV_RESET},
	{ "pptpd_auth_source", "local", NV_RESET},
	{ "pptpd_authmethod", "DB", NV_RESET},
	{ "pptpd_conn_", "", NV_RESET, 0, 25},
	
	{ "edit_vpn", "-1", NV_RESET},
	{ "edit_type", "-1", NV_RESET},
	
	{ "x509_", "", NV_RESET, 1, 24},
	{ "x509_ca_", "", NV_RESET, 1, 24},
	{NULL, NULL}
};
