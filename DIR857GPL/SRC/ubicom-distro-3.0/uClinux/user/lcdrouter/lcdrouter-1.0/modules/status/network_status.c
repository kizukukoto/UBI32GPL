/*
 * Ubicom Network Player
 *
 * (C) Copyright 2009, Ubicom, Inc.
 * Celil URGAN curgan@ubicom.com
 *
 * The Network Player Reference Code is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Network Player Reference Code is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Network Audio Device Reference Code.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <lite/lite.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/textbutton.h>
#include <leck/list.h>
#include <leck/image.h>
#include <leck/check.h>
#include <leck/label.h>
#include <string.h>

#include <directfb.h>

#include <debug.h>
#include <gui/widgets/grid.h>
#include <gui/widgets/header.h>
#include <gui/widgets/statusbar.h>

#include <gui/widgets/scroll.h>
//#include <gui/widgets/keyboard.h>
#include <gui/mm/menu_data.h>
#include <gui/engine/app_loader.h>

#ifndef PC
#include <nvram.h>
#include <linux_vct.h>
#include "network_utils.h"
#endif

#define NETWORK_STATUS_FONT_SIZE 14

#undef MODULE_NAME 

#if BUILD_STATIC 
#define MODULE_NAME network_status
#else
#define MODULE_NAME 
#endif

#define COMBINE(a,b) a##b
#define SWAP(a,b) COMBINE(b,a)
#define M_(function) SWAP( function, MODULE_NAME )

#define DEBUG

struct M_(app_instance){
	LiteWindow      *window;

	IDirectFBSurface *icon;
	DFBRectangle *rect;
	//add more gui related components to here.

	LiteImage *info_bg;
	LiteLabel *wan_conn_type;
	LiteLabel *wan_cable_status;
	LiteLabel *wan_mac;
	LiteLabel *wan_netmask;
	LiteLabel *wan_ip;
	LiteLabel *wan_primary_dns;
	LiteLabel *wan_secondary_dns;
	LiteLabel *lan_mac;
	LiteLabel *lan_ip;
	LiteLabel *lan_netmask;
/*	LiteLabel *lan_dhcp_enabled;
	LiteLabel *wlan_radio_enabled;
	LiteLabel *wlan_mode;
	LiteLabel *wlan_channel_width;*/
	struct header_instance *hi;
	struct statusbar_instance *sbi;

	int refs;
#ifdef DEBUG
	int	magic;
#endif
};

static debug_levels_t debug_level = ERROR;
#ifndef BUILD_STATIC
debug_levels_t global_debug_level = ERROR;
#endif
#if defined(DEBUG)
#undef  APP_MAGIC
#undef  APP_VERIFY_MAGIC
#define APP_MAGIC 0xBACC
#define APP_VERIFY_MAGIC(app) DEBUG_ERROR("magic called");M_(app_verify_magic)(app)
static
void M_(app_verify_magic)(struct M_(app_instance) *app)
{
	DEBUG_ASSERT(app->magic == APP_MAGIC, "%p: bad magic %x", app, app->magic);
}
#else
#define APP_VERIFY_MAGIC(app)
#endif

static
void M_(app_instance_destroy)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	//stop service thread
	lite_destroy_window( app->window );
	free(app);
}
#if 0
int  M_(app_instance_ref)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	//needs pthread lock
	
	return ++app->refs;
}

int  M_(app_instance_deref)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	DEBUG_ASSERT(app->refs, "%p: app has no references", app);
	//needs pthread llock
	if ( --app->refs == 0 ) {
		M_(app_instance_destroy)(app);
		return 0;
	}
	else {
		return app->refs;
	}
}
#endif

char*  M_(app_instance_get_info)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return "network_status";
}

char*  M_(app_instance_get_type)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return "1";
}

char*  M_(app_instance_get_parent)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return "Root";
}

char*  M_(app_instance_get_weight)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);	
	return "3"; // 1 : desktop 2 : main menu 3: here we are.
}

IDirectFBSurface* M_(app_instance_get_snapshot)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return app->window->surface;
}

IDirectFBSurface* M_(app_instance_get_icon)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	return app->icon;
}


struct M_(app_instance) *global_app = NULL;

void* M_(app_instance_get_shared_data)(struct M_(app_instance) *app)
{
	return NULL;
}

int M_(app_instance_set_shared_data)(struct M_(app_instance) *app, void *data)
{
	return 1;
}

