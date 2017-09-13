#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <directfb.h>
#include <lite/event.h>
#include "global.h"
#include "menu.h"
#include "image.h"
#include "label.h"
#include "timer.h"
#include "mouse.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

struct _LiteImage {
	LiteBox                  box;
	LiteImageTheme          *theme;
	DFBRectangle             clipping_rect;

	int                      width, height;
	IDirectFBSurface        *surface;
	DFBSurfaceBlittingFlags  blitting_flags;
};

int moving_gap = HORIZON_MOVE_GAP;

static int cur_menu_id = 10000;

static int get_new_menu_id()
{
	return cur_menu_id++;
}

#ifdef STATIC_MENU
static void menu_init(LiteWindow *win, menu_t *menu)
{
	int i = 0;
	DFBRectangle rect, clock_rect;
	DFBSurfaceDescription dsc;
	LiteLabel *lab1, *lab2, *lab3, *lab4;

	menu->menu_id = get_new_menu_id();
	menu->bg_image = image_create(win, LITE_BOX(win)->rect, menu->bg, 0);
	clock_rect = (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 };

	menu->label_clock = label_create(win,
				clock_rect,
				NULL,
				"",
				12,
				(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF});

	lab1 = label_create(win,
			(DFBRectangle) { 60, 10, 100, 25 },
			NULL,
			menu->items[0].title,
			14,
			(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	lab2 = label_create(win,
			(DFBRectangle) {  200, 10, 100 ,24 },
			NULL,
			menu->items[1].title,
			14,
			(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	lab3 = label_create(win,
			(DFBRectangle) { 60, 120, 100, 25 },
			NULL,
			menu->items[2].title,
			14,
			(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	lab4 = label_create(win,
			(DFBRectangle) { 200, 120, 100, 25 },
			NULL,
			menu->items[3].title,
			14,
			(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	rect = (DFBRectangle) { 40, 30, 90, 115 };
	image_create(win, rect, menu->items[0].png, 0);
	rect = (DFBRectangle) { 180, 30, 90, 115 };
	image_create(win, rect, menu->items[1].png, 0);
	rect = (DFBRectangle) { 45, 140, 90, 115 };
	image_create(win, rect, menu->items[2].png, 0);
	rect = (DFBRectangle) { 180, 140, 90, 115 };
	image_create(win, rect, menu->items[3].png, 0);

	lite_set_window_opacity(win, 0xFF);
}
#else
static void menu_init(LiteWindow *win, menu_t *menu)
{
	int i = 0;
	DFBRectangle rect, clock_rect;
	DFBSurfaceDescription dsc;

	menu->menu_id = get_new_menu_id();
	menu->bg_image = image_create(win, LITE_BOX(win)->rect, menu->bg, 0);
	rect = (DFBRectangle) { 10, 20, layer_width - 20, 30 };
	clock_rect = (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 };
	menu->label_dsc = label_create(win,
				rect,
				NULL,
				menu->items[menu->idx].title,
				20,
				(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	menu->label_clock = label_create(win,
				clock_rect,
				NULL,
				"",
				12,
				(DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	menu->bg_surface = LITE_BOX(menu->bg_image)->surface;
	lite_set_label_alignment(menu->label_dsc, LITE_LABEL_CENTER);

	lite_set_window_opacity(win, 0xFF);

	if (menu->count == 3) {
		menu->items[3] = menu->items[0];
		menu->items[4] = menu->items[1];
		menu->items[5] = menu->items[2];
		menu->count = 6;
	} else if (menu->count == 2) {
		menu->items[2] = menu->items[0];
		menu->items[3] = menu->items[1];
		menu->count = 4;
	}

	for (; i < menu->count; i++) {
		dsc = image_surface_dsc(menu->items[i].png);
		rect = (DFBRectangle) { 5 + 5 * i + 100 * i, 80, dsc.width, dsc.height };
		menu->items[i].handle = image_create(win, rect, menu->items[i].png, 0);
	}
}
#endif

static void menu_destroy(LiteWindow *win, menu_t *menu)
{
/*
	lite_destroy_box(LITE_BOX(menu->bg_image));
	lite_destroy_box(LITE_BOX(menu->label_dsc));
	lite_destroy_box(LITE_BOX(menu->label_clock));

	for (; i < menu->count; i++)
		lite_destroy_box(LITE_BOX(menu->items[i].handle));
*/
	lite_set_window_opacity(win, 0x00);
	lite_destroy_window(win);
}

static void menu_rotate(LiteWindow *win, menu_t *menu, int x)
{
	int i, rotate_idx[4];
	int j = 0;
	DFBSurfaceDescription dsc[4];
	IDirectFBSurface *img_surface[4];
	DFBRectangle img_rect[4];

	if (x > 0) {	/* Right */
		rotate_idx[1] = ((menu->idx - 1) < 0)? menu->count - 1: menu->idx - 1;
		rotate_idx[2] = menu->idx;
		rotate_idx[3] = (menu->idx + 1) % menu->count;
		rotate_idx[0] = ((rotate_idx[1] - 1) < 0)? menu->count - 1: rotate_idx[1] - 1;
	} else {	/* Left */
		rotate_idx[0] = ((menu->idx - 1) < 0)? menu->count - 1: menu->idx - 1;
		rotate_idx[1] = menu->idx;
		rotate_idx[2] = (menu->idx + 1) % menu->count;
		rotate_idx[3] = (rotate_idx[2] + 1) % menu->count;
	}

	for (i = 0; i < 4; i++) {
		img_surface[i] = image_surface_create(menu->items[rotate_idx[i]].png);
		dsc[i] = image_surface_dsc(menu->items[rotate_idx[i]].png);
		img_rect[i] = LITE_BOX(menu->items[rotate_idx[i]].handle)->rect;

#if 0
		if (menu->items[rotate_idx[i]].handle->surface)
			menu->items[rotate_idx[i]].handle->surface->Release(menu->items[rotate_idx[i]].handle->surface);
		lite_destroy_box(LITE_BOX(menu->items[rotate_idx[i]].handle));
#endif
		image_destroy(menu->items[rotate_idx[i]].handle);
		menu->items[rotate_idx[i]].handle = NULL;
		lite_window_event_loop(win, 1);

		img_surface[i]->SetDstBlendFunction(img_surface[i], DSBF_SRCALPHA);
	}

	main_surface->SetBlittingFlags(main_surface, DSBLIT_BLEND_ALPHACHANNEL);

	if (img_rect[0].x >= layer_width)
		img_rect[0].x = -100;
#if 0
	for (i = 0; i < 105; i++) {
		int j = 0;
		x = (x < 0)? -1: 1;

		main_surface->Blit(main_surface, menu->bg_surface, NULL, 0, 0);

		for (; j < 4; j++)
			main_surface->Blit(main_surface, img_surface[j], NULL, img_rect[j].x += x, 80);

		main_surface->Flip(main_surface, NULL, 0);
	}
#else
	/*
	TODO:
		Improve the algrorithm for adjusting the sliding speed
	*/
	for (i = 1; i < HORIZON_MOVE_BOUNDARY; i += moving_gap) {
		x = (x < 0)? -moving_gap: moving_gap;

		main_surface->Blit(main_surface, menu->bg_surface, NULL, 0, 0);

		for (j = 0; j < 4; j++){
			main_surface->Blit(main_surface, img_surface[j], NULL, img_rect[j].x += x, 80);
		}
		main_surface->Flip(main_surface, NULL, 0);
	}

	// make sure the last pic shown in the boundary,  move to x + HORIZON_MOVE_BOUNDARY
	//if( (i != HORIZON_MOVE_BOUNDARY)  && (i - moving_gap < HORIZON_MOVE_BOUNDARY)){
	if( (i != HORIZON_MOVE_BOUNDARY+1 )  ){
		if(x < 0){
			for (j = 0; j < 4; j++){
				main_surface->Blit(main_surface, img_surface[j], NULL, img_rect[j].x += (i - HORIZON_MOVE_BOUNDARY - 1), 80);
			}
		}else{
			for (j = 0; j < 4; j++){
				main_surface->Blit(main_surface, img_surface[j], NULL, img_rect[j].x -= (i - HORIZON_MOVE_BOUNDARY - 1), 80);
			}
		}
	}
#endif
	menu->idx = (x < 0)? (menu->idx + 1) % menu->count: (menu->idx - 1) < 0? menu->count - 1: menu->idx - 1;

	for (i = 0; i < menu->count; i++) {
		int idx = (menu->idx - 1 + i) < 0? menu->count - 1: (menu->idx - 1 + i) % menu->count;
		DFBSurfaceDescription dscn = image_surface_dsc(menu->items[idx].png);
		DFBRectangle rectn = (DFBRectangle) { 5 + 5 * i + 100 * i, 80, dscn.width, dscn.height };
		menu->items[idx].handle = image_create(win, rectn, menu->items[idx].png, 0);
	}

	for (i = 0; i < 4; i++)
		img_surface[i]->Release(img_surface[i]);

	lite_set_label_text(menu->label_dsc, menu->items[menu->idx].title);
}

static void menu_prerotate(LiteWindow *win, menu_t *menu, int *x, int *y)
{
	int lidx, ridx;
	DFBRectangle lrect, rrect;
	DFBSurfaceDescription ldsc, rdsc;

	lidx = (menu->idx - 1 < 0)? menu->count - 1: menu->idx - 1;
	ridx = (menu->idx + 1) % menu->count;
	lrect = LITE_BOX(menu->items[lidx].handle)->rect;
	rrect = LITE_BOX(menu->items[ridx].handle)->rect;
	ldsc = image_surface_dsc(menu->items[lidx].png);
	rdsc = image_surface_dsc(menu->items[ridx].png);

	if ((*x >= lrect.x && *x <= lrect.x + ldsc.width &&
		*y >= lrect.y && *y <= lrect.y + ldsc.height) ||
		(*x >= rrect.x && *x <= rrect.x + rdsc.width &&
		*y >= rrect.y && *y <= rrect.y + rdsc.height))
	{
		menu_rotate(win, menu, (*x >= rrect.x)? -1: 1);
		*x = LITE_BOX(menu->items[menu->idx].handle)->rect.x;
		*y = LITE_BOX(menu->items[menu->idx].handle)->rect.y;
	}
}

void menu_clock_timer(void *data)
{
	time_t rawtime;
	struct tm * timeinfo;
	char str[MAX_FILENAME];
	char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	LiteLabel *label = (LiteLabel *)data;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	sprintf(str, "%02d:%02d:%02d %d/%d/%d  %s",
		timeinfo->tm_hour,
		timeinfo->tm_min,
		timeinfo->tm_sec,
		timeinfo->tm_year + 1900,
		timeinfo->tm_mon,
		timeinfo->tm_mday,
		weekday[timeinfo->tm_wday]);
	set_label_text(label, str);
}

void menu_mouse_handler(int x, int y, menu_t *menu)
{
	DFBSurfaceDescription dsc;

	DEBUG_MSG("XXX %s: (%d, %d)\n", __FUNCTION__, x, y);
	dsc = image_surface_dsc(menu->items[menu->idx].png);

	if (x >= 110 && x <= dsc.width + 110 && y >= 80 && y <= dsc.height + 80) {
		if (menu->items[menu->idx].func)
			menu->items[menu->idx].func(menu);
	}
}

int menu_loop(LiteWindow *win, menu_t *menu)
{
	timer_option opt;

	menu_init(win, menu);
	opt = (timer_option) {
		.func = menu_clock_timer,
		.timeout = 1000,
		.timeout_id = menu->menu_id,
		.data = menu->label_clock };

	timer_start(&opt);

	while (true) {
		int x, y;

		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			DEBUG_MSG("XXX %s: Down (%d, %d)\n", __FUNCTION__, x, y);
			menu_prerotate(win, menu, &x, &y);
			menu_mouse_handler(x, y, menu);
		} else if (type == DWET_MOTION) {
			DEBUG_MSG("XXX %s Motion \n", __FUNCTION__);
			if(abs(x) > abs(y) ){
				DEBUG_MSG("XXX %s Motion (%s)\n", __FUNCTION__, (x < 0)? "Left": "Right");
				menu_rotate(win, menu, x);
			}else{
				DEBUG_MSG("XXX %s Motion (%s)\n", __FUNCTION__, (y > 0)? "Down": "Up");
				
			}
		} else if (type == DWET_NONE){
			DEBUG_MSG("XXX %s: DWET_NONE\n", __FUNCTION__);
			if(menu->name && (strncmp(menu->name,"main",4) == 0)){
				// when in first menu, we do not leave the program
				DEBUG_MSG("XXX %s: already in top menu\n", __FUNCTION__);
				continue;
			}
			DEBUG_MSG("XXX %s: leave current menu\n", __FUNCTION__);
			break;
		}
	}

	timer_stop(&opt);
	menu_destroy(win, menu);

	return 0;
}
