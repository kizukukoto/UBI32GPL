#include <stdio.h>
#include <string.h>

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
#endif	/* ENABLE_FEATURE_HTTPD_DEBUG */

#if ENABLE_FEATURE_HTTPD_HNAP_SUPPORT
char *hnap_query = "/HNAP1/";
char *hnap_fullpath = "/cgi/ssi/hnap.cgi";
#endif	/* ENABLE_FEATURE_HTTPD_HNAP_SUPPORT */

#if ENABLE_FEATURE_HTTPD_ACCESS_CONTROL
int httpd_ext_access_control(char *urlcopy_map);
int httpd_ext_access_control(char *urlcopy_map)
{
	int ret = -1;
	struct {
		const char *url;
		int policy;
	} *p, l[] = {
		{ "/chklst.txt", 0 },
		{ "/wlan.txt", 0 },
		{ "/cgi/ssi/redirect_version.html", 0 },
		{ "/cgi/ssi/chklst.txt", 0 },
		{ "/cgi/ssi/wlan.txt", 0 },
		{ "/cgi/ssi/css_router.css", 0 },
		{ "/cgi/ssi/lang_en.js", 0 },
		{ "/cgi/ssi/lang.js", 0 },
		{ "/cgi/ssi/public.js", 0 },
		{ "/cgi/ssi/wlan_masthead.gif", 0 },
		{ "/cgi/ssi/wireless_tail.gif", 0 },
		{ "/cgi/ssi/login.cgi", 0 },
		{ "/cgi/ssi/device_status.xml", 0 },
		{ "/cgi/ssi/internet_session.xml", 0 },
		{ "/cgi/ssi/ipv6_autoconnect.xml", 0 },
		{ "/cgi/ssi/internet_session.js", 0 },
		/* reject page only */
		{ "/cgi/ssi/reject.html", 0 },
		{ "/css_router.css", 0 },
		{ "/lang_en.js", 0 },
		{ "/lang.js", 0 },
		{ "/public.js", 0 },
		{ "/public_msg.js", 0 },
		{ "/wlan_masthead.gif", 0 },
		{ "/wireless_tail.gif", 0 },
		{ "/internet_session.js", 0 },
#if ENABLE_FEATURE_HTTPD_GRAPHICAL_AUTH
		{ CONFIG_FEATURE_HTTPD_GRAPHICAL_AUTH_INDEX, 0 },
		{ "/cgi/ssi/login_pic.asp", 0 },
		{ "/cgi/ssi/login_fail.asp", 0 },
		{ "/cgi/ssi/auth.bmp", 0 },
#endif
#if ENABLE_FEATURE_HTTPD_HNAP_SUPPORT
		{ hnap_fullpath, 0 },
#endif
		{ "/cgi/ssi/apply.cgi", 0 },
		{ NULL, -1 }
	};

	for (p = l; p->url; p++) {
		if (strcmp(urlcopy_map, p->url) == 0) {
			ret = p->policy;
			break;
		}
	}

#if ENABLE_FEATURE_HTTPD_DEBUG
	cprintf("XXX %s(%d) urlcopy_map: '%s' ret: '%d'\n",
		__FUNCTION__,
		__LINE__,
		urlcopy_map,
		ret);
#endif

	return ret;
}
#endif	/* ENABLE_FEATURE_HTTPD_ACCESS_CONTROL */

#if ENABLE_FEATURE_HTTPD_MAPPING_URL

char *httpd_ext_url_mapping(const char *urlcopy, char *urlcopy_map);
char *httpd_ext_url_mapping(const char *urlcopy, char *urlcopy_map)
{
	struct {
		const char *tptr;
		const char *mptr;
	} *p, lst[] = {
		{ "/chklst.txt", "/cgi/ssi/chklst.txt" },
		{ "/wlan.txt", "/cgi/ssi/wlan.txt" },
		{ "/version.txt", "/cgi/ssi/redirect_version.html" },
		{ "/login.cgi",  "/cgi/ssi/login.cgi" },
		{ "/device_status.xml", "/cgi/ssi/device_status.xml" },
		{ "/ipv6_autoconnect.xml", "/cgi/ssi/ipv6_autoconnect.xml" },
		{ "/internet_session.xml", "/cgi/ssi/internet_session.xml" },
		{ "/internet_session.js", "/cgi/ssi/internet_session.js" },
		{ "/reject.html", "/cgi/ssi/reject.html" },
#if ENABLE_FEATURE_HTTPD_HNAP_SUPPORT
		{ hnap_query, hnap_fullpath },
#endif
#if ENABLE_FEATURE_HTTPD_GRAPHICAL_AUTH
		{ "/", CONFIG_FEATURE_HTTPD_GRAPHICAL_AUTH_INDEX },
#endif
		{ NULL, NULL }
	};

	//strcpy(urlcopy_map, urlcopy + 1);	/* skip the first '/' */

	for (p = lst; p->tptr; p++) {
		if (strcmp(urlcopy, p->tptr) == 0) {
			strcpy(urlcopy_map, p->mptr);
			break;
		}
	}
#if ENABLE_FEATURE_HTTPD_DEBUG
	cprintf("XXX %s(%d) urlcopy: '%s' urlcopy_map: '%s'\n",
		__FUNCTION__,
		__LINE__,
		urlcopy,
		urlcopy_map);
#endif

	return urlcopy_map + 1;
}

#if ENABLE_FEATURE_HTTPD_HNAP_SUPPORT
int httpd_ext_account_verify(const char *user_and_passwd);
int httpd_ext_account_verify(const char *user_and_passwd)
{
	FILE *fp;
	int ret = -1;
	char tmp[128], user_and_passwd_t[256];
	char cmd_usr[] = "/bin/nvram get admin_username";
	char cmd_pwd[] = "/bin/nvram get admin_password";

	bzero(tmp, sizeof(tmp));
	bzero(user_and_passwd_t, sizeof(user_and_passwd_t));

	if ((fp = popen(cmd_usr, "r")) == NULL)
		goto out;
	fgets(tmp, sizeof(tmp) - 1, fp);
	pclose(fp);

	/* skip '=' */
	strcat(user_and_passwd_t, strstr(tmp, "=") + 2);
	//strcat(user_and_passwd_t, tmp);

	if (user_and_passwd_t[strlen(user_and_passwd_t) - 1] == '\n')
		user_and_passwd_t[strlen(user_and_passwd_t) - 1] = '\0';
	strcat(user_and_passwd_t, ":");
	bzero(tmp, sizeof(tmp));

	if ((fp = popen(cmd_pwd, "r")) == NULL)
		goto out;
	fgets(tmp, sizeof(tmp) - 1, fp);
	pclose(fp);

	/* skip '=' */
	strcat(user_and_passwd_t, strstr(tmp, "=") + 2);
	//strcat(user_and_passwd_t, tmp);

	if (user_and_passwd_t[strlen(user_and_passwd_t) - 1] == '\n')
		user_and_passwd_t[strlen(user_and_passwd_t) - 1] = '\0';
	if (strcmp(user_and_passwd_t, user_and_passwd) == 0)
		ret = 0;
out:
	return ret;
}
#endif	/* ENABLE_FEATURE_HTTPD_HNAP_SUPPORT */
#endif	/* ENABLE_FEATURE_HTTPD_MAPPING_URL */
