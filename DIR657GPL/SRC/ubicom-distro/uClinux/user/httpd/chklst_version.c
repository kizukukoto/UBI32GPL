#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <httpd_util.h>
#include <nvram.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#ifdef CONFIG_LP
#include "lp.h"
#include "lp_version.h"
#endif

#ifdef HTTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define VER_FIRMWARE_LEN 7

extern const char VER_FIRMWARE[];
extern const char VER_DATE[];
extern const char HW_VERSION[];
extern const char VER_BUILD[];
extern const char SVN_TAG[];
extern const unsigned char __CAMEO_LINUX_VERSION__[];
extern const unsigned char __CAMEO_LINUX_BUILD__[];
extern const unsigned char __CAMEO_LINUX_BUILD_DATE__[];
extern const unsigned char __CAMEO_ATHEROS_VERSION__[];
extern const unsigned char __CAMEO_ATHEROS_BUILD__[];
extern const unsigned char __CAMEO_ATHEROS_BUILD_DATE__[];
extern const unsigned char __CAMEO_WIRELESS_VERSION__[];
extern const unsigned char __CAMEO_WIRELESS_BUILD__[];
extern const unsigned char __CAMEO_WIRELESS_DATE_[];

void get_wlan_domain ( char* wlan_domain )
{
        char *hex_wlan_domain = NULL ;

#ifdef SET_GET_FROM_ARTBLOCK
        if(artblock_get("wlan0_domain"))
                hex_wlan_domain = artblock_safe_get("wlan0_domain");
        else
                hex_wlan_domain = nvram_safe_get("wlan0_domain");
	sprintf(wlan_domain,"%s", hex_wlan_domain);
#else
        hex_wlan_domain = nvram_safe_get("wlan0_domain");
        sprintf(wlan_domain,"%s", hex_wlan_domain);
#endif
}

void get_wirelan_mac ( char** wirelan_mac, char* str )
{
#ifdef SET_GET_FROM_ARTBLOCK
	if ( artblock_get( str ) )
        	*wirelan_mac = artblock_safe_get ( str );
    	else
    		*wirelan_mac = nvram_safe_get ( str );
#else
    	*wirelan_mac = nvram_safe_get ( str );
#endif
}

void get_hw_version ( char** hw_version )
{
#ifdef SET_GET_FROM_ARTBLOCK
	if (artblock_get("hw_version"))
        	*hw_version = artblock_get("hw_version");
    	else
        	*hw_version = HW_VERSION;
#else
    	*hw_version = HW_VERSION;
#endif
}

#ifdef NVRAM_CONF_VER_CTL
char config_version[12]= {};
#endif

void set_default_value ()
{
        chklst.FW = (char *)VER_FIRMWARE;
        chklst.Build = (char *)VER_BUILD;
        chklst.Date = (char *)VER_DATE;
        chklst.LINUX = (char *)__CAMEO_LINUX_VERSION__;
        chklst.LINUX_BUILD= (char *)__CAMEO_LINUX_BUILD__;
        chklst.LINUX_BUILD_DATE = (char *)__CAMEO_LINUX_BUILD_DATE__;
        chklst.SDK = (char *)__CAMEO_ATHEROS_VERSION__;
        chklst.SDK_Build = (char *)__CAMEO_ATHEROS_BUILD__;
        chklst.SDK_Date = (char *)__CAMEO_ATHEROS_BUILD_DATE__;
        chklst.WLAN_Version = (char *)__CAMEO_WIRELESS_VERSION__;
        chklst.WLAN_Build = (char *)__CAMEO_WIRELESS_BUILD__;
        chklst.WLAN_Date = (char *)__CAMEO_WIRELESS_DATE_;
#ifdef NVRAM_CONF_VER_CTL
		if (strlen(config_version) == 0) {
			strcpy(config_version, nvram_safe_get("configuration_version"));
		}
        chklst.Config_Version = config_version;
#endif        
}

