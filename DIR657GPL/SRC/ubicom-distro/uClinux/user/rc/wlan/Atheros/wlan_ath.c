#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

#ifdef  RC_DEBUG_WLAN
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef CONFIG_USER_WISH
static void set_wish_rule(int, int);
static void set_wish(int, int);
#endif

static void set_security(int bss_index, int ath_count);
static void create_wsc_conf(int bss_index, int ath_count, int wps_enable, int wep_len);
 
static void set_11b_rate(int bss_index, int ath_count, char *dot_mode);
static void set_11bg_rate(int bss_index, int ath_count, char *dot_mode);
static void set_11bgn_rate(int bss_index, int ath_count, char *dot_mode);

int stop_wlan_flag = 0;
int wlan0_enable = 0;
int wlan0_vap_enable = 0;
int wlan1_enable = 0;
int wlan1_vap_enable = 0;
int wlan_vap[4]={0};//support 4 VAP => 11bgn=wlan_vap[0], wlan_vap[2]  => 11a=wlan_vap[1], wlan_vap[3]
static char ap_ifnum[30]={0};
static char ap_ifnum_2[30]={0};
static char ap_ifnum_3[30]={0};
static char ap_ifnum_4[30]={0};
/*
	use_hostapd[bss_index]=4 => do not use hostapd, using wep
	use_hostapd[2]=1 => 2.4G GuestZone(bss_index=2) is ath1 using hostapd
*/
int use_hostapd[4]={4, 4, 4, 4};

void stop_insmod(void)
{
/*	NickChou add, 
maybe we can only "rmmod" to accelerate wlan procedure.
Do not run "brctl delif", "ifconfig down", "wlanconfig destroy"	.
*/
	int i=0, j=0;
	char *lan_bridge = nvram_safe_get("lan_bridge");

	for(i=0; i<4; i++)
	{
		if(wlan_vap[i]==1)
		{
			_system("brctl delif %s ath%d", lan_bridge, j);
			_system("ifconfig ath%d down", j);
			_system("wlanconfig ath%d destroy", j);
			j++;
		}	
	}

	system("[ -n \"`lsmod | grep ath_pci`\" ] && rmmod ath_pci");
	sleep(1);
	system("[ -n \"`lsmod | grep ath_pktlog`\" ] && rmmod ath_pktlog");
	system("[ -n \"`lsmod | grep wlan_acl`\" ] && rmmod wlan_acl");
	system("[ -n \"`lsmod | grep wlan_wep`\" ] && rmmod wlan_wep");
	system("[ -n \"`lsmod | grep wlan_tkip`\" ] && rmmod wlan_tkip");
	system("[ -n \"`lsmod | grep wlan_ccmp`\" ] && rmmod wlan_ccmp");
	system("[ -n \"`lsmod | grep wlan_xauth`\" ] && rmmod wlan_xauth");
	system("[ -n \"`lsmod | grep wlan_scan_ap`\" ] && rmmod wlan_scan_ap");
	system("[ -n \"`lsmod | grep wlan_scan_sta`\" ] && rmmod wlan_scan_sta");
	sleep(1);
	system("[ -n \"`lsmod | grep wlan_me`\" ] && rmmod wlan_me");//Atheros multicast enhancement algorithm
	system("[ -n \"`lsmod | grep ath_dev`\" ] && rmmod ath_dev");
	system("[ -n \"`lsmod | grep ath_dfs`\" ] && rmmod ath_dfs");
	system("[ -n \"`lsmod | grep ath_rate_atheros`\" ] && rmmod ath_rate_atheros");
	system("[ -n \"`lsmod | grep wlan`\" ] && rmmod wlan");
	system("[ -n \"`lsmod | grep ath_hal`\" ] && rmmod ath_hal");
	sleep(1);        
}

int stop_wlan(void)
{	
    if(!( action_flags.all || action_flags.wlan ) )
        return 1;
        
	if(stop_wlan_flag == 1)
	{
   		DEBUG_MSG("******** Stop Wlan (Wireless Radio ON) ********\n");
        stop_insmod();
        wlan0_enable = wlan1_enable = wlan0_vap_enable = wlan1_vap_enable = 0;
        use_hostapd[0] = use_hostapd[1] = use_hostapd[2] = use_hostapd[3] = 4;
		memset(wlan_vap, 0, sizeof(wlan_vap));
	}	
	
	system("gpio WLAN_LED off");
	DEBUG_MSG("******* Stop Wlan Finish ******\n");
	return 1;
}

int stop_wps(void)
{
	if(!( action_flags.all || action_flags.wlan ) )
        return 1;

	KILL_APP_ASYNCH("hostapd");
	KILL_APP_ASYNCH("wpatalk");
	KILL_APP_ASYNCH("wpa_supplicant");
	KILL_APP_ASYNCH("wpsbtnd");
	/*avoid the PBC-led is always on ,if it is on before we shutdown wps */
	//(Ubicom) No such file!
	//system("exec echo 0 > /proc/ar531x/gpio/tricolor_led");
	return 1;
}

void wlan0_ifnum_setup(char *op_mode)
{	
	int wlan0_auto_channel_enable = atoi(nvram_safe_get("wlan0_auto_channel_enable"));
	char *wlan0_channel = nvram_safe_get("wlan0_channel");
	char *wlan0_11n_protection = nvram_safe_get("wlan0_11n_protection");
	char *wlan0_dot11_mode = nvram_safe_get("wlan0_dot11_mode");	
	char *Ap_Chmode=NULL;
	char *Ap_Primary_Ch=NULL;
		
	DEBUG_MSG("wlan0_ifnum_setup: start: op_mode=%s\n", op_mode);

	memset(ap_ifnum, 0, sizeof(ap_ifnum));
	memset(ap_ifnum_3, 0, sizeof(ap_ifnum_3));

	if (strcmp(op_mode, "APC")==0)
		system("insmod /lib/modules/wlan_scan_sta.ko");
	else if (strcmp(op_mode, "AP")==0)
	{
		/*NickChou 20090805
		Before wirelsess driver is 7.3.0.386, we only insert wlan_scan_ap.ko in auto channel mode.
		But 7.3.0.386 need wlan_scan_ap.ko to do a overlaping BSS scan for 20/40 coext in 2G channel.
		(AP94\802_11\linux\net80211\ieee80211_proto.c)
		So whether in auto channel or not, we must insert it.
		*/
		system("insmod /lib/modules/wlan_scan_ap.ko");	
	}	

	if (wlan0_auto_channel_enable==1)
		Ap_Primary_Ch="11ng";
	else
		Ap_Primary_Ch=wlan0_channel;
			
	if ( (strcmp(wlan0_dot11_mode, "11bgn")==0 || 
		  strcmp(wlan0_dot11_mode, "11gn")==0  || 
		  strcmp(wlan0_dot11_mode, "11n")==0) )
	{
		if(wlan0_auto_channel_enable==0)
		{
			if (strcmp(wlan0_11n_protection, "auto") != 0)
				Ap_Chmode="11NGHT20";
		    else/*wlan0_11n_protection=auto*/
		    {    	
			    if(atoi(wlan0_channel) < 6) 
			    	Ap_Chmode="11NGHT40PLUS";
			    else
			        Ap_Chmode="11NGHT40MINUS";
			}
		}
		else /*auto channel*/	 
		{
			if(strcmp(wlan0_11n_protection, "auto") == 0)
				Ap_Chmode="11NGHT40";
			else
				Ap_Chmode="11NGHT20";
		}
			 
	}
	else if (strcmp(wlan0_dot11_mode, "11b")==0)
		Ap_Chmode="11B";
	else if (strcmp(wlan0_dot11_mode, "11g")==0 || strcmp(wlan0_dot11_mode, "11bg")==0)
		Ap_Chmode="11G";
	
	if(strcmp(wlan0_dot11_mode, "11n")==0) //set puren
		sprintf(ap_ifnum, "0:RF:%s:%s:1", Ap_Primary_Ch, Ap_Chmode);
	else if (strcmp(wlan0_dot11_mode, "11g")==0 || strcmp(wlan0_dot11_mode, "11gn")==0) //set pureg
		sprintf(ap_ifnum, "0:RF:%s:%s:0", Ap_Primary_Ch, Ap_Chmode);
	else
		sprintf(ap_ifnum, "0:RF:%s:%s:2", Ap_Primary_Ch, Ap_Chmode);
	     
	sprintf(ap_ifnum_3, "0:NO:%s:%s:2", Ap_Primary_Ch, Ap_Chmode);
			 	    
	DEBUG_MSG("wlan0_ifnum_setup: end\n");	
}

void wlan1_ifnum_setup(void)
{
	int wlan1_auto_channel_enable = atoi(nvram_safe_get("wlan1_auto_channel_enable"));
	char *wlan1_channel = nvram_safe_get("wlan1_channel");
	char *wlan1_11n_protection = nvram_safe_get("wlan1_11n_protection");	
	char *wlan1_dot11_mode = nvram_safe_get("wlan1_dot11_mode");
	char *Ap_Chmode_2=NULL;
	char *Ap_Primary_Ch_2=NULL;

	memset(ap_ifnum_2, 0, sizeof(ap_ifnum_2));
	memset(ap_ifnum_4, 0, sizeof(ap_ifnum_4));
		
	if(wlan1_auto_channel_enable==1)
	{	
		Ap_Primary_Ch_2="11na";
		system("insmod /lib/modules/wlan_scan_ap.ko");
	}	
	else if( (atoi(wlan1_channel)>35) && (atoi(wlan1_channel)<166) )
	{
		/* HT40MHz*/
		/* PLUS  => 36, 44, 52, 60, 100, 108, 116, 124, 132, 149, 157 */ 
		/* MINUS => 40, 48, 56, 64, 104, 112, 120, 128, 136, 153, 161 */
		/* HT20MHz => 165 */		
		Ap_Primary_Ch_2=wlan1_channel;
	}
	else /*default*/
		Ap_Primary_Ch_2="40";
			
	if ( (strcmp(wlan1_dot11_mode, "11na")==0 || strcmp(wlan1_dot11_mode, "11n")==0) )
	{
	 	if(wlan1_auto_channel_enable==0)
	 	{
		 	if (strcmp(wlan1_11n_protection, "auto") != 0)
		 	{
		 		Ap_Chmode_2="11NAHT20";
		 	}	
     		else if( ((atoi(wlan1_channel)-28)%8==0)||
		    		 (atoi(wlan1_channel)==149)		||
		    		 (atoi(wlan1_channel)==153)		||
		    		 (atoi(wlan1_channel)==157)
		    		)
		    {
		    	//36, 44, 52, 60, 100, 108, 116, 124, 132, 149, 153, 157
		    	Ap_Chmode_2="11NAHT40PLUS";
		    }	
		    else if(atoi(wlan1_channel)==165)
		    	Ap_Chmode_2="11NAHT20";
		    else
		    {
		       	//40, 48, 56, 64, 104, 112, 120, 128, 136, 161
		       	Ap_Chmode_2="11NAHT40MINUS";
		    }   	
		}	
		else /*auto channel*/
		{
			if (strcmp(wlan1_11n_protection, "auto") == 0)
				Ap_Chmode_2="11NAHT40";
			else
				Ap_Chmode_2="11NAHT20";
		}	    
	}
	else if (strcmp(wlan1_dot11_mode, "11a")==0)
		Ap_Chmode_2="11A";
	else /*default*/
		Ap_Chmode_2="11NAHT40MINUS";
	
	if (strcmp(wlan1_dot11_mode, "11n")==0)
	{
		sprintf(ap_ifnum_2, "1:RF:%s:%s:1", Ap_Primary_Ch_2,  Ap_Chmode_2);
		sprintf(ap_ifnum_4, "1:NO:%s:%s:1", Ap_Primary_Ch_2,  Ap_Chmode_2);
	}
	else
	{
	sprintf(ap_ifnum_2, "1:RF:%s:%s:2", Ap_Primary_Ch_2,  Ap_Chmode_2);
	sprintf(ap_ifnum_4, "1:NO:%s:%s:2", Ap_Primary_Ch_2,  Ap_Chmode_2);
}
}

