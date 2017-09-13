#include <leck/image.h>
#include <leck/label.h>

struct _LiteImage {
	LiteBox                  box;
	LiteImageTheme          *theme;
	DFBRectangle             clipping_rect;

	int                      width, height;
	IDirectFBSurface        *surface;
	DFBSurfaceBlittingFlags  blitting_flags;
};


void lite_destroy_image(LiteImage *img[], int length)
{
	int i;
	for (i = 0 ; i < length; i++) {
		if (img[i]->surface)
			img[i]->surface->Release(img[i]->surface);
		lite_destroy_box(LITE_BOX(img[i]));
	}
}

void lite_destroy_label(LiteLabel *label[], int length)
{
	int i;
	for (i = 0 ; i < length; i++) {
		lite_destroy_box(LITE_BOX(label[i]));
	}
}

