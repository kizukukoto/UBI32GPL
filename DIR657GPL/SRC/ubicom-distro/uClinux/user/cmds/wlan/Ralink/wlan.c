#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
//#include <rc.h>

//#define WLAN_DEBUG
#ifdef WLAN_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static int start_vap()
{
        if (strtol(nvram_safe_get("wlan0_vap1_enable"), NULL, 10)) {
                sleep(1);
                _system("ifconfig ra01_1 up");
                _system("brctl addif %s ra01_1", nvram_safe_get("lan_bridge"));
        }

        if (strtol(nvram_safe_get("wlan1_vap1_enable"), NULL, 10)) {
                sleep(1);
                _system("ifconfig ra00_1 up");
                _system("brctl addif %s ra00_1", nvram_safe_get("lan_bridge"));
        }

        return 0;
}

static int _start_ap()
{
        DEBUG_MSG("Start AP\n");
        /* set up interface for 5G */
        if (nvram_match("wlan1_enable", "1") == 0)
		_system("ifconfig ra00_0 up");
        /* set up interface for 2.4G */
        if (nvram_match("wlan0_enable", "1") == 0)
		_system("ifconfig ra01_0 up");
        /* add interface to bridge */
        if (nvram_match("wlan1_enable", "1") == 0)
                _system("brctl addif %s ra00_0", nvram_safe_get("lan_bridge"));
        if (nvram_match("wlan0_enable", "1") == 0)
                _system("brctl addif %s ra01_0", nvram_safe_get("lan_bridge"));
        /* start up guess_zone */
        start_vap();
        _system("brctl addif %s eth1", nvram_safe_get("lan_bridge"));
        DEBUG_MSG("finish start_ap()\n");
	return 0;
}

int start_ap(void)
{
	/* FIXME: */
	// if (!( action_flags.all || action_flags.wlan || action_flags.firewall))
        //        return 1;
        return _start_ap();
}

static void do_iNIC_Card()
{
        char *p_lan_mac, tmp[24];
        int  lan_mac[6]={0};
        char wlan0_mac[18]={0};
        char wlan1_mac[18]={0};

        sprintf(tmp, "%s", nvram_safe_get("lan_mac"));
        p_lan_mac = tmp;

        sscanf(p_lan_mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                &lan_mac[0], &lan_mac[1], &lan_mac[2],
                &lan_mac[3], &lan_mac[4], &lan_mac[5]);

        sprintf(wlan0_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
                lan_mac[0], lan_mac[1], lan_mac[2],
                lan_mac[3], lan_mac[4], lan_mac[5]);
        sprintf(wlan1_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
                lan_mac[0], lan_mac[1], lan_mac[2],
                lan_mac[3], lan_mac[4], lan_mac[5] + 2);

        _system("echo \'#The word of \"Default\" must not be removed, maximum 32 cards, 00 ~ 31\' > /etc/Wireless/RT3883/iNIC_ap_card.dat");
        _system("echo \"Default\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat");
#if 0
        _system("echo \"00ProfilePath=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", "/etc/RT3883/iNIC_ap.dat");
        _system("echo \"01ProfilePath=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", "/etc/RT3883/iNIC_ap1.dat");
        _system("echo \"00MAC=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", "00:15:f2:a2:dd:00");
        _system("echo \"01MAC=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", "00:15:f2:a2:df:00");
#else
        _system("echo \"00ProfilePath=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", "/var/tmp/iNIC_ap.dat");
        _system("echo \"01ProfilePath=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", "/var/tmp/iNIC_ap1.dat");
        _system("echo \"00MAC=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", wlan1_mac);
        _system("echo \"01MAC=%s\" >> /etc/Wireless/RT3883/iNIC_ap_card.dat", wlan0_mac);
#endif
}

static void wlan_rmmod(void)
{
        _system("rmmod rt3883_iNIC");
        _system("ifconfig eth1 down");
}

static void wlan_insmod(void)
{
	if (access("/etc/Wireless/RT3883/iNIC_ap_card.dat", F_OK) != 0)
		do_iNIC_Card();

        _system("ifconfig eth1 up");
        _system("insmod /lib/modules/rt3883_iNIC.ko mode=ap miimaster=eth1");
}

static int wlan_up()
{
	/* set configuration */
	_system("cmds wlan wconfig ra01_0");
	_system("cmds wlan wconfig ra00_0");
	/* insert modules */
	wlan_insmod();
	/* stop rt2880apd */
	_system("cmds wlan eap stop ra01_0");
	_system("cmds wlan eap stop ra00_0");
	/* set up ap */
	start_ap();
	/*start rt2880apd */
	_system("cmds wlan eap start ra01_0");
	_system("cmds wlan eap start ra00_0");
	return 0;
}

static int wlan_down()
{
        /* del interface from bridge */
        _system("brctl delif %s ra00_0", nvram_safe_get("lan_bridge"));
        _system("brctl delif %s ra01_0", nvram_safe_get("lan_bridge"));
        _system("brctl delif %s eth1", nvram_safe_get("lan_bridge"));
        /* set down interface*/
        _system("ifconfig ra00_0 down");
        _system("ifconfig ra01_0 down");
        /* unload modules */
        wlan_rmmod();
	return 0;
}

int start_wireless(int argc, char *argv[])
{
	wlan_insmod();
	_start_ap();
	return 0;
}

int stop_wireless(int argc, char *argv[])
{
	return wlan_down();
}

int _start_wlan()
{
        /* Add loopback */
        //ifconfig_up("lo", "127.0.0.1", "255.0.0.0");
        //route_add("lo", "127.0.0.0", "255.0.0.0", "0.0.0.0", 0);
	config_loopback();

        if(nvram_match("wlan0_enable", "1") != 0 && nvram_match("wlan1_enable", "1") != 0) {
                DEBUG_MSG("******* Wireless Radio OFF *********\n");
	} else {
                DEBUG_MSG("********** Start WLAN **************\n");
		wlan_up();
        }
        DEBUG_MSG("********** Start WLAN Finished**************\n");
	return 0;
}

int start_wlan(void)
{
        DEBUG_MSG(" start_wlan\n");
        return _start_wlan();
}

int stop_wlan(void)
{
	return wlan_down();
}
