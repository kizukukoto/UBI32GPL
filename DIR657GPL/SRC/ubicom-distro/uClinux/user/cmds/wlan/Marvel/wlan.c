#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>                                                         
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/wireless.h>
//#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <signal.h>

#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
//#include <rc.h>
//#include "bsp.h"

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define MAX_RADIO_NUMS 2 /* set max radio numbers */
#define MAX_MSSID_NUMS 8
int RADIO_NUMS = 0; /* how many radios */
char WLAN_OPMODE[][MAX_RADIO_NUMS] = {0,0}; /* operation mode: AP or APC or ... for each radio */
int MSSID_NUMS = 0; /* how many SSIDs */ 

enum {
	WIFI_G_BAND = 0, /* 2.4G */
	WIFI_A_BAND,     /* 5G */
};
int wlan_restart = 0;
int wlan_mvl_enable = -1;
#if 0
struct wdev_desc{
	char *wdev;
	char *ssid;
	int radius_mode; /* 0: 2.4G, 1: 5G */
};

static const struct wdev_desc *get_wdev_desc()
{
	static struct wdev_desc w, *p = &w;
	
	p->wdev = "wdev0ap0";
	p->radius_mode = atoi(nvram_safe_get("wlan0_radio_mode"));
	p->ssid = wlanxxx_nvram_safe_get("_ssid");
	return (const struct wdev_desc *)p;
}
#endif
static const char *get_wdev()
{
	return "wdev0ap0";
}

void init_wps()
{
	int ssid_idx = 0;
	const char *wdev = get_wdev();
	/*
	 * FIXME:
	if ( MSSID_NUMS != 1) //multi-ssid, maybe future work
		return;
	*/
	_system("mkdir -p /var/etc/wsc/%s", wdev);
	_system("cp -r /etc/mvl_wps/* /var/etc/wsc/%s", wdev);
	_system("cp -r /etc/mvl_hostapd/* /var/etc/wsc");
}

void wlanxxx_nvram_set(const char *key, const char *value)
{
	char buf[128];
	char *m;
	
	if ((m = nvram_get("wlan0_radio_mode")) == NULL)
		m = "0";
	sprintf(buf, "wlan%s_%s", m, key);
	nvram_set(buf, value);
}

char *wlanxxx_nvram_safe_get(const char *key)
{
	char buf[128];
	char *m;
	
	if ((m = nvram_get("wlan0_radio_mode")) == NULL)
		m = "0";
	sprintf(buf, "wlan%s_%s", m, key);
	return nvram_safe_get(buf);
}

static char *get_configed_ssid(char *ssid)
{
	char *mac;

	strcpy(ssid, "dlink");

	/* XXX: should we get from APIs? */
	mac = wlanxxx_nvram_safe_get("mac");
	if (strlen(mac) != 17)	/* 17 len of 00:11:22:33:44:55 */
		return strcpy(ssid, "dlink4321");
	ssid[5] = mac[12];
	ssid[6] = mac[13];
	ssid[7] = mac[15];
	ssid[8] = mac[16];

	ssid[9] = '\0';

	return ssid;
}

/*
 * initial wsc_config.txt:
 *
 * Authentication types
 * #define WSC_AUTHTYPE_OPEN        0x0001
 * #define WSC_AUTHTYPE_WPAPSK      0x0002
 * #define WSC_AUTHTYPE_SHARED      0x0004
 * #define WSC_AUTHTYPE_WPA         0x0008
 * #define WSC_AUTHTYPE_WPA2        0x0010
 * #define WSC_AUTHTYPE_WPA2PSK     0x0020
 * 
 *
 * Encryption types(ENCR_TYPE_FLAGS)
 * #define WSC_ENCRTYPE_NONE    0x0001
 * #define WSC_ENCRTYPE_WEP     0x0002
 * #define WSC_ENCRTYPE_TKIP    0x0004
 * #define WSC_ENCRTYPE_AES     0x0008
 * */
static int __wps_setup_wsc_configured_mode(const char *dev)
{

	char path[128], *e, *s;
	char buf[128];
	int rev = -1;
	struct {
		char *auth;
		char *auth_val;
		char *enc;
		char *enc_val;
	} *w, wl_security[] = {
		{ "disable", "0x0001", NULL, "0x0001"},
		{ "wep_open", "0x0001", NULL, "0x0002"}, /* not support WEP!*/
		{ "wep_share", "0x0001", NULL, "0x0002"},/* not support WEP!*/
		{ "wpa_psk", "0x0002", "tkip", "0x0004"},
		{ "wpa_psk", "0x0002", "aes", "0x0008"},	/* support? */
		{ "wpa2_psk", "0x0020", "tkip", "0x0004"},	/* support? */
		{ "wpa2_psk", "0x0020", "aes", "0x0008"},
		{ "wpa2auto_psk", "0x0022", "both", "0x000C"},
		{ NULL, NULL, NULL, NULL},
	};
	sprintf(path, "/etc/wsc/%s/wsc_config.txt", dev);
	s = wlanxxx_nvram_safe_get("security");
	e = wlanxxx_nvram_safe_get("psk_cipher_type");

	for (w = wl_security; w->auth != NULL; w++) {
		if (strncmp(s, w->auth, strlen(w->auth)) != 0)
			continue;
		if (w->enc != NULL && strncmp(e, w->enc, strlen(w->enc)) != 0)
			continue;
		break;
	}
	
	if (w->auth == NULL)
		goto out;
	
	fprintf(stderr, "%s:%d w->auth:%s<%s> w->enc:%s<%s>",
			__FUNCTION__, __LINE__,
			w->auth, w->auth_val, w->enc, w->enc_val);
	sprintf(buf, "AUTH_TYPE_FLAGS=%s", w->auth_val);
	file_strncmp2replace(path, "AUTH_TYPE_FLAGS=", buf);
	
	sprintf(buf, "ENCR_TYPE_FLAGS=%s", w->enc_val);
	file_strncmp2replace(path, "ENCR_TYPE_FLAGS=", buf);
	
	if (w->enc != NULL) {
		/* FIXME: always support WPS-PSK only */
		file_strncmp2replace(path, "KEY_MGMT=", "KEY_MGMT=WPA-PSK");
		fprintf(stderr, "%s:%d set KEY_MGMT\n", __FUNCTION__, __LINE__);

		/* NW_KEY for PSK ? */
		sprintf(buf, "NW_KEY=%s", wlanxxx_nvram_safe_get("psk_pass_phrase"));
		file_strncmp2replace(path, "NW_KEY=", buf);
		fprintf(stderr, "%s:%d set NW_KEY=%s\n", __FUNCTION__, __LINE__,
				wlanxxx_nvram_safe_get("psk_pass_phrase"));
	}
	rev = 0;
out:
	return rev;
}

/*
 * 
 * in wsc_config.txt: update keys for configured mode
 * o SSID: new ssid in configured mode.
 * o NW_KEY: phrase<text mode or hex mode?>
 * o CONFIGURED_MODE: 4
 * o AUTH_TYPE_FLAGS:
 * o ENCR_TYPE_FLAGS:
 * o UUID: (16 lenght)? (optional)
 * o DEVIE_NAME=DlinkDIR665. (optional)
 * 
 * In WPS function(MVL). they will config hostapd.conf and exec hostapd.
 * so, we should not to take care about of hostapd and hostapd.conf
 *
 * */
static int __wps_setup_wsc_config()
{
	const char *dev = get_wdev();
	char path[128];
	char buf[128], configed_ssid[20], *ssid;
	int configured = (nvram_match("wps_configured_mode", "5") == 0);
	
	
	ssid = wlanxxx_nvram_safe_get("ssid");
	_system("echo \"%s\" > /etc/wsc/%s/wsc_ap_pin.txt",
			nvram_safe_get("wps_pin"), dev);
	
	sprintf(path, "/etc/wsc/%s/wsc_config.txt", dev);
	/*
	 * SSID:
	 * config: the ssid of AP
	 * unconfig: the ssid of AP. after config, this key will not be changed
	 * 	but, new ssid can read from hostapd.conf
	 * */
	sprintf(buf, "SSID=%s", ssid);
	file_strncmp2replace(path, "SSID=", buf);

	sprintf(buf, "CONFIGURED_MODE=%s", configured ? "4" : "1");
	file_strncmp2replace(path, "CONFIGURED_MODE=", buf);

	/*
	 * FIXME: according to MVL, hostapd.conf should be update from wsc_cmd.
	 * 	But it seems to don't do that.
	 * */
	sprintf(buf, "ssid=%s", ssid);
	sprintf(path, "/etc/wsc/%s/hostapd.conf", dev); 
	file_strncmp2replace(path, "ssid=", buf);
	/*
	 * FIXME: according to MVL, wsc_op_result.txt should reset to 3
	 * 	indicate status of "NOT start", but it seems to don't do that.
	 * */
	_system("echo \"3\" > /etc/wsc/%s/wsc_op_result.txt", dev);
	if (configured) {
		__wps_setup_wsc_configured_mode(dev);
	} else{
		char configed_ssid[20];
		
		_system("echo -n \"%s\" > /etc/wsc/%s/wsc_dir665_preSSID.txt",
					get_configed_ssid(configed_ssid), dev);
	}
	return 0;
}



static void start_wps_daemon()
{
	const char *wdev = get_wdev();
	_system("wsccmd /var/etc/wsc/%s &", get_wdev());
}

void __start_wps()
{
	if (nvram_match("wps_enable", "1") != 0)
		return;
	
	RADIO_NUMS = 1; /* -RR */
	__wps_setup_wsc_config();
	start_wps_daemon();
}

void start_wps(void)
{
	/* FIXME: */
//	if (!( action_flags.all  || action_flags.wlan ))
//		return 1;
	__start_wps();
}

int __stop_wps(void)
{
	_system("killall wsccmd");		
	_system("killall hostapd");
	
	/*
	 * FIXME: according to MVL, wsc_op_result.txt should reset to 3
	 * 	indicate status of "NOT start", but it seems to don't do that.
	 * */
	_system("echo \"3\" > /etc/wsc/%s/wsc_op_result.txt", get_wdev());
	init_wps();
	return 0;
}

