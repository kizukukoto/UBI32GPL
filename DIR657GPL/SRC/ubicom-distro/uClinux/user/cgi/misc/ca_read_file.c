#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libdb.h"
#include "debug.h"

#define CA_PATH "/tmp/ca/"

struct __ca_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int misc_ca_help(struct __ca_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

static int read_public(int argc, char *argv[])
{
	cprintf("Nothing to do!\n");
	return 0;
}

static int read_private(int argc, char *argv[])
{
	cprintf("Nothing to do!\n");
	return 0;
}

static int read_CA(int argc, char *argv[])
{
	FILE *fp;
	char path[256], key[32], value[4096];
	
	memset(value, '\0', sizeof(value));

	if (argc < 3) {
		cprintf("Ex: ca_read_file CA out2 key\n");
		goto out;
	}

	sprintf(path, "%s%s", CA_PATH, argv[1]);
	sprintf(key, "%s", argv[2]);
	if ((fp = fopen(path, "r+") ) == NULL) {
		cprintf("File does not exit!");
		goto out;
	} else
		fread(value, sizeof(value), 2, fp);
	
	cprintf("key :: %s\n", key);
	cprintf("value :: %s\n", value);
	
	nvram_set(key, value);
	nvram_commit();
	fclose(fp);
out:
	return 0;
}


int ca_read_file_main(int argc, char *argv[])
{
	int ret = -1;
	struct __ca_struct *p, list[] = {
		{ "public", "Read Public Certificate key", read_public},
		{ "private", "Read Private Certificate key", read_private},
		{ "CA", "Read CA Certificate key", read_CA},
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
	ret = misc_ca_help(list);
out:
	return ret;
}
