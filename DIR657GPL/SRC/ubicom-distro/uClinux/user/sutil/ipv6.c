#ifdef IPv6_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <mtd/mtd-user.h>
#include "shvar.h"
#include "sutil.h"
#include "scmd_nvram.h"
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

/*
 * Shared utility Debug Messgae Define
 */
#ifdef SUTIL_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#ifdef MRD_ENABLE
void write_mrd_conf(void)
{
	init_file(MRD_CONF_FILE);
	save2file(MRD_CONF_FILE,"load-module mld;\n");
	save2file(MRD_CONF_FILE,"groups {\n");
	save2file(MRD_CONF_FILE,"ff0e::/16 {\n");
	save2file(MRD_CONF_FILE,"}\n");
	save2file(MRD_CONF_FILE,"}\n");
}
#endif

int show_eth_ip6_address(char *interface_name,int ifindex)
{
        int socket_index;
        struct ifreq ifr;
        struct sockaddr_in6 *socket_addr;
        unsigned char mac[6] = "";
        memset(&ifr, 0, sizeof(struct ifreq));

        if ((socket_index = socket(AF_INET6,SOCK_RAW,IPPROTO_RAW)) < 0)
                {}
        else
        {
                ifr.ifr_addr.sa_family = AF_INET6;
                strcpy(ifr.ifr_name,interface_name);
        }

        if (ioctl(socket_index, SIOCGIFADDR, &ifr) == 0)
        {
                socket_addr = (struct sockaddr_in6 *) &ifr.ifr_addr;
        }
        else

        if (ioctl(socket_index, SIOCGIFHWADDR, &ifr) == 0)
        {
                memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
        }

        if (ioctl(socket_index, SIOCGIFINDEX, &ifr) == 0)
                ifindex = ifr.ifr_ifindex;


        close(socket_index);
        return ifindex;
}

int raw_socket6(int ifindex)
{
        int fd;
        struct sockaddr_ll sock;

        if ((fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
        {
                DEBUG_MSG("socket call failed: \n");
                return -1;
        }

        sock.sll_family = PF_PACKET;
        sock.sll_protocol = htons(ETH_P_IPV6);
        sock.sll_ifindex = ifindex;

        if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0)
        {
                DEBUG_MSG("bind call failed: \n");
                close(fd);
                return -1;
        }

        return fd;

}

struct EthPacket6 {
        uint8_t dst_mac[6];
        uint8_t src_mac[6];
        uint8_t type[2];
        struct ip6_hdr ip;
        uint16_t sport;
        uint16_t dport;
        uint8_t data[1500-38];
} __attribute__((packed));


/*1. return "1" when receive ipv6 packet (only ipv6)    */
/*2. ipv6 packet should be global address 2000::~3FFF:: */

int listen_socket6(char* lan_bridge)
{
        int fd, bytes, ifindex=0;
        fd_set rfds;
        struct EthPacket6 packet;
        struct in6_addr saddr, daddr, myaddr;
        ifindex = show_eth_ip6_address(lan_bridge,ifindex);
        FD_ZERO(&rfds);
        fd = raw_socket6(ifindex);
        if(fd < 0)
        {
                printf("FAIL: couldn't listen on socket\n");
                return 0;
        }
        else
        {
                FD_SET(fd, &rfds);
                memset(&packet, 0, sizeof(struct EthPacket6));
                bytes = read(fd, &packet, sizeof(struct EthPacket6));
                if(bytes < 0)
                {
                        printf("no packets\n");
                        close(fd);
                        return 0;
                }
                else
                {
                        saddr = (packet.ip.ip6_src);
                        daddr = (packet.ip.ip6_dst);
                        /* src or dst address is Global (0x20~0x3F) should dial */
                        if ( (daddr.in6_u.u6_addr8[0] >= 32 && daddr.in6_u.u6_addr8[0] <= 63) ||
                                (saddr.in6_u.u6_addr8[0] >= 32 && saddr.in6_u.u6_addr8[0] <= 63))
                        {
                                        close(fd);
                                        return 1;//dial now 
                        }
                        else
                                return 0;
                }
        }
        close(fd);
}
#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt, ## args); \
                fclose(fp); \
        } \
} while (0)

