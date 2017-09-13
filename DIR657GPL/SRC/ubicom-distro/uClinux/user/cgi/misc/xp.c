#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include "inetsocket.h"

//#define X86_TEST

void help_display()
{
	printf("Usage: firmupdate option URL [limit]\n\nOptions:\n");
	printf("\t-d  Parse the firmware updating URL from the indicated host.\n");
	printf("\t-D  Parse the XML contents from indicated host.\n");
	printf("\t-x  Parse the entire HTTP packet content from indicated host.\n");
	printf("\tlimit: That's mean the number of firmware updating URL will be display.\n\n");

	printf("Example: firmupdate -d http://wrpd.dlink.com.tw/router/firmware/");
	printf("query.asp?model=DIR-130_Ax_VPNTool 1\n\n");
}

int parameter_verify(int argc)
{
	if (argc < 2) return -1;

	return 1;
}

void create_sending_msg(char *buf, char *hostname, char *request)
{
	sprintf(buf, "GET %s HTTP/1.1\n\
Host: %s\n\
User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.6) Gecko/20071008 Ubuntu/7.10 (gutsy) Firefox/2.0.0.6\n\
Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\n\
Accept-Language: en-us,en;q=0.5\n\
Accept-Encoding: gzip,deflate\n\
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\n\
Keep-Alive: 300\n\
Connection: close\n\
Cookie: ASPSESSIONIDQQTAQDCQ=CHJIIBKAILGOLEPNFEICPHDB\n\n", request, hostname);
}

size_t get_xml_content(SOCKET fd, char xmlbuf[], size_t bufSize, char opt)
{
	char cur;
	unsigned int bufIndex = -1;

	switch (opt) {
		case 'x':
			while (read(fd, &cur, 1) > 0)
				printf("%c", cur);
			break;

		case 'D':
			while (read(fd, &cur, 1) > 0)
				if (cur == '<') {
					printf("<");
					while (read(fd, &cur, 1) > 0)
						printf("%c", cur);
				}
			break;

		case 'd':
			while (read(fd, &cur, 1) > 0)
				if (cur == '<') {
					xmlbuf[0] = '<';
					bufIndex = 1;
					while (read(fd, &cur, 1) > 0) {
						if (bufIndex == bufSize) {
							fprintf(stderr, "Buffer overflow.\n");
							return -1;
						}
						xmlbuf[bufIndex++] = cur;
					}
				}
	}

	return bufIndex;
}

void display_firm_url(char xmlbuf[], int limit, int flag)
{
	char *curDoc = xmlbuf;
	size_t count = 1;
	char fronttarget[] = "<Firmware>";
	char tailtarget[] = "</Firmware>";
	char *fronttag = NULL, *tailtag = NULL;

	if(flag > 0) printf("\tfunction url_func() {\n");

	while ((fronttag = strstr(curDoc, fronttarget)) != NULL && limit > 0) {
		tailtag = strstr(fronttag + strlen(fronttarget), tailtarget);
		size_t len = tailtag - (fronttag + strlen(fronttarget));
		char *url = (char*)malloc(len + 1);
		
		bzero(url, len + 1);
		memcpy(url, fronttag + strlen(fronttarget), len);
		if(url == NULL || url[0] == '\0')
			printf("http://www.dlink.com.tw");
		else if(flag > 0) {
			printf("\t\tif(document.getElementById('firmware_area').selectedIndex == %d)\n", count++);
			printf("\t\t\twindow.location.href='%s';\n", url);
		} else {
			printf("%s", url);
		}
		curDoc = tailtag + strlen(tailtarget);

		limit--;
		if(limit > 0)
			printf("\n");
	}

	if(tailtag == NULL)
		if(flag == 0)
			printf("http://www.dlink.com.tw");
		else {
			printf("\t\tif(document.getElementById('firmware_area').selectedIndex == 0)\n");
			printf("\t\t\twindow.location.href='http://www.dlink.com.tw';\n");
		}
	if(flag > 0)
		printf("\t}");
}

void url_parser(char *url, int ulen, char *hostname, char *request)
{
	char *http = strstr(url, "http://");
	char *spea;

	if (http) {
		spea = strstr(url + strlen("http://"), "/");
		if (spea)
			memcpy(hostname, url + strlen("http://"), spea - (http + strlen("http://")));
		else {
			memcpy(hostname, url + strlen("http://"), (url + ulen) - (http + strlen("http://")));
			strcpy(request, "/");
			return;
		}
	} else {
		spea = strstr(url, "/");
		if (spea)
			memcpy(hostname, url, spea - url);
		else {
			memcpy(hostname, url, ulen);
			strcpy(request, "/");
			return;
		}
	}

	memcpy(request, spea, url + ulen - spea);
}

int main_procedure(char *url, char opt, int limit)
{
	SOCKET s = -1;
	char sendbuf[1513], xmlbuf[10000];
	char hostname[100], request[100];
	struct sockaddr_in saddrin;

	bzero(&saddrin, sizeof(struct sockaddr_in));
	bzero(hostname, 100);
	bzero(request, 100);
	bzero(sendbuf, sizeof(sendbuf));
	bzero(xmlbuf, sizeof(xmlbuf));

	url_parser(url, strlen(url), hostname, request);

	s = InetSockCliInit(hostname, 80, SOCK_STREAM, &saddrin);

	if (s < 0) {
		printf("http://www.dlink.com.tw");
		InetDestroySocket(s);
		return -1;
	}

	create_sending_msg(sendbuf, hostname, request);
	write(s, sendbuf, strlen(sendbuf));

	get_xml_content(s, xmlbuf, sizeof(xmlbuf), opt);

	display_firm_url(xmlbuf, limit, 0);

	close(s);

	return 1;
}

#ifdef X86_TEST
int main(int argc, char *argv[])
#else
int firmupdate_main(int argc, char *argv[])
#endif
{
	char c = -1;
	int limit = 1;

	while (1) {
		int opt_idx;
		static struct option opts[] = {
			{"limit", 0, 0, 'l'},
			{"dumpurl", 0, 0, 'd'},
			{"dumpxml", 0, 0, 'D'},
			{"dumphttp", 0, 0, 'x'},
			{"help", 0, 0, 'h'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "l:d:D:x:", opts, &opt_idx);

		if (c == -1)
			break;

		switch (c) {
		case 'l':
			printf("%s\n", optarg);
		case 'd':
		case 'D':
		case 'x':
			if (argc == 4)
				limit = atoi(argv[3]);
			return main_procedure(optarg, c, limit);
			break;
		case 'h':
		default:
			help_display();
			return 1;
		}
	}
	help_display();

	return 1;
}
