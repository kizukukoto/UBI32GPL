#include <stdio.h>
#include "cmds.h"

static int tftpd_start(int argc, char *argv[])
{
	system("/sbin/tftpd &");
	system("iptables -I INPUT -p udp --dport 69 -j ACCEPT");
	return 0;
}

static int tftpd_stop(int argc, char *argv[])
{
	system("killall tftpd");
	system("iptables -D INPUT -p udp --dport 69 -j ACCEPT");
	return 0;
}

int tftpd_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", tftpd_start},
                { "stop", tftpd_stop},
                { NULL, NULL}
        };
        return lookup_subcmd_then_callout(argc, argv, cmds);
}
