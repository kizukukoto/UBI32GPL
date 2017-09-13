#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "nvram_ext.h"
#define INBOUND_FILTER_PREFIX	"inbound_list_"
#define INBOUND_FILTER_MAX	10

#if 1
#define DEBUG_MSG	cprintf
#else
#define DEBUG_MSG
#endif

static struct inbound_env {
	char *table;
	char *chain;
	char *proto;
	char *port;
	char *policy;
	char *key_store;
};

static int inbound_search(const char *inb_name, char *buf)
{
	char *p, *pt, t[512];
	int i = 1, ret = -1;

	if (inb_name == NULL)
		goto out;

	for (; i <= INBOUND_FILTER_MAX; i++) {
		pt = nvram_safe_get_i(INBOUND_FILTER_PREFIX, i, g1);

		if (strlen(pt) <= 21)
			goto out;

		p = t;
		strcpy(t, pt);

		if (strcmp(inb_name, strsep(&p, ";")) == 0) {
			strcpy(buf, pt);
			goto suc;
		}
	}
suc:
	ret = 0;
out:
	rd(ret, "inbound name not found");
	return ret;
}

static void init_inbound_env(struct inbound_env *inb_env)
{
	inb_env->table = getenv("INBOUND_TABLE");
	inb_env->chain = getenv("INBOUND_CHAIN");
	inb_env->proto = getenv("INBOUND_PROTO");
	inb_env->port = getenv("INBOUND_PORT");
	inb_env->policy = getenv("INBOUND_POLICY");
	inb_env->key_store = getenv("INBOUND_STORE");

	if (!inb_env->table || !inb_env->chain)
		inb_env->table = inb_env->chain = NULL;

	if (!inb_env->proto || !inb_env->port)
		inb_env->proto = inb_env->port = NULL;

	if (!inb_env->policy)
		inb_env->policy = "ACCEPT";

	if (inb_env->key_store)
		nvram_unset(inb_env->key_store);
}

static void inbound_do_keystore(const char *cmds, const char *key)
{
	char *store, t[512];

	if (cmds == NULL || *cmds == '\0')
		return;
	if (key == NULL || *key == '\0')
		return;

	store = nvram_get(key);
	sprintf(t, "%s;%s", store?store:"", cmds);
	printf("key [%s]\n", key);
	printf("t [%s]\n", t);
	nvram_set(key, t);
}

static void inbound_do_firewall(
	int up,
	const char *dev,
	const char *ip_pair,
	const char *policy,
	struct inbound_env *inb_env)
{
	char cmds[512];
	char *inb_table, *inb_chain, *inb_proto, *inb_port;
	char *ip_start = strsep(&ip_pair, "-");
	char *ip_end = ip_pair;

	/* @cmds: iptables [-t @inb_env->table] -@up INPUT -i @dev
	 * 	  [-p @inb_env->proto] [--dport @inb_env->port]
	 * 	  -m iprange [!] --src-range @ip_start-@ip_end -j @policy */

	sprintf(cmds, "iptables %s%s -%s %s -i %s %s%s %s%s -m iprange "
		"%s --src-range %s-%s -j %s",
		inb_env->table ? "-t ": "",
		inb_env->table ? inb_env->table: "",
		up ? "I": "D",
		inb_env->chain ? inb_env->chain: "INPUT",
		dev,
		inb_env->proto ? "-p ": "",
		inb_env->proto ? inb_env->proto: "",
		inb_env->port ? "-m mport --dports ": "",
		inb_env->port ? inb_env->port: "",
		//(*policy == '0')? "!": "",
		"",
		ip_start,
		ip_end,
		(*policy == '0')? "DROP": inb_env->policy);

	DEBUG_MSG("__inbound_updown, cmds: %s\n", cmds);
	system(cmds);

	if (inb_env->key_store)
		inbound_do_keystore(cmds, inb_env->key_store);
}

