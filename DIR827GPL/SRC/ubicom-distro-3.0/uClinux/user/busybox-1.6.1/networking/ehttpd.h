#ifndef __EHTTPD__
#define __EHTTPD__

#include <stdio.h>
#include <sys/types.h>
#include "libbb.h"

#if ENABLE_FEATURE_HTTPD_SSL
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif /* ENABLE_FEATURE_HTTPD_SSL */
/* amount of buffering in a pipe */
#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif

#define MAX_MEMORY_BUFF 8192	/* IO buffer */

#define _printf(dev, fmt, args...) do { \
	FILE *fp = fopen(dev, "w"); \
	if (fp) { \
		fprintf(fp, fmt, ## args); \
		fclose(fp); \
	} \
} while (0)

#define cprintf(fmt, args...)	_printf("/dev/console", fmt, ## args);

#if ENABLE_FEATURE_HTTPD_DEBUG
#define DEBUG_MSG(fmt, args...) \
	cprintf("XXX %s(%d) - "fmt, __FUNCTION__, __LINE__, ## args);
#else
#define DEBUG_MSG(fmt, args...) \
	_printf("/dev/null", fmt, ## args);
#endif	/* ENABLE_FEATURE_HTTPD_DEBUG */

/* httpd.c */
extern void decodeBase64(char *);

/* ehttpd.c */
extern int call_cmd(const char *cmd);
extern int allowed_page(const char *url, const char *fromfunc);
extern int auth_session_timeout(const char *rmt_ip_str);
extern int auth_session_exist(const char *rmt_ip_str);
extern int auth_setenv_from_file(const char *rmt_ip_str);
extern int str_replace(char *str, const char *dst, const char *tok, size_t slen);

/* httpd_auth.c */
extern int checkPerm(const char *, const char *);
extern int checkPerm2(const char *, const char *);

typedef struct HT_ACCESS {
	char *after_colon;
	struct HT_ACCESS *next;
	char before_colon[1];	/* really bigger, must last */
} Htaccess;

typedef struct HT_ACCESS_IP {
	unsigned int ip;
	unsigned int mask;
	int allow_deny;
	struct HT_ACCESS_IP *next;
} Htaccess_IP;

typedef struct HttpdConfig{
	char buf[MAX_MEMORY_BUFF];

	USE_FEATURE_HTTPD_BASIC_AUTH(const char *realm;)
	USE_FEATURE_HTTPD_BASIC_AUTH(char *remoteuser;)

	const char *query;

	USE_FEATURE_HTTPD_CGI(char *referer;)

	const char *configFile;

	unsigned int rmt_ip;
#if ENABLE_FEATURE_HTTPD_CGI || DEBUG
	char *rmt_ip_str;	/* for set env REMOTE_ADDR */
#endif
        unsigned port;		/* server initial port and for
				   set env REMOTE_PORT */
	const char *found_mime_type;
	const char *found_moved_temporarily;

	off_t ContentLength;	/* -1 - unknown */
	time_t last_mod;

	Htaccess_IP *ip_a_d;	/* config allow/deny lines */
	int flg_deny_all;
#if ENABLE_FEATURE_HTTPD_BASIC_AUTH
	Htaccess *auth;		/* config user:password lines */
#endif
#if ENABLE_FEATURE_HTTPD_CONFIG_WITH_MIME_TYPES
	Htaccess *mime_a;	/* config mime types */
#endif

	int server_socket;
	int accepted_socket;
	volatile int alarm_signaled;

#if ENABLE_FEATURE_HTTPD_CONFIG_WITH_SCRIPT_INTERPR
	Htaccess *script_i;	/* config script interpreters */
#endif
#if  ENABLE_FEATURE_HTTPD_SSL
	int do_ssl;
	const char* certfile;
	char* cipher;
	SSL_CTX* ssl_ctx;
	SSL *ssl;
#endif /* ENABLE_FEATURE_HTTPD_SSL */
	ssize_t (*read)(struct HttpdConfig *,void *, size_t);
	ssize_t (*safe_read)(struct HttpdConfig *, void *, size_t);
	ssize_t (*safe_write)(struct HttpdConfig *, const void *, size_t);
	ssize_t (*full_read)(struct HttpdConfig *, void *, size_t);
	ssize_t (*write)(struct HttpdConfig *, const void *, size_t);
	ssize_t (*full_write)(struct HttpdConfig *, const void *, size_t);
	USE_FEATURE_HTTPD_MAPPING_URL(const char *map_url;)
} HttpdConfig;

#if ENABLE_FEATURE_HTTPD_GRAPH_AUTH
#define INDEX_PAGE	"redirect2login.html"
#elif ENABLE_FEATURE_HTTPD_SSLUSER_LOGIN
#define INDEX_PAGE	"index.html"
#else
#define INDEX_PAGE	"index_nossl.html"
#endif	/* ENABLE_FEATURE_HTTPD_GRAPH_AUTH */
#define SESSION_TIMEOUT	60
#endif	/* __EHTTPD__ */
