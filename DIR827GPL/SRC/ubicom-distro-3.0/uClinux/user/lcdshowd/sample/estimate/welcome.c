#include <lite/window.h>

#include "global.h"
#include "image.h"
#include "label.h"
#include "mouse.h"
#include "wizard.h"

int wizard_welcome_ui_1(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{120,  20, 270, 20}, "Welcome", COLOR_WHITE, 18 },
		{{ 10,  80, 310, 20}, "This Touch Setup Wizard will guide you through ", COLOR_WHITE, 12 },
		{{ 10,  95, 310, 20}, "a step-by-step process to configure your new ", COLOR_WHITE, 12 },
		{{ 10, 110, 310, 20}, "D-Link router and connect to the Internet.", COLOR_WHITE, 12 }
	};
	DFBRectangle img_rect[] = {
		{  36, 190, 76, 55 },
		{ 206, 190, 76, 55 }
	};

	image_create(win, img_rect[0], "wizard_nothanks.png", 0);
	image_create(win, img_rect[1], "wizard_begin.png", 0);

	for ( i = 0; i < 4; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	lite_set_window_opacity(win, 0xFF);

	return 0;
}

int wizard_welcome_evt_1(LiteWindow *win, wizard_page **p)
{
	return option_return_click(win, PAGE_NOTHANKS | PAGE_BEGIN);
}

int wizard_welcome_ui_2(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	DFBColor step_color = { 0xFF, 0x66, 0x99, 0xFF };
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{120,  20, 270, 20}, "Welcome", COLOR_WHITE, 18 },
		{{ 50,  60, 270, 20}, "Step 1: ", step_color, 12 },
		{{100,  60, 270, 20}, "Install your router", COLOR_WHITE, 12 },
		{{ 50,  80, 270, 20}, "Step 2: ", step_color, 12 },
		{{100,  80, 270, 20}, "Configure your Internet Connection", COLOR_WHITE, 12 },
		{{ 50, 100, 270, 20}, "Step 3", step_color, 12 },
		{{100, 100, 270, 20}, "Configure your WLAN settings", COLOR_WHITE, 12 },
		{{ 50, 120, 270, 20}, "Step 4:", step_color, 12 },
		{{100, 120, 270, 20}, "Select your time zone", COLOR_WHITE, 12 },
		{{ 50, 140, 270, 20}, "Step 5:", step_color, 12 },
		{{100, 140, 270, 20}, "Save settings and Connect", COLOR_WHITE, 12 }
	};
	DFBRectangle img_rect[] = {
		{ 122, 190,  76, 55 },	/* Cancel */
		{ 206, 190,  76, 55 }	/* Next */
	};

	image_create(win, img_rect[0], "wizard_cancel.png", 0);
	image_create(win, img_rect[1], "wizard_next.png", 0);

	for ( i = 0; i < 11; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	lite_set_window_opacity(win, 0xFF);

	return 0;
}

int wizard_welcome_evt_2(LiteWindow *win, wizard_page **p)
{
	return option_return_click(win, PAGE_CANCLE | PAGE_NEXT);
}
