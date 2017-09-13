#include "global.h"
#include "misc.h"
#include "image.h"
#include "menu.h"

extern void lan_start(menu_t *parent_menu);
extern void wan_start(menu_t *parent_menu);
extern void wlan_start(menu_t *parent_menu);
extern void router_info_start(menu_t *parent_menu);

static menu_t status_menu = {
	.name = "status",
	.bg = "background.png",
	.items = (items_t []) {
		{ "wan.png",	"WAN",		wan_start,		NULL },
		{ "lan.png",	"LAN",		lan_start,		NULL },
		{ "wlan.png",	"WLAN",		wlan_start	,	NULL },
		{ "info.png",	"Information",	router_info_start,	NULL },
		{ NULL }
	},
	.idx = 1,
	.count = 4
};

void status_start(menu_t parent_menu)
{
	LiteWindow *win;
	DFBRectangle rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	menu_loop(win, &status_menu);
}
