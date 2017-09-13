#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lite/lite.h>
#include <lite/window.h>
#include <leck/image.h>

int main(int argc, char *argv[])
{
	int loop = 1, time = 1000;
	LiteWindow *window;
	LiteImage *image;
	DFBRectangle rect;

	if (lite_open(&argc, &argv))
		return -1;
	if (getenv("DFB_LOOP"))
		loop = atoi(getenv("DFB_LOOP"));
	if (getenv("DFB_SLEEP"))
		time = atoi(getenv("DFB_SLEEP"));

	rect = (DFBRectangle) { 0, 0, 320, 240 };
	lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL, liteDefaultWindowTheme, "Image", &window);
	lite_set_window_opacity(window, liteFullWindowOpacity);

	while (loop--) {
		fprintf(stdout, "Create Image...");
		fflush(stdout);
		lite_new_image(LITE_BOX(window), &rect, NULL, &image);
		lite_load_image(image, "./wizard_C4.png");
		lite_window_event_loop(window, time);

		usleep(time);
		fprintf(stdout, "Destroy\n");
		lite_destroy_box(LITE_BOX(image));
	}

	lite_destroy_window(window);
	lite_close();
	return 0;
}
