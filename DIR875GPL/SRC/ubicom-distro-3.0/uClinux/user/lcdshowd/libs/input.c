#include <lite/font.h>
#include <leck/textline.h>
#include <lite/window.h>
#include "mouse.h"
#include "global.h"
#include "image.h"
#include "label.h"
#include "input.h"

#define KEYBOARD_SLIDING_SPEED	5
#define RECT(a, b, c, d)	(DFBRectangle) {a, b, c, d}

static int keyboard_state;	/* 0: alpha, 1: number 2:... */
static LiteImage *keyboard_image = NULL;

static int keypress_word(const char *, LiteTextLine *);
static int keyboard_image_change(const char *, LiteTextLine *);
static int keyboard_backspace(const char *, LiteTextLine *);
static int keyboard_return(const char *, LiteTextLine *);

static key_map upper_alpha_map[] = {
	{{  10,  50 }, {  29,  82 }, "Q", keypress_word },
	{{  41,  50 }, {  60,  82 }, "W", keypress_word },
	{{  72,  50 }, {  91,  82 }, "E", keypress_word },
	{{ 103,  50 }, { 122,  82 }, "R", keypress_word },
	{{ 134,  50 }, { 153,  82 }, "T", keypress_word },
	{{ 165,  50 }, { 184,  82 }, "Y", keypress_word },
	{{ 196,  50 }, { 215,  82 }, "U", keypress_word },
	{{ 227,  50 }, { 246,  82 }, "I", keypress_word },
	{{ 258,  50 }, { 277,  82 }, "O", keypress_word },
	{{ 289,  50 }, { 308,  82 }, "P", keypress_word },
	{{  26,  99 }, {  46, 131 }, "A", keypress_word },
	{{  57,  99 }, {  77, 131 }, "S", keypress_word },
	{{  88,  99 }, { 108, 131 }, "D", keypress_word },
	{{ 119,  99 }, { 139, 131 }, "F", keypress_word },
	{{ 150,  99 }, { 170, 131 }, "G", keypress_word },
	{{ 181,  99 }, { 201, 131 }, "H", keypress_word },
	{{ 212,  99 }, { 232, 131 }, "J", keypress_word },
	{{ 243,  99 }, { 263, 131 }, "K", keypress_word },
	{{ 274,  99 }, { 294, 131 }, "L", keypress_word },
	{{  57, 147 }, {  77, 179 }, "Z", keypress_word },
	{{  88, 147 }, { 108, 179 }, "X", keypress_word },
	{{ 119, 147 }, { 139, 179 }, "C", keypress_word },
	{{ 150, 147 }, { 170, 179 }, "V", keypress_word },
	{{ 181, 147 }, { 201, 179 }, "B", keypress_word },
	{{ 212, 147 }, { 232, 179 }, "N", keypress_word },
	{{ 243, 147 }, { 263, 179 }, "M", keypress_word },
	{{  72, 196 }, { 201, 230 }, " ", keypress_word },
	{{  10, 196 }, {  61, 230 }, NULL, .func = keyboard_image_change },
	{{ 274, 196 }, { 308, 230 }, NULL, .func = keyboard_backspace },
	{{ 212, 196 }, { 263, 230 }, NULL, .func = keyboard_return },
	{ .func = NULL }
}, lower_alpha_map[] = {
	{{  10,  50 }, {  29,  82 }, "q", keypress_word },
	{{  41,  50 }, {  60,  82 }, "w", keypress_word },
	{{  72,  50 }, {  91,  82 }, "e", keypress_word },
	{{ 103,  50 }, { 122,  82 }, "r", keypress_word },
	{{ 134,  50 }, { 153,  82 }, "t", keypress_word },
	{{ 165,  50 }, { 184,  82 }, "y", keypress_word },
	{{ 196,  50 }, { 215,  82 }, "u", keypress_word },
	{{ 227,  50 }, { 246,  82 }, "i", keypress_word },
	{{ 258,  50 }, { 277,  82 }, "o", keypress_word },
	{{ 289,  50 }, { 308,  82 }, "p", keypress_word },
	{{  26,  99 }, {  46, 131 }, "a", keypress_word },
	{{  57,  99 }, {  77, 131 }, "s", keypress_word },
	{{  88,  99 }, { 108, 131 }, "d", keypress_word },
	{{ 119,  99 }, { 139, 131 }, "f", keypress_word },
	{{ 150,  99 }, { 170, 131 }, "g", keypress_word },
	{{ 181,  99 }, { 201, 131 }, "h", keypress_word },
	{{ 212,  99 }, { 232, 131 }, "j", keypress_word },
	{{ 243,  99 }, { 263, 131 }, "k", keypress_word },
	{{ 274,  99 }, { 294, 131 }, "l", keypress_word },
	{{  57, 147 }, {  77, 179 }, "z", keypress_word },
	{{  88, 147 }, { 108, 179 }, "x", keypress_word },
	{{ 119, 147 }, { 139, 179 }, "c", keypress_word },
	{{ 150, 147 }, { 170, 179 }, "v", keypress_word },
	{{ 181, 147 }, { 201, 179 }, "b", keypress_word },
	{{ 212, 147 }, { 232, 179 }, "n", keypress_word },
	{{ 243, 147 }, { 263, 179 }, "m", keypress_word },
	{{  72, 196 }, { 201, 230 }, " ", keypress_word },
	{{  10, 196 }, {  61, 230 }, NULL, .func = keyboard_image_change },
	{{ 274, 196 }, { 308, 230 }, NULL, .func = keyboard_backspace },
	{{ 212, 196 }, { 263, 230 }, NULL, .func = keyboard_return },
	{ .func = NULL }
}, number_map[] = {
	{{  10,  49 }, {  29,  70 }, "1", keypress_word },
	{{  41,  49 }, {  60,  70 }, "2", keypress_word },
	{{  72,  49 }, {  91,  70 }, "3", keypress_word },
	{{ 103,  49 }, { 122,  70 }, "4", keypress_word },
	{{ 134,  49 }, { 153,  70 }, "5", keypress_word },
	{{ 165,  49 }, { 184,  70 }, "6", keypress_word },
	{{ 196,  49 }, { 215,  70 }, "7", keypress_word },
	{{ 227,  49 }, { 246,  70 }, "8", keypress_word },
	{{ 258,  49 }, { 277,  70 }, "9", keypress_word },
	{{ 289,  49 }, { 308,  70 }, "0", keypress_word },
	{{  10,  86 }, {  29, 107 }, "!", keypress_word },
	{{  41,  86 }, {  60, 107 }, "@", keypress_word },
	{{  72,  86 }, {  91, 107 }, "#", keypress_word },
	{{ 103,  86 }, { 122, 107 }, "$", keypress_word },
	{{ 134,  86 }, { 153, 107 }, "%", keypress_word },
	{{ 165,  86 }, { 184, 107 }, "^", keypress_word },
	{{ 196,  86 }, { 215, 107 }, "&", keypress_word },
	{{ 227,  86 }, { 246, 107 }, "*", keypress_word },
	{{ 258,  86 }, { 277, 107 }, "(", keypress_word },
	{{ 289,  86 }, { 308, 107 }, ")", keypress_word },
	{{  10, 123 }, {  29, 144 }, "~", keypress_word },
	{{  41, 123 }, {  60, 144 }, "`", keypress_word },
	{{  72, 123 }, {  91, 144 }, "{", keypress_word },
	{{ 103, 123 }, { 122, 144 }, "}", keypress_word },
	{{ 134, 123 }, { 153, 144 }, "[", keypress_word },
	{{ 165, 123 }, { 184, 144 }, "]", keypress_word },
	{{ 196, 123 }, { 215, 144 }, ":", keypress_word },
	{{ 227, 123 }, { 246, 144 }, ";", keypress_word },
	{{ 258, 123 }, { 277, 144 }, "\"", keypress_word },
	{{ 289, 123 }, { 308, 144 }, "|", keypress_word },
	{{  10, 160 }, {  29, 181 }, "<", keypress_word },
	{{  41, 160 }, {  60, 181 }, ">", keypress_word },
	{{  72, 160 }, {  91, 181 }, "?", keypress_word },
	{{ 103, 160 }, { 122, 181 }, "/", keypress_word },
	{{ 134, 160 }, { 153, 181 }, "\\", keypress_word },
	{{ 165, 160 }, { 184, 181 }, ",", keypress_word },
	{{ 196, 160 }, { 215, 181 }, ".", keypress_word },
	{{ 227, 160 }, { 246, 181 }, "+", keypress_word },
	{{ 258, 160 }, { 277, 181 }, "-", keypress_word },
	{{ 289, 160 }, { 308, 181 }, "=", keypress_word },
	{{  72, 196 }, { 201, 230 }, " ", keypress_word },
	{{  10, 196 }, {  61, 230 }, NULL, .func = keyboard_image_change },
	{{ 274, 196 }, { 308, 230 }, NULL, .func = keyboard_backspace },
	{{ 212, 196 }, { 263, 230 }, NULL, .func = keyboard_return },
	{ .func = NULL }
};

