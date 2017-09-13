#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "querys.h"
#include "libdb.h"
#include "ssi.h"
#include "public.h"

#if 1
#define DEBUG_MSG	cprintf
#else
#define DEBUG_MSG
#endif

#define GRAPH_AUTH_TEMPDIR      "/tmp/graph"

int call_cmd(char *cmd)
{
	int status, pret, ret = -1;
	pid_t pid;
#if NOMMU
	pid = vfork();
#else
	pid = fork();
#endif
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
	return ret;
}

static void auth_graph_toupper(const char *gcode, char *buf)
{
	int i = 0;

	if (gcode == NULL || *gcode == '\0')
		return;

	for (; i < strlen(gcode); i++) {
		if (islower(gcode[i]))
			buf[i] = toupper(gcode[i]);
		else
			buf[i] = gcode[i];
	}
	buf[i] = '\0';
}


static int user_exist_type(
	const char *cfg,
	const char *user,
	const char *pass)
{
	FILE *fp;
	int ret = 0;
	char t[128], dir[64], uid[64], pwd[64];

	if ((fp = fopen(cfg, "r")) == NULL) {
		if (strcmp(cfg, "/tmp/password") == 0) {
			DEBUG_MSG("XXX Cannot found /tmp/password, login always pass\n");
			goto out;
		}
		goto fail;
	}

	bzero(t, sizeof(t));
	DEBUG_MSG("XXX user_exist_type %s user [%s] pass [%s]\n", cfg, user, pass);
	while (fscanf(fp, "%s", t) != EOF) {
		/* ignore the first line 'A:192.168.0.1/24' */
		if (*t == 'A') continue;

		bzero(dir, sizeof(dir));
		bzero(uid, sizeof(uid));
		bzero(pwd, sizeof(pwd));

		sscanf(t, "%[^:]:%[^:]:%s", dir, uid, pwd);
		DEBUG_MSG("XXX uid [%s] <==> user [%s], pwd [%s] <==> pass [%s]\n",
			uid, user, pwd, pass);
		if (strcmp(user, uid) == 0 && strcmp(pass, pwd) == 0)
			goto out;
		bzero(t, sizeof(t));
	}
fail:
	ret = -1;
out:
	if (fp) fclose(fp);
	return ret;
}


static int http_user_exist(const char *user, const char *pass)
{
	return user_exist_type("/tmp/password", user, pass);
}

static int sslvpn_user_exist(const char *user, const char *pass)
{
	int in_https = user_exist_type("/tmp/https.conf", user, pass);
	int in_ssl = user_exist_type("/tmp/ssl.users", user, pass);

	return (in_https == 0 && in_ssl == 0)?0:-1;
}

static int https_user_exist(const char *user, const char *pass)
{
	return user_exist_type("/tmp/https.conf", user, pass);
}

static void ipaddr_formatting(const char *addr, char *buf)
{
	char *lp;

	if (strstr(addr, ":") == NULL)  /* it's means @addr is ipv4 */
		return;

	lp = strrchr(addr, ':');
	strcpy(buf, lp + 1);
	buf[strlen(buf) - 1] = '\0';    /* replace ']' to '\0' */
}

