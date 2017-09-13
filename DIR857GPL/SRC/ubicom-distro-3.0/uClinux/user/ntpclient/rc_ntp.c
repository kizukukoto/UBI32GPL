#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <wait.h>
#include <sutil.h>
#include <shvar.h>

#ifdef NTP_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define RUNFILE "/var/run/ntptime.pid"
#define EXCEPTION_ARRAY_SIZE 2
#define ONE_HOUR	3600

#if 0
float timezone_daylight_savings_exception[EXCEPTION_ARRAY_SIZE] =
                {-82   /* timezone is indiana*/ ,
                 -112  /* timezone is Arizona */};

unsigned int year_month_days[2][13] = {
	{ 365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    { 366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

int adjust_timezone(int time_zone)
{
	struct timeval tv;
    struct timezone tz;
    
	DEBUG_MSG("time_zone=%d\n", time_zone);
    
    gettimeofday(&tv,&tz);  	
  	tv.tv_sec = tv.tv_sec+ (time_zone/8)*1800;   	
   	settimeofday(&tv,&tz);
   	return 0;
}

int read_option_from_file(time_t *start_time, time_t *end_time, int *time_zone)
{
    struct timeval tv;
    struct timezone tz;
    struct tm tm_start, tm_end;
    struct tm *p_start, *p_end, *p_cur;
	int start_sday_of_week, end_sday_of_week;
    int i;
	time_t time_start_mon_1st, time_end_mon_1st; // The 1st day of start/end month of daylight saving time
  	char wday_start_mon_1st, wday_end_mon_1st; // The weekday of the 1th day in both start/end DLS months
   	char days2target_start_day, days2target_end_day;
   	
	
	FILE *fp;
	char buf[24], *pos;
	char daylight_saving_enable[2]={0};
	int start_week = 0, end_week = 0;

	fp = fopen("/var/etc/daylight_saving.conf", "r");
	if (fp == NULL) 
	{
		printf("Could not open /var/etc/daylight_saving.conf\n");
		return 0;
	}
	
	while (fgets(buf, sizeof(buf), fp) != NULL ) 
	{
		pos = buf;
		while (*pos != '\0') {
			if (*pos == '\n') {
				*pos = '\0';
				break;
			}
			pos++;
		}
		pos = strchr(buf, '=');
		
		if (pos == NULL) 
		{
			printf("daylight_saving.conf: invalid line '%s'\n", buf);
			continue;
		}
		pos++;
		
		if(strstr(buf, "enable")) 
			strncpy(daylight_saving_enable, pos, strlen(pos));
		else if(strstr(buf, "zone"))
			*time_zone = atoi(pos);
		else if(strstr(buf, "start_month"))
			tm_start.tm_mon = atoi(pos) - 1;
		else if(strstr(buf, "start_time"))
			tm_start.tm_hour = atoi(pos);
		else if(strstr(buf, "start_day_of_week"))
			tm_start.tm_wday = atoi(pos) - 1 ;
		else if(strstr(buf, "start_week"))
			start_week = atoi(pos);
		else if(strstr(buf, "end_month"))
			tm_end.tm_mon = atoi(pos) - 1;
		else if(strstr(buf, "end_time"))
			tm_end.tm_hour = atoi(pos);
		else if(strstr(buf, "end_day_of_week"))
			tm_end.tm_wday = atoi(pos) - 1;
		else if(strstr(buf, "end_week"))
			end_week = atoi(pos);
	}
	
	fclose(fp);
	  	
   	gettimeofday(&tv,&tz);
  	
	// DL 
	p_cur = localtime(&tv.tv_sec);
	
	DEBUG_MSG("start day of week %d \n",tm_start.tm_wday);
	tm_start.tm_year = p_cur->tm_year;
	tm_start.tm_yday = 0;
	tm_start.tm_mday = 1;
	tm_start.tm_min  = 0;
	tm_start.tm_sec  = 0;
	start_sday_of_week = tm_start.tm_wday;
	for( i = 1; i < tm_start.tm_mon + 1; i++)
	{
        if((tm_start.tm_year % 4) == 0)
       		tm_start.tm_yday += year_month_days[1][i];
       	else
       		tm_start.tm_yday += year_month_days[0][i];	
	}

	time_start_mon_1st = mktime(&tm_start);    
   	p_start = gmtime(&time_start_mon_1st); // only want tm_wday
   	wday_start_mon_1st = p_start->tm_wday;
	
    days2target_start_day = ( (start_week - 1) * 7) - wday_start_mon_1st + start_sday_of_week;

   	tm_start.tm_mday += days2target_start_day; // Now we know what month day is
   	tm_start.tm_yday += days2target_start_day; 		
		
	// End Time
	tm_end.tm_year = p_cur->tm_year;
	tm_end.tm_yday = 0;
	tm_end.tm_mday = 1;
	tm_end.tm_min  = 0;
   	tm_end.tm_sec = 0;
	end_sday_of_week = tm_end.tm_wday;

	for( i = 1; i < tm_end.tm_mon + 1; i++)
	{
   		if((tm_end.tm_year % 4) == 0)
       		tm_end.tm_yday += year_month_days[1][i];
       	else
       		tm_end.tm_yday += year_month_days[0][i];	
	}

   	time_end_mon_1st = mktime(&tm_end);    
  	p_end = gmtime(&time_end_mon_1st); // only want tm_wday
	wday_end_mon_1st = p_end->tm_wday;   
 
	days2target_end_day = (( end_week - 1) * 7) - wday_end_mon_1st + end_sday_of_week;
	tm_end.tm_mday += days2target_end_day; // Now we know what month day is
	tm_end.tm_yday += days2target_end_day; 			

 	DEBUG_MSG("\n,start Year:%d,Month:%d,Day:%d,Hour:%d\n",tm_start.tm_year+1900,tm_start.tm_mon+1,tm_start.tm_mday,tm_start.tm_hour);	
 	DEBUG_MSG("\n,end Year:%d,Month:%d,Day:%d,Hour:%d\n",tm_end.tm_year+1900,tm_end.tm_mon+1,tm_end.tm_mday,tm_end.tm_hour);	
 	//syslog(LOG_INFO,"\n,Year:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	

 	// Get Secs 
	*start_time = mktime(&tm_start);                            
	*end_time = mktime(&tm_end); 
          	   
	return 0;
}


/*
 * 1) When we enter the daylight saving time, add one hour.
 * 2) When we leave the daylight saving time, subtract one hour.
 * return the flag that if we have added one hour or not.
 */
int adjust_daylight(int done,time_t start_time,time_t end_time,int time_zone)
{
	time_t cur_time;
	int i=0,check_flag=0;
	struct timeval tv;
    struct timezone tz;
    	
	gettimeofday(&tv,&tz);
	cur_time = tv.tv_sec;
	
	/*
	 * Since start_time and end_time are based on time which is not adjusted,
	 * we need to recover cur_time to be an unadjusted one.
	 */
	if(done)
		cur_time -= ONE_HOUR;
	
	
	/* Check if it is in daylight saving time now */  
    if (((start_time <= end_time) && ((cur_time >= start_time) && (cur_time < end_time))) ||
    	((start_time > end_time)  && ((cur_time >= start_time) || (cur_time < end_time))))
    {
    	/* Check if we have not add one hour*/
    	if(!done)
    	{
	        /* Check the timezone */
			for(i=0; i < EXCEPTION_ARRAY_SIZE;i++)
			{
				if(time_zone == timezone_daylight_savings_exception[i])
				{
	                check_flag = 1;
	                break;
	            }
	        }
	        if(check_flag == 0)
	        {
	        	/* Add one hour */
	        	DEBUG_MSG("Add one hour\n");
	    		tv.tv_sec += ONE_HOUR;
		    	settimeofday(&tv, &tz);
		    	
		    	/* Tell web server to get the adjusted time */
		    	kill(read_pid(HTTPD_PID), SIGUSR2);
		    	
		    	/* Record we have added one hour */
		    	done=1;
	    	}
	    }
    } 
    /* It's not in daylight saving time now */
    else
    {
    	/* We have added one hour*/
    	if(done)
    	{
	        /* Check the timezone */
			for(i=0; i < EXCEPTION_ARRAY_SIZE;i++)
			{
				if(time_zone == timezone_daylight_savings_exception[i])
				{
	                check_flag = 1;
	                break;
	            }
	        }
	        if(check_flag == 0)
	        {
	        	/* Subtract one hour */
	        	DEBUG_MSG("Subtract one hour\n");
	    		tv.tv_sec -= ONE_HOUR;
		    	settimeofday(&tv, &tz);
		    	
		    	/* Tell web server to get the adjusted time */
		    	kill(read_pid(HTTPD_PID), SIGUSR2);
		    	
		    	/* Record we have not add one hour */
		    	done=0;
	    	}
	    }
    }
   	return done;
}


/* Syn with ntp server, and wait until we reach XX o'clock */
int syn_ntp(int daylight_enable,int daylight_done,int *syn_remaining_time,int time_zone,time_t start_time,time_t end_time,char *wan_proto, char *wan_eth, char *ntp_server)
{
	int remaining_time_to_oclcok = 0;
	struct timeval tv;
	struct timezone tz;
	
	/* Syn time with ntp server */
	ntpclient_process(wan_proto, wan_eth, ntp_server);
	
	/* Adjust time zone */
	adjust_timezone(time_zone);
	
	/* Adjust day light */
	if(daylight_enable)
		daylight_done = adjust_daylight(daylight_done,start_time,end_time,time_zone);
	
	/* Wait until we reach XX o'clock */
	gettimeofday(&tv,&tz);
	remaining_time_to_oclcok = ONE_HOUR - (tv.tv_sec % ONE_HOUR);
	DEBUG_MSG("remaining_time_to_oclcok =%d\n",remaining_time_to_oclcok);	
	sleep(remaining_time_to_oclcok);
	
	/* renew remaining time for syn */
	*syn_remaining_time = *syn_remaining_time - remaining_time_to_oclcok;
	DEBUG_MSG("syn_remaining_time =%d\n",*syn_remaining_time);	
	
	return daylight_done;
}

int ntpclient_process(char *wan_proto, char *wan_eth, char *ntp_server)
{
	int wan_status = 1;
	char if_name[10];

	if( strcmp(wan_proto, "dhcpc")== 0 || strcmp(wan_proto, "static") ==0 )
		strcpy( if_name, wan_eth );
	else 
		strcpy( if_name, "ppp0" );

	if( (wan_status = wan_connected_cofirm(if_name)) ) // Don't execute during upgrading
	{ 
			//_system("ntpclient -h %s -s -i 5 -c 3 ", ntp_server);
			_system("ntpclient -h %s -s -i 4 -c 2 ", ntp_server);
			return 0;
		}

	DEBUG_MSG("ntp: nothing to do...\n");  
	return -1;
}

int stop_ntp(void)
{
	_system("killall","-9", "ntpclient");

	DEBUG_MSG("done\n");
	return 0;
}

void usage(void)
{
	printf("Usage: ntptime [-i interval in sec] [-s sleep time in sec ] [-p wan_proto] [-w wan_eth] [-n ntp_server]\n");
}

int main (int argc, char *argv[])
{
	int c = 0;
	int first_query = 0;
	char wan_proto[8]={0};
	char wan_eth[8]={0};
	char ntp_server[24]={0};
	
	time_t start_time, end_time;
	int time_zone = 0;
	int daylight_enable = 0;
	int daylight_done = 0;
	int time_interval = 0;
	int syn_remaining_time = 0;
	
/*	char *runfile = RUNFILE;
	FILE *pidfile;

	chdir("/");
	umask(022); // make pidfile 0644 

	// write pidfile _after_ forking ! 
	if (runfile && (pidfile = fopen(runfile, "w")))
	{
		DEBUG_MSG(pidfile, "%d\n", (int) getpid());
		fclose(pidfile);
	}

	umask(0);
*/
	for(;;){
		c = getopt(argc, argv, "i:p:w:n:fd");
		if ( c == EOF)break;
		switch(c)
		{
			case 'i':
				time_interval = atoi(optarg);
				break;
			case 'f':
				first_query = 1;
				break;
			case 'd':
				daylight_enable = 1;
				break;
			case 'p':
				strncpy(wan_proto, optarg, strlen(optarg));
				break;
			case 'w':
				strncpy(wan_eth, optarg, strlen(optarg));
				break;
			case 'n':
				strncpy(ntp_server, optarg, strlen(optarg));
				break;
			default:
				usage();
				exit(1);
		} 
	}

	read_option_from_file(&start_time, &end_time, &time_zone);
	
	/* Get remaining time for syn next time */
	syn_remaining_time= time_interval;
	DEBUG_MSG("At first, syn_remaining_time =%d\n",syn_remaining_time);
	
	/* Syn time with ntp server. Then sleep until we reach XX o'clock.*/
	daylight_done = syn_ntp(daylight_enable, 0 ,&syn_remaining_time,time_zone,start_time,end_time,wan_proto, wan_eth, ntp_server);
	
	for(;;) {
		/* Now it's XX o'clock*/
		if(daylight_enable)
		{
			daylight_done = adjust_daylight(daylight_done,start_time,end_time,time_zone);
			DEBUG_MSG("daylight_done=%d\n",daylight_done);
		}
		/* Check if we are near the time for syn */
		if(syn_remaining_time < ONE_HOUR)
		{
			/* Wait until we reach the time to syn */
			sleep(syn_remaining_time);
			
			/* Get remaining time for syn next time */
			syn_remaining_time= time_interval;
			DEBUG_MSG("After syn with ntp server, syn_remaining_time =%d\n",syn_remaining_time);
			
			/* Syn time with ntp server. Then sleep until we reach XX o'clock.*/
			daylight_done = syn_ntp(daylight_enable, 0 ,&syn_remaining_time,time_zone,start_time,end_time,wan_proto, wan_eth, ntp_server);
		}
		else
		{
			/* Wait until we reach next XX o'clock*/
			sleep(ONE_HOUR);
			
			/* renew remaining time for syn*/
			syn_remaining_time = syn_remaining_time - ONE_HOUR ;
			DEBUG_MSG("After sleeping one hour, syn_remaining_time =%d\n",syn_remaining_time);	
		}
	}	  
	return 0;
}

#endif
