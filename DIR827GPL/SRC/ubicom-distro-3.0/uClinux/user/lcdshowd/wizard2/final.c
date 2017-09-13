#include <lite/window.h>
#include <leck/image.h>
#include <leck/textbutton.h>
#include <leck/label.h>

#include "global.h"
#include "image.h"
#include "label.h"
#include "wizard.h"

int wz_summary_ui(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	char *ptr, *tz1, *tz2, *tz3, *tz4, tmp_tz[256];
	DFBColor title_color = {0xFF, 0x66, 0x99, 0xFF};

	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  80,  20, 270, 20 }, "Settings Summary", COLOR_WHITE, 18 },
		{{  63,  40, 160, 20 }, "Connection Type: ", title_color, 10 },
		//{{ 160,  40, 270, 20 }, "", COLOR_WHITE, 10 },
		{{  59,  55, 160, 20 }, "PPPoE User Name: ", title_color, 10 },
		//{{ 160,  55, 270, 20 }, "", COLOR_WHITE, 10 },
		{{  71,  70, 160, 20 }, "PPPoE Password: ", title_color, 10 },
		//{{ 160,  70, 270, 20 }, "", COLOR_WHITE, 10 },
		{{  42,  85, 170, 20 }, "Cloned MAC Address: ", title_color, 10 },
		//{{ 160,  85, 270, 20 }, "", COLOR_WHITE, 10 },
		{{  35, 100, 170, 20 }, "2.4GHz Network Name: ", title_color, 10 },
		//{{ 160, 100, 270, 20 }, "", COLOR_WHITE, 12 },
		{{   0, 115, 190, 20 }, "2.4GHz Wireless Network Key: ", title_color, 10 },
		//{{ 160, 115, 270, 20 }, "", COLOR_WHITE, 12 },
		{{  43, 130, 170, 20 }, "5GHz Network Name: ", title_color, 10 },
		//{{ 160, 130, 270, 20 }, "", COLOR_WHITE, 12 },
		{{  11, 145, 190, 20 }, "5GHz Wireless Network key: ", title_color, 10 },
		//{{ 160, 145, 270, 20 }, "", COLOR_WHITE, 12 },
		{{  97, 160, 150, 20 }, "TimeZone: ", title_color, 10 },
		//{{ 160, 160, 270, 20 }, "", COLOR_WHITE, 10 },
		{{ 160, 175, 270, 20 }, "", COLOR_WHITE, 10 },
		{{ 160, 190, 270, 20 }, "", COLOR_WHITE, 10 },
		{{ 160, 205, 270, 20 }, "", COLOR_WHITE, 10 }
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76,  55 },
		{ 122, 190,  76,  55 },
		{ 206, 190,  76,  55 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);

	//for (i = 0; i < 22; i++) {
	for (i = 0; i < 13; i++) {
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
        }

#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_summary_evt(LiteWindow *win, wizard_page **p)
{
	static LiteLabel *conntype;
	static LiteLabel *poe_usr, *poe_pwd;
	static LiteLabel *wan_mac;
	static LiteLabel *ssid_24g, *ssid_5g;
	static LiteLabel *key_24g, *key_5g;
	static LiteLabel *timezone;

	if (poe_usr == NULL) {
		conntype = label_create(win, (DFBRectangle){ 160,  40, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		poe_usr = label_create(win, (DFBRectangle){ 160, 55, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		poe_pwd = label_create(win, (DFBRectangle){ 160, 70, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		wan_mac = label_create(win, (DFBRectangle){ 160, 85, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		ssid_24g = label_create(win, (DFBRectangle){ 160, 100, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		key_24g = label_create(win, (DFBRectangle){ 160, 115, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		ssid_5g = label_create(win, (DFBRectangle){ 160, 130, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		key_5g = label_create(win, (DFBRectangle){ 160, 145, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
		timezone = label_create(win, (DFBRectangle){ 160, 160, 270, 20 },
				NULL, "", 10, COLOR_WHITE);
	}

	set_label_text(conntype, info_load("conntype"));
	set_label_text(poe_usr, wz_nvram_get_local("wan_pppoe_username_00"));
	set_label_text(poe_pwd, wz_nvram_get_local("wan_pppoe_password_00"));
	set_label_text(wan_mac, wz_nvram_get_local("wan_mac"));
	set_label_text(ssid_24g, wz_nvram_get_local("wlan0_ssid"));
	set_label_text(key_24g, wz_nvram_get_local("wlan0_psk_pass_phrase"));
	set_label_text(ssid_5g, wz_nvram_get_local("wlan1_ssid"));
	set_label_text(key_5g, wz_nvram_get_local("wlan1_psk_pass_phrase"));
	set_label_text(timezone, info_load("timezone"));

	return option_return_click(win, PAGE_BACK | PAGE_CANCLE | PAGE_NEXT);
}

int wz_final_ui(LiteWindow *win, wizard_page **p)
{
	int i = 0;

	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
                {{ 70, 20, 270, 20 }, "Save Settings and Connect", COLOR_WHITE, 14 }
        };

	DFBRectangle img_rect[] = {
		{ 122, 190,  76, 55 },
		{  23,  50, 284, 83 },
		{  23, 100, 284, 83 }
	};

	image_create(win, img_rect[0], "wizard_cancel.png", 0);
	image_create(win, img_rect[1], "wizard_savesettings.png", 0);
	image_create(win, img_rect[2], "wizard_startover.png", 0);

	for (; i < 1; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_final_evt(LiteWindow *win, wizard_page **p)
{
#ifdef LOAD_INTO_DDR
	int ret = option_return_click(win, PAGE_CANCLE | PAGE_SAVESETTING | PAGE_STARTOVER);

	(*p)->offset = 0;

	if (ret == PAGE_STARTOVER) {
		(*p)->offset = -18;
		ret = PAGE_NEXT;
	}

	return ret;
#else
	return option_return_click(win, PAGE_CANCLE | PAGE_SAVESETTING | PAGE_STARTOVER);
#endif
}
