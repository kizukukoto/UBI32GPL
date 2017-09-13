#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"
/*Joe Huang: This cmd would call from GUI st_ipv6.asp
XXX When user refresh the page, this cmd would update the client list file from neighbor
*/

static int compare_back(FILE *fp,int current_line,char *buffer);
static int check_mac_previous(char *mac);
static char *value(FILE *fp,int line,int token);
static void find_hostname_by_mac(char *mac, char *hostname);
static int ipv6_client_list_result();
int total_lines = 0;

static int ipv6_client_numbers(void)
{
        FILE *fp;
        int numbers = 0;
        char temp[IPV6_BUFFER_SIZE] = {};
        fp = fopen(IPV6_CLIENT_RESULT,"r");
        if(fp){
                while(fgets(temp,IPV6_BUFFER_SIZE,fp))
                        numbers++;
                fclose(fp);
                return numbers;
        }
        return 0;
}

static int compare_back(FILE *fp,int current_line,char *buffer)
{
        int i=0;
        char mac[32] = {},compare_mac[32] = {};

        buffer[strlen(buffer) -1] = '\0';
        strcpy(mac,value(fp,current_line,MAC));
        if(check_mac_previous(mac))
                return 0;
        for(i=0;i<(total_lines - current_line);i++){
                strcpy(compare_mac,value(fp,current_line+1+i,MAC));
                if(strcmp(mac,compare_mac) == 0){
                        strcat(buffer,",");
                        strcat(buffer,value(fp,current_line+1+i,IPV6_ADDRESS));
                }
        }
        save2file(IPV6_CLIENT_RESULT,"%s\n",buffer);
        return 0;
}

static int check_mac_previous(char *mac)
{
        FILE *fp;
        char temp[IPV6_BUFFER_SIZE];
        memset(temp,0,sizeof(temp));
        fp = fopen(IPV6_CLIENT_RESULT,"r");
        if(fp){
                while(fgets(temp,IPV6_BUFFER_SIZE,fp)){
                        if(strstr(temp,mac)){
                                fclose(fp);
                                return 1;
                        }
                }
                fclose(fp);
        }
        return 0;
}

static char *value(FILE *fp,int line,int token)
{
        int i;
        static char temp[IPV6_BUFFER_SIZE],buffer[IPV6_BUFFER_SIZE];
        fseek(fp,0,SEEK_SET);
        for(i = 0;i < line;i++){
                memset(temp,0,sizeof(temp));
                fgets(temp,sizeof(temp),fp);
        }
        memset(buffer,0,sizeof(buffer));
        switch(token){
                case MAC:
                        strcpy(buffer,strtok(temp,"/"));
                        break;
                case HOSTNAME:
                        strtok(temp,"/");
                        strcpy(buffer,strtok(NULL,"/"));
                        break;
                case IPV6_ADDRESS:
                        strtok(temp,"/");
                        strtok(NULL,"/");
                        strcpy(buffer,strtok(NULL,"/"));
                        buffer[strlen(buffer) -1] = '\0';
                        break;
                default:
                        cprintf("error option\n");
                        strcpy(buffer,"ERROR");
                        break;
        }
        return buffer;
}

static void get_ipv6_client_list(void)
{
        FILE *fp;
        int line_index = 1;
        char temp[IPV6_BUFFER_SIZE],*buffer[IPV6_BUFFER_SIZE];
        memset(temp,0,sizeof(temp));
        fp = fopen(IPV6_CLIENT_INFO,"r");
        init_file(IPV6_CLIENT_RESULT);
        if(fp){
                while(fgets(temp,IPV6_BUFFER_SIZE,fp))
                        total_lines++;
                fseek(fp,0,SEEK_SET);
                memset(temp,0,sizeof(temp));
                memset(buffer,0,sizeof(buffer));

                while(fgets(temp,IPV6_BUFFER_SIZE,fp)){
                        compare_back(fp,line_index,temp);
                        value(fp,line_index,MAC);
                        line_index++;
                }
                fclose(fp);
        }
        line_index = 1;
}

static void find_hostname_by_mac(char *mac, char *hostname)
{
	FILE *fp;
	char buffer[128] = {};
	char *client_lan_mac=NULL, *host_name=NULL;
	fp = fopen(CLIENT_LIST_FILE,"r");
	if (fp){
        	while (fgets(buffer, 128, fp)){
			strtok(buffer, "/");
			host_name = strtok(NULL, "/");
			client_lan_mac = strtok(NULL, "/");
			//When hostname is NULL that will make client_lan_mac be NULL and
			//hostname be mac address
			//ex:192.168.0.100//11:22:33:44:55:66
			//client_lan_mac = NULL , hostname = 11:22:33:44:55:66
        		if ( client_lan_mac == NULL ){
        		        client_lan_mac = host_name ;
		                host_name = "";
			}

			if (!client_lan_mac)
				continue;

			if (strncasecmp(client_lan_mac, mac, 17) == 0 && (strlen(host_name) != 0)){
				fclose(fp);
				strcpy(hostname, host_name);
				return;
			}

			memset(buffer, 0, sizeof(buffer));
        	}
		fclose(fp);
	}
	strcpy(hostname, "Native IPv6 Client");
}


static int ipv6_client_list_result()
{
	FILE *fp;
	char buffer[128] = {},ipv6_address[128] = {},mac[32] = {};
	char *ptr_end, hostname[64];
	_system("ip -f inet6 neigh show dev %s > %s",nvram_safe_get("lan_bridge"),IPV6_CLIENT_LIST);
	usleep(500);
	fp = fopen(IPV6_CLIENT_LIST,"r");
	if(fp){
		init_file(IPV6_CLIENT_INFO);
		while(fgets(buffer,128,fp)){
			if(ptr_end = strstr(buffer,"lladdr"))
			{
				ptr_end = ptr_end - 1;
				strncpy(ipv6_address,buffer,ptr_end - buffer);
				ptr_end = ptr_end + 8;
				strncpy(mac,ptr_end,17);
				find_hostname_by_mac(mac, hostname);
				if ( (ipv6_address[0] == '2' || ipv6_address[0] == '3')
				&& ipv6_address[0] != ':' && ipv6_address[1] != ':'
				&& ipv6_address[2] != ':' && ipv6_address[3] != ':'
				)
					save2file(IPV6_CLIENT_INFO,"%s/%s/%s\n", mac, hostname, ipv6_address);
				else if (NVRAM_MATCH("ipv6_wan_proto","link_local"))
					save2file(IPV6_CLIENT_INFO,"%s/%s/%s\n", mac, hostname, ipv6_address);
				memset(buffer,0,sizeof(buffer));
				memset(ipv6_address,0,sizeof(ipv6_address));
				memset(mac,0,sizeof(mac));
			}
		}
		fclose(fp);
	}
}

int clientlist_main(int argc, char *argv[])
{
	ipv6_client_list_result();
	get_ipv6_client_list();

        return 0;
}
