/*
 *  Keyboard GUI Component
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

#ifndef KEYBOARD_WINDOW_H
#define KEYBOARD_WINDOW_H

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

#include <lite/lite.h>
#include <lite/box.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/button.h>
#include <leck/textbutton.h>
#include <leck/list.h>
#include <leck/image.h>
#include <leck/label.h>
#include <leck/progressbar.h>
#include <leck/textline.h>

#include <string.h>
#include <debug.h>

#include <gui/widgets/keyboard.h>

struct keyboard_window_instance;

typedef void (*keyboard_window_instance_on_button_pressed) (void* app_data, char *str);

extern DFBResult lite_new_keyboard_window(void *app_data, struct keyboard_window_instance **ret_kwi);
extern int keyboard_window_instance_ref(struct keyboard_window_instance *kwi);
extern int keyboard_window_instance_deref(struct keyboard_window_instance *kwi);

extern DFBResult keyboard_window_instance_set_text(struct keyboard_window_instance *kwi, char *str);
extern void keyboard_window_instance_set_on_ok_pressed(struct keyboard_window_instance *kwi, void *cb_fnc);
extern void keyboard_window_instance_set_on_cancel_pressed(struct keyboard_window_instance *kwi, void *cb_fnc);
#endif
