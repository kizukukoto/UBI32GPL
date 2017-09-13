#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sutil.h>
#include <nvram.h>

void wlan_ath_setup(int wifi_num, int ath_count)
{
	char *wlan_short_gi = NULL, *wlan_rts_threshold = NULL,
	     *wlan_fragmentation = NULL, *wlan_dtim = NULL, 
	     *wlan_wmm_enable = NULL, *wlan_beacon_interval = NULL;
	int wlan_ssid_broadcast = 0, wlan_partition = 0;

	if (wifi_num == 0) {
		wlan_short_gi=nvram_safe_get("wlan0_short_gi");
		wlan_ssid_broadcast=atoi(nvram_safe_get("wlan0_ssid_broadcast"));
		wlan_rts_threshold=nvram_safe_get("wlan0_rts_threshold");
		wlan_fragmentation = nvram_safe_get("wlan0_fragmentation");
		wlan_dtim = nvram_safe_get("wlan0_dtim");
		wlan_partition = atoi(nvram_safe_get("wlan0_partition"));
		wlan_wmm_enable = nvram_safe_get("wlan0_wmm_enable");
		wlan_beacon_interval = nvram_safe_get("wlan0_beacon_interval");
	} else if(wifi_num == 1) {
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

	if (20 <= atoi(wlan_beacon_interval) && atoi(wlan_beacon_interval) <= 1000)
		_system("iwpriv ath%d bintval %s", ath_count, wlan_beacon_interval);

	if (256 <= atoi(wlan_rts_threshold) && atoi(wlan_rts_threshold) <= 2347)
		_system("iwconfig ath%d rts %s", ath_count, wlan_rts_threshold);

	if (256 <= atoi(wlan_fragmentation) && atoi(wlan_fragmentation) <= 2346)
		_system("iwconfig ath%d frag %s", ath_count, wlan_fragmentation);

	if (1 <= atoi(wlan_dtim) && atoi(wlan_dtim) <= 255)
		_system("iwpriv ath%d dtim_period %s", ath_count, wlan_dtim);
	sleep(1);
}

