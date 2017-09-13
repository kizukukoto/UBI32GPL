#include <leck/label.h>
#include "global.h"
#include "timer.h"
#include "image.h"
#include "menu.h"
#include "label.h"
#include "lite_destroy.h"
#include "network_util.h"

extern void lite_destroy_image(LiteImage *img[], int length);
extern void lite_destroy_label(LiteLabel *label[], int length);
extern void menu_clock_timer();

static LiteWindow *win;
static LiteImage *meter[4], *number1[3], *number2[3], *number3[3], *number4[3];
static LiteLabel *time_label;
static int dflag = 0;
static int digital[3];

#ifdef PC
#else
static char *lan_if_name, *wan_if_name, *wlan0_if_name, *wlan1_if_name;
#endif

#if 0
//#define DEBUG_MSG(fmt, arg...)       printf("lcdshowd: %s [%d], "fmt, __FILE__, __LINE__, ##arg)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

performance_info performance_lan, performance_wan, performance_wlan_24G, performance_wlan_5G;

/*destory image*/
static void lite_destroy()
{
	lite_destroy_image(meter, 4);
	lite_destroy_image(number1, 3);
	lite_destroy_image(number2, 3);
	lite_destroy_image(number3, 3);
	lite_destroy_image(number4, 3);
}

/* split speed to hunder, ten, and number */
static void split_speed(int speed)
{
	digital[0] = (int)speed/100;
	digital[1] = (int)(speed - digital[0]*100)/10;
	digital[2] = (int)(speed - digital[0]*100 - digital[1]*10) ;
}

/* drawImage to draw meter or digit by type to distinguish. */
static LiteImage *drawImage(LiteImage *img, DFBRectangle rect, int type, int num)
{
	char filename[24];

	switch (type) {
		case 0 : 
			sprintf(filename, "meter%d.png", num);
			break;
		case 1 : 
			sprintf(filename, "digit%d.png", num);
			break;
	}

	img = image_create(win, rect, filename, 0);
	
	return img;
}

static void drawSpeed(void *data)
{
	int wan, lan, wlan0, wlan1, i;
	int wan_ret, lan_ret, wlan0_ret, wlan1_ret;
/*
	DFBRectangle 
	meter_info[3] = {
		{  35,  20, 108,  56}, 
		{ 185,  20, 108,  56}, 
		{ 106, 120, 108,  56}
	}, wlan_info[3] = {
		{  56,  45,  22,  37},
		{  78,  45,  22,  37},
		{ 100,  45,  22,  37}
	}, lan_info[3] = {
		{ 205,  45,  22,  37}, 
		{ 227,  45,  22,  37}, 
		{ 249,  45,  22,  37}
	}, wan_info[3] = {
		{ 126, 140,  22,  37}, 
		{ 148, 140,  22,  37}, 
		{ 170, 140,  22,  37} 
	};
*/
	DFBRectangle 
	meter_info[4] = {
		{  35,  20, 108,  56}, 
		{ 185,  20, 108,  56}, 
		{  35, 120, 108,  56},
		{ 185, 120, 108,  56}
	}, wlan0_info[3] = {
		{  56,  45,  22,  37},
		{  78,  45,  22,  37},
		{ 100,  45,  22,  37}
	}, wlan1_info[3] = {
		{ 205,  45,  22,  37}, 
		{ 227,  45,  22,  37}, 
		{ 249,  45,  22,  37}
	}, wan_info[3] = {
		{  56, 140,  22,  37}, 
		{  78, 140,  22,  37}, 
		{ 100, 140,  22,  37} 
	}, lan_info[3] = {
		{ 205,  140,  22,  37}, 
		{ 227,  140,  22,  37}, 
		{ 249,  140,  22,  37}
	};


	if (dflag == 1)
		lite_destroy();

#ifdef PC
	srand((int)time(0));
	wan = (int)(999.0*rand()/(RAND_MAX + 1.0));
	lan = (int)(999.0*rand()/(RAND_MAX + 1.0));
	wlan0 = (int)(999.0*rand()/(RAND_MAX + 1.0));
	wlan1 = (int)(999.0*rand()/(RAND_MAX + 1.0));
#else
	get_net_stats(lan_if_name, &performance_lan);
	get_net_stats(wan_if_name, &performance_wan);
	get_wlan_stats(wlan0_if_name, &performance_wlan_24G);
	get_wlan_stats(wlan1_if_name, &performance_wlan_5G);

	/* get_net_stats() returns Bytes, GUI needs kbps */
	wan = (performance_wan.throughput_rx + performance_wan.throughput_tx) * 8 / 1000; // GUI show kbps
	lan = (performance_lan.throughput_rx + performance_lan.throughput_tx) * 8 / 1000; // GUI show kbps
	wlan0 = (performance_wlan_24G.throughput_rx + performance_wlan_24G.throughput_tx); // GUI show kbps
	wlan1 = (performance_wlan_5G.throughput_rx + performance_wlan_5G.throughput_tx); // GUI show kbps

	if (wan > 999){
		wan = 999;
	}
	if (lan > 999){
		lan = 999;
	}
	if (wlan0 > 999) {
		wlan0 = 999;
	}
	if (wlan1 > 999) {
		wlan1 = 999;
	}

	//wlan0 = lan;
	DEBUG_MSG("performance wan: %ld \n",wan);
	DEBUG_MSG("performance lan: %ld \n",lan);
	DEBUG_MSG("performance wlan0: %ld \n", wlan0);
	DEBUG_MSG("performance wlan0: %ld \n", wlan1);
#endif
	wan_ret = (int)(wan*180*1.0/1000*33/180);
	lan_ret = (int)(lan*180*1.0/1000*33/180);
	wlan0_ret = (int)(wlan0*180*1.0/1000*33/180);
	wlan1_ret = (int)(wlan1*180*1.0/1000*33/180);

	/* draw wlan0 meter */
	meter[0] = drawImage(meter[0], meter_info[0], 0, wlan0_ret);
	split_speed(wlan0);
	/* draw wlan speed number */
	for ( i = 0; i < 3; i++)
		number1[i] = drawImage(number1[i], wlan0_info[i], 1, digital[i]);

	/* draw wlan1 meter */
	meter[1] = drawImage(meter[1], meter_info[1], 0, wlan1_ret);
	split_speed(wlan1);
	for ( i = 0; i < 3; i++)
		number2[i] = drawImage(number2[i], wlan1_info[i], 1, digital[i]);

	/* draw lan meter */
	meter[2] = drawImage(meter[2], meter_info[2], 0, lan_ret);
	split_speed(lan);
	for ( i = 0; i < 3; i++)
		number3[i] = drawImage(number3[i], lan_info[i], 1, digital[i]);

	/* draw wan meter */
	meter[3] = drawImage(meter[3], meter_info[3], 0, wan_ret);
	split_speed(wan);
	for ( i = 0; i < 3; i++)
		number4[i] = drawImage(number4[i], wan_info[i], 1, digital[i]);

	dflag = 1;
}

