/*
20110708, CAMEO RobertChen add this file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "debug.h"

#define WLAN_EN	"/var/tmp/wlan_en"
#define WLAN_CONFIG_STATE "/var/tmp/wlan_config_state"
#define OWNER_WPS "/sys/module/umac/parameters/owps"
/* 
Charles Teng
sync status for wizard 
*/
#ifdef ATH_GENERAL_WDS_EXTAP
static int extap_enable=0;
#endif

char *parse_special_char(char *ori_str, int len, char *new_str)
{
	int i, j;
	
	for (i = 0, j = 0; i < len; i++) {
		/*   " = 0x22, ` = 0x60,  $ = 0x24,  \ = 0x5c, ' = 0x27  > = 3e, < = 3c */
                if ((ori_str[i] == 0x22) || (ori_str[i] == 0x24) ||
                   (ori_str[i] == 0x5c) || (ori_str[i] == 0x60) ||
                   (ori_str[i] == 0x3e) || (ori_str[i] == 0x3c)){
                        new_str[j] = 0x5c;
                        j++;
                        new_str[j] = ori_str[i];
                } else {
                        new_str[j] = ori_str[i];
                }
                j++;
	}
	return new_str;
}

/* Get Status of WPS */
static int gsWPS()
{
	int fp, ret = -1;
	char buf[2];
	
	memset(buf, '\0', sizeof(buf));

	if (access(OWNER_WPS, F_OK) != 0)
		goto out;
        if ((fp = open(OWNER_WPS, O_RDONLY))) {
		ret = read(fp, buf, 1);
		close(fp);
	}
out:
#if WPS_SUPPORT_GUEST
	return ret != -1 ? atoi(buf) : -1;
#else
	return 1;
#endif
}


static int wlan_mode(int i)
{
        int fp, size;
        char cmd[256], buf[12];
        int ret = 0;

        memset(cmd, '\0', sizeof(cmd));
        memset(buf, '\0', sizeof(buf));

        sprintf(cmd, "nvram get wlan%d_mode > /var/tmp/wlan_mode", i);
        system(cmd);

        if ((fp = open("/var/tmp/wlan_mode", O_RDONLY))) {
                size = read(fp, buf, 2);
                if(!strcmp(buf, "rt"))
                	ret=1;
                close(fp);
        }
        return ret;
	
}

int wlan_en(int i)
{
        int fp, size;
        char cmd[256], buf[12];
        int ret = 0;

        memset(cmd, '\0', sizeof(cmd));
        memset(buf, '\0', sizeof(buf));

	sprintf(cmd, "nvram get wlan%d_enable > %s", i, WLAN_EN);
        system(cmd);

        if ((fp = open(WLAN_EN, O_RDONLY))) {
                size = read(fp, buf, 1);
                ret = atoi(buf);
                close(fp);
        }

	if (!access(WLAN_EN, F_OK))
		unlink(WLAN_EN);
        return ret;
}

int wlan_guest_en(int i, int j)
{
        int fp, size;
        char cmd[256], buf[12];
        int ret = 0;

        memset(cmd, '\0', sizeof(cmd));
        memset(buf, '\0', sizeof(buf));

	sprintf(cmd, "nvram get wlan%d_val%d_enable > %s", i, j, WLAN_EN);
        system(cmd);

        if ((fp = open(WLAN_EN, O_RDONLY))) {
                size = read(fp, buf, 1);
                ret = atoi(buf);
                close(fp);
        }

	if (!access(WLAN_EN, F_OK))
		unlink(WLAN_EN);
        return ret;

}

#ifdef ATH_GENERAL_WDS_EXTAP
static void get_extap_enable(void)
{
	FILE *fp=NULL;
	char buf[2];
	
	fp=popen("nvram get wlan0_wds_extap_enable", "r");
	if(fp){
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), fp);
		if(strlen(buf)){
			extap_enable=atoi(buf);
		}
		pclose(fp);
	}
}
#endif

