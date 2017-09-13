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

#include <gui/widgets/keyboard.h>
struct keyboard_instance {
	LiteBox            box;
	LiteTextLineTheme *theme;

	void * caller;

	LiteTextLine 		*textline;
	struct keyboard_button	buttons[MAX_NO_OF_BUTTONS];

 	struct keyboard_definition	*kybrd_def;
	
	enum keyboard_styles	style;

	int num_of_buttons_of_sub_keyboards[MAX_SUB_KEYBOARDS];
	int num_sub_keyboards;
	int current_sub_keyboard;
	int caps_off_on;

	struct scroll_instance *si;

	keyboard_instance_on_button_pressed on_ok_pressed;
	keyboard_instance_on_button_pressed on_cancel_pressed;

	int refs;

#ifdef DEBUG
	int	magic;
#endif
};

const struct keyboard_button_definition keyboard_definition_ENGLISH_ABC[] = {
	{BT_LETTER, "a", "A", 0,0,1,1},
	{BT_LETTER, "b", "B", 0,1,1,1},
	{BT_LETTER, "c", "C", 0,2,1,1},
	{BT_LETTER, "d", "D", 0,3,1,1},
	{BT_LETTER, "e", "E", 0,4,1,1},
	{BT_LETTER, "f", "F", 0,5,1,1},
	{BT_LETTER, "g", "G", 1,0,1,1},
	{BT_LETTER, "h", "H", 1,1,1,1},
	{BT_LETTER, "i", "I", 1,2,1,1},
	{BT_LETTER, "j", "J", 1,3,1,1},
	{BT_LETTER, "k", "K", 1,4,1,1},
	{BT_LETTER, "l", "L", 1,5,1,1},
	{BT_LETTER, "m", "M", 2,0,1,1},
	{BT_LETTER, "n", "N", 2,1,1,1},
	{BT_LETTER, "o", "O", 2,2,1,1},
	{BT_LETTER, "p", "P", 2,3,1,1},
	{BT_LETTER, "q", "Q", 2,4,1,1},
	{BT_LETTER, "r", "R", 2,5,1,1},
	{BT_LETTER, "s", "S", 3,0,1,1},
	{BT_LETTER, "t", "T", 3,1,1,1},
	{BT_LETTER, "u", "U", 3,2,1,1},
	{BT_LETTER, "v", "V", 3,3,1,1},
	{BT_LETTER, "w", "W", 3,4,1,1},
	{BT_LETTER, "x", "X", 3,5,1,1},
	{BT_LETTER, "y", "Y", 4,0,1,1},
	{BT_LETTER, "z", "Z", 4,1,1,1},
	{BT_LETTER, ".", ".", 4,2,1,1},
	{BT_LETTER, " ", " ", 4,3,3,1},
	{BT_CAPS,   	"", "", 5,0,1,1},
	{BT_LEFT,   	"", "", 5,2,1,1},
	{BT_RIGHT,  	"", "", 5,3,1,1},
	{BT_BACKSPACE,  "", "", 5,5,1,1},

	{END_OF_BUTTONS,0,0,0,0},
};
	
const struct keyboard_button_definition keyboard_definition_ENGLISH_123[] = {
	{BT_LETTER, "1", "1", 0,0,1,1},
	{BT_LETTER, "2", "2", 0,1,1,1},
	{BT_LETTER, "3", "3", 0,2,1,1},
	{BT_LETTER, "4", "4", 1,0,1,1},
	{BT_LETTER, "5", "5", 1,1,1,1},
	{BT_LETTER, "6", "6", 1,2,1,1},
	{BT_LETTER, "7", "7", 2,0,1,1},
	{BT_LETTER, "8", "8", 2,1,1,1},
	{BT_LETTER, "9", "9", 2,2,1,1},
	{BT_LETTER, "*", "*", 3,0,1,1},
	{BT_LETTER, "0", "0", 3,1,1,1},
	{BT_LETTER, "#", "#", 3,2,1,1},
	{BT_LEFT,   	"", "", 4,0,1,1},
	{BT_RIGHT,  	"", "", 4,1,1,1},
	{BT_BACKSPACE,  "", "", 4,2,1,1},

	{END_OF_BUTTONS,0,0,0,0},
};

