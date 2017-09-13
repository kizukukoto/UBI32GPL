#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <shvar.h>
#include <time.h>


//#define  MAILOSD_DEBUG 1
#ifdef MAILOSD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

int send_log_by_smtp(void);

/*NickChou, call by GUI cgi*/
static void send_log_manual(int sig)
{
	int rtn_value;
	switch (sig)
	{
		case SIGUSR1:
			DEBUG_MSG("mailosd:rcv SIGUSR1\n");
			rtn_value=send_log_by_smtp();
			break;
	}
}

/*
* Name Albert Chen
* Date 2009-03-04
* Modify for independant with http, copy from http
*/
int send_log_by_smtp(void)
{
/*
    	msmtp -d test_recipient@testingsmtp.com     //recipient
        --auth=login                        		//authentication method
        --user=test                 				//username
        --host=testing.smtp.com         			//smtp server
        --from=test_sender@testingsmtp.com  		//sender
        --passwd test                     			//password
        --port => NickChou Add for setting SMTP Server Port
*/
	FILE *fp;
	char log_email_server[64]={0}, log_email_username[64]={0}, log_email_password[64]={0};
	char log_email_recipien[64]={0}, log_email_sender[64]={0}, log_email_auth[64]={0};
	char log_email_server_port[6]={0};
	char buffer[256];
	char *ptr;
	char sendCmd[300]={};

	fp=fopen(SMTP_CONF, "r");
	if(fp==NULL)
		return 0;

	while(fgets(buffer, 256, fp))
	{
		ptr = strstr(buffer, "log_email_auth=");
		if(ptr)
		{
			ptr = ptr + strlen("log_email_auth=");
			strncpy(log_email_auth, ptr, strlen(buffer)-strlen("log_email_auth=")-1);
			continue;
		}

		ptr = strstr(buffer, "log_email_recipien=");
		if(ptr)
		{
			ptr = ptr + strlen("log_email_recipien=");
			strncpy(log_email_recipien, ptr, strlen(buffer)-strlen("log_email_recipien=")-1);
			if(strlen(log_email_recipien) == 0)
				return 0;
			continue;
		}

		ptr = strstr(buffer, "log_email_username=");
		if(ptr)
		{
			ptr = ptr + strlen("log_email_username=");
			strncpy(log_email_username, ptr, strlen(buffer)-strlen("log_email_username=")-1);
			continue;
		}

		ptr = strstr(buffer, "log_email_password=");
		if(ptr)
		{
			ptr = ptr + strlen("log_email_password=");
			strncpy(log_email_password, ptr, strlen(buffer)-strlen("log_email_password=")-1);
			continue;
		}

		ptr = strstr(buffer, "log_email_server=");
		if(ptr)
		{
			ptr = ptr + strlen("log_email_server=");
			strncpy(log_email_server, ptr, strlen(buffer)-strlen("log_email_server=")-1);
			if(strlen(log_email_server) == 0)
				return 0;
			continue;
		}

		ptr = strstr(buffer, "log_email_sender=");
		if(ptr)
		{
			ptr = ptr + strlen("log_email_sender=");
			strncpy(log_email_sender, ptr, strlen(buffer)-strlen("log_email_sender=")-1);
			if(strlen(log_email_sender) == 0)
				return 0;
			continue;
		}

		ptr = strstr(buffer, "log_email_server_port=");
		if(ptr)
		{
			ptr = ptr + strlen("log_email_server_port=");
			strncpy(log_email_server_port, ptr, strlen(buffer)-strlen("log_email_server_port=")-1);
			if(strlen(log_email_server_port) == 0)
				return 0;
			continue;
		}

	}

	fclose(fp);
	DEBUG_MSG("send_log_by_smtp\n");

	if( (strcmp(log_email_auth, "1") == 0) )
	{
		sprintf(sendCmd, "msmtp %s --auth=  --user=%s  --passwd %s --host=%s --from=%s --port=%s &",
				log_email_recipien, log_email_username, log_email_password, log_email_server, log_email_sender, log_email_server_port);
		system(sendCmd);
		printf(sendCmd);
	}
	else
	{
		sprintf(sendCmd, "msmtp %s --host=%s --from=%s --port=%s &", log_email_recipien, log_email_server, log_email_sender, log_email_server_port);
		system(sendCmd);
		printf(sendCmd);
	}

	return 1;
}



