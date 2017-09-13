#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static int dnsmasq_start_main(int argc, char *argv[])
{
	char *lan_if = nvram_safe_get("if_dev0");

	DEBUG_MSG("%s, Begin, args :\n", __FUNCTION__);
	{
		int i = 0;
		for(i = 0; i < argc ; i++){
			DEBUG_MSG("%s ", argv[i] ? argv[i] : "");
		}
	}
	DEBUG_MSG("\n");

	if (NVRAM_MATCH("dns_relay", "1")) {
		DEBUG_MSG("%s, dns_relay is enable\n",__FUNCTION__);

		if (lan_if != NULL) {
			char *dnsmasq_argv[] = {"dnsmasq", "-i", lan_if, NULL};
			pid_t dnsmasq_pid;
			
			DEBUG_MSG("%s, _eval dnsmasq -i %s\n",__FUNCTION__,lan_if);
			//_eval(char *const argv[], char *path, int timeout, int *ppid)
			_eval(dnsmasq_argv, NULL, 0, &dnsmasq_pid);
			DEBUG_MSG("%s, dnsmasq pid = %d\n",__FUNCTION__,dnsmasq_pid);
		}else{
			char *dnsmasq_argv[] = {"dnsmasq", NULL};
			pid_t dnsmasq_pid;
			DEBUG_MSG("%s, _eval dnsmasq\n",__FUNCTION__);
			
			_eval(dnsmasq_argv, NULL, 0, &dnsmasq_pid);
			DEBUG_MSG("%s, dnsmasq pid = %d\n",__FUNCTION__,dnsmasq_pid);
		}
	}else{
		DEBUG_MSG("%s, dns_relay is not enable\n",__FUNCTION__);
	}

	return 0;
}

static int dnsmasq_stop_main(int argc, char *argv[])
{
	int ret = eval("killall", "dnsmasq");
	DEBUG_MSG("%s, killall dnsmasq\n",__FUNCTION__);
	return ret;
}

int dnsmasq_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", dnsmasq_start_main},
		{ "stop", dnsmasq_stop_main},
		{ NULL, NULL}
	};

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
