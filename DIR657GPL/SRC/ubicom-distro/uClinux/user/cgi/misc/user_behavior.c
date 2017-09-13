#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdb.h"
#define HTTPD_PASSWORD "/tmp/password"

static int is_user_in_group(const char *user, const char *group)
{
	char *userinfo = NULL;
	char *username = NULL;
	char *password = NULL;
	char group_users[128];
	char *tmp_group_users = NULL;

	bzero(group_users, sizeof(group_users));
	query_record(group, group_users, sizeof(group_users));

	if(strlen(group_users) == 0)
		return 0;

	tmp_group_users = group_users;
	strsep(&tmp_group_users, "/");

	while((userinfo = strsep(&tmp_group_users, "/")) != NULL && *userinfo != '\0') {
		username = strsep(&userinfo, ",");
		password = strsep(&userinfo, ",");

		if(strcmp(user, username) == 0)
			return 1;
	}

	return 0;
}

static void redir_remote_admin()
{
	printf("<script>\n");
	printf("	location.href = \"/cgi/ssi/index.asp\";\n");
	printf("</script>\n");
}

static void redir_ssl()
{
	printf("<script>\n");
	printf("        location.href = \"/cgi/ssi/ssl/auth.asp\";\n");
	printf("</script>\n");
}

static int is_user_in_http(const char *user)
{
	FILE *fp = fopen("/tmp/remote.users", "r");
	char u[80];

	if(fp == NULL)
		return 0;

	bzero(u, sizeof(u));
	while((fscanf(fp, "%s", u)) != EOF)
		if(strcmp(u, user) == 0)
			return 1;

	fclose(fp);
	return 0;
}

static int is_user_in_sv(const char *user, char *sv)
{
	char *groups = NULL;
	char *group = NULL;

	strsep(&sv, ",");
	strsep(&sv, ",");
	groups = strsep(&sv, ",");

	while((group = strsep(&groups, ":")) != NULL && *group != '\0')
		if(is_user_in_group(user, group))
			return 1;

	return 0;
}

static int is_user_in_sslvpn(const char *user)
{
	int i = 1;
	char sv[80];
	char tmp_str[80];

	bzero(sv, sizeof(sv));

	for(; i <= 6; i++) {
		bzero(tmp_str, sizeof(tmp_str));
		sprintf(tmp_str, "sslvpn%d", i);
		query_record(tmp_str, sv, sizeof(sv));

		if(strlen(sv) == 0 || *sv == '0') continue;

		if(is_user_in_sv(user, sv) == 1)
			return 1;
	}

	return 0;
}

int user_behavior(int argc, char *argv[])
{
	char groups_name[80];
	char http_group_enable[5];
	char *remote_user = NULL;
/*	char *group_name = NULL;
	char *tmp_groups = NULL; */

	bzero(http_group_enable, sizeof(http_group_enable));
	bzero(groups_name, sizeof(groups_name));
	query_record("http_group_enable", http_group_enable, sizeof(http_group_enable));
	query_record("http_group", groups_name, sizeof(groups_name));
	remote_user = getenv("REMOTE_USER");

#ifdef DEBUG
	printf("REMOTE_USER: %s\n", remote_user);
	printf("http_group: %s\n", groups_name);
#endif

	if(is_user_in_sslvpn(remote_user) && is_user_in_http(remote_user)) {
		redir_ssl();
		return 1;
	}

	if(is_user_in_http(remote_user)) {
		redir_remote_admin();
		return 1;
	}

/*
	if(strcmp(http_group_enable, "0") == 0 || strlen(http_group_enable) == 0) {
		redir_ssl();
		return 1;
	}

	tmp_groups = groups_name;
	while((group_name = strsep(&tmp_groups, "/")) != NULL && *group_name != '\0') {
		if(is_user_in_group(remote_user, group_name)) {
			redir_remote_admin();
			return 1;
		}
	}
*/
	redir_ssl();
	return 1;
}
