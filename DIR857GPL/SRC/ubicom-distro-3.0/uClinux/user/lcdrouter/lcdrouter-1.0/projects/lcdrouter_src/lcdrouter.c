/*
 * Ubicom Network Player
 *
 * (C) Copyright 2009, Ubicom, Inc.
 * Celil URGAN curgan@ubicom.com
 *
 * The Network Player Reference Code is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Network Player Reference Code is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Network Audio Device Reference Code.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include <lite/lite.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/textbutton.h>
#include <leck/list.h>
#include <leck/image.h>
#include <leck/check.h>
#include <leck/label.h>
#include <string.h>

#include <directfb.h>

#include <debug.h>
#include <gui/widgets/statusbar.h>
#include <gui/engine/app_loader.h>

struct statusbar_instance *global_sbi;

debug_levels_t global_debug_level = TRACE;

int main (int argc, char *argv[])
{
	DFBRectangle    *rect;
	LiteFont        *font;
	IDirectFBFont   *font_interface;
	void *desktop = NULL;

	int             width, height;

	/* initialize */
	if (lite_open( &argc, &argv )) {
		return 1;
	}
#if 1
	IDirectFBDisplayLayer *layer;

	lite_get_layer_interface(&layer);
	layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
	layer->SetRotation(layer,0);
	layer->SetCooperativeLevel(layer, DLSCL_SHARED);
#endif
	
	lite_get_font("default", LITE_FONT_PLAIN, 18, DEFAULT_FONT_ATTRIBUTE, &font);
	lite_font(font, &font_interface);
	
	lite_get_layer_size( &width, &height );
	rect = malloc(sizeof(DFBRectangle)); 
	if( rect == NULL) {
		goto error;
	}
	rect->x = 0; rect->y = 0; rect->w = width; rect->h = height;

	global_sbi = statusbar_instance_alloc();
	if( global_sbi == NULL) {
		goto error;
	}
	statusbar_instance_set_icon(global_sbi, "/share/LiTE/lcdrouter/theme/sky/clock.png", 1);
	statusbar_instance_set_icon(global_sbi, "/share/LiTE/lcdrouter/theme/sky/internet.png", 2);
	statusbar_instance_set_icon(global_sbi, "/share/LiTE/lcdrouter/theme/sky/wifi2_on.png", 3);

	struct app_loader_instance *dali = app_loader_instance_create(MODULES_PATH "/desktop.so", rect, global_sbi);
	if (!dali) {
		printf("Desktop load failed\n");
		goto error;
	}
	app_loader_instance_start(dali);

	desktop = app_loader_instance_get_app_p(dali);
	if (!desktop) {
		printf("Can not get Desktop pointer\n");
		goto error;
	}

	printf("Main Desktop pointer1 %p\n", desktop);

	printf("Main Desktop pointer2 %p\n", *LITE_WINDOW(desktop));

	LiteWindow *window ;

	window = LITE_WINDOW(*(int*)(desktop));

	printf("Main Desktop pointer3 %p\n", window);

	struct app_loader_instance *main_dali = app_loader_instance_create(MODULES_PATH "/main_menu.so", rect, global_sbi);

	if (!main_dali) {
		printf("Main Menu load failed\n");
		goto error;
	}
	app_loader_instance_start(main_dali);

	printf("global list from main= %p\n", global_app_list);
	

	while (lite_window_event_loop( LITE_WINDOW(window), 1000) != DFB_DESTROYED) {
		//process idle callbacks.
	}

error:
	/* destroy the window with all this children and resources */
	lite_destroy_window( LITE_WINDOW(desktop));
	lite_destroy_list_theme(liteDefaultListTheme);
	lite_release_font(font);

	/* deinitialize */
	_exit(1);
}
