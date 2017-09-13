struct items_range admin[] = { 
	{ "httpd_alias0", "httpd_LAN", NV_RESET},
	{ "httpd_wanif0", "eth1", NV_RESET},
	{ "httpd_cfg0", "/tmp/password", NV_RESET},
	{ "httpd_port0", "80", NV_RESET},
	{ "https_alias0", "https_REMOTE", NV_RESET},
	{ "https_lanif0", "br0", NV_RESET},
	{ "https_cfg0", "/tmp/https.conf", NV_RESET},
	{ "https_port0", "443", NV_RESET},
	{ "remote_enable", "0", NV_RESET},
	{ "httpd_authmethod", "DB", NV_RESET},
	{ "https_pem", "", NV_RESET},
	{ "inbound_filter", "Allow ALL", NV_RESET},
	{ "auth_enable", "0", NV_RESET},
	{ "auth_iprange", "", NV_RESET},
	{ "auth_group1", "Group1/", NV_RESET},
	{ "auth_group2", "Group2/", NV_RESET},
	{ "auth_group3", "Group3/", NV_RESET},
	{ "auth_group4", "Group4/", NV_RESET},
	{ "auth_group5", "Group5/", NV_RESET},
	{ "auth_group6", "Group6/", NV_RESET},
	{ "auth_group7", "Group7/", NV_RESET},
	{ "auth_group8", "Group8/", NV_RESET},
	{ "enable_ext_server", "0", NV_RESET},
	{ "ext_server_mode", "0", NV_RESET},
	{ "radius0", "", NV_RESET},
	{ "radius1", "", NV_RESET},
	{ "radio_RADIUS", "", NV_RESET},
	{ "ldap", "", NV_RESET},
	{ "ad", "", NV_RESET},
	{ "enable_wpa_ent", "0", NV_RESET},
	{ "log_server", "0" , NV_RESET },	/* ebanle/disable log to remote server */
	{ "log_ipaddr_", "", NV_RESET, 0, 8},	/* syslog recipient 0 to 8 */
	{ "auth_pptpd_l2tpd", "", NV_RESET },	/* pptpd user */
	{ "auth_user_auth", "", NV_RESET },	/* lan to wan auth */
	{ "auth_status", "", NV_RESET },	/* lan to wan auth */
	{ "ro_username", "user", 1 },		/* read only username */
	{ "ro_password", "", 1 },		/* read only password */
	{ "http_username", "admin", NV_RESET },	/* Username */
	{ "admin_password", "", NV_RESET },	/* Password */
	{ "http_wanport", "8080", NV_RESET },	/* WAN port to listen on */
	{ "remote_ssl", "0", NV_RESET },	/* allow admin page login from https */
	{ "session_timeout", "60", NV_RESET},	/* user login session timeout seconds */
	{ "wan_access_allow", "0", NV_RESET},	/* allow wan login */
	{ "dmz_access_allow", "0", NV_RESET},	/* allow dmz login */
	{ "lan_access_allow", "1", NV_RESET},	/* allow lan login, default accept */
	{ "wlan_access_allow", "0", NV_RESET},	/* allow wlan login */
	{ "graph_enable", "", NV_RESET},	/* empty: enable graphical auth, 'none': disabled */
	{NULL, NULL}
};
