#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nvramcmd.h"
#include "cmds.h"

#define KEY_PREFIX	"multi_ssid_lan"
#define VAP_PREFIX	"wlan0_vap%d_enable"
#define KEY_MAX		3
#if 1
#define DEBUG_MSG(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
		fprintf(fp, "XXX %s(%d)" fmt, __FUNCTION__, __LINE__, ## args); \
		fclose(fp); \
	} \
} while (0)
#else
#define DEBUG_MSG(fmt, args...)
#endif

static int vif_up(const char *vif, const char *ip, const char *mask)
{
	char cmds[256];

	sprintf(cmds, "ifconfig %s %s netmask %s", vif, ip, mask);
	DEBUG_MSG("cmds[%s]\n", cmds);

	return system(cmds);
}

static int vif_down(const char *vif)
{
	char cmds[256];

	sprintf(cmds, "ifconfig %s down", vif);
	DEBUG_MSG("cmds[%s]\n", cmds);

	return system(cmds);
}

/* @raw := "br0:0,192.168.1.1,255.255.255.0" */
static int vif_updown(const char *raw, const char *act)
{
	int res = -1;
	char *vif, *ip, *mask, *p, _raw[128];

	p = _raw;
	strcpy(_raw, raw);

	if ((vif = strsep(&p, ",")) == NULL)
		goto out;
	if ((ip = strsep(&p, ",")) == NULL && strcmp(act, "up") == 0)
		goto out;
	if ((mask = strsep(&p, ",")) == NULL && strcmp(act, "up") == 0)
		goto out;
	DEBUG_MSG("vif[%s] ip[%s] mask[%s] act[%s]\n", vif, ip, mask, act);
	if (strcmp(act, "up") == 0)
		res = vif_up(vif, ip, mask);
	else
		res = vif_down(vif);
out:
	return res;
}

/*
 * argv[-1] := "vif"
 * argv[ 0] := "up"|"down"
 * */
static int vif_updown_main(int argc, char *argv[])
{
	int i = 1;

	for (; i <= KEY_MAX; i++) {
		char key[32], vap_key[32], vif_raw[128];

		sprintf(vap_key, VAP_PREFIX, i);
		if (NVRAM_MATCH(vap_key, "0"))
			continue;
		sprintf(key, "%s%d", KEY_PREFIX, i);
		strcpy(vif_raw, nvram_safe_get(key));
		DEBUG_MSG("vif_raw[%s] argv[1][%s]\n", vif_raw, argv[0]);

		if (*vif_raw == '\0')
			continue;
		if (vif_updown(vif_raw, argv[0]) != 0)
			fprintf(stdout, "XXX %s(%d) virtual interface start [fail]\n",
				__FUNCTION__, __LINE__);
	}

	return 0;
}

static int vif_help_main(int argc, char *argv[])
{
	printf("Usage:\n");
	printf("\t1. %s [up|down]\n", argv[-1]);
	printf("\t2. %s help\n", argv[-1]);

	return 0;
}

int vif_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "up", vif_updown_main },
		{ "down", vif_updown_main },
		{ "help", vif_help_main },
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
