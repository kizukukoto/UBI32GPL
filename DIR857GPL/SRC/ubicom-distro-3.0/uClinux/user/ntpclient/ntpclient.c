/*
 * ntpclient.c - NTP client
 *
 * Copyright 1997, 1999, 2000, 2003  Larry Doolittle  <larry@doolittle.boa.org>
 * Last hack: July 5, 2003
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License (Version 2,
 *  June 1991) as published by the Free Software Foundation.  At the
 *  time of writing, that license was published by the FSF with the URL
 *  http://www.gnu.org/copyleft/gpl.html, and is incorporated herein by
 *  reference.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Possible future improvements:
 *      - Double check that the originate timestamp in the received packet
 *        corresponds to what we sent.
 *      - Verify that the return packet came from the host we think
 *        we're talking to.  Not necessarily useful since UDP packets
 *        are so easy to forge.
 *      - Write more documentation  :-(
 *
 *  Compile with -D_PRECISION_SIOCGSTAMP if your machine really has it.
 *  There are patches floating around to add this to Linux, but
 *  usually you only get an answer to the nearest jiffy.
 *  Hint for Linux hacker wannabes: look at the usage of get_fast_time()
 *  in net/core/dev.c, and its definition in kernel/time.c .
 *
 *  If the compile gives you any flak, check below in the section
 *  labelled "XXXX fixme - non-automatic build configuration".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>     /* gethostbyname */
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <shvar.h>
#include <signal.h>
#ifdef _PRECISION_SIOCGSTAMP
#include <sys/ioctl.h>
#endif

#define ENABLE_DEBUG

extern char *optarg;

/* XXXX fixme - non-automatic build configuration */
#ifdef linux
#include <sys/utsname.h>
#include <sys/time.h>
typedef u_int32_t __u32;
#include <sys/timex.h>
#else
extern struct hostent *gethostbyname(const char *name);
extern int h_errno;
#define herror(hostname) \
	fprintf(stderr,"Error %d looking up hostname %s\n", h_errno,hostname)
typedef uint32_t __u32;
#endif

#ifdef NTP_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define JAN_1970        0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */
#define NTP_PORT (123)

/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via settimeofday) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

/* Converts NTP delay and dispersion, apparently in seconds scaled
 * by 65536, to microseconds.  RFC1305 states this time is in seconds,
 * doesn't mention the scaling.
 * Should somehow be the same as 1000000 * x / 65536
 */
#define sec2u(x) ( (x) * 15.2587890625 )

struct ntptime {
	unsigned int coarse;
	unsigned int fine;
};

/* prototype for function defined in phaselock.c */
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);

/* prototypes for some local routines */
void send_packet(int usd);
int rfc1305print(uint32_t *data, struct ntptime *arrival);
void udp_handle(int usd, char *data, int data_len, struct sockaddr *sa_source, int sa_len);

/* variables with file scope
 * (I know, bad form, but this is a short program) */
static uint32_t incoming_word[325];
#define incoming ((char *) incoming_word)
#define sizeof_incoming (sizeof(incoming_word)*sizeof(uint32_t))
static struct timeval time_of_send;
static int live=0;
static int set_clock=0;   /* non-zero presumably needs root privs */
static int Daylight=0;  //The daylight function that it's used the option for sl3516. 

/* when present, debug is a true global, shared with phaselock.c */
#ifdef ENABLE_DEBUG
int debug=0;
#define DEBUG_OPTION "d"
#else
#define debug 0
#define DEBUG_OPTION
#endif

#if 1 //The daylight function that it's used the option for sl3516.
#define EXCEPTION_ARRAY_SIZE 2

float timezone_daylight_savings_exception[EXCEPTION_ARRAY_SIZE] =
                {-82   /* timezone is indiana*/ ,
                 -112  /* timezone is Arizona */};

