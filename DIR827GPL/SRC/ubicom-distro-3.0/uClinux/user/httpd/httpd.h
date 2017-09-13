/*
 * milli_httpd - pretty small HTTP server
 *
 */

#ifndef _httpd_h_
#define _httpd_h_

#if defined(DEBUG) && defined(DMALLOC)
#include <dmalloc.h>
#endif

/* Basic authorization userid and passwd limit */
#define AUTH_MAX 120

/* Generic MIME type handler */
struct mime_handler {
	char *pattern;
	char *mime_type;
	char *extra_header;
	void (*input)(char *path, FILE *stream, int len, char *boundary);
	void (*output)(char *path, FILE *stream);
	void (*auth)(char *admin_userid, char *admin_passwd, char *user_userid, char *user_passwd, char *realm);
};
extern struct mime_handler mime_handlers[];
extern char no_cache[];
extern char download_nvram[];
extern char log_to_hd[];

/* CGI helper functions */
extern void init_cgi(char *query);
extern char * get_cgi(char *name);
extern void set_cgi(char *name, char *value);
extern int count_cgi(void);
extern int b64_decode( const char* str, unsigned char* space, int size );
extern char *b64_encode( unsigned char *src ,int src_len, unsigned char* space, int space_len);

/* Regular file handler */
extern void do_file(char *path, FILE *stream);

/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
extern void do_auth_pic(char *path, FILE *stream);
#endif
/* ------------------------------------- */
extern int GMTtoEPOCH(char *year, char *month, char *day, char *time);
extern char *url_decode(char *str);
#ifdef UBICOM_JFFS2
#define WLAN0_ETH "ath0"
#define WLAN1_ETH "ath1"
#else
#define WLAN0_ETH nvram_safe_get("wlan0_eth")
#define WLAN1_ETH nvram_safe_get("wlan1_eth")
#endif

#ifdef HAVE_HTTPS
#include <openssl/ssl.h>
extern BIO *bio_err;
extern int openssl_request;
#endif

/* GoAhead 2.1 compatibility */
typedef FILE * webs_t;
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
extern char *wfgets (char *buf, int len, FILE * fp);
extern int wfprintf (FILE * fp, char *fmt, ...);
extern int wfclose (FILE * fp);
extern int wfflush (FILE * fp);
extern int wfputc (char c, FILE * fp);
extern int wfputs (char *buf, FILE * fp);

typedef char char_t;
#define T(s) (s)
#define __TMPVAR(x) tmpvar ## x
#define _TMPVAR(x) __TMPVAR(x)
#define TMPVAR _TMPVAR(__LINE__)
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
#define websWrite(wp, fmt, args...) ({ int TMPVAR = wfprintf(wp, fmt, ## args); wfflush(wp); TMPVAR; })
#define websError(wp, code, msg, args...) wfprintf(wp, msg, ## args)
#define websHeader(wp) wfputs("<html lang=\"en\">", wp)
#define websFooter(wp) wfputs("</html>", wp)
#define websDone(wp, code) wfflush(wp)
#define websGetVar(wp, var, default) (get_cgi(var) ? : default)
#define websSetVar(wp, var, value) set_cgi(var, value)
#define websDefaultHandler(wp, urlPrefix, webDir, arg, url, path, query) ({ do_ej(path, wp); wfflush(wp); 1; })
#define websWriteData(wp, buf, nChars) ({ int TMPVAR = wfwrite(buf, 1, nChars, wp); wfflush(wp); TMPVAR; })
#define websWriteDataNonBlock websWriteData
#ifdef BCMDBG
#define a_assert(a) assert((a))
#else
#define a_assert(a)
#endif

extern int ejArgs(int argc, char_t **argv, char_t *fmt, ...);

/* GoAhead 2.1 Embedded JavaScript compatibility */
extern void do_ej(char *path, FILE *stream);

struct ej_handler {
	char *pattern;
	int (*output)(int eid, webs_t wp, int argc, char_t **argv);
};
extern struct ej_handler ej_handlers[];

struct hsearch_data
  {
    struct _ENTRY *table;
    unsigned int size;
    unsigned int filled;
  };


#define WEBS_BUF_SIZE 5000	
#define websBufferInit(wp) {webs_buf = malloc(WEBS_BUF_SIZE); webs_buf_offset = 0;}
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
#define websBufferFlush(wp) {webs_buf[webs_buf_offset] = '\0'; wfprintf(wp, webs_buf); wfflush(wp); free(webs_buf); webs_buf = NULL;}
#define ARRAYSIZE(a)		(sizeof(a)/sizeof(a[0]))


struct lease_t {
	unsigned char chaddr[16];
	u_int32_t yiaddr;
	u_int32_t expires;
	char hostname[64];
};

/* Copy each token in wordlist delimited by space into word */
/*#define foreach(word, wordlist, next) \
	for (next = &wordlist[strspn(wordlist, " ")], \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '); \
	     strlen(word); \
	     next = next ? &next[strspn(next, " ")] : "", \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '))
  */
#endif /* _httpd_h_ */
