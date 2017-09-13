
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include "cmds.h"
static void main_using(const char *cmd, struct subcmd *cmds)
{
	if (cmd != NULL)
		fprintf(stderr, "Unknown command: %s, use subcommands below\n", cmd);
	else
		fprintf(stderr, "Sub-commands list\n");

	for (;cmds->action != NULL; cmds++) {
		fprintf(stderr, "  %-15s: %s\n", cmds->action,
			cmds->help_string == NULL?"":cmds->help_string);
	}
	fprintf(stderr, "cli build %s %s\n", __DATE__ , __TIME__);
}

#include <dlfcn.h>
static int plugin(int argc, char *argv[], const char *upcmd, int *ret)
{
#define PLUGIN_ROOTDIR		"/lib/cmds/"
#define PLUGIN_INIT_MAIN	"plugin_init_main"
#define PLUGIN_MNT_POINT	"plugin_mnt_point"
	
	char obj[1024] = PLUGIN_ROOTDIR, *mnt;
	void *h;
	int (*fn_plugin_main)(int, char *argv[]);
	char *(*fn_plugin_mnt_point)();
	
	if (argc == 0)
		goto out;
	strcat(obj, argv[0]);
	strcat(obj, ".so");
	
	if (access(obj, X_OK) == -1)
		goto out;
	if ((h = dlopen(obj, RTLD_NOW)) == NULL)
		goto out;
	
	/* check mount(hook) point */
	if ((fn_plugin_mnt_point = dlsym(h,PLUGIN_MNT_POINT)) == NULL)
		goto dlerr;
	if ((mnt = fn_plugin_mnt_point()) == NULL)
		goto release;
	if (strncmp(upcmd, mnt, strlen(upcmd)) != 0)
		goto release;
	
	/* call main function */
	if ((fn_plugin_main = dlsym(h, PLUGIN_INIT_MAIN)) == NULL)
		goto dlerr;
	*ret = fn_plugin_main(argc, argv);
	return 0;
dlerr:
	fprintf(stderr, "dlsym: %s\n", dlerror());
release:
	dlclose(h);
out:
	return -1;
}
struct subcmd *lookup_subcmd(int argc, char *argv[], struct subcmd *cmds)
{
	
	struct subcmd *p = NULL;

	if (argc < 2 ||
		(strcmp(argv[1], "-h") == 0 ||
		strcmp(argv[1], "--help") == 0))
	{
		goto out;
	}
	
	for (p = cmds; p->action != NULL; p++) {
		if (strncmp(argv[1], p->action, strlen(argv[1])) == 0) {
			return p;
		}
	}
out:
	return NULL;
}

int lookup_subcmd_then_callout(int argc, char *argv[], struct subcmd *cmds)
{
	struct subcmd *p = NULL;
	int rev = -1;
	char *upcmd;
	upcmd = argv[0];
	
	if ((p = lookup_subcmd(argc, argv, cmds)) == NULL) {
		if (plugin(--argc, ++argv, upcmd, &rev) == -1)
			main_using(argv[0], cmds);
		goto out;
	}
	if (p->fn == NULL) {
		fprintf(stderr, "%s not implement!\n", p->action);
		goto out;
	}
	rev = p->fn(--argc, ++argv);
out:
	return rev;
}
