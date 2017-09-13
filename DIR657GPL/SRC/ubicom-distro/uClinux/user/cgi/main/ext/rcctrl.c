#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#ifdef LINUX_NVRAM
/*
 * copy(override) from sutils library
 * */
#ifndef RC_PID
#define RC_PID			"/var/run/rc.pid"
#endif
#include "ssi.h"
static int read_pid(char *file)
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

/* Add vfork : execlp function does not retun when implementation of successful*/
int rc_restart()
{
#if NOMMU
	char cmds[64];
	sprintf(cmds, "(sleep %s; killall -SIGHUP rc) &", nvram_safe_get("sys_delaytime"));
	pid_t pid;
	pid = vfork();
	if(pid == 0)
		execlp("/bin/sh", "/bin/sh", "-c", cmds, NULL);
	else if (pid >0)
		return 0;
	else
		cprintf("XXX %s[%d] vfotk error XXX",__func__,__LINE__);
#else
	return kill(read_pid(RC_PID), SIGHUP);
#endif
}

int rc_term()
{
#if NOMMU
	char cmds[64];
	sprintf(cmds, "(sleep %s; killall -SIGTERM rc) &", nvram_safe_get("sys_delaytime"));
	pid_t pid;
	pid =vfork();
	if(pid == 0)	
		execlp("/bin/sh", "/bin/sh", "-c", cmds, NULL);
	else if (pid > 0)
		return 0;
	else 
		cprintf("XXX %s[%d] vfotk error XXX",__func__,__LINE__);
#else
	return kill(read_pid(RC_PID), SIGTERM);
#endif
}
#else
/* DIR-130/330 */

int rc_restart()
{
	return kill(1, SIGHUP);
}

int rc_term()
{
	return kill(1, SIGTERM);
}
#endif
