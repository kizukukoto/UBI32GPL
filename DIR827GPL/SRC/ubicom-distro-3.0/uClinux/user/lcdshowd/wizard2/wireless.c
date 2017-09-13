#include <lite/window.h>
#include <leck/textbutton.h>

#include "global.h"
#include "misc.h"
#include "image.h"
#include "label.h"
#include "timer.h"
#include "mouse.h"
#include "input.h"
#include "wizard.h"

int wz_wireless_ui_1(LiteWindow *win, wizard_page **p)
{
	int i = 0;

	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  50, 20, 270, 20 }, "2.4GHz", { 0xFF, 0x66, 0x99, 0xFF }, 18 },
		{{ 120, 20, 270, 20 }, "Wireless Settings", COLOR_WHITE, 18},
		{{  15, 60, 320, 20 }, "Give your network a name, using up to 32 characters.", COLOR_WHITE, 11 }
	}, _txtline[] = {
		{{  60, 100, 130, 20 }, "Network Name ", COLOR_WHITE, 12 },
		{{ 150, 100, 150, 20 }, "", COLOR_WHITE, 12 }
	};

        DFBRectangle img_rect[] = {
		{  36, 190,  76,  55 },
		{ 122, 190,  76,  55 },
		{ 206, 190,  76,  55 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);

        for (i = 0; i < 3; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	label_create(win, _txtline[0].rect, NULL, _txtline[0].txt,
		_txtline[0].font_size, _txtline[0].def_color);
	lite_new_textline(LITE_BOX(win), &_txtline[1].rect,
		liteNoTextLineTheme, (LiteTextLine **)&(*p)->param);
	lite_set_textline_text((LiteTextLine *)(*p)->param,
		wz_nvram_safe_get("wlan0_ssid"));
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_wireless_evt_1(LiteWindow *win, wizard_page **p)
{
	int ret, opt_idx, x, y;
	DFBRegion btr[] = {
		{  36, 192, 112, 218 },	/* Back */
		{ 122, 190, 206, 218 },	/* Cancle */
		{ 216, 190, 292, 218 },	/* Next */
		{ 150, 100, 300, 120 }	/* TextLine */
	};

#ifdef LOAD_INTO_DDR
	(*p)->offset = 0;
#endif

	while (true) {
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			opt_idx = bt_ripple_handler(x, y, btr, 4);

			if (opt_idx == -1)
				continue;
			else if (opt_idx == 3) {
				keyboard_input("Network Name", (*p)->param);
				continue;
			} else if (opt_idx == 0) {
				ret = PAGE_BACK;
				/* Jump back to WAN type detecting page */
#ifndef LOAD_INTO_DDR
				(*p) -= 3;
#else
				(*p)->offset = -4;
#endif
			} else if (opt_idx == 1)
				ret = PAGE_CANCLE;
			else
				ret = PAGE_NEXT;
			break;
				
		}
	}

	if (ret == PAGE_NEXT) {
		char *wlan0_ssid;

		lite_get_textline_text((LiteTextLine *)(*p)->param, &wlan0_ssid);
		wz_nvram_set("wlan0_ssid", wlan0_ssid);
	}

	return ret;
}

int wz_wireless_ui_2(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  50, 20, 270, 20 }, "2.4GHz", { 0xFF, 0x66, 0x99, 0xFF }, 18 },
		{{ 120, 20, 270, 20 }, "Wireless Settings", COLOR_WHITE, 18 },
		{{  80, 60, 270, 20 }, "Please assign a network key", COLOR_WHITE, 12 },
		{{  60, 80, 270, 20 }, "(8-to 63-character alphanumerically)", COLOR_WHITE, 12 }
	}, _txtline[] = {
		{{  60, 100, 150, 20 }, "Network Key", COLOR_WHITE, 12 },
		{{ 150, 100, 150, 20 }, "", COLOR_WHITE, 12 }
	};

	DFBRectangle img_rect[] = {
		{  36, 190, 76, 55 },
		{ 122, 190, 76, 55 },
		{ 206, 190, 76, 55 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);

	for (; i < 4; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
	label_create(win,
		_txtline[0].rect, NULL,
		_txtline[0].txt, _txtline[0].font_size, _txtline[0].def_color);
	lite_new_textline(LITE_BOX(win), &_txtline[1].rect,
		liteNoTextLineTheme, (LiteTextLine **)&(*p)->param);
	lite_set_textline_text((LiteTextLine *)(*p)->param,
		wz_nvram_safe_get("wlan0_psk_pass_phrase"));
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_wireless_evt_2(LiteWindow *win, wizard_page **p)
{
	int ret, opt_idx, x, y;
	DFBRegion btr[] = {
		{  36, 192, 112, 218 }, /* Back */
		{ 122, 190, 206, 218 }, /* Cancle */
		{ 216, 190, 292, 218 }, /* Next */
		{ 150, 100, 300, 120 }  /* TextLine */
	};

	while (true) {
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			opt_idx = bt_ripple_handler(x, y, btr, 4);

			if (opt_idx == -1)
				continue;
			else if (opt_idx == 3) {
				keyboard_input("Network Key", (*p)->param);
				continue;
			} else if (opt_idx == 0)
				ret = PAGE_BACK;
			else if (opt_idx == 1)
				ret = PAGE_CANCLE;
			else
				ret = PAGE_NEXT;
			break;
		}
	}

	if (ret == PAGE_NEXT) {
		char *wlan0_pwd;

		lite_get_textline_text((LiteTextLine *)(*p)->param, &wlan0_pwd);
		wz_nvram_set("wlan0_psk_pass_phrase", wlan0_pwd);
	}


	return ret;
}

int wz_wireless_ui_3(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  60, 20, 270, 20 }, "5GHz", { 0xFF, 0x66, 0x99, 0xFF }, 18 },
		{{ 120, 20, 270, 20 }, "Wireless Settings", COLOR_WHITE, 18 },
		{{  30, 60, 290, 20 }, "Give your            network a name, using up to", COLOR_WHITE, 12 },
		{{  95, 60, 270, 20 }, "media", { 0xFF, 0x66, 0x99, 0xFF }, 12 },
		{{  30, 80, 270, 20 }, "32 characters.", COLOR_WHITE, 12 }
	}, _txtline[] = {
		{{  60, 100, 100, 20 }, "Network Name ", COLOR_WHITE, 12 },
		{{ 150, 100, 150, 20 }, "", COLOR_WHITE, 12 }
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);

	for (; i < 5; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	label_create(win, _txtline[0].rect, NULL,
		_txtline[0].txt, _txtline[0].font_size, _txtline[0].def_color);
	lite_new_textline(LITE_BOX(win), &_txtline[1].rect,
		liteNoTextLineTheme, (LiteTextLine **)&(*p)->param);
	lite_set_textline_text((LiteTextLine *)(*p)->param,
		wz_nvram_safe_get("wlan1_ssid"));
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_wireless_evt_3(LiteWindow *win, wizard_page **p)
{
	int ret, opt_idx, x, y;
	DFBRegion btr[] = {
		{  36, 192, 112, 218 }, /* Back */
		{ 122, 190, 206, 218 }, /* Cancle */
		{ 216, 190, 292, 218 }, /* Next */
		{ 150, 100, 300, 120 }  /* TextLine */
	};

	while (true) {
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			opt_idx = bt_ripple_handler(x, y, btr, 4);

			if (opt_idx == -1)
				continue;
			else if (opt_idx == 3) {
				keyboard_input("Network Name", (*p)->param);
				continue;
			} else if (opt_idx == 0)
				ret = PAGE_BACK;
			else if (opt_idx == 1)
				ret = PAGE_CANCLE;
			else
				ret = PAGE_NEXT;
			break;
		}
	}

	if (ret == PAGE_NEXT) {
		char *wlan1_ssid;

		lite_get_textline_text((LiteTextLine *)(*p)->param, &wlan1_ssid);
		wz_nvram_set("wlan1_ssid", wlan1_ssid);
	}

	return ret;
}

