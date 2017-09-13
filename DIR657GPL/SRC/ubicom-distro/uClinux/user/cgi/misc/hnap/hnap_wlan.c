#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

#include "sutil.h"
#include "libdb.h"
#include "debug.h"
#include "hnap.h"

#define WIRELESS_BAND_24G   0
#define WLA_ESSID_LEN   256

extern int do_xml_response(const char *);
extern int do_xml_mapping(const char *, hnap_entry_s *);
extern void do_get_element_value(const char *, char *, char *);

static int do_wlan_settings24(const char *key, char *ct)
{

	char *enable = "1";
	if(strcmp(key,"wlan0_enable") == 0 || strcmp(key,"wlan0_ssid_broadcast") == 0) {
	
		if(strcasecmp(ct,"false") == 0)
			enable = "0";
		
		nvram_set(key,enable);	
	
	} else {
		
        	nvram_set(key, ct);
	
	}
	
        return 0;
}

static char *parse_hex_to_ascii(char *hex_string){
	
	char hex[400];
	char hex_tmp[200];
	int len;
	int c1 = 0;
	int c2 = 0;
	
	memset(hex, 0, sizeof(hex));
	memset(hex_tmp, 0, sizeof(hex_tmp));
	len = strlen(hex_string);
	strcpy(hex_tmp, hex_string);
	
	for(c1 = 0; c1 < len; c1++) {
		/*   ` = 0x60, " = 0x22, \ = 0x5c, ' = 0x27       */
	
		if((hex_tmp[c1] == 0x22) || (hex_tmp[c1] == 0x5c) || (hex_tmp[c1] == 0x60)) {
			hex[c2] = 0x5c;
			c2++;
			hex[c2] = hex_tmp[c1];
		} else
			hex[c2] = hex_tmp[c1];
		c2++;
	}

	strcpy(hex_string, hex);
	return hex_string;
}


static int do_wlan_security(const char *raw)
{
	char enable[6] = {};
	char type[4] = {};
	char wep_key_len[4] = {};
	char key_tmp[200+1] = {};
	char radioip1[16] = {};
	char radioip2[16] = {};
	char radioport1[6] = {};
	char radioport2[6] = {};
	char wlan0_eap_radius_server_0[64] = {};
	char wlan0_eap_radius_server_1[64] = {};

	do_get_element_value(raw, enable, "Enabled");
	do_get_element_value(raw, type, "Type");
	do_get_element_value(raw, key_tmp, "Key");
	do_get_element_value(raw, wep_key_len, "WEPKeyBits");
	do_get_element_value(raw, radioip1, "RadiusIP1");
	do_get_element_value(raw, radioip2, "RadiusIP2");
	do_get_element_value(raw, radioport1, "RadiusPort1");
	do_get_element_value(raw, radioport2, "RadiusPort2");

	if(strcasecmp(enable,"false") == 0){
		nvram_set("wlan0_security","disable");
		return 1;
	} else if(strcasecmp(enable,"true") == 0) 
		nvram_set("wlan0_wep_default_key","1");         // set to index 1 for pure network (total is 4)
	else
		return 0;
	
	sprintf(wlan0_eap_radius_server_0, "%s/%s/0", radioip1, radioport1);
	sprintf(wlan0_eap_radius_server_1, "%s/%s/0", radioip2, radioport2);
	nvram_set("wlan0_eap_radius_server_0", wlan0_eap_radius_server_0);
	nvram_set("wlan0_eap_radius_server_1", wlan0_eap_radius_server_1);
	
	key_tmp[200+1] = '\0';
	// wpa type need more data to config , use default here 
	if(strcasecmp(type,"wep") == 0 && strcasecmp(wep_key_len,"64") == 0 && strlen(key_tmp) == 10) {
		nvram_set("wlan0_security","wep_open_64");
		nvram_set("wlan0_wep64_key_1",key_tmp);
		nvram_set("asp_temp_37",key_tmp); //for add wireless device with wps GUI

	} else if(strcasecmp(type,"wep") == 0 && strcasecmp(wep_key_len,"128") == 0 && strlen(key_tmp) == 26) {
		nvram_set("wlan0_security","wep_open_128");
		nvram_set("wlan0_wep128_key_1",key_tmp);
		nvram_set("asp_temp_37",key_tmp); //for add wireless device with wps GUI

	} else if(strcasecmp(type,"wep") == 0 && strcasecmp(wep_key_len,"152") == 0 && strlen(key_tmp) == 32) {
		nvram_set("wlan0_security","wep_open_152");
		nvram_set("wlan0_wep152_key_1",key_tmp);
		nvram_set("asp_temp_37",key_tmp); //for add wireless device with wps GUI

	} else if(strcasecmp(type,"wpa") == 0 && 
		( (strcasecmp(wep_key_len,"0") == 0) || (strcasecmp(wep_key_len,"64") == 0) || 
			(strcasecmp(wep_key_len,"128") == 0) || (strcasecmp(wep_key_len,"152") == 0) ) ) {
		nvram_set("wlan0_security","wpa");// data miss, use default
		nvram_set("wlan0_psk_cipher_type","aes");// data miss, use default
		nvram_set("wlan0_psk_pass_phrase",key_tmp);

	} else
		return 0;
	
	if(strlen(nvram_safe_get("blank_status")) != 0)
		nvram_set("blank_status","1");
	
	nvram_set("wps_configured_mode","5");

	return 1;


}