void get_checksum( char* ImgCheckSum, int s )
{
	int i ;
    	FILE *fp;

	fp = fopen("/var/tmp/checksum.txt","r");
    	if (fp){
        	fread(ImgCheckSum, s, 1, fp);
        	fclose(fp);
    	}
    	printf("ImgCheckSum=%s\n",ImgCheckSum);
    	for( i = 0 ; i < s ; i++ ){
        	ImgCheckSum[i] = toupper( ImgCheckSum[i] );
    	}
}
#ifdef CONFIG_LP
void get_lp_checksum( char* ImgLPCheckSum, int s )
{
	int i ;
    	FILE *fp;

	fp = fopen("/var/tmp/LP_checksum.txt","r");
    	if (fp){
        	fread(ImgLPCheckSum, s, 1, fp);
        	fclose(fp);
    	}
    	else {
            printf("can't fopen LP_checksum.txt");
    	    strcpy(ImgLPCheckSum, "0xFFFFFFFF");	
   	}
    	printf("ImgLPCheckSum=%s\n",ImgLPCheckSum);
    	for( i = 0 ; i < s ; i++ ){
        	ImgLPCheckSum[i] = toupper( ImgLPCheckSum[i] );
    	}
}
#endif

void get_fw_notify( char* fw_url, char** ptr_tmp, int s )
{
	char *p_fw_url = NULL;

    	p_fw_url = nvram_safe_get( "check_fw_url" );
    	if (strlen( p_fw_url ) != 0){
        	strncpy ( fw_url, nvram_safe_get ( "check_fw_url" ), s );
		*ptr_tmp = strchr ( fw_url, ',' ) ;
        	if (*ptr_tmp) {
            		**ptr_tmp='\0';
			(*ptr_tmp) ++;
		}else
            		**ptr_tmp="";
	}
}

