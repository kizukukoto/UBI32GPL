#include <stdio.h>
#include <string.h>

#include "global.h"
#include "misc.h"
#include "image.h"
#include "menu.h"
#include "label.h"
#include "mouse.h"

#include "select.h"
#include "wizard.h"

static struct _time_zone *tz, _timezone[] = {
	{"(GMT-12:00) Eniwetok, Kwajalein", -192},
	{"(GMT-11:00) Midway Island, Samoa", -176},
	{"(GMT-10:00) Hawaii", -160},
	{"(GMT-09:00) Alaska", -144},
	//{"(GMT-08:00) Pacific Time (US/Canada), Tijuana", -128},
	{"(GMT-08:00) Pacific Time, Tijuana", -128},
	{"(GMT-07:00) Arizona",-112},
	//{"(GMT-07:00) Mountain Time (US/Canada)", -112},
	{"(GMT-07:00) Mountain Time", -112},
	{"(GMT-06:00) Central America", -96},
	//{"(GMT-06:00) Central Time (US/Canada)", -96},
	{"(GMT-06:00) Central Time", -96},
	{"(GMT-06:00) Mexico City", -96},
	{"(GMT-06:00) Saskatchewan", -96},
	{"(GMT-05:00) Bogota, Lima, Quito", -80},
	//{"(GMT-05:00) Eastern Time (US/Canada)", -80},
	{"(GMT-05:00) Eastern Time", -80},
	//{"(GMT-05:00) Indiana (East)", -64},
	{"(GMT-05:00) Indiana", -64},
	//{"(GMT-04:00) Atlantic Time (Canada)", -64},
	{"(GMT-04:00) Atlantic Time", -64},
	{"(GMT-04:00) Caracas, La Paz", -64},
	{"(GMT-04:00) Santiago", -64},
	{"(GMT-03:30) Newfoundland", -48},
	{"(GMT-03:00) Brazilia", -48},
	{"(GMT-03:00) Buenos Aires, Georgetown", -48},
	{"(GMT-03:00) Greenland", -48},
	{"(GMT-02:00) Mid-Atlantic", -32},
	{"(GMT-01:00) Azores", -16},
	{"(GMT-01:00) Cape Verde Is.", -16},
	{"(GMT) Casablanca, Monrovia", 0},
	{"(GMT) Greenwich Time: Dublin, Edinburgh, Lisbon, London", 0},
	{"(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm", 16},
	{"(GMT+01:00) Belgrade, Brastislava, Ljubljana", 16},
	{"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris", 16},
	{"(GMT+01:00) Sarajevo, Skopje, Sofija, Vilnus, Zagreb", 16},
	{"(GMT+01:00) Budapest, Vienna, Prague, Warsaw", 16},
	{"(GMT+01:00) West Central Africa", 16},
	{"(GMT+02:00) Athens, Istanbul, Minsk", 32},
	{"(GMT+02:00) Bucharest", 32},
	{"(GMT+02:00) Cairo", 32},
	{"(GMT+02:00) Harare, Pretoria", 32},
	{"(GMT+02:00) Helsinki, Riga, Tallinn", 32},
	{"(GMT+02:00) Jerusalem", 32},
	{"(GMT+03:00) Baghdad", 48},
	{"(GMT+03:00) Kuwait, Riyadh", 48},
	{"(GMT+03:00) Moscow, St. Petersburg, Volgograd", 48},
	{"(GMT+03:00) Nairobi", 48},
	{"(GMT+03:00) Tehran", 48},
	{"(GMT+04:00) Abu Dhabi, Muscat", 64},
	{"(GMT+04:00) Baku, Tbilisi, Yerevan", 64},
	{"(GMT+04:30) Kabul", 72},
	{"(GMT+05:00) Ekaterinburg", 80},
	{"(GMT+05:00) Islamabad, Karachi, Tashkent", 80},
	{"(GMT+05:30) Calcutta, Chennai, Mumbai, New Delhi", 88},
	{"(GMT+05:45) Kathmandu", 92},
	{"(GMT+06:00) Almaty, Novosibirsk", 96},
	{"(GMT+06:00) Astana, Dhaka", 96},
	{"(GMT+06:00) Sri Jayawardenepura", 96},
	{"(GMT+06:30) Rangoon", 104},
	{"(GMT+07:00) Bangkok, Hanoi, Jakarta", 112},
	{"(GMT+07:00) Krasnoyarsk", 112},
	{"(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi", 128},
	{"(GMT+08:00) Irkutsk, Ulaan Bataar", 128},
	{"(GMT+08:00) Kuala Lumpur, Singapore", 128},
	{"(GMT+08:00) Perth", 128},
	{"(GMT+08:00) Taipei", 128},
	{"(GMT+09:00) Osaka, Sapporo, Tokyo", 144},
	{"(GMT+09:00) Seoul", 144},
	{"(GMT+09:00) Yakutsk", 144},
	{"(GMT+09:30) Adelaide", 152},
	{"(GMT+09:30) Darwin", 152},
	{"(GMT+10:00) Brisbane", 160},
	{"(GMT+10:00) Canberra, Melbourne, Sydney", 160},
	{"(GMT+10:00) Guam, Port Moresby", 160},
	{"(GMT+10:00) Hobart", 160},
	{"(GMT+10:00) Vladivostok", 160},
	{"(GMT+11:00) Magadan, Solomon Is., New Caledonia", 176},
	{"(GMT+12:00) Auckland, Wellington", 192},
	{"(GMT+12:00) Fiji, Kamchatka, Marshall Is.", 192},
	{"(GMT+13:00) Nuku'alofa, Tonga", 208},
	{NULL}
};