static void do_get_wlan_security_to_xml(char *dest_xml)
{
	char *wlan0_security_tmp;
	char *enable = "disable";
	char key_type[5] = {"WEP"};
	char sub_type[5] = {"open"};
	char wep_key_len[4] = {"64"};
	char key[256] = "0000000000000000000000000000000000000000000000000000000000000000";
	char *wlan0_wpa_eap_radius_server_0;
	char *wlan0_wpa_eap_radius_server_1;
#ifdef LINUX_NVRAM
	char *eap_radius_srv_str[2][128] = {{},{}};
#endif
	char *radioip1 = "0.0.0.0";
	char *radioip2 = "0.0.0.0";
	char *radioport1 = "0";
	char *radioport2 = "0";
	char key_xml[200] = {};
	char wlan0_security[200]= {};
	int i = 0;

	wlan0_security_tmp = nvram_safe_get("wlan0_security");
	strcpy(wlan0_security,wlan0_security_tmp);

	if(strcmp(wlan0_security,"disable") == 0){
		enable = "false";
	} else {
		enable = "true";
		/* substitute "_" to " " , for sscanf function */
		for(i=0; i<strlen(wlan0_security); i++)
			if(*(wlan0_security+i) == '_')
				*(wlan0_security+i) = ' ';
		/* format scan in & write to xml */
		sscanf(wlan0_security,"%s %s %s",key_type,sub_type,wep_key_len);

		
		
		/* get key value */
		if((strcasecmp(key_type,"WPA") == 0) ){
			strcpy(key, nvram_get("wlan0_psk_pass_phrase"));
			memcpy(wep_key_len,"128",3);          // error , need more data info
		}
		else if(strcmp(wep_key_len,"64") == 0)
			strcpy(key, nvram_safe_get("wlan0_wep64_key_1"));
		else if(strcmp(wep_key_len,"128") == 0)
			strcpy(key, nvram_safe_get("wlan0_wep128_key_1"));
		else if(strcmp(wep_key_len,"152") == 0)
			strcpy(key, nvram_safe_get("wlan0_wep152_key_1"));
		
		/* parse hex key to ascii */
		memcpy(key_xml,parse_hex_to_ascii(key),sizeof(key_xml));


		/* get eap config */
		wlan0_wpa_eap_radius_server_0 = nvram_safe_get("wlan0_eap_radius_server_0");
		wlan0_wpa_eap_radius_server_1 = nvram_safe_get("wlan0_eap_radius_server_1");

		if(strlen(wlan0_wpa_eap_radius_server_0) > 0) {

#ifdef LINUX_NVRAM
			sprintf(eap_radius_srv_str[0], "%s", wlan0_wpa_eap_radius_server_0);
			radioip1 = strtok(eap_radius_srv_str[0], "/");
	
#else
			radioip1 = strtok(wlan0_wpa_eap_radius_server_0, "/");
#endif
			radioport1 = strtok(NULL,"/");
			if(radioip1 == NULL || radioport1 == NULL || !strcmp(radioip1,"not_set") || !strcmp(radioport1,"not_set")) {

				radioip1 = "0.0.0.0";
				radioport1 = "0";
           		}
	
                } else {

			radioip1 = "0.0.0.0";
			radioport1 = "0";
		}

		if(strlen(wlan0_wpa_eap_radius_server_1) > 0) {
#ifdef LINUX_NVRAM
			sprintf(eap_radius_srv_str[1], "%s", wlan0_wpa_eap_radius_server_1);
			radioip2 = strtok(eap_radius_srv_str[1], "/");

#else
			radioip2 = strtok(wlan0_wpa_eap_radius_server_1, "/");
#endif

			radioport2 = strtok(NULL,"/");
			if(radioip2 == NULL || radioport2 == NULL  || !strcmp(radioip2,"not_set") || !strcmp(radioport2,"not_set")) {

				radioip2 = "0.0.0.0";
				radioport2 = "0";
			}
	
		} else {
	
			radioip2 = "0.0.0.0";
			radioport2 = "0";
		}
	}
	
   	if(strcasecmp(enable,"true")==0) {
                sprintf(dest_xml, get_wlan_security_result, 
			"OK", enable, key_type, wep_key_len, key_xml, radioip1, radioport1, radioip2, radioport2);
        } else{   // for false case , for test-tool , default type=wep
		
                sprintf(dest_xml, get_wlan_security_result, 
			"OK", enable, "WEP", "0", "", radioip1, radioport1, radioip2, radioport2);
        }
}


