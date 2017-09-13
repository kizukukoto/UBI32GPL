#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nvram.h>
#include <sutil.h>

extern void get_country_Code(int *);

void start_insmod(void)
{
	int country_Code = 840;

	get_country_Code(&country_Code);
        printf("Loading Wireless Modules: country_Code=%d\n", country_Code);
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
        printf("Loading Wireless Modules: end\n");
}

void stop_insmod(void)
{
/*      NickChou add,
maybe we can only "rmmod" to accelerate wlan procedure.
Do not run "brctl delif", "ifconfig down", "wlanconfig destroy" .
*/

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

