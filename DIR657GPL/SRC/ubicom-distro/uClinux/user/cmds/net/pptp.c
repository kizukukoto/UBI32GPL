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

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

extern int init_file(char *file);
extern void save2file(const char *file, const char *fmt, ...);

#define PPP_OPTIONS 			"/var/ppp/options"
#define CHAP_SECRETS			"/var/ppp/chap-secrets"
#define PAP_SECRETS				"/var/ppp/pap-secrets"
#define CHAP_REMOTE_NAME        "CAMEO"

extern char *PROTO_STR[];
#if 0
int ppp_xxx_start(int proto, const char *alias, const char *dev, const char *phy)
{
	int idx, rev = -1, unit;
	struct proto_pppoe poe;
	pid_t pid, ppp_pid;
	char *prefix = PROTO_STR[proto];
	char poly[512];
	
	if ((idx = nvram_find_index(strcat_r(prefix, "_alias", g1), alias)) == -1)
		goto out;
	if (init_proto_struct(proto, (struct proto_base *)poly, sizeof(poly), idx) == -1)
		goto out;
	unit = atoi(dev + 3);
	dump_proto_struct(proto, (struct proto_base *)poly, stdout);
	
	pid = fork();
	if (pid == -1)
		goto out;
	if (pid == 0) {
		fcloseall();
		if (fork() == 0) {
			setsid();
			/* Here! I want to implement as a external hook */
			
			setenv1("PPPD_PROTO", prefix);
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
			setenv1("PPPD_DAIL", poe.dial);
			setenv1("PPPD_MODE", poe.mode);
			eval("/etc/sys_hooks/pppd");
		}
		exit(0);
	}
	waitpid(pid, NULL, 0);
	sleep(2);	/* XXX: wait pppd create pid file */
	ppp_pid = __ppp_get_pid(dev);
	nvram_set_i(strcat_r(prefix, "_pid", g2), idx, g1, itoa(ppp_pid));
	
	debug("%s init pid:%s [%s]\n", itoa(ppp_pid),
		nvram_get_i(strcat_r(prefix, "_pid", g2), idx, g1));
	rev = 0;
out:
	return rev;
}
#endif


