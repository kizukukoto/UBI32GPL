#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <leck/progressbar.h>
#include <lite/font.h>
#include <leck/textline.h>

#include "global.h"
#include "label.h"
#include "menu.h"
#include "mouse.h"
#include "image.h"
#include "misc.h"
#include "input.h"
#include "wizard.h"
#include "select.h"
#include "keydefine.h"

#include "check.h"

extern char *get_local_lan();

/* A */
void wizard_step1 (LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};

	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{120, 20, 270, 20}, "Welcome", color, 18},
		{{10, 80, 310, 20}, "This Touch Setup Wizard will guide you through ", color, 12},
		{{10, 95, 310, 20}, "a step-by-step process to configure your new ", color, 12},
		{{10, 110, 310, 20}, "D-Link router and connect to the Internet.", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_nothanks.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_begin.png", 0 ); 

	for ( i = 0; i < 4; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

}

/* B */
void wizard_step2 (LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	DFBColor step_color = {0xFF, 0x66, 0x99, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{120, 20, 270, 20}, "Welcome", color, 18},
		{{50, 60, 270, 20}, "Step 1: ", step_color, 12},
		{{100, 60, 270, 20}, "Install your router", color, 12},
		{{50, 80, 270, 20}, "Step 2: ", step_color, 12},
		{{100, 80, 270, 20}, "Configure your Internet Connection", color, 12},
		{{50, 100, 270, 20}, "Step 3", step_color, 12},
		{{100, 100, 270, 20}, "Configure your WLAN settings", color, 12},
		{{50, 120, 270, 20}, "Step 4:", step_color, 12},
		{{100, 120, 270, 20}, "Select your time zone", color, 12},
		{{50, 140, 270, 20}, "Step 5:", step_color, 12},
		{{100, 140, 270, 20}, "Save settings and Connect", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[1], "wizard_next.png", 0 );

	for ( i = 0; i < 11; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

}

/* C1 */
void wizard_install_router1 (LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{90, 20, 270, 20}, "Install your Router", color, 18},
		{{200, 75, 270, 20}, "Please turn off or ", color, 13},
		{{200, 90, 270, 20}, "unplug power to ", color, 13},
		{{200, 105, 270, 20}, "your Cable or DSL", color, 13},
		{{200, 120, 270, 20}, "modem", color, 13},
		{{125, 125, 270, 20}, "modem", color, 9}
	};

	DFBRectangle img_rect[] =
	{
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55},
		{ 0, 0, 296, 240}
	};

	image_create ( win, img_rect[0], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[1], "wizard_next.png", 0 );
	image_create ( win, img_rect[2], "wizard_C1.png", 0 );

	for ( i = 0; i < 6; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

}

/* C2 */
void wizard_install_router2 ( LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{90, 20, 270, 20}, "Install your Router", color, 18},
		{{10, 50, 270, 20}, "Plug the Ethernet cable of Cable or", color, 12},
		{{10, 65, 270, 20}, "DSL Modem to the              Port of ", color, 12},
		{{125, 65, 270, 20}, "Internet", {0xFF, 0x66, 0x99, 0xFF}, 12},
		{{10, 80, 270, 20}, "the router", color, 12},
		//{{10, 110, 270, 20}, "router", color, 9},
		{{267, 110, 270, 20}, "modem", color, 9}

	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55},
		{   0,   0, 320, 228}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 );
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 );
	image_create ( win, img_rect[3], "wizard_C2.png", 0 );

	for ( i = 0; i < 6; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
}

/* C3 */
void wizard_install_router3 ( LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{90, 20, 270, 20}, "Install your Router", color, 18},
		{{10, 50, 270, 20}, "Plug the Ethernet cable between", color, 12},
		{{10, 65, 270, 20}, "any Ethernet port of the Router and", color, 12},
		{{10, 80, 270, 20}, "your PC", color, 12},
		//{{10, 110, 270, 20}, "router", color, 9},
		{{267, 110, 270, 20}, "    PC", color, 9}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55},
		{   0,   0, 320, 228}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 );
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 );
	image_create ( win, img_rect[3], "wizard_C3.png", 0 ); 

	for ( i = 0; i < 5; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
}

