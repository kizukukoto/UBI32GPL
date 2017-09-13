/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file connection.c
 * \brief HTTP session connection abstraction implementation
 */
#include <string.h>
#include <stdlib.h>


#include "tr_strings.h"
#include "network.h"
#include "connection.h"
#include "log.h"
#include "ssl.h"
#include "war_string.h"
#include "war_socket.h"
#include "war_errorcode.h"

int tr_conn(struct connection *conn, const char *u)
{
	char url[256];

// 2011.03.24
	char ipv6_host[256];

#ifdef __ENABLE_SSL__
	char *proto = "http";
#endif
        char *host = NULL;
	char *c;

	conn->secure = 0;
	conn->fd = -1;
#ifdef __ENABLE_SSL__
	conn->ctx = NULL;
	conn->ssl = NULL;
#endif

        u = skip_blanks(u);

	war_snprintf(url, sizeof(url), "%s", u);
	c = strstr(url, "://");
	if(c) {
		*c = '\0';
		host = c + 3;
#ifdef __ENABLE_SSL__
		if(war_strcasecmp(url, "https") == 0) {
			proto = "https";
		} else 
#endif
                    if(war_strcasecmp(url, "http")) {                       
			tr_log(LOG_WARNING, "Unsupported protocol: %s", url);
                        return -1;
		}
	} else {
		host = url;
	}

printf("[%s]\n",host);


	c = strchr(host, '/');
	if(c) {
		war_snprintf(conn->path, sizeof(conn->path), "%s", c);
		*c = '\0';
	} else {
		war_snprintf(conn->path, sizeof(conn->path), "/");
	}


// 2011.03.24
// add for ipv6 http:://[address]
// here for port issue
	if( strchr(host,']')) {
		host = strchr(host,']');
		host++;
	}

printf("[%s][%s]\n",url,host);

	c = strchr(host, ':');
	if(c) {
		war_snprintf(conn->port, sizeof(conn->port), "%s", c + 1);
		*c = '\0';
#ifdef __ENABLE_SSL__
	} else if(war_strcasecmp(proto, "https") == 0){
		war_snprintf(conn->port, sizeof(conn->port), "443"); /**The default port for HTTPS*/
#endif
	} else {
		war_snprintf(conn->port, sizeof(conn->port), "80");
	}

printf("[%s][%s]\n",url,host);

// 2011.03.24
// here for address
	if( strchr(u,'[') && strchr(u,']')) {
		
		war_snprintf(ipv6_host, sizeof(ipv6_host), "%s", u);
		
		c = strchr(ipv6_host,'[');
		c++;
		host = c;
		if( strchr(host,']')) {
			c = strchr(host,']');
			*c = '\0';
		}
		
		 
	}

printf("[%s]\n",host);
	war_snprintf(conn->host, sizeof(conn->host), "%s", host);
        

#if defined(__DEVICE_IPV4__)
        {
            struct sockaddr_in addr;
	    struct hostent *hp;

	    memset(&addr, 0, sizeof(addr));
	    addr.sin_port = htons(tr_atos(conn->port));
	    addr.sin_family = AF_INET;

	    if((addr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
		hp = war_gethostbyname(host);
		if(hp) {
		    memcpy(&(addr.sin_addr), hp->h_addr, sizeof(addr.sin_addr));
		} else {
		    tr_log(LOG_WARNING, "Resolve server address(%s) failed!", host);
		    return -1;
		}
	    }

	    conn->fd = war_socket(AF_INET, SOCK_STREAM, 0);
	    if(conn->fd < 0) {
		tr_log(LOG_ERROR, "Create socket failed: %s", war_strerror(war_geterror()));
		return -1;
	    } else if(tr_connect(conn->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		tr_log(LOG_ERROR, "Connect to server(%s) failed: %s", host, war_strerror(war_geterror()));
		war_sockclose(conn->fd);
		conn->fd = -1;
		return -1;
	    }
	}
#else
	{
	    int rc;
	    struct addrinfo hints, *res, *ressave;
        //int on=10;
        struct timeval timeout;
        timeout.tv_sec = 20;
        timeout.tv_usec = 0;

	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = AF_UNSPEC;
	    hints.ai_socktype = SOCK_STREAM;

//
			hints.ai_protocol = IPPROTO_TCP;

printf("[%s]\n",conn->host);

	    if((rc = getaddrinfo(conn->host, conn->port, &hints, &res)) != 0) {
		tr_log(LOG_WARNING, "Get server(%s) address information failed: %s!", host, gai_strerror(rc));
		return -1;
	    }

	    ressave = res;

	    do {
		conn->fd = war_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (setsockopt(conn->fd, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout)) == 0) //Brian+: set socket recieve timeout=20 secs
            tr_log(LOG_NOTICE, "SETSOCKOPT recieve timeout success!!"); //Brian
        else
            tr_log(LOG_NOTICE, "SETSOCKOPT recieve timeout fail!!"); //Brian

        if (setsockopt(conn->fd, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout)) == 0) //Brian+: set socket send timeout=20 secs
            tr_log(LOG_NOTICE, "SETSOCKOPT send timeout success!!"); //Brian
        else
            tr_log(LOG_NOTICE, "SETSOCKOPT send timeout fail!!"); //Brian
    

		if(conn->fd < 0) {
		    tr_log(LOG_ERROR, "Create socket failed: %s", war_sockstrerror(war_getsockerror()));
		} else if(tr_connect(conn->fd, res->ai_addr, res->ai_addrlen) == 0){
		    break;
		} else {
		    tr_log(LOG_ERROR, "Connect to server(%s) failed: %s", conn->host, war_sockstrerror(war_getsockerror()));
		    war_sockclose(conn->fd);
		    conn->fd = -1;
		}
	    } while((res = res->ai_next) != NULL);

	    freeaddrinfo(ressave);
	}
#endif

#ifdef __ENABLE_SSL__
	if(conn->fd >= 0)
	    block_socket(conn->fd);

	if(conn->fd >= 0 && war_strcasecmp(proto, "https") == 0) {
	    conn->secure = 1;
	    if(setup_ssl_connection(conn) != 0) {
		war_sockclose(conn->fd);
		conn->fd = -1;
	    }
	}
#endif

	if(conn->fd >= 0)
	    nonblock_socket(conn->fd);

	return conn->fd;
}


void tr_disconn(struct connection *conn)
{
#ifdef __ENABLE_SSL__
    destroy_ssl_connection(conn);
#endif

    if(conn->fd >= 0) {
	war_sockclose(conn->fd);
	conn->fd = -1;
    }
}


int tr_conn_recv(struct connection *conn, void *buf, int len)
{
#ifdef __ENABLE_SSL__
    if(conn->ssl)
	return SSL_read(conn->ssl, buf, len);
    else
#endif
	return recv(conn->fd, buf, len, 0);
}

int tr_conn_send(struct connection *conn, const void *buf, int len)
{
    tr_log(LOG_DEBUG, "Send to peer: \n%s", (char *)buf);
#ifdef __ENABLE_SSL__
    if(conn->ssl)
	return SSL_write(conn->ssl, buf, len);
    else
#endif
	return send(conn->fd, buf, len, 0);
}
