#include <stdio.h>
#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"
#include "libcameo.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define WEBFILTER_CONFIG	"/tmp/url_domain"
#define WEBFILTER_SCHEDULE	"/tmp/url_schedule"
#define WEBFILTER_RULES		50
#define SYS_SCHEDULE		25
#define WEBFILTER_PROXY_PORT	3128
#define WEBFILTER_TYPE		NVRAM_MATCH("url_domain_filter_type", "2")
#define WEBFILTER_POLICY	WEBFILTER_TYPE ? "permit-domains":"deny-domains"

static int webfilter_config()
{
	int i, ret = -1;
	char *url_content, url_keys[32];
	FILE *cfg_fp = fopen(WEBFILTER_CONFIG, "w");
	FILE *sch_fp = fopen(WEBFILTER_SCHEDULE, "w");

	if (cfg_fp == NULL || sch_fp == NULL)
		goto out;

	for (i = 0; i < WEBFILTER_RULES; i++) {
		char *sch_content, url_schedule[64];

		sprintf(url_keys, "url_domain_filter_%d", i);
		sprintf(url_schedule, "url_domain_filter_schedule_%d", i);
		url_content = nvram_safe_get(url_keys);
		sch_content = nvram_safe_get(url_schedule);

		if (*url_content == '\0')
			continue;

		fprintf(cfg_fp, "%s\n", url_content);
		fprintf(sch_fp, "%s=%s\n", url_schedule, sch_content);
	}

	for (i = 0; i < SYS_SCHEDULE; i++) {
		char *syssch_content, sys_schedule[64];

		sprintf(sys_schedule, "schedule_rule%d", i);
		syssch_content = nvram_safe_get(sys_schedule);

		if (*syssch_content == '\0')
			continue;

		fprintf(sch_fp, "%s=%s\n", sys_schedule, syssch_content);
	}

	ret = 0;
out:
	fcloseall();
	return ret;
}

static void
webfilter_firewall_updown(
	int up, const char *dev,
	const char *ip, const char *netmask)
{
	char in_cmds[256], nat_cmds[256];

	sprintf(in_cmds, "iptables -%c INPUT -i %s -p tcp --dport %d "
			"-j ACCEPT", up ? 'I': 'D', dev, WEBFILTER_PROXY_PORT);

	sprintf(nat_cmds, "iptables -t nat -%c PREROUTING -i %s -d ! %s "
			"-p tcp --dport 80 -j DNAT --to-destination %s:%d",
			up ? 'I': 'D', dev, ip, ip, WEBFILTER_PROXY_PORT);

	DEBUG_MSG("in_cmds: %s\n", in_cmds);
	DEBUG_MSG("nat_cmds: %s\n", nat_cmds);

	system(in_cmds);
	system(nat_cmds);
}

static void webfilter_updown(int up, const char *ip, const char *netmask)
{
	char crowd_cmds[256];

	if (up) {
		sprintf(crowd_cmds, "crowdcontrol -a %s -s %s -p %d --%s %s",
			ip, netmask, WEBFILTER_PROXY_PORT,
			WEBFILTER_POLICY, WEBFILTER_CONFIG);
		sprintf(crowd_cmds, "%s -S %s", crowd_cmds, WEBFILTER_SCHEDULE);
	} else {
		sprintf(crowd_cmds, "killall crowdcontrol");
	}

	DEBUG_MSG("crowd_cmds: %s\n", crowd_cmds);

	system(crowd_cmds);
}

static int webfilter_start_main(int argc, char *argv[])
{
	int ret = -1;
	char *dev, *ip, *netmask;	/* from argv */

	DEBUG_MSG("##### WEB FILTER START #####\n");

	if (NVRAM_MATCH("url_domain_filter_type", "0"))
		goto out;

	if (webfilter_config() != 0)
		goto out;

	if (get_std_args(argc, argv, &dev, &ip, &netmask, NULL) == -1)
		goto out;

	webfilter_updown(1, ip, netmask);
	webfilter_firewall_updown(1, dev, ip, netmask);

	ret = 0;
out:
	return ret;
}

static int webfilter_stop_main(int argc, char *argv[])
{
	int ret = -1;
	char *dev, *ip, *netmask;	/* from argv */

	DEBUG_MSG("##### WEB FILTER STOP #####\n");

	if (get_std_args(argc, argv, &dev, &ip, &netmask, NULL) == -1)
		goto out;

	webfilter_updown(0, ip, netmask);
	webfilter_firewall_updown(0, dev, ip, netmask);

	ret = 0;
out:
	return ret;
}

int webfilter_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", webfilter_start_main},
		{ "stop", webfilter_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
