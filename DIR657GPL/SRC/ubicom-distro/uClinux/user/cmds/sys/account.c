#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cmds.h"

#define HTTPD_CONF	"/tmp/password"

extern int base64_encode(char *, char *, int);	/* in lib/common-tools.c */

/* 2010/7/6 FredHung
 * This function will create login account used for graphical/SSL login.
 * The following file will be create in here:
 *
 * /tmp/password
 * */

static int account_httpd(char const *conf)
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

int account_main(int argc, char *argv[])
{
	unlink(HTTPD_CONF);
	account_httpd(HTTPD_CONF);

	return 0;
}