int stop_wps(void)
{
	/* FIXME: */
//	if(!( action_flags.all || action_flags.wlan ) )
//        	return 1;
	__stop_wps();
        
}
/**************************************
 * End of WPS parser
 * ************************************/


/* 
 * set_radius (int index/int radio_mode)
 * RF is switchable radio_mode for distinguish 2.4G/5G
 */
/* radio mode */
/* if RF is switchable like 2.4/5G switchable, 
	if (RADIO_NUMS == 1), then
	radio_mode (which is wlan0/(1?)_radio_mode) == 0 means 2.4G; == 1 means 5G
*/
static void set_radius(int index)
{
	char *radius0_server, *radius0_ip = "", *radius0_port = "", *radius0_secret = "";
	char *radius1_server, *radius1_ip = "", *radius1_port = "", *radius1_secret = "";
	char *wlan0_eap_mac_auth="", *wlan0_eap_reauth_period="";
	char *wlan1_eap_mac_auth="", *wlan1_eap_reauth_period="";

	char ssid[64]={0};

	char wlan0_eap_mac_auth_temp[50]={0}, wlan1_eap_mac_auth_temp[50]={0};
	char radius0_server_temp[50]={0};
	char radius1_server_temp[50]={0};
	char wlan0_eap_reauth_period_temp[50]={0}, wlan1_eap_reauth_period_temp[50]={0};

	char tmp[64] = {0};
	char reauth_period[20] = {0};

	char *rekey_time;
	char wpa_group_rekey_time[64];

	/* set ssid to hostapd */
	sprintf(tmp, "ssid=%s", wlanxxx_nvram_safe_get("ssid"));
	file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "ssid=", tmp);

	/* set ssid to hostapd */
	sprintf(tmp, "eap_server=%s", "0");
	file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "eap_server=", tmp);

	/* eap_mac_auth */
	/* set mac_auth feature - Ported from Nick Chou */
	/*  
		mac_auth==0, don't need eap_mac_auth
		mac_auth==1, radius server 1 need eap_mac_auth
		mac_auth==2, radius server 2 need eap_mac_auth
		mac_auth==3, radius server all need eap_mac_auth
	*/
	
	sprintf(tmp, "wlan%d_eap_mac_auth", index);  
	switch (index) {
		case 0:
			wlan0_eap_mac_auth = nvram_safe_get(tmp);
			if(strlen(wlan0_eap_mac_auth) > 0) {
				_system("echo \"mac_auth=%s\" >> /etc/wsc/wdev0ap0/hostapd.conf", wlan0_eap_mac_auth);
			} else {
				_system("echo \"mac_auth=3\" >> /etc/wsc/wdev0ap0/hostapd.conf");
			}
		break;
		case 1:
			wlan1_eap_mac_auth = nvram_safe_get(tmp);
			if(strlen(wlan1_eap_mac_auth) > 0) {
				_system("echo \"mac_auth=%s\" >> /etc/wsc/wdev0ap0/hostapd.conf", wlan1_eap_mac_auth);
			} else {
				_system("echo \"mac_auth=3\" >> /etc/wsc/wdev0ap0/hostapd.conf");
			}
		break;
		default:
			printf("wlan%d_eap_mac_auth is not supported!\n");
		break;
	}


	/* radius server 1 */
	sprintf(tmp, "wlan%d_eap_radius_server_0", index);  
	radius0_server = nvram_safe_get(tmp);
	strcpy (radius0_server_temp, radius0_server);
	if(strlen(radius0_server) > 0 &&
		       	strncmp(radius0_server, "0.0.0.0", 7)) {
		radius0_ip = strtok(radius0_server_temp, "/");
		radius0_port = strtok(NULL, "/");
		radius0_secret = strtok(NULL, "/");
	}

	if( strncmp(radius0_server, "0.0.0.0", 7)) {
		sprintf(tmp, "auth_server_addr=%s", radius0_ip);
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "auth_server_addr=", tmp);
		sprintf(tmp, "auth_server_port=%s", radius0_port);
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "auth_server_port=", tmp);
		sprintf(tmp, "auth_server_shared_secret=%s", radius0_secret);
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "auth_server_shared_secret=", tmp);
	}
	/* radius server 2 */
	sprintf(tmp, "wlan%d_eap_radius_server_1", index);  
	radius1_server = nvram_safe_get(tmp);
	strcpy (radius1_server_temp, radius1_server);
	if(strlen(radius1_server) > 0 &&
		       	strncmp(radius1_server, "0.0.0.0", 7)) {
		radius1_ip = strtok(radius1_server_temp, "/");
		radius1_port = strtok(NULL, "/");
		radius1_secret = strtok(NULL, "/");
	}

	if( strncmp(radius1_server, "0.0.0.0", 7)) {
		sprintf(tmp, "auth_server_addr=%s", radius1_ip);
		_system("echo \"%s\" >> /etc/wsc/wdev0ap0/hostapd.conf", tmp);
		sprintf(tmp, "auth_server_port=%s", radius1_port);
		_system("echo \"%s\" >> /etc/wsc/wdev0ap0/hostapd.conf", tmp);
		sprintf(tmp, "auth_server_shared_secret=%s", radius1_secret);
		_system("echo \"%s\" >> /etc/wsc/wdev0ap0/hostapd.conf", tmp);
	}

	/* set reauth period */
	/* eap_reauth_period */
	sprintf(tmp, "wlan%d_eap_reauth_period", index);  
	switch (index) {
		case 0:
			wlan0_eap_reauth_period = nvram_safe_get(tmp);
			sprintf(reauth_period, "eap_reauth_period=%d", atol(wlan0_eap_reauth_period)*60 );
		break;
		case 1:
			wlan1_eap_reauth_period = nvram_safe_get(tmp);
			sprintf(reauth_period, "eap_reauth_period=%d", atol(wlan1_eap_reauth_period)*60 );
		break;
		default:
			printf("wlan%d_eap_reauth_period is not supported!\n", index);
		break;
	}
	file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "eap_reauth_period=", reauth_period);

	/* write configuration to file */
	_system("echo \"RADIUS_Server=%s;%s\" >> /var/tmp/iNIC_ap.dat",
			radius0_ip, radius1_ip);
	_system("echo \"RADIUS_Port=%s;%s\" >> /var/tmp/iNIC_ap.dat",
			radius0_port, radius1_port);
	_system("echo \"RADIUS_Key=%s;%s\" >> /var/tmp/iNIC_ap.dat",
			radius0_secret, radius1_secret);
	_system("echo \"own_ip_addr=%s\" >> /var/tmp/iNIC_ap.dat",
			       	nvram_safe_get("lan_ipaddr"));

	/* set own ip */
	sprintf(tmp, "own_ip_addr=%s", nvram_safe_get("lan_ipaddr") );
	file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "own_ip_addr=", tmp);

	switch (index) {
		case 0:
			_system("echo \"wlan%d_eap_mac_auth=%s\" >> /var/tmp/iNIC_ap.dat",
			       	index, wlan0_eap_mac_auth);
			_system("echo \"wlan%d_eap_reauth_period=%s\" >> /var/tmp/iNIC_ap.dat",
			       	index, wlan0_eap_reauth_period);
		break;
		case 1:
			_system("echo \"wlan%d_eap_mac_auth=%s\" >> /var/tmp/iNIC_ap.dat",
			       	index, wlan1_eap_mac_auth);
			_system("echo \"wlan%d_eap_reauth_period=%s\" >> /var/tmp/iNIC_ap.dat",
			       	index, wlan1_eap_reauth_period);

		break;
		default:
			printf("wlan%d_eap_reauth_period/mac_auth is not supported!\n", index);
		break;
	}

	/* set rekey period */
	sprintf(tmp, "wlan%d_gkey_rekey_time", index);  
	rekey_time = nvram_safe_get(tmp);
	if (atoi(rekey_time) < 120)
		rekey_time = "120";
	sprintf(wpa_group_rekey_time, "wpa_group_rekey=%s", rekey_time );
	file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa_group_rekey=", wpa_group_rekey_time);
	/* sync rekey to wifi interface */
	_system("iwpriv wdev0ap0 grouprekey %s", rekey_time);


}

