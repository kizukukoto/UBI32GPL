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

#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>
#include "bsp.h"

#define AP0 "/var/tmp/iNIC_ap.dat" // set 5G config
#define AP1 "/var/tmp/iNIC_ap1.dat" // set 2.4G config

//#define WLAN_DEBUG
#ifdef WLAN_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static void set_wep_key(int len, int bss_index, int ath_count, const char *path)
{
	int default_key;
	char *wlan_wep64_key_1;
	char *wlan_wep64_key_2;
	char *wlan_wep64_key_3;
	char *wlan_wep64_key_4;
	char *wlan_wep128_key_1;
	char *wlan_wep128_key_2;
	char *wlan_wep128_key_3;
	char *wlan_wep128_key_4;
#if 0
	char *wlan_wep152_key_1;
	char *wlan_wep152_key_2;
	char *wlan_wep152_key_3;
	char *wlan_wep152_key_4;
#endif 
	//unsigned char *wlan_wep_default_key; 
	char *wep64_key=NULL;
	char *wep128_key=NULL;
	//char *wep152_key=NULL;

	char wep64_key_1[30];
	char wep64_key_2[30];
	char wep64_key_3[30];
	char wep64_key_4[30];
	char wep128_key_1[30];
	char wep128_key_2[30];
	char wep128_key_3[30];
	char wep128_key_4[30];
	char wep_default_key[30];
#if 0
	char wep152_key_1[30];
	char wep152_key_2[30];
	char wep152_key_3[30];
	char wep152_key_4[30];
	char wep_default_key[30];
#endif	
	switch(len)
	{
		case 64:
			if (bss_index == 0) {
				sprintf(wep64_key_1, "wlan%d_wep64_key_1",
					       	ath_count);
				sprintf(wep64_key_2, "wlan%d_wep64_key_2",
					       	ath_count);
				sprintf(wep64_key_3, "wlan%d_wep64_key_3",
					       	ath_count);
				sprintf(wep64_key_4, "wlan%d_wep64_key_4",
					       	ath_count);
				sprintf(wep_default_key, "wlan%d_wep_default_key", 
						ath_count);
			} else {
				sprintf(wep64_key_1, "wlan%d_vap%d_wep64_key_1",
					       	ath_count, bss_index);
				sprintf(wep64_key_2, "wlan%d_vap%d_wep64_key_2",
					       	ath_count, bss_index);
				sprintf(wep64_key_3, "wlan%d_vap%d_wep64_key_3",
					       	ath_count, bss_index);
				sprintf(wep64_key_4, "wlan%d_vap%d_wep64_key_4",
					       	ath_count, bss_index);
				sprintf(wep_default_key, "wlan%d_vap1_wep_default_key", 
						ath_count);
			}

			wlan_wep64_key_1 = nvram_safe_get(wep64_key_1);
			wlan_wep64_key_2 = nvram_safe_get(wep64_key_2);    
			wlan_wep64_key_3 = nvram_safe_get(wep64_key_3);
			wlan_wep64_key_4 = nvram_safe_get(wep64_key_4);
			default_key = strtol(nvram_safe_get(wep_default_key), 
					NULL, 10);
			switch(default_key) {            
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
				DEBUG_MSG("wlan_wep64_key_%d null or length error.\n", 
				default_key);
			else{
				if (bss_index == 0)
					_system("echo \"Key%dStr1=%s\" >> %s", 
						default_key, wep64_key, path);
				else
					_system("echo \"Key%dStr2=%s\" >> %s", 
						default_key, wep64_key, path);
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
				sprintf(wep_default_key, "wlan%d_wep_default_key", 
						ath_count);
			} else {
				sprintf(wep128_key_1, "wlan%d_wep128_key_1",
					       	ath_count);
				sprintf(wep128_key_2, "wlan%d_wep128_key_2",
					       	ath_count);
				sprintf(wep128_key_3, "wlan%d_wep128_key_3",
					       	ath_count);
				sprintf(wep128_key_4, "wlan%d_wep128_key_4",
					       	ath_count);
				sprintf(wep_default_key, "wlan%d_vap1_wep_default_key", 
						ath_count);
			}   
			wlan_wep128_key_1 = nvram_safe_get(wep128_key_1);
			wlan_wep128_key_2 = nvram_safe_get(wep128_key_2);    
			wlan_wep128_key_3 = nvram_safe_get(wep128_key_3);
			wlan_wep128_key_4 = nvram_safe_get(wep128_key_4);
			default_key = strtol(nvram_safe_get(wep_default_key), 
					NULL, 10);
			switch(default_key) {            
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
				DEBUG_MSG("wlan_wep128_key_%d null or length error.\n", 
				default_key);
			else{
				if (bss_index == 0)
					_system("echo \"Key%dStr1=%s\" >> %s", 
						default_key, wep128_key, path);
				else
					_system("echo \"Key%dStr2=%s\" >> %s", 
						default_key, wep128_key, path);
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
					else{
						if(ath_count==0){//2.4G
							_system("echo \"Key%dStr1=%s\" >> /var/tmp/iNIC_ap1.dat", i, wep152_key);							
						}
						else{//5G
							_system("echo \"Key%dStr1=%s\" >> /var/tmp/iNIC_ap.dat", i, wep152_key);	
						}		
					}	
		     }
			
			break;
#endif
		default:
			 DEBUG_MSG("WEP key len is not correct. \n");
	}
}

/* set mbssid security */
static void set_multiple_bssid(char *auth, char *encrypt, 
			int auth_size, int encrypt_size, 
			int type, const char *path)
{
	char _auth[auth_size], _encrypt[encrypt_size], tmp[50];
	char *wlan_security, *wlan_psk_pass_phrase, *cipher_type;
	int wpa_flag = 0;	

	if (type == 0) { /** 2.4G  */
		sprintf(tmp, "wlan%d_vap1_security", type);
		wlan_security = nvram_safe_get(tmp);
		sprintf(tmp, "wlan%d_vap1_psk_pass_phrase", type);
		wlan_psk_pass_phrase = nvram_safe_get(tmp);
	} else { /** 5G */
		sprintf(tmp, "wlan%d_vap1_security", type);
		wlan_security = nvram_safe_get(tmp);
		sprintf(tmp, "wlan%d_vap1_psk_pass_phrase", type);
		wlan_psk_pass_phrase = nvram_safe_get(tmp);
	}

	if ((strcmp(wlan_security, "disable") == 0)) {
		sprintf(_auth, "%s;%s", auth, "OPEN");
		sprintf(_encrypt, "%s;%s", encrypt, "NONE");
	} else if ((strcmp(wlan_security, "wep_open_64") == 0)) {
		sprintf(_auth, "%s;%s", auth, "OPEN");
		sprintf(_encrypt, "%s;%s", encrypt, "WEP");
		set_wep_key(64, 1, type, path);
	} else if ((strcmp(wlan_security, "wep_share_64") == 0)) {
		sprintf(_auth, "%s;%s", auth, "SHARED");
		sprintf(_encrypt, "%s;%s", encrypt, "WEP");
		set_wep_key(64, 1, type, path);
	}  else if ((strcmp(wlan_security, "wep_open_128") == 0)) {
		sprintf(_auth, "%s;%s", auth, "OPEN");
		sprintf(_encrypt, "%s;%s", encrypt, "WEP");
		set_wep_key(128, 1, type, path);
	}  else if ((strcmp(wlan_security, "wep_share_128") == 0)) {
		sprintf(_auth, "%s;%s", auth, "SHARED");
		sprintf(_encrypt, "%s;%s", encrypt, "WEP");
		set_wep_key(128, 1, type, path);
	}  else if ((strcmp(wlan_security, "wpa_psk") == 0)) {
		sprintf(_auth, "%s;%s", auth, "WPAPSK");
		_system("echo \"WPAPSK2=%s\" >> %s", wlan_psk_pass_phrase, path);
		wpa_flag = 1;
	}  else if ((strcmp(wlan_security, "wpa2_psk") == 0)) {
		sprintf(_auth, "%s;%s", auth, "WPA2PSK");
		_system("echo \"WPAPSK2=%s\" >> %s", wlan_psk_pass_phrase, path);
		wpa_flag = 1;
	}  else if ((strcmp(wlan_security, "wpa2auto_psk") == 0)) {
		sprintf(_auth, "%s;%s", auth, "WPAPSKWPA2PSK");
		_system("echo \"WPAPSK2=%s\" >> %s", wlan_psk_pass_phrase, path);
		wpa_flag = 1;
	}  else if ((strcmp(wlan_security, "wpa_eap") == 0)) {
		sprintf(_auth, "%s;%s", auth, "WPA");
		wpa_flag = 2;
	}  else if ((strcmp(wlan_security, "wpa2_eap") == 0)) {
		sprintf(_auth, "%s;%s", auth, "WPA2");
		wpa_flag = 2;
	}  else if ((strcmp(wlan_security, "wpa2auto_eap") == 0)) {
		sprintf(_auth, "%s;%s", auth, "WPA1WPA2");
		wpa_flag = 2;
	}

	if(wpa_flag) {
		sprintf(tmp, "wlan%d_vap1_psk_cipher_type", type);
		cipher_type = nvram_safe_get(tmp);
		if(strcmp(cipher_type, "tkip") == 0)
			sprintf(_encrypt, "%s;%s", encrypt, "TKIP");
		else if(strcmp(cipher_type, "aes")== 0)
			sprintf(_encrypt, "%s;%s", encrypt, "AES");
		else	 
			sprintf(_encrypt, "%s;%s", encrypt, "TKIPAES");
	}

	strcpy(auth, _auth);
	strcpy(encrypt, _encrypt);
}

static void set_txrate(int ath_count, const char *path)
{
	char *wlan_txrate, tmp[64];

	sprintf(tmp, "wlan%d_auto_txrate", ath_count);
	wlan_txrate = nvram_safe_get(tmp);    
		/* Tx rate */
		if(strcmp(wlan_txrate, "33") == 0) {
		        _system("echo \"HT_MCS=33\" >> %s", path);
		} else if(strcmp(wlan_txrate, "1000000") == 0) {
		        _system("echo \"FixedTxMode=1\" >> %s", path);
		        _system("echo \"HT_MCS=0\" >> %s", path);
		} else if(strcmp(wlan_txrate, "2000000") == 0) {
		        _system("echo \"FixedTxMode=1\" >> %s", path);
		        _system("echo \"HT_MCS=1\" >> %s", path);
		} else if(strcmp(wlan_txrate, "5500000") == 0) {
		        _system("echo \"FixedTxMode=1\" >> %s", path);
		        _system("echo \"HT_MCS=2\" >> %s", path);
		} else if(strcmp(wlan_txrate, "6000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=0\" >> %s", path);
		} else if(strcmp(wlan_txrate, "9000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=1\" >> %s", path);
		} else if(strcmp(wlan_txrate, "11000000") == 0) {
		        _system("echo \"FixedTxMode=1\" >> %s", path);
		        _system("echo \"HT_MCS=3\" >> %s", path);
		} else if(strcmp(wlan_txrate, "12000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=2\" >> %s", path);
		} else if(strcmp(wlan_txrate, "18000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=3\" >> %s", path);
		} else if(strcmp(wlan_txrate, "24000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=4\" >> %s", path);
		} else if(strcmp(wlan_txrate, "36000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=5\" >> %s", path);
		} else if(strcmp(wlan_txrate, "48000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=6\" >> %s", path);
		} else if(strcmp(wlan_txrate, "54000000") == 0) {
		        _system("echo \"FixedTxMode=2\" >> %s", path);
		        _system("echo \"HT_MCS=7\" >> %s", path);
		} else if (strncmp(wlan_txrate, "0", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=0\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "1", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=1\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "2", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=2\" >> %s", path);
		} else if (strncmp(wlan_txrate, "3", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=3\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "4", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=4\" >> %s", path);
		} else if (strncmp(wlan_txrate, "5", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=5\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "6", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=6\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "7", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=7\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "8", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=8\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "9", 1) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=9\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "10", 2) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=10\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "11", 2) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=11\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "12", 2) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=12\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "13", 2) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=13\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "14", 2) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=14\" >> %s", path);		
		} else if (strncmp(wlan_txrate, "15", 2) == 0) {
		        _system("echo \"FixedTxMode=0\" >> %s", path);
		        _system("echo \"HT_MCS=15\" >> %s", path);		
		}
}

