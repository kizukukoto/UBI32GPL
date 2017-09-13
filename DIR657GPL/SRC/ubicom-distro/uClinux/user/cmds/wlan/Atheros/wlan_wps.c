#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sutil.h>
#include <nvram.h>
#include "wlan.h"

#define APC_WSC_CONF_FILE  "/var/etc/stafwd-wsc.conf"
#define AP_WSC_CONF_FILE  "/var/tmp/topology.conf"

extern wlan *init_wlan(wlan **, int);
extern void release(wlan*);

/* fix me, coding style ... XD */
static void set_bss_file(FILE *fp, int wps, wlan *w)
{
	if (w->gidx == 0) {
		if(wps) {	
			fprintf(fp, "\t\tbss ath%d\n", w->ath);
			fprintf(fp, "\t\t{\n");
			fprintf(fp, "\t\t\tconfig /var/tmp/WSC_ath%d.conf\n", w->ath);
			fprintf(fp, "\t\t}\n");
		} else {
			if (strstr(w->security, "wpa")) {
				fprintf(fp, "\t\tbss ath%d\n", w->ath);
				fprintf(fp, "\t\t{\n");
				fprintf(fp, "\t\t\tconfig /var/tmp/secath%d\n", w->ath);
				fprintf(fp, "\t\t}\n");
			}
		}
	} else {
		if (strstr(w->security, "wpa")) {
			fprintf(fp, "\t\tbss ath%d\n", w->ath);
			fprintf(fp, "\t\t{\n");
			fprintf(fp, "\t\t\tconfig /var/tmp/secath%d\n", w->ath);
			fprintf(fp, "\t\t}\n");
		}
	}
}

static void set_radio_wifi(FILE *fp, int wps, wlan *w)
{
	fprintf(fp, "radio wifi%d\n", w->bssid);
	fprintf(fp, "{\n");
	fprintf(fp, "\tap\n");
	fprintf(fp, "\t{\n");
	for (;w != NULL; w = w->next) {
		if (w->en) {
			set_bss_file(fp, wps, w);
		}
	}
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");
}

static void set_topo_iface(FILE *fp, wlan *w)
{
	for (; w != NULL; w = w->next) {
		if (w->en) {
			if (w->gidx == 0 && nvram_match("wps_enable", "1") == 0) {
				fprintf(fp, "\tinterface ath%d\n", w->ath);
				continue;
			}
			if (strstr(w->security, "wpa")) {
				fprintf(fp, "\tinterface ath%d\n", w->ath);
			}
		}
	}
}

