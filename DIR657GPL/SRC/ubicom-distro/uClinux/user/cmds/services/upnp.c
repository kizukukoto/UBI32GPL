#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"

#define UPNPD_CONF	"/var/etc/miniupnpd.conf"

static int upnp_start_main(int argc, char *argv[])
{
	FILE *fp;
	int ret = -1;
	char *dev, *ip;

	if (!NVRAM_MATCH("upnp_enable", "1"))
		return -1;	/* II did not add it to stop list*/
	if (argc < 3)
		goto out;
	
	dev = argv[1];
	ip = argv[2];
	
	ret = eval("killall", "-SIGUSR1", "miniupnpd");

	if ((fp = fopen(UPNPD_CONF, "w")) == NULL)
		goto out;

	fprintf(fp, "wps_enable=%s\n"
			"friendly_name=%s\n"
			"manufacturer_name=D-Link\n"
			"model_name=D-Link Router\n"
			"manufacturer_url=http://www.dlink.com\n"
			"model_url=http://support.dlink.com\n"
			"model_number=%s\n"
			"serial=none\n"
			"listening_ip=%s\n"
			"ext_ifname=%s\n",
		       	nvram_safe_get("wps_enable"),
			nvram_safe_get("model_name"),
			nvram_safe_get("model_name"),
			ip,
			dev);
	fclose(fp);
	eval("iptables", "-I", "INPUT", "-p", "tcp", "-m", "mport",
		"--dports","60001,65535", "-j", "ACCEPT");

	eval("iptables", "-I", "INPUT", "-p", "udp", "--dport", 
	     "1900", "-j", "ACCEPT");
	eval("iptables", "-I", "INPUT", "-p", "tcp", "--dport", 
             "2869", "-j", "ACCEPT");
	
	ret = eval("miniupnpd");
out:
	rd(ret, "err\n");
	return ret;
}

static int upnp_stop_main(int argc, char *argv[])
{
	int ret = eval("killall", "miniupnpd");
	eval("iptables", "-D", "INPUT", "-p", "tcp", "-m", "mport",
		"--dports","60001,65535", "-j", "ACCEPT");
	eval("iptables", "-D", "INPUT", "-p", "udp", "--dport", 
             "1900", "-j", "ACCEPT");
	eval("iptables", "-D", "INPUT", "-p", "tcp", "--dport", 
             "2869", "-j", "ACCEPT");
	
	cprintf("done\n");

	return ret;
}

/*
 * upnp services
 * start:
 * 	@argv[1]: dev
 * 	@argv[2]: ip
 */
int upnp_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", upnp_start_main},
		{ "stop", upnp_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