static void set_wps_infomation(int ath_count, const char *path)
{
	char *p_lan_mac, tmp[32], ssid[32];
	int lan_mac[6] = {0};

	_system("echo \"WscManufacturer=D-Link\" >> %s", path);
	_system("echo \"WscDeviceName=D-Link WPS\" >> %s", path);
	_system("echo \"WscModelName=DIR-865\" >> %s", path);

/* get last four digital of mac address */
	sprintf(tmp, "%s", nvram_safe_get("lan_mac"));
	p_lan_mac = tmp;
	sscanf(p_lan_mac,"%02x:%02x:%02x:%02x:%02x:%02x",
		&lan_mac[0], &lan_mac[1], &lan_mac[2],
		&lan_mac[3], &lan_mac[4], &lan_mac[5]);

	if (ath_count == 0)
		sprintf(ssid, "dlink%02x%02x", \
				(lan_mac[4] & 0xFF), \
				(lan_mac[5] & 0xFF));
	else
		sprintf(ssid, "dlink%02x%02x", \
				(lan_mac[4] & 0xFF), \
				(lan_mac[5] & 0xFF) + 2);

	_system("echo \"WscDefaultSSID1=%s\" >> %s", ssid, path);
}

/* Get Wlan domain from artblock */
static int get_wlan_domain()
{
	char *wlan0_domain = NULL;
	int regdomain = 0;
#ifdef SET_GET_FROM_ARTBLOCK
	wlan0_domain = artblock_get("wlan0_domain");
	DEBUG_MSG("wlan0_domain=%s\n", wlan0_domain);
	if(wlan0_domain != NULL)		
		regdomain = strtol(wlan0_domain, NULL, 16);
	else
		regdomain = strtol(nvram_safe_get("wlan0_domain"), NULL, 16);
	DEBUG_MSG("regdomain=%x\n", regdomain);				
#else
	regdomain = strtol(nvram_safe_get("wlan0_domain"), NULL, 16);
	DEBUG_MSG("regdomain=%x\n", regdomain);
#endif
	return regdomain;
}