//???
// 
static long get_syslog_data(char *enable,char *ip)
{
	FILE *fp;
        char *syslog_enable, *syslog_ip;

	fp =fopen ("/var/tmp/syslogd.tmp", "r");
	if(fp==NULL) {
		printf("syslogd.tmp fail\n");
		return 0;
	}
	else
	{
		char buf[256];
		buf[0] = 0;
		char *tmp_syslog_server;
                while(fgets(buf,255,fp)) {
                	
                	if(strlen(buf) > 1) { // len > 1 
                		tmp_syslog_server = buf;
                                syslog_enable = strtok(tmp_syslog_server, "/");
				syslog_ip = strtok(NULL, "/");
				
				if( syslog_enable == NULL || syslog_ip == NULL) {
					return 0;
				}
				else {
					printf(" log data:[%s][%s]\n",	syslog_enable,syslog_ip);
					strcpy(enable,syslog_enable);
					strcpy(ip,syslog_ip);
					return 1;
				}
    		
                	}
                	
                	
                	
                }
	
		
	}
	
	
	return 0;
}


void create_log_full_file(void)
{
	char *buf, *ptr, cmds[128];
	FILE *fp;
	size_t size, log_size;
	int  count = 1;
	
	char syslog_enable[64];
	char syslog_ip[64];
	long lret;
	
	

	printf("LOG_FILE full !!\n");
	sprintf(cmds, "cat %s | tac > %s", LOG_FILE_BAK, LOG_FILE_FULL);
	system(cmds);

printf(">>>[%s]\n",cmds);

	/* define max log file size at busybox/sysklogd/syslogd.c
	   .logFileSize = 200 * 1024  */
	buf = (char *) malloc(200 * 1024);
	if (buf) {
		system("killall syslogd");

		/* read log file */
		fp = fopen(LOG_FILE_BAK, "r");
		log_size = fread(buf, 1, 200 * 1024, fp);
		fclose(fp);
		printf("Read LOG_FILE size = %d\n", log_size);

		/* Reduce log file size about 30K */
		while (1) {
			ptr = buf + (LOG_REDUCE_SIZE * count * 1024);
			while (*ptr != '\n') {
				ptr++;
			}
			ptr++;   /* skip character '\n' */
			size = log_size - (ptr - buf);
			printf("Reduced LOG_FILE size = %d\n", size);
			count++;

			if (size <= LOG_MIN_SIZE * 1024)   break;
		}

		/* re-write log file */
		fp = fopen(LOG_FILE_BAK, "w");
		fwrite(ptr, 1, size, fp);
		fclose(fp);


// ??? issue
		syslog_enable[0]=0;
		syslog_ip[0]=0;

		lret = get_syslog_data(syslog_enable,syslog_ip);
		
		if( lret ) {
						
			if( strcmp(syslog_enable, "1") == 0 )
				sprintf(cmds,"syslogd -O %s -L -R %s:514 &",LOG_FILE_BAK , syslog_ip);
	                else
                                sprintf(cmds,"syslogd -O %s &",LOG_FILE_BAK);
			
		}
		else 

		sprintf(cmds, "syslogd -O %s &", LOG_FILE_BAK);
		
		printf(">>>[%s]\n",cmds);
		
		system(cmds);

		free(buf);
	}
}


