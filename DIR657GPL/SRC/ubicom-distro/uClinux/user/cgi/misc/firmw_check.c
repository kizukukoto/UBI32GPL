//#define I386
#include <unistd.h>
#include "inetsocket.h"
#ifndef I386
#include "libdb.h"
#endif

#define MSG_FW_ERROR    "Error contacting the server, please check the Internet connecting status<br>"
#define MSG_FW_NOTFIND	"Can't find firmware on the server<br>"
#define MSG_FW_LATEST   "This firmware is the latest version<br>"
#define MSG_FW_NEWER    "There is a newer firmware available, please click the link below to obtain the new firmware<br>"

/* These function extern from xp.c */
extern void create_sending_msg(char *buf, char *hostname, char *request);
extern size_t get_xml_content(SOCKET fd, char xmlbuf[], size_t bufSize, char opt);
extern void display_firm_url(char xmlbuf[], int limit, int flag);

int list_neighbor_node(char *xmlbuf, size_t buflen, char *findtag, size_t ftlen, int *f)
{
	size_t idx = 0;
	size_t option_idx = 1;
	char untag[16];

	for(; idx < buflen; idx++)
		if(xmlbuf[idx] == '<' && xmlbuf[idx + 1] != '/') {
			bzero(findtag, ftlen);
			bzero(untag, sizeof(untag));
			memcpy(findtag, &xmlbuf[idx + 1], strstr(&xmlbuf[idx + 2], ">") - &xmlbuf[idx + 1]);
			printf("<option value=\"%d\">%s</option>\n", option_idx++, findtag);

			sprintf(untag, "</%s>", findtag);
			idx = strstr(xmlbuf, untag) - xmlbuf;
		}

	return 0;
}

int list_child_node(char *xmlbuf, size_t buflen, char *tag)
{
	if(buflen <= 0 || xmlbuf == NULL || tag == NULL)
		return -1;

	char untag[16];
	char *front = NULL;
	char *tail = NULL;
	char dl_buf[2048];
	char findtag[16];
	int idx = 0;

	bzero(findtag, sizeof(findtag));
	bzero(dl_buf, sizeof(dl_buf));
	bzero(untag, sizeof(untag));
	strcpy(untag, "</");
	strcpy(&untag[2], tag + 1);

	printf("<form>\n");
	printf("<select id=\"firmware_area\" name=\"firmware_area\" size=1>\n");
	printf("  <option value=\"-1\">Download Area</option>\n");

	front = strstr(xmlbuf, tag);
	tail = strstr(front + 1, untag);

	if(front == NULL || tail == NULL)
		return -1;

	memcpy(dl_buf, front + strlen(tag), tail - (front + strlen(tag)));

	list_neighbor_node(dl_buf + idx, strlen(dl_buf), findtag, sizeof(findtag), &idx);

	printf("</select>\n<input type=\"button\" value=\"Download\" name=\"dlbutton\" onClick=\"url_func()\">\n");
	printf("</form>\n");
	return 1;
}

int check_version(char *xmlbuf, size_t buflen, char *os_version, char *latest_ver, char *latest_date)
{
	char *tag = "<Major>";
	char *untag = "</Major>";
	char *tag2 = "<Minor>";
	char *untag2 = "</Minor>";
	char *tag3 = "<Date>";
	char *untag3 = "</Date>";

	char *majortag = strstr(xmlbuf, tag);
	char *majoruntag = strstr(xmlbuf, untag);
	char *minortag = strstr(xmlbuf, tag2);
	char *minoruntag = strstr(xmlbuf, untag2);
	char *datetag = strstr(xmlbuf, tag3);
	char *dateuntag = strstr(xmlbuf, untag3);
	char major[12], minor[12], version_str[12];

	bzero(version_str, sizeof(version_str));
	bzero(major, sizeof(major));
	bzero(minor, sizeof(minor));

	if(majortag == NULL || majoruntag == NULL ||
		minortag == NULL || minoruntag == NULL ||
		datetag == NULL || dateuntag == NULL) {
		nvram_set("fw_latest_ver", "--");
		nvram_set("fw_latest_date", "--");

		return 0;
	}

	memcpy(major, majortag + strlen(tag), majoruntag - majortag - strlen(tag));
	memcpy(minor, minortag + strlen(tag2), minoruntag - minortag - strlen(tag2));
	memcpy(latest_date, datetag + strlen(tag3), dateuntag - datetag - strlen(tag3));

	sprintf(version_str, "%s.%s", major, minor);
	sprintf(latest_ver, "%s.%s", major, minor);

#ifndef I386
	if(latest_ver == NULL || latest_date == NULL) {
		nvram_set("fw_latest_ver", "--");
		nvram_set("fw_latest_date", "--");
	} else {
		nvram_set("fw_latest_ver", latest_ver);
		nvram_set("fw_latest_date", latest_date);
	}
#endif

	{
		int local_fw_major;
		char local_fw_minor[10];

		bzero(local_fw_minor, sizeof(local_fw_minor));

		sscanf(os_version, "%d.%s", &local_fw_major, local_fw_minor);

		if(local_fw_major < atoi(major))
			return 1;

		if(local_fw_major == atoi(major) && strcmp(local_fw_minor, minor) < 0)
			return 1;
	}

//	if(strcmp(version_str, os_version) <= 0)
//		return -1;
//	return 1;

	return -1;
}