static void set_ssid(int i, const char *value)
{
        char cmd[256], new_ssid[256];
	int ret = 0;

        memset(cmd, '\0', sizeof(cmd));
        memset(new_ssid, '\0', sizeof(new_ssid));
	

        if (wlan_en(i)) {
#ifdef ATH_GENERAL_WDS_EXTAP
        	if(extap_enable)
        		sprintf(cmd, "nvram set wlan%d_wds_ap_ssid=%s", 
			i, parse_special_char(value, strlen(value), new_ssid));
		else
#endif
		if ((ret = gsWPS()) != -1) {
			if (ret)
				sprintf(cmd, "nvram set wlan%d_ssid=%s",
				i, parse_special_char(value, strlen(value), new_ssid));
			else
				sprintf(cmd, "nvram set wlan%d_vap1_ssid=%s",
				i, parse_special_char(value, strlen(value), new_ssid));
			system(cmd);
		}
        }
}

static void set_wpa(int i, const char *value)
{
        char cmd[256], security[24];
	int type = atoi(value), ret = 0;

        memset(cmd, '\0', sizeof(cmd));
        memset(security, '\0', sizeof(security));
#ifdef ATH_GENERAL_WDS_EXTAP
	if(extap_enable)
		sprintf(cmd, "nvram set wlan%d_wds_ap_security=", i);
	else
#endif
	if ((ret = gsWPS()) != -1) {
		if (ret)
			sprintf(cmd, "nvram set wlan%d_security=", i);
		else
			sprintf(cmd, "nvram set wlan%d_vap1_security=", i);
	}


        if (wlan_en(i)) {
		switch (type) {
			case 3 : /* WPA/WPA2 */
				sprintf(security, "%s", "wpa2auto_psk");
				break;
			case 2 : /* WPA2 */
				sprintf(security, "%s", "wpa2_psk");
				break;
			case 1 : /* WPA */
				sprintf(security, "%s", "wpa_psk");
				break;
			case 0 : /* None Security */
				sprintf(security, "%s", "disable");
				break;
			default :
				/* Nothing to do! */
				break;
		}

		if (strlen(security) != 0 && strlen(cmd) != 0) {
			sprintf(cmd, "%s%s", cmd, security);
			system(cmd);
		}
        }
}

static void set_cipher(int i, const char *value)
{
        char cmd[256];
	int ret = 0;

        memset(cmd, '\0', sizeof(cmd));
#ifdef ATH_GENERAL_WDS_EXTAP
	if(extap_enable)
		sprintf(cmd, "nvram set wlan%d_wds_ap_psk_cipher_type=%s", i, value);
	else
#endif
	if ((ret = gsWPS()) != -1) {
		if (ret)
			sprintf(cmd, "nvram set wlan%d_psk_cipher_type=%s", i, value);
		else
			sprintf(cmd, "nvram set wlan%d_vap1_psk_cipher_type=%s", i, value);
	}
	
        if (wlan_en(i)) {
		system(cmd);
        }
}

static void set_psk(int i, const char *value)
{
        char cmd[256];
	int ret = 0;

        memset(cmd, '\0', sizeof(cmd));
#ifdef ATH_GENERAL_WDS_EXTAP
	if(extap_enable)
		sprintf(cmd, "nvram set wlan%d_wds_ap_psk_pass_phrase=", i);
	else
#endif
        if ((ret = gsWPS()) != -1) {
                if (ret)
			sprintf(cmd, "nvram set wlan%d_psk_pass_phrase=", i);
                else
			sprintf(cmd, "nvram set wlan%d_vap1_psk_pass_phrase=", i);
        }

        if (wlan_en(i)) {
		sprintf(cmd, "%s'%s'", cmd, value);
                system(cmd);
        }
}