const struct keyboard_button_definition keyboard_definition_ENGLISH_SYM[] = {
	{BT_LETTER, "`", "`", 0,0,1,1},
	{BT_LETTER, "~", "~", 0,1,1,1},
	{BT_LETTER, "!", "!", 0,2,1,1},
	{BT_LETTER, "@", "@", 0,3,1,1},
	{BT_LETTER, "#", "#", 0,4,1,1},
	{BT_LETTER, "$", "$", 0,5,1,1},
	{BT_LETTER, "%", "%", 1,0,1,1},
	{BT_LETTER, "^", "^", 1,1,1,1},
	{BT_LETTER, "&", "&", 1,2,1,1},
	{BT_LETTER, "*", "*", 1,3,1,1},
	{BT_LETTER, "(", "(", 1,4,1,1},
	{BT_LETTER, ")", ")", 1,5,1,1},
	{BT_LETTER, "_", "_", 2,0,1,1},
	{BT_LETTER, "+", "+", 2,1,1,1},
	{BT_LETTER, "-", "-", 2,2,1,1},
	{BT_LETTER, "=", "=", 2,3,1,1},
	{BT_LETTER, "[", "[", 2,4,1,1},
	{BT_LETTER, "]", "]", 2,5,1,1},
	{BT_LETTER, "'", "'", 3,0,1,1},
	{BT_LETTER, "\\", "\\", 3,1,1,1},
	{BT_LETTER, ";", ";", 3,2,1,1},
	{BT_LETTER, ",", ",", 3,3,1,1},
	{BT_LETTER, ".", ".", 3,4,1,1},
	{BT_LETTER, "/", "/", 3,5,1,1},
	{BT_LETTER, "<", "<", 4,0,1,1},
	{BT_LETTER, ">", ">", 4,1,1,1},
	{BT_LETTER, "|", "|", 4,2,1,1},
	{BT_LETTER, " ", " ", 4,3,3,1},
	{BT_LEFT,   	"", "", 5,0,2,1},
	{BT_RIGHT,  	"", "", 5,2,2,1},
	{BT_BACKSPACE,  "", "", 5,4,2,1},

	{END_OF_BUTTONS,0,0,0,0},
};

const struct keyboard_definition keyboard_definitions[] = {
	{ENGLISH_ABC, keyboard_definition_ENGLISH_ABC, 6, 6},
	{ENGLISH_123, keyboard_definition_ENGLISH_123, 5, 3},
	{ENGLISH_SYM, keyboard_definition_ENGLISH_SYM, 6, 6},
	{MAX_KEYBOARDS, NULL, 0, 0},
};

#if defined(DEBUG)
#define KEYBOARD_MAGIC 0x4432
#define KEYBOARD_VERIFY_MAGIC(ki) keyboard_verify_magic(ki)
static void keyboard_verify_magic(struct keyboard_instance *ki)
{
	DEBUG_ASSERT(ki->magic == KEYBOARD_MAGIC, "%p: bad magic", ki);
}
#else
#define KEYBOARD_VERIFY_MAGIC(ki)
#endif

static void keyboard_instance_destroy(struct keyboard_instance *ki)
{
	DEBUG_ASSERT(ki != NULL, " keyboard destroy with NULL instance ");

	//TODO: Free everything


}

int keyboard_instance_ref(struct keyboard_instance *ki)
{
	KEYBOARD_VERIFY_MAGIC(ki);
	return ++ki->refs;
}

int keyboard_instance_deref(struct keyboard_instance *ki)
{
	KEYBOARD_VERIFY_MAGIC(ki);
	DEBUG_ASSERT(ki->refs, "%p: keyboard has no references", ki);

	if (--ki->refs == 0) {
		lite_destroy_box(LITE_BOX(ki));
#ifdef DEBUG
		ki->magic = 0;
#endif
		//free(ki);	
		//ki = NULL;

		//TODO: do other clean up stuff

		return 0;
	}
	else {
		return ki->refs;
	}
}

