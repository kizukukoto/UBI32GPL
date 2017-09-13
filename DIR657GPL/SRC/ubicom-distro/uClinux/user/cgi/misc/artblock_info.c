#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ssi.h"
#include "libdb.h"
#include "debug.h"
#include "artblock.h"

struct __artblock_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int misc_artblock_help(struct __artblock_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

static int artblock_get_info(int argc, char *argv[])
{
	char *buf = NULL;

	if ((buf = artblock_safe_get(argv[0])) == NULL || strlen(buf) == 0) 
		buf = nvram_safe_get(argv[0]);
	printf("%s", buf);
	return 0;
}

static int fw_version_info(int argc, char *argv[])
{
	
	return 0;
}

int artblock_info_main(int argc, char *argv[])
{
	int ret = -1;
	struct __artblock_struct *p, list[] = {
		{ "hw_version", "Show Hardware Version", artblock_get_info},
		{ "lan_mac", "Show Lan Mac Address", artblock_get_info},
		{ "wan_mac", "Show Wan Mac Address", artblock_get_info},
		{ "wlan0_domain", "Show Wan Mac Address", artblock_get_info},
		{ "version", "Show Firmware version", fw_version_info},
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
	ret = misc_artblock_help(list);
out:
	return ret;
}
