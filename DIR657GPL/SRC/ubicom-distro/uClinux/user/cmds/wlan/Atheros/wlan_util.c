#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sutil.h>
#include <shvar.h>
#include <nvram.h>
#include "wlan.h"

extern void create_wsc_conf(wlan *, int);

static int rg2Country(int regdomain)
{
/*
for ath_dfs.ko
domainoverride  0 = Unitialized (default) 
	=> Disable radar detection since we don't have a radar domain
	1=FCC Domain (FCC3, US)
	2=ETSI Domain (Europe)
	3=Japan Domain
*/
        int countryCode=840;
        switch(regdomain) {
           case 0x03://UNITED ARAB EMIRATES
                countryCode = 784;
                break;
           case 0x07://CTRY_ISRAEL
                countryCode = 376;
                break;
           case 0x14://CTRY_CANADA2
                countryCode = 5001;
                break;
           case 0x21://new
                countryCode = 36;//Australia&HK
                break;
           case 0x23://CTRY_AUSTRALIA2
                countryCode = 5000;
                break;
           case 0x36://CTRY_EGYPT
                countryCode = 818;
                break;
           case 0x3C://CTRY_RUSSIA
                countryCode = 643;
                break;
           case 0x30:
           case 0x37:   //CTRY_GERMANY
           case 0x40:   //CTRY_JAPAN
                countryCode = 276;
                break;
           case 0x10:
           case 0x3A://new
                countryCode = 840; //CTRY_UNITED_STATES
                break;
           case 0x3B://CTRY_BRAZIL
                countryCode = 76;
                break;
           case 0x41:
                countryCode = 393; //CTRY_JAPAN1
           case 0x50://CTRY_TAIWAN
                countryCode = 158;
                break;
           case 0x51://CTRY_CHILE
                countryCode = 152;
                break;
           case 0x52://CTRY_CHINA
                countryCode = 156;
                break;
           case 0x5B://CTRY_SINGAPORE
                countryCode = 702;
                break;
          case 0x5E://CTRY_KOREA_ROC3
                countryCode = 412;
                break;
          case 0x5F://CTRY_KOREA_NORTH
                countryCode = 408;
                break;
          default :
                countryCode = 840; //CTRY_UNITED_STATES
                break;
        }
        return countryCode;
}

void get_country_Code(int *code)
{
        char *wlan0_domain = NULL;

#ifdef SET_GET_FROM_ARTBLOCK
        if (nvram_get("sys_wlan0_domain") != NULL) {
                wlan0_domain = nvram_safe_get("sys_wlan0_domain");
                nvram_set("wlan0_domain", wlan0_domain);
        } else
                wlan0_domain = artblock_get("wlan0_domain");

        if(wlan0_domain != NULL)
                *code = rg2Country(strtol(wlan0_domain, NULL, 16));
        else
                *code = rg2Country(strtol(nvram_safe_get("wlan0_domain"), NULL, 16));
#else
        *code = rg2Country(strtol(nvram_safe_get("wlan0_domain"), NULL, 16));
#endif
}

int get_wlan_vap_gzone(int bssid)
{
        int ret = 2;
#ifdef CONFIG_USER_WL_ATH_GUEST_ZONE
        char cmds[32];
        sprintf(cmds, "wlan%d_vap_guest_zone", bssid);
        ret = atoi(nvram_safe_get(cmds));
#endif
        return ret;
}

int get_wlan_netusb(void)
{
        int ret = 2;
#ifdef CONFIG_USER_WL_ATH_GUEST_ZONE
        ret = atoi(nvram_safe_get("netusb_guest_zone"));
#endif
        return ret;
}


char *get_radius_server(char *eap_radius_server, int bssid, int gidx, int server)
{
        char cmd[128], radius_server[64];
        if (gidx == 0)
                sprintf(cmd, "wlan%d_eap_radius_server_%d", bssid, server);
        else
                sprintf(cmd, "wlan%d_vap%d_eap_radius_server_%d", bssid, gidx, server);
        sprintf(radius_server, "%s", nvram_safe_get(cmd));
        strcpy(eap_radius_server, radius_server);
	return eap_radius_server;
}