static key_combination keycomb[] = {
	{ "keyboard_upper.png",	upper_alpha_map },
	{ "keyboard_lower.png",	lower_alpha_map },
	{ "keyboard_number.png",number_map },
	{ NULL }
};

static int keyboard_return(const char *str, LiteTextLine *text)
{
	return -1;
}

static int keyboard_backspace(const char *str, LiteTextLine *text)
{
	char *gbuf, sbuf[MAX_FILENAME];

	lite_get_textline_text(text, &gbuf);

	if (*gbuf == '\0')
		return 0;
	strcpy(sbuf, gbuf);
	sbuf[strlen(sbuf) - 1] = '\0';
	lite_set_textline_text(text, sbuf);

	return 0;
}

static int keyboard_image_change(const char *str, LiteTextLine *text)
{
	int max_keymap = 0;
	LiteBox *win = LITE_BOX(keyboard_image)->parent;
	DFBRectangle rect = LITE_BOX(keyboard_image)->rect;

	for (; keycomb[max_keymap].png != NULL; max_keymap++);

	keyboard_state = (keyboard_state + 1) % max_keymap;
	image_destroy(keyboard_image);
        keyboard_image = image_create(LITE_WINDOW(win), rect, keycomb[keyboard_state].png, 0);

	return 0;
}

