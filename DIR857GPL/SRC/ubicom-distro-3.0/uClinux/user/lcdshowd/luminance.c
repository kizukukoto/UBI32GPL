#include <leck/progressbar.h>

#include "global.h"
#include "mouse.h"
#include "misc.h"
#include "timer.h"
#include "image.h"
#include "menu.h"
#include "label.h"
#include "lite_destroy.h"


#if 1
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

extern void lite_destroy_label ( LiteLabel *label[], int length );
extern void menu_clock_timer();

static int flag = 0;
static float degree = 0.0;
static LiteProgressBar *progressbar;
static LiteLabel *time_label;

static void mouse_event ( DFBWindowEvent *event )
{
	int backlight_value = 0;
	int tmp_value = 0;
	switch ( event->type )
	{
		case DWET_BUTTONDOWN:
			if ( (event->cy > (LUMINANCE_BAR_Y - LUMINANCE_BAR_DRIFT_Y))  &&  (event->cy < (LUMINANCE_BAR_Y + LUMINANCE_BAR_HEIGHT + LUMINANCE_BAR_DRIFT_Y)) 
					&& (event->cx > (LUMINANCE_BAR_X - LUMINANCE_BAR_DRIFT_X))  &&  (event->cx < (LUMINANCE_BAR_X + LUMINANCE_BAR_WIDTH + LUMINANCE_BAR_DRIFT_X) )
			)
			{
				tmp_value = (event->cx - LUMINANCE_BAR_X);
				if(tmp_value < 0){
					tmp_value = 0;
				}else if(tmp_value > LUMINANCE_BAR_WIDTH){
					tmp_value = LUMINANCE_BAR_WIDTH;
				}
				
				degree = ( float ) ( tmp_value *1.0 ) / LUMINANCE_BAR_WIDTH;
				//degree = ( float ) ( (event->cx - LUMINANCE_BAR_X) *1.0 ) / LUMINANCE_BAR_WIDTH;
				backlight_value = (int)(degree * 255);
				lite_set_progressbar_value ( progressbar, degree );
				adjust_luminance(backlight_value);
				DEBUG_MSG("luminance, set backlight value = %d , degree = %f\n",backlight_value,degree);
				flag = 1;
			}
			break;
		case DWET_BUTTONUP:
			flag = 0;
			break;
		case DWET_MOTION:
			if ( flag )
			{
				tmp_value = (event->cx - LUMINANCE_BAR_X);
				if(tmp_value < 0){
					tmp_value = 0;
				}else if(tmp_value > LUMINANCE_BAR_WIDTH){
					tmp_value = LUMINANCE_BAR_WIDTH;
				}
				degree = ( float ) ( tmp_value *1.0 ) / LUMINANCE_BAR_WIDTH;
				//degree = ( float ) ( (event->cx - LUMINANCE_BAR_X) *1.0 ) / LUMINANCE_BAR_WIDTH;
				backlight_value = (int)(degree * 255);
				adjust_luminance(backlight_value);
				DEBUG_MSG("luminance, set backlight value = %d, defree = %f \n",backlight_value,degree);
				lite_set_progressbar_value ( progressbar, degree );
			}
			break;
		default:
			break;
	}
}

static int lumi_page_loop ( LiteWindow *win )
{
	int x, y;
	lite_set_window_opacity ( win, 0xFF );
	timer_option opt;
	opt = ( timer_option )
	{
		.func = menu_clock_timer,
		.timeout = 1000,
		.timeout_id = 10004,
		.data = time_label
	};

	timer_start ( &opt );

	while ( true )
	{
		x = 0; y = 0;
		DFBWindowEventType type = mouse_handler ( win, &x, &y, 0 );
		if ( type == DWET_NONE )
			break;
	}

	timer_stop ( &opt );

	lite_set_window_opacity ( win, 0x00 );

	return 0;
}

static void create_label ( LiteWindow *win, LiteLabel *label[] )
{
	struct data
	{
		DFBRectangle rect;
		char *txt;
	} info[17] =
	{
		{{40,  20, 280, 20}, "Adjust the Display Luminance"},
		{{0, 135, LUMINANCE_BAR_WIDTH, 20}, "darker                                                         brighter"}
	};

	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };

	time_label = label_create ( win,
		( DFBRectangle ) { layer_width / 2 - 80, 220, 180, 20 },
		NULL,
		"",
		12,
		( DFBColor ) { 0xFF, 0xFF, 0xFF, 0xFF }
	);

	label[0] = label_create ( win, info[0].rect, NULL, info[0].txt, 18, title_color );
	label[1] = label_create ( win, info[1].rect, NULL, info[1].txt, 12, title_color );

}

static void proc_progress_bar ( LiteWindow *win )
{
	DFBRectangle bar_rect = {LUMINANCE_BAR_X, LUMINANCE_BAR_Y, LUMINANCE_BAR_WIDTH, LUMINANCE_BAR_HEIGHT};
	char fullname[MAX_FILENAME] = {'\0'};
	char fullname_tmp[MAX_FILENAME] = {'\0'};
	int backlight_value = 0;
	float new_degree = 0.0;
	backlight_value = get_luminance();
	new_degree = ((float) backlight_value ) / BRIGHTNESS_MAX ;
	DEBUG_MSG("luminance, current backlight value = %d, degree = %f \n",backlight_value,new_degree);
	lite_new_progressbar ( LITE_BOX ( win ), &bar_rect, liteNoProgressBarTheme, &progressbar );
	sprintf(fullname, "%swizard_D1_bar.png", IMAGE_DIR);
	sprintf(fullname_tmp, "%swizard_D1_bar_bg.png", IMAGE_DIR);
	lite_set_progressbar_images ( progressbar, fullname, fullname_tmp );
	lite_set_progressbar_value ( progressbar, new_degree );
}

void luminance_start ( menu_t *parent_menu )
{
	LiteWindow *win;
	LiteImage *bg_image;
	LiteLabel *label[2];

	flag = 0;
	degree = 0.0;
	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window ( NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win );

	bg_image = image_create ( win, win_rect, "background.png", 0 );

	create_label ( win, label );

	proc_progress_bar ( win );

	add_mouse_event_listener ( mouse_event );
	lumi_page_loop ( win );
	remove_mouse_event_listener();

	//printf ( "degree :: %f\n", degree );
	lite_destroy_window ( win );
}
