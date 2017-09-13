#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
			fprintf(fp, fmt, ## args); \
			fclose(fp); \
		} \
} while (0)

/* this value should be consistent with busybox-1.6.1/sysklogd/syslogd.c */
#define LOG_IP_MAX 8

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static int syslogd_start_main(int argc, char *argv[])
{
	int ret = 0;
	char cmd[128];
	char *ptr = cmd;
	char *remote_server = NULL;
	int i = 0;
	
	DEBUG_MSG("%s, Begin, args :\n", __FUNCTION__);
	{
		for(i = 0; i < argc ; i++){
			DEBUG_MSG("%s ", argv[i] ? argv[i] : "");
		}
	}
	DEBUG_MSG("\n");

	memset(cmd,'\0',sizeof(cmd));

	ptr += sprintf(ptr,"syslogd ");
	
	/*	-L:
		Log locally and via network logging	*/
	ptr += sprintf(ptr,"-L ");

	if (NVRAM_MATCH("log_server", "1")){
		DEBUG_MSG("%s, log to remote server is enable\n",__FUNCTION__);
		for( i = 0 ; i < LOG_IP_MAX ; i++ ){
			if (((remote_server = nvram_get_i("log_ipaddr_", i, g1)) != NULL)  
					&& ( -1 != inet_addr(remote_server)) ){
				DEBUG_MSG("%s, log_ipaddr_%d %s is the first IP\n",__FUNCTION__,i,remote_server);
				ptr += sprintf(ptr,"-R ");
				ptr += sprintf(ptr,"%s:514 ",remote_server);
				//because we only need to add 1 server via command
				//others, if has, will be automatically added by syslogd itself
				break;
			}
		}
		
	}else{
		DEBUG_MSG("%s, log to remote server is disable\n",__FUNCTION__);
	}
	/*	-m: not support in busybox-1.6.1 anymore - "SYSLOGD_MARK"
	//ptr += sprintf(ptr,"-m ");

	/*	-s SIZE:
		MAX size (KB) before rotate, default = 200KB, 0 = off	*/
	//the size of 1600 records are about 85K
	ptr += sprintf(ptr,"-s 100 ");
	
	/*	-b NUM:
		Number of rotated logs to keep	*/
	ptr += sprintf(ptr,"-b 0 ");
	
	/*	-n: 
		Avoid auto-backgrounding.  This is needed especially if the sys-
		logd is started and controlled by init(8).	*/
	//ptr += sprintf(ptr,"-n ");
	
	//ret = eval("syslogd","-m", "0");
	ptr += sprintf(ptr,"&");
	
	DEBUG_MSG("%s, %s\n",__FUNCTION__,cmd);
	system(cmd);
	eval("klogd","&");
	return ret;
}

static int syslogd_stop_main(int argc, char *argv[])
{
	int ret = 0;
	DEBUG_MSG("stop klogd syslogd\n",__FUNCTION__);
	ret = eval("killall","klogd" , "syslogd");
	
	return ret;
}

/*
 * operation policy:
 * start: evel syslogd
 * */
int syslogd_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", syslogd_start_main},
		{ "stop", syslogd_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
