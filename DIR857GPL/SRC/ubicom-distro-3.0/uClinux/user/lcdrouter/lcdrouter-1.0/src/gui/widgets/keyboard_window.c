/*
 * Ubicom Network Player
 *
 * (C) Copyright 2009, Ubicom, Inc.
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
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>

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

#include <gui/widgets/keyboard_window.h>

#define DEBUG

struct keyboard_window_instance{
	LiteWindow      *window;

	IDirectFBSurface *icon;
	DFBRectangle *rect;

	struct keyboard_instance *ki;
	struct scroll_instance *si;

	keyboard_window_instance_on_button_pressed on_ok_pressed;
	keyboard_window_instance_on_button_pressed on_cancel_pressed;
	void *caller;

	int refs;
#ifdef DEBUG
	int	magic;
#endif
};

#if defined(DEBUG)
#define KEYBOARD_WINDOW_MAGIC 0x8479
#define KEYBOARD_WINDOW_VERIFY_MAGIC(ki) keyboard_window_verify_magic(ki)
static void keyboard_window_verify_magic(struct keyboard_window_instance *kwi)
{
	DEBUG_ASSERT(kwi->magic == KEYBOARD_WINDOW_MAGIC, "%p: bad magic", kwi);
}
#else
#define KEYBOARD_WINDOW_VERIFY_MAGIC(ki)
#endif

int keyboard_window_instance_ref(struct keyboard_window_instance *kwi)
{
	KEYBOARD_WINDOW_VERIFY_MAGIC(kwi);
	return ++kwi->refs;
}

int keyboard_window_instance_deref(struct keyboard_window_instance *kwi)
{
	KEYBOARD_WINDOW_VERIFY_MAGIC(kwi);
	DEBUG_ASSERT(kwi->refs, "%p: keyboard window has no references", kwi);

	if ( --kwi->refs == 0 ) {
		//TODO: destroy window
		//keyboard_window_instance_destroy(kwi);
		
		keyboard_instance_deref(kwi->ki);
		scroll_instance_deref(kwi->si);
		lite_destroy_window(LITE_WINDOW(kwi->window));
		return 0;
	}
	else {
		return kwi->refs;
	}
}

static void keyboard_window_instance_on_ok_pressed(void* app_data, char* str)
{
	struct keyboard_window_instance *kwi = (struct keyboard_window_instance *)app_data;

	DEBUG_ERROR("%p: str: %s", kwi, str);

	if (kwi->on_ok_pressed) {
		kwi->on_ok_pressed(kwi->caller, str);
	}

	keyboard_window_instance_deref(kwi);
}

static void keyboard_window_instance_on_cancel_pressed(void* app_data, char* str)
{
	struct keyboard_window_instance *kwi = (struct keyboard_window_instance *)app_data;

	DEBUG_ERROR("%p: str: %s", kwi, str);

	if (kwi->on_cancel_pressed) {
		kwi->on_cancel_pressed(kwi->caller, str);
	}
	
	keyboard_window_instance_deref(kwi);
}

DFBResult lite_new_keyboard_window(void *app_data, struct keyboard_window_instance **ret_kwi)
{
	DFBResult res = DFB_FAILURE;

	struct keyboard_window_instance *kwi = calloc( 1, sizeof(struct keyboard_window_instance));

	kwi->refs = 1;
#ifdef DEBUG
	kwi->magic = KEYBOARD_WINDOW_MAGIC;
#endif
	DFBRectangle rect;
	
	rect.x =  0;
	rect.y =  0;
	rect.w = 320;
	rect.h = 240;

	res = lite_new_window( NULL, 
		&rect,
		DWCAPS_ALPHACHANNEL,
		liteNoWindowTheme,
		"",
		&kwi->window);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("create new keyboard window failed with app_data %p", app_data);
		//TODO: handle error!
		return res;
	}

	lite_set_window_background_color(kwi->window,0xFF,0xFF,0x88,0xFF);
	kwi->window->window->SetOpaqueRegion(kwi->window->window, 0,0,0,0);

	lite_window_set_modal( kwi->window, 1);
	lite_set_window_opacity( kwi->window, liteFullWindowOpacity );
	
	rect.x =   5;
	rect.y =  40;
	rect.w = 270;
	rect.h = 195;

	kwi->caller = app_data;

	res = lite_new_keyboard(LITE_BOX(kwi->window), &rect, 0, &kwi->ki, kwi);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: header_instance_alloc failed", kwi);
		//TODO: handle error!
		return res;
	}

	keyboard_instance_set_on_ok_pressed(kwi->ki, keyboard_window_instance_on_ok_pressed);
	keyboard_instance_set_on_cancel_pressed(kwi->ki, keyboard_window_instance_on_cancel_pressed);

	rect.x = 275;
	rect.y =  50;
	rect.w =  40;
	rect.h = 180;

	res = scroll_instance_alloc(LITE_BOX(kwi->window), &rect, &kwi->si);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: scroll_instance failed", kwi);
	}
	
	keyboard_instance_attach_scrollbar(kwi->ki, kwi->si);

	*ret_kwi = kwi;

	lite_focus_box(LITE_BOX(kwi->window));
		
	return res;
}

DFBResult keyboard_window_instance_set_text(struct keyboard_window_instance *kwi, char *str)
{
	KEYBOARD_WINDOW_VERIFY_MAGIC(kwi);

	DFBResult res = DFB_FAILURE;

	DEBUG_ASSERT(str, "%p: null string to keyboard_window_instance_set_text", kwi);

	res = keyboard_instance_set_text(kwi->ki, str);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: keyboard_window_instance_set_text failed", kwi);
	}

	return res;
}

void keyboard_window_instance_set_on_ok_pressed(struct keyboard_window_instance *kwi, void *cb_fnc)
{
	KEYBOARD_WINDOW_VERIFY_MAGIC(kwi);

	kwi->on_ok_pressed = cb_fnc;
}

void keyboard_window_instance_set_on_cancel_pressed(struct keyboard_window_instance *kwi, void *cb_fnc)
{
	KEYBOARD_WINDOW_VERIFY_MAGIC(kwi);
	
	kwi->on_cancel_pressed = cb_fnc;

	keyboard_instance_set_on_cancel_pressed(kwi->ki, keyboard_window_instance_on_cancel_pressed);
}

/*
	lite_destroy_box(LITE_BOX(ki));

	if (ki->si) {
		lite_destroy_box(LITE_BOX(ki->si));
	}
*/
