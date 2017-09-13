#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/wait.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "interface.h"
#include "proto.h"
#include "libcameo.h"
#include "nvram_ext.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

static char *__var_in_ii(const char *var, struct info_if *ii, char *buf)
{
	char *p;
	
	if ((p = nvram_get_i(var, ii->index, g1)) == NULL)
		return NULL;
	return strcpy(buf, p);
}

static char *__var_in_env(const char *var, struct info_if *ii, char *buf)
{
	char *e = getenv(var);

	if (e != NULL)
		strcpy(buf, e);
	return e;
}

static int
service_parser(char *s, char *argv[], struct info_if *ii, char *tmp, size_t len)
{
	int rev = -1, idx, i;
	char *sp[20], *p;
	struct {
		char *var;
		char *(*fn)(const char *, struct info_if *ii, char *);
	} ext_var[] = {
		{ "if_dev", __var_in_ii},
		{ "if_route", __var_in_ii},
		{ "Z_LOCALIP", __var_in_env},
		{ "Z_LOCALMASK", __var_in_env},
		{ "Z_GATEWAY", __var_in_env},
		{ NULL, NULL }
	}, *exp;
	idx = split(s, ",", sp, 1);

	bzero(tmp, len);
	for (i = 0; i < idx; i++) {
		p = sp[i];
		if (*p != '@') {
			argv[i] = sp[i];
			continue;
		}
		p++;
		for (exp = ext_var; exp->var != NULL; exp++) {
			if (strcmp(exp->var, p) != 0)
				continue;
			if (exp->fn(exp->var, ii, tmp) == NULL) {
				debug( "can not get :%s", exp->var);
				goto err;
			}
			//debug( "========GET ENV[%s][%s]", exp->var, tmp);
			argv[i] = tmp;
			tmp += strlen(tmp) + 1;
			break;
		}
	}
	rev = i;
err:
	return rev;
}
/*
 * @ret: return value of child process
 * 
 * Return: -1 on failure, indicates no executable file to execute
 * 		in this case, we will continue call "cli servies XXXX"
 * 	   0 success on call hooker.(even hooker return -1 or coredump...)
 * 	   	in this case, we will not call "cli servies XXXX"
 * 	   	and @ret is meaningful.
 * */
int fire_service_hook_updown(int updown, char *argv[], int *ret)
{
#define SERVICES_HOOK	"/etc/init.d/services/"
	char hook_cmd[256] = SERVICES_HOOK;
	pid_t pid;
	int _ext, rev = -1;

	/* buffer ? */
	strcat(hook_cmd, argv[0]);
	
	if (access(hook_cmd, X_OK) == -1)
		goto call_service;
	
	if ((pid = fork()) == -1)
		goto call_service;
	
	if (pid == 0) {
		execv(hook_cmd, argv);
		debug( "services hook:%s", argv[0]);
		exit(-1);
	}
	*ret = -1;
	if ((waitpid(pid, &_ext, 0) == -1))
		goto no_call_service;
	if (WIFEXITED(_ext))
		*ret = WEXITSTATUS(_ext);
	
no_call_service:
	return 0;	/* do not call service */
call_service:
	return -1;
}

extern struct subcmd *get_services_cmds();

/*
 * Call "cli services <XXXX> <start|stop> [args...]"
 *
 * Ex:
 * if_services0="http,@if_dev,@Z_LOCALIP,@Z_LOCALMASK"	   \
 * 	" dhcpd,LAN_dhcpd,@if_dev,@Z_LOCALIP,@Z_LOCALMASK" \
 * 	" upnp,@if_dev,@Z_LOCALIP"
 *  call http, dhcpd, upnp in each of @foreach loop
 * */
int ii_services_updown(int up, struct info_if *ii)
{
	int rev = -1, i, x;
	char s[512], argv_tmp[1024], stop_buf[1024];
	char *p, *n, *t, *argv[24] = { "services"};
	struct subcmd *cmds;
	
	debug( "%s,ii->services=%s,ii->index=%d\n", up?"up":"down",
		ii->services, ii->index);
	p = up ? ii->services: nvram_get_i("if_services_stop", ii->index, g1);

	if (p == NULL || strlen(p) <= 0){
		DEBUG_MSG("%s: p NULL fail!\n", __FUNCTION__);
		return 0;
	}	

	if ((p = strdup(p)) == NULL){
		DEBUG_MSG("%s: out fail!\n", __FUNCTION__);
		goto out;
	}	
		
	//p = "ntp httpd,@if_dev upnp,@if_dev, dhcpd,DMZ_dhcpd,@if_dev,@Z_LOCALIP,@Z_LOCALMASK";
	bzero(stop_buf, sizeof(stop_buf));
	foreach(s, p, n) {
		char bug[1024];
		int res = -1;
		t = s;
		argv[1] = strsep(&t, ", ");
		argv[2] = up ? "start" : "stop";
		
		/* service_parser: to inital from argv[3] to end of args */
		i = service_parser(t, argv + 3, ii, argv_tmp, sizeof(argv_tmp));
		bzero(bug, sizeof(bug));
		strcat(bug, argv[1]);
		strcat(bug, ",");
		for (x = 3; x < i + 3; x++) {
			strcat(bug, argv[x]);
			strcat(bug, ",");
		}
		bug[strlen(bug) - 1] = '\0';
		debug( "%s", bug);
		/*
		 * Here have a hooked to call
		 * external program 
		 * 	OR
		 * internal routine.
		 * */
		if (fire_service_hook_updown(up, argv + 1, &res) == -1 &&
			(res = lookup_subcmd_then_callout(i + 3 /* argc */,
				argv, get_services_cmds())) != 0)
		{
			/* Both of hook and cli services XXXX are fail */
			debug( "%s services: %s fail", argv[2], argv[1]);
		}
		if (res == 0 && up) {
			strcat(stop_buf, bug);
			strcat(stop_buf, " ");
		}
	}
	DEBUG_MSG("%s:%s services: %s stop_buf=%s\n", __FUNCTION__, argv[2], argv[1], stop_buf);
	up ? nvram_set_i("if_services_stop", ii->index, g1, chomp(stop_buf)):
		nvram_unset_i("if_services_stop", ii->index, g1);
	
	rev = 0;
	free(p);
out:	
	rd(rev, "error");
	return rev;
}