static int set_country(const char *path)
{
	int domain = 0;
	int CountryRegion = 0;
	int CountryRegionABand = 0;

	domain = get_wlan_domain();

	switch(domain)
	{
	   case 0x03://UNITED ARAB EMIRATES	784;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 0;
	   	break;
	   case 0x07://CTRY_ISRAEL 376;	
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 0;
	   	break;	   
	   case 0x14://CTRY_CANADA2 5001;****
	   	DEBUG_MSG("domain=%x\n",domain);	
	   	break;	
	   case 0x21://Australia&HK 36 
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 0;
	   	break;
	   case 0x23://CTRY_AUSTRALIA2 5000;****
	   	DEBUG_MSG("domain=%x\n",domain);
	   	break;	   
	   case 0x36://CTRY_EGYPT 818;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 2;
	   	break;
	   case 0x3C://CTRY_RUSSIA 643;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 0;
	   	break;	
	   case 0x30:
	   case 0x37:	//CTRY_GERMANY 276; 
	   case 0x40:	//CTRY_JAPAN 
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 1;
	   	break;	
	   case 0x10: 
	   case	0x3A:   //CTRY_UNITED_STATES 840;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion=0;	
		CountryRegionABand=0;
	   	break;		   
	   case 0x3B://CTRY_BRAZIL 76;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 1;
	   	break;
	   case 0x41: //CTRY_JAPAN1 393;****
	   	DEBUG_MSG("domain=%x\n",domain);
	   	break;
	   case 0x50://CTRY_TAIWAN 158;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 0;
	   	CountryRegionABand = 3;
	   	break;
	   case 0x51://CTRY_CHILE 152;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 0;
	   	break;
	   case 0x52://CTRY_CHINA 156;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 4;
	   	break;
	   case 0x5B://CTRY_SINGAPORE 702;
	   	DEBUG_MSG("domain=%x\n",domain);
	   	CountryRegion = 1;
	   	CountryRegionABand = 0;
	   	break;		 
	  case 0x5E://CTRY_KOREA_ROC3 412;*****
	  	DEBUG_MSG("domain=%x\n",domain);
	   	break;		 	 			 			
	  case 0x5F://CTRY_KOREA_NORTH 408;
	  	DEBUG_MSG("domain=%x\n",domain);
	  	CountryRegion = 1;
	   	CountryRegionABand = 5;
	   	break;  	   			
	  default ://CTRY_UNITED_STATES 840 
	  	DEBUG_MSG("unknow domain=%x\n",domain);
	   	CountryRegion=0;	
		CountryRegionABand=0;
	   	break;		    	  	   			
	}

	DEBUG_MSG("CountryRegion=%d\n",CountryRegion);
	DEBUG_MSG("CountryRegionABand=%d\n",CountryRegionABand);

	_system("echo \"CountryRegion=%d\" >> %s", CountryRegion, path);
	_system("echo \"CountryRegionABand=%d\" >> %s", CountryRegionABand, path);
	_system("echo \"CountryCode=\" >> %s", path);

	return 0;
}

static char *replace_special_characters(char *wlan_ssid)
{
	char ssid[128];
	char ssid_tmp[64];
	int len = 0, c1 = 0, c2 = 0;
		
	memset(ssid, 0, sizeof(ssid));
	memset(ssid_tmp, 0, sizeof(ssid_tmp));

	sprintf(ssid_tmp, "%s", wlan_ssid);
	len = strlen(ssid_tmp);

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

	strcpy(wlan_ssid, ssid);

	return wlan_ssid;
}

