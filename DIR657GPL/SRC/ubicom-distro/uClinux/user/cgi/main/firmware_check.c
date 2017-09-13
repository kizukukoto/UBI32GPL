#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT		80
#define SERVER_IP 	"wrpd.dlink.com.tw"
#define SERVER_PATH	"/router/firmware/query.asp?model=DIR-615_B2_Default"

int main(int argc, char *argv[]) {

	int s;
	struct in_addr saddr;
	struct sockaddr_in addr;
	struct hostent *host;
	char recv_buf[1024]={0}, send_buf[1024]={0}, online_fw_info[30]={0};
	char online_fw_major[4]={0}, online_fw_minor[4]={0}, online_fw_date[12]={0}, online_fw[8]={0}, down_site[512]={0};
	char *major_s=NULL, *major_e=NULL, *minor_s=NULL, *minor_e=NULL, *date_s=NULL, *date_e=NULL, *dl_site_s=NULL, *dl_site_e=NULL;
	char temp[128]={0}, site[128]={0}, fw_site[128]={0};
	char *fw_site_s=NULL, *fw_site_e=NULL, *fw_e=NULL, *area_e=NULL;

	//step 1
	//check wan connection
	//not coding yet
	
	host = (struct hostent *)malloc(128);
	host = gethostbyname(SERVER_IP);
	if (host == NULL) {
		printf("gethostbyname() fail\n");
		return;
	}
	
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	memcpy(&saddr.s_addr, host->h_addr, 4);
	addr.sin_addr.s_addr = saddr.s_addr;

	if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket error\n");
		close(s);
		return;
	}

	if(connect(s, &addr, sizeof(addr)) < 0) {
		printf("connect error\n");
		close(s);
		return;
	}

	sprintf(send_buf, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n", SERVER_PATH, SERVER_IP);	

	if (send(s, send_buf, strlen(send_buf), 0) < 0) {
		printf("send error\n");
		close(s);
		return;
	}

	memset(recv_buf, 0, sizeof(recv_buf));

	if (recv(s, recv_buf, sizeof(recv_buf), 0) < 0) {
		printf("recv error\n");
		close(s);
		return;
	}

	printf("recv_buf: \n%s\n", recv_buf);

	if (shutdown(s, 2) < 0) {
		printf("shutdown error\n");
		return;
	}

	if (strstr(recv_buf, "HTTP/1.1 200 OK")) {
		major_s = strstr(recv_buf, "<Major>");
		major_e = strstr(recv_buf, "</Major>");
		minor_s = strstr(recv_buf, "<Minor>");
		minor_e = strstr(recv_buf, "</Minor>");
		date_s = strstr(recv_buf, "<Date>");
		date_e = strstr(recv_buf, "</Date>");
		dl_site_s = strstr(recv_buf, "<Download_Site>");
		dl_site_e = strstr(recv_buf, "</Download_Site>");
	
		if (major_s == NULL || major_e == NULL || minor_s == NULL || minor_e == NULL || date_s == NULL || date_e == NULL || 
		    dl_site_s == NULL || dl_site_e == NULL) {
			printf("strstr error\n");
		}else{
			strncpy(online_fw_major, major_s+7, major_e - (major_s+7));	
			strncpy(online_fw_minor, minor_s+7, minor_e - (minor_s+7));
			sprintf(online_fw, "%s.%2d", online_fw_major, atoi(online_fw_minor));
			strncpy(online_fw_date, date_s+6, date_e - (date_s+6));
			printf("fw: %s\ndate: %s\n", online_fw, online_fw_date);

			strncpy(down_site, dl_site_s, dl_site_e - (dl_site_s));
			printf("down_site: %s\n", down_site);
			
			fw_site_s = strstr(down_site, "<Firmware>");
			fw_site_e = strstr(down_site, "</Firmware>");
			strncpy(site, down_site+16, fw_site_s - (down_site+17));
			strncpy(fw_site, fw_site_s+10, fw_site_e - (fw_site_s+10));
			printf("site: %s\nfw_site: %s\n", site, fw_site);
			sprintf(temp, "</%s>", site);
			printf("%s\n", temp);
			fw_e = strstr(down_site, temp);
		
			while (strncmp(fw_site_e, "<Firmware>", strlen("<Firmware>")) == 0) {
				fw_e++;
				area_e = strchr(fw_e, '<');
				fw_site_s = strstr(fw_e, "<Firmware>");
				fw_site_e = strstr(fw_e, "</Firmware>");
				strncpy(site, area_e+1, fw_site_s - (area_e+1));
				strncpy(fw_site, fw_site_s+10, fw_site_e - (fw_site_s+10));
			}
			/*	
			printf("fw_e: %d\ndl_site_e: %d\n", fw_e, dl_site_e);
			strncpy(down_site, fw_e+8, dl_site_e - (fw_e+8));
			printf("down_site: %s\n", down_site);
			*/
			
		}
	}else {
		printf("recv_buf don't have HTTP/1.1 200 OK\n");
	}

}
