const unsigned char __CAMEO_WIRELESS_VERSION__[] = "ap81-7.1.3.58";
const unsigned char __CAMEO_WIRELESS_BUILD__[] = "0001";
const unsigned char __CAMEO_WIRELESS_DATE_[] = "Tue, 22, Sep, 2009";

#if 0
/*
[Release Note]
Name        : Jimmy Huang
Type        : wireless
Version     : AP81-7.1.3.58 build 0046
Tag         : AP81-2008-08-18-0047
Decription  : 1.Fix compatibility for Vista Logo Test (DTM) Qos : Two Stream Priority Tests
				Mark the debug msg
Detail      : 1. 802_11/linux/net80211/ieee80211_input.c
			  2. 802_11/linux/net80211/ieee80211_node.c

Name        : Jimmy Huang
Type        : wireless
Version     : AP81-7.1.3.58 build 0046
Tag         : AP81-2008-08-18-0046
Decription  : 1.Fix compatibility for Vista Logo Test (DTM) Qos : Two Stream Priority Tests
Detail      : 802_11/linux/net80211/ieee80211_output.c

Name        : Albert Chen
Type        : wireless
Version     : AP81-7.1.3.58 build 0045
Tag         : AP81-2008-08-15-0045
Decription  : 1.Fix compatibility with INTEL-4965AGN card
Detail      : 802_11\if_ath_net80211\if_ath.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.58 build 0044
Tag 	    : AP81-2008-06-24-0044
Decription  : 1.Fix the set wep mode will cause wireless driver crash issue
Detail      :  802_11\common\hal\ar5416\ar5416_misc.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.58 build 0043
Tag 	    : AP81-2008-06-20-0043
Decription  : 1.add wireless led parameter in module ahb
Detail      :  802_11/linux/ath/if_ath_ahb.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.58 build 0042
Tag 	    : AP81-2008-06-06-0042
Decription  : 1.update wireless driver to 7.1.3.58
Detail	    :
          	802_11/common/ath_dev/if_athrate.h 802_11/common/dfs/dfs.c
          	802_11/common/dfs/dfs.h 802_11/common/hal/ah_regdomain.h
          	802_11/common/hal/ah_regdomain_common.h
          	802_11/common/hal/ar5416/ar5416.h
          	802_11/common/hal/ar5416/ar5416_attach.c
          	802_11/common/hal/ar5416/ar5416_eeprom.c
          	802_11/common/hal/ar5416/ar5416_howl.ini
	        802_11/common/hal/ar5416/ar5416_keycache.c
	        802_11/common/hal/ar5416/ar5416_misc.c
	        802_11/common/hal/ar5416/ar5416_radar.c
	        802_11/common/hal/ar5416/ar5416_reset.c
	        802_11/common/hal/ar5416/ar5416_xmit_ds.c
	        802_11/common/hal/ar5416/ar5416reg.h
	        802_11/common/ratectrl/ar5416Phy.c
	        802_11/common/ratectrl/if_athrate.c
	        802_11/common/ratectrl/ratectrl11n.h
	        802_11/common/ratectrl/ratectrl_11n.c
	        802_11/if_ath_net80211/if_ath.c
	        802_11/if_ath_net80211/if_ath_cwm.c
	        802_11/linux/ath/ath_linux.c
	        802_11/linux/net80211/ieee80211_input.c
	        802_11/linux/net80211/ieee80211_output.c
	        802_11/linux/net80211/ieee80211_scan_ap.c
	        802_11/linux/net80211/ieee80211_wireless.c
	        802_11/madwifi/doc/wireless_version.c
	      
	      
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.51 build 0041
Tag 	    : AP81-2008-06-03-0041
Decription  : 1.Separate the multicast and broadcast packet for NDS connect issue
Detail	    :
	      802_11/if_ath_net80211/if_ath.c
		
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.51 build 0040
Tag 	    : AP81-2008-05-15-0040
Decription  : 1.Take off debug message.
Detail	    :
		802_11/common/hal/ar5416/ar2133.c
		802_11/common/hal/ar5416/ar5416_reset.c
		802_11/madwifi/doc/wireless_version.c


Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.51 build 0038
Tag 	    : AP81-2008-04-08-0038
Decription  : 1.update makfile for wlanconfig

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.51 build 0037
Tag 	    : AP81-2008-04-07-0037
Decription  : 1.update LSDK 7.1.3.51
Detail	    : 

Name   	    : Nick Chou
Type        : wireless
Version	    : AP81-7.1.3.36 build 0036
Tag 	    : AP81-2008-03-31-0036
Decription  : 1.Enable WISH function
Detail	    : \802_11\linux\net80211\ieee80211_output.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.36 build 0035
Tag 	    : AP81-2008-02-26-0035
Decription  : 1.update LSDK 7.1.3.38

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.36 build 0034
Tag 	    : AP81-2008-02-19-0034
Decription  : 1.update LSDK 7.1.3.36


Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.26 build 0033
Tag 	    : AP81-2008-01-18-0033
Decription  : 1.update wireless for DFS

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.26 build 0032
Tag 	    : AP81-2008-01-09-0032
Decription  : 1.update CCA value for through put vs MVL usb dongle issue.
Detail	    : 802_11\common\hal\ar5416\ar5416_eeprom.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.3.26 build 0031
Tag 	    : AP81-2008-01-07-0031
Decription  : 1.Update wireless driver to 7.1.3.26


Name   	    : Ken Chiang
Type        : wireless
Version	    : AP81-7.1.2.47 build 0030
Tag 	    : AP81-2008-01-02-0030
Decription  : 1.Fixed wireless packet had right QoS tag for WISH function.
			  2.Modify HTTP and Windows Media Center Priority from VO to VI for WISH function.
Detail      : 802_11/linux/net80211/ieee80211_output.c			  

Name   	    : Ken Chiang
Type        : wireless
Version	    : AP81-7.1.2.47 build 0029
Tag 	    : AP81-2007-12-27-0029
Decription  : 1.added WISH function.
Detail      : 802_11/linux/net80211/ieee80211_wireless.c
			  802_11/linux/net80211/ieee80211_output.c
			  802_11/linux/net80211/ieee80211_ioctl.h			  
	      	  
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.47 build 0028
Tag 	    : AP81-2007-12-24-0028
Decription  : 1.take of function ieee80211_add_athextcap for wifi test.
Detail      : 802_11/linux/net80211/ieee80211_beacon.c
	      802_11/linux/net80211/ieee80211_output.c
	
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.47 build 0027
Tag 	    : AP81-2007-12-12-0027
Decription  : 1.update LSDK 7.1.2.47
Detail      : 
              
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.46 build 0026
Tag 	    : AP81-2007-11-26-0026
Decription  : 1.Take off led control

Detail      : 802_11\common\ath_dev\linux\ath_osdep.c
              802_11\common\hal\ar5416\ar5416_gpio.c

Version	    : AP81-7.1.2.46 build 0025
Tag 	    : AP81-2007-11-29-0025
Decription  : 1.Fixed sta list show information error issue

Detail      : \802_11\linux\tools\wlanconfig.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.46 build 0024
Tag 	    : AP81-2007-11-28-0024
Decription  : 1.update wireless dirver to LSDK 7.1.2.46
	      2.add ioctl option for getting wireless stats
Detail      : \802_11\linux\net80211\ieee_wireless.c
	
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.43 build 0023
Tag 	    : AP81-2007-11-23-0023
Decription  : update wireless dirver to LSDK 7.1.2.43
Detail      : 

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.38 build 0022
Tag 	    : AP81-2007-11-16-0022
Decription  : fixed 802.11d enable issue
Detail      : /802_11/linux/net80211/ieee80211_beacon.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.38 build 0021
Tag 	    : AP81-2007-11-08-0021
Decription  : update wireless dirver to LSDK 7.1.2.38
Detail      : 

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.32 build 0020
Tag 	    : AP81-2007-10-25-0020
Decription  : 1. Add multicast rate 11Mbps instead 1Mbps
Detail      : AP81_0730/802_11/common/ath_dev/ath_xmit.c

              
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.27 build 0019
Tag 	    : AP81-2007-10-05-0019
Decription  : 1. Add sta association and authentication
Detail      : AP81_0730/802_11/linux/net80211/ieee80211_input.c
              AP81_0730/802_11/linux/net80211/ieee80211_node.c
              AP81_0730/802_11/madwifi/doc/wireless_version.c
    
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.27 build 0018
Tag 	    : AP81-2007-09-14-0018
Decription  : 1. update wireless dirver 7.1.2.32
              2. remove country code character 'I' and 'O'
Detail      :
              802_11/common/ath_dev/ath_internal.h
              802_11/common/ath_dev/ath_xmit.c
              802_11/common/ath_pktlog/pktlog.c 
              802_11/common/hal/ah.c
              802_11/common/ratectrl/if_athrate.c
              802_11/linux/ath/ath_linux.c
              802_11/linux/net80211/_ieee80211.h
              802_11/linux/net80211/ieee80211.c
              802_11/linux/net80211/ieee80211_input.c
              802_11/linux/net80211/ieee80211_ioctl.h
              802_11/linux/net80211/ieee80211_node.c
              802_11/linux/net80211/ieee80211_scan_ap.c
              802_11/linux/net80211/ieee80211_var.h
              802_11/linux/net80211/ieee80211_wireless.c
              802_11/linux/tools/Makefile
              802_11/madwifi/doc/wireless_version.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.27 build 0017
Tag 	    : AP81-2007-09-14-0017
Decription  : 1. update wireless dirver 7.1.2.27
Detail	   :  802_11/common/ath_dev/ath_main.c
	      802_11/common/ath_dev/ath_xmit.c
	      802_11/common/hal/ar5416/ar5416_howl.ini
	      802_11/common/hal/ar5416/ar5416_reset.c
	      802_11/common/hal/ar5416/ar5416reg.h
	      802_11/common/hal/linux/ah_osdep.c
	      802_11/common/hal/linux/ah_osdep.h
	      802_11/if_ath_net80211/if_ath.c
	      802_11/if_ath_net80211/if_athvar.h
	      802_11/linux/ath/ath_linux.c
	      802_11/linux/net80211/ieee80211_input.c
	      802_11/linux/net80211/ieee80211_node.c
	      802_11/linux/net80211/ieee80211_output.c
	      802_11/linux/net80211/ieee80211_var.h
	      802_11/madwifi/doc/wireless_version.c


Name   	    : Ken Chiang
Type        : wireless
Version	    : AP81-7.1.2.19 build 0016
Tag 	    : AP81-2007-09-13-0016
Decription  : 1. when unload ahb module, chiang delay to sleep for remove module
Detail	   :  802_11/common/ath_dev/ath_main.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.19 build 0015
Tag 	    : AP81-2007-09-11-0015
Decription  : 1. when unload ahb module, add delay for remove module
Detail	   :  802_11/common/ath_dev/ath_main.c


Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.19 build 0014
Tag 	    : AP81-2007-09-10-0014
Decription  : 1. when unload ahb module, do not remove ath_sysctl_unregister
Detail	   :  802_11/linux/ath/if_ath_ahb.c


Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.19 build 0013
Tag 	    : AP81-2007-08-29-0013
Decription  : 1. temp take off calibration data checksum check in code
Detail	   :  802_11/common/hal/ar5416/ar5416_eeprom.c    

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.19 build 0012
Tag 	    : AP81-2007-08-28-0012
Decription  : 1. update Atheros RC1 7.1.2.12    
Detail 	    : 802_11/common/ath_dev/ath_internal.h
  	      802_11/common/ath_dev/ath_led.c
              802_11/common/ath_dev/ath_main.c
              802_11/common/ath_dev/ath_xmit.c
              802_11/common/ath_pktlog/linux.c
              802_11/common/ath_pktlog/pktlog.c
              802_11/common/ath_pktlog/pktlog_i.h
              802_11/common/hal/ar5416/ar5416_ani.c
              802_11/common/hal/ar5416/ar5416_eeprom.c
              802_11/common/hal/ar5416/ar5416_howl.ini
              802_11/common/hal/ar5416/ar5416_reset.c
              802_11/common/hal/ar5416/ar5416_xmit.c
              802_11/common/ratectrl/ratectrl_11n.c

Name   	    : Nick Chou
Type        : wireless
Version	    : AP81-7.1.2.12 build 0011
Tag 	    : AP81-2007-08-16-0011
Decription  : 1. update Atheros RC1 7.1.2.12             
Detail	    : AP81_0730/802_11/common/hal/ar5416/ar5416_howl.ini
              AP81_0730/802_11/common/hal/ar5416/ar5416_power.c
              AP81_0730/802_11/common/hal/ar5416/ar5416_reset.c
              AP81_0730/802_11/common/ratectrl/Kbuild
              AP81_0730/802_11/if_ath_net80211/if_ath.c
              AP81_0730/802_11/madwifi/doc/wireless_version.c


Name   	    : Nick Chou
Type        : wireless
Version	    : AP81-7.1.2.10 build 00010
Tag 	    : AP81-2007-08-16-0010
Decription  : 1. add si->isi_nrates for checking STA using 11b, 11g, 11n 
              2. add cameo_txrate for checking STA txrate
Detail	    : 802_11\linux\tools\wlanconfig.c

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.10 build 0009
Tag 	    : AP81-2007-08-15-0009
Decription  : support smac write domain to driver
Detail	    : 802_11/commom/ath_dev/linux/ath_osdep.c
	      802_11/commom/hal/ah_regdomain.c
	      802_11/common/hal/ah_regdomain_common.h
	      
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.20 build 0008
Tag 	    : AP81-2007-08-14-0008
Decription  : Add wps_enable file in /proc/sys/dev/ath/wps_enable for MP
Detail	    : 802_11/linux/ath/ath_linux.c
	      802_11/linux/ath/if_ath_ahb.c
	      802_11/if_ath_net80211\if_athvar.h
	      
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.20 build 0008
Tag 	    : AP81-2007-08-14-0008
Decription  : Add wps_enable file in /proc/sys/dev/ath/wps_enable for MP
Detail	    : 802_11/linux/ath/ath_linux.c
	      802_11/linux/ath/if_ath_ahb.c
	      802_11/if_ath_net80211\if_athvar.h	      
 

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.20 build 0007
Tag 	    : AP81-2007-08-14-0007
Decription  : turn off ATH_DEBUG_LED
Detail	    : 802_11/common/ath_dev/ath_main.c	      
 
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.20 build 0006
Tag 	    : AP81-2007-08-13-0006
Decription  : turn off ATH_DEBUG
Detail	    : 802_11/common/hal/linux/Makefile.inc	      
              
Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.20 build 0005
Tag 	    : AP81-2007-08-09-0005
Decription  : wireless led enable
              wireless time

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.2.20 build 0004
Tag 	    : AP81-2007-08-03-0004
                update LSDK-wlan 7.1.20
Decription  : 

Name   	    : Albert Chen
Type        : wireless
Version	    : AP81-7.1.0.24 build 0002
Tag 	    : AP81-2007-07-04-0002
          AP81/wireless_tools.28/Makefile
          AP81/wireless_tools.28/iwconfig.c
          AP81/wireless_tools.28/iwlib.c AP81/wireless_tools.28/iwlib.h

Name        : 
Type        : Wireless
Version     : AP81-7.1.0.24 build 0001
Tag         : AP71-2007-06-26-0001
Description : first release 
*/
#endif