//inbound_list_1=1;0;*,0.0.0.0-255.255.255.255,0.0.0.0-255.255.255.255
//inbound_list_2=2;0;*,0.0.0.0-255.255.255.255,0.0.0.0-255.255.255.255,0.0.0.0-255.255.255.255
//inbound_list_3="name;0;*,1.1.1.1-2.2.2.2,3.0.0.0-3.0.0.4,192.168.0.0-192.168.0.255"
static void __inbound_updown(int up, char *raw, const char *dev)
{
	struct inbound_env inb_env;
	char *p, *policy, *ip_pair;

	p = raw;
	strsep(&p, ";");		// inbound filter name, ignore it
	policy = strsep(&p, ";");	// policy, ALLOW or DENY
	strsep(&p, ",");		// unused *, ignore it

	init_inbound_env(&inb_env);

	if (*policy == '1') {	// ALLOW IPs
		char cmds[512];

		sprintf(cmds, "iptables %s%s -%s %s -i %s %s%s %s%s -j DROP",
			inb_env.table ? "-t ": "",
			inb_env.table ? inb_env.table: "",
			up ? "I": "D",
			inb_env.chain ? inb_env.chain: "INPUT",
			dev,
			inb_env.proto ? "-p ": "",
			inb_env.proto ? inb_env.proto: "",
			inb_env.port ? "--dport ": "",
			inb_env.port ? inb_env.port: "");
		DEBUG_MSG("__inbound_updown cmds: %s\n", cmds);
		system(cmds);

		if (inb_env.key_store)
			inbound_do_keystore(cmds, inb_env.key_store);
	}

	while (ip_pair = strsep(&p, ","))
		inbound_do_firewall(up, dev, ip_pair, policy, &inb_env);
}

static int inbound_updown(int up, int argc, char **argv)
{
	int ret = -1;
	char *inb_name, *dev, raw[512];

	if (get_std_args(argc, argv, &inb_name, &dev, NULL) == -1)
		goto out;
	if (inb_name == NULL || dev == NULL)
		goto out;
	if (inbound_search(inb_name, raw) != 0)
		goto out;

	__inbound_updown(up, raw, dev);
	ret = 0;
out:
	return ret;
}

static int inbound_start_main(int argc, char *argv[])
{
	return inbound_updown(1, argc, argv);
}

static int inbound_stop_main(int argc, char *argv[])
{
	return inbound_updown(0, argc, argv);
}

static int inbound_help_main(int argc, char *argv[])
{
	printf("example in shell command:\n");
	printf("	cli security inbound_filter start inbname1 eth1\n");
	printf("	* This means inbound filter 'inbname1' starting at device eth1 *\n\n");

	printf("	cli security inbound_filter stop inbname2 eth1\n");
	printf("	* This means inbound filter 'inbname1' stoping at device eth1\n\n");

	printf("example in C code:\n");
	printf("	setenv(\"INBOUND_PROTO\", \"tcp\", 1);\n");
	printf("	setenv(\"INBOUND_PORT\", \"80\", 1);\n");
	printf("	system(\"/bin/cli security inbound_filter start inbname1 eth1\");\n");
	printf("	* This means inbound filter 'inbname1' starting at device eth1\n");
	printf("	  indicate proto tcp and port 80\n\n");

	printf("	setenv(\"INBOUND_TABLE\", \"nat\", 1);\n");
	printf("	setenv(\"INBOUND_CHAIN\", \"PREROUTING\", 1);\n");
	printf("	setenv(\"INBOUND_PROTO\", \"tcp\", 1);\n");
	printf("	setenv(\"INBOUND_PORT\", \"80\", 1);\n");
	printf("	setenv(\"INBOUND_POLICY\", \"DNAT --to-destination 192.168.0.1:80\", 1);\n");
	printf("	system(\"/bin/cli security inbound_filter start inbname1 eth1\");\n");
	printf("	* This means inbound filter 'inbname1' starting at device eth1\n");
	printf("	  indicate proto tcp and port 80, only working in nat PREROUTING chain\n\n");
}

int inbound_filter_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", inbound_start_main, "ex: cli security inbound_filter start [name] [dev]" },
		{ "stop", inbound_stop_main, "ex: cli security inbound_filter stop [name] [dev]" },
		{ "help", inbound_help_main },
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
