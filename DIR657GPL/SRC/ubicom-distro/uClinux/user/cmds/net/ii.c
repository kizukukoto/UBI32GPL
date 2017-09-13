#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "libcameo.h"
#include "nvram_ext.h"

extern int get_std_args(int, char **, char **, char **, char **);
extern struct proto_struct proto_static;
extern struct proto_struct proto_dhcpc;
static struct proto_struct *proto_ops[] = {
	&proto_static,
	&proto_dhcpc,
	NULL
};

struct proto_call {
	char *proto;
	int (*call)(struct info_if *);
};

static struct proto_call *
lookup_proto_call(struct proto_call *pc, const char *proto)
{
	for (; pc->proto != NULL; pc++) {
		if (strcmp(pc->proto, proto) != 0)
			continue;
		return pc;
	}
	return NULL;
}

extern int dhcpc_bound_main(int, char **);  
extern int dhcpc_deconfig_main(int, char **);
//extern proto_static_main(int, char **);
typedef int (*main_t)(int, char **);
static int
ii_do_proto_bound_or_deconfig(
	struct info_if *ii,
	int argc,
	char *argv[],
	int bound)
{
	struct {
		char *proto;
		int (*bound_main)(int, char **);
		int (*deconfig_main)(int, char **);
	} proto[] = {
		{ PROTO_DHCPC_STR, dhcpc_bound_main, dhcpc_deconfig_main},
		{ PROTO_STATIC_STR, NULL, NULL},
		{ PROTO_PPPOE_STR, NULL, NULL},
		{ NULL, NULL}
	}, *p;
	int rev = -1;
	main_t main_fn;
	
	syslog(LOG_INFO, "*******%s ! PROTOCOLS %s %s %s********\n",
			bound?"BOUND":"DECONFIG",
			ii->alias, ii->dev, getenv("dhcpc_pid"));
	for (p = proto; p->proto != NULL; p++) {
		if (strcmp(p->proto, ii->proto) != 0)
			continue;
		main_fn = (bound) ? p->bound_main : p->deconfig_main;
		if (main_fn == NULL) {
			syslog(LOG_INFO, "*** funstion not implement!");
			goto out;
		}
		rev = main_fn(argc, argv);
	}
out:
	return rev;
}
int ii_do_proto_status()
{
	return 0;
}

int __init_ii(struct info_if *ii, int i)
{
	int rev = -1;
	char tmp[512];

	ii->index = i;
	ii->dev = nvram_safe_get(strcat_r("if_dev", itoa(i), tmp));
	
	//DEBUG_MSG("%s, set ii's if_dev%d  = %s\n",__FUNCTION__,i,ii->dev);
	ii->phy = nvram_safe_get(strcat_r("if_phy", itoa(i), tmp));
	ii->alias = nvram_safe_get(strcat_r("if_alias", itoa(i), tmp));
	ii->proto = nvram_safe_get(strcat_r("if_proto", itoa(i), tmp));
	ii->proto_alias = nvram_safe_get(strcat_r("if_proto_alias", itoa(i), tmp));
	ii->mode = nvram_safe_get(strcat_r("if_mode", itoa(i), tmp));
	ii->trust = nvram_safe_get(strcat_r("if_trust", itoa(i), tmp));
	ii->services = nvram_safe_get(strcat_r("if_services", itoa(i), tmp));
	ii->route = nvram_safe_get(strcat_r("if_route", itoa(i), tmp));
	ii->icmp = nvram_safe_get(strcat_r("if_icmp", itoa(i), tmp));
	rev = 0;
	return rev;

}

int update_ii(struct info_if *ii)
{
	char tmp[512];
	int i = ii->index;
	//DEBUG_MSG("%s, begin\n",__FUNCTION__);
	nvram_set(strcat_r("if_dev", itoa(i), tmp), ii->dev);
	//DEBUG_MSG("%s, set if_dev%d  = %d\n",__FUNCTION__,i,ii->dev);
	nvram_set(strcat_r("if_phy", itoa(i), tmp), ii->phy);
	nvram_set(strcat_r("if_alias", itoa(i), tmp), ii->alias);
	nvram_set(strcat_r("if_proto", itoa(i), tmp), ii->proto);
	nvram_set(strcat_r("if_proto_alias", itoa(i), tmp), ii->proto_alias);
	nvram_set(strcat_r("if_mode", itoa(i), tmp), ii->mode);
	nvram_set(strcat_r("if_trust", itoa(i), tmp), ii->trust);
	nvram_set(strcat_r("if_services", itoa(i), tmp), ii->services);
	nvram_set(strcat_r("if_route", itoa(i), tmp), ii->route);
	nvram_set(strcat_r("if_icmp", itoa(i), tmp), ii->icmp);
	__init_ii(ii, i);
	return 0;
}

