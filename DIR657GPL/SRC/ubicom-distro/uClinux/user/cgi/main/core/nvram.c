#include <stdio.h>
#include "nvram.h"
#include "public.h"

static void nvram_exit_dummy(void *unused)
{
	return;
}
static int nvram_init_dummy(void *unused)
{
#ifdef __USE_ATH_BASE_NVRAM
	nvram_init();
	return 0;
#else
	return nvram_init(NULL);
#endif
}
struct nvram_ops nvops = { 
	init:	nvram_init_dummy,
	exit:	nvram_exit_dummy,
	get:	nvram_get,
	set:	nvram_set,
	unset:	nvram_unset,
	commit:	nvram_commit
};

int do_nvram_register(int argc, char *argv[])
{
	return nvram_register(&nvops);
}

