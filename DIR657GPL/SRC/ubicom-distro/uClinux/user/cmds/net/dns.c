#include <stdio.h>
#include "cmds.h"
#include "nvram.h"
int dns_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "insert", NULL},
		{ "append", NULL},
		{ "delete", NULL},
		{ "clearall", NULL},
		{ "showconfig", NULL},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
