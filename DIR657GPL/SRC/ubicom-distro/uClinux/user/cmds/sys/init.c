#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>
#include "cmds.h"
static commit;
static void nvram_sanity_check(const char *k, const char *v, void *unused)
{
        if (nvram_get(k) == NULL) {
                printf("restore key: %s = \"%s\"\n", k, v);
                nvram_set(k, v);
		commit++;
        }
}
static int init_nvram_v2()
{
        if (nvram_match("restore_default", "0") != 0) {
                nvram_restore_default();
                nvram_set("restore_default", "0");
		commit++;
                //nvram_commit();
        } else
                foreach_nvram_from(NVRAM_DEFAULT, nvram_sanity_check, NULL);
        return 0;
}

static int init_nvram_main(int argc, char *argv[])
{
	init_nvram_v2();
	if (commit)
		nvram_commit();
	return 0;
}
int init_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "nvram_check", init_nvram_main, "check sanity nvram to"
				" restore, reset lose keys"},
		{ NULL, NULL }
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