/***************************************************************************/
/*The function gets the information of version.txt from other functions    */
/*given above. Then assign the information in *p_chklst which from         */
/*do_version_cgi() in core.c				                   */
/*NOTE: In version.txt wireless domain should print CountryName ex: US/NA  */
/*      DIR865 has to show other 3 information :			   */
/*      WlanMAC, WlanDomain, and SSID for 5GHz				   */
/***************************************************************************/
void print_version ( char* p_chklst )
{
	int wlan_domain[30] = {};
    	int _10_wlan_domain = 0;
	char* p_wlan_domain = NULL;
	char wlan1_mac[32] = {};
    	char fw_url[80] = {};  //currently , there r 69 characters in nvram config
    	char *ptr_tmp = NULL;
    	char ImgCheckSum[10] = {};
        int i;
	char lan_mac[32] = {},*ptr_lan = NULL;

	memset ( fw_url, '\0', sizeof ( fw_url ) );
        set_default_value();
	get_wlan_domain ( wlan_domain );
	if(!(strncmp(wlan_domain,"0x",2))){     /* ex :"wireless domain : US/NA"  */
        	_10_wlan_domain = (int) strtoul ( wlan_domain , NULL , 16 );
//		_10_wlan_domain = hexTO10(wlan_domain+2,wlan_domain+3);
        	printf("_10_wlan_domain = [%d]\n",_10_wlan_domain);
		if(_10_wlan_domain < 255)                    // defined domain
            		p_wlan_domain = GetCountryName(_10_wlan_domain);
        	else                               // 0xff??   --> undefined
            		p_wlan_domain = "Unknown Domain";
    	}else
        p_wlan_domain = "Error Domain";
    	DEBUG_MSG("%s: p_wlan_domain=%s",__func__,p_wlan_domain);
    	strncpy(wlan_domain,p_wlan_domain, sizeof(wlan_domain));
    	chklst.LAN_MAC = lan_mac;
	get_wirelan_mac ( &ptr_lan, "lan_mac" );
	get_wirelan_mac ( &chklst.WAN_MAC, "wan_mac" );
	strcpy(chklst.LAN_MAC, ptr_lan);
	for(i = 0 ; i < strlen(chklst.LAN_MAC) ; i++){
		chklst.LAN_MAC[i] = toupper(chklst.LAN_MAC[i]);
	}
	for(i = 0 ; i < strlen(chklst.WAN_MAC) ; i++){
		chklst.WAN_MAC[i] = toupper(chklst.WAN_MAC[i]);
	}	
	chklst.SSID = nvram_safe_get ("wlan0_ssid");
	get_hw_version ( &chklst.HW_Version );
	get_fw_notify ( fw_url, &ptr_tmp, sizeof(fw_url));
	chklst.WLAN_MAC = chklst.LAN_MAC;
	chklst.WLAN_Domain = wlan_domain;
#ifdef USER_WL_ATH_5GHZ
	chklst.SSID1 = nvram_safe_get ("wlan1_ssid");
	strncpy(wlan1_mac, chklst.LAN_MAC, sizeof(wlan1_mac));
	get_ath_5g_macaddr ( wlan1_mac );
	chklst.WLAN1_MAC = wlan1_mac;
	chklst.WLAN_Domain1 = wlan_domain; //5.0GHz
#endif
	get_checksum( ImgCheckSum, sizeof(ImgCheckSum));

   	p_chklst += sprintf( p_chklst, \
		"Firmware External Version: V%s\n<br />",  \
		 chklst.FW );
       	p_chklst += sprintf( p_chklst, \
		"Firmware Internal Version: V%sB%s\n<br />", \
		chklst.FW, chklst.Build );
   	p_chklst += sprintf( p_chklst, \
		"Date: %s\n<br />", \
		chklst.Date );
   	p_chklst += sprintf( p_chklst, \
		"CheckSum: 0x%s\n<br />", \
		ImgCheckSum );
  	p_chklst += sprintf( p_chklst, \
		"WLAN Domain(2.4GHz): %s\n<br />", \
		chklst.WLAN_Domain );
#ifdef USER_WL_ATH_5GHZ
       	p_chklst += sprintf( p_chklst, \
		"WLAN Domain(5.0GHz): %s\n<br />", \
		chklst.WLAN_Domain1 );
#endif
       	p_chklst += sprintf( p_chklst, \
		"Firmware Notify: http://%s%s\n<br />", \
		fw_url,ptr_tmp );
       	p_chklst += sprintf( p_chklst, \
		"Kernel: %s, B%s, Date=%s\n<br />", \
		chklst.LINUX, chklst.LINUX_BUILD, chklst.LINUX_BUILD_DATE );
      	p_chklst += sprintf( p_chklst, \
		"LAN MAC: %s\n<br />", \
		chklst.LAN_MAC);
       	p_chklst  += sprintf( p_chklst, \
		"WAN MAC: %s\n<br />", \
		chklst.WAN_MAC);
#ifdef USER_WL_ATH_5GHZ
       	p_chklst += sprintf( p_chklst, \
		"WLAN MAC 0: %s\n<br />", \
		chklst.WLAN_MAC );
	p_chklst += sprintf( p_chklst, \
 	       "WLAN MAC 1: %s\n<br />", \
        	chklst.WLAN1_MAC );
	p_chklst += sprintf( p_chklst, \
        	"SSID 0: %s\n<br />", \
        	chklst.SSID );
	p_chklst += sprintf( p_chklst, \
        	"SSID 1: %s\n<br />", \
        	chklst.SSID1 );
#else
        p_chklst += sprintf( p_chklst, \
		"WLAN MAC: %s\n<br />", \
		chklst.WLAN_MAC );
	p_chklst += sprintf( p_chklst, \
                "SSID : %s\n<br />", \
        	chklst.SSID );
#endif
	p_chklst += sprintf( p_chklst, \
		"Restore Default=%d\n<br />", \
		chklst.DEFAULT_VALUE );
        p_chklst += sprintf( p_chklst, \
               "SVN Revision: %s <br />", SVN_TAG);
}
/***************************************************************************/
/*The function gets the information of chklst.txt from other functions     */
/*given above. Then assign the information in *p_chklst which form         */
/*do_chklst_cgi() in core.c                                                */
/*NOTE: In chklst.txt wireless domain should print HEX.position ex : 0x10  */
/*      DIR865 has to show other 4 information :                           */
/*      WlanMAC, WlanDomain, SSID,and ChannelList for 5GHz                 */
/***************************************************************************/

