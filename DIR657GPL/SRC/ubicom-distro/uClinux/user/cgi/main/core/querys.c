/*
 * These uitiltys are for Query string coming form browser 
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>

#include "ssi.h"
#include "querys.h"
#include "log.h"
#include "public.h"

#define QUERY_SIZE      512
#define BUF_SIZE	(8192 << 3)
extern struct nvram_ops *nvram;

//extern from apps/linux_nvram/linux_nvram.c
extern char *nvram_get(const char *name);


#if 0
#include <stdarg.h>
void __DEBUG_MSG(const char *fmt, ...){
    va_list args;
    char buf[512];
    FILE *fp;
    if((fp=fopen("/tmp/jlog","a"))==NULL){
        return ;
    }

    va_start(args, fmt);
    vsprintf(buf, fmt,args);
    fprintf(fp,"%s",buf);
    va_end(args);

    fclose(fp);
}
#else
#define __DEBUG_MSG(fmt, arg...)
#endif

#define NIBBLE_VAL(X)	((X) >= 'A' ? (((X) & 0xdf) - 'A') + 10 : (X) - '0')
static char x2c(char *what)
{
	register char digit;
	
	if (what[1] == '\0')
		digit = NIBBLE_VAL(what[0]);
	else
		digit = NIBBLE_VAL(what[0]) * 16 + NIBBLE_VAL(what[1]);
	return(digit);
}

void unescape_url(char *url)
{
	register int i, j;

	for (i = 0, j = 0; url[j]; ++i, ++j) {
		if ((url[i] = url[j]) == '%') {
			url[i] = x2c(&url[j + 1]);
			j += 2;
		}
	}
	url[i] = '\0';
}

void put_querystring_env()
{
	char *buf, *s, *t;
	int i = 0, content_len;

	s = getenv("CONTENT_LENGTH");
	if (s == NULL)
		goto out;
	content_len = atoi(s);
	if (content_len > BUF_SIZE) {
		SYSLOG("large query string: %d byte",
			content_len);

		cprintf("++++++++++Content size %d +++++++++++\n", content_len);
		cprintf("++++++++++Buf size %d +++++++++++\n", BUF_SIZE);
		
		goto out;
	}
	SYSLOG("CONTENT_LENGTH %d<br>", content_len);
	buf = malloc(BUF_SIZE);
	if (buf == NULL)
		goto out;

	// Read from STDIN for getting borwser query string
	do {
		int c;
		if ((c = read(0, buf + i, content_len - i)) == -1)
			break;
		i += c;
	} while (i < content_len);

	// Can not read more data from STDIN
	if (i <= 0)
		goto free_buf;
	SYSLOG("read %d bytes strlen:%d <br>", i, strlen(buf));
	
	buf[i] = '\0';
	// Change all plusses back to spaces.
	for (i = 0; buf[i]; i++)
		if (buf[i] == '+')
			buf[i] = ' ';
	
	s = strtok_r(buf, "&;", &t);
	while (s) {
		unescape_url(s);
		putenv(s);
		SYSLOG(LOG_INFO, "Q: %s", s);
		s = strtok_r(NULL, "&;", &t);
	}
	/* 
	 * XXX: Because use putenv so memory of buf is belong environ, 
	 * can not free it
	 */
out:
	return;

free_buf:
	free(buf);
}