/*  Bug : no route paramter */
void dump_ii(struct info_if *ii)
{
	printf("[%s, %d]\n", ii->alias, ii->index);
	printf(	"\tdev: %s@%s\n"
		"\tproto: %s\n"
		"\tproto_alias: %s\n"
		"\tmode: %s\n"
		"\troute: %s\n",
		ii->dev, ii->phy,
		ii->proto, ii->proto_alias, ii->mode, ii->route);
	
}

int init_ii(struct info_if *ii, const char *alias)
{
	int i, rev = -1;

	if ((i = nvram_find_index("if_alias", alias)) == -1) {
		err_msg("init_ii: nvram_find_index fail!\n");
		goto out;
	}	
	if (__init_ii(ii, i) == -1)
		goto out;
	rev = 0;
out:
	return rev;
}

int ii_start_main_by_alias(const char *alias)
{
	int argc = 2;
	char *argv[2] = {
		"start",
		(char *)alias
	};

	return ii_start_main(argc, argv);
}


int ii_status_main(int argc, char *argv[])
{
	int rev = -1;
	struct info_if ii;
	
	if (argc < 2) {
		printf("ii status <alias>\n");
		goto out;
	}
	
	if (init_ii(&ii, argv[1]) != 0)
		goto out;
	
	if (ii.dev == NULL)
		goto out;
	if (ii.phy == NULL)
		goto out;

	rev = proto_xxx_status(ii.dev, ii.phy);
out:
	return rev;	
}

int ii_showall_main(int argc, char *argv[]) {
	int i = 0;
	char tmp[512], *p;
	struct info_if ii;
	/*XXX: argv[1] unused! */
	foreach_nvram(i, 20, p, "if_alias", tmp, 1) {
		if (p == NULL)
			continue;
		if (__init_ii(&ii, i) == -1)
			continue;
		dump_ii(&ii);
		printf("\tstatus: ");
		ii_status_main(2, (char *[]){"status", ii.alias });
		putchar('\n');
	}
	return 0;
}
/*
 * list trust interaces aliae name ex: "WAN_primary,WAN_primary"
 * EX:	WAN: "LAN,DMZ" or "all"
 * 	LAN: "DMZ"
 * 	LAN: ""
 * */
static int trust_updown(int up, struct info_if *ii)
{
	char t[1024], *p, *s, *ifname;
	char *_updown = up ? "-A":"-D";
	
	p = strncpy(t, ii->trust, sizeof(t));
	/* XXX: "all" to allow all but "strstr"... */
	if (strstr(p, "all") != NULL) {
		eval("iptables", "-t", "filter", _updown, "FORWARD",
			"-o", ii->dev, "-m", "state", "--state", "NEW",
			"-j", "ACCEPT");
		return 0;
	}
	while (p != NULL && (s = strsep(&p, ",")) != NULL) {
		if ((ifname = __nvram_map("if_alias", s, "if_dev")) == NULL)
			continue;
		eval("iptables", "-t", "filter", _updown, "FORWARD",
			"-i", ifname, "-o", ii->dev, "-m", "state",
			"--state", "NEW", "-j", "ACCEPT");
	}
	return 0;
}

static inline int ii_policy_updown(int up, struct info_if *ii)
{
	char *_updown = up ? "-A":"-D";
	
	eval("iptables", _updown, "INPUT", "-i", ii->dev, "-m",
		"state", "--state", "ESTABLISHED,RELATED", "-j", "ACCEPT");

	if (strcmp(ii->icmp, "1") == 0)
		eval("iptables", _updown, "INPUT", "-i", ii->dev,
			"-p", "icmp", "-j", "ACCEPT");
	
	trust_updown(up, ii);
	if (strcmp(ii->mode,"nat") == 0) {
		eval("iptables", "-t", "nat", _updown, "POSTROUTING",
			"-o", ii->dev, "-j", "MASQUERADE");
	}
	return 0;
}