static DFBResult keyboard_instance_box_destroy(LiteBox *box)
{
	DFBResult res = DFB_FAILURE;
	
	DEBUG_ASSERT(box != NULL, "keyboard box destroy with NULL instance");
	
	struct keyboard_instance *ki = (struct keyboard_instance*)box;
	
	keyboard_instance_deref(ki);
	
	return DFB_OK;//TODO:!!!!
}

static DFBResult keyboard_instance_get_text(struct keyboard_instance *ki, char **str)
{
	KEYBOARD_VERIFY_MAGIC(ki);

	DFBResult res = DFB_FAILURE;

	res = lite_get_textline_text(ki->textline, str);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: keyboard_instance_get_text failed", ki);
	}

	return res;
}

static void ok_pressed_generic(struct keyboard_instance* ki)
{
	char *str;
	DFBWindowEvent ev;

	if (ki->on_ok_pressed) {
		ev.key_symbol = DIKS_ENTER;
		LITE_BOX(ki->textline)->OnKeyDown(LITE_BOX(ki->textline), &ev);
		keyboard_instance_get_text(ki, &str);
		ki->on_ok_pressed(ki->caller, str);
	}
}

static void cancel_pressed_generic(struct keyboard_instance* ki)
{
	char *str;
	DFBWindowEvent ev;

	if (ki->on_cancel_pressed) {
		ev.key_symbol = DIKS_ESCAPE;
		LITE_BOX(ki->textline)->OnKeyDown(LITE_BOX(ki->textline), &ev);
		keyboard_instance_get_text(ki, &str);
		ki->on_cancel_pressed(ki->caller, str);
	}
}

static char* get_button_text(struct keyboard_button_definition* btn, int caps_off_on)
{
	char *str;

	switch (btn->type) {
		case BT_OK:
			str = "OK";
		break;
		case BT_CANCEL:
			str = "CANCEL";
		break;
		case BT_LETTER:
			str = caps_off_on?btn->letter_1:btn->letter_2;
		break;
		case BT_SPACE:
			str = "----";
		break;
		case BT_BACKSPACE:
			str = "BS";
		break;
		case BT_RIGHT:
			str = ">";
		break;
		case BT_LEFT:
			str = "<";
		break;
		case BT_CAPS:
			str = "CAPS";
		break;
	}
	return str;
}

static void refresh_sub_keyboard_icon(struct keyboard_instance* ki)
{
	struct keyboard_definition *kybrd_def = &(keyboard_definitions[ki->current_sub_keyboard]);
	
	if (!ki->si) {
 		DEBUG_ERROR("%p: no scroll bar attached", ki);
		return;
	}

	switch (kybrd_def->style)
	{
		case ENGLISH_ABC:
			scroll_instance_set_button_icon(ki->si, 1, "/share/LiTE/lcdrouter/theme/sky/123_40x40.png", NULL, NULL);
		break;
		case ENGLISH_123:
			scroll_instance_set_button_icon(ki->si, 1, "/share/LiTE/lcdrouter/theme/sky/SYM_40x40.png", NULL, NULL);
		break;
		case ENGLISH_SYM:
			scroll_instance_set_button_icon(ki->si, 1, "/share/LiTE/lcdrouter/theme/sky/ABC_40x40.png", NULL, NULL);
		break;
	}
}