void print_chklst ( char* p_chklst )
{
	int wlan_domain[30] = {};
        char wlan1_mac[32] = {};
        char fw_url[80] = {};  //currently , there r 69 characters in nvram config
        char *ptr_tmp = NULL;
        char ImgCheckSum[10] = {};
	char *ver_main = NULL, *ver_sub = NULL;
	char ver_tmp[VER_FIRMWARE_LEN]={'\0'};
#ifdef CONFIG_LP
        char ImgLPCheckSum[10] = {};
        char *lp_main = NULL, *lp_sub = NULL;
        char lp_tmp[12]={'\0'};	
#endif
	char channel_list_5G[512],channel_list_24G[128];
	int domain, i;
	char lan_mac[32] = {},*ptr_lan = NULL;

	memset(fw_url,'\0',sizeof(fw_url));
	set_default_value();
        strncpy(ver_tmp,VER_FIRMWARE,VER_FIRMWARE_LEN);
	ver_main = strtok(ver_tmp,".");
        ver_sub = strtok(NULL,".");

        if(ver_main == NULL){
                ver_main = "";
        }
        if(ver_sub == NULL){
                ver_sub = "";
        }
	get_wlan_domain ( wlan_domain );/*show "wireless domain : 0x10"*/
#ifdef USER_WL_ATH_5GHZ
    	DEBUG_MSG("DIR865:%c %c\n", *(wlan_domain+2), *(wlan_domain+3));
    	if(islower(*(wlan_domain+2)))
        	*(wlan_domain+2)=toupper(*(wlan_domain+2));
    	if(islower(*(wlan_domain+3)))
        	*(wlan_domain+3)=toupper(*(wlan_domain+3));
#endif
	chklst.WLAN_Domain = wlan_domain;
	if(wlan_domain)
        	sscanf(wlan_domain,"%x",&domain);
    	else
        	domain = 0x10;

	for(i = 2 ; i < strlen(chklst.WLAN_Domain) ; i++){
		chklst.WLAN_Domain[i] = toupper(chklst.WLAN_Domain[i]);
	}	  
    	chklst.LAN_MAC = lan_mac;
	get_wirelan_mac ( &ptr_lan, "lan_mac" );
        get_wirelan_mac ( &chklst.WAN_MAC, "wan_mac" );
        strcpy(chklst.LAN_MAC, ptr_lan);
	for(i = 0 ; i < strlen(chklst.LAN_MAC) ; i++){
		chklst.LAN_MAC[i] = toupper(chklst.LAN_MAC[i]);
	}
	for(i = 0 ; i < strlen(chklst.WAN_MAC) ; i++){
		chklst.WAN_MAC[i] = toupper(chklst.WAN_MAC[i]);
	}
	chklst.SSID = nvram_safe_get ("wlan0_ssid");
        get_hw_version ( &chklst.HW_Version );
	get_fw_notify ( fw_url, &ptr_tmp, sizeof(fw_url));
        chklst.WLAN_MAC = chklst.LAN_MAC;
#ifdef USER_WL_ATH_5GHZ
        chklst.WLAN_Domain1 = wlan_domain; //5.0Hz
        chklst.SSID1 = nvram_safe_get ("wlan1_ssid");
        strncpy(wlan1_mac, chklst.LAN_MAC, sizeof(wlan1_mac));
        get_ath_5g_macaddr ( wlan1_mac );
        chklst.WLAN1_MAC = wlan1_mac;
        GetDomainChannelList(domain,WIRELESS_BAND_50G,channel_list_5G,1);
#endif
        get_checksum( ImgCheckSum, sizeof(ImgCheckSum));
	GetDomainChannelList(domain,WIRELESS_BAND_24G,channel_list_24G,1);
	
#ifdef CONFIG_LP
        get_lp_checksum( ImgLPCheckSum, sizeof(ImgLPCheckSum));     
#endif	
	p_chklst  += sprintf ( p_chklst, \
		"Firmware Version: ver%s.%sb%s\n<br />", \
		ver_main, ver_sub, chklst.Build ) ;
    	p_chklst  += sprintf ( p_chklst, \
		"Firmware Date: %s\n<br />", \
		chklst.Date ) ;
    	p_chklst  += sprintf ( p_chklst, \
		"KERNEL: %s, SDK: %s, Build: %s, Date: %s\n<br />", \
		chklst.LINUX,chklst.SDK,chklst.LINUX_BUILD,chklst.LINUX_BUILD_DATE ) ;
	p_chklst  += sprintf ( p_chklst, \
		"Wireless Domain: %s\n<br />", \
		chklst.WLAN_Domain );
#ifdef USER_WL_ATH_5GHZ
	p_chklst  += sprintf ( p_chklst, \
		"Wireless Domain(5.0GHz): %s\n<br />", \
		chklst.WLAN_Domain1 );
	p_chklst  += sprintf ( p_chklst, \
		"SSID 0: %s\n<br />", \
		chklst.SSID );
	p_chklst  += sprintf ( p_chklst, \
		"SSID 1: %s\n<br />", \
		chklst.SSID1 );
	p_chklst  += sprintf ( p_chklst, \
		"WLAN MAC 0: %s\n<br />", \
		chklst.WLAN_MAC );
	p_chklst  += sprintf ( p_chklst, \
		"WLAN MAC 1: %s\n<br />", \
		chklst.WLAN1_MAC );
	p_chklst  += sprintf ( p_chklst, \
		"WLAN 0 Channel List: %s\n<br />", \
		channel_list_24G );
	p_chklst  += sprintf ( p_chklst, \
		"WLAN 1 Channel List: %s\n<br />", \
		channel_list_5G );
#else
	p_chklst  += sprintf ( p_chklst, \
		"SSID: %s\n<br />", \
		chklst.SSID );
	p_chklst  += sprintf ( p_chklst, \
                "WLAN Channel List: %s\n<br />", \
                channel_list_24G );
	p_chklst  += sprintf ( p_chklst, \
		"WLAN MAC: %s\n<br />", \
		chklst.WLAN_MAC );
#endif
    	p_chklst  += sprintf ( p_chklst, \
		"LAN MAC: %s\n<br />", \
		chklst.LAN_MAC );
    	p_chklst  += sprintf ( p_chklst, \
		"WAN MAC: %s\n<br />", \
		chklst.WAN_MAC );
    	p_chklst  += sprintf ( p_chklst, \
		"HW Ver: %s\n<br />", \
		chklst.HW_Version );
    	p_chklst  += sprintf ( p_chklst, \
		"Firmware Notify: http://%s%s\n<br />", \
		fw_url, ptr_tmp );
    	p_chklst  += sprintf ( p_chklst, \
		"Firmware Global URL: %s\n<br />", \
		nvram_safe_get("manufacturer_url") );
		p_chklst  += sprintf ( p_chklst, \
		"checksum: 0x%s\n<br />", \
		ImgCheckSum );
#ifdef NVRAM_CONF_VER_CTL
		p_chklst += sprintf(p_chklst, \
		"CONFIG_VERSION: %s\n<br />", \
		chklst.Config_Version);
#endif            

#ifdef CONFIG_LP
	char *lp_info = NULL;
	char lp_region[12];
	lp_info = (char *)malloc(12);
	if(get_lp_info(LP_REG_PATH, lp_info,12)==NULL || strlen(lp_info) ==0){
		strcpy(lp_region, "DEFAULT");
	}else{
		strcpy(lp_region, lp_info);
	}
		p_chklst += sprintf( p_chklst, \
		"Language: %s\n<br />", \
		lp_region ) ; 	
	
	if(!strncmp(lp_region,"DEFAULT", 7)){  //DEFAULT
		p_chklst += sprintf( p_chklst, \
		    "Language-Version: ver0.00b00\n<br />");
		p_chklst  += sprintf ( p_chklst, \
		    "Language-Checksum: 0x00000000\n<br />");		
	}
	else{
		char lp_ver[12] = {};
		char lp_build[12] = {};

		FILE *fp = NULL, *fp1 = NULL;
		fp = fopen("/var/lp_ver","r");
		if(fp){
		    fscanf(fp,"%s",lp_ver);
		    fclose(fp);
		}   
		fp1 = fopen("/var/lp_build","r");
		if(fp1){
		    fscanf(fp1,"%s",lp_build);
		    fclose(fp1);
		}
		if(strcmp(lp_build,"00")==0)//if build number is 00, don't show build number
                        p_chklst += sprintf( p_chklst, "Language-Version: ver%s%s\n<br />",lp_ver, lp_region);
                else   
		p_chklst += sprintf( p_chklst, \
		    "Language-Version: ver%s%sb%s\n<br />", \
		    lp_ver, lp_region, lp_build);
		p_chklst  += sprintf ( p_chklst, \
		"Language-Checksum: 0x%s\n<br />", \
		ImgLPCheckSum );
	}
	if(lp_info)
		free(lp_info);
#else
		p_chklst += sprintf( p_chklst, \
		"Language: DISABLE\n<br />") ; 
#endif		
 				
        p_chklst += sprintf( p_chklst, \
               "SVN Revision: %s <br />", SVN_TAG);
}
