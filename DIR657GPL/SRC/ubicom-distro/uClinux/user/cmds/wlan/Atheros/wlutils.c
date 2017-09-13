#include <stdio.h>
#include <stdlib.h>

#include "cmds.h"

extern int start_wlan();
extern int stop_wlan();
extern int start_wps();
extern int stop_wps();

static int wlan_main(int argc, char *argv[])
{
	int ret = 0;
	struct subcmd cmds[] = {
		{ "start", start_wlan, "start wireless"},
		{ "stop", stop_wlan, "stop wireless"},
		{ NULL, NULL}
	};
	lock_prog(argc, argv, 0);
	ret = lookup_subcmd_then_callout(argc, argv, cmds);
	unlock_prog();
	return ret;

}

static int wlan_wps(int argc, char *argv[])
{
	int ret = 0;
	struct subcmd cmds[] = {
		{ "start", start_wps, "start wps"},
		{ "stop", stop_wps, "stop wps"},
		{ NULL, NULL}
	};
	lock_prog(argc, argv, 0);
	ret = lookup_subcmd_then_callout(argc, argv, cmds);
	unlock_prog();
	return ret;

}

int wlan_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "wlan_all", wlan_main, "setup wireless"},
		{ "wps", wlan_wps, "setup wps"},
		{ NULL, NULL}
	};
	int ret;

	lock_prog(argc, argv, 0);
	ret = lookup_subcmd_then_callout(argc, argv, cmds);
	unlock_prog();
	return ret;
}
