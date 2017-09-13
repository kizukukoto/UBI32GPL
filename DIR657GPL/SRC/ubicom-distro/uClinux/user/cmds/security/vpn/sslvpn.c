#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <nvramcmd.h>
#include <sys/types.h>
#include <signal.h>
#include "cmds.h"
#define SSL_MAX	6
#define SSL_ACCOUNT	"/tmp/ssl.users"
#define HTTPS_ACCOUNT	"/tmp/https.conf"
#define HTTPS_ACCOUNT_T	"/tmp/https.conf.tmp"
#define HTTPS_PIDFILE	"/var/run/https.pid"
#define HTTPS_PEM	"/tmp/ssl.pem"
#define SSLVPN_START	0x00
#define SSLVPN_STOP	0x01

/* format:
 * [ENABLE|DISABLE],[RULENAME],[USERGROUP],[LAN NETWORK],[I DON'T WHAT THIS..]
 *
 * example:
 * @ct := "1,sslvpn,auth_group2:,192.168.2.0/24,1"
 *
 * */

/* in sys/account.c */
extern int base64_encode(char *, char *, int);
typedef int (*do_action)(void *, const char *, const char *);

static int sslvpn_add_config(void *parm, const char *ub64, const char *pb64)
{
	int ret = -1;

	if (parm == NULL)
		goto out;

	fprintf((FILE *)parm, "/:%s:%s\n", ub64, pb64);
	ret = 0;
out:
	return ret;
}

#if 0
/* example:
 * @sln := "/:YWRtaW4=:YWRtaW4"
 * */
static int __sslvpn_del_config(
	const char *sln,
	const char *ub64,
	const char *pb64)
{
	int ret = -1;
	char *tu, *tp, *tptr, tmp[128];

	tptr = tmp;
	strcpy(tmp, sln);

	strsep(&tptr, ":");	/* ignore first '/' */

	if ((tu = strsep(&tptr, ":")) == NULL)
		goto out;
	if ((tp = tptr) == NULL)
		goto out;
	if (strcmp(ub64, tu) != 0 || strcmp(pb64, tp) != 0)
		goto out;
	ret = 0;
out:
	if (tu == NULL)
		cprintf("XXX tu null\n");
	if (tp == NULL)
		cprintf("XXX tp null\n");
	return ret;
}

static int sslvpn_del_config(const char *ub64, const char *pb64)
{
	int ret = -1;
	FILE *sfp = NULL, *dfp = NULL;
	char cmds[128], line[128], tmpfile[24] = "/tmp/httpsXXXXXX";

	if (mkstemp(tmpfile) == -1)
		goto out;
	if ((sfp = fopen(HTTPS_ACCOUNT, "r")) == NULL)
		goto out;
	if ((dfp = fopen(tmpfile, "a+")) == NULL)
		goto out;
	while (fscanf(sfp, "%s", line) != EOF) {
		if (__sslvpn_del_config(line, ub64, pb64) == 0)
			continue;
		fprintf(dfp, "%s\n", line);
	}

	fclose(sfp);
	fclose(dfp);
	unlink(HTTPS_ACCOUNT);
	sprintf(cmds, "mv %s %s", tmpfile, HTTPS_ACCOUNT);
	system(cmds);

	ret = 0;
out:
	if (sfp) fclose(sfp);
	if (dfp) fclose(dfp);

	return ret;
}
#endif

static int __sslvpn_group_parsing(const char *__group, do_action fn, void *parm)
{
	int ret = -1;
	char *g = nvram_get(__group);
	char *gue, *gp, gt[1024];

	if (g == NULL || *g == '\0')
		goto out;
	/* example:
	 * @g := "Group2/ssluser,sslpassword/ssluser2,sslpassword2/"
	 * */
	gp = gt;
	strcpy(gt, g);

	strsep(&gp, "/");	/* ignore group name */
	while ((gue = strsep(&gp, "/"))) {
		char *uname, *passwd, *gutp, gut[128];
		char unameb64[128], passwdb64[128];

		gutp = gut;
		strcpy(gut, gue);

		if ((uname = strsep(&gutp, ",")) == NULL)
			goto out;
		if ((passwd = gutp) == NULL)
			goto out;

		base64_encode(uname, unameb64, strlen(uname));
		base64_encode(passwd, passwdb64, strlen(passwd));

		fn? fn(parm, unameb64, passwdb64): 0;
	}

	ret = 0;
out:
	return ret;
}