void create_topo_file(int wps_en, wlan *w)
{
	FILE *fp;

	if ((fp = fopen(AP_WSC_CONF_FILE, "w+")) == NULL) {
		printf("create_topo_file: create topology.conf : fail\n");
		return;
	}

	fprintf(fp, "bridge br0\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tipaddress %s\n", nvram_safe_get("lan_ipaddr"));
	fprintf(fp, "\tipmask %s\n", nvram_safe_get("lan_netmask"));
	fprintf(fp, "\tinterface %s\n", nvram_safe_get("lan_eth"));
	set_topo_iface(fp, w);
	fprintf(fp, "}\n");
	set_radio_wifi(fp, wps_en, w);

	fclose(fp);
}
/////////////////////////////////////// APC WPA/WPS topology file ///////////////////////////////////////
static void create_APC_wpa_conf_file_NetworkSection(FILE *fp)
{
	char *wlan0_security, *wlan0_psk_pass_phrase, *wlan0_psk_cipher_type;

	wlan0_security = nvram_safe_get("wlan0_security");
	wlan0_psk_pass_phrase = nvram_safe_get("wlan0_psk_pass_phrase");
	wlan0_psk_cipher_type = nvram_safe_get("wlan0_psk_cipher_type");

	fprintf(fp, "ap_scan=1\n");
	fprintf(fp, "network={\n");
	fprintf(fp, " disabled=0\n");    //1:Disable this network block
	fprintf(fp, " ssid=\"%s\"\n", nvram_safe_get("wlan0_ssid"));
	fprintf(fp, " scan_ssid=0\n");   //SSID-specific Probe Request
	fprintf(fp, " priority=0\n");    //same to all
	fprintf(fp, " mode=0\n");        //Infrastructure

	// NONE security
	if (!strcmp(wlan0_security, "disable")) {
		fprintf(fp, " key_mgmt=NONE\n");
	} else {
		// WEP 
		if (strstr(wlan0_security, "wep")) {
			int keyidx = 0, ascii = 0;
			fprintf(fp, " key_mgmt=NONE\n");

			if (strstr(wlan0_security, "share"))
				fprintf(fp, " auth_alg=SHARED\n");
			else
				fprintf(fp, " auth_alg=OPEN\n");

			keyidx = atoi(nvram_safe_get("wlan0_wep_default_key"))-1;
			fprintf(fp, " wep_tx_keyidx=%d\n", (0>keyidx) ? 0 : ((3<keyidx) ? 3 : keyidx)); //0~3

			if (nvram_match("wlan0_wep_display", "ascii") == 0)
				ascii = 1;
			if (strstr(wlan0_security, "64")) {
				fprintf(fp, " wep_key0=%s\n", nvram_safe_get("wlan0_wep64_key_1"));
				fprintf(fp, " wep_key1=%s\n", nvram_safe_get("wlan0_wep64_key_2"));
				fprintf(fp, " wep_key2=%s\n", nvram_safe_get("wlan0_wep64_key_3"));
				fprintf(fp, " wep_key3=%s\n", nvram_safe_get("wlan0_wep64_key_4"));
			} else if (strstr(wlan0_security, "128")) {
				fprintf(fp, " wep_key0=%s\n", nvram_safe_get("wlan0_wep128_key_1"));
				fprintf(fp, " wep_key1=%s\n", nvram_safe_get("wlan0_wep128_key_2"));
				fprintf(fp, " wep_key2=%s\n", nvram_safe_get("wlan0_wep128_key_3"));
				fprintf(fp, " wep_key3=%s\n", nvram_safe_get("wlan0_wep128_key_4"));
			}
		} else { // WPA-PSK or WPA-EAP or ...
			if (strstr(wlan0_security, "psk")) {
				fprintf(fp, " key_mgmt=WPA-PSK\n");
				if (strlen(wlan0_psk_pass_phrase) == 64) /*64 HEX*/
					fprintf(fp, " psk=%s\n", wlan0_psk_pass_phrase);
				else /*8~63 ASCII*/
					fprintf(fp, " psk=\"%s\"\n", wlan0_psk_pass_phrase);
			}

			if (strstr(wlan0_security, "wpa_"))
				fprintf(fp, " proto=WPA\n");
			else if (strstr(wlan0_security, "wpa2_"))
				fprintf(fp, " proto=WPA2\n");
			else if (strstr(wlan0_security, "wpa2auto_"))
				fprintf(fp, " proto=WPA WPA2\n");

			if (strcmp(wlan0_psk_cipher_type, "tkip") == 0)
				fprintf(fp, " pairwise=TKIP\n");
			else if (strcmp(wlan0_psk_cipher_type, "aes") == 0)
				fprintf(fp, " pairwise=CCMP\n");
			else
				fprintf(fp, " pairwise=CCMP TKIP\n");
		}
	}
	fprintf(fp, "}\n");
}

static void create_APC_wpa_conf_file_WpsSection(FILE *fp) 
{
	fprintf(fp, "update_config=1\n");
	fprintf(fp, "wps_property={\n");
	fprintf(fp, " wps_disable=0\n");
	fprintf(fp, " wps_upnp_disable=1\n");
	fprintf(fp, " version=0x10\n");
//		uuid=
	fprintf(fp, " auth_type_flags=0x0023\n"); //OPEN/WPAPSK/WPA2PSK
	fprintf(fp, " encr_type_flags=0x000f\n"); //NONE/WEP/TKIP/AES
	fprintf(fp, " conn_type_flags=0x1\n");    //ESS
	fprintf(fp, " config_methods=0x84\n");    //PBC/PIN
	fprintf(fp, " wps_state=0x01\n");         //not configured
	fprintf(fp, " rf_bands=0x01\n");          //1:(2.4G), 2:(5G), 3:(2.4G+5G)
	fprintf(fp, " manufacturer=\"%s\"\n",  nvram_safe_get("manufacturer"));
	fprintf(fp, " model_name=\"%s\"\n",    nvram_safe_get("model_name"));
	fprintf(fp, " model_number=\"%s\"\n",  nvram_safe_get("model_number"));
	fprintf(fp, " serial_number=\"%s\"\n", nvram_safe_get("serial_number"));
	fprintf(fp, " dev_category=1\n");         //Computer
	fprintf(fp, " dev_sub_category=1\n");     //PC
	fprintf(fp, " dev_oui=0050f204\n");
	fprintf(fp, " dev_name=\"%s\"\n",      nvram_safe_get("model_name"));
	fprintf(fp, " os_version=0x00000001\n");
	fprintf(fp, "}\n");
}


