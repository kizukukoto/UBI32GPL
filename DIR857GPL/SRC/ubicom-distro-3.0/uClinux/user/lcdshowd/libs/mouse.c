#include <stdlib.h> //abs()
#include <directfb.h>
#include <lite/event.h>
#include <time.h>
#include "mouse.h"
#include "misc.h"

#if 1
//#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

static MouseEventListener mouse_listener_hook = NULL;
static DFBResult mouse_callback_ret = ~DFB_OK;

static DFBResult mouse_callback(DFBWindowEvent *evt, void *data)
{
	mouse_data_t *md = (mouse_data_t *)data;

	if (mouse_listener_hook)
		mouse_listener_hook(evt);

	ms_advanced_handler(evt);

	if (evt->type == DWET_MOTION) {

		md->x = md->y = -1;
		md->offset_x = evt->cx;
		md->offset_y = evt->cy;
		lite_exit_event_loop();

	} else if (evt->type == DWET_BUTTONUP) {

		md->x = evt->cx;
		md->y = evt->cy;
		md->offset_x = md->offset_y = 0;
		lite_exit_event_loop();
	}

	return mouse_callback_ret;
}

DFBWindowEventType mouse_handler(LiteWindow *win, int *x, int *y, int timeout)
{
	static mouse_data_t md;

	lite_on_raw_window_mouse(win, mouse_callback, &md);
	lite_on_raw_window_mouse_moved(win, mouse_callback, &md);

	window_enable(win, 1);
	lite_flush_window_events(win);
	lite_window_event_loop(win, timeout);
	window_enable(win, 0);

	*x = md.x;
	*y = md.y;

	/* lcd backlight power status, check in sig_lcdpower() */
	last_action_time = time(NULL);
	/* 
		make sure once lcd is off, any touch event can immediately turn on lcd 
		This is because LCDPOWER_CHECK_INTERVAL is not 1
	*/
	switch_lcd_power(LCDPOWER_ON);

	DEBUG_MSG("x = %d , y = %d, offset_x = %d, offset_y = %d \n",*x,*y,md.offset_x,md.offset_y);

	if (md.offset_x == 0 && md.offset_y == 0) {
		DEBUG_MSG("DWET_BUTTONUP\n");
		return DWET_BUTTONUP;
	}
	if ((md.offset_y < 0) && (abs(md.offset_x) < abs(md.offset_y))) {
		DEBUG_MSG("DWET_NONE\n");
		return DWET_NONE;
	}
	if ((md.offset_y > 0) && (abs(md.offset_x) < md.offset_y)) {
		DEBUG_MSG("DWET_LEAVE\n");
		return DWET_LEAVE;
	}

	*x = md.offset_x;
	*y = md.offset_y;

	DEBUG_MSG("DWET_MOTION \n");
	return DWET_MOTION;
}

DFBResult add_mouse_event_listener(MouseEventListener funcp) 
{
	mouse_listener_hook = funcp;
	return DFB_OK;
}

DFBResult remove_mouse_event_listener()
{
	mouse_listener_hook = NULL;
	return DFB_OK;
}

DFBResult set_mouse_event_passthrough(DFBResult ret)
{
	mouse_callback_ret = ret;
	return DFB_OK;
}
