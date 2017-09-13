#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "cmds.h"
#include "nvram.h"
#include "shutils.h"


#define MAX 3
#define CONFIG "/var/tmp/iNIC_ap.dat"


/* MulitSSID LIST Formart
 * 0-uneable | 1-enable (enable);
 * 0-no schedule (schedule);
 * 0 | 1 |2 (MULTI-SSID-INDEX);
 * NAME;
 * 0 | 1 (Visibility Status)
 * 0 | 1 (WLAN)
 * 0 | 1 (Enable Routing Between Zones)
 * 0-None | 1-WEP | 2-WPA | 3-EAP (Security Mode)
 * ENABLE;SCHEDULE;SSID-INDEX;NAME;STATUS;WALN;SECURITY
 * EX List1: 1;0;0;guest1;1;1;1;0
 *
 */

struct MultiSSID
{
	char ENABLE[2];
	char SCHEDULE[2];
	char SSIDINDEX[2];
	char NAME[256];
	char STATUS[2];
	char WLAN[2];
	char ERBZ[2];
	char SECURITY[2];
	char DEVICE[4];
} multissid[MAX];

static void set_ssid(int index)
{
	char key[256], ssid[256], tmp_data[256];
	char *ptr;
	int fd;
	
	if(strcmp(multissid[index].NAME, "") != 0) {
		if((fd = open(CONFIG, O_RDWR)) != -1) {
			sprintf(key, "SSID%d", index+2);
			sprintf(ssid, "%s=%s", key, multissid[index].NAME);
			stream_replace_line(fd, key, ssid);
			set_public_key(fd, "NoForwarding", multissid[index].WLAN);
			set_public_key(fd, "NoForwardingBTNBSSID", multissid[index].WLAN);
		}else{
			system("cp /etc/Wireless/RT2880/iNIC_ap.dat /var/tmp/");
			set_ssid(index);
		}
	}
}

static void set_key(const char *key, int index, int num, int fd)
{
	char key_data[256], _key[256];
	sprintf(key_data, "Key%dStr%d=%s", num, index, key);
	sprintf(_key, "Key%dStr%d", num, index);
	stream_replace_line(fd, _key, key_data);
}

static int set_public_key(int fd, const char *str, const char *type)
{
	char tmp[256], tmp1[256], key[256];
	char *ptr_key;
	int ret = -1;

	ptr_key = stream_search(fd, str, tmp, sizeof(tmp));
	if(ptr_key == NULL)
		goto out;
	sscanf(tmp, "%s", tmp1);
	sprintf(key,"%s;%s", tmp1, type);
	stream_replace_line(fd, tmp1, key);
	ret = 0;
out:
	return ret;
}

static void set_wep(int index, int fd)
{
	int i, ret = -1;
	char str_wep[256], tmp[256], tmp1[256];
	char *ptr, *wep_key_len, *default_key, *auth_type, *ptr_key, *_ptr;
	char key1[256], key2[256], key3[256], key4[256], key[256];
	
	sprintf(str_wep, "%s", 
                nvram_safe_get_i("multi_ssid_security", index, g1));
	ptr = str_wep;
	wep_key_len = strsep(&ptr, ";");
	default_key = strsep(&ptr, ";");
	auth_type = ptr;
	for( i = 1; i < 5; i++) {
		sprintf(tmp, "wlan%d_wep%s_key_%d", index+2, wep_key_len, i);
		switch(i) {
			case 1: sprintf(key1, "%s", nvram_safe_get(tmp)); break;
			case 2: sprintf(key2, "%s", nvram_safe_get(tmp)); break;
			case 3: sprintf(key3, "%s", nvram_safe_get(tmp)); break;
			case 4: sprintf(key4, "%s", nvram_safe_get(tmp)); break;
		}
	}

	bzero(tmp, sizeof(tmp));
	ptr_key = stream_search(fd, "DefaultKeyID", tmp, sizeof(tmp));
	if(ptr_key == "" || ptr_key == NULL) {
		sprintf(key,"DefaultKeyID=%s", default_key);
		stream_replace_line(fd, "DefaultKeyID", key);
	}else{	
		sscanf(tmp, "%s", tmp1);
		sprintf(key,"%s;%s", tmp1, default_key);
		stream_replace_line(fd, tmp1, key);
		sprintf(key,"DefaultKeyID=%s", default_key);
	}
	/* set wep key*/
	set_key(key1, index+2, 1, fd);
	set_key(key2, index+2, 2, fd);
	set_key(key3, index+2, 3, fd);
	set_key(key4, index+2, 4, fd);

	/* set AuthMode */
	if(strcmp(auth_type, "open") == 0){
		set_public_key(fd, "AuthMode", "OPEN");
	}else{
		set_public_key(fd, "AuthMode", "SHARED");
	}
	/* set EncrypType */
	set_public_key(fd, "EncrypType", "WEP");
	/* set HideSSID */
	set_public_key(fd, "HideSSID", multissid[index].STATUS);
}