/* C4 */
void wizard_install_router4 ( LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{90, 20, 270, 20}, "Install your Router", color, 18},
		{{20, 50, 320, 20}, "Please double check your network is like below", color, 12},
		//{{10, 120, 270, 20}, "router", color, 10},
		{{285, 140, 270, 20}, "PC", color, 10},
		{{200, 120, 270, 20}, "modem", color, 9}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55},
		{   0,   0, 320,  237}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 );
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 );
	image_create ( win, img_rect[3], "wizard_C4.png", 0 );

	for ( i = 0; i < 4; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
}

/* C5 */
void wizard_install_router5 ( LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{90, 20, 270, 20}, "Install your Router", color, 18},
		{{60, 50, 320, 20}, "Please power on your Modem and PC", color, 12},
		//{{10, 120, 270, 20}, "router", color, 10},
		{{285, 140, 270, 20}, "PC", color, 10},
		{{200, 120, 270, 20}, "modem", color, 9}

	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55},
		{   0,   0, 320, 237}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 ); 
	image_create ( win, img_rect[3], "wizard_C5.png", 0 ); 

	for ( i = 0; i < 4; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
}
/*
struct detect_struct {
	char text1[24];
	LiteProgressBar *progress;
	float degree;
};
*/
/* D1 detect function */
void auto_detect_processbar (void *data)
{
	struct detect_struct *kboard = ( struct detect_struct * ) data;
	if ( kboard->degree <= 1.0 )
	{
		kboard->degree += 0.3;
		lite_set_progressbar_value ( kboard->progress, kboard->degree );
		/*
		if ( kboard->degree >= 1.0) {
			label_create (kboard->win, 
					(DFBRectangle){100, 140, 280, 24}, NULL,
		                         "Detection Over", 20, 
					(DFBColor){0xFF, 0xFF, 0xFF, 0xFF});

		}
		*/

		if ( kboard->degree >= 0.5 && kboard->degree < 1.0 ) 
		{
			int k = 0;
#ifndef PC
			k = system ( "wt eth0" );
#else
			k = system ( "./wizard/wt/wt eth0" );
#endif
			wait ( &k );
			int tmp = WEXITSTATUS ( k );
			if ( tmp <= 2 )
			{
				printf ( "dhcpc\n" );
				strcpy ( kboard->text1,
				         STR_2_GUI_WAN_PROTO_DHCPC );// wan_proto
			}
			else if ( tmp == 3 )
			{
				printf ( "pppoe\n" );
				strcpy ( kboard->text1,
				         STR_2_GUI_WAN_PROTO_PPPOE );
			}
			kboard->degree = 0.7;
		}
	}
}

