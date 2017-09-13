#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <directfb.h>
#include <lite/window.h>
#include <leck/image.h>
#include <leck/label.h>

#ifdef PC

#else

#include <nvram.h>
#include <linux_vct.h> //for portNum used ,ex VCTLANPORT0,VCTLANPORT1 ...

#endif // end ifdef PC

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)
#define IMAGE_DIR	"/share/LiTE/lcdshowd/images/"
#define MAX_FILENAME	1024
#define STATISTICS_TIMEOUT 1000 //  1 sec

#define GLOABL_LCD_WIDTH 320

#ifndef LCDSHOWD_PID
#define LCDSHOWD_PID       "/var/run/lcdshowd.pid"
#endif


extern IDirectFB *lite_dfb;			/* from dfb */
extern IDirectFBDisplayLayer *lite_layer;	/* from dfb */
extern IDirectFBSurface *main_surface;			/* from main.c */
extern IDirectFBSurface *main_background_surface;	/* from main.c */

extern int layer_width, layer_height;		/* from main.c */

struct menu_t;
typedef struct items_t {
	const char *png;
	const char *title;
	void (*func)(struct menu_t *menu);
	LiteImage *handle;
} items_t;

typedef struct menu_t {
	const char *name;
	const char *bg;
	IDirectFBSurface *bg_surface;
	LiteImage *bg_image;
	LiteLabel *label_dsc;
	LiteLabel *label_clock;
	items_t *items;
	int count;
	int idx;
	int menu_id;
} menu_t;

#define COLOR_WHITE	(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF }
#define COLOR_BLACK	(DFBColor) { 0xFF, 0x00, 0x00, 0x00 }
#define COLOR_RED	(DFBColor) { 0xFF, 0xFF, 0x00, 0x00 }
#define COLOR_GREEN	(DFBColor) { 0xFF, 0x00, 0xFF, 0x00 }
#define COLOR_BLUE	(DFBColor) { 0xFF, 0x00, 0x00, 0xFF }
#endif