static int 
init_ii_by_alias_or_dev(
	struct info_if *ii,
	const char *alias,
	const char *dev)
{
	int rev = -1, i;
	if ((rev = init_ii(ii, alias == NULL ? "" : alias)) == -1) {
		if (dev == NULL)
			goto out;
		if ((i = nvram_find_index("if_dev", dev)) == -1)
			goto out;
		rev = __init_ii(ii, i);
	}
out:
	return rev;
}
/*
 * __ii_attatch_detach_main: call by externl,
 * 
 * called after proto(pppX, ethX) get ip and initaled address info,
 * that is meanning a interface either pppX or ethX is already.
 * 
 * In the case of "attach", we should NOTIFY "protocal" layer, and NOTIFY "zone".
 * 
 * Now! we can startup(or stop) some services which they are interface dependence 
 * ex: startup MASQ, DNAT, FIREWALL, "WAN" services such as NTP, DDNS ...
 * 	or "LAN", "DMZ" services such as "DHCPD", "UPNP" ...
 * 
 * */
extern int ii_services_updown(int up, struct info_if *ii);
int __ii_attatch_detach_main(int attach, const char *dev, const char *proto)
{
	char *p, *as, buf[128];
	int rev = -1, found = 0, i = 0;
	struct info_if ii;
	
	//debug( "%s, attach = %d, dev = %s, proto = %s\n", __FUNCTION__,attach,dev,proto);
	/*
	 * Initial/search interface info, find which IF is ready
	 * */
	foreach_nvram(i, 20, p, "if_proto", g1, 1) {
		if (p == NULL)
			continue;
		/* search dev and protocol, XXX: disabled? */
		//debug("proto<%s == %s>", p, proto);
		debug("if_proto%d = %s", i, p);
		if (strcmp(p, proto) != 0)
			continue;
		sprintf(buf, "if_dev%d", i);
		
		if (strcmp(dev, nvram_safe_get(buf)) != 0){
			//debug("%s != nvram_safe_get(%s) : %s, continue",
			//		dev, buf, nvram_safe_get(buf));
			continue;
		}
		found = 1;
		break;
	}
	if (found == 0){
		err_msg("can't found if_proto base on if_devXXX: %s,  ",  dev);
		goto out;
	}
	
	/* After found index of IF, get the alias of protocol */
	if ((as = nvram_get_i("if_proto_alias", i, g1)) == NULL){
		err_msg("can't get if_proto_alias");
		goto out;
	}
	/* call proto layer */
	/*
	 * Identify by @if_dev, @if_proto to get @proto_alias
	 * */
	
	if (init_ii_by_alias_or_dev(&ii, nvram_get_i("if_alias", i, g1), NULL) == -1) {
		err_msg("init_ii_by_alias_or_dev() failed");
		goto out;
	}
	debug("%s->%s( %s, %s,%s)", proto, attach ? "attach" : "detach", as, dev, proto);
	
	if (proto_operator_main(5, (char *[]) { attach == 1 ? 
			"attach":"detach", as, dev, proto, ii.phy}) == -1)
	{
		err_msg("proto_operator_main() failed");
		goto out;
	}

	/*
	 * begin Notify each subsystem.
	 * */
	if (ii_policy_updown(attach, &ii) == -1)
		goto out;
	if (ii_services_updown(attach, &ii) == -1)
		goto out;
	/*
	 * Firewall security stuff
	 * */
	//attach ? fw_firewall_start(ii.dev) : fw_firewall_stop(ii.dev);
	attach ? fw_forward_port_start(ii.dev) : fw_forward_port_stop(ii.dev);
	if (attach) {
		/* always keep at last rule of FORWARD chain */
		eval("iptables", "-D", "FORWARD", "-m", "state", "--state",
			"ESTABLISHED,RELATED", "-j", "ACCEPT");
		eval("iptables", "-A", "FORWARD", "-m", "state", "--state",
			"ESTABLISHED,RELATED", "-j", "ACCEPT");
	}
	/*
	 * call zone layer
	 * */
	//debug("call zone_attach_detach");
	//rev = zone_attach_detach(attach, &ii);
	rev = 0;
out:
	rd(rev, "error");
	return rev;
}

int ii_attach_detach_main(int argc, char *argv[])
{
	char *dev = NULL, *proto = NULL;
	int rev = -1, i, tmp[256];
	
	args_dev_proto(argc, argv, &dev, &proto);
	if (dev == NULL || proto == NULL)
		goto out;
	/* @alias is unknow now */
	rev = __ii_attatch_detach_main(
		strstr(argv[0], "attach") == NULL? 0 : 1, dev, proto);
out:
	rd(rev, "error");
	return rev;
}
#define SYS_HOOKS_SCRIPT	"/etc/sys_hooks/hook.script"
/*
 * A hook helper
 * I use this feature for init brctl of wireless & lan...
 * */