static void set_wep_key(int len, int bss_index, int radio_index, char *auth_mode)
{
	int i;
	char *wlan_wep64_key_1;
	char *wlan_wep64_key_2;
	char *wlan_wep64_key_3;
	char *wlan_wep64_key_4;
	
char *wlan_wep128_key_1;
	char *wlan_wep128_key_2;
	char *wlan_wep128_key_3;
	char *wlan_wep128_key_4;
	char *wlan_wep152_key_1;
	char *wlan_wep152_key_2;
	char *wlan_wep152_key_3;
	char *wlan_wep152_key_4;
	unsigned char *wlan_wep_default_key; 
	char *wep64_key=NULL;
	char *wep128_key=NULL;
	char *wep152_key=NULL;

	char wep64_key_1[30];
	char wep64_key_2[30];
	char wep64_key_3[30];
	char wep64_key_4[30];
	char wep128_key_1[30];
	char wep128_key_2[30];
	char wep128_key_3[30];
	char wep128_key_4[30];
	char wep152_key_1[30];
	char wep152_key_2[30];
	char wep152_key_3[30];
	char wep152_key_4[30];
	char wep_default_key[30];
	
	char tmp_radio_mode[64];
	int radio_mode = 0;
     
	sprintf(tmp_radio_mode, "wlan%d_radio_mode", radio_index);/* for distinguish radio_index(RF) is 2.4G or 5G mode */
	radio_mode = atoi(nvram_safe_get(tmp_radio_mode)); 

	switch(len)
	{
		case 64:
			if(bss_index > 0) {
				sprintf(wep64_key_1, "wlan%d_vap%d_wep64_key_1",
					       	radio_index, bss_index);
				sprintf(wep64_key_2, "wlan%d_vap%d_wep64_key_2",
					       	radio_index, bss_index);
				sprintf(wep64_key_3, "wlan%d_vap%d_wep64_key_3",
					       	radio_index, bss_index);
				sprintf(wep64_key_4, "wlan%d_vap%d_wep64_key_4",
					       	radio_index, bss_index);
			} else {
			 	if (RADIO_NUMS == 1) { /* wlan switch 2.4G/5G, always has one radio at once. */
					sprintf(wep64_key_1, "wlan%d_wep64_key_1",
						       	radio_mode);
					sprintf(wep64_key_2, "wlan%d_wep64_key_2",
						       	radio_mode);
					sprintf(wep64_key_3, "wlan%d_wep64_key_3",
						       	radio_mode);
					sprintf(wep64_key_4, "wlan%d_wep64_key_4",
						       	radio_mode);
				}
				else if (RADIO_NUMS > 1) {
					sprintf(wep64_key_1, "wlan%d_wep64_key_1",
						       	radio_index);
					sprintf(wep64_key_2, "wlan%d_wep64_key_2",
						       	radio_index);
					sprintf(wep64_key_3, "wlan%d_wep64_key_3",
						       	radio_index);
					sprintf(wep64_key_4, "wlan%d_wep64_key_4",
						       	radio_index);
				}
				else {
					DEBUG_MSG("error..radio number is less 1\n");
					return;
				}
			}   
			wlan_wep64_key_1 = nvram_safe_get(wep64_key_1);
			wlan_wep64_key_2 = nvram_safe_get(wep64_key_2);    
			wlan_wep64_key_3 = nvram_safe_get(wep64_key_3);
			wlan_wep64_key_4 = nvram_safe_get(wep64_key_4);
			    
			for(i=1; i<5; i++) {
				switch(i) {            
					case 1:
						wep64_key = wlan_wep64_key_1;
						break;
					case 2:
						wep64_key = wlan_wep64_key_2;
						break;
					case 3:
						wep64_key = wlan_wep64_key_3;
						break;
					case 4:
						wep64_key = wlan_wep64_key_4;
						break;		
				}
		     
				if(strlen(wep64_key) < 10)
					DEBUG_MSG("wlan_wep64_key_%d null or"
						       " length error.\n", i);  
				else
	        		_system("iwconfig wdev%dap0 key %s [%d]", radio_index, wep64_key, i);
		    }
			
			break;
		case 128:
			if(bss_index > 0) {
				sprintf(wep128_key_1, "wlan%d_vap%d_wep128_key_1",
					       	radio_index, bss_index);
				sprintf(wep128_key_2, "wlan%d_vap%d_wep128_key_2",
					       	radio_index, bss_index);
				sprintf(wep128_key_3, "wlan%d_vap%d_wep128_key_3",
					       	radio_index, bss_index);
				sprintf(wep128_key_4, "wlan%d_vap%d_wep128_key_4",
					       	radio_index, bss_index);
			} else {
			 	if (RADIO_NUMS == 1) { /* wlan switch 2.4G/5G, always has one radio at once. */
					sprintf(wep128_key_1, "wlan%d_wep128_key_1",
						       	radio_mode);
					sprintf(wep128_key_2, "wlan%d_wep128_key_2",
						       	radio_mode);
					sprintf(wep128_key_3, "wlan%d_wep128_key_3",
						       	radio_mode);
					sprintf(wep128_key_4, "wlan%d_wep128_key_4",
						       	radio_mode);
				}
				else if (RADIO_NUMS > 1) {
					sprintf(wep128_key_1, "wlan%d_wep128_key_1",
						       	radio_index);
					sprintf(wep128_key_2, "wlan%d_wep128_key_2",
						       	radio_index);
					sprintf(wep128_key_3, "wlan%d_wep128_key_3",
						       	radio_index);
					sprintf(wep128_key_4, "wlan%d_wep128_key_4",
						       	radio_index);
				}
				else {
					DEBUG_MSG("error..radio number is less 1\n");
					return;
				}
			}   
			wlan_wep128_key_1 = nvram_safe_get(wep128_key_1);
			wlan_wep128_key_2 = nvram_safe_get(wep128_key_2);    
			wlan_wep128_key_3 = nvram_safe_get(wep128_key_3);
			wlan_wep128_key_4 = nvram_safe_get(wep128_key_4);
			    
			for(i=1; i<5; i++) {
				switch(i) {            
					case 1:
						wep128_key = wlan_wep128_key_1;
						break;
					case 2:
						wep128_key = wlan_wep128_key_2;
						break;
					case 3:
						wep128_key = wlan_wep128_key_3;
						break;
					case 4:
						wep128_key = wlan_wep128_key_4;
						break;		
				}
		     
				if(strlen(wep128_key) < 26)
					DEBUG_MSG("wlan_wep128_key_%d null or"
						       " length error.\n", i);  
				else
	        		_system("iwconfig wdev%dap0 key %s [%d]", radio_index, wep128_key, i);
		   	}
			
			break;
			    
		case 152:
			if(bss_index > 0)
			{
				sprintf(wep152_key_1, "wlan0_vap%d_wep152_key_1", bss_index);
				sprintf(wep152_key_2, "wlan0_vap%d_wep152_key_2", bss_index);
				sprintf(wep152_key_3, "wlan0_vap%d_wep152_key_3", bss_index);
				sprintf(wep152_key_4, "wlan0_vap%d_wep152_key_4", bss_index);
		    }
		    else
		    {
			 	if (RADIO_NUMS == 1) { /* wlan switch 2.4G/5G, always has one radio at once. */
					sprintf(wep152_key_1, "wlan%d_wep152_key_1",
						       	radio_mode);
					sprintf(wep152_key_2, "wlan%d_wep152_key_2",
						       	radio_mode);
					sprintf(wep152_key_3, "wlan%d_wep152_key_3",
						       	radio_mode);
					sprintf(wep152_key_4, "wlan%d_wep152_key_4",
						       	radio_mode);
				}
				else if (RADIO_NUMS > 1) {
					sprintf(wep152_key_1, "wlan%d_wep152_key_1",
						       	radio_index);
					sprintf(wep152_key_2, "wlan%d_wep152_key_2",
						       	radio_index);
					sprintf(wep152_key_3, "wlan%d_wep152_key_3",
						       	radio_index);
					sprintf(wep152_key_4, "wlan%d_wep152_key_4",
						       	radio_index);
				}
				else {
					DEBUG_MSG("error..radio number is less 1\n");
					return;
				}
		    }
				
			wlan_wep152_key_1 = nvram_safe_get(wep152_key_1);
			wlan_wep152_key_2 = nvram_safe_get(wep152_key_2);    
			wlan_wep152_key_3 = nvram_safe_get(wep152_key_3);
			wlan_wep152_key_4 = nvram_safe_get(wep152_key_4); 

		    for(i=1; i<5; i++)
		    {
			    switch(i)
				{            
					case 1:
						wep152_key = wlan_wep152_key_1;
						break;
					case 2:
						wep152_key = wlan_wep152_key_2;
						break;
					case 3:
						wep152_key = wlan_wep152_key_3;
						break;
					case 4:
						wep152_key = wlan_wep152_key_4;
						break;		
				}
		     
			    if(strlen(wep152_key) < 32)
					DEBUG_MSG("wlan0_wep152_key_%d null or length error. \n", i); 
				else
					_system("iwconfig wdev%dap0 key %s [%d]", radio_index, wep152_key, i);
		    }
			
			break;
		default:
			 DEBUG_MSG("WEP key len is not correct. \n");
	}
	 
	/* set wep default key */
	if(bss_index > 0)
		sprintf(wep_default_key, "wlan%d_vap%d_wep_default_key",
			       	radio_index, bss_index);
	else {
	 	if (RADIO_NUMS == 1) { /* wlan switch 2.4G/5G, always has one radio at once. */
			sprintf(wep_default_key, "wlan%d_wep_default_key", radio_mode);
		} else if (RADIO_NUMS > 1) {
			sprintf(wep_default_key, "wlan%d_wep_default_key", radio_index);
		} else {
			DEBUG_MSG("error..radio number is less 1\n");
			return;
		}
	}
	wlan_wep_default_key = nvram_safe_get(wep_default_key);	
	 
	if((atoi(wlan_wep_default_key) < 1) ||
		       	(atoi(wlan_wep_default_key) > 4)) {
		DEBUG_MSG("wlan_wep_default_key not correct. Default : 1. \n");
		wlan_wep_default_key = "1";
	}
	_system("iwconfig wdev%dap0 key [%s] %s", radio_index, wlan_wep_default_key, auth_mode);
}

