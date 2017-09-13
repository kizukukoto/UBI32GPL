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
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <signal.h>
#include <fcntl.h>

#include <nvram.h>
#include "cmds.h"
#define HOSTAPD_TMP_CONFIG_0	"/tmp/hostapd.conf"
#define	HOSTAPD_DUMP_FILE_0	"/tmp/hostapd.dump"
#define GPIO_DRIVER  		"/dev/gpio_ioctl"
#define init_file(f)	_system("echo \"\" > %s", f)
static int ath_count=0;
//#define RC_DEBUG
#define nvram_match	nvram_invmatch
#define WALN_LED  3
#define LEDENABLE 0
#define LEDDISABLE 1
/*
*/
#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

extern int init_wps_main(int argc, char *argv[]);
extern int trigger_wsc_main(int argc, char *argv[]);
extern int wsc_button_main(int argc, char *argv[]);
extern int wsc_getstatus_main(int argc, char *argv[]);

/*for wpa(psk), wpa2(psk)*/
void start_hostapd(int bss_index, int ath_count)
{
	FILE *fp=NULL;
	char *wlan_security;
	char *ssid;
	char *passphrase;
	char vap_passphrase[30];
	char *wlan0_wpa_eap_radius_server_0;
	char *wlan0_wpa_eap_radius_server_1;
	char *radius1_ip;
	char *radius1_port;
	char *radius1_secret;
	char *radius2_ip;
	char *radius2_port;
	char *radius2_secret;
	char *lan_ipaddr;
	char *cipher_type;
	char vap_rekey_time[30];
	unsigned char *rekey_time;
	char *wlan0_eap_mac_auth;
	char *wlan0_eap_reauth_period;
	char temp_buffer[30];

	sprintf(temp_buffer,"wlan%d_security", ath_count);
	wlan_security = nvram_safe_get(temp_buffer);

	sprintf(temp_buffer,"wlan%d_ssid", ath_count);
	ssid = nvram_safe_get(temp_buffer);

	sprintf(temp_buffer,"wlan%d_psk_pass_phrase", ath_count);
	passphrase = nvram_safe_get(temp_buffer);

	sprintf(temp_buffer,"wlan%d_psk_cipher_type", ath_count);
	cipher_type = nvram_safe_get(temp_buffer);

	sprintf(temp_buffer,"wlan%d_gkey_rekey_time", ath_count);
	rekey_time = nvram_safe_get(temp_buffer);
        
        
	/*generate hostapd config file*/
	fp = fopen(HOSTAPD_TMP_CONFIG_0, "w");

	fprintf(fp, "interface=ra%d\n", ath_count);
	fprintf(fp, "bridge=br0\n");
	fprintf(fp, "driver=madwifi\n");
	fprintf(fp, "logger_syslog=0\n");
	fprintf(fp, "logger_syslog_level=0\n");
	fprintf(fp, "logger_stdout=0\n");
	fprintf(fp, "logger_stdout_level=0\n");
	fprintf(fp, "debug=0\n");
	fprintf(fp, "eapol_key_index_workaround=0\n");
	fprintf(fp, "\n");
	fprintf(fp, "dump_file=%s\n", HOSTAPD_DUMP_FILE_0);
#ifndef SSID_HEX_MODE
	fprintf(fp, "ssid=%s\n", ssid);
#else
	fprintf(fp, "ssid=%s\n", ssid_transfer(ssid));
#endif
	/*
	if((strcmp(wlan_security, "wpa_psk")== 0) ||
		       	(strcmp(wlan_security, "wpa_eap")== 0))
		fprintf(fp, "wpa=1\n");
	else if((strcmp(wlan_security, "wpa2_psk")== 0) ||
		       	(strcmp(wlan_security, "wpa2_eap")== 0))
		fprintf(fp, "wpa=2\n");
	else if((strcmp(wlan_security, "wpa2auto_psk")== 0) ||
		       	(strcmp(wlan_security, "wpa2auto_eap")== 0))
		fprintf(fp, "wpa=3\n");		
	*/
	
	if((strcmp(wlan_security, "wpa_psk")== 0) ||
		       	(strcmp(wlan_security, "wpa2_psk")== 0) ||
		       	(strcmp(wlan_security, "wpa2auto_psk")== 0)) {
		if(strlen(passphrase) == 64) {
			/*64 HEX*/
			fprintf(fp, "wpa_psk=%s\n", passphrase);
		} else {
			/*8~63 ASCII*/
			fprintf(fp, "wpa_passphrase=%s\n", passphrase);
		}
		fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");

	} else if((strcmp(wlan_security, "wpa_eap")== 0) ||
		       	(strcmp(wlan_security, "wpa2_eap")== 0) ||
		       	(strcmp(wlan_security, "wpa2auto_eap")== 0)) {

		lan_ipaddr = nvram_safe_get("static_addr0");
		
		sprintf(temp_buffer,"wlan%d_eap_mac_auth",ath_count); 
		wlan0_eap_mac_auth = nvram_safe_get(temp_buffer);

		sprintf(temp_buffer,"wlan%d_eap_radius_server_0", ath_count);  
		wlan0_wpa_eap_radius_server_1 = nvram_safe_get(temp_buffer);
		
		sprintf(temp_buffer,"wlan%d_eap_reauth_period", ath_count);  
		wlan0_eap_reauth_period = nvram_safe_get(temp_buffer);

		sprintf(temp_buffer,"wlan%d_eap_radius_server_0", ath_count);  
		wlan0_wpa_eap_radius_server_0 = nvram_safe_get(temp_buffer);

	    	if(strlen(wlan0_wpa_eap_radius_server_0) > 0) {
			radius1_ip = strtok(wlan0_wpa_eap_radius_server_0, "/");
			radius1_port = strtok(NULL,"/");
			radius1_secret = strtok(NULL,"/");
			if(radius1_ip == NULL || radius1_port == NULL ||
					radius1_secret == NULL ||
					!strcmp(radius1_ip,"not_set") ||
					!strcmp(radius1_port,"not_set") ||
					!strcmp(radius1_secret,"not_set") ) {	
				radius1_ip = "0.0.0.0"; 
				radius1_port = "1812";	
				radius1_secret = "00000";
			}
		} else {
			radius1_ip = "0.0.0.0"; 
			radius1_port = "1812";	
	    		radius1_secret = "11111";
		}
			  
		if(strlen(wlan0_wpa_eap_radius_server_1) > 0) {
			radius2_ip = strtok(wlan0_wpa_eap_radius_server_1, "/");
			radius2_port = strtok(NULL,"/");
			radius2_secret = strtok(NULL,"/");
			if(radius2_ip == NULL || radius2_port == NULL ||
				       	radius2_secret == NULL ||
				       	!strcmp(radius2_ip,"not_set") ||
				       	!strcmp(radius2_port,"not_set") ||
				       	!strcmp(radius2_secret,"not_set")) {
				radius2_ip = "0.0.0.0"; 
				radius2_port = "1812";	
				radius2_secret = "00000";
	    		}	
		} else {
			radius2_ip = "0.0.0.0"; 
			radius2_port = "1812";	
	    		radius2_secret = "11111";
		}
		
		fprintf(fp, "own_ip_addr=%s\n", lan_ipaddr);
		fprintf(fp, "auth_server_addr=%s\n", radius1_ip);
		fprintf(fp, "auth_server_port=%s\n", radius1_port);
		fprintf(fp, "auth_server_shared_secret=%s\n", radius1_secret);
		fprintf(fp, "auth_server_addr=%s\n", radius2_ip);
		fprintf(fp, "auth_server_port=%s\n", radius2_port);
		fprintf(fp, "auth_server_shared_secret=%s\n", radius2_secret);
		fprintf(fp, "ieee8021x=1\n");
		fprintf(fp, "wpa_key_mgmt=WPA-EAP\n");
		
		if( atoi(wlan0_eap_reauth_period) < 0)
			fprintf(fp, "eap_reauth_period=0\n");
		else
			fprintf(fp, "eap_reauth_period=%d\n",
				       	atoi(wlan0_eap_reauth_period)*60);	
			
		/*  
			mac_auth==0, don't need eap_mac_auth
			mac_auth==1, radius server 1 need eap_mac_auth
			mac_auth==2, radius server 2 need eap_mac_auth
			mac_auth==3, radius server all need eap_mac_auth
		if(strlen(wlan0_eap_mac_auth) > 0)
			fprintf(fp, "mac_auth=%s\n", wlan0_eap_mac_auth);
		else
			fprintf(fp, "mac_auth=3\n");	
		*/
		    
		if(strcmp(cipher_type, "tkip")== 0)
			fprintf(fp, "wpa_pairwise=TKIP\n"); 
		else if(strcmp(cipher_type, "aes")== 0)
			fprintf(fp, "wpa_pairwise=CCMP\n");
		else	 
			fprintf(fp, "wpa_pairwise=CCMP TKIP\n"); 
			
		if (atoi(rekey_time) < 120)
            		rekey_time = "120";

		fprintf(fp, "wpa_group_rekey=%s\n", rekey_time); 
		  
		fclose(fp); 
		_system("hostapd -B /tmp/hostapd.conf.0.%d", bss_index);
		//_system("hostapd -ddB /tmp/hostapd.conf.0.%d", bss_index);	//for debug	    
	}
}

