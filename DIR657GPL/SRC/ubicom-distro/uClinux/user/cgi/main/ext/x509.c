#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "log.h"
#include "mime.h"
#include "libdb.h"
#include <errno.h>
#ifdef SYSLOG
#undef SYSLOG
#endif
#define SYSLOG(fmt, ...) syslog(LOG_INFO, fmt, ## __VA_ARGS__)
#define XDBG(fmt, ...)	do {} while(0)
//#define XDBG(fmt, ...) cprintf(fmt, ## __VA_ARGS__)
#define X509_KEY_MAX	24

static char X509_ACTION[256];

static inline int up2db(const char *key, const char *value)
{
	int rev;
	rev = nvram_set(key, value);
	XDBG("UP2DB:%s, %s, REV:%d, %s\n", key, value, rev, strerror(errno));

	return rev;
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

static int stream2buf(FILE *stream, char *buf, int buflen)
{
	int i = 0, c;
	
	buflen--;
	while (i < buflen && (c = fgetc(stream)) != EOF) {
		buf[i++] = c;
	}
	//XDBG("BUFLEN:%d, NOW:%d, c:%X\n",buflen, i, c);
	
	/* 
	 * assume file is stream device,
	 * neither FIFO nor PIPE, socket ...
	 * */
	rewind(stream);
	buf[i] = '\0';
	return i;
}


static int do_x509_action(struct mime_desc *dc)
{
	if (strcmp("x509_action", dc->name) == 0) {
		stream2buf(dc->fd, X509_ACTION, sizeof(X509_ACTION));
		chop(X509_ACTION);
		goto out;
	}
out:
	return 0;
}
/* Define the CA file type identifier which upload from HTML */
#define ID_PRIVATE	"pki_priv_file"
#define ID_HOSTCERT	"pki_pub_file"
#define ID_PEERCERT	"cert_file"
#define ID_CA		"ca_file"

struct pem_desc {
	char *type;
	char buf[2048];
	int flag;	/* 0: Uninitial(No data), 1: already Initicated*/
	int bufsize;
	int (*check_fmt)(struct mime_desc *);
} PEM[] = {
	{ ID_PRIVATE, "", 0, 0, NULL},
	{ ID_HOSTCERT, "", 0, 0, NULL},
	{ ID_PEERCERT, "", 0, 0, NULL},
	{ ID_CA, "", 0, 0, NULL},
	{ NULL, "", 0, 0, NULL}
};

#ifndef LINUX_NVRAM
/* For DIR-X30 */
#define CMD_PLUTO	"/usr/local/lib/ipsec/pluto"
#define CMD_READ_CERT	"/usr/local/lib/ipsec/pluto --certinfo "
#define CMD_READ_RSA	"/usr/local/lib/ipsec/pluto --rsainfo "
#else
/* For DIR-730 */
#define CMD_PLUTO	"/sbin/cert_verify"
#define CMD_READ_CERT	"cert_verify "
#define CMD_READ_RSA	"openssl rsa -noout -in "
#endif
static int dump_buffer(const char *file, const char *buffer, int len)
{
	FILE *fd;
	int rev = -1;

	if ((fd = fopen(file, "w")) == NULL)
		goto err1;
	
	if (fwrite(buffer, len, 1, fd) == len) {
		LOGERR("Certificate: write Error\n");
		goto err2;
	}
	rev = 0;
err2:
	fclose(fd);
err1:
	return rev;
}

static int valid_rsa(const char *string, int len)
{
	char cmd[256], *tmp = "/tmp/RSA.key";
	int rev = -1, r;

	if (dump_buffer(tmp, string, len) == -1)
		goto out;
	sprintf(cmd, "%s%s 2>/dev/null", CMD_READ_RSA, tmp);
	/*
	 * In httpd, signal ignore SIGCHLD, we MUST set as default for caught
	 * child processes. if not, waitpid() will always return -1 which
	 * errno indicated "No Child Process"
	 * */
	signal(SIGCHLD, SIG_DFL);
	//XDBG("%s:%d:%s\n", __FUNCTION__, __LINE__, cmd);
	if ((r = system(cmd)) == -1)
		goto out;
	if (WIFSIGNALED(r))
		goto out;
	if (WEXITSTATUS(r) == 0)
		rev = 0;	/* successful */
	signal(SIGCHLD, SIG_IGN);
out:
	return rev;
}
/* 
 * Check CA file format, then parse CA informat to @info
 * Input:
 * 	@string: A buffer has read already from a CA.
 * 	@len: length of @string.
 * Output:
 * 	@info: Parserd from @string if CA was valid.
 * 	return: -1 on error or invalid format, 0 on valid format
 * */
static int valid_cert(const char *string, int len, char *info)
{
	char buf[512] = CMD_READ_CERT, *tmpf = "/tmp/XXX.pem";
	int rev = -1;
	FILE *pfd;

	/* define keywords that read from pipe,
	 * refer to pluto/main.c on openswan form detail.
	 * 
	 * The result of parser to nvram should look like as below.
	 * x509_1="D-Link Demo,PKI;"
	 * "Issuer$ C=TW, ST=TP, L=Taipei, O=D-Link, OU=VPN Router, CN=DIR Model, E=SampleCA@dlink.com.tw;"
	 * "Subject$ C=TW, ST=TP, L=Taipei, O=D-Link, OU=VPN Router, CN=DIR Model, E=SampleCA@dlink.com.tw;"
	 * "Validity$ Mar 21 18:21:16 2008-Mar 19 18:21:16 2018"
	 * */
	char *key[] = {		
		"Issuer",
		"Subject",
		"Validity",
		NULL
	}, **p;
	
	if (dump_buffer(tmpf, string, len) == -1)
		goto out;

	/* Open pipe to check CA format from tmporary file */
	strcat(buf, tmpf);
	strcat(buf, " 2>/dev/null");
	XDBG("Validity:%s By popen(%s)\n", tmpf, buf);
	if (access(CMD_PLUTO, X_OK) == -1) {
		LOGERR("FILE %s not executable\n", CMD_PLUTO);
		goto out;
	}
	/* 
	 * I discovered that whether the file of @buf is exist or not.
	 * The popen(3) always return NOT NULL.
	 * */
	if ((pfd = popen(buf, "r")) == NULL)
		goto out;
	info[0] = '\0';

	/* Read from pipe, to fetch informations of CA to @info */
	while (fgets(buf, sizeof(buf), pfd) != NULL) {
		//XDBG("CERT INFO:%s\n", buf);
		for (p = key; *p != NULL; p++) {
			if (strncmp(buf, *p, strlen(*p)) == 0) {
				strcat(info, ";");
				strcat(info, chop(buf));
				rev = 0;
			}
		}
	}

	pclose(pfd);
out:
	return rev;
}


static int upload_pem(struct pem_desc *pem, char *s)
{
	struct pem_desc *pki_priv = NULL, *pki_pub = NULL;
	char *act, *key, *k, *h, *value, *type, cakey[] = "x509_ca_XX", buf[1024];
	int rev = -1;

	/* 
	 * @s = X509_ACTION
	 * x509_action: <ACTION>,<KEY>,<VALUE>
	 * ACTION: add|delete|edit
	 * KEY: x509_XX
	 * VALUE: NANE,<TYPE>
	 * TYPE: PKI|CA_ROOT|CERT
	 * EX:
	 * 	"add,x509_3,mycert_pki,PKI"
	 * 	"delete,x509_2,"
	 * 	"edit,x509,Name,CERT
	 * */
#define ACT_DELETE 	"delete"
#define ACT_EDIT 	"edit"
	k = strdup(s);
	h = k;
	act = strsep(&k, ",");
	key = strsep(&k, ",");
	
	XDBG("ACT:%s, KEY:%s\n", act, key);
	if (!(act && key))
		goto err;

	/* x509_XX to x509_ca_XX */
	sprintf(cakey, "x509_ca_%d", atoi(strchr(key, '_') + 1));

	/* If action was delete, just push "" into nvram */
	if (strcmp(act, ACT_DELETE) == 0) {
		up2db(key, "");
		up2db(cakey, "");
		rev = 0;
		goto ret;
	}
	value = k;
	if ((type = strchr(k, ',')) == NULL)
		goto err;
	type ++;
	if (strcmp(act, ACT_EDIT) == 0)
	{
		up2db(key, value);
		rev = 0;
		goto ret;	
	}

	XDBG("Name:%s, Type:%s\n", k, type);
	value = strcpy(buf, value);
	value += strlen(buf);
	XDBG("Append cert ID to :%s\n", buf);
	for (;pem->type != NULL; pem++) {
		if (pem->flag == 0)
			continue;
		XDBG("PEM:%s, S:%d, F:%d\n", pem->type, pem->bufsize, pem->flag);
		if (strcmp(pem->type, ID_PRIVATE) == 0) {
			pki_priv = pem;
			continue;
		}
		if (strcmp(pem->type, ID_HOSTCERT) == 0) {
			pki_pub = pem;
			continue;
		}

		if (valid_cert(pem->buf, pem->bufsize, value) == -1) {
			XDBG("invalid cert format\n");
			goto err;
		}
		up2db(cakey, pem->buf);
	}
	if (pki_priv && pki_pub) {
		char tmp[sizeof(pem->buf) * 2];

		if (valid_cert(pki_pub->buf, pki_pub->bufsize, value) == -1) {
			XDBG("Upload invalid Host-Certification\n");
			goto err;
		}
		if (valid_rsa(pki_priv->buf, pki_priv->bufsize) == -1) {
			XDBG("Upload Invlid Private key file\n");
			goto err;
		}

		
		/* Update PKI form 2 pem files */
		sprintf(tmp, "%s,%s", pki_priv->buf, pki_pub->buf);
		up2db(cakey, tmp);
	}
	up2db(key, buf);
ret:
	nvram_commit();
	kill(1, SIGHUP);
	rev = 0;
err:
	free(h);
	return rev;
}
static int PEMERR = 0;
/*
 * We did not to check the CA file format yet, until parser finished,
 * then check CA from @PEM[]
 * */
static int do_x509ca_files(struct mime_desc *dc)
{
	struct pem_desc *k;

	/*
	 * Setup @PEM from @dc, which @PEM[] describes a CA file
	 * */
	for (k = PEM; k->type != NULL; k++) {
		if (strcmp(k->type, dc->name) == 0) {
			XDBG("Fetch:%s:%s\n", k->type, dc->name);
			/* IN PKI, there are 2 files to upload... */
			if (dc->size > sizeof(k->buf)) {
				PEMERR = 1;
				return 0;
			}
			k->bufsize = stream2buf(dc->fd, k->buf, sizeof(k->buf));
			//XDBG("UPLOAD:%d, should be:%d\n", strlen(k->buf), dc->size);
			k->flag = 1;
		}
	}
	return 0;
}

int do_putquerystring_env(struct mime_desc *dc)
{
	char *querystring[] = {
		"html_response_page",
		"html_response_message",
		"html_response_error_message",
		"html_response_return_page",
		NULL
	}, **q;
	char buf[512];
	
	/* Insert the value of key within @querystring to environment */
	for (q = querystring; *q != NULL; q++) {
		if (strncmp(*q, dc->name, strlen(*q)) != 0)
			continue;

		stream2buf(dc->fd, buf, sizeof(buf));
		setenv(*q, buf, 1);
	}
	return 0;
}

char *upload_ca()
{
	int rev = -1;
	char *rev_page = "error.asp";

	register_mime_handler(do_x509_action);
	register_mime_handler(do_x509ca_files);
	register_mime_handler(do_putquerystring_env);
	http_post_upload(stdin);
	//XDBG("UPLOAD...[%s]\n", X509_ACTION);
	if (PEMERR == 0)
		rev = upload_pem(PEM, X509_ACTION);
	if (rev == 0)
		rev_page = getenv("html_response_page");
	//XDBG("X509 CHECK POPEN INFO CAT? BACK:%s\n", rev_page);
	return rev_page;
}
