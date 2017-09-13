#include <lite/window.h>

#include "global.h"
#include "image.h"
#include "label.h"
#include "mouse.h"
#include "wizard.h"

int wz_install_ui_1(LiteWindow *win, wizard_page **p)
{
	int i = 0;

	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{ 90,  20, 270, 20}, "Install your Router", COLOR_WHITE, 18 },
		{{200,  75, 270, 20}, "Please turn off or ", COLOR_WHITE, 13 },
		{{200,  90, 270, 20}, "unplug power to ", COLOR_WHITE, 13 },
		{{200, 105, 270, 20}, "your Cable or DSL", COLOR_WHITE, 13 },
		{{200, 120, 270, 20}, "modem", COLOR_WHITE, 13 },
		{{125, 125, 270, 20}, "modem", COLOR_WHITE, 9 }
	};
	DFBRectangle img_rect[] = {
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55},
		{   0,   0, 296, 240}
	};

	image_create(win, img_rect[0], "wizard_cancel.png", 0);
	image_create(win, img_rect[1], "wizard_next.png", 0);
	image_create(win, img_rect[2], "wizard_C1.png", 0);

	for (i = 0; i < 6; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_install_evt_1(LiteWindow *win, wizard_page **p)
{
	return option_return_click(win, PAGE_CANCLE | PAGE_NEXT);
}

int wz_install_ui_2(LiteWindow *win, wizard_page **p)
{
        int i = 0;
        struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  90,  20, 270, 20 }, "Install your Router", COLOR_WHITE, 18 },
		{{  10,  50, 270, 20 }, "Plug the Ethernet cable of Cable or", COLOR_WHITE, 12 },
		{{  10,  65, 270, 20 }, "DSL Modem to the              Port of ", COLOR_WHITE, 12 },
		{{ 125,  65, 270, 20 }, "Internet", { 0xFF, 0x66, 0x99, 0xFF }, 12 },
		{{  10,  80, 270, 20 }, "the router", COLOR_WHITE, 12 },
		{{ 267, 110, 270, 20 }, "modem", COLOR_WHITE, 9 }
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76,  55 },
		{ 122, 190,  76,  55 },
		{ 206, 190,  76,  55 },
		{   0,   0, 320, 228 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);
	image_create(win, img_rect[3], "wizard_C2.png", 0);

	for (i = 0; i < 6; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_install_evt_2(LiteWindow *win, wizard_page **p)
{
	return option_return_click(win, PAGE_BACK | PAGE_CANCLE | PAGE_NEXT);
}

int wz_install_ui_3(LiteWindow *win, wizard_page **p)
{
	int i = 0;

	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  90,  20, 270, 20 }, "Install your Router", COLOR_WHITE, 18},
		{{  10,  50, 270, 20 }, "Plug the Ethernet cable between", COLOR_WHITE, 12},
		{{  10,  65, 270, 20 }, "any Ethernet port of the Router and", COLOR_WHITE, 12},
		{{  10,  80, 270, 20 }, "your PC", COLOR_WHITE, 12},
		{{ 267, 110, 270, 20 }, "    PC", COLOR_WHITE, 9}
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76,  55 },
		{ 122, 190,  76,  55 },
		{ 206, 190,  76,  55 },
		{   0,   0, 320, 228 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0 );
	image_create(win, img_rect[1], "wizard_cancel.png", 0 );
	image_create(win, img_rect[2], "wizard_next.png", 0 );
	image_create(win, img_rect[3], "wizard_C3.png", 0 );

	for (i = 0; i < 5; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_install_evt_3(LiteWindow *win, wizard_page **p)
{
	return option_return_click(win, PAGE_BACK | PAGE_CANCLE | PAGE_NEXT);
}

int wz_install_ui_4(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  90,  20, 270, 20 }, "Install your Router", COLOR_WHITE, 18},
		{{  20,  50, 320, 20 }, "Please double check your network is like below", COLOR_WHITE, 12},
		{{ 285, 140, 270, 20 }, "PC", COLOR_WHITE, 10},
		{{ 200, 120, 270, 20 }, "modem", COLOR_WHITE, 9}
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76,  55 },
		{ 122, 190,  76,  55 },
		{ 206, 190,  76,  55 },
		{   0,   0, 320, 237 }
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);
	image_create(win, img_rect[3], "wizard_C4.png", 0);

	for (i = 0; i < 4; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_install_evt_4(LiteWindow *win, wizard_page **p)
{
	return option_return_click(win, PAGE_BACK | PAGE_CANCLE | PAGE_NEXT);
}

int wz_install_ui_5(LiteWindow *win, wizard_page **p)
{
	int i = 0;
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  90,  20, 270, 20 }, "Install your Router", COLOR_WHITE, 18},
		{{  60,  50, 320, 20 }, "Please power on your Modem and PC", COLOR_WHITE, 12},
		{{ 285, 140, 270, 20 }, "PC", COLOR_WHITE, 10},
		{{ 200, 120, 270, 20 }, "modem", COLOR_WHITE, 9}
	};

	DFBRectangle img_rect[] = {
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55},
		{   0,   0, 320, 237}
	};

	image_create(win, img_rect[0], "wizard_back.png", 0);
	image_create(win, img_rect[1], "wizard_cancel.png", 0);
	image_create(win, img_rect[2], "wizard_next.png", 0);
	image_create(win, img_rect[3], "wizard_C5.png", 0);

	for (i = 0; i < 4; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_install_evt_5(LiteWindow *win, wizard_page **p)
{
	return option_return_click(win, PAGE_BACK | PAGE_CANCLE | PAGE_NEXT);
}
