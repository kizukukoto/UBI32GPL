#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"

#define IGMP_CONF	"/tmp/igmpproxy.conf"


static int igmp_start_main(int argc, char *argv[])
{
	FILE *fp;
	int rev = -1;

	if(!(fp = fopen(IGMP_CONF, "w")) ) {
		cprintf("open %s fail!!", IGMP_CONF);
		fclose(fp);
	}

	fprintf(fp, "quickleave\n\n"
			"phyint %s upstream  ratelimit 0  threshold 1\n"
			"phyint %s downstream  ratelimit 0  threshold 1\n\n",
			get_if_val("WAN","if_phy"),
			get_if_val("LAN","if_phy"));
	fclose(fp);
	/* XXX: Why can not use eval()? */
	//rev = eval("/usr/sbin/igmpproxy", "-c", "/tmp/igmpproxy.conf");
	system("igmpproxy -c /tmp/igmpproxy.conf");
	system("iptables -I FORWARD -s 224.0.0.0/224.0.0.0 -j ACCEPT");
	rev = 0;
out:
	return rev;
}

static int igmp_stop_main(int argc, char *argv[])
{
	int ret = eval("killall", "igmpproxy");
	system("iptables -D FORWARD -s 224.0.0.0/224.0.0.0 -j ACCEPT");
	cprintf("done\n");

	return ret;
}

/*
 * 
 */

int igmp_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", igmp_start_main},
		{ "stop", igmp_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
