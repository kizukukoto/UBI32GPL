#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <error.h>
#include "mime.h"
#include "debug.h"

//#define SYSLOG(fmt, a...) syslog(LOG_INFO, fmt, ##a)
#define SYSLOG(fmt, a...) do { } while(0)

/*
 * Helpers
 * */

#define SKIP_SPACE(p) while (isspace(*(p)) && *(p) != '\0'){(p)++;}
#define JUMP_SPACE(p) while (!isspace(*(p)) && *(p) != '\0'){(p)++;}
static int split(char *buf, const char *delim, char *tok[], int strip)
{
	int i = 0;
	char *p;
	
	while ((p = strsep(&buf, delim)) != NULL) {
		if (strip && (strchr(delim, *p) != NULL)) {
			continue;
		}
		tok[i++] = p;
	}
	if (buf)
		tok[i++] = buf;
	return i;
}

static char *chop(char *io)
{
	char *p;

	p = io + strlen(io) - 1;
	while ( *p == '\n' || *p == '\r') {
		*p = '\0';
		p--;
	}
	return io;
}

void dump_mime_desc(FILE *stream, struct mime_desc *dc)
{
	fprintf(stream, "name: %s\n", dc->name);
	fprintf(stream, "filename: %s\n", dc->filename);
	fprintf(stream, "nime_type: %s\n", dc->mime_type);
}

static void strip_char(const char *s, const char *c, char *out)
{
	for (;*s != '\0'; s++) {
		if (strchr(c, *s) == NULL)
			*out++ = *s;
	}
	*out = '\0';
	return;
}

/* 
 * MIME Handler routines
 * */

typedef int (*mime_handler_t)(struct mime_desc *);
#define MIME_HANDLER_MAX 128
static mime_handler_t MIME_HANDLER[MIME_HANDLER_MAX];

static inline void destroy_mime_dsec(struct mime_desc *dc)
{
	fclose(dc->fd);
}

static int call_mime_handlers(struct mime_desc *dc)
{
	int i;

	for (i = 0; i < MIME_HANDLER_MAX && MIME_HANDLER[i] != 0; i++) {
		fseek(dc->fd, 0L, SEEK_SET);
		MIME_HANDLER[i](dc);
	}
	destroy_mime_dsec(dc);
	return 0;
}

int register_mime_handler(int (*fn)(struct mime_desc *))
{
	int i, rev = -1;
	
	for (i = 0; i < MIME_HANDLER_MAX && MIME_HANDLER[i] != 0; i++);
	
	if (i == MIME_HANDLER_MAX)
		goto out;
	MIME_HANDLER[i] =  fn;
	rev = 0;
out:
	return rev;
}

/*
 * Initial struct mime_desc from @stream.
 * then call each of registed routines.
 * routines register by @register_mime_handler.
 */

static int read_token(FILE *stream, char *delim, char *out, int out_max)
{
	int i;

	for (i = 0; i < out_max; i++) {
		out[i] = fgetc(stream);
		if (strchr(delim, out[i]) != NULL)
			goto ok;
	}
	return -1;
ok:
	out[i + 1] = '\0';
	return 0;
}


static void parser_mime_content(FILE *stream, const char *boundary, struct mime_desc *dc);
static void destroy_desc(FILE *stream, const char *boundary, struct mime_desc *dc);

/*
 * Parser: Content-Disposition: form-data; name="priv_file"; filename="x86cert.pem"
 */
static void __full_content_disp(const char *value, struct mime_desc *dc)
{
	char *sp[8], *s, *strip;
	int i, t;
	struct mime_keys {
		char *key;
		char *value;
	} key[] = {
		{ "name", dc->name},
		{ "filename", dc->filename},
		{ NULL, NULL}
	}, *p;

	s = strdup(value);
	t = split(s, ";", sp, 1);

	for (p = key; p->key != NULL; p++) {
		for (i = 0; i < t; i++) {
			strip = sp[i];
			SKIP_SPACE(strip);
			if (strncasecmp(strip, p->key, strlen(p->key)) == 0) {
				char buf[256];

				strip = strchr(strip, '=');
				strip++;
				SKIP_SPACE(strip);
				strip_char(strip, "\"", buf);
				strcpy(p->value, buf);
			}

		}
	}
	free(s);
}

/*
 * Parser MIME head, full to @dc
 * 
 * Ex:
 * Content-Disposition: form-data; name="priv_file"; filename="x86cert.pem"
 * Content-Type: application/octet-stream
 *
 * */
