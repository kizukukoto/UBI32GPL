#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmds.h"
#include "shutils.h"

//#define VPN_MAX 99
#define TMP_VPN_PAIR	"/tmp/vpn_pair_clear"
#define PROC_VPN_PAIR	"/proc/sys/net/vpn/vpn_pair"


int hw_ipsec_clear()
{
	int i, rev = -1;
	FILE *fd;
	char *vpn_max = nvram_get("vpn_max");
	char buf[1024] = "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ";

	if (access(TMP_VPN_PAIR, F_OK) == 0)
		goto file_exist;
	
	if ((fd = fopen(TMP_VPN_PAIR, "w")) == NULL)
		goto out;

	for (i = 0; i < ((vpn_max == NULL)?20:atoi(vpn_max)) ; i++)
		fputs(buf, fd);
	fclose(fd);
file_exist:
	sprintf(buf, "cat %s > %s", TMP_VPN_PAIR, PROC_VPN_PAIR);
	system(buf);
	rev = 0;
out:
	return -1;
}
#if 0
/* depricated */
int ipsec_hw_parsers()
{
	int i;
	char cmds1[256];

	sprintf(cmds1, "echo \"\"> %s", TMP_VPN_PAIR);
	system(cmds1);
	
	for (i = 0; i < VPN_MAX ; i++) {
		char cmds2[1024];

		sprintf(cmds2, "echo 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 >> %s",
			TMP_VPN_PAIR);
		system(cmds2);
	}
	
	bzero(cmds1, sizeof(cmds1));
	sprintf(cmds1, "cat %s > %s", TMP_VPN_PAIR, VPN_PAIR);
	system(cmds1);
	
	bzero(cmds1, sizeof(cmds1));
	sprintf(cmds1, "echo \"\"> %s", TMP_VPN_PAIR);
	system(cmds1);

	return 0;
}
#endif

/*
int ipsec_hw_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "del", del_ipsec_hw_parser},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}*/
