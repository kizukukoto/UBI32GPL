#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "convert.h"
#include "nvram_ext.h"
#include "libcameo.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define UDHCPC_PID_DIR	"/var/run"

int kill_nic_monitor(const char *dev)
{
	return kill_from_pidfile(SIGINT, "/var/run/nic_monitor.%s.pid", dev);
}

void nic_monitor(
	const char *dev,
	const char *plugin_cmd,
	const char *plugout_cmd
)
{
#if 0
	DEVNAME=eth1
	PLUGIN_CMD="cli net ii start WAN_primary"
	PLUGOUT_CMD="cli net ii stop WAN_primary"
	INTERVAL=1
	CHECK_CMD="cli misc hotplug net $DEVNAME"
#endif
	FILE *fd = stdout;
	char buf[256];
	int pid;
	char *arg2;
	
	sprintf(buf, "/tmp/misc/nic_monitor.conf.%s", dev);
	if ((fd = fopen(buf, "w")) == NULL)
		return;
	
	fprintf(fd,
		"DEVNAME=%s\n"
		"PLUGIN_CMD=\"%s\"\n"
		"PLUGOUT_CMD=\"%s\"\n"
		"INTERVAL=10\n"
		"CHECK_CMD=\"cli misc hotplug net %s\"\n",
		dev,
		plugin_cmd,
		plugout_cmd,
		dev
	);
	fclose(fd);
	arg2 = net_phy_present(dev) == 0 ? "plugin":"plugout";
	eval_nowait(&pid, "nic_monitor", buf, arg2);
	/*XXX: kill on xxx_stop_main */
}

static inline int __dhcpc_eval(const char *dev,const char *hostname, const char *script)
{
	char pid_file[128];

	sprintf(pid_file, "%s/udhcpc_%s.pid", UDHCPC_PID_DIR, dev);
	
	if (script == NULL)
		script = "/etc/udhcpc/sample.script";
	
	/* SIGUSR1:renew, SIGUSR2: release */
	nic_monitor(dev, "killall -USR1 udhcpc", "");

	return eval("udhcpc", "-i", (char *)dev , "-s",
		(char *)script, "-R", "-p", pid_file, "-b");
}

int __dhcpc_start(const char *alias, const char *dev)
{
	char *hname, tmp[512];
	int rev = -1;
	int idx;
       
	if (alias == NULL || alias[0] == '\0' || dev == NULL || dev[0] == '\0')
		goto out;
	if ((idx = nvram_find_index("dhcpc_alias", alias)) == -1)
		goto out;	

	hname = nvram_get(strcat_r("dhcpc_hostname", itoa(idx), tmp));

	nvram_set_i("dhcpc_status", idx, g1, "connecting");
	printf("dhcpc status:%s", nvram_get(g1));
	rev = __dhcpc_eval(dev, hname, NULL);
out:
	return rev;
}

/*
struct devinf {
	char *suffix;
	uint8_t exit_on_null;
	char *(*get_suffix)(const char *suffix);
	char *(*set_suffix)(const char *suffix);
} DEVINF[] = {};

int dhcpc_dconfig(int argc, char *argv[])
{
	
}
*/


static struct dhcpcenv {
	char *env;
	uint8_t exit_on_null;	     
} devinfo[] = {
	{ "ipaddr", 1},
	{ "router", 1},
	{ "subnet", 1},
	{ "dns", 0},
	{ NULL, 0}
};

/* 
 * bound when dhcpc get ipaddr.
 * deconfig when dhcpc release or pre-renew, pre-discovery ...
 *
 * all parameters we need is come from environment
 * virables which setup by dhcpc.
 * 
 * @int = 1 as bound
 *
 * */
int dhcpc_bound_or_deconfig(int bound, const char *alias, const char *dev)
{
	char prefix[128], tmp[512], *pid;
	int rev = -1, i;
	struct dhcpcenv *dp;
	struct proto_dhcpc pb;
	
	if (dev == NULL)
		goto out;
	if ((pid = getenv("dhcpc_pid")) == NULL)
		goto out;

	strcat_r(dev, "_", prefix);	/* ex: prefix = "eth1_" */
	if ((i = nvram_find_index("dhcpc_alias", alias)) == -1)
		goto out;
	/*
	if (init_proto_dhcpc((struct proto_base *)&pb, sizeof(pb), i) == -1)
		goto out;
	*/
	bound ? nvram_set(strcat_i("dhcpc_pid", i, tmp), pid) :
		nvram_unset(strcat_i("dhcpc_pid", i, tmp));
	debug("***********dhcpc bound ok:%s:%s", nvram_get("dhcpc_pid0"), nvram_get("dhcpc_pid1"));
	
	/* just set or unset into nvram ! */
	for (dp = devinfo; dp->env != NULL; dp++) {
		dev = getenv(dp->env);
		if (bound) {
			if (dev == NULL && dp->exit_on_null == 1)
				break;
			nvram_set(strcat_r(prefix, dp->env, tmp), dev);
		} else {
			/* unset even if they was not in env */
			nvram_unset(strcat_r(prefix, dp->env, tmp));
		}
	}
	rev = 0;
out:
	return rev;
}
void args_dev_proto(int argc, char *argv[], char **dev, char **proto)
{

	if (dev != NULL)
		*dev = getenv("interface");
	if (proto != NULL)
		*proto = getenv("if_proto");
	
	if (dev != NULL && argc >= 2)
		*dev = argv[1];
	if (proto != NULL && argc >= 3)
		*proto = argv[2];
	return;
	
}
/*
 * Get one of @alias or @dev, or even both of them.
 * @proto is optional and NULL to ignore
 * if nothing be set, -1 return.
 * */
