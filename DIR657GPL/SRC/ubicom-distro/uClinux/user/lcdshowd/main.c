#include "global.h"
#include "misc.h"
#include "image.h"
#include "menu.h"
#include "calibrate.h"
#include "check.h"
#include <getopt.h>

int layer_width, layer_height;
IDirectFBSurface *main_surface;
IDirectFBSurface *main_background_surface;

extern void setting_start(menu_t *parent_menu);
extern void status_start(menu_t *parent_menu);
extern void wps_start(menu_t *parent_menu);
extern void statistics_start(menu_t *parent_menu);
extern void wizard_start();

/* The menu_t cannot less than 7 items, otherwise assign a NULL */
static menu_t main_menu = {
	.name = "main",
	.bg = "background.png",
	.items = (items_t []) {
		{ "settings.png",	"Settings",	setting_start,		NULL },
		{ "status.png",		"Status",	status_start,		NULL },
		{ "statistics.png",	"Statistics",	statistics_start,	NULL },
		{ "wps.png",		"WPS",		wps_start,		NULL },
		{ NULL },
		{ NULL },
		{ NULL }
	},
	.idx = 1,
	.count = 4,
};

void __attribute__((noreturn)) show_usage(void){
    printf(
"\n"
"   Usage: lcdshowd [OPTIONS]\n\n"
"     -m, --move_gap=GAPLENGTH         Set sliding move gap distance (default: %d)\n"
"     -c, --enable_cursor              Set cursor visable \n"
"     -t, --power_saving=secs          Set Power Saving Time (secs, default: %d)\n"
"     -h, --help                       Show help messages\n"
,HORIZON_MOVE_GAP
,LCDPOWER_SAVING_TIME
    );
    exit(0);
}

static void save_pid(void){
	FILE *fp;
	char buf[8] = {'\0'};
	if((fp = fopen(LCDSHOWD_PID,"w"))!=NULL){
		fprintf(fp,"%d\n",getpid());
		fclose(fp);
	}
	return ;
}

int main(int argc, char *argv[])
{
	int c, cursor_show=0;
	LiteWindow *main_window;
	LiteWindow *background_window;
	DFBRectangle main_rect;

	check_mac("xxx");
/*
	struct option {
		const char *name;
		int has_arg;
		int *flag;
		int val;
	};
	has_tag:
	"no_argument" (or 0) if the option does not take an argument,
	"required_argument" (or 1) if the option requires an argument,  or
	"optional_argument"  (or  2) if the option takes an optional argu-
	ment.
*/
	static const struct option arg_options[] = {
		{"enable_cursor", no_argument, 0, 'c'},
		{"move_gap", required_argument, 0, 'm'},
		{"power_saving", required_argument, 0, 't'},
		{"help", no_argument, 0, 'h'},
		//here, required_argument is for --option_c to check if user input with parameter
		{0, 0, 0, 0}
	};

#ifndef PC
	save_pid();
	power_saving_time = atoi(nvram_safe_get("lcd_power_save_time"));
#endif
	if(power_saving_time < 0){
		power_saving_time = LCDPOWER_SAVING_TIME;
	}

	while(1){
		int option_index = 0;
		c = getopt_long(argc, argv, "cm:t:", arg_options, &option_index);
		//here, c: is for -c to check if user input with parameter
		if(c==-1)
			break;
		switch (c){
			case 'c':
				cursor_show = 1;
				break;

			case 'm':
				moving_gap = atoi(optarg);
				if(moving_gap > HORIZON_MOVE_BOUNDARY){
					printf("lcdshowd: move_gap \"%d\" is bigger than limit %d, using default (%d)\n"
						,moving_gap,HORIZON_MOVE_BOUNDARY,HORIZON_MOVE_GAP);
					moving_gap = HORIZON_MOVE_GAP;
				}
				break;
			case 't':
				power_saving_time = atoi(optarg);
				if(power_saving_time < 0){
					printf("lcdshowd: power saving time \"%s\" is invalid, using default (%d)\n"
							,optarg,LCDPOWER_SAVING_TIME);
					power_saving_time = LCDPOWER_SAVING_TIME;
				}
				printf("lcdshowd: power saving time set to \"%ld\" secs \n",power_saving_time);
				break;
			case 'h':
				show_usage();
				break;
			default:
				break;
		}
	}

	if(cursor_show == 0){
		setenv ("LITE_NO_CURSOR", "1", 1);
	}

	LITE_INIT_CHECK(argc, argv);
	screen_init(&layer_width, &layer_height);

#ifndef PC
	/*	adjust calibration 
		On behalf of Ubicom's clark
		the code can refer to http://www.embedded.com/story/OEG20020529S0046 
	*/
	if(strncmp(nvram_safe_get("lcd_need_calibrate"),"1",1) == 0){
		// we need calibrated
		if(get_lcd_power_status() == LCDPOWER_OFF){
			switch_lcd_power(LCDPOWER_ON);
		}
		ts_calibration(main_surface);
		nvram_set("lcd_need_calibrate","0");
		nvram_commit();
	}

	main_rect = (DFBRectangle) { 0, 0, layer_width, layer_height };
	lite_new_window(NULL, &main_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &background_window);
	image_create(LITE_WINDOW(background_window), main_rect, "background_pure.png", 0);
	lite_set_window_opacity(background_window, 0xFF);
	if (strcmp(nvram_safe_get("lcd_need_run_wizard"), "1") == 0) {
		wizard_start();
	}
#endif


	main_rect = (DFBRectangle) { 0, 0, layer_width, layer_height };
	lite_new_window(NULL, &main_rect, DWCAPS_ALPHACHANNEL, NULL, NULL, &main_window);

#ifdef PC
	/* default in x86 debugging, we show the cursor */
	cursor_show = 1;
// 	if (strcmp(getenv("TERM"), "xterm")){
// 		lite_layer->SetCursorOpacity (lite_layer, 0);
// 	}
#endif
	if(cursor_show == 0){
		// let cursor tansparent
		lite_layer->SetCursorOpacity (lite_layer, 0);
	}

	last_action_time = time(NULL);
	init_power_save_timer();

	menu_loop(main_window, &main_menu);

	LITE_CLOSE();
	return 0;
}