static int keypress_word(const char *str, LiteTextLine *text)
{
	char *gbuf, sbuf[MAX_FILENAME];

	lite_get_textline_text(text, &gbuf);
	sprintf(sbuf, "%s%s", gbuf, str);
	lite_set_textline_text(text, sbuf);

	return 0;
}

static void image_sliding(
	LiteWindow *win,
	LiteImage *bg,
	int y)
{
	int i;
	IDirectFBSurface *bg_surface = LITE_BOX(bg)->surface;
	IDirectFBSurface *img_surface = image_surface_create(keycomb[keyboard_state].png);
	DFBRectangle orig_rect = LITE_BOX(keyboard_image)->rect;

	image_destroy(keyboard_image);
	lite_window_event_loop(win, 1);
	img_surface->SetDstBlendFunction(img_surface, DSBF_SRCALPHA);
	main_surface->SetBlittingFlags(main_surface, DSBLIT_BLEND_ALPHACHANNEL);

	for (i = orig_rect.y; i >= y; i -= KEYBOARD_SLIDING_SPEED) {
		main_surface->Blit(main_surface, bg_surface, NULL, 0, 0);
		main_surface->Blit(main_surface, img_surface, NULL, orig_rect.x, i);
		main_surface->Flip(main_surface, NULL, 0);
	}

	img_surface->Release(img_surface);
	keyboard_image = image_create(win,
			(DFBRectangle) { orig_rect.x, y, orig_rect.w, orig_rect.h },
			keycomb[keyboard_state].png, 0);
}

