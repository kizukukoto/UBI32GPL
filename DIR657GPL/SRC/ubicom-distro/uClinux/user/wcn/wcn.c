#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "wcn.h"
#include <sys/signal.h>
#include <sys/types.h>

#if 0
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef AR7161
#define DUAL_RADIO 1
#endif

int tag_parser(char *WFC_SETTING_FILE);
int creat_wfc_file(int mywcn_file_exist, char *WFC_FILE_PATH);
void ch_wfc_name(void);
void wsetting_to_nvram(void);
void ascii2hex(char *);
char *StrDecode(char *to, char *from, int en_nbsp);
static int PasswordHash(char *password, char *output);
char *get_firmware_version(void);

char buffer[XML_LENGTH] = "";
char configId_tmp[XML_LENGTH] = "";
char configId_dev[XML_LENGTH] = "";
//xml tag
char ssid_tmp[XML_LENGTH]="";
char connectionType_tmp[XML_LENGTH]="";
char authentication_tmp[XML_LENGTH]="";
char encryption_tmp[XML_LENGTH]="";
char networkKey_tmp[128]="";
char keyProvideAutomatically_tmp[XML_LENGTH]="";
char ieee802Dot1xEnabled_tmp[XML_LENGTH]="";
char eapMethod_tmp[XML_LENGTH]="";
char channel2Dot4_tmp[XML_LENGTH]="";
char channel5Dot0_tmp[XML_LENGTH]="";
char deviceMode_tmp[XML_LENGTH]="";
char timeToLive_tmp[XML_LENGTH]="";
char configHash_tmp[XML_LENGTH]="";
char keyIndex_tmp[XML_LENGTH]="";
extern char *configId_s, *configId_e;
char *ssid_s,*ssid_e;
char *connectionType_s, *connectionType_e;
char *authentication_s,*authentication_e;
char *encryption_s,*encryption_e;
char *networkKey_s,*networkKey_e;
char *eapMethod_s, *eapMethod_e;
char *channel2Dot4_s, *channel2Dot4_e;
char *channel5Dot0_s, *channel5Dot0_e;
char *deviceMode_s, *deviceMode_e;
char *timeToLive_s, *timeToLive_e;
char *configHash_s, *configHash_e;
char *keyIndex_s, *keyIndex_e;

char *wlan0_ssid = NULL;
char *wlan1_ssid = NULL;
char *wlan0_security = NULL;
char *wlan0_wep_display = NULL;
char *wlan0_psk_cipher_type = NULL;
char *wlan0_psk_pass_phrase = NULL;
char *wlan0_wep_default_key = NULL;
char *wlan0_channel = NULL;
char *wlan1_channel = NULL;
char *wlan0_auto_channel_enable = NULL;
char *wlan1_auto_channel_enable = NULL;
char *wlan0_mode = NULL;
char *wlan0_eap_proto = NULL;
char *wlan0_peap_inner_proto = NULL;
char *wlan1_peap_inner_proto = NULL;

int check_dual_ap;

void do_wcn(int mywcn_file_exist, char *tmp_mountname)
{		
	int i;
	char filesystem[256]={0};
	
	memset(configId_tmp, 0, XML_LENGTH);
	
	memset(filesystem, 0, 256);
	sprintf(filesystem, "%s/smrtntky/wsetting.wfc", tmp_mountname);
	if(tag_parser(filesystem)){
		printf("tag_parser fail\n");	
		for(i=0; i<3; i++){
			wcn_led_unsuccesfully_show();
		}	
		//_system("reboot -d2 &");
	   	kill(read_pid(RC_PID), SIGTSTP);
	    sleep(3);						
	    	    									
		return;
	}	
		
	memset(filesystem, 0, 256);
	sprintf(filesystem, "%s/smrtntky/device/", tmp_mountname);
	if(creat_wfc_file(mywcn_file_exist,filesystem)){
		printf("creat_wfc_file fail\n");		
		for(i=0; i<3; i++){
			wcn_led_unsuccesfully_show();
		}	
		//_system("reboot -d2 &");
	   	kill(read_pid(RC_PID), SIGTSTP);
	   	sleep(3);						
	   										
		return;
	}
		
	wsetting_to_nvram();
	printf("wsetting_to_nvram succesfully\n");	
	/*FIXME:*/
	/*
		I don't know why after we umount /var/usb, the directory /var/usb/smrtntky/device exists...
		Thus.. If that directory exists, remove that directory
	*/

#if 1//configId equal do wcn
	_system("echo 1 > %s",WCN_SET_FILE);		
#endif	
	//wcn_led_succesfully_show();		
	//_system("reboot -d2 &");
	kill(read_pid(RC_PID), SIGTSTP);	
	sleep(3);		
			
	return;
}

char *get_firmware_version(void)
{
	char firmware_version[128] = "";
	char *firmware_version_ptr;
	firmware_version_ptr = firmware_version;
	sprintf(firmware_version,"%s","test");
	return firmware_version_ptr;
}

