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

int main(int argc, char *argv[])
{
	LiteImage *image;
	LiteWindow *window;
	DFBRectangle rect;
	DFBResult res;
	int loop = 1;
	char buf[100];

	DirectResult ret;
	DirectLogType log_type = DLT_STDERR;
	const char *log_param = NULL;
	DirectLog *log;
	IDirectFB *dfb;

	D_MALLOC(1351);



	ret = direct_log_create(log_type, log_param, &log);
	if (ret)
		return -1;
	direct_log_set_default(log);

	D_INFO("Direct/Test: Application starting...\n");
	/* initialize */
	if (argc > 1)
		loop = atoi(argv[1]);
	if (loop > 10)
		loop = 10;
	loop = 1;
	if (getenv("DFB_LOOP"))
		loop = atoi(getenv("DFB_LOOP"));
	fprintf(stderr, "loop %d times\n", loop);
	if (lite_open(&argc, &argv))
		return -1;
	while (loop--) {


		/* create a window */
		rect = (DFBRectangle) {
		0, 0, 320, 240};
		lite_new_window(NULL,
				&rect,
				DWCAPS_ALPHACHANNEL,
				liteDefaultWindowTheme, "Image", &window);
		/* load image */
		lite_new_image(LITE_BOX(window), &rect, NULL, &image);
		lite_load_image(image, "wizard_C4.png");

		/* show the window */
		lite_set_window_opacity(window, liteFullWindowOpacity);

		/* run the default event loop */
		lite_window_event_loop(window, 100);
		//sleep(2);
		usleep(500);

		/* destroy image */
		lite_destroy_box(LITE_BOX(image));
		/* destroy the window with all this children and resources */
		lite_destroy_window(window);
		fprintf(stderr, "close lite\n");
	}
      out:
	lite_close();
	D_MALLOC(4321);
	direct_log_destroy(log);
	direct_config->debug = true;
	direct_print_memleaks();

	fprintf(stderr, "type any key to close lite\n");
	read(0, buf, sizeof(buf));




	/* deinitialize */
	fprintf(stderr, "type any key to exit\n");
	read(0, buf, sizeof(buf));

	return 0;
}