int rg2Country(int regdomain)
{
	/*
	for ath_dfs.ko
	domainoverride	0=Unitialized (default) => Disable radar detection since we don't have a radar domain
					1=FCC Domain (FCC3, US)
					2=ETSI Domain (Europe)
					3=Japan Domain
	*/
	
	int countryCode=840;
	switch(regdomain)
	{
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
	   case 0x37:	//CTRY_GERMANY
	   case 0x40:	//CTRY_JAPAN
	   	countryCode = 276; 
	   	break;	
	   case 0x10: 
	   case	0x3A://new
	   	countryCode = 840; //CTRY_UNITED_STATES 
	   	break;		   
	   case 0x3B://CTRY_BRAZIL
	   	countryCode = 76;
	   	break;
	   case 0x41:	
	   	countryCode = 393; //CTRY_JAPAN1
	   	break;
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

void save_wlan0_mac_to_nvram(void)
{
	/*
	 * Read WLAN MAC from the wireless card and save it in nvram. We are not saving "lan_mac"
	 * read from nvram as wlan0 MAC, because set_wlan_mac() may fail to set "lan_mac" as the new WLAN MAC. 
         * So, we are always saving the actual MAC on the card to nvram. 
	 * Thus, the correct MAC will be displayed at status page
	 */
	FILE *in;
	char wlan0_mac_on_card[18];

	if (!(in = popen("ifconfig ath0 | grep HWaddr | awk ' {print $5} '", "r"))) {
		exit(1);
	}

	fgets(wlan0_mac_on_card, sizeof(wlan0_mac_on_card), in);

	/* close the pipe */	
	pclose(in);

	nvram_set("wlan0_mac", wlan0_mac_on_card);
}

void set_wlan_mac(void)
{
	char *p_lan_mac=NULL;
	char *p_wan_mac=NULL;
	int  lan_mac[6]={0};
	int  wan_mac[6]={0};
	char wlan0_mac[13]={0}; // Don't forget space for NULL terminator
	char wlan1_mac[13]={0}; // Don't forget space for NULL terminator
	
	DEBUG_MSG("set_wlan_mac: start\n");
	
#ifdef SET_GET_FROM_ARTBLOCK	
	p_lan_mac = artblock_get("lan_mac");
	if(p_lan_mac == NULL)	
		p_lan_mac = nvram_safe_get("lan_mac");
	
	p_wan_mac = artblock_get("wan_mac");
	if(p_wan_mac == NULL)
		p_wan_mac = nvram_safe_get("wan_mac");			
#else		
	p_lan_mac = nvram_safe_get("lan_mac");
	p_wan_mac = nvram_safe_get("wan_mac");
#endif    		
	
	sscanf(p_lan_mac,"%02x:%02x:%02x:%02x:%02x:%02x",
		&lan_mac[0],&lan_mac[1],&lan_mac[2],&lan_mac[3],&lan_mac[4],&lan_mac[5]);
		
	sprintf(wlan0_mac, "%02x%02x%02x%02x%02x%02x",
		lan_mac[0],lan_mac[1],lan_mac[2],lan_mac[3],lan_mac[4],lan_mac[5]);

	_system("ifconfig wifi0 hw ether %s", wlan0_mac);

	/* (Ubicom) We have a single radio. Comment out wifi1 setting.
	sscanf(p_wan_mac,"%02x:%02x:%02x:%02x:%02x:%02x",&wan_mac[0],&wan_mac[1],&wan_mac[2],&wan_mac[3],&wan_mac[4],&wan_mac[5]);
		
	if(wan_mac[5]!= 0xff)							
		sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5]+1);
	else if(wan_mac[4]!= 0xff)
		sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0], wan_mac[1], wan_mac[2], wan_mac[3], wan_mac[4]+1, 0);
	else if(wan_mac[3]!= 0xff)
		sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3]+1, 0, 0);
	else if(wan_mac[2]!= 0xff)
		sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1],wan_mac[2]+1, 0, 0, 0);									
	else if(wan_mac[1]!= 0xff)
		sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0],wan_mac[1]+1, 0, 0, 0, 0);
	else
		sprintf(wlan1_mac, "%02x%02x%02x%02x%02x%02x",wan_mac[0]+1, 0, 0, 0, 0, 0);
	
	_system("ifconfig wifi1 hw ether %s", wlan1_mac);
	*/
	
	DEBUG_MSG("set_wlan_mac: end\n");
}

void modify_ssid(void)
{
	int ath_count=0;
	char ssid[128]={0};
	char ssid_tmp[64]={0};
	char *wlan_ssid=NULL;
	int len=0;
	int c1 = 0;
	int c2 = 0;
	int i=0;
	
	for(i=0; i<4; i++)
	{
		if(wlan_vap[i]==1)
		{
			switch(i)
			{
				case 0:
					wlan_ssid = nvram_safe_get("wlan0_ssid");
					break;
				case 1:
					wlan_ssid = nvram_safe_get("wlan1_ssid");
					break;
				case 2:
					wlan_ssid = nvram_safe_get("wlan0_vap1_ssid");
					break;
				case 3:
					wlan_ssid = nvram_safe_get("wlan1_vap1_ssid");
					break;							
			}
			
			memset(ssid, 0, sizeof(ssid));
			memset(ssid_tmp, 0, sizeof(ssid_tmp));
			c1=0;
			c2=0;
			
			len = strlen(wlan_ssid);
			strncpy(ssid_tmp, wlan_ssid, len);
			
			for(c1 = 0; c1 < len; c1++)
			{
				/*   test ssid = ',.`'~!@#$%^&*()_+[]\:"0-=  */				
				/*   " = 0x22, ` = 0x60,  $ = 0x24,  \ = 0x5c, ' = 0x27,      */
			
				if((ssid_tmp[c1] == 0x22) || (ssid_tmp[c1] == 0x24) ||
				   (ssid_tmp[c1] == 0x5c) || (ssid_tmp[c1] == 0x60))
				{
					ssid[c2] = 0x5c;
					c2++;
					ssid[c2] = ssid_tmp[c1];
				}	
				else
					ssid[c2] = ssid_tmp[c1];
				c2++;		
			}
			
			_system("iwconfig ath%d essid \"%s\"", ath_count, ssid);
			
			/*	NickChou/Albert, 2009.3.25
				Enable all Wireless MulticastPackets change to UnicastPackets.
			*/
#ifdef SUPPORT_IGMP_SNOOPY
			if(nvram_match("multicast_stream_enable", "1")==0)
				_system("iwpriv ath%d mcastenhance 2", ath_count);	
#else
			_system("iwpriv ath%d mcastenhance 2", ath_count);
#endif			
			ath_count++;	
		}
	}	
}

void start_insmod(int country_Code)
{
	DEBUG_MSG("Loading Wireless Modules: country_Code=%d\n", country_Code);
		
	system("insmod /lib/modules/ath_hal.ko");
        system("insmod /lib/modules/wlan.ko");
        system("insmod /lib/modules/ath_rate_atheros.ko");
        system("[ -f \"/lib/modules/ath_dfs.ko\" ] && insmod /lib/modules/ath_dfs.ko");
    
        sleep(1);    
        system("insmod /lib/modules/ath_dev.ko");  
    
        /*
    	NickChou add sleep(1) to prevent
    	ath_pci: Unknown symbol ath_dev_free
		ath_pci: Unknown symbol ath_cancel_timer
		ath_pci: Unknown symbol ath_dev_attach
		ath_pci: Unknown symbol ath_initialize_timer
		ath_pci: Unknown symbol wbuf_alloc
		ath_pci: Unknown symbol ath_timer_is_active
		ath_pci: Unknown symbol ath_iw_attach
		ath_pci: Unknown symbol ath_start_timer
        */
        sleep(1);    
        //NickChou add WPS_ARGS in wlan_ath_ap94.c for register_simple_config_callback
        system("insmod /lib/modules/ath_pci.ko");
    
        system("insmod /lib/modules/wlan_xauth.ko");
        system("insmod /lib/modules/wlan_ccmp.ko");
        system("insmod /lib/modules/wlan_tkip.ko");
        system("insmod /lib/modules/wlan_wep.ko");
        system("insmod /lib/modules/wlan_acl.ko");
        system("insmod /lib/modules/wlan_me.ko");
        system("insmod /lib/modules/ath_pktlog.ko");
   
        _system("iwpriv wifi0 setCountryID %d", country_Code);
    
        /*NickChou add 20090721
        To fix 11n and 11g STA connect to device concurrently,
        then 11n STA throughput will lower.
        */
        system("iwpriv wifi0 limit_legacy 13");
    
        //(Ubicom) We have a single radio. Comment out wifi1 setting.
        //_system("iwpriv wifi1 setCountryID %d", country_Code);
    
        DEBUG_MSG("Loading Wireless Modules: end\n");
}

void wlan_ath_setup(int wifi_num, int ath_count)
{
	
	char *wlan_short_gi=NULL;
	int wlan_ssid_broadcast=0;
	char *wlan_rts_threshold=NULL;
	char *wlan_fragmentation=NULL;
	char *wlan_dtim=NULL;
	int wlan_partition=0;
	char *wlan_wmm_enable=NULL;
	char *wlan_beacon_interval=NULL;
	
	if(wifi_num==0)
	{
		wlan_short_gi=nvram_safe_get("wlan0_short_gi");
		wlan_ssid_broadcast=atoi(nvram_safe_get("wlan0_ssid_broadcast"));
		wlan_rts_threshold=nvram_safe_get("wlan0_rts_threshold");
		wlan_fragmentation = nvram_safe_get("wlan0_fragmentation");
		wlan_dtim = nvram_safe_get("wlan0_dtim");
		wlan_partition = atoi(nvram_safe_get("wlan0_partition"));
		wlan_wmm_enable = nvram_safe_get("wlan0_wmm_enable");
		wlan_beacon_interval = nvram_safe_get("wlan0_beacon_interval");
		    
	}
	else if(wifi_num==1)
	{
		wlan_short_gi=nvram_safe_get("wlan1_short_gi");
		wlan_ssid_broadcast=atoi(nvram_safe_get("wlan1_ssid_broadcast"));
		wlan_rts_threshold=nvram_safe_get("wlan1_rts_threshold");
		wlan_fragmentation = nvram_safe_get("wlan1_fragmentation");
		wlan_dtim = nvram_safe_get("wlan1_dtim");
		wlan_partition = atoi(nvram_safe_get("wlan1_partition"));
		wlan_wmm_enable = nvram_safe_get("wlan1_wmm_enable");
		wlan_beacon_interval = nvram_safe_get("wlan1_beacon_interval");
	}
	
	_system("iwpriv ath%d shortgi %s", ath_count, wlan_short_gi);
	_system("iwpriv ath%d hide_ssid %d", ath_count, !wlan_ssid_broadcast);
	_system("iwpriv ath%d ap_bridge %d", ath_count, !wlan_partition);
	_system("iwpriv ath%d wmm %s", ath_count, wlan_wmm_enable);
	
	if( 20 <= atoi(wlan_beacon_interval) && atoi(wlan_beacon_interval) <= 1000 )
		_system("iwpriv ath%d bintval %s", ath_count, wlan_beacon_interval);
		
	if( 256 <= atoi(wlan_rts_threshold) && atoi(wlan_rts_threshold) <=  2347)
        _system("iwconfig ath%d rts %s", ath_count, wlan_rts_threshold);
				
	if( 256 <= atoi(wlan_fragmentation) && atoi(wlan_fragmentation) <=  2346)
		_system("iwconfig ath%d frag %s", ath_count, wlan_fragmentation);
	    	    
	if( 1 <= atoi(wlan_dtim) && atoi(wlan_dtim) <=  255 )
	    _system("iwpriv ath%d dtim_period %s", ath_count, wlan_dtim);
}

void start_makeVAP(int ath_count, int wds_enable, char *apifnum, char *op_mode)
{
	char *Ifnum=NULL;
	char *set_RF=NULL;
	char *Pri_Ch=NULL;
	char *Ch_Mode=NULL;
	char *pure_mode=NULL;
	char temp_ap_ifnum[30]={0};
	char Freq[10]={0};
	char *wlan0_dot11_mode = nvram_safe_get("wlan0_dot11_mode");	
	
	DEBUG_MSG("start_makeVAP: ath_count=%d, wds_enable=%d, apifnum=%s, op_mode=%s\n",
		ath_count, wds_enable, apifnum, op_mode);
	
	strncpy(temp_ap_ifnum, apifnum, strlen(apifnum));
	
	Ifnum = strtok(temp_ap_ifnum, ":");	
	set_RF = strtok(NULL, ":");	
	Pri_Ch = strtok(NULL, ":");	
	Ch_Mode = strtok(NULL, ":");
	pure_mode = strtok(NULL, ":");
	
	if(Ifnum==NULL || Pri_Ch==NULL || Ch_Mode==NULL)
	{
		printf("start_makeVAP error !!\n");
		return;
	}
	
	DEBUG_MSG("start_makeVAP: Ifnum=%s, set_RF=%s, Pri_Ch=%s, Ch_Mode=%s, pure_mode=%s\n",
		Ifnum, set_RF, Pri_Ch, Ch_Mode, pure_mode);
			
    if(strcmp(op_mode, "AP")==0)
    	_system("wlanconfig ath create wlandev wifi%s wlanmode ap &", Ifnum);
    else if(strcmp(op_mode, "APC")==0)
    	_system("wlanconfig ath create wlandev wifi%s wlanmode sta nosbeacon &", Ifnum);
    		
    sleep(1);//NickChou, Do not remove it, For wait create ath
    
    if(wds_enable==1)
    {
		_system("iwpriv ath%d wds 1", ath_count);
	}
	else
	{
		if(strcmp(op_mode, "APC")==0)
			_system("iwpriv ath%d stafwd 1", ath_count);
	}	

	_system("iwpriv ath%d dbgLVL 0x100", ath_count);
	_system("iwpriv ath%d bgscan 0", ath_count); //Disable Background Scan

	if(strcmp(Pri_Ch, "11na") && strcmp(Pri_Ch, "11ng"))
	    sprintf(Freq, "freq %s", Pri_Ch);

	if(strcmp(set_RF, "RF")==0)
	{    
	    _system("iwpriv ath%d mode %s", ath_count, Ch_Mode);
	    
		if( atoi(pure_mode)<2 ) /* 2 => do not set pure mode */
		{
			if( atoi(pure_mode)==0 )
				_system("iwpriv ath%d pureg 1", ath_count);
			else if( atoi(pure_mode)==1 )	
				_system("iwpriv ath%d puren 1", ath_count);	
		}	
	
	    if(strstr(Ch_Mode, "11NG"))
	        _system("iwpriv wifi%s ForBiasAuto 1", Ifnum);
		
		if(strstr(Ch_Mode, "PLUS"))//channel 1~5
	        _system("iwpriv ath%d extoffset 1", ath_count);
		
		if(strstr(Ch_Mode, "MINUS"))//channel 6~13
	        _system("iwpriv ath%d extoffset -1", ath_count);
		
	    /* 0 is static 20; 1 is dyn 2040; 2 is static 40 */	
	    if(strcmp(Ch_Mode, "11NGHT20")==0 || strcmp(Ch_Mode, "11NAHT20")==0 )
	        _system("iwpriv ath%d cwmmode 0", ath_count);
	    else
	         _system("iwpriv ath%d cwmmode 1", ath_count);		
		
		if(strcmp(op_mode, "AP")==0)
			_system("iwconfig ath%d essid dlink mode master %s", ath_count, Freq);	
		 else if(strcmp(op_mode, "APC")==0)
		 	_system("iwconfig ath%d essid dlink mode managed %s", ath_count, Freq);
		
	    if ( (strcmp(wlan0_dot11_mode, "11bgn")==0 || 
		  strcmp(wlan0_dot11_mode, "11gn")==0  || 
		  strcmp(wlan0_dot11_mode, "11n")==0) )
	    {
	    	DEBUG_MSG("wlan0_dot11_mode = b/g/n mixed\n");
    		set_11bgn_rate(0, ath_count, wlan0_dot11_mode);
	    }
	    else if (strcmp(wlan0_dot11_mode, "11g")==0 || strcmp(wlan0_dot11_mode, "11bg")==0)
	    {
        	DEBUG_MSG("dot11_mode value : bg_only \n");
    		set_11bg_rate(0, ath_count, wlan0_dot11_mode);
	    }		
	    else if (strcmp(wlan0_dot11_mode, "11b")==0)
	    {
        	DEBUG_MSG("dot11_mode value : b_only \n");
    		set_11b_rate(0, ath_count, wlan0_dot11_mode);
	    }		
	    else
	    {
	    	DEBUG_MSG("dot11_mode value error. Default: b/g/n mixed\n");
    		set_11bgn_rate(0, ath_count, wlan0_dot11_mode);
	    }		   
		
		_system("ifconfig ath%d txqueuelen 250", ath_count);	
		_system("ifconfig wifi%s txqueuelen 250", Ifnum);
		_system("iwpriv wifi%s AMPDU 1", Ifnum);
		_system("iwpriv wifi%s AMPDUFrames 32", Ifnum);
		_system("iwpriv wifi%s txchainmask 3", Ifnum);
		_system("iwpriv wifi%s rxchainmask 3", Ifnum);
		_system("iwpriv wifi%s AMPDULim 50000", Ifnum);
		system("echo 1 > /proc/sys/dev/ath/htdupieenable");
	    
	   	if(strcmp(Ifnum, "0")==0)
			wlan_ath_setup(0, ath_count);
		else if(strcmp(Ifnum, "1")==0)
			wlan_ath_setup(1, ath_count);
	}
	else
	{
	    _system("iwpriv ath%d mode %s", ath_count, Ch_Mode);
		_system("iwconfig ath%d essid dlink mode master %s", ath_count, Freq);
	}
	
	DEBUG_MSG("start_makeVAP: end\n");
}

void create_topo_file(int wps_enable)
{ 
	FILE *fp=NULL;
	int bss_index=0;
				
	fp = fopen("/var/tmp/topology.conf", "w+");
	
	if(fp==NULL)
    {
    	printf("create_topo_file: create topology.conf : fail\n");
    	return;
    }
	
	fprintf(fp, "bridge br0\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tipaddress %s\n", nvram_safe_get("lan_ipaddr"));
	fprintf(fp, "\tipmask %s\n", nvram_safe_get("lan_netmask"));
    fprintf(fp, "\tinterface %s\n", nvram_safe_get("lan_eth"));      
    for(bss_index=0; bss_index<4; bss_index++)
    {
    	 if(use_hostapd[bss_index]!=4)
    	 {
    	 	DEBUG_MSG("create_topo_file: use_hostapd[%d]=ath%d\n", bss_index, use_hostapd[bss_index]);
    	 	fprintf(fp, "\tinterface ath%d\n", use_hostapd[bss_index]);
    	 }	
    }   
    fprintf(fp, "}\n");
		    
    if(use_hostapd[0]!=4 || use_hostapd[2]!=4)
    {
    	fprintf(fp, "radio wifi0\n");
    	fprintf(fp, "{\n");
    	fprintf(fp, "\tap\n");
    	fprintf(fp, "\t{\n");
    	if(use_hostapd[0]!=4)
    	{
			fprintf(fp, "\t\tbss ath%d\n", use_hostapd[0]);
			fprintf(fp, "\t\t{\n");
			if(wps_enable==1)
           		fprintf(fp, "\t\t\tconfig /var/tmp/WSC_ath%d.conf\n", use_hostapd[0]);
        	else
        		fprintf(fp, "\t\t\tconfig /var/tmp/secath%d\n", use_hostapd[0]); 	
        	fprintf(fp, "\t\t}\n");
        }
        
        if(use_hostapd[2]!=4)	
        {
			fprintf(fp, "\t\tbss ath%d\n", use_hostapd[2]);
			fprintf(fp, "\t\t{\n");
			fprintf(fp, "\t\t\tconfig /var/tmp/secath%d\n", use_hostapd[2]);
	        fprintf(fp, "\t\t}\n");
        }   
		fprintf(fp, "\t}\n");
        fprintf(fp, "}\n");
	}
	
	if(use_hostapd[1]!=4 || use_hostapd[3]!=4)
	{
		fprintf(fp, "radio wifi1\n");
    	fprintf(fp, "{\n");
    	fprintf(fp, "\tap\n");
    	fprintf(fp, "\t{\n");
    	if(use_hostapd[1]!=4)
    	{
    		fprintf(fp, "\t\tbss ath%d\n", use_hostapd[1]);
			fprintf(fp, "\t\t{\n");
        	if(wps_enable==1)
           		fprintf(fp, "\t\t\tconfig /var/tmp/WSC_ath%d.conf\n", use_hostapd[1]);
        	else
        		fprintf(fp, "\t\t\tconfig /var/tmp/secath%d\n",  use_hostapd[1]); //ath0 or ath1 	
        	fprintf(fp, "\t}\n");
        }
        
        if(use_hostapd[3]!=4)
        {
			fprintf(fp, "\t\tbss ath%d\n", use_hostapd[3]);
			fprintf(fp, "\t\t{\n");
			fprintf(fp, "\t\t\tconfig /var/tmp/secath%d\n", use_hostapd[3]);
	        fprintf(fp, "\t\t}\n");
        }   
		fprintf(fp, "\t}\n");
        fprintf(fp, "}\n");
    }    
    
    fclose(fp); 
}


/////////////////////////////////////// APC WPA/WPS topology file ///////////////////////////////////////
////
////
static void create_APC_wpa_conf_file_NetworkSection(FILE *fp) {
	char *wlan0_security = 0;
	char *wlan0_psk_pass_phrase = 0;
	char *wlan0_psk_cipher_type = 0;

	if (!fp) return;

	wlan0_security = nvram_safe_get("wlan0_security");
	wlan0_psk_pass_phrase = nvram_safe_get("wlan0_psk_pass_phrase");
	wlan0_psk_cipher_type = nvram_safe_get("wlan0_psk_cipher_type");

	fprintf(fp, "ap_scan=1\n");
	fprintf(fp, "network={\n");
	{
		fprintf(fp, " disabled=0\n");    //1:Disable this network block
		fprintf(fp, " ssid=\"%s\"\n", nvram_safe_get("wlan0_ssid"));
		fprintf(fp, " scan_ssid=0\n");   //SSID-specific Probe Request
		fprintf(fp, " priority=0\n");    //same to all
		fprintf(fp, " mode=0\n");        //Infrastructure

		// NONE security
		if (!strcmp(wlan0_security, "disable")) {
			fprintf(fp, " key_mgmt=NONE\n");
		}else {
			// WEP
			if (strstr(wlan0_security, "wep")) {
				int keyidx=0, ascii=0;
				fprintf(fp, " key_mgmt=NONE\n");
				if (strstr(wlan0_security, "share")) {
					fprintf(fp, " auth_alg=SHARED\n");
				}else {
					fprintf(fp, " auth_alg=OPEN\n");
				}
				keyidx = atoi(nvram_safe_get("wlan0_wep_default_key"))-1;
				fprintf(fp, " wep_tx_keyidx=%d\n", (0>keyidx)?0:((3<keyidx)?3:keyidx)); //0~3
				if (0==nvram_match("wlan0_wep_display", "ascii")) {
					ascii = 1;
				}
				if (strstr(wlan0_security, "64")) {
					fprintf(fp, " wep_key0=%s\n", nvram_safe_get("wlan0_wep64_key_1"));
					fprintf(fp, " wep_key1=%s\n", nvram_safe_get("wlan0_wep64_key_2"));
					fprintf(fp, " wep_key2=%s\n", nvram_safe_get("wlan0_wep64_key_3"));
					fprintf(fp, " wep_key3=%s\n", nvram_safe_get("wlan0_wep64_key_4"));
				}else if (strstr(wlan0_security, "128")) {
					fprintf(fp, " wep_key0=%s\n", nvram_safe_get("wlan0_wep128_key_1"));
					fprintf(fp, " wep_key1=%s\n", nvram_safe_get("wlan0_wep128_key_2"));
					fprintf(fp, " wep_key2=%s\n", nvram_safe_get("wlan0_wep128_key_3"));
					fprintf(fp, " wep_key3=%s\n", nvram_safe_get("wlan0_wep128_key_4"));
				}
			}
			// WPA-PSK or WPA-EAP or ...
			else {
				if (strstr(wlan0_security, "psk")) {
					fprintf(fp, " key_mgmt=WPA-PSK\n");
					if (strlen(wlan0_psk_pass_phrase) == 64) { /*64 HEX*/
						fprintf(fp, " psk=%s\n", wlan0_psk_pass_phrase);
					}else { /*8~63 ASCII*/
						fprintf(fp, " psk=\"%s\"\n", wlan0_psk_pass_phrase);
					}
				}

				if (strstr(wlan0_security, "wpa_")) {
					fprintf(fp, " proto=WPA\n");
				}else if (strstr(wlan0_security, "wpa2_")) {
					fprintf(fp, " proto=WPA2\n");
				}else if (strstr(wlan0_security, "wpa2auto_")) {
					fprintf(fp, " proto=WPA WPA2\n");
				}

				if (strcmp(wlan0_psk_cipher_type, "tkip")== 0) {
					fprintf(fp, " pairwise=TKIP\n");
				}else if (strcmp(wlan0_psk_cipher_type, "aes")== 0) {
					fprintf(fp, " pairwise=CCMP\n");
				}else {
					fprintf(fp, " pairwise=CCMP TKIP\n");
				}
			}
		}
	}
	fprintf(fp, "}\n");
}


static void create_APC_wpa_conf_file_WpsSection(FILE *fp) {
	if (!fp) return;

	fprintf(fp, "update_config=1\n");
	fprintf(fp, "wps_property={\n");
	{
		fprintf(fp, " wps_disable=0\n");
		fprintf(fp, " wps_upnp_disable=1\n");
		fprintf(fp, " version=0x10\n");
//		uuid=
		fprintf(fp, " auth_type_flags=0x0023\n"); //OPEN/WPAPSK/WPA2PSK
		fprintf(fp, " encr_type_flags=0x000f\n"); //NONE/WEP/TKIP/AES
		fprintf(fp, " conn_type_flags=0x1\n");    //ESS
		fprintf(fp, " config_methods=0x84\n");    //PBC/PIN
		fprintf(fp, " wps_state=0x01\n");         //not configured
		fprintf(fp, " rf_bands=0x01\n");          //1:(2.4G), 2:(5G), 3:(2.4G+5G)
		fprintf(fp, " manufacturer=\"%s\"\n",  nvram_safe_get("manufacturer"));
		fprintf(fp, " model_name=\"%s\"\n",    nvram_safe_get("model_name"));
		fprintf(fp, " model_number=\"%s\"\n",  nvram_safe_get("model_number"));
		fprintf(fp, " serial_number=\"%s\"\n", nvram_safe_get("serial_number"));
		fprintf(fp, " dev_category=1\n");         //Computer
		fprintf(fp, " dev_sub_category=1\n");     //PC
		fprintf(fp, " dev_oui=0050f204\n");
		fprintf(fp, " dev_name=\"%s\"\n",      nvram_safe_get("model_name"));
		fprintf(fp, " os_version=0x00000001\n");
	}
	fprintf(fp, "}\n");
}


#define APC_WSC_CONF_FILE  "/var/etc/stafwd-wsc.conf"
static void create_APC_wpa_conf_file(void) {
	FILE *fp=fopen(APC_WSC_CONF_FILE, "w+");
	if (!fp) {
		DEBUG_MSG("create_APC_wsc_conf: create %s fail\n", APC_WSC_CONF_FILE);
		return;
	}

	fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");

	//SECTION: network={}
	create_APC_wpa_conf_file_NetworkSection(fp);

	//SECTION: wps_property={}
	if (0==nvram_match("wps_enable", "1"))
		create_APC_wpa_conf_file_WpsSection(fp);

	fclose(fp);
//	system("cat /var/etc/stafwd-wsc.conf");printf("\n");
}


static void create_APC_topo_file() {
	FILE *fp=fopen("/var/tmp/stafwd-topology.conf", "w+");
	if(fp==NULL) {
		DEBUG_MSG("create_topo_file: create stafwd-topology.conf : fail\n");
		return;
	}

	fprintf(fp, "bridge %s\n", nvram_safe_get("lan_bridge"));
	fprintf(fp, "{\n");
	fprintf(fp, "\tipaddress %s\n", nvram_safe_get("lan_ipaddr"));
	fprintf(fp, "\tipmask %s\n", nvram_safe_get("lan_netmask"));
	fprintf(fp, "\tinterface %s\n", nvram_safe_get("wlan0_eth"));
	fprintf(fp, "\tinterface %s\n", nvram_safe_get("lan_eth"));
	fprintf(fp, "}\n");

	fprintf(fp, "radio wifi0\n");
	fprintf(fp, "{\n");
	fprintf(fp, "\tsta %s\n", nvram_safe_get("wlan0_eth"));
	fprintf(fp, "\t{\n");
	fprintf(fp, "\t\t\tconfig %s\n", APC_WSC_CONF_FILE);
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");

	fclose(fp);
//	system("cat /var/tmp/stafwd-topology.conf");printf("\n");
}
////
////
/////////////////////////////////////// APC WPA/WPS topology file ///////////////////////////////////////


int start_wlan(void)
{		
	int wlan0_wds_enable=0;
	int wlan1_wds_enable=0;
	char *lan_bridge=NULL;
	char *lan_eth=NULL;
	char *wlan0_domain=NULL;
	int country_Code=840;
	int ath_count=0;
	char *wlan0_op_mode = nvram_safe_get("wlan0_op_mode");
	
	if(!( action_flags.all  || action_flags.wlan ) ) 
		return 1;	
		
	//For safety, delete all /var/tmp nodes we may re-create
	system("rm -rf /var/tmp/sec*");
	system("rm -rf /var/tmp/topo*");
	system("rm -rf /var/tmp/WSC_ath*");
		
	lan_bridge = nvram_safe_get("lan_bridge");
	lan_eth = nvram_safe_get("lan_eth");
		
	/* Add loopback */
	ifconfig_up("lo", "127.0.0.1", "255.0.0.0");
	route_add("lo", "127.0.0.0", "255.0.0.0", "0.0.0.0", 0);
	
	/* Run 2.4G or 5G */
	if(nvram_match("wlan0_enable", "1")==0)
	{
		printf("##################### wlan0_enable#########################\n");
		wlan0_enable = 1;			
		if(nvram_match("wlan0_vap1_enable", "1")==0)
		{
			printf("##################### wlan0_vap1_enable####################\n");
			wlan0_vap_enable = 1;
		}				
	}	
	
	if(nvram_match("wlan1_enable", "1")==0)
	{
		printf("##################### wlan1_enable#########################\n");
		wlan1_enable = 1;
		if(nvram_match("wlan1_vap1_enable", "1")==0)
		{
			printf("##################### wlan1_vap1_enable#########################\n");
			wlan1_vap_enable = 1;	
		}			
	}
		
	if(wlan0_enable || wlan1_enable)
	{
		stop_wlan_flag = 1;
				
#ifdef SET_GET_FROM_ARTBLOCK
		wlan0_domain = artblock_get("wlan0_domain");
		if(wlan0_domain != NULL)		
			country_Code=rg2Country(strtol(wlan0_domain, NULL, 16));
		else
			country_Code=rg2Country(strtol(nvram_safe_get("wlan0_domain"), NULL, 16));			
#else
		country_Code=rg2Country(strtol(nvram_safe_get("wlan0_domain"), NULL, 16));
#endif	

		if( (wlan0_enable + wlan1_enable == 1) && (!wlan0_vap_enable) && (!wlan1_vap_enable) )	
		{	/*standard or rootap => only 2.4G or 5G*/
			DEBUG_MSG("standard or rootap => only 2.4G or 5G\n");
			if(wlan0_enable)
			{
				wlan_vap[0] = 1;
				wlan0_wds_enable = atoi(nvram_safe_get("wlan0_wds_enable"));	
			}
			else	/*wlan1_enable*/
			{
				wlan1_wds_enable = atoi(nvram_safe_get("wlan1_wds_enable"));
				wlan_vap[1] = 1;
			}
		}
		else	/* multi(dual) */
		{
			if(wlan0_enable)
			{	
				wlan_vap[0] = 1;
				if(wlan0_vap_enable)
					wlan_vap[2] = 1;
			}
			
			if(wlan1_enable)
			{
				wlan_vap[1] = 1;
				if(wlan1_vap_enable)
					wlan_vap[3] = 1;
			}		
		}
		
		start_insmod(country_Code);
		
		set_wlan_mac();
					
		if(wlan0_enable)
		{
			if(strlen(wlan0_op_mode)==0)
				wlan0_op_mode="AP";
				
			wlan0_ifnum_setup(wlan0_op_mode);  // 11bgn VAP setting									
			start_makeVAP(ath_count, wlan0_wds_enable, ap_ifnum, wlan0_op_mode);
			
			ath_count++;
		}
		
		if(wlan1_enable)
		{
			wlan1_ifnum_setup();  // 11a VAP setting						
			start_makeVAP(ath_count, wlan1_wds_enable, ap_ifnum_2, "AP");
			ath_count++;	
		}
		
		if(wlan0_enable && wlan0_vap_enable)
		{
			start_makeVAP(ath_count, 0, ap_ifnum_3, "AP");
			ath_count++;
		}
		
		if(wlan1_enable && wlan1_vap_enable)
		{
			start_makeVAP(ath_count, 0, ap_ifnum_4, "AP");
		}

		save_wlan0_mac_to_nvram();
		
		modify_ssid();//NickChou, add for special char
		
		start_ap();
		
		//NickChou, keep br0 MAC = lan_MAC, not wifi0 MAC or wifi1 MAC
		_system("brctl delif %s %s &", lan_bridge, lan_eth);
		sleep(1);
		_system("brctl addif %s %s &", lan_bridge, lan_eth);
		
		//ensure all nodes are updated on the network
		_system("arping -U -c 1 -I %s %s &", lan_bridge, nvram_safe_get("lan_ipaddr"));	

		system("gpio WLAN_LED blink &");
	}
	else
	{
		printf("******* Wireless Radio OFF *********\n");	
		stop_wlan_flag = 0;
	}	
	
#ifdef CONFIG_USER_WIRELESS_SCHEDULE	
	KILL_APP_ASYNCH("-SIGTSTP timer");
#endif	
	
	printf("#################################Start WLAN Finished############################\n");
        return 1;
}

void start_activateVAP(int ath_count, int guest_zone, int netusb_guest_zone)
{
	char *bridge=nvram_safe_get("lan_bridge");
	char *lan_ipadr=nvram_safe_get("lan_ipaddr");
	
	DEBUG_MSG("start_activateVAP: ath_count=%d, guest_zone=%d, netusb_guest_zone=%d\n", 
		ath_count, guest_zone, netusb_guest_zone);
	
	_system("ifconfig ath%d up", ath_count);
	_system("brctl addif %s ath%d", bridge, ath_count);
	
	//guest_zone=1 => Disable Disable Routing between LAN
	//netusb_guest_zone=1 => Guest Zone support NetWork USB Utility
	//setguestzone <bridge> <device>:<Enable_SharePort>:<Enable_RouteInLan> <bridge ip>

	if(guest_zone==1)
	{
		if(netusb_guest_zone==1)
			_system("brctl setguestzone %s ath%d:1:0 %s", bridge, ath_count, lan_ipadr); 
		else if(netusb_guest_zone==0)
			_system("brctl setguestzone %s ath%d:0:0 %s", bridge, ath_count, lan_ipadr);
		else
			_system("brctl setguestzone %s ath%d:1:0 %s", bridge, ath_count, lan_ipadr);
	}
	else if(guest_zone ==0)
	{
		if(netusb_guest_zone==1)
			_system("brctl setguestzone %s ath%d:1:1 %s", bridge, ath_count, lan_ipadr);
		else if(netusb_guest_zone==0)
			_system("brctl setguestzone %s ath%d:0:1 %s", bridge, ath_count, lan_ipadr);
		else
			_system("brctl setguestzone %s ath%d:1:1 %s", bridge, ath_count, lan_ipadr);
	}
}

int set_mac_filter(int bss_index, int ath_count)
{
	char mac_filter_rule[15] = {0};
	char mac_filter_value[64] = {0};
	char *filter_enable=NULL, *filter_name=NULL, *mac=NULL, *schedule=NULL;
	int i=0, maccmd_flag=0;
	char *mac_filter_type=nvram_safe_get("mac_filter_type");
	char *p_mac_filter_value=NULL;

	/*Flush the ACL database*/
	_system("iwpriv ath%d maccmd 3", ath_count);

	if( (strcmp(mac_filter_type, "list_allow") == 0)  || (strcmp(mac_filter_type, "list_deny") == 0) )
	{
		for(i=0; i<MAX_MAC_FILTER_NUMBER; i++)
		{
			snprintf(mac_filter_rule, sizeof(mac_filter_rule), "mac_filter_%02d",i);
			p_mac_filter_value=nvram_safe_get(mac_filter_rule);
				
			if ( strcmp(p_mac_filter_value, "") == 0 )
				continue;
			else
				strcpy(mac_filter_value, p_mac_filter_value);

			if( getStrtok(mac_filter_value, "/", "%s %s %s %s", &filter_enable, &filter_name, &mac, &schedule) !=4 )
			{
				strcpy(mac_filter_value, p_mac_filter_value);
				/* for old NVRAM setting */
				if( getStrtok(mac_filter_value, "/", "%s %s %s", &filter_name, &mac, &schedule) == 3 )
					filter_enable = "1";
				else
					continue;
			}

			if(mac == NULL)
			{
				printf("wlan mac format not correct\n");
				continue;   /* return; */
			}
				
			if ( strcmp(mac, "") == 0 || strcmp(mac, "00:00:00:00:00:00") == 0)
				continue;
					
			if(strcmp(filter_enable, "1")==0)
			{
				_system("iwpriv ath%d addmac %s", ath_count, mac);
				DEBUG_MSG("set_mac_filter: set %s in ath%d macfilter\n", mac, ath_count);	
				maccmd_flag=1;
			}	
		}
		
		if ( maccmd_flag && (strcmp(mac_filter_type, "list_allow") == 0) )
		{
			_system("iwpriv ath%d maccmd 1", ath_count);
			DEBUG_MSG("set_mac_filter: ath%d macfilter is allow\n", ath_count);	
		}
		else
		{
			_system("iwpriv ath%d maccmd 2", ath_count);
			DEBUG_MSG("set_mac_filter: ath%d macfilter is deny\n", ath_count);	
		}	
		
	}
	else
		_system("iwpriv ath%d maccmd 0", ath_count);
			
	return 1;		
}

int start_ap(void)
{
    int bss_index;
    int ath_count = 0;
    
    if(!( action_flags.all || action_flags.wlan || action_flags.firewall ) )
        return 1;
        
    DEBUG_MSG("start_ap start\n");	
      	  	
	/*bring up Multi_BSS*/
	/*
		wlan_vap[0] must be ath0
		wlan_vap[1] may be ath0 or ath1
		wlan_vap[2] may be ath1 or ath2
		wlan_vap[3] may be ath1 or ath2 or ath3
		
		ath must increate by order, ex, we don't allow ath0 ath2 ath3. 
	*/  
	for(bss_index = 0; bss_index<4; bss_index++)
    {
	    if(wlan_vap[bss_index] == 0)
	    	continue;

#ifdef CONFIG_USER_WISH	    
	    /* wish */
	    //set_wish(bss_index, ath_count);
#endif	   	    
      	
      	switch(bss_index)
	    {
	    	case 0:
	    		start_activateVAP(ath_count, 2, 2);
	    		break;
	    	case 1:
	    		start_activateVAP(ath_count, 2, 2);
	    		break;
	    	case 2:
#ifdef CONFIG_USER_WL_ATH_GUEST_ZONE
				start_activateVAP(ath_count, atoi(nvram_safe_get("wlan0_vap_guest_zone")), atoi(nvram_safe_get("netusb_guest_zone")));					
#else
				start_activateVAP(ath_count, 2, 2);
#endif						
	    		break;
	    	case 3:
#ifdef CONFIG_USER_WL_ATH_GUEST_ZONE	
				start_activateVAP(ath_count, atoi(nvram_safe_get("wlan1_vap_guest_zone")), atoi(nvram_safe_get("netusb_guest_zone")));	
#else
				start_activateVAP(ath_count, 2, 2);
#endif						
	    		break;			
	    }
	    
	    set_security(bss_index, ath_count);
	    
	    set_mac_filter(bss_index, ath_count);
	    
	    ath_count++;  // ath0->ath1->ath2->ath3  	
	}
	  
	DEBUG_MSG("start_ap end\n");	
       
    return 0;
}

static void set_wep_key(int len, int wep_authmode, int bss_index, int ath_count)
{
	int i;
	char *wlan_wep64_key_1=NULL;
    char *wlan_wep64_key_2=NULL;
    char *wlan_wep64_key_3=NULL;
    char *wlan_wep64_key_4=NULL;
    char *wlan_wep128_key_1=NULL;
    char *wlan_wep128_key_2=NULL;
    char *wlan_wep128_key_3=NULL;
    char *wlan_wep128_key_4=NULL;
    unsigned char *wlan_wep_default_key=NULL; 
    char *wepkey_1=NULL;
	char *wepkey_2=NULL;
	char *wepkey_3=NULL;
	char *wepkey_4=NULL;
    
    DEBUG_MSG("set_wep_key: wlan_vap[%d]: ath%d\n", bss_index, ath_count);
     
    switch(len)
    {
    	case 64:
    		switch(bss_index)	
    		{	    		
	    		case 0:/*set 2.4GHz (wlan_vap[0]) WEP*/
	    			wlan_wep64_key_1 = nvram_safe_get("wlan0_wep64_key_1");
			    	wlan_wep64_key_2 = nvram_safe_get("wlan0_wep64_key_2");    
			    	wlan_wep64_key_3 = nvram_safe_get("wlan0_wep64_key_3");
			    	wlan_wep64_key_4 = nvram_safe_get("wlan0_wep64_key_4");
					break;
				case 1:/*set 5GHz (wlan_vap[1]) WEP*/
					wlan_wep64_key_1 = nvram_safe_get("wlan1_wep64_key_1");
				    wlan_wep64_key_2 = nvram_safe_get("wlan1_wep64_key_2");    
				    wlan_wep64_key_3 = nvram_safe_get("wlan1_wep64_key_3");
				    wlan_wep64_key_4 = nvram_safe_get("wlan1_wep64_key_4");
				    break;
				case 2:/*set 2.4GHz guest (wlan_vap[2]) WEP*/
					wlan_wep64_key_1 = nvram_safe_get("wlan0_vap1_wep64_key_1");
				    wlan_wep64_key_2 = nvram_safe_get("wlan0_vap1_wep64_key_2");    
				    wlan_wep64_key_3 = nvram_safe_get("wlan0_vap1_wep64_key_3"); 
				    wlan_wep64_key_4 = nvram_safe_get("wlan0_vap1_wep64_key_4");
				    break;
				case 3:/*set 5GHz guest (wlan_vap[3]) WEP*/
					wlan_wep64_key_1 = nvram_safe_get("wlan1_vap1_wep64_key_1");
				    wlan_wep64_key_2 = nvram_safe_get("wlan1_vap1_wep64_key_2");    
				    wlan_wep64_key_3 = nvram_safe_get("wlan1_vap1_wep64_key_3");
				    wlan_wep64_key_4 = nvram_safe_get("wlan1_vap1_wep64_key_4");
				    break;    	    
			}   
		    
		   	for(i=1; i<5; i++)
            {
			    switch(i)
            	{            
			    	case 1:
			    		wepkey_1=wlan_wep64_key_1;
			    		break;
			    	case 2:
			    		wepkey_2=wlan_wep64_key_2;
			    		break;
			    	case 3:
			    		wepkey_3=wlan_wep64_key_3;
			    		break;
			    	case 4:
			    		wepkey_4=wlan_wep64_key_4;
			    		break;		
            	}
            }
                
        	break;
        	   
    	case 128:
    		switch(bss_index)
    		{
	    		case 0:
		    		wlan_wep128_key_1 = nvram_safe_get("wlan0_wep128_key_1");
				    wlan_wep128_key_2 = nvram_safe_get("wlan0_wep128_key_2");    
				    wlan_wep128_key_3 = nvram_safe_get("wlan0_wep128_key_3");
				    wlan_wep128_key_4 = nvram_safe_get("wlan0_wep128_key_4"); 
					break;
				case 1:	
			    	wlan_wep128_key_1 = nvram_safe_get("wlan1_wep128_key_1");
				    wlan_wep128_key_2 = nvram_safe_get("wlan1_wep128_key_2");    
				    wlan_wep128_key_3 = nvram_safe_get("wlan1_wep128_key_3");
				    wlan_wep128_key_4 = nvram_safe_get("wlan1_wep128_key_4"); 
				    break;
				case 2:	
			    	wlan_wep128_key_1 = nvram_safe_get("wlan0_vap1_wep128_key_1");
				    wlan_wep128_key_2 = nvram_safe_get("wlan0_vap1_wep128_key_2");    
				    wlan_wep128_key_3 = nvram_safe_get("wlan0_vap1_wep128_key_3");
				    wlan_wep128_key_4 = nvram_safe_get("wlan0_vap1_wep128_key_4"); 
				    break;
				case 3:	
			    	wlan_wep128_key_1 = nvram_safe_get("wlan1_vap1_wep128_key_1");
				    wlan_wep128_key_2 = nvram_safe_get("wlan1_vap1_wep128_key_2");    
				    wlan_wep128_key_3 = nvram_safe_get("wlan1_vap1_wep128_key_3");
				    wlan_wep128_key_4 = nvram_safe_get("wlan1_vap1_wep128_key_4"); 
				    break;        
		    } 
             
		    for(i=1; i<5; i++)
            {
		   		switch(i)
            	{            
			    	case 1:
			    		wepkey_1 = wlan_wep128_key_1;
			    		break;
			    	case 2:
			    		wepkey_2 = wlan_wep128_key_2;
			    		break;
			    	case 3:
			    		wepkey_3 = wlan_wep128_key_3;
			    		break;
			    	case 4:
			    		wepkey_4 = wlan_wep128_key_4;
			    		break;		
            	}
            }
                
        	break;      	    
    	default:
    		 DEBUG_MSG("WEP key len is not correct. \n");
    }
	 
    switch(bss_index)
    {
    	case 0:
    		wlan_wep_default_key = nvram_safe_get("wlan0_wep_default_key");
    		break;
    	case 1:	
    		wlan_wep_default_key = nvram_safe_get("wlan1_wep_default_key");	
    		break;
    	case 2:
    		wlan_wep_default_key = nvram_safe_get("wlan0_vap1_wep_default_key");	
    		break;
    	case 3:
    		wlan_wep_default_key = nvram_safe_get("wlan1_vap1_wep_default_key");	
    		break;	
    }
	 
    if((atoi(wlan_wep_default_key) < 1)|| (atoi(wlan_wep_default_key) > 4)   )
    {
        DEBUG_MSG("wep_default_key not correct. Default : 1. \n");
        wlan_wep_default_key = "1";
    }
        
	_system("iwpriv ath%d authmode %d", ath_count, wep_authmode);
	_system("iwconfig ath%d key [1] %s", ath_count, wepkey_1);
	_system("iwconfig ath%d key [2] %s", ath_count, wepkey_2);
	_system("iwconfig ath%d key [3] %s", ath_count, wepkey_3);
	_system("iwconfig ath%d key [4] %s", ath_count, wepkey_4);
	_system("iwconfig ath%d key [%s]", ath_count, wlan_wep_default_key);
}

static void set_security(int bss_index, int ath_count)
{
	char *wlan_security=NULL;
	int wps_enable=0;
	char *wlan0_op_mode = nvram_safe_get("wlan0_op_mode");

	if(nvram_match("wps_enable", "1") == 0)
		wps_enable=1;

	if(strlen(wlan0_op_mode)==0)
		wlan0_op_mode="AP";

	switch(bss_index)
	{
		case 0:
			wlan_security = nvram_safe_get("wlan0_security");
			break;
		case 1:
			wlan_security = nvram_safe_get("wlan1_security");
			break;
		case 2:
			wlan_security = nvram_safe_get("wlan0_vap1_security");
			break;
		case 3:
			wlan_security = nvram_safe_get("wlan1_vap1_security");
			break;
	}

	DEBUG_MSG("set_security: wlan_vap[%d]: ath%d: security=%s\n", bss_index, ath_count, wlan_security);

	if((strcmp(wlan_security,"disable") == 0))
	{
		if(strcmp(wlan0_op_mode, "AP")==0)
		{
			if(wps_enable==1 && (bss_index==0 || bss_index==1))
			{
				use_hostapd[bss_index]=ath_count;
				create_wsc_conf(bss_index, ath_count, wps_enable, 0);
			}
			else
				use_hostapd[bss_index]=4;//GuestZone do not support WPS
		}
	}
	else if((strcmp(wlan_security,"wep_open_64") == 0))
	{
		set_wep_key(64, 4, bss_index, ath_count);
		if(strcmp(wlan0_op_mode, "AP")==0)
		{
			if(wps_enable==1 && (bss_index==0 || bss_index==1))
			{
				use_hostapd[bss_index]=ath_count;
				create_wsc_conf(bss_index, ath_count, wps_enable, 64);
			}
			else
				use_hostapd[bss_index]=4;//GuestZone do not support WPS, so bss do not use hostapd
		}
	}
	else if((strcmp(wlan_security,"wep_share_64") == 0))
	{
		set_wep_key(64, 2, bss_index, ath_count);
		if(strcmp(wlan0_op_mode, "AP")==0)
		{
			if(wps_enable==1 && (bss_index==0 || bss_index==1))
			{
				use_hostapd[bss_index]=ath_count;
				create_wsc_conf(bss_index, ath_count, wps_enable, 64);
			}
			else
				use_hostapd[bss_index]=4;
		}
	}       
	else if((strcmp(wlan_security,"wep_open_128") == 0))
	{
		set_wep_key(128, 4, bss_index, ath_count);
		if(strcmp(wlan0_op_mode, "AP")==0)
		{
			if(wps_enable==1 && (bss_index==0 || bss_index==1))
			{
				use_hostapd[bss_index]=ath_count;
				create_wsc_conf(bss_index, ath_count, wps_enable, 128);
			}
			else
				use_hostapd[bss_index]=4;
		}
	}
	else if((strcmp(wlan_security,"wep_share_128") == 0))  
	{
		set_wep_key(128, 2, bss_index, ath_count);
		if(strcmp(wlan0_op_mode, "AP")==0)
		{
			if(wps_enable==1 && (bss_index==0 || bss_index==1))
			{
				use_hostapd[bss_index]=ath_count;
				create_wsc_conf(bss_index, ath_count, wps_enable, 128);
			}
			else
				use_hostapd[bss_index]=4;
		}
	}
	else if((strcmp(wlan_security,"wpa_psk") == 0) ||
	        (strcmp(wlan_security,"wpa2_psk") == 0)||
	        (strcmp(wlan_security,"wpa2auto_psk") == 0))
	{   
		if(strcmp(wlan0_op_mode, "AP")==0)
		{
			use_hostapd[bss_index]=ath_count;
			create_wsc_conf(bss_index, ath_count, wps_enable, 0);
		}
	}
	else if((strcmp(wlan_security,"wpa_eap") == 0) || 
		(strcmp(wlan_security,"wpa2_eap") == 0)|| 
		(strcmp(wlan_security,"wpa2auto_eap") == 0))
	{
		if(strcmp(wlan0_op_mode, "AP")==0)
		{
			use_hostapd[bss_index]=ath_count;
			create_wsc_conf(bss_index, ath_count, wps_enable, 0);
		}
	}
}

#ifdef CONFIG_USER_WISH
static void set_wish_rule(int bss_index, int ath_count)
{
    char *getValue, wish_value[256]={};
	char wish_rule[]="wish_rule_XXXX";
	unsigned char *prio, *uplink, *downlink;
	char *enable, *name, *proto, *src_ip_start, *src_ip_end, *src_port_start, *src_port_end;
	char *dst_ip_start, *dst_ip_end, *dst_port_start, *dst_port_end;		
	unsigned int i;
                  
    for(i=0; i<MAX_WISH_NUMBER; i++) 
    {
		snprintf(wish_rule, sizeof(wish_rule), "wish_rule_%02d",i) ;
		getValue = nvram_get(wish_rule);
		if ( getValue == NULL ||  strlen(getValue) == 0 ) 
			continue;
		else 
			strncpy(wish_value, getValue, sizeof(wish_value));	
		
		if( getStrtok(wish_value, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s %s", &enable, &name, &prio, &uplink, &downlink, &proto, &src_ip_start, &src_ip_end, \
							&src_port_start, &src_port_end, &dst_ip_start, &dst_ip_end, &dst_port_start, &dst_port_end) !=14 )
				continue;
        // index, enable, priority, protocol, 
        // host1_start_ip, host1_end_ip, host2_start_ip, host2_end_ip,
        // host1_start_port, host1_end_port, host2_start_port, host2_end_port    

		if( strcmp("1", enable)==0 )  {	
			 _system("iwpriv ath%d wish_rule %02x%02x%02x%04x%08x%08x%08x%08x%04x%04x%04x%04x", ath_count, i, atoi(enable), atoi(prio), atoi(proto),
			 inet_addr(src_ip_start), inet_addr(src_ip_end), inet_addr(dst_ip_start), inet_addr(dst_ip_end),
			 atoi(src_port_start), atoi(src_port_end), atoi(dst_port_start), atoi(dst_port_end) ); 
		}	 
    }
}


static void set_wish(int bss_index, int ath_count)
{
    char *wlan0_wish_enable;
    char wish_enable[30];
    char *wlan0_wmm_enable;
    char wmm_enable[30];
    
    if(bss_index > 0)
    {
    	sprintf(wmm_enable, "wlan0_vap%d_wmm_enable", bss_index);
    	wlan0_wmm_enable = nvram_safe_get(wmm_enable);
    }
    else
    	wlan0_wmm_enable = nvram_safe_get("wlan0_wmm_enable");
    
    if(bss_index > 0)
    {
    	sprintf(wish_enable, "wlan0_vap%d_wish_engine_enable", bss_index);
    	wlan0_wish_enable = nvram_safe_get(wish_enable);
    }
    else
    	wlan0_wish_enable = nvram_safe_get("wish_engine_enabled");
    	
    if((strcmp(wlan0_wmm_enable,"0") == 0)){
		_system("iwpriv ath%d wish 0", ath_count);
    }	
    else{
		if((strcmp(wlan0_wish_enable,"0") == 0)){
			_system("iwpriv ath%d wish 0", ath_count);	
		}		                
    	else
    	{
    		_system("iwpriv ath%d wish 1", ath_count);
    		wlan0_wish_enable = nvram_safe_get("wish_http_enabled");
    		if((strcmp(wlan0_wish_enable,"0") == 0)){
				_system("iwpriv ath%d wishhttp 0", ath_count);	
			}
			else
    		{
    			_system("iwpriv ath%d wishhttp 1", ath_count);
    		}
    		//wish_media_enabled    	
    		wlan0_wish_enable = nvram_safe_get("wish_media_enabled");
    		if((strcmp(wlan0_wish_enable,"0") == 0)){
				_system("iwpriv ath%d wishmedia 0", ath_count);	
			}
			else
    		{
    			_system("iwpriv ath%d wishmedia 1", ath_count);
    		}
    		//wish_auto_enabled
    		wlan0_wish_enable = nvram_safe_get("wish_auto_enabled");
    		if((strcmp(wlan0_wish_enable,"0") == 0)){
    			//printf("wish_auto:disable\n");
				_system("iwpriv ath%d wishauto 0", ath_count);	
			}
			else
    		{
    			_system("iwpriv ath%d wishauto 1", ath_count);
    		}
    		//set_wish_rule
    		set_wish_rule(bss_index,ath_count);			          						          							          					            
    	}
	}
}
#endif

void start_wps(void)
{
	int wps_enable=0;
	char *wlan0_security=0;

	if(!( action_flags.all  || action_flags.wlan ) ) 
		return ;

	if(wlan0_enable==0 && wlan1_enable==0)
		return ;

	if(nvram_match("wps_enable", "1")==0)
		wps_enable=1;

	//make the topology file for wpa_supplicant
	DEBUG_MSG("wlan0_op_mode  =%s\n", nvram_safe_get("wlan0_op_mode"));
	DEBUG_MSG("wlan0_security =%s\n", nvram_safe_get("wlan0_security"));
	DEBUG_MSG("wps_enable     =%s\n", nvram_safe_get("wps_enable"));
	wlan0_security = nvram_safe_get("wlan0_security");
	if ( wlan0_enable && !nvram_match("wlan0_op_mode", "APC") &&
	     (wps_enable || strstr(wlan0_security, "wpa")) ) {
		create_APC_wpa_conf_file();
		create_APC_topo_file();
		_system("wpa_supplicant /var/tmp/stafwd-topology.conf &");
		return;
	}

	//make the topology file for hostapd
	if( wps_enable   == 1 ||
		use_hostapd[0]!=4 ||
		use_hostapd[1]!=4 ||
		use_hostapd[2]!=4 ||
		use_hostapd[3]!=4
	)
	{	
		create_topo_file(wps_enable);
		/*	Date: 2009-06-01
		*	Name: jimmy huang
		*	Reason: for lcm feature - infod used
		*	Note:	to let hostapd know if he needs to tell infod the PBC status
		*/
#ifdef LCM_FEATURE_INFOD
     		system("hostapd -s /var/tmp/topology.conf &");
#else
		//system("hostapd -dd /var/tmp/topology.conf &"); //debug mode
		system("hostapd /var/tmp/topology.conf &");
#endif
		// Start WPS Daemon for polling WPS Button
		system("wpsbtnd &");
	}
}
	

/*for wpa(psk), wpa2(psk)*/
void create_wsc_conf(int bss_index, int ath_count, int wps_enable, int wep_len)
{
	FILE *fp=NULL;
	char *wlan_security=NULL;
    char *ssid=NULL;
    char *passphrase=NULL;
    char *wlan_wpa_eap_radius_server_0=NULL;
    char *wlan_wpa_eap_radius_server_1=NULL;
    char *radius1_ip=NULL;
	char *radius1_port=NULL;
	char *radius1_secret=NULL;
	char *radius2_ip=NULL;
	char *radius2_port=NULL;
	char *radius2_secret=NULL;
	char *lan_ipaddr=NULL;
	char *cipher_type=NULL;
	unsigned char *rekey_time=NULL;
	char *wlan_eap_mac_auth=NULL;
	char *wlan_eap_reauth_period=NULL;
	char *wep_default_key=NULL;
	char *wlan_key_1=NULL; 
	char *wlan_key_2=NULL; 
	char *wlan_key_3=NULL; 
	char *wlan_key_4=NULL; 
	char sec_file[30]={0};
	
	DEBUG_MSG("create_wsc_conf: wlan_vap[%d]: ath%d: wps_enable=%d, wep_len=%d\n", bss_index, ath_count, wps_enable, wep_len);
	
	if(wps_enable==1 && (bss_index==0 || bss_index==1)) //guest zone do not support WPS
		sprintf(sec_file, "/var/tmp/WSC_ath%d.conf", ath_count);
	else
		sprintf(sec_file, "/var/tmp/secath%d", ath_count);
    
    DEBUG_MSG("create_wsc_conf: create %s\n", sec_file);
    
    fp = fopen(sec_file, "w+");
    
    if(fp==NULL)
    {
    	printf("create_wsc_conf: create %s fail\n", sec_file);
    	return;
    }
    
    switch(bss_index)
    {
    	case 0:
    		wlan_security = nvram_safe_get("wlan0_security");
    		ssid = nvram_safe_get("wlan0_ssid");
    		passphrase = nvram_safe_get("wlan0_psk_pass_phrase");
    		cipher_type = nvram_safe_get("wlan0_psk_cipher_type");
    		rekey_time = nvram_safe_get("wlan0_gkey_rekey_time");
    		break;
    	case 1:
    		wlan_security = nvram_safe_get("wlan1_security");
    		ssid = nvram_safe_get("wlan1_ssid");
    		passphrase = nvram_safe_get("wlan1_psk_pass_phrase");
    		cipher_type = nvram_safe_get("wlan1_psk_cipher_type");
    		rekey_time = nvram_safe_get("wlan1_gkey_rekey_time");
    		break;
    	case 2:
    		wlan_security = nvram_safe_get("wlan0_vap1_security");
    		ssid = nvram_safe_get("wlan0_vap1_ssid");
    		passphrase = nvram_safe_get("wlan0_vap1_psk_pass_phrase");
    		cipher_type = nvram_safe_get("wlan0_vap1_psk_cipher_type");
    		rekey_time = nvram_safe_get("wlan0_vap1_gkey_rekey_time");
    		break;
    	case 3:
    		wlan_security = nvram_safe_get("wlan1_vap1_security");
    		ssid = nvram_safe_get("wlan1_vap1_ssid");
    		passphrase = nvram_safe_get("wlan1_vap1_psk_pass_phrase");
    		cipher_type = nvram_safe_get("wlan1_vap1_psk_cipher_type");
    		rekey_time = nvram_safe_get("wlan1_vap1_gkey_rekey_time");
    		break;
    }
    	
	/*generate hostapd config file => see in apps\wps_ath_2\hostapd\hostapd.conf*/	
	
	fprintf(fp, "ignore_file_errors=1\n");
	fprintf(fp, "logger_syslog=-1\n");
	fprintf(fp, "logger_syslog_level=4\n");
	fprintf(fp, "logger_stdout=-1\n");
	fprintf(fp, "logger_stdout_level=4\n");
	fprintf(fp, "debug=0\n");   
	
	fprintf(fp, "ctrl_interface=/var/run/hostapd\n");
	fprintf(fp, "ctrl_interface_group=0\n");
	fprintf(fp, "ssid=%s\n", ssid);
	fprintf(fp, "dtim_period=2\n");
	fprintf(fp, "max_num_sta=255\n");
	fprintf(fp, "macaddr_acl=0\n");
	fprintf(fp, "auth_algs=1\n");
	fprintf(fp, "ignore_broadcast_ssid=0\n");
	fprintf(fp, "wme_enabled=0\n");
	fprintf(fp, "eapol_version=2\n");
	fprintf(fp, "eapol_key_index_workaround=0\n");
		
	if (strstr(wlan_security, "_psk"))	
	{
		if (strstr(wlan_security, "wpa_"))
			fprintf(fp, "wpa=1\n");
		else if (strstr(wlan_security, "wpa2_"))
			fprintf(fp, "wpa=2\n");
		else if (strstr(wlan_security, "wpa2auto_"))
			fprintf(fp, "wpa=3\n");		
		fprintf(fp, "ieee8021x=0\n");
		fprintf(fp, "eap_server=1\n");
		fprintf(fp, "eap_user_file=/etc/wpa2/hostapd.eap_user\n");		
		if(strlen(passphrase) == 64)/*64 HEX*/
			fprintf(fp, "wpa_psk=%s\n", passphrase);
		else/*8~63 ASCII*/
			fprintf(fp, "wpa_passphrase=%s\n", passphrase);	
		fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
		
		if(strcmp(cipher_type, "tkip")== 0)
			fprintf(fp, "wpa_pairwise=TKIP\n"); 
		else if(strcmp(cipher_type, "aes")== 0)
			fprintf(fp, "wpa_pairwise=CCMP\n");
		else	 
			fprintf(fp, "wpa_pairwise=CCMP TKIP\n");
	}	
	else if (strstr(wlan_security, "_eap") && (wps_enable==0 || bss_index==2 || bss_index==3))	
	{
		if (strstr(wlan_security, "wpa_"))
			fprintf(fp, "wpa=1\n");
		else if (strstr(wlan_security, "wpa2_"))
			fprintf(fp, "wpa=2\n");
		else if (strstr(wlan_security, "wpa2auto_"))
			fprintf(fp, "wpa=3\n");		
		lan_ipaddr = nvram_safe_get("lan_ipaddr");
		switch(bss_index)
		{
			case 0:
				wlan_eap_mac_auth = nvram_safe_get("wlan0_eap_mac_auth");  
				wlan_wpa_eap_radius_server_0 = nvram_safe_get("wlan0_eap_radius_server_0");
				wlan_wpa_eap_radius_server_1 = nvram_safe_get("wlan0_eap_radius_server_1");
				wlan_eap_reauth_period = nvram_safe_get("wlan0_eap_reauth_period");
				break;	
			case 1:
				wlan_eap_mac_auth = nvram_safe_get("wlan1_eap_mac_auth");  
				wlan_wpa_eap_radius_server_0 = nvram_safe_get("wlan1_eap_radius_server_0");
				wlan_wpa_eap_radius_server_1 = nvram_safe_get("wlan1_eap_radius_server_1");
				wlan_eap_reauth_period = nvram_safe_get("wlan1_eap_reauth_period");
				break;	
			case 2:
				wlan_eap_mac_auth = nvram_safe_get("wlan0_vap_eap_mac_auth");  
				wlan_wpa_eap_radius_server_0 = nvram_safe_get("wlan0_vap1_eap_radius_server_0");
				wlan_wpa_eap_radius_server_1 = nvram_safe_get("wlan0_vap1_eap_radius_server_1");
				wlan_eap_reauth_period = nvram_safe_get("wlan0_vap1_eap_reauth_period");
				break;	
			case 3:
				wlan_eap_mac_auth = nvram_safe_get("wlan1_vap_eap_mac_auth");  
				wlan_wpa_eap_radius_server_0 = nvram_safe_get("wlan1_vap1_eap_radius_server_0");
				wlan_wpa_eap_radius_server_1 = nvram_safe_get("wlan1_vap1_eap_radius_server_1");
				wlan_eap_reauth_period = nvram_safe_get("wlan1_vap1_eap_reauth_period");
				break;				
		}	
			    
		if(strlen(wlan_wpa_eap_radius_server_0) > 0) 
		{
			radius1_ip = strtok(wlan_wpa_eap_radius_server_0, "/");
			radius1_port = strtok(NULL,"/");
			radius1_secret = strtok(NULL,"/");
			if(radius1_ip == NULL || radius1_port == NULL || radius1_secret == NULL)
			{	
				radius1_ip = "0.0.0.0"; 
		       	radius1_port = "1812";	
	           	radius1_secret = "00000";
			} 
		}    
		else
		{
			radius1_ip = "0.0.0.0"; 
		    radius1_port = "1812";	
	        radius1_secret = "11111";
		}
				  
		if(strlen(wlan_wpa_eap_radius_server_1) > 0) 
		{
			radius2_ip = strtok(wlan_wpa_eap_radius_server_1, "/");
			radius2_port = strtok(NULL,"/");
			radius2_secret = strtok(NULL,"/");
			if(radius2_ip == NULL || radius2_port == NULL || radius2_secret == NULL)
			{
				radius2_ip = "0.0.0.0"; 
		       	radius2_port = "1812";	
	           	radius2_secret = "00000";
	        }	
		}
		else
		{
			radius2_ip = "0.0.0.0"; 
		    radius2_port = "1812";	
	        radius2_secret = "11111";
		}
		
		fprintf(fp, "eap_server=0\n");
		fprintf(fp, "own_ip_addr=%s\n", lan_ipaddr);
		fprintf(fp, "auth_server_addr=%s\n", radius1_ip);
		fprintf(fp, "auth_server_port=%s\n", radius1_port);
		fprintf(fp, "auth_server_shared_secret=%s\n", radius1_secret);
		fprintf(fp, "auth_server_addr=%s\n", radius2_ip);
		fprintf(fp, "auth_server_port=%s\n", radius2_port);
		fprintf(fp, "auth_server_shared_secret=%s\n", radius2_secret);
		fprintf(fp, "ieee8021x=1\n");
		fprintf(fp, "wpa_key_mgmt=WPA-EAP\n");
			
		if( atoi(wlan_eap_reauth_period) < 0)
			fprintf(fp, "eap_reauth_period=0\n");
		else
			fprintf(fp, "eap_reauth_period=%d\n", atoi(wlan_eap_reauth_period)*60);	
				
		/*  
			mac_auth==0, don't need eap_mac_auth
			mac_auth==1, radius server 1 need eap_mac_auth
			mac_auth==2, radius server 2 need eap_mac_auth
			mac_auth==3, radius server all need eap_mac_auth
		*/
		if(strlen(wlan_eap_mac_auth) > 0)
			fprintf(fp, "mac_auth=%s\n", wlan_eap_mac_auth);
		else
			fprintf(fp, "mac_auth=3\n");
			
		if(strcmp(cipher_type, "tkip")== 0)
			fprintf(fp, "wpa_pairwise=TKIP\n"); 
		else if(strcmp(cipher_type, "aes")== 0)
			fprintf(fp, "wpa_pairwise=CCMP\n");
		else	 
			fprintf(fp, "wpa_pairwise=CCMP TKIP\n"); 	
	}
	else if (strstr(wlan_security, "wep_") && (wps_enable==1 && (bss_index==0 || bss_index==1)))
	{
		switch(bss_index)
		{
			case 0:
				wep_default_key=nvram_safe_get("wlan0_wep_default_key");
				if(wep_len==64)
				{ 
					wlan_key_1=nvram_safe_get("wlan0_wep64_key_1");
					wlan_key_2=nvram_safe_get("wlan0_wep64_key_2"); 
					wlan_key_3=nvram_safe_get("wlan0_wep64_key_3"); 
					wlan_key_4=nvram_safe_get("wlan0_wep64_key_4");
				}
				else if(wep_len==128)
				{
					wlan_key_1=nvram_safe_get("wlan0_wep128_key_1");
					wlan_key_2=nvram_safe_get("wlan0_wep128_key_2"); 
					wlan_key_3=nvram_safe_get("wlan0_wep128_key_3"); 
					wlan_key_4=nvram_safe_get("wlan0_wep128_key_4");
				}	
				break;
			case 1:
				wep_default_key=nvram_safe_get("wlan1_wep_default_key"); 
				if(wep_len==64)
				{ 
					wlan_key_1=nvram_safe_get("wlan1_wep64_key_1");
					wlan_key_2=nvram_safe_get("wlan1_wep64_key_2"); 
					wlan_key_3=nvram_safe_get("wlan1_wep64_key_3"); 
					wlan_key_4=nvram_safe_get("wlan1_wep64_key_4");
				}
				else if(wep_len==128)
				{
					wlan_key_1=nvram_safe_get("wlan1_wep128_key_1");
					wlan_key_2=nvram_safe_get("wlan1_wep128_key_2"); 
					wlan_key_3=nvram_safe_get("wlan1_wep128_key_3"); 
					wlan_key_4=nvram_safe_get("wlan1_wep128_key_4");
				}	
				break;
			case 2:
				wep_default_key=nvram_safe_get("wlan0_vap1_wep_default_key"); 
				if(wep_len==64)
				{ 
					wlan_key_1=nvram_safe_get("wlan0_vap1_wep64_key_1");
					wlan_key_2=nvram_safe_get("wlan0_vap1_wep64_key_2"); 
					wlan_key_3=nvram_safe_get("wlan0_vap1_wep64_key_3"); 
					wlan_key_4=nvram_safe_get("wlan0_vap1_wep64_key_4");
				}
				else if(wep_len==128)
				{
					wlan_key_1=nvram_safe_get("wlan0_vap1_wep128_key_1");
					wlan_key_2=nvram_safe_get("wlan0_vap1_wep128_key_2"); 
					wlan_key_3=nvram_safe_get("wlan0_vap1_wep128_key_3"); 
					wlan_key_4=nvram_safe_get("wlan0_vap1_wep128_key_4");
				}	
				break;
			case 3:
				wep_default_key=nvram_safe_get("wlan1_vap1_wep_default_key"); 
				if(wep_len==64)
				{ 
					wlan_key_1=nvram_safe_get("wlan1_vap1_wep64_key_1");
					wlan_key_2=nvram_safe_get("wlan1_vap1_wep64_key_2"); 
					wlan_key_3=nvram_safe_get("wlan1_vap1_wep64_key_3"); 
					wlan_key_4=nvram_safe_get("wlan1_vap1_wep64_key_4");
				}
				else if(wep_len==128)
				{
					wlan_key_1=nvram_safe_get("wlan1_vap1_wep128_key_1");
					wlan_key_2=nvram_safe_get("wlan1_vap1_wep128_key_2"); 
					wlan_key_3=nvram_safe_get("wlan1_vap1_wep128_key_3"); 
					wlan_key_4=nvram_safe_get("wlan1_vap1_wep128_key_4");
				}
				break;			
		}
		fprintf(fp, "eap_server=1\n");
		fprintf(fp, "ieee8021x=0\n");
		fprintf(fp, "eap_user_file=/etc/wpa2/hostapd.eap_user\n");	
		fprintf(fp, "wpa=0\n");
		fprintf(fp, "wep_default_key=%d\n", atoi(wep_default_key)-1); //0~3
		fprintf(fp, "wep_key0=%s\n", wlan_key_1); 
		fprintf(fp, "wep_key1=%s\n", wlan_key_2); 
		fprintf(fp, "wep_key2=%s\n", wlan_key_3); 
		fprintf(fp, "wep_key3=%s\n", wlan_key_4); 
	}
	else
	{
		fprintf(fp, "eap_server=1\n");
		fprintf(fp, "ieee8021x=0\n");
		fprintf(fp, "eap_user_file=/etc/wpa2/hostapd.eap_user\n");
	}
		    
	if(wps_enable==0 || bss_index==2 || bss_index==3) //guest zone do not support WPS
	{	
		if (atoi(rekey_time) < 120)
           rekey_time = "120";
		fprintf(fp, "wpa_group_rekey=%s\n", rekey_time); 
	
		fprintf(fp, "wps_disable=1\n"); 
		fprintf(fp, "wps_upnp_disable=1\n"); 
	}
	else
	{
		fprintf(fp, "wps_disable=0\n");
		fprintf(fp, "wps_upnp_disable=0\n");
		fprintf(fp, "wps_version=0x10\n");
		fprintf(fp, "wps_auth_type_flags=0x0023\n");//capabilities of network authentication
		fprintf(fp, "wps_encr_type_flags=0x000f\n");//capabilities of network encryption
		fprintf(fp, "wps_conn_type_flags=0x01\n"); //ESS
		//fprintf(fp, "wps_config_methods=0x0082\n"); //supported configuration methods
		fprintf(fp, "wps_config_methods=0x0084\n"); //for pass WIN7 DTM 1.4
		
		if(atoi(nvram_safe_get("wps_configured_mode"))==1)
			fprintf(fp, "wps_configured=0\n");
		else
			fprintf(fp, "wps_configured=1\n");
				
		if(atoi(nvram_safe_get("wps_lock"))==1)
			fprintf(fp, "ap_setup_locked=1\n");
		else
			fprintf(fp, "ap_setup_locked=0\n");
				
		fprintf(fp, "wps_rf_bands=0x03\n");
		fprintf(fp, "wps_manufacturer=%s\n", nvram_safe_get("manufacturer"));
		fprintf(fp, "wps_model_name=%s\n", nvram_safe_get("model_name"));
		fprintf(fp, "wps_model_number=%s\n", nvram_safe_get("model_number"));
		fprintf(fp, "wps_serial_number=%s\n", nvram_safe_get("serial_number"));
		fprintf(fp, "wps_friendly_name=%s\n", nvram_safe_get("friendlyname"));
		fprintf(fp, "wps_manufacturer_url=%s\n", nvram_safe_get("manufacturer_url"));
		fprintf(fp, "wps_model_description=DIR-655\n");
		fprintf(fp, "wps_model_url=%s\n", nvram_safe_get("model_url"));
		fprintf(fp, "wps_upc_string=upc string here\n");
		fprintf(fp, "wps_default_pin=%s\n", nvram_safe_get("wps_pin"));
		fprintf(fp, "wps_dev_category=6\n"); // Network Infrastructure
		fprintf(fp, "wps_dev_sub_category=1\n"); //AP
		fprintf(fp, "wps_dev_oui=0050f204\n");
		/*
		the length of dev_name must be less than 32 bytes
		this name appears in the Vista "networks" window icon,when the device reports itself as unconfigured
		*/
		fprintf(fp, "wps_dev_name=DIR-655\n");
		fprintf(fp, "wps_os_version=0x00000001\n");
		fprintf(fp, "wps_atheros_extension=0\n"); //use Atheros WPS extensions
	}	  
	fclose(fp); 
}

static int bg_table[] = {54,48,36,24,18,12,9,6,11,5.5,2,1};
static int bg_rate_table[] = {12,8,13,9,14,10,15,11,24,25,26,27};
void set_11bgn_rate(int bss_index, int ath_count, char *dot_mode)
{	
	char *wlan0_11bgn_txrate,*mcs_start,*mcs_end,mcs_c[4];
	char bgn_txrate[50];
	int mcs_i=0,mcs_i_next=0,len=0,i;

	if(bss_index > 0)
	{ 
		sprintf(bgn_txrate, "wlan0_vap%d_11bgn_txrate", bss_index);   
		wlan0_11bgn_txrate = nvram_safe_get(bgn_txrate);
	}
	else
	{
		if(strcmp(dot_mode, "11bgn")==0)
			wlan0_11bgn_txrate = nvram_safe_get("wlan0_11bgn_txrate");
		else if(strcmp(dot_mode, "11gn")==0)
			wlan0_11bgn_txrate = nvram_safe_get("wlan0_11gn_txrate");
		else
			wlan0_11bgn_txrate = nvram_safe_get("wlan0_11n_txrate");
	}
	
	DEBUG_MSG("set_11bgn_rate: rate is %s\n",wlan0_11bgn_txrate);
	if (strcmp(wlan0_11bgn_txrate, "Best (automatic)") == 0)
	{
		DEBUG_MSG("set_11bgn_rate: rate = auto\n");
		//_system("iwconfig ath%d rate auto", ath_count);
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
	}
	else if(!memcmp(wlan0_11bgn_txrate, "MCS", 3))
	{
		memset(bgn_txrate, 0, 50);
		memset(mcs_c, 0, 4);
		mcs_start = wlan0_11bgn_txrate + 4;
		mcs_end = strstr(mcs_start, "-");
		if(mcs_end)
		{
			len = (unsigned long) mcs_start - (unsigned long) mcs_end - 1;
			//if(len > 2) 
			len = 2;
			memcpy(mcs_c, mcs_start, len);
			mcs_i = 0x80 | atoi(mcs_c);
			if(mcs_i != 0x80) mcs_i_next = mcs_i - 1;
			else mcs_i_next = mcs_i;
			sprintf(bgn_txrate, "iwpriv ath%d set11NRates 0x%02X%02X%02X%02X", ath_count, mcs_i, mcs_i, mcs_i_next, mcs_i_next);
			DEBUG_MSG("set_11bgn_rate: set rate - %s\n",bgn_txrate);
			_system(bgn_txrate);
			_system("iwpriv ath%d set11NRetries 0x04040404", ath_count);
			_system("iwpriv ath%d shortgi 0", ath_count);
			_system("iwpriv ath%d cwmmode 0", ath_count);
		}
		//else
			//_system("iwconfig ath%d rate auto", ath_count);
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
	}	
	else 
	{
		mcs_i = atoi(wlan0_11bgn_txrate);
		for(i=0;i<12;i++)
		{
			if(mcs_i == bg_table[i]) break;
		}
		if((i == 12)&&(mcs_i != 1)) mcs_i = 0;
		
		if(mcs_i != 0)
		{
			mcs_i = bg_rate_table[i];
			memset(bgn_txrate, 0, 50);
			sprintf(bgn_txrate, "iwpriv ath%d set11NRates 0x%02X%02X%02X%02X", ath_count, mcs_i, mcs_i, mcs_i, mcs_i);
			DEBUG_MSG("set_11bgn_rate: set rate - %s\n",bgn_txrate);
			_system(bgn_txrate);
			_system("iwpriv ath%d set11NRetries 0x04040404", ath_count);
			_system("iwpriv ath%d shortgi 0", ath_count);
			_system("iwpriv ath%d cwmmode 0", ath_count);
		}
		else
		{	
			DEBUG_MSG("11bgn_txrate value error\n");
			//_system("iwconfig ath%d rate auto", ath_count);
		}
		if(mcs_i == 1)
			_system("iwpriv ath%d mcast_rate 1000", ath_count);
		else
			_system("iwpriv ath%d mcast_rate 2000", ath_count);
	}	
} 

void set_11bg_rate(int bss_index, int ath_count, char *dot_mode)
{	
	char *wlan0_11bg_txrate;
	char bg_txrate[30];
	int mcs_i=0,i;

	if(bss_index > 0)
	{ 
		sprintf(bg_txrate, "wlan0_vap%d_11bg_txrate", bss_index);   
		wlan0_11bg_txrate = nvram_safe_get(bg_txrate);
	}
	else
	{
		if(strcmp(dot_mode, "11bg")==0)
			wlan0_11bg_txrate = nvram_safe_get("wlan0_11bg_txrate");
		else
			wlan0_11bg_txrate = nvram_safe_get("wlan0_11g_txrate");
	}
	   
	DEBUG_MSG("set_11bg_rate: rate is %s\n",wlan0_11bg_txrate);
	if (strcmp(wlan0_11bg_txrate, "Best (automatic)") == 0)
	{
		DEBUG_MSG("set_11bg_rate: rate = auto\n");
		//_system("iwconfig ath%d rate auto", ath_count);	
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
	}
	else 
	{
		mcs_i = atoi(wlan0_11bg_txrate);
		for(i=0;i<12;i++)
		{
			if(mcs_i == bg_table[i]) break;
		}
		if((i == 12)&&(mcs_i != 1)) mcs_i = 0;
		
		if(mcs_i != 0)
		{
			memset(bg_txrate, 0, 30);
			sprintf(bg_txrate, "iwconfig ath%d rate %dM", ath_count, mcs_i);
			DEBUG_MSG("set_11bg_rate: set rate - %s\n",bg_txrate);
			_system(bg_txrate);
		}
		else
		{	
			DEBUG_MSG("11bg_txrate value error\n");
			//_system("iwconfig ath%d rate auto", ath_count);
		}
		if(mcs_i == 1)
			_system("iwpriv ath%d mcast_rate 1000", ath_count);
		else
			_system("iwpriv ath%d mcast_rate 2000", ath_count);
	}	
} 
 
void set_11b_rate(int bss_index, int ath_count, char *dot_mode)
{
	char *wlan0_11b_txrate;
	char b_txrate[30];
	
	if(bss_index > 0)
	{ 
		sprintf(b_txrate, "wlan0_vap%d_11b_txrate", bss_index);   
		wlan0_11b_txrate = nvram_safe_get(b_txrate);
	}
	else
		wlan0_11b_txrate = nvram_safe_get("wlan0_11b_txrate");
		
	DEBUG_MSG("set_11b_rate: rate is %s\n",wlan0_11b_txrate);
	if (strcmp(wlan0_11b_txrate, "Best (automatic)") == 0)
	{
		DEBUG_MSG("set_11b_rate: rate = auto\n");
		//_system("iwconfig ath%d rate auto", ath_count);
	}	
	else if (strcmp(wlan0_11b_txrate, "11") == 0)
		_system("iwconfig ath%d rate 11M", ath_count);
	else if (strcmp(wlan0_11b_txrate, "5.5") == 0)
		_system("iwconfig ath%d rate 5.5M", ath_count);
	else if (strcmp(wlan0_11b_txrate, "2") == 0)
		_system("iwconfig ath%d rate 2M", ath_count);
	else if (strcmp(wlan0_11b_txrate, "1") == 0)
		_system("iwconfig ath%d rate 1M", ath_count);
	else
	{	
		DEBUG_MSG("11b_txrate value error\n");
		//_system("iwconfig ath%d rate 11M", ath_count);
	}
	if(strcmp(wlan0_11b_txrate, "1") == 0)
		_system("iwpriv ath%d mcast_rate 1000", ath_count);
	else
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
} 

