#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "hnap.h"

extern int do_xml_response(const char *);
extern int do_xml_mapping(const char *, hnap_entry_s *);
extern int do_xml_create_mime(char *);

extern int do_unknown_call(const char *, char *);
extern int do_isdevice_ready(const char *, char *);
extern int do_getdevice_settings(const char *, char *);
extern int do_reboot(const char *, char *);
extern int do_getrouter_lansettings(const char *, char *);
extern int do_setdevice_settings(const char *, char *);
extern int do_setrouter_lansettings(const char *, char *);
extern int do_get_wlan_setting24(const char *, char *);
extern int do_set_wlan_setting24(const char *,char *);
extern int do_get_wlan_security(const char *,char *);
extern int do_set_wlan_security(const char *,char *);
extern int do_get_wlan_radios(const char *,char *);
extern int do_get_wlan_radio_settings(const char *,char *);
extern int do_set_wlan_radio_settings(const char *,char *);
extern int do_get_wlan_radio_security(const char *,char *);
extern int do_set_wlan_radio_security(const char *,char *);
extern int do_get_wan_settings(const char *,char *);
extern int do_set_wan_settings(const char *,char *);
extern int do_renew_wan_connection(const char *,char *);
extern int do_get_port_mappings(const char *,char *);
extern int do_add_port_mappings(const char *,char *);
extern int do_delete_port_mapping(const char *,char *);
extern int do_get_macfilters2(const char *,char *);
extern int do_set_macfilters2(const char *,char *);
extern int do_get_connected_devices(const char *,char *);
extern int do_get_network_stats(const char *,char *);

int do_xml_main(char *xraw)
{
	hnap_entry_s ls[] = {
		{ "GetDeviceSettings",		NULL,	do_getdevice_settings },
		{ "IsDeviceReady",		NULL,	do_isdevice_ready },
		{ "Reboot",			NULL,	do_reboot },
		{ "GetRouterLanSettings",	NULL,	do_getrouter_lansettings },
		{ "SetDeviceSettings",		NULL,	do_setdevice_settings },
		{ "SetRouterLanSettings",	NULL,	do_setrouter_lansettings },
		{ "GetWLanSettings24",		NULL,	do_get_wlan_setting24 },
		{ "SetWLanSettings24",		NULL,	do_set_wlan_setting24 },
		{ "GetWLanSecurity",		NULL,	do_get_wlan_security },
		{ "SetWLanSecurity",		NULL,	do_set_wlan_security },
		{ "GetWLanRadios",		NULL,	do_get_wlan_radios },
		{ "GetWLanRadioSettings",	NULL,	do_get_wlan_radio_settings },
		{ "SetWLanRadioSettings",	NULL,	do_set_wlan_radio_settings },
		{ "GetWLanRadioSecurity",	NULL,	do_get_wlan_radio_security },
		{ "SetWLanRadioSecurity",	NULL,	do_set_wlan_radio_security },
		{ "GetWanSettings",		NULL,	do_get_wan_settings },
		{ "SetWanSettings",		NULL,	do_set_wan_settings },
		{ "RenewWanConnection",		NULL,	do_renew_wan_connection },
		{ "GetPortMappings",		NULL,	do_get_port_mappings },
		{ "AddPortMapping",		NULL,	do_add_port_mappings },
		{ "DeletePortMapping",		NULL,	do_delete_port_mapping },
		{ "GetMACFilters2",		NULL,	do_get_macfilters2 },
		{ "SetMACFilters2",		NULL,	do_set_macfilters2 },
		{ "GetConnectedDevices",	NULL,	do_get_connected_devices },
		{ "GetNetworkStats",		NULL,	do_get_network_stats },
		{ NULL, NULL, do_unknown_call },
		{ NULL, NULL, NULL}
	};

	return do_xml_mapping(xraw, ls);
}
