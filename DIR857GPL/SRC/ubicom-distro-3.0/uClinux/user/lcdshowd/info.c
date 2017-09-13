#include <leck/textbutton.h>

#include "global.h"
#include "misc.h"
#include "image.h"
#include "mouse.h"

int info_page_loop(LiteWindow *win)
{
	lite_set_window_opacity(win, 0xFF);

	while (true) {
		int x, y;
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			printf("XXX %s Down (%d, %d)\n", __FUNCTION__, x, y);

			if (x >= 10 && x <= 110 && y >= 10 && y <= 30)
				break;
		}
	}

	lite_set_window_opacity(win, 0x00);

	return 0;
}

void info_start(menu_t parent_menu)
{
	LiteWindow *win;
	LiteTextButton *button;
	LiteTextButtonTheme *textButtonTheme;
	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };
	DFBRectangle button_rect = { 10, 10, 100, 20 };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);
	lite_new_text_button_theme(IMAGE_DIR"textbuttonbgnd.png", &textButtonTheme);
	lite_new_text_button(LITE_BOX(win), &button_rect, "Exit", textButtonTheme, &button);

	info_page_loop(win);
	lite_destroy_window(win);
}