int main(int argc, char* argv[])
{
	int flag_send_log_weekly=1,from=0, current=0, to=0;
	static int rtn_value=0,olddat=0,oldweekday=0,oldmonth=0;
	char *format = "%k %M %P %d %w %m %Y";
	char *p_type, *p_set_hour, *p_set_weekday, *p_log_sche_name;
	char *log_email_schedule, *type, *next;
	char *sche_name = NULL, *sche_weekdays = NULL;
	static char buf[30];
	char cur_hour[5], cur_min[5], cur_ampm[5], cur_day[5], cur_weekday[5], cur_month[5], cur_year[5];
	char tmp_set_hour[4] = {}, tmp_set_weekday[4] = {};
	char tmp_log_sche_name[20] = {}, tmp_sche_list[64] = {};
	char tmp_type[4] = {};
	char sche_start_time[10] = {}, sche_end_time[10] = {};
	char sche_start_time_hour[5] = {}, sche_start_time_min[5] = {}, sche_end_time_hour[5] = {}, sche_end_time_min[5] = {};
	time_t clock;
	struct tm *tm;

	log_email_schedule = argv[1];
	if(log_email_schedule)
	{
		if(strcmp(log_email_schedule, "manual") == 0)
		{
			/*for "send mail now button" on GUI*/
        	p_type = "manual";
		}
		else
		{
			DEBUG_MSG("log_email_schedule : %s\n", log_email_schedule);
			type = strtok(log_email_schedule, "/") ? : "";
			strcpy(tmp_type, type);
			p_type = tmp_type;
			next = strtok(NULL, "/") ? : "";
			strcpy(tmp_set_hour, next);
			p_set_hour = tmp_set_hour;
			next = strtok(NULL, "/") ? : "";
			strcpy(tmp_set_weekday, next);
			p_set_weekday = tmp_set_weekday;
			next = strtok(NULL, "/") ? : "";
			strcpy(tmp_log_sche_name, next);
			p_log_sche_name = tmp_log_sche_name;
		}
	}
	else
	{//for other customer ex:TRENNET.... do log full
		p_type = "2";
	}

	//DEBUG_MSG("p_type = %s,p_set_hour = %s\n", p_type, p_set_hour);
	if(!strcmp(p_type, "0")){
		DEBUG_MSG("onlogfull disable\n");
		if(!strcmp(p_set_hour, "25")){//if p_set_hour==25 then onschedule
			DEBUG_MSG("onschedule enable\n");
			if(strcmp("Never", p_log_sche_name) == 0 || strcmp("", p_log_sche_name) == 0){
				p_type = "nothing";
			}
		}
	}
	else if(!strcmp(p_type, "1")){
		DEBUG_MSG("onlogfull enable\n");
		if(!strcmp(p_set_hour, "25")){//if p_set_hour==25 then onschedule
			DEBUG_MSG("onschedule enable\n");
			if(strcmp("Never", p_log_sche_name) == 0 || strcmp("", p_log_sche_name) == 0){//log full
				p_type = "2";
			}
		}
		else{//log full
			DEBUG_MSG("onschedule disable\n");
			p_type = "2";
		}
	}
	DEBUG_MSG("p_type = %s,p_log_sche_name = %s\n", p_type, p_log_sche_name);

	if(argv[2])
	{
		DEBUG_MSG("sche_list = %s\n" , argv[2]);
		strcpy(tmp_sche_list, argv[2]);
		sche_name = strtok(tmp_sche_list, "/");
		DEBUG_MSG("sche_name = %s\n", sche_name);
		sche_weekdays = strtok(NULL, "/") ? : "";
		DEBUG_MSG("sche_weekdays = %s\n", sche_weekdays);
		next = strtok(NULL, "/") ? : "";
		strcpy(sche_start_time, next);
		DEBUG_MSG("sche_start_time = %s\n", sche_start_time);
		next = strtok(NULL, "/") ? : "";
		strcpy(sche_end_time, next);
		DEBUG_MSG("sche_end_time = %s\n", sche_end_time);

		if(strlen(sche_start_time) > 0)
		{
			next = strtok(sche_start_time, ":") ? : "";
			strcpy(sche_start_time_hour, next);
			DEBUG_MSG("sche_start_time_hour = %s \n", sche_start_time_hour);
			next = strtok(NULL, ":") ? : "";
			strcpy(sche_start_time_min, next);
			DEBUG_MSG("sche_start_time_min = %s \n", sche_start_time_min);
		}

		if(strlen(sche_end_time) > 0)
		{
			next = strtok(sche_end_time, ":") ? : "";
			strcpy(sche_end_time_hour, next);
			DEBUG_MSG("sche_end_time_hour = %s \n", sche_end_time_hour);
			next = strtok(NULL, ":") ? : "";
			strcpy(sche_end_time_min, next);
			DEBUG_MSG("sche_end_time_min = %s \n", sche_end_time_min);
		}
	}

	DEBUG_MSG("mailosd for-loop start\n");
	time(&clock);
	tm = gmtime(&clock);
	strftime(buf, sizeof(buf), format, tm);
	//DEBUG_MSG("\n%s\n", buf);
	sscanf(buf, "%s%s%s%s%s%s%s", cur_hour, cur_min, cur_ampm, cur_day, cur_weekday, cur_month, cur_year);
	DEBUG_MSG("cur_hour:%s,cur_min:%s,cur_ampm:%s,cur_day:%s,cur_weekday:%s\n",\
				cur_hour, cur_min, cur_ampm, cur_day, cur_weekday);
	olddat = atoi(cur_day);
	oldweekday = atoi(cur_weekday);
	oldmonth = atoi(cur_month);

	signal(SIGUSR1, send_log_manual);

	for(;;)
	{
		/* jimmy added for IPC syslog */
		{
			char command[48];
/*
 Reason: let ui show log
 Modified: John Huang
 Date: 2009.08.04
*/
			//sprintf(command,"logread | tac > %s",LOG_FILE_HTTP);
			sprintf(command,"cat %s | tac > %s", LOG_FILE_BAK, LOG_FILE_HTTP);
			system(command);
		}
		/* ------------------------------ */
		if(strcmp(p_type, "manual") == 0)
		{
			sleep(1000000);
		}
		else
		{
			time(&clock);
			tm = gmtime(&clock);
			strftime(buf, sizeof(buf), format, tm);
			//DEBUG_MSG("\n%s\n", buf);
			sscanf(buf, "%s%s%s%s%s%s%s", cur_hour, cur_min, cur_ampm, cur_day, cur_weekday, cur_month, cur_year);
			DEBUG_MSG("cur_hour:%s,cur_min:%s,cur_ampm:%s,cur_day:%s,cur_weekday:%s\nolddat:%d,oldweekday:%d,oldmonth:%d\n",\
						cur_hour, cur_min, cur_ampm, cur_day, cur_weekday, olddat, oldweekday, oldmonth);

			if( (oldweekday!=atoi(cur_weekday)) ){
				oldweekday = atoi(cur_weekday);
				printf("over day\n");
				rtn_value = 0;
			}
			/* Ken modify for Uibcome log behavior */
			/* send logs according to type variable
				0 -> on schedule
				1 -> when log is full or on schedule
				2 -> log is full
				3 -> hourly,
				4 -> daily,
				5 -> weekly

			*/

			if(!strcmp(p_type, "0"))//on schedule
			{
				struct stat file_buf;

				DEBUG_MSG("type : 0\n");
				if( !stat(LOG_FILE_HTTP, &file_buf) )
				{
					DEBUG_MSG("LOG_FILE_HTTP Size = %d Bytes\n", file_buf.st_size);
					if( file_buf.st_size){
						if(strcmp(sche_name, p_log_sche_name) == 0)
						{
							DEBUG_MSG("send log email on schedule %s\n", p_log_sche_name);
							//DEBUG_MSG("sche_weekdays = %s, cur_weekday = %s\n", sche_weekdays, cur_weekday);
							//DEBUG_MSG("sche_weekdays + cur_weekday = %s\n", sche_weekdays + atoi(cur_weekday));
							//DEBUG_MSG("sche_start_time_hour = %s, sche_start_time_min = %s\n", sche_start_time_hour, sche_start_time_min );
							//DEBUG_MSG("sche_end_time_hour = %s, sche_end_time_min = %s\n", sche_end_time_hour, sche_end_time_min );
							if ( *(sche_weekdays + atoi(cur_weekday)) == '1')
							{
								DEBUG_MSG("send log mail today\n");
								if(strcmp(sche_start_time_hour, sche_end_time_hour) == 0 &&
									strcmp(sche_start_time_min, sche_end_time_min) == 0)
								{
									printf("sche_start_time = sche_end_time\n");
									DEBUG_MSG("rtn_value : %d\n", rtn_value);
									if(!rtn_value)
										rtn_value = send_log_by_smtp();
								}
								else
								{
									//DEBUG_MSG("cur_ampm =%s, cur_hour = %d, cur_min = %d\n", cur_ampm, atoi(cur_hour), atoi(cur_min));
									//DEBUG_MSG("sche_start_time_hour = %d, sche_end_time_hour = %d\n", atoi(sche_start_time_hour), atoi(sche_end_time_hour));
									//DEBUG_MSG("sche_start_time_min = %d, sche_end_time_min = %d\n", atoi(sche_start_time_min), atoi(sche_end_time_min));
									from = atoi(sche_start_time_hour)*60 + atoi(sche_start_time_min);
									to  = atoi(sche_end_time_hour)*60 + atoi(sche_end_time_min);
									current = atoi(cur_hour)*60 + atoi(cur_min);
									DEBUG_MSG("%d < %d < %d\n", from, current, to);

									/*Michael jhong modify send logs every hours */
									if ((current %60 == 0) && (current >= from) && (current <= to))
									{
										 DEBUG_MSG("send log mail every hours\n");
										 rtn_value = 0;

									}
									else if( (current >= from) && (current <= to) )
									{
										DEBUG_MSG("send log mail now rtn_value : %d\n", rtn_value);
										if(!rtn_value)
											rtn_value = send_log_by_smtp();
									}
									else{
										DEBUG_MSG("leave schedule\n");
										rtn_value = 0;
									}
								}
							}
							else{
								DEBUG_MSG("leave schedule\n");
								rtn_value = 0;
							}
						}
					}
				}
				else
					DEBUG_MSG("mailosd: check LOG_FILE_HTTP error\n");
			}
			else if(!strcmp(p_type, "1"))//when log is full or on schedule
			{
				struct stat file_buf;
				DEBUG_MSG("type : 1\n");

				if (stat(LOG_FILE_BAK, &file_buf)) {
					printf("LOG_FILE %s does not exist\n", LOG_FILE_BAK);
					rtn_value = 0;
					return 1;
				}
				DEBUG_MSG("LOG_FILE Size = %d KB\n", file_buf.st_size/1024);

				// check log full
				if (file_buf.st_size > LOG_MAX_SIZE * 1024) {
					create_log_full_file();
				}

				/* Send mail when the log full file is existed */
				if (!stat(LOG_FILE_FULL, &file_buf)) {
					rtn_value = send_log_by_smtp();
					DEBUG_MSG("rtn_value : %d\n", rtn_value);
					if (rtn_value == 1) {
						sleep(50);
					}
					return 1;
				}

				// check on schedule
				if (strcmp(sche_name, p_log_sche_name) == 0)
				{
					DEBUG_MSG("send log email on schedule %s\n", p_log_sche_name);
					//DEBUG_MSG("sche_weekdays = %s, cur_weekday = %s\n", sche_weekdays, cur_weekday);
					//DEBUG_MSG("sche_weekdays + cur_weekday = %s\n", sche_weekdays + atoi(cur_weekday));
					//DEBUG_MSG("sche_start_time_hour = %s, sche_start_time_min = %s\n", sche_start_time_hour, sche_start_time_min );
					//DEBUG_MSG("sche_end_time_hour = %s, sche_end_time_min = %s\n", sche_end_time_hour, sche_end_time_min );
					if ( *(sche_weekdays + atoi(cur_weekday)) == '1')
					{
						DEBUG_MSG("send log mail today\n");
						if (strcmp(sche_start_time_hour, sche_end_time_hour) == 0 &&
							strcmp(sche_start_time_min, sche_end_time_min) == 0)
						{
							printf("sche_start_time = sche_end_time\n");
							DEBUG_MSG("rtn_value : %d\n", rtn_value);
							if (!rtn_value)
								rtn_value = send_log_by_smtp();
						}
						else
						{
							//DEBUG_MSG("cur_ampm =%s, cur_hour = %d, cur_min = %d\n", cur_ampm, atoi(cur_hour), atoi(cur_min));
							//DEBUG_MSG("sche_start_time_hour = %d, sche_end_time_hour = %d\n", atoi(sche_start_time_hour), atoi(sche_end_time_hour));
							//DEBUG_MSG("sche_start_time_min = %d, sche_end_time_min = %d\n", atoi(sche_start_time_min), atoi(sche_end_time_min));
							from = atoi(sche_start_time_hour)*60 + atoi(sche_start_time_min);
							to  = atoi(sche_end_time_hour)*60 + atoi(sche_end_time_min);
							current = atoi(cur_hour)*60 + atoi(cur_min);
							DEBUG_MSG("%d < %d < %d\n", from, current, to);

      						if ((current %60 == 0) && (current >= from) && (current <= to) )
      						{
      							DEBUG_MSG("send log mail every hours\n");
      							rtn_value = 0;
      						}
							else if( (current >= from) && (current <= to) )
							{
								DEBUG_MSG("send log mail now rtn_value : %d\n", rtn_value);
								if (!rtn_value)
									rtn_value = send_log_by_smtp();
							}
							else {
								DEBUG_MSG("leave schedule\n");
								rtn_value = 0;
							}
						}
					}
					else{
						DEBUG_MSG("leave schedule\n");
						rtn_value = 0;
					}
				}
			}
			else if (!strcmp(p_type, "2"))   //log is full
			{
				DEBUG_MSG("type : 2\n");

				struct stat file_buf;
				if (stat(LOG_FILE_BAK, &file_buf)) {
					printf("LOG_FILE %s does not exist\n", LOG_FILE_BAK);
					rtn_value = 0;
					return 1;
				}

				DEBUG_MSG("LOG_FILE Size = %d KB\n", file_buf.st_size/1024);
				if (file_buf.st_size > LOG_MAX_SIZE * 1024) {
					create_log_full_file();
				}

				/* Send mail when the log full file is existed */
				if (!stat(LOG_FILE_FULL, &file_buf)) {
					rtn_value = send_log_by_smtp();
					DEBUG_MSG("rtn_value : %d\n", rtn_value);
					if (rtn_value == 1) {
						sleep(50);
					}
				}
			}
			else if(!strcmp(p_type, "3"))//hourly
			{
				DEBUG_MSG("type : 3\n");
				DEBUG_MSG("cur_hour : %s cur_min : %s\n", cur_hour, cur_min);
//				DEBUG_MSG("set_hour : %s set_min : %s\n", set_hour, set_min);
				if (strcmp("00", cur_min) == 0)
				{
					DEBUG_MSG("hourly works!\n");
					rtn_value = send_log_by_smtp();
					DEBUG_MSG("rtn_value : %d\n", rtn_value);
					if (rtn_value == 1)
						sleep(61);
				}
			}
			else if(!strcmp(p_type, "4"))//daily
			{
				DEBUG_MSG("type : 4\n");
				DEBUG_MSG("cur_day : %s cur_hour : %s cur_min : %s\n", cur_day, cur_hour, cur_min);
//				DEBUG_MSG("set_day : %s p_set_hour : %s\n", set_day, p_set_hour);
				if ((strcmp(p_set_hour, cur_hour)==0) && (strcmp("00", cur_min)==0))
				{
					DEBUG_MSG("daily works!\n");
					rtn_value = send_log_by_smtp();
					DEBUG_MSG("rtn_value : %d\n", rtn_value);
					if (rtn_value == 1)
						sleep(61);
				}
			}
			else if(!strcmp(p_type, "5"))//weekly
			{
				DEBUG_MSG("type : 5\n");
				DEBUG_MSG("cur_weekday : %s cur_hour : %s\n", cur_weekday, cur_hour);
				DEBUG_MSG("p_set_weekday : %s p_set_hour : %s\n", p_set_weekday, p_set_hour);
				if (((atoi(p_set_weekday)) == (atoi(cur_weekday))) && (strcmp(p_set_hour, cur_hour)==0)) {
					if (flag_send_log_weekly == 1) {
						DEBUG_MSG("flag_send_log_weekly : %d\n", flag_send_log_weekly);
						DEBUG_MSG("weekly is work, send mail\n");
						rtn_value = send_log_by_smtp();
						DEBUG_MSG("rtn_value : %d\n", rtn_value);
						if (rtn_value == 1)
							sleep(61);
					}
					if (rtn_value) {
						if (!strcmp(p_set_hour, cur_hour))
							flag_send_log_weekly = 0;
						else
							flag_send_log_weekly = 1;
					}
				}
				else if (((!strcmp(p_set_weekday, "0")) && (!strcmp(cur_weekday, "7"))) && (!strcmp(p_set_hour, cur_hour))) {
					if (flag_send_log_weekly == 1){
						DEBUG_MSG("flag_send_log_weekly : %d\n", flag_send_log_weekly);
						DEBUG_MSG("weekly is work on sunday, send mail\n");
						rtn_value = send_log_by_smtp();
						DEBUG_MSG("rtn_value : %d\n", rtn_value);
						if (rtn_value == 1)
							sleep(61);
					}
					if (rtn_value) {
						if (!strcmp(p_set_hour, cur_hour))
							flag_send_log_weekly = 0;
						else
							flag_send_log_weekly = 1;
					}
				}
			}

			sleep(10);
		}
	}

	return 1;
}
