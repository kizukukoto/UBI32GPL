#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "vpn.h"

#define CA_ROOT_DIR "/tmp/ipsec.d"
#define CACERTS "cacerts"
#define PRIVATE "private"
#define CERTS	"certs"

static inline int x509_index(const char *k)
{
	return atoi(k + strlen("x509_"));
}

static void create_ca_dir()
{
	char *ca_dir[] = {
		CACERTS,
		CERTS,
		PRIVATE,
		NULL
	}, **p, cmd[64];
	
	sprintf(cmd, "rm -rf %s", CA_ROOT_DIR);
	system(cmd);
	for (p = ca_dir; *p != NULL; p++) {
		sprintf(cmd, "mkdir -p %s/%s", CA_ROOT_DIR, *p);
		system(cmd);
	}
}

static int ca_write(const char *subdir, const char *name, const char *value)
{
	char fpath[256];
	int fd;

	DEBUG("Write CA: %s/%s/%s\n", CA_ROOT_DIR, subdir, name);
	sprintf(fpath, "%s/%s/%s", CA_ROOT_DIR, subdir, name);
	if ((fd = open(fpath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR)) == -1)
		return -1;
	
	write(fd, value, strlen(value));
	close(fd);
	return 0;
}

/*
 * Dump CA files from DB to filesystem.
 * */
static int ca_create() {
	char key[24], fn[24], *v, *type;
	int i, rev = -1;
/* CA_TYPE_XXX defined from UI */	
#define CA_TYPE_CERT	"CERT"
#define CA_TYPE_PKI	"PKI"
#define CA_TYPE_CA	"CA_ROOT"
#define MAX_CA		24
	DEBUG("CA CREATE\n");
	for (i = 1; i <= MAX_CA; i++) {
		sprintf(fn, "x509_%d", i);
		if ((v = nvram_safe_get(fn)) == "")
			continue;

		if ((type = strchr(v, ',')) == NULL)
			continue;	/* UI|DB error. */
		type++;
		sprintf(key, "x509_ca_%d", i);
		if ((v = nvram_safe_get(key)) == "")
			continue;
		//DEBUG("PARSER KEY: %s, Type:%s\n", key, type);
		/*
		 * type = "<TYPE>;<[CA_INFO]; ...>"
		 * ex: "PKI;Subject: C=TW, O=CAMEO"
		 *	^^^
		 * We just compare with <TYPE>.
		 * */
		if (strncmp(CA_TYPE_CERT, type, strlen(CA_TYPE_CERT)) == 0) {
			ca_write(CERTS, fn, v);
		} else if (strncmp(CA_TYPE_CA, type, strlen(CA_TYPE_CA)) == 0) {
			ca_write(CACERTS, fn, v);
		} else if (strncmp(CA_TYPE_PKI, type, strlen(CA_TYPE_PKI)) == 0) {
			char *r, *k, *z;
			/*
			 * In CGI x509.c, I ordered by "@CERT,@PRIV_KEY"
			 * */
			if ((z = strdup(v)) == NULL)
				goto out;
			r = z;
			k = strsep(&r, ",");
			ca_write(CERTS, fn, r);
			ca_write(PRIVATE, fn, k);
			free(z);
		}
	}
	rev = 0;
out:
	return rev;
}

/*
 * Format/initial the part of x509, which including in struct vpn_rule.
 * 
 * Format x509 AUTH: <[CA_TYPE=x509_XX], ...>;<[CA_INFO:CA_VALUE], ...>
 * We don't care ca, pluto upload them automatically.
 * ex: @buf = "pki=x509_1,cert=x509_2;Subject: C=TW, O=CAMEO;...."
 * */
int vpn_auth_format_x509(struct vpn_rule *vr, char *buf)
{
	struct vpn_auth *ah = &vr->auth;
	char *st[10];
	int i, t;
	struct {
		char *key;
		char **value;
	} keys[] = {
		{ "pki=", &ah->extra.x509.pki},
		{ "cert=", &ah->extra.x509.cert},
		{ "ca=", &ah->extra.x509.ca},
		{ NULL, NULL}
	}, *k;

	t = split(buf, ",", st, 1);
	for (k = keys; k->key != NULL; k++) {
		*k->value = NULL;
		for(i = 0; i < t; i ++) {
			//printf("k->key %s:%s\n", k->key, st[i]);
			if (strncmp(k->key, st[i], strlen(k->key)) == 0) {
				//printf("HIT\n");
				*k->value = st[i] + strlen(k->key);
			}
		}
	}
	return 0;
}
/* Fetch subject of X509 ca, which recorded in x509_XX of DB */
static int cert_subject(const char *key, char *out)
{
	char *v, *st[10];
	int i, idx, rev = -1;
	
	if ((v = nvram_safe_get(key)) == "")
		goto out;
	
	v = strdup(v);
	idx = split(v, ";", st, 1);
	for (i = 1; i < idx; i ++) {
		if (strncasecmp(st[i], "Subject", strlen("Subject")) != 0)
			continue;
		strcpy(out, st[i] + strlen("Subject") + 1);
		rev = 0;
		break;
	}
	free(v);
out:
	return rev;
}

int conf_vpn_auth_x509(struct vpn_rule *vr, FILE *fd)
{
	struct vpn_auth *ah = &vr->auth;
	char buf[256], *pki, *cert, *cacert;

	pki = ah->extra.x509.pki;
	cert = ah->extra.x509.cert;
	cacert = ah->extra.x509.ca;

	fprintf(fd, "\tauthby=rsasig\n");
	DEBUG("X509:PKI=%s|CERT=%s|\n", pki, cert);
	if (pki) {
		sprintf(buf, "%s/%s/%s", CA_ROOT_DIR, CERTS, pki);
		fprintf(fd, "\tleftcert=%s\n", buf);
		if (cert_subject(pki, buf) == 0) {
			fprintf(fd, "\tleftid=\"%s\"\n", buf);
			DEBUG("\tleftid=\"%s\"\n", buf);
		}
	}
	if (cert) {
		sprintf(buf, "%s/%s/%s", CA_ROOT_DIR, CERTS, cert);
		fprintf(fd, "\trightcert=%s\n", buf);
		if (cert_subject(cert, buf) == 0) {
			fprintf(fd, "\trightid=\"%s\"\n", buf);
			DEBUG("\trightid=\"%s\"\n", buf);
		}
	} else
		fputs("\trightrsasigkey=%cert\n", fd);
	create_ca_dir();
	ca_create();
	return 0;
}