/* D1 */
void wizard_auto_detect(LiteWindow *win, struct detect_struct *kboard)
{
	int i = 0;
	char fullname[MAX_FILENAME] = {'\0'};
	char fullname_tmp[MAX_FILENAME] = {'\0'};
	sprintf ( fullname, "%swizard_D1_bar.png", IMAGE_DIR );
	sprintf ( fullname_tmp, "%swizard_D1_bar_bg.png", IMAGE_DIR );
	
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	DFBRectangle bar_rect = {20, 100, 280, 24};

	struct wizard {
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] = {
		{{80, 20, 270, 20}, "Internet Connecttion", color, 18},
		{{10, 60, 300, 20}, "The router is detecting your Internet Connecttion.", color, 12}
	};

	DFBRectangle img_rect[] = {
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_cancel.png", 0 );
	//image_create ( win, img_rect[1], "wizard_next.png", 0 );

	for ( i = 0; i < 2; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
	/* process bar initialize */
	lite_new_progressbar ( LITE_BOX (win), &bar_rect, liteNoProgressBarTheme, &kboard->progress);
	lite_set_progressbar_images ( kboard->progress, fullname, fullname_tmp );
	lite_set_progressbar_value ( kboard->progress, 0.0 );
	kboard->degree = 0.0;
}

/* D2 */
void wizard_dhcp_check_cable ( LiteWindow *win)
{
	printf("XXX %s(%d)\n", __FUNCTION__, __LINE__);
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{90, 20, 270, 20}, "DHCP Connection", color, 18},
		{{50, 60, 270, 20}, "Are you a Cable Broadband subscriber?", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{  30, 80, 133, 83},
		{ 170, 80, 133, 83}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 ); 
	image_create ( win, img_rect[2], "wizard_yes.png", 0 ); 
	image_create ( win, img_rect[3], "wizard_no.png", 0 ); 

	for ( i = 0; i < 2; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
}

/* D3 */
#ifdef SELECT_UI
void wizard_DHCP_Settings(LiteWindow *win, struct dhcp_struct *kboard, LiteTextLine **text, char *opt[])
#else
void wizard_DHCP_Settings ( LiteWindow *win, struct dhcp_struct *kboard, ComponentList **list)
#endif
{
	int i = 0, j = 0;
	char tmp[1024],  tmp1[128], *pp;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
#ifdef SELECT_UI
	int opt_i = 0;
	LiteTextButton *btn;
	LiteTextButtonTheme *textButtonTheme;
	DFBRectangle text_rect = { 140, 95, 160, 20 };
	DFBRectangle btn_rect = { 300, 95, 20, 20 };
#endif

	memset ( tmp, '\0', sizeof ( tmp ) );
	memset ( tmp1, '\0', sizeof ( tmp1 ) );

	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{90, 20, 270, 20}, "DHCP Connection", color, 18},
		{{10, 50, 320, 20}, "Please make sure that you are connected to the D-Link", color, 11},
		{{10, 65, 320, 20}, "Router with the PC that was originally connected to", color, 11},
		{{10, 80, 320, 20}, "your broadband connection.", color, 11},
	}, _txtline[] =
	{
		{{40, 95, 100, 20}, "Select Your PC : ", color, 12}, //txt
		{{140, 95, 150, 20}, "", color, 12}, //txtline
		{{140, 120, 150, 40}, "", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 );
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 );

	for ( i = 0; i < 4; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

	strcpy ( kboard->text1, _txtline[0].txt );
	kboard->title = label_create ( win, _txtline[0].rect, NULL, _txtline[0].txt,
	                                  _txtline[0].font_size, _txtline[0].def_color );
#ifndef SELECT_UI
	lite_new_textline ( LITE_BOX ( win ), &_txtline[1].rect,
                            liteNoTextLineTheme, &kboard->txtline);
#endif
	get_local_lan ( &tmp, &j );
	
	memset ( kboard->client, '\0', sizeof ( kboard->client ) );
	memcpy ( kboard->client, tmp, sizeof ( tmp ) );

	pp = tmp;

#ifdef SELECT_UI
	lite_new_textline(LITE_BOX(win), &text_rect, liteNoTextLineTheme, text);
	lite_new_text_button_theme(IMAGE_DIR"textbuttonbgnd.png", &textButtonTheme);
	lite_new_text_button(LITE_BOX(win), &btn_rect, "Yes", textButtonTheme, &btn);

	if (kboard->str) {
		opt[opt_i++] = strdup(kboard->str);
		if (opt_i == 1)
			lite_set_textline_text(*text, opt[0]);
	}

	if (strlen(pp) > 0 && j != 0) {
		for (i = 0; i < j && strlen(pp); i++) {
			if (check_mac(pp) == -1) break;
			sscanf(pp, "%s", tmp1);
			if (kboard->str) {
				if (strncmp(tmp1, kboard->str, strlen(kboard->str)) != 0) {
					opt[opt_i++] = strdup(tmp1);
					printf("XXX %s\n", tmp1);
					fflush(stdout);
				}
			} else {
				opt[opt_i++] = strdup(tmp1);
				printf("XXX %s\n", tmp1);
				fflush(stdout);
			}
			pp += 24;

			if (opt_i == 1)
				lite_set_textline_text(*text, tmp1);
		}
	}
	opt[opt_i] = NULL;
#else
	*list = select_create(win, _txtline[1].rect, _txtline[2].rect,
                                 "textbuttonbgnd.png", "scrollbar.png", color, true);

	select_set_name(*list, _txtline[0].txt);
        select_set_button_text(*list, "V");

	if (kboard->str) {
		select_add_item(*list, kboard->str);
	}
	if (strlen(pp) > 0 && j != 0) {
		for (i = 0; i < j && strlen(pp); i++) {
			sscanf(pp, "%s", tmp1);
			if (kboard->str){
				if (strncmp(tmp1, kboard->str, strlen(kboard->str)) != 0)
					select_add_item(*list, tmp1);
			} else 
				select_add_item(*list, tmp1);
			pp+=24;
		}
		//select_set_default_item(*list, 0);
	}
        set_mouse_event_passthrough(DFB_OK);
	//set_mouse_advanced_flag(false);
#endif
}

/* D4 */
void wizard_PPPoE_Settings ( LiteWindow *win, struct pppoe_struct *kboard)
{
	int i = 0;

	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{80, 20, 270, 20}, "PPPoE Connection", color, 18},
		{{10, 60, 310, 20}, "To set up this connection you will need to have a User", color, 11},
		{{10, 75, 310, 20}, "Name and Password from your Internet Service Provider.", color, 11}
	}, _txtline[] =
	{
		{{60, 140, 100, 20}, "User Name : ", color, 12},
		{{140, 140, 150, 20}, "", color, 12},
		{{60, 160, 100, 20}, "Password  : ", color, 12},
		{{140, 160, 150, 20}, "", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 );
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 ); 
	image_create ( win, img_rect[2], "wizard_next.png", 0 ); 

	for ( i = 0; i < 3; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

	strcpy ( kboard->text1, _txtline[0].txt );
	kboard->title[0] = label_create ( win, _txtline[0].rect, NULL, _txtline[0].txt,
	                                  _txtline[0].font_size, _txtline[0].def_color );
	lite_new_textline ( LITE_BOX ( win ), &_txtline[1].rect,
	                    liteNoTextLineTheme, &kboard->txtline[0] );
	strcpy ( kboard->text2, _txtline[2].txt );
	kboard->title[1] = label_create ( win, _txtline[2].rect, NULL, _txtline[2].txt,
	                                  _txtline[2].font_size, _txtline[2].def_color );
	lite_new_textline ( LITE_BOX ( win ), &_txtline[3].rect,
	                    liteNoTextLineTheme, &kboard->txtline[1] );
}

