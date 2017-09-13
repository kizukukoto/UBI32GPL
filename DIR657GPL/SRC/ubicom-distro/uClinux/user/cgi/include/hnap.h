#ifndef __HNAP_H__
#define __HNAP_H__

#define HNAP_RC_FLAG		"/var/etc/rc_flag.conf"
#define HNAP_CGI_ENTRY		"hnap.cgi"
#define HNAP_CGI_ENTRY_PATH	"/www/"HNAP_CGI_ENTRY
#define HNAP_CGI_FIFO_TO_MISC   "/tmp/hnap_fifo_to_misc"
#define HNAP_CGI_FIFO_FROM_MISC "/tmp/hnap_fifo_from_misc"
#define HNAP_CGI_MISC		"/bin/hnap_main &"
#define HNAP_RESPONSE_FILE	"hnap_response.xml"
#define HNAP_RESPONSE_PAGE	"/tmp/"HNAP_RESPONSE_FILE
#define HNAP_RESPONSE_PATH	"/www/"HNAP_RESPONSE_FILE

typedef struct hnap_entry_t {
	const char *tag;
	const char *key;
	int (*fn)(const char *key, char *);
} hnap_entry_s;

extern int do_xml_create_mime(char *);	/* lib/libhnap.c */

extern char *unknown_call;
extern char *get_device_settings_result;
extern char *reboot_result;
extern char *getrouter_lansettings_result;
extern char *setdevice_settings_result;
extern char *is_device_ready_result;
extern char *setrouter_lansettings_result;
extern char *get_wlan_setting24_result;
extern char *set_wlan_setting24_result;
extern char *set_wlan_security_result;
extern char *get_wlan_security_result;
extern char *WideChannels_24GHZ;
extern char *RadioInfo_24GHZ;
extern char *SecurityInfo;
extern char *get_wlan_radios_result;
extern char *get_wlan_radio_settings_result;
extern char *set_wlan_radio_settings_result;
extern char *get_wlan_radio_security_result;
extern char *set_wLan_radio_security_result;
extern char *get_wan_settings_result;
extern char *set_wan_settings_result;
extern char *renew_wan_connection_result;
extern char *get_port_mappings_result;
extern char *port_mapping_result;
extern char *add_port_mapping_result;
extern char *delete_port_mapping_result;
extern char *get_macfilters2_result;
extern char *set_macfilters2_result;
extern char *mac_info;
extern char *get_connected_devices_result;
extern char *cnted_client;
extern char *get_network_stats_result;

#endif