static void parser_mime_head(FILE *stream, const char *boundary, struct mime_desc *dc)
{
	char *type = "Content-Type", *dispos = "Content-Disposition", *s;
	int type_len = 12, dispos_len = 18;
	char head_buf[256], value_buf[256];

	
	/* For each MIME header entry line */
	while (read_token(stream, ":\r\n", head_buf, sizeof(head_buf)) == 0) {
		
		/* XXX: We just check ':' to verify that end of MIME header. */
		if (head_buf[strlen(head_buf) -1] != ':')
			goto start_content;
		
		/* Get value of MIME header. */
		fgets(value_buf, sizeof(value_buf), stream);
		chop(value_buf);
		s = value_buf;
		SKIP_SPACE(s);

		if (strncmp(head_buf, type, type_len) == 0) {
			strncpy(dc->mime_type, s, sizeof(dc->mime_type));
		} else if (strncmp(head_buf, dispos, dispos_len) == 0) {
			__full_content_disp(s, dc);
		}
	}
	dc->fn = destroy_desc;
	return;

start_content:
	/* get newline which end of mime head */
	fgets(value_buf, sizeof(value_buf), stream);
	dc->fn = parser_mime_content;
	return;
}
void alm(int signo)
{
	SYSLOG("STD I/O blocking!");
	SYSLOG("CLOSING STDIN!");
	fclose(stdin);
	return;
}

static int do_mime_option(struct mime_desc *dc, FILE **fd)
{
	int rev = -1;

	if (dc->opt == NULL ||
	    dc->opt->name == NULL ||
	    strcmp(dc->name, dc->opt->name) != 0)
		*fd = tmpfile();
	else {
		sprintf(dc->tpath, "/tmp/%sXXXXXX", dc->name);

		if (mkstemp(dc->tpath) == -1)
			goto out;
		if ((*fd = fopen(dc->tpath, "wb+")) == NULL)
			goto out;
	}

	dc->fd = *fd;
#if 0
	if (dc->opt->name == NULL || dc->opt->desc == NULL)
		goto out;
	if (strcmp(dc->name, dc->opt->name) != 0)
		goto out;

	sprintf(dc->tpath, "/tmp/%sXXXXXX", dc->name);
	if (mkstemp(dc->tpath) == -1)
		goto out;
	if ((dc->fd = fopen(dc->tpath, "wb+")) == NULL)
		goto out;
#endif
	rev = 0;
out:
	return rev;
}

static void parser_mime_content(FILE *stream, const char *boundary, struct mime_desc *dc)
{
	char c, *buf;
	FILE *fd = NULL;
	int len, i = 0;

	do_mime_option(dc, &fd);

	buf = malloc(strlen(boundary) + 5);
	SYSLOG("called %s(%s)\n", __FUNCTION__, boundary);
	/*
	 * I suppose client send "\r\n--@boundary", CRLF is standard format.
	 * But I did not know that all of clients will follow CRLF or not.
	 * 
	 * FIXME:
	 * 	"seomething text\r\n--@boundary"
	 * 	were taken as end of MIME body.
	 * 	but in MIME standard. it should be as below.
	 * 	"seomething text\r\n\r\n--@boundary" 
	 * */
	sprintf(buf, "\r\n--%s", boundary);
	boundary = buf;
	
	len = strlen(boundary);
	buf = malloc(len + 1);
	bzero(buf, len + 1);
	//signal(SIGALRM, alm);
	//alarm(5);	
	while (!feof(stream)) {
		c = fgetc(stream);

		if (c == boundary[i]) {
			//SYSLOG("PARSER %d:%d:[%s%c]\n", i + 1, len, buf, c);
			if ((i + 1 )== len) {
				char tmp[128];

				fgets(tmp, sizeof(tmp), stream);
				
				/* XXX: How about: "--@boundary--bad_fmt" ? */
				dc->fn = strncmp(tmp, "--", 2) == 0 ?
				       destroy_desc : parser_mime_head;
				SYSLOG("N:[%s]%c\n[%s]\nfn:%s\n", buf, c, tmp,
					strncmp(tmp, "--", 2) == 0 ?
				       "destroy_desc" : "parser_mime_head");
				goto next;
			}
			buf[i++] = c;
		} else {
			if (i > 0) {
				SYSLOG("RESET BIN:[%s]\n", buf);
				fwrite(buf, i, 1, fd);
				i = 0;
			}
			fputc(c, fd);
		}
	}
	//alarm(0);
	SYSLOG("BAD EOF!");
	dc->fn = destroy_desc;
next:
	dc->size = ftell(fd);
	SYSLOG("MIME SIZE:%d\n", dc->size);
	fseek(fd, 0L, SEEK_SET);
	/* XXX: It is better that call_mime_handler@ should be call on caller */
	call_mime_handlers(dc);

	return;
	
}

