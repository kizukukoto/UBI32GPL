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

#ifndef KEYBOARD_H
#define KEYBOARD_H

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

/*
 * Widget size definitions
 */
#define EDIT_BOX_HEIGHT		30
#define KEY_SPACING_H	1
#define KEY_SPACING_V	1

/*
 * Keyboard definitions
 */
#define MAX_NO_OF_BUTTONS	100
#define MAX_BUTTON_CAPTION	32
#define MAX_SUB_KEYBOARDS	5

enum keyboard_styles {
	ENGLISH_ABC = 0,
	ENGLISH_123,
	ENGLISH_SYM,
	MAX_KEYBOARDS
};

typedef enum {
	BT_OK,
	BT_CANCEL,
	BT_LETTER,
	BT_SPACE,
	BT_BACKSPACE,
	BT_RIGHT,
	BT_LEFT,
	BT_SHIFT,
	BT_SWITCH_ABC,
	BT_SWITCH_123,
	BT_SWITCH_SYM,
	BT_CAPS,
	END_OF_BUTTONS,
} button_type_t;

struct keyboard_button_definition {
	button_type_t type;
	char *letter_1;
	char *letter_2;
	int row;
	int col;
	int w;
	int h;
};

struct keyboard_definition {
	enum keyboard_styles		style;
	struct keyboard_button_definition*	kbd;
	int				num_rows;
	int				num_cols;
};

struct keyboard_button {
	LiteTextButton		*button;
	struct keyboard_button_definition* btn_def;
};

struct keyboard_instance;

typedef void (*keyboard_instance_on_button_pressed) (void* app_data, char *str);

DFBResult lite_new_keyboard(LiteBox *parent, DFBRectangle *rect, enum keyboard_styles style, struct keyboard_instance **ret_ki, void* app_data);
extern int keyboard_instance_ref(struct keyboard_instance *ki);
extern int keyboard_instance_deref(struct keyboard_instance *ki);

extern DFBResult keyboard_instance_set_text(struct keyboard_instance *ki, char *str);
//extern void keyboard_instance_attach_scrollbar(struct keyboard_instance *ki, struct scroll_instance *si);
extern void keyboard_instance_set_on_ok_pressed(struct keyboard_instance *ki, void *cb_fnc);
extern void keyboard_instance_set_on_cancel_pressed(struct keyboard_instance *ki, void *cb_fnc);
#endif
