#include <lite/window.h>
#include <leck/image.h>

#include "global.h"
#include "image.h"
#include "wizard.h"

static pppoe_account pppoe_ac;
static wizard_page *p, page_list[] = {
#ifdef LOAD_INTO_DDR
	{ NULL, wz_welcome_ui_1, wz_welcome_evt_1, NULL, 0 },
	{ NULL, wz_welcome_ui_2, wz_welcome_evt_2, NULL, 0 },
	{ NULL, wz_install_ui_1, wz_install_evt_1, NULL, 0 },
	{ NULL, wz_install_ui_2, wz_install_evt_2, NULL, 0 },
	{ NULL, wz_install_ui_3, wz_install_evt_3, NULL, 0 },
	{ NULL, wz_install_ui_4, wz_install_evt_4, NULL, 0 },
	{ NULL, wz_install_ui_5, wz_install_evt_5, NULL, 0 },
	{ NULL, wz_network_ui_1, wz_network_evt_1, NULL, 0 },		/* wt detect */
	{ NULL, wz_network_ui_2, wz_network_evt_2, NULL, 0 },		/* dhcp page */
	{ NULL, wz_network_ui_3, wz_network_evt_3, NULL, 0 },		/* dhcp page */
	{ NULL, wz_network_ui_4, wz_network_evt_4, &pppoe_ac, 0 },	/* pppoe page */
	{ NULL, wz_wireless_ui_1, wz_wireless_evt_1, NULL, 0 },		/* 2.4G network name */
	{ NULL, wz_wireless_ui_2, wz_wireless_evt_2, NULL, 0 },		/* 2.4G network key */
	{ NULL, wz_wireless_ui_3, wz_wireless_evt_3, NULL, 0 },		/* 5G network name */
	{ NULL, wz_wireless_ui_4, wz_wireless_evt_4, NULL, 0 },		/* 5G network key */
	{ NULL, wz_wireless_ui_5, wz_wireless_evt_5, NULL, 0 },		/* optimized */
	{ NULL, wz_timezone_ui, wz_timezone_evt, NULL, 0},		/* timezone */
	{ NULL, wz_summary_ui, wz_summary_evt, NULL, 0},
	{ NULL, wz_final_ui, wz_final_evt, NULL, 0},
	{ NULL, NULL, NULL, NULL, 0 }
#else
	{ wz_welcome_ui_1, wz_welcome_evt_1, NULL },
	{ wz_welcome_ui_2, wz_welcome_evt_2, NULL },
	{ wz_install_ui_1, wz_install_evt_1, NULL },
	{ wz_install_ui_2, wz_install_evt_2, NULL },
	{ wz_install_ui_3, wz_install_evt_3, NULL },
	{ wz_install_ui_4, wz_install_evt_4, NULL },
	{ wz_install_ui_5, wz_install_evt_5, NULL },
	{ wz_network_ui_1, wz_network_evt_1, NULL },	  /* wt detect */
	{ wz_network_ui_2, wz_network_evt_2, NULL },	  /* dhcp page */
	{ wz_network_ui_3, wz_network_evt_3, NULL },	  /* dhcp page */
	{ wz_network_ui_4, wz_network_evt_4, &pppoe_ac }, /* pppoe page */
	{ wz_wireless_ui_1, wz_wireless_evt_1, NULL },	  /* 2.4G network name */
	{ wz_wireless_ui_2, wz_wireless_evt_2, NULL },	  /* 2.4G network key */
	{ wz_wireless_ui_3, wz_wireless_evt_3, NULL },	  /* 5G network name */
	{ wz_wireless_ui_4, wz_wireless_evt_4, NULL },	  /* 5G network key */
	{ wz_wireless_ui_5, wz_wireless_evt_5, NULL },	  /* optimized */
	{ wz_timezone_ui, wz_timezone_evt, NULL },	  /* timezone */
	//{ wz_timezone_area_ui, wz_timezone_area_evt, NULL },
	{ wz_summary_ui, wz_summary_evt, NULL },
	{ wz_final_ui, wz_final_evt, NULL },
	{ NULL }
#endif
};

#ifndef LOAD_INTO_DDR
static LiteWindow *wizard_window_init()
{
	LiteWindow *win;
	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);
	image_create(win, win_rect, "wizard_bg.png", 0);

	return win;
}
#endif

#ifdef LOAD_INTO_DDR
int wizard_start()
{
	do_page_preload_all(page_list);

	p = page_list;
	while (p && p->evt_func) {
		int ret;

		do_page_visible(p->win, TRUE);
		switch(ret = p->evt_func(p->win, &p)) {
		case PAGE_BEGIN:
		case PAGE_NEXT:
			do_page_next(&p);
			break;
		case PAGE_BACK:
			do_page_back(&p);
			break;
		case PAGE_NOTHANKS:
		case PAGE_CANCLE:
			break;
		case PAGE_SAVESETTING:
			wz_nvram_set("lcd_need_run_wizard", "0");
			wz_nvram_commit();
			system("reboot");
		}

		if (ret == PAGE_NOTHANKS || ret == PAGE_CANCLE)
			break;
	}

	do_page_distroy_all(page_list);

	return 0;
}
#else
int wizard_start()
{
	LiteWindow *win, *prev_win = NULL, *prev_win2 = NULL;

	p = page_list;

	while (p && p->ui_func) {
		win = wizard_window_init();

		p->ui_func(win, &p);
		lite_window_event_loop(win, 1);

		if (prev_win2 != NULL)
			lite_destroy_window(prev_win2);

		switch(p->evt_func(win, &p)) {
		case PAGE_PREV:
		case PAGE_BACK:
			p--;
			break;
		case PAGE_NOTHANKS:
		case PAGE_CANCLE:
			goto cancle;
		case PAGE_NEXT:
		case PAGE_BEGIN:
			p++;
			break;
		case PAGE_STARTOVER:
			p = page_list;
			wz_nvram_reset();
			break;
		case PAGE_SAVESETTING:
			wz_nvram_set("lcd_need_run_wizard", "0");
			wz_nvram_commit();
			system("reboot");
			break;
		}

		prev_win2 = prev_win;
		prev_win = win;
	}
cancle:
	lite_destroy_window(prev_win2);
	lite_destroy_window(prev_win);
	lite_destroy_window(win);

	return 0;
}
#endif
