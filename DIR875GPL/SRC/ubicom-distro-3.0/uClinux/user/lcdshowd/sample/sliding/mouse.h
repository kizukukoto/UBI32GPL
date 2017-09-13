#ifndef __MOUSE_H__
#define __MOUSE_H__
#include <lite/window.h>

typedef struct mouse_data_t {
	int x;
	int y;
	int offset_x;
	int offset_y;
	long long usec;
} mouse_data_t;

typedef void (*MouseEventListener)(DFBWindowEvent *);

extern DFBWindowEventType mouse_handler(LiteWindow *win, int *x, int *y, int timeout);
extern DFBResult add_mouse_event_listener(MouseEventListener);
extern DFBResult remove_mouse_event_listener();
extern DFBResult set_mouse_event_passthrough(DFBResult);

#endif