static void set_security(int bss_index, int radio_index)
{
	char *wlan_security, *wlan_psk_pass_phrase, *cipher_type;
	char tmp[64], tmp2[64], tmp3[64], tmp4[64], tmp_wpa[64], tmp_radio_mode[64];
	int wpa_flag = 0;
	int radio_mode = 0;
	char *auth = "open";
	enum {
		AUTH_WPA_NONE = 0,       /* disable wpa/wpa2 */
		AUTH_WPA_PSK,            /* wpa psk */
		AUTH_WPA2_PSK,           /* wpa2 psk */
		AUTH_WPA_WPA2_PSK_MIXED, /* wpa/wpa2 psk mixed */
	};
	int wpa_auth = AUTH_WPA_NONE; 
	int rekey_time = 120;
	char *psk_passphrase_mode = "";

	sprintf(tmp_radio_mode, "wlan%d_radio_mode", radio_index);/* for distinguish radio_index(RF) is 2.4G or 5G mode */
	radio_mode = atoi(nvram_safe_get(tmp_radio_mode)); 

 	if (RADIO_NUMS == 1) { /* wlan switch 2.4G/5G, always has one radio at once. */
		sprintf(tmp, "wlan%d_security", radio_mode);
		sprintf(tmp2, "wlan%d_psk_pass_phrase", radio_mode);
		sprintf(tmp3, "wlan%d_psk_cipher_type", radio_mode);
		sprintf(tmp4, "wlan%d_gkey_rekey_time", radio_mode);
	}
	else if (RADIO_NUMS > 1) {
		sprintf(tmp, "wlan%d_security", radio_index);
		sprintf(tmp2, "wlan%d_psk_pass_phrase", radio_index);
		sprintf(tmp3, "wlan%d_psk_cipher_type", radio_index);
		sprintf(tmp4, "wlan%d_gkey_rekey_time", radio_index);
	}
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	if (strcmp(WLAN_OPMODE[radio_index], "sta") == 0) { /* sta(ap client) mode doesn't support security */
		DEBUG_MSG("wlan in sta mode, doesn't support security\n");
		return;	
	}


   	wlan_security = nvram_safe_get(tmp);	                
   	wlan_psk_pass_phrase = nvram_safe_get(tmp2);	                

	/* set AuthMode EncrypType WPAPSK DefaultKeyID */
	if((strcmp(wlan_security,"disable") == 0))
	{	
		_system("iwconfig wdev%dap0 key off", radio_index);
	}
	else if((strcmp(wlan_security,"wep_open_64") == 0))
	{
		set_wep_key(64, bss_index, radio_index, "open");
	}
	else if((strcmp(wlan_security,"wep_share_64") == 0))
	{
		set_wep_key(64, bss_index, radio_index, "restricted");
	}       
	else if((strcmp(wlan_security,"wep_open_128") == 0))
   	{
		set_wep_key(128, bss_index, radio_index, "open");
	}
	else if((strcmp(wlan_security,"wep_share_128") == 0))  
	{
		set_wep_key(128, bss_index, radio_index, "restricted");
	}
	else if((strcmp(wlan_security,"wep_open_152") == 0))
   	{
		set_wep_key(152, bss_index, radio_index, "open");
	}
	else if((strcmp(wlan_security,"wep_share_152") == 0))  
	{
		set_wep_key(152, bss_index, radio_index, "restricted");
	}
	else if((strcmp(wlan_security,"wpa_psk") == 0))
	{
		wpa_auth = AUTH_WPA_PSK;
		wpa_flag = 1;
	}
	else if((strcmp(wlan_security,"wpa2_psk") == 0))
	{
		wpa_auth = AUTH_WPA2_PSK;
		wpa_flag = 1;
	}
	else if((strcmp(wlan_security,"wpa2auto_psk") == 0))
	{
		wpa_auth = AUTH_WPA_WPA2_PSK_MIXED;
		wpa_flag = 1;
	}
	else if((strcmp(wlan_security,"wpa_eap") == 0))
	{
		//_system("iwpriv wdev%dap0 wpawpa2mode %d", radio_index, AUTH_WPA_PSK);
		wpa_flag = 2;
		sprintf(tmp_wpa, "wpa=%s", "1");	// wpa=0 bit0 = WPA
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa=", tmp_wpa);
		sprintf(tmp_wpa, "wpa_key_mgmt=%s", "WPA-EAP");
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa_key_mgmt=", tmp_wpa);
	}
	else if((strcmp(wlan_security,"wpa2_eap") == 0))
	{
		//_system("iwpriv wdev%dap0 wpawpa2mode %d", radio_index, AUTH_WPA2_PSK);
		wpa_flag = 2;
		sprintf(tmp_wpa, "wpa=%s", "2");	// wpa=0 bit1 = WPA2 IEEE 802.11i/RSN (WPA2) (dot11RSNAEnabled)
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa=", tmp_wpa);
		sprintf(tmp_wpa, "wpa_key_mgmt=%s", "WPA-EAP");
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa_key_mgmt=", tmp_wpa);
	}
	else if((strcmp(wlan_security,"wpa2auto_eap") == 0))
	{
		//_system("iwpriv wdev%dap0 wpawpa2mode %d", radio_index, AUTH_WPA_WPA2_PSK);
		wpa_flag = 2;
		sprintf(tmp_wpa, "wpa=%s", "3");	// wpa=0 bit1 = WPA2 IEEE 802.11i/RSN (WPA2) (dot11RSNAEnabled)
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa=", tmp_wpa);
		sprintf(tmp_wpa, "wpa_key_mgmt=%s", "WPA-EAP");
		file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa_key_mgmt=", tmp_wpa);
	}

	/* cipher type*/
	if(wpa_flag == 1) { /* only WPA-PSK */
		char *wlan_cipher = "none";
		char * wlan_rekey_time;
		int rekey_time = 120;

		if (wpa_auth == AUTH_WPA_PSK) {
			_system("iwpriv wdev%dap0 wpawpa2mode %d", radio_index, AUTH_WPA_PSK);
		    _system("iwpriv wdev%dap0 passphrase \"%s %s\"", 
				radio_index, "wpa", wlan_psk_pass_phrase);
		} else if (wpa_auth == AUTH_WPA2_PSK) {
			_system("iwpriv wdev%dap0 wpawpa2mode %d", radio_index, AUTH_WPA2_PSK);
		    _system("iwpriv wdev%dap0 passphrase \"%s %s\"", 
				radio_index, "wpa2", wlan_psk_pass_phrase);
		} else if (wpa_auth == AUTH_WPA_WPA2_PSK_MIXED) {
			_system("iwpriv wdev%dap0 wpawpa2mode %d", radio_index, AUTH_WPA_WPA2_PSK_MIXED);
			/* mixed mode, set WPA and WPA2 passphrase */
		    _system("iwpriv wdev%dap0 passphrase \"%s %s\"", 
				radio_index, "wpa", wlan_psk_pass_phrase);
		    _system("iwpriv wdev%dap0 passphrase \"%s %s\"", 
				radio_index, "wpa2", wlan_psk_pass_phrase);
		} else {
			//DEBUG_MSG("Invaild");
		}
		
		/* cipher type*/
		wlan_cipher = nvram_safe_get(tmp2);
		if(strcmp(wlan_cipher, "tkip") == 0)
		{
			_system("iwpriv wdev%dap0 ciphersuite \"%s %s\"", 
				radio_index, (psk_passphrase_mode==AUTH_WPA_PSK?"wpa":"wpa2"), "tkip");
		}
		else if(strcmp(wlan_cipher, "aes")== 0)
		{
			if (wpa_auth == AUTH_WPA_PSK) //should only wpa2 support AES
				DEBUG_MSG("WPA NOT support AES: please block this setup on GUI!\n");
			_system("iwpriv wdev%dap0 ciphersuite \"%s %s\"", 
				radio_index, (psk_passphrase_mode==AUTH_WPA_PSK?"wpa":"wpa2"), "aes-ccmp"); 
		}
		else // TKIP and AES, support??
		{
			_system("iwpriv wdev%dap0 ciphersuite \"%s %s\"", 
				radio_index, "wpa", "tkip"); 
			_system("iwpriv wdev%dap0 ciphersuite \"%s %s\"", 
				radio_index, "wpa2", "aes-ccmp");
		}
	
        wlan_rekey_time = nvram_safe_get(tmp);
		rekey_time = atoi(wlan_rekey_time);
		if (rekey_time < 60 || rekey_time > 86400) {
			DEBUG_MSG("Invaild group rekey interval(%d), use 120 for instead\n", rekey_time);
			rekey_time = 120;
		}

	} else { //not wpa/wpa2, turn it off.
		_system("iwpriv wdev%dap0 wpawpa2mode 0", radio_index);
	}
   
	/* start WPA WPA2 Enterprise */
	if(wpa_flag == 2 && nvram_match("wps_enable", "0") == 0) {
		
		/* RF is switchable radio_mode for distinguish 2.4G/5G */
		/* radio mode */
		/* if RF is switchable like 2.4/5G switchable, 
			if (RADIO_NUMS == 1), then
			radio_mode (which is wlan0/(1?)_radio_mode) == 0 means 2.4G; == 1 means 5G
		*/
		if (RADIO_NUMS == 1) {
			set_radius(radio_mode);
		} else if (RADIO_NUMS > 1) {
			set_radius(radio_index);
		} else {
			DEBUG_MSG("error..radio number is less 1\n");
			return;
		}
	
		cipher_type = nvram_safe_get(tmp3); //wlan0_psk_cipher_type
		if(strcmp(cipher_type, "tkip")== 0 ) {
			sprintf(tmp_wpa, "wpa_pairwise=%s", "TKIP");
			file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa_pairwise=", tmp_wpa);
		} else if(strcmp(cipher_type, "aes")== 0) {
			sprintf(tmp_wpa, "wpa_pairwise=%s", "CCMP");
			file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa_pairwise=", tmp_wpa);
		} else {	
			sprintf(tmp_wpa, "wpa_pairwise=%s", "TKIP CCMP");
			file_strncmp2replace("/etc/wsc/wdev0ap0/hostapd.conf", "wpa_pairwise=", tmp_wpa);
		}
	}

}

static void set_wmm(int bss_index, int radio_index)
{
    char *wlan0_wmm_enable;
    char wmm_enable[30];
	int wmm = 0;
 
 	if (RADIO_NUMS == 1) {
		if (bss_index > 0)
			sprintf(wmm_enable, "wlan%d_vap%d_wmm_enable", atoi(nvram_safe_get("wlan0_radio_mode")),
				       	bss_index);
		else
    		sprintf(wmm_enable, "wlan%d_wmm_enable", atoi(nvram_safe_get("wlan0_radio_mode")));
	}
	else if (RADIO_NUMS > 1) {
		if (bss_index > 0)
			sprintf(wmm_enable, "wlan%d_vap%d_wmm_enable", radio_index,
				       	bss_index);
		else
    		sprintf(wmm_enable, "wlan%d_wmm_enable", radio_index);
	}
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	} 
  
	wlan0_wmm_enable = nvram_safe_get(wmm_enable);
    	
   	if((strcmp(wlan0_wmm_enable,"0") == 0)) {
		wmm = 0;
	}
   	else {
		wmm = 1;
   	}
	_system("iwpriv wdev%d wmm %d", radio_index, wmm);
}

