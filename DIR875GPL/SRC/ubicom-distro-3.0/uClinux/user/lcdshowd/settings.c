#include "global.h"
#include "misc.h"
#include "image.h"
#include "menu.h"
extern void lan_setting_start();	/* from wan_settings.c */
extern void luminance_start();
extern void power_saving_start();

/* The menu_t cannot less than 7 items, otherwise assign a NULL */
static menu_t setting_menu = {
	.name = "settings",
	.bg = "background.png",
	.items = (items_t []) {
		{ "luminance.png",	"Luminance",	luminance_start,	NULL },
		{ "powersaving.png",	"Power Saving",	power_saving_start,	NULL },
		{ NULL},
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL }
	},
	.idx = 1,
	.count = 2
};

void setting_start(menu_t *parent_menu)
{
	LiteWindow *win;
	DFBRectangle rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	menu_loop(win, &setting_menu);
}