char *read_ipv6addr(const char *if_name,const int scope, char *ipv6addr,const int length)
{
        FILE *fp;
        char buf[1024]={};
        char *start=NULL,*end=NULL;
        int num_flag=0;

        if(!ipv6addr)
                return NULL;

	ipv6addr[0] = '\0';
        init_file(LINK_LOCAL_INFO);
        _system("ifconfig %s 2>&- > %s", if_name,LINK_LOCAL_INFO);
        fp = fopen(LINK_LOCAL_INFO,"r+");
        if(!fp)
                return NULL;

        while(start = fgets(buf,1024,fp)){
                if(scope== SCOPE_GLOBAL)
                        end = strstr(buf,"Scope:Global");
                else if(scope== SCOPE_LOCAL)
                        end = strstr(buf,"Scope:Link");
                if(end){
                        end = end - 1;
                        if(start = strstr(start,"addr:")){
                                start = start + 5 + 1; //skip space
                                if(strlen(ipv6addr)+(end - start)>=length){
                                        break;
                                }
                                if(num_flag==0){
                                        strncpy(ipv6addr,start,end - start);
                                        num_flag=1;
                                }else{
                                        strcat(ipv6addr,",");
                                        strncat(ipv6addr,start,end - start);
                                }
                        }
                }
        }
        fclose(fp);
	if (num_flag)
                return ipv6addr;
	else
                return NULL;
}

/*FIXME Joe Huang : Do not use printf in read_ipv6defaultgatewy & read_ipv6_dns	*/
/*It would made cgi/ajax ipv6_status.xml error					*/
char *read_ipv6defaultgateway(const char *if_name, char *ipv6addr){
        char nvram_temp[30]={0};
        if(!ipv6addr)
                return NULL;
	cmd_nvram_safe_get("ipv6_wan_proto", nvram_temp);
        if(strncmp(nvram_temp,"ipv6_static",11)==0){
		cmd_nvram_safe_get("ipv6_static_default_gw", ipv6addr);
        }else if(strncmp(nvram_temp,"ipv6_6in4",9)==0){
		cmd_nvram_safe_get("ipv6_6in4_remote_ip", ipv6addr);
        }else {
                char buf[200], *pos;
                FILE *pp;

                sprintf(buf,"ip -6 route show match ::/0 dev %s", if_name);
                if( (pp = popen(buf,"r")) != NULL )     {
                        fgets(buf, sizeof buf, pp);
                        pclose(pp);

                        if (strstr(buf, "default via")==buf) { // if start with "default via"
                                pos = strstr(buf+12, " "); // find next " "
                                strncpy(ipv6addr, buf+12, pos-buf-12);
                        }
                } else {
                        DEBUG_MSG("popen() error!\n");
                        return NULL;
                }
        }
        return ipv6addr;
}

char *read_ipv6_dns(void)
{
        FILE *fp;
        char *buff;
        char dns[3][MAX_IPV6_IP_LEN]={};
        static char return_dns[3*MAX_IPV6_IP_LEN] ="";
        int num, i=0, len=0;

        fp = fopen (RESOLVFILE_IPV6, "r");
        if(!fp) {
                perror(RESOLVFILE_IPV6);
                return 0;
        }

        buff = (char *) malloc(1024);
        if (!buff)
        {       fclose(fp);
                return;
        }
        memset(buff, 0, 1024);
        while( fgets(buff, 1024, fp))
        {
                if (strstr(buff, "nameserver") )
                {
                        num = strspn(buff+10, " ");
                        len = strlen( (buff + 10 + num) );

                        if(strchr(buff, '\n'))/*Line Feed*/
                                strncpy(dns[i], (buff+10+num), len-1);
                        else
                                strncpy(dns[i], (buff+10+num), len);
                        i++;
                }
                if(3<=i)
                {
                        strcat(dns[2], "\0");
                        break;
                }
        }
        free(buff);
        sprintf(return_dns, "%s/%s", dns[0], dns[1]);
        fclose(fp);
        return return_dns;
}

