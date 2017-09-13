#include "global.h"
#include "timer.h"
#include "image.h"
#include "label.h"
#include "menu.h"
#include "lite_destroy.h"

extern void lite_destroy_image(LiteImage *image[], int length);
extern void lite_destroy_label(LiteLabel *image[], int length);
extern void menu_clock_timer();

static LiteImage *pbc[2], *pin[2];
static LiteLabel *time_label;

static int wps_page_loop(LiteWindow *win)
{
        lite_set_window_opacity(win, 0xFF);
	timer_option opt;
        opt = (timer_option) {
                .func = menu_clock_timer,
                .timeout = 1000,
                .timeout_id = 10007,
                .data = time_label};

        timer_start(&opt);

        while (true) {
                int x, y;
                DFBWindowEventType type = mouse_handler(win, &x, &y, 0);
		if (type == DWET_BUTTONUP) {
			if( x > 30 && x < 150) {
				lite_set_image_visible(pin[0], false);
				lite_set_image_visible(pin[1], true);
				lite_set_image_visible(pbc[0], true);
				lite_set_image_visible(pbc[1], false);
			}
		
			if (x > 150 && x < 320) {
				lite_set_image_visible(pin[0], true);
				lite_set_image_visible(pin[1], false);
				lite_set_image_visible(pbc[0], false);
				lite_set_image_visible(pbc[1], true);
			}
		} else if (type == DWET_NONE)
                        break;
        }

        timer_stop(&opt);

        lite_set_window_opacity(win, 0x00);
        return 0; 
}

void wps_start(menu_t *parent_menu)
{
	LiteWindow *win;
	LiteImage *bg_image;
	LiteLabel *title;

	DFBRectangle rect = { 0, 0, layer_width, layer_height };

	struct img {
		DFBRectangle rect;
		char filename[24];
	} wps[4] = {
		{{20, 80, 118, 150}, "PIN_0.png"},
		{{20, 50, 157, 200}, "PIN_1.png"},
		{{160, 80, 118, 150}, "PBC_0.png"},
		{{160, 50, 157, 200}, "PBC_1.png"}
	};

	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };

	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };
	DFBRectangle title_rect = { 130, 20, 160, 20 };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

        bg_image = image_create(win, win_rect, "background.png", 0);
	time_label = label_create(win,
                                (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
                                NULL,
                                "",
                                12,
                                (DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });

        title = label_create(win, title_rect, NULL, "WPS", 20, title_color);
	pin[0] = image_create(win, wps[0].rect, wps[0].filename, 0);
	pin[1] = image_create(win, wps[1].rect, wps[1].filename, 0);
	pbc[0] = image_create(win, wps[2].rect, wps[2].filename, 0);
	pbc[1] = image_create(win, wps[3].rect, wps[3].filename, 0);

	lite_set_image_visible(pbc[1], false);
	lite_set_image_visible(pin[0], false);

	wps_page_loop(win);

        lite_destroy_window(win);
}
