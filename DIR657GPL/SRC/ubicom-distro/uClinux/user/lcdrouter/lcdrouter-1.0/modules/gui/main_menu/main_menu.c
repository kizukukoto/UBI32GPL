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
#include <gui/widgets/menuv2.h>
#include <gui/widgets/grid.h>
#include <gui/widgets/header.h>
#include <gui/widgets/statusbar.h>

#include <gui/widgets/scroll.h>
#include <gui/mm/menu_data.h>
#include <gui/engine/app_loader.h>
#include "populate.h"


#undef MODULE_NAME 

#if BUILD_STATIC 
#define MODULE_NAME main_menu
#else
#define MODULE_NAME 
#endif

#define COMBINE(a,b) a##b
#define SWAP(a,b) COMBINE(b,a)
#define M_(function) SWAP( function, MODULE_NAME )

#define DEBUG

struct M_(app_instance){
	LiteWindow      *window;

	IDirectFBSurface *icon;
	DFBRectangle *rect;
	//add more gui related components to here.

	struct grid_instance *gi;
	struct menu_instance *mi;

	struct header_instance *hi;
	struct scroll_instance *si;

	struct statusbar_instance *sbi;
	struct keyboard_window_instance *kwi;

	int refs;
#ifdef DEBUG
	int	magic;
#endif
};

static debug_levels_t debug_level = TRACE;
#ifndef BUILD_STATIC
debug_levels_t global_debug_level = TRACE;
#endif
#if defined(DEBUG)
#undef  APP_MAGIC
#undef  APP_VERIFY_MAGIC
#define APP_MAGIC 0xBACC
#define APP_VERIFY_MAGIC(app) DEBUG_ERROR("magic called");M_(app_verify_magic)(app)
static
void M_(app_verify_magic)(struct M_(app_instance) *app)
{
	DEBUG_ASSERT(app->magic == APP_MAGIC, "%p: bad magic %x", app , app->magic);
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
	return "Main Menu";
}

char*  M_(app_instance_get_type)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return "1";
}

char*  M_(app_instance_get_parent)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return "Root";
}

char*  M_(app_instance_get_weight)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);	
	return "2";
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
void goto_root_menu (struct menu_instance *mi, void *app_data)
{
	struct M_(app_instance) *app = (struct M_(app_instance)*) app_data;

	lite_set_box_visible(LITE_BOX(app->si), 0);
	lite_draw_box(LITE_BOX(mi)->parent, NULL, 0);

#ifdef ANIMATION_ENABLED
	SlideAnimation *animation;
	animation = create_animation(LITE_BOX(mi)->parent->surface, NULL, LITE_BOX(mi)->parent->surface, RIGHT2LEFT);
#endif	
	lite_set_box_visible(LITE_BOX(app->gi), 1);
	lite_set_box_visible(LITE_BOX(app->mi), 0);
	header_instance_set_text(app->hi, "Main Menu");
#ifdef ANIMATION_ENABLED
	//lite_draw_box(LITE_BOX(list), NULL, 1);
	lite_draw_box(LITE_BOX(mi)->parent, NULL, 0);
#else
	lite_update_box(LITE_BOX(mi), NULL);
#endif

#ifdef ANIMATION_ENABLED
	render_to_animation_surface(animation, LITE_BOX(mi)->parent->surface, 1);
        do_animation(animation);
	destroy_animation(animation);
	lite_update_box(LITE_BOX(mi), NULL);
#endif
	lite_set_box_visible(LITE_BOX(app->si), 1);
	grid_instance_attach_scrollbar(app->gi, app->si);
	lite_focus_box(LITE_BOX(app->gi));
}

static
void app_list_on_item_select(struct menu_instance *mi, void *app_data)
{
	struct M_(app_instance) *app = (struct M_(app_instance)*) app_data;

	int id = menu_instance_get_current_item(mi);
	struct menu_data_instance *mdi = menu_instance_get_menu_data(mi);
	struct list_item *li = menu_data_instance_get_list_item(mdi, id);
	
	printf("app_list item selected %d\n", id);

	if (menu_data_instance_get_listitem_flag(li) == IS_APP) {
		char *text = menu_data_instance_get_metadata(li, APP_URL);
		if (text) {
			printf("global list %p\n", global_app_list);

			if ( !app_loader_instance_find_app_by_name(global_app_list, text) ) {		
				struct app_loader_instance *dali =  app_loader_instance_create(text, NULL, app->sbi);
				if (dali) {
					app_loader_instance_switch_to_app_by_name(global_app_list, text);
				}
			}
			else {
				app_loader_instance_switch_to_app_by_name(global_app_list, text);
			}
			//TODO:check errors and show pop-up
		}
	}
}

static
void fv_on_ok(void *app, char *str)
{
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
	DEBUG_ERROR("%s", str);
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
 	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
	DEBUG_ERROR("*");
//	app_loader_instance_switch_to_app_by_name(global_app_list, "Main Menu");
}

