#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wlan.h"

extern int get_wlan_en(int, int);

extern void set_wlan(wlan *, int, int);
extern void set_wds_en(wlan *, int, int);
extern void set_wlan_ssid(char *, int, int);
extern void set_rekey_time(char *, int, int);
extern void set_security(char *, int, int);
extern void set_wep(wlan *, int, int);
extern void set_psk(wlan *, int, int);
extern void set_eap(wlan *, int, int);

void display_wlan(wlan *w)
{
	for (;w != NULL; w = w->next) {
		printf("ath : %d, security : %s\n", w->ath, w->security);
	}
}

void init_wlan_security(wlan *w, int bssid, int gidx)
{
	if (strstr(w->security, "wep")) {
		set_wep(w, bssid, gidx);
	} else if (strstr(w->security, "_psk")) {
		set_psk(w, bssid, gidx);
	} else if (strstr(w->security, "_eap")) {
		set_psk(w, bssid, gidx);
		set_eap(w, bssid, gidx);
	} else {
                strcpy(w->security, "disable");
	}
}

void init_data(wlan *w, int band, int gidx, int ath)
{
	strcpy(w->band, band == 0 ? "2.4G" : "5G");
	set_wlan(w, band, gidx);
	w->bssid = band;
	w->ath = ath;
	w->gidx = gidx;
	w->en = get_wlan_en(band, gidx);
	set_wds_en(w, band, gidx);
	set_wlan_ssid(w->ssid, band, gidx);
	set_rekey_time(w->wpa.rekey_timeout, band, gidx);
	set_security(w->security, band, gidx);
	init_wlan_security(w, band, gidx);
	w->next = NULL;
}

wlan *init_wlan_data(wlan *w, int band, int gidx, int ath)
{
	wlan *head = NULL, *cur = NULL, *p = NULL;

	p = (wlan *)malloc(sizeof(wlan));
	if (p == NULL)
		goto out;
	init_data(p, band, gidx, ath);
	if (w == NULL) {
		w = p;
	} else {
		head = w; 
		/* move to the end node */
		for (;head != NULL; head = head->next)
			cur = head;
		cur->next = p;
	}
	return w;
out:
	return NULL;
}

int chk_init(wlan *w) 
{
	int ret = 0;

	if (w != NULL)
		ret = 1;
	return ret;
}

wlan *init_wlan(wlan **w, int band)
{
	int gidx = 0, ath = 0;
	for (gidx = 0; gidx < 4; gidx++) {
		if (gidx == 0 && get_wlan_en(band, 0) != 1) {
			break;
		}
		if (get_wlan_en(band, gidx)) {
			*w = init_wlan_data(*w, band, gidx, ath);
			if (!chk_init(*w)) {
				printf("Init Error!\n");
				break;
			}
			ath++;
		}
	}

//	if (*w != NULL) 
//		display_wlan(*w);
	return *w;
}

void release(wlan *w)
{
	if (w->next != NULL)
		release(w->next);
//	printf("XX %s(%d), ath:%d, security: %s\n", __func__, __LINE__, w->ath, w->security);
	free(w);
}
