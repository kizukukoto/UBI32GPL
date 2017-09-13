/*
 * This cgi required in Applet. SSL Applet list all of utility download area.
 * When this cgi is running, get the xml document from wrpd.dlink.com.tw with
 * specify model name(DIR-130 or DIR-330). After xml document get successful,
 * 'list_download_area' static function can list all of download area and do-
 * wnload url. Area name and URL address spliting with '|' charactor, each a-
 * rea spliting with double '|' charactor.
 *
 * Example:
 * 	Global|http://tw.yahoo.com||Taiwan|http://www.google.com.tw||
 */

//#define x86
#include <unistd.h>
#include "inetsocket.h"
#ifndef x86
#include "libdb.h"
#endif

extern void create_sending_msg(char *buf, char *hostname, char *request);
extern size_t get_xml_content(SOCKET fd, char *xmlbuf, size_t bufSize, char opt);

#ifdef x86
int is_wan_ok()
{
	return 1;
}
#else
extern int is_wan_ok();
#endif

static int list_download_url(char *buf)
{
	const char *urltag = "<Firmware>";
	const char *__urltag = "</Firmware>";
	if(buf == NULL || *buf == '\0')
		return -1;

	char *fwtag = strstr(buf, urltag);
	char url[128];

	if(fwtag == NULL)
		return -1;

	bzero(url, sizeof(url));
	memcpy(url, fwtag + strlen(urltag), strstr(fwtag + strlen(urltag), __urltag) - fwtag - strlen(urltag));
	printf("%s||", url);

	return 1;
}

static int list_file_checksum(char *buf)
{
	char *cs_front = strstr(buf, "<Checksum>");
	char *cs_tail = NULL;
	char checksum[256];

	if (cs_front == NULL) {
		printf("00000000||");
		return 0;
	}

	cs_tail = strstr(buf + strlen("<Checksum>"), "</Checksum>");

	if (cs_tail == NULL) {
		printf("00000000||");
		return 0;
	}

	bzero(checksum, sizeof(checksum));
	memcpy(checksum, cs_front + strlen("<Checksum>"), cs_tail - (cs_front + strlen("<Checksum>")));
	printf("%s||", checksum);

	return 1;
}

static int __list_neighbor_node(char *xmlbuf, size_t buflen, char *findtag, size_t ftlen)
{
	size_t idx = 0;
	char untag[16];

	for(; idx < buflen; idx++) {
		if(xmlbuf[idx] == '<' && xmlbuf[idx + 1] != '/') {
			bzero(findtag, ftlen);
			bzero(untag, sizeof(untag));
			memcpy(findtag, &xmlbuf[idx + 1], strstr(&xmlbuf[idx + 2], ">") - &xmlbuf[idx + 1]);	//download area
			printf("%s|", findtag);

			list_download_url(strstr(&xmlbuf[idx + 2], ">")) + 1;	// download url
//			list_file_checksum(strstr(&xmlbuf[idx + 2], ">")) + 1;

			sprintf(untag, "</%s>", findtag);
			idx = strstr(xmlbuf, untag) - xmlbuf;
		}
	}

	return 0;
}

static int list_download_area(char *xmlbuf, size_t xml_len)
{
	if(xmlbuf == NULL || *xmlbuf == '\0' || xml_len == 0)
		return 1;

	char *tag = "<Download_Site>";
	char untag[16];
	char *front = NULL;
	char *tail = NULL;
	char dl_buf[2048];
	char findtag[16];

	bzero(findtag, sizeof(findtag));
	bzero(dl_buf, sizeof(dl_buf));
	bzero(untag, sizeof(untag));
	strcpy(untag, "</");
	strcpy(&untag[2], tag + 1);

	front = strstr(xmlbuf, tag);
	tail = strstr(front + 1, untag);

	if(front == NULL || tail == NULL)
		return -1;

	memcpy(dl_buf, front + strlen(tag), tail - (front + strlen(tag)));
	__list_neighbor_node(dl_buf, strlen(dl_buf), findtag, sizeof(findtag));

	return 0;
}

#ifdef x86
int main(int argc, char *argv[])
#else
int list_url_main(int argc, char *argv[])
#endif
{
	SOCKET s = -1;
	char *hostname = "wrpd.dlink.com.tw";
	struct sockaddr_in saddrin;
	char sendbuf[1513], request[128];
	char model_name[12], xmlbuf[10000];
	char os_version[12];

	bzero(sendbuf, sizeof(sendbuf));
	bzero(request, sizeof(request));
	bzero(model_name, sizeof(model_name));
	bzero(os_version, sizeof(os_version));
	bzero(xmlbuf, sizeof(xmlbuf));
	bzero(&saddrin, sizeof(struct sockaddr_in));

	if(is_wan_ok() == 0) {
		printf("WAN Interface error.");
		return 0;
	}

	s = InetSockCliInit(hostname, 80, SOCK_STREAM, &saddrin);

	if(s < 0) {
		InetDestroySocket(s);
		printf("Socket error.");
		return 0;
	}

#ifdef x86
	strcpy(model_name, "DIR-330");
	strcpy(os_version, "1.20B13");
#else
	query_record("model_name", model_name, sizeof(model_name));
	query_record("os_version", os_version, sizeof(os_version));
#endif
	sprintf(request, "/router/firmware/query.asp?model=VC_Ax_Default");
//	sprintf(request, "/router/firmware/query.asp?model=%s_Ax_SSLUtilityW", model_name);
//	sprintf(request, "/wrpd/index.xml");

	create_sending_msg(sendbuf, hostname, request);
	write(s, sendbuf, strlen(sendbuf));
	get_xml_content(s, xmlbuf, sizeof(xmlbuf) - 1, 'd');
	list_download_area(xmlbuf, strlen(xmlbuf));

	return 1;
}