int wz_wireless_ui_4(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  60, 20, 270, 20 }, "5GHz", { 0xFF, 0x66, 0x99, 0xFF }, 18 },
		{{ 120, 20, 270, 20 }, "Wireless Settings", COLOR_WHITE, 18 },
		{{  80, 60, 270, 20 }, "Please assign a network key.", COLOR_WHITE, 12 },
		{{  60, 80, 270, 20 }, "(8-to 63-character alphanumerically).", COLOR_WHITE, 12 },
	}, _txtline[] = {
		{{  60, 100, 270, 20 }, "Network Key : ", COLOR_WHITE, 12 },
		{{ 150, 100, 150, 20 }, "", COLOR_WHITE, 12 }
	};

        DFBRectangle img_rect[] = {
		{  36, 190,  76,  55 },
		{ 122, 190,  76,  55 },
		{ 206, 190,  76,  55 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);

	for (; i < 4; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	label_create(win, _txtline[0].rect, NULL,
		_txtline[0].txt, _txtline[0].font_size, _txtline[0].def_color);
	lite_new_textline(LITE_BOX(win), &_txtline[1].rect,
		liteNoTextLineTheme, (LiteTextLine **)&(*p)->param);
	lite_set_textline_text((LiteTextLine *)(*p)->param,
		wz_nvram_safe_get("wlan1_psk_pass_phrase"));
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_wireless_evt_4(LiteWindow *win, wizard_page **p)
{
	int ret, opt_idx, x, y;
	DFBRegion btr[] = {
		{  36, 192, 112, 218 }, /* Back */
		{ 122, 190, 206, 218 }, /* Cancle */
		{ 216, 190, 292, 218 }, /* Next */
		{ 150, 100, 300, 120 }  /* TextLine */
	};

	while (true) {
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			opt_idx = bt_ripple_handler(x, y, btr, 4);

			if (opt_idx == -1)
				continue;
			else if (opt_idx == 3) {
				keyboard_input("Network Key", (*p)->param);
				continue;
			} else if (opt_idx == 0)
				ret = PAGE_BACK;
			else if (opt_idx == 1)
				ret = PAGE_CANCLE;
			else
				ret = PAGE_NEXT;
			break;
		}
	}

	if (ret == PAGE_NEXT) {
		char *wlan1_pwd;

		lite_get_textline_text((LiteTextLine *)(*p)->param, &wlan1_pwd);
		wz_nvram_set("wlan1_psk_pass_phrase", wlan1_pwd);
	}

	return ret;
}

int wz_wireless_ui_5(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{ 30, 20, 270, 20 }, "Optimize Channel bandwidth", COLOR_WHITE, 18 },
		{{  5, 60, 320, 20 }, "Do you want the setup wizard to automatically optimize", COLOR_WHITE, 11 },
		{{  5, 80, 320, 20 }, "channel bandwidth to increase the speed of your router?", COLOR_WHITE, 11 },
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76, 55 },
		{ 122, 190,  76, 55 },
		{  30,  80, 133, 83 },
		{ 170,  80, 133, 83 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_yes_1.png", 0);
	image_create(win, img_rect[3], "wizard_no.png", 0);

	for (; i < 3; i++)
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

int wz_wireless_evt_5(LiteWindow *win, wizard_page **p)
{
	int ret = option_return_click(win,
			PAGE_YES | PAGE_NO | PAGE_BACK | PAGE_CANCLE);

	if (ret == PAGE_YES)
		wz_nvram_set("wlan0_11n_protection", "auto");
	ret = PAGE_NEXT;

        return ret;
}
