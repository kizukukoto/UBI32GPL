#include <stdio.h>
#include <stdlib.h>
#include <sutil.h>
#include <shvar.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <build.h>
#include <nvram.h>
#include <linux_vct.h>
#include <unistd.h>//NickChou add for DynDNS CheckIP
#include <netdb.h>//NickChou add for DynDNS CheckIP
#include <errno.h>//NickChou add for errno (DynDNS CheckIP)

//#define TIMER_DEBUG 1
#ifdef TIMER_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define EXCEPTION_ARRAY_SIZE 2
#define ONE_HOUR    3600
    char *tz_daylight_sel;
    char *ntp_enable;


float timezone_daylight_savings_exception[EXCEPTION_ARRAY_SIZE] =
                {-82   /* timezone is indiana*/ ,
                 -112  /* timezone is Arizona */};

unsigned int year_month_days[2][13] = {
    { 365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    { 366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

struct daemonInfo
{
    int enable;
/* interval :
 *
 * 1). If we need do something every 10 seconds, set the following fields.
 *      interval = 10
 *      remaining_time = 10
 *      repeat = 1
 *      timeout_action = something
 *
 * 2). If we need do something after 10 seconds , set the following fields.
 *      interval = 10
 *      remaining_time = 10
 *      repeat = 0
 *      timeout_action = something
 *
 */
    int interval;
    int remaining_time;
    int repeat;
    int (*timeout_action)(void);
/* period and schedule:
 *
 * 3). If we need do something between a period(2007/1/1/pm3:00~2007/2/1/pm5:00) , set the following fields.
 *      done = 0
 *      start_time = transfer 2007/1/1/pm3:00 to seconds
 *      end_time = transfer 2007/2/1/pm5:00 to seconds
 *      schedule_action = something
 *
 */
    int done;
    int weekday[7];
    time_t start_time;
    time_t end_time;
    int (*schedule_action)(int flag);
}typedef daemonStatus;

/* Common variable */
char wan_eth[64] = {0};

/* ntp variable */
int time_zone = 0;
char ntp_server[64] = {0};
char this_system_time[13] = {0};
char last_system_time[] = "000000000000";
int this_time_method = 0; /*0:manual 1:ntp*/
int last_time_method = 0; /*0:manual 1:ntp*/
int last_time_dls_enable = 0;
int ntp_finish = 0;

/* ddns variable*/
int type = 0;
char ddns_hostname[64] ={0};
char ddns_username[64] = {0};
char ddns_password[64] = {0};
char wildcards[64]={0};
char ddns_dyndns_kinds[64] = {0};
char model_name[64] = {0};
char model_number[64] = {0};
int LanPortConnectState[4]={0};
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server
*   Notice :
*/
char ddns_type[256] = {0};

//NickChou add for DynDNS Check IP Tool
char dyndns_checkip[17] = {0};

int noip_initialized = 0;

/* prototype */
int monitor_http(void);
int kill_tracegw(void);
int syn_ntp_server(void);
int adjust_daylight(int flag);
int syn_ddns_server(void);

#ifdef CONFIG_IP_LOOKUP
int start_arpping(void);
#endif

#ifdef CONFIG_USER_WIRELESS_SCHEDULE
int check_wlan_sche(void);
#endif
/*  Date: 2009-4-02
*   Name: Ken Chiang
*   Reason: lib upnpd will acquire too many cpu time then cause booting time too long
            so start lib upnpd by timer to avoid upnpd delay other app process , root cause still not found
*   Notice :
*/
#ifdef UPNP_ATH_LIBUPNP
int start_lib_upnp(void);
#endif

#ifdef USE_SILEX
int check_meminfo_dirty(void);
int light_power_led=0;
int delay_blink=0;
#endif

daemonStatus http=  {1, 10, 10, 1,  monitor_http,   0,  {0},    0,  0,  NULL};
daemonStatus qos=   {0, 0,  0,  0,  kill_tracegw,   0,  {0},    0,  0,  NULL};
daemonStatus ntp=   {0, 0,  0,  1,  syn_ntp_server, 0,  {0},    0,  0,  NULL};
daemonStatus ddns=  {0, 0,  0,  1,  syn_ddns_server,0,  {0},    0,  0,  NULL};
daemonStatus dls=   {0, 0,  0,  0,  NULL,           0,  {0},    0,  0,  adjust_daylight};
#ifdef CONFIG_IP_LOOKUP
#ifdef CONFIG_USER_IP_LOOKUP_INTERVAL
daemonStatus arpping={1, CONFIG_USER_IP_LOOKUP_INTERVAL,	CONFIG_USER_IP_LOOKUP_INTERVAL, 1,	start_arpping,	0,	{0},	0,	0,	NULL};
#else
daemonStatus arpping={1,    180,    180,    1,  start_arpping,  0,  {0},    0,  0,  NULL};
#endif
#endif
#ifdef CONFIG_USER_WIRELESS_SCHEDULE
daemonStatus wlan_sche={0, 30, 30, 1, check_wlan_sche, 0, {0}, 0, 0, NULL};
#endif
/*  Date: 2009-4-02
*   Name: Ken Chiang
*   Reason: lib upnpd will acquire too many cpu time then cause booting time too long
            so start lib upnpd by timer to avoid upnpd delay other app process , root cause still not found
*   Notice :
*/
#ifdef UPNP_ATH_LIBUPNP
daemonStatus lib_upnp={0, 60, 60, 0, start_lib_upnp, 0, {0}, 0, 0, NULL};
#endif

#ifdef USE_SILEX
daemonStatus meminfo_dirty={1, 5, 5, 1, check_meminfo_dirty, 0, {0}, 0, 0, NULL};
#endif

#if defined (IPV6_CLIENT_LIST) || defined (CONFIG_IP_LOOKUP)
int check_link_state() {
    /*  NickChou modify
        if LAN Port state change, we need to arpping
        if not, we only check state, do nothing
    */
    int need_to_send=0;
    char *lan_eth = nvram_safe_get("lan_eth");
    int i;
    char status[10];

    for (i=VCTLANPORT0; i<VCTLANPORT4; i++) //i=2~5
    {
        if (VCTGetPortConnectState( lan_eth, i, status) == 0 )
        {
            DEBUG_MSG("LanPortConnectState[%d] = %d\n", i, LanPortConnectState[i-2]);
            if( !strncmp("disconnect", status, 10) ) // lan port disconnect now
            {
                if (LanPortConnectState[i-2]) //last state is connect
                {
                    DEBUG_MSG("VCTLANPORT%d state form connect to disconnect\n", i-2);
                    LanPortConnectState[i-2]=0;
                    need_to_send=1;
                }
            }
            else
            {
                if(!LanPortConnectState[i-2]) //last state is disconnect
                {
                    DEBUG_MSG("VCTLANPORT%d state form disconnect to connect\n", i-2);
                    LanPortConnectState[i-2]=1;
                    need_to_send=1;
                }
            }
        }
        else
            printf("VCTGetPortConnectState() error\n");
    }

    return need_to_send;
}
#endif

/****************************************************************************************************/

int monitor_http(void)
{
    int pid;

    pid = read_pid(HTTPD_PID);
    if (pid > 0) {
        DEBUG_MSG("http pid=%d\n", pid);
        if (!kill(pid, 0)) {
            DEBUG_MSG("http is running\n");
        }
        else {
            DEBUG_MSG("http restart\n");
#if (CONFIG_USER_HTTPD_HTTPD == 1)
            _system("httpd &");
#else
	    system("/bin/httpd -h /www -p 80");
#endif
        }
    }
	return 0;
}

/****************************************************************************************************/
/* return 0:fail
 * return 1:success
 */

int qos_initialize(void)
{
    qos.enable = 1;
    /* ken modify for trace router more time */
    //qos.interval = 5;
    qos.interval = 60;
    qos.remaining_time = qos.interval;
    return 1;
}

int kill_tracegw(void)
{
    int pid;

    pid = read_pid(TRACEGW_PID);

    if(pid > 0)
    {
        DEBUG_MSG("tracegw pid=%d\n", pid);
        if(!kill(pid, 0))
        {
            DEBUG_MSG("tracegw is still running, so we need to terminate it.\n");
            kill(pid,SIGTERM);
        }
    }
}

/****************************************************************************************************/


/* return 0:fail and disable
 * return 1:success and enable
 */

int ntp_initialize(void)
{
    FILE *fp;
    char buf[256], *pos;
    int start_week = 0, end_week = 0;
    struct timeval tv;
    struct timezone tz;
    struct tm tm_start, tm_end;
    struct tm *p_start, *p_end, *p_cur;
    int start_sday_of_week, end_sday_of_week;
    int i;
    int year = 2006,month = 4, date = 12, hour = 12, min = 24, sec = 12;
    time_t time_start_mon_1st, time_end_mon_1st; // The 1st day of start/end month of daylight saving time
    int wday_start_mon_1st, wday_end_mon_1st; // The weekday of the 1th day in both start/end DLS months
    int days2target_start_day, days2target_end_day;
    int start_time = 0, end_time = 0;


    fp = fopen("/var/etc/ntp.conf", "r");
    if (fp == NULL)
    {
        DEBUG_MSG("Could not open /var/etc/ntp.conf\n");
        ntp.enable = 0;
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
            DEBUG_MSG("ntp.conf: invalid line '%s'\n", buf);
            continue;
        }
        pos++;

        if(strstr(buf, "ntp_client_enable"))
            ntp.enable = atoi(pos);
        else if(strstr(buf, "time_zone"))
            time_zone = atoi(pos);
        else if(strstr(buf, "wan_eth"))
            strcpy(wan_eth , pos);
        else if(strstr(buf, "ntp_server"))
            strcpy(ntp_server , pos);
        else if(strstr(buf, "ntp_sync_interval"))
            ntp.interval = atoi(pos);
        else if(strstr(buf, "system_time"))
            strcpy(this_system_time , pos);
        else if(strstr(buf, "time_daylight_saving_enable"))
            dls.enable = atoi(pos);
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

    if(dls.enable == 1)
    {
        /* Get start and end time*/
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
        start_time = mktime(&tm_start);
        end_time = mktime(&tm_end);

        /* If setting time manually on this time and on the last time,
         * and the system_time are the same,
         * we take it as rc restarts on second time
         * and don't initialize dls parameters.
         */
        if((ntp.enable == 0) && (last_time_method == 0) && !strcmp(this_system_time,last_system_time))
        {
            DEBUG_MSG("rc restart on second time\n");
            //do nothing
        }
        else
        {
            /* initialize other parameters*/
            dls.start_time = start_time;
            dls.end_time = end_time;
            dls.done = 0;
        }
    }

    if(ntp.enable == 1)
    {
        ntp_finish = 0;
        DEBUG_MSG("ntp_finish = 0\n");

        /* initialize other parameters*/
        ntp.remaining_time = ntp.interval;

        return 1;
    }
    else /* set time manually */
    {
        ntp_finish = 1;
        DEBUG_MSG("ntp_finish = 1\n");

        time_zone = 0;
/*
   Name Albert Chen
   Date 2009-05-22
   Detail: it will cause time roll back, the system time has been set in rc initail
*/

//      if(strlen(this_system_time) < 1)
//      {
//          DEBUG_MSG("set system_time = __BUILD_DATE__\n");
//          _system("date -s %s", __BUILD_DATE__);
//      }else if(strcmp(this_system_time,last_system_time))
        if(strlen(this_system_time) >=1 && strcmp(this_system_time,last_system_time))
        {
            DEBUG_MSG("set time manually!!!\n");
	    	sscanf(this_system_time,"%d/%d/%d/%d/%d/%d",&year,&month,&date,&hour,&min,&sec);
            //_system("date -s %s", this_system_time);
            _system("date -s %02d%02d%02d%02d%04d",month,date,hour,min,year);
            strcpy(last_system_time,this_system_time);
        }
        return 0;
    }
}

int syn_ntp_server(void)
{
    int wan_status = 1;

    DEBUG_MSG("Because timeout, syn_ntp_server()\n");

    if( (wan_status = wan_connected_cofirm(wan_eth)) ) // Don't execute during upgrading
    {
        /* Syn time with ntp server */
        _system("ntpclient -h %s -s -i 4 -c 2 -t %d &", ntp_server,time_zone);
        return 0;
    }
    else
    {
        DEBUG_MSG("ntp: nothing to do...\n");
        return -1;
    }
}


/*
 * 1) When we enter the daylight saving time, add one hour.
 * 2) When we leave the daylight saving time, subtract one hour.
 *
 */
int adjust_daylight(int flag)
{
    struct timeval tv;
    struct timezone tz;
tz_daylight_sel = nvram_safe_get("time_daylight_offset");
ntp_enable=nvram_safe_get("ntp_client_enable");
    int i=0;

    DEBUG_MSG("Because timeout, adjust_daylight()\n");

    /* Check if we need not adjust daylight in the timezone */
    for(i=0; i < EXCEPTION_ARRAY_SIZE;i++)
    {
        if(time_zone == timezone_daylight_savings_exception[i])
        {
            return 0;
        }
    }

    gettimeofday(&tv,&tz);

    if(flag == 1)
    {

        DEBUG_MSG("start add hour\n");
        DEBUG_MSG("tz_daylight_sel=%s\n",tz_daylight_sel);
        if(strcmp(tz_daylight_sel,"-7200")==0)
        {
        DEBUG_MSG("- 2 hour\n");
        if(strcmp(ntp_enable,"1")==0)
        {
        tv.tv_sec -= 7200;
        }
        settimeofday(&tv, &tz);
        kill(read_pid(HTTPD_PID), SIGUSR2);
    dls.done=1;
        dls.start_time -= 7200;
        dls.end_time -= 7200;
        }
        if(strcmp(tz_daylight_sel,"-5400")==0)
        {
            DEBUG_MSG("- 1.5 hour\n");
            if(strcmp(ntp_enable,"1")==0){
            tv.tv_sec -= 5400;
        }
            settimeofday(&tv, &tz);
      kill(read_pid(HTTPD_PID), SIGUSR2);
    dls.done=1;
        dls.start_time -= 5400;
        dls.end_time -= 5400;
        }
        if(strcmp(tz_daylight_sel,"-3600")==0)
        {
                    DEBUG_MSG("- 1 hour\n");
        if(strcmp(ntp_enable,"1")==0){
            tv.tv_sec -= 3600;
        }
            settimeofday(&tv, &tz);
                  kill(read_pid(HTTPD_PID), SIGUSR2);
    dls.done=1;
        dls.start_time -= 3600;
        dls.end_time -= 3600;
        }
        if(strcmp(tz_daylight_sel,"-1800")==0)
        {
                    DEBUG_MSG("- 0.5 hour\n");
            if(strcmp(ntp_enable,"1")==0){
            tv.tv_sec -= 1800;
        }
            settimeofday(&tv, &tz);
      kill(read_pid(HTTPD_PID), SIGUSR2);
    dls.done=1;
        dls.start_time -= 1800;
        dls.end_time -= 1800;
        }
        if(strcmp(tz_daylight_sel,"1800")==0)
            {
                        DEBUG_MSG("+0.5 hour\n");
                if(strcmp(ntp_enable,"1")==0){
                tv.tv_sec += 1800;
            }
                settimeofday(&tv, &tz);
                      kill(read_pid(HTTPD_PID), SIGUSR2);
    dls.done=1;
        dls.start_time += 1800;
        dls.end_time += 1800;
            }
        if(strcmp(tz_daylight_sel,"3600")==0)
	{/* Add one hour */
        	DEBUG_MSG("Add one hour\n");
		/* when enable daylight and disable ntp server,must adjecte time */
        	//if(strcmp(ntp_enable,"1")==0){
	        tv.tv_sec += ONE_HOUR;
		//}
	        settimeofday(&tv, &tz);

        	/* Tell web server to get the adjusted time */
	        kill(read_pid(HTTPD_PID), SIGUSR2);

        	/* Record we have added one hour */
	        dls.done=1;
        	dls.start_time += ONE_HOUR;
	        dls.end_time += ONE_HOUR;

      	}
        if(strcmp(tz_daylight_sel,"5400")==0)
        {
                    DEBUG_MSG("+1.5 hour\n");
                    if(strcmp(ntp_enable,"1")==0){
                            tv.tv_sec += 5400;
                    }
                            settimeofday(&tv, &tz);
                      kill(read_pid(HTTPD_PID), SIGUSR2);
    dls.done=1;
        dls.start_time += 5400;
        dls.end_time += 5400;
        }
        if(strcmp(tz_daylight_sel,"7200")==0)
        {
                    DEBUG_MSG("+2 hour\n");
                if(strcmp(ntp_enable,"1")==0){
                    tv.tv_sec += 7200;
                    }
                                        settimeofday(&tv, &tz);
                      kill(read_pid(HTTPD_PID), SIGUSR2);
    dls.done=1;
        dls.start_time += 7200;
        dls.end_time += 7200;
        }

    }
    else if(flag == 2)
    {
        /* Subtract one hour */
        DEBUG_MSG("Subtract one hour\n");
        if(strcmp(ntp_enable,"1")==0){
        tv.tv_sec -= ONE_HOUR;
            }
        settimeofday(&tv, &tz);

        /* Tell web server to get the adjusted time */
        kill(read_pid(HTTPD_PID), SIGUSR2);

        /* Record we do not add one hour */
        dls.done=0;
        dls.start_time -= ONE_HOUR;
        dls.end_time -= ONE_HOUR;
    }
    return 1;
}

int check_mem()
{
        char mfree[32];
        int memfree = 0, ret = 0;

        if (!_GetMemInfo("MemFree:", mfree))
		goto out;
        if ((memfree = strtol(mfree, NULL, 10)) < 7) {
                printf("Memory free : %d \n", memfree);
		goto out;
        }
	ret = 1;
out:
        return ret;
}

int check_session()
{
        int ret = 0;
        FILE *fp = NULL;
        
	fp = fopen("/proc/sys/net/ipv4/netfilter/ip_conntrack_count","r");
	if(fp)
	{
		fscanf(fp,"%d",&ret);
		fclose(fp);
	}
	//printf("ip_conntrack_count = %d\n",ret);
	return ((ret > 4000)? 1:0);
}

/*NickChou copy from rc/app.c 20080716 to run every 180 sec*/
#ifdef CONFIG_IP_LOOKUP
int start_arpping(void)
{
     if(checkServiceAlivebyName("arpping"))
     {
         system("killall arpping &");
         sleep(1);
     }
     /* Robert, 2010/10/08, Prevent crush of DUT on Ubicom */
     if (!check_mem()) {
		printf("Memory is not enough\n");
		return 0;
     }

#ifndef IP8000
     if(check_session()) {
		//printf("Sessions are too big\n");
		return 0;
     }        
#endif
     /*NickChou, 2009.3.23, Do not need to check it
     if (check_link_state() == 1)
     */
     _system("arpping %s %s %s %s %s &",
        nvram_safe_get("lan_ipaddr"), nvram_safe_get("dhcpd_start"), nvram_safe_get("dhcpd_end"), nvram_safe_get("lan_bridge"), nvram_safe_get("lan_eth"));
}
#endif

#ifdef CONFIG_USER_WIRELESS_SCHEDULE

#ifdef ATH_AR938X
#define MAX_WLAN_INTERFACE 4
#else
#define MAX_WLAN_INTERFACE 2
#endif

char ws[MAX_WLAN_INTERFACE][64] = {};
int set_wlan_disable[MAX_WLAN_INTERFACE]={};

int check_wlan_sche(void) //NickChou Add
{
    char time_buf[30];
    char wlan_schedule[64] = {};
    char sche_start_time[10] = {}, sche_end_time[10] = {};
    char sche_start_time_hour[5] = {}, sche_start_time_min[5] = {}, sche_end_time_hour[5] = {}, sche_end_time_min[5] = {};
    char cur_hour[5], cur_min[5], cur_weekday[5];
    time_t clock;
    struct tm *tm;
    char *format = "%k %M %w";
    char *sche_name = NULL, *sche_weekdays = NULL, *next = NULL;
    int from=0, to=0, current=0, weekday=0;
    int ath_count = 0;
    int wifi_count = 0;
    int skfd;

#ifdef ATH_AR938X
    char *wlan_enable[] = {"wlan0_enable", "wlan1_enable", "wlan0_vap1_enable", "wlan1_vap1_enable"};
    char *wlan_sche[] = {"wlan0_schedule", "wlan1_schedule", "wlan0_vap1_schedule", "wlan1_vap1_schedule"};
#else
    char *wlan_enable[] = {"wlan0_enable", "wlan0_vap1_enable"};
    char *wlan_sche[] = {"wlan0_schedule", "wlan0_vap1_schedule"};
#endif

    /*  Normal Schedule format => name/weekdays/start_time/end_time => Test/1101011/08:00/18:00
        OverNight Schedule format => name/weekdays/start_time/end_time => Test/1101011/18:00/8:00
    */

    time(&clock);
    tm = gmtime(&clock);
    memset(time_buf, 0, sizeof(time_buf));
    strftime(time_buf, sizeof(time_buf), format, tm);
    sscanf(time_buf, "%s %s %s", cur_hour, cur_min, cur_weekday);
    DEBUG_MSG("cur_weekday = %d, cur_hour = %d, cur_min = %d\n", atoi(cur_weekday), atoi(cur_hour), atoi(cur_min));

    for(wifi_count=0; wifi_count<MAX_WLAN_INTERFACE; wifi_count++)
    {
        if(nvram_match(wlan_enable[wifi_count], "1") == 0)
        {
            memset(wlan_schedule, 0, sizeof(wlan_schedule));
            strcpy( wlan_schedule, ws[wifi_count] );
            DEBUG_MSG("check_wlan_sche:%s=%s\n", wlan_sche[wifi_count], wlan_schedule);

            if(strcmp("Never", wlan_schedule) == 0)
            {
                if(set_wlan_disable[wifi_count]==0)
                {
                    set_wlan_disable[wifi_count]=1;
                    printf("check_wlan_sche: Never: ath%d from enable to disable\n", ath_count);
                    _system("ifconfig ath%d down", ath_count);
/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                    if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                    if (wifi_count == 0) {
#endif
                        _system("kill %d", read_pid(GPIO_WLAN_PID));
                        system("gpio WLAN_LED off &");
                    }
		
                    /*	Date:	2009-11-27
                     *	Name:	Tina Tsao
                     *	Reason:	Fixed wireless broadcast status always in enabled when wireless schedule is not in system time
                     *	Note: if ath0 not down, down again.
                     */
                    skfd = socket(AF_INET, SOCK_DGRAM, 0);
                    if(!(skfd < 0))
                    {
                        DEBUG_MSG("ath0 already down. \n");
                    }else{
                        DEBUG_MSG("ath0 down again! \n");
                	    _system("ifconfig ath%d down", ath_count);
/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                        if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                        if (wifi_count == 0) {
#endif
                	        _system("kill %d", read_pid(GPIO_WLAN_PID));
                	        system("gpio WLAN_LED off &");
                	    }
                    }
                    close(skfd);
                }
                else
                    DEBUG_MSG("check_wlan_sche: Never: ath%d keep disable\n", ath_count);

                ath_count++;
                continue;
            }
            else
            {
                sche_name = strtok(wlan_schedule, "/");
                if(sche_name==NULL)
                {
                    printf("check_wlan_sche: ath%d sche_name error\n", ath_count);
                    ath_count++;
                    continue;
                }
            }

            memset(sche_start_time, 0, sizeof(sche_start_time));
            memset(sche_end_time, 0, sizeof(sche_end_time));
            memset(sche_start_time_hour, 0, sizeof(sche_start_time_hour));
            memset(sche_start_time_min, 0, sizeof(sche_start_time_min));
            memset(sche_end_time_hour, 0, sizeof(sche_end_time_hour));
            memset(sche_end_time_min, 0, sizeof(sche_end_time_min));

            sche_weekdays = strtok(NULL, "/");
            if(sche_weekdays==NULL)
            {
                ath_count++;
                continue;
            }

            next = strtok(NULL, "/") ? : "";
            strcpy(sche_start_time, next);

            next = strtok(NULL, "/") ? : "";
            strcpy(sche_end_time, next);

            if(strlen(sche_start_time) > 0)
            {
                next = strtok(sche_start_time, ":") ? : "";
                strcpy(sche_start_time_hour, next);
                next = strtok(NULL, ":") ? : "";
                strcpy(sche_start_time_min, next);
            }

            if(strlen(sche_end_time) > 0)
            {
                next = strtok(sche_end_time, ":") ? : "";
                strcpy(sche_end_time_hour, next);
                next = strtok(NULL, ":") ? : "";
                strcpy(sche_end_time_min, next);
            }

            DEBUG_MSG("sche_name = %s\n", sche_name);
            DEBUG_MSG("sche_weekdays = %s\n", sche_weekdays);
            DEBUG_MSG("cur_hour   = %d, cur_min   = %d\n", atoi(cur_hour), atoi(cur_min));
            DEBUG_MSG("start_hour = %d, start_min = %d\n", atoi(sche_start_time_hour), atoi(sche_start_time_min));
            DEBUG_MSG("end_hour   = %d, end_min   = %d\n", atoi(sche_end_time_hour), atoi(sche_end_time_min));

            from = atoi(sche_start_time_hour)*60 + atoi(sche_start_time_min);
            to  = atoi(sche_end_time_hour)*60 + atoi(sche_end_time_min);
            current = atoi(cur_hour)*60 + atoi(cur_min);
            weekday = atoi(cur_weekday);
            DEBUG_MSG("weekday=%d, from=%d, to=%d, current=%d\n", weekday, from, to, current);

            if(from <= to)
            {
                DEBUG_MSG("Wireless Schedule is normal schedule\n");
                if ( *(sche_weekdays + weekday) == '1')
                {
                    DEBUG_MSG("check today wireless schedule: cur_weekday=%d\n", weekday);
                    if( from == to)
                {
                    DEBUG_MSG("Enable Wireless All Day !!\n");
                    ath_count++;
                    continue;
                }
                else
                {
                    if( (current >= from) && (current <= to) )
                    {
                        if(set_wlan_disable[wifi_count])
                        {
                            printf("check_wlan_sche: in schedule: ath%d from disable to enable\n", ath_count);
                            set_wlan_disable[wifi_count]=0;
                            _system("ifconfig ath%d up", ath_count);
/*
 * Reason: Turn on LED only when main wireless is on schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                            if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                            if (wifi_count == 0) {
#endif
                                _system("kill %d", read_pid(GPIO_WLAN_PID));
                                system("gpio WLAN_LED blink &");
                            }    
                        }
                        else {
                            DEBUG_MSG("check_wlan_sche: in schedule: ath%d keep enable\n", ath_count);

                            /* if current time = end time, stop wireless */
                            if (current == to) {
                                goto check_sche;
                            }
                        }
                    }
                    else
                    {
check_sche:
                        if(set_wlan_disable[wifi_count]==0)
                        {
                            set_wlan_disable[wifi_count]=1;
                            printf("check_wlan_sche: out schedule: ath%d from enable to disable\n", ath_count);
                            _system("ifconfig ath%d down", ath_count);
/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                            if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                            if (wifi_count == 0) {
#endif
                                _system("kill %d", read_pid(GPIO_WLAN_PID));
                                system("gpio WLAN_LED off &");
                            }    

                            /*	Date:	2009-11-27
                             *	Name:	Tina Tsao
                             *	Reason:	Fixed wireless broadcast status always in enabled when wireless schedule is not in system time
                             *	Note: if ath0 not down, down again.
                             */
                            skfd = socket(AF_INET, SOCK_DGRAM, 0);
                            if(!(skfd < 0))
                            {
                                DEBUG_MSG("ath0 already down. \n");
                            }else{
                                DEBUG_MSG("ath0 down again! \n");
                        	    _system("ifconfig ath%d down", ath_count);
/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                                if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                                if (wifi_count == 0) {
#endif
                        	        _system("kill %d", read_pid(GPIO_WLAN_PID));
                        	        system("gpio WLAN_LED off &");
                        	    }    
                            }
                            close(skfd);
                        }
                        else
                            DEBUG_MSG("check_wlan_sche: out schedule: ath%d keep disable\n", ath_count);
                    }
                }
            }
            else
            {
                if(set_wlan_disable[wifi_count]==0)
                {
                    DEBUG_MSG("Disable Wireless All Day !!\n");
                    _system("ifconfig ath%d down", ath_count);
/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                    if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                    if (wifi_count == 0) {
#endif
                        _system("kill %d", read_pid(GPIO_WLAN_PID));
                        system("gpio WLAN_LED off &");
                    }    
 
                    /*	Date:	2009-11-27
                     *	Name:	Tina Tsao
                     *	Reason:	Fixed wireless broadcast status always in enabled when wireless schedule is not in system time
                     *	Note: if ath0 not down, down again.
                     */
                    skfd = socket(AF_INET, SOCK_DGRAM, 0);
                    if(!(skfd < 0))
                    {
                        DEBUG_MSG("ath0 already down. \n");
                    }else{
                        DEBUG_MSG("ath0 down again! \n");
                        _system("ifconfig ath%d down", ath_count);
/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                        if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                        if (wifi_count == 0) {
#endif
                            _system("kill %d", read_pid(GPIO_WLAN_PID));
                            system("gpio WLAN_LED off &");
                        }
                    }
                    close(skfd);
                    set_wlan_disable[wifi_count]=1;
                }
            }
            }
            else if(from > to)
            {
                DEBUG_MSG("Wireless Schedule is OverNight schedule\n");
                /*
                OverNight Schedule format => name/weekdays/start_time/end_time => Test/1101011/18:00/8:00
                If today is Friday. we need to check 18:00~24:00 at Fri and 00:00~08:00 at Thu.

                if( ((*(sche_weekdays+weekday)=='1')&&(current>=from)&&(current<=24*60)) )
                    printf("Wireless OverNight Schedule: match first part of today schedule!!\n");
                if( ((weekday!=0)&&(*(sche_weekdays+weekday-1)=='1')&&(current>=0)&&(current<=to)) )
                    printf("Wireless OverNight Schedule: match second part of yesterday schedule, today is not Sunday !!\n");
                if( ((weekday==0)&&(*(sche_weekdays+6)=='1')&&(current>=0)&&(current <= to)) )
                    printf("Wireless OverNight Schedule: match second part of yesterday schedule, today is Sunday !!\n");
                */
                if( ((*(sche_weekdays+weekday)=='1')&&(current>=from)&&(current<=24*60)) ||
                    ((weekday!=0)&&(*(sche_weekdays+weekday-1)=='1')&&(current>=0)&&(current<=to)) ||
                    ((weekday==0)&&(*(sche_weekdays+6)=='1')&&(current>=0)&&(current <= to))
                )
                {
                    if(set_wlan_disable[wifi_count])
                    {
                        printf("check_wlan_sche: in OverNight schedule: ath%d from disable to enable\n", ath_count);
                        set_wlan_disable[wifi_count]=0;
                        _system("ifconfig ath%d up", ath_count);
/*
 * Reason: Turn on LED only when main wireless is on schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                        if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                        if (wifi_count == 0) {
#endif
                            _system("kill %d", read_pid(GPIO_WLAN_PID));
                            system("gpio WLAN_LED blink &");
                        }    
                    }
                    else
                        DEBUG_MSG("check_wlan_sche: in OverNight schedule: ath%d keep enable\n", ath_count);
                }
                else
                {
                    if(set_wlan_disable[wifi_count]==0)
                    {
                        set_wlan_disable[wifi_count]=1;
                        printf("check_wlan_sche: Out Over-Night-Schedule: ath%d from enable to disable\n", ath_count);
                        _system("ifconfig ath%d down", ath_count);

/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                        if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                        if (wifi_count == 0) {
#endif
                            _system("kill %d", read_pid(GPIO_WLAN_PID));
                            system("gpio WLAN_LED off &");
                        }    

                        /*	Date:	2009-11-27
                         *	Name:	Tina Tsao
                         *	Reason:	Fixed wireless broadcast status always in enabled when wireless schedule is not in system time
                         *	Note: if ath0 not down, down again.
                         */
                        skfd = socket(AF_INET, SOCK_DGRAM, 0);
                        if(!(skfd < 0))
                        {
                            DEBUG_MSG("ath0 already down. \n");
                        }else{
                            DEBUG_MSG("ath0 down again! \n");
                            _system("ifconfig ath%d down", ath_count);

/*
 * Reason: Turn off LED only when main wireless is out of schedule.
 * Modified: Yufa Huang
 * Date: 2010.09.20
 */
#ifdef AR7161
                            if ((wifi_count == 0) || (wifi_count == 1)) {
#else
                            if (wifi_count == 0) {
#endif
                                _system("kill %d", read_pid(GPIO_WLAN_PID));
                                system("gpio WLAN_LED off &");
                            }    
                        }
                        close(skfd);
                    }
                    else
                        DEBUG_MSG("check_wlan_sche: Out Over-Night-Schedule: ath%d keep disable\n", ath_count);
                }
            }
            ath_count++;
        }
    }
    return 0;
}
#endif //CONFIG_USER_WIRELESS_SCHEDULE

/*  Date: 2009-4-02
*   Name: Ken Chiang
*   Reason: lib upnpd will acquire too many cpu time then cause booting time too long
            so start lib upnpd by timer to avoid upnpd delay other app process , root cause still not found
*   Notice :
*/
#ifdef UPNP_ATH_LIBUPNP
int start_lib_upnp(void)
{
    if(checkServiceAlivebyName("upnpd"))
    {
        printf("timer lib upnp stop\n");
        system("killall upnpd &");
        sleep(1);
    }
    printf("timer lib upnp start\n");
    _system("upnpd %s %s &",nvram_safe_get("wan_eth"), nvram_safe_get("lan_bridge"));
}
#endif

#ifdef USE_SILEX
int check_meminfo_dirty(void)
{
	char mdirty[32];
	int memdirty = 0, ret = 0;
	int extern_mount_flag=0;

	FILE *fp;
	char buf[80], mem[32], *ptr;
	int index = 0;
	
	if ((fp = fopen("/proc/mounts", "r")) == NULL)
		goto out;

	while(fgets(buf, sizeof(buf), fp)!= NULL)
	{
		if(strstr(buf, "/tmp/mm") || strstr(buf, "/tmp/sd"))
			extern_mount_flag = 1;
	}
	fclose(fp);
	DEBUG_MSG("extern mount flag =(%d)\n", extern_mount_flag);

	if(extern_mount_flag==1){
		memset(mem, '\0', sizeof(mem));
  	
		if ((fp = popen("cat /proc/meminfo", "r")) == NULL)
			goto out;
		
		while (fgets(buf, sizeof(buf), fp)) {
			if (ptr = strstr(buf, "Dirty:")) {
				ptr += (strlen("Dirty:") + 2);
  	
				/* Skip space */
				while (*ptr == ' ')  ptr++;
  	
				/* Get value */
				while (*ptr != ' ') {
					mem[index] = *ptr;
					ptr++;
					index++;
				}
				break;
			}
		}
		pclose(fp);
		memdirty = atol(mem);
		DEBUG_MSG("Memory dirty : %d \n", atol(mem));
	
		if(memdirty > 10){
			DEBUG_MSG("Blinking LED~~~ \n");
			system("gpio POWER_LED blink&");
			light_power_led = 1;
			extern_mount_flag = 0;
			delay_blink = 2;
		}
		else if(light_power_led == 1){
			if(delay_blink > 0){
				DEBUG_MSG("Delay Blink = (%d)\n", delay_blink);
				sync();
				delay_blink -= 1;
			}
			else if(delay_blink == 0){
				DEBUG_MSG("Safely Remove!!! \n");
				system("kill -9 `cat /var/run/gpio_power.pid`");
				system("rm -rf /var/run/gpio_power.pid");
				system("gpio POWER_LED on");
				light_power_led = 0;
				delay_blink = 0;
			}
		}
	}
	
	ret = 1;
out:
	return ret;	
}
#endif

/**************************************************************************************/
/* return 0:fail and disable
 * return 1:success and enable
 */

int ddns_initialize(void)
{
    FILE *fp;
	char buf[256], *pos,*ddns_type_e=NULL;
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server
*   Notice :
*/
    char ddns_wildcards_enable[64] = {0};

/* jimmy modified 20081217, for add oray ddns support */
if(!nvram_match("ddns_type","www.oray.cn"))
    return 1;
    fp = fopen("/var/etc/ddns.conf", "r");
    if (fp == NULL)
    {
        DEBUG_MSG("Could not open /var/etc/ddns.conf\n");
        ddns.enable = 0;
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
            DEBUG_MSG("ddns.conf: invalid line '%s'\n", buf);
            continue;
        }
        pos++;

        if(strstr(buf, "ddns_enable"))
            ddns.enable = atoi(pos);

        else if(strstr(buf, "ddns_sync_interval"))
        {
            ddns.interval = atoi(pos);
            if( ddns.interval < ONE_HOUR )
                    ddns.interval = ONE_HOUR;
        }
        else if(strstr(buf, "wan_eth"))
            strcpy(wan_eth , pos);
        else if(strstr(buf, "ddns_hostname"))
            strcpy(ddns_hostname , pos);
        else if(strstr(buf, "ddns_type"))
		{
			ddns_type_e = strstr(pos, "(");
			if(ddns_type_e == NULL)
				strcpy(ddns_type , pos);
			else
				strncpy(ddns_type, pos, ddns_type_e-pos);

			/* convert to lower string */
			int i;
			for (i=0; i< strlen(ddns_type); i++)
				ddns_type[i] = tolower(ddns_type[i]);
		}
        else if(strstr(buf, "ddns_username"))
            strcpy(ddns_username , pos);
        else if(strstr(buf, "ddns_password"))
            strcpy(ddns_password , pos);
        else if(strstr(buf, "ddns_wildcards_enable"))
            strcpy(ddns_wildcards_enable , pos);
        else if(strstr(buf, "ddns_dyndns_kinds"))
            strcpy(ddns_dyndns_kinds , pos);
        else if(strstr(buf, "model_name"))
            strcpy(model_name , pos);
        else if(strstr(buf, "model_number"))
            strcpy(model_number , pos);
    }

    fclose(fp);

    if(ddns.enable == 1)
    {
		_system("ping -c 1 \"%s\" > /var/etc/ddns_check.txt 2>&1", ddns_hostname);
		
        /* initialize other parameters*/
        ddns.remaining_time = ddns.interval;

        /*Decide the type*/
		if (strcmp( ddns_type, "dyndns.org" ) == 0 || strcmp( ddns_type, "0" ) == 0 || strcmp(ddns_type, "dyndns.com", 10) ==0 )
		{	
			system("ping -c 1 checkip.dyndns.org > /var/etc/DynDNS_CheckIP.txt 2>&1");
			type = 0;
		}	
        else if (strcmp( ddns_type, "easydns.com" ) == 0 || strcmp( ddns_type, "1" ) == 0)
            type = 1;
        else if (strstr( ddns_type, "no-ip" ) || strcmp( ddns_type, "2" ) == 0) {
        	system("ping -c 1 checkip.dyndns.org > /var/etc/DynDNS_CheckIP.txt 2>&1");
            type = 2;
        }    
        else if (strcmp( ddns_type, "changeip.com" ) == 0 || strcmp( ddns_type, "3" ) == 0)
            type = 3;
        else if (strcmp( ddns_type, "eurodyndns.com" ) == 0 || strcmp( ddns_type, "4" ) == 0)
            type = 4;
        else if (strcmp( ddns_type, "ods.com" ) == 0 || strcmp( ddns_type, "5" ) == 0)
            type = 5;
        else if (strcmp( ddns_type, "ovh.com" ) == 0 || strcmp( ddns_type, "6" ) == 0)
            type = 6;
        else if (strcmp( ddns_type, "regfish.com" ) == 0 || strcmp( ddns_type, "7" ) == 0)
            type = 7;
        else if (strcmp( ddns_type, "tzo.com" ) == 0 || strcmp( ddns_type, "8" ) == 0)
            type = 8;
        else if (strcmp( ddns_type, "zoneedit.com" ) == 0 || strcmp( ddns_type, "9" ) == 0)
            type = 9;
        else if (strcmp( ddns_type, "dlinkddns.com" ) == 0 || strcmp( ddns_type, "10" ) == 0) {
        	system("ping -c 1 checkip.dyndns.org > /var/etc/DynDNS_CheckIP.txt 2>&1");
            type = 10;
        }    
        else if (strcmp( ddns_type, "cybergate.com" ) == 0 || strcmp( ddns_type, "11" ) == 0)
            type = 11;
		else//other ddns
/*  Date: 2010-01-25
*   Name: Ken Chiang
*   Reason: modify for the DDNS can support other like DDNS server
*   Notice :
*/
			type = 12;

        /* enable dyndns wildcard*/
        if((strcmp(ddns_wildcards_enable,"1") == 0) && (type == 0) )
            strcpy(wildcards,"-w");
        else
            strcpy(wildcards," ");

        return 1;
    }
    else
        return 0;
/* --------------------------------------------------------- */
}

char *get_ipaddr(char *if_name)
{
    int sockfd;
    struct ifreq ifr;
    struct in_addr temp_ip;

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
        return 0;

    strcpy(ifr.ifr_name, if_name);
    if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        close(sockfd);
        return 0;
    }

    close(sockfd);

    temp_ip.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
    return inet_ntoa(temp_ip);
}


/*
	1. An error while accessing Check IP, the client should not send an update
	2. Checks must be spaced 10 minutes apart to help reduce server load
	3. Check IP responds to valid HTTP requests for http://checkip.dyndns.com/.
		A valid request will result in the following sample response:
		<html><head><title>Current IP Check</title></head><body>Current IP Address: 124.9.4.124</body></html>
*/
char *dyndns_checkip_tool(char *host_ip)
{
	struct  hostent *host;
	struct  sockaddr_in addr;
	long arg;
	int		fd=0, ret=0;
	char send_buf[256]={0}, recv_buf[512]={0};
	fd_set afdset;
	struct timeval timeval;
	char *public_ip_s=NULL, *public_ip_e=NULL;
	static char public_ip[20]={0};
	FILE *fp;
	char checkip_buf[80]={0};
	char checkip_ip[20]={0};
	char *ip_s=NULL, *ip_e=NULL;
	
	DEBUG_MSG("!!! DynDNS use Web IP Detection (CheckIP) !!!!! %s\n", host_ip);

	/*NickChou 20100505 modify from gethostbyname to PING
	If we use gethostbyname(), 
	when WAN IP change to other domain, device will use old IP/DNS to query.
	(If kill timer then restart => gethostbyname() and PING is OK)
	*/
	fp = fopen("/var/etc/DynDNS_CheckIP.txt", "r");
	if(fp)
	{		
		DEBUG_MSG("dyndns_checkip_tool: check DynDNS_CheckIP.txt\n");
		memset(checkip_buf, 0, 80);
		fgets(checkip_buf, sizeof(checkip_buf), fp);
		ip_s = strstr(checkip_buf, "(");
		ip_e = strstr(checkip_buf, ")");
		if(ip_s && ip_e)
		{
			ip_s++;
			memset(checkip_ip, 0, sizeof(checkip_ip));
			strncpy(checkip_ip, ip_s, (ip_e - ip_s));
			inet_aton(checkip_ip, &addr.sin_addr);
		}	
		else
		{
			//ex. ping: dlinktest1111.dyndns.org: Unknown host
			printf("dyndns_checkip_tool: parser DynDNS_CheckIP.txt error\n");
			return NULL;
		}						
		fclose(fp);
	}
	else
	{
		printf("dyndns_checkip_tool: open /var/tmp/DynDNS_CheckIP.txt fail\n");
		return NULL;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	DEBUG_MSG("dyndns_checkip_tool: checkip.dyndns.org=%s\n", inet_ntoa(addr.sin_addr));

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("DynDNS CheckIP: socket()");
		close(fd);
		return NULL;
	}

	DEBUG_MSG("DynDNS CheckIP: connect()...\n");

	if (connect(fd, &addr, sizeof(addr)) < 0)
	{
		perror("DynDNS CheckIP: connect()");
		close(fd);
		return "retry";
	}


	sprintf(send_buf, "GET / HTTP/1.1\r\nHost: checkip.dyndns.org\r\nConnection: Keep-Alive\r\n\r\n");
	DEBUG_MSG("DynDNS CheckIP :send_buf =\n%s\n", send_buf);

	if( send(fd, send_buf, strlen(send_buf), 0 ) < 0)
	{
		perror("DynDNS CheckIP: send()");
		close(fd);
		return NULL;
	}

	memset(recv_buf, 0, sizeof(recv_buf));
	DEBUG_MSG("DynDNS CheckIP: recv()...\n");

	FD_ZERO(&afdset);
	FD_SET(fd, &afdset);
	timeval.tv_sec = 3;
	timeval.tv_usec = 0;
	/* timeout error */
	ret = select(fd + 1, &afdset, NULL, NULL, &timeval);
	if( ret == 0)
	{
		printf("DynDNS CheckIP: select() timeout\n");
		close(fd);
		return NULL;
	}
	else if(ret == -1)
	{
		if(errno == EINTR)
			printf("DynDNS CheckIP: select(): Interrupted system call: Keep going to read()\n");
		else
		{
			perror("DynDNS CheckIP: select()");
		close(fd);
		return NULL;
	}
	}

	/* receive fail */
	if(FD_ISSET(fd, &afdset))
		read(fd, recv_buf, sizeof(recv_buf));
	else
	{
		DEBUG_MSG("DynDNS CheckIP: recv_buf = NULL\n");
		return NULL;
	}

	DEBUG_MSG("DynDNS CheckIP: recv_buf = \n%s\n", recv_buf);

	if( shutdown(fd, 2) < 0)
	{
		perror("DynDNS CheckIP: shutdown()");
		return NULL;
	}

	/* check end of data */
	public_ip_s = strstr(recv_buf, "Current IP Address: ");
	if( public_ip_s == NULL)
	{
		printf("DynDNS CheckIP: recv_buf error\n");
		return NULL;
	}
	else
		public_ip_e = strstr(recv_buf, "</body>");

	if(public_ip_e)
	{
		memset(public_ip, 0, sizeof(public_ip));
		strncpy(public_ip, public_ip_s+20, public_ip_e - (public_ip_s+20) );	
		printf("DynDNS CheckIP: Current Public IP Address=%s\n", public_ip);
		_system("echo %s > /var/etc/dyndns_checkip", public_ip);
		
		if(strcmp(public_ip, host_ip)==0)
		{
			printf("DynDNS CheckIP: Do not need to Update DynDNS\n");		
			return NULL;	
		}
		else
		{	
			printf("DynDNS CheckIP: Update DynDNS: public_ip(%s) != host_ip(%s)\n", public_ip, host_ip);
			return public_ip;	
		}
	}
	else
	{
		printf("DynDNS CheckIP: parser error\n");
		return NULL;
	}
}

int wan_ip_differ_from_ddns_ip(void)
{
	FILE *fp = NULL;
	char buf[256]={0}, host_ip[20]={0};
	char *ip_s=NULL, *ip_e=NULL;	
	char *public_ip=NULL;
		
	DEBUG_MSG("!!!!!!!!!!check wan_ip_differ_from_ddns_ip !!!!!!!!!type=%d\n", type);

	fp = fopen("/var/etc/ddns_check.txt", "r");
	if(fp)
	{		
		DEBUG_MSG("wan_ip_differ_from_ddns_ip: check ddns_check.txt\n");
		memset(buf, 0, 80);
		fgets(buf, sizeof(buf), fp);
		DEBUG_MSG("ddns_check.txt=%s\n", buf);
		ip_s = strstr(buf, "(");
		ip_e = strstr(buf, ")");
		fclose(fp);
		if(ip_s && ip_e)
		{
			memset(host_ip, 0, sizeof(host_ip));
			strncpy(host_ip, ip_s+1, ip_e-(ip_s+1));
			if ((type == 0) || (type == 2) || (type == 10))   //DynDNS || NO-IP || DLINKDDNS, CheckIP Tool
			{
retry_dyndns:				
				printf("wan_ip_differ_from_ddns_ip: DynDNS CheckIP Tool\n");
				if( (public_ip = dyndns_checkip_tool(host_ip)) != NULL){
						printf("wan_ip_differ_from_ddns_ip: public_ip=%s\n", public_ip); 
						if(strcmp(public_ip, "retry")==0){
							DEBUG_MSG("retry_dyndns\n");
							goto retry_dyndns;
						}
						else	
						{	
							memset(dyndns_checkip, 0, sizeof(dyndns_checkip));
							strncpy(dyndns_checkip, public_ip, strlen(public_ip));	
						}	
						return 1;
					}
					else
				{
#if 1
					printf("Update no matter checkIP success or not\n");
					return 1;
#else					
					printf("wan_ip_differ_from_ddns_ip: dyndns_checkip_tool: Do not need update\n");
					return 0;
#endif						
				}
			}
			else if(strcmp(get_ipaddr(wan_eth), host_ip))
			{
				DEBUG_MSG("wan_ip_differ_from_ddns_ip: wan ip %s != %s %s\n", get_ipaddr(wan_eth), ddns_hostname, host_ip);
				return 1;
			}	
		}	
		else
		{
			//ex. ping: dlinktest1111.dyndns.org: Unknown host
			printf("wan_ip_differ_from_ddns_ip: check ddns_check.txt: fail\n");
			return 1;	//NickChou	
		}			
	}
	else
	{
		printf("wan_ip_differ_from_ddns_ip: open /var/tmp/ddns_check.txt fail");
		return	0;	
	}
}

int syn_ddns_server(void)
{
	int wan_status = 1;
/* jimmy modified 20081217, for add oray ddns support */
if(!nvram_match("ddns_type","www.oray.cn"))
	return 0;
	printf("Because timeout or wan_ip_differ_from_ddns_ip, syn_ddns_server()\n");
	if ((wan_status = wan_connected_cofirm(wan_eth)))   //Don't execute during upgrading
	{
		if (type == 12) {   //other ddns
			DEBUG_MSG("type==12 ,ddns_type=%s\n", ddns_type);
			_system("/sbin/noip2 -T %d -t %s -R %s -u %s -p %s %s -I %s -k %s -a %s -n %s &", \
				type, ddns_type, ddns_hostname, ddns_username, ddns_password, wildcards, \
				wan_eth, ddns_dyndns_kinds, model_name, model_number);
		}
		else {
			if ((type == 0) || (type == 2) || (type == 10)) //DynDNS || NO-IP
			{
				char *syn_dyndns_ip;
				DEBUG_MSG("syn_ddns_server: WAN IP=%s, dyndns_checkip=%s\n", get_ipaddr(wan_eth), dyndns_checkip);

retry_syn_dyndns:
				printf("syn_ddns_server: DynDNS CheckIP Tool\n");
				if( (syn_dyndns_ip = dyndns_checkip_tool(dyndns_checkip)) != NULL){
					printf("syn_ddns_server: syn_dyndns_ip=%s\n", syn_dyndns_ip);
					if (strcmp(syn_dyndns_ip, "retry") == 0) {
						goto retry_syn_dyndns;
					}
					else {
						memset(dyndns_checkip, 0, sizeof(dyndns_checkip));
						strncpy(dyndns_checkip, syn_dyndns_ip, strlen(syn_dyndns_ip));
					}
				}

				_system("/sbin/noip2 -T %d -R %s -u %s -p %s %s -i %s -k %s -a %s -n %s &", \
					type, ddns_hostname, ddns_username, ddns_password, wildcards, \
					dyndns_checkip, ddns_dyndns_kinds, model_name, model_number);
			}
			else
			{
				_system("/sbin/noip2 -T %d -R %s -u %s -p %s %s -I %s -k %s -a %s -n %s &", \
					type, ddns_hostname, ddns_username, ddns_password, wildcards, \
					wan_eth, ddns_dyndns_kinds, model_name, model_number);
			}
		}
		return 0;
	}
    else
	{
		DEBUG_MSG("syn_ddns_server: nothing to do...\n");
		return -1;
	}
}


/**************************************************************************************/
/* If something is triggered by a interval(every 10 seconds),
 * it is monitored by trigger_by_interval().
 */
int trigger_by_interval(daemonStatus *daemon)
{
    if(daemon->remaining_time > 0)
    {
        daemon->remaining_time--;
        return 0;
    }
    else
    {
        if(daemon->repeat)
            daemon->remaining_time = daemon->interval;
        else
            daemon->enable = 0;

        return 1;
    }
}
/* If something is triggered by a period(2007/1/3/pm3:00~2007/2/1/pm5:00),
 * it is monitored by trigger_by_period().
 */
int trigger_by_period(daemonStatus *daemon,time_t cur_time)
{
    DEBUG_MSG("daemon->start_time=%d\n",daemon->start_time);
    DEBUG_MSG("daemon->end_time=%d\n",daemon->end_time);
    DEBUG_MSG("cur_time=%d\n",cur_time);
    DEBUG_MSG("daemon->done=%d\n",daemon->done);
    /* Check if it is schedule time now */
    if (((daemon->start_time <= daemon->end_time) && ((cur_time >= daemon->start_time) && (cur_time < daemon->end_time))) ||
        ((daemon->start_time > daemon->end_time)  && ((cur_time >= daemon->start_time) || (cur_time < daemon->end_time))))
    {
        /* Check if we have not performed the action yet*/
        if(!daemon->done)
        {
            daemon->done = 1;
            return 1;
        }
    }
    /* It's not in schedule time now */
    else
    {
        /* We have performed the action*/
        if(daemon->done)
        {
            daemon->done = 0;
            return 2;
        }
    }
    return 0;
}


int checkServiceAlivebyName(char *service)
{
        FILE * fp;
        char buffer[1024];
        if((fp = popen("ps -ax", "r"))){
                while( fgets(buffer, sizeof(buffer), fp) != NULL ) {
                        if(strstr(buffer, service)) {
                                pclose(fp);
                                return 1;
                        }
                }
        }
        pclose(fp);
        return 0;
}

/* Signal handling*/
static void timer_signal(int sig)
{
    FILE *fp;
    char sig_tmp[32] = {};
    switch(sig)
    {
        case SIGUSR1:
            if (ntp_initialize())
                ntp.timeout_action();
            break;

        case SIGHUP:
            /* Almost all singal can not receive except SIGHUP for Ubicom */
            //qos_initialize();

            fp = fopen(SIG_TMP_FILE,"r");
            if (fp)
            {
                while (fgets(sig_tmp, 32, fp))
                {
                    DEBUG_MSG("Timer_Signal = %s\n",sig_tmp);
                    if (strcmp(sig_tmp, "ddns") == 0) {
if(nvram_match("ddns_type","www.oray.cn"))
{
                        if (ddns_initialize())
                            //if (wan_ip_differ_from_ddns_ip())
                                ddns.timeout_action();
}
                    }
/*start radvd using SIGQUIT */
/*                    if (strcmp(sig_tmp, "ipv6") == 0) {
#ifdef RADVD
                        if (checkServiceAlivebyName("radvd")) {
                            _system("rm -f /var/run/radvd.pid");
                            _system("killall radvd");
                            sleep(1);
                        }
			if ( nvram_match("ipv6_autoconfig" , "1") == 0 &&
				nvram_match("ipv6_wan_proto" , "link_local") != 0 )
			{
                        	start_radvd();
			}
#endif
                    }

*/
                    if (strcmp(sig_tmp, "ntp") == 0) {
                        if (ntp_initialize())
                            ntp.timeout_action();
                    }
                }
                fclose(fp);
            }
            break;

        case SIGINT:
/* jimmy modified 20081217, for add oray ddns support */
if(nvram_match("ddns_type","www.oray.cn"))
{
                if(ddns_initialize())
                    //if(wan_ip_differ_from_ddns_ip())
                        ddns.timeout_action();
}
/* -------------------------------------------- */
                break;

        case SIGUSR2:
                DEBUG_MSG("ntp_finish = 1\n");
                ntp_finish = 1;
                break;

#ifdef IPv6_SUPPORT
        case SIGPIPE:
                _system("killall -SIGUSR2 httpd");
                break;
#endif

#ifdef CONFIG_USER_WIRELESS_SCHEDULE
        case SIGTSTP:
#ifdef ATH_AR938X
                if(!nvram_match("wlan0_enable", "1") || !nvram_match("wlan1_enable", "1"))
                {
                    if( ((!nvram_match("wlan0_schedule", "Always")) || (!nvram_match("wlan0_schedule",""))) &&
                        ((!nvram_match("wlan1_schedule", "Always")) || (!nvram_match("wlan1_schedule","")))  &&
                        ((!nvram_match("wlan0_vap1_schedule", "Always")) || (!nvram_match("wlan0_vap1_schedule",""))) &&
                        ((!nvram_match("wlan1_vap1_schedule", "Always")) || (!nvram_match("wlan1_vap1_schedule","")))
                    )
                        wlan_sche.enable=0;
                    else
                    {
                        strcpy(ws[0], nvram_safe_get("wlan0_schedule"));
                        strcpy(ws[1], nvram_safe_get("wlan1_schedule"));
                        strcpy(ws[2], nvram_safe_get("wlan0_vap1_schedule"));
                        strcpy(ws[3], nvram_safe_get("wlan1_vap1_schedule"));
                        wlan_sche.enable=1;
                    }

                    set_wlan_disable[0]=0;
                    set_wlan_disable[1]=0;
                    set_wlan_disable[2]=0;
                    set_wlan_disable[3]=0;
                }
#else
                if(!nvram_match("wlan0_enable", "1"))
                {
                if( ((!nvram_match("wlan0_schedule", "Always")) || (!nvram_match("wlan0_schedule","")) ) &&
                            ((!nvram_match("wlan0_vap1_schedule", "Always")) || (!nvram_match("wlan0_vap1_schedule","")))
                    )
                        wlan_sche.enable=0;
                    else
                    {
                    strcpy(ws[0], nvram_safe_get("wlan0_schedule"));
                    strcpy(ws[1], nvram_safe_get("wlan0_vap1_schedule"));
                    wlan_sche.enable=1;
                    }

                    set_wlan_disable[0]=0;
                    set_wlan_disable[1]=0;
                }
#endif
                else
                    wlan_sche.enable=0;

                break;
#endif//CONFIG_USER_WIRELESS_SCHEDULE

/*  Date: 2009-4-02
*   Name: Ken Chiang
*   Reason: lib upnpd will acquire too many cpu time then cause booting time too long
            so start lib upnpd by timer to avoid upnpd delay other app process , root cause still not found
*   Notice :
*/
#ifdef UPNP_ATH_LIBUPNP
        case SIGILL:
            printf("SIGILL lib upnp\n");
            if( !nvram_match("upnp_enable", "1")  ) {
                lib_upnp.enable=1;
            }
            else{
                lib_upnp.enable=0;
            }
        break;
#endif

#ifdef USE_SILEX
	case SIGQUIT:
		printf("SIGQUIT SILEX\n");
	break;
#endif

        default:
                break;
    }
}

int writepidfile(void)
{
    FILE *pid_fp;

    if (!(pid_fp = fopen(TIMER_PID, "w")))
    {
        return -1;
    }
    fprintf(pid_fp, "%d", getpid());
    fclose(pid_fp);

    return 0;
}

void check()
{
    struct timeval tv;
    struct timezone tz;
    int ret = 0;
    time_t timep;
    struct tm *p;

    gettimeofday(&tv,&tz);
    time(&timep);
    p = gmtime(&timep);

    if(http.enable)
        if(trigger_by_interval(&http))
            http.timeout_action();

    if(qos.enable)
        if(trigger_by_interval(&qos))
            qos.timeout_action();

/* jimmy modified 20081217, for add oray ddns support */
if(nvram_match("ddns_type","www.oray.cn"))
{
    if(ddns.enable)
        if(trigger_by_interval(&ddns))
            ddns.timeout_action();
}

/* -------------------------------------------------- */

    if(ntp.enable)
        if(trigger_by_interval(&ntp))
            ntp.timeout_action();

/*	Date:	2010-04-20
*	Name:	jimmy huang
*	Reason:	After booting, once we change dls from disable to enable
*			dls.schedule_action will not be triggered again (dls.done always 1)
*			ex:
*				restore_default booting -> manually set time and daylight saving simutaneously
*				-> system time is coorectty 1 hour ahead -> disable daylight saving -> system time is correct
*				-> enable daylight saving -> system time is not correctly 1 hour ahead
*/
/*
    if(dls.enable && ntp_finish)
        if(ret = trigger_by_period(&dls,tv.tv_sec))
            dls.schedule_action(ret);
*/
	if(dls.enable){
		if(last_time_dls_enable == 0){
			dls.done = 0;
		}
		last_time_dls_enable = 1;
		if(ntp_finish){
			if(ret = trigger_by_period(&dls,tv.tv_sec))
				dls.schedule_action(ret);
		}
	}else{
		if(last_time_dls_enable == 1){
			dls.done = 0;
		}
		last_time_dls_enable = 0;
	}

#ifdef CONFIG_IP_LOOKUP
    if(arpping.enable)
        if(trigger_by_interval(&arpping))
            arpping.timeout_action();
#endif

#ifdef CONFIG_USER_WIRELESS_SCHEDULE
    if(wlan_sche.enable)
        if(trigger_by_interval(&wlan_sche))
            wlan_sche.timeout_action();
#endif
/*  Date: 2009-4-02
*   Name: Ken Chiang
*   Reason: lib upnpd will acquire too many cpu time then cause booting time too long
            so start lib upnpd by timer to avoid upnpd delay other app process , root cause still not found
*   Notice :
*/

#ifdef UPNP_ATH_LIBUPNP
    if(lib_upnp.enable)
        if(trigger_by_interval(&lib_upnp))
            lib_upnp.timeout_action();
#endif

#ifdef USE_SILEX
	if(meminfo_dirty.enable)
		if(trigger_by_interval(&meminfo_dirty))
			meminfo_dirty.timeout_action();
#endif
}

int main(int argc,char* argv[])
{

    struct itimerval value;
    struct sigaction act;

    //perform check() after one second continuously
    act.sa_handler=check;
    act.sa_flags=SA_RESTART;
    sigaction(SIGALRM, &act,NULL);

    value.it_value.tv_sec = 1;
    value.it_value.tv_usec = 0;
    value.it_interval.tv_sec = 1;
    value.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &value, NULL);

    signal(SIGUSR1, timer_signal);  //NTP
    signal(SIGHUP, timer_signal);   //QOS
    signal(SIGINT, timer_signal);   //DDNS
    signal(SIGUSR2, timer_signal);  //NTP finish

#ifdef IPv6_SUPPORT
    signal(SIGPIPE,timer_signal); //sync httpd & nvram
#endif

#ifdef CONFIG_USER_WIRELESS_SCHEDULE
    signal(SIGTSTP,timer_signal); //wireless schedule
#endif

/*  Date: 2009-4-02
*   Name: Ken Chiang
*   Reason: lib upnpd will acquire too many cpu time then cause booting time too long
            so start lib upnpd by timer to avoid upnpd delay other app process , root cause still not found
*   Notice :
*/
#ifdef UPNP_ATH_LIBUPNP
    signal(SIGILL,timer_signal); //lib_upnp
#endif

#ifdef USE_SILEX
	signal(SIGQUIT, timer_signal);
#endif

    writepidfile();

    while(1)
    {
        //keep the timer daemon running
        sleep(INT_MAX);
    }
    return 0;
}
