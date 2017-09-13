#include <stdio.h>
#include <stdlib.h>

#include "cmds.h"

int wlan_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ NULL, NULL}
	};
	int ret;

	lock_prog(argc, argv, 0);
	ret = lookup_subcmd_then_callout(argc, argv, cmds);
	unlock_prog();
	return ret;
}