static
void main_menu_on_item_select(struct grid_instance *gi, void *app_data)
{
	struct M_(app_instance) *app = (struct M_(app_instance)*) app_data;
	DFBResult res;

	int id = grid_instance_get_current_item(gi);
	struct menu_data_instance *mdi = grid_instance_get_menu_data(gi);
	struct list_item *li = menu_data_instance_get_list_item(mdi, id);
	
	printf("Grid item selected %d\n", id);

	if (id == 1) {
		res = lite_new_keyboard_window(app_data, &app->kwi);
		if (res != DFB_OK) {
			DEBUG_ERROR("keyboard_window alloc failed\n");
			return;
		}
		keyboard_window_instance_set_on_ok_pressed(app->kwi, fv_on_ok);
	}

	if (menu_data_instance_get_listitem_flag(li) == IS_APP) {
		char *text = menu_data_instance_get_metadata(li, APP_URL);
		if (text) {
			printf("global list %p\n", global_app_list);

			if ( !app_loader_instance_find_app_by_name(global_app_list, text) ) {		
				struct app_loader_instance *dali =  app_loader_instance_create(text, NULL, app->sbi);
				if (dali) {
					app_loader_instance_switch_to_app_by_name(global_app_list, text);
				}
			}
			else {
				app_loader_instance_switch_to_app_by_name(global_app_list, text);
			}
			//TODO:check errors and show pop-up
		}
	}

	if (menu_data_instance_get_listitem_flag(li) == MAY_HAVE_SUBMENU) {
		menu_data_instance_callback_t callback = (menu_data_instance_callback_t)menu_data_instance_get_listitem_callback(li);
		if( callback != NULL) {
			void *callback_data = NULL;// menu_data_instance_get_listitem_callbackdata(mdi);
			callback(mdi, id, callback_data);
		}

		struct menu_data_instance *smdi = menu_data_instance_get_listitem_submenu(li);

		if( smdi != NULL) {
			menu_data_instance_set_focused_item(mdi, id);
			//show new listbox with icons using smdi menu_data

			lite_set_box_visible(LITE_BOX(app->si), 0);
			lite_draw_box(LITE_BOX(app->mi)->parent, NULL, 0);

#ifdef ANIMATION_ENABLED
			SlideAnimation *animation;
			animation = create_animation(LITE_BOX(app->mi)->parent->surface, NULL, LITE_BOX(app->mi)->parent->surface, LEFT2RIGHT);
#endif	
			lite_set_box_visible(LITE_BOX(app->gi), 0);

			menu_instance_attach_scrollbar(app->mi, app->si);
			menu_instance_set_menu_data(app->mi, smdi);

#ifdef ANIMATION_ENABLED
			//lite_draw_box(LITE_BOX(list), NULL, 1);
			lite_draw_box(LITE_BOX(app->mi)->parent, NULL, 0);
#else
			lite_update_box(LITE_BOX(app->mi), NULL);
#endif

#ifdef ANIMATION_ENABLED
			render_to_animation_surface(animation, LITE_BOX(app->mi)->parent->surface, 2);
		        do_animation(animation);
			destroy_animation(animation);
			lite_update_box(LITE_BOX(app->mi), NULL);
#endif

			lite_set_box_visible(LITE_BOX(app->mi), 1);
			lite_set_box_visible(LITE_BOX(app->si), 1);
			lite_focus_box(LITE_BOX(app->mi));
			lite_update_box(LITE_BOX(app->mi), NULL);
		}
	}
}

static
void M_(app_instance_draw_static_components)(struct M_(app_instance) *app)
{
	DFBResult       res;
	DFBRectangle      rect;

	rect.x = 5;
	rect.y = 40 ;
	rect.w = 255 ;
	rect.h = 190 ;

	res = grid_instance_alloc(LITE_BOX(app->window), &rect, &app->gi);

	rect.y = 45 ;
	res = menu_instance_alloc(LITE_BOX(app->window), &rect, &app->mi);

	rect.x = 5 ;
	rect.y = 5 ;
	rect.w = app->rect->w-10 ;
	rect.h = 40 ;

	res = header_instance_alloc(LITE_BOX(app->window), &rect, app->sbi, &app->hi);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("header_instance_alloc failed");
	}

	menu_instance_set_header_instance(app->mi, app->hi);
	menu_instance_set_on_prev_menu(app->mi, goto_root_menu, app);
	menu_instance_set_on_item_select(app->mi, app_list_on_item_select, app);
	lite_set_box_visible(LITE_BOX(app->mi), 0);

	grid_instance_set_header_instance(app->gi, app->hi);
	grid_instance_set_menu_data(app->gi, (struct menu_data_instance*)create_and_populate_menu_data());
	grid_instance_set_on_item_select(app->gi, main_menu_on_item_select, app);

	rect.x = 265 ;
	rect.y = 50 ;
	rect.w = 45;
	rect.h = 180 ;

	res = scroll_instance_alloc(LITE_BOX(app->window), &rect, &app->si);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("header_instance_alloc failed");
	}

	grid_instance_attach_scrollbar(app->gi, app->si);
	lite_focus_box(LITE_BOX(app->gi));
}

static
int M_(app_instance_show_app)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	header_instance_set_text(app->hi, "Main Menu");
	lite_update_box(LITE_BOX(app->window), NULL);
	lite_window_set_modal( app->window, 1);
	lite_set_window_opacity( app->window, liteFullWindowOpacity );
	return 1;
}

static
int M_(app_instance_hide_app)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
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
				//M_(app_instance_destroy)( app );
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

#ifdef DEBUG
	app->magic = APP_MAGIC;
#endif

	app->sbi = (struct statusbar_instance *)(app_data);
	
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

	DEBUG_ERROR("main_menu dfb %p", lite_get_dfb_interface());

	M_(app_instance_draw_static_components)(app);

	lite_on_window_keyboard(LITE_WINDOW(app->window), M_(on_key_press), (void*)app);	

	return app;	
error:
	M_(app_instance_destroy)(app);
	return NULL;
}

