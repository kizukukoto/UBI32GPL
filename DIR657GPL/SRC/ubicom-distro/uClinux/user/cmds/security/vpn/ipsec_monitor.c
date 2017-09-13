#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
		fprintf(fp, fmt, ## args); \
		fclose(fp); \
	} \
} while (0)

static void ipsec_ikerekey(int sig)
{
	int sec;
	char conn[128];
	FILE *fp = fopen("/tmp/ipsec_monitor.conf", "r");

	if (fp == NULL) {
		cprintf("XXX %s(%d) cannot open /tmp/ipsec_monitor.conf", __FUNCTION__, __LINE__);
		return;
	}

	fscanf(fp, "%s %d", conn, &sec);
	fclose(fp);

	cprintf("XXX %s(%d) conn[%s] sec[%d]\n", __FUNCTION__, __LINE__, conn, sec);

	sleep(sec);
	//system("/bin/sh -c \"rc restart &\"");
	system("/bin/sh -c \"ipsec setup restart &\"");
}

/*
 * argv[0] := "monitor"
 * argv[1] := "connection name"
 * argv[2] := "seconds"
 * */
int ipsec_main_monitor(int argc, char *argv[])
{
	FILE *fp;
	struct sigaction act;
	pid_t pid = getpid();

	if ((fp = fopen("/tmp/ipsec_monitor.pid", "w")) == NULL)
		goto out;
	fprintf(fp, "%d", pid);
	fclose(fp);

	act.sa_handler = ipsec_ikerekey;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGPROF, &act, NULL);

	while(1);
out:
	return 0;
}
