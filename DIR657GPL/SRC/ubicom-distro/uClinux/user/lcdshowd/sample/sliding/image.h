#ifndef __IMAGE_H__
#define __IMAGE_H__

extern LiteImage *image_create(
	LiteWindow *win,
	DFBRectangle rect,
	const char *filename,
	bool is_center);

DFBSurfaceDescription image_surface_dsc(const char *filename);
IDirectFBSurface *image_surface_create(const char *filename);

#endif
