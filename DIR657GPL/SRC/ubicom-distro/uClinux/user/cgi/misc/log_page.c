#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define LOG_FILE	"/var/log/messages"
#define TEMP_FILE	"/tmp/log_temp"
#define LOG_AWK		"/bin/log_awk"
#define MAX_TEMP_LOG    3000   /* /tmp/log_temp file size max 16KBytes */
#define LINE_PAGE	20
//#define debug

#ifndef debug
#include "ssi.h"
#include "querys.h"
#include "log.h"
#endif

/* ====== Sub Function ====== */
static int total_page_cnt(int count)
{

	div_t answer;

	answer = div(count, LINE_PAGE);
	if (answer.rem != 0 && answer.quot >= 0)
		answer.quot++;
	if (answer.quot == 0)
		answer.quot = 1;
	//printf("%d", answer.quot);
	return answer.quot;
}

/* to do format */
static void str_trans(char *s, char *t)
{
	struct {
		char from;
		char *to;
	} TRAN[] = {
		{'"', "&#34;"},
		{'\'', "&#39;"},
	};
	int l = strlen(s);
	int i, j;

	for (i = 0; i < l; i++) {
		for (j = 0; j < sizeof(TRAN)/sizeof(TRAN[0]); j++) {
			if (TRAN[j].from == s[i]) {
				strcat(t, TRAN[j].to);
				break;
			}
		};
		if (j == sizeof(TRAN)/sizeof(TRAN[0]))
				t[strlen(t)] = s[i];
	}
}