static void set_wpa(int index, int fd)
{
	char str_wpa[256], str_psk[256], key[256], key_data[256];
	char *ptr, *mode, *type, *timer, *ptr_key;
	char *psk;
	sprintf(str_wpa, "%s",
                nvram_safe_get_i("multi_ssid_security", index, g1));
	ptr = str_wpa;
	mode = strsep(&ptr, ";");
	type = strsep(&ptr, ";");
	timer = ptr;

	sprintf(str_psk, "%s", 
		nvram_safe_get_i("multi_ssid_psk", index, g1));
	
		set_public_key(fd, "HideSSID", multissid[index].STATUS);
		/* set psk */
		sprintf(key, "WPAPSK%d", index+2);
		sprintf(key_data, "WPAPSK%d=%s", index+2, str_psk);
		stream_replace_line(fd, key, key_data);

		/* set AuthMode */
		if(strcmp(mode, "auto") == 0)
			set_public_key(fd, "AuthMode", "WPAPSKWPA2PSK");
		if(strcmp(mode, "wpa") == 0)
			set_public_key(fd, "AuthMode", "WPAPSK");
		if(strcmp(mode, "wpa2") == 0)
			set_public_key(fd, "AuthMode", "WPA2PSK");
			
		/* set EncrypType */
		if(strcmp(type, "tkip") == 0)
			set_public_key(fd, "EncrypType", "TKIP");
		if(strcmp(type, "aes") == 0)
			set_public_key(fd, "EncrypType", "AES");
		if(strcmp(type, "both") == 0)
			set_public_key(fd, "EncrypType", "TKIPAES");

}

static void set_radius(int fd, const char *_key, char *data1)
{
	char *ptr_key;
	char tmp[256], tmp1[256], key[256];
	
	ptr_key = stream_search(fd, _key, tmp, sizeof(tmp));
	if(ptr_key == "" || ptr_key == NULL) {
		sprintf(key,"%s%s;", _key, data1);
		stream_replace_line(fd, _key, key);
	}else{
		sscanf(tmp, "%s", tmp1);
		sprintf(key, "%s%s;", tmp1, data1);
		stream_replace_line(fd, _key, key);
	}
}

static void set_eap(int index, int fd)
{	
	char str_wpa[256], str_secret[256];
	char *ptr, *ptr_key, *mode, *type, *timer, *auth_time,
	     *radius_data, *radius1_data, *radius2_data,
	     *radius1, *port1, *radius2, *port2,
	     *secret1, *secret2;
	char tmp[256], tmp1[256];

	sprintf(str_wpa, "%s",
                nvram_safe_get_i("multi_ssid_security", index, g1));
	ptr = str_wpa;
	mode = strsep(&ptr, ";");
	type = strsep(&ptr, ";");
	timer = strsep(&ptr, ";");
	auth_time = strsep(&ptr, ";");
	radius_data = ptr;

	if(strstr(radius_data, "-")){
		radius1_data = strsep(&radius_data, "-");
		radius2_data = radius_data;
		radius1 = strsep(&radius1_data, ":");
		port1 = radius1_data;
		radius2 = strsep(&radius2_data, ":");
		port2 = radius2_data;
	}else{
		radius1 = strsep(&radius_data, ":");
		port1 = radius_data;
		radius2 = NULL;
		port2 = NULL;
	}
		set_public_key(fd, "HideSSID", multissid[index].STATUS);
		/* set AuthMode */
		if(strcmp(mode, "auto") == 0)
			set_public_key(fd, "AuthMode", "WPA1WAP2");
		if(strcmp(mode, "wpa") == 0)
			set_public_key(fd, "AuthMode", "WPA");
		if(strcmp(mode, "wpa2") == 0)
			set_public_key(fd, "AuthMode", "WPA2");
			
		/* set EncrypType */
		if(strcmp(type, "tkip") == 0)
			set_public_key(fd, "EncrypType", "TKIP");
		if(strcmp(type, "aes") == 0)
			set_public_key(fd, "EncrypType", "AES");
		if(strcmp(type, "both") == 0)
			set_public_key(fd, "EncrypType", "TKIPAES");
		
		
		/* set Radius */
		set_radius(fd, "RADIUS_Server=", radius1);
		set_radius(fd, "RADIUS_Port=", port1);
		if(radius2 != NULL){
			set_radius(fd, "RADIUS_Server=", radius2);
		}
		if(port2 != NULL){
			set_radius(fd, "RADIUS_Port=", port2);
		}
		set_radius(fd, "RADIUS_Key=", 
			   nvram_safe_get_i("multi_ssid_secret", index, g1));
		
		/* set own_ip_addr */
		ptr_key = stream_search(fd, "own_ip_addr=", tmp, sizeof(tmp));
		if(ptr_key == "" || ptr_key == NULL) {
			sprintf(tmp1, "%s%s;", "own_ip_addr=", nvram_safe_get("static_addr0"));
			stream_replace_line(fd, "own_ip_addr=", tmp1);
		}

		set_public_key(fd, "DefaultKeyID", "2");
}

