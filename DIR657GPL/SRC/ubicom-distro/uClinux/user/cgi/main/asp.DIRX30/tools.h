
static struct items tools_admin[] = {
	{ "http_username", "YWRtaW4="},
	{ "admin_password", ""},
	{ "graph_enable", "none"},
	{ "session_timeout", "60"},
	{ "https_pem", ""},
	{ "http_group", ""},
	{ "http_group_enable", ""},
	{ "http_wanport", "80"},
	{ "remote_ssl", "0"},
	{ "remote_enable", "0"},
	{ "remote_schedule", "always"},
	{ "remote_ipaddr", "0.0.0.0"},
	{ "ssl_remote_ipaddr", "*" },
	{ "sslport", "443"},
	{ NULL, NULL}
};


static struct items tools_ddns[] = {
	{ "ddns_enable", "0"},
	{ "ddns_status", ""},
	{ "ddns_type", ""},
	{ "ddns_hostname", ""},
	{ "ddns_username", ""},
	{ "ddns_password", ""},
	{ "ddns_timeout", "864000"},
	{ NULL, NULL}
};

static struct items_range tools_schedules[] = {
	{ "schedule_rule", 0, 24, NULL},
	{ NULL, -1, -1, NULL}
};
struct items timeset[] = {
	{ "sel_time_zone", NULL},
	{ "daylight_en", NULL},
	{ "daylight_smonth", NULL},
	{ "daylight_sweek", NULL},
	{ "daylight_sday", NULL},
	{ "daylight_stime", NULL},
	{ "daylight_emonth", NULL},
	{ "daylight_eweek", NULL},
	{ "daylight_eday", NULL},
	{ "daylight_etime", NULL},
	{ "ntp_client_enable", NULL},
	{ "ntp_server", NULL},
	{ "system_time", "2002/01/01/00/00/00"},
	{ NULL, NULL}
};
static struct items tools_email[] = {
	{ "auth_active", "" },
	{ "auth_acname", "" },
	{ "auth_passwd", "" },
	{ "log_email_from" , "" },
	{ "log_email_server", "" },
	{ "log_email_port", "25" },
	{ "log_email_sender", "" },
	{ "log_system_activity", ""},
	{ "log_debug_information", ""},
	{ "log_attacks", ""},
	{ "log_dropped_packets", ""},
	{ "log_notice", ""},
	{ "log_level", ""},
	{ NULL, NULL},
};

static struct items tools_syslog[] = {
	/* log server setting */
	{ "log_server", NULL},
	{ "log_ipaddr_1", ""},
	{ "log_ipaddr_2", ""},
	{ "log_ipaddr_3", ""},
	{ "log_ipaddr_4", ""},
	{ "log_ipaddr_5", ""},
	{ "log_ipaddr_6", ""},
	{ "log_ipaddr_7", ""},
	{ "log_ipaddr_8", ""},
	/* log type setting */		
	{ "log_system_activity", NULL},
	{ "log_debug_information", NULL},
	{ "log_attacks", NULL},
	{ "log_dropped_packets", NULL},	
	{ "log_notice", NULL},	
	{ NULL, NULL},
};

