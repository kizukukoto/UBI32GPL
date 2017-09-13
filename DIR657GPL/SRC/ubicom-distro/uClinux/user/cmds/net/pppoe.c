#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include <syslog.h>
#include <signal.h>
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "convert.h"
#include "libcameo.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

int pppoe_start(const char *alias, const char *dev, const char *phy)
{
	int idx, rev = -1, unit;
	struct proto_pppoe poe;
	pid_t pid, ppp_pid;
	extern int manual_start;

	if ((idx = nvram_find_index("pppoe_alias", alias)) == -1)
		goto out;
	if (init_proto_pppoe((struct proto_base *)&poe, sizeof(poe), idx) == -1)
		goto out;
	trace("ppp dev:%s", dev);
	unit = atoi(dev + 3);
	dump_proto_struct(PROTO_PPPOE, (struct proto_base *)&poe, stdout);
#if NOMMU
	pid = vfork();
#else	
	pid = fork();
#endif
	if (pid == -1)
		goto out;
	if (pid == 0) {
		fcloseall();
#if NOMMU
		if (vfork() == 0) {
#else
		if (fork() == 0) {
#endif

			setsid();
			/* Here! I want to implement as a external hook */
			
			setenv1("PPPD_PROTO", "pppoe");
			setenv1("PPPD_INDEX",itoa(idx));
			setenv1("PPPD_UNIT", itoa(unit));
			setenv1("PPPD_USER", poe.user);
			setenv1("PPPD_PASS", poe.pass);
			setenv1("PPPD_IDLE", poe.idle);
			setenv1("PPPD_IFNAME", phy);
			setenv1("PPPD_SERVICENAME", poe.servicename);
			setenv1("PPPD_DNS1", poe.dns1);
			setenv1("PPPD_DNS2", poe.dns2);
			setenv1("PPPD_IFNAME", phy);
			setenv1("PPPD_DIAL", poe.dial);
			setenv1("PPPD_MODE", poe.mode);	/* 0: static,1: dyn */
			setenv1("PPPD_ADDR", ((struct proto_base *)&poe)->addr);
			setenv1("PPPD_MPPE", poe.mppe);
			setenv1("PPPD_MTU", poe.mtu);
			if (*((struct proto_ppp *)&poe)->dial == '0' &&
				manual_start == 0) {
				trace("Manual mode not start!");
				return 0;
			}
			eval("/etc/sys_hooks/pppd");
		}
		exit(0);
	}
	waitpid(pid, NULL, 0);
	sleep(2);	/* XXX: wait pppd create pid file */
	nic_monitor(phy, "killall -HUP pppd", "");
	nvram_set_i("pppoe_status", idx, g1, "connecting");
	rev = 0;
out:
	return rev;
}

int pppoe_start_main(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *dev, *phy;
	
	DEBUG_MSG("%s: argc=%d\n", __func__, argc);	
	if (argc < 4){
		goto out;
	}	
	args(argc - 1, argv + 1, &alias, &dev, &phy);
	if (strncmp(dev, "ppp", 3) != 0)
		goto out;

	// set clone mac and mtu
	clone_mac("pppoe", alias, argv[3]);	// argv[3]: phy
	/* only ethX in static/dhcpc need set mtu */
	//set_mtu("pppoe", alias, argv[3]);

	rev = pppoe_start(alias, dev, phy);
out:
	rd(rev, "error");
	return rev;
}

static int pppoe_showconfig_main(int argc, char *argv[])
{
	return proto_showconfig_main(PROTO_PPPOE, argc, argv);
}

static int
pppoe_attach_dns(const char *pdev, const char *phy, const char *remote)
{
	int rev = -1;
	char pdev_gw[16], pdev_ip[16], pdev_mask[16];
	char phy_dev[16], phy_ip[16], phy_mask[16];
	char _pdev[16];

	if (!remote || strlen(remote) == 0)
		return rev;

	dns_delete(remote);
	dns_append(remote);
	DEBUG_MSG("%s, ppp: %s, phy: %s, remote: %s\n", __func__, pdev, phy, remote);

	/* if remote (DNS IP) in the same network with LAN, then... return */
	if (get_ip(phy, phy_ip) == -1)
		return rev;
	if (get_netmask(phy, phy_mask) == -1)
		return rev;
	if (same_subnet(phy_ip, remote, phy_mask))
		return rev;

	/* add remote (DNS IP) into routing table which using pppx interface go out */
	if (get_ip(pdev, pdev_ip) == -1)
		return rev;
	if (get_netmask(pdev, pdev_mask) == -1)
		return rev;
	if (same_subnet(pdev_ip, remote, pdev_mask))
		return rev;

        /* getGatewayInterfaceByIP(remote, _pdev, pdev_gw); */
        route_add_host_if_need(pdev, pdev_ip, remote, pdev_mask, getenv("IPREMOTE"));

	rev = 0;
	return rev;
}
#if 0
// obsolete
int pppoe_attach_main(int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;

	DEBUG_MSG("%s\n", __func__);
	__ppp_convert();
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	
	if ((i = nvram_find_index("pppoe_alias", alias)) == -1)
		goto out;
	/* 7 = strlen("1.1.1.1") */
	if (strlen(nvram_safe_get_i("pppoe_primary_dns", i, g1)) >= 7 ||
	    strlen(nvram_safe_get_i("pppoe_secondary_dns", i, g1)) >= 7)
	{
		dns_delete(nvram_safe_get_i("pppoe_primary_dns", i, g1));
		dns_append(nvram_safe_get_i("pppoe_primary_dns", i, g1));
		dns_delete(nvram_safe_get_i("pppoe_secondary_dns", i, g1));
		dns_append(nvram_safe_get_i("pppoe_secondary_dns", i, g1));
	} else {
		//trace("pppoe attach_dns: %s, %s", getenv("Z_DNS1"), getenv("Z_DNS2"));
		pppoe_attach_dns(argv[2], argv[3], getenv("Z_DNS1"));
		pppoe_attach_dns(argv[2], argv[3], getenv("Z_DNS2"));
	}

	nvram_set_i("pppoe_pid", i, g1, getenv("Z_PID"));
	nvram_set_i("pppoe_status", i, g1, "connected");
	/* XXX: ? */
	eval("iptables", "-I", "INPUT", "-i", dev, "-j", "ACCEPT");
	rev = 0;
out:
	return rev;
}
#endif
int pppoe_attach_main(int argc, char *argv[])
{
	return pppoe_usb3g_attach_main("pppoe", argc, argv);
}

int pppoe_detach_main(int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;
	
	system("set >/tmp/pppd.detach");
	__ppp_convert();
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;

	if ((i = nvram_find_index("pppoe_alias", alias)) == -1)
		goto out;
	nvram_set_i("pppoe_pid", i, g1, "");
	nvram_set_i("pppoe_status", i, g1, "disconnected");
	eval("iptables", "-D", "INPUT", "-i", dev, "-j", "ACCEPT");

	rev = 0;

out:
	rd(rev, "error");
	return rev;
}

int pppoe_stop_main(int argc, char *argv[])
{
	int rev = -1, i, pid;
	char *alias = NULL, *dev = NULL, *phy = NULL;

	if (argc < 4)
		goto out;
	for (i = 0;i<argc;i++)
		debug("argv[%d]=%s\n", i, argv[i]);
		
	args(argc - 1, argv + 1, &alias, &dev, &phy);
	
	if ((i = nvram_find_index("pppoe_alias", alias)) == -1)
		goto out;

	if ((kill_pppd_from_pidfile(SIGINT, dev)) == -1)
		goto out;
	/* FIXME: when/where is it come from ? */
	//eval("iptables", "-D", "FORWARD", "-o", argv[3], "-j", "ACCEPT");
	debug("phy %s, argv3=%s\n", phy, argv[3]);
	if (kill_nic_monitor(phy) == -1)
		goto out;
	/* FIXME: kill again: to avoid pppd is idled, or responed,
	 * or something unknow happen.
	 * */
	sleep(1);
	kill_pppd_from_pidfile(SIGKILL, dev);

	rev = 0;
out:
	return rev;
}

int pppoe_status_main(int argc, char *argv[])
{
	if (argc < 4)
		help_proto_args(stderr, 1);
	return proto_xxx_status2(PROTO_PPPOE, argv[1], argv[2], argv[3]);
}

/*
 * See proto_operator_main for argc, argv:
 * by default argv: act, alias, dev, phy
 */
int pppoe_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", pppoe_start_main}, 
		{ "stop", pppoe_stop_main},   
		{ "status", pppoe_status_main}, 
		{ "showconfig", pppoe_showconfig_main},
		{ "attach", pppoe_attach_main}, 
		{ "detach", pppoe_detach_main}, 
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
/*
 * Most all of operations has 2 basic args @alias, @device.
 * 
 * */