int get_wlan_en(int bssid, int gidx)
{
        char cmds[64];

        if (gidx == 0)
                sprintf(cmds, "wlan%d_enable", bssid);
        else
                sprintf(cmds, "wlan%d_vap%d_enable", bssid, gidx);
        return atoi(nvram_safe_get(cmds));
}

int get_wds_en(int bssid)
{
	char cmds[64];
	sprintf(cmds, "wlan%d_wds_enable", bssid);
	return atoi(nvram_safe_get(cmds));
}

int get_wlan_auto_channel(int bssid)
{
        char cmd[32];
        sprintf(cmd, "wlan%d_auto_channel_enable", bssid);
        return atoi(nvram_safe_get(cmd));
}

void set_wlan_channel(char *channel, int bssid)
{
        char cmd[32], tmp_ch[32];
	
	memset(channel, '\0', sizeof(channel));
        sprintf(cmd, "wlan%d_channel", bssid);
        sprintf(tmp_ch, "%s", nvram_safe_get(cmd));
        if (strlen(tmp_ch) > 0)
                strcpy(channel, tmp_ch);
}

void set_wlan_11n_protection(char *protection, int bssid)
{
        char cmd[32], tmp_protect[32];

	memset(protection, '\0', sizeof(protection));
        sprintf(cmd, "wlan%d_11n_protection", bssid);
        sprintf(tmp_protect, "%s", nvram_safe_get(cmd));
        if (strlen(tmp_protect) > 0)
                strcpy(protection, tmp_protect);
}

void set_wlan_channel_mode(char *channel_mode, int bssid)
{
        char cmd[32], tmp_mode[32];

	memset(channel_mode, '\0', sizeof(channel_mode));
        sprintf(cmd, "wlan%d_dot11_mode", bssid);
        sprintf(tmp_mode, "%s", nvram_safe_get(cmd));
        strcpy(channel_mode, tmp_mode);
}

void set_wlan(wlan *w, int bssid, int gidx)
{
        set_wlan_channel(w->channel, bssid);
        set_wlan_channel_mode(w->channel_mode, bssid);
        set_wlan_11n_protection(w->protection, bssid);
        w->auto_channel = get_wlan_auto_channel(bssid);
}

void set_wds_en(wlan *w, int bssid, int gidx)
{
        int wds_en= 0, i;
	
        for (i = 1; i < 4; i++) {
                if (get_wlan_en(bssid, gidx) > 0) {
                        wds_en = 1;
                        break;
                }
        }
        if (w->en && !wds_en)
                w->wds_en = get_wds_en(bssid);
}

void set_wlan_ssid(char *ssid, int ath_cnt, int gidx)
{
        char cmd[128], wlan_ssid[256];

	memset(ssid, '\0', sizeof(ssid));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_ssid", ath_cnt);
        else
                sprintf(cmd, "wlan%d_vap%d_ssid", ath_cnt, gidx);
        sprintf(wlan_ssid, "%s",nvram_safe_get(cmd));
        strcpy(ssid, wlan_ssid);
}

void set_rekey_time(char *rekey_time, int ath_cnt, int gidx)
{
        char cmd[128], wlan_rekey_time[64];

	memset(rekey_time, '\0', sizeof(rekey_time));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_gkey_rekey_time", ath_cnt);
        else
                sprintf(cmd, "wlan%d_vap%d_gkey_rekey_time", ath_cnt, gidx);
        sprintf(wlan_rekey_time, "%s", nvram_safe_get(cmd));
        strcpy(rekey_time, wlan_rekey_time);
}

void set_security(char *security, int bssid, int gidx)
{
	char cmd[128], wlan_security[128];

	memset(security, '\0', sizeof(security));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_security", bssid);
        else
                sprintf(cmd, "wlan%d_vap%d_security", bssid, gidx);
        sprintf(wlan_security, "%s", nvram_safe_get(cmd));
        strcpy(security, wlan_security);
}

