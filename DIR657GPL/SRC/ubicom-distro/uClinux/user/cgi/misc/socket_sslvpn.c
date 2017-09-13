#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "libdb.h"
#include "ssi.h"

//#define SSL_SOCKET_ENABLED

#ifdef SSL_SOCKET_ENABLED
#include <signal.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#define CERTFILE	"/tmp/ssl.pem"
#endif

#define SERVER_PORT		20000
#define LENGTH_OF_LISTEN_QUEUE	10 
#define BUFFER_SIZE		255
#define WELCOME_MESSAGE		"DIR-x30 SSLVPN server"

#if 0
#define DEBUG_MSG cprintf
#else
#define DEBUG_MSG
#endif

static int clifd, servfd;

static void socket_sslvpn_terminate(int signal_no)
{
//	shutdown(servfd, SHUT_WR);
//	shutdown(servfd, SHUT_RD);
	close(servfd);
}

/* @cli_req := "USERNAME/xxx,PASSWORD/xxx" */
static int socket_sslvpn_splitreq(const char *cli_req, char *usr, char *pwd)
{
	int ret = -1;
	char *s, *p, req[128];

	p = req;
	strcpy(req, cli_req);

	if ((s = strsep(&p, ",")) == NULL ||
		strstr(s, "USERNAME/") == NULL ||
		strlen(s) <= 9)
		goto out;

	if (p == NULL || strstr(p, "PASSWORD/") == NULL || strlen(p) <= 9)
		goto out;

	strcpy(usr, strstr(s, "/") + 1);
	strcpy(pwd, strstr(p, "/") + 1);

	if (*usr == '\0' || *pwd == '\0')
		goto out;

	ret = 0;
out:
	return ret;
}

/* @g_text := "Group1/sslvpn,pass/"
 * @usr := "sslvpn"
 * @pwd := "passwd"
 * */
static int __socket_sslvpn_auth(
	const char *g_text,
	const char *usr,
	const char *pwd)
{
	int ret = 0;
	char *s, *p, t[512];

//	DEBUG_MSG("XXX __ssa.g_text [%s] usr [%s] pwd [%s]\n",
//		g_text, usr, pwd);

	if (*g_text == '\0')
		goto out;

	p = t;
	strcpy(t, g_text);

	strsep(&p, "/");	/* drop 'Group1' */
	while ((s = strsep(&p, "/")) != NULL) {
		char gusr[128], gpwd[128];

		sscanf(s, "%[^,],%s", gusr, gpwd);
		if (strcmp(usr, gusr) == 0 && strcmp(pwd, gpwd) == 0)
			goto out;
	}

	ret = -1;
out:
	return ret;
}

/* @g := "auth_group1:"
 * @usr := "sslvpn"
 * @pwd := "passwd"
 * */
static int socket_sslvpn_auth(
	char *g,
	const char *usr,
	const char *pwd)
{
	int ret = -1;

	if (g == NULL || usr == NULL || pwd == NULL)
		goto out;

	if (g[strlen(g) - 1] == ':')
		g[strlen(g) - 1] = '\0';

	ret = __socket_sslvpn_auth(nvram_safe_get(g), usr, pwd);
out:
	return ret;
}

extern void sslvpn_init(
	FILE *out,
	const char *usr,
	const char *subnet,
	const char *masq);

/* @cli_req := "USERNAME/xxx,PASSWORD/xxx" */
static int socket_sslvpn_verify(FILE *out, char *cli_req)
{
	int i, ret = -1;
	char usr[128], pwd[128];

	if (out == NULL)
		goto out;
	if (cli_req == NULL || *cli_req == '\0')
		goto out;

	if (socket_sslvpn_splitreq(cli_req, usr, pwd) == -1)
		goto out;

	for (i = 1; i <= 6; i++) {
		char *masq, *p, *g, sslvpn_key[16], sslvpn_txt[128];

		p = sslvpn_txt;
		sprintf(sslvpn_key, "sslvpn%d", i);
		strcpy(sslvpn_txt, nvram_safe_get(sslvpn_key));

		if (*p == '\0' || *p != '1')
			continue;
		if (strsep(&p, ",") == NULL)
			continue;
		if (strsep(&p, ",") == NULL)
			continue;
		if ((g = strsep(&p, ",")) == NULL)
			continue;
		//if (username_in_auth_groups(g, usr) != 1)
		if (socket_sslvpn_auth(g, usr, pwd) != 0)
			continue;
		if ((g = strsep(&p, ",")) == NULL)
			continue;
		if ((masq = strsep(&p, ",")) == NULL)
			continue;
		sslvpn_init(out, usr, g, masq);
		break;
	}

	fclose(out);
	ret = 0;
out:
	return ret;
}

