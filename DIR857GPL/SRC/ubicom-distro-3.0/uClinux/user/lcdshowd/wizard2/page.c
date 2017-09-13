#include <lite/window.h>
#include "wizard.h"

#ifdef LOAD_INTO_DDR
#include "global.h"
#include "image.h"

static LiteWindow *wizard_window_init()
{
	LiteWindow *win;
	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);
	image_create(win, win_rect, "wizard_bg.png", 0);

	return win;
}
#endif

int do_page_visible(LiteWindow *win, int visible)
{
	return lite_set_window_opacity(win, (visible == TRUE)? 0xFF: 0x00);
}

int do_page_next(wizard_page **p)
{
	wizard_page *next = (*p) + (((*p)->offset == 0)? 1: (*p)->offset);

	printf("XXX %s offset %d\n", __FUNCTION__, (*p)->offset);

	if (next && next->ui_func) {
		do_page_visible(next->win, TRUE);
		do_page_visible((*p)->win, FALSE);
		(*p) += ((*p)->offset == 0)? 1: (*p)->offset;
		//(*p) = next;
	}

	return 0;
}

int do_page_back(wizard_page **p)
{
	wizard_page *next = (*p) + (((*p)->offset == 0)? -1: (*p)->offset);

	printf("XXX %s offset %d\n", __FUNCTION__, (*p)->offset);

	if (next && next->ui_func) {
		do_page_visible(next->win, TRUE);
		do_page_visible((*p)->win, FALSE);
		(*p) += ((*p)->offset == 0)? -1: (*p)->offset;
		//(*p) = next;
	}

	return 0;
}

int do_page_distroy_all(wizard_page *p)
{
	while (p && p->win) {
		lite_destroy_window(p->win);
		p++;
	}

	return 0;
}

int do_page_preload_all(wizard_page *p)
{
	while (p && p->ui_func) {
		p->win = wizard_window_init();
		p->ui_func(p->win, &p);
		p++;
	}

	return 0;
}