static void set_gprotect(int index)
{
	enum {
		DOT11G_PROTECT_OFF = 0,
		DOT11G_PROTECT_AUTO,
	};
	char tmp[64];
	char *wlan_gprotect;
	int gprotect = DOT11G_PROTECT_AUTO;
	if (RADIO_NUMS == 1) {
		sprintf(tmp, "wlan%d_11g_protection", atoi(nvram_safe_get("wlan0_radio_mode")));
	}
	else if (RADIO_NUMS > 1) {
		sprintf(tmp, "wlan%d_11g_protection", index);
	}
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}
	wlan_gprotect = nvram_safe_get(tmp);
	if (strcmp(wlan_gprotect, "auto") == 0) {
		gprotect = DOT11G_PROTECT_AUTO;	
	} else if (strcmp(wlan_gprotect, "off") == 0) {
		gprotect = DOT11G_PROTECT_OFF;	
	} else {
		gprotect = DOT11G_PROTECT_AUTO;	/* default */
	}
	
	_system("iwpriv wdev%d gprotect %d", index, gprotect);
	return;
}

static void set_GI_BW(int index)
{
	/* Guard Interval: 11N only */
	enum {
		GUARD_INTERVAL_AUTO = 0,
		GUARD_INTERVAL_SHORT,
		GUARD_INTERVAL_LONG,
	};
	/* Channel BandWidth: 11N only */
	enum {
		HT_CH_BW_AUTO = 0, /* auto channel bandwidth */
		HT_CH_BW_RES,      /* Reserved */
		HT_CH_BW_20,       /* 20 MHz channel bandwidth */
		HT_CH_BW_40,       /* 40 MHz channel bandwidth */
	};
	/* 802.11N Protection: 11N only */
	enum {
		DOT11N_PROTECT_OFF = 0, /* diable/off (default) */
		DOT11N_PROTECT_OPT,     /* optional */
		DOT11N_PROTECT_20,      /* protect only 20 MHz */
		DOT11N_PROTECT_20_40,      /* protect both 20/40 MHz */
		DOT11N_PROTECT_AUTO,    /* auto */
	};
	/* Extension Sub-Channel: 11N only */
	enum {
		EXT_SUB_CH_AUTO = 0, /* auto(default) */
		EXT_SUB_CH_LOWER,     /* use lower channel */
		EXT_SUB_CH_UPPER,     /* use upper channel */
	};
	char *wlan_dot11_mode, *wlan_auto_channel_enable, *wlan_channel;
	char *wlan_gi, *wlan_protect_n, *wlan_channel_bw, *wlan_gi_;
	char tmp[64], tmp2[64], tmp3[64], tmp4[64], tmp5[64], tmp6[64], tmp7[64];
	int gi = GUARD_INTERVAL_AUTO, htbw = HT_CH_BW_AUTO, 
		protect = DOT11N_PROTECT_AUTO, subch = EXT_SUB_CH_AUTO; /* default settings */

	if (RADIO_NUMS == 1) {
		sprintf(tmp, "wlan%d_dot11_mode", atoi(nvram_safe_get("wlan0_radio_mode")));
		sprintf(tmp2, "wlan%d_auto_channel_enable", atoi(nvram_safe_get("wlan0_radio_mode")));
		sprintf(tmp3, "wlan%d_channel", atoi(nvram_safe_get("wlan0_radio_mode")));
		sprintf(tmp4, "wlan%d_short_gi", atoi(nvram_safe_get("wlan0_radio_mode")));
		sprintf(tmp5, "wlan%d_11n_protection",atoi(nvram_safe_get("wlan0_radio_mode")));
		sprintf(tmp6, "wlan%d_channel_bandwidth",atoi(nvram_safe_get("wlan0_radio_mode")));
		sprintf(tmp7, "wlan%d_guard_interval",atoi(nvram_safe_get("wlan0_radio_mode")));
	}
	else if (RADIO_NUMS > 1) {
		sprintf(tmp, "wlan%d_dot11_mode", index);
		sprintf(tmp2, "wlan%d_auto_channel_enable", index);
		sprintf(tmp3, "wlan%d_channel", index);
		sprintf(tmp4, "wlan%d_short_gi", index);	
		sprintf(tmp5, "wlan%d_11n_protection", index);
		sprintf(tmp6, "wlan%d_channel_bandwidth", index);
		sprintf(tmp7, "wlan%d_guard_interval", index);
	}
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	wlan_dot11_mode = nvram_safe_get(tmp);
	wlan_auto_channel_enable = nvram_safe_get(tmp2); 
	wlan_channel = nvram_safe_get(tmp3);
	    
	/* HT_GI(guard interval) */
	wlan_gi = nvram_safe_get(tmp4); /*TODO: should not get wlanx_short_gi from nvram, use wlanx_guard_interval for instead */
	if (strlen(wlan_gi)) {
		if (atoi(wlan_gi) == GUARD_INTERVAL_SHORT)
			gi = GUARD_INTERVAL_SHORT;
		else if (atoi(wlan_gi) == 0)/* 0 in nvram is mean for "long gi" */
			gi = GUARD_INTERVAL_LONG;
		else /* bad value in nvram, use default(AUTO) gi */
			gi = GUARD_INTERVAL_AUTO;
	} else /* no gi in nvram, use auto gi as default */
		gi = GUARD_INTERVAL_AUTO;
	/* set Guard Interval */
	_system("iwpriv wdev%d guardint %d", index, gi);

	
	/* HT_BW  HT_EXTCHA */
	/* set n protection */
	wlan_protect_n = nvram_safe_get(tmp5);
	if (strcmp(wlan_protect_n, "disable") == 0) {
		protect = DOT11N_PROTECT_OFF;
	} else if (strcmp(wlan_protect_n, "20") == 0) {
		protect = DOT11N_PROTECT_20; 
	} else if (strcmp(wlan_protect_n, "20_40") == 0) {
		protect = DOT11N_PROTECT_20_40; 
	}else if (strcmp(wlan_protect_n, "auto") == 0) {
		protect = DOT11N_PROTECT_AUTO;
	} else {
		protect = DOT11N_PROTECT_AUTO; /* default */
	} 
	/* set channel bandwidth */
	wlan_channel_bw = nvram_safe_get(tmp6);
	if (strcmp(wlan_channel_bw, "auto") == 0) {
		htbw = HT_CH_BW_AUTO; 
	} else if (strcmp(wlan_channel_bw, "20") == 0) {
		htbw = HT_CH_BW_20;	
	} else if (strcmp(wlan_channel_bw, "40") == 0) { 
		htbw = HT_CH_BW_40; 
	} else {
		htbw = HT_CH_BW_AUTO; /* auto as default */
	}

	/* HT_GI(guard interval) */
	wlan_gi_ = nvram_safe_get(tmp7);
	if (strlen(wlan_gi_)) {
		if (strcmp(wlan_gi_, "auto") == 0)
			gi = GUARD_INTERVAL_AUTO;
		else if (strcmp(wlan_gi_, "long") == 0)
			gi = GUARD_INTERVAL_LONG;
		else if (strcmp(wlan_gi_, "short") == 0)
			gi = GUARD_INTERVAL_SHORT;
	} else /* no gi in nvram, use auto gi as default */
		gi = GUARD_INTERVAL_AUTO;
	/* set Guard Interval */
	_system("iwpriv wdev%d guardint %d", index, gi);

	if(strcmp(wlan_dot11_mode, "11bgn")==0 ||
		strcmp(wlan_dot11_mode, "11gn")==0 ||
		(strcmp(wlan_dot11_mode, "11n")==0 && atoi(nvram_safe_get("wlan0_radio_mode")) == WIFI_G_BAND)) { /* 2.4G mode */
		if (strcmp(wlan_auto_channel_enable, "1") != 0) { /* not auto channel */
			if(strcmp(wlan_channel_bw, "20") != 0) { /* bandwidth:20/40 auto and 40 */
				if(atoi(wlan_channel) <= 4) {
					subch = EXT_SUB_CH_UPPER;
				} else if(atoi(wlan_channel) >= 8) {
					subch = EXT_SUB_CH_LOWER;
				}
			}
		}	
	}	
	    	
	if(strcmp(wlan_dot11_mode, "11an")==0 ||
	   (strcmp(wlan_dot11_mode, "11n")==0 && atoi(nvram_safe_get("wlan0_radio_mode")) == WIFI_A_BAND)) { /* 5G mode */
        if(strcmp(wlan_auto_channel_enable, "1") != 0) { /* not auto channel */
			/* U-NII Band */
			enum {
				UNII_LOWER_BAND_36 = 36,
				UNII_LOWER_BAND_40 = 40,
				UNII_LOWER_BAND_44 = 44,
				UNII_LOWER_BAND_48 = 48,
				UNII_MIDDLE_BAND_52 = 52,
				UNII_MIDDLE_BAND_56 = 56,
				UNII_MIDDLE_BAND_60 = 60,
				UNII_MIDDLE_BAND_64 = 64,
				UNII_UPPER_BAND_149 = 149,
				UNII_UPPER_BAND_153 = 153,
				UNII_UPPER_BAND_157 = 157,
				UNII_UPPER_BAND_161 = 161,
			};
			/* H or CEPT Band */
			enum {
				H_CEPT_BAND_100 = 100,
				H_CEPT_BAND_104 = 104,
				H_CEPT_BAND_108 = 108,
				H_CEPT_BAND_112 = 112,
				H_CEPT_BAND_116 = 116,
				H_CEPT_BAND_120 = 120,
				H_CEPT_BAND_124 = 124,
				H_CEPT_BAND_128 = 128,
				H_CEPT_BAND_132 = 132,
				H_CEPT_BAND_136 = 136,
				H_CEPT_BAND_140 = 140,
			};
			/* ISM Band */
			enum {
				ISM_BAND_165 = 165,
			};
			switch(atoi(wlan_channel)) {
				case UNII_LOWER_BAND_36:
				case UNII_LOWER_BAND_44:
				case UNII_MIDDLE_BAND_52:
				case UNII_MIDDLE_BAND_60:
				case H_CEPT_BAND_100:
				case H_CEPT_BAND_108:
				case H_CEPT_BAND_116:
				case H_CEPT_BAND_124:
				case H_CEPT_BAND_132:
				case UNII_UPPER_BAND_149:
				case UNII_UPPER_BAND_157:
					subch = EXT_SUB_CH_UPPER;
					break;
				case UNII_LOWER_BAND_40:
				case UNII_LOWER_BAND_48:
				case UNII_MIDDLE_BAND_56:
				case UNII_MIDDLE_BAND_64:
				case H_CEPT_BAND_104:
				case H_CEPT_BAND_112:
				case H_CEPT_BAND_120:
				case H_CEPT_BAND_128:
				case H_CEPT_BAND_136:
				case UNII_UPPER_BAND_153:
				case UNII_UPPER_BAND_161:
					subch = EXT_SUB_CH_LOWER;
					break;
				case ISM_BAND_165:
					htbw = HT_CH_BW_20; /*channel 165 for 802.11a only support 20Mhz bandwidth */
					break;
				default:
					DEBUG_MSG("channel=%d is not allow\n", atoi(wlan_channel));
					return;
			} //switch()
		} //else
	}

	/* set channel bandwidth */
	_system("iwpriv wdev%d htbw %d", index, htbw);
	/* set extension sub-channel */
	_system("iwpriv wdev%d extsubch %d", index, subch);
	/*set 11n protection */
	_system("iwpriv wdev%d htprotect %d", index, protect);
}
/* ssid */
static void set_ssid(int radio_index)
{
	char *wlan_ssid, tmp[64];

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_ssid", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_ssid", radio_index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	wlan_ssid = nvram_safe_get(tmp);
#ifndef SSID_HEX_MODE
	    if((strlen(wlan_ssid)==0) || (strlen(wlan_ssid)>32)) {
	        DEBUG_MSG("wlan ssid null pointer or too long. Default"
			       " : default \n");
	        wlan_ssid = "dlink_default";
			_system("iwconfig wdev%d%s%d essid %s", radio_index, WLAN_OPMODE[radio_index], 0/* TODO: multi-ssid index */, wlan_ssid);
	    }
#else
	    if((strlen(wlan_ssid) == 0) || (strlen(wlan_ssid) > 64)) {
	    	DEBUG_MSG("wlan ssid null pointer or too long. Default"
			       " : default \n");
			wlan_ssid = "dlink_default";
			_system("iwconfig wdev%d%s%d essid %s", radio_index, WLAN_OPMDOE[radio_index], 0/* TODO: multi-ssid index */, wlan_ssid);
	    }
#endif        
	    else {
		char ssid[128];
		char ssid_tmp[64];
		int len;
		int c1 = 0;
		int c2 = 0;
			
		memset(ssid, 0, sizeof(ssid));
		memset(ssid_tmp, 0, sizeof(ssid_tmp));
		len = strlen(wlan_ssid);
		strcpy(ssid_tmp, wlan_ssid);
		
		for(c1 = 0; c1 < len; c1++) {
			/*   ssid = ~!@#$%^&*()_+`-={}|[]\:";'<>?,./  */				
			/*   ` = 0x60, " = 0x22, \ = 0x5c, ' = 0x27, $ = 0x24     */	
			if(((ssid_tmp[c1] >= 0x20) && (ssid_tmp[c1] <= 0x2f)) || 
			   ((ssid_tmp[c1] >= 0x3a) && (ssid_tmp[c1] <= 0x40)) || 
			   ((ssid_tmp[c1] >= 0x5b) && (ssid_tmp[c1] <= 0x60)) || 
			   ((ssid_tmp[c1] >= 0x7b) && (ssid_tmp[c1] <= 0x7e))) { 
				ssid[c2] = 0x5c;
				c2++;
				ssid[c2] = ssid_tmp[c1];
			} else
				ssid[c2] = ssid_tmp[c1];
			c2++;		
		}
			_system("iwconfig wdev%d%s%d essid %s", radio_index, WLAN_OPMODE[radio_index], 0/* TODO: multi-ssid index */, ssid);
			
	    }
}