static void set_radius(int ath_count)
{
	char radius0_server[50], *radius0_ip = "", *radius0_port = "", *radius0_secret = "";
	char radius1_server[50], *radius1_ip = "", *radius1_port = "", *radius1_secret = "";
	char tmp[64];

	/* radius server 1 */
	sprintf(tmp, "wlan%d_eap_radius_server_0", ath_count);  

	if(strlen(nvram_safe_get(tmp)) > 0 &&
		       	strncmp(nvram_safe_get(tmp), "0.0.0.0", 7)) {
		sprintf(radius0_server, "%s",  nvram_safe_get(tmp));		       		
		radius0_ip = strtok(radius0_server, "/");
		radius0_port = strtok(NULL, "/");
		radius0_secret = strtok(NULL, "/");
	}
	/* radius server 2 */
	sprintf(tmp, "wlan%d_eap_radius_server_1", ath_count);  
	if(strlen(nvram_safe_get(tmp)) > 0 &&
		       	strncmp(nvram_safe_get(tmp), "0.0.0.0", 7)) {
		sprintf(radius1_server, "%s",  nvram_safe_get(tmp));		       		
		radius1_ip = strtok(radius1_server, "/");
		radius1_port = strtok(NULL, "/");
		radius1_secret = strtok(NULL, "/");
	}
	
	_system("echo \"RADIUS_Server=%s;%s\" >> /var/tmp/iNIC_ap.dat",
			radius0_ip, radius1_ip);
	_system("echo \"RADIUS_Port=%s;%s\" >> /var/tmp/iNIC_ap.dat",
			radius0_port, radius1_port);
	_system("echo \"RADIUS_Key=%s;%s\" >> /var/tmp/iNIC_ap.dat",
			radius0_secret, radius1_secret);
	_system("echo \"own_ip_addr=%s\" >> /var/tmp/iNIC_ap.dat",
			       	nvram_safe_get("static_addr0"));
	_system("echo \"DefaultKeyID=2\" >> /var/tmp/iNIC_ap.dat");			       	
}

