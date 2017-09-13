#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"

//#define USED_EAVL 1

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif
static int daylight_saving()
{
	FILE *fp;
#if 1	
	if((fp = fopen("/var/tmp/daylight_saving.conf", "w+")) != NULL)
	{
		DEBUG_MSG("ntp_start_main: open /var/tmp/daylight_saving.conf\n");
		fprintf(fp, "daylight_saving_enable=%s\n", nvram_safe_get("daylight_en"));
		if (NVRAM_MATCH("daylight_en", "1")  )
		{
			fprintf(fp, "start_month=%s\n", nvram_safe_get("daylight_smonth"));
			fprintf(fp, "start_time=%s\n", nvram_safe_get("daylight_stime"));
			fprintf(fp, "start_day_of_week=%s\n", nvram_safe_get("daylight_sday"));
			fprintf(fp, "start_week=%s\n", nvram_safe_get("daylight_sweek"));
			fprintf(fp, "end_month=%s\n", nvram_safe_get("daylight_emonth"));
			fprintf(fp, "end_time=%s\n", nvram_safe_get("daylight_etime"));
			fprintf(fp, "end_day_of_week=%s\n", nvram_safe_get("daylight_eday"));
			fprintf(fp, "end_week=%s\n", nvram_safe_get("daylight_eweek"));	
		}			
		fclose(fp);
	}
	else{
		DEBUG_MSG("ntp_start_main: open /var/tmp/daylight_saving.conf fail!\n");
	}		
#endif		
}
static int ntp_start_main(int argc, char *argv[])
{
	int ret = -1, zone=-128;
	FILE *fp;	
	char *pos, buf[512]={0};	
	char time_zone[128], *servers, *interval;

	if (!NVRAM_MATCH("ntp_client_enable", "1"))
		goto out;
	
	servers = nvram_safe_get("ntp_server");
	interval = nvram_safe_get("timer_interval");
	strcpy(time_zone, nvram_safe_get("sel_time_zone"));
	
	if (strlen(servers) == 0 || strlen(interval) == 0 ||
		strlen(time_zone) == 0)
	{
		err_msg("miss paramaters server:%s, interval:%s, time_zone: %s\n",
				servers, interval, time_zone);
		goto out;
	}

	pos = strchr(time_zone, '_');
	if(pos)
		*pos = '\0';
	debug("ntp_start_main: time_zone=%s\n", time_zone);	
	/* set time */
	do_settime();	//XXX: why
	debug("ntp_start_main: set time by ntp_client\n");
	/* ntpclient */
	daylight_saving();
	
#if USED_EAVL									
	DEBUG_MSG("ntp_start_main: eval servers=%s,time_zone=%s\n",servers ,time_zone);
	if(interval){				 
		DEBUG_MSG("ntp_start_main: interval=%s\n",interval);
		ret = eval("ntpclient",  /* "-d", debug message */
				   "-h", servers, "-s", "-c", "2", "-i", interval,
				   "-D", "-t", time_zone, "-d");				
	}
	else{
		ret = eval("ntpclient" /* "-d", debug message */
				   "-h", servers, "-s", "-c", "2", "-i", "4",
				   "-D", "-t", time_zone, "-d");	
	}					
#else			
	sprintf(buf,"/sbin/ntpclient -h %s -s -c 2 -i %s -D -t %s &",
			servers, interval ,time_zone);
	debug("cmds: %s", buf);
	system(buf);
#endif
	ret = 0;	
out:
	rd(ret, "disabled or error");	
	return ret;
}

static int ntp_stop_main(int argc, char *argv[])
{	
	int ret = -1;
#if USED_EAVL	
	ret = eval("killall", "ntpclient");
#else	
	system("killall ntpclient &");
	ret = 0;
#endif	
	DEBUG_MSG("ntp_stop_main:done\n");
	return ret;
}

/*
 * operation policy:
 * start: evel ntp with interfces
 * */
int ntp_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", ntp_start_main},
		{ "stop", ntp_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}

static int manual_time_start_main(int argc, char *argv[])
{
	printf("manural time\n");
	if (NVRAM_MATCH("ntp_client_enable", "0"))
		do_settime();
	return 0;
}

static int manual_time_stop_main(int argc, char *argv[])
{
	return 0;
}

int manual_time_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", manual_time_start_main},
		{ "stop", manual_time_stop_main},
		{ NULL, NULL}
	};
	printf("XXXXXXXXXXXX\n");
	return manual_time_start_main(argc, argv);
	//return lookup_subcmd_then_callout(argc, argv, cmds);
}
