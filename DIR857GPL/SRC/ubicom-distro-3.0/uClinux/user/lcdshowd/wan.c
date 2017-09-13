#include <leck/label.h>
#include "global.h"
#include "timer.h"
#include "network_util.h"
#include "image.h"
#include "menu.h"
#include "label.h"

#if 0
//#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

extern void menu_clock_timer();
static LiteLabel *time_label;

int wan_page_loop(LiteWindow *win)
{
	lite_set_window_opacity(win, 0xFF);
	timer_option opt = (timer_option) {
                .func = menu_clock_timer,
                .timeout = 1000,
                .timeout_id = 10002,
                .data = time_label};

        timer_start(&opt);
	printf("XXX %s(%d)\n", __FUNCTION__, __LINE__);
	while (true) {
		int x, y;
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_NONE)
			break;
	}
	printf("XXX %s(%d)\n", __FUNCTION__, __LINE__);
        timer_stop(&opt);
	lite_set_window_opacity(win, 0x00);
	return 0;
}

//static void create_label(LiteWindow *win, LiteLabel *label[], int *label_idx )
static void create_label(LiteWindow *win)
{
	int i = 0;
	char wan_ip[16] = {'\0'};
	char wan_gateway[16] = {'\0'};

	char wan_proto[12] = {'\0'};
#ifdef PC
	//char *cur_wan_proto = "dhcpc";
	char *wan_ifname = "eth0";
#else
	char *cur_wan_proto = NULL;
	char *wan_ifname = NULL;
#endif
	char wan_port_connect[10] = {'\0'};
	char wan_status[24] = {'\0'};
	char wan_netmask[16] = {'\0'};
	char wan_mac[18] = {'\0'};
	unsigned long wan_connect_time = 0;
	int str_day, str_hour, str_min, str_sec ;

	char str_wan_connect_time[36] = {'\0'};

	struct data {
		DFBRectangle rect;
		char *txt;
	} info[17] = {
		{{100,  20, 160, 20}, "Internet Status"},
		{{ 35,  50, 130, 15}, "Connection Type:"},
		{{165,  50, 135, 15}, wan_proto},
		{{ 35,  65, 100, 15}, "Cable Status:"},
		{{130,  65,  90, 15}, wan_port_connect},
		{{ 35,  80, 120, 15}, "Network Status:"},
		{{150,  80, 110, 15}, wan_status},
		{{ 35,  95, 130, 15}, "Connection time:"},
		{{160,  95, 150, 15}, str_wan_connect_time }, //"0 Days,03h:12m:40s"
		{{ 35, 110, 100, 15}, "MAC Address:"},
		{{140, 110, 130, 15}, wan_mac},
		{{ 35, 125, 100, 15}, "IP Address:"},
		{{120, 125, 130, 15}, wan_ip},//12
		{{ 35, 140, 100, 15}, "Subnet Mask:"},
		{{135, 140, 130, 15}, wan_netmask},
		{{ 35, 155,  80, 15}, "Gateway:"},
		{{100, 155, 130, 15}, wan_gateway}
	};


#ifdef PC
	strcpy(wan_port_connect,"1000 Full");
	strcpy(wan_proto,"DHCP Client");
	strcpy(wan_status,"Connect");
	strcpy(wan_mac,"00:22:b0:5e:aa:25");
	wan_connect_time = uptime();
#else
	cur_wan_proto = nvram_safe_get("wan_proto");
	if(cur_wan_proto){
		if(strcasecmp(cur_wan_proto,"dhcpc") == 0){
			strcpy(wan_proto,"DHCP Client");
		}else if(strcasecmp(cur_wan_proto,"static") == 0){
			strcpy(wan_proto,"Static");
		}else if(strcasecmp(cur_wan_proto,"pppoe") == 0){
			strcpy(wan_proto,"PPPoE");
		}else if(strcasecmp(cur_wan_proto,"pptp") == 0){
			strcpy(wan_proto,"PPTP");
		}else if(strcasecmp(cur_wan_proto,"l2tp") == 0){
			strcpy(wan_proto,"L2TP");
		}
#ifdef CONFIG_USER_3G_USB_CLIENT
		else if(strcasecmp(cur_wan_proto,"usb3g") == 0){
			strcpy(wan_proto,"USB_3G");
		}
#endif
		else{
			//unknown proto
		}
		strcpy(wan_status,wan_statue(cur_wan_proto));
	}

	wan_ifname = nvram_safe_get("wan_eth");
	VCTGetPortConnectState(wan_ifname,VCTWANPORT0,wan_port_connect);
	strcpy(wan_mac,nvram_safe_get("wan_mac"));
	wan_connect_time = get_wan_connect_time(cur_wan_proto,1);
#endif // end ifdef PC
	sprintf(wan_ip, "%s", read_ipaddr_no_mask(wan_ifname));
	sprintf(wan_netmask, "%s", read_ipaddr_no_mask(wan_ifname));
	sprintf(wan_gateway, "%s", read_ipaddr_no_mask(wan_ifname));
	//strcpy(wan_ip ,read_ipaddr_no_mask(wan_ifname));
	//strcpy(wan_netmask,read_netmask_addr(wan_ifname));
	//strcpy(wan_gateway,read_gatewayaddr(wan_ifname));
	str_day = wan_connect_time / 86400; //60*60*24 ;
	str_hour = (wan_connect_time % ((str_day != 0) ? (str_day*86400) : 86400)) / 3600; // 60*60
	str_min = (wan_connect_time % ((str_hour != 0) ? (str_hour*3600) : 3600)) / 60 ;
	str_sec = (wan_connect_time % ((str_min != 0) ? (str_min*60) : 60));
	sprintf(str_wan_connect_time,"%d Days,%02dh:%02dm:%02ds",
				str_day,str_hour,str_min,str_sec);

	DEBUG_MSG("lcdshowd: %s (%d), wan connection up time = %s \n",__func__,__LINE__,str_wan_connect_time);

	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };
	DFBColor color = { 0xFF, 145, 150, 255 };


	label_create(win, info[0].rect, NULL, info[0].txt, 20, title_color);
	
	for (i = 1; i < 17; i++) {
		if (i%2 == 0){
			label_create(win, info[i].rect, NULL, 
				info[i].txt, 14, title_color);
		}else{
			label_create(win, info[i].rect, NULL, 
				info[i].txt, 14, color);
		}
	}
}

void wan_start(menu_t *parent_menu)
{
	LiteWindow *win;
	LiteImage *bg_image;

	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };

	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	bg_image = image_create(win, win_rect, "background.png", 0);

	create_label(win);

	time_label = label_create(win,
                                (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
                                NULL,
                                "",
                                12,
                                (DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	wan_page_loop(win);

	lite_destroy_window(win);
}
