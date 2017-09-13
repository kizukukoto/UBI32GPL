/*=======================================================================

  Copyright(c) 2009, Works Systems, Inc. All rights reserved.

  This software is supplied under the terms of a license agreement 
  with Works Systems, Inc, and may not be copied nor disclosed except 
  in accordance with the terms of that agreement.

  =======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "war_string.h"
#include "network.h"
#include "connection.h"
#include "war_socket.h"
#include "war_errorcode.h"
#include "tr_strings.h"

/*!
 * \brief third process communication with Agent 
 *  Local web server config or other change from local,can notify Agent what happen by CLI
 *
 * \param url cli server address
 * \param body http send body
 *  example: tr069_cli("http://127.0.0.1:1234/add/event/", "code=8 DIAGNOSTICS COMPLETE");
 * \return -1 fail; 0 success
 */
int tr069_cli(const char *url, const char *body)
{
	int fd;
	struct sockaddr_in server;
	struct hostent *hp;
	char *host, *path, *port;
	int iport = 80;

	char tmp[128];

	war_snprintf(tmp, sizeof(tmp), "%s", url);

	if(war_strncasecmp(tmp, "http://", 7) == 0)
		host = tmp + 7;
	else
		host = tmp;
	path = strchr(host, '/');
	if(path) {
		*path = '\0';
		path++;
	} else {
		path = "";
	}

	port = strchr(host, ':');
	if(port) {
		*port = '\0';
		iport = atoi(port + 1);
	}

	fd = war_socket(AF_INET, SOCK_STREAM, 0);
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(iport);

	if((server.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
		hp = war_gethostbyname(host);
		if(hp) {
			memcpy(&(server.sin_addr), hp->h_addr, sizeof(server.sin_addr));
		} else {
			printf("Resolve server address(%s) failed!", host);
			return -1;
		}
	}

	if(war_connect(fd, (struct sockaddr *)&server, sizeof(server)) == 0) {
		int len;
		int res = 0;
		char buffer[512];
		char *from;

		len = strlen(body);

		len = war_snprintf(buffer, sizeof(buffer),
				"POST /%s HTTP/1.1\r\n"
				"Host: %s:%d\r\n"
				"Content-Type: application/x-www-form-urlencoded\r\n"
				"Content-Length: %d\r\n"
				"\r\n"
				"%s", path, host, iport, len, body);
		printf("send:%s\n", buffer);

		from = buffer;
		do {
			len -= res;
			from += res;
			res = send(fd, from, len, 0);
		} while(res > 0);

		if(res == 0)
			printf("send OK\n");

		printf("Wait response...\n");
		memset(buffer, 0, sizeof(buffer));

		res = recv(fd, buffer, sizeof(buffer), 0);
		if(res > 0) {
			printf("recv return %d\n", res);
			printf("%s\n", buffer);
		} else if(res == 0) {
			printf("TCP have closed by peer\n");
		} else {
			printf("recv fail = %s\n", war_sockstrerror(war_getsockerror()));
		}
	} else {
		printf("Connect to peer failed: %s\n", war_sockstrerror(war_getsockerror()));
		war_sockclose(fd);
		return -1;
	}
	war_sockclose(fd);
	return 0;
}