static int pptp_start(const char *alias, const char *dev, const char *phy)
{
	int idx, rev = -1, unit;
	struct proto_pptp pptp;
	char *remote,tmp[256];
	FILE *fd;
	extern int manual_start;
	
	if ((idx = nvram_find_index("pptp_alias", alias)) == -1)
		goto out;
	if (init_proto_struct(PROTO_PPTP, (struct proto_base *)&pptp,
			sizeof(pptp), idx) == -1)
		goto out;
	
	unit = atoi(dev + 3);
	dump_proto_struct(PROTO_PPTP, (struct proto_base *)&pptp, stdout);
	
	if ((remote = nvram_get_i("pptp_serverip", idx, g1)) == NULL)
		goto out;
	

	pppxxx_route_update(phy, remote, "pptp", idx);
	
	init_pap_secret((struct proto_ppp *)&pptp);
	init_chap_secret((struct proto_ppp *)&pptp);
	
	if ((fd = fopen(PPP_OPTIONS, "w")) == NULL)
		exit (-1);
	
	init_pppd_options(fd, (struct proto_ppp *)&pptp, "pptp", -1);
	fclose(fd);

	if (*((struct proto_ppp *)&pptp)->dial == '0' &&
		manual_start == 0) {
		trace("Manual mode not start!");
		return 0;
	}
	
	eval("iptables", "-I", "INPUT", "-p", "47", "-j", "ACCEPT");
	sprintf(tmp, "pppd linkname pptp unit %d pty \"pptp %s --nolaunchpppd\" &", unit, remote);
	system(tmp);
	
	nvram_set_i("pptp_status", idx, g1, "connecting");
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

#if 0
int ppp_xxx_start_main(int proto, int argc, char *argv[])
{
	struct {
		int proto;
		int (*start)(int, const char *, const char *, const char *);
	} pppxxx[] = {
		{ PROTO_PPPOE, ppp_xxx_start},
		{ PROTO_PPTP, ppp_xxx_start},
		{ PROTO_L2TP, ppp_xxx_start},
		{ 0, NULL}
	}, *px;
	int rev = -1, i;
	char *alias, *dev, *phy;
	
	debug("ok pppxxx start argc %d", argc);
	if (argc < 4)
		goto out;
	args(argc - 1, argv + 1, &alias, &dev, &phy);
	debug("ok pppxxx start dev:%s", dev);
	if (strncmp(dev, "ppp", 3) != 0)
		goto out;

	for (px = pppxxx; px->proto != 0; px++)
		if (px->proto == proto && px->start != NULL) {
			rev = px->start(px->proto, alias, dev, phy);
			break;                       
		}
out:
	return rev;
	
}
#endif

int pptp_start_main(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *dev, *phy;

	if (argc < 4)
		goto out;
	
	args(argc - 1, argv + 1, &alias, &dev, &phy);	
	if (strncmp(dev, "ppp", 3) != 0)
		goto out;
	
	eval("/etc/sysconfig/ppp", "pptp", dev, phy ,"Test");

	/* only ethX in static/dhcpc need set mtu */
	//set_mtu("pptp", alias, phy);

	rev = pptp_start(alias, dev, phy);
out:
	rd(rev, "Error");
	return rev;
}


static int pppoe_showconfig_main(int argc, char *argv[])
{
	return proto_showconfig_main(PROTO_PPPOE, argc, argv);
}

int pptp_attach_main(int argc, char *argv[])
#if 0	
if (argc < 3) {
	int i = 0;
	char tmp[512], *p;
	/*XXX: argv[1] unused! */
	printf("status [alias] [device] [phy]\n");
	printf("Avalid devives:\n");
	foreach_nvram(i, 20, p, "dhcpc_alias", tmp, 1) {
		if (p == NULL)
			continue;
		printf("alias = %s\n", p);
	}
	goto out;
}
#endif
//printf(" %s :%s\n", alias, dev);
{
	return pptp_l2tp_attach("pptp", argc, argv);
}

int pptp_detach_main(int argc, char *argv[])
{
	/* for dial-on-demand */
	eval("iptables", "-D", "FORWARD", "-o", "ppp+", "-j", "ACCEPT");
	eval("iptables", "-I", "FORWARD", "-o", "ppp+", "-j", "ACCEPT");
	eval("iptables", "-D", "INPUT", "-p", "47", "-j", "ACCEPT");
	eval("iptables", "-I", "INPUT", "-p", "47", "-j", "ACCEPT");
	return pptp_l2tp_detach_main("pptp", argc, argv);
}

int pptp_stop_main(int argc, char *argv[])
{
	
	int rev = -1, i, ppp_pid;
	char *alias = NULL, *dev = NULL;

	if (argc < 4)
		goto out;
	args(argc - 1, argv + 1, &alias, &dev);

	if ((i = nvram_find_index("pptp_alias", alias)) == -1)
		goto out;

	/* remove iptable rules */
	eval("iptables", "-D", "INPUT", "-p", "47", "-j", "ACCEPT");
	eval("iptables", "-D", "FORWARD", "-o", "ppp+", "-j", "ACCEPT");
	//eval("iptables", "-D", "FORWARD", "-o", argv[3], "-j", "ACCEPT");
	if ((kill_pppd_from_pidfile(SIGINT, dev)) == -1)
		goto out;
	/* FIXME: kill again: to avoid pppd is idled, or responed,
	 * or something unknow happen.
	 * */
	sleep(1);
	kill_pppd_from_pidfile(SIGKILL, dev);
				
	rev = 0;
out:
	debug( "return %d", rev);
	return rev;
}

int pptp_status_main(int argc, char *argv[])
{
	if (argc < 4)
		help_proto_args(stderr, 1);
	return proto_xxx_status2(PROTO_PPTP, argv[1], argv[2], argv[3]);
}

int pptp_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", pptp_start_main}, /* @alias @dev, @phy */
		{ "stop", pptp_stop_main},   /* @alias */
		{ "status", pptp_status_main}, 
		{ "showconfig", pppoe_showconfig_main},
		{ "attach", pptp_attach_main}, /* @alias */
		{ "detach", pptp_detach_main}, /* @alias */
		{ NULL, NULL}
	};
	DEBUG_MSG("%s, Begin, args :\n", __FUNCTION__);
	{
		int i = 0;
		for(i = 0; i < argc ; i++){
			DEBUG_MSG("%s ", argv[i] ? argv[i] : "");
		}
	}
	DEBUG_MSG("\n");

	return lookup_subcmd_then_callout(argc, argv, cmds);
}
/*
 * Most all of operations has 2 basic args @alias, @device.
 * 
 * */
