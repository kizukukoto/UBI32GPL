#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
			fprintf(fp, fmt, ## args); \
			fclose(fp); \
		} \
} while (0)

static int klogd_start_main(int argc, char *argv[])
{
	int ret;

	ret = eval("klogd");

	return ret;
}

static int klogd_stop_main(int argc, char *argv[])
{
	int ret;

	ret = eval("killall", "klogd");
	
	return ret;
}

/*
 * operation policy:
 * start: evel klogd
 * */
int klogd_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", klogd_start_main},
		{ "stop", klogd_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
