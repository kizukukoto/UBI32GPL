#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sutil.h>
#include <nvram.h>
#include "wlan.h"


extern void config_loopback(void);

extern wlan *init_wlan(wlan **, int);
extern void release(wlan *);

extern void init_wlan0_if(char *, wlan *);
extern void init_wlan1_if(wlan *);
extern void init_wlan0_mac_to_nvram(void);

extern void start_makeVAP(wlan *, char *);
extern void start_activateVAP(int , int, int);

extern void start_insmod(void);
extern void stop_insmod(void);

extern void set_bridge_arpping_and_led(void);
extern void set_wlan_mac(void);
extern void set_modify_ssid(wlan *);
extern void set_wlan_security(wlan *, const char *);
extern void set_mac_filter(int);
extern int get_wlan_vap_gzone(int);
extern int get_wlan_netusb(void);

int start_ap(wlan *w, const char *op_mode)
{
	int vap_guestzone = 0, netusb = 0;
        for (;w != NULL; w = w->next) {
                if (w->en == 0)
                        continue;
                vap_guestzone = get_wlan_vap_gzone(w->bssid);
                netusb = get_wlan_netusb();
		if (w->gidx == 0)
			start_activateVAP(w->ath, 2, 2);
		else
			start_activateVAP(w->ath, vap_guestzone, netusb);
                set_wlan_security(w, op_mode);
                set_mac_filter(w->ath);
        }
	return 0;
}

int start_wlan(int argc, char *argv[]) 
{
	char op_mode[12];
	wlan *w = NULL;

        //For safety, delete all /var/tmp nodes we may re-create
        system("rm -rf /var/tmp/sec*");
        system("rm -rf /var/tmp/topo*");
        system("rm -rf /var/tmp/WSC_ath*");

        sprintf(op_mode, "%s", nvram_safe_get("wlan0_op_mode"));
	if (strlen(op_mode) == 0)
		sprintf(op_mode, "%s", "AP");

        /* Add loopback */
	config_loopback();

	init_wlan(&w, 0); //2.4G
	init_wlan(&w, 1); //5G
	if (w->en != 0) {
                start_insmod();
                set_wlan_mac();
                if (!strcmp(w->band, "2.4G")) {
                        init_wlan0_if(op_mode, w);
                        start_makeVAP(w, op_mode);
                }

                if (!strcmp(w->band, "5G")) {
                        init_wlan1_if(w);
                        start_makeVAP(w, op_mode);
                }
		init_wlan0_mac_to_nvram();
		set_modify_ssid(w);
 		start_ap(w, op_mode);
		set_bridge_arpping_and_led();
		release(w);
	}
	return 0;
}

int stop_wlan(int argc, char *argv[])
{
        char *lan_bridge = NULL;
	int i = 0;
	wlan *w = NULL;

	lan_bridge = nvram_safe_get("lan_bridge");
	init_wlan(&w, 0); //2.4G
	init_wlan(&w, 1); //5G

	if (w == NULL || w->en == 0)
		return 1;	
        for (i = 0;w != NULL; w = w->next, i++) {
                _system("brctl delif %s ath%d", lan_bridge, i);
                _system("ifconfig ath%d down", i);
                _system("wlanconfig ath%d destroy", i);
        }

	stop_insmod();
        _system("kill %d", read_pid("/var/run/gpio.pid"));
        return 1;
}
