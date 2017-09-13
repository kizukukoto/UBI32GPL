#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cmds.h"
#include "shutils.h"
#include "nvram.h"
#include "nvram_ext.h"


#define MAC_FILTER_MAX		99
#define ALG_BUFSIZE		64
static int __fw_mac_filter_manip2(
		const char *src,
		const char *mac,
		const char *sched,
		int act,
		int start)
{
	char *args[24] = {
		"iptables", "-I", "FORWARD"	
	};
	char time_cmd[][ALG_BUFSIZE] = {
		"-m",
		"time",
		"--timestart",
		"HH:MM:SS",
		"--timestop",
		"HH:MM:SS",
		"--days",
		"",
		""
	};
	int i = 3;

	args[1] = start ? "-I" : "-D";
	
	if (*src != '\0') {
		args[i++] = "-s";
		args[i++] = src;
	}
	if (*mac != '\0') {
		args[i++] = "-m";
		args[i++] = "mac";
		args[i++] = "--mac-source";
		args[i++] = mac;
	}

	args[i] = NULL;
	if (!(*sched < '0' || '9' < *sched)) {
		/*
		 * @alg_append will format schedule_rule to @time_cmd
		 * then APPEND to @prefix_cmd
		 * */
		fprintf(stderr, "yes to schedule :%s\n", sched);
		alg_append_sched(args, atoi(sched), time_cmd);
	}
	while (args[i] != NULL) i++;
	args[i++] = "-j";
	args[i++] = act == 1 ? "ACCEPT" : "DROP";
	args[i] = NULL;
	fputc('\n', stderr);
	for (i = 0; args[i] != NULL; i++)
		fprintf(stderr, "%s ", args[i]);
	fputc('\n', stderr);
	_eval(args, ">/dev/console", 0, NULL);
}

int __fw_mac_filter_manip(char *raw, int act, int start)
{
	char *src, *mac, *sched;
	int rev = -1;

	/* "1;LAN>WAN_primary;my_fw;always;192.168.100.1;00:11:22:33:44:55:66" */
	if ((raw = fw_paser_common(raw, NULL, NULL, NULL, NULL, &sched)) == NULL)
		goto fmt_err;
	mac = strsep(&raw, ";");
	src = raw;
	//printf("start mac:%s,%s:\n", mac, src);
	return __fw_mac_filter_manip2(src, mac, sched, act, start);
no_dev:
out:
	return rev;
fmt_err:
	rd(rev, "Format error");
	return rev;
}

int fw_mac_filter_manip(int start)
{
	char *p, buf[512], *if_alias, *manip;
	int i, rev = -1, act;

	if ((p = nvram_get("filter_macmode")) == NULL)
		goto out;
	if (strcmp(p, "disabled") == 0)
		return 0;
	else if (strcmp(p, "allow") == 0)
		act = 1;
	else if (strcmp(p, "deny") == 0)
		act = 0;
	else {
		err_msg("Unknow mac filter:%s", p);
		goto out;
	}
	manip = start ? "-I" : "-D";

	/* We insert ESTABLISHED,RELATED accepting rule in FORWARD chain,
	 * because the package need to know the content of each other,
	 * ex: ICMP, TCP ack.. Here we indicate ! br0 interface */
	if (act == 1) {
		eval("iptables", manip, "FORWARD", "-j", "DROP");
		eval("iptables", manip, "FORWARD", "-i" , "!",  "br0", "-m", "state",
			"--state", "ESTABLISHED,RELATED", "-j", "ACCEPT");
	}

	for (i = 0; i <= MAC_FILTER_MAX; i++) {
		if ((p = nvram_get_i("mac_filter", i, g1)) == NULL)
			continue;
		if (*p != '1')	// @p = '\0' or '1' or '0'
			continue;
		
		p = strlen(p) > sizeof(buf) ? strdup(p) : strcpy(buf, p);
		
		__fw_mac_filter_manip(p, act, start);
		if (p != buf)
			free(p);
	}
	rev = 0;
out:
	return rev;
}

static int mac_filter_start_main(int argc, char *argv[])
{
	return fw_mac_filter_manip(1);
}

static int mac_filter_stop_main(int argc, char *argv[])
{
	return fw_mac_filter_manip(0);
}

int mac_filter_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", mac_filter_start_main},
		{ "stop", mac_filter_stop_main},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
