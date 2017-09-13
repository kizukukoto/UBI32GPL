#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvramcmd.h>
#include <sys/types.h>
#include <signal.h>
#include "cmds.h"

#define HTTPS_CONF	"/tmp/https.conf"
#if 0
#define DEBUG	cprintf

extern void __https_setup_config(FILE *stream, const char *group);
static void remote_write_to_record(FILE *fd, const char *g)
{
	char p[80];
	char *g_users, *userinfo;

	g_users = nvram_safe_get(g);
	strcpy(p, g_users);

	g_users = p;
	strsep(&g_users, "/");
	while((userinfo = strsep(&g_users, "/")) != NULL && *userinfo != '\0') {
		char *username = strsep(&userinfo, ",");

		fprintf(fd, "%s\n", username);
	}
}

static void https_setup_config(FILE *stream, const char *group)
{
	int rev = -1;
	char *p, *g, *m;

	if (strchr(group, ':') == NULL){
		__https_setup_config(stream, group);
		goto out;
	}
	/*
	 * @group support multi-group
	 * ex: "auth_group1:auth_group3:auth_group4"
	 */

	if ((g = strdup(group)) == NULL)
		goto out;

	m = g;
	while (g != NULL && *g != '\0' &&
		(p = strsep(&g, ":")) != NULL && *p != '\0') {
		__https_setup_config(stream, p);
	}
	free(m);
	rev = 0;
out:
	rd(rev, "Error");
}

static int has_remote_ssl()
{
	char ssl[20];

	bzero(ssl, sizeof(ssl));
	if(nvram_safe_get_cmp("remote_ssl", ssl, sizeof(ssl), "1") == 0)
		return 1;

	return 0;
}

static int https_enabled_and_setup(const char *config)
{
	FILE *fd, *fd_remote, *fd_ssl;
	int i, ret = -1;
	char k[64], v[256], *p, *g;
	char *admin_acc;

        /* 
	 * sslvpn: <enable>,<name>,<group>,<pki ca>,<subnet>,<masq=0|1>
	 * ex: "1,SSLname,auth_group1,192.168.0.0/24"
	 * In this case, we focus on the <group> field.
	 */
	if ((fd = fopen(config, "w")) == NULL)
		goto out;
	if ((fd_remote = fopen("/tmp/remote.users", "w")) == NULL)
		goto out;

	if ((fd_ssl = fopen("/tmp/ssl.users", "w")) == NULL)
		goto out;

	for (i = 1; i <= 6; i++) {
		sprintf(k, "sslvpn%d", i);
		if (nvram_safe_get_cmp(k, v, sizeof(v), "") == 0)
			continue;

		if (*v == '0')
			continue;
		p = v;
		strsep(&p, ",");
		strsep(&p, ",");
		g = strsep(&p, ",");
		https_setup_config(fd, g);
		https_setup_config(fd_ssl, g);
		ret = 0; //success
	}

	admin_acc = nvram_safe_get("http_username");
	if (*admin_acc == '\0')
		admin_acc = "admin";

	fprintf(fd_remote, "%s\n", admin_acc);

	if(has_remote_ssl() == 1) {
		fprintf(fd, "/:%s:%s\n", admin_acc, nvram_safe_get("admin_password"));
		ret = 0;
	}

	if(nvram_safe_get_cmp("http_group_enable", v, sizeof(v), "1") == 0) {
		if(nvram_safe_get_cmp("http_group", v, sizeof(v), "") != 0) {

			p = v;
			while((g = strsep(&p, "/")) != NULL && *g != '\0') {
				if(has_remote_ssl() == 1) {
					https_setup_config(fd, g);
					ret = 0;
				}
				remote_write_to_record(fd_remote, g);
			}
		}
	}

out:
	fcloseall();
	rd(ret, "Error");
	return ret;
}

