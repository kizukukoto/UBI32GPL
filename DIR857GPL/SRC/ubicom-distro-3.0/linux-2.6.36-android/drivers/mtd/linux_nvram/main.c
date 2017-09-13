#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "typedefs.h"
#include "nvram.h"

static void
usage(void)
{
	printf("usage: nvram [get name] [set name=value] [unset name] [show] [restore_default] [upgrade] [eraseall]\n");
	exit(0);
}

/* NVRAM utility */
int
main(int argc, char **argv)
{
	char *name, *value, *buf;
	int size, rev = -1, nvram_space;


	/* Skip program name */
	--argc;
	++argv;

	if (!*argv) 
		usage();

	if ((nvram_space = nvram_get_nvramspace()) <= 0) {
		fprintf(stderr, "can not get space size from nvram device\n");
		return -1;
	}
	if ((buf = malloc(nvram_space)) == NULL) {
		fprintf(stderr, "can not malloc()\n");
		return -1;
	}
	/* Process the remaining arguments. */
	for (; *argv; argv++) {
		if (!strncmp(*argv, "get", 3)) {
			if (*++argv) {
				if ((value = nvram_get(*argv)))
					puts(value);
			}
		}
		else if (!strncmp(*argv, "set", 3)) {
			if (*++argv) {
				//memset(buf, 0, nvram_space);
				strncpy(value = buf, *argv, nvram_space);
				name = strsep(&value, "=");
				nvram_set(name, value);
			}
		}
		else if (!strncmp(*argv, "unset", 5)) {
			if (*++argv)
				nvram_unset(*argv);
		}
		else if (!strncmp(*argv, "commit", 5)) {
			nvram_commit();
		}
		else if (!strncmp(*argv, "show", 4) ||
			   !strncmp(*argv, "getall", 6)) {
			//memset(buf, 0, nvram_space);  	
			nvram_getall(buf, nvram_space);
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);
			size = sizeof(struct nvram_header) + (int) name - (int) buf;
			printf("size: %#X bytes (%#X left)\n", size, nvram_space - size);
		} else if (nvram_compatible_args(argv, &rev) == 0) {
			free(buf);
			return rev;
		}
		if (!*argv)
			break;
	}
	free(buf);
	return 0;
}	
