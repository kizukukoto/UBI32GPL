#include <leck/label.h>
#include "global.h"
#include "misc.h"
#include "timer.h"
#include "network_util.h"
#include "image.h"
#include "menu.h"
#include "label.h"
#include "lite_destroy.h"


#ifdef PC
#define VER_FIRMWARE "1.00NA"
#else
extern const char VER_FIRMWARE[]; // declared in libplatform/version.c
#endif

extern void lite_destroy_label(LiteLabel *label[], int length);
extern void menu_clock_timer();

static LiteLabel *label[11], *time_label;

static int router_info_page_loop(LiteWindow *win)
{
	lite_set_window_opacity(win, 0xFF);
 	timer_option opt;
        opt = (timer_option) {
                .func = menu_clock_timer,
                .timeout = 1000,
                .timeout_id = 10009,
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
	char sys_up_time[64] = {'\0'};
	int day_local,hour_local, min_local,sec_local;
	struct data {
		DFBRectangle rect;
		char *txt;
	} info[11] = {
		{{  30,  20, 320,  20}, "General Router Information"},
		{{  25,  50, 100,  15}, "Model Name:"},
#ifdef PC
		{{ 120,  50, 100,  15}, "DIR-865"},
#else
		{{ 120,  50, 100,  15}, nvram_safe_get("model_number")},
#endif
		{{  25,  70, 130,  15}, "Hardware Version:"},
		{{ 160,  70,  25,  15}, "A1"},
		{{  25,  90, 130,  15}, "Firmware Version:"},
		{{ 155,  90,  60,  15}, (char *)VER_FIRMWARE},
		{{  25, 110, 130,  15}, "System Up Time:"},
		{{ 155, 110, 150,  15}, sys_up_time},
		{{  25, 130, 130,  15}, "New Firmware:"},
		{{ 135, 130, 180,  15}, "Lastest Firmware version"}
	};
	int i = 0;
	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };
	DFBColor color = { 0xFF, 145, 150, 255 };

	get_sys_uptime(&day_local,&hour_local,&min_local,&sec_local);
	sprintf(sys_up_time,"%d Days, %dh:%dm:%ds",day_local,hour_local,min_local,sec_local);

	time_label = label_create(win,
                                (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
                                NULL,
                                "",
                                12,
                                (DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	label[0] = label_create(win, info[0].rect, NULL, info[0].txt, 20, title_color);

 	for (i = 1; i < 11; i++) {
                if (i%2 == 0)
                        label[i] = label_create(win, info[i].rect, NULL,
                                info[i].txt, 14, title_color);
                else
                        label[i] = label_create(win, info[i].rect, NULL,
                                info[i].txt, 14, color);
        }
}

void router_info_start(menu_t *parent_menu)
{
	LiteWindow *win;
	LiteImage *bg_image;

	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	bg_image = image_create(win, win_rect, "background.png", 0);

	create_label(win);

	router_info_page_loop(win);

	lite_destroy_window(win);
}