static void set_sub_keyboard(struct keyboard_instance* ki, int skyb)
{
	DEBUG_ASSERT(skyb < MAX_SUB_KEYBOARDS, "%p: sub keyboard higher than MAX_SUB_KEYBOARDS", ki);

	/*
	 * Hide current buttons
	 */
	int count;
	int start = (ki->current_sub_keyboard == 0)?0:ki->num_of_buttons_of_sub_keyboards[ki->current_sub_keyboard-1];
	
	int end = ki->num_of_buttons_of_sub_keyboards[ki->current_sub_keyboard];

	for (count = start; count < end; count++) {
		lite_set_box_visible(LITE_BOX(((ki->buttons)[count]).button), 0);
	}
	/*
	 * Show new buttons
	 */
	start = (skyb == 0)?0:ki->num_of_buttons_of_sub_keyboards[skyb-1];
	end = ki->num_of_buttons_of_sub_keyboards[skyb];
	for (count = start; count < end; count++) {
		lite_set_box_visible(LITE_BOX(((ki->buttons)[count]).button), 1);
	}

	ki->current_sub_keyboard = skyb;
}

static void set_next_sub_keyboard(struct keyboard_instance* ki)
{
	int new_sk;
	new_sk = ki->current_sub_keyboard + 1;
	
	if (new_sk == ki->num_sub_keyboards) {
		new_sk = 0;
	}

	set_sub_keyboard(ki, new_sk);
}

static void set_caps_off_on(struct keyboard_instance* ki, int off_on)
{

	char button_text[MAX_BUTTON_CAPTION];
	struct keyboard_definition *kybrd_def = keyboard_definitions;
	int count = 0;
	ki->caps_off_on = off_on;
	DEBUG_TRACE("%p: caps = %d", ki, ki->caps_off_on);

	while(((ki->buttons)[count]).button) {

		sprintf(button_text, "%s", get_button_text(((ki->buttons)[count]).btn_def, ki->caps_off_on));
		DEBUG_TRACE("%p: count = %d text = %s", ki, count, button_text);
		lite_set_text_button_caption(((ki->buttons)[count]).button, button_text);
		count++;
	}
}

static void keyboard_button_pressed( LiteTextButton *button, void *ctx)
{
	struct keyboard_instance* ki = (struct keyboard_instance*)ctx;
	struct keyboard_button_definition* btn;
	int count = 0;

	while((((ki->buttons)[count]).button) != button) {
		count++;
	}

	int count_sub_kybrd = 0;

	while (count >= ki->num_of_buttons_of_sub_keyboards[count_sub_kybrd]) {
		count_sub_kybrd++;
	}
	
	struct keyboard_definition *kybrd_def = &(keyboard_definitions[count_sub_kybrd]);

	int index = count - ((ki->current_sub_keyboard == 0)?0:ki->num_of_buttons_of_sub_keyboards[ki->current_sub_keyboard-1]);
	
	btn = &kybrd_def->kbd[index];

	DFBWindowEvent ev;

	switch(btn->type) {
		case BT_LETTER:
			DEBUG_TRACE("%p: button pressed: %d\n", ki, ki->caps_off_on?((char)*btn->letter_1):((char)*btn->letter_2));
			ev.key_symbol = ki->caps_off_on?((char)*btn->letter_1):((char)*btn->letter_2);
			LITE_BOX(ki->textline)->OnKeyDown(LITE_BOX(ki->textline), &ev);
		break;
		case BT_BACKSPACE:
			DEBUG_TRACE("%p: button pressed: BACKSPACE\n", ki);
			ev.key_symbol = DIKS_BACKSPACE;
			LITE_BOX(ki->textline)->OnKeyDown(LITE_BOX(ki->textline), &ev);
		break;
		case BT_RIGHT:
			DEBUG_TRACE("%p: button pressed: RIGHT\n", ki);
			ev.key_symbol = DIKS_CURSOR_RIGHT;
			LITE_BOX(ki->textline)->OnKeyDown(LITE_BOX(ki->textline), &ev);
		break;
		case BT_LEFT:
			DEBUG_TRACE("%p: button pressed: LEFT\n", ki);
			ev.key_symbol = DIKS_CURSOR_LEFT;
			LITE_BOX(ki->textline)->OnKeyDown(LITE_BOX(ki->textline), &ev);
		break;
		case BT_OK:
			DEBUG_TRACE("%p: button pressed: OK\n", ki);
			ok_pressed_generic(ki);
		break;
		case BT_CANCEL:
			DEBUG_TRACE("%p: button pressed: CANCEL\n", ki);
			cancel_pressed_generic(ki);
		break;
		case BT_CAPS:
			DEBUG_TRACE("%p: button pressed: CAPS\n", ki);
			set_caps_off_on(ki, ki->caps_off_on?0:1);
		break;
	}
}