void set_radius_profile(char *radius, char *ip, char *port, char *secret)
{
        char *radius_ip = NULL, *radius_port = NULL, *radius_secret = NULL;

	memset(ip, '\0', sizeof(ip));
	memset(port, '\0', sizeof(port));
	memset(secret, '\0', sizeof(secret));
        if (strlen(radius) > 0) {
                radius_ip = strtok(radius, "/");
                radius_port = strtok(NULL,"/");
                radius_secret = strtok(NULL,"/");
                if(radius_ip == NULL || radius_port == NULL || radius_secret == NULL) {
                        radius_ip = "0.0.0.0";
                        radius_port = "1812";
                        radius_secret = "00000";
                }
        } else {
                radius_ip = "0.0.0.0";
                radius_port = "1812";
                radius_secret = "00000";
        }
	
        strcpy(ip, radius_ip);
        strcpy(port, radius_port);
        strcpy(secret, radius_secret);
}


void set_eap_mac_auth(char *eap_mac_auth, int ath_cnt, int gidx)
{
        char cmd[128], mac_auth[24];

	memset(eap_mac_auth, '\0', sizeof(eap_mac_auth));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_eap_mac_auth", ath_cnt);
        else
                sprintf(cmd, "wlan%d_vap%d_eap_mac_auth", ath_cnt, gidx);
        sprintf(mac_auth, "%s", nvram_safe_get(cmd));
        strcpy(eap_mac_auth, mac_auth);
}



void set_reauth_period(char *eap_reauth_period, int ath_cnt, int gidx)
{
        char cmd[128], reauth_period[64];

	memset(eap_reauth_period, '\0', sizeof(eap_reauth_period));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_eap_reauth_period", ath_cnt);
        else
                sprintf(cmd, "wlan%d_vap%d_reauth_period", ath_cnt, gidx);
        sprintf(reauth_period, "%s", nvram_safe_get(cmd));
        strcpy(eap_reauth_period, reauth_period);
}



void set_wpa_mode(wlan *w)
{
	memset(w->wpa.mode, '\0', sizeof(w->wpa.mode));
	if (strstr(w->security, "wpa_"))
                strcpy(w->wpa.mode, "1");
        else if (strstr(w->security, "wpa2_"))
                strcpy(w->wpa.mode, "2");
        else if (strstr(w->security, "wpa2auto_"))
                strcpy(w->wpa.mode, "3");
}

void set_cipher(char *cipher, int ath_cnt, int gidx)
{
        char cmd[128], wlan_cipher[64];

	memset(cipher, '\0', sizeof(cipher));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_psk_cipher_type", ath_cnt);
        else
                sprintf(cmd, "wlan%d_vap%d_psk_cipher_type", ath_cnt, gidx);
        sprintf(wlan_cipher, "%s", nvram_safe_get(cmd));
        strcpy(cipher, wlan_cipher);
}

void set_passphrase(char *pass, int ath_cnt, int gidx)
{
        char cmd[128], wlan_pass[128];

	memset(pass, '\0', sizeof(pass));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_psk_pass_phrase", ath_cnt);
        else
                sprintf(cmd, "wlan%d_vap%d_psk_pass_phrase", ath_cnt, gidx);
        sprintf(wlan_pass, "%s", nvram_safe_get(cmd));
        strcpy(pass, wlan_pass);
}


void set_wep_key(int kidx, char *key, int klen, int bssid, int gidx)
{
        char cmd[128], tmp_key[128];

	memset(key, '\0', sizeof(key));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_wep%d_key_%d", bssid, klen, kidx);
        else
                sprintf(cmd, "wlan%d_vap%d_wep%d_key_%d", bssid, gidx, klen, kidx);
        sprintf(tmp_key, "%s", nvram_safe_get(cmd));
        strcpy(key, tmp_key);
}

void set_wep_default_key(int bssid, char *wep_default_key, int gidx)
{
        char cmd[128], wep_def_key[24];

	memset(wep_default_key, '\0', sizeof(wep_default_key));
        if (gidx == 0)
                sprintf(cmd, "wlan%d_wep_default_key", bssid);
        else
                sprintf(cmd, "wlan%d_vap%d_wep_default_key", bssid, gidx);
        sprintf(wep_def_key, "%s", nvram_safe_get(cmd));
        if ((atoi(wep_def_key) < 1)|| (atoi(wep_def_key) > 4)) {
                //DEBUG_MSG("wep_default_key not correct. Default : 1. \n");
                sprintf(wep_def_key, "1");
        }
        strcpy(wep_default_key, wep_def_key);
}

