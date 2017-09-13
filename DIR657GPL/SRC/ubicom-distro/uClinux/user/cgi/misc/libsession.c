#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libdb.h"
#include "nvram.h"
#include "debug.h"

#define MAX_NUMBER_FIELD 14

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

char *get_ipaddr(char *if_name)
{
        int sockfd;
        struct ifreq ifr;
        struct in_addr in_addr;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return 0;

        strcpy(ifr.ifr_name, if_name);
        if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
                close(sockfd);
                return 0;
        }

        close(sockfd);

        in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
        return inet_ntoa(in_addr);
}

int check_valid( char *local_ip, char *remote_ip,  
		char *lan_ip, char *wan_ip_1)
{
        if (!strcmp(local_ip, "127.0.0.1") || !strcmp(local_ip, "0.0.0.0") \
            || !strcmp(remote_ip, "127.0.0.1")||!strcmp(remote_ip, "0.0.0.0")) {
                return 1;
        } else if (!strcmp(local_ip, lan_ip)  || !strcmp(remote_ip, lan_ip) ) {
                return 1;
        } else if ((strlen(wan_ip_1) && (!strcmp(local_ip, wan_ip_1))) || !strcmp(remote_ip,wan_ip_1)) {
                return 1;
        } else
                return 0;
}

char *get_wan_ip(char *wan_ip_1)
{
        char *tmp_ip_1 = NULL;

        if ((NVRAM_MATCH("wan_proto", "dhcpc") != 0) ||
            (NVRAM_MATCH("wan_proto", "static") != 0)) {
                tmp_ip_1 = get_ipaddr(nvram_safe_get("wan_eth"));
                if (tmp_ip_1)
                        strncpy(wan_ip_1, tmp_ip_1, strlen(tmp_ip_1));
        } else {
                tmp_ip_1 = get_ipaddr("ppp0");
                if(tmp_ip_1)
                        strncpy(wan_ip_1, tmp_ip_1, strlen(tmp_ip_1));
        }
}

char* get_protocol_name(char *pro_number)
{
        switch (atoi(pro_number)) {
                case 1: return "ICMP";
                case 2: return "IGMP";
                case 3: return "GGP";
                case 4: return "IP";
                case 5: return "ST";
                case 6: return "TCP";
                case 7: return "CBT";
                case 8: return "EGP";
                case 9: return "IGP";
                case 17: return "UDP";
                case 46: return "GRE";
                default: return "UNKONWN";
        }
}

