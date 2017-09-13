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


//#define FWQD_DEBUG

#ifdef FWQD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define MSG_FW_ERROR	"ERROR"
#define MSG_FW_LATEST	"LATEST"


extern const char VER_FIRMWARE[];
char const *wan_eth=NULL;
char const *fw_host=NULL, *fw_path=NULL;
char const *result_file=NULL;

void write_result(char *result)
{
	FILE *fp;

	DEBUG_MSG("fwqd: result = %s\n", result);
	fp = fopen(result_file, "w+");
	if(fp == NULL)
	{
		perror("write_result()");
		return;
	}

	fprintf(fp, "%s", result);
	fclose(fp);
	DEBUG_MSG("fwqd: write_result fnished\n");
}
void igore_term(void)
{
     DEBUG_MSG("someone want to kill me \n");

}
void online_firmware_check(void)
{
	int	fd, ret=0;
	char recv_buf[2048]={0};
	char dls_buf[512]={0};
	char send_buf[256]={0}, result[2048]={0};
	struct  in_addr saddr;
	struct  sockaddr_in addr;
	struct  hostent *host;
	char online_fw[8]={0}, online_fw_major[4]={0}, online_fw_minor[4]={0}, fw_date[12]={0};
	char *ver_ptr=NULL;
	char *major_s=NULL, *major_e=NULL, *minor_s=NULL, *minor_e=NULL, *date_s=NULL, *date_e=NULL;
	char *dl_site_s=NULL, *dl_site_e=NULL;
	char *fw_region[] = { "Global", "TW", "CN", "KO", "KR", "RU", "SH", "WW", "EU", "EA", "NA", "Europe", ""};	
	int index;
	char *region_s=NULL, *region_e=NULL, *fw_region_s=NULL, *fw_region_e=NULL, *rn_region_s=NULL, *rn_region_e=NULL;
	char fw_region_info[256]={0}, fw_region_url[128]={0}, rn_region_url[128]={0};
	char fw_region_site[128]={0}, fw_region_site_url[512]={0};
	char region_s_tmp[15]={0}, region_e_tmp[15]={0};
	int region_len=0;
	char status[10];
	fd_set afdset;
	struct timeval timeval;

#ifdef CONFIG_USER_3G_USB_CLIENT
	DEBUG_MSG("fw_query: Support 3G USB => Do not check WANPORT connection state\n");
#else
	if( VCTGetPortConnectState( wan_eth, VCTWANPORT0, status) == 0 )
	{
		if( !strncmp("disconnect", status, 10) )
		{
			write_result(MSG_FW_ERROR);
			DEBUG_MSG("fw_query:VCTWANPORT0 disconnect\n");
			return;
		}
	}
	else
	{
		write_result(MSG_FW_ERROR);
		DEBUG_MSG("fw_query:VCTGetPortConnectState() error\n");
		return;
	}
#endif	//CONFIG_USER_3G_USB_CLIENT

	host = gethostbyname(fw_host);
	if( host == NULL )
	{
		perror("fw_query:gethostbyname()");
		write_result(MSG_FW_ERROR);
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
		write_result(MSG_FW_ERROR);
		return;
	}

	DEBUG_MSG("fw_query:connect()...\n");
	if (connect(fd, &addr, sizeof(addr)) < 0)
	{
		perror("fw_query:connect()");
		close(fd);
		write_result(MSG_FW_ERROR);
		return;
	}

	sprintf(send_buf, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n"
			, fw_path, fw_host);

	DEBUG_MSG("fw_query:send_buf =\n%s\n", send_buf);

	if( send(fd, send_buf, strlen(send_buf), 0 ) < 0)
	{
		perror("fw_query:send()");
		close(fd);
		write_result(MSG_FW_ERROR);
		return;
	}

#ifdef AR7161
	sleep(2);
#else
	sleep(1); /*wait for recv buf*/
#endif


	memset(recv_buf, 0, sizeof(recv_buf));
	DEBUG_MSG("fw_query:recv()...\n");

	FD_ZERO(&afdset);
	FD_SET(fd, &afdset);

	timeval.tv_sec = 3;
	timeval.tv_usec = 0;

	/* timeout error */
	ret = select(fd + 1, &afdset, NULL, NULL, &timeval);
	if(ret == -1)
	{
		perror("fw_query:select()");
		close(fd);
		write_result(MSG_FW_ERROR);
		return;
	}

	/* receive fail */
	if(FD_ISSET(fd, &afdset))
		read(fd, recv_buf, sizeof(recv_buf));
	else
	{
		DEBUG_MSG("fwqd: recv_buf = NULL\n");
		return;
	}

	DEBUG_MSG("fw_query:recv_buf size = %d\n", strlen(recv_buf));
	DEBUG_MSG("fw_query:recv_buf = \n%s\n", recv_buf);

	/* check end of data */
	if(strstr(recv_buf, "</Download_Site>") == NULL)
	{
		DEBUG_MSG("fwqd: recv_buf error\n");
		write_result(MSG_FW_ERROR);
		return;
	}

	if( shutdown(fd, 2) < 0)
	{
		write_result(MSG_FW_ERROR);
		perror("fw_query:shutdown()");
		return;
	}

	DEBUG_MSG("Parsing recv_buf...\n");
	if(strstr(recv_buf, "HTTP/1.1 200 OK"))
	{
		//only one information
		major_s = strstr(recv_buf, "<Major>");
		major_e = strstr(recv_buf, "</Major>");
		minor_s = strstr(recv_buf, "<Minor>");
		minor_e = strstr(recv_buf, "</Minor>");
		date_s = strstr(recv_buf, "<Date>");
		date_e = strstr(recv_buf, "</Date>");
		dl_site_s = strstr(recv_buf, "<Download_Site>");
		dl_site_e = strstr(recv_buf, "</Download_Site>");

		if( major_s==NULL || major_e==NULL )
		{
			DEBUG_MSG("fw_query parse error: major\n");
			write_result(MSG_FW_ERROR);
			return;
		}
		else if( minor_s==NULL || minor_e==NULL )
		{
			DEBUG_MSG("fw_query parse error: minor\n");
			write_result(MSG_FW_ERROR);
			return;
		}
		else if( date_s==NULL  || date_e==NULL  )
		{
			DEBUG_MSG("fw_query parse error: date\n");
			write_result(MSG_FW_ERROR);
			return;
		}
		else if( dl_site_s==NULL || dl_site_e==NULL )
		{
			DEBUG_MSG("fw_query parse error: dl_site\n");
			write_result(MSG_FW_ERROR);
			return;
		}
		else
		{
			strncpy( online_fw_major, major_s+7, major_e - (major_s+7) );
			strncpy( online_fw_minor, minor_s+7, minor_e - (minor_s+7) );
			sprintf( online_fw, "%s.%02d", online_fw_major, atoi(online_fw_minor) );
			strncpy( fw_date, date_s+6, date_e - (date_s+6) );
			DEBUG_MSG("online_fw =%s\n", online_fw);
			DEBUG_MSG("fw_date =%s\n", fw_date);
			//For legacy support
			if( strcmp(result_file, "/var/tmp/fw_check_result.txt") == 0){
				ver_ptr = (online_fw[0] == '0')? &online_fw[1]: &online_fw[0];
				if(strncmp(ver_ptr, VER_FIRMWARE, strlen(VER_FIRMWARE))<=0){
					write_result(MSG_FW_LATEST);
					return;
				} else
					sprintf(result, "%s+%s", online_fw, fw_date);
            } else	// Let GUI do check the version is LATEST or not.
			sprintf(result, "%s+%s", online_fw, fw_date);
		}

		//maybe many download site, ex, Global, TW, CN
		for( index=0; strlen(fw_region[index])>1; index++)
			{
			memset(region_s_tmp, 0, sizeof(region_s_tmp));
			memset(region_e_tmp, 0, sizeof(region_s_tmp));
			sprintf(region_s_tmp, "<%s>", fw_region[index]);
			sprintf(region_e_tmp, "</%s>", fw_region[index]);
			region_s = strstr(dl_site_s, region_s_tmp);
			region_e = strstr(dl_site_s, region_e_tmp);

			if( region_s==NULL || region_e==NULL)
				DEBUG_MSG("fw_query parse : region : not found %s\n", fw_region[index]);
			else{
				memset(fw_region_info, 0, sizeof(fw_region_info));
				region_len=strlen(fw_region[index]);
				strncpy( fw_region_info, region_s+region_len, region_e - (region_s+region_len) );
				fw_region_s = strstr(fw_region_info, "<Firmware>");
				fw_region_e = strstr(fw_region_info, "</Firmware>");
				rn_region_s = strstr(fw_region_info, "<Release_Note>");
				rn_region_e = strstr(fw_region_info, "</Release_Note>");

				if( fw_region_s!=NULL && fw_region_e!=NULL && rn_region_s!=NULL && rn_region_e!=NULL)
				{
					memset(fw_region_url, 0, sizeof(fw_region_url));
					strncpy( fw_region_url, fw_region_s+10, fw_region_e - (fw_region_s+10) );
					DEBUG_MSG("fw_region_url =%s\n", fw_region_url);
					strncpy( rn_region_url, rn_region_s+14, rn_region_e - (rn_region_s+14) );
					DEBUG_MSG("rn_region_url =%s\n", rn_region_url);
				}
				else
				{
					DEBUG_MSG("fw_query parse error: fw_region_url\n");
					write_result(MSG_FW_ERROR);
					return;
				}

				if(strlen(fw_region_site)>0)
					sprintf(fw_region_site, "%s,%s", fw_region_site, fw_region[index]);
				else
					sprintf(fw_region_site, "%s", fw_region[index]);

				if(strlen(fw_region_site_url)>0)
					sprintf(fw_region_site_url, "%s,%s", fw_region_site_url, fw_region_url);
			else
					sprintf(fw_region_site_url, "%s", fw_region_url);
					
				}
				}
		sprintf(result, "%s+%s+%s", result, fw_region_site, fw_region_site_url);			
					write_result(result);
	}
	else
	{
		DEBUG_MSG("fw_query:recv_buf don't have HTTP/1.1 200 OK\n");
		write_result(MSG_FW_ERROR);
		return;
    }

    return;
}


