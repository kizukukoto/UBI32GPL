#include <leck/label.h>
#include "global.h"
#include "timer.h"
#include "image.h"
#include "menu.h"
#include "label.h"

extern void menu_clock_timer();

static LiteLabel *time_label;

int wlan_page_loop(LiteWindow *win)
{
	lite_set_window_opacity(win, 0xFF);
	timer_option opt;
        opt = (timer_option) {
                .func = menu_clock_timer,
                .timeout = 1000,
                .timeout_id = 10003,
                .data = time_label};

        timer_start(&opt);
	while (true) {
		int x, y;
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_NONE)
			break;
	}

        timer_stop(&opt);
	lite_set_window_opacity(win, 0x00);
	return 0;
}

static void create_label(LiteWindow *win)
{
	char wlan_channel[3] = {'\0'};
	char wlan_enable[12] = {'\0'};
	char wlan_ssid[24] = {'\0'};
	char wlan_guest_ssid[24] = {'\0'};
	char wlan_dot_11_mode[12] = {'\0'};
	char wlan_security[12] = {'\0'};
	char wlan_guest_security[12] = {'\0'};
	char wps_pin[9] = {'\0'};
	char wlan_mac[19] = {'\0'};

	struct data {
		DFBRectangle rect;
		char *txt;
	} info[19] = {
		{{ 100,  20, 320,  20}, "Wireless Status"},
		{{  35,  50, 130,  15}, "Wireless Radio:"},
		{{ 150,  50, 100,  15}, wlan_enable},
		{{  35,  65, 130,  15}, "Wireless Channel:"},
		{{ 165,  65,  10,  15}, wlan_channel},
		{{  35,  80, 200,  15}, "Host NetWork Name(SSID):"},
		{{ 230,  80,  80,  15}, wlan_ssid},
		{{  35,  95, 205,  15}, "Guest NetWork Name(SSID):"},
		{{ 240,  95, 100,  15}, wlan_guest_ssid},
		{{  35, 110, 140,  15}, "802.11n Mode:"},
		{{ 150, 110, 100,  15}, wlan_dot_11_mode},
		{{  35, 125, 200,  15}, "Host Zone Security Mode:"},
		{{ 230, 125, 100,  15}, wlan_security},
		{{  35, 140, 200,  15}, "Guest Zone Security Mode:"},
		{{ 230, 140, 100,  15}, wlan_guest_security},
		{{  35, 155, 100,  15}, "MAC Address:"},
		{{ 140, 155, 150,  15}, wlan_mac},
		{{  35, 170, 130,  15}, "WPS PIN Number:"},
		{{ 165, 170, 100,  15}, wps_pin}
	};
	int i = 0;

#ifdef PC
	strcpy(wlan_channel,"3");
	strcpy(wlan_enable,"Enable");
	strcpy(wlan_ssid,"dlink");
	strcpy(wlan_guest_ssid,"dlink_guest");
	strcpy(wlan_dot_11_mode,"11bgn");
	strcpy(wlan_security,"None");
	strcpy(wlan_guest_security,"None");
	strcpy(wps_pin,"20622336");
	strcpy(wlan_mac,"00:22:b0:5e:aa:24");
#else
	strcpy(wlan_channel, nvram_safe_get("wlan0_channel"));
	if(atoi(nvram_safe_get("wlan0_enable")) == 1){
		strcpy(wlan_enable,"Enable");
	}else{
		strcpy(wlan_enable,"Disable");
	}
	strcpy(wlan_ssid,nvram_safe_get("wlan0_ssid"));
	strcpy(wlan_guest_ssid,nvram_safe_get("wlan0_vap1_ssid"));
	strcpy(wlan_dot_11_mode,nvram_safe_get("wlan0_dot11_mode"));
	strcpy(wlan_security,nvram_safe_get("wlan0_security"));
	strcpy(wlan_guest_security,nvram_safe_get("wlan0_vap1_security"));
	strcpy(wps_pin,nvram_safe_get("wps_pin"));
	strcpy(wlan_mac,nvram_safe_get("lan_mac"));
	//get_ath_macaddr(wlan_mac);
#endif

	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };
	DFBColor color = { 0xFF, 145, 150, 255 };

	time_label = label_create(win,
                                (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
                                NULL,
                                "",
                                12,
                                (DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	label_create(win, info[0].rect, NULL, info[0].txt, 20, title_color);

	for (i = 1; i < 19; i++) {
                if (i%2 == 0)
                        label_create(win, info[i].rect, NULL,
                                info[i].txt, 14, title_color);
                else
                        label_create(win, info[i].rect, NULL,
                                info[i].txt, 14, color);
        }
}

void wlan_start(menu_t parent_menu)
{
	LiteWindow *win;
	LiteImage *bg_image;

	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	bg_image = image_create(win, win_rect, "background.png", 0);

	create_label(win);

	wlan_page_loop(win);

	lite_destroy_window(win);
}