static void set_none(int index, int fd)
{
	/* set AuthMode */
	set_public_key(fd, "AuthMode", "OPEN");
	/* set EncrypType */
	set_public_key(fd, "EncrypType", "NONE");
	/* set HideSSID */
	set_public_key(fd, "HideSSID", multissid[index].STATUS);
}

static int set_security(int index)
{
	int security, fd, ret = -1;

	if((fd = open(CONFIG, O_RDWR)) == -1) {
		goto out;
	}
	if(strcmp(multissid[index].NAME, "") != 0) {
		security = atoi(multissid[index].SECURITY);
		switch(security){
			case 0: set_none(index, fd); break;/* NONE */
			case 1: set_wep(index, fd); break;/* WEP */
			case 2: set_wpa(index, fd); break;/* WPA */
			case 3: set_eap(index, fd); break;/* EAP */
		}
	}
	ret = 0;
out:
	return ret;
}

void set_bssidnum(int cnt)
{
	char key[256];
	int fd;
	if((fd = open(CONFIG, O_RDWR)) != -1){
		sprintf(key, "BssidNum=%d", cnt);
		stream_replace_line(fd, "BssidNum", key);
	}
}


extern rt2880_config(int auth_mode);

int set_rt2880_config()
{
	char dev[256];
	int i = 0, ret = -1;
	int cnt = 1;
	int auth_mode =atoi(nvram_safe_get("wlan0_radio_mode"));
	rt2880_config(auth_mode);
	/* set ssid */
	for( i = 0; i < MAX; i++){
		set_ssid(i);
		if(strcmp(multissid[i].NAME, "") != 0) {
			cnt++;		
		}
	}
	/* set bssidnum */
	set_bssidnum(cnt);

	/* set security */
	for( i = 0; i < MAX; i++){
		if(set_security(i) == -1)
			goto out;
	}
	
	//system("rmmod rt2880_iNIC");
	//system("insmod /lib/modules/2.6.15/net/rt2880_iNIC.ko");
	system("ifconfig ra0 up");
	for( i = 0; i< MAX; i++){
		if(strcmp(multissid[i].ENABLE, "1") == 0){
			sprintf(dev, "ifconfig %s up", multissid[i].DEVICE);
			system(dev);
		}
	}
	system("brctl addif br0 ra0");
	for( i = 0; i< MAX; i++){
		if(strcmp(multissid[i].ENABLE, "1") == 0){
			sprintf(dev, "brctl addif br0 %s", multissid[i].DEVICE);
			system(dev);
		}
	}	
	ret = 0;
out:
	return ret;
}

void _init_multissid(struct MultiSSID *ssid, const char *list)
{
	char LIST[256];
	char *ptr, *_ptr;
	char *enable, *schedule, *ssidindex, *name,
	     *status, *wlan, *erbz, *security;
	
	sprintf(LIST, "%s", list);
	ptr = LIST;
	enable =  strsep(&ptr, ";");
	schedule =  strsep(&ptr, ";");
	ssidindex = strsep(&ptr, ";");
	name = strsep(&ptr, ";");
	status = strsep(&ptr, ";");
	wlan = strsep(&ptr, ";");
	erbz = strsep(&ptr, ";");
	security = ptr;

	sprintf(ssid->ENABLE, "%s", enable);
	sprintf(ssid->SCHEDULE, "%s", schedule);
	sprintf(ssid->SSIDINDEX, "%s", ssidindex);
	sprintf(ssid->NAME, "%s", name);
	sprintf(ssid->STATUS, "%s", 
		((strcmp(status, "0") == 0)?"1":"0"));
	sprintf(ssid->WLAN, "%s", wlan);
	sprintf(ssid->ERBZ, "%s", erbz);
	sprintf(ssid->SECURITY, "%s", security);
	sprintf(ssid->DEVICE, "ra%d", atoi(ssidindex)+1);
}

int init_multissid()
{
	int i = 0;
	int ret = -1;
	char *tmp;
	char str[256];

	for(i = 0; i < MAX; i++) {
		tmp = nvram_safe_get_i("multi_ssid_list", i, g1);
		sprintf(str, "%s", tmp);
		if((strcmp(str, "") == 0))
			continue;
		else{
			ret = 0;
			_init_multissid(&multissid[i], tmp);	
		}
		tmp = NULL;
	}
	
	return ret;
}

int start_multissid(void)
{
	int ret = -1;
	
	if(init_multissid() != 0)
		goto out;

	set_rt2880_config();
	ret = 0;
out:
	return ret;
}

int start_multissid_main(int argc, char *argv[]){
	return start_multissid();
}

int stop_multissid(void)
{
	return 0;
}

int stop_multissid_main(int argc, char *argv[]){
	return stop_multissid();
}

int multissid_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"start", start_multissid_main},
		{"stop", stop_multissid_main},
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
