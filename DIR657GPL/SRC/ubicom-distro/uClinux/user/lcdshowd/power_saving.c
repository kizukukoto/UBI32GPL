#include <leck/slider.h>
#include <lite/theme.h>
#include <leck/progressbar.h>

#include "global.h"
#include "timer.h"
#include "mouse.h"
#include "image.h"
#include "menu.h"
#include "label.h"
#include "lite_destroy.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

extern void lite_destroy_label(LiteLabel *label[], int length);
extern void menu_clock_timer();

static int flag = 0;
static	float degree = 0.0;
static LiteProgressBar *progressbar;
static LiteLabel *time_label;

static void mouse_event(DFBWindowEvent *event)
{
	int powersaving_value = 0;
#ifndef PC
	char char_value[8] = {'\0'};
#endif
	switch (event->type) {
		case DWET_BUTTONDOWN:
			if (event->cy > 115 && event->cy < 135) {
				if(event->cx > 20 && event->cx < 90){
					lite_set_progressbar_value(progressbar, 0.25);
					powersaving_value = 30 * 60 ; // 30 min = 30 * 60 sec
				}else if (event->cx > 90 && event->cx < 160){
					lite_set_progressbar_value(progressbar, 0.5);
					powersaving_value = 60 * 60 ; // 60 min = 60 * 60 sec
				}else if (event->cx > 160 && event->cx < 230){
					lite_set_progressbar_value(progressbar, 0.75);
					powersaving_value = 90 * 60 ; // 900 min = 90 * 60 sec
				}else if (event->cx > 230 && event->cx < 300){
					lite_set_progressbar_value(progressbar, 1.0);
					powersaving_value = 120 * 60 ; // 120 min = 120 * 60 sec
				}

				adjust_power_save_time(powersaving_value); // 30 min = 30 * 60 sec
#ifndef PC
				sprintf(char_value,"%d",powersaving_value);
				nvram_set("lcd_power_save_time",char_value);
				nvram_commit();
#endif
				DEBUG_MSG("set power saving value = %d secs\n",powersaving_value);
			}
			break;
		default:
			break;
	}
}

static int powersaving_page_loop(LiteWindow *win)
{
	lite_set_window_opacity(win, 0xFF);
	timer_option opt;
	opt = (timer_option) {
                .func = menu_clock_timer,
                .timeout = 1000,
                .timeout_id = 10008,
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

static void create_label(LiteWindow *win, LiteLabel *label[], int *label_idx)
{
	int i = 0;
	struct data {
		DFBRectangle rect;
		char *txt;
	} info[] = {
		{{40,  20, 280, 20}, "Adjust LCD Hibernation Time"},
		{{10, 50, 310, 20}, "Hibernate the LCD after being inactive for:"},
		{{10, 135, 320, 20}, "0"},
		{{280, 135, 320, 20}, "120"},
		{{90, 135, 320, 20}, "|"},
		{{160, 135, 320, 20}, "|"},
		{{230, 135, 320, 20}, "|"}
	};

	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };
	DFBColor font_color = { 0xFF, 0x66, 0xCC, 0x00 };

	time_label = label_create(win,
                                (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
                                NULL,
                                "",
                                12,
                                (DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	for ( i = 0; i < 7; i++, (*label_idx)++) {
		if ( i == 0) {
			label[i] = label_create(win, info[i].rect, NULL, info[i].txt, 18, title_color);
			continue;
		}

		if ( i > 0 && i< 4) {
			label[i] = label_create(win, info[i].rect, NULL, info[i].txt, 14, font_color);
		}else{
			label[i] = label_create(win, info[i].rect, NULL, info[i].txt, 12, title_color);
		}
	}
}

static void proc_progress_bar(LiteWindow *win)
{
#ifndef PC
	int powersaving_value = 0;
#endif
	char fullname[MAX_FILENAME] = {'\0'};
	char fullname_tmp[MAX_FILENAME] = {'\0'};
	DFBRectangle bar_rect = {20, 115, 280, 20};

	sprintf(fullname, "%swizard_D1_bar.png", IMAGE_DIR);
	sprintf(fullname_tmp, "%swizard_D1_bar_bg.png", IMAGE_DIR);

	lite_new_progressbar(LITE_BOX(win), &bar_rect, liteNoProgressBarTheme, &progressbar);
	lite_set_progressbar_images(progressbar, fullname, fullname_tmp);

#ifndef PC
	powersaving_value = atoi(nvram_safe_get("lcd_power_save_time"));
	switch(powersaving_value){
		case 0:
			degree = 0.0;
			break;
		case 1800: // secs
			degree = 0.25;
			break;
		case 3600:
			degree = 0.5;
			break;
		case 5400:
			degree = 0.75;
			break;
		case 7200:
			degree = 1.0;
			break;
		default:
			degree = 0.0;
			break;
	}
#endif
	lite_set_progressbar_value(progressbar, degree);

}

void power_saving_start(menu_t *parent_menu)
{
	LiteWindow *win;
	LiteImage *bg_image;
	LiteLabel *label[7];
	int label_idx = 0;

	flag = 0;
	/* initial degree value */
	degree = 0.0;

	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	bg_image = image_create(win, win_rect, "background.png", 0);

	create_label(win, label, &label_idx);

	proc_progress_bar(win);
	
	add_mouse_event_listener(mouse_event);

	powersaving_page_loop(win);

	remove_mouse_event_listener();
	lite_destroy_window(win);
}