static void set_wep_key(int len, int bss_index, int ath_count)
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
     
	switch(len)
	{
		case 64:
			if(bss_index > 0) {
				sprintf(wep64_key_1, "wlan%d_vap%d_wep64_key_1",
					       	ath_count, bss_index);
				sprintf(wep64_key_2, "wlan%d_vap%d_wep64_key_2",
					       	ath_count, bss_index);
				sprintf(wep64_key_3, "wlan%d_vap%d_wep64_key_3",
					       	ath_count, bss_index);
				sprintf(wep64_key_4, "wlan%d_vap%d_wep64_key_4",
					       	ath_count, bss_index);
			} else {
				sprintf(wep64_key_1, "wlan%d_wep64_key_1",
					       	ath_count);
				sprintf(wep64_key_2, "wlan%d_wep64_key_2",
					       	ath_count);
				sprintf(wep64_key_3, "wlan%d_wep64_key_3",
					       	ath_count);
				sprintf(wep64_key_4, "wlan%d_wep64_key_4",
					       	ath_count);
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
	        			_system("echo \"Key%dStr1=%s\" >> "
							"/var/tmp/iNIC_ap.dat",
							i, wep64_key);
		    	}
			
			break;
		case 128:
			if(bss_index > 0) {
				sprintf(wep128_key_1, "wlan%d_vap%d_wep128_key_1",
					       	ath_count, bss_index);
				sprintf(wep128_key_2, "wlan%d_vap%d_wep128_key_2",
					       	ath_count, bss_index);
				sprintf(wep128_key_3, "wlan%d_vap%d_wep128_key_3",
					       	ath_count, bss_index);
				sprintf(wep128_key_4, "wlan%d_vap%d_wep128_key_4",
					       	ath_count, bss_index);
			} else {
				sprintf(wep128_key_1, "wlan%d_wep128_key_1",
					       	ath_count);
				sprintf(wep128_key_2, "wlan%d_wep128_key_2",
					       	ath_count);
				sprintf(wep128_key_3, "wlan%d_wep128_key_3",
					       	ath_count);
				sprintf(wep128_key_4, "wlan%d_wep128_key_4",
					       	ath_count);
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
		     
				if(strlen(wep128_key) < 10)
					DEBUG_MSG("wlan_wep128_key_%d null or"
						       " length error.\n", i);  
				else
	        			_system("echo \"Key%dStr1=%s\" >> "
							"/var/tmp/iNIC_ap.dat",
							i, wep128_key);
		    	}
			
			break;
			    
#if 0
		case 152:
			if(bss_index > 0)
			{
				sprintf(wep152_key_1, "wlan0_vap%d_wep152_key_1", bss_index);
				sprintf(wep152_key_2, "wlan0_vap%d_wep152_key_2", bss_index);
				sprintf(wep152_key_3, "wlan0_vap%d_wep152_key_3", bss_index);
				sprintf(wep152_key_4, "wlan0_vap%d_wep152_key_4", bss_index);
				wlan0_wep152_key_1 = nvram_safe_get(wep152_key_1);
				wlan0_wep152_key_2 = nvram_safe_get(wep152_key_2);    
				    wlan0_wep152_key_3 = nvram_safe_get(wep152_key_3);
				    wlan0_wep152_key_4 = nvram_safe_get(wep152_key_4); 
			    }
			    else
			    {
				wlan0_wep152_key_1 = nvram_safe_get("wlan0_wep152_key_1");
				wlan0_wep152_key_2 = nvram_safe_get("wlan0_wep152_key_2");    
				    wlan0_wep152_key_3 = nvram_safe_get("wlan0_wep152_key_3");
				    wlan0_wep152_key_4 = nvram_safe_get("wlan0_wep152_key_4"); 
			    }
			    
			    for(i=1; i<5; i++)
		    {
				    switch(i)
			{            
					case 1:
						wep152_key = wlan0_wep152_key_1;
						break;
					case 2:
						wep152_key = wlan0_wep152_key_2;
						break;
					case 3:
						wep152_key = wlan0_wep152_key_3;
						break;
					case 4:
						wep152_key = wlan0_wep152_key_4;
						break;		
			}
		     
				    if(strlen(wep152_key) < 32)
				DEBUG_MSG("wlan0_wep152_key_%d null or length error. \n", i); 
			else
				_system("iwconfig ath%d key [%d] \"%s\"", ath_count, i, wep152_key);
		    }
			
			break;
#endif
		default:
			 DEBUG_MSG("WEP key len is not correct. \n");
	}
	 
	/* set wep default key */
	if(bss_index > 0)
		sprintf(wep_default_key, "wlan%d_vap%d_wep_default_key",
			       	ath_count, bss_index);
	else
		sprintf(wep_default_key, "wlan%d_wep_default_key", ath_count);
	wlan_wep_default_key = nvram_safe_get(wep_default_key);	
	 
	if((atoi(wlan_wep_default_key) < 1) ||
		       	(atoi(wlan_wep_default_key) > 4)) {
		DEBUG_MSG("wlan_wep_default_key not correct. Default : 1. \n");
		wlan_wep_default_key = "1";
		_system("echo \"DefaultKeyID=1\" >> /var/tmp/iNIC_ap.dat");
		_system("iwconfig ath%d key [1]", ath_count);
	} else {
		_system("echo \"DefaultKeyID=%s\" >> /var/tmp/iNIC_ap.dat",
			       	wlan_wep_default_key);
	}
}

