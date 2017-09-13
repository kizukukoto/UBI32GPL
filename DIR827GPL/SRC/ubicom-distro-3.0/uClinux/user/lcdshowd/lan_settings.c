#include <lite/font.h>
#include <leck/textline.h>
#include "global.h"
#include "label.h"
#include "menu.h"
#include "mouse.h"
#include "image.h"
#include "timer.h"
#include "input.h"

static struct {
	const char *title;
	LiteLabel *lb;
	LiteTextLine *text;
	DFBRectangle rect1;
	DFBRectangle rect2;
} *p, component_ary[] = {
	{ "IP Address: ", NULL, NULL, { 30, 10, 80, 20 }, { 115, 10, 120, 20 }},
	{ "Mask      : ", NULL, NULL, { 30, 30, 80, 20 }, { 115, 30 ,120, 20 }},
	{ NULL }
};

static void lan_setting_loop(LiteWindow *win, LiteLabel *time_label)
{
	lite_set_window_opacity(win, 0xFF);
	timer_option opt;

	opt = (timer_option) {
		.func = menu_clock_timer,
		.timeout = 1000,
		.timeout_id = 20000,
		.data = time_label};

        timer_start(&opt);

	while (true) {
		int x, y;
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_NONE)
			break;
		else if (type == DWET_BUTTONUP) {
			for (p = component_ary; p && p->title; p++) {
				if (x >= p->rect2.x &&
					x <= p->rect2.x + p->rect2.w &&
					y >= p->rect2.y &&
					y <= p->rect2.y + p->rect2.h)
					keyboard_input(p->title, p->text);
			}
		}
	}

	timer_stop(&opt);
	lite_set_window_opacity(win, 0x00);
}

void lan_setting_start()
{
	LiteWindow *win;
	LiteImage *bg_image;
	LiteLabel *time_label;
	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);
	bg_image = image_create(win, win_rect, "background.png", 0);
	time_label = label_create(win,
				(DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
				NULL,
				"",
				12,
				(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	for (p = component_ary; p && p->title; p++) {
		p->lb = label_create(win, p->rect1, NULL, p->title, 12, COLOR_WHITE);
		lite_new_textline(LITE_BOX(win), &p->rect2, liteNoTextLineTheme, &p->text);
	}

	lan_setting_loop(win, time_label);

	lite_destroy_window(win);
}