int socket_sslvpn_main(int argc, char *argv[])
{
	int val = 1, ret = -1;
	struct sockaddr_in servaddr, cliaddr;
#ifdef SSL_SOCKET_ENABLED
        SSL_METHOD *my_method;
        SSL_CTX *mt_ctx;
        SSL *ssl;
	struct sigaction act;

	act.sa_handler = socket_sslvpn_terminate;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (sigaction(SIGINT, &act, NULL) < 0 ||
		sigaction(SIGTERM, &act, NULL) < 0)
	{
		printf("Install sigaction error\n");
		goto out;
	}
#endif
	if ((servfd = socket(AF_INET,SOCK_STREAM, 0)) < 0) {
		printf("create socket error!\n");
		goto fail;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERVER_PORT);
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
#ifdef SSL_SOCKET_ENABLED
	if (setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR,
		(char *)&val, sizeof(val)) == -1)
	{
		perror("setsockopt(SO_REUSEADDR)");
		goto fail;
	}
#endif
	if (bind(servfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		printf("bind to port %d failure!\n", SERVER_PORT);
		goto out;
	}

	if (listen(servfd, LENGTH_OF_LISTEN_QUEUE) < 0) {
		printf("call listen failure!\n");
		goto out;
	}

	/* server loop will nerver exit unless any body kill the process */
	while (1) {
		int ssl_ret;
		char buf[BUFFER_SIZE];
		socklen_t length = sizeof(cliaddr);
#ifdef SSL_SOCKET_ENABLED
		SSL_library_init();
		SSL_load_error_strings();

		my_method = SSLv2_method();
		mt_ctx = SSL_CTX_new(my_method);

		SSL_CTX_use_certificate_file(mt_ctx, CERTFILE, SSL_FILETYPE_PEM);
		SSL_CTX_use_PrivateKey_file(mt_ctx, CERTFILE, SSL_FILETYPE_PEM);
		SSL_CTX_check_private_key(mt_ctx);

		ssl = SSL_new(mt_ctx);
#endif
		clifd = accept(servfd, (struct sockaddr*)&cliaddr, &length);

		if (clifd < 0) {
			printf("error comes when call accept!\n");
			goto fail;
		}
#ifdef SSL_SOCKET_ENABLED
		SSL_set_fd(ssl, clifd);
		if ((ssl_ret = SSL_accept(ssl)) != 1) {
			switch(ssl_ret = SSL_get_error(ssl, ssl_ret)) {
			case SSL_ERROR_SSL:
				printf("SSL protocol error\n");
				break;
			case SSL_ERROR_ZERO_RETURN:
				printf("SSL ZERO RETURN ERROR?\n");
				break;
			case SSL_ERROR_SYSCALL:
				printf("CALL back return\n");
				break;
			default:
				printf("SSL default ERROR: %d\n", ssl_ret);
				break;
			}

			close(clifd);
			SSL_free(ssl);
			continue;
		}
#endif
		strcpy(buf, WELCOME_MESSAGE);
#if 0
		printf("from client, IP: %s, Port: %d\n",
			inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
#endif
		send(clifd, buf, BUFFER_SIZE, 0);
		recv(clifd, buf, BUFFER_SIZE, 0);

		setenv("REMOTE_ADDR", inet_ntoa(cliaddr.sin_addr), 1);
		setenv("REMOTE_USER", inet_ntoa(cliaddr.sin_addr), 1);
		socket_sslvpn_verify(fdopen(clifd, "w"), buf);

		close(clifd);
	}

	ret = 0;
fail:
	close(servfd);
out:
	return ret;

}
