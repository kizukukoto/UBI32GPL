#include "ehttpd.h"

/* from httpd.c */
extern HttpdConfig *config;

/****************************************************************************
 *
 > $Function: checkPerm()
 *
 * $Description: Check the permission file for access password protected.
 *
 *   If config file isn't present, everything is allowed.
 *   Entries are of the form you can see example from header source
 *
 * $Parameters:
 *      (const char *) path  . . . . The file path.
 *      (const char *) request . . . User information to validate.
 *
 * $Return: (int)  . . . . . . . . . 1 if request OK, 0 otherwise.
 *
 ****************************************************************************/

#if ENABLE_FEATURE_HTTPD_BASIC_AUTH
int checkPerm(const char *path, const char *request)
{
	Htaccess * cur;
	const char *p;
	const char *p0;

	const char *prev = NULL;
#if ENABLE_FEATURE_HTTPD_BASE64_CONFIG
	char enc_up[128];
#endif
//#if ENABLE_FEATURE_HTTPD_MAPPING_URL
#if ENABLE_FEATURE_HTTPD_MAPPING_STRING
	//syslog(LOG_INFO, "%s == %s?", config->map_url, path);
	if (strcmp(config->map_url, path) == 0)
		return 1;

	if (allowed_page(path, __func__) == 1)
		return 1;
#endif
	/* This could stand some work */
	for (cur = config->auth; cur; cur = cur->next) {
		size_t l;

		p0 = cur->before_colon;
		if (prev != NULL && strcmp(prev, p0) != 0)
			continue;       /* find next identical */
		p = cur->after_colon;
		DEBUG_MSG("checkPerm: '%s' ? '%s'\n", p0, request);
		DEBUG_MSG("checkPerm: p0(%s) path(%s)\n", p0, path);

		l = strlen(p0);
		if (strncmp(p0, path, l) == 0
		 && (l == 1 || path[l] == '/' || path[l] == '\0')
		) {
			char *u;
			/* path match found.  Check request */
			/* for check next /path:user:password */
			prev = p0;
			u = strchr(request, ':');
			if (u == NULL) {
				/* bad request, ':' required */
				break;
			}

			if (ENABLE_FEATURE_HTTPD_AUTH_MD5) {
				char *cipher;
				char *pp;
				if (strncmp(p, request, u-request) != 0) {
					/* user uncompared */
					continue;
				}
				pp = strchr(p, ':');
				if (pp && pp[1] == '$' && pp[2] == '1' &&
						pp[3] == '$' && pp[4]) {
					pp++;
					cipher = pw_encrypt(u+1, pp);
					if (strcmp(cipher, pp) == 0)
						goto set_remoteuser_var;   /* Ok */
					/* unauthorized */
					continue;
				}
			}

#if ENABLE_FEATURE_HTTPD_BASE64_CONFIG
			{
				char *pp = strchr(p, ':');
				char enc_u[64], enc_p[64];

				bzero(enc_u, sizeof(enc_u));
				bzero(enc_p, sizeof(enc_p));

				strncpy(enc_u, p, pp - p);
				strcpy(enc_p, pp + 1);

				decodeBase64(enc_u);
				decodeBase64(enc_p);
				sprintf(enc_up, "%s:%s", enc_u, enc_p);
			}

			DEBUG_MSG("account (%s) <==> (%s)\n", enc_up, request);
			if (strcmp(enc_up, request) == 0) {
#else
			DEBUG_MSG("account (%s) <==> (%s)\n", p, request);
			if (strcmp(p, request) == 0) {
#endif
set_remoteuser_var:
				config->remoteuser = strdup(request);
				if (config->remoteuser)
					config->remoteuser[(u - request)] = 0;
				return 1;   /* Ok */
			}
			/* unauthorized */
		}
	}   /* for */

	return prev == NULL;
}

#endif  /* FEATURE_HTTPD_BASIC_AUTH */

/* fred_hung 2008/12/23
 *
 * This function will check username and password using http_login
 * external executable file by pass env USERNAME and PASSWORD. And
 * this function can check url mapping if needed.
 *
 * @url := /cgi/ssi/index.asp
 * @uidpwd := fred:12345678
 */
#if ENABLE_FEATURE_HTTPD_GRAPH_AUTH
int checkPerm2(const char *url, const char *uidpwd)
{
#if !ENABLE_FEATURE_HTTPD_HIERACHICAL_AUTH
	DEBUG_MSG("url(%s) uidpwd(%s)\n", url, uidpwd);

	access(config->rmt_ip_str, F_OK);
	config->remoteuser = strdup("");

	return 1;
#else
	int ret = 1;
	char *p, tmp_uidpwd[128], uid[64], pwd[64];

	DEBUG_MSG("url(%s) uidpwd(%s)\n", url, uidpwd);
	strncpy(tmp_uidpwd, uidpwd, sizeof(tmp_uidpwd) - 1);
	p = tmp_uidpwd;

	strncpy(uid, strsep(&p, ":"), sizeof(uid) - 1);
	strncpy(pwd, p, sizeof(pwd) - 1);

	/*
	 *  USERNAME and PASSWORD environment pass to
	 * /etc/sysconfig/http_login. In http_login, use getenv(...) can
	 * get USERNAME and PASSWORD and check permission
	 */
	setenv1("USERNAME", uid);
	setenv1("PASSWORD", pwd);

	setenv1("PROTO", config->do_ssl?"HTTPS":"HTTP");

#if ENABLE_FEATURE_HTTPD_MAPPING_STRING
	if (strcmp(config->map_url, url) == 0)
		goto out;

	if (allowed_page(url, __func__) == 1)
		goto out;
#endif
	ret = (call_cmd("/etc/sysconfig/http_login_main") == 0)?1:0;
out:
	if (ret)
		config->remoteuser = strdup(uid);

	return ret;
#endif	/* !ENABLE_FEATURE_HTTPD_HIERACHICAL_AUTH */
}
#endif