/* rts */
static void set_rts(int index)
{
	char *wlan_rts_threshold, tmp[64];
	int rts = 0;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_rts_threshold", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_rts_threshold", index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	wlan_rts_threshold = nvram_safe_get(tmp);
        if( 256 <= atoi(wlan_rts_threshold) && atoi(wlan_rts_threshold) <= 2347)
			rts = atoi(wlan_rts_threshold);
        else {
        	DEBUG_MSG("rts threshold value not correct. Default"
			       " : 2347 (OFF)\n");
			rts = 2347; //default
		}
		_system("iwconfig wdev%d rts %d", index, rts);
	    
}

/* hide ssid*/
static void set_hidessid(int index)
{
	char tmp[64]={0}, *ssid_bcast = NULL;
	int hide = 0;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_ssid_broadcast", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_ssid_broadcast", index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	if (strcmp(WLAN_OPMODE[index], "sta") == 0) { /* sta(ap client) mode doesn't need to set hidessid */
		DEBUG_MSG("wlan in sta mode, no need to set hidessid\n");
		return;	
	}

    ssid_bcast = nvram_safe_get(tmp);
	if (!strlen(ssid_bcast)) //no define in nvram
		hide = 0; //no hide.
	else
		hide = !(atoi(ssid_bcast));
	
   	_system("iwpriv wdev%dap0 hidessid %d", index, hide);
}

/* fragment */		
static void set_fragment(int index)
{
	char *wlan_fragmentation, tmp[64];
	int frag = 0;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_fragmentation", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_fragmentation", index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	wlan_fragmentation = nvram_safe_get(tmp);
	if(256 <= atoi(wlan_fragmentation) && atoi(wlan_fragmentation) <= 2346)
		frag = atoi(wlan_fragmentation);
	else {
		DEBUG_MSG("fragmentation value not correct. Default"
			       " : 2346 (OFF)\n");
		frag = 2346; //default.
	}
	        
    _system("iwconfig wdev%d frag %d", index, frag);
}

/* mode */
static void set_mode(int index)
{
	enum {
		DOT11_B_ONLY = 1,    /* 11b only */
		DOT11_G_ONLY,        /* 11g only */
		DOT11_BG_MIXED,      /* 11bg mixed */ 
		DOT11_N_ONLY,        /* 11n only */
		DOT11_GN_MIXED_,     /* 11gn mixed(unused)*/
		DOT11_GN_MIXED,      /* 11gn mixed */
		DOT11_BGN_MIXED,     /* 11bgn mixed(default) */
		DOT11_A_ONLY,        /* 11a only */
		DOT11_AN_MIXED = 12, /* 11an mixed */
	};

	char *wlan_dot11_mode, tmp[64];
	int dot11_mode = 0;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_dot11_mode", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_dot11_mode", index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}
	
	wlan_dot11_mode = nvram_safe_get(tmp);
	
	if (strcmp(wlan_dot11_mode, "11b")==0) {
		dot11_mode = DOT11_B_ONLY;
	} else if (strcmp(wlan_dot11_mode, "11gn")==0) {
		dot11_mode = DOT11_GN_MIXED;
	} else if (strcmp(wlan_dot11_mode, "11n")==0) {
		dot11_mode = DOT11_N_ONLY;
	} else if (strcmp(wlan_dot11_mode, "11bg")==0) {
		dot11_mode = DOT11_BG_MIXED;
	} else if (strcmp(wlan_dot11_mode, "11g")==0) {
		dot11_mode = DOT11_G_ONLY;
	} else if (strcmp(wlan_dot11_mode, "11a")==0) {
		dot11_mode = DOT11_A_ONLY;
	} else if (strcmp(wlan_dot11_mode, "11an")==0) {
		dot11_mode = DOT11_AN_MIXED;
	} else { //bgn mode(default)
		dot11_mode = DOT11_BGN_MIXED;
	}		    			    
	_system("iwpriv wdev%d opmode %d", index, dot11_mode);
}

static int set_domain(int index) 
{
	char *wlan_domain; 

	wlan_domain = nvram_safe_get("wlan0_domain"); /* Is possible to have two or more domain? */
	if (!wlan_domain) {
		DEBUG_MSG("error..no domain value in nvram, please check wlan0_domain!\n");
		return -1;
	}
	else{	
        _system("iwpriv wdev%d regioncode %d", index, strtol(wlan_domain, NULL, 16)); /* BUG: in sta(macclone=1) mode will cause system hang up !!!*/
	}
}

/* (auto) channel */
static int set_channel(int index)
{
	char *wlan_auto_channel_enable, *wlan_channel, tmp[64], tmp2[64];
	int wlan_autochannel = 0;

	if (RADIO_NUMS == 1) { 
		sprintf(tmp, "wlan%d_auto_channel_enable", atoi(nvram_safe_get("wlan0_radio_mode")));
		sprintf(tmp2, "wlan%d_channel", atoi(nvram_safe_get("wlan0_radio_mode")));
	} else if (RADIO_NUMS > 1) {
		sprintf(tmp, "wlan%d_auto_channel_enable", index);
		sprintf(tmp2, "wlan%d_channel", index);
	} else {
		DEBUG_MSG("error..radio number is less 1\n");
		return -1;
	}

	if (strcmp(WLAN_OPMODE[index], "sta") == 0) { /* sta(ap client) mode doesn't need to set channel */
		DEBUG_MSG("wlan in sta mode, no need to set channel\n");
		return 0;	
	}


	wlan_auto_channel_enable = nvram_safe_get(tmp);	
	if (strlen(wlan_auto_channel_enable) && atoi(wlan_auto_channel_enable))	
		wlan_autochannel = 1;
	/* setup autochannel */
	_system("iwpriv wdev%d autochannel %d",index, wlan_autochannel);

	if (!wlan_autochannel) 
	{
		int channel = 0;

		wlan_channel = nvram_safe_get(tmp2);
		if (strlen(wlan_channel))
			channel = atoi(wlan_channel);
		else 
			channel = 0;//no channel config in nvram.

        if(atoi(nvram_safe_get("wlan0_radio_mode")) == WIFI_G_BAND ) { //wlan0_radio_mode = 0 is 2.4G
			if (!(channel && channel <= 13)) {
           		DEBUG_MSG("2.4G channel value=%d error!!, set 6 for instead.\n", channel);
				channel = 6; //set 6 as default 2.4G channel
			} else {
	           	DEBUG_MSG("2.4G channel value=%d\n", channel);
			}
		} else if(atoi(nvram_safe_get("wlan0_radio_mode")) == WIFI_A_BAND) { //wlan0_radio_mode = 1 is 5G
			if (!(channel && channel <= 165)) {
           		DEBUG_MSG("5G channel value=%d error!!, set 40 for instead.\n", channel);
				channel = 40; //set 40 as default 5G channel
			} else {
	           	DEBUG_MSG("5G channel value=%d\n", channel);
			}
		} else { //wlan0_radio_mode = ??
           	DEBUG_MSG("radio/channel Error!! please check wlan0_radio_mode and wlan0_channel in nvram\n");
			return -1;
		}	
		/* set channel */	
        _system("iwconfig wdev%d channel %d", index, channel);
	}	
	return 0;		
}