int ii_hook(int start, struct info_if *ii)
{
	const char *hook = SYS_HOOKS_SCRIPT;

	setenv1("IIIFNAME", ii->dev);
	setenv1("IIIFPHY", ii->phy);
	setenv1("IIPROTO", ii->proto);
	setenv1("IIPROTO_ALIAS", ii->proto_alias);
	setenv1("IIALIAS", ii->alias);
	setenv1("IIACTION", start == 1 ? "start" : "stop");
	return eval(hook, start == 1 ? "start" : "stop");
}
#define DUMP_ARGS(argc, argv)					\
{								\
	int i = 0;						\
	__rd__("%s, Begin, args :\n", __FUNCTION__);		\
		for(i = 0; i < argc ; i++)			\
			__rd__("%s ", argv[i] ? argv[i] : "");	\
	__rd__("\n");						\
}
/*
 * */
int manual_start = 0;
int ii_start_main(int argc, char *argv[]) {
	char *alias = NULL, *dev = NULL;
	struct info_if ii;
	int rev = -1;
	
	if (argc >= 3) {
		if (strncmp(argv[2], "manual", strlen(argv[2])) == 0)
			manual_start = 1;
	} 
	if (get_std_args(argc, argv, &alias, NULL, NULL) == -1){
		err_msg("get_std_args fail");
		goto out;
	}	
	if (init_ii(&ii, alias) == -1){
		err_msg("init_ii (%s) fail", alias);
		goto out;
	}	
	
	if (ii_hook(1, &ii) != 0) {
		err_msg("%s: ii_hook fail!");
		goto out;
	}	

	rev = proto_operator_main(5, (char *[]){ "start",
			ii.proto_alias, ii.dev, ii.proto, ii.phy});

	sprintf(g1, "%s %s %s %s", ii.proto_alias, ii.dev,
			ii.proto, ii.phy);
	/* XXX: ? what for this
	debug("Start to:[%s]", g1);
	nvram_set_i("if_start", ii.index, g2, g1);
	debug( "PPTP_STATUS:%s", nvram_safe_get("pptp_status2"));
	*/
out:
	
	rd(rev, "");
	return rev;
}

int ii_stop_main(int argc, char *argv[])
{
	char *alias = NULL, *dev = NULL, *s, *sp[10];
	struct info_if ii;
	int rev = -1, i;
	
	//DUMP_ARGS(argc, argv);

	if (argc < 2) {
		err_msg("%s, argc < 2, go out");
		goto out;
	}
	
	args(argc - 1, argv + 1, &alias);
	if (init_ii(&ii, alias) == -1) {
		err_msg("init_ii with alias = %s failed, go out\n", alias);
		goto out;
	}
	dump_ii(&ii);
	/*
	if (( s = nvram_get_i("if_stop", ii.index, g1)) == NULL) {
		debug("Unknow how to stop II:%s", ii.alias);
		goto out;
	}
	debug( "stop cmd:[%s]", s);
	
	if ((s = strdup(s)) == NULL)
		goto out;
	if ((i = split(s, " ", sp, 1)) < 4){
		debug( "stop cmds error:%s", s);
		goto e1;
	}
	*/
	if (ii_hook(0, &ii) != 0)
		goto out;
	
	rev = proto_operator_main(5, (char *[]){ "stop",
			ii.proto_alias, ii.dev, ii.proto, ii.phy});
	/*
	rev = proto_operator_main(5, (char *[]){ "stop",
			sp[0], sp[1], sp[2], sp[3]});
	nvram_unset_i("if_stop", ii.index, g1);
	*/
e1:
	//free(s);
out:
	rd(rev, "error");
	return rev;
	
}
int ii_status_main_2(int argc, char *argv[])
{
	char *alias = NULL, *dev = NULL, *s, *sp[10];
	struct info_if ii;
	int rev = -1, i;
	
	if (argc < 2){
		err_msg("argc < 2, go out");
		goto out;
	}
	
	args(argc - 1, argv + 1, &alias);
	if (init_ii(&ii, alias) == -1){
		err_msg("init_ii with alias = %s failed, go out", alias);
		goto out;
	}
	//dump_ii(&ii);
	rev = proto_operator_main(5, (char *[]){ "status",
			ii.proto_alias, ii.dev, ii.proto, ii.phy});
out:
	return rev;
}

int ii_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", ii_start_main, "start up device by <if_aliasX>"},/* start by @alias */
		{ "stop", ii_stop_main, "stop device by <if_aliasX>"},	/* stop by @alias */
		{ "status", ii_status_main_2, "show status"},
		{ "showconfig", ii_showall_main, "dump config"},
		{ "attach", ii_attach_detach_main, "used by system"}, /* @device, @proto */
		{ "detach", ii_attach_detach_main, "used by system"}, /* @device, @proto */
		{ NULL, NULL}
	};
	
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

