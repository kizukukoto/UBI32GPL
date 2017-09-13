#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nvram.h>
#include "wlan.h"

extern void get_wep_key(int , char *, int , int , int );
extern void get_wep_default_key(int , char *, int );
extern void get_eap_mac_auth(char *, int , int );
extern void get_radius_server(char *, int , int , int );
extern void get_reauth_period(char *, int , int );
extern void get_rekey_time(char *, int , int );
extern void get_cipher(char *, int , int );
extern void get_passphrase(char *, int , int );
extern void get_ssid(char *, int , int );
extern void get_security(char *, int , int );
extern void get_radius_profile(const char *, char *, char *, char *);

void wsc_wps_setting(FILE *fp, int wps_enable, char *rekey_time)
{
	if(!wps_enable) { //guest zone do not support WPS
		if (atoi(rekey_time) < 120)
			rekey_time = "120";
		fprintf(fp, "wpa_group_rekey=%s\n", rekey_time);
		fprintf(fp, "wps_disable=1\n");
		fprintf(fp, "wps_upnp_disable=1\n");
	} else {
		fprintf(fp, "wps_disable=0\n");
		fprintf(fp, "wps_upnp_disable=0\n");
		fprintf(fp, "wps_version=0x10\n");
		fprintf(fp, "wps_auth_type_flags=0x0023\n");//capabilities of network authentication
		fprintf(fp, "wps_encr_type_flags=0x000f\n");//capabilities of network encryption
		fprintf(fp, "wps_conn_type_flags=0x01\n"); //ESS
		//fprintf(fp, "wps_config_methods=0x0082\n"); //supported configuration methods
		fprintf(fp, "wps_config_methods=0x0084\n"); //for pass WIN7 DTM 1.4

		if(atoi(nvram_safe_get("wps_configured_mode"))==1)
			fprintf(fp, "wps_configured=0\n");
		else
			fprintf(fp, "wps_configured=1\n");

		if(atoi(nvram_safe_get("wps_lock"))==1)
			fprintf(fp, "ap_setup_locked=1\n");
		else
			fprintf(fp, "ap_setup_locked=0\n");

		fprintf(fp, "wps_rf_bands=0x03\n");
		fprintf(fp, "wps_manufacturer=%s\n", nvram_safe_get("manufacturer"));
		fprintf(fp, "wps_model_name=%s\n", nvram_safe_get("model_name"));
		fprintf(fp, "wps_model_number=%s\n", nvram_safe_get("model_number"));
		fprintf(fp, "wps_serial_number=%s\n", nvram_safe_get("serial_number"));
		fprintf(fp, "wps_friendly_name=%s\n", nvram_safe_get("friendlyname"));
		fprintf(fp, "wps_manufacturer_url=%s\n", nvram_safe_get("manufacturer_url"));
		fprintf(fp, "wps_model_description=%s\n", nvram_safe_get("model_number"));
		fprintf(fp, "wps_model_url=%s\n", nvram_safe_get("model_url"));
		fprintf(fp, "wps_upc_string=upc string here\n");
		fprintf(fp, "wps_default_pin=%s\n", nvram_safe_get("wps_pin"));
		fprintf(fp, "wps_dev_category=6\n"); // Network Infrastructure
		fprintf(fp, "wps_dev_sub_category=1\n"); //AP
		fprintf(fp, "wps_dev_oui=0050f204\n");
		/*
		the length of dev_name must be less than 32 bytes
		this name appears in the Vista "networks" window icon,when the device reports itself as unconfigured
		*/
		fprintf(fp, "wps_dev_name=%s\n", nvram_safe_get("model_number"));
		fprintf(fp, "wps_os_version=0x00000001\n");
		fprintf(fp, "wps_atheros_extension=0\n"); //use Atheros WPS extensions
	}
}

void set_wsc_wpa(FILE *fp, wlan *w)
{
	fprintf(fp, "wpa=%s\n", w->wpa.mode);
	fprintf(fp, "ieee8021x=0\n");
	fprintf(fp, "eap_server=1\n");
	fprintf(fp, "eap_user_file=/etc/wpa2/hostapd.eap_user\n");

	if (strlen(w->wpa.psk) == 64)/*64 HEX*/
		fprintf(fp, "wpa_psk=%s\n", w->wpa.psk);
	else/*8~63 ASCII*/
		fprintf(fp, "wpa_passphrase=%s\n", w->wpa.psk);
	fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");

	if (strcmp(w->wpa.cipher, "tkip") == 0)
		fprintf(fp, "wpa_pairwise=TKIP\n");
	else if (strcmp(w->wpa.cipher, "aes") == 0)
		fprintf(fp, "wpa_pairwise=CCMP\n");
	else
		fprintf(fp, "wpa_pairwise=CCMP TKIP\n");
}