/* WDS */
static void set_wds(int index)
{
	char *wlan_wds_enable, tmp[64];

	sprintf(tmp, "wlan%d_wds_enable", index);
	wlan_wds_enable = nvram_safe_get(tmp);    

	if(strcmp(wlan_wds_enable, "0") == 0){
	        _system("echo \"WdsEnable=0\" >> /var/tmp/iNIC_ap.dat");
#ifdef ATH_WDS_RPT
		sprintf(tmp, "wlan%d_op_mode", index);
		if(nvram_match(tmp, "rpt_wds") == 0) {
	        	_system("echo \"WdsEnable=0\" >> /var/tmp/iNIC_ap.dat");
		}
#endif
	} else if(strcmp(wlan_wds_enable, "1") == 0) {
	        _system("echo \"WdsEnable=1\" >> /var/tmp/iNIC_ap.dat");
#ifdef ATH_WDS_RPT
		sprintf(tmp, "wlan%d_op_mode", index);
		if(nvram_match(tmp, "rpt_wds") == 0) {
	        	_system("echo \"WdsEnable=1\" >> /var/tmp/iNIC_ap.dat");
		}
#endif			
	} else {
		DEBUG_MSG("WDS value error. Default : disable\n");
	        _system("echo \"WdsEnable=0\" >> /var/tmp/iNIC_ap.dat");
	}
	    	    

}

static void set_beacon_interval(int index)
{
	char *wlan_beacon_interval, tmp[64];
	int beacon = 100;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_beacon_interval", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_beacon_interval", index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	wlan_beacon_interval = nvram_safe_get(tmp);   
	beacon = atoi(wlan_beacon_interval); 

	/* beacon interval */
	if(!(20 <= beacon && beacon <= 1000)) {
		DEBUG_MSG("Invalid becon interval(%d) value, use 100 for instead! \n", beacon);
		beacon = 100;
	}
		        
    _system("iwpriv wdev%d bcninterval %d", index, beacon);
}

static void set_dtim(int index)
{
	char *wlan_dtim, tmp[64];
	int dtim = 1;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_dtim", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_dtim", index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	if (strcmp(WLAN_OPMODE[index], "sta") == 0) { /* sta(ap client) mode doesn't need to set DTIM */
		DEBUG_MSG("wlan in sta mode, no need DTIM\n");
		return;	
	}

	wlan_dtim = nvram_safe_get(tmp);  
	dtim = atoi(wlan_dtim);

	/* dtim period*/
	if( !(1 <= atoi(wlan_dtim) && atoi(wlan_dtim) <=  255) ) {
		DEBUG_MSG("wlan0_dtim value=%d not correct. Default :1 \n", dtim);
		dtim = 1;
	}
    _system("iwpriv wdev%dap0 dtim %d", index, dtim);
}

/* intra bss bridging */
static void set_intrabss(int index)
{
	char *wlan_isolation = NULL, tmp[64] = {0};
	int isolation = 0;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_partition", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_partition", index);
	else {
		DEBUG_MSG("error..radio number is 0\n");
		return;
	}

	if (strcmp(WLAN_OPMODE[index], "sta") == 0) { /* sta(ap client) mode doesn't need to set intrabss */
		DEBUG_MSG("wlan in sta mode, no need intra-bss\n");
		return;	
	}

	wlan_isolation = nvram_safe_get(tmp);    

	if (atoi(wlan_isolation) == 1)
		isolation = 1;
	else
		isolation = 0; /*default*/

    _system("iwpriv wdev%dap0 intrabss %d", index, !(isolation));

}

/* set tx power*/
static void set_max_power(int index)
{
	char *wlan_txpower = NULL, tmp[64] = {0};
	#define MAX_POWER_HIGH 18
	#define MAX_POWER_MIDD 13
	#define MAX_POWER_LOW  11
	int max_txpower = 0xff;
	static int last_txpower = 0xff;

	if (RADIO_NUMS == 1)
		sprintf(tmp, "wlan%d_txpower", atoi(nvram_safe_get("wlan0_radio_mode")));
	else if (RADIO_NUMS > 1)
		sprintf(tmp, "wlan%d_txpower", index);
	else {
		DEBUG_MSG("error..radio number is less 1\n");
		return;
	}

	wlan_txpower = nvram_safe_get(tmp);    
	/*TODO: */
	if (strcmp(wlan_txpower, "high") == 0)
	{	
		if (last_txpower != 0xff) 
			max_txpower = MAX_POWER_HIGH;
		else {
			DEBUG_MSG("default max tx power = 0xff\n");
			return;
		}
	}
	else if (strcmp(wlan_txpower, "medium") == 0)
		max_txpower =  MAX_POWER_MIDD;
	else if (strcmp(wlan_txpower, "low") == 0)
		max_txpower =  MAX_POWER_LOW;
	else {
		DEBUG_MSG("error, txpower should be high, middle or low\n");
		return;
	}
	last_txpower = max_txpower;
    _system("iwpriv wdev%d maxtxpower %d", index, max_txpower);
}

/* 2.4G and 5G have the same prefix wlan0_ */
/* only channel and dot11_mode has wlan0_ and wlan1_ */
int mv88w8366_config(int index)
{
	/**********************************************************
 	 * 1. suppose AP mode, other modes are future work: e.g. APC 
 	 * 2. only setup single SSID
 	 **********************************************************/
	int radio_mode=0, stamode=0;

	/* ssid */
	set_ssid(index);

	/* hide ssid */
	set_hidessid(index);

	/* HT_GI HT_BW */
	set_GI_BW(index);	    
	
	/* (auto) channel */
	/*
     *  need set_channel() after set_GI_BW().
     *  channel 40, 48, 153, 161..., with bandwidth 20/40 have no Auth. response issue.
     */ 
	set_channel(index); 
	    	    
	/* G protection */
	set_gprotect(index);
		    	    
	/* mode */
	set_mode(index);

	/* security*/
	set_security(0, index);

	/* wmm */
	set_wmm(0, index);

	/* wds */
	//set_wds(index); //support??
	    
	/* beacon interval*/
	set_beacon_interval(index);

	/* dtim */
	set_dtim(index);

	/* rts */
	set_rts(index);
	
	/* fragment */		
	set_fragment(index);

	/* intra bss bridging (AP/STA isolation)*/
	set_intrabss(index);

	/* set max tx power */
	set_max_power(index);
	
    _system("iwpriv wdev%d%s%d ampdutx %d", index, WLAN_OPMODE[index], 0 /* TODO: multi-ssid */, 1);
  	_system("iwpriv wdev%d%s%d optlevel %d", index, WLAN_OPMODE[index], 0 /* TODO: muiti-ssid */, 1);
   	_system("iwpriv wdev%d%s%d mcastproxy %d", index, WLAN_OPMODE[index], 0 /* TODO: multi-ssid */, 1); /* enable multicast proxy */

	
	radio_mode = atoi(nvram_safe_get("wlan0_radio_mode")); /*TODO: move below codes to be a function */
	if (strcmp(WLAN_OPMODE[index], "sta") == 0) { /* sta(ap client) mode */
		switch(radio_mode) {
			case 0:	// 2.4G
				stamode = 7;
			break;
			case 1: // 5G
				stamode = 8;
			break;
			case 2: // 2.4G/5G Auto
				stamode = 6;
			break;
			default:
				DEBUG_MSG("error..not support radio_mode value in nvram, please check \"wlan%d_radio_mode\"!\n", radio_mode);
			break;
		}
	    	_system("iwpriv wdev%d%s%d stamode %d", index, WLAN_OPMODE[index], 0 /* TODO: multi-ssid */, stamode/* 2.4G/5G/Auto */); /* sta mode */
	}
}

static int get_radio_nums()
{
	int i;
	RADIO_NUMS = 0;

	for(i = 0; i < MAX_RADIO_NUMS ; i++) {
		char tmp[32] = {0};
		char *radio_mode = NULL;

		sprintf(tmp, "wlan%d_radio_mode", i);
		radio_mode = nvram_safe_get(tmp);
		if (!strlen(radio_mode)) //no value in nvram
		{
			if (RADIO_NUMS == 0) {
				RADIO_NUMS = 1; /* at lease one radio */
				return 1;
			} else 
				return 0;
		} else {
			RADIO_NUMS++;
		}
	}
	return 0;
}


/* operation mode: AP, APC...*/
static int get_opmode()
{
	char *opmode = NULL;
	int i;
	for (i = 0;i < MAX_RADIO_NUMS ;i++) {
		char tmp[24] = {0};
		sprintf(tmp, "wlan%d_op_mode", i);
		opmode = nvram_safe_get(tmp);
		if (strcmp(opmode, "AP") == 0) {
			opmode = "ap";
		} else if (strcmp(opmode, "APC") == 0) {
			opmode = "sta";
		} else {
			DEBUG_MSG("error..no opmode value in nvram, please check \"wlan%d_op_mode\"!\n", i);
			return -1;
		}
		strcpy(WLAN_OPMODE[i], opmode);  
	}
}