void wcn_led_succesfully_show(void)
{
	int i;		
	for(i = 0; i < 2; i++){		
#ifdef AR7161
		_system("/sbin/gpio USB_LED on &");//on
		usleep(500000);		
		_system("/sbin/gpio USB_LED off &");//off
		usleep(500000);					
	}	
	_system("/sbin/gpio USB_LED on &");//on
	usleep(500000);		
	_system("/sbin/gpio USB_LED off &");//off	
#else				
		_system("/sbin/gpio USB_LED on &");//on
		usleep(500000);		
		_system("/sbin/gpio USB_LED off &");//off
		usleep(500000);			
	}	
	_system("/sbin/gpio USB_LED on &");//on
	usleep(500000);		
	_system("/sbin/gpio USB_LED off &");//off			
#endif				
}

void wcn_led_unsuccesfully_show(void)
{
	int i;	
	for(i = 0; i < 2; i++){		
#ifdef AR7161
		_system("/sbin/gpio USB_LED on &");//on
		usleep(300000);			
		_system("/sbin/gpio USB_LED off &");//off	
		usleep(300000);		
	}	
	_system("/sbin/gpio USB_LED on &");//on
	sleep(1);	
	_system("/sbin/gpio USB_LED off &");//off
	usleep(300000);
#else			
		_system("/sbin/gpio USB_LED on &");//on
		usleep(300000);			
		_system("/sbin/gpio USB_LED off &");//off	
		usleep(300000);		
	}	
	_system("/sbin/gpio USB_LED on &");//on
	sleep(1);	
	_system("/sbin/gpio USB_LED off &");//off
	usleep(300000);				
#endif					
}