static void destroy_desc(FILE *stream, const char *boundary, struct mime_desc *dc)
{
	int fg, fd;
	
	if ((fd = fileno(stream)) == -1)
		SYSLOG("fileno %s\n", strerror(errno));
	if ((fg = fcntl(fd, F_GETFL)) == -1)
		SYSLOG("F_GETFL %s\n", strerror(errno));
	fg |= O_NONBLOCK;
	if ((fg = fcntl(fd, F_SETFL, fg)) == -1)
		SYSLOG("F_SETFL %s\n", strerror(errno));
	SYSLOG("NOW destory_desc\n");
	do { } while (fgetc(stream) != EOF);

	SYSLOG("FIN  destory_desc\n");
	dc->fn = NULL;
	return;
}

static void search_boundary(FILE *stream, const char *boundary, struct mime_desc *dc)
{
	char buf[512];
	int len;
	len = strlen(boundary);
	
	while (!feof(stream)) {
		fgets(buf, sizeof(buf), stream);
		chop(buf);
		SYSLOG("[%s:%s]", buf, boundary);
		
		if (buf[0] =='-' && buf[1] == '-' &&
			strncmp(buf + 2, boundary, len) == 0)
		{
			dc->fn = parser_mime_head;
			return;
		}
	}
	dc->fn = destroy_desc;
	return;
}

static int each_boundary(FILE *stream, const char *boundary, struct mime_opt *opt)
{
	struct mime_desc mdec, *dc;

	bzero(&mdec, sizeof(mdec));
	dc = &mdec;
	dc->fn = search_boundary;
	dc->opt = opt;
	while (dc->fn != NULL)
		dc->fn(stream, boundary, dc);
	return 0;
}

static inline char *get_boundary_from_env()
{
	char *p;

	if ((p = getenv("CONTENT_TYPE")) == NULL)
		goto out;
	/*
	 * FIXME:
	 * Format captured from IE, Mozilla by ethereal utility.
	 * ex: "Content-Type: multipart/form-data; boundary=-----------string"
	 * But not sure whether another format syntax appear or not?
	 * */
	if ((p = strstr(p, "boundary=")) == NULL)
		goto out;
	
	p += 9; /* strlen("boundary=") */
out:
	return p;
}

static char *__http_post_upload(FILE *stream)
{
	int total;
	char *p, *boundary = NULL;

	if ((p = getenv("CONTENT_LENGTH")) == NULL)
		goto out;
	total = atoi(p);
	boundary = get_boundary_from_env();
out:
	return boundary;
}

int http_post_upload(FILE *stream)
{
#if 0
	char *p, *boundary;
	int total, rev = -1;

	if ((p = getenv("CONTENT_LENGTH")) == NULL)
		goto out;
	total = atoi(p);
	if ((boundary = get_boundary_from_env()) == NULL)
		goto out;
#endif
	int rev = -1;
	char *boundary = __http_post_upload(stream);

	if (boundary == NULL)
		goto out;
	each_boundary(stream, boundary, NULL);
	rev = 0;
	//SYSLOG("fgets tatol:%d, %s\n", total, strerror(errno));
out:
	return rev;
}

int http_post_upload_ext(FILE *stream, struct mime_opt *opt)
{
	int rev = -1;
	char *boundary = __http_post_upload(stream);

	if (boundary == NULL)
		goto out;
	each_boundary(stream, boundary, opt);
	rev = 0;
out:
	return rev;
}
#if 0
/* Simple code of using MIME_HANDLER */
int handler_fn(struct mime_desc *dc)
{
	char tmp[512];

	//dump_mime_desc(stdout, dc);
	while (fgets(tmp, sizeof(tmp), dc->fd)) {
		printf("%s", tmp);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	setenv("CONTENT_LENGTH", "10000", 1);
	setenv("CONTENT_TYPE", "Content-Type: multipart/form-data; boundary=----1234", 1);
	register_mime_handler(handler_fn);
	http_post_upload(stdin);
	return 0;
}
#endif
