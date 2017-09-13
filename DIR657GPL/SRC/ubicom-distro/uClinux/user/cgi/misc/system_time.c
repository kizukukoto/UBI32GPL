#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <getopt.h>
#include <time.h>
#include <fcntl.h>
#include <linux_vct.h>
#include <sutil.h>
#include <shvar.h>
#include <nvram.h>

#include "ssi.h"
#include "libdb.h"
#include "debug.h"

/* get the wan port connection time */
static int wan_connect_time(int argc, char *argv[]){
        char tmp[32] = {0};

	if (argc < 2)
		return -1;

	memset(tmp, '\0', sizeof(tmp));

	sprintf(tmp, "%lu", get_wan_connect_time(atol(argv[1])));
	cprintf("%s", tmp);
	printf("%s", tmp);
}

static int get_system_time(int argc, char *argv[]){
        char sys_time[50] = {};
        time_t timep;
        struct tm *p;
        time(&timep);
        p = gmtime(&timep);

        cprintf("return_system_time: %d/%d/%d/%d/%d/%d\n", \
                (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

        sprintf(sys_time, "%d/%d/%d/%d/%d/%d",
                (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

        printf("%s", sys_time);
	return 0;
}

struct __system_time_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int misc_system_time_help(struct __system_time_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

int system_time_main(int argc, char *argv[])
{
	int ret = -1;
	struct __system_time_struct *p, list[] = {
		{ "get", "Get System Time", get_system_time},
		{ "get_wantime", "Get Wan Time Duration of Connection", wan_connect_time},
		{ NULL, NULL, NULL }
	};

	if (argc == 1 || strcmp(argv[1], "help") == 0)
		goto help;

	for (p = list; p && p->param; p++) {
		if (strcmp(argv[1], p->param) == 0) {
			ret = p->fn(argc - 1, argv + 1);
			goto out;
		}
	}
help:
	ret = misc_system_time_help(list);
out:
	return ret;
}
