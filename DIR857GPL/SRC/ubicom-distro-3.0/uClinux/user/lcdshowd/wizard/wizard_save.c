#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <leck/label.h>
#include <lite/font.h>
#include <leck/textline.h>
#include <leck/progressbar.h>

#include "wizard.h"
#include "misc.h"
//#include "keydefine.h"

#if 0
//#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif
static enum wizard_key_idx {
		MAC = 0,
		WAN_TYPE,
		USERNAME,
		PASSWORD,
		WLAN0_SSID,
		WLAN0_SECURITY,
		WLAN0_PWD,
		WLAN1_SSID,
		WLAN1_SECURITY,
		WLAN1_PWD,
		OPTIMIZE,
		TIMEZONE,
		WAN_MAC,
		LCDSHOWD,
};

static struct wizard_nvram_key{
        char *key;
} wkey[] = {
		{"mac_clone_addr"}, 		// pc
		{"wan_proto"},			// wan_type 
		{"wan_pppoe_username_00"}, 		// username
		{"wan_pppoe_password_00"}, 		// password
		{"wlan0_ssid"}, 			// wlan0_ssid
		{"wlan0_security"},
		{"wlan0_psk_pass_phrase"},		// wlan0_pwd 
		{"wlan1_ssid"}, 			// wlan1_ssid
		{"wlan1_security"},
		{"wlan1_psk_pass_phrase"}, 		// wlan1_pwd
		{"wlan0_11n_protection"}, 		// optimize
		{"time_zone"},			// time_zone
		{"wan_mac"},
		{"lcd_need_run_wizard"},
		{NULL}
	};

static void debug_msg()
{
	DEBUG_MSG("wizard finished !\n");
	DEBUG_MSG("rd->wan_type = %s (wan_proto) !!!\n",
			record.wan_type);
	DEBUG_MSG("rd->username = %s   (wan_pppoe_username_00) !!!\n",
			record.username ? record.username : "EmtpyString");
	DEBUG_MSG("rd->password = %s   (wan_pppoe_password_00) !!!\n",
			record.password ? record.password : "EmtpyString");
	DEBUG_MSG("rd->optimize = %d   (wlan0_11n_protection) !!!\n",
			record.optimize);
	DEBUG_MSG("rd->wlan0_ssid = %s (wlan0_ssid) !!!\n",
			record.wlan0_ssid);
	DEBUG_MSG("rd->wlan0_pwd = %s  (wlan0_psk_pass_phrase) !!!\n",
			record.wlan0_pwd);
	DEBUG_MSG("rd->wlan1_ssid = %s (wlan1_ssid) !!!\n",
			record.wlan1_ssid);
	DEBUG_MSG("rd->wlan1_pwd = %s  (wlan1_psk_pass_phrase) !!!\n",
			record.wlan1_pwd);
	DEBUG_MSG("rd->pc = %s         (mac_clone_addr) !!!\n",
			record.pc);
}

int wizard_save(struct _record *rd)
{
	// do you want to do something.
	debug_msg();
	printf("wizard save\n");
#ifndef PC
	char tmp[24];
	if (strcmp(rd->wan_type, STR_2_GUI_WAN_PROTO_DHCPC) == 0 ) {
		nvram_set(wkey[WAN_TYPE].key, STR_2_NVRAM_WAN_PROTO_DHCPC);
	} else if (strcmp(rd->wan_type, STR_2_GUI_WAN_PROTO_PPPOE) == 0 ) {
		nvram_set(wkey[WAN_TYPE].key, STR_2_NVRAM_WAN_PROTO_PPPOE);
		nvram_set(wkey[USERNAME].key, rd->username);
		nvram_set(wkey[PASSWORD].key, rd->password);
	}

	if (rd->optimize == 1) {
		nvram_set(wkey[OPTIMIZE].key, "auto");
	}
	nvram_set(wkey[WLAN0_SSID].key, rd->wlan0_ssid );
	nvram_set(wkey[WLAN0_SECURITY].key, "wpa2auto_psk" );
	nvram_set(wkey[WLAN0_PWD].key, rd->wlan0_pwd );
	nvram_set(wkey[WLAN1_SSID].key, rd->wlan1_ssid );
	nvram_set(wkey[WLAN1_SECURITY].key, "wpa2auto_psk" );
	nvram_set(wkey[WLAN1_PWD].key, rd->wlan1_pwd );
	sprintf(tmp, "%d", rd->tz_value);
	nvram_set(wkey[TIMEZONE].key, tmp);
	if (rd->pc && (strlen(rd->pc) >10)){
		nvram_set(wkey[MAC].key, rd->pc);
		nvram_set(wkey[WAN_MAC].key, rd->pc);
	}
	nvram_set(wkey[LCDSHOWD].key, "0");
	nvram_commit();
	printf("LCD wizard finished, rebooting ...\n");
        system("reboot");
#endif
	return 0;
}

int wizard_clear(struct _record *rd)
{
	memset(rd->pc, '\0', sizeof(rd->pc));
	memset(rd->wan_type, '\0', sizeof(rd->wan_type));
	memset(rd->username, '\0', sizeof(rd->username));
	memset(rd->password, '\0', sizeof(rd->password));
	memset(rd->wlan0_ssid, '\0', sizeof(rd->wlan0_ssid));
	memset(rd->wlan0_pwd, '\0', sizeof(rd->wlan0_pwd));
	memset(rd->wlan1_ssid, '\0', sizeof(rd->wlan0_ssid));
	memset(rd->wlan1_pwd, '\0', sizeof(rd->wlan0_pwd));
	memset(rd->timezone, '\0', sizeof(rd->timezone));
	rd->optimize = 0;
	rd->tz_value = -128;
	rd->save_or_start = 0;
	return 0;
}

int wizard_init(struct _record *rd)
{
#ifndef PC
	strcpy(rd->pc,  nvram_safe_get(wkey[WAN_MAC].key));
	strcpy(rd->wan_type,  nvram_safe_get(wkey[WAN_TYPE].key));
	strcpy(rd->username,  nvram_safe_get(wkey[USERNAME].key));
	strcpy(rd->password,  nvram_safe_get(wkey[PASSWORD].key));
	strcpy(rd->wlan0_ssid,  nvram_safe_get(wkey[WLAN0_SSID].key));
	strcpy(rd->wlan0_pwd,  nvram_safe_get(wkey[WLAN0_PWD].key));
	strcpy(rd->wlan1_ssid,  nvram_safe_get(wkey[WLAN1_SSID].key));
	strcpy(rd->wlan1_pwd,  nvram_safe_get(wkey[WLAN1_PWD].key));
	rd->optimize = 0;
	rd->tz_value = atoi(nvram_safe_get(wkey[TIMEZONE].key));
	rd->save_or_start = 0;
#endif
	return 0;
}
