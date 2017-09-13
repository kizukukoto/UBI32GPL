#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <netinet/in.h> /* for sockaddr_in */
#include <sys/ioctl.h>

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

struct subcmd {
        char *action;
        int (*fn)(int argc, char *argv[]);
        char *help_string;
};

/*wireless*/
extern int start_wireless(int argc, char *argv[]);
extern int stop_wireless(int argc, char *argv[]);
/*utility*/
extern int get_wlan_statistics(int argc, char *argv[]);
extern int get_wlan_mac_table(int argc, char *argv[]);
extern int get_wlan_channel(int argc, char *argv[]);
/*config*/
extern int wconfig_main(int argc, char *argv[]);
/*eap*/
extern int start_rt2880apd(int argc, char *argv[]);
extern int stop_rt2880apd(int argc, char *argv[]);
/*wps*/
extern int init_wps_main(int argc, char *argv[]);
extern int start_wps_main(int argc, char *argv[]);
extern int stop_wps_main(int argc, char *argv[]);
extern int pin_main(int argc, char *argv[]);
extern int pbc_main(int argc, char *argv[]);
extern int wps_led_main(int argc, char *argv[]);
extern int wps_ok_main(int argc, char *argv[]);
extern int wps_pbc_monitor_main(int argc, char *argv[]);
extern int wps_monitor_main(int argc, char *argv[]);
extern int update_wireless_nvram(int argc, char *argv[]);

static int wl_debug;
#define wlmsg(fmt, a...) { wl_debug ? fprintf(stderr, fmt, ##a) : 0;}

/*************************************************************
 * subcmds helpers
 * **********************************************************/
static void main_using(const char *cmd, struct subcmd *cmds)
{
	fprintf(stderr, "Unknown command: %s, use subcommands below\n", cmd);

	for (;cmds->action != NULL; cmds++) {
		fprintf(stderr, "  %-15s: %s\n", cmds->action,
			cmds->help_string == NULL?"":cmds->help_string);
	}
	fprintf(stderr, "build %s %s\n", __DATE__ , __TIME__);
}

static struct subcmd *lookup_subcmd(int argc, char *argv[], struct subcmd *cmds)
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
			DEBUG_MSG("lookup_subcmd: p->action=%s\n",p->action);
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

static int wps_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"init", init_wps_main, "init once configures for wirless"}, // ??
		{"start", start_wps_main, "start wps daemon"}, 
		{"stop", stop_wps_main, "stop wps daemon"}, 
		{"pin", pin_main, "pin utility"}, 
		{"pbc", pbc_main, "pbc utility"}, 
		{"led", wps_led_main, "toggle wps led"},
		{"monitor", wps_monitor_main, "A daemon which monitor wps status"},
		{"wps_ok", wps_ok_main, "wps_ok...(for test)"}, // ???
		{"update", update_wireless_nvram, "update wireless nvram"},
		{"pbcmonitor", wps_pbc_monitor_main, "A deamon which monitor push button action"},
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

static int ralink_utility(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"statistics", get_wlan_statistics, 
		"Statistics information of Wireless," \
		"Ex: cmds wlan utility statistics interface" \
		" [statistics]"},
		{"mac_table", get_wlan_mac_table, "Ap Client information," \
		"Ex: cmds wlan utility mac_table interface" \
		" [mtable]"},
		{"channel", get_wlan_channel, "Get AP Channel," \
		"Ex: cmds wlan utility channel interface [channel]"},
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

static int eap_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"start", start_rt2880apd, "start WPA-EAP," \
		" Example : cmds wlan eap start interface[ra01_0|ra00_0]"}, 
		{"stop", stop_rt2880apd, "stop WPA-EAP," \
		" Example : cmds wlan eap stop interface[ra01_0|ra00_0]"}, 
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

static int wireless_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"start", start_wireless, "start Wireless," \
		" Example : cmds wlan wireless start."},
		{"stop", stop_wireless, "stop Wireless," \
		" Example : cmds wlan wireless stop."}, 
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}


static int wlan_utility(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{"wireless", wireless_main, "Wireless utility"},
		{"wps", wps_main, "Wps utility"},
		{"eap", eap_main, "Radius Server for WPA-EAP utility"},
		{"wconfig", wconfig_main, "Do iNIC config" \
		", Example : cmds wlan wconfig interface[ra01_0|ra00_0]"},
		{"utility", ralink_utility, "Wlan information utility"},
		{NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

#ifdef __WLUTILS_MAIN__
int main(int argc, char *argv[])
#else
int cmds_main(int argc, char *argv[])
#endif
{
	struct subcmd cmds[] = {
		{"wlan", wlan_utility, "wlan utility"},
		{NULL, NULL}
	};
	wl_debug = getenv("WL_DEBUG") == NULL ? 0 : !strcmp("1", getenv("WL_DEBUG"));
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
