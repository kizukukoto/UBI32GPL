#include <lite/window.h>
#include <leck/progressbar.h>
#include <leck/textbutton.h>
#include <leck/textline.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "global.h"
#include "misc.h"
#include "image.h"
#include "label.h"
#include "timer.h"
#include "mouse.h"
#include "check.h"
#include "input.h"
#include "wizard.h"

static timer_option opt_proc;
static struct __detect_struct {
	char txt[256];
	LiteProgressBar *progress;
	float degree;
} kboard;

static int opt_i = 0;
static char *opt[256] = { NULL };

static void auto_detect_processbar(void *data)
{
	struct __detect_struct *kboard = (struct __detect_struct *)data;
	if (kboard->degree <= 1.0) {
		kboard->degree += 0.3;
		lite_set_progressbar_value(kboard->progress, kboard->degree);

		if (kboard->degree >= 0.5 && kboard->degree < 1.0) {
			int tmp, k = system("wt eth0");

			wait(&k);
			tmp = WEXITSTATUS(k);

			if (tmp <= 2) {
				printf("dhcpc\n");
				strcpy(kboard->txt, STR_2_GUI_WAN_PROTO_DHCPC);
			} else if (tmp == 3) {
				printf("pppoe\n");
				strcpy(kboard->txt, STR_2_GUI_WAN_PROTO_PPPOE);
			}
			kboard->degree = 0.7;
		}
	}
}