static
void M_(app_instance_draw_static_components)(struct M_(app_instance) *app)
{
	DFBResult       res;
	DFBRectangle      rect;
	DFBColor  textcolor_static = { 0xff, 0xff, 0xff, 0xff};

	// draw this for to understand we are in network status
	//lite_set_window_background_color(app->window, 0xFF, 0x15 * app->window->id, 0x15 * app->window->id, 0xff);

	rect.x = 5 ;
	rect.y = 5 ;
	rect.w = app->rect->w-10 ;
	rect.h = 40 ;

	res = header_instance_alloc(LITE_BOX(app->window), &rect, app->sbi, &app->hi);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("header_instance_alloc failed");
	}

	header_instance_set_text(app->hi, "Network Status");

	// draw static info texts first...
	rect.x = 5; //left offset

	rect.y += rect.h + 5; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->wan_conn_type );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->wan_conn_type create failed");
	}
	lite_set_label_color(app->wan_conn_type, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->wan_cable_status );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->wan_cable_status create failed");
	}
	lite_set_label_color(app->wan_cable_status, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->wan_mac );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->wan_mac create failed");
	}
	lite_set_label_color(app->wan_mac, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->wan_ip );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->wan_ip create failed");
	}
	lite_set_label_color(app->wan_ip, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->wan_netmask );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->wan_netmask create failed");
	}
	lite_set_label_color(app->wan_netmask, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->lan_mac );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->lan_mac create failed");
	}
	lite_set_label_color(app->lan_mac, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->lan_ip );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->lan_ip create failed");
	}
	lite_set_label_color(app->lan_ip, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->lan_netmask );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->lan_netmask create failed");
	}
	lite_set_label_color(app->lan_netmask, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->wan_primary_dns );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->wan_primary_dns create failed");
	}
	lite_set_label_color(app->wan_primary_dns, &textcolor_static);

	rect.y += NETWORK_STATUS_FONT_SIZE + 2; //top offset
	res = lite_new_label(LITE_BOX(app->window), &rect, liteNoLabelTheme, NETWORK_STATUS_FONT_SIZE, &app->wan_secondary_dns );//font size will too small...
	if ( res != DFB_OK ) {
		DEBUG_ERROR("app->wan_secondary_dns create failed");
	}
	lite_set_label_color(app->wan_secondary_dns, &textcolor_static);
}

static
int M_(app_instance_show_app)(struct M_(app_instance) *app)
{
	char status[15] = {0};
	char str[40] = {0};
	char *tmp_str = NULL;

	APP_VERIFY_MAGIC(app);

	strcpy(str, "WAN Connection Type: ");
#ifdef PC
	tmp_str = "wan_proto";
#else
	tmp_str = nvram_safe_get("wan_proto");
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->wan_conn_type, str);

	strcpy(str, "WAN Cable Status: ");
#ifdef PC
	strcpy(status, "connect");
#else
	VCTGetPortConnectState(nvram_safe_get("wan_eth"),VCTWANPORT0,status);
#endif
	if (status) {
		strcat(str, status);
		strcat(str, "ed");
	}
	lite_set_label_text(app->wan_cable_status, str);

	strcpy(str, "WAN MAC Address: ");
#ifdef PC
	tmp_str = "wan_mac";
#else
	tmp_str = nvram_safe_get("wan_mac");
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->wan_mac, str);

	strcpy(str, "WAN IP Address: ");
#ifdef PC
	tmp_str = "wan_eth";
#else
	tmp_str = get_ipaddr(nvram_safe_get("wan_eth"));
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->wan_ip, str);

	strcpy(str, "WAN Netmask: ");
#ifdef PC
	tmp_str = "wan_netmask";
#else
	tmp_str = get_netmask(nvram_safe_get("wan_eth"));
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->wan_netmask, str);

	strcpy(str, "LAN MAC Address: ");
#ifdef PC
	tmp_str = "lan_mac";
#else
	tmp_str = nvram_safe_get("lan_mac");
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->lan_mac, str);

	strcpy(str, "LAN IP Address: ");
#ifdef PC
	tmp_str = "lan_ipaddr";
#else
	tmp_str = nvram_safe_get("lan_ipaddr");
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->lan_ip, str);

	strcpy(str, "LAN Netmask: ");
#ifdef PC
	tmp_str = "lan_netmask";
#else
	tmp_str = nvram_safe_get("lan_netmask");
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->lan_netmask, str);

	strcpy(str, "Primary DNS: ");
#ifdef PC
	tmp_str = "DNS 1";
