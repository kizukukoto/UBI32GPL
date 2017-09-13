#include "inetsocket.h"
#include <stdio.h>
#include <unistd.h>
//#define I386
//#define DEBUG
#ifndef I386
#include "libdb.h"
#endif

#define MSG_JAR_TIMEOUT "<body><center>Connection fail, the D-Link server has no response.<Br>Please try again after few minutes.</center></body>"
#define MSG_JAR_NOEXIST "<body><center>Download fail, the Applet file has not exists on D-Link server.</center></body>"
#define MSG_JAR_PARSERR "<body><center>Connection fail, the D-Link server response an incorrect URL of Java Applet.</center></body>"
#define MSG_JAR_CONTACT "<body><center>Please contact <a href=\"http://support.dlink.com.tw\">us</a> to solve this problem.</center></body>"

static int is_jar_url(char *url)
{
#ifdef DEBUG
	printf("%s<Br>\n", url);
#endif

	if(strstr(url, "http://") != NULL && strstr(url, ".jar") != NULL)
		return 1;

	printf("%s%s", MSG_JAR_PARSERR, MSG_JAR_CONTACT);
	return 0;
}

static int is_jar_exist(char *url)
{
#ifdef DEBUG
	printf("%s<Br>\n", url);
#endif
	char host[1024];
	char getmsg[1024];
	char request[1024];
	char header[2048];
	SOCKET s = -1;
	size_t i = 0;
	struct sockaddr_in saddrin;


	bzero(&saddrin, sizeof(struct sockaddr_in));
	bzero(getmsg, sizeof(getmsg));
	bzero(header, sizeof(header));
	bzero(host, sizeof(host));
	bzero(request, sizeof(request));

	memcpy(host, url + strlen("http://"), strstr(url + strlen("http://"), "/") - (url + strlen("http://")));
	memcpy(request, strstr(url, host) + strlen(host), strlen(url) - strlen("http://") - strlen(host));

#ifdef DEBUG
	printf("host: %s\nrequest: %s\n", host, request);
#endif

	sprintf(header, "GET %s HTTP/1.1\r\n", request);
	sprintf(header, "%sHost: %s\r\n", header, host);
	sprintf(header, "%sUser-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.11) ", header);
	sprintf(header, "%sGecko/20071204 Ubuntu/7.10 (gutsy) Firefox/2.0.0.11\r\n", header);
	sprintf(header, "%sAccept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9", header);
	sprintf(header, "%s,text/plain;q=0.8,image/png,*/*;q=0.5\r\n", header);
	sprintf(header, "%sAccept-Language: en-us,en;q=0.5\r\n", header);
	sprintf(header, "%sAccept-Encoding: gzip,deflate\r\n", header);
	sprintf(header, "%sAccept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n", header);
	sprintf(header, "%sKeep-Alive: 300\r\n", header);
	sprintf(header, "%sConnection: close\r\n", header);
	sprintf(header, "%sIf-Modified-Since: Sat, 12 Mar 2005 07:47:05 GMT\r\n", header);
	sprintf(header, "%sIf-None-Match: \"da443afd726c51:194f\"\r\n", header);
	sprintf(header, "%sCache-Control: max-age=0\r\n\r\n", header);

	s = InetSockCliInit(host, 80, SOCK_STREAM, &saddrin);
	if(s < 0) {
		InetDestroySocket(s);
		printf("%s", MSG_JAR_TIMEOUT);
		return 0;
	}

	write(s, header, strlen(header));
	for(; i<sizeof(getmsg) - 1; i++) {
		read(s, &getmsg[i], 1);
#ifdef DEBUG
		printf("%c", getmsg[i]);
#endif
	}

	InetDestroySocket(s);

	if(strstr(getmsg, "404 Not Found") ||
		strstr(getmsg, "HTTP/1.1 404") ||
		strstr(getmsg, "HTTP/1.1 401")) {
		printf("%s", MSG_JAR_NOEXIST);
		return 0;
	}

	return 1;
}

void display_script()
{
	char model_name[12];
	char res[1024];
	char cmd[1024];
	FILE *fp = NULL;

	bzero(res, sizeof(res));
	bzero(cmd, sizeof(cmd));
	bzero(model_name, sizeof(model_name));
#ifdef I386
	strcpy(model_name, "DIR-330");
#else
	query_record("model_name", model_name, sizeof(model_name));
#endif
	sprintf(cmd, "/bin/firmupdate -d http://wrpd.dlink.com.tw/router/firmware/query.asp?model=%s_Ax_SSLApplet", model_name);
	fp = popen(cmd, "r");
	fread(res, 1, sizeof(res), fp);
	fclose(fp);

	printf("<body LANGUAGE=javascript scroll=\"no\" onload=\"return window_onload()\">\n");
	printf("<Center>\n");
	printf("<SCRIPT>\n");
	printf("if(navigator.userAgent.toLowerCase().indexOf(\"windows\") < 0) {\n");
	printf("	document.write(\"Your operating system is not Windows,<br>the following procedure is terminal.\");\n");
	printf(" } else {\n");
	printf("var _app = navigator.appName;\n");

	printf("if (_app == 'Microsoft Internet Explorer') {\n");
	printf("	document.write( '<object classid=\"clsid:8AD9C840-044E-11D1-B3E9-00805F499D93\"',\n");
	printf("	'width=\"437\"',\n");
	printf("	'height=\"214\"',\n");
	printf("	'name=\"Firm\"',\n");
	printf("	'codebase=\"http://java.sun.com/update/1.6.0/jinstall-6u7-windows-i586.cab\">',\n");
	printf("	'<param name=\"model\" value=\"%s\">',\n", model_name);
	printf("	'<param name=\"browser\" value=\"IE\">',\n");
	printf("	'<param name=\"code\" value=\"Firm.class\">',\n");
	printf("	'<param name=\"archive\" value=\"%s\">',\n", res);
	printf("	'</object>');\n");

	printf("} else {\n");
	printf("	document.write( '<embed code=\"Firm.class\"',\n");
	printf("	'archive=\"%s\"',\n", res);
	printf("	'width=\"437\"',\n");
	printf("	'height=\"214\"',\n");
	printf("	'name=\"Firm\"',\n");
	printf("	'model=\"%s\"',\n", model_name);
	printf("	'browser=\"Firefox\"',\n");
	printf("	'type=\"application/x-java-applet;version=1.5\"',\n");
	printf("	'pluginspage=\"http://java.sun.com/products/plugin/index.html#download\"/>');\n");

	printf("}\n");
	printf("}\n");
	printf("</SCRIPT>\n");
	printf("</Center>\n");
	printf("</body>\n");
}

#ifdef I386
int main(int argc, char *argv[])
#else
int file_exist(int argc, char *argv[])
#endif
{
	if(argc <= 1)
		return 0;

	char cmd[1024];
	char buf[1024];
	FILE *fp = NULL;
	size_t i = 1;

	bzero(buf, sizeof(buf));
	bzero(cmd, sizeof(cmd));

	for(; i<argc; i++)
		sprintf(cmd, "%s %s", cmd, argv[i]);

	fp = popen(cmd, "r");
	fread(buf, 1, sizeof(buf), fp);

	if(is_jar_url(buf) == 0 || is_jar_exist(buf) == 0) {
//	if(is_jar_url(buf) == 0) {
		fclose(fp);
		return 0;
	}

	display_script();
	fclose(fp);

	return 1;
}
