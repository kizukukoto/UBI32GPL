#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include "cmds.h"
#include <nvramcmd.h>
#include "shutils.h"

#define DEBUG	cprintf
#if 0
static char devip[16];
#endif
inline int nvram_safe_get_cmp(const char *k, char *v, size_t len, const char *cmp)
{
	return strcmp(strncpy(v, nvram_safe_get(k), len), cmp);
}

int get_sched_cmd(int index, char *out)
{
	int i, sec1, sec2, ret = -1;
	char *str, tmp[256], *cmd, *d, *t1, *t2, *tp, timestr[48] = "shed1/1010101/60/120";
	char *w[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL};

	sprintf(tmp, "schedule_rule%d", index);
	str = nvram_safe_get(tmp);
	if(strlen(str) == 0)
		goto out;

	strcpy(timestr, str);
	tp = timestr;
	strsep(&tp, "/");
	d = strsep(&tp, "/");
	t1 = strsep(&tp, "/");
	t2 = strsep(&tp, "/");
	sec1 = atoi(t1);
	sec2 = atoi(t2);

	sprintf(tmp, "-m time --timestart %d:%d --timestop %d:%d",
			 sec1 / 60, sec1 % 60, sec2 / 60, sec2 % 60);

	if (strcmp(d, "0000000") != 0){
		strcat(tmp, " --days ");
		for (i = 0; i < 7; i++) {
			if (d[i] == '1'){
				strcat(tmp, w[i]);
				strcat(tmp, ",");
			}
		}
	}

	i = strlen(tmp);
	if (strcmp(strrchr(tmp, ','), ",") == 0)
		tmp[i-1] = '\0';
	strcpy(out, tmp);
	ret = 0;
out:
	return ret;
}

void __https_setup_config(FILE *stream, const char *group)
{
	char buf[2048], *user, *pass, *p;

	/* format of value of @group = "Groupname/user1,pass1,user2,pass2 " */
	if (nvram_safe_get_cmp(group, buf, sizeof(buf), "") == 0)
		return;

	p = buf;
	strsep(&p, "/");
	do {
		if ((user = strsep(&p, ",")) == NULL ||
			((pass = strsep(&p, "/")) == NULL))
			break;
		fprintf(stream, "/:%s:%s\n", user, pass);
		DEBUG("/:%s:%s\n", user, pass);
	} while (p != NULL);
}

static int update_htpasswd(const char *ip, const char *cfg)
{
	int rev = -1;
	FILE *fp;
	char passwd[32], *p, *g;
	char rou[128], rop[128], v[256];

	DEBUG("update_htpasswd ip: %s\n", ip);

	bzero(v, sizeof(v));
	if (!(fp = fopen(cfg, "w+"))) {
		perror(cfg);
		goto update_pwd_out;
	}

	p = nvram_safe_get("http_username");
	if (*p == '\0')
		p = "admin";

	sprintf(passwd, "A:%s/24\n", ip);
	fputs(passwd, fp);
	sprintf(passwd, "/:%s:%s\n", p, nvram_safe_get("admin_password"));
	fputs(passwd, fp);

	strcpy(rou, nvram_safe_get("ro_username"));
	strcpy(rop, nvram_safe_get("ro_password"));
	sprintf(passwd, "/:%s:%s\n", rou, rop);
	fputs(passwd, fp);

	if(nvram_safe_get_cmp("http_group_enable", v, sizeof(v), "1") == 0)
		if(nvram_safe_get_cmp("http_group", v, sizeof(v), "") != 0) {
			DEBUG("group: %s\n", v);
			p = v;
			while((g = strsep(&p, "/")) != NULL && *g != '\0')
				__https_setup_config(fp, g);
		}

	bzero(v, sizeof(v));
	if(nvram_safe_get_cmp("rdonly_group_enable", v, sizeof(v), "1") == 0)
		if(nvram_safe_get_cmp("rdonly_group", v, sizeof(v), "") != 0) {
			DEBUG("group: %s\n", v);
			p = v;
			while((g = strsep(&p, "/")) != NULL && *g != '\0')
				__https_setup_config(fp, g);
		}

	fclose(fp);
	rev = 0;

update_pwd_out:
	rd(rev, "Error");
	return rev;
}

#if 0
static void getip_by_dev(const char *dev, char *ip)
{
	int inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq ifr;

	strcpy(ifr.ifr_name, dev);
	if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0) {
		perror("ioctl");
		return;
	}

	strcpy(ip, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
}