/* filter to useful information by filter struct*/
static int log_filter(char *name, char *type_buf)
 {
#ifdef	debug
	struct {
		char *name;
		char *type;
		int value;
	} *p, filter_items[] = {
		{"CROND", "System Activity", 1},
		{"kernel", "Debug Information", 1},
		{"nmbd", "Dropped Packets", 1},
		{"su", "Attacks", 1},
		{"smbd", "Notice", 1},
		{ NULL, NULL, 0}
	};
#else
	static struct {
		char *name;
		char *type;
		char *key_name;
	} *p, filter_items[] = {
		{"HTTP", "System Activity", "log_system_activity"},
		{"PPP",  "PPP"            , "log_system_activity"},
		{"DHCP", "System Activity", "log_system_activity"},
		{"DDNS", "System Activity", "log_system_activity"},
		{"IPSec", "IPSec"	  , "log_system_activity"},
		{"Firewall", "Dropped Packets","log_dropped_packets"},
		{"TEST", "System Activity", "log_system_activity"},
#if 0
		{"info", "System Activity", "log_system_activity"},
		{"warn", "Debug Information", "log_debug_information"},
		{"err", "Dropped Pockets","log_dropped_packets"},
		{"thttpd", "Dropped Pockets", "log_dropped_packets"},
		{"crit", "Attacks", "log_attacks"},
		{"info", "Attacks", "log_attacks"},
		{"daemon",  "Notice", "log_notice"},
		{"notice", "Notice", "log_notice"},
#endif		
		{ NULL, NULL, 0}
	};
#endif
	int ret = 0;
	char value[256];
	for (p = filter_items; p->name != NULL; p++)
	{
		if (strstr(name, p->name) != 0) {
#ifdef debug
			if (p->value == 1) {
				if (type_buf != NULL)
					strcpy(type_buf, p->type);
				ret = 1;
				break;
#else
			if (query_vars(p->key_name, value, sizeof(value)) < 0)
				continue;

			if (atoi(value) == 1){
				if (type_buf != NULL)
					strcpy(type_buf, p->type);
				ret = 1;
				break;
#endif
			} else
				break;
		}
	}
	return ret;
}

/* to be format */
static int log_paser(char *log_string, char **time, char **pid, char **message)
{
	char *temp;

	*time = log_string;
	temp = strchr(log_string, ':');
	if (temp == NULL)
		return 1;
	temp = strchr(temp, ' ');
	*temp++= '\0';
	
	*pid = strchr(temp, ' ');
	
	temp = strchr(*pid, ':');
	if (temp == NULL) {
		temp = *pid;
		*temp++ = '\0';
		*message = temp;
	} else {
		*temp++= '\0';
		(*pid)++;
		*(temp+(strlen(temp)-1)) = '\0';
		*message = temp;
	}
	return 0;
}

/* In curren not use. */
static int message_filter()
{
        struct stat stat_buf;
        char tmp_buf[128];

	if (stat(LOG_FILE, &stat_buf) == 0) {
		/* LOG_FILE transferred to TEMP_FILE. */
		sprintf(tmp_buf, "awk -f %s %s >> %s", LOG_AWK, LOG_FILE, TEMP_FILE);
		system(tmp_buf);
		sprintf(tmp_buf, "/bin/echo \"\" > %s", LOG_FILE);
		system(tmp_buf);
	} else {
		/* No LOG_FILE exist */
		return 1;
	}
	/* if TEMP_FILE bigger than limitation then clear to empty. */
	if (stat(TEMP_FILE, &stat_buf) == 0) {
		if (stat_buf.st_size > MAX_TEMP_LOG ) {
			/* just log needed log messages in /tmp/log_temp */
			sprintf(tmp_buf, "/bin/echo \"\" > %s", TEMP_FILE);
			system(tmp_buf);
		}
	}
	return 0;
}

/* to do filter by log_awk */
static void filter_log(void)
{
	char tmp_buf[1024];

	bzero(tmp_buf, sizeof(tmp_buf));
	sprintf(tmp_buf, "awk -f %s %s >> %s", 
		LOG_AWK, LOG_FILE, TEMP_FILE);
	system(tmp_buf);
	sprintf(tmp_buf, "/bin/echo \"\" > %s", 
		LOG_FILE);
	system(tmp_buf);
}

/* Get Line NUM */
static int get_line(FILE *fp)
{
	int cnt = 0;
	char string_buf[256], type_buf[32];
	char *time, *pid, *message;
	
	while(!feof(fp)){
		/* Get Data from fp description */
		if (fgets(string_buf, sizeof(string_buf), fp) == NULL)
			break;
		/* to be format */
		if (log_paser(string_buf, &time, &pid, &message))
			continue;
		/* parser to need information */
		if (log_filter(pid, type_buf))
			cnt++;
	}
	return cnt;
}

/* every page only show 20 items. */
static void _page(FILE *fp, int show_page)
{
	char string_buf[256], type_buf[32], time_buf[32];
        char *out_buf[LINE_PAGE], tmp_buf[1024];
	char *time, *pid, *message, *app_name;
	int cnt = 0, i = 20, j = 0;
	
	while(!feof(fp)){
		/* Get Data from fp description */
		if (fgets(string_buf, sizeof(string_buf), fp) == NULL)
			break;
		/* to be format */
		if (log_paser(string_buf, &time, &pid, &message))
			continue;
		/* catch app_name */
		if((app_name = strrchr(pid, ' ')) != NULL)
			app_name++;
		/* parser to need information */
		if (log_filter(pid, type_buf)) {
			strcpy(time_buf, time);
			cnt++;
			bzero(tmp_buf, sizeof(tmp_buf));
			if ( cnt > (show_page * LINE_PAGE)){
				str_trans(message, tmp_buf);
				if (app_name  != NULL)
					printf("%s|%s|%s %s|syslog|",
						time, type_buf, app_name, tmp_buf);
				else
					printf("%s|%s|%s|syslog|", 
						time, type_buf, tmp_buf);
			} else {
				if ( i == 20) {
					i = 0;
				}
				str_trans(message, tmp_buf);

				if (app_name  != NULL)
					sprintf(string_buf, "%s|%s|%s %s|syslog|",
						time, type_buf, app_name, tmp_buf);
				else
					sprintf(string_buf, "%s|%s|%s|syslog|",
						time_buf, type_buf, tmp_buf);

				out_buf[i] = strdup(string_buf);
				i++;	
			}
			if ( cnt > ((show_page + 1) * LINE_PAGE) - 1)
				break;
		}
	}

	if ( (i != 20) || (show_page > total_page_cnt(cnt)) ) {
			for (j = 0; j < i; j++)
				printf("%s", out_buf[j]);
	}
}

static void _file(FILE *fp)
{
	FILE *log_fp = NULL;
	char string_buf[256], type_buf[32], time_buf[32];
	char *time, *pid, *message, *app_name;
	
	/* Ready to write content to logfile */
	if((log_fp = fopen("/tmp/logfile", "w+")) == NULL){
		SYSLOG("Open LOG record file FAIL\n");
		return;
	}
	fputs("Time             || Type          || Message", log_fp);

	while(!feof(fp)){
		fputs("\r\n", log_fp);
		/* Get Data from fp description */
		if (fgets(string_buf, sizeof(string_buf), fp) == NULL)
			break;
		/* to be format */
		if (log_paser(string_buf, &time, &pid, &message))
			continue;
		/* catch app_name */
		if((app_name = strrchr(pid, ' ')) != NULL)
			app_name++;
		/* parser to need information */
		if (log_filter(pid, type_buf)) {
			strcpy(time_buf, time);
			if (app_name  != NULL)
				sprintf(string_buf, "%s  %s %s %s",
					time, type_buf, app_name, message);
			else
				sprintf(string_buf, "%s  %s %s", 
					time, type_buf, message);
			fputs(string_buf, log_fp);
		}
	}
	fclose(log_fp);
}

static void _save(FILE *fp)
{
	char string_buf[256], type_buf[32], time_buf[32];
        char tmp_buf[1024];
	char *time, *pid, *message, *app_name;

	/* save log file */
	query_vars("model_name", string_buf, sizeof(string_buf));
#if 0
	fputs("Cache-Control: no-cache\r\n", stdout);
	fputs("Pragma: no-cache\r\n", stdout);
	fputs("Expires: 0\r\n", stdout);
#endif
	fputs("Content-Type: application/download\r\n", stdout);
	sprintf(tmp_buf, "Content-Disposition: attachment ;"
			 "filename=%s_log.txt\n\n", string_buf);
	fputs(tmp_buf, stdout);

	while(!feof(fp)){
		/* Get Data from fp description */
		if (fgets(string_buf, sizeof(string_buf), fp) == NULL)
			break;
		/* to be format */
		if (log_paser(string_buf, &time, &pid, &message))
			continue;
		/* catch app_name */
		if((app_name = strrchr(pid, ' ')) != NULL)
			app_name++;
		/* parser to need information */
		if (log_filter(pid, type_buf)) {
			strcpy(time_buf, time);
			if (app_name  != NULL)
				sprintf(string_buf, "%s  %s %s %s\r\n",
					time, type_buf, app_name, message);
			else
				sprintf(string_buf, "%s  %s %s\r\n", 
					time, type_buf, message);
			fputs(string_buf, stdout);
		}
	}
}

static void _current_page(const char *action, const char *page, int cnt)
{
	char value[128];
	if (query_vars(action, value, sizeof(value)) >= 0) {
		if ( atoi(value) > total_page_cnt(cnt)){
			printf("%d", total_page_cnt(cnt));
		} else {
			fputs(value, stdout);
		}
	} else {
		if (page != NULL)
			fputs(page, stdout);
	}
}

/*
 * Main Process to do by action
 *	action = total --- count total page number
 *		 page  --- show page log infomation
 *		 file  --- print out log file to /tmp/logfile FIXME:logfile is hard code
 *		 save  --- save log file to local hard drive
		 current_page --- mainly to show current page, it usually use with page.
 *	page = show page number --- only use for "page" at console
 */
static void _log_page(const char *action,const char *page, int show_page)
{
	int cnt = 0;
	FILE *fp;

	/* Read log_temp file */
	if((fp = fopen(TEMP_FILE, "r")) == NULL) {
		SYSLOG("Open LOG record file FAIL\n");
		return;
	}

	if(strcmp(action, "page") == 0)
		_page(fp, show_page);
	else if(strcmp(action, "file") == 0)
		_file(fp);
	else if(strcmp(action, "save") == 0)
		_save(fp);
	else
		cnt = get_line(fp);

	fclose(fp);
	
	if (strcmp(action, "current_page") == 0)
		_current_page(action, page, cnt);
	if (strcmp(action, "total") == 0)
		printf("%d", total_page_cnt(cnt));
}

/*
 *	argv[1] = total --- count total page number
 *		  page  --- show page log infomation
 *		  file  --- print out log file to /tmp/logfile FIXME:logfile is hard code
 *		  save  --- save log file to local hard drive
 *	argv[2] = show page number --- only use for "page" at console
 */
int log_page_main(int argc, char *argv[])
{
	char tmp_buf[1024];
	char *s, *t;
	struct stat st;
	int show_page = 0;
	
	if (argv[1] == NULL)
		return -1;

	//if (message_filter())
	//	return -1;

	/* Get Query String */
	s = getenv("QUERY_STRING");
	if (s != NULL) {
		if ((s = strstr(s, "current_page"))) {
			t = strchr(s, '=');
			if (strstr(t, "clear") != 0) {
				sprintf(tmp_buf, "/bin/echo "" > %s",
					       	TEMP_FILE);
				system(tmp_buf);
				sleep(1);
			} else {
				show_page = (atoi(++t) - 1);
			}
		}
	} else if ( (strcmp(argv[1], "page") == 0) && ( argv[2] != NULL))
		show_page = (atoi(argv[2]) - 1);

	/* just log needed log messages in /tmp/log_temp.
	 * and do filter
	 */
	if (stat(TEMP_FILE, &st) != 0)
		filter_log();

	/* main process */
	_log_page(argv[1], argv[2], show_page);

	return 0;
}