/* E1 */
void wizard_24G_Set_SSID ( LiteWindow *win, struct wizard_wlan_struct *kboard)
{
	int i = 0;

	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{50, 20, 270, 20}, "2.4GHz", {0xFF, 0x66, 0x99, 0xFF}, 18},
		{{120, 20, 270, 20}, "Wireless Settings", color, 18},
		{{15, 60, 320, 20}, "Give your network a name, using up to 32 characters.", color, 11}
	}, _txtline[] =
	{
		{{60, 100, 130, 20}, "Network Name ", color, 12},
		{{150, 100, 150, 20}, "", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 ); 

	for ( i = 0; i < 3; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

	strcpy ( kboard->text1, _txtline[0].txt );
	kboard->title = label_create ( win, _txtline[0].rect, NULL, _txtline[0].txt,
	                                  _txtline[0].font_size, _txtline[0].def_color );
	lite_new_textline ( LITE_BOX ( win ), &_txtline[1].rect,
	                    liteNoTextLineTheme, &kboard->txtline);
}

/* E2 */
void wizard_24G_Set_PWD ( LiteWindow *win, struct wizard_wlan_struct *kboard)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{50, 20, 270, 20}, "2.4GHz", {0xFF, 0x66, 0x99, 0xFF}, 18},
		{{120, 20, 270, 20}, "Wireless Settings", color, 18},
		{{80, 60, 270, 20}, "Please assign a network key", color, 12},
		{{60, 80, 270, 20}, "(8-to 63-character alphanumerically)", color, 12}
	}, _txtline[] =
	{
		{{60, 100, 150, 20}, "Network Key", color, 12},
		{{150, 100, 150, 20}, "", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 ); 

	for ( i = 0; i < 4; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

	strcpy ( kboard->text1, _txtline[0].txt );
	kboard->title = label_create ( win, _txtline[0].rect, NULL, _txtline[0].txt,
	                                  _txtline[0].font_size, _txtline[0].def_color );
	lite_new_textline ( LITE_BOX ( win ), &_txtline[1].rect,
	                    liteNoTextLineTheme, &kboard->txtline );
}

/* E3 */
void wizard_5G_Set_SSID ( LiteWindow *win, struct wizard_wlan_struct *kboard)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{60, 20, 270, 20}, "5GHz", {0xFF, 0x66, 0x99, 0xFF}, 18},
		{{120, 20, 270, 20}, "Wireless Settings", color, 18},
		{{30, 60, 290, 20}, "Give your            network a name, using up to", color, 12},
		{{95, 60, 270, 20}, "media", {0xFF, 0x66, 0x99, 0xFF}, 12},
		{{30, 80, 270, 20}, "32 characters.", color, 12}
	}, _txtline[] =
	{
		{{60, 100, 100, 20}, "Network Name ", color, 12},
		{{150, 100, 150, 20}, "", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 );
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 );
	image_create ( win, img_rect[2], "wizard_next.png", 0 ); 

	for ( i = 0; i < 5; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

	strcpy ( kboard->text1, _txtline[0].txt );
	kboard->title = label_create ( win, _txtline[0].rect, NULL, _txtline[0].txt,
	                                  _txtline[0].font_size, _txtline[0].def_color );
	lite_new_textline ( LITE_BOX ( win ), &_txtline[1].rect,
	                    liteNoTextLineTheme, &kboard->txtline);
}