#else
	tmp_str = get_dns(0);
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->wan_primary_dns, str);

	strcpy(str, "Secondary DNS: ");
#ifdef PC
	tmp_str = "DNS 2";
#else
	tmp_str = get_dns(1);
#endif
	if (tmp_str) {
		strcat(str, tmp_str);
	}
	lite_set_label_text(app->wan_secondary_dns, str);

	lite_update_box(LITE_BOX(app->window), NULL);
	lite_window_set_modal( app->window, 1);
	lite_set_window_opacity( app->window, liteFullWindowOpacity );
	return 1;
}

static
int M_(app_instance_hide_app)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	lite_window_set_modal( app->window, 0);
	lite_set_window_opacity( app->window, liteNoWindowOpacity );
	return 1;
}

static
DFBResult M_(on_key_press)(DFBWindowEvent* evt, void *data)
{
	struct M_(app_instance) *app = (struct M_(app_instance)*) data;
	APP_VERIFY_MAGIC(app);
	
	DEBUG_ERROR("window %d on_key_down() called. key_symbol : %d  \n", app->window->id, evt->key_symbol);
	if (evt->type == DWET_KEYDOWN ) {

		switch ( evt->key_symbol ) {
			case (DIKS_CURSOR_LEFT) :
				app_loader_instance_switch_to_app_by_name(global_app_list, "Main Menu");				
			break;
			default:
			break;
		}
	}
	return DFB_OK;
}

static
DFBResult M_(on_button_press)(DFBWindowEvent* evt, void *data)
{
	struct M_(app_instance) *app = (struct M_(app_instance)*) data;
	APP_VERIFY_MAGIC(app);

	DEBUG_ERROR("window %d on_key_down() called. key_symbol : %d  \n", app->window->id, evt->key_symbol);
	if (evt->type == DWET_BUTTONUP ) {
		app_loader_instance_switch_to_app_by_name(global_app_list, "Main Menu");
	}
	return DFB_OK;
}

int  M_(app_instance_start)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	M_(app_instance_show_app)(app);

	return 1;
}

int  M_(app_instance_pause)(struct M_(app_instance) *app)
{
	APP_VERIFY_MAGIC(app);
	M_(app_instance_hide_app)(app);

	return 1;
}

int  M_(app_instance_idle_callback)(struct M_(app_instance) *app)
{
	//do something not critical
	return 1;
}

struct M_(app_instance)*  M_(app_instance_create)(DFBRectangle *rect, void *app_data)
{
	DFBResult       res;
	
	struct M_(app_instance) *app = calloc(1, sizeof(struct M_(app_instance)));
	if ( app == NULL ) {
		DEBUG_ERROR("create new app instance failed");
		return NULL;
	}

	app->refs = 1;

	int width,height;
	if (!rect) {
		lite_get_layer_size( &width, &height );
		rect = malloc(sizeof(DFBRectangle)); 
		if( rect == NULL) {
			goto error;
		}
		rect->x = 0; rect->y = 0; rect->w = width; rect->h = height;
	}

	app->rect = rect;

#ifdef DEBUG
	app->magic = APP_MAGIC;
#endif

	app->sbi = (struct statusbar_instance *)(app_data);
	
	res = lite_new_window( NULL, 
		app->rect,
		DWCAPS_ALPHACHANNEL,
		liteNoWindowTheme,
		"",
		&app->window);

	if ( res != DFB_OK ) {
		DEBUG_ERROR("create new window failed");
		goto error;
	}

	//DFBWindowOptions	options;
	//app->window->window->GetOptions(app->window->window, &options);
	//app->window->window->SetOptions(app->window->window, options | DWOP_COLORKEYING | DWOP_ALPHACHANNEL);
	//app->window->window->SetColorKey(app->window->window, 0xee,0xee,0xee); //Lite Gui window default bg color

	lite_set_window_background_color(app->window,0xee,0xee,0xee,0x00);
	app->window->window->SetOpaqueRegion(app->window->window, 0,0,0,0);

	DEBUG_ERROR("network status dfb %p", lite_get_dfb_interface());

	M_(app_instance_draw_static_components)(app);

	lite_on_window_keyboard(LITE_WINDOW(app->window), M_(on_key_press), (void*)app);
	lite_on_window_mouse(LITE_WINDOW(app->window), M_(on_button_press), (void*)app);

	global_app = app;

	return app;	
error:
	M_(app_instance_destroy)(app);
	return NULL;
}

