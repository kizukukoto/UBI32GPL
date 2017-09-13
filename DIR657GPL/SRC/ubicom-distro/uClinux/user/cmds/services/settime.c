#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"

//#define USED_EAVL 1

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static char *get_timezone(char *value)
{
	typedef struct {
		char *val;
		char *tz;
	} zone_t;
	zone_t *pt;

	char tmp_zone[32], *token;
	
	static zone_t zone[] = {			              	
		{"-192", "GMT+12"},//GMT-12
		{"-176", "GMT+11"},//GMT-11
		{"-160", "GMT+10"},//GMT-10
		{"-144", "GMT+9"},//GMT-9
		{"-128", "GMT+8"},//GMT-8
		{"-112", "GMT+7"},//GMT-7	
		{"-96", "GMT+6"},//GMT-6
		{"-80", "GMT+5"},//GMT-5
		{"-64", "GMT+4"},//GMT-4	
		{"-48", "GMT+3"},//GMT-3
		{"-32", "GMT+2"},//GMT-2
		{"-16", "GMT+1"},//GMT-1
		{"0", "GMT"},//GMT	
		{"16", "GMT-1"},//GMT+1
		{"32", "GMT-2"},//GMT+2
		{"48", "GMT-3"},//GMT+3
		{"64", "GMT-4"},//GMT+4
		{"72", "GMT-4:30"},//GMT+4:30
		{"80", "GMT-5"},//GMT+5
		{"88", "GMT-5:30"},//GMT+5:30
		{"92", "GMT-5:45"},//GMT+5:45
		{"96", "GMT-6"},//GMT+6
		{"104", "GMT-6:30"},//GMT+6:30	
		{"112", "GMT-7"},//GMT+7
		{"128", "GMT-8"},//GMT+8
		{"144", "GMT-9"},//GMT+9
		{"152", "GMT-9:30"},//GMT+9:30
		{"160", "GMT-10"},//GMT+10
		{"176", "GMT-11"},//GMT+11
		{"192", "GMT-12"},//GMT+12
		{"208", "GMT-13"},//GMT+13
		{NULL, NULL}
	};

	strcpy(tmp_zone, value);
	token = strtok(tmp_zone, "_");

	for(pt = zone; pt->val != NULL; pt++) {
		if(!strcmp(pt->val, tmp_zone))
			return pt->tz;
	}
	return NULL;
}

/* Get the timezone from NVRAM and set the timezone in the kernel
 * and export the TZ variable 
 */
void set_timezone(void)
{
	int time_zone;
	char *zone;
	char tmp[32];	
	
	DEBUG_MSG("set_timezone:\n");
	zone = get_timezone(nvram_safe_get("sel_time_zone"));
	DEBUG_MSG("set_timezone:zone=%s\n",zone);
	
	if(NVRAM_MATCH("daylight_en", "1")) {
		DEBUG_MSG("set_timezone:daylight_en\n");
#if 1
		sprintf(tmp, "%sPDT,M3.2.0/0,M10.5.0/23", zone);
#else
		sprintf(tmp, "%sPDT,M%d.%d.%d/%d,M%d.%d.%d/%d", zone,
			    atoi(nvram_safe_get("daylight_smonth")),
			    atoi(nvram_safe_get("daylight_sweek")),
				atoi(nvram_safe_get("daylight_sday")),
				atoi(nvram_safe_get("daylight_stime")),
				atoi(nvram_safe_get("daylight_emonth")),
				atoi(nvram_safe_get("daylight_eweek")),
				atoi(nvram_safe_get("daylight_eday")),
				atoi(nvram_safe_get("daylight_etime")));
#endif
		DEBUG_MSG("set_timezone:tmp=%s\n",tmp);	
		nvram_set("time_zone", tmp);
	} else {
		DEBUG_MSG("set_timezone:daylight_disen\n");		
		sprintf(tmp, "%s", zone);	
		DEBUG_MSG("set_timezone:tmp=%s\n",tmp);		
		nvram_set("time_zone", tmp);
	}	
	
	// set timezone to kernel
#if 1		
	time_zone = atoi(nvram_safe_get("sel_time_zone"));
	DEBUG_MSG("set_timezone:time_zone=%d\n",time_zone);
	adjust_timezone(time_zone);
#endif		
	
	// Export TZ variable for the time libraries to use.
#if USED_EAVL
	setenv("TZ",nvram_get("time_zone"),1);
#else	
	sprintf(tmp, "export TZ=%s", nvram_safe_get("time_zone"));
	DEBUG_MSG("set_timezone:TZ tmp=%s\n",tmp);
	system(tmp);
#endif	
	
}