void restart_wlan(void)
{
	char buf[5], cmd[128];
	FILE *fp = NULL;
	
	memset(buf, '\0', sizeof(buf));
	memset(cmd, '\0', sizeof(cmd));
	
	if (access("/tmp/wps_restart_wlan", F_OK) == 0) {
		if ((fp=popen("cat /var/run/rc.pid", "r")) != NULL ) {
			if (fgets(buf, sizeof(buf), fp)) {
				sprintf(cmd, "kill -SIGTSTP %s", buf);
				cprintf("WPS: restart_wlan: cmd : %s\n", cmd);
				system(cmd);
			}
			pclose(fp);
		}
		unlink("/tmp/wps_restart_wlan");
	}
}

/* Date : 2012/1/6
 * Author : Robert
 * Rason : Need to check when to lock disable wps pin
 * */
int check_need_lock()
{
        int fp, size;
        char cmd[256], buf[12];
        int ret = 0;

        memset(cmd, '\0', sizeof(cmd));
        memset(buf, '\0', sizeof(buf));

#ifdef ATH_GENERAL_WDS_EXTAP
	if(extap_enable)
		sprintf(cmd, "nvram get wds_ap_wps_configured_mode > %s", WLAN_CONFIG_STATE);
	else
#endif
	sprintf(cmd, "nvram get wps_configured_mode > %s", WLAN_CONFIG_STATE);
        system(cmd);

        if ((fp = open(WLAN_CONFIG_STATE, O_RDONLY))) {
                size = read(fp, buf, 1);
		if (atoi(buf) == 1) {
			ret = 1;
			memset(cmd, '\0', sizeof(cmd));
#ifdef ATH_GENERAL_WDS_EXTAP
			if(extap_enable)
				sprintf(cmd, "nvram set disable_ap_wps_pin=1");
			else
#endif
			sprintf(cmd, "nvram set disable_wps_pin=1");
			system(cmd);
		}
                close(fp);
        }

	if (!access(WLAN_CONFIG_STATE, F_OK))
		unlink(WLAN_CONFIG_STATE);
        return ret;
}

#ifdef SUPPORT_HOSTAPD_WPS_CONFIG_SINGLE_MBSSID

void save_to_nvram_for_single_mbssid(const char *iface, const char *data)
{
	char *key = NULL, *value = NULL;
	char cmd[256] = {}, buf[strlen(data)+1];
	char *delim = "=";
	char tmp[60] = {};
	int type = 0;

	if (!strcmp(data, "done")) {
		system("nvram commit");
		return;
	}

	memset(buf, '\0', sizeof(buf));

	sprintf(buf, "%s", data);
	key = strtok(buf, delim);
	value = strtok(NULL, delim);
	
	if (!strcmp(key, "wps_state")) {
		sprintf(cmd, 
			"nvram set wps_guest_configured_mode=5;"
//			"nvram set disable_wps_pin=1"
		);
		system(cmd);
		return;
	}
	
	if (!strcmp(key, "ssid"))
	{
		sprintf(cmd, "nvram set mbssid_vap%c_ssid=%s", iface[3], 
			parse_special_char(value, strlen(value), tmp));
		system(cmd);
		return;
	}
	else if (!strcmp(key, "wpa"))
	{
		type = atoi(value);
		switch (type) {
			case 3 : /* WPA/WPA2 */
				strcpy(tmp, "wpa2auto_psk");
				break;
			case 2 : /* WPA2 */
				strcpy(tmp, "wpa2_psk");
				break;
			case 1 : /* WPA */
				strcpy(tmp, "wpa_psk");
				break;
			case 0 : /* None Security */
				strcpy(tmp, "disable");
				break;
			default :
				/* Nothing to do! */
				break;
		}
		if (strlen(tmp) > 0) {
			sprintf(cmd, "nvram set mbssid_vap%c_security=%s", iface[3], tmp);
			system(cmd);
		}
		return;
	}
	else if (!strcmp(key, "wpa_pairwise"))
	{
		if (strcmp(value, "CCMPTKIP") == 0) {
			sprintf(cmd, "nvram set mbssid_vap%c_psk_cipher_type=both", iface[3]);
		} else if (strcmp(value, "CCMP") == 0) {
			sprintf(cmd, "nvram set mbssid_vap%c_psk_cipher_type=aes", iface[3]);
		} else {
			sprintf(cmd, "nvram set mbssid_vap%c_psk_cipher_type=tkip", iface[3]);
		}
		system(cmd);
		return;
	}
	else if (!strcmp(key, "wpa_psk") || !strcmp(key, "wpa_passphrase"))
	{
		sprintf(cmd, "nvram set mbssid_vap%c_psk_pass_phrase=%s", iface[3], value);
		system(cmd);
		return;
	}
	else if (!strcmp(key, "setup_wizard"))
	{
		// nothing to do
		return;
	}

	// others
	sprintf(cmd, "nvram set %s=%s", key, value);
	system(cmd);
}