int wz_network_ui_1(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	char fullname[MAX_FILENAME];
	char fullname_tmp[MAX_FILENAME];

	sprintf(fullname, "%swizard_D1_bar.png", IMAGE_DIR);
	sprintf(fullname_tmp, "%swizard_D1_bar_bg.png", IMAGE_DIR);

	DFBRectangle bar_rect = {20, 100, 280, 24};

        struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
        } component_list[] = {
		{{ 80, 20, 270, 20 }, "Internet Connecttion", COLOR_WHITE, 18 },
		{{ 10, 60, 300, 20 }, "The router is detecting your Internet Connection.", COLOR_WHITE, 12 }
	};

	DFBRectangle img_rect[] = {
		{ 122, 190,  76,  55 },
		{ 206, 190,  76,  55 }
	};

	image_create(win, img_rect[0], "wizard_cancel.png", 0);

	for ( i = 0; i < 2; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	lite_new_progressbar(LITE_BOX(win), &bar_rect, liteNoProgressBarTheme, &kboard.progress);
	lite_set_progressbar_images(kboard.progress, fullname, fullname_tmp);
	lite_set_progressbar_value(kboard.progress, 0.0);
	kboard.degree = 0.0;

	opt_proc = (timer_option) {
		.func = auto_detect_processbar,
		.timeout = 1000,
		.timeout_id = 11000,
		.data = &kboard
	};
#ifndef LOAD_INTO_DDR
	timer_start(&opt_proc);
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_network_evt_1(LiteWindow *win, wizard_page **p)
{
	int x, y, ret = PAGE_UNDEFINE;

	kboard.degree = 0.0;
#ifdef LOAD_INTO_DDR
	timer_start(&opt_proc);
	(*p)->offset = 0;
#endif
	while (true) {
		if (kboard.degree >= 1.0) {
			ret = PAGE_NEXT;
			break;
		} else {
			DFBWindowEventType type = mouse_handler(win, &x, &y, 1);
			if (type == DWET_BUTTONDOWN) {
				static DFBRegion btr = { 122, 190, 206, 245 };	/* Cancle */
				if (bt_ripple_handler(x, y, &btr, 1) == 0)
					break;
			}
		}
	}

	timer_stop(&opt_proc);

	if (strcmp(kboard.txt, STR_2_GUI_WAN_PROTO_PPPOE) == 0)
#ifndef LOAD_INTO_DDR
		(*p) += 2;	/* skip dhcp settings */
#else
	{
		info_save("conntype", "PPPoE");
		(*p)->offset = 3;
		ret = PAGE_NEXT;
	} else
		info_save("conntype", "DHCP");
#endif

	return ret;
}

int wz_network_ui_2(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
        } component_list[] = {
		{{ 90, 20, 270, 20 }, "DHCP Connection", COLOR_WHITE, 18 },
		{{ 50, 60, 270, 20 }, "Are you a Cable Broadband subscriber?", COLOR_WHITE, 12 }
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76, 55 },
		{ 122, 190,  76, 55 },
		{  30,  80, 133, 83 },
		{ 170,  80, 133, 83 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_yes.png", 0);
	image_create(win, img_rect[3], "wizard_no.png", 0);

	for (i = 0; i < 2; i++)
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

int wz_network_evt_2(LiteWindow *win, wizard_page **p)
{
	int ret = option_return_click(win,
			PAGE_YES | PAGE_NO | PAGE_BACK | PAGE_CANCLE);

#ifdef LOAD_INTO_DDR
	(*p)->offset = 0;
#endif

	if (ret == PAGE_YES)
		ret = PAGE_NEXT;
	else if (ret == PAGE_NO) {
		ret = PAGE_NEXT;
		/* Skip MAC select page and PPPoE setting page */
#ifndef LOAD_INTO_DDR
		(*p) += 2;
#else
		(*p)->offset = 3;
#endif
	}

	wz_nvram_set("wan_mac", wz_nvram_safe_get("wan_mac"));
	wz_nvram_set("mac_clone_addr", wz_nvram_safe_get("wan_mac"));

	return ret;
}

int wz_network_ui_3(LiteWindow *win, wizard_page **p)
{
	int i = 0, j = 0;
	char tmp[1024],  tmp1[128], *pp;
	char *wan_mac = wz_nvram_safe_get("wan_mac");
	LiteTextButton *btn;
	LiteTextButtonTheme *textButtonTheme;
	DFBRectangle text_rect = { 140, 95, 160, 20 };
	DFBRectangle btn_rect = { 300, 95, 20, 20 };

	bzero(tmp, sizeof(tmp));
	bzero(tmp1, sizeof(tmp1));

	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{ 90, 20, 270, 20 }, "DHCP Connection", COLOR_WHITE, 18 },
		{{ 10, 50, 320, 20 }, "Please make sure that you are connected to the D-Link", COLOR_WHITE, 11 },
		{{ 10, 65, 320, 20 }, "Router with the PC that was originally connected to", COLOR_WHITE, 11 },
		{{ 10, 80, 320, 20 }, "your broadband connection.", COLOR_WHITE, 11 },
	}, _txtline[] = {
		{{  40,  95, 100, 20 }, "Select Your PC : ", COLOR_WHITE, 12} ,
		{{ 140,  95, 150, 20 }, "", COLOR_WHITE, 12 },
		{{ 140, 120, 150, 40 }, "", COLOR_WHITE, 12 }
	};

	DFBRectangle img_rect[] = {
		{  36, 190, 76, 55 },
		{ 122, 190, 76, 55 },
		{ 206, 190, 76, 55 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);

	for (i = 0; i < 4; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	label_create(win,
		_txtline[0].rect,
		NULL,
		_txtline[0].txt,
		_txtline[0].font_size,
		_txtline[0].def_color);
	lite_new_textline(LITE_BOX(win),
		&text_rect,
		liteNoTextLineTheme,
		(LiteTextLine **)&(*p)->param);
	lite_new_text_button_theme(IMAGE_DIR"textbuttonbgnd.png", &textButtonTheme);
	lite_new_text_button(LITE_BOX(win), &btn_rect, "Yes", textButtonTheme, &btn);

	get_local_lan(tmp, &j);
	pp = tmp;

	if (wan_mac) {
		opt[opt_i++] = strdup(wan_mac);
		if (opt_i == 1);
			lite_set_textline_text((LiteTextLine *)(*p)->param, opt[0]);
	}
	if (strlen(pp) > 0 && j != 0) {
		for (i = 0; i < j && strlen(pp); i++) {
			if (check_mac(pp) == -1) break;
			sscanf(pp, "%s", tmp1);
			if (wan_mac) {
				if (strncmp(tmp1, wan_mac, strlen(wan_mac)) != 0) {
					opt[opt_i++] = strdup(tmp1);
					printf("XXX %s\n", tmp1);
					fflush(stdout);
				}
			} else {
				opt[opt_i++] = strdup(tmp1);
				printf("XXX %s\n", tmp1);
				fflush(stdout);
			}
			pp += 24;

			if (opt_i == 1)
				lite_set_textline_text((LiteTextLine *)(*p)->param, tmp1);
		}
	}

	opt[opt_i] = NULL;
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_network_evt_3(LiteWindow *win, wizard_page **p)
{
	int i, ret, opt_idx, x, y;
	DFBRegion btr[] = {
		{  36, 192, 112, 218 }, /* Back */
		{ 122, 190, 206, 218 }, /* Cancle */
		{ 216, 190, 292, 218 }, /* Next */
		{ 150, 100, 300, 120 }  /* TextLine */
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
				select_input(opt, "Select Your PC:", (LiteTextLine *)(*p)->param, EDIT_ENABLE);
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
		char *text;

		lite_get_textline_text((LiteTextLine *)(*p)->param, &text);
		wz_nvram_set("wan_mac", text);
		wz_nvram_set("mac_clone_addr", text);
#ifdef LOAD_INTO_DDR
		(*p)->offset = 2;
#endif
	}

	for (i = 0; i < opt_i; i++)
		free(opt[i]);
	opt_i = 0;
	opt[0] = NULL;
#ifndef LOAD_INTO_DDR
	(*p)++;	/* Skip PPPoE Settings */
#endif
	return ret;
}

int wz_network_ui_4(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	pppoe_account *pppoe_ac = (*p)->param;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{ 80, 20, 270, 20 }, "PPPoE Connection", COLOR_WHITE, 18 },
		{{ 10, 60, 310, 20 }, "To set up this connection you will need to have a User", COLOR_WHITE, 11 },
		{{ 10, 75, 310, 20 }, "Name and Password from your Internet Service Provider.", COLOR_WHITE, 11 }
	}, _txtline[] = {
		{{  60, 140, 100, 20 }, "User Name : ", COLOR_WHITE, 12 },
		{{ 140, 140, 150, 20 }, "", COLOR_WHITE, 12 },
		{{  60, 160, 100, 20 }, "Password  : ", COLOR_WHITE, 12 },
		{{ 140, 160, 150, 20 }, "", COLOR_WHITE, 12 }
	};

	DFBRectangle img_rect[] = {
		{  36, 190, 76, 55 },
		{ 122, 190, 76, 55 },
		{ 206, 190, 76, 55 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);

	for (; i < 3; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	label_create(win,
		_txtline[0].rect,
		NULL,
		_txtline[0].txt,
		_txtline[0].font_size,
		_txtline[0].def_color );
	lite_new_textline(LITE_BOX(win),
		&_txtline[1].rect,
		liteNoTextLineTheme,
		&pppoe_ac->text_username);
        label_create(win,
		_txtline[2].rect,
		NULL,
		_txtline[2].txt,
		_txtline[2].font_size,
		_txtline[2].def_color );
	lite_new_textline(LITE_BOX(win),
		&_txtline[3].rect,
		liteNoTextLineTheme,
		&pppoe_ac->text_password);

	lite_set_textline_text(pppoe_ac->text_username,
		wz_nvram_safe_get("wan_pppoe_username_00"));
	lite_set_textline_text(pppoe_ac->text_password,
		wz_nvram_safe_get("wan_pppoe_password_00"));
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_network_evt_4(LiteWindow *win, wizard_page **p)
{
	int ret, opt_idx, x, y;
	DFBRegion btr[] = {
		{  36, 192, 112, 218 },	/* Back */
		{ 122, 190, 206, 218 },	/* Cancle */
		{ 216, 190, 292, 218 },	/* Next */
		{ 140, 140, 290, 160 },	/* Username */
		{ 140, 160, 290, 180 }	/* Password */
	};

#ifdef LOAD_INTO_DDR
	(*p)->offset = 0;
#endif
	while (true) {
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			opt_idx = bt_ripple_handler(x, y, btr, 5);

			if (opt_idx == -1)
				continue;
			else if (opt_idx == 3) {
				keyboard_input("Username",
				((pppoe_account *)(*p)->param)->text_username);
				continue;
			} else if (opt_idx == 4) {
				keyboard_input("Password",
				((pppoe_account *)(*p)->param)->text_password);
				continue;
			} else if (opt_idx == 0) {
				ret = PAGE_BACK;
				/* Skip DHCP setting page */
#ifndef LOAD_INTO_DDR
				(*p) -= 2;
#else
				(*p)->offset = -2;
#endif
			} else if (opt_idx == 1)
				ret = PAGE_CANCLE;
			else
				ret = PAGE_NEXT;
			break;
		}
	}

	if (ret == PAGE_NEXT) {
		char *usr, *pwd;

		lite_get_textline_text(((pppoe_account *)(*p)->param)->text_username, &usr);
		lite_get_textline_text(((pppoe_account *)(*p)->param)->text_password, &pwd);

		wz_nvram_set("wan_pppoe_username_00", usr);
		wz_nvram_set("wan_pppoe_password_00", pwd);
	}

	return ret;
}