#define DAYLIGHT_S "/proc/sys/net/ipv4/netfilter/datelight_start"
#define DAYLIGHT_E "/proc/sys/net/ipv4/netfilter/datelight_end"

int do_setdaylight(void)
{
	int s_month, s_week, s_day, s_time;
	int e_month, e_week, e_day, e_time;
	FILE *fp;
	int sum;

	if (NVRAM_MATCH("daylight_en", "1")) {
		s_month = atoi(nvram_safe_get("daylight_smonth"));
		s_week = atoi(nvram_safe_get("daylight_sweek"));
		s_day = atoi(nvram_safe_get("daylight_sday"));
		s_time = atoi(nvram_safe_get("daylight_stime"));

		/*set for daylight saving*/
		if ((fp = fopen(DAYLIGHT_S, "w")) == NULL) {
			perror("fopen daylight_start");
			return -1;
		} else {
			sum = s_time + s_day * 24 + s_week * 24 * 7 +
			       	s_month * 24 * 7 * 5;
			fprintf(fp, "%d", sum);
			fclose(fp);
		}

		e_month = atoi(nvram_safe_get("daylight_emonth"));
		e_week = atoi(nvram_safe_get("daylight_eweek"));
		e_day = atoi(nvram_safe_get("daylight_eday"));
		e_time = atoi(nvram_safe_get("daylight_etime"));

		if ((fp = fopen(DAYLIGHT_E, "w")) == NULL) {
			perror("fopen daylight_end");
			return -1;
		} else {
			sum = e_time + e_day * 24 + e_week * 24 * 7 +
			       	e_month * 24 * 7 * 5;
			fprintf(fp, "%d", sum);
			fclose(fp);
		}
	}
	return 0;
}

int adjust_timezone(int time_zone)
{
	struct timeval tv;
    struct timezone tz;	
	
	DEBUG_MSG("(adjust_timezone)time_zone=%d\n", time_zone);	    
	gettimeofday(&tv,&tz);  	
	tv.tv_sec = tv.tv_sec+ (time_zone/8)*1800;   	
	settimeofday(&tv,&tz);
}

int do_settime(void)
{
	char *system_time, *tok;
	char tmp[64], data[7][5];
	int i = 0;

	DEBUG_MSG("do_settime:\n");
	/*set user define time*/
	nvram_set("time_zone", "GMT");
	system_time = strcpy(tmp, nvram_safe_get("system_time"));
	while((tok = strtok(system_time, "/")) != NULL) {
		sprintf(data[i], "%s", tok);
		i++;
		system_time = NULL;
	}
#if USED_EAVL
	sprintf(tmp, "%s%s%s%s%s.%s", data[1], data[2], data[3],
			data[4], data[0], data[5]);
	DEBUG_MSG("do_settime:eval tmp=%s\n",tmp);
	eval("date", "-s", tmp);
#else		
	sprintf(tmp, "date -s %s%s%s%s%s.%s", data[1], data[2], data[3],
			data[4], data[0], data[5]);
	DEBUG_MSG("do_settime:tmp=%s\n",tmp);
	system(tmp);
#endif		
	
	return 0;
}
int set_timezone_main(int argc, char *argv[])
{
	set_timezone();
	return 0;
}
