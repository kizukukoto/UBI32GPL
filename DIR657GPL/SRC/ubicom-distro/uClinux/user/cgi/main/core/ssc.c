#include <stdio.h>
#include <signal.h>

#include "querys.h"
#include "log.h"
#include "ssi.h"
#include "ssc.h"
#include "helper.h"

//#include "virtual_file.h"

#define PERM_VERIFY 0

struct ssc_s *SSC_OBJS = NULL;
struct method_plugin *ssc_post_method_plugin = NULL;
/*
extern char *upload_ca();
static struct method_plugin special_files[] = {
	{"/update_firmw.cgi", update_firmware},
	{"/upload_configure.cgi", upload_conf},
        {"/upload_certificate.cgi", upload_ca},
	{"", NULL}
};
*/

extern char *nvram_get(const char *);
static int
nvram_safe_get_cmp(const char *key, char *buf, size_t len, const char *comp)
{
	char *p = nvram_get(key);

	bzero(buf, len);
	if (p == NULL || comp == NULL)
		return -1;

	strncpy(buf, p, len);
	return strcmp(buf, comp);
}

#if 0
static int __rdonly_check(const char *remote_user, const char *group)
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
		if (strcmp(user, remote_user) == 0)
			return 0;
	} while (p != NULL);

	return 1;
}
#endif

/* in cgi/lib/libutil.c */
extern int base64_encode(char *, char *, int);

char *do_ssc()
{
	struct method_plugin *f;
	struct ssc_s *opt;
	char *res = "error.asp";
	char value[128];
	char *path_info = getenv("PATH_INFO");
	char *current_user = nvram_get("current_user");
	char *rdonly_user = nvram_get("user_username");
	char rdonly_user_b64[128];

	if (rdonly_user && current_user) {
		base64_encode(rdonly_user, rdonly_user_b64, strlen(rdonly_user));

		if (strcmp(current_user, rdonly_user_b64) == 0) {
			setenv("html_response_error_message",
				"Only admin account can change the settings.", 1);
			setenv("html_response_return_page", "index.asp", 1);
			return res;
		}
	}
#if 0
	char *remote_user = getenv("REMOTE_USER");
	char *rdonly_user = nvram_get("ro_username");

//#if PERM_VERIFY
	if (strcmp(remote_user, (rdonly_user == NULL)?"":rdonly_user) == 0) {
		setenv("html_response_error_message",
			"Only admin account can change the settings.", 1);
		setenv("html_response_return_page",
			"index.asp", 1);
		return res;
	}
#endif
#if 0
	char *rdonly_enabled = nvram_get("rdonly_group_enable");
	char rdonly_groups_v[256], *g, *p;

	bzero(rdonly_groups_v, sizeof(rdonly_groups_v));
	path_info = getenv("PATH_INFO");
	strncpy(rdonly_groups_v, nvram_get("rdonly_group"), sizeof(rdonly_groups_v));

	/* read only account acecking, 2008/10/17 fredhung..I want to go home!! */
	if (rdonly_enabled != NULL && strcmp(rdonly_enabled, "1") == 0) {
		p = rdonly_groups_v;
		while((g = strsep(&p, "/")) != NULL && *g != '\0') {
			if (__rdonly_check(remote_user, g) == 0) {

				setenv("html_response_error_message",
					"Only admin account can change the settings.", 1);
				setenv("html_response_return_page",
					"index.asp", 1);
				return res;
			}
		}
	}
#endif

	if (path_info == NULL) {
		not_found("PATH_INFO");
		goto out;
	}

	for(f = ssc_post_method_plugin; f != NULL && f->func != NULL; f++) {
		if(strncmp(path_info, f->file_path, 
					sizeof(f->file_path)) == 0) {
			/* XXX: return (char *) ? */
			res = (char *)f->func();
			goto out;
		}
	}

	put_querystring_env();

	if (query_vars("action", value, sizeof(value)) == -1) {
		make_back_msg("The action_key does not be posted");
		cprintf("The action_key does not be posted");
		goto out;
	}

	for (opt = SSC_OBJS; opt->action != NULL; opt++) {
		//cprintf("searching...%s\n", opt->action);
		if (strcmp(opt->action, value) != 0)
			continue;
		//cprintf("string match %s = %s\n", value, opt->action);
		if (opt->fn) {
			//cprintf("Found action %s\n", value);
			res = opt->fn(opt);
			cprintf("Action result: %s\n", res);
		} else {
			make_back_msg("The action %s have not a valid function", value);
			cprintf("The action %s have not a valid function", value);
		}	
		goto out;
	}
	make_back_msg("No OBJS for action: %s", value);
	cprintf("No OBJS for action: %s\n", value);
out:
	return res;
}
