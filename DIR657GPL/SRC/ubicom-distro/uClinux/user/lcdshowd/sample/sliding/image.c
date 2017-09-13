#include "global.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

LiteImage *image_create(
	LiteWindow *win,
	DFBRectangle rect,
	const char *filename,
	bool is_center)
{
	LiteImage *image;
	DFBSurfaceDescription dsc;
	IDirectFBImageProvider *provider;
	char fullname[MAX_FILENAME];

	sprintf(fullname, "%s%s", IMAGE_DIR, filename);

	lite_dfb->CreateImageProvider(lite_dfb, fullname, &provider);
	provider->GetSurfaceDescription(provider, &dsc);
	provider->Release(provider);

	if (is_center)
		rect.x -= dsc.width/2;

	lite_new_image(LITE_BOX(win), &rect, NULL, &image);
	lite_load_image(image, fullname);

	return image;
}

DFBSurfaceDescription image_surface_dsc(const char *filename)
{
	char png_fname[MAX_FILENAME];
	DFBSurfaceDescription dsc;
	IDirectFBImageProvider *provider;

	sprintf(png_fname, "%s%s", IMAGE_DIR, filename);

	if (lite_dfb->CreateImageProvider(lite_dfb, png_fname, &provider) == DFB_FILENOTFOUND) {
		DEBUG_MSG("XXX %s: %s DFB_FILENOTFOUND\n", __FUNCTION__, filename);
		goto out;
	}

	provider->GetSurfaceDescription(provider, &dsc);
	provider->Release(provider);

out:
	return dsc;
}

IDirectFBSurface *image_surface_create(const char *filename)
{
	char fname[MAX_FILENAME];
	DFBSurfaceDescription dsc;
	IDirectFBSurface *surface;
	IDirectFBImageProvider *provider;

	sprintf(fname, "%s%s", IMAGE_DIR, filename);

	lite_dfb->CreateImageProvider(lite_dfb, fname, &provider);
	provider->GetSurfaceDescription(provider, &dsc);

	dsc.flags |= DSDESC_CAPS;
	dsc.caps = DSCAPS_SHARED;

	lite_dfb->CreateSurface(lite_dfb, &dsc, &surface);
	provider->RenderTo(provider, surface, NULL);
	provider->Release(provider);

	return surface;
}
