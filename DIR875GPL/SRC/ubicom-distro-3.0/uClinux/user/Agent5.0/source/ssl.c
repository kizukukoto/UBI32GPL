/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file ssl.c
 *
 */
#ifdef __ENABLE_SSL__

#include <string.h>
#include <stdio.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include <openssl/ssl2.h>

#include "log.h"
#include "connection.h"
#include "ssl.h"
#include "war_string.h"

static char password[80] = "";
char ca_cert[256] = "";
static char client_cert[256] = "";
static char client_key[256] = "";

int set_ssl_config(const char *name, const char *value)
{
	if(war_strcasecmp(name, "SSLPassword") == 0) {
		war_snprintf(password, sizeof(password), "%s", value);
	} else if(war_strcasecmp(name, "CACert") == 0) {
		war_snprintf(ca_cert, sizeof(ca_cert), "%s", value);
	} else if(war_strcasecmp(name, "ClientCert") == 0) {
		war_snprintf(client_cert, sizeof(client_cert), "%s", value);
	} else if(war_strcasecmp(name, "ClientKey") == 0) {
		war_snprintf(client_key, sizeof(client_key), "%s", value);
	}

	return 0;
}


static int password_cb(char *buf, int buf_len, int rwflag, void *userdata)
{
	const char *pass;

	pass = (const char *)userdata;

	return strlen(pass) > buf_len ? -1 : war_snprintf(buf, buf_len, "%s", pass);
}

static int verify_cb(int ok, X509_STORE_CTX *store)
{
	if(!ok) {
		X509 *cert;
		int depth;
		int err;
		char data[256];

		cert = X509_STORE_CTX_get_current_cert(store);
		depth = X509_STORE_CTX_get_error_depth(store);
		err = X509_STORE_CTX_get_error(store);

		X509_NAME_oneline(X509_get_issuer_name(cert), data, sizeof(data));
		X509_NAME_oneline(X509_get_subject_name(cert), data, sizeof(data));
	}

	return ok;
}

void destroy_ssl_connection(struct connection *conn)
{
	if(conn->ssl) {
		SSL_set_shutdown(conn->ssl, 2);
		SSL_shutdown(conn->ssl);
		SSL_free(conn->ssl);
	}

	if(conn->ctx)
		SSL_CTX_free(conn->ctx);

	conn->ssl = NULL;
	conn->ctx = NULL;
}

int setup_ssl_connection(struct connection *conn)
{
	conn->ssl = NULL;
	conn->ctx = NULL;

	if(SSL_library_init() != 1) {
		tr_log(LOG_ERROR, "Init ssl library failed!");
		return -1;
	}

	SSL_load_error_strings();

	if((conn->ctx = SSL_CTX_new(SSLv3_method())) == NULL) {
		tr_log(LOG_WARNING, "Create SSLv3 context failed, try TLSv1");

		if((conn->ctx = SSL_CTX_new(TLSv1_method())) == NULL) {
			tr_log(LOG_ERROR, "Create TLSv1 context failed");
			return -1;
		}
	}

	SSL_CTX_set_verify(conn->ctx, SSL_VERIFY_PEER, verify_cb);
	SSL_CTX_set_verify_depth(conn->ctx, 1);

	if(ca_cert[0] && SSL_CTX_load_verify_locations(conn->ctx, ca_cert, NULL) != 1) {
		tr_log(LOG_ERROR, "Loading CA cert file failed");
		goto error;
	}

	SSL_CTX_set_default_passwd_cb_userdata(conn->ctx, (char *)password);
	SSL_CTX_set_default_passwd_cb(conn->ctx, password_cb);

	if(client_cert[0] && SSL_CTX_use_certificate_file(conn->ctx, client_cert, SSL_FILETYPE_PEM) != 1) {
		tr_log(LOG_ERROR, "Loading client cert file failed");
	}

	if(client_key[0] && SSL_CTX_use_PrivateKey_file(conn->ctx, client_key, SSL_FILETYPE_PEM) != 1) {
		tr_log(LOG_ERROR, "Loading client key file failed");
	}

	if(SSL_CTX_check_private_key(conn->ctx) != 1) {
		tr_log(LOG_ERROR, "Check private key and cert file failed");
	}

	if((conn->ssl = SSL_new(conn->ctx)) == NULL || SSL_set_fd(conn->ssl, conn->fd) != 1 || SSL_connect(conn->ssl) != 1) {
		tr_log(LOG_ERROR, "SSL socket connection failed");
		goto error;
	}

	/* Get the server's certificate (optional) */
	{
		X509 *server_cert;
		char *str;

		server_cert = SSL_get_peer_certificate (conn->ssl);     

		if(server_cert != NULL) {
			str = X509_NAME_oneline(X509_get_subject_name(server_cert),0,0);
			if(str == NULL)
				goto error;

			free (str);

			str = X509_NAME_oneline(X509_get_issuer_name(server_cert),0,0);
			if(str == NULL)
				goto error;

			free(str);

			X509_free (server_cert);
		}
	}


	return 0;

error:
	destroy_ssl_connection(conn);

	return -1;
}
#endif