/* E4 */
void wizard_5G_Set_PWD ( LiteWindow *win, struct wizard_wlan_struct *kboard)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{60, 20, 270, 20}, "5GHz", {0xFF, 0x66, 0x99, 0xFF}, 18},
		{{120, 20, 270, 20}, "Wireless Settings", color, 18},
		{{80, 60, 270, 20}, "Please assign a network key.", color, 12},
		{{60, 80, 270, 20}, "(8-to 63-character alphanumerically).", color, 12},
	}, _txtline[] =
	{
		{{60, 100, 270, 20}, "Network Key : ", color, 12},
		{{150, 100, 150, 20}, "", color, 12}
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 ); 
	image_create ( win, img_rect[2], "wizard_next.png", 0 ); 

	for ( i = 0; i < 4; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

	strcpy ( kboard->text1, _txtline[0].txt );
	kboard->title = label_create ( win, _txtline[0].rect, NULL, _txtline[0].txt,
	                                  _txtline[0].font_size, _txtline[0].def_color );
	lite_new_textline ( LITE_BOX ( win ), &_txtline[1].rect,
	                    liteNoTextLineTheme, &kboard->txtline);
}

/* E5 */
void wizard_optimize_channel_band ( LiteWindow *win)
{
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{30, 20, 270, 20}, "Optimize Channel bandwidth", color, 18},
		{{5, 60, 320, 20}, "Do you want the setup wizard to automatically optimize", color, 11},
		{{5, 80, 320, 20}, "channel bandwidth to increase the speed of your router?", color, 11},
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{  30, 80, 133, 83},
		{ 170, 80, 133, 83}
	};

	image_create ( win, img_rect[0], "wizard_back.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 ); 
	image_create ( win, img_rect[2], "wizard_yes_1.png", 0 ); 
	image_create ( win, img_rect[3], "wizard_no.png", 0 ); 

	for ( i = 0; i < 3; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
}

/* G1 Summary */
void wizard_Summary_internet ( LiteWindow *win, struct _record *rd)
{
	char tmp_tz[256], *ptr, *tz1 = NULL, *tz2 = NULL, *tz3 = NULL, *tz4 = NULL;
	int i = 0;
	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	DFBColor title_color = {0xFF, 0x66, 0x99, 0xFF};

	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{80, 20, 270, 20}, "Settings Summary", color, 18}, //0
		{{63, 40, 160, 20}, "Connection Type: ", title_color, 10}, //1
		{{160, 40, 270, 20}, "", color, 10}, //2
		{{59, 55, 160, 20}, "PPPoE User Name: ", title_color, 10}, //3
		{{160, 55, 270, 20}, "", color, 10}, //4
		{{71, 70, 160, 20}, "PPPoE Password: ", title_color, 10}, //5
		{{160, 70, 270, 20}, "", color, 10}, //6
		{{42, 85, 170, 20}, "Cloned MAC Address: ", title_color, 10}, //7
		{{160, 85, 270, 20}, "", color, 10}, //8
		{{35, 100, 170, 20}, "2.4GHz Network Name: ", title_color, 10}, //9
		{{160, 100, 270, 20}, "", color, 12}, //10
		{{0, 115, 190, 20}, "2.4GHz Wireless Network Key: ", title_color, 10}, //11
		{{160, 115, 270, 20}, "", color, 12}, //12
		{{43, 130, 170, 20}, "5GHz Network Name: ", title_color, 10}, //13
		{{160, 130, 270, 20}, "", color, 12}, //14
		{{11, 145, 190, 20}, "5GHz Wireless Network key: ", title_color, 10},//15
		{{160, 145, 270, 20}, "", color, 12}, //16
		{{97, 160, 150, 20}, "TimeZone: ", title_color, 10}, //17
		{{160, 160, 270, 20}, "", color, 10}, //18
		{{160, 175, 270, 20}, "", color, 10}, //19
		{{160, 190, 270, 20}, "", color, 10}, //20
		{{160, 205, 270, 20}, "", color, 10} //21
	};

	DFBRectangle img_rect[] =
	{
		{  36, 190,  76,  55},
		{ 122, 190,  76,  55},
		{ 206, 190,  76,  55}
	};
	
	image_create ( win, img_rect[0], "wizard_back.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_cancel.png", 0 ); 
	image_create ( win, img_rect[2], "wizard_next.png", 0 ); 

	memset ( wizard[2].txt, '\0', 256 );
	memset ( wizard[4].txt, '\0', 256 );
	memset ( wizard[6].txt, '\0', 256 );
	memset ( wizard[8].txt, '\0', 256 );
	memset ( wizard[10].txt, '\0', 256 );
	memset ( wizard[12].txt, '\0', 256 );
	memset ( wizard[14].txt, '\0', 256 );
	memset ( wizard[16].txt, '\0', 256 );
	memset ( wizard[18].txt, '\0', 256 );

	strcpy ( wizard[2].txt, rd->wan_type );
	strcpy ( wizard[4].txt, rd->username );
	strcpy ( wizard[6].txt, rd->password );
	strcpy ( wizard[8].txt, rd->pc );
	strcpy ( wizard[10].txt, rd->wlan0_ssid );
	strcpy ( wizard[12].txt, rd->wlan0_pwd );
	strcpy ( wizard[14].txt, rd->wlan1_ssid );
	strcpy ( wizard[16].txt, rd->wlan1_pwd );

	sprintf(tmp_tz, "%s", rd->timezone);
	ptr = tmp_tz;
	tz1 = strsep(&ptr, ",");
	tz2 = strsep(&ptr, ",");
	tz3 = strsep(&ptr, ",");
	tz4 = strsep(&ptr, ",");
	//strcpy ( wizard[18].txt, rd->timezone );

	for ( i = 0; i < 3; i++) {
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );
	}

	if (strcmp(wizard[2].txt , "pppoe") == 0) {
                for (i = 3; i < 18; i++) {
                        label_create(win, wizard[i].rect, NULL,
                                wizard[i].txt, wizard[i].font_size, wizard[i].def_color);
                }
		label_create(win, wizard[18].rect, NULL, tz1, 9, wizard[18].def_color);
		if (tz2)
			label_create(win, wizard[19].rect, NULL, tz2, 9, wizard[19].def_color);
		if (tz3)
			label_create(win, wizard[20].rect, NULL, tz3, 9, wizard[20].def_color);
		if (tz4)
			label_create(win, wizard[21].rect, NULL, tz4, 9, wizard[21].def_color);

        } else {
                for (i = 7; i < 18; i++) {
                        wizard[i].rect.y = wizard[i].rect.y - 30;
                        label_create(win, wizard[i].rect, NULL,
                                wizard[i].txt, wizard[i].font_size, wizard[i].def_color);
                }
                wizard[18].rect.y = wizard[18].rect.y - 30;
                wizard[19].rect.y = wizard[19].rect.y - 30;
                wizard[20].rect.y = wizard[20].rect.y - 30;
                wizard[21].rect.y = wizard[21].rect.y - 30;
		label_create(win, wizard[18].rect, NULL, tz1, 9, wizard[18].def_color);
		if (tz2)
			label_create(win, wizard[19].rect, NULL, tz2, 9, wizard[19].def_color);
		if (tz3)
			label_create(win, wizard[20].rect, NULL, tz3, 9, wizard[20].def_color);
		if (tz4)
			label_create(win, wizard[21].rect, NULL, tz4, 9, wizard[21].def_color);
        }
}

/* G2 Save Settings and Connect */
void wizard_Set_and_Conn ( LiteWindow *win)
{
	int i = 0;

	DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
	struct wizard
	{
		DFBRectangle rect;
		char txt[256];
		DFBColor def_color;
		int font_size;
	} wizard[] =
	{
		{{70, 20, 270, 20}, "Save Settings and Connect", color, 14}
	};


	DFBRectangle img_rect[] =
	{
		{ 122, 190,  76,  55},
		{ 23, 50, 284, 83},
		{ 23, 100, 284, 83}
	};

	image_create ( win, img_rect[0], "wizard_cancel.png", 0 ); 
	image_create ( win, img_rect[1], "wizard_savesettings.png", 0 ); 
	image_create ( win, img_rect[2], "wizard_startover.png", 0 ); 

	for ( i = 0; i < 1; i++)
		label_create ( win, wizard[i].rect, NULL,
		                          wizard[i].txt, wizard[i].font_size, wizard[i].def_color );

}