static DFBResult prepare_keyboard(struct keyboard_instance* ki)
{
	DFBResult res = DFB_FAILURE;
	DFBColor        textcolor_static = {0xFF,0xFF, 0x00, 0x00};
	DFBRectangle    rect;
	LiteTextButtonTheme *textButtonTheme;

	/* 
	 * create textline
	 */
	rect.x = 0; rect.y = 0 ; rect.w = ki->box.rect.w - KEY_SPACING_H * 2; rect.h = EDIT_BOX_HEIGHT;
	res = lite_new_textline(LITE_BOX(ki), &rect, liteNoTextLineTheme, &ki->textline);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: Textline for keyboard_instance couldn't be created...", ki);
		return res;
	}

	/* 
	 * setup the keyboard
	 */
	res = lite_new_text_button_theme("/share/LiTE/textbuttonbgnd.png", &textButtonTheme);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: Text button theme for keyboard_instance couldn't be created...", ki);
		return res;
	}

	int count_kybrd = 0;
	int count_btn = 0;
	int count_btn_general = 0;
	int h_step, v_step;
	char button_text[MAX_BUTTON_CAPTION];

	struct keyboard_definition *kybrd_def = &(keyboard_definitions[count_kybrd]);
	while((kybrd_def->style != MAX_KEYBOARDS)) {
		/* 
		* calculate step sizes, this is the real size of the button
		*/
		h_step = ki->box.rect.w / kybrd_def->num_cols;
		v_step = (ki->box.rect.h - EDIT_BOX_HEIGHT) / kybrd_def->num_rows;

		/* 
		* Create buttons
		*/
		while(kybrd_def->kbd[count_btn].type != END_OF_BUTTONS) {
			DEBUG_ASSERT(strlen(get_button_text(&kybrd_def->kbd[count_btn], ki->caps_off_on)) < MAX_BUTTON_CAPTION - 1, "string too long");
			
			sprintf(button_text, "%s", get_button_text(&kybrd_def->kbd[count_btn], ki->caps_off_on), ki->caps_off_on);
	
			/* 
			* Calculate real size and position of the button
			*/
			rect.x = (kybrd_def->kbd[count_btn].col) * h_step;
			rect.y = (kybrd_def->kbd[count_btn].row) * v_step + EDIT_BOX_HEIGHT + KEY_SPACING_H;
			rect.w = (h_step) * kybrd_def->kbd[count_btn].w - KEY_SPACING_H;
			rect.h = (v_step) * kybrd_def->kbd[count_btn].h - KEY_SPACING_V;
	
			res = lite_new_text_button(LITE_BOX(ki), &rect, 
							button_text, textButtonTheme, 
							&(((ki->buttons)[count_btn_general]).button));

			//TODO:add check!
	
			lite_on_text_button_press(((ki->buttons)[count_btn_general]).button, keyboard_button_pressed, ki);
			lite_set_box_visible(((ki->buttons)[count_btn_general]).button, 0);

			((ki->buttons)[count_btn_general]).btn_def = &kybrd_def->kbd[count_btn];

			count_btn++;
			count_btn_general++;
		}
		ki->num_of_buttons_of_sub_keyboards[count_kybrd] = count_btn_general;
		count_btn = 0;
		count_kybrd++;
		kybrd_def = &(keyboard_definitions[count_kybrd]);
	}
	ki->num_sub_keyboards = count_kybrd;
	set_sub_keyboard(ki,0);
	return DFB_OK;
}

