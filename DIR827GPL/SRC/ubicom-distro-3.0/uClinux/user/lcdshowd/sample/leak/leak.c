#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <lite/lite.h>
#include <lite/window.h>
#include <leck/label.h>
#include <leck/textbutton.h>

static LiteLabel *label;

static void button_pressed(LiteTextButton *button, void *ctx)
{
	DFBRectangle rect = { rand() % 100 + 100,
				rand() % 100 + 100,
				100,
				20 };

	lite_destroy_box(LITE_BOX(label));
	lite_new_label(LITE_BOX(ctx), &rect, liteNoLabelTheme, 15, &label);
	lite_set_label_text(label, "I'm a Label");
}

int main(int argc, char *argv[])
{
	LiteWindow *window;
	DFBRectangle win_rect, label_rect;
	LiteTextButton *btn;
	LiteTextButtonTheme *textButtonTheme;
	DFBRectangle btn_rect = { 130, 10, 60, 20 };

	if (lite_open(&argc, &argv))
		return -1;

	srand((unsigned)time(NULL));

	win_rect = (DFBRectangle) { 0, 0, 320, 240 };
	label_rect = (DFBRectangle) { 20, 20, 280, 20 };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, liteDefaultWindowTheme, "Image", &window);
	lite_new_label(LITE_BOX(window), &label_rect, liteNoLabelTheme, 15, &label);

	lite_set_label_text(label, "I'm a Label");
	lite_new_text_button_theme("./textbuttonbgnd.png", &textButtonTheme);
	lite_new_text_button(LITE_BOX(window), &btn_rect, "Create", textButtonTheme, &btn);
	lite_on_text_button_press(btn, button_pressed, window);

	lite_set_window_opacity(window, liteFullWindowOpacity);
	lite_window_event_loop( window, 0 );

	/* destroy the window with all this children and resources */
	lite_destroy_window( window );
	lite_destroy_text_button_theme(liteDefaultTextButtonTheme);

	/* deinitialize */
	lite_close();

	return 0;
}
