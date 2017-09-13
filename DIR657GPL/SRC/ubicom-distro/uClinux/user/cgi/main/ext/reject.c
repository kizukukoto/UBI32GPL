#include <stdlib.h>
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
#include "ssc.h"
#include "rcctrl.h"

#define GRAPH_AUTH_TEMPDIR	"/tmp/graph"
#define MAX_URL_FILTER_NUMBER	20
#define PASSWORD_PATH		"/tmp/password"

static int user_exist_type(const char *cfg,const char *user,const char *pass)
{
	FILE *fp;
	int ret = 0;
	char t[128], dir[64], uid[64], pwd[64];

	if ((fp = fopen(cfg, "r")) == NULL) {
		if (strcmp(cfg, PASSWORD_PATH) == 0) {
			cprintf("XXX Cannot found %s, login always pass\n", PASSWORD_PATH);
			goto out;
		}
		goto fail;
	}

	bzero(t, sizeof(t));
	cprintf("XXX user_exist_type %s user [%s] pass [%s]\n", cfg, user, pass);
	while (fscanf(fp, "%s", t) != EOF) {
	/* ignore the first line 'A:192.168.0.1/24' */
		if (*t == 'A') continue;

		bzero(dir, sizeof(dir));
		bzero(uid, sizeof(uid));
		bzero(pwd, sizeof(pwd));

		sscanf(t, "%[^:]:%[^:]:%s", dir, uid, pwd);
		cprintf("XXX uid [%s] <==> user [%s], pwd [%s] <==> pass [%s]\n",
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

static void *index_page_return(const char *user,const char *pass)
{
	char *response_page = "reject.html";

	cprintf("XXX index_page_return user [%s] pass [%s]\n", user, pass);

	if (user_exist_type(PASSWORD_PATH, user, pass) == 0)
		response_page = get_response_page();

	if (strcmp(response_page, "reject.html") != 0) {
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

void *do_allow_reject_page(struct ssc_s *obj)
{
	char *user, *pass, *pass_encode;
	char *response_page = get_response_page();
	char reject_url[256] = {};
	char rule[256] = {};
	char rule_name[] = "url_domain_filter_XXXY";
	int i;

	cprintf("XXXX%s:login_name:%s, login_pass:%s, login_n: %s, log_pass:%s\n",
		__FUNCTION__,
		getenv("login_name"), getenv("login_pass"),
		getenv("login_n"), getenv("log_pass"));

	user = getenv("login_name");
	pass = getenv("login_pass");
	pass_encode = getenv("log_pass");
	strcpy(reject_url, getenv("reject_url"));
	response_page = index_page_return(user, pass_encode);

	/*Joe Huang:It means Authentication failed when response_page is reject.html*/
	if (strcmp(response_page, "reject.html") == 0){
		nvram_set("graph_auth_state", "0");
		goto fail;
	}else{
		nvram_set("graph_auth_state", "1");
		setenv("REMOTE_USER", user, 1);
		nvram_set("current_user", user);
	}

	for(i = 0 ; i < MAX_URL_FILTER_NUMBER ; i++){
		sprintf(rule_name, "url_domain_filter_%02d", i);
		strcpy(rule, nvram_safe_get(rule_name));

		if(nvram_match("url_domain_filter_type", "list_deny") == 0){
			if(strcmp(reject_url, rule) == 0 ){
				nvram_set(rule_name, "");
				break;
			}
		}else if(nvram_match("url_domain_filter_type", "list_allow") == 0){
			if(strlen(rule) == 0){
				nvram_set(rule_name, reject_url);
				break;
			}
		} 
	}

	rc_restart();

	fail:
		return response_page;
}


