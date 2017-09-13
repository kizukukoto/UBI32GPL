#ifndef __WIZARD_H__
#define __WIZARD_H__
#include <leck/progressbar.h>
#include <lite/font.h>
#include <leck/textline.h>
#include <leck/label.h>
#include <lite/window.h>

#define SELECT_UI

struct _record {
	char pc[128];
	char wan_type[64];
	char username[128];
	char password[128];
	char wlan0_ssid[128];
	char wlan0_pwd[128];
	char wlan1_ssid[128];
	char wlan1_pwd[128];
	int  optimize;
	char timezone[128];
	int  tz_value;
	int  save_or_start;
};

static struct _time_zone {
        char *txt;
        int value;
};

struct __timezone {
	struct _time_zone *tzone;
	int tz_value;
	char *tz;
};

struct wizard_wlan_struct {
	char *str;
	char text1[24];
	LiteLabel *title;
	LiteTextLine *txtline;
};

struct pppoe_struct {
	char *str;
	char text1[24];
	char text2[24];
	LiteLabel *title[2];
	LiteTextLine *txtline[2];
};

struct dhcp_struct {
	char *str;
	char client[1024];
	char text1[24];
	LiteLabel *title;
	LiteTextLine *txtline;
};

struct detect_struct {
	char text1[24];
	LiteProgressBar *progress;
	float degree;
};

#endif