DFBResult lite_new_keyboard(LiteBox *parent, DFBRectangle *rect, enum keyboard_styles style, struct keyboard_instance **ret_ki, void* app_data)
{
	//TODO: HANDLE ERROR CASES!

	DFBResult res = DFB_FAILURE;

	DEBUG_ASSERT(style < MAX_KEYBOARDS, "no such keyboard, choose a keyboard from enum keyboard_styles");

	struct keyboard_instance *ki = calloc( 1, sizeof(struct keyboard_instance) );
	if( !ki ) {
		DEBUG_WARN("keyboard_instance create failed");
		return res;
	}
	
	ki->refs = 1;	

#ifdef DEBUG
	ki->magic = KEYBOARD_MAGIC;
#endif

	ki->caller = app_data;

	ki->box.parent = parent;
	ki->box.rect = *rect;
	//ki->box.Destroy = keyboard_instance_box_destroy;

	res = lite_init_box(LITE_BOX(ki));
	if (res != DFB_OK) {
		DEBUG_WARN("keyboard_instance lite_init_box failed");
		D_FREE(ki);
		return res;
	}

	ki->style = style;

	res = prepare_keyboard(ki);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: prepare keyboard failed", ki);
		return res;
	}

	DEBUG_INFO("%p: Keyboard created\n", ki);
	*ret_ki = ki;
	return DFB_OK;
}

DFBResult keyboard_instance_set_text(struct keyboard_instance *ki, char *str)
{
	KEYBOARD_VERIFY_MAGIC(ki);

	DFBResult res = DFB_FAILURE;

	DEBUG_ASSERT(str, "%p: null string to keyboard_instance_set_text", ki);

	res = lite_set_textline_text(ki->textline, str);
	if (res != DFB_OK) {
		DEBUG_ERROR("%p: keyboard_instance_set_text failed", ki);
	}

	return res;
}

void keyboard_instance_set_on_ok_pressed(struct keyboard_instance *ki, void *cb_fnc)
{
	KEYBOARD_VERIFY_MAGIC(ki);
	
	ki->on_ok_pressed = cb_fnc;
}

void keyboard_instance_set_on_cancel_pressed(struct keyboard_instance *ki, void *cb_fnc)
{
	KEYBOARD_VERIFY_MAGIC(ki);
	
	ki->on_cancel_pressed = cb_fnc;
}

static void keyboard_instance_external_ok_pressed( LiteButton *button, void *ctx )
{
	struct keyboard_instance *ki = (struct keyboard_instance*)(ctx);
	
	ok_pressed_generic(ki);
}

static void keyboard_instance_external_cancel_pressed( LiteButton *button, void *ctx )
{
	struct keyboard_instance *ki = (struct keyboard_instance*)(ctx);
	cancel_pressed_generic(ki);
}

static void keyboard_instance_external_caps_pressed( LiteButton *button, void *ctx )
{
	struct keyboard_instance *ki = (struct keyboard_instance*)(ctx);
	set_caps_off_on(ki, ki->caps_off_on?0:1);
}

static void keyboard_instance_external_next_sub_keyboard_pressed( LiteButton *button, void *ctx )
{
	struct keyboard_instance *ki = (struct keyboard_instance*)(ctx);
	set_next_sub_keyboard(ki);
	refresh_sub_keyboard_icon(ki);
}

void keyboard_instance_attach_scrollbar(struct keyboard_instance *ki, struct scroll_instance *si)
{
	KEYBOARD_VERIFY_MAGIC(ki);

	ki->si = si;
	lite_on_button_press( scroll_instance_get_button(si, 0), keyboard_instance_external_ok_pressed, ki);
	lite_on_button_press( scroll_instance_get_button(si, 1), keyboard_instance_external_next_sub_keyboard_pressed, ki);
	lite_on_button_press( scroll_instance_get_button(si, 2), keyboard_instance_external_cancel_pressed, ki);

	scroll_instance_set_button_icon(si, 0, "/share/LiTE/lcdrouter/theme/sky/ok_40x40.png", NULL, NULL);
	scroll_instance_set_button_icon(si, 2, "/share/LiTE/lcdrouter/theme/sky/cancel_40x40.png", NULL, NULL);

	refresh_sub_keyboard_icon(ki);
}
