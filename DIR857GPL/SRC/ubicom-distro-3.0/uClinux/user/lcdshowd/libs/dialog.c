#include <leck/textbutton.h>
#include "global.h"
#include "mouse.h"
#include "label.h"
#include "dialog.h"

#define MSG_BOX_WIDTH   (layer_width - 100)
#define MSG_BOX_HEIGHT  (layer_height - 160)

static int message_box_loop(LiteWindow *win, DFBRectangle btn_rect)
{
	lite_set_window_opacity(win, 0xFF);

	while (true) {
		int x, y;
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		x -= (layer_width / 2 - MSG_BOX_WIDTH / 2);
		y -= (layer_height / 2 - MSG_BOX_HEIGHT / 2);

		if (type == DWET_BUTTONUP &&
			x >= btn_rect.x && x <= (btn_rect.x + btn_rect.w) &&
			y >= btn_rect.y && y <= (btn_rect.y + btn_rect.h))
			break;
	}

	lite_set_window_opacity(win, 0x00);
	lite_destroy_window(win);

	return 0;
}

int message_box(const char *title, const char *str)
{
	LiteWindow *win;
	LiteLabel *label;
	LiteTextButton *btn;
	LiteTextButtonTheme *textButtonTheme;
	DFBRectangle rect = { layer_width / 2 - MSG_BOX_WIDTH / 2,
				layer_height / 2 - MSG_BOX_HEIGHT / 2,
				MSG_BOX_WIDTH,
				MSG_BOX_HEIGHT };
	DFBRectangle btn_rect = { rect.w / 2 - 20, rect.h / 2 + 10, 40, 20 };

	lite_new_window(NULL,
			&rect,
			DWCAPS_ALPHACHANNEL,
			liteDefaultWindowTheme,
			title,
			&win);
	label = label_create(win,
			(DFBRectangle) { 20, rect.h / 2 - 15, rect.w - 40, 30 },
			NULL,
			str,
			12,
			COLOR_BLACK);
	lite_set_label_alignment(label, LITE_LABEL_CENTER);
	lite_new_text_button_theme(IMAGE_DIR"textbuttonbgnd.png", &textButtonTheme);
	lite_new_text_button(LITE_BOX(win), &btn_rect, "Yes", textButtonTheme, &btn);

	message_box_loop(win, btn_rect);

	lite_destroy_window(win);

	return 0;
}