void set_wep(wlan *w, int bssid, int gidx)
{
        char *p_type = NULL, *p_status = NULL, *ptr = NULL, security[128];
        int klen;

	sprintf(security, "%s", w->security);
        ptr = security;
        p_type = strsep(&ptr, "_");
        p_status = strsep(&ptr, "_");
        klen = atoi(ptr);

        if (!strcmp(p_status, "open"))
                w->wep.authmode = 4; //auto
        else
                w->wep.authmode = 2; //share key

	memset(w->security, '\0', sizeof(w->security));
        strcpy(w->security, p_type);
        strcpy(w->wep.len, ptr);
        set_wep_key(1, w->wep.key1, klen, bssid, gidx);
        set_wep_key(2, w->wep.key2, klen, bssid, gidx);
        set_wep_key(3, w->wep.key3, klen, bssid, gidx);
        set_wep_key(4, w->wep.key4, klen, bssid, gidx);
        set_wep_default_key(bssid, w->wep.default_key, gidx);
}

void set_psk(wlan *w, int bssid, int gidx)
{
        w->en_eap = 0;
	set_wpa_mode(w);
	set_passphrase(w->wpa.psk, bssid, gidx);
        set_cipher(w->wpa.cipher, bssid, gidx);
	memset(w->security, '\0', sizeof(w->security));
        strcpy(w->security, "wpa");
}

void set_eap(wlan *w, int bssid, int gidx)
{
        char radius_server_0[128], radius_server_1[128];
        memset(radius_server_0, '\0', sizeof(radius_server_0));
        memset(radius_server_1, '\0', sizeof(radius_server_1));
        w->en_eap = 1;
        set_eap_mac_auth(w->wpa.mac_auth, bssid, gidx);
        set_reauth_period(w->wpa.reauth_period, bssid, gidx);
        get_radius_server(radius_server_0, bssid, gidx, 0);
        get_radius_server(radius_server_1, bssid, gidx, 1);
        set_radius_profile(radius_server_0, w->wpa.ra0_ip, w->wpa.ra0_port, w->wpa.ra0_secret);
        set_radius_profile(radius_server_1, w->wpa.ra1_ip, w->wpa.ra1_port, w->wpa.ra1_secret);
}

static void set_multicase(int ath)
{
/*      NickChou/Albert, 2009.3.25
	Enable all Wireless MulticastPackets change to UnicastPackets.
*/
#ifdef SUPPORT_IGMP_SNOOPY
        if(nvram_match("multicast_stream_enable", "1")==0)
                _system("iwpriv ath%d mcastenhance 2", ath);
#else
        _system("iwpriv ath%d mcastenhance 2", ath);
#endif
}

static void _set_modify_ssid(char *wlan_ssid, int ath_count)
{
        char ssid[128], ssid_tmp[64]; 
        int c1 = 0, c2 = 0, len = 0;

        memset(ssid, 0, sizeof(ssid));
        memset(ssid_tmp, 0, sizeof(ssid_tmp));

        len = strlen(wlan_ssid);
        strncpy(ssid_tmp, wlan_ssid, len);

        for(c1 = 0; c1 < len; c1++) {
                /*   test ssid = ',.`'~!@#$%^&*()_+[]\:"0-=  */
                /*   " = 0x22, ` = 0x60,  $ = 0x24,  \ = 0x5c, ' = 0x27,      */

                if((ssid_tmp[c1] == 0x22) || (ssid_tmp[c1] == 0x24) ||
                   (ssid_tmp[c1] == 0x5c) || (ssid_tmp[c1] == 0x60)) {
                        ssid[c2] = 0x5c;
                        c2++;
                        ssid[c2] = ssid_tmp[c1];
                } else 
                        ssid[c2] = ssid_tmp[c1];
                c2++;
        }

        _system("iwconfig ath%d essid \"%s\"", ath_count, ssid);
}

void set_modify_ssid(wlan *w)
{
        for (;w != NULL; w = w->next)
                if (w->en) {
                        _set_modify_ssid(w->ssid, w->ath);
			set_multicase(w->ath);
		}
}

static void set_iwf_wep_key(wlan *w)
{
	_system("iwpriv ath%d authmode %d", w->ath, w->wep.authmode);
        _system("iwconfig ath%d key [1] %s", w->ath, w->wep.key1);
        _system("iwconfig ath%d key [2] %s", w->ath, w->wep.key2);
        _system("iwconfig ath%d key [3] %s", w->ath, w->wep.key3);
        _system("iwconfig ath%d key [4] %s", w->ath, w->wep.key4);
        _system("iwconfig ath%d key [%s]", w->ath, w->wep.default_key);
}