static char *__getip_by_dev(const char *dev)
{
	char wanport[8];
	char devname[8];
	FILE *fp = fopen("/tmp/http_wanport", "r");

	if (fp == NULL)
		return NULL;

	bzero(wanport, sizeof(wanport));
	bzero(devname, sizeof(devname));
	bzero(devip, sizeof(devip));

	fscanf(fp, "%s\n", wanport);
	while (fscanf(fp, "%[^:]: %s", devname, devip) != EOF) {
		DEBUG("devname: %s, devip: %s\n", devname, devip);
		if (strcmp(dev, devname) == 0)
			break;
		bzero(devip, sizeof(devip));
	}

	fclose(fp);
	return devip;
}
#endif

static void __remote_management(
	const char *action,
	const char *wanport,
	const char *lanip,
	const char *lanport,
	const char *dev,
	const char *policy)
{
	char cmds[512];

#if 0
	if (strcmp(policy, "DROP") == 0) {
		sprintf(cmds, "iptables -t nat -%s PREROUTING -i %s "
			      "-p tcp --dport %s -j DROP",
			action,
			dev,
			wanport);
		system(cmds);
		DEBUG("__remote_management: %s\n", cmds);
		return;
	}
#endif
	sprintf(cmds, "iptables -t nat -%s PREROUTING -i %s "
		      "-p tcp --dport %s -j DNAT --to-destination %s:%s",
		action,
		dev,
		wanport,
		lanip,
		lanport);
	DEBUG("__remote_management: %s\n", cmds);
	system(cmds);

	sprintf(cmds, "iptables -%s INPUT -i %s -p tcp "
		      "-d %s --dport %s -j ACCEPT",
		action,
		dev,
		lanip,
		lanport);
	DEBUG("__remote_management: %s\n", cmds);
	system(cmds);
#if 0
	char cmd[512];
	char lanip[16];
	char sched_cmd[256], *str;
	int index = 0;

	bzero(cmd, sizeof(cmd));

	strcpy(lanip, __getip_by_dev("br0"));

	str = nvram_safe_get("remote_schedule");
	
	if( strcmp(str, "Always") == 0 || strlen(str) == 0){
		memset(sched_cmd, 0, sizeof(sched_cmd));
	} else {
		index = atoi(str);
		get_sched_cmd(index, sched_cmd);
	}

	DEBUG("remote management %s: ", (*action == 'D')?"stop":"start");
	sprintf(cmd, "iptables -t nat -%s PREROUTING -i ! br0 -p tcp --dport %s %s -j DNAT --to-destination %s:%s",
			action, wanport, sched_cmd, lanip, httpd_port);
	DEBUG("%s\n", cmd);
	system(cmd);

	bzero(cmd, sizeof(cmd));
	sprintf(cmd, "iptables -%s INPUT -i ! br0 -p tcp -d %s --dport %s -j ACCEPT", action, lanip, httpd_port);
	DEBUG("remote management %s\n", cmd);
	system(cmd);
#endif
}

static void remote_inbound_filter_updown(
	const char *dev,
	const char *lanip,
	const char *action)
{
	int i = 1;
	char inb_content[512], inb_policy[512], cmds[512];
	char *inb_name = nvram_safe_get("inbound_filter");

	if (!NVRAM_MATCH("wan_access_allow", "1"))
		goto out;
	if (NVRAM_MATCH("inbound_filter", "Allow ALL"))
		goto out;
#if 0
	if (strcmp(inb_name, "Allow ALL") == 0) {
		sprintf(cmds, "iptables -t nat -%s PREROUTING -i %s -j ACCEPT",
			action,
			dev);
		DEBUG("remote_inbound_filter_updown: %s\n", cmds);
		system(cmds);
		goto out;
	}
#endif
	setenv("INBOUND_TABLE", "nat", 1);
	setenv("INBOUND_CHAIN", "PREROUTING", 1);
	setenv("INBOUND_PROTO", "tcp", 1);
	setenv("INBOUND_PORT", nvram_get("http_wanport"), 1);

	sprintf(inb_policy, "DNAT --to-destination %s:80", lanip);
	setenv("INBOUND_POLICY", inb_policy, 1);

	sprintf(cmds, "/bin/cli security inbound_filter %s %s %s",
		(*action == 'I')? "start": "stop",
		inb_name,
		dev);
	printf("cmds: %s\n", cmds);
	system(cmds);
out:
	return;
}

static int remote_management_start(
	const char *lanport,
	const char *lanip)
{
	int rev = -1;
	char cmds[512], *wanport = nvram_safe_get("http_wanport");
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

#if 0
	if (strcmp(nvram_safe_get("remote_enable"), "1") != 0)
		goto out;
#endif
	for (p = t; p->dev; p++) {
		char *policy = "ACCEPT";

		if (*(p->flag) == '\0' || strcmp(p->flag, "0") == 0)
			//policy = "DROP";
			continue;

		__remote_management("I", wanport, lanip, lanport, p->dev, policy);
	}

	remote_inbound_filter_updown(t[0].dev, lanip, "I");
#if 0
	for(; key_idx < 5; key_idx++) {
		allow_ip = nvram_safe_get_i("remote_ipaddr", key_idx, g1);
		if (allow_ip == NULL || *allow_ip == '\0')
			continue;

		bzero(ipaddr, sizeof(ipaddr));
		sprintf(ipaddr, "-s %s", allow_ip);
		__remote_management("I", ipaddr, wanport, wanif, httpd_port);
	}
#endif
	rev = 0;
out:
	rd(rev, "Error");
	return rev;
}