static void do_wlan_radios_to_xml(char *dest_xml)
{
	char RadioInfo24[7168] = {};
	char g_channels[300] = {"<int>0</int>"};
	char *g_channels_t;
	char g_channel_list[128];
	char *wlan0_domain = NULL;
	int domain;
	int wlan_radio_mode = 0;
	char RadioInfo_tmp[8192] = {};
	
	wlan0_domain = nvram_get("wlan0_domain");

	if (wlan0_domain != NULL)
		sscanf(wlan0_domain, "%x", &domain);
	else
		domain = 0x10;

	wlan_radio_mode = atoi(nvram_safe_get("wlan0_radio_mode"));

	if (wlan_radio_mode == 0) { /* 2.4G */	
		GetDomainChannelList(domain, WIRELESS_BAND_24G, g_channel_list, 0);
		g_channels_t=strtok(g_channel_list, ",");
		sprintf(g_channels, "%s<int>%s</int>", g_channels, g_channels_t);

		while(g_channels_t=strtok(NULL, ",")) {
			
			sprintf(g_channels, "%s<int>%s</int>", g_channels, g_channels_t);
		}

		sprintf(RadioInfo_tmp,RadioInfo_24GHZ, "", "", "", "", g_channels, "", WideChannels_24GHZ, "", SecurityInfo);
 		sprintf(RadioInfo24,"<RadioInfo>\n%s\n</RadioInfo>", RadioInfo_tmp);
		sprintf(dest_xml,get_wlan_radios_result,"OK",RadioInfo24);
	}
}


static int do_wlan_radio_settings_to_xml(char *dest_xml, char *general_xml)
{
	char *wlan_enable = "false";
	char *wlan_dot11_mode = NULL;
	char *mode = "";
	char *mac = "";
	char *ssid_broadcast = "false";
	char ssid[256];
	char *wlan_11n_protection = NULL;
	char *protection = "0";
	char *channel = "0";
	char *qos = "false";
	char *result = "OK";
        
	char *secondary_channel  = "0";

	if(nvram_match("wlan0_enable", "1")==0)
		wlan_enable = "true";

	wlan_dot11_mode=nvram_safe_get("wlan0_dot11_mode");

	if(strcmp(wlan_dot11_mode, "11bgn")==0)
		mode="802.11bgn";
	else if(strcmp(wlan_dot11_mode, "11g")==0)
		mode="802.11g";
	else if(strcmp(wlan_dot11_mode, "11n")==0)
		mode="802.11n";
	else if(strcmp(wlan_dot11_mode, "11bg")==0)
		mode="802.11bg";
	else if(strcmp(wlan_dot11_mode, "11gn")==0)
		mode="802.11gn";
	else if(strcmp(wlan_dot11_mode, "11b")==0)
		mode="802.11b";

	mac = nvram_safe_get("lan_mac");

	bzero(ssid, sizeof(ssid));
	strcpy(ssid, nvram_safe_get("wlan0_ssid"));
	/* parse hex ssid to ascii */
	parse_hex_to_ascii(ssid);

	if(nvram_match("wlan0_ssid_broadcast", "1")==0)
		ssid_broadcast = "true";
	else
		ssid_broadcast = "false";

	wlan_11n_protection = nvram_safe_get("wlan0_11n_protection");

	if(strcmp(wlan_11n_protection,"20")==0)
		protection = "20";
	else if (strcmp(wlan_11n_protection,"40")==0)
		protection = "40";
	else if(strcmp(wlan_11n_protection,"auto")==0)
		protection = "0";

	if(nvram_match("wlan0_auto_channel_enable","1")==0)
		channel = "0";
	else
		channel = nvram_safe_get("wlan0_channel");
      
	secondary_channel = nvram_safe_get("wlan0_secondary_channel");

	if(nvram_match("wlan0_wmm_enable", "1")==0)
		qos = "true";
	else
		qos = "false";                                                                                                                        

        sprintf(dest_xml, general_xml, result, wlan_enable, mode, mac, ssid, ssid_broadcast, protection, channel, secondary_channel,qos);
	
        return 1;

}