static void input_loop(
	LiteWindow *win,
	DFBRectangle base_rect,
	LiteTextLine *act_text)
{
	lite_set_window_opacity(win, 0xFF);

	while (true) {
		int x, y;
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			key_map *p = keycomb[keyboard_state].registed_map;
			x -= base_rect.x;
			y -= base_rect.y;

			printf("%d %d\n", x, y);
			for (; p && p->func; p++) {
				DFBPoint p1 = p->p1;
				DFBPoint p2 = p->p2;

				if (x >= p1.x && x <= p2.x && y >= p1.y && y <= p2.y) {
					if (p->func(p->context, act_text) != 0)
						goto out;
				}
			}
		}
	}
out:
	lite_set_window_opacity(win, 0x00);
}

void keyboard_input(const char *lb_title, LiteTextLine *orig_text)
{
	char *buf;
	LiteWindow *win;
	LiteImage *bg_image;
	LiteLabel *lb;
	LiteTextLine *text;
	DFBRectangle key_rect, win_rect = { 0, 0, layer_width, layer_height };
	DFBRectangle text_rect = { 115, 10, 120, 20 };
	DFBSurfaceDescription dsc = image_surface_dsc(keycomb[keyboard_state].png);

	printf("XXX %s(%d) Enter\n", __FUNCTION__, __LINE__);

	keyboard_state = 0;	/* alpha keyboard */

	//set_mouse_advanced_flag(true);
	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);
	bg_image = image_create(win, win_rect, "background.png", 0);
	lb = label_create(win, (DFBRectangle) { 10, 10, 100, 20 }, NULL, lb_title, 12, COLOR_WHITE);
	lite_new_textline(LITE_BOX(win), &text_rect, liteNoTextLineTheme, &text);
	key_rect = (DFBRectangle) { layer_width / 2 - dsc.width / 2, layer_height, dsc.width, dsc.height };
	keyboard_image = image_create(win, key_rect, keycomb[keyboard_state].png, 0);

	lite_get_textline_text(orig_text, &buf);
	lite_set_textline_text(text, buf);

	image_sliding(win, bg_image, layer_height - dsc.height);
	input_loop(win, LITE_BOX(keyboard_image)->rect, text);

	lite_get_textline_text(text, &buf);
	lite_set_textline_text(orig_text, buf);

	lite_destroy_window(win);
	//set_mouse_advanced_flag(false);
}

static int select_option_count(char *opt[])
{
	int i = 0;

	for (; opt[i] != NULL; i++);

	return i;
}

static void select_subwin_pageup(
	const char *opt[],
	LiteLabel *lb[],
	size_t lb_count,
	int *head_idx)
{
	int i;

	printf("XXX %s(%d) head_idx(%d)\n", __FUNCTION__, __LINE__, *head_idx);
	if (*head_idx == 0)
		return;

	(*head_idx)--;
	for (i = 0; i < lb_count; i++)
		lite_set_label_text(lb[i], opt[i + *head_idx]);
}

static void select_subwin_pagedown(
	const char *opt[],
	LiteLabel *lb[],
	size_t lb_count,
	int *head_idx)
{
	int i, opt_count = select_option_count(opt);

	printf("XXX %s(%d)\n", __FUNCTION__, __LINE__);

	if (*head_idx + lb_count >= opt_count)
		return;
	(*head_idx)++;
	for (i = 0; i < lb_count; i++)
		lite_set_label_text(lb[i], opt[i + *head_idx]);
}

static int select_get_selected_idx(
	int head_idx,
	LiteLabel *lb[],
	size_t lb_count,
	const char *opt[],
	int x,
	int y)
{
	int ret = -1, i = 0;
	int opt_count = select_option_count(opt);

	for (; i < lb_count; i++) {
		if (x >= 10 && x <= 230 && y >= i * 30 + 60 && y <= i * 30 + 85 && head_idx + i <= opt_count) {
			ret = i;
			break;
		}
	}
	return ret;
}

