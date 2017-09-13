#include "ehttpd.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

/* fred_hung 2008/12/23
 * This function can call external executable file, and
 * get return value of external main function
 * */
int call_cmd(const char *cmd)
{
	pid_t pid;
        int status, pret, ret = -1;

	signal(SIGCHLD, SIG_DFL);
	pid = fork();

	if (pid == -1)
		goto out;

	if (pid == 0) {
		execv(cmd, (char *[]){cmd, NULL});
		exit(-1);
	}

	if ((pret = waitpid(pid, &status, 0)) == -1)    /* NO PID RETURN */
		goto out;
	else if (pret == 0)                             /* NO CHILD RETURN */
		goto out;
	else if (WIFEXITED(status))                     /* EXIT NORMALLY */
		ret =  WEXITSTATUS(status);
out:
	signal(SIGCHLD, SIG_IGN);
	return ret;
}

int allowed_page(const char *url, const char *fromfunc)
{
	struct {
		const char *url;
		int status;     // 0: deny, 1: allow
        } *p, allow[] = {
		{ "version.txt", 1 },
#if ENABLE_FEATURE_HTTPD_GRAPH_AUTH
		{ INDEX_PAGE, 1 },
		{ "login_pic.asp", 1},
		{ "auth.bmp", 1},
#endif
		{ "index.html", 1},
		{ "apply.cgi", 1},
		{ "chklst.txt", 1 },
		{ "wlan.txt", 1 },
		{ "css_router.css", 1 },
		{ "public.js", 1 },
		{ "favicon.ico", 1},
		{ "wlan_masthead.gif", 1},
		{ "wireless_bottom.gif", 1},
		{ "wired.gif", 1},
		{ "deny.html", 1},
		{ "wireless_tail.gif", 1},
		{ "lang_en.js", 1},
		{ "wl.txt", 1},
		{ NULL, -1 }
	};

	DEBUG_MSG("url(%s), from(%s)\n", url, fromfunc);

	for (p = allow; p->url != NULL; p++)
		if (strstr(url, p->url))
			return p->status;

	return 0;
}

static int auth_session_renew(const char *s_record)
{
	int ret = -1;
	char cmds[128];

	if (access(s_record, F_OK) != 0)
		goto out;

	sprintf(cmds, "touch %s", s_record);
	system(cmds);

	ret = 0;
out:
	return ret;
}

static int auth_get_timeout_second(void)
{
	FILE *fp = popen("/usr/sbin/nvram get session_timeout", "r");
	char session_timeout[16];

	if (fp == NULL)
		return SESSION_TIMEOUT;

	bzero(session_timeout, sizeof(session_timeout));
	fscanf(fp, "%s", session_timeout);
	pclose(fp);

	if (*session_timeout == '\0')
		return SESSION_TIMEOUT;

	return atoi(session_timeout);
}

int auth_session_timeout(const char *rmt_ip_str)
{
	int ret = -1;
	struct stat buf;
	char s_record[128], rmt_ip[16], rmt_port[6];
	time_t cur_time = time(NULL);

	bzero(rmt_ip, sizeof(rmt_ip));
	bzero(rmt_port, sizeof(rmt_port));

	sscanf(rmt_ip_str, "%[^:]:%s", rmt_ip, rmt_port);
	sprintf(s_record, "/tmp/graph/%s_allow", rmt_ip);
	stat(s_record, &buf);

	if (cur_time - buf.st_atime <= auth_get_timeout_second()) {
		auth_session_renew(s_record);
		ret = 0;
	} else
		unlink(s_record);

	return ret;
}

#if ENABLE_FEATURE_HTTPD_MAINTAIN_USERINFO
static void auth_envfn(const char *item, const char *contents)
{
	DEBUG_MSG("XXX auth_envfn [%s] [%s]\n", item, contents);
	setenv(item, contents, 1);
}

static void auth_nvrfn(const char *item, const char *contents)
{
	char cmds[128];

	DEBUG_MSG("XXX auth_nvrfn [%s] [%s]\n", item, contents);
	sprintf(cmds, "/usr/sbin/nvram set %s=\"%s\"", item, contents);
	system(cmds);
}

int auth_setenv_from_file(const char *rmt_ip_str)
{
	int ret = -1;
	char rmt_ip[16], rmt_port[8];
	char f_session[128], type[16], item[128], contents[128];
	FILE *fp;
	struct {
		const char *type;
		void (*fn)(const char *, const char *);
	} *p, s[] = {
		{ "var", NULL },
		{ "env", auth_envfn },
		{ "nvr", auth_nvrfn },
		{ NULL , NULL }
	};

	if (rmt_ip_str == NULL || *rmt_ip_str == '\0')
		goto out;

	sscanf(rmt_ip_str, "%[^:]:%s", rmt_ip, rmt_port);
	sprintf(f_session, "/tmp/graph/%s_allow", rmt_ip);
	DEBUG_MSG("XXX f_session [%s]\n", f_session);

	if ((fp = fopen(f_session, "r")) == NULL)
		goto out;

	bzero(type, sizeof(type));
	bzero(item, sizeof(item));
	bzero(contents, sizeof(contents));

	while (fscanf(fp, "%s %s %s", type, item, contents) != EOF) {
		if (*type == '\0' || *item == '\0' || *contents == '\0')
			continue;

		for (p = s; p->type; p++) {
			if (strcmp(type, p->type) == 0) {
				if (p->fn)
					p->fn(item, contents);
				break;
			}
		}

		bzero(type, sizeof(type));
		bzero(item, sizeof(item));
		bzero(contents, sizeof(contents));
	}

	fclose(fp);
	ret = 0;
out:
	return ret;
}
#endif // ENABLE_FEATURE_HTTPD_MAINTAIN_USERINFO

int auth_session_exist(const char *rmt_ip_str)
{
	char fname[128];
	char rmt_ip[16], rmt_port[6];

	bzero(rmt_ip, sizeof(rmt_ip));
	bzero(rmt_port, sizeof(rmt_port));

	sscanf(rmt_ip_str, "%[^:]:%s", rmt_ip, rmt_port);
	sprintf(fname, "/tmp/graph/%s_allow", rmt_ip);

	return access(fname, F_OK);
}

/* replace string tok with dst from str, and str length is slen */
int str_replace(char *str, const char *dst, const char *tok, size_t slen)
{
	char *pt, *buf;

	if (strlen(dst) > slen)
		return -1;

	if ((pt = strstr(str, tok)) == NULL)
		return 0;

	pt[0] = '\0';
	pt += strlen(tok);

	if ((buf = malloc(slen)) == NULL)
		return -1;

	sprintf(buf, "%s%s%s", str, dst, pt);
	strcpy(str, buf);
	free(buf);

	return 0;
}