void usage(void)
{
	printf("Usage: fwqd [-i wan_eth ] [-u fw_url ] [-t interval in sec] [-r result_file]\n");
}

int main(int argc, char* argv[])
{
	int c = 0;
	int time_interval = 0;
	char *check_fw_url = NULL;

	signal(SIGSYS, online_firmware_check);
#ifndef AR9100
	signal(SIGTERM, igore_term);
#endif
	while( (c = getopt(argc, argv, "i:u:t:r:")) != -1 )
	{
		switch(c)
		{
			case 't':
				time_interval = atoi(optarg);
				break;
			case 'u':
				check_fw_url = optarg;
				break;
			case 'i':
				wan_eth = optarg;
				break;
			case 'r':
				result_file = optarg;
				break;
			default:
				usage();
				exit(1);
		}
	}

	if(check_fw_url == NULL || wan_eth == NULL)
	{
		DEBUG_MSG("fwqd: check_fw_url or wan_eth = null\n");
		exit(1);
	}
	//For legacy support
	if( result_file == NULL)
		result_file = "/var/tmp/fw_check_result.txt";

	//Ex. wrpd.dlink.com.tw,/router/firmware/query.asp?model=DIR-615_Cx
	fw_host = strtok(check_fw_url, ",");
	fw_path = strtok(NULL, ",");

	DEBUG_MSG("time_interval = %d, wan_eth = %s, fw_host = %s, fw_path = %s\n",
				time_interval, wan_eth, fw_host, fw_path);
/*
NickChou 20080721, change from deamon to exe.
reduce memory cost for run BT
*/
//	for(;;)
//	{
//		if(time_interval)
//		{
			online_firmware_check();
//			DEBUG_MSG("fwqd: sleep %d sec\n", time_interval);
//			sleep(time_interval);
//		}
//	}

	DEBUG_MSG("fwqd: break for_loop\n");
}