void set_wsc_eap(FILE *fp, wlan *w)
{
	char *lan_ipaddr;
	lan_ipaddr = nvram_safe_get("lan_ipaddr");
	fprintf(fp, "wpa=%s\n", w->wpa.mode);
	fprintf(fp, "eap_server=0\n");
	fprintf(fp, "own_ip_addr=%s\n", lan_ipaddr);
	fprintf(fp, "auth_server_addr=%s\n", w->wpa.ra0_ip);
	fprintf(fp, "auth_server_port=%s\n", w->wpa.ra0_port);
	fprintf(fp, "auth_server_shared_secret=%s\n", w->wpa.ra0_secret);
	fprintf(fp, "auth_server_addr=%s\n", w->wpa.ra1_ip);
	fprintf(fp, "auth_server_port=%s\n", w->wpa.ra1_port);
	fprintf(fp, "auth_server_shared_secret=%s\n", w->wpa.ra1_secret);
	fprintf(fp, "ieee8021x=1\n");
	fprintf(fp, "wpa_key_mgmt=WPA-EAP\n");

	if (atoi(w->wpa.reauth_period) < 0)
		fprintf(fp, "eap_reauth_period=0\n");
	else
		fprintf(fp, "eap_reauth_period=%d\n", atoi(w->wpa.reauth_period)*60);

	if (strlen(w->wpa.mac_auth) > 0)
		fprintf(fp, "mac_auth=%s\n", w->wpa.mac_auth);
	else
		fprintf(fp, "mac_auth=3\n");

	if (strcmp(w->wpa.cipher, "tkip") == 0)
		fprintf(fp, "wpa_pairwise=TKIP\n");
	else if (strcmp(w->wpa.cipher, "aes") == 0)
		fprintf(fp, "wpa_pairwise=CCMP\n");
	else
		fprintf(fp, "wpa_pairwise=CCMP TKIP\n");
}

static void set_wsc_wep(FILE *fp, wlan *w)
{
	fprintf(fp, "eap_server=1\n");
	fprintf(fp, "ieee8021x=0\n");
	fprintf(fp, "eap_user_file=/etc/wpa2/hostapd.eap_user\n");
	fprintf(fp, "wpa=0\n");
	fprintf(fp, "wep_default_key=%d\n", atoi(w->wep.default_key)-1); //0~3
	fprintf(fp, "wep_key0=%s\n", w->wep.key1);
	fprintf(fp, "wep_key1=%s\n", w->wep.key2);
	fprintf(fp, "wep_key2=%s\n", w->wep.key3);
	fprintf(fp, "wep_key3=%s\n", w->wep.key4);
}

void create_wsc_conf(wlan *w, int wps_enable)
{
	FILE *fp=NULL;
	char sec_file[30];

	if(wps_enable && w->gidx == 0)
		sprintf(sec_file, "/var/tmp/WSC_ath%d.conf", w->ath);
	else
		sprintf(sec_file, "/var/tmp/secath%d", w->ath);

	//DEBUG_MSG("create_wsc_conf: create %s\n", sec_file);
	if ((fp = fopen(sec_file, "w+")) == NULL) {
		printf("create_wsc_conf: create %s fail\n", sec_file);
		return;
	}
	
	/*generate hostapd config file => see in apps\wps_ath_2\hostapd\hostapd.conf*/
	fprintf(fp, "ignore_file_errors=1\n");
	fprintf(fp, "logger_syslog=-1\n");
	fprintf(fp, "logger_syslog_level=4\n");
	fprintf(fp, "logger_stdout=-1\n");
	fprintf(fp, "logger_stdout_level=4\n");
	fprintf(fp, "debug=0\n");
	fprintf(fp, "ctrl_interface=/var/run/hostapd\n");
	fprintf(fp, "ctrl_interface_group=0\n");
	fprintf(fp, "ssid=%s\n", w->ssid);
	fprintf(fp, "dtim_period=2\n");
	fprintf(fp, "max_num_sta=255\n");
	fprintf(fp, "macaddr_acl=0\n");
	fprintf(fp, "auth_algs=1\n");
	fprintf(fp, "ignore_broadcast_ssid=0\n");
	fprintf(fp, "wme_enabled=0\n");
	fprintf(fp, "eapol_version=2\n");
	fprintf(fp, "eapol_key_index_workaround=0\n");

	if (strcmp(w->security, "wep") == 0) {
		set_wsc_wep(fp, w);
	} else if (strcmp(w->security, "wpa") == 0 && w->en_eap == 0) {
		set_wsc_wpa(fp, w);
	} else if (strcmp(w->security, "wpa") == 0 && w->en_eap == 1) {
		set_wsc_eap(fp, w);
	} else {
		fprintf(fp, "eap_server=1\n");
		fprintf(fp, "ieee8021x=0\n");
		fprintf(fp, "eap_user_file=/etc/wpa2/hostapd.eap_user\n");
	}
 	wsc_wps_setting(fp, wps_enable, w->wpa.rekey_timeout);
	fclose(fp);
}