static void set_security(int bss_index, int ath_count)
{
	char *wlan_security, *cipher_type, *wlan_psk_pass_phrase, tmp[64];
	int wpa_flag = 0;
    
	sprintf(tmp, "wlan%d_security", ath_count);
    	wlan_security = nvram_safe_get(tmp);	                
	sprintf(tmp, "wlan%d_psk_pass_phrase", ath_count);
    	wlan_psk_pass_phrase = nvram_safe_get(tmp);	                


	/* set AuthMode EncrypType WPAPSK DefaultKeyID */
	if((strcmp(wlan_security,"disable") == 0))
	{	
	        _system("echo \"AuthMode=OPEN\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"EncrypType=NONE\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		_system("echo \"DefaultKeyID=1\" >> /var/tmp/iNIC_ap.dat");
	}
	else if((strcmp(wlan_security,"wep_open_64") == 0))
	{
	        _system("echo \"AuthMode=OPEN\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"EncrypType=WEP\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		set_wep_key(64, bss_index, ath_count);
	}
	else if((strcmp(wlan_security,"wep_share_64") == 0))
	{
	        _system("echo \"AuthMode=SHARED\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"EncrypType=WEP\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		set_wep_key(64, bss_index, ath_count);
	}       
	else if((strcmp(wlan_security,"wep_open_128") == 0))
    	{
	        _system("echo \"AuthMode=OPEN\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"EncrypType=WEP\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		set_wep_key(128, bss_index, ath_count);
	}
	else if((strcmp(wlan_security,"wep_share_128") == 0))  
	{
	        _system("echo \"AuthMode=SHARED\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"EncrypType=WEP\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		set_wep_key(128, bss_index, ath_count);
	}
	else if((strcmp(wlan_security,"wpa_psk") == 0))
	{
	        _system("echo \"AuthMode=WPAPSK\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=%s\" >> /var/tmp/iNIC_ap.dat",
			       	wlan_psk_pass_phrase);
		_system("echo \"DefaultKeyID=1\" >> /var/tmp/iNIC_ap.dat");
		wpa_flag = 1;
	}
	else if((strcmp(wlan_security,"wpa2_psk") == 0))
	{
	        _system("echo \"AuthMode=WPA2PSK\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=%s\" >> /var/tmp/iNIC_ap.dat",
			       	wlan_psk_pass_phrase);
		_system("echo \"DefaultKeyID=1\" >> /var/tmp/iNIC_ap.dat");
		wpa_flag = 1;
	}
	else if((strcmp(wlan_security,"wpa2auto_psk") == 0))
	{
	        _system("echo \"AuthMode=WPAPSKWPA2PSK\" >>"
			       " /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=%s\" >> /var/tmp/iNIC_ap.dat",
			       	wlan_psk_pass_phrase);
		_system("echo \"DefaultKeyID=1\" >> /var/tmp/iNIC_ap.dat");
		wpa_flag = 1;
	}
	else if((strcmp(wlan_security,"wpa_eap") == 0))
	{
	        _system("echo \"AuthMode=WPA\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		wpa_flag = 2;
	}
	else if((strcmp(wlan_security,"wpa2_eap") == 0))
	{
	        _system("echo \"AuthMode=WPA2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		wpa_flag = 2;
	}
	else if((strcmp(wlan_security,"wpa2auto_eap") == 0))
	{
	        _system("echo \"AuthMode=WPA1WPA2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"WPAPSK1=\" >> /var/tmp/iNIC_ap.dat");
		wpa_flag = 2;
	}	

	/* cipher type*/
	if(wpa_flag) {
		sprintf(tmp, "wlan%d_psk_cipher_type", ath_count);
		cipher_type = nvram_safe_get(tmp);
		if(strcmp(cipher_type, "tkip") == 0)
			_system("echo \"EncrypType=TKIP\" >> "
					"/var/tmp/iNIC_ap.dat");
		else if(strcmp(cipher_type, "aes")== 0)
			_system("echo \"EncrypType=AES\" >> "
					"/var/tmp/iNIC_ap.dat");
		else	 
			_system("echo \"EncrypType=TKIPAES\" >> "
					"/var/tmp/iNIC_ap.dat");
	}
   
	/* start WPA WPA2 Enterprise */
	if(wpa_flag == 2 && nvram_match("wps_enable", "0") == 0) {
		set_radius(ath_count);
	}else{
		_system("echo \"RADIUS_Server=%s;\" >> /var/tmp/iNIC_ap.dat",
				"0.0.0.0");
		_system("echo \"RADIUS_Port=%s;\" >> /var/tmp/iNIC_ap.dat",
				"0");
		_system("echo \"RADIUS_Key=%s;\" >> /var/tmp/iNIC_ap.dat",
				"0");
		_system("echo \"own_ip_addr=%s\" >> /var/tmp/iNIC_ap.dat",
			       	nvram_safe_get("static_addr0"));
	}

}

static void set_wmm(int bss_index, int ath_count)
{
    char *wlan0_wmm_enable;
    char wmm_enable[30];
    
	if(bss_index > 0) {
		sprintf(wmm_enable, "wlan%d_vap%d_wmm_enable", ath_count,
			       	bss_index);
		wlan0_wmm_enable = nvram_safe_get(wmm_enable);
	} else {
    		sprintf(wmm_enable, "wlan%d_wmm_enable", ath_count);
    		wlan0_wmm_enable = nvram_safe_get(wmm_enable);
	}
    	
    	if((strcmp(wlan0_wmm_enable,"0") == 0))
		_system("echo \"WmmCapable=0\" >> /var/tmp/iNIC_ap.dat");
    	else {
		_system("echo \"WmmCapable=1\" >> /var/tmp/iNIC_ap.dat");
    	}
}

static void set_GI_BW(int ath_count)
{
	char *wlan_dot11_mode, *wlan_auto_channel_enable, *wlan_channel;
	char tmp[64];

	sprintf(tmp, "wlan%d_dot11_mode",ath_count);
	wlan_dot11_mode = nvram_safe_get(tmp);
	sprintf(tmp, "wlan%d_auto_channel_enable", ath_count);
	wlan_auto_channel_enable = nvram_safe_get(tmp); 
	sprintf(tmp, "wlan%d_channel",ath_count);
	wlan_channel = nvram_safe_get(tmp);
	    
	/* HT_GI */
	sprintf(tmp, "wlan%d_short_gi", ath_count);
	if(nvram_match(tmp, "1") == 0)
	        _system("echo \"HT_GI=1\" >> /var/tmp/iNIC_ap.dat");
	else
	        _system("echo \"HT_GI=0\" >> /var/tmp/iNIC_ap.dat");
	     	
	/* HT_BW  HT_EXTCHA */
	if(ath_count == 0){
		if((strcmp(wlan_dot11_mode, "11bgn")==0 ||
				       	strcmp(wlan_dot11_mode, "11gn")==0 ||
				       	(strcmp(wlan_dot11_mode, "11n")==0 &&
					nvram_match("wlan0_radio_mode", "0")))/* &&
			       		strcmp(wlan_auto_channel_enable, "1") != 0*/) {
			sprintf(tmp, "wlan%d_11n_protection", ath_count);
			if((nvram_match(tmp, "auto") != 0) && !strcmp(wlan_auto_channel_enable, "1"))
				_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");
			else if(nvram_match(tmp, "auto") != 0)
		        	_system("echo \"HT_BW=0\" >> /var/tmp/iNIC_ap.dat");
		        else if(atoi(wlan_channel) <= 4) {
		        	_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");
		        	_system("echo \"HT_EXTCHA=1\" >> /var/tmp/iNIC_ap.dat");
		    	} else if(atoi(wlan_channel) >= 8) {
		        	_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");
		        	_system("echo \"HT_EXTCHA=0\" >> /var/tmp/iNIC_ap.dat");
			} else { 
		        	_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");
			}
		}
	}else{	    	 
		if((strcmp(wlan_dot11_mode, "11an")==0 ||
				       	(strcmp(wlan_dot11_mode, "11n")==0 &&
					nvram_match("wlan0_radio_mode", "1")))/* &&
			       		strcmp(wlan_auto_channel_enable, "1") != 0*/) {
			sprintf(tmp, "wlan%d_11n_protection", ath_count);
			
			if((nvram_match(tmp, "auto") != 0) && !strcmp(wlan_auto_channel_enable, "1"))
				_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");		
		        else if(nvram_match(tmp, "auto") != 0)
		        	_system("echo \"HT_BW=0\" >> /var/tmp/iNIC_ap.dat");
		        else if(atoi(wlan_channel) == 36) {
		        	_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");
		        	_system("echo \"HT_EXTCHA=1\" >> /var/tmp/iNIC_ap.dat");
		    	} else if(atoi(wlan_channel) == 165) {
		        	_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");
		        	_system("echo \"HT_EXTCHA=0\" >> /var/tmp/iNIC_ap.dat");
			} else { 
		        	_system("echo \"HT_BW=1\" >> /var/tmp/iNIC_ap.dat");
			}
		}
	}
	printf("%s\n", __FUNCTION__);	
}


/* ssid */
static void set_ssid(int ath_count)
{
	char *wlan_ssid, tmp[64];

	sprintf(tmp, "wlan%d_ssid", ath_count);
	wlan_ssid = nvram_safe_get(tmp);
#ifndef SSID_HEX_MODE
	    if((strlen(wlan_ssid)==0) || (strlen(wlan_ssid)>32)) {
	        DEBUG_MSG("wlan ssid null pointer or too long. Default"
			       " : default \n");
	        wlan_ssid = "dlink_default";
		_system("echo \"SSID1=dlink_default\" >> /var/tmp/iNIC_ap.dat");
	    }
#else
	    if((strlen(wlan_ssid) == 0) || (strlen(wlan_ssid) > 64)) {
	    	DEBUG_MSG("wlan ssid null pointer or too long. Default"
			       " : default \n");
		wlan_ssid = "dlink_default";
		_system("echo \"SSID1=dlink_default\" >> /var/tmp/iNIC_ap.dat");
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
			/*   test ssid = ',./`'~!@#$%^&*()_+[]\:",./[]\0-=  */				
			/*   ` = 0x60, " = 0x22, \ = 0x5c, ' = 0x27, $ = 0x24     */
			
			if((ssid_tmp[c1] == 0x22) || (ssid_tmp[c1] == 0x24) ||
			   (ssid_tmp[c1] == 0x5c) || (ssid_tmp[c1] == 0x60)) {
				ssid[c2] = 0x5c;
				c2++;
				ssid[c2] = ssid_tmp[c1];
			} else
				ssid[c2] = ssid_tmp[c1];
			c2++;		
		}
			_system("echo \"SSID1=%s\" >> /var/tmp/iNIC_ap.dat",
				       	ssid);
	    }
}

/* rts */
static void set_rts(int ath_count)
{
	char *wlan_rts_threshold, tmp[64];

	sprintf(tmp, "wlan%d_rts_threshold", ath_count);
	wlan_rts_threshold = nvram_safe_get(tmp);
        if( 256 <= atoi(wlan_rts_threshold) && atoi(wlan_rts_threshold) <= 2346)
		_system("echo \"RTSThreshold=%s\" >> /var/tmp/iNIC_ap.dat",
				       	wlan_rts_threshold);
        else
        	DEBUG_MSG("rts threshold value not correct. Default"
			       " : 2346 (OFF)\n");
	    
}

/* hide ssid*/
static void set_hidessid(int ath_count)
{
	char tmp[64];

	sprintf(tmp, "wlan%d_ssid_broadcast", ath_count);
        if(strcmp(nvram_safe_get(tmp),"0") == 0)
            _system(" echo \"HideSSID=1\" >> /var/tmp/iNIC_ap.dat");
        else if (strcmp(nvram_safe_get(tmp),"1") == 0)
            _system(" echo \"HideSSID=0\" >> /var/tmp/iNIC_ap.dat");
}

/* fragment */		
static void set_fragment(int ath_count)
{
	char *wlan_fragmentation, tmp[64];

	sprintf(tmp, "wlan%d_fragmentation", ath_count);
	wlan_fragmentation = nvram_safe_get(tmp);
	if(256 <= atoi(wlan_fragmentation) && atoi(wlan_fragmentation) <= 2346)
	        _system("echo \"FragThreshold=%s\" >> /var/tmp/iNIC_ap.dat",
			       	wlan_fragmentation);
	else
	        DEBUG_MSG("fragmentation value not correct. Default"
			       " : 2346 (OFF)\n");
	        
}

/* mode */
static void set_mode(int ath_count)
{
	char *wlan_dot11_mode, tmp[64];


	sprintf(tmp, "wlan%d_dot11_mode", ath_count);
	wlan_dot11_mode = nvram_safe_get(tmp);

	if (ath_count == 1) // 5G
	{

		if (strcmp(wlan_dot11_mode, "11n")==0) {
		        _system("echo \"WirelessMode=11\" >> /var/tmp/iNIC_ap.dat");
		} else if (strcmp(wlan_dot11_mode, "11a")==0) {
		        _system("echo \"WirelessMode=2\" >> /var/tmp/iNIC_ap.dat");
		} else if (strcmp(wlan_dot11_mode, "11an")==0) {
		        _system("echo \"WirelessMode=8\" >> /var/tmp/iNIC_ap.dat");
		} 	
	}else{//2.4G	
		if (strcmp(wlan_dot11_mode, "11b")==0) {
		        _system("echo \"WirelessMode=1\" >> /var/tmp/iNIC_ap.dat");
		} else if (strcmp(wlan_dot11_mode, "11gn")==0) {
		        _system("echo \"WirelessMode=7\" >> /var/tmp/iNIC_ap.dat");
		} else if (strcmp(wlan_dot11_mode, "11n")==0) {
		        _system("echo \"WirelessMode=6\" >> /var/tmp/iNIC_ap.dat");
		} else if (strcmp(wlan_dot11_mode, "11bg")==0) {
		        _system("echo \"WirelessMode=0\" >> /var/tmp/iNIC_ap.dat");
		} else if (strcmp(wlan_dot11_mode, "11g")==0) {
		        _system("echo \"WirelessMode=4\" >> /var/tmp/iNIC_ap.dat");
		} else {
			//bgn mode
		        _system("echo \"WirelessMode=9\" >> /var/tmp/iNIC_ap.dat");
		}
	}
	    			    
}

/* (auto) channel */
static void set_channel(int ath_count)
{
	char *wlan_auto_channel_enable, *wlan_channel, tmp[64];

	sprintf(tmp, "wlan%d_auto_channel_enable", ath_count);
	wlan_auto_channel_enable = nvram_safe_get(tmp);		
	//sprintf(tmp, "wlan%d_channel", ath_count);
	sprintf(tmp, "wlan%d_channel", atoi(nvram_safe_get("wlan0_radio_mode")));
	wlan_channel = nvram_safe_get(tmp);

	if(strcmp(wlan_auto_channel_enable, "1") == 0) {
		sprintf(tmp, "wlan%d_auto_channel_enable", ath_count);
		_system("echo \"AutoChannelSelect=%s\" >> /var/tmp/iNIC_ap.dat",
		      nvram_safe_get(tmp));
		DEBUG_MSG("AUTO Channel\n");
	} else {
	        if(atoi(nvram_safe_get("wlan0_radio_mode")) == 0 &&
			(atoi(wlan_channel) < 1 || atoi (wlan_channel) > 13)) {
	            	DEBUG_MSG("channel value error\n");
	            	_system("echo \"Channel=6\" >> /var/tmp/iNIC_ap.dat");
		} else if(atoi(nvram_safe_get("wlan0_radio_mode")) == 1 &&
			       	(atoi(wlan_channel) < 36 ||
				 atoi(wlan_channel) > 165)) {
	            	DEBUG_MSG("channel value error\n");
	            	_system("echo \"Channel=36\" >> /var/tmp/iNIC_ap.dat");
		} else {
	        	_system("echo \"Channel=%d\" >> /var/tmp/iNIC_ap.dat",
				       	atoi(wlan_channel));
		}	
	}    
	if (ath_count == 1){
	        	_system("echo \"AvoidDfsChannel=1\" >> /var/tmp/iNIC_ap.dat");
                        _system("echo \"IEEE80211H=0\" >> /var/tmp/iNIC_ap.dat");		
        }
}

/* WDS */
static void set_wds(int ath_count)
{
	char *wlan_wds_enable, tmp[64];

	sprintf(tmp, "wlan%d_wds_enable", ath_count);
	wlan_wds_enable = nvram_safe_get(tmp);    

	if(strcmp(wlan_wds_enable, "0") == 0){
	        _system("echo \"WdsEnable=0\" >> /var/tmp/iNIC_ap.dat");
#ifdef ATH_WDS_RPT
		sprintf(tmp, "wlan%d_op_mode", ath_count);
		if(nvram_match(tmp, "rpt_wds") == 0) {
	        	_system("echo \"WdsEnable=0\" >> /var/tmp/iNIC_ap.dat");
		}
#endif
	} else if(strcmp(wlan_wds_enable, "1") == 0) {
	        _system("echo \"WdsEnable=1\" >> /var/tmp/iNIC_ap.dat");
#ifdef ATH_WDS_RPT
		sprintf(tmp, "wlan%d_op_mode", ath_count);
		if(nvram_match(tmp, "rpt_wds") == 0) {
	        	_system("echo \"WdsEnable=1\" >> /var/tmp/iNIC_ap.dat");
		}
#endif			
	} else {
		DEBUG_MSG("WDS value error. Default : disable\n");
	        _system("echo \"WdsEnable=0\" >> /var/tmp/iNIC_ap.dat");
	}
	    	    

}

static void set_beacon_interval(int ath_count)
{
	char *wlan_beacon_interval, tmp[64];

	sprintf(tmp, "wlan%d_beacon_interval", ath_count);
	wlan_beacon_interval = nvram_safe_get(tmp);    

	/* beacon interval */
	if(20 <= atoi(wlan_beacon_interval) &&
		       	atoi(wlan_beacon_interval) <= 1000) {
	        _system("echo \"BeaconPeriod=%s\" >> /var/tmp/iNIC_ap.dat",
		      wlan_beacon_interval);
	} else {
		DEBUG_MSG("Invalid becon interval value \n");
	        _system("echo \"BeaconPeriod=100\" >> /var/tmp/iNIC_ap.dat");
	}
		        
}

static void set_dtim(int ath_count)
{
	char *wlan_dtim, tmp[64];

	sprintf(tmp, "wlan%d_dtim", ath_count);
	wlan_dtim = nvram_safe_get(tmp);    

	/* dtim period*/
	if( 1 <= atoi(wlan_dtim) && atoi(wlan_dtim) <=  255 )
	        _system("echo \"DtimPeriod=%s\" >> /var/tmp/iNIC_ap.dat",
			       	wlan_dtim);
	else {
		DEBUG_MSG("wlan0_dtim value not correct. Default :1 \n");
	        _system("echo \"DtimPeriod=1\" >> /var/tmp/iNIC_ap.dat");
	}
}
	    
static void set_preamble(int ath_count)
{
	char *wlan_short_preamble, tmp[64];

	sprintf(tmp, "wlan%d_short_preamble", ath_count);
	wlan_short_preamble = nvram_safe_get(tmp);    

	/* preamble */
	if(atoi(wlan_short_preamble) == 1)
	        _system("echo \"TxPreamble=1\" >> /var/tmp/iNIC_ap.dat");
	else {
	        _system("echo \"TxPreamble=0\" >> /var/tmp/iNIC_ap.dat");
	}
}

static void set_txrate(int ath_count)
{
	char *wlan_txrate, tmp[64];

	sprintf(tmp, "wlan%d_auto_txrate", ath_count);
	wlan_txrate = nvram_safe_get(tmp);    

	/* Tx rate */
	if(strcmp(wlan_txrate, "33") == 0) {
	        _system("echo \"HT_MCS=33\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "1000000") == 0) {
	        _system("echo \"FixedTxMode=1\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=0\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "2000000") == 0) {
	        _system("echo \"FixedTxMode=1\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=1\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "5500000") == 0) {
	        _system("echo \"FixedTxMode=1\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=2\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "6000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=0\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "9000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=1\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "11000000") == 0) {
	        _system("echo \"FixedTxMode=1\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=3\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "12000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=2\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "18000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=3\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "24000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=4\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "36000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=5\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "48000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=6\" >> /var/tmp/iNIC_ap.dat");
	} else if(strcmp(wlan_txrate, "54000000") == 0) {
	        _system("echo \"FixedTxMode=2\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=7\" >> /var/tmp/iNIC_ap.dat");
	} else if (strncmp(wlan_txrate, "0", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=0\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "1", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=1\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "2", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=2\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "3", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=3\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "4", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=4\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "5", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=5\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "6", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=6\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "7", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=7\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "8", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=8\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "9", 1) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=9\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "10", 2) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=10\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "11", 2) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=11\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "12", 2) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=12\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "13", 2) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=13\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "14", 2) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=14\" >> /var/tmp/iNIC_ap.dat");		
	} else if (strncmp(wlan_txrate, "15", 2) == 0) {
	        _system("echo \"FixedTxMode=0\" >> /var/tmp/iNIC_ap.dat");
	        _system("echo \"HT_MCS=15\" >> /var/tmp/iNIC_ap.dat");		
	}
}

/* Date: 2008-12-18
 * Name: Albert 
 * Descrption  add WPS icon information seting to Ralink config
 *  
 */
static void set_wps_infomation(int ath_count)
{	     
	     _system("echo \"WscManufacturer=D-Link\" >> /var/tmp/iNIC_ap.dat");
	     _system("echo \"WscDeviceName=D-Link WPS\" >> /var/tmp/iNIC_ap.dat");
	     _system("echo \"WscModelName=DIR-730\" >> /var/tmp/iNIC_ap.dat");	
		
}
/* 2.4G and 5G have the same prefix wlan0_ */
/* only channel and dot11_mode has wlan0_ and wlan1_ */
int rt2880_config(int ath_count)
{

	/*FIXME*/
	_system("cp /etc/Wireless/RT2880/iNIC_ap.dat /var/tmp/");

	/* ssid */
	set_ssid(ath_count);

	/* (auto) channel */
	set_channel(ath_count);

	/* hide ssid */
	set_hidessid(ath_count);

	/* HT_GI HT_BW */
	set_GI_BW(ath_count);	    
		    	    
	/* mode */
	set_mode(ath_count);

	/* security*/
	set_security(0, ath_count);

	/* wmm */
	set_wmm(0, ath_count);

	/* wds */
	set_wds(ath_count);
	    
	/* beacon interval*/
	set_beacon_interval(ath_count);

	/* dtim */
	set_dtim(ath_count);

	/* rts */
	set_rts(ath_count);
	
	/* fragment */		
	set_fragment(ath_count);
	
	/* tx rate */
	set_txrate(ath_count);
	
	set_wps_infomation(ath_count);
	

}

static void wlan_rmmod(void)
{
	_system("rmmod rt2880_iNIC");
}

static void wlan_insmod(void)
{
	char mac[32];
	/*
	get_mac(nvram_safe_get("if_phy0"), mac);
	_system("insmod /lib/modules/2.6.15/net/rt2880_iNIC.ko mac=%s", mac);
	*/
	_system("insmod /lib/modules/2.6.15/net/rt2880_iNIC.ko");
}
extern start_multissid(void);
int start_ap(void)
{
	int ath_count = 0;
	int flag;
	char tmp[32];
	ath_count = atoi(nvram_safe_get("wlan0_radio_mode"));

	DEBUG_MSG("Start AP ath_count = %d\n", ath_count);	

	if((flag=start_multissid()) != 0){
		rt2880_config(ath_count);
		_system("ifconfig ra0 up");
		_system("brctl addif %s ra0", nvram_safe_get("if_dev0"));
	}
	/* set interface attribute */
	//rt2880_config(ath_count);
       	// set iNIC_ap_dat for rt2880 before interface ra0 up 

	/* set up interface*/
	//_system("ifconfig ra0 up");
	//_system("brctl addif %s ra0", nvram_safe_get("if_dev0"));

	/* start rt2880apd */
	//sprintf(tmp, "wlan%d_security", ath_count);
	/*if(nvram_match("wps_enable", "0") == 0 &&
		       	(nvram_match(tmp,"wpa_eap") == 0 ||
			 nvram_match(tmp,"wpa2_eap") == 0 ||
			 nvram_match(tmp,"wpa2auto_eap") == 0)) {
		_system("rt2880apd &");
		//start_hostapd(0, ath_count);
	}*/
    	DEBUG_MSG("finish start_ap()\n");
    
    	return 0;
}
void trigger_wireless_led(int action)
{
	int ret;
	int gpio_fd;
	gpio_fd = open( GPIO_DRIVER, O_RDONLY);
	if (gpio_fd < 0 ) 
	{
		printf( "ERROR : cannot open gpio driver\n");
		return;
	}	
	ret = ioctl(gpio_fd, WALN_LED, action);	
	close(gpio_fd);
	return;
   	
}

int start_wlan(void)
{
	DEBUG_MSG("***** start_wlan *******\n");
	wlan_insmod();
	DEBUG_MSG("match test:%d", nvram_match("wlan0_enable", "1"));
	if(nvram_match("wlan0_enable", "1") == 0) {
		/* insert modules */

		/* set up ap */
		trigger_wireless_led(LEDENABLE);
		start_ap();
		_system("cli net wl wps_init");
	} else {
		DEBUG_MSG("******* Wireless Radio OFF *********\n");	
		trigger_wireless_led(LEDDISABLE);	
		return 1;
	}	

	DEBUG_MSG("********** Start WLAN Finished**************\n");
	return 0;
}

int stop_wlan(void)
{
	/* set down interface*/
	_system("ifconfig ra%d down", ath_count);
	/* no lan_bridge key */
	//_system("brctl delif %s ra0", nvram_safe_get("lan_bridge"));
	_system("brctl delif %s ra0", "br0");

	/* unload modules */
	wlan_rmmod();
}



int start_wlan_main(int argc, char *argv[])
{
	return start_wlan();
}

int stop_wlan_main(int argc, char *argv[])
{
	return stop_wlan();
}

int wl_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", start_wlan_main},
		{ "stop", stop_wlan_main},
//Albert add
		{ "wps_init", init_wps_main},
		{ "trigger_wsc", trigger_wsc_main},
		{ "wsc_button", wsc_button_main},
		{ "wsc_getstatus", wsc_getstatus_main},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