static void create_APC_wpa_conf_file(void) 
{
	FILE *fp;

	if((fp=fopen(APC_WSC_CONF_FILE, "w+")) == NULL) {
		//DEBUG_MSG("create_APC_wsc_conf: create %s fail\n", APC_WSC_CONF_FILE);
		return;
	}

	fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");

	//SECTION: network={}
	create_APC_wpa_conf_file_NetworkSection(fp);

	//SECTION: wps_property={}
	if (nvram_match("wps_enable", "1") == 0)
		create_APC_wpa_conf_file_WpsSection(fp);

	fclose(fp);
//	system("cat /var/etc/stafwd-wsc.conf");printf("\n");
}


static void create_APC_topo_file()
{
	FILE *fp;

	if ((fp=fopen("/var/tmp/stafwd-topology.conf", "w+")) == NULL) {
		//DEBUG_MSG("create_topo_file: create stafwd-topology.conf : fail\n");
		return;
	}

	fprintf(fp, "bridge %s\n", nvram_safe_get("lan_bridge"));
	fprintf(fp, "{\n");
	fprintf(fp, "\tipaddress %s\n", nvram_safe_get("lan_ipaddr"));
	fprintf(fp, "\tipmask %s\n", nvram_safe_get("lan_netmask"));
	fprintf(fp, "\tinterface %s\n", nvram_safe_get("wlan0_eth"));
	fprintf(fp, "\tinterface %s\n", nvram_safe_get("lan_eth"));
	fprintf(fp, "}\n");

	fprintf(fp, "radio wifi0\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tsta %s\n", nvram_safe_get("wlan0_eth"));
	fprintf(fp, "\t{\n");
	fprintf(fp, "\t\t\tconfig %s\n", APC_WSC_CONF_FILE);
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");

	fclose(fp);
//	system("cat /var/tmp/stafwd-topology.conf");printf("\n");
}
/////////////////////////////////////// APC WPA/WPS topology file ///////////////////////////////////////
static int check_create_topo(wlan *w, int wps)
{
	int ret = 0;
	/* if nvram match is true , it will return 0. */
	if (wps ==  1) {
		ret = 1; 
		goto out;
	}
	for (;w != NULL; w = w->next) {
		if (strcmp(w->security, "wpa") == 0) {
			ret = 1; break;
		}
	}
out:
	return ret;
}

int start_wps(int argc, char *argv[])
{
	int wps_enable = 0;
	wlan *w = NULL;

	init_wlan(&w, 0); //2.4G
        init_wlan(&w, 1); //5G
	if(w->en == 0 || w == NULL)
		return ;
	wps_enable = atoi(nvram_safe_get("wps_enable"));

	//make the topology file for wpa_supplicant
	/* APC */
	if (w->en && !nvram_match("wlan0_op_mode", "APC") &&
	     (wps_enable || strstr(w->security, "wpa"))) {
		create_APC_wpa_conf_file();
		create_APC_topo_file();
		_system("wpa_supplicant /var/tmp/stafwd-topology.conf &");
		return;
	}
	
	if (check_create_topo(w, wps_enable)) {
	//make the topology file for hostapd
		create_topo_file(wps_enable, w);
#ifdef LCM_FEATURE_INFOD
     		_system("hostapd -s %s &", AP_WSC_CONF_FILE);
#else
		//system("hostapd -dd /var/tmp/topology.conf &"); //debug mode
		_system("hostapd %s &", AP_WSC_CONF_FILE);
#endif
		// Start WPS Daemon for polling WPS Button
		system("wpsbtnd &");
	}
	if (w != NULL)
		release(w);
}

int stop_wps(int argc, char *argv[])
{
	KILL_APP_ASYNCH("hostapd");
	KILL_APP_ASYNCH("wpatalk");
	KILL_APP_ASYNCH("wpa_supplicant");
	KILL_APP_ASYNCH("wpsbtnd");
	/*avoid the PBC-led is always on ,if it is on before we shutdown wps */
	//(Ubicom) No such file!
	//system("exec echo 0 > /proc/ar531x/gpio/tricolor_led");
	return 1;
}