static void select_statistic_desc(
	char *opt[],
	LiteLabel *lb,
	int lb_count,
	int head_idx)
{
	char desc[128];
	int opt_count = select_option_count(opt);

	sprintf(desc, "Display: %d-%d, Total: %d",
		head_idx + 1,
		(head_idx + lb_count <= opt_count)? head_idx + lb_count: opt_count,
		opt_count);
	lite_set_label_text(lb, desc);
}

static void select_subwin_loop(
	LiteWindow *subwin,
	const char *options[],
	LiteLabel *lb[],
	size_t lb_count,
	LiteTextLine *text,
	LiteLabel *desc,
	int editable)
{
	int selected_idx, head_idx = 0;

	while (true) {
		int x, y;

		DFBWindowEventType type = mouse_handler(subwin, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			if (x >= 240 && y >= 180 && x <= 316 && y <= 235)	/* Accept */
				break;
			else if (x >= 120 && y >= 10 && x <= 280 && y <= 30 && editable == EDIT_ENABLE) {	/* Edit text field */
				keyboard_input("Select Your PC:", text);
				continue;
			} else if ((selected_idx = select_get_selected_idx(head_idx, lb, lb_count, options, x, y)) == -1)
				continue;

			lite_set_textline_text(text, options[selected_idx + head_idx]);
		} else if (type == DWET_MOTION) {
			printf("XXX left <-> right (%d, %d)\n", x, y);
		} else if (type == DWET_NONE) {
			printf("XXX up <- down (%d, %d)\n", x, y);
			select_subwin_pageup(options, lb, lb_count, &head_idx);
			select_statistic_desc(options, desc, lb_count, head_idx);
		} else if (type == DWET_LEAVE) {
			printf("XXX up -> down (%d, %d)\n", x, y);
			select_subwin_pagedown(options, lb, lb_count, &head_idx);
			select_statistic_desc(options, desc, lb_count, head_idx);
		}
	}
}

void select_input(
	char *options[],
	const char *title,
	LiteTextLine *orig_text,
	int editable)
{
	int i = 0;
	char *buf, sta_buf[64];
	LiteWindow *win;
	LiteLabel *lb, *sta, *label[5];
	LiteImage *bg_image, *accept;
	LiteTextLine *text;
	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };
	DFBRectangle text_rect = { 120, 10, 160, 20 };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	bg_image = image_create(win, win_rect, "background.png", 0);
	accept = image_create(win, RECT(240, 180, 76, 55), "wizard_next.png", 0);
	lb = label_create(win, RECT(10, 10, 100, 20), NULL, title, 12, COLOR_WHITE);
	sta = label_create(win, RECT(85, 215, 150, 25), NULL, "", 12, COLOR_WHITE);

	select_statistic_desc(options, sta, 5, 0);
	lite_new_textline(LITE_BOX(win), &text_rect, liteNoTextLineTheme, &text);
	lite_set_label_alignment(lb, LITE_LABEL_RIGHT);
	lite_set_label_alignment(sta, LITE_LABEL_CENTER);

	for (; i < 5; i++) {
		if (options[i] == NULL)
			break;
		label[i] = label_create(win, RECT(10, i * 30 + 60, 220, 25), NULL, options[i], 16, COLOR_WHITE);
		//lite_set_label_alignment(label[i], LITE_LABEL_CENTER);
		lite_set_label_alignment(label[i], LITE_LABEL_LEFT);
	}

	lite_set_window_opacity(win, 0xFF);

	lite_get_textline_text(orig_text, &buf);
	lite_set_textline_text(text, buf);
	select_subwin_loop(win, options, label, 5, text, sta, editable);
	lite_get_textline_text(text, &buf);
	lite_set_textline_text(orig_text, buf);

	lite_set_window_opacity(win, 0x00);
	lite_destroy_window(win);
}