static int do_set_wlan_radio_settings_to_xml(const char *key,char *ct)
{
	char enable[2]="0";
	char *mode="11n";
	char ssid_bcast[2] = "0";
	char channel_width[5]="20";	
	char channel[2]="1";
	char qos[2]="0";


	if (strcmp(key,"wlan0_enable") == 0 ) {
		
		if ( ct != NULL && strcasecmp(ct,"true") == 0)
			strcpy(enable,"1");
	
		nvram_set(key,enable);
			
	} else if ( strcasecmp(nvram_safe_get("wlan0_enable"),"1")==0 ) {
		 if(strcmp(key, "wlan0_radio_mode") == 0 && strcasecmp(ct, "RADIO_24GHz") == 0) {
				nvram_set("wlan0_radio_mode","0");
			nvram_set("wps_configured_mode","5");

		} else if( strcasecmp(key, "wlan0_dot11_mode") == 0) {
			if (ct != NULL ) {
	
				if(strcasecmp(ct,"802.11a") == 0)
					mode = "11a";
				else if(strcasecmp(ct,"802.11b") == 0)
					 mode = "11b";
				else if(strcasecmp(ct,"802.11g") == 0)
					mode = "11g";
				else if(strcasecmp(ct,"802.11n") == 0)
					mode = "11n";
				else if(strcasecmp(ct,"802.11bg") == 0)
					mode = "11bg";
				else if(strcasecmp(ct,"802.11bgn") == 0)
					mode = "11bgn";
				else if(strcasecmp(ct,"802.11gn") == 0)
					mode = "11gn";
				else if(strcasecmp(ct,"802.11an") == 0)
					mode ="11an";
			}	
	
			nvram_set("wlan0_dot11_mode",mode);
			//nvram_set("wlan1_dot11_mode",mode);

		} else if (strcasecmp(key,"wlan0_ssid") == 0) {
	
				nvram_set("wlan0_ssid",ct);

		} else if (strcasecmp(key,"wlan0_ssid_broadcast") == 0) {
		
			if(strcasecmp(ct,"true") == 0)
					
				strcpy(ssid_bcast,"1");
			
			nvram_set("wlan0_ssid_broadcast",ssid_bcast);		
		
		} else if (strcasecmp(key,"wlan0_11n_protection") == 0) {

			 if(strcasecmp(ct,"20") != 0 )
				strcpy(channel_width,"auto");	
				
			nvram_set("wlan0_11n_protection",ct);

		} else if (strcasecmp(key,"wlan0_channel") == 0) {	
			
			if(strcasecmp(ct,"0") != 0) {
				strcpy(channel,"0");
			} 
				
			nvram_set("wlan0_auto_channel_enable", channel);
			nvram_set("wlan0_channel", ct);
	
		} else if (strcasecmp(key,"wlan0_wmm_enable") == 0) {
			 if(strcasecmp(ct,"true") == 0) 
				strcpy(qos,"1");
			nvram_set("wlan0_wmm_enable",qos);
		
		} else if (strcasecmp(key,"wlan0_secondary_channel") == 0){
        			nvram_set("wlan0_secondary_channel", ct);
		}
		
	}	
}