extern int iprange_ctrl(const char *multi_addr_fmt, const char *userip);
static int access_ctrl(const char *remote_ipaddr_key)
{
	int ret = -1;
	char *remote_ipaddr = nvram_safe_get(remote_ipaddr_key);
	char user_ipaddr_v4[128], *user_ipaddr = getenv("REMOTE_ADDR");

	DEBUG_MSG("XXX access_ctrl.remote_ipaddr_key [%s]\n", remote_ipaddr_key);
	DEBUG_MSG("XXX access_ctrl.remote_ipaddr [%s]\n", remote_ipaddr);
	DEBUG_MSG("XXX access_ctrl.user_ipaddr [%s]\n", user_ipaddr);

	if (user_ipaddr == NULL || *user_ipaddr == '\0')
		goto out;
	strcpy(user_ipaddr_v4, user_ipaddr);
	ipaddr_formatting(user_ipaddr, user_ipaddr_v4);
	user_ipaddr = user_ipaddr_v4;

	DEBUG_MSG("XXX access_ctrl.user_ipaddr_v4 [%s]\n", user_ipaddr_v4);

	if (*remote_ipaddr == '\0')
		goto out;
	DEBUG_MSG("XXX access_ctrl.flags1\n");
	if (*remote_ipaddr == '*' || strcmp(remote_ipaddr, "0.0.0.0") == 0)
		goto accept;
	DEBUG_MSG("XXX access_ctrl.flags2\n");
	if (iprange_ctrl(remote_ipaddr, user_ipaddr) == -1)
		goto out;
accept:
	DEBUG_MSG("XXX access_ctrl.accept flags\n");
	ret = 0;
out:
	DEBUG_MSG("XXX access_ctrl.ret [%d]\n", ret);
	return ret;
}

static void *index_page_return(
	const char *user,
	const char *pass,
	const char *sslport)
{
	char *sport = nvram_safe_get("sslport");
	char *status = nvram_safe_get("remote_ssl");
	char *response_page = "redirect2login.html";

	sport = (*sport == '\0')? "443": sport;
	status = (*status == '\0')? "0": status;

	DEBUG_MSG("XXX index_page_return user [%s] pass [%s]\n", user, pass);

	/* If login user in sslvpn group and login by HTTPS, redirect to
	 * auth.asp. (login_pic.asp -> redirect2auth.html -> auth.asp)
	 * Otherwise, if login user in sslvpn group but login by HTTP,
	 * then goto out flag and return redirect2login.html */
	if (sslvpn_user_exist(user, pass) == 0)
		if (strcmp(sport, sslport) == 0 &&
			access_ctrl("sslvpn_remote_ipaddr") == 0)
			response_page = "redirect2auth.html";
		else
			goto out;
	/* Login user not in sslvpn group will run here.
	 * If user login by HTTPS but remote ssl disabled, redirect to
	 * redirect2login.html. Otherwise redirect to index.asp */
	else if (https_user_exist(user, pass) == 0 ||
		http_user_exist(user, pass) == 0)
	{
		if (strcmp(sport, sslport) == 0 &&
			(strcmp(status, "0") == 0 ||
			access_ctrl("ssl_remote_ipaddr") == -1))
			response_page = "redirect2login.html";
		else
			response_page = "index.asp";
	}
out:
	/* if redirected page is not redirect2login.html, this means
	 * user login successful. If login successful, the allowed
	 * record file should be exists.
	 *
	 * The record file looks like /tmp/graph/192.168.0.100_allow
	 *
	 * We writing some user information into session record file.
	 * There are three fields in each line: info type, info item,
	 * and info contents.
	 *
	 * Info type
	 *
	 * 	var: General information, httpd don't care this. This
	 * type info will used for other side, maybe cgi.
	 * 	env: Environment variable. Httpd will read this item
	 * and save item's contents into environmant.
	 * 	nvr: nvram key. Httpd will read this item and save
	 * item's contents into nvram.
	 *
	 * Httpd read each page, record file will also read concurrently.
	 * When httpd read record file, httpd will save these user info
	 * into environment variable or nvram key according to the first
	 * field is env or nvr in each line.
	 *
	 * fredhung 2009/6/4
	 * */
	if (strcmp(response_page, "redirect2login.html") != 0) {
		FILE *fp;
		char fname[128];

		sprintf(fname, "%s/%s_allow", GRAPH_AUTH_TEMPDIR,
					      getenv("REMOTE_ADDR"));
		if ((fp = fopen(fname, "w")) == NULL)
			goto fail;

		fprintf(fp, "var REMOTE_USER %s\n", user);
		fclose(fp);
	}

fail:
	return response_page;
}

