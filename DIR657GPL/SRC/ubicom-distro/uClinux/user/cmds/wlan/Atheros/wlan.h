struct wlan_wep{
	int authmode;
	char len[4];
	char key1[128];
	char key2[128];
	char key3[128];
	char key4[128];
	char default_key[2];
};

struct wlan_wpa{
	char mode[2];
	char psk[128];
	char cipher[64];
	char ra0_ip[24];
	char ra0_port[6];
	char ra0_secret[128];
	char ra1_ip[64];
	char ra1_port[6];
	char ra1_secret[128];
	char reauth_period[12];
	char mac_auth[24];
	char rekey_timeout[6];
};

struct wlan_struct {
	int en;
	int ath;
	int bssid;
	int gidx;
	int en_eap;
	int wds_en;
	int auto_channel;
	char ssid[128];
	char security[32];
	char ifnum[30];
	char band[12];
	char channel_mode[24];
	char channel[5];
	char protection[24];
	struct wlan_wep wep;
	struct wlan_wpa wpa;
	struct wlan_struct *next;
};

typedef struct wlan_struct wlan;

#ifndef __KILL_APP_ASYNCH__
#define __KILL_APP_ASYNCH__
#define KILL_APP_ASYNCH(app_name) _system("killall " app_name " > /dev/null 2>&1 &")
#endif
