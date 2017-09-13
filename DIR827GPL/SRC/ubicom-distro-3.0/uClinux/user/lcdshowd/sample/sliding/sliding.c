#include <stdio.h>
#include "global.h"
#include "misc.h"
#include "menu.h"

#if 1
#define DEBUG_MSG(fmt, arg...)	printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

int layer_width, layer_height;
IDirectFBSurface *main_surface;
IDirectFBSurface *main_background_surface;

static menu_t main_menu = {
	.name = "main",
	.bg = "background.png",
	.items = (items_t []) {
		{ "settings.png",	"Settings",	NULL,	NULL },
		{ "status.png",		"Status",	NULL,	NULL },
		{ "statistics.png",	"Statistics",	NULL,	NULL },
		{ "wps.png",		"WPS",		NULL,	NULL },
		{ NULL },
		{ NULL },
		{ NULL }
	},
	.idx = 1,
	.count = 4,
};

int main(int argc, char *argv[])
{
	LiteWindow *main_window;
	DFBRectangle main_rect;

	LITE_INIT_CHECK(argc, argv);
	screen_init(&layer_width, &layer_height);

	main_rect = (DFBRectangle) { 0, 0, layer_width, layer_height };
	lite_new_window(NULL, &main_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &main_window);

	menu_loop(main_window, &main_menu);

	LITE_CLOSE();
	return 0;
}
