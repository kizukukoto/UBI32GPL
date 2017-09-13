/*
   (c) Copyright 2001-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   Written by Denis Oliver Kropp <dok@directfb.org>
              
   This file is subject to the terms and conditions of the MIT License:

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <unistd.h>
#include <lite/lite.h>
#include <lite/window.h>
#include <leck/image.h>

#include "global.h"
#include "label.h"
#include "image.h"

static char *image_list[] = {
	"images/wizard_bg.png",
	"images/wizard_C1.png",
	"images/wizard_C2.png",
	"images/wizard_C3.png",
	"images/wizard_C4.png",
	"images/wizard_C5.png",
	"images/background_pure.png",
	"images/background.png",
	"images/bg.png"
};

static void ui_deploy(LiteWindow *win)
{
	int i = 0;
	LiteImage *image1, *image2;
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
		{  36, 190, 76, 55 },
		{ 206, 190, 76, 55 }
	};

	lite_new_image(LITE_BOX(win), &img_rect[0], NULL, &image1);
	lite_new_image(LITE_BOX(win), &img_rect[1], NULL, &image2);
	lite_load_image(image1, "images/wizard_nothanks.png");
	lite_load_image(image2, "images/wizard_begin.png");

	for ( i = 0; i < 11; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);
}

int main(int argc, char *argv[])
{
	int loop, interval;
	LiteImage *image;
	LiteWindow *window;
	DFBRectangle rect;

	/* initialize */
	if (getenv("DFB_LOOP"))
		loop = atoi(getenv("DFB_LOOP"));
	else
		loop = 10;

	if (getenv("DFB_SLEEP"))
		interval = atoi(getenv("DFB_SLEEP"));
	else
		interval = 1000;

	fprintf(stderr, "Loop: %d, Interval: %d\n", loop, interval);

	if (lite_open(&argc, &argv))
		return -1;
	rect = (DFBRectangle) { 0, 0, 320, 240 };

	while (loop--) {
		/* create a window */
		lite_new_window(NULL,
				&rect,
				DWCAPS_ALPHACHANNEL,
				liteDefaultWindowTheme, "Image", &window);
		/* load image */
		lite_new_image(LITE_BOX(window), &rect, NULL, &image);
		//lite_load_image(image, "wizard_C4.png");
		lite_load_image(image, image_list[loop % 9]);
		ui_deploy(window);
		/* show the window */
		lite_set_window_opacity(window, liteFullWindowOpacity);

		/* run the default event loop */
		lite_window_event_loop(window, interval);
		//sleep(2);
		//usleep(interval);

		/* destroy the window with all this children and resources */
		lite_destroy_window(window);
		fprintf(stderr, "close lite\n");
	}

	lite_close();
	return 0;
}
