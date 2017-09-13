#include <stdio.h>
#include <lite/lite.h>
#include "global.h"
#include "misc.h"
#include "input.h"
#include "mouse.h"

int layer_width, layer_height;
IDirectFBSurface *main_surface;

static int text_on_button_down(
        struct _LiteBox *self,
        int x,
        int y,
        DFBInputDeviceButtonIdentifier button)
{
	char *options[] = { "item1", "item2", "item3", "item4", "item5", "item6", "item7", "item8", NULL };
	//keyboard_input(select_name, LITE_TEXTLINE(self));
	select_input(options, "title", LITE_TEXTLINE(self), EDIT_DISABLE);
	return 0;
}

static void main_loop(LiteWindow *win, LiteTextLine *text)
{
	char *options[] = { "item1", "item2", "item3", "item4", "item5", "item6", "item7", "item8", NULL };

	while (true) {
		int x, y;

		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP &&
			x >= 10 && y >= 10 &&
			x <= 100 && y <= 25)
			select_input(options, "title", text, EDIT_DISABLE);
	}
}

int main(int argc, char *argv[])
{
	LiteWindow *win;
	LiteTextLine *text;
	DFBRectangle rect = { 0, 0, 320, 240 };
	DFBRectangle text_rect = { 10, 10, 100, 25 };

	if (lite_open(&argc, &argv))
		return -1;
	screen_init(&layer_width, &layer_height);

	lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);
	lite_new_textline(LITE_BOX(win), &text_rect, liteNoTextLineTheme, &text);
	//LITE_BOX(text)->OnButtonUp = text_on_button_down;
	lite_set_window_opacity(win, liteFullWindowOpacity);
	//lite_window_event_loop(win, 0);
	main_loop(win, text);

	lite_destroy_window(win);
	lite_close();
	return 0;
}