static int https_need_start()
{
	char remote_ssl_content[4];
	char sk[16], sk_content[128];
	int ssl_enabled, ssl_idx = 1;

	if (nvram_safe_get_cmp("remote_ssl",
		remote_ssl_content,
		sizeof(remote_ssl_content),
		"1") == 0)
		return 1;

	for(; ssl_idx <= 6; ssl_idx++) {
		sprintf(sk, "sslvpn%d", ssl_idx);
		if (nvram_safe_get_cmp(sk, sk_content, sizeof(sk_content), "") == 0)
			continue;

		sscanf(sk_content, "%d", &ssl_enabled);
		if (ssl_enabled == 1)
			return 1;
	}

	return 0;
}

static void https_inbound_updown(const char *dev, const char *action)
{
	char cmds[512], policy[128], *inb = nvram_safe_get("inbound_filter");

	if (*inb == '\0')
		return;
	if (strcmp(inb, "Allow ALL") == 0)
		return;

	setenv("INBOUND_TABLE", "nat", 1);
	setenv("INBOUND_CHAIN", "PREROUTING", 1);
	setenv("INBOUND_PROTO", "tcp", 1);
	setenv("INBOUND_PORT", "443", 1);

	sprintf(policy, "REDIRECT --to-ports 443");
	setenv("INBOUND_POLICY", policy, 1);

	sprintf(cmds, "/bin/cli security inbound_filter %s %s %s",
		(*action == 'I')? "start": "stop",
		inb,
		dev);
	DEBUG("https_inbound: %s\n", cmds);
	system(cmds);
}

static void https_firewall_setting(const char *action)
{
	char cmds[512];
	struct {
		char *dev;
		char *flag;
	} *p, t[] = {
		{ "eth1", nvram_safe_get("wan_access_allow") },
		{ "eth2", nvram_safe_get("dmz_access_allow") },
		{ "br0", nvram_safe_get("lan_access_allow") },
		{ "ra0", nvram_safe_get("wlan_access_allow") },
		{ NULL, NULL}
	};

	for (p = t; p->dev; p++) {
		if (*(p->flag) == '\0' || strcmp(p->flag, "0") == 0) {
			sprintf(cmds, "iptables -t nat -%s PREROUTING -i %s "
				      "-p tcp --dport 443 -j DROP",
				action,
				p->dev);
			DEBUG("https_firewall_setting: %s\n", cmds);
			system(cmds);
			continue;
		}

		sprintf(cmds, "iptables -t nat -%s PREROUTING -i %s "
			      "-p tcp --dport 443 -j REDIRECT --to-ports 443",
			action,
			p->dev);
		DEBUG("https_firewall_setting: %s\n", cmds);
		system(cmds);

		sprintf(cmds, "iptables -%s INPUT -i %s "
			      "-p tcp --dport 443 -j ACCEPT",
			action,
			p->dev);
		DEBUG("https_firewall_setting: %s\n", cmds);
		system(cmds);
	}

	if (NVRAM_MATCH("wan_access_allow", "1"))
		https_inbound_updown(t[0].dev, action);
}

static int https_start(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *wanif, *wanip;	/* from argv */
	char *lanif, *cfg, *port;	/* from nvram */
	int alias_idx = -1;		/* from nvram */
	char cmd[256];

	DEBUG("================ https start ================\n");

	if (https_need_start() == 0)
		return 0;

	if (argc != 4)
		goto out;

	alias = argv[1];
	wanif = argv[2];
	wanip = argv[3];

	if ((alias_idx = nvram_find_index("https_alias", alias)) == -1)
		goto out;

	if ((lanif = nvram_safe_get_i("https_lanif", alias_idx, g1)) == NULL)
		goto out;

	if ((cfg = nvram_safe_get_i("https_cfg", alias_idx, g1)) == NULL)
		goto out;

	if ((port = nvram_safe_get_i("https_port", alias_idx, g1)) == NULL)
		goto out;

	https_enabled_and_setup(cfg);

	sprintf(cmd, "httpd -h /www -p %s -S -k /tmp/cert -u 0 -r \"Welcome to the dlinkrouter DIR-730\" -c %s",
			port, cfg);
	/* FIXME: redesign */
	system("cat /etc/DIR-730_key.pem > /tmp/cert");
	system("cat /etc/DIR-730_cert.pem >> /tmp/cert");
	system(cmd);

	https_firewall_setting("I");
	rev = 0;

out:
	rd(rev, "Error");
	return rev;
}

