#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <linux_vct.h>


//#define MIIICASA_DEBUG

#ifdef MIIICASA_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

int main(int argc, char* argv[])
{
	int	fd, ret=0;
	char recv_buf[2048]={0};
	char send_buf[256]={0};
	struct  in_addr saddr;
	struct  sockaddr_in addr;
	struct  hostent *host;
	fd_set afdset;
	struct timeval timeval;
	char fw_host[] = "w1.partner.staging.miiicasa.com", fw_path[] = "/65500000?frameset=0&wsip=&wip=&m=&did=";

	host = gethostbyname(fw_host);
	if( host == NULL )
	{
		perror("miiicasa_UST:gethostbyname()");
		return;
	}
	
	memcpy(&saddr.s_addr, host->h_addr_list[0], 4);
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = saddr.s_addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("fw_query:socket()");
		close(fd);
		return;
	}

	DEBUG_MSG("miiicasa_UST:connect()...\n");
	if (connect(fd, &addr, sizeof(addr)) < 0)
	{
		perror("miiicasa_UST:connect()");
		close(fd);
		return;
	}

	sprintf(send_buf, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n"
			, fw_path, fw_host);

	DEBUG_MSG("miiicasa_UST:send_buf =\n%s\n", send_buf);

	if( send(fd, send_buf, strlen(send_buf), 0 ) < 0)
	{
		perror("miiicasa_UST:send()");
		close(fd);
		return;
	}

	sleep(2);

	memset(recv_buf, 0, sizeof(recv_buf));
	DEBUG_MSG("miiicasa_UST:recv()...\n");

	FD_ZERO(&afdset);
	FD_SET(fd, &afdset);

	timeval.tv_sec = 3;
	timeval.tv_usec = 0;

	/* timeout error */
	ret = select(fd + 1, &afdset, NULL, NULL, &timeval);
	if(ret == -1)
	{
		perror("miiicasa_UST:select()");
		close(fd);
		return;
	}

	/* receive fail */
	if(FD_ISSET(fd, &afdset))
		read(fd, recv_buf, sizeof(recv_buf));
	else
	{
		DEBUG_MSG("miiicasa_UST: recv_buf = NULL\n");
		return;
	}

	DEBUG_MSG("miiicasa_UST:recv_buf size = %d\n", strlen(recv_buf));
	DEBUG_MSG("miiicasa_UST:recv_buf = \n%s\n", recv_buf);

	if( shutdown(fd, 2) < 0)
	{
		perror("miiicasa_UST:shutdown()");
		return;
	}

	DEBUG_MSG("Parsing recv_buf...\n");

        return;
}
