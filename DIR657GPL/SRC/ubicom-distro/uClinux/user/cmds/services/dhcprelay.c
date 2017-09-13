#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmds.h"
#include "nvramcmd.h"

#define MULTI_SSID_START	1
#define MULTI_SSID_END		3
#define MULTI_SSID_PREFIX	"multi_ssid_lan%d"
#define MULTI_SSID_VAP_PREFIX	"wlan0_vap%d_enable"

static int dhcprelay_mssid_interface(char *buf)
{
	int i = MULTI_SSID_START;
	char *dev, *ip, *mask, *p;
	char vap_key[64], ssid_key[64], ssid_info[128];

	*buf = '\0';

	for (; i <= MULTI_SSID_END; i++) {
		p = ssid_info;
		sprintf(ssid_key, MULTI_SSID_PREFIX, i);
		sprintf(vap_key, MULTI_SSID_VAP_PREFIX, i);
		strcpy(ssid_info, nvram_safe_get(ssid_key));

		if (NVRAM_MATCH(vap_key, "0"))
			continue;
		if ((dev = strsep(&p, ",")) == NULL)
			continue;
		if ((ip = strsep(&p, ",")) == NULL)
			continue;
		if ((mask = strsep(&p, ",")) == NULL)
			continue;
		sprintf(buf, "%s,%s", buf, dev);
	}

	return 0;
}

static int dhcprelay_start(int argc, char *argv[])
{
	char mssid_dev[64];
	char cmd[256], *wandev = "eth0.1";

	if (NVRAM_MATCH("wan_proto", "pppoe") ||
	    NVRAM_MATCH("wan_proto", "pptp") ||
	    NVRAM_MATCH("wan_proto", "l2tp"))
		wandev = "ppp0";

	dhcprelay_mssid_interface(mssid_dev);

	sprintf(cmd, "dhcprelay %s,%s%s %s %s &",
		nvram_safe_get("lan_bridge"),
		nvram_safe_get("lan_eth"),
		mssid_dev,
		wandev,
		nvram_safe_get("dhcp_relay_ip"));

	fprintf(stderr, "XXX %s(%d) cmd[%s]\n", __FUNCTION__, __LINE__, cmd);

	return system(cmd);
}

static int dhcprelay_stop(int argc, char *argv[])
{
	return system("killall dhcprelay");
}

int dhcprelay_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", dhcprelay_start},
		{ "stop", dhcprelay_stop},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