static int remote_management_stop(
	const char *lanport,
	const char *lanip)
{
	int rev = -1;
	char *wanport = nvram_safe_get("http_wanport");
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
		char *policy = "ACCEPT";

		if (*(p->flag) == '\0' || strcmp(p->flag, "0") == 0)
			//policy = "DROP";
			continue;

		__remote_management("D", wanport, lanip, lanport, p->dev, policy);
	}

	remote_inbound_filter_updown(t[0].dev, lanip, "D");
#if 0
	for(; key_idx < 5; key_idx++) {
		allow_ip = nvram_safe_get_i("remote_ipaddr", key_idx, g1);
		if (allow_ip == NULL || *allow_ip == '\0')
			continue;

		bzero(ipaddr, sizeof(ipaddr));
		sprintf(ipaddr, "-s %s", allow_ip);
		__remote_management("D", ipaddr, wanport, wanif, httpd_port);
	}
#endif
	rev = 0;
out:
	rd(rev, "Error");
	return rev;
}

static int httpd_start(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *lanif, *lanip;	/* from argv */
	char *wanif, *cfg, *port;	/* from nvram */
	int alias_idx = -1;		/* from nvram */
	char cmd[256];

	if (argc != 4)
		return rev;

	alias = argv[1];
	lanif = argv[2];
	lanip = argv[3];

	DEBUG("================ httpd start ================\n");
	DEBUG("lanif: %s\n", lanif);
	DEBUG("lanip: %s\n", lanip);
	DEBUG("alias: %s\n", alias);
	DEBUG("alias_idx: %d\n", alias_idx = nvram_find_index("httpd_alias", alias));
	DEBUG("cfg: %s\n", nvram_safe_get_i("httpd_cfg", alias_idx, g1));
	DEBUG("port: %s\n", nvram_safe_get_i("httpd_port", alias_idx, g1));

	if ((alias_idx = nvram_find_index("httpd_alias", alias)) == -1)
		goto out;

	if ((cfg = nvram_safe_get_i("httpd_cfg", alias_idx, g1)) == NULL)
		goto out;

	if ((port = nvram_safe_get_i("httpd_port", alias_idx, g1)) == NULL)
		goto out;

	DEBUG("nvram keys preparation complete\n");
	update_htpasswd(lanip, cfg);
	remote_management_start(port, lanip);

	eval("iptables", "-I", "INPUT", "-i", lanif,
		"-p", "tcp", "--dport", port, "-j", "ACCEPT");

	sprintf(cmd, "httpd -h /www -c %s -u 0 -r `nvram get model_name`", cfg);
	system(cmd);
	rev = 0;
out:
	rd(rev, "Error");
	return rev;
}

static int httpd_stop(int argc, char *argv[])
{
	int rev = -1;
	char *alias, *lanif, *lanip;	/* from argv */
	char *wanif, *cfg, *port;	/* from nvram */
	int alias_idx;			/* from nvram */
	char cmd[256];

	DEBUG("================ httpd stop ================\n");
	
	if (argc != 4)
		return rev;

	alias = argv[1];
	lanif = argv[2];
	lanip = argv[3];

	if ((alias_idx = nvram_find_index("httpd_alias", alias)) == -1)
		goto out;

	if ((wanif = nvram_safe_get_i("httpd_wanif", alias_idx, g1)) == NULL)
		goto out;

	if ((cfg = nvram_safe_get_i("httpd_cfg", alias_idx, g1)) == NULL)
		goto out;

	if ((port = nvram_safe_get_i("httpd_port", alias_idx, g1)) == NULL)
		goto out;

	remote_management_stop(port, lanip);

	eval("iptables", "-D", "INPUT", "-i", lanif, "-p", "tcp", "--dport", port, "-j", "ACCEPT");

	sprintf(cmd, "kill -9 `ps aux|grep \"httpd -h /www -c %s\"|awk '{print $1}'`", cfg);
	system(cmd);
	sprintf(cmd, "rm -rf %s", cfg);
	system(cmd);
	rev = 0;
out:
	rd(rev, "Error");
	return rev;
}

/*
 * http_main
 * start:
 * 	<device> <ip> <mask>
 * */
int httpd_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", httpd_start},
		{ "stop", httpd_stop},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