void get_ipv6_wanif(char *wan_interface)
{
        char wan_proto[16] = {};
        char wan_eth[16] = {};
        char ppp6_ip[64] = {};
        char autoconfig_ip[64] = {};

        wan_interface[0] = '\0';
        cmd_nvram_safe_get("ipv6_wan_proto", wan_proto);
        cmd_nvram_safe_get("wan_eth", wan_eth);

        if (strcmp(wan_proto, "ipv6_6to4") == 0)
                strcpy(wan_interface, "tun6to4");

        else if (strcmp(wan_proto, "ipv6_6in4") == 0)
                strcpy(wan_interface, "sit1");

        else if (strcmp(wan_proto, "ipv6_pppoe") == 0)
                if(cmd_nvram_match("ipv6_pppoe_share", "1") == 0)
                        strcpy(wan_interface, "ppp0");
                else
                        strcpy(wan_interface, "ppp6");

        else if(strcmp(wan_proto, "ipv6_6rd") == 0)
                strcpy(wan_interface, "sit2");

        else if(strcmp(wan_proto, "ipv6_autodetect") == 0)
                if(read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip))){
                        strcpy(wan_interface, "ppp0");
                        strcpy(wan_proto, "ipv6_pppoe");
                }else if(read_ipv6addr(wan_eth, SCOPE_GLOBAL, autoconfig_ip, sizeof(autoconfig_ip))){
                        strcpy(wan_interface, wan_eth);
                        strcpy(wan_proto, "ipv6_autoconfig");
                }else{
                        strcpy(wan_interface, wan_eth);
                        strcpy(wan_proto, "ipv6_autodetect");
                }
        else
                strcpy(wan_interface, wan_eth);
}

// copy from "cmds/lib/libipv6.c"
#define NVRAM_MATCH(name,match) (!nvram_match(name,match))
void get_pfltime(int *pltime, int *vltime){

	FILE *fp_dhcppd;
	FILE *fp_dhcpc;
	char buf[256] = {};
	int i;

	if(NVRAM_MATCH("ipv6_dhcp_pd_enable", "1") && 
		(NVRAM_MATCH("ipv6_wan_proto", "ipv6_autoconfig") || 
		NVRAM_MATCH("ipv6_wan_proto", "ipv6_pppoe") || 
		NVRAM_MATCH("ipv6_wan_proto", "ipv6_6in4") || 
		NVRAM_MATCH("ipv6_wan_proto", "ipv6_autodetect"))){
		fp_dhcppd = fopen (DHCP_PD , "r" );
		if(fp_dhcppd){
			for( i = 0 ; i < 5 ; i++)
				fgets(buf, sizeof(buf), fp_dhcppd); //ignore line 1~5
			fgets(buf, sizeof(buf), fp_dhcppd);
			*pltime = atoi(buf);
			fgets(buf, sizeof(buf), fp_dhcppd);
			*vltime = atoi(buf);
			fclose(fp_dhcppd);
		}else
			cprintf("Open file error : %s (errno : %d)\n", DHCP_PD, errno);
	}else if(NVRAM_MATCH("ipv6_wan_proto", "ipv6_6rd") ||
		 NVRAM_MATCH("ipv6_wan_proto", "ipv6_6to4")){
		if (NVRAM_MATCH("wan_proto","dhcpc")){
			// DHCPC_LEASE_FILE define in sutil/shvar.h
			fp_dhcpc = fopen(DHCPC_LEASE_FILE, "r");
			if (fp_dhcpc){
				fgets(buf, sizeof(buf), fp_dhcpc);
				*vltime = atoi(buf);
				*pltime = *vltime / 4;
				fclose(fp_dhcpc);
			}
		}
		// check whether never read DHCPC_LEASE_FILE success
		if (!buf[0]){
			// set default lifetime to 7 days
			*vltime = 604800;
			*pltime = *vltime / 4;
		}
	}else{
		if(NVRAM_MATCH("ipv6_autoconfig_type","stateful")){
			*pltime = 60 / 4 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime")); //XXX Joe.H: perfer time = 1/4 valid time.
			*vltime = 60 * atoi(nvram_safe_get("ipv6_dhcpd_lifetime"));	//TODO dhcpd lifetime is minutes
		}else{									//     RA lifetime is seconeds
			*pltime = atoi(nvram_safe_get("ipv6_ra_adv_valid_lifetime_l_one")) / 4;
			*vltime = atoi(nvram_safe_get("ipv6_ra_adv_valid_lifetime_l_one"));
		}
	}
}

#endif