/* F */
#ifdef SELECT_UI
void wizard_Time_Zone(LiteWindow *win, struct __timezone *kboard, LiteTextLine **text, char *opt[])
#else
void wizard_Time_Zone(LiteWindow *win, struct __timezone *kboard, ComponentList **list)
#endif
{
        int i = 0;
	tz = _timezone;
        DFBColor color = {0xFF, 0xFF, 0xFF, 0xFF};
#ifdef SELECT_UI
	int opt_i = 0;
	LiteTextButton *btn;
	LiteTextButtonTheme *textButtonTheme;
	DFBRectangle text_rect = { 140, 95, 160, 20 };
	DFBRectangle btn_rect = { 300, 95, 20, 20 };
#endif

        struct wizard {
                DFBRectangle rect;
                char txt[256];
                DFBColor def_color;
                int font_size;
        } wizard[] = {
                {{30, 20, 270, 20}, "Select your Time Zone", color, 14},
                {{50, 60, 270, 20}, "Please select the appropriate time zone ", color, 12},
                {{50, 80, 270, 20}, "for your location.", color, 12},
		{{50, 100, 270, 20}, "Time Zone : ", color, 12},
		{{130, 100, 180, 20}, "", color, 12},
		{{10, 120, 300, 60}, "", color, 12}
        };

	DFBRectangle img_rect[] = {
                { 122, 190,  76,  55},
                { 206, 190,  76,  55}
        };

        image_create(win, img_rect[0], "wizard_cancel.png", 0);
        image_create(win, img_rect[1], "wizard_next.png", 0);

        for ( i = 0; i < 4; i++)
                label_create(win, wizard[i].rect, NULL,
                                wizard[i].txt, wizard[i].font_size, wizard[i].def_color);
#ifdef SELECT_UI
	lite_new_textline(LITE_BOX(win), &text_rect, liteNoTextLineTheme, text);
	lite_new_text_button_theme(IMAGE_DIR"textbuttonbgnd.png", &textButtonTheme);
	lite_new_text_button(LITE_BOX(win), &btn_rect, "Yes", textButtonTheme, &btn);

	for (; tz->txt; tz++) {
		printf("%s(%d) tz->txt(%s) opt_i(%d)\n", __FUNCTION__, __LINE__, tz->txt, opt_i);
		fflush(stdout);
		opt[opt_i++] = strdup(tz->txt);
		if (opt_i == 1)
			lite_set_textline_text(*text, tz->txt);
	}
	opt[opt_i] = NULL;
#else
	*list = select_create(win, wizard[4].rect, wizard[5].rect,
				 "textbuttonbgnd.png", "scrollbar.png", color, false);
	select_set_button_text(*list, "V");
	set_mouse_event_passthrough(DFB_OK);
	for (;tz->txt != NULL; tz++) {
		select_add_item(*list, tz->txt);
	}
#ifndef PC
	tz = _timezone; i = 0;
	
	for (;tz->txt != NULL; tz++, i++)
		if (kboard->tz_value == tz->value) {
			break;
		}
	
	select_set_default_item(*list, i);
#else
	select_set_default_item(*list, 0);
#endif
#endif
	kboard->tzone = _timezone;
}
