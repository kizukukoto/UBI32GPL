
static struct items tools_admin[] = {
	{ "admin_password", NULL},
	{ "user_password", NULL},
	{ "hostname", "DIR-665"},
	{ "graph_enable", "0"},
	{ "session_timeout", "60"},
	{ "remote_http_management_enable", ""},
	{ "remote_http_management_port", "8080"},
	{ "remote_http_management_inbound_filter", "Allow_All"},
	{ "remote_ssl", NULL},
	{ "ssl_remote_ipaddr", NULL},
	{ "sslport", "443"},
	{ "https_pem", NULL},
	{ NULL, NULL}
};


static struct items tools_ddns[] = {
	{ "ddns_enable", "0"},
	{ "ddns_status", ""},
	{ "ddns_dynds_kinds", ""},
	{ "ddns_type",""},
	{ "ddns_hostname", ""},
	{ "ddns_username", ""},
	{ "ddns_password", ""},
	{ "ddns_sync_interval", "576"},
	{ NULL, NULL}
};

static struct items_range tools_schedules[] = {
	{ "log_email_schedule",-1,-1,""},
	{ "schedule_rule_0", 0, 9, ""},
	{ "schedule_rule_", 10, 31, ""},
	{ "port_forward_both_0", 0, 9, ""},
	{ "port_forward_both_", 10, 23, ""},
	{ "application_0", 0, 9, ""},
	{ "application_", 10, 23, ""},
	{ "vs_rule_", 0, 9, ""},
	{ "vs_rule_", 10, 23, ""},
	{ "wlan0_schedule",-1,-1,""},
	{ NULL, -1, -1, NULL}
};
struct items timeset[] = {
	{ "time_zone", ""},
	{ "time_zone_area", ""},
	{ "time_daylight_saving_enable",""},
	{ "time_daylight_saving_start_day_of_week",""},
	{ "time_daylight_saving_end_day_of_week",""},
	{ "time_daylight_saving_start_month",""},
	{ "time_daylight_saving_end_month",""},
	{ "time_daylight_saving_start_week",""},
	{ "time_daylight_saving_end_week",""},
	{ "time_daylight_saving_start_time",""},
	{ "time_daylight_saving_end_time",""},
	{ "ntp_client_enable", NULL},
	{ "ntp_server", NULL},
	{ "system_time", "2002/01/01/00/00/00"},
	{ NULL, NULL}
};
static struct items tools_email[] = {
	{ "log_email_schedule", NULL},
	{ "log_email_enable", NULL},
	{ "log_email_sender" , NULL },
	{ "log_email_recipien", NULL},
	{ "log_email_server", NULL},
	{ "log_email_server_port", "25"},
	{ "log_email_auth", NULL},
	{ "log_email_username", NULL},
	{ "log_email_password", NULL},
	{ "log_current_page", NULL},
	{ "log_total_page", NULL},
	{ "log_system_activity", NULL},
	{ "log_debug_information", NULL},
	{ "log_attacks", NULL},
	{ "log_dropped_packets", NULL},
	{ "log_notice", NULL},
	{ NULL, NULL},
};

static struct items tools_syslog[] = {
	{ "syslog_server",NULL},
	{ NULL, NULL},
};