void wsetting_to_nvram(void)
{
	char to[64]={0};		
	char ssid_decoded[64] = "";
	char ssid_final[64] = "";
	char ssid_temp[64] = "";
	char nvram_wepkey_value[256] ={0};
	char wep64_key[24];
	char wep128_key[48];		
	
	DEBUG_MSG("wsetting_to_nvram start\n");	
	memset(to, 0, 64);	
	memset(nvram_wepkey_value, 0, 256);
	DEBUG_MSG("ssid_tmp=%s\n",ssid_tmp);	
	StrDecode(to, ssid_tmp, 0);
	strcpy(ssid_decoded, to);
	DEBUG_MSG("ssid_decoded=%s\n", ssid_decoded);
	
#ifdef DUAL_RADIO
	if(check_dual_ap) 
	{
		if(strlen(ssid_decoded)<31){
			/*	Date:	2009-03-05
			*	Name:	jimmy huang
			*	Reason: strcat will append -2 to the variable ssid_decoded
			*			which means wlan1_ssid is also affected, so we need to
			*			use another memory to store the value for 5G
			*	Note:	modified the codes below
			*/
			/*
			wlan1_ssid = ssid_decoded;//5.0g
			*/
			strcpy(ssid_final,ssid_decoded);
			wlan1_ssid = ssid_final;//5.0g
			wlan0_ssid = strcat(ssid_decoded, "-2");//2.4g
			DEBUG_MSG("SSID for Dual AP 5.0g = %s\n", wlan1_ssid);
			DEBUG_MSG("Attach -2 to SSID for Dual AP 2.4g = %s\n", wlan0_ssid);
		}	
		else
		{
			wlan1_ssid = ssid_decoded;//5.0g
			strncpy(ssid_final, ssid_decoded, 30);
			wlan0_ssid = strcat(ssid_final, "-2");//2.4g
			DEBUG_MSG("SSID for Dual AP 5.0g = %s\n", wlan1_ssid);
			DEBUG_MSG("Length too long, Attach -2 to SSID for Dual AP 2.4g = %s\n", wlan0_ssid);
		}			
	}
	else
		wlan0_ssid = ssid_decoded;
					
	//SSID
	if(wlan0_ssid != NULL)
	{
		DEBUG_MSG("wlan0_ssid = %s\n", wlan0_ssid);		
		nvram_set("wlan0_ssid", wlan0_ssid);
	}	
	if(wlan1_ssid != NULL)
	{
		DEBUG_MSG("wlan1_ssid = %s\n", wlan1_ssid);		
		nvram_set("wlan1_ssid", wlan1_ssid);
	}	
	//CHANEL
	if(wlan0_channel != NULL)
	{
		DEBUG_MSG("wlan0_channel = %s\n", wlan0_channel);
		nvram_set("wlan0_channel", wlan0_channel);		
	}	
	if(wlan1_channel != NULL)
	{
		DEBUG_MSG("wlan1_channel = %s\n", wlan1_channel);		
		nvram_set("wlan1_channel", wlan1_channel);
	}		
	if(wlan0_auto_channel_enable != NULL)
	{	
		DEBUG_MSG("wlan0_auto_channel_enable = %s\n", wlan0_auto_channel_enable);
		nvram_set("wlan0_auto_channel_enable", wlan0_auto_channel_enable);		
	}	
	if(wlan1_auto_channel_enable != NULL)
	{	
		DEBUG_MSG("wlan1_auto_channel_enable = %s\n", wlan1_auto_channel_enable);		
		nvram_set("wlan1_auto_channel_enable", wlan1_auto_channel_enable);
	}	
	
	if(wlan0_mode != NULL)	
	{
		DEBUG_MSG("wlan0_mode = %s\n", wlan0_mode);
		nvram_set("wlan0_mode", wlan0_mode);
	}
	
	//SECURITY
	if(wlan0_security != NULL)
	{
		DEBUG_MSG("wlan0_security = %s\n", wlan0_security);
		nvram_set("wlan0_security", wlan0_security);
		nvram_set("wlan1_security", wlan0_security);
	}	
	//WPA
	if(wlan0_psk_cipher_type != NULL)	
	{
		DEBUG_MSG("wlan0_psk_cipher_type = %s\n", wlan0_psk_cipher_type);
		nvram_set("wlan0_psk_cipher_type", wlan0_psk_cipher_type);
		nvram_set("wlan1_psk_cipher_type", wlan0_psk_cipher_type);
	}		
	if(wlan0_eap_proto != NULL){		
		DEBUG_MSG("wlan0_eap_proto = %s\n", wlan0_eap_proto);
		nvram_set("wlan0_eap_proto", wlan0_eap_proto);
		nvram_set("wlan1_eap_proto", wlan0_eap_proto);
	}	
	if(wlan0_peap_inner_proto != NULL){
		DEBUG_MSG("wlan0_peap_inner_proto = %s\n", wlan0_peap_inner_proto);
		nvram_set("wlan0_peap_inner_proto", wlan0_peap_inner_proto);
		nvram_set("wlan1_peap_inner_proto", wlan1_peap_inner_proto);
	}	
	if(wlan0_psk_pass_phrase != NULL){
		DEBUG_MSG("wlan0_psk_pass_phrase = %s\n", wlan0_psk_pass_phrase);
		nvram_set("wlan0_psk_pass_phrase", wlan0_psk_pass_phrase);
		nvram_set("wlan1_psk_pass_phrase", wlan0_psk_pass_phrase);
	}	
	//WEP
	if(wlan0_wep_display != NULL){	
		DEBUG_MSG("wlan0_wep_display = %s\n", wlan0_wep_display);
		nvram_set("wlan0_wep_display", wlan0_wep_display);
		nvram_set("wlan1_wep_display", wlan0_wep_display);
	}				
	if(wlan0_wep_default_key != NULL){
		DEBUG_MSG("wlan0_wep_default_key = %s\n", wlan0_wep_default_key);
		nvram_set("wlan0_wep_default_key", wlan0_wep_default_key);	
	}	
	if(strcmp(wlan0_security, "wep_open_64") == 0 || strcmp(wlan0_security, "wep_share_64") == 0)
	{
		if(strcmp(wlan0_wep_display, "ascii") == 0){
			ascii2hex(nvram_wepkey_value);
			sprintf(wep64_key, "wlan0_wep64_key_%s", wlan0_wep_default_key);
			nvram_set(wep64_key, nvram_wepkey_value);
			sprintf(wep64_key, "wlan1_wep64_key_%s", wlan0_wep_default_key);
			nvram_set(wep64_key, nvram_wepkey_value);
		}else{
			sprintf(wep64_key, "wlan0_wep64_key_%s", wlan0_wep_default_key);
			nvram_set(wep64_key, networkKey_tmp);
			sprintf(wep64_key, "wlan1_wep64_key_%s", wlan0_wep_default_key);
			nvram_set(wep64_key, networkKey_tmp);
		}
	}
	else if(strcmp(wlan0_security, "wep_open_128") == 0 || strcmp(wlan0_security, "wep_share_128") == 0)
	{	
		if(strcmp(wlan0_wep_display, "ascii") == 0){				
			ascii2hex(nvram_wepkey_value);
			sprintf(wep128_key, "wlan0_wep128_key_%s", wlan0_wep_default_key);
			nvram_set(wep128_key, nvram_wepkey_value);
			sprintf(wep128_key, "wlan1_wep128_key_%s", wlan0_wep_default_key);
			nvram_set(wep128_key, nvram_wepkey_value);
		}else{	
			sprintf(wep128_key, "wlan0_wep128_key_%s", wlan0_wep_default_key);
			nvram_set(wep128_key, networkKey_tmp);
			sprintf(wep128_key, "wlan1_wep128_key_%s", wlan0_wep_default_key);
			nvram_set(wep128_key, networkKey_tmp);
		}
	}
#else//DUAL_RADIO
	
	wlan0_ssid = ssid_decoded;
					
	//SSID
	if(wlan0_ssid != NULL)
	{
		DEBUG_MSG("wlan0_ssid = %s\n", wlan0_ssid);		
		nvram_set("wlan0_ssid", wlan0_ssid);
	}		
	//CHANEL		
	if(wlan0_channel != NULL)
	{
		DEBUG_MSG("wlan0_channel = %s\n", wlan0_channel);
		nvram_set("wlan0_channel", wlan0_channel);
	}		
	if(wlan0_auto_channel_enable != NULL)
	{	
		DEBUG_MSG("wlan0_auto_channel_enable = %s\n", wlan0_auto_channel_enable);
		nvram_set("wlan0_auto_channel_enable", wlan0_auto_channel_enable);
	}	
	
	if(wlan0_mode != NULL)	
	{
		DEBUG_MSG("wlan0_mode = %s\n", wlan0_mode);
		nvram_set("wlan0_mode", wlan0_mode);
	}
	
	//SECURITY
	if(wlan0_security != NULL)
	{
		DEBUG_MSG("wlan0_security = %s\n", wlan0_security);
		nvram_set("wlan0_security", wlan0_security);
	}
	//WPA
	if(wlan0_psk_cipher_type != NULL)	
	{
		DEBUG_MSG("wlan0_psk_cipher_type = %s\n", wlan0_psk_cipher_type);
		nvram_set("wlan0_psk_cipher_type", wlan0_psk_cipher_type);
	}			
	if(wlan0_eap_proto != NULL){	
		DEBUG_MSG("wlan0_eap_proto = %s\n", wlan0_eap_proto);	
		nvram_set("wlan0_eap_proto", wlan0_eap_proto);
	}	
	if(wlan0_peap_inner_proto != NULL){
		DEBUG_MSG("wlan0_peap_inner_proto = %s\n", wlan0_peap_inner_proto);
		nvram_set("wlan0_peap_inner_proto", wlan0_peap_inner_proto);
	}		
	if(wlan0_psk_pass_phrase != NULL){
		DEBUG_MSG("wlan0_psk_pass_phrase = %s\n", wlan0_psk_pass_phrase);
		nvram_set("wlan0_psk_pass_phrase", wlan0_psk_pass_phrase);
	}	
	//WEP
	if(wlan0_wep_display != NULL){	
		DEBUG_MSG("wlan0_wep_display = %s\n", wlan0_wep_display);
		nvram_set("wlan0_wep_display", wlan0_wep_display);	
	}					
	if(wlan0_wep_default_key != NULL){
		DEBUG_MSG("wlan0_wep_default_key = %s\n", wlan0_wep_default_key);
		nvram_set("wlan0_wep_default_key", wlan0_wep_default_key);
	}	
	if(strcmp(wlan0_security, "wep_open_64") == 0 || strcmp(wlan0_security, "wep_share_64") == 0)
	{
		if(strcmp(wlan0_wep_display, "ascii") == 0){
			ascii2hex(nvram_wepkey_value);
			sprintf(wep64_key, "wlan0_wep64_key_%s", wlan0_wep_default_key);
			DEBUG_MSG("%s = %s\n", wep64_key,nvram_wepkey_value);
			nvram_set(wep64_key, nvram_wepkey_value);
		}else{
			sprintf(wep64_key, "wlan0_wep64_key_%s", wlan0_wep_default_key);
			DEBUG_MSG("%s = %s\n", wep64_key,networkKey_tmp);
			nvram_set(wep64_key, networkKey_tmp);
		}
	}
	else if(strcmp(wlan0_security, "wep_open_128") == 0 || strcmp(wlan0_security, "wep_share_128") == 0)
	{	
		if(strcmp(wlan0_wep_display, "ascii") == 0){				
			ascii2hex(nvram_wepkey_value);
			sprintf(wep128_key, "wlan0_wep128_key_%s", wlan0_wep_default_key);
			DEBUG_MSG("%s = %s\n", wep128_key,nvram_wepkey_value);
			nvram_set(wep128_key, nvram_wepkey_value);
		}else{	
			sprintf(wep128_key, "wlan0_wep128_key_%s", wlan0_wep_default_key);
			DEBUG_MSG("%s = %s\n", wep128_key,networkKey_tmp);
			nvram_set(wep128_key, networkKey_tmp);
		}
	}
#endif

	nvram_set("wps_enable", "1");
	nvram_set("wps_configured_mode", "5");
	/*	Date: 2009-02-04
	*	Name: jimmy huang
	*	Reason: When using wcn usb, if we do not set wps_lock = 1
	*			wps can't aceept client's wireless connection request
	*			(client's wifi UI will say wireless profile setting
	*			is not match with our router)
	*			
	*			wps_ath/wsc is modified earlier to meet dtm 1.3,
	*			and the modification does do something about wps_lock
	*			which is not actually implemented inside wsc 
	*	Note:	Add new codes below
	*			Marked at 2009-03-16 by jimmy huang
	*				due to previous mis-modified codes
	*				for dtm-1.3, currently with wps_ath,
	*				we don't really implement the function of
	*				wps_lock which means we should not use that
	*				key to achieve some function untill we do actually
	*				implement the wps_lock function in wps_ath.
	*/
	//nvram_set("wps_lock","1");
	
	nvram_commit();
}