int get_std_args(int argc, char *argv[], char **alias, char **dev, char **proto)
{
	*alias = getenv("if_alias");
	
	if (argc >= 2)
		*alias = argv[1];
	if (argc < 2)
		return -1;
	args_dev_proto(--argc, ++argv, dev, proto);
	
	if (*alias == NULL && *dev == NULL) {
		printf("[alias] [dev]\n");
		return -1;
	}
	return 0;
}

int dhcpc_deconfig_main(int argc, char *argv[])
{
	char *alias, *dev;
	int rev = -1;

	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	rev = dhcpc_bound_or_deconfig(0, alias, dev);
out:
	return rev;
}

int dhcpc_bound_main(int argc, char *argv[])
{
	char *alias, *dev;
	int rev = -1;
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	rev = dhcpc_bound_or_deconfig(1, alias, dev);
out:
	return rev;
}

int dhcpc_start_main(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *dev;
	
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;

	// set clone mac and mtu
	clone_mac("dhcpc", alias, dev);
	set_mtu("dhcpc", alias, dev);

	rev = __dhcpc_start(argv[1], argv[2]);
out:
	rd(rev, "Fail");
	return rev;
}

struct proto_struct proto_dhcpc = {
	name: PROTO_DHCPC_STR,
	start: dhcpc_start_main,
	stop: NULL,
	status:NULL,
	showconfig:NULL,
};


static int dhcpc_showconfig_main(int argc, char *argv[])
{
	return proto_showconfig_main(PROTO_DHCPC, argc, argv);
}
void help_proto_args(FILE *stream, int is_exit)
{
	fprintf(stream, "usage: [proto alias] [device] [phy]");
	if (is_exit)
		exit(0);
}

int dhcpc_status_main(int argc, char *argv[])
{
	if (argc < 3)
		help_proto_args(stderr, 1);
	return proto_xxx_status2(PROTO_DHCPC, argv[1], argv[2], argv[2]);
}

int dhcpc_release_main(int argc, char *argv[])
{
	/*
	 * to clear dhcpd infos in nvram
	 * and dns setting from dhcpc ...
	 * */
	
	return 0;
}
int dhcpc_stop_main(int argc, char *argv[])
{
	/* stop dhcpc by interface or alias
	 * via kill pid which from dhcpc_pid
	 * */
	char *alias, *dev;
	int rev = -1, i, pid;

	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	if (alias == NULL)
		goto out;
	if ((i = nvram_find_index("dhcpc_alias", alias)) == -1)
		goto out;

	if (kill_from_pidfile(SIGTERM, "%s/udhcpc_%s.pid", UDHCPC_PID_DIR, dev) == -1)
		goto out;
	if (kill_nic_monitor(dev) == -1)
		goto out;
	/*
	if ((pid = nvram_get(strcat_i("dhcpc_pid", i, g1))) == NULL)
		goto out;
	debug("dhcp PID:%d", atoi(pid));
	rev = kill(atoi(pid), SIGTERM);
	nvram_unset(strcat_i("dhcpc_pid", i, g1));
	*/
	rev = 0;
	
out:
	rev =0;
	rd(rev, "error");
	return rev;
		
}

/* 
 * Not implements:
 *	siaddr		sname		boot_file	timezone	
 *	timesvr		namesvr		logsvr		cookiesvr	
 *	lprsvr		hostname	bootsize	swapsvr		
 *	rootpath	ipttl		mtu		broadcast	
 *	ntpsrv		wins		lease		dhcptype	
 *	serverid	message		tftp		bootfile	
 */
struct zone_convert_struct zc_dhcp[] = {
	{"Z_DEVICE", "interface", NULL, 0, NULL},
	{"Z_DUMMPY_DEVICE", "interface", NULL, 0, NULL},
	{"Z_LOCALIP", "ip", NULL, 0, NULL},
	{"Z_LOCALMASK", "subnet", NULL, 0, NULL},
	{"Z_HOSTNAME", "hostname"},
//	{"Z_DNS1", "dns", NULL, 0, NULL},
//	{"Z_DNS2", "dns", NULL, 0, NULL},
	{"Z_DOMAIN", "domain", NULL, 0, NULL},
	{"Z_ROUTE", "router", NULL, 0, NULL},
	{"Z_PID", "dhcpc_pid", NULL, 0, NULL},
	{NULL, NULL, NULL, 0, NULL}
};