static int sslvpn_group_parsing(const char *group, do_action fn, void *parm)
{
	int ret = -1;
	char *gname, *gp, g[512];

	if (group == NULL || *group == '\0')
		goto out;

	gp = g;
	strcpy(g, group);

	while ((gname = strsep(&gp, ":")) && *gname != '\0')
		__sslvpn_group_parsing(gname, fn, parm);

	ret = 0;
out:
	return ret;
}

static int sslvpn_entry_parsing(char *ct, do_action fn, void *parm)
{
	int ret = -1;
	char *sp, sct[128];
	char *usergroup, *network;

	if (ct == NULL || *ct == '\0' || *ct != '1')
		goto out;

	sp = sct;
	strcpy(sct, ct);
	strsep(&sp, ",");	/* ignore enable/disable */
	strsep(&sp, ",");	/* ignore rule name */
	if ((usergroup = strsep(&sp, ",")) == NULL)
		goto out;
	if ((network = strsep(&sp, ",")) == NULL)
		goto out;
	if (sslvpn_group_parsing(usergroup, fn, parm) == -1)
		goto out;

	ret = 0;
out:
	return ret;
}

static int is_sslvpn_enable()
{
	int  i = 1;
	char *p, nvk[1024];

	for (; i <= 6; i++) {
		sprintf(nvk, "sslvpn%d", i);
		p = nvram_safe_get(nvk);

		if (*p == '1')
			goto enable;
	}

	return 0;
enable:
	return 1;
}

extern int https_pem(const char *ca);	/* in cmds/lib/libca.c */
static int sslvpn_start_main(int argc, char *argv[])
{
	int i = 0;
	char *sslport, key[128], cmds[128];
	FILE *sslfp = fopen(SSL_ACCOUNT, "w+");
	FILE *httpsfp = fopen(HTTPS_ACCOUNT, "a+");

	/* mknod */
	if (access("/dev/net", F_OK) != 0)
		system("mkdir /dev/net");
	if (access("/dev/net/tun", F_OK) != 0)
		system("mknod /dev/net/tun c 10 200");
	if (!is_sslvpn_enable())
		goto out;
	if (access(HTTPS_ACCOUNT, F_OK) == 0)
		system("cp "HTTPS_ACCOUNT" "HTTPS_ACCOUNT_T);
	/* add account into /tmp/ssl.users and /tmp/https.conf */
	for (; i < SSL_MAX; i++) {
		sprintf(key, "sslvpn%d", i + 1);
		sslvpn_entry_parsing(nvram_safe_get(key), sslvpn_add_config, sslfp);
		sslvpn_entry_parsing(nvram_safe_get(key), sslvpn_add_config, httpsfp);
	}

	/* start https if https.pid not exists */
	if (access(HTTPS_PIDFILE, F_OK) == 0)
		goto out;
	if (https_pem(HTTPS_PEM) != 0)
		goto out;
	if (strcmp((sslport = nvram_safe_get("sslport")), "") == 0)
		sslport = "443";
	sprintf(cmds, "/bin/httpd -h /www -p %s -k %s", sslport, HTTPS_PEM);
	system(cmds);
out:
	if (sslfp) fclose(sslfp);
	if (httpsfp) fclose(httpsfp);

	return 0;
}

static int sslvpn_stop_main(int argc, char *argv[])
{
	FILE *pid = NULL;
	int https_pid;
	struct stat buf;

	/* stop https */
	if (access(HTTPS_PIDFILE, F_OK) != 0)
		goto out;
	if ((pid = fopen(HTTPS_PIDFILE, "r")) == NULL)
		goto out;
	if (NVRAM_MATCH("remote_ssl", "0")) {
		fscanf(pid, "%d", &https_pid);
		fclose(pid);
		kill(https_pid, SIGTERM);
		unlink(HTTPS_PIDFILE);
	}

	stat(HTTPS_ACCOUNT_T, &buf);

	/* del ssl.users, mv https.conf.tmp -> https.conf, del pid */
	if (access(HTTPS_ACCOUNT_T, F_OK) == 0) {
		if (buf.st_size > 0)
			system("mv "HTTPS_ACCOUNT_T" "HTTPS_ACCOUNT);
		else
			unlink(HTTPS_ACCOUNT_T);
	}
	unlink(SSL_ACCOUNT);
out:
	return 0;
}

int sslvpn_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", sslvpn_start_main, "start sslvpn"},
		{ "stop", sslvpn_stop_main, "stop sslvpn"},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