static int do_get_wlan_radio_security_to_xml(char *dest_xml, char *general_xml)
{
	char radioid[10]={};
	char *enable = "true";
	char *wlan_security = NULL;
	char *type = "";
	char *encryption = "";
	char *key = "";
	char *wep64_key=NULL, *wep128_key=NULL, *wep_key_index=NULL;
	char *cipher_type = "";
	char *psk_pass_phrase="";
	char *keyrenewal="";
	char *wlan_wpa_eap_radius_server_0=NULL;
	char *wlan_wpa_eap_radius_server_1=NULL;
	char radius_server0[256], radius_server1[256];
#ifdef LINUX_NVRAM
	char eap_radius_srv_str[2][128] ={{},{}};
#endif
	char *radiusip1="", *radiusport1="0", *radiussecret1="";
	char *radiusip2="", *radiusport2="0", *radiussecret2="";
	char *result;


	wlan_security = nvram_safe_get("wlan0_security");
	if(strstr(wlan_security, "wep")) {
    		wep_key_index = nvram_safe_get("wlan0_wep_default_key");
    		switch(atoi(wep_key_index)) {
	
	    		case 1:
	    			wep64_key=nvram_safe_get("wlan0_wep64_key_1");
	    			wep128_key=nvram_safe_get("wlan0_wep128_key_1");
	    			break;

	    		case 2:
	    			wep64_key=nvram_safe_get("wlan0_wep64_key_2");
	    			wep128_key=nvram_safe_get("wlan0_wep128_key_2");
	    			break;

	    		case 3:
	    			wep64_key=nvram_safe_get("wlan0_wep64_key_3");
	    			wep128_key=nvram_safe_get("wlan0_wep128_key_3");
	    			break;

	    		case 4:
	    			wep64_key=nvram_safe_get("wlan0_wep64_key_4");
	    			wep128_key=nvram_safe_get("wlan0_wep128_key_4");
	    			break;
		}
	
	} else {
	    	
		cipher_type = nvram_safe_get("wlan0_psk_cipher_type");
	    	psk_pass_phrase=nvram_safe_get("wlan0_psk_pass_phrase");
	}

	keyrenewal=nvram_safe_get("wlan0_gkey_rekey_time");
	sprintf(radius_server0, "%s", nvram_safe_get("wlan0_eap_radius_server_0"));
	sprintf(radius_server1, "%s", nvram_safe_get("wlan0_eap_radius_server_1"));
	wlan_wpa_eap_radius_server_0 = radius_server0;
	wlan_wpa_eap_radius_server_1 = radius_server1;

	if(strlen(wlan_wpa_eap_radius_server_0) > 0)  {
#ifdef LINUX_NVRAM
		sprintf(eap_radius_srv_str[0], "%s", wlan_wpa_eap_radius_server_0);
		radiusip1 = strtok(eap_radius_srv_str[0], "/");
		
#else
		radiusip1 = strtok(wlan_wpa_eap_radius_server_0, "/");
#endif
		radiusport1 = strtok(NULL,"/");
		radiussecret1=strtok(NULL,"/");
		if(radiusip1 == NULL || radiusport1 == NULL || radiussecret1 == NULL) {
				
			radiusip1 = ""; 
			radiusport1 = "";
			radiussecret1 = "";
		}
	}
	if(strlen(wlan_wpa_eap_radius_server_1) > 0)  {
#ifdef LINUX_NVRAM
		sprintf(eap_radius_srv_str[1], "%s", wlan_wpa_eap_radius_server_1);
		radiusip2 = strtok(eap_radius_srv_str[1], "/");
#else
		radiusip2 = strtok(wlan_wpa_eap_radius_server_1, "/");
#endif
		radiusport2 = strtok(NULL,"/");
		radiussecret2=strtok(NULL,"/");

		if(radiusip2 == NULL || radiusport2 == NULL  || radiussecret2 == NULL ) {

			radiusip2 = ""; 
		      	radiusport2 = "";
		       	radiussecret2 = "";	
		}
	}
	result="OK"; 	      

	if((strcmp(wlan_security,"disable") == 0)) {	
		enable = "false";
	
	} else if((strcmp(wlan_security,"wep_open_64") == 0)) {
		type = "WEP-OPEN";
		encryption = "WEP-64";
		key=wep64_key;

	} else if((strcmp(wlan_security,"wep_share_64") == 0)) {
		type = "WEP-SHARED"; 
		encryption = "WEP-64";
		key=wep64_key;

	} else if((strcmp(wlan_security,"wep_open_128") == 0)) {
		type = "WEP-OPEN"; 
		encryption = "WEP-128";
		key=wep128_key;
	} else if((strcmp(wlan_security,"wep_share_128") == 0)) {
		type = "WEP-SHARED"; 
		encryption = "WEP-128";
		key=wep128_key;

	} else if((strcmp(wlan_security,"wpa_psk") == 0) || (strcmp(wlan_security,"wpa2auto_psk") == 0)) {
	        type = "WPA-PSK"; 
		if(strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if(strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";	
		
		key=psk_pass_phrase;	

	} else if((strcmp(wlan_security,"wpa2_psk") == 0)) {
		type = "WPA2-PSK";
		if(strcmp(cipher_type, "tkip") == 0)
		encryption = "TKIP";
		else if(strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";
	
		key=psk_pass_phrase;

	} else if((strcmp(wlan_security,"wpa_eap") == 0) ||  (strcmp(wlan_security,"wpa2auto_eap") == 0)) {
		type = "WPA-RADIUS"; 
		if(strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if(strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";
	
	} else if((strcmp(wlan_security,"wpa2_eap") == 0)) {
		type = "WPA2-RADIUS"; 
		if(strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if(strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";

	} else if((strcasecmp(wlan_security,"wpaorwpa2_psk") == 0)) {
/*
*   Date: 2010-05-25
*   Name: Marine Chiu
*   Reason: to pass HNAP TestDevice 10.2.8130
*   Notice : new item
*/
		type = "WPAORWPA2-PSK";
		if(strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
			else if(strcmp(cipher_type, "aes") == 0)
				encryption = "AES";
			else
				encryption = "TKIPORAES";
	}
    
	sprintf(dest_xml, general_xml, result, enable, type, encryption, 
			key, keyrenewal, radiusip1, radiusport1, radiussecret1, radiusip2, radiusport2, radiussecret2);

	return 1;
}




static int do_set_wlan_radio_security_to_xml(char *source_xml)
{
	char radioid[15] = {""};
	char enable[6] = {""}; 
	char type[10] = {""}; 
	char encryption[10] = {""};
	char key_tmp[200+1] = {""};
	char rekey_time[32] ={""};
	char radiusip1[16] = {""};
	char radiusip2[16] = {""};
	char radiusport1[6] = {""};
	char radiusport2[6] = {""};
	char radiussecret1[64] = {""};
	char radiussecret2[64] = {""};
	char wlan_eap_radius_server_0[64] = {""};
	char wlan_eap_radius_server_1[64] = {""};
	
	do_get_element_value(source_xml, radioid, "RadioID");
	do_get_element_value(source_xml, enable, "Enabled");	
	do_get_element_value(source_xml, type, "Type");
	do_get_element_value(source_xml, encryption, "Encryption");    
	do_get_element_value(source_xml, key_tmp, "Key");
	do_get_element_value(source_xml, rekey_time, "KeyRenewal");        
	do_get_element_value(source_xml, radiusip1, "RadiusIP1");
	do_get_element_value(source_xml, radiusport1, "RadiusPort1");
	do_get_element_value(source_xml, radiussecret1, "RadiusSecret1");
	do_get_element_value(source_xml, radiusip2, "RadiusIP2");
	do_get_element_value(source_xml, radiusport2, "RadiusPort2");
	do_get_element_value(source_xml, radiussecret2, "RadiusSecret2");	



	/* parse ascii to hex*/		
	key_tmp[200+1] = '\0';	
	
	if(strcasecmp(enable,"false") == 0){
		nvram_set("wlan0_security","disable");	
		return 1;

	} else if(strcasecmp(enable,"true") == 0)
		nvram_set("wlan0_wep_default_key","1");	// set to index 1 for pure network (total is 4)

	else 
		return 0;

	if(strcasecmp(type,"WEP-OPEN") == 0) {
		if(strcasecmp(encryption,"WEP-64") == 0)
			nvram_set("wlan0_security","wep_open_64");
	
		else if(strcasecmp(encryption,"WEP-128") == 0)
			nvram_set("wlan0_security","wep_open_128");
		
		if(strlen(key_tmp) == 10) {
			nvram_set("wlan0_gkey_rekey_time", rekey_time);
			nvram_set("wlan0_wep64_key_1",key_tmp);
			nvram_set("asp_temp_37",key_tmp); //for add wireless device with wps GUI

		} else if(strlen(key_tmp) == 26) {
			nvram_set("wlan0_gkey_rekey_time", rekey_time);
			nvram_set("asp_temp_37",key_tmp); ////for add wireless device with wps GUI
			nvram_set("wlan0_wep128_key_1",key_tmp);

		} else
			return 0;
				
	} else if(strcasecmp(type,"WEP-SHARED") == 0) {
		if(strcasecmp(encryption,"WEP-64") == 0)
			nvram_set("wlan0_security","wep_share_64");

		else if(strcasecmp(encryption,"WEP-128") == 0)
			nvram_set("wlan0_security","wep_share_128");
	
		if(strlen(key_tmp) == 10) {
			nvram_set("asp_temp_37",key_tmp); //for add wireless device with wps GUI
			nvram_set("wlan0_wep64_key_1",key_tmp);

		} else if(strlen(key_tmp) == 26) {
			nvram_set("asp_temp_37",key_tmp); //for add wireless device with wps GUI
			nvram_set("wlan0_wep128_key_1",key_tmp);

		} else
			return 0;				

	} else if(strcasecmp(type,"WPA-PSK") == 0) {
		nvram_set("wlan0_security","wpa_psk");
		if(strcasecmp(encryption,"AES") == 0)
			nvram_set("wlan0_psk_cipher_type","aes");

		else if(strcasecmp(encryption,"TKIP") == 0)
			nvram_set("wlan0_psk_cipher_type","tkip");
	
		else if	(strcasecmp(encryption,"TKIPORAES") == 0)
			nvram_set("wlan0_psk_cipher_type","both");
		
		nvram_set("wlan0_gkey_rekey_time", rekey_time);	
		nvram_set("wlan0_psk_pass_phrase",key_tmp); 

	} else if(strcasecmp(type,"WPA-RADIUS") == 0) {
		nvram_set("wlan0_security","wpa_eap");

		if(strcasecmp(encryption,"AES") == 0)
			nvram_set("wlan0_psk_cipher_type","aes");

		else if(strcasecmp(encryption,"TKIP") == 0)
			nvram_set("wlan0_psk_cipher_type","tkip");	 

		else if	(strcasecmp(encryption,"TKIPORAES") == 0)
			nvram_set("wlan0_psk_cipher_type","both");
		
		nvram_set("wlan0_gkey_rekey_time",rekey_time);

	} else if(strcasecmp(type,"WPA2-PSK") == 0) {
		nvram_set("wlan0_security","wpa2_psk");
		if(strcasecmp(encryption,"AES") == 0)
			nvram_set("wlan0_psk_cipher_type","aes");

		else if(strcasecmp(encryption,"TKIP") == 0)
			nvram_set("wlan0_psk_cipher_type","tkip");	 

		else if	(strcasecmp(encryption,"TKIPORAES") == 0)
			nvram_set("wlan0_psk_cipher_type","both");
			
		nvram_set("wlan0_gkey_rekey_time", rekey_time);	
		nvram_set("wlan0_psk_pass_phrase",key_tmp); 
		nvram_set("wlan0_eap_radius_server_0", wlan_eap_radius_server_0);
                nvram_set("wlan0_eap_radius_server_1", wlan_eap_radius_server_1);


	} else if(strcasecmp(type,"WPA2-RADIUS") == 0) {
		nvram_set("wlan0_security","wpa2_eap");
		if(strcasecmp(encryption,"AES") == 0)
			nvram_set("wlan0_psk_cipher_type","aes");

		else if(strcasecmp(encryption,"TKIP") == 0)
			nvram_set("wlan0_psk_cipher_type","tkip");

		else if	(strcasecmp(encryption,"TKIPORAES") == 0)
			nvram_set("wlan0_psk_cipher_type","both");
			
		nvram_set("wlan0_gkey_rekey_time", rekey_time);	 
				 
		//sprintf(wlan_eap_radius_server_0, "%s/%s/%s", radiusip1, radiusport1, radiussecret1);
		//sprintf(wlan_eap_radius_server_1, "%s/%s/%s", radiusip2, radiusport2, radiussecret2);
		//nvram_set("wlan0_eap_radius_server_0", wlan_eap_radius_server_0);
		//nvram_set("wlan0_eap_radius_server_1", wlan_eap_radius_server_1);		 

	} else if(strcasecmp(type,"WPAORWPA2-PSK") == 0) {
		nvram_set("wlan0_security","wpaorwpa2_psk");
		
		if(strcasecmp(encryption,"AES") == 0)
			nvram_set("wlan0_psk_cipher_type","aes");
	
		else if(strcasecmp(encryption,"TKIP") == 0)
			nvram_set("wlan0_psk_cipher_type","tkip");
		
		else if	(strcasecmp(encryption,"TKIPORAES") == 0)
			 nvram_set("wlan0_psk_cipher_type","both");

		nvram_set("wlan0_gkey_rekey_time", rekey_time);	
		nvram_set("wlan0_psk_pass_phrase",key_tmp);
	}

	sprintf(wlan_eap_radius_server_0, "%s/%s/%s", radiusip1, radiusport1, radiussecret1);
	sprintf(wlan_eap_radius_server_1, "%s/%s/%s", radiusip2, radiusport2, radiussecret2);
	nvram_set("wlan0_eap_radius_server_0", wlan_eap_radius_server_0);
	nvram_set("wlan0_eap_radius_server_1", wlan_eap_radius_server_1);     
						
	if(strlen(nvram_safe_get("blank_status")) != 0)	
		nvram_set("blank_status","1");
	
	nvram_set("wps_configured_mode","5");
	return 1;	
}	


int do_get_wlan_setting24(const char *key,char *raw)
{

	char xml_resraw[2048];
	char xml_resraw2[1024];

	bzero(xml_resraw,sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);
	
	sprintf(xml_resraw2, get_wlan_setting24_result,
		atoi(nvram_safe_get("wlan0_enable")) == 0? "false" : "true",
		nvram_safe_get("wlan0_mac"),
		nvram_safe_get("wlan0_ssid"),
		atoi(nvram_safe_get("wlan0_ssid_broadcast")) == 0? "false" : "true",
		nvram_safe_get("wlan0_channel")
	);
	
	strcat(xml_resraw, xml_resraw2);
	
	return do_xml_response(xml_resraw);	

}

int do_set_wlan_setting24(const char *key,char *raw)
{
	int ret;
	char xml_resraw[1024];

	hnap_entry_s ls[] = {
                { "Enabled", "wlan0_enable", do_wlan_settings24 },
                { "SSID", "wlan0_ssid", do_wlan_settings24 },
                { "SSIDBroadcast", "wlan0_ssid_broadcast", do_wlan_settings24 },
                { "Channel", "wlan0_channel", do_wlan_settings24 },
                { NULL, NULL }
        };

	ret=do_xml_mapping(raw, ls);
        bzero(xml_resraw, sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);
        
	strcat(xml_resraw, set_wlan_setting24_result);

	return do_xml_response(xml_resraw);

}


int do_get_wlan_security(const char *key,char *raw)
{
	char xml_resraw[2048];
	char xml_resraw2[1024];

	bzero(xml_resraw,sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);

	do_get_wlan_security_to_xml(xml_resraw2);
        strcat(xml_resraw, xml_resraw2);

        return do_xml_response(xml_resraw);
}


int do_set_wlan_security(const char *key,char *raw)
{
	int ret;
	char xml_resraw[1024];

	bzero(xml_resraw,sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);
	
	ret=do_wlan_security(raw);

	
	strcat(xml_resraw, set_wlan_security_result);

	return do_xml_response(xml_resraw);
}

int do_get_wlan_radios(const char *key,char *raw)
{
	
	char xml_resraw[8192];
	char xml_resraw2[7168];
	
	bzero(xml_resraw,sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	do_wlan_radios_to_xml(xml_resraw2);
	strcat(xml_resraw, xml_resraw2);
	
        return do_xml_response(xml_resraw);

}

int do_get_wlan_radio_settings(const char *key,char *raw)
{
	int ret;
        char xml_resraw[2048];
        char xml_resraw2[1024];


        bzero(xml_resraw,sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);
	
	ret=do_wlan_radio_settings_to_xml(xml_resraw2, get_wlan_radio_settings_result);

        strcat(xml_resraw, xml_resraw2);

        return do_xml_response(xml_resraw);

}

int do_set_wlan_radio_settings(const char *key,char *raw)
{
	char xml_resraw[1024];
	
	int ret;
	hnap_entry_s ls[] = {
		{ "Enabled", "wlan0_enable", do_set_wlan_radio_settings_to_xml },
		{ "RadioID", "wlan0_radio_mode", do_set_wlan_radio_settings_to_xml },
		{ "Mode", "wlan0_dot11_mode", do_set_wlan_radio_settings_to_xml },
		{ "SSID", "wlan0_ssid", do_set_wlan_radio_settings_to_xml },
		{ "SSIDBroadcast", "wlan0_ssid_broadcast", do_set_wlan_radio_settings_to_xml },
		{ "ChannelWidth", "wlan0_11n_protection", do_set_wlan_radio_settings_to_xml },
		{ "Channel", "wlan0_channel", do_set_wlan_radio_settings_to_xml },
		{ "SecondaryChannel", "wlan0_secondary_channel", do_set_wlan_radio_settings_to_xml },
		{ "QoS", "wlan0_wmm_enable", do_set_wlan_radio_settings_to_xml },
		{ NULL, NULL }
	};

	ret = do_xml_mapping(raw, ls);
	bzero(xml_resraw, sizeof(xml_resraw));

	do_xml_create_mime(xml_resraw);
	strcat(xml_resraw, set_wlan_radio_settings_result);
	
	return do_xml_response(xml_resraw);
}

int do_get_wlan_radio_security(const char *key,char *raw)
{
	int ret;
        char xml_resraw[2048];
        char xml_resraw2[1024];


        bzero(xml_resraw,sizeof(xml_resraw));
        do_xml_create_mime(xml_resraw);

        ret=do_get_wlan_radio_security_to_xml(xml_resraw2, get_wlan_radio_security_result);

        strcat(xml_resraw, xml_resraw2);

        return do_xml_response(xml_resraw);

}

int do_set_wlan_radio_security(const char *key,char *raw)
{
	int ret;
	char xml_resraw[2048];
	char xml_resraw2[1024];

	bzero(xml_resraw,sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	ret=do_set_wlan_radio_security_to_xml(raw);

	strcat(xml_resraw,set_wLan_radio_security_result);


	return do_xml_response(xml_resraw);
}