void ascii2hex(char *nvram_wepkey_value)
{
	int i;

	memset(nvram_wepkey_value, 0, 256);
	for(i =0 ; i < strlen(networkKey_tmp); i++)
		sprintf(nvram_wepkey_value, "%s%x", nvram_wepkey_value,*(networkKey_tmp + i));
}

int creat_wfc_file(int mywcn_file_exist, char *WFC_FILE_PATH)
{
	FILE *fp;	
	char p_cur_configId[512] = "";	
	char p_cur_manufacturer[128] = "";
	char p_cur_modelName[128] = "";
	char p_cur_serialNum[128] = "";
	char p_cur_firmware[512] = "";
	char p_cur_device[128] = "";		
	char cur_buffer[XML_LENGTH] = "";
	char filename_wcn[XML_LENGTH] = "";
	char filename_wcn_tmp[XML_LENGTH] = "";
	char filename_raw[XML_LENGTH] = "";	
	int i;
	
	get_mywcn_filename(cur_buffer);
	strcpy(filename_raw, cur_buffer);
	DEBUG_MSG("creat_wfc_file: mywcn_file raw is %s\n", filename_raw);							
	strncpy(filename_wcn_tmp, filename_raw, 8);
	sprintf(filename_wcn, "%s%s.wfc",WFC_FILE_PATH , filename_wcn_tmp);							
	DEBUG_MSG("creat_wfc_file:  mywcn_file is %s\n", filename_wcn);	
	
	if(mywcn_file_exist){
		DEBUG_MSG("creat_wfc_file: mywcn_file is exist\n");			
		_system("rm -f %s",filename_wcn);
	}	
	else{
		DEBUG_MSG("creat_wfc_file: mywcn_file isn't exist\n");	
	}		
	
	fp = fopen(filename_wcn, "w+");		
	if(fp == NULL){
		DEBUG_MSG("creat_wfc_file : filename_wcn %s file open r+ failed\n",filename_wcn);			
		return 1;
	}
		
	fputs("<?xml version=\"1.0\"?>\n",fp);
	fputs("<device xmlns=\"http://www.microsoft.com/provisioning/DeviceProfile/2004\">\n",fp);
	strcpy(p_cur_configId, "<configId>");
	strcat(p_cur_configId, configId_tmp);
	strcat(p_cur_configId, "</configId>\n");
	fputs(p_cur_configId,fp);
	DEBUG_MSG("p_cur_configId %s", p_cur_configId);
	strcpy(p_cur_manufacturer, "<manufacturer>");
	strcat(p_cur_manufacturer, nvram_safe_get("manufacturer"));
	strcat(p_cur_manufacturer,"</manufacturer>\n");
	fputs(p_cur_manufacturer,fp);
	DEBUG_MSG("p_cur_manufacturer %s", p_cur_manufacturer);
	strcpy(p_cur_modelName,"<modelName>");
	strcat(p_cur_modelName, nvram_safe_get("model_number"));
	strcat(p_cur_modelName,"</modelName>\n");
	fputs(p_cur_modelName,fp);
	DEBUG_MSG("p_cur_modelName %s", p_cur_modelName);
	strcpy(p_cur_serialNum,"<serialNumber>");
	strcat(p_cur_serialNum, nvram_safe_get("model_url"));
	strcat(p_cur_serialNum,"</serialNumber>\n");
	fputs(p_cur_serialNum,fp);
	DEBUG_MSG("p_cur_serialNum %s", p_cur_serialNum);
	strcpy(p_cur_firmware,"<firmwareVersion>");
	strcat(p_cur_firmware,get_firmware_version());
	strcat(p_cur_firmware,"</firmwareVersion>\n");
	fputs(p_cur_firmware,fp);
	DEBUG_MSG("p_cur_firmware %s", p_cur_firmware);
	strcpy(p_cur_device,"<deviceType>");
	strcat(p_cur_device, nvram_safe_get("pure_model_description"));
	strcat(p_cur_device,"</deviceType>\n");
	fputs(p_cur_device,fp);
	DEBUG_MSG("p_cur_device %s", p_cur_device);
	fputs("</device>\n",fp);
	
	fclose(fp);
	
#ifdef DUAL_RADIO	//23456789 => 9->A => F->0
	
	for(i=7; i>0; i--)
	{
		if( filename_raw[i]==0x47 || filename_raw[i]==0x67 )
		{
			filename_raw[i]=0x30;
			filename_raw[i-1]++;
		}
		else if(filename_raw[i]==0x39) //CHAR 9
		{
			filename_raw[i]=0x61; //CHAR a
		}
		else if( filename_raw[i]==0x46 || filename_raw[i]==0x66 ) // CHAR F or CHAR f
		{
			filename_raw[i]=0x30; //CHAR 0
			if(filename_raw[i-1]==0x39) //CHAR 9
			{
				filename_raw[i-1]=0x61; //CHAR a
			}
			else if(filename_raw[i-1]==0x46 || filename_raw[i-1]==0x66) // CHAR F or CHAR f
				filename_raw[i-1]++ ;
		}
		else
		{
			filename_raw[i]++ ;
			break;	
		}	
	}	
	
	DEBUG_MSG("creat_wfc_file: mywcn_file raw is %s\n", filename_raw);							
	_system("cp -f %s %s%s.wfc",filename_wcn,WFC_FILE_PATH,filename_raw);	
	DEBUG_MSG("creat_wfc_file: filename_wcn is %s\n", filename_wcn);
#endif
	
	return 0;
}

