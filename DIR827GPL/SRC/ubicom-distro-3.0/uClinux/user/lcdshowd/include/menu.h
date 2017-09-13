#ifndef __MENU_H__
#define __MENU_H__
#include <lite/window.h>
#include "global.h"

#define MAX_MENU_STACK	5
#define HORIZON_MOVE_GAP 6
#define HORIZON_MOVE_BOUNDARY 105

extern void menu_clock_timer();
extern int menu_loop(LiteWindow *win, menu_t *menu);
extern void menu_mouse_handler(int x, int y, menu_t *menu);
extern DFBWindowEventType mouse_handler(LiteWindow *win, int *x, int *y, int timeout);
//extern DFBWindowEventType mouse_handler(LiteWindow *win, int *x, int *y, int timeout);
//extern int menu_loop(LiteWindow *win, menu_t *menu);
extern int moving_gap;

#endif