static int dhcpc_static_convert_main(int dhcpc)
{
	char *p, *d, buf[64];
	int rev = -1;

	convert_env(zc_dhcp);
	setenv1("Z_PROTO", dhcpc?"dhcpc":"static");
	if ((p = getenv("dns")) == NULL)
		goto out;
	if ((d = strsep(&p, " ")) == NULL)
		goto out;
	setenv1("Z_DNS1", d);
	if ((d = strsep(&p, " ")) != NULL)
		setenv1("Z_DNS2", d);
	rev = 0;
out:
	return rev;
}

int __dhcpc_attach_detach_final(int attach, int i /* index */)
{
	struct {
		char *e;
		char *k;
	} dynmap[] = {
		{ "Z_LOCALIP", "dhcpc_addr"},
		{ "Z_LOCALMASK", "dhcpc_netmask"},
		{ "Z_ROUTE", "dhcpc_gateway"},
		//{ "Z_DNS", "dhcpc_primary_dns"},
		{ NULL, NULL},
	}, *p;
	char *d1, *d2;
	int rev = -1;
	
	/* for dhcp proto, I need record some info to nvram */
	
	for (p = dynmap; p->e != NULL; p++) {
		if ((p->e = getenv(p->e)) == NULL)
			goto out;
		nvram_set_i(p->k, i, g1, attach ? p->e : "");
	}
	
	d1 = nvram_safe_get_i("dhcpc_dns1_actived", i ,g1);
	d2 = nvram_safe_get_i("dhcpc_dns2_actived", i, g1);
	
	system("echo -n \"\"> /tmp/resolv.conf");
	if (strlen(d1) > 0)
		dns_append(nvram_safe_get_i("dhcpc_dns1_actived", i ,g1));
	if (strlen(d2) > 0)
		dns_append(nvram_safe_get_i("dhcpc_dns2_actived", i ,g1));
			
		
dns_ready:
	rev = 0;
out:
	debug("RETURN:%d\n", rev);
	rd(rev, "Error");
	return rev;
}

int dhcpc_static_attach_main(int dhcpc, int argc, char *argv[])
{
	int rev = -1, i;
	char *alias = NULL, *dev = NULL, *mac;
	char buf[256] = "";
	
	dhcpc_static_convert_main(dhcpc);
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	
	strcat_r(dhcpc ? "dhcpc" : "static", "_alias", buf);
	
	if ((i = nvram_find_index(buf, alias)) == -1)
		goto out;
	
	if ((rev = proto_xxx_attach_detach(1, dhcpc ? PROTO_DHCPC : PROTO_STATIC, alias)) == -1)
		goto out;
	
	
	if (dhcpc)
		__dhcpc_attach_detach_final(1, i);

	rev = 0;
	system("set >/tmp/dhcpc_static.attach");
out:
	rd(rev, "error");
	return rev;
}

int dhcpc_attach_main(int argc, char *argv[])
{
	dhcpc_static_attach_main(1, argc, argv);
	return 0;
}

int dhcpc_static_detach_main(int dhcpc, int argc, char *argv[])
{
	int rev = -1, i;
	char *alias, *dev;
	char buf[256] = "";
	
	dhcpc_static_convert_main(dhcpc);
	
	if (get_std_args(argc, argv, &alias, &dev, NULL) == -1)
		goto out;
	
	strcat_r(dhcpc ? "dhcpc" : "static", "_alias", buf);
	if ((i = nvram_find_index(buf, alias)) == -1)
		goto out;
	if (dhcpc)
		__dhcpc_attach_detach_final(0, 1);
	rev = proto_xxx_attach_detach(0, dhcpc ? PROTO_DHCPC : PROTO_STATIC, alias);
	system("set >/tmp/dhcpc_static.detach");
out:
	rd(rev, "error");
	return rev;
}

int dhcpc_detach_main(int argc, char *argv[])
{
	return dhcpc_static_detach_main(1, argc, argv);
}
/*
 * operation policy:
 * start: evel dhcpc with interfces, script opitons
 * stop:  kill dhpcpc, then dhcpc will send release packet before eval script.
 * bound: set nvram such as pid of dhcpc.
 * deconfig: reverse of bound.
 * */
int dhcpc_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", dhcpc_start_main},
		{ "stop", dhcpc_stop_main},
		{ "status", },
		{ "showconfig", dhcpc_showconfig_main},
		{ "bound", dhcpc_bound_main},
		{ "release", dhcpc_release_main},
		{ "deconfig", dhcpc_deconfig_main},
		{ "attach", dhcpc_attach_main},	/* @alias @dev */
		{ "detach", dhcpc_detach_main},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
