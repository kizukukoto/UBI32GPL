#include <stdio.h>
#include <stdlib.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "nvram_ext.h"
#include "libcameo.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/*
void dump_proto_status(const char *dev, FILE *strem)
{
	fprintf(stream, "dev: %s, ADDR: %s/%s router: %s, dns: %s\n",
		nvram_safe_get());
}
*/
int static_create_env(int i, const char *dev)
{
	struct {
		char *key;
		char *env;
	} m[] = {
		{ "static_hostname", "hostname"},
		{ "static_addr", "ip"},
		{ "static_netmask", "subnet"},
		{ "static_gateway", "router"},
		{ NULL, NULL}
	}, *p;
	char buf[128], *k;
	int rev = -1;

	for (p = m; p->key != NULL; p++) {
		if ((k = nvram_get_i(p->key, i, g1)) == NULL)
			goto out;
		debug("set:%s = %s", p->env, k);
		setenv1(p->env, k);
	}
		
	if ((k = nvram_get_i("static_primary_dns", i, g1)) != NULL)
		strcpy(buf, k);
	if ((k = nvram_get_i("static_secondary_dns", i, g1)) != NULL) {
		strcat(buf, " ");
		strcat(buf, k);
	}
	setenv1("dns", buf);
	rev = 0;
out:
	rd(rev, "");
	return rev;
}

int __static_start_stop(int start, const char *alias, const char *dev)
{
	int rev = -1, i;
	char tmp[512], idx[8];

	if ((i = nvram_find_index("static_alias", alias)) == -1)
		goto out;
	strncpy(idx, itoa(i), sizeof(idx));
	setenv1("interface", dev);
	setenv1("proto", "static");
	static_create_env(i, dev);
	debug("start static:%s", dev);
	nvram_set_i("static_status", i, g1, start ? "connecting":"disconnected");
	rev = eval("/etc/udhcpc/sample.script", start ? "bound":"deconfig");
out:
	rd(rev, "ErrOn:%d static->%s, alias: %s, dev: %s",
		rev, start ? "start ":"stop", alias, dev);
	return rev;
}


int static_start_main(int argc, char *argv[])
{
	int rev = -1;
	
	DEBUG_MSG("static_start_main:\n");
	if (argc < 3) {
		printf("miss [static_alias] [device]\n");
		goto out;
	}
	DEBUG_MSG("dhcpc_start_main: __static_start====>\n");

	// set clone mac
	clone_mac("static", argv[1], argv[2]);	// argv[1]: alias, argv[2]: dev
	set_mtu("static", argv[1], argv[2]);
	rev = __static_start_stop(1, argv[1], argv[2]);
out:
	return rev;
}

int static_stop_main(int argc, char *argv[])
{
	int rev = -1;
	
	if (argc < 3) {
		printf("miss [static_alias] [device]\n");
		goto out;
	}

	rev = __static_start_stop(0, argv[1], argv[2]);
out:
	return rev;
}

static int static_showconfig_main(int argc, char *argv[])
{
	return proto_showconfig_main(PROTO_STATIC, argc, argv);
}


int static_status_main(int argc, char *argv[])
{
	if (argc < 3)
		help_proto_args(stderr, 1);
	return proto_xxx_status2(PROTO_STATIC, argv[1], argv[2], argv[2]);
}

static int __updown_iplist(int up, const char *alias, const char *dev)
{
	int i, rev = -1;
	char *p, *_p, *s, ifname[] = "ethXXXXX", *mask;
#define MAX_ADDRESSES	999
	
	if ((i = nvram_find_index("static_alias", alias)) == -1)
		goto out;
	if ((p = nvram_get_i("static_iplist", i, g1)) == NULL)
		goto out;
	if ((mask = nvram_get_i("static_netmask", i, g1)) == NULL)
		goto out;
	if (strlen(p) == 0) 
		return 0;
	if ((p = strdup(p)) == NULL)
		goto out;
	_p = p;
	i = 0;
	
	/* 
	 * ex: smtp@192.168.3.2,dns@192.168.3.3
	 * */
	while ((s = strsep(&p, ",")) != NULL) {
		sprintf(ifname, "%s:%d", dev, i++);
		if (strsep(&s, "@") == NULL)
			goto free;
		debug( "ip alias extension: %s:%s %s %s\n", up ? "up" : "down",
			ifname, s , mask, p);
		//ifconfig(ifname, up ? IFUP : 0, up ? s : "0.0.0.0", "255.255.255.0");
		eval("ifconfig", ifname, s, "netmask", mask , up ? "up" : "down");
		if (i >= MAX_ADDRESSES)
			break;
	}
	rev = 0;
free:
	free(_p);
out:
	rd(rev, "add static ip list error");
	return rev;
}


static inline int updown_iplist(int up, int argc, char *argv[])
{
	char *alias = NULL, *dev = NULL;
	args(argc - 1, argv + 1, &alias, &dev);
	if (alias == NULL || dev == NULL)
		return -1;
	
	return __updown_iplist(up, alias, dev);
}

int static_attach_main(int argc, char *argv[])
{
	if (dhcpc_static_attach_main(0, argc, argv) == -1)
		return -1;
	return updown_iplist(1, argc, argv);
}

int static_detach_main(int argc, char *argv[])
{
	if (dhcpc_static_detach_main(0, argc, argv) == -1)
		return -1;
	return updown_iplist(0, argc, argv);
}

struct proto_struct proto_static = {
	name: PROTO_STATIC_STR,
	start: static_start_main,
	status: NULL,
	showconfig: static_showconfig_main
};

int static_main(int argc, char *argv[])
{
	
	struct subcmd cmds[] = {
		{ "start", static_start_main},
		{ "stop", static_stop_main},
		{ "status", static_status_main},
		{ "showconfig", static_showconfig_main},
		{ "attach", static_attach_main},
		{ "detach", static_detach_main},
		{ NULL, NULL}
	};
	
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
