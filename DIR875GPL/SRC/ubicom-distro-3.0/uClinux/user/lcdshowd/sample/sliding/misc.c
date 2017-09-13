#include <sys/sysinfo.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#include "global.h"
#include "misc.h"
#include "label.h"

#if 1
//#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

time_t power_saving_time = LCDPOWER_SAVING_TIME; //misc.h
time_t last_action_time;
struct itimerval power_saving_value ;

DFBResult timer_start(void *data)
{
	timer_option *opt = (timer_option *)data;

	if (opt->func)
		opt->func(opt->data);

	return lite_enqueue_window_timeout(opt->timeout, (LiteTimeoutFunc)timer_start, data, &opt->timeout_id);
}

DFBResult timer_stop(timer_option *opt)
{
	return lite_remove_window_timeout(opt->timeout_id);
}

void window_enable(LiteWindow *win, int enabled)
{
	lite_layer->EnableCursor(lite_layer, enabled);
	lite_set_window_enabled(win, enabled);
}

void cursor_enable(int enabled)
{
	lite_layer->SetCursorOpacity (lite_layer, enabled);
}

DFBResult screen_init(int *layer_width, int *layer_height)
{
	lite_layer->SetCooperativeLevel(lite_layer, DLSCL_ADMINISTRATIVE); //DLSCL_SHARED?
	lite_get_layer_size(layer_width, layer_height);

	lite_layer->GetSurface(lite_layer, &main_surface);
	main_surface->SetBlittingFlags(main_surface, DSBLIT_BLEND_ALPHACHANNEL);

	return DFB_OK;
}

void get_sys_uptime(int *day, int *hour, int *min, int *sec){
	/*
	1 day = 86400 sec
	1 hour = 3600 sec
	*/
	struct sysinfo info;
	long sec_ori = 0;
	const long _minute = 60;
	const long _hour = _minute * 60;
	const long _day = _hour * 24;

	*day = 0;
	*hour = 0;
	*min = 0;
	*sec = 0;

	sysinfo(&info);
	sec_ori = info.uptime;

	*day = sec_ori / _day;

	*hour = (sec_ori % _day) / _hour;
	*min = (sec_ori % _hour) / _minute ;
	*sec = (sec_ori % _minute);

}

unsigned long uptime(void){
	struct sysinfo info;
	sysinfo(&info);
	return info.uptime;
}

int get_luminance(void){
	FILE *fp = NULL;
	char buf[32] = {'\0'};
	int value = 0;
	// /sys/class/backlight/ubicom32bl/brightness
	if((fp = fopen (BRIGHTNESS_FILE, "r"))!=NULL){
		fgets(buf,sizeof(buf),fp);
		fclose(fp);
		value = atoi(buf);
	}else{
		return -1;
	}
	return value;
}

int adjust_luminance(int adjust_value){
	//BRIGHTNESS_FILE
	int tmp_value = 0;
	char cmd[128] = {'\0'};
#ifdef PC
	FILE *fp;
	if((fp = fopen(BRIGHTNESS_FILE,"w"))!=NULL){
		fprintf(fp,"%d\n",adjust_value);
		fclose(fp);
	}
	return 1;
#endif
	tmp_value = adjust_value;
	if(tmp_value > BRIGHTNESS_MAX){
		tmp_value = BRIGHTNESS_MAX;
	}
	if(tmp_value < BRIGHTNESS_MIN){
		tmp_value = BRIGHTNESS_MIN;
	}

	sprintf(cmd,"echo %d > %s",tmp_value,BRIGHTNESS_FILE);
	system(cmd);
	return 1;
}


int get_lcd_power_status(void){
	FILE *fp = NULL;
	char buf[32] = {'\0'};
	int value = 0;
	// /sys/class/backlight/ubicom32bl/bl_power
#ifdef PC
	return 1;
#endif
	if((fp = fopen (LCDPOWER_FILE, "r"))!=NULL){
		fgets(buf,sizeof(buf),fp);
		fclose(fp);
		value = atoi(buf);
	}else{
		return -1;
	}
	return value;
}


int switch_lcd_power(int action){
	int cur_value = 0;
	char cmd[128] = {'\0'};

#ifdef PC
	return 1;
#endif
	cur_value = get_lcd_power_status();

	if( cur_value == -1 ){
		return 0;
	}

	if(cur_value == action){
		return 1;
	}else{
		sprintf(cmd,"echo %d > %s",action,LCDPOWER_FILE);
		system(cmd);
	}

	return 1;
}

void lcdpower_check(int signo){
	time_t cur_time;
	cur_time = time(NULL);
	time_t diff_time;
	diff_time = cur_time - last_action_time;
	if(diff_time > 1259000000){
		printf("lcdshowd: Abnormal time check for activation (%d secs), ignore this time.\n", (int)diff_time);
		return ;
	}
	if(diff_time > power_saving_time){
		// disable lcd backlight
		if(get_lcd_power_status() == LCDPOWER_ON){
			DEBUG_MSG("lcdshowd: No activation in last %ld secs, turn lcd backlight off \n",diff_time);
			switch_lcd_power(LCDPOWER_OFF);
		}
	}else{
		// enable lcd backlight
		if(get_lcd_power_status() == LCDPOWER_OFF){
			DEBUG_MSG("lcdshowd: turn lcd backlight on \n");
			switch_lcd_power(LCDPOWER_ON);
		}
	}
//	setitimer(ITIMER_REAL, &value, NULL);
//	signal(SIGALRM,sig_lcdpower);
	return ;
}

/*
TODO:
	We may directly use menu_clock_timer to handle the lcd power check and switch
	shortage for menu_clock_timer:
		1. check the lcd status every 1 secs
		2. boundled with menu_clock_timer
	advantage for menu_clock_timer:
		1. only maintain 1 timer
		2. resource saving ?
*/

int init_power_save_timer(void){
	DEBUG_MSG("lcdshowd: init_power_save_timer, check interval = %d secs\n",LCDPOWER_CHECK_INTERVAL);
	signal(SIGALRM,lcdpower_check);
	//printf("%d \n",time(NULL));
	last_action_time = time(NULL);
	power_saving_value.it_value.tv_sec = LCDPOWER_CHECK_INTERVAL;
	power_saving_value.it_value.tv_usec = 0;
	power_saving_value.it_interval.tv_sec = LCDPOWER_CHECK_INTERVAL;
	power_saving_value.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &power_saving_value, NULL);

	return 1;
}

int adjust_power_save_time(int new_value){
	power_saving_time = new_value;
	return 1;
}



