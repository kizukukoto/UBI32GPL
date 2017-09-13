#include <leck/label.h>
#include "global.h"
#include "timer.h"
#include "image.h"
#include "menu.h"
#include "label.h"
#include "network_util.h"

extern void menu_clock_timer();
static LiteLabel *time_label;
int lan_page_loop(LiteWindow *win)
{
	lite_set_window_opacity(win, 0xFF);
	timer_option opt;
        opt = (timer_option) {
                .func = menu_clock_timer,
                .timeout = 1000,
                .timeout_id = 10001,
                .data = time_label};

        timer_start(&opt);

	while (true) {
		int x, y;
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_NONE)
			break;
	}

        timer_stop(&opt);
	lite_set_window_opacity(win, 0x00);

	return 0;
}

void lan_start(menu_t *parent_menu)
{
	LiteWindow *win;
	LiteImage *bg_image;

	LiteLabel *title;
	LiteLabel *mac_title, *mac_label;
	LiteLabel *ip_title, *ip;
	LiteLabel *mask_title, *mask;
	LiteLabel *dhcp_title, *dhcp;
	LiteLabel *cable_sta_title1, *cable_sta1;
	LiteLabel *cable_sta_title2, *cable_sta2;
	LiteLabel *cable_sta_title3, *cable_sta3;
	LiteLabel *cable_sta_title4, *cable_sta4;

	DFBColor title_color = { 0xFF, 0xFF, 0xFF, 0xFF };
	DFBColor color = { 0xFF, 145, 150, 255 };

	DFBRectangle win_rect = { 0, 0, layer_width, layer_height };
	DFBRectangle title_rect = { 100, 20, 160, 20 };
	DFBRectangle mac_title_rect = { 35, 50, 100, 15 };
	DFBRectangle mac_rect = { 140, 50, 135, 15 };
	DFBRectangle ip_title_rect = { 35, 65, 80, 15 };
	DFBRectangle ip_rect = { 120, 65, 120, 15 };
	DFBRectangle mask_title_rect = { 35, 80, 95, 15 };
	DFBRectangle mask_rect = { 140, 80, 110, 15 };
	DFBRectangle dhcp_title_rect = { 35, 95, 95, 15 };
	DFBRectangle dhcp_rect = { 140, 95, 70, 15 };
	DFBRectangle cable_title_rect1 = { 35, 110, 140, 15 };
	DFBRectangle cable_rect1 = { 180, 110, 80, 15 };
	DFBRectangle cable_title_rect2 = { 35, 125, 140, 15 };
	DFBRectangle cable_rect2 = { 180, 125, 80, 15 };
	DFBRectangle cable_title_rect3 = { 35, 140, 140, 15 };
	DFBRectangle cable_rect3 = { 180, 140, 80, 15 };
	DFBRectangle cable_title_rect4 = { 35, 155, 140, 15 };
	DFBRectangle cable_rect4 = { 180, 155, 80, 15 };
#ifdef PC
	char *mac_value = "00:11:22:33:44:55";
	char *ip_value = "192.168.0.1";
	char *netmask_value = "255.255.255.0";
	char *lan_port_connect_0 = "Disconnect";
	char *lan_port_connect_1 = "Disconnect";
	char *lan_port_connect_2 = "Disconnect";
	char *lan_port_connect_3 = "Disconnect";
#else
	char *mac_value = NULL;
	char *ip_value = NULL;
	char *netmask_value = NULL;
	char *lan_ifname = NULL;
	char lan_port_connect_0[10] = "\0";
	char lan_port_connect_1[10] = "\0";
	char lan_port_connect_2[10] = "\0";
	char lan_port_connect_3[10] = "\0";
#endif

	char dhcpd_enable[16] = "Disabled";

#ifdef PC
	
#else
	lan_ifname = nvram_safe_get("lan_eth");
	mac_value = nvram_safe_get("lan_mac");
	ip_value = nvram_safe_get("lan_ipaddr"); //get_ipaddr("eth0");
	netmask_value = nvram_safe_get("lan_netmask"); //get_netmask("eth0");
	if(strcmp(nvram_safe_get("dhcpd_enable"),"1") == 0){
		memset(dhcpd_enable,'\0',sizeof(dhcpd_enable));
		strncpy(dhcpd_enable,"Enabled",strlen("Enabled"));
	}
	VCTGetPortConnectState(lan_ifname,VCTLANPORT0,lan_port_connect_0);
	VCTGetPortConnectState(lan_ifname,VCTLANPORT1,lan_port_connect_1);
	VCTGetPortConnectState(lan_ifname,VCTLANPORT2,lan_port_connect_2);
	VCTGetPortConnectState(lan_ifname,VCTLANPORT3,lan_port_connect_3);
#endif


	lite_new_window(NULL, &win_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &win);

	bg_image = image_create(win, win_rect, "background.png", 0);

	time_label = label_create(win,
                                (DFBRectangle) { layer_width / 2 - 80, 220, 180, 20 },
                                NULL,
                                "",
                                12,
                                (DFBColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	title = label_create(win, title_rect, NULL, "Wired Status", 20, title_color);

	mac_title = label_create(win, mac_title_rect, NULL, "MAC Address:", 14, color);
	mac_label = label_create(win, mac_rect, NULL, mac_value, 14, title_color);

	ip_title = label_create(win, ip_title_rect, NULL, "IP Address:", 14, color);
	ip = label_create(win, ip_rect, NULL, ip_value, 14, title_color);

	mask_title = label_create(win, mask_title_rect, NULL, "Subnet Mask:", 14, color);
	mask = label_create(win, mask_rect, NULL, netmask_value, 14, title_color);

	dhcp_title = label_create(win, dhcp_title_rect, NULL, "DHCP Server:", 14, color);
	dhcp = label_create(win, dhcp_rect, NULL, dhcpd_enable, 14, title_color);

	cable_sta_title1 = label_create(win, cable_title_rect1, NULL, "Cable Status Port 1:", 14, color);
	cable_sta1 = label_create(win, cable_rect1, NULL, lan_port_connect_0, 14, title_color);

	cable_sta_title2 = label_create(win, cable_title_rect2, NULL, "Cable Status Port 2:", 14, color);
	cable_sta2 = label_create(win, cable_rect2, NULL, lan_port_connect_1, 14, title_color);

	cable_sta_title3 = label_create(win, cable_title_rect3, NULL, "Cable Status Port 3:", 14, color);
	cable_sta3 = label_create(win, cable_rect3, NULL, lan_port_connect_2, 14, title_color);

	cable_sta_title4 = label_create(win, cable_title_rect4, NULL, "Cable Status Port 4:", 14, color);
	cable_sta4 = label_create(win, cable_rect4, NULL, lan_port_connect_3, 14, title_color);

	lan_page_loop(win);
	lite_destroy_window(win);
}