void set_wlan_security(wlan *w, const char *op_mode)
{
        int wps_enable = 0;

	if (strcmp(op_mode, "AP") != 0)
		return;
        if (nvram_match("wps_enable", "1") == 0)
                wps_enable=1;
        if (strstr(w->security, "disable")) {
                if (wps_enable && w->gidx == 0)
                        create_wsc_conf(w, wps_enable);
        } else if (strstr(w->security, "wep")) {
                set_iwf_wep_key(w);
                if (wps_enable && w->gidx == 0)
                        create_wsc_conf(w, wps_enable);
        } else if (strstr(w->security, "wpa")) {
                create_wsc_conf(w, wps_enable);
        }
}

void set_mac_filter(int ath_count)
{
        char mac_filter_rule[15] = {0}, mac_filter_value[64] = {0};
        char *filter_enable=NULL, *filter_name=NULL, *mac=NULL, *schedule=NULL;
        int i=0, maccmd_flag=0;
        char *mac_filter_type = nvram_safe_get("mac_filter_type"), *p_mac_filter_value=NULL;

        /*Flush the ACL database*/
        _system("iwpriv ath%d maccmd 3", ath_count);

        if ((strcmp(mac_filter_type, "list_allow") == 0)  || (strcmp(mac_filter_type, "list_deny") == 0)) {
                for (i = 0; i < MAX_MAC_FILTER_NUMBER; i++) {
                        snprintf(mac_filter_rule, sizeof(mac_filter_rule), "mac_filter_%02d",i);
                        p_mac_filter_value=nvram_safe_get(mac_filter_rule);

                        if ( strcmp(p_mac_filter_value, "") == 0 )
                                continue;
                        else
                                strcpy(mac_filter_value, p_mac_filter_value);

                        if (getStrtok(mac_filter_value, "/", "%s %s %s %s",
                                &filter_enable, &filter_name, &mac, &schedule) != 4) {
                                strcpy(mac_filter_value, p_mac_filter_value);
                                /* for old NVRAM setting */
                                if (getStrtok(mac_filter_value, "/", "%s %s %s",
                                        &filter_name, &mac, &schedule) == 3)
                                        filter_enable = "1";
                                else
                                        continue;
                        }

                        if (mac == NULL) {
                                printf("wlan mac format not correct\n");
                                continue;   /* return; */
                        }

                        if (strcmp(mac, "") == 0 || strcmp(mac, "00:00:00:00:00:00") == 0)
                                continue;

                        if (strcmp(filter_enable, "1") == 0) {
                                _system("iwpriv ath%d addmac %s", ath_count, mac);
                //              DEBUG_MSG("set_mac_filter: set %s in ath%d macfilter\n", mac, ath_count);
                                maccmd_flag=1;
                        }
                }

                if (maccmd_flag && (strcmp(mac_filter_type, "list_allow") == 0)) {
                        _system("iwpriv ath%d maccmd 1", ath_count);
                        //DEBUG_MSG("set_mac_filter: ath%d macfilter is allow\n", ath_count);
                } else {
                        _system("iwpriv ath%d maccmd 2", ath_count);
                        //DEBUG_MSG("set_mac_filter: ath%d macfilter is deny\n", ath_count);
                }
        } else
                _system("iwpriv ath%d maccmd 0", ath_count);
}

void set_bridge_arpping_and_led(void)
{
	char *lan_bridge = NULL, *lan_eth = NULL;

        lan_bridge = nvram_safe_get("lan_bridge");
        lan_eth = nvram_safe_get("lan_eth");

	//NickChou, keep br0 MAC = lan_MAC, not wifi0 MAC or wifi1 MAC
	_system("brctl delif %s %s &", lan_bridge, lan_eth);
	sleep(1);
	_system("brctl addif %s %s &", lan_bridge, lan_eth);
	//ensure all nodes are updated on the network
	_system("arping -U -c 1 -I %s %s &", lan_bridge, nvram_safe_get("lan_ipaddr"));
	system("gpio WLAN_LED blink &");
}
