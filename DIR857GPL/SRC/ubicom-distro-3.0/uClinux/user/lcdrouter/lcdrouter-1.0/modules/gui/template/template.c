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

#include <gui/engine/app_loader.h>

#undef MODULE_NAME 

#if BUILD_STATIC 
#define MODULE_NAME snake_
#else
#define MODULE_NAME 
#endif

#define COMBINE(a,b) a##b
#define SWAP(a,b) COMBINE(b,a)
#define M_(function) SWAP( function, MODULE_NAME )

//#define DEBUG

struct M_(app_instance){
	LiteWindow      *window;
	IDirectFBSurface *icon;
	DFBRectangle *rect;
	//add more gui related components to here.

	int refs;
#ifdef DEBUG
	int	magic;
#endif
};

debug_levels_t snake_debug_level = TRACE;
static debug_levels_t debug_level = TRACE;
#ifndef BUILD_STATIC
debug_levels_t global_debug_level = TRACE;
#endif
#if defined(DEBUG)
#undef  APP_MAGIC
#undef  APP_VERIFY_MAGIC
#define APP_MAGIC 0xBABE
#define APP_VERIFY_MAGIC(app) M_(app_verify_magic)(app)
static
void M_(app_verify_magic)(struct M_(app_instance) *app)
{
	DEBUG_ASSERT(app->magic == APP_MAGIC, "%p: bad magic", app);
}
#else
#define APP_VERIFY_MAGIC(app)
#endif

static
void M_(app_instance_destroy)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	//stop service thread
	lite_destroy_window( app->window );
	free(app);
}
#if 0
int  M_(app_instance_ref)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	//needs pthread lock
	
	return ++app->refs;
}

int  M_(app_instance_deref)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	DEBUG_ASSERT(app->refs, "%p: app has no references", app);
	//needs pthread llock
	if ( --app->refs == 0 ) {
		M_(app_instance_destroy)(app);
		return 0;
	}
	else {
		return app->refs;
	}
}
#endif

char*  M_(app_instance_get_info)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return "Snake Game";
}

int  M_(app_instance_get_type)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return 1;
}

char*  M_(app_instance_get_parent)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return "Games";
}

int  M_(app_instance_get_weight)(struct M_(app_instance) *app)
{	
	APP_VERIFY_MAGIC(app);
	return 1;
}

IDirectFBSurface* M_(app_instance_get_snapshot)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return app->window->surface;
}

IDirectFBSurface* M_(app_instance_get_icon)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return app->icon;
}

static
void M_(app_instance_draw_static_components)(struct M_(app_instance) *app)
{
	DFBRectangle    rect;
	DFBResult       res;
	DFBColor        textcolor_static = { 0x00,0x00, 0xFF, 0xFF};
	LiteLabel       *label;

	lite_set_window_background_color(app->window, 0xFF, 0x15 * app->window->id, 0x15 * app->window->id, 0xff);

	rect.x = 20; rect.y = 20 ; rect.w = 240; rect.h = 40;
	res = lite_new_label (LITE_BOX(app->window), &rect, liteNoLabelTheme, 20, &label);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("create label failed");
	}
	
	DEBUG_ERROR("%d %d %d %d",rect.x, rect.y, rect.w, rect.h);
	
	lite_set_label_color (label, &textcolor_static);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("set label color failed");
	}
	
	char str[20];
	sprintf(str, "%d" , app->window->id);
	DEBUG_ERROR("window id= %s %d", str, app->window->id);
	
	lite_set_label_text (label, str);
}

static
int M_(app_instance_show_app)(struct M_(app_instance) *app)
{
	//APP_VERIFY_MAGIC(app);
	lite_update_box(LITE_BOX(app->window), NULL);
	lite_window_set_modal( app->window, 1);
	lite_set_window_opacity( app->window, liteFullWindowOpacity );
	return 1;
}

static
int M_(app_instance_hide_app)(struct M_(app_instance) *app)
{
	//APP_VERIFY_MAGIC(app);
	lite_window_set_modal( app->window, 0);
	lite_set_window_opacity( app->window, liteNoWindowOpacity );
	return 1;
}

static
DFBResult M_(on_key_press)(DFBWindowEvent* evt, void *data)
{
	struct M_(app_instance) *app = (struct M_(app_instance)*) data;
	APP_VERIFY_MAGIC(app);
	
	DEBUG_ERROR("window %d on_key_down() called. key_symbol : %d  \n", app->window->id, evt->key_symbol);
	if (evt->type == DWET_KEYDOWN ) {

		switch ( evt->key_symbol ) {
			case 61440 : 
				app_loader_instance_switch_to_app_by_name(global_app_list, "Main Menu");
			break;
			default:
			break;
		}
	}
	return DFB_OK;
}

int  M_(app_instance_start)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	M_(app_instance_show_app)(app);
	//start service thread
	return 1;
}

int  M_(app_instance_pause)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	M_(app_instance_hide_app)(app);
	//pause service thread
	return 1;
}

int  M_(app_instance_idle_callback)(struct M_(app_instance) *app)
{
	//do something not critical
	return 1;
}

struct M_(app_instance)*  M_(app_instance_create)(DFBRectangle *rect, void *app_data)
{
	DFBResult       res;
	
	struct M_(app_instance) *app = calloc(1, sizeof(struct M_(app_instance)));
	if ( app == NULL ) {
		DEBUG_ERROR("create new app instance failed");
		return NULL;
	}
	app->refs = 1;
#ifdef DEBUG
	app->magic = APP_MAGIC;
#endif
	int width,height;
	if (!rect) {
		lite_get_layer_size( &width, &height );
		rect = malloc(sizeof(DFBRectangle)); 
		if( rect == NULL) {
			goto error;
		}
		rect->x = 0; rect->y = 0; rect->w = width; rect->h = height;
	}

	app->rect = rect;

	res = lite_new_window( NULL, 
		app->rect,
		DWCAPS_ALPHACHANNEL, 
		liteNoWindowTheme,
		"",
		&app->window);

	if ( res != DFB_OK ) {
		DEBUG_ERROR("create new window failed");
		goto error;
	}

	//DFBWindowOptions	options;
	//app->window->window->GetOptions(app->window->window, &options);
	//app->window->window->SetOptions(app->window->window, options | DWOP_COLORKEYING | DWOP_ALPHACHANNEL);
	//app->window->window->SetColorKey(app->window->window, 0xee,0xee,0xee); //Lite Gui window default bg color

	lite_set_window_background_color(app->window,0xee,0xee,0xee,0x00);
	app->window->window->SetOpaqueRegion(app->window->window, 0,0,0,0);

	DEBUG_TEST("test...");

	M_(app_instance_draw_static_components)(app);

	lite_on_window_keyboard(LITE_WINDOW(app->window), M_(on_key_press), (void*)app);	

	return app;	
error:
	M_(app_instance_destroy)(app);
	return NULL;
}