int tag_parser(char *WFC_SETTING_FILE)
{
	FILE *fp;
	memset(configId_tmp, 0, XML_LENGTH);
	memset(ssid_tmp, 0, XML_LENGTH);
	memset(connectionType_tmp, 0, XML_LENGTH);
	memset(authentication_tmp, 0, XML_LENGTH);
	memset(encryption_tmp, 0, XML_LENGTH);
	memset(networkKey_tmp, 0, 128);
	memset(keyProvideAutomatically_tmp, 0, XML_LENGTH);
	memset(ieee802Dot1xEnabled_tmp, 0, XML_LENGTH);
	memset(eapMethod_tmp, 0, XML_LENGTH);
	memset(channel2Dot4_tmp, 0, XML_LENGTH);
	memset(channel5Dot0_tmp, 0, XML_LENGTH);
	memset(deviceMode_tmp, 0, XML_LENGTH);
	memset(timeToLive_tmp, 0, XML_LENGTH);
	memset(configHash_tmp, 0, XML_LENGTH);
	memset(keyIndex_tmp, 0, XML_LENGTH);
	
	check_dual_ap=0;
	memset(buffer, 0, XML_LENGTH);
	
	if((fp = fopen(WFC_SETTING_FILE,"r+")) == NULL)
	{
		DEBUG_MSG("tag_parser : fail to open %s\n", WFC_SETTING_FILE);
		return 1;
	}
	else
	{
		DEBUG_MSG("tag_parser : to open %s\n", WFC_SETTING_FILE);
		while(fgets(buffer,XML_LENGTH,fp))
		{
			if((configId_s = (strstr(buffer,"configId"))) != NULL)
			{
				configId_s = strstr(buffer,">") + 1;
				configId_e = strstr(configId_s, "<");
				strncpy(configId_tmp, configId_s, configId_e - configId_s);
				DEBUG_MSG("tag_parser : configId_tmp %s\n", configId_tmp);			
			}
			
			if((ssid_s = (strstr(buffer,"ssid"))) != NULL)
			{
				ssid_s = strstr(buffer,">") + 1;
				ssid_e = strstr(ssid_s,"<");
				strncpy(ssid_tmp,ssid_s,ssid_e - ssid_s);
				DEBUG_MSG("tag_parser : ssid_tmp %s\n", ssid_tmp);			
			}
			
			if((connectionType_s = (strstr(buffer,"connectionType"))) != NULL)
			{
				connectionType_s = strstr(buffer,">") + 1;
				connectionType_e = strstr(connectionType_s,"<");
				strncpy(connectionType_tmp,connectionType_s,connectionType_e - connectionType_s);
				DEBUG_MSG("tag_parser : connectionType_tmp %s\n", connectionType_tmp);			
			}
			
			if((authentication_s = (strstr(buffer,"authentication"))) != NULL)
			{
				authentication_s = strstr(buffer,">") + 1;
				authentication_e = strstr(authentication_s,"<");
				strncpy(authentication_tmp,authentication_s,authentication_e - authentication_s);
				DEBUG_MSG("tag_parser : authentication_tmp %s\n", authentication_tmp);				
			}
		
			if((encryption_s = (strstr(buffer,"encryption"))) != NULL)
			{
				encryption_s = strstr(buffer,">") + 1;
				encryption_e = strstr(encryption_s,"<");
				strncpy(encryption_tmp,encryption_s,encryption_e - encryption_s);
				DEBUG_MSG("tag_parser : encryption_tmp %s\n", encryption_tmp);			
				if(strcmp(encryption_tmp,"TKIP") == 0)
					wlan0_psk_cipher_type = "tkip";
				else if(strcmp(encryption_tmp,"AES") == 0)
					wlan0_psk_cipher_type = "aes";					
			}
		
			if((networkKey_s = (strstr(buffer,"networkKey"))) != NULL)
			{
				networkKey_s = strstr(buffer,">") + 1;
				networkKey_e = strstr(networkKey_s,"<");
				memset(networkKey_tmp, 0, sizeof(networkKey_tmp));
				strncpy(networkKey_tmp,networkKey_s,networkKey_e - networkKey_s);
				DEBUG_MSG("tag_parser : networkKey_tmp %s\n", networkKey_tmp);			
			}
			
			if((eapMethod_s = (strstr(buffer,"eapMethod"))) != NULL)
			{
				eapMethod_s = strstr(buffer,">") + 1;
				eapMethod_e = strstr(eapMethod_s,"<");
				strncpy(eapMethod_tmp,eapMethod_s,eapMethod_e - eapMethod_s);
				DEBUG_MSG("tag_parser : eapMethod_tmp %s\n", eapMethod_tmp);	
				if(strcmp(eapMethod_tmp, "EAP-TLS")==0)
					wlan0_eap_proto = "tls";
				else if(strcmp(eapMethod_tmp, "PEAP-EAP-MSCHAPv2")==0)
				{
					wlan0_eap_proto = "peap";
					wlan0_peap_inner_proto = "mschapv2";
				}
				else if(strcmp(eapMethod_tmp, "PEAP-EAP-TLS")==0)
				{
					wlan0_eap_proto = "peap";
					wlan0_peap_inner_proto = "mschapv2"; //need to verify
				}	 				
			}
			
			if((channel2Dot4_s = (strstr(buffer,"channel2Dot4"))) != NULL)
			{
				channel2Dot4_s = strstr(buffer,">") + 1;
				channel2Dot4_e = strstr(channel2Dot4_s,"<");
				strncpy(channel2Dot4_tmp,channel2Dot4_s,channel2Dot4_e - channel2Dot4_s);
				DEBUG_MSG("tag_parser : channel2Dot4_tmp %s\n", channel2Dot4_tmp);
				if(atoi(channel2Dot4_tmp)>0)
				{
					wlan0_channel = channel2Dot4_tmp;
					wlan0_auto_channel_enable = "0";
				}
				else
				{
					wlan0_auto_channel_enable = nvram_safe_get("wlan0_auto_channel_enable");
					DEBUG_MSG("tag_parser : wlan0_auto_channel_enable %s\n", wlan0_auto_channel_enable);
					if(wlan0_auto_channel_enable){
						if(atoi(wlan0_auto_channel_enable)>0){
							wlan0_auto_channel_enable = "1";
						}
						else{
							wlan0_channel = nvram_safe_get("wlan0_channel");
							wlan0_auto_channel_enable = "0";
						}	
					}
					else{
						wlan0_channel = nvram_safe_get("wlan0_channel");
						wlan0_auto_channel_enable = "0";
					}						
				}	
				
				check_dual_ap=1;			
			}	
					
			if((channel5Dot0_s = (strstr(buffer,"channel5Dot0"))) != NULL)
			{
				channel5Dot0_s = strstr(buffer,">") + 1;
				channel5Dot0_e = strstr(channel5Dot0_s,"<");
				strncpy(channel5Dot0_tmp,channel5Dot0_s,channel5Dot0_e - channel5Dot0_s);
				DEBUG_MSG("tag_parser : channel5Dot0_tmp %s\n", channel5Dot0_tmp);
				if(atoi(channel5Dot0_tmp)>0)
				{
					wlan1_channel = channel5Dot0_tmp;
					wlan1_auto_channel_enable = "0";
				}
				else
				{
					wlan1_auto_channel_enable = nvram_safe_get("wlan1_auto_channel_enable");
					DEBUG_MSG("tag_parser : wlan1_auto_channel_enable %s\n", wlan1_auto_channel_enable);
					if(wlan1_auto_channel_enable){
						if(atoi(wlan1_auto_channel_enable)>0){
							wlan1_auto_channel_enable = "1";
						}
						else{
							wlan1_channel = nvram_safe_get("wlan1_channel");
							wlan1_auto_channel_enable = "0";
						}	
					}
					else{
						wlan1_channel = nvram_safe_get("wlan1_channel");
						wlan1_auto_channel_enable = "0";
					}					
				}
				check_dual_ap=1;							
			}
			
			if((deviceMode_s = (strstr(buffer,"deviceMode"))) != NULL)
			{
				deviceMode_s = strstr(buffer,">") + 1;
				deviceMode_e = strstr(deviceMode_s,"<");
				strncpy(deviceMode_tmp,deviceMode_s,deviceMode_e - deviceMode_s);
				DEBUG_MSG("tag_parser : deviceMode_tmp %s\n", deviceMode_tmp);	
				if(strcmp(deviceMode_tmp, "infrastructure")==0)
					wlan0_mode = "ap";
				else if(strcmp(deviceMode_tmp, "bridge")==0)
					wlan0_mode = "bridge";
				else if(strcmp(deviceMode_tmp, "repeater")==0)
					wlan0_mode = "repeat";
				else if(strcmp(deviceMode_tmp, "station")==0)	
					wlan0_mode = "sta";		
				else if(strcmp(connectionType_tmp, "IBSS")==0)
					wlan0_mode = "adhoc";			
			}		
				
			if((timeToLive_s = (strstr(buffer,"timeToLive"))) != NULL)
			{
				timeToLive_s = strstr(buffer,">") + 1;
				timeToLive_e = strstr(timeToLive_s,"<");
				strncpy(timeToLive_tmp,timeToLive_s,timeToLive_e - timeToLive_s);
				DEBUG_MSG("tag_parser : timeToLive_tmp %s\n", timeToLive_tmp);			
			}
			
			if((configHash_s = (strstr(buffer,"configHash"))) != NULL)
			{
				configHash_s = strstr(buffer,">") + 1;
				configHash_e = strstr(configHash_s,"<");
				strncpy(configHash_tmp,configHash_s,configHash_e - configHash_s);
				DEBUG_MSG("tag_parser : configHash_tmp %s\n", configHash_tmp);			
			}		
				
			if((keyIndex_s = (strstr(buffer,"keyIndex"))) != NULL)
			{
				keyIndex_s = strstr(buffer,">") + 1;
				keyIndex_e = strstr(keyIndex_s,"<");
				strncpy(keyIndex_tmp,keyIndex_s,keyIndex_e - keyIndex_s);
				DEBUG_MSG("tag_parser : keyIndex_tmp %s\n", keyIndex_tmp);	
				if(atoi(keyIndex_tmp)>0 && atoi(keyIndex_tmp)<5)	
					wlan0_wep_default_key = keyIndex_tmp;
				else
					wlan0_wep_default_key = "1";			
			}
			
			//if((configAuthor_s = (strstr(buffer,"configAuthor"))) != NULL){
				//Don't care
			//}
			//if((primaryProfile_s = (strstr(buffer,"primaryProfile"))) != NULL){
				//Don't care
			//}
			//if((optionalProfile_s = (strstr(buffer,"optionalProfile"))) != NULL){
				//Don't care
			//}
			//if((keyProvideAutomatically_s = (strstr(buffer,"keyProvideAutomatically"))) != NULL){
				//Don't care
			//}
			
		}		
		
		// authentication and encryption parser
		if(strcmp(authentication_tmp, "open") == 0 || strcmp(authentication_tmp, "shared") == 0 )
		{
			if(strcmp(encryption_tmp,"WEP") == 0)
			{
				switch(strlen(networkKey_tmp))
				{
					case 5: 
						wlan0_wep_display = "ascii";
						if(strcmp(authentication_tmp, "open") == 0)
							wlan0_security = "wep_open_64";	
						else
							wlan0_security = "wep_share_64";						
						break;
					case 10: 
						wlan0_wep_display = "hex";
						if(strcmp(authentication_tmp, "open") == 0)
							wlan0_security = "wep_open_64";	
						else
							wlan0_security = "wep_share_64";					
						break;
					case 13:
						wlan0_wep_display = "ascii";
						if(strcmp(authentication_tmp, "open") == 0)
							wlan0_security = "wep_open_128";
						else
							wlan0_security = "wep_share_128";	
						break;
					case 26:
						wlan0_wep_display = "hex";
						if(strcmp(authentication_tmp, "open") == 0)
							wlan0_security = "wep_open_128";	
						else
							wlan0_security = "wep_share_128";	
						break;		
				}								
			}
			else
				wlan0_security = "disable";
				
			if(wlan0_wep_default_key == NULL)
				wlan0_wep_default_key = "1";	
		}
		else if(strcmp(authentication_tmp, "WPA") == 0)
		{
			wlan0_security = "wpa_eap";		
		}
		else if(strcmp(authentication_tmp, "WPAPSK")==0)
		{
			wlan0_security = "wpa_psk";
			//wlan0_security = "wpa2auto_psk";
			wlan0_psk_pass_phrase = networkKey_tmp;
		}
		else if(strcmp(authentication_tmp, "WPA2")==0)
		{
			wlan0_security = "wpa2_eap";
			
		}
		else if(strcmp(authentication_tmp, "WPA2PSK")==0)
		{
			wlan0_security = "wpa2_psk";
			//wlan0_security = "wpa2auto_psk";
			wlan0_psk_pass_phrase = networkKey_tmp;
		}
		else
			wlan0_security = "disable";
	
		DEBUG_MSG("configId_tmp = %s\n", configId_tmp);
		DEBUG_MSG("ssid_tmp = %s\n", ssid_tmp);
		DEBUG_MSG("authentication_tmp = %s\n", authentication_tmp);
		DEBUG_MSG("encryption_tmp = %s\n", encryption_tmp);
		DEBUG_MSG("networkKey_tmp = %s\n", networkKey_tmp);				
		DEBUG_MSG("wlan0_auto_channel_enable = %s\n", wlan0_auto_channel_enable);
		DEBUG_MSG("wlan0_channel = %s\n", wlan0_channel);
		DEBUG_MSG("wlan1_auto_channel_enable = %s\n", wlan1_auto_channel_enable);
		DEBUG_MSG("wlan1_channel = %s\n", wlan1_channel);
		DEBUG_MSG("wlan0_security = %s\n", wlan0_security);
		DEBUG_MSG("wlan0_psk_cipher_type = %s\n", wlan0_psk_cipher_type);
		DEBUG_MSG("wlan0_psk_pass_phrase = %s\n", wlan0_psk_pass_phrase);
		DEBUG_MSG("wlan0_eap_proto = %s\n", wlan0_eap_proto);
		DEBUG_MSG("wlan0_peap_inner_proto = %s\n", wlan0_peap_inner_proto);
		DEBUG_MSG("wlan0_wep_display = %s\n", wlan0_wep_display);
		DEBUG_MSG("wlan0_wep_default_key = %s\n", wlan0_wep_default_key);	
		
	}
	fclose(fp);
	return 0;
}