static char* check_ssid(int iface, char *wlan_ssid)
{
	char tmp_ssid[128], *ssid;
#ifndef SSID_HEX_MODE
	if ((strlen(wlan_ssid)==0) || (strlen(wlan_ssid)>32)) {
		DEBUG_MSG("wlan ssid null pointer or too long. Default : auth_count = %d \n",
			 iface);
	goto out;
#else 
	if((strlen(wlan_ssid) == 0) || (strlen(wlan_ssid) > 64)) {
		debug_msg("wlan ssid null pointer or too long. default : auth_count = %d \n", 
			iface);
	goto out;
#endif//SSID_HEX_MODE
	} else {
		sprintf(tmp_ssid, "%s", wlan_ssid);
		ssid = tmp_ssid;
		replace_special_characters(ssid);
	}
	strcpy(wlan_ssid, ssid);
	return wlan_ssid;
out:
	return (iface == 0 ? "dlink" : "dlink_media");
}

static void set_ssid(int iface, const char *path)
{
	char *wlan_ssid, tmp[32], *wlan_ssid1, *wlan_enable;

	sprintf(tmp, "wlan%d_ssid", iface);
	wlan_ssid = nvram_safe_get(tmp);

	wlan_ssid = check_ssid(iface, wlan_ssid);
	_system("echo \"SSID1=%s\" >> %s", wlan_ssid, path);

	sprintf(tmp, "wlan%d_vap1_enable", iface);
	wlan_enable = nvram_safe_get(tmp);
	
	if (strtol(wlan_enable, NULL, 10)) {
		sprintf(tmp, "wlan%d_vap1_ssid", iface);
		wlan_ssid1 = nvram_safe_get(tmp);
		wlan_ssid1 = check_ssid(iface, wlan_ssid1);
		_system("echo \"SSID2=%s\" >> %s", wlan_ssid1, path);
	} else {
		_system("echo \"SSID2=\" >> %s", path);
	}

	_system("echo \"SSID3=\" >> %s", path);
	_system("echo \"SSID4=\" >> %s", path);
}

/* mode */
static void set_mode(int ath_count, const char *path)
{
	char *wlan_dot11_mode, tmp[64];

	sprintf(tmp, "wlan%d_dot11_mode", ath_count);
	wlan_dot11_mode = nvram_safe_get(tmp);
	
	if(ath_count==0){//2.4G	
		if (strcmp(wlan_dot11_mode, "11b") == 0) {
			_system("echo \"WirelessMode=1\" >> %s", path);
		} else if (strcmp(wlan_dot11_mode, "11gn") == 0) {
			_system("echo \"WirelessMode=7\" >> %s", path);
		} else if (strcmp(wlan_dot11_mode, "11n") == 0) {
			_system("echo \"WirelessMode=6\" >> %s", path);
		} else if (strcmp(wlan_dot11_mode, "11bg") == 0) {
			_system("echo \"WirelessMode=0\" >> %s", path);
		} else if (strcmp(wlan_dot11_mode, "11g") == 0) {
			_system("echo \"WirelessMode=4\" >> %s", path);
		} else { //bgn mode
			_system("echo \"WirelessMode=9\" >> %s", path);
		}	
	}
	else{//5G
		if (strcmp(wlan_dot11_mode, "11n") == 0) {
			_system("echo \"WirelessMode=11\" >> %s", path);
		} else if (strcmp(wlan_dot11_mode, "11a") == 0) {
			_system("echo \"WirelessMode=2\" >> %s", path);
		} else { //an mode
			_system("echo \"WirelessMode=8\" >> %s", path);
		}
	}			    			    
}

/* (auto) channel */
static void set_channel(int ath_count, const char *path)
{
	char *wlan_auto_channel_enable, *wlan_channel, tmp[64];

	sprintf(tmp, "wlan%d_auto_channel_enable", ath_count);
	wlan_auto_channel_enable = nvram_safe_get(tmp);		
	sprintf(tmp, "wlan%d_channel", ath_count);
	wlan_channel = nvram_safe_get(tmp);

	if (strcmp(wlan_auto_channel_enable, "1") == 0) {
		//DEBUG_MSG("AUTO Channel\n");
			_system("echo \"AutoChannelSelect=1\" >> %s", path);
	} else {	    
	    if (ath_count == 0){//2.4G	
	    	if (atoi(wlan_channel) < 1 || atoi(wlan_channel) > 13 ) {
	            	DEBUG_MSG("2.4 channel value error\n");
	            	_system("echo \"Channel=6\" >> %s", path);
		} else
	        	_system("echo \"Channel=%d\" >> %s", atoi(wlan_channel), path);	
	    } else{//5G
		if (atoi(wlan_channel) < 36 || atoi(wlan_channel) > 165 ) {
			DEBUG_MSG("5 channel value error\n");
			_system("echo \"Channel=36\" >> %s", path);
		} else
			_system("echo \"Channel=%d\" >> %s", atoi(wlan_channel), path);
	    }	
	}
}

static void set_beacon_interval(int ath_count, const char *path)
{
	char *wlan_beacon_interval, tmp[64];

	sprintf(tmp, "wlan%d_beacon_interval", ath_count);
	wlan_beacon_interval = nvram_safe_get(tmp);    

	/* beacon interval */
	if(20 <= atoi(wlan_beacon_interval) && 	atoi(wlan_beacon_interval) <= 1000) {
	    _system("echo \"BeaconPeriod=%s\" >> %s", 
			wlan_beacon_interval, path);
	} else {
	    DEBUG_MSG("Invalid becon interval value \n");
	    _system("echo \"BeaconPeriod=100\" >> %s", path);
	}
}

static void set_dtim(int ath_count, const char *path)
{
	char *wlan_dtim, tmp[64];

	sprintf(tmp, "wlan%d_dtim", ath_count);
	wlan_dtim = nvram_safe_get(tmp);    

	/* dtim period*/
	if( 1 <= atoi(wlan_dtim) && atoi(wlan_dtim) <=  255 )
		_system("echo \"DtimPeriod=%s\" >> %s", wlan_dtim, path);
	else {
		DEBUG_MSG("wlan0_dtim value not correct. Default :1 \n");
	        _system("echo \"DtimPeriod=1\" >> %s", path);
	}
}

static void set_preamble(int ath_count, const char *path)
{
	char *wlan_short_preamble, tmp[64];

	sprintf(tmp, "wlan%d_short_preamble", ath_count);
	wlan_short_preamble = nvram_safe_get(tmp);    

	/* preamble */
	if(wlan_short_preamble && atoi(wlan_short_preamble) == 1)
		_system("echo \"TxPreamble=1\" >> %s", path);
	else
		_system("echo \"TxPreamble=0\" >> %s", path);
	
}

/* rts */
static void set_rts(int ath_count, const char *path)
{
	char *wlan_rts_threshold, tmp[64];

	sprintf(tmp, "wlan%d_rts_threshold", ath_count);
	wlan_rts_threshold = nvram_safe_get(tmp);
    	if( 256 <= atoi(wlan_rts_threshold) && atoi(wlan_rts_threshold) <= 2347)
		_system("echo \"RTSThreshold=%s\" >> %s", wlan_rts_threshold, path);			       	
	else
 	       DEBUG_MSG("rts threshold value not correct. Default : 2347 (OFF)\n");
}

/* fragment */		
static void set_fragment(int ath_count, const char *path)
{
	char *wlan_fragmentation, tmp[64];
	sprintf(tmp, "wlan%d_fragmentation", ath_count);
	wlan_fragmentation = nvram_safe_get(tmp);
	if(256 <= atoi(wlan_fragmentation) && atoi(wlan_fragmentation) <= 2346)
	     	_system("echo \"FragThreshold=%s\" >> %s", wlan_fragmentation, path);
	else
	    DEBUG_MSG("fragmentation value not correct. Default : 2346 (OFF)\n");
}

static void set_wmm(int bss_index, int ath_count, const char *path)
{
	char *wlan0_wmm_enable;
	char wmm_enable[30];

	sprintf(wmm_enable, "wlan%d_wmm_enable", ath_count);
	wlan0_wmm_enable = nvram_safe_get(wmm_enable);

	if((strcmp(wlan0_wmm_enable, "0") == 0))
		_system("echo \"WmmCapable=%s\" >> %s", (bss_index == 1) ? "0" : "0;0", path);
	else
		_system("echo \"WmmCapable=%s\" >> %s", (bss_index == 1) ? "1" : "1;1", path);

	_system("echo \"APSDCapable=0\" >> %s", path);
	_system("echo \"APAifsn=3;7;1;1\" >> %s", path);
	_system("echo \"APCwmin=4;4;3;2\" >> %s", path);
	_system("echo \"APCwmax=6;10;4;3\" >> %s", path);
	_system("echo \"APTxop=0;0;94;47\" >> %s", path);
	_system("echo \"APACM=0;0;0;0\" >> %s", path);
	_system("echo \"BSSAifsn=3;7;2;2\" >> %s", path);
	_system("echo \"BSSCwmin=4;4;3;2\" >> %s", path);
	_system("echo \"BSSCwmax=10;10;4;3\" >> %s", path);
	_system("echo \"BSSTxop=0;0;94;47\" >> %s", path);
	_system("echo \"BSSACM=0;0;0;0\" >> %s", path);
	_system("echo \"DLSCapable=0\" >> %s", path);
	_system("echo \"AckPolicy=0;0;0;0\" >> %s", path);
}

/* hide ssid*/
static void set_hidessid(int ath_count, const char *path, int bssid)
{
    char tmp[64], *hidessid;
    sprintf(tmp, "wlan%d_ssid_broadcast", ath_count);
    hidessid = nvram_safe_get(tmp);

    if (bssid > 1)
	_system(" echo \"HideSSID=%s\" >> %s", (hidessid == 0) ? "1;1": "0;0" , path);
    else
	_system(" echo \"HideSSID=%s\" >> %s", (hidessid == 0) ? "1": "0" , path);
}

/* set wep default key */
static void set_wep_default_key(int ath_count, const char *path)
{
	char wep_default_key[30];
	char default_key[128];
	char default_type[30];
	char *wlan_wep_default_key;
	char *wlan_wep_type;
	char tmp[128];

	sprintf(default_key, "echo \"DefaultKeyID=");
	sprintf(default_type, "wlan%d_security", ath_count);
	wlan_wep_type = nvram_safe_get(default_type);
	sprintf(wep_default_key, "wlan%d_wep_default_key", ath_count);
	wlan_wep_default_key = nvram_safe_get(wep_default_key);
	sprintf(default_key, "%s%s", default_key, wlan_wep_default_key);
	_system("echo \"Key1Type=0\" >> %s", path);
	_system("echo \"Key2Type=0\" >> %s", path);
	_system("echo \"Key3Type=0\" >> %s", path);
	_system("echo \"Key4Type=0\" >> %s", path);

	sprintf(tmp, "wlan%d_vap1_enable", ath_count);
	if (strtol(nvram_safe_get(tmp), NULL, 10)) {
		sprintf(default_type, "wlan%d_vap1_security", ath_count);
		wlan_wep_type = nvram_safe_get(default_type);
		sprintf(wep_default_key, "wlan%d_vap1_wep_default_key", ath_count);
		wlan_wep_default_key = nvram_safe_get(wep_default_key);
		sprintf(default_key, "%s;%s", default_key, wlan_wep_default_key);
	}
	_system("%s\" >> %s", default_key, path);
}

/*set radius*/
static void set_radius(int bssidnum, int ath_count, const char *path)
{
	char *radius0_server, *radius0_ip = "", *radius0_port = "", *radius0_secret = "";
	char *radius1_server, *radius1_ip = "", *radius1_port = "", *radius1_secret = "";
	char *radius2_server, *radius2_ip = "", *radius2_port = "", *radius2_secret = "";
	char *radius3_server, *radius3_ip = "", *radius3_port = "", *radius3_secret = "";
	char *en;
	char tmp[64], tmp1[128], tmp2[128], tmp3[128], tmp4[128],
		      ra0_ip[128], ra1_ip[128], // ip
		      ra0_port[128], ra1_port[128], // port
		      ra0_secret[128], ra1_secret[128]; //key

	_system("echo \"IEEE8021X=%s\" >> %s", (bssidnum > 1) ? "0;0" : "0", path);

/* fix me : coding style */
	/* radius server 1 */
	sprintf(tmp, "wlan%d_eap_radius_server_0", ath_count);
	sprintf(tmp1, "%s", nvram_safe_get(tmp));
	radius0_server = tmp1;
	if(strlen(radius0_server) > 0 && strncmp(radius0_server, "0.0.0.0", 7)) {
		radius0_ip = strtok(radius0_server, "/");
		radius0_port = strtok(NULL, "/");
		radius0_secret = strtok(NULL, "/");
	}

	/* radius server 2 */
	sprintf(tmp, "wlan%d_eap_radius_server_1", ath_count);  
	sprintf(tmp2, "%s", nvram_safe_get(tmp));
	radius1_server = tmp2;
	if(strlen(radius1_server) > 0 && strncmp(radius1_server, "0.0.0.0", 7)) {
		radius1_ip = strtok(radius1_server, "/");
		radius1_port = strtok(NULL, "/");
		radius1_secret = strtok(NULL, "/");
	}

	/* guest zone radius server 1 */
	sprintf(tmp, "wlan%d_vap1_eap_radius_server_0", ath_count);  
	sprintf(tmp3, "%s", nvram_safe_get(tmp));
	radius2_server = tmp3;
	if(strlen(radius2_server) > 0 && strncmp(radius2_server, "0.0.0.0", 7)) {
		radius2_ip = strtok(radius2_server, "/");
		radius2_port = strtok(NULL, "/");
		radius2_secret = strtok(NULL, "/");
	}

	/* guest zone radius server 2 */
	sprintf(tmp, "wlan%d_vap1_eap_radius_server_1", ath_count);  
	sprintf(tmp4, "%s", nvram_safe_get(tmp));
	radius3_server = tmp4;
	if (strlen(radius3_server) > 0 && strncmp(radius3_server, "0.0.0.0", 7)) {
		radius3_ip = strtok(radius3_server, "/");
		radius3_port = strtok(NULL, "/");
		radius3_secret = strtok(NULL, "/");
	}

	/*set config*/
	sprintf(ra0_ip, "echo \"RADIUS_Server=");
	sprintf(ra0_port, "echo \"RADIUS_Port=");
	sprintf(ra0_secret, "echo \"RADIUS_Key=");
	sprintf(ra1_ip, "echo \"RADIUS_Server=");
	sprintf(ra1_port, "echo \"RADIUS_Port=");
	sprintf(ra1_secret, "echo \"RADIUS_Key=");

	sprintf(ra0_ip, "%s%s", ra0_ip, 
		(strcmp(radius0_ip, "") == 0) ? "0.0.0.0" : radius0_ip);
	sprintf(ra0_port, "%s%s", ra0_port, 
		(strcmp(radius0_port, "") == 0) ? "0" : radius0_port);
	sprintf(ra0_secret, "%s%s", ra0_secret, 
		(strcmp(radius0_secret, "") == 0) ? "0" : radius0_secret);
	sprintf(ra1_ip, "%s%s", ra1_ip, 
		(strcmp(radius1_ip, "") == 0) ? "0.0.0.0" : radius1_ip);
	sprintf(ra1_port, "%s%s", ra1_port, 
		(strcmp(radius1_port, "") == 0) ? "0" : radius1_port);
	sprintf(ra1_secret, "%s%s", ra1_secret, 
		(strcmp(radius1_secret, "") == 0) ? "0" : radius1_secret);

	sprintf(tmp, "wlan%d_vap1_enable", ath_count);
	en = nvram_safe_get(tmp);
	if (!strcmp(en, "1")) {
		sprintf(ra0_ip, "%s;%s", ra0_ip, 
			(strcmp(radius2_ip, "") == 0) ? "0.0.0.0" : radius2_ip);
		sprintf(ra0_port, "%s;%s", ra0_port, 
			(strcmp(radius2_port, "") == 0) ? "0" : radius2_port);
		sprintf(ra0_secret, "%s;%s", ra0_secret,
			(strcmp(radius2_secret, "") == 0) ? "0" : radius2_secret);
		sprintf(ra1_ip, "%s;%s", ra1_ip, 
			(strcmp(radius3_ip, "") == 0) ? "0.0.0.0" : radius3_ip);
		sprintf(ra1_port, "%s;%s", ra1_port, 
			(strcmp(radius3_port, "") == 0) ? "0" : radius3_port);
		sprintf(ra1_secret, "%s;%s", ra1_secret, 
			(strcmp(radius3_secret, "") == 0) ? "0" : radius3_secret);
	}

	sprintf(ra0_ip, "%s\" >> %s", ra0_ip, path);
	sprintf(ra0_port, "%s\" >> %s", ra0_port, path);
	sprintf(ra0_secret, "%s\" >> %s", ra0_secret, path);
	sprintf(ra1_ip, "%s\" >> %s", ra1_ip, path);
	sprintf(ra1_port, "%s\" >> %s", ra1_port, path);
	sprintf(ra1_secret, "%s\" >> %s", ra1_secret, path);

	if (strcmp(radius0_ip, "") != 0   || \
	    strcmp(radius0_port, "") != 0 || \
	    strcmp(radius0_secret, "") != 0 || \
	    strcmp(radius2_ip, "") != 0   || \
	    strcmp(radius2_port, "") != 0 || \
	    strcmp(radius2_secret, "") != 0 ) {
		_system(ra0_ip);
		_system(ra0_port);
		_system(ra0_secret);
	}

	if (strcmp(radius1_ip, "") != 0   || \
	    strcmp(radius1_port, "") != 0 || \
	    strcmp(radius1_secret, "") != 0 || \
	    strcmp(radius3_ip, "") != 0   || \
	    strcmp(radius3_port, "") != 0 || \
	    strcmp(radius3_secret, "") != 0 ) {
		_system(ra1_ip);
		_system(ra1_port);
		_system(ra1_secret);
	}

	_system("echo \"own_ip_addr=%s\" >> %s",
		nvram_safe_get("lan_ipaddr"), path);
	_system("echo \"EAPifname=br0\" >> %s", path);
	_system("echo \"PreAuthifname=br0\" >> %s", path);

}

static void set_security(int bss_index, int ath_count, const char *path)
{
	char *wlan_security, *cipher_type, *wlan_psk_pass_phrase, tmp[64];
	int wpa_flag = 0;
	char auth[1024] = {'\0'}, encryp[1024] = {'\0'};
    
	sprintf(tmp, "wlan%d_security", ath_count);
 	wlan_security = nvram_safe_get(tmp);	                
	sprintf(tmp, "wlan%d_psk_pass_phrase", ath_count);
	wlan_psk_pass_phrase = nvram_safe_get(tmp);	                

	/* set AuthMode EncrypType WPAPSK DefaultKeyID */
	if ((strcmp(wlan_security, "disable") == 0)) {
		sprintf(auth, "AuthMode=OPEN");
		sprintf(encryp, "EncrypType=NONE");
		_system("echo \"WPAPSK1=\" >> %s", path);
	} else if ((strcmp(wlan_security, "wep_open_64") == 0)) {
		sprintf(auth, "AuthMode=OPEN");
		sprintf(encryp, "EncrypType=WEP");
		set_wep_key(64, 0, ath_count, path);
	} else if ((strcmp(wlan_security, "wep_share_64") == 0)) {
		sprintf(auth, "%s", "AuthMode=SHARED");
		sprintf(encryp, "%s", "EncrypType=WEP");
		set_wep_key(64, 0, ath_count, path);
	}  else if ((strcmp(wlan_security, "wep_open_128") == 0)) {
		sprintf(auth, "%s", "AuthMode=OPEN");
		sprintf(encryp, "%s", "EncrypType=WEP");
		set_wep_key(128, 0, ath_count, path);
	}  else if ((strcmp(wlan_security, "wep_share_128") == 0)) {
		sprintf(auth, "%s", "AuthMode=SHARED");
		sprintf(encryp, "%s", "EncrypType=WEP");
		set_wep_key(128, 0, ath_count, path);
	}  else if ((strcmp(wlan_security, "wpa_psk") == 0)) {
		sprintf(auth, "%s", "AuthMode=WPAPSK");
		_system("echo \"WPAPSK1=%s\" >> %s", wlan_psk_pass_phrase, path);
		wpa_flag = 1;
	}  else if ((strcmp(wlan_security, "wpa2_psk") == 0)) {
		sprintf(auth, "%s", "AuthMode=WPA2PSK");
		_system("echo \"WPAPSK1=%s\" >> %s", wlan_psk_pass_phrase, path);
		wpa_flag = 1;
	}  else if ((strcmp(wlan_security, "wpa2auto_psk") == 0)) {
		sprintf(auth, "%s", "AuthMode=WPAPSKWPA2PSK");
		_system("echo \"WPAPSK1=%s\" >> %s", wlan_psk_pass_phrase, path);
		wpa_flag = 1;
	}  else if ((strcmp(wlan_security, "wpa_eap") == 0)) {
		sprintf(auth, "%s", "AuthMode=WPA");
		wpa_flag = 2;
	}  else if ((strcmp(wlan_security, "wpa2_eap") == 0)) {
		sprintf(auth, "%s", "AuthMode=WPA2");
		wpa_flag = 2;
	}  else if ((strcmp(wlan_security, "wpa2auto_eap") == 0)) {
		sprintf(auth, "%s", "AuthMode=WPA1WPA2");
		wpa_flag = 2;
	}

	set_wep_default_key(ath_count, path);

	/* cipher type*/
	if(wpa_flag) {
		sprintf(tmp, "wlan%d_psk_cipher_type", ath_count);
		cipher_type = nvram_safe_get(tmp);
		if(strcmp(cipher_type, "tkip") == 0)
			sprintf(encryp, "%s", "EncrypType=TKIP");
		else if(strcmp(cipher_type, "aes")== 0)
			sprintf(encryp, "%s", "EncrypType=AES");
		else	 
			sprintf(encryp, "%s", "EncrypType=TKIPAES");
	}
	
	sprintf(tmp, "wlan%d_vap1_enable", ath_count);

	if (strtol(nvram_safe_get(tmp), NULL, 10)) {
		set_multiple_bssid(auth, encryp, sizeof(auth), sizeof(encryp), ath_count, path);
	}

	_system("echo \"%s\" >> %s", auth, path);
	_system("echo \"%s\" >> %s", encryp, path);
	_system("echo \"PreAuth=%s\" >> %s", (bss_index > 1) ? "0;0" : "0", path);

	/* set Rekey */
	set_Rekey(bss_index, ath_count, path);

	_system("echo \"PMKCachePeriod=10\" >> %s", path);

	/* start WPA/WPA2 Enterprise */
	if(nvram_match("wps_enable", "0") == 0) {
		set_radius(bss_index, ath_count, path);
	}
}


/* WDS */
static void set_wds(int ath_count, const char *path)
{
	char *wlan_wds_enable, tmp[64];

	sprintf(tmp, "wlan%d_wds_enable", ath_count);
	wlan_wds_enable = nvram_safe_get(tmp);    

	if(strcmp(wlan_wds_enable, "0") == 0){
		_system("echo \"WdsEnable=0\" >> %s", path);
	} else if(strcmp(wlan_wds_enable, "1") == 0) {
		_system("echo \"WdsEnable=1\" >> %s", path);
	} else {
		DEBUG_MSG("WDS value error. Default : disable\n");
		_system("echo \"WdsEnable=0\" >> %s", path);
	}

	_system("echo \"WdsEncrypType=NONE\" >> %s", path);
	_system("echo \"WdsList=\" >> %s", path);
	_system("echo \"Wds0Key=\" >> %s", path);
	_system("echo \"Wds1Key=\" >> %s", path);
	_system("echo \"Wds2Key=\" >> %s", path);
	_system("echo \"Wds3Key=\" >> %s", path);
	_system("echo \"WdsDefaultKeyID=1\" >> %s", path);

}

static void set_BW(int ath_count, const char *path)
{
	char *wlan_dot11_mode, *wlan_channel;
	char tmp[64];

	sprintf(tmp, "wlan%d_dot11_mode", ath_count);
	wlan_dot11_mode = nvram_safe_get(tmp);
	sprintf(tmp, "wlan%d_channel", ath_count);
	wlan_channel = nvram_safe_get(tmp);

	if(ath_count == 0) {//2.4G
		/* HT_BW  HT_EXTCHA */
		if( strcmp(wlan_dot11_mode, "11bgn")==0 || 
			strcmp(wlan_dot11_mode, "11gn")==0 ||
		    strcmp(wlan_dot11_mode, "11n")==0 ) {
			sprintf(tmp, "wlan%d_11n_protection", ath_count);
	        if(nvram_match(tmp, "auto") != 0)
	        	_system("echo \"HT_BW=0\" >> %s", path);
	        else if(atoi(wlan_channel) <= 4) {
	        	_system("echo \"HT_BW=1\" >> %s", path);
	        	_system("echo \"HT_EXTCHA=1\" >> %s", path);
	    	} else if(atoi(wlan_channel) >= 8) {
	        	_system("echo \"HT_BW=1\" >> %s", path);
	        	_system("echo \"HT_EXTCHA=0\" >> %s", path);
			} else { 
	        	_system("echo \"HT_BW=1\" >> %s", path);
			}
		}
	} else {
		if( strcmp(wlan_dot11_mode, "11na")==0 ||
			strcmp(wlan_dot11_mode, "11n")==0 ) {
			sprintf(tmp, "wlan%d_11n_protection", ath_count);
	        if(nvram_match(tmp, "auto") != 0)
	        	_system("echo \"HT_BW=0\" >> %s", path);
	        else if(atoi(wlan_channel) == 36) {
	        	_system("echo \"HT_BW=1\" >> %s", path);
	        	_system("echo \"HT_EXTCHA=1\" >> %s", path);
	    	} else if(atoi(wlan_channel) == 165) {
	        	_system("echo \"HT_BW=1\" >> %s", path);
	        	_system("echo \"HT_EXTCHA=0\" >> %s", path);
			} else { 
	        	_system("echo \"HT_BW=1\" >> %s", path);
			}
		}
	}

}

static void set_GI(int ath_count, const char *path)
{
	char tmp[64];
	
	/* HT_GI */
	sprintf(tmp, "wlan%d_short_gi", ath_count);
	if(nvram_match(tmp, "1") == 0)
		_system("echo \"HT_GI=1\" >> %s", path);
	else
		_system("echo \"HT_GI=0\" >> %s", path);

}

static int set_bssid(int iface, const char *path)
{
	int bssid = 1;
	char tmp[32];

	sprintf(tmp, "wlan%d_vap1_enable", iface);

	if (strtol(nvram_safe_get(tmp), NULL, 10)) {
		_system("echo \"BssidNum=2\" >> %s", path);
		bssid += 1;
	} else
		_system("echo \"BssidNum=1\" >> %s", path);

	return bssid;
}

static void set_basicrate(const char *path)
{
	_system("echo \"BasicRate=15\" >> %s", path);
}

static void set_noforwarding(int iface, const char *path)
{
	char tmp[64];

	_system("echo \"NoForwarding=0\" >> %s", path);

	sprintf(tmp, "wlan%d_vap_guest_zone", iface);

	if (strtol(nvram_safe_get(tmp), NULL, 10))
		_system("echo \"NoForwardingBTNBSSID=1\" >> %s", path);			
	else
		_system("echo \"NoForwardingBTNBSSID=0\" >> %s", path);
}

/* set HT Paramaters */
static void set_HT(int iface, const char *path)
{
	/* HT_HTC */
	_system("echo \"HT_HTC=1\" >> %s", path);
	/* HT_RDG */
	_system("echo \"HT_RDG=1\" >> %s", path);
	/* HT_LinkAdapt */
	_system("echo \"HT_LinkAdapt=1\" >> %s", path);
	/* HT_BW */
	set_BW(iface, path);
	/* HT_OpMode */
	_system("echo \"HT_OpMode=0\" >> %s", path);
	/* HT_MpduDensity */
	_system("echo \"HT_MpduDensity=0\" >> %s", path);
	/* HT_AutoBA */
	_system("echo \"HT_AutoBA=1\" >> %s", path);
	/* HT_AMSDU */
	_system("echo \"HT_AMSDU=0\" >> %s", path);
	/* HT_GI*/ 
	set_GI(iface, path);
	/* HT_BAWinSize */
	_system("echo \"HT_BAWinSize=64\" >> %s", path);
	/* HT_STBC */
	_system("echo \"HT_STBC=1\" >> %s", path);
	/* HT_MCS */
	set_txrate(iface, path);
	/* HT_BADecline*/
	_system("echo \"HT_BADecline=0\" >> %s", path);
	/* HT_TxStream*/
	_system("echo \"HT_TxStream=3\" >> %s", path);
	/* HT_RxStream*/
	_system("echo \"HT_RxStream=3\" >> %s", path);

	//_system("echo \"HT_PROTECT=1\" >> %s", path);
	//_system("echo \"HT_MIMOPS=3\" >> %s", path);
	//_system("echo \"HT_40MHZ_INTOLERANT=0\" >> %s", path);
	//_system("echo \"HT_DisallowTKIP=0\" >> %s", path);
}

static void set_IEEE802(int iface, const char *path)
{
	_system("echo \"MaxTxPowerLevel=16\" >> %s", path); 
	_system("echo \"IEEE80211H=0\" >> %s", path);
	_system("echo \"CSPeriod=10\" >> %s", path);
	//_system("echo \"RDRegion=\" >> %s", path);
	_system("echo \"CarrierDetect=1\" >> %s", path);
	_system("echo \"ChGeography=2\" >> %s", path);
}

void set_Rekey(int bssid, int iface, const char *path)
{
	char tmp[64];

	sprintf(tmp, "wlan%d_gkey_rekey_time", iface);

	if(nvram_safe_get(tmp)){
		_system("echo \"RekeyInterval=%s\" >> %s", \
			nvram_safe_get(tmp), path);
		_system("echo \"RekeyMethod=%s\" >> %s", \
			(bssid > 1) ? "TIME;TIME" : "TIME", path);
	} else {	
		_system("echo \"RekeyInterval=0\" >> %s", path);
		_system("echo \"RekeyMethod=DISABLE\" >> %s", path);
	}	
}

/*
 * @iface : 0 or 1, 0 express 2.4G, 1 express 5G.
 */
int set_rt3883_config(int iface)
{
	DEBUG_MSG("rt3883_config interface=ra0%d_0\n", iface);
	char *path = NULL;
	int bssid = 0;

	path = iface == 0 ? AP1 : AP0;

	/* Default Description must be exited. */
	_system("echo \'#The word of \"Default\" must not be removed\' > %s", path);
	_system("echo \"Default\" >> %s", path);

	/* set Region */
	set_country(path);

	/* set Bssid */
	bssid = set_bssid(iface, path);

	/* set ssid */
	set_ssid(iface, path);
	
	/* set rate */
	set_basicrate(path);

	/* set beacon interval*/
	set_beacon_interval(iface, path);

	/*set dtim */
	set_dtim(iface, path);

	/* set Default config value */
	_system("echo \"TxPower=100\" >> %s", path);
	_system("echo \"DisableOLBC=0\" >> %s", path);
	_system("echo \"BGProtection=0\" >> %s", path);

	/* set Preamble */
	set_preamble(iface, path);

	/* rts */
	set_rts(iface, path);
	
	/* fragment */		
	set_fragment(iface, path);

	/* set Default config value */
	_system("echo \"TxBurst=1\" >> %s", path);
	_system("echo \"PktAggregate=0\" >> %s", path);


	/* set NoForwarding */
	set_noforwarding(iface, path);

	/* hide ssid */
	set_hidessid(iface, path, bssid);

	_system("echo \"ShortSlot=1\" >> %s", path);

	/* (auto) channel */
	set_channel(iface, path);

	/* mode */
	set_mode(iface, path);

	_system("echo \"WiFiTest=0\" >> %s", path);
	_system("echo \"WirelessEvent=0\" >> %s", path);
	_system("echo \"AccessPolicy0=0\" >> %s", path);
	_system("echo \"AccessControlList0=\" >> %s", path);
	_system("echo \"AccessPolicy1=0\" >> %s", path);
	_system("echo \"AccessControlList1=\" >> %s", path);
	_system("echo \"AccessPolicy2=0\" >> %s", path);
	_system("echo \"AccessControlList2=\" >> %s", path);
	_system("echo \"AccessPolicy3=0\" >> %s", path);
	_system("echo \"AccessControlList3=\" >> %s", path);

	/* set HT parameters */
	set_HT(iface, path);

	/* set wireless multimedia parameters */
	set_wmm(bssid, iface, path);

	/* set IEEE802.1h+d */
	set_IEEE802(bssid, path);	

	/* security*/
	set_security(bssid, iface, path);

	//_system("echo \"HSCounter=0\" >> %s", path);
	
	/* wds */
//	set_wds(ath_count, path);

	/* set WPS */
	set_wps_infomation(iface, path);

	_system("echo \"session_timeout_interval=0\" >> %s", path);
	_system("echo \"idle_timeout_interval=0\" >> %s", path);
	
	if(iface == 0){//2.4G		
		_system("echo \"ExtEEPROM=0\" >> %s", path);
		_system("echo \"Bss2040Coexist=0\" >> %s", path);		
		_system("echo \"DBG_LEVEL=0\" >> %s", path);
	} else {//5G		
		_system("echo \"ExtEEPROM=0\" >> %s", path);
		_system("echo \"DBG_LEVEL=0\" >> %s", path);
	}

	return 0;
}

int wconfig_main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage : wconfig interface\n");
		return -1;
	}	

	if (!strcmp(argv[1], "ra01_0"))
		set_rt3883_config(1);
	else
		set_rt3883_config(0);

	return 0;
}