static int auth_session_exist(const char *sid, const char *gcode)
{
	FILE *fp;
	int ret = -1;
	char sid_contain[128], fname[128];
	char sid_id[16], sid_code[16], gcode_upper[16];

	sprintf(fname, "%s/%s", GRAPH_AUTH_TEMPDIR, getenv("REMOTE_ADDR"));

	/* if 'graph_enable' key equal to "none"
	 * means graphical authentication disable */
	if (NVRAM_MATCH("graph_enable", "none"))
		goto suc;

	if ((fp = fopen(fname, "r")) == NULL)
		goto out;

	bzero(sid_contain, sizeof(sid_contain));
	bzero(sid_id, sizeof(sid_id));
	bzero(sid_code, sizeof(sid_code));

	fscanf(fp, "%s", sid_contain);
	fclose(fp);

	/* XXXX */
	/*
	DEBUG_MSG("sid: %s\n", sid);
	DEBUG_MSG("gcode: %s\n", gcode);
	DEBUG_MSG("fname: %s\n", fname);
	DEBUG_MSG("sid_contain: %s\n", sid_contain);
	*/

	sscanf(sid_contain, "%[^.].%s", sid_id, sid_code);
	/* XXXX */
	/*
	DEBUG_MSG("sid_id: %s\n", sid_id);
	DEBUG_MSG("sid_code: %s\n", sid_code);
	*/
	/* XXX all gcode charactor to upper case */
	auth_graph_toupper(gcode, gcode_upper);

	if (strncmp(sid_id, sid, sizeof(sid_id)) != 0 ||
		strncmp(sid_code, gcode_upper, sizeof(sid_code)) != 0) {
		goto out;
	}

suc:
	unlink(fname);
	sprintf(fname, "%s/%s_allow", GRAPH_AUTH_TEMPDIR, getenv("REMOTE_ADDR"));
	if ((fp = fopen(fname, "w")) == NULL)
		goto out;

	fclose(fp);
	ret = 0;
out:
	return ret;
}

extern char *get_response_page();	/* from sscinf.c */
void *do_graph_auth(struct ssc_s *obj)
{
	char *remote, *port, *user, *pass, *pass_encode, *graph, *session_id;
	char *response_page = get_response_page();
	char *sslport;

	DEBUG_MSG("XXXX%s: REMOTE_ADDR: %s, REMOTE_PORT: %s\n"
		"       login_name:%s, login_pass:%s\n"
		"       login_n: %s, log_pass:%s\n"
		"       graph_code:%s, session_id: %s\n", __FUNCTION__,
		getenv("REMOTE_ADDR"), getenv("REMOTE_PORT"),
		get_env_value("login_name"), get_env_value("login_pass"),
		get_env_value("login_n"), get_env_value("log_pass"),
		get_env_value("graph_code"), get_env_value("session_id"));

	user = get_env_value("login_name");
	pass = get_env_value("login_pass");
	pass_encode = get_env_value("log_pass");
	graph = get_env_value("graph_code");
	session_id = get_env_value("session_id");
	sslport = get_env_value("SERVER_PORT");	

	remote = getenv("REMOTE_ADDR");
	port = getenv("REMOTE_PORT");

	if (access(GRAPH_AUTH_TEMPDIR, F_OK) != 0) {
		if (mkdir("/tmp/graph", 0777) != 0)
			DEBUG_MSG("XXX %s(%d) create %s fail\n", GRAPH_AUTH_TEMPDIR);
	}

	if (remote == NULL || port == NULL)
		goto fail;

	if (auth_session_exist(session_id, graph) == 0) {
		response_page = index_page_return(user, pass_encode, sslport);
		if (strcmp(response_page, "redirect2login.html") == 0)
			nvram_set("graph_auth_state", "0");
		else
			nvram_set("graph_auth_state", "1");

		setenv("REMOTE_USER", user, 1);
		nvram_set("current_user", user);
	}

fail:
	return response_page;
}