static int statistics_page_loop()
{
	lite_set_window_opacity ( win, 0xFF );
	
	timer_option opt;
	opt = ( timer_option )
	{
		.func = menu_clock_timer,
		.timeout = 1000,
		.timeout_id = 10005,
		.data = time_label
	};
	
	timer_start ( &opt );
	
	while ( true )
	{
		int x, y;
	
		DFBWindowEventType type = mouse_handler ( win, &x, &y, 0 );
	
		if ( type == DWET_NONE )
			break;
	}
	
	timer_stop ( &opt );
	lite_set_window_opacity ( win, 0x00 );
	
	return 0;
}

void statistics_start(menu_t parent_menu)
{
	LiteImage *bg_image;
	LiteLabel *label[6];
	int i = 0;
	dflag = 0;
	//timer_option topt = { .func = drawSpeed, .timeout = 500, .timeout_id = 100 };
	timer_option topt = { .func = drawSpeed, .timeout = STATISTICS_TIMEOUT, .timeout_id = 100 };
	DFBRectangle rect = { 0, 0, layer_width, layer_height };

	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };
        DFBColor color = { 0xFF, 145, 150, 255 };
	
	lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	static struct mydata {
		DFBRectangle rect;
		char txt[128];
	} info[8] = {
		{{ 25, 90, 150, 20}, "Wireless 2.4G"},
		{{ 25, 80, 140, 20}, "Send/Receive Rate  kbps"},
		{{175, 90, 120, 20}, "Wireless 5G"},
		{{175, 80, 140, 20}, "Send/Receive Rate  kbps"},
		{{45,190, 100, 20}, "Internet"},
		{{25, 180, 140, 20}, "Send/Receive Rate  kbps"},
		{{195,190, 100, 20}, "Network"},
		{{175, 180, 140, 20}, "Send/Receive Rate  kbps"}
	};

#ifdef PC
#else
	lan_if_name = nvram_safe_get("lan_eth");
	wan_if_name = nvram_safe_get("wan_eth");
	wlan0_if_name = nvram_safe_get("wlan0_eth");
	wlan1_if_name = nvram_safe_get("wlan1_eth");
	reset_performance(&performance_lan);
	reset_performance(&performance_wan);
	reset_performance(&performance_wlan_24G);
	reset_performance(&performance_wlan_5G);
#endif
	
        bg_image = image_create(win, rect, "background.png", 0);

	time_label = label_create(win,
                                (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
                                NULL,
                                "",
                                12,
                                (DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	for ( i = 0 ; i < 8 ; i++) {
		if (i%2 == 0)
			label[i] = label_create(win, info[i].rect, NULL, info[i].txt, 20, title_color);
		else
			label[i] = label_create(win, info[i].rect, NULL, info[i].txt, 10, color);
	}
	
	timer_start(&topt);
	
	statistics_page_loop();

	timer_stop(&topt);

	lite_destroy_window ( win );
}