#else // SUPPORT_HOSTAPD_WPS_CONFIG_SINGLE_MBSSID

void save_to_nvram(const char *data)
{
	char *key = NULL, *value = NULL;
	char cmd[256], buf[strlen(data)+1];
	char *delim = "=";
	int ret = -1;

	if (!strcmp(data, "done")) {
		system("nvram commit");
		return;
	}
	
#ifdef ATH_GENERAL_WDS_EXTAP
	get_extap_enable();
#endif

	memset(cmd, '\0', sizeof(cmd));
	memset(buf, '\0', sizeof(buf));

	sprintf(buf, "%s", data);
	key = strtok(buf, delim);
	value = strtok(NULL, delim);

	if (!strcmp(key, "wps_state")) {
#ifdef ATH_GENERAL_WDS_EXTAP
		if(extap_enable) {
			sprintf(cmd, "nvram set wds_ap_wps_configured_mode=5");
			system(cmd);
			sprintf(cmd, "nvram set disable_wps_pin=1");
		} else {
#endif
			if ((ret = gsWPS()) != -1) {
				if (ret) {
					sprintf(cmd, "nvram set wps_configured_mode=5");
					system(cmd);
					sprintf(cmd, "nvram set disable_wps_pin=1");
				} else {
					sprintf(cmd, "nvram set wps_guest_configured_mode=5");
					system(cmd);
					sprintf(cmd, "nvram set disable_wps_pin=1");
				}
			}
#ifdef ATH_GENERAL_WDS_EXTAP
		}
#endif
		system(cmd);
		return;
	}

	if (!strcmp(key, "ssid")) {
		set_ssid(0, value);
		set_ssid(1, value);
		return;
	}

	if (!strcmp(key, "wpa")) {
		set_wpa(0, value);
		set_wpa(1, value);
		return;
	}

	if (!strcmp(key, "wpa_pairwise")) {
		if (strcmp(value, "CCMPTKIP") == 0) {
			set_cipher(0, "both");
			set_cipher(1, "both");
		} else if (strcmp(value, "CCMP") == 0) {
			set_cipher(0, "aes");
			set_cipher(1, "aes");
		} else {
			set_cipher(0, "tkip");
			set_cipher(1, "tkip");
		}
		return;
	}

	if (!strcmp(key, "wpa_psk")) {
		set_psk(0, value);
		set_psk(1, value);
		return;
	}

	if (!strcmp(key, "wpa_passphrase")) {
		set_psk(0, value);
		set_psk(1, value);
		return;
	}
	/* 
	   Charles Teng
	   sync status for wizard 
	 */
	if(!strcmp(key, "setup_wizard")){        	
		if (wlan_mode(0)==1){
			sprintf(cmd, "nvram set setup_wizard_rt=%s",value);
			system(cmd);
		} else {
			sprintf(cmd, "nvram set setup_wizard_ap=%s",value);
			system(cmd);
		}
		return;
	}

	sprintf(cmd, "nvram set %s=%s", key, value);
	system(cmd);

}

#endif // SUPPORT_HOSTAPD_WPS_CONFIG_SINGLE_MBSSID