char *StrDecode(char *to, char *from, int en_nbsp)
{
	char *p = to;
	char *pos_s, *pos_e;
	char tmp_str[64] = {0};
	
	memset(tmp_str, 0, 64);

	while (*from)
	{
		switch (*from)
		{
			case '&':
				pos_s = strstr(from, "&") + 1;
				pos_e = strstr(pos_s, ";");
				strncpy(tmp_str, pos_s, pos_e - pos_s);	
				if(strcmp(tmp_str, "amp") == 0){
					from += 4;
					strcpy(p, "&");
					p += 1;
				}else if(strcmp(tmp_str, "gt") == 0){			
					from += 3;
					strcpy(p, ">");
					p += 1;
				}else if(strcmp(tmp_str, "lt") == 0){					
					from += 3;
					strcpy(p, "<");
					p += 1;				
				}else if(strcmp(tmp_str, "quot") == 0){						
					from += 5;
					strcpy(p, "\"");
					p += 1;
				}
				memset(tmp_str, 0, 64);
				break;
			case ' ':
				if (en_nbsp)
				{
					strcpy(p, "&nbsp;");
					p += 6;
					break;
				}				
				// fall through to default
			default:
				*p++ = *from;
		}
		from++;
	}	
	*p = '\0';
	return to;
}
/*
static int PasswordHash(char *password, char *output)
{
	unsigned char Tmp = 0;
	int i, j = 0, flag = 0;

	if (strlen(password) == 64) { // Brent<Add>:If input is 64hex 
		for(i = 0; i < 64; i++){
			if(password[i] >= 'a' && password[i] <= 'f'){
				Tmp =(Tmp << 4);
				Tmp |= password[i] - 'a' + 10;
				flag++;
			}else if(password[i] >= 'A' && password[i] <= 'F'){
				Tmp = (Tmp << 4);
				Tmp |= password[i] - 'A' + 10;
				flag++;
			}else if(password[i] >= '0' && password[i] <= '9'){
				Tmp = (Tmp << 4);
				Tmp |= password[i] - '0';
				flag++;
			}else if (1) // over hex range
				return 0; 
			if(flag % 2 == 0)
				output[j++] = Tmp;
		}
		return 1;	
	} 

	if ((strlen(password) > 63)){// || (ssidlength > 32)) {
		return 0;
	}
	return 0;
}
*/