int is_wan_ok()
{
	char wan0_ipaddr[16];
	char wan0_gateway[16];

	bzero(wan0_ipaddr, sizeof(wan0_ipaddr));
	bzero(wan0_gateway, sizeof(wan0_gateway));

#ifdef I386
	strcpy(wan0_ipaddr, "172.21.33.123");
	strcpy(wan0_gateway, "172.21.33.254");
#else
	query_record("wan0_ipaddr", wan0_ipaddr, sizeof(wan0_ipaddr));
	query_record("wan0_gateway", wan0_gateway, sizeof(wan0_gateway));
#endif

	if(strcmp(wan0_ipaddr, "0.0.0.0") == 0 ||
		strcmp(wan0_gateway, "0.0.0.0") == 0)
		return 0;

	return 1;
}

void get_latest_info(char *xmlbuf, char *version, char *date)
{
}

#ifdef I386
int main(int argc, char *argv[])
#else
int firmw_check_main(int argc, char *argv[])
#endif
{
	if(argc <= 1)
		return 1;

	SOCKET s = -1;
	char *hostname = "wrpd.dlink.com.tw";
	struct sockaddr_in saddrin;
	char sendbuf[1513], request[128];
	char model_name[12], xmlbuf[10000];
	char os_version[12];
	char latest_version[12];
	char latest_date[12];
	int ckv;

	bzero(os_version, sizeof(os_version));
	bzero(xmlbuf, sizeof(xmlbuf));
	bzero(model_name, sizeof(model_name));
	bzero(&saddrin, sizeof(struct sockaddr_in));
	bzero(sendbuf, sizeof(sendbuf));
	bzero(request, sizeof(request));
	bzero(latest_version, sizeof(latest_version));
	bzero(latest_date, sizeof(latest_date));

	if(is_wan_ok() == 0) {
		if(strcmp(argv[1], "-l") == 0)
			printf("%s\n", MSG_FW_ERROR);
		else if(strcmp(argv[1], "-s") == 0) {
			nvram_set("fw_latest_ver", "--");
			nvram_set("fw_latest_date", "--");
		}

		return 0;
	}

	s = InetSockCliInit(hostname, 80, SOCK_STREAM, &saddrin);

	if(s < 0) {
		InetDestroySocket(s);

		if(strcmp(argv[1], "-l") == 0)
			printf("%s", MSG_FW_ERROR);
		else if(strcmp(argv[1], "-s") == 0) {
			nvram_set("fw_latest_ver", "--");
			nvram_set("fw_latest_date", "--");
		}

		return -1;
	}

#ifdef I386
	strcpy(model_name, "DIR-330");
	strcpy(os_version, "1.20B13");
#else
	query_record("model_name", model_name, sizeof(model_name));
	query_record("os_version", os_version, sizeof(os_version));
#endif
	sprintf(request, "/router/firmware/query.asp?model=%s_Ax_Default", model_name);

	create_sending_msg(sendbuf, hostname, request);
	write(s, sendbuf, strlen(sendbuf));
	get_xml_content(s, xmlbuf, sizeof(xmlbuf), 'd');
	ckv = check_version(xmlbuf, strlen(xmlbuf), os_version, latest_version, latest_date);

	if(strcmp(argv[1], "-u") == 0 && ckv >= 0)
		display_firm_url(xmlbuf, 999, 1);
	else if(strcmp(argv[1], "-l") == 0) {
		if(ckv == -1) {
			printf("%s", MSG_FW_LATEST);
			return 1;
		} else if(ckv == 0) {
			printf("%s", MSG_FW_ERROR);
			return 1;
		} else {
		//	printf("%s", MSG_FW_NEWER);
			list_child_node(xmlbuf, strlen(xmlbuf), "<Download_Site>");
		}
	}

	return 1;
}
