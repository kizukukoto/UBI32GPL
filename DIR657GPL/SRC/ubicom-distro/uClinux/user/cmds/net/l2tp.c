#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

extern char *PROTO_STR[];

#define L2TP_CONF				"/var/etc/l2tp.conf"

/* Init File and clear the content */
int init_file(char *file)
{
	FILE *fp;

	DEBUG_MSG("%s: file=%s\n", __func__,file);
	if ((fp = fopen(file ,"w")) == NULL) {
		DEBUG_MSG("Can't open %s\n", file);
		exit(1);
	}

	fclose(fp);
	return 0;
}


/* Save data into file	
 * Notice: You must call init_file() before save2file()
 */
void save2file(const char *file, const char *fmt, ...)
{
	char buf[10240];
	va_list args;
	FILE *fp;

	DEBUG_MSG("%s: file=%s\n", __func__,file);
	if ((fp = fopen(file ,"a")) == NULL) {
		DEBUG_MSG("%s: file=%s\n", __func__,file);
		exit(1);
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	va_start(args, fmt);
	fprintf(fp, "%s", buf);
	va_end(args);
	fclose(fp);
}

#if 0//in pptp.c
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

static int init_l2tp_conf(struct proto_l2tp *l2tp)
{

	system("mkdir /var/etc");	
	init_file(L2TP_CONF);
	save2file(L2TP_CONF, 
		"global\n"
		"load-handler \"sync-pppd.so\"\n"
		"load-handler \"cmd.so\"\n"
		"listen-port 1701\n"
		"section sync-pppd\n"
		"pppd-path /sbin/pppd\n"
		"section peer\n"
		"persist 0\n"
		"retain-tunnel 0\n"
		"peer %s\n"
		"port 1701\n"
		"lac-handler sync-pppd\n"
		"hide-avps no\n"
		"section cmd\n",
		l2tp->serverip);
}
static int init_xl2tp_conf(struct proto_l2tp *l2tp, const char *ppp_options_file)
{
#define XL2TPD_LNC_CONF	"/etc/xl2tpd/xl2tpd.conf"
	FILE *fd;
	
	if ((fd = fopen(XL2TPD_LNC_CONF, "w")) == NULL)
		return -1;
	
	fprintf(fd,
"[global]\n"
"[lac linux]\n"
"lns = %s\n"
"require authentication = no\n"
"pppoptfile = %s\n"
"redial = yes\n"
"autodial = yes\n",
	l2tp->serverip,
	ppp_options_file == NULL ? PPP_OPTIONS : ppp_options_file);
	fclose(fd);
	return 0;
}

static int l2tp_write_config(const struct proto_l2tp *l2tp, int unit)
{
	FILE *fd;
	int rev = -1;
#define PPP_OPTIONS_XL2TP_LNC	"/etc/ppp/options.xl2tp.lnc"
	init_pap_secret((struct proto_ppp *)l2tp);
	init_chap_secret((struct proto_ppp *)l2tp);

	if ((fd = fopen(PPP_OPTIONS_XL2TP_LNC, "w")) == NULL)
		goto out;
	
	init_pppd_options(fd, (struct proto_ppp *)l2tp, "l2tp", unit);
	fclose(fd);
	
	init_xl2tp_conf(l2tp, PPP_OPTIONS_XL2TP_LNC);
	rev = 0;
out:
	return rev;
}
static int l2tp_start(const char *alias, const char *dev, const char *phy)
{
	int rev = -1, idx, unit;
	struct proto_l2tp l2tp;
	char args[256];
	extern int manual_start;

	if ((idx = nvram_find_index("l2tp_alias", alias)) == -1)
		goto out;
	if (init_proto_struct(PROTO_L2TP, (struct proto_base *)&l2tp,
				sizeof(l2tp), idx) == -1)
		goto out;
	
	unit = atoi(dev + 3);
	dump_proto_struct(PROTO_L2TP, (struct proto_base *)&l2tp, stdout);
	
	pppxxx_route_update(phy, l2tp.serverip, "l2tp", idx);
	
	if (l2tp_write_config(&l2tp, unit) != 0)
		goto out;
	//ppp_xxx_start(PROTO_L2TP, alias, dev, phy);

	setenv("PPP_SERVER", l2tp.serverip, 1);
	set_ppp_env((struct proto_ppp *)&l2tp);
	/* FIXME: how about xl2tpd VPN server? */
	eval("killall", "xl2tpd");
	if (*((struct proto_ppp *)&l2tp)->dial == '0' &&
		manual_start == 0) {
		trace("Manual mode not start!");
		return 0;
	}
	if (fork() == 0) {
		setsid();
		execl("/bin/xl2tpd_lnc", "xl2tpd_lnc", dev, "Delete",
			"xl2tpd -c "XL2TPD_LNC_CONF, NULL);
	}
	nvram_set_i("l2tp_status", idx, g1, "connecting");
	return 0;
	rev = 0;
out:
	rd(rev, "error ");
	return rev;
}

#if 0//in pptp.c
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

int l2tp_start_main(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *dev, *phy;
	
	if (argc < 4)
		goto out;

	args(argc - 1, argv + 1, &alias, &dev, &phy);
	
	if (strncmp(dev, "ppp", 3) != 0)
		goto out;

	/* only ethX in static/dhcpc need set mtu */
	//set_mtu("l2tp", alias, phy);

	rev = l2tp_start(alias, dev, phy);
out:
	rd(rev, "error");
	return rev;
}


static int l2tp_showconfig_main(int argc, char *argv[])
{
	return proto_showconfig_main(PROTO_L2TP, argc, argv);
}

int l2tp_attach_main(int argc, char *argv[])
{
	eval("iptables", "-D", "FORWARD", "-o", "ppp+", "-j", "ACCEPT");
	eval("iptables", "-A", "FORWARD", "-o", "ppp+", "-j", "ACCEPT");
	return pptp_l2tp_attach("l2tp", argc, argv);
}

int l2tp_detach_main(int argc, char *argv[])
{
	return pptp_l2tp_detach_main("l2tp", argc, argv);
}
static int l2tp_updown_iptables(int up, const char *ppp)
{
	char *act;
	act = up ? "-A":"-D";
	eval("iptables", "-D", "FORWARD", "-o", "ppp+", "-j", "ACCEPT");
}
int l2tp_stop_main(int argc, char *argv[])
{
	
	int rev = -1, i, ppp_pid;
	char *alias = NULL, *dev = NULL;
	
	if (argc < 3)
		goto out;
	args(argc - 1, argv + 1, &alias, &dev);
	
	if ((i = nvram_find_index("l2tp_alias", alias)) == -1)
		goto out;
/*	FIXME: if no pppd is runing?
	if ((ppp_pid = __ppp_get_pid(dev)) <= 0)
		goto out;
	kill(ppp_pid, SIGINT);
*/
	l2tp_updown_iptables(0, dev);
	trace("L2TPD kill pppd:%d\n", ppp_pid);
	eval("killall", "xl2tpd_lnc");
	eval("killall", "ip");
	rev = 0;
out:
	return rev;
}

int l2tp_status_main(int argc, char *argv[])
{
	if (argc < 4)
		help_proto_args(stderr, 1);
	return proto_xxx_status2(PROTO_L2TP, argv[1], argv[2], argv[3]);
}

int l2tp_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", l2tp_start_main}, /* @alias @dev, @phy */
		{ "stop", l2tp_stop_main},   /* @alias */
		{ "status", l2tp_status_main},
		{ "showconfig", l2tp_showconfig_main},
		{ "attach", l2tp_attach_main}, /* @alias */
		{ "detach", l2tp_detach_main}, /* @alias */
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
/*
 * Most all of operations has 2 basic args @alias, @device.
 * 
 * */
