/* CONFIG_FEATURE_HTTPD_GRAPHICA_AUTH_GDIR := graphical exists folder */

#if ENABLE_FEATURE_HTTPD_GRAPHICAL_AUTH
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define _printf(dev, fmt, args...) do { \
	FILE *fp = fopen(dev, "w"); \
	if (fp) { \
		fprintf(fp, fmt, ## args); \
		fclose(fp); \
	} \
} while (0)

#define cprintf(fmt, args...) _printf("/dev/console", fmt, ## args);

#if ENABLE_FEATURE_HTTPD_DEBUG
#define DEBUG_MSG(fmt, args...) \
	cprintf("XXX %s(%d) - "fmt, __FUNCTION__, __LINE__, ## args);
#else
#define DEBUG_MSG(fmt, args...) \
	_printf("/dev/null", fmt, ## args);
#endif

#if ENABLE_FEATURE_HTTPD_ACCESS_CONTROL
extern int httpd_ext_access_control(char *);
#endif

static int auth_session_exist(const char *session_handle)
{
#if ENABLE_FEATURE_HTTPD_DEBUG
	int ret = access(session_handle, F_OK);

	cprintf("XXX %s(%d) session_handle: '%s' ret: '%d'\n",
		__FUNCTION__,
		__LINE__,
		session_handle,
		ret);

	return ret;
#else
	return access(session_handle, F_OK);
#endif
}

static int auth_session_renew(const char *session_handle)
{
	int ret = -1;
	char cmds[128];

	if (access(session_handle, F_OK) != 0)
		goto out;

	sprintf(cmds, "touch %s", session_handle);
	system(cmds);

	ret = 0;
out:
#if ENABLE_FEATURE_HTTPD_DEBUG
	cprintf("XXX %s(%d) session_handle: '%s' cmds: '%s' ret: '%d'\n",
		__FUNCTION__,
		__LINE__,
		session_handle,
		cmds,
		ret);
#endif
	return ret;
}

static int auth_get_timeout_second(void)
{
	FILE *fp = popen("/bin/nvram get session_timeout", "r");
	char *p, session_timeout[128], timeout_second[16];

	if (fp == NULL)
		return CONFIG_FEATURE_HTTPD_GRAPHICAL_AUTH_TIMEOUT;

	bzero(session_timeout, sizeof(session_timeout));
	fgets(session_timeout, sizeof(session_timeout) - 1, fp);
	//fscanf(fp, "%s", session_timeout);
	pclose(fp);

	if (*session_timeout == '\0')
		return CONFIG_FEATURE_HTTPD_GRAPHICAL_AUTH_TIMEOUT;

	/* @session_timeout := "session_timeout=60"
	 * We must skip the '=' charactor here.
	 *
	 * 2010/07/07 FredHung */
	if ((p = strstr(session_timeout, "=")) == NULL)
		strcpy(timeout_second, "60");	/* default value */
	else {
		strcpy(timeout_second, p + 2);
		timeout_second[strlen(timeout_second) - 1] = '\0';
	}
#if ENABLE_FEATURE_HTTPD_DEBUG
	cprintf("XXX %s(%d) timeout_second: '%s'\n",
		__FUNCTION__,
		__LINE__,
		timeout_second);
#endif
	return atoi(timeout_second);
}


static int auth_session_timeout(const char *session_handle)
{
	int ret = -1;
	struct stat buf;
	time_t cur_time = time(NULL);

	stat(session_handle, &buf);

	if (cur_time - buf.st_atime <= auth_get_timeout_second()) {
		auth_session_renew(session_handle);
		ret = 0;
	} else
		unlink(session_handle);

	DEBUG_MSG("ret: '%d'\n", ret);

#if ENABLE_FEATURE_HTTPD_DEBUG
	cprintf("XXX %s(%d) session_handle: '%s' ret: '%d'\n",
		__FUNCTION__,
		__LINE__,
		session_handle,
		ret);
#endif
	return ret;
}

/* Input:
 *
 * IPv6 @rmt_ip_str := [::ffff:192.168.0.100]:1234
 * IPv4 @rmt_ip_str := 192.168.0.100:1234
 *
 * Return: Always 0
 * */
static int auth_create_session_handle(const char *rmt_ip_str, char *buf)
{
	char *pe, ip[128];

	bzero(ip, sizeof(ip));

	if ((pe = strrchr(rmt_ip_str, ':')) == NULL)	/* port not found */
		strcpy(ip, rmt_ip_str);
	else
		strncpy(ip, rmt_ip_str, pe - rmt_ip_str);
#if 0
	bzero(ip, sizeof(ip));
	if ((pe = strrchr(rmt_ip_str, ']')) == NULL) {
		strncpy(ip, rmt_ip_str, strrchr(rmt_ip_str, ':') - rmt_ip_str);
		goto out;
	}

	strncpy(ip, rmt_ip_str + 1, pe - rmt_ip_str - 1);
	for (pe = ip; *pe != '\0'; pe++)
		if (*pe == ':')
			*pe = '-';
out:
#endif
	sprintf(buf, "%s/%s_allow", CONFIG_FEATURE_HTTPD_GRAPHICAL_AUTH_SDIR, ip);
	return 0;
}

const char *httpd_auth_session_timeout(const char *rmt_ip_str, char *urlcopy, char *urlcopy_map);
const char *httpd_auth_session_timeout(const char *rmt_ip_str, char *urlcopy, char *urlcopy_map)
{
	char session_handle[128];
	int a, b;

	bzero(session_handle, sizeof(session_handle));
	auth_create_session_handle(rmt_ip_str, session_handle);

#if ENABLE_FEATURE_HTTPD_ACCESS_CONTROL
	if (httpd_ext_access_control(urlcopy_map) == 0)
		goto out;
	strcpy(urlcopy_map, urlcopy);
	if (httpd_ext_access_control(urlcopy) == 0)
		goto out;
#endif
	if ((a = auth_session_exist(session_handle)) != 0)
		goto fail;
	if ((b = auth_session_timeout(session_handle)) != 0)
		goto fail;

	auth_session_renew(session_handle);
out:
	return urlcopy_map + 1;
fail:
#if ENABLE_FEATURE_HTTPD_DEBUG
	cprintf("XXX %s(%d) session_exist result: '%d'\n",
		__FUNCTION__,
		__LINE__,
		a);
	cprintf("XXX %s(%d) session_timeout result: '%d'\n",
		__FUNCTION__,
		__LINE__,
		b);
#endif
	/* XXX: If user click 'page A' link but session timeout, browser will
	 * return to index page for user relogin again. After relogin, if user
	 * click 'page A' again, browser return to login page even if session
	 * haven't timeout. I don't what happen about this. I just chage returned
	 * page from 'redirect2login.html' to '/cgi/ssi/login_pic.asp', this bug
	 * was fixed. Have somebody tell me why?
	 *
	 * Fredhung 2010/7/14 */
	/* strcpy(urlcopy_map, CONFIG_FEATURE_HTTPD_GRAPHICAL_AUTH_INDEX); */
	strcpy(urlcopy_map, "/cgi/ssi/login_pic.asp");
	return urlcopy_map + 1;
}

#endif