static int get_mssid_nums()
{
	/* TODO ...*/
	MSSID_NUMS = 1; /*at lease one ssid */
}

/* get wlan configurations form nvram */
int get_board_config()
{
	/* get radio numbers */
	get_radio_nums();
	
	get_opmode();
	
	get_mssid_nums();
}

void mv88w8366_bringup()
{
	char mac[13], *ptr = NULL;
	int idx = 0;
	int WLAN_BAND;
	int i;	

	memset(mac, '\0', sizeof(mac));

#ifdef SET_GET_FROM_ARTBLOCK
        if (artblock_get("lan_mac"))
                ptr = artblock_safe_get("lan_mac");
        else
                ptr = nvram_safe_get("lan_mac");
#else
        ptr = nvram_safe_get("lan_mac");
#endif

        if ((strcmp(nvram_safe_get("wlan0_mac"), ptr) != 0) ||
            (strcmp(nvram_safe_get("wlan1_mac"), ptr) != 0)
        ) {
                nvram_set("wlan0_mac", ptr);
                nvram_set("wlan1_mac", ptr);
                nvram_commit();
        }

	/* reformat wireless mac to remove ":" string*/
	mac[0] = ptr[0]; mac[1] = ptr[1];
	mac[2] = ptr[3]; mac[3] = ptr[4];
	mac[4] = ptr[6]; mac[5] = ptr[7];
	mac[6] = ptr[9]; mac[7] = ptr[10];
	mac[8] = ptr[12]; mac[9] = ptr[13];
	mac[10] = ptr[15]; mac[11] = ptr[16];
	mac[12] = '\0';

	/* off all wlan led */
	for (i=0; i < MAX_RADIO_NUMS; i++) // TODO: more than two radios?
		_system("gpio %s off &", (i == WIFI_G_BAND)?"WLAN_LED":"WLAN_LED_2"); 

	for (i = 0; i < RADIO_NUMS; i++)
	{
		char *wlan_enable = NULL;
		char tmp[24];
		if (RADIO_NUMS == 1) 
			sprintf(tmp, "wlan%d_enable", atoi(nvram_safe_get("wlan0_radio_mode")));
		else
			sprintf(tmp, "wlan%d_enable", i);
		wlan_enable = nvram_safe_get(tmp);	
		if(strlen(wlan_enable) && atoi(wlan_enable)) {
	                _system("iwpriv wdev%d bssid %s", i, mac); /* set wireless mac */
			_system("ifconfig wdev%d up", i); /* bring up wlan radio */

			/* multi-SSID, maybe future work... */
			if (!strcmp(WLAN_OPMODE[i],"ap")) //should be ap mode, which has mssid feature
			{
				int j;
				for (j=0; j < MSSID_NUMS;j++) {
				    _system("iwpriv wdev%dap%d bssid %s", i, j, mac); /* set wireless ap mac */
					_system("ifconfig wdev%dap%d up", i, j); /* bring up "j"th ap virtual interface */
					_system("brctl addif %s wdev%dap%d", nvram_safe_get("lan_bridge"), i, j); /* bridge it in */
				}
			} else if (!strcmp(WLAN_OPMODE[i],"sta")) { //APC mode, should dosen't have mssid
				_system("iwpriv wdev%dsta0 bssid %s", i, mac); /* set wireless sta mac */
				_system("brctl addif %s wdev%dsta0", nvram_safe_get("lan_bridge"), i); /* bridge sta in?? */
				_system("ifconfig wdev%dsta0 up", i); /* bring up sta virtual interface */
			}

			if (RADIO_NUMS == 1) 
				WLAN_BAND =  atoi(nvram_safe_get("wlan0_radio_mode"));
			else
				WLAN_BAND = i;
			
			_system("gpio %s on &", (WLAN_BAND == WIFI_G_BAND)?"WLAN_LED":"WLAN_LED_2"); 
		} 
	}
}

void mv88w8366_postconfig(int index)
{
	if (strcmp(WLAN_OPMODE[index], "ap") == 0) { /* only for ap mode?? */
		if (RADIO_NUMS == 1) {
			/* domain */
			set_domain(index); // this command works only after interface(wdev0) up

			_system("iwconfig wdev%d commit", index); // needed?? (Auth-response)
		} else { /*TODO: two or more radios concurrent?? */
		}
	}
}

void mv88w8366_shutdown()
{
	int i;
	for (i = 0; i < RADIO_NUMS; i++)
	{
		char *wlan_enable = NULL;
		char tmp[24];
		if (RADIO_NUMS == 1) 
			sprintf(tmp, "wlan%d_enable", atoi(nvram_safe_get("wlan0_radio_mode")));
		else
			sprintf(tmp, "wlan%d_enable", i);
		wlan_enable = nvram_safe_get(tmp);	
		if(strlen(wlan_enable)) { // just check this key exist, will disable wireless whatever wlan on/off.
			_system("ifconfig wdev%d down", i); /* shut down wlan radio */
			/* multi-SSID, maybe future work... */
			if (!strcmp(WLAN_OPMODE[i],"ap")) //should be ap mode, which has mssid feature
			{
				int j;
				for (j=0; j < MSSID_NUMS;j++) {
					_system("ifconfig wdev%dap%d down", i, j); /* shut down "j"th ap virtual interface */
					_system("brctl delif %s wdev%dap%d", nvram_safe_get("lan_bridge"), i, j); /* bridge it out */
				}
			} else if (!strcmp(WLAN_OPMODE[i],"sta")) { //APC mode, should dosen't have mssid
				_system("brctl delif %s wdev%dsta0", nvram_safe_get("lan_bridge"), i); /* bridge sta out?? */
				_system("ifconfig wdev%dsta0 down", i); /* shut down sta virtual interface */
			}
		}
	}

	/* off all wlan led */
	kill_wlanled_pid();
	for (i=0; i < MAX_RADIO_NUMS; i++) // TODO: more than two radios?
		_system("gpio %s off &", (i == WIFI_G_BAND)?"WLAN_LED":"WLAN_LED_2"); 

}

static void wlan_rmmod(void)
{
	_system("rmmod ap8x");
}

static void wlan_insmod(void)
{
	_system("insmod /lib/modules/ap8x.ko");
}

int start_ap(void)
{
	int radio_index = 0, i = 0;
	char tmp[32];

	/* FIXME: */
//	if(!(action_flags.all || action_flags.wlan || action_flags.firewall))
//		return 1;

	DEBUG_MSG("Start AP\n");	

	get_board_config();

	/* set interface attribute */
    // set configuration date for mv88w8366 before interface wdev0 up 
	for (i = 0;i < RADIO_NUMS; i++)
		mv88w8366_config(i);

	/* set up interface */
	mv88w8366_bringup();

	/* set configuration data after interface wdev0 up */
	for (i = 0;i < RADIO_NUMS; i++)
		mv88w8366_postconfig(i);

	/* start hostapd */
	if (strcmp(WLAN_OPMODE[radio_index], "ap") == 0) { /* hostapd running for ap mode */
		for (i = 0;i < RADIO_NUMS; i++) {
			if (RADIO_NUMS == 1) 
				sprintf(tmp, "wlan%d_security", atoi(nvram_safe_get("wlan0_radio_mode")));
			else if (RADIO_NUMS > 1)
				sprintf(tmp, "wlan%d_security", i);

			if(nvram_match("wps_enable", "0") == 0 &&
					(nvram_match(tmp,"wpa_eap") == 0 ||
					 nvram_match(tmp,"wpa2_eap") == 0 ||
					 nvram_match(tmp,"wpa2auto_eap") == 0)) {
				_system("hostapd -B /etc/wsc/wdev%dap%d/hostapd.conf", i, i);
			}
		}
	}
   	DEBUG_MSG("finish start_ap()\n");

   	return 0;
}

int start_wlan(void)
{
	int wlan_band = -1;
	int i;
	DEBUG_MSG(" start_wlan\n");

	/* FIXME: */
	/* Add loopback */
	config_loopback();
//	ifconfig_up("lo", "127.0.0.1", "255.0.0.0");
//	route_add("lo", "127.0.0.0", "255.0.0.0", "0.0.0.0", 0);
	
	if(nvram_match("wlan0_enable", "1") == 0 || nvram_match("wlan1_enable", "1") == 0) {
		DEBUG_MSG("********** Start WLAN **************\n");

		wlan_band = atoi(nvram_safe_get("wlan0_radio_mode"));
		if (wlan_band != WIFI_G_BAND && wlan_band != WIFI_A_BAND)
		{	
			wlan_band = -1; /* -1 as wirless disabled */
		} 
		/* insert modules */
		if (!wlan_restart)
			wlan_insmod();

		/* set up ap */
		start_ap();
	} else {
		DEBUG_MSG("******* Wireless Radio OFF *********\n");
		wlan_restart = -1; /* -1 as system initialized, wlan is disabled */	
		wlan_band = -1; /* -1 as wirless disabled */
	}

#ifdef CONFIG_WIRELESS_SCHEDULE 
	_system("killall -SIGTSTP timer &");
#endif

	DEBUG_MSG("********** Start WLAN Finished**************\n");
	if (wlan_band != -1) {
		kill_wlanled_pid();
		_system("wlutils wlan led blink %s &", (wlan_band == WIFI_G_BAND)?"2.4G":"5G");
	}
	else {
		kill_wlanled_pid();
		/* off all wlan led */
		for (i=0; i < MAX_RADIO_NUMS; i++) // TODO: more than two radios?
			_system("gpio %s off &", (i == WIFI_G_BAND)?"WLAN_LED":"WLAN_LED_2"); 
	}
}

int stop_wlan(void)
{
	int radio_index = 0;

	/* FIXME: */
//	if(!( action_flags.all || action_flags.wlan ) )
//		return 1;
   
	wlan_mvl_enable = -1; /* -1 as wireless disabled */ 
	/* set down interface*/
	mv88w8366_shutdown();
	/* unload modules */
	//wlan_rmmod();
	wlan_restart = 1;
}
