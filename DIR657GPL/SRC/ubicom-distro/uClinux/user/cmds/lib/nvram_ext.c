#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "interface.h"		/* NVRAM_MAX_IF */

int nvram_find_index(const char *prefix, const char *value)
{
	int i;
	char tmp[512];

	for (i = 0; i < NVRAM_MAX_IF; i++) {
		snprintf(tmp, sizeof(tmp), "%s%d", prefix, i);
		if (NVRAM_MATCH(tmp, value))
			return i;
	}
	return -1;
}

char *nvram_eval_safe_get(const char *key, int __key)
{
	char cmdline[1024];

	bzero(cmdline, sizeof(cmdline));
	snprintf(cmdline, sizeof(cmdline) - 1, "%s%d", key, __key);

	return nvram_safe_get(cmdline);
}


char *__nvram_map(const char *key1, const char *value, const char *key2)
{
	//get devname from aliase.
	int i;
	char *p = NULL;
	
	if ((i = nvram_find_index(key1, value)) != -1)
		p = nvram_get_i(key2, i, g1);
	
	return p;
}

char *nvram_map(const char *key1, const char *value, const char *key2, char *buf)
{

	int i;
	char *p = NULL;
	
	if ((i = nvram_find_index(key1, value)) == -1)
		p = nvram_get_i(key2, i, g1);
	
	return (p == NULL) ? NULL : strcpy(buf, p);
}