static int https_stop(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *wanif, *wanip;	/* from argv */
	char *lanif, *cfg, *port;	/* from nvram */
	int alias_idx = -1;		/* from nvram */
	char cmd[256];

	DEBUG("================ https stop ================\n");

	if (https_need_start() == 0)
		return 0;

	if (argc != 4)
		goto out;

	alias = argv[1];
	wanif = argv[2];
	wanip = argv[3];

	if ((alias_idx = nvram_find_index("https_alias", alias)) == -1)
		goto out;

	if ((lanif = nvram_safe_get_i("https_lanif", alias_idx, g1)) == NULL)
		goto out;

	if ((cfg = nvram_safe_get_i("https_cfg", alias_idx, g1)) == NULL)
		goto out;

	if ((port = nvram_safe_get_i("https_port", alias_idx, g1)) == NULL)
		goto out;

	sprintf(cmd, "kill -9 `ps aux|grep \"httpd -h /www -p %s\"|awk '{print $1}'`", port);
	system(cmd);
	sprintf(cmd, "rm -rf %s", cfg);
	system(cmd);

	https_firewall_setting("D");
	rev = 0;
out:
	rd(rev, "Error");
	return rev;
}
#endif	/* DIR730 style */

#define HTTPS_START	0x00
#define HTTPS_STOP	0x01

static pid_t __getpid(const char *record)
{
	FILE *fp;
	int ret = -1;
	pid_t pid;

	if (record == NULL || *record == '\0')
		goto out;
	if (access(record, F_OK) != 0)
		goto out;
	if ((fp = fopen(record, "r")) == NULL)
		goto out;
	fscanf(fp, "%d", &pid);
	fclose(fp);

	ret = pid;
out:
	return ret;
}

extern int https_pem(const char *ca);   /* in lib/libca.c */
extern int base64_encode(char *, char *, int);	/* in lib/common_tools.c */

static int https_account(char const *conf)
{
	FILE *fp = fopen(conf, "w+");
	char *admin_usr = nvram_safe_get("admin_username");
	char *admin_pwd = nvram_safe_get("admin_password");
	char *user_usr = nvram_safe_get("user_username");
	char *user_pwd = nvram_safe_get("user_password");
	char admin_usr_b64[128], admin_pwd_b64[128];
	char user_usr_b64[128], user_pwd_b64[128];

	if (fp == NULL)
		goto out;

	base64_encode(admin_usr, admin_usr_b64, strlen(admin_usr));
	base64_encode(admin_pwd, admin_pwd_b64, strlen(admin_pwd));
	base64_encode(user_usr, user_usr_b64, strlen(user_usr));
	base64_encode(user_pwd, user_pwd_b64, strlen(user_pwd));

	fprintf(fp, "/:%s:%s\n", admin_usr_b64, admin_pwd_b64);
	fprintf(fp, "/:%s:%s\n", user_usr_b64, user_pwd_b64);
	fclose(fp);

	printf("%s has been created\n", conf);
out:
	return 0;
}

static int https_start(int argc, char *argv[])
{
	char cmds[128];
	char *sslport = nvram_safe_get("sslport");

	if (NVRAM_MATCH("remote_ssl", "0"))
		goto out;
	if (access("/var/run/https.pid", F_OK) == 0)
		goto out;
	if (*sslport == '\0')
		goto out;
	if (https_pem("/tmp/ssl.pem") != 0)
		goto out;

	unlink(HTTPS_CONF);
	https_account(HTTPS_CONF);
	sprintf(cmds, "/bin/httpd -h /www -p %s -k /tmp/ssl.pem", sslport);
	system(cmds);
out:
	return 0;
}

static int https_stop(int argc, char *argv[])
{
	pid_t pid;

	if (access("/var/run/https.pid", F_OK) != 0)
		goto out;
	if ((pid = __getpid("/var/run/https.pid")) == -1)
		goto out;

	kill(pid, SIGTERM);
	unlink("/var/run/https.pid");
	unlink(HTTPS_CONF);
out:
	return 0;
}

int https_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", https_start},
		{ "stop", https_stop},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