unsigned int year_month_days[2][13] = {
	{ 365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    { 366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};
#endif

int get_current_freq(void)
{
	/* OS dependent routine to get the current value of clock frequency.
	 */
#ifdef linux
	struct timex txc;
	txc.modes=0;
	if (__adjtimex(&txc) < 0) {
		perror("adjtimex"); exit(1);
	}
	return txc.freq;
#else
	return 0;
#endif
}

int set_freq(int new_freq)
{
	/* OS dependent routine to set a new value of clock frequency.
	 */
#ifdef linux
	struct timex txc;
	txc.modes = ADJ_FREQUENCY;
	txc.freq = new_freq;
	if (__adjtimex(&txc) < 0) {
		perror("adjtimex"); exit(1);
	}
	return txc.freq;
#else
	return 0;
#endif
}

void send_packet(int usd)
{
	__u32 data[12];
	struct timeval now;
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4 
#define PREC -6

	if (debug) fprintf(stderr,"Sending ...\n");
	if (sizeof(data) != 48) {
		fprintf(stderr,"size error\n");
		return;
	}
	bzero((char *) data,sizeof(data));
	data[0] = htonl (
		( LI << 30 ) | ( VN << 27 ) | ( MODE << 24 ) |
		( STRATUM << 16) | ( POLL << 8 ) | ( PREC & 0xff ) );
	data[1] = htonl(1<<16);  /* Root Delay (seconds) */
	data[2] = htonl(1<<16);  /* Root Dispersion (seconds) */
	gettimeofday(&now,NULL);
	data[10] = htonl(now.tv_sec + JAN_1970); /* Transmit Timestamp coarse */
	data[11] = htonl(NTPFRAC(now.tv_usec));  /* Transmit Timestamp fine   */
	send(usd,data,48,0);
	time_of_send=now;
}

void get_packet_timestamp(int usd, struct ntptime *udp_arrival_ntp)
{
	struct timeval udp_arrival;
#ifdef _PRECISION_SIOCGSTAMP
	if ( ioctl(usd, SIOCGSTAMP, &udp_arrival) < 0 ) {
		perror("ioctl-SIOCGSTAMP");
		gettimeofday(&udp_arrival,NULL);
	}
#else
	gettimeofday(&udp_arrival,NULL);
#endif
	udp_arrival_ntp->coarse = udp_arrival.tv_sec + JAN_1970;
	udp_arrival_ntp->fine   = NTPFRAC(udp_arrival.tv_usec);
}

void check_source(int data_len, struct sockaddr *sa_source, int sa_len)
{
	/* This is where one could check that the source is the server we expect */
	if (debug) {
		struct sockaddr_in *sa_in=(struct sockaddr_in *)sa_source;
		printf("packet of length %d received\n",data_len);
		if (sa_source->sa_family==AF_INET) {
			printf("Source: INET Port %d host %s\n",
				ntohs(sa_in->sin_port),inet_ntoa(sa_in->sin_addr));
		} else {
			printf("Source: Address family %d\n",sa_source->sa_family);
		}
	}
}

double ntpdiff( struct ntptime *start, struct ntptime *stop)
{
	int a;
	unsigned int b;
	a = stop->coarse - start->coarse;
	if (stop->fine >= start->fine) {
		b = stop->fine - start->fine;
	} else {
		b = start->fine - stop->fine;
		b = ~b;
		a -= 1;
	}
	
	return a*1.e6 + b * (1.e6/4294967296.0);
}

/* Does more than print, so this name is bogus.
 * It also makes time adjustments, both sudden (-s)
 * and phase-locking (-l).  */
/* return value is number of microseconds uncertainty in answer */
int rfc1305print(uint32_t *data, struct ntptime *arrival)
{
/* straight out of RFC-1305 Appendix A */
	int li, vn, mode, stratum, poll, prec;
	int delay, disp, refid;
	struct ntptime reftime, orgtime, rectime, xmttime;
	double el_time,st_time,skew1,skew2;
	int freq;

#define Data(i) ntohl(((uint32_t *)data)[i])
	li      = Data(0) >> 30 & 0x03;
	vn      = Data(0) >> 27 & 0x07;
	mode    = Data(0) >> 24 & 0x07;
	stratum = Data(0) >> 16 & 0xff;
	poll    = Data(0) >>  8 & 0xff;
	prec    = Data(0)       & 0xff;
	if (prec & 0x80) prec|=0xffffff00;
	delay   = Data(1);
	disp    = Data(2);
	refid   = Data(3);
	reftime.coarse = Data(4);
	reftime.fine   = Data(5);
	orgtime.coarse = Data(6);
	orgtime.fine   = Data(7);
	rectime.coarse = Data(8);
	rectime.fine   = Data(9);
	xmttime.coarse = Data(10);
	xmttime.fine   = Data(11);
#undef Data

	if (set_clock) {   /* you'd better be root, or ntpclient will crash! */
		struct timeval tv_set;
		/* it would be even better to subtract half the slop */
		tv_set.tv_sec  = xmttime.coarse - JAN_1970;
		/* divide xmttime.fine by 4294.967296 */
		tv_set.tv_usec = USEC(xmttime.fine);
		if (settimeofday(&tv_set,NULL)<0) {
			perror("settimeofday");
			exit(1);
		}
		if (debug)
			printf("NTP: set time to %lu.%.6lu\n", tv_set.tv_sec, tv_set.tv_usec);
	}

	if (debug) 
	{
		printf("LI=%d  VN=%d  Mode=%d  Stratum=%d  Poll=%d  Precision=%d\n",
				li, vn, mode, stratum, poll, prec);
		printf("Delay=%.1f  Dispersion=%.1f  Refid=%u.%u.%u.%u\n",
			sec2u(delay),sec2u(disp),refid>>24&0xff, refid>>16&0xff, refid>>8&0xff, refid&0xff);
		printf("Reference %u.%.10u\n", reftime.coarse, reftime.fine);
		printf("Originate %u.%.10u\n", orgtime.coarse, orgtime.fine);
		printf("Receive   %u.%.10u\n", rectime.coarse, rectime.fine);
		printf("Transmit  %u.%.10u\n", xmttime.coarse, xmttime.fine);
		printf("Our recv  %u.%.10u\n", arrival->coarse, arrival->fine);
	}
	el_time=ntpdiff(&orgtime,arrival);   /* elapsed */
	st_time=ntpdiff(&rectime,&xmttime);  /* stall */
	skew1=ntpdiff(&orgtime,&rectime);
	skew2=ntpdiff(&xmttime,arrival);
	freq=get_current_freq();
	if (debug) 
	{
		printf("Total elapsed: %9.2f\n"
	       		"Server stall:  %9.2f\n"
	       		"Slop:          %9.2f\n",
				el_time, st_time, el_time-st_time);
		printf("Skew:          %9.2f\n"
	       		"Frequency:     %9d\n"
	       		" day   second     elapsed    stall     skew  dispersion  freq\n",
				(skew1-skew2)/2, freq);
	}
	/* Not the ideal order for printing, but we want to be sure
	 * to do all the time-sensitive thinking (and time setting)
	 * before we start the output, especially fflush() (which
	 * could be slow).  Of course, if debug is turned on, speed
	 * has gone down the drain anyway. */
	if (live) {
		int new_freq;
		new_freq = contemplate_data(arrival->coarse, (skew1-skew2)/2,
									el_time+sec2u(disp), freq);
		if (!debug && new_freq != freq) 
			set_freq(new_freq);
	}
#if 0
	printf("%d %.5d.%.3d  %8.1f %8.1f  %8.1f %8.1f %9d\n",
		arrival->coarse/86400, arrival->coarse%86400,
		arrival->fine/4294967, el_time, st_time,
		(skew1-skew2)/2, sec2u(disp), freq);
#endif
	fflush(stdout);
	return(el_time-st_time);
}

void stuff_net_addr(struct in_addr *p, char *hostname)
{
	struct hostent *ntpserver;
	
	ntpserver=gethostbyname(hostname);
	if (ntpserver == NULL) {
		herror(hostname);
		exit(1);
	}
	if (ntpserver->h_length != 4) {
		fprintf(stderr,"oops %d\n",ntpserver->h_length);
		exit(1);
	}
	memcpy(&(p->s_addr),ntpserver->h_addr_list[0],4);
}

void setup_receive(int usd, unsigned int interface, short port)
{
	struct sockaddr_in sa_rcvr;
	bzero((char *) &sa_rcvr, sizeof(sa_rcvr));
	sa_rcvr.sin_family=AF_INET;
	sa_rcvr.sin_addr.s_addr=htonl(interface);
	sa_rcvr.sin_port=htons(port);
	if(bind(usd,(struct sockaddr *) &sa_rcvr,sizeof(sa_rcvr)) == -1) {
		fprintf(stderr,"could not bind to udp port %d\n",port);
		perror("bind");
		exit(1);
	}
	listen(usd,3);
}

void setup_transmit(int usd, char *host, short port)
{
	struct sockaddr_in sa_dest;
	bzero((char *) &sa_dest, sizeof(sa_dest));
	sa_dest.sin_family=AF_INET;
	stuff_net_addr(&(sa_dest.sin_addr),host);
	sa_dest.sin_port=htons(port);
	if (connect(usd,(struct sockaddr *)&sa_dest,sizeof(sa_dest))==-1)
		{perror("connect");exit(1);}
}

int adjust_timezone(int time_zone)
{
	struct timeval tv;
    struct timezone tz;	
	
	printf("(adjust_timezone)time_zone=%d\n", time_zone);	    
	gettimeofday(&tv,&tz);  	
	tv.tv_sec = tv.tv_sec+ (time_zone/8)*1800;   	
	settimeofday(&tv,&tz);
}

#if 1 //The daylight function that it's used the option for sl3516.
static int daylight_adjflag=0;	   	
int adjust_timezone_daylight(int time_zone)
{	
	int i,check_flag=0;
	int start_sday_of_week, end_sday_of_week;
    int start_week = 0, end_week = 0;
	struct timeval tv;
    struct timezone tz;
    struct tm tm_start, tm_end;
    struct tm *p_start, *p_end, *p_cur;
	char wday_start_mon_1st, wday_end_mon_1st; // The weekday of the 1th day in both start/end DLS months
    char days2target_start_day, days2target_end_day;
	time_t time_start_mon_1st, time_end_mon_1st; // The 1st day of start/end month of daylight saving time
    time_t start_time, end_time, cur_time;
	FILE *fp;
	char buf[28], *pos;
	char daylight_saving_enable[2]={0};
	

	printf("%s:time_zone=%d\n",__func__,time_zone);
	
	// time zone
	if (debug) 
			printf("adjust time_zone\n");
	gettimeofday(&tv,&tz);
    tv.tv_sec = tv.tv_sec+ (time_zone/8)*1800;
    settimeofday(&tv,&tz);
    
	fp = fopen("/var/tmp/daylight_saving.conf", "r");
	if (fp == NULL) 
	{
		printf("Could not open /var/tmp/daylight_saving.conf\n");
		return 0;
	}
	
	while (fgets(buf, sizeof(buf), fp) != NULL ) 
	{
		pos = buf;
		while (*pos != '\0'){
			if (*pos == '\n'){
				*pos = '\0';
				break;
			}
			pos++;
		}
		pos = strchr(buf, '=');
		
		if (pos == NULL){
			printf("daylight_saving.conf: invalid line '%s'\n", buf);
			continue;
		}
		pos++;
		
		if(strstr(buf, "enable"))			
			strncpy(daylight_saving_enable, pos, strlen(pos));					
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
		//else if(strstr(buf, "zone"))
			//time_zone = atoi(pos);	
	}	
	fclose(fp);

	
	if (debug) 
			printf("daylight_saving_enable=%s\n",daylight_saving_enable);
	if( strcmp(daylight_saving_enable, "1") == 0 )
 	{
	    if (debug)                    
	    	printf("daylight_saving_enable\n");	 
		p_cur = localtime(&tv.tv_sec);		
		// START Time
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

 		if (debug) {
 			printf("\n,start Year:%d,Month:%d,Day:%d,Hour:%d\n",tm_start.tm_year+1900,tm_start.tm_mon+1,tm_start.tm_mday,tm_start.tm_hour);	
 			printf("\n,end Year:%d,Month:%d,Day:%d,Hour:%d\n",tm_end.tm_year+1900,tm_end.tm_mon+1,tm_end.tm_mday,tm_end.tm_hour);	
			//syslog(LOG_INFO,"\n,Year:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	
		}	
 	
	    if (debug)                      
	    	printf("Adjust daylight\n");
 		 
		start_time = mktime(&tm_start);                            
		end_time = mktime(&tm_end); 
		
    	while(1){ 				 
 			// Get Secs				        
			gettimeofday(&tv,&tz);
	    	cur_time = tv.tv_sec;
			      
    		if (start_time <= end_time)
    		{
        		//if (debug)                                 
 					//printf("s<=e:");
        		/* check the timezone */
				for(i=0; i < EXCEPTION_ARRAY_SIZE;i++)
				{
					if(time_zone == timezone_daylight_savings_exception[i])
					{
        	             check_flag = 1;
        	             break;
        	        }
        	    }
        		if ((cur_time >= start_time) && (cur_time <= end_time))
        	    {                
        	       	if(check_flag == 0 && !daylight_adjflag){
        	       		if (debug)                                 
 							printf("+1\n");
        				tv.tv_sec += 3600;
        				start_time += 3600;
        				end_time += 3600;
        				daylight_adjflag=1;
        			}	
    			} 
    			else{
    				if(check_flag == 0 && daylight_adjflag){
    					if (debug)                                 
 							printf("-1\n");
        				tv.tv_sec -= 3600;
        				start_time -= 3600;
        				end_time -= 3600;
        				daylight_adjflag=0;
        			}	
    			}	
    		} 
    		else
    		{
        		//if (debug)                                 
 					//printf("s>e:");
        		/* check the timezone */
				for(i=0; i < EXCEPTION_ARRAY_SIZE;i++)
				{
					if(time_zone == timezone_daylight_savings_exception[i])
					{
        	               check_flag = 1;
        	               break;
        	         }
        	    }        	
        		if ((cur_time <= end_time) || (cur_time >= start_time))
        	    {
        	        if(check_flag == 0 && !daylight_adjflag){
        	        	if (debug)                                 
 							printf("+1\n");
        				tv.tv_sec += 3600;
        				start_time += 3600;
        				end_time += 3600;
        				daylight_adjflag=1;
        			}	
    			}
    			else{
    				if(check_flag == 0 && daylight_adjflag){
    					if (debug)                                 
 							printf("-1\n");
        				tv.tv_sec -= 3600;
        				start_time -= 3600;
        				end_time -= 3600;
        				daylight_adjflag=0;
        			}	
    			}		
    		}	
    		settimeofday(&tv, &tz);	
    		sleep(1);	
    	}       
	}    
	return 0;
}
#endif

int read_pid(char *file)
{
	FILE *pidfile;
	char pid[20];
	pidfile = fopen(file, "r");
	if(pidfile) {
		fgets(pid,20, pidfile);
		fclose(pidfile);
	} else
		return -1;	
	return atoi(pid);
}

void primary_loop(int usd, int num_probes, int interval, int goodness,int time_zone)
{
	fd_set fds;
	struct sockaddr sa_xmit;
	int i, pack_len, sa_xmit_len, probes_sent, error;
	struct timeval to;
	struct ntptime udp_arrival_ntp;
	int pid;

	if (debug) 
		printf("primary_loop:\n");

	probes_sent=0;
	sa_xmit_len=sizeof(sa_xmit);
	to.tv_sec=0;
	to.tv_usec=0;
	for (;;) {
		if (debug) 
			printf("primary_loop:listening...\n");
		FD_ZERO(&fds);
		FD_SET(usd,&fds);
		i=select(usd+1,&fds,NULL,NULL,&to);  /* Wait on read or error */
		if ((i!=1)||(!FD_ISSET(usd,&fds))) {
			if (i==EINTR) 
				continue;
			if (i<0) 
				perror("select");
			if ( to.tv_sec == 0 ) {
				if (probes_sent >= num_probes && num_probes != 0){ 
					if (debug) 
						printf("primary_loop:probes_sent=%d >= num_probes=%d\n",probes_sent ,num_probes);
					break;
				}	
				send_packet(usd);
				++probes_sent;
				to.tv_sec=interval;
				to.tv_usec=0;
			}	
			continue;
		}
		pack_len=recvfrom(usd,incoming,sizeof_incoming,0,&sa_xmit,&sa_xmit_len);
		error = goodness+1;
		if (pack_len<0) {
			perror("recvfrom");
		} else if (pack_len>0 && (unsigned)pack_len<sizeof_incoming){
			get_packet_timestamp(usd, &udp_arrival_ntp);
			check_source(pack_len, &sa_xmit, sa_xmit_len);
			error = rfc1305print(incoming_word, &udp_arrival_ntp);
			/* udp_handle(usd,incoming,pack_len,&sa_xmit,sa_xmit_len); */

			/* Adjust timezone and daylight saving */
			if(Daylight)
				adjust_timezone_daylight(time_zone);
			else	
				adjust_timezone(time_zone); /*move to rc_ntp.c*/

		} else {
			printf("Ooops. pack_len=%d\n",pack_len);
			fflush(stdout);
		}
		if ( error < goodness && goodness != 0){ 
			if (debug) 
				printf("primary_loop:error=%d < goodness=%d\n",error , goodness);
			break;
		}	
		if (probes_sent >= num_probes && num_probes != 0){
			 if (debug) 
				printf("primary_loop:probes_sent=%d >= num_probes=%d end\n",probes_sent ,num_probes);
			 break;
		}	 
	}
	
	//signal to timer
	if (debug) 
		printf("primary_loop:signal to timer\n");
	pid = read_pid(TIMER_PID);
	if((pid > 0) && !kill(pid, 0))
		kill(pid, SIGUSR2);
		
}

void do_replay(void)
{
	char line[100];
	int n, day, freq, absolute;
	float sec, el_time, st_time, disp;
	double skew, errorbar;
	int simulated_freq = 0;
	unsigned int last_fake_time = 0;
	double fake_delta_time = 0.0;

	while (fgets(line,sizeof(line),stdin)) {
		n=sscanf(line,"%d %f %f %f %lf %f %d",
			&day, &sec, &el_time, &st_time, &skew, &disp, &freq);
		if (n==7) {
			fputs(line,stdout);
			absolute=day*86400+(int)sec;
			errorbar=el_time+disp;
			if (debug) printf("contemplate %u %.1f %.1f %d\n",
				absolute,skew,errorbar,freq);
			if (last_fake_time==0) simulated_freq=freq;
			fake_delta_time += (absolute-last_fake_time)*((double)(freq-simulated_freq))/65536;
			if (debug) printf("fake %f %d \n", fake_delta_time, simulated_freq);
			skew += fake_delta_time;
			freq = simulated_freq;
			last_fake_time=absolute;
			simulated_freq = contemplate_data(absolute, skew, errorbar, freq);
		} else {
			fprintf(stderr,"Replay input error\n");
			exit(2);
		}
	}
}

void usage(char *argv0)
{
	fprintf(stderr,
	"Usage: %s [-c count] [-d] [-g goodness] -h hostname [-i interval]\n"
	"\t[-l] [-p port] [-r] [-s] [-D]\n",
	argv0);
}

int main(int argc, char *argv[]) {
	int usd;  /* socket */
	int c;
	/* These parameters are settable from the command line
	   the initializations here provide default behavior */
	short int udp_local_port=0;   /* default of 0 means kernel chooses */
	int cycle_time=600;           /* seconds */
	int probe_count=0;            /* default of 0 means loop forever */
	/* int debug=0; is a global above */
	int goodness=0;
	char *hostname=NULL;          /* must be set */
	int replay=0;                 /* replay mode overrides everything */
	int time_zone =0;

	for (;;) {
		c = getopt( argc, argv, "c:" DEBUG_OPTION "g:h:i:lp:rst:D");
		if (c == EOF) break;
		switch (c) {
			case 'c':
				probe_count = atoi(optarg);
				break;
#ifdef ENABLE_DEBUG
			case 'd':
				++debug;
				break;
#endif
			case 'g':
				goodness = atoi(optarg);
				break;
			case 'h':
				hostname = optarg;
				break;
			case 'i':
				cycle_time = atoi(optarg);
				break;
			case 'l': 
				live++;
				break;
			case 'p':
				udp_local_port = atoi(optarg);
				break;
			case 'r':
				replay++;
				break;
			case 's':
				set_clock++;
				probe_count = 1;
				break;
			case 't':
				time_zone = atoi(optarg);
				break;
			case 'D'://The daylight function that it's used the option for sl3516. 
				Daylight++;
				break;	
			default:
				usage(argv[0]);
				exit(1);
		}
	}
	if (replay) {
		if (debug) {
			printf("do_replay\n");
		}	
		do_replay();
		exit(0);
	}
	if (hostname == NULL) {
		usage(argv[0]);
		exit(1);
	}
	if (debug) {
		printf("Configuration:\n"
		"  -c probe_count %d\n"
		"  -d (debug)     %d\n"
		"  -g goodness    %d\n"
		"  -h hostname    %s\n"
		"  -i interval    %d\n"
		"  -l live        %d\n"
		"  -p local_port  %d\n"
		"  -s set_clock   %d\n"
		"  -t time_zone   %d\n"
		"  -D Daylight    %d\n",
		probe_count, debug, goodness, hostname, cycle_time,
		live, udp_local_port, set_clock ,time_zone ,Daylight);
	}

	/* Adjust timezone anyway */
	//adjust_timezone();
	/* Startup sequence */
	if ((usd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1){
		perror ("socket");
		exit(1);
	}

	setup_receive(usd, INADDR_ANY, udp_local_port);

	setup_transmit(usd, hostname, NTP_PORT);

	primary_loop(usd, probe_count, cycle_time, goodness, time_zone);

	close(usd);
	return 0;
}
