#ifndef __IMAGE_H__
#define __IMAGE_H__

extern LiteImage *image_create(
	LiteWindow *win,
	DFBRectangle rect,
	const char *filename,
	bool is_center);

extern DFBSurfaceDescription image_surface_dsc(const char *filename);
extern IDirectFBSurface *image_surface_create(const char *filename);
extern DFBResult image_destroy(LiteImage *img);
#endif
