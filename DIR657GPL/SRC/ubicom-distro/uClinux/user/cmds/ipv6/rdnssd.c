#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"

static int rdnssd_main_stop (int argc, char *argv[])
{
        printf("Stop IPv6 rdnssd\n");
        _system ("killall rdnssd");
	unlink(RDNSSD_PID);
        return 0;
}

static int rdnssd_main_start (int argc, char *argv[])
{
	if (argc != 2){
		printf("Invalid argument : cli ipv6 rdnssd start [wan_if]\n");
		return 0;
	}
	if (access(RDNSSD_PID, F_OK) == 0){
		printf("RDNSSD is already exist\n");
		return 0;
	}

	set_default_accept_ra(1);

        	_system("rdnssd -H %s -r %s -i %s &", 
			RDNSSD_SCRIPT_FILE, RDNSSD_RESOLV_FILE, argv[1]);

	trigger_send_rs(argv[1]);

        return 0;
}

int rdnssd_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", rdnssd_main_start, "start RDNSSD"},
                { "stop", rdnssd_main_stop, "stop RDNSSD"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}
