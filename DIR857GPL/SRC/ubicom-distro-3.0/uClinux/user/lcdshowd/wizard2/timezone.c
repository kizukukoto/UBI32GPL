#include <lite/window.h>
#include <leck/image.h>
#include <leck/textbutton.h>
#include <leck/label.h>

#include "global.h"
#include "mouse.h"
#include "image.h"
#include "label.h"
#include "input.h"
#include "wizard.h"

static int opt_i = 0;
static char *opt[256];
static struct __timezone {
	const char *desc;
	int id;
} *tzp, timezone_data[] = {
	{"(GMT-12:00) Eniwetok, Kwajalein", -192},
	{"(GMT-11:00) Midway Island, Samoa", -176},
	{"(GMT-10:00) Hawaii", -160},
	{"(GMT-09:00) Alaska", -144},
	{"(GMT-08:00) Pacific Time, Tijuana", -128},
	{"(GMT-07:00) Arizona",-112},
	{"(GMT-07:00) Mountain Time", -112},
	{"(GMT-06:00) Central America", -96},
	{"(GMT-06:00) Central Time", -96},
	{"(GMT-06:00) Mexico City", -96},
	{"(GMT-06:00) Saskatchewan", -96},
	{"(GMT-05:00) Bogota, Lima, Quito", -80},
	{"(GMT-05:00) Eastern Time", -80},
	{"(GMT-05:00) Indiana", -64},
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
	{ NULL }
};

int wz_timezone_ui(LiteWindow *win, wizard_page **p)
{
	int opt_i = 0, i = 0;
	LiteTextButton *btn;
	LiteTextButtonTheme *textButtonTheme;
	DFBRectangle text_rect = { 140, 95, 160, 20 };
	DFBRectangle btn_rect = { 300, 95, 20, 20 };
	struct {
		DFBRectangle rect;
		const char *txt;
		DFBColor def_color;
		int font_size;
	} component_list[] = {
		{{  30,  20, 270, 20 }, "Select your Time Zone", COLOR_WHITE, 14 },
		{{  50,  60, 270, 20 }, "Please select the appropriate time zone ", COLOR_WHITE, 12 },
		{{  50,  80, 270, 20 }, "for your location.", COLOR_WHITE, 12 },
		{{  50, 100, 270, 20 }, "Time Zone : ", COLOR_WHITE, 12 },
		{{ 130, 100, 180, 20 }, "", COLOR_WHITE, 12 },
		{{  10, 120, 300, 60 }, "", COLOR_WHITE, 12 }
	};

	DFBRectangle img_rect[] = {
		{ 122, 190, 76, 55},
		{ 206, 190, 76, 55}
	};

	image_create(win, img_rect[0], "wizard_cancel.png", 0);
	image_create(win, img_rect[1], "wizard_next.png", 0);

	for (; i < 4; i++)
		label_create(win,
			component_list[i].rect,
			NULL,
			component_list[i].txt,
			component_list[i].font_size,
			component_list[i].def_color);

	lite_new_textline(LITE_BOX(win), &text_rect, liteNoTextLineTheme, (LiteTextLine **)&(*p)->param);
	lite_new_text_button_theme(IMAGE_DIR"textbuttonbgnd.png", &textButtonTheme);
	lite_new_text_button(LITE_BOX(win), &btn_rect, "Yes", textButtonTheme, &btn);

	opt_i = 0;
#if 1
	for (tzp = timezone_data; tzp && tzp->desc; tzp++) {
		opt[opt_i++] = strdup(tzp->desc);
		if (opt_i == 1)
			lite_set_textline_text((LiteTextLine *)(*p)->param, tzp->desc);
	}
	opt[opt_i] = NULL;
#endif
#ifndef LOAD_INTO_DDR
	lite_set_window_opacity(win, 0xFF);
#endif
	return 0;
}

int wz_timezone_evt(LiteWindow *win, wizard_page **p)
{
	int i, ret, opt_idx, x, y;
	DFBRegion btr[] = {
		{ 122, 190, 206, 218 }, /* Cancle */
		{ 216, 190, 292, 218 }, /* Next */
		{ 140,  95, 300, 115 }  /* TextLine */
	};

	while (true) {
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			opt_idx = bt_ripple_handler(x, y, btr, 4);

			if (opt_idx == -1)
				continue;
			else if (opt_idx == 2) {
				select_input(opt, "Time Zone:", (LiteTextLine *)(*p)->param, EDIT_DISABLE);
				continue;
			} else if (opt_idx == 0)
				ret = PAGE_CANCLE;
			else
				ret = PAGE_NEXT;
			break;
		}
	}

	if (ret == PAGE_NEXT) {
		char *tz_desc, tz_value[8];

		lite_get_textline_text((LiteTextLine *)(*p)->param, &tz_desc);
		for (tzp = timezone_data; tzp && tzp->desc; tzp++) {
			if (strcmp(tzp->desc, tz_desc) == 0) {
				sprintf(tz_value, "%d", tzp->id);
				wz_nvram_set("time_zone", tz_value);
				info_save("timezone", tz_desc);
				goto out;
			}
		}
	}
out:
	for (i = 0; i < opt_i; i++)
		free(opt[i]);
	opt_i = 0;
	opt[0] = NULL;

	return ret;
}

int wz_timezone_area_ui(LiteWindow *win, wizard_page **p)
{
	return 0;
}

int wz_timezone_area_evt(LiteWindow *win, wizard_page **p)
{
	return 0;
}
