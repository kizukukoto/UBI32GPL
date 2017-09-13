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

#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <netdb.h> 	 
#include <linux/if_packet.h> 
#include <linux/if_ether.h>

#include "cmds.h"
#include "shutils.h"
#include <nvramcmd.h>
#include "ipv6.h"

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

void get_v6_prefix_with_len(struct in6_addr *ip6, int len)
{
	uint32_t mask;
	int quotient = len / 32;
	int remainder = len % 32;
	
	if (len<0 || len >128) return;
	
	mask = 0xFFFFFFFF << (32-remainder);
	ip6->s6_addr32[quotient] &= mask;
	
	for (quotient++; quotient<4; quotient++)
		ip6->s6_addr32[quotient] = 0;
}

void get_v4_prefix_with_len(struct in_addr *ip4, int len)
{
	uint32_t mask;
	
	if (len<0 || len >32) return;
	
	mask = 0xFFFFFFFF << (32-len);
	ip4->s_addr &= mask;
}

/*
	try to get correct in6_addr by append ":" or "::" to ip6
	if success return a pointer to in6_addr 
	else return NULL
*/
struct in6_addr* get_correct_in6_addr(char *ip6, struct in6_addr *in6)
{
	char ip6_tmp[50];
	int str_len;
	
	str_len = strlen(
			strncpy(ip6_tmp, ip6, sizeof(ip6_tmp)-3)
		);

	if (!inet_pton(AF_INET6, ip6, in6)) // if transform to in6_addr fail
	{
		if (str_len>2) {
			// try append ":" or "::" to ip6_tmp, then transform to in6_addr again
			if (ip6_tmp[str_len-1] == ':')
				strcat(ip6_tmp, ":");
			else
				strcat(ip6_tmp, "::");

			if (!inet_pton(AF_INET6, ip6_tmp, in6)) // if also transform to in6_addr fail
				return (struct in6_addr*)NULL;
		} else
			return (struct in6_addr*)NULL;
	}
	
	return in6;
}

void gen_6rd_addr(struct in6_addr *pf, int pf_len, struct in_addr *ip4, int v4_masklen, char *mac)
{
	u_int32_t mask;

	if (pf_len <= 32) {
		mask = 0xFFFFFFFF << (32-pf_len);

		pf->s6_addr32[0] &= mask;
		pf->s6_addr32[0] |= (unsigned long long)(ip4->s_addr) >> pf_len;
		pf->s6_addr32[1] = (ip4->s_addr) << (32-pf_len);
	} else {
		mask = 0xFFFFFFFF << (64-pf_len);

		pf->s6_addr32[1] &= mask;
		pf->s6_addr32[1] |= 
			( (unsigned long long)((ip4->s_addr) << (v4_masklen)) ) >> (pf_len-32);
	}
	copy_mac_to_eui64(mac, pf->s6_addr);
}

int get_net_mask(char *if_name, struct in_addr *netmask)
{
	int sockfd;
	struct ifreq ifr; 

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) 
		return -1;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) { 
		close(sockfd);
		return -1;
	}

	close(sockfd);

	netmask->s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
	return 0;
}

/*Use in app.c*/
/*When IPv4 address changed would do start_app*/
static int v6rd_main_diagnose (int argc, char *argv[])
{
        char *wan_if;
	char wan_v4addr[20] = {};
	struct in_addr v4addr_in;

        char *lan_if;
        char lan_mac[6]={};

	struct in6_addr pf_in6;
	int pf_len;
	int v4_masklen;

        char lan_6rd_addr[50];

        if(!NVRAM_MATCH("ipv6_wan_proto","ipv6_6rd"))
                return 0;

        if(NVRAM_MATCH("wan_proto","pppoe") ||
		NVRAM_MATCH("wan_proto","pptp") || 
		NVRAM_MATCH("wan_proto","l2tp"))
                wan_if = "ppp0";
        else
                wan_if = nvram_safe_get("wan_eth");

	getIPbyDevice(wan_if, wan_v4addr);

        if (strlen(wan_v4addr) != 0 && strcmp(wan_v4addr,"0.0.0.0")!=0) {
		inet_aton(wan_v4addr, &v4addr_in);

		if( !get_correct_in6_addr(nvram_safe_get("ipv6_6rd_prefix"), &pf_in6))
			return 0;

		pf_len = atoi(nvram_safe_get("ipv6_6rd_prefix_length"));
		v4_masklen = atoi(nvram_safe_get("ipv6_6rd_ipv4_mask_length"));

		lan_if = nvram_safe_get("lan_bridge");
                get_mac_addr(lan_if, lan_mac);

		gen_6rd_addr(&pf_in6, pf_len, &v4addr_in, v4_masklen, lan_mac);

		inet_ntop(AF_INET6, &pf_in6, lan_6rd_addr, sizeof(lan_6rd_addr));

                _system("ip -6 addr flush scope global dev %s", lan_if);
                _system("ip -6 addr add %s/64 dev %s", lan_6rd_addr, lan_if);
                nvram_set("ipv6_6rd_lan_ip", lan_6rd_addr);

/*XXX Joe Huang : using busybox httpd do not need synchronize*/
#ifndef DIR652V
                _system("killall -SIGUSR2 httpd");
#endif
        }
	return 0;
}

static int v6rd_main_start (int argc, char *argv[])
{
        char *wan_if;
	char wan_v4addr[20] = {};
	struct in_addr v4addr_in;
	struct in6_addr pf_in6;
	int pf_len = 0;
	int v4_masklen = 0;
	int mtu = 1280;
        char *relay_ptr;
        char relay_addr[200] = {};

        if (access(SIT2_PID, F_OK) == 0){
                printf("sit2.pid is already active !\n");
                return 0;
        } else
                fclose(fopen(SIT2_PID, "w+"));          // create empty SIT2_PID file

        if(NVRAM_MATCH("wan_proto","pppoe")){
                wan_if = "ppp0";
		mtu = atoi(nvram_safe_get("wan_pppoe_mtu")) - 20;
	}
	else if(NVRAM_MATCH("wan_proto","pptp")){
                wan_if = "ppp0";
		mtu = atoi(nvram_safe_get("wan_pptp_mtu")) - 20;
	}
	else if(NVRAM_MATCH("wan_proto","l2tp")){
                wan_if = "ppp0";
		mtu = atoi(nvram_safe_get("wan_l2tp_mtu")) - 20;
	}
        else{
                wan_if = nvram_safe_get("wan_eth");
		mtu = atoi(nvram_safe_get("wan_mtu")) - 20;
	}
	if(mtu < 0)
		mtu = 1280; 

	getIPbyDevice(wan_if, wan_v4addr);

        if (strlen(wan_v4addr) != 0 && strcmp(wan_v4addr,"0.0.0.0") != 0){

		if( !get_correct_in6_addr(nvram_safe_get("ipv6_6rd_prefix"), &pf_in6))
			goto setup_6rd_fail;
		
		// read nvram lengths of 6rd_prefix & v4_mask
		pf_len = atoi(nvram_safe_get("ipv6_6rd_prefix_length"));
		v4_masklen = atoi(nvram_safe_get("ipv6_6rd_ipv4_mask_length"));

		// transform 6rd_relay(maybe hostname or ipv4 addr) to ipv4 address
                relay_ptr = nvram_safe_get("ipv6_6rd_relay");
                if (inet_aton(relay_ptr, &v4addr_in))
                        strcpy(relay_addr, relay_ptr);
                else{
                	struct hostent *relay_h=NULL;
                        if (relay_h = gethostbyname(relay_ptr))
                                strcpy(relay_addr, inet_ntoa(*(struct in_addr *)(relay_h->h_addr)));
                }
        }

setup_6rd_fail:
	if(strlen(relay_addr) > 0){
		char pf[50] = {};
		
		// eliminate none zero bits that behind prefix_len in 6rd prefix 
		get_v6_prefix_with_len(&pf_in6, pf_len);
		inet_ntop(AF_INET6, &pf_in6, pf, sizeof(pf));

		_system("ip tunnel add sit2 mode sit local %s ttl 64", wan_v4addr);

		if (pf_len <= 32) {
			_system("ip tunnel 6rd dev sit2 6rd-prefix %s/%d", pf, pf_len);
		} else {
	                inet_pton(AF_INET, wan_v4addr, &v4addr_in);
	                
			// eliminate none zero bits that behind v4_masklen in wan_v4addr 
			get_v4_prefix_with_len(&v4addr_in, v4_masklen);

			_system("ip tunnel 6rd dev sit2 6rd-prefix %s/%d 6rd-relay_prefix %s/%d", 
				pf, pf_len, inet_ntoa(v4addr_in), v4_masklen);
		}

		_system("ip link set dev sit2 up mtu %d", mtu);
		_system("ip -6 addr add fe80::%s/64 dev sit2", wan_v4addr);
		_system("ip -6 route add ::/0 via ::%s dev sit2 metric 1", relay_addr);
        } else
                unlink(SIT2_PID);  // create 6rd tunnel failed, so del SIT2_PID

	return 0;
}

/*Joe Huang:When dhcpc get option 212 would call this command*/
static int v6rd_main_dhcp (int argc, char *argv[])
{
	char _6rdPrefix[256] = {0};
	char _6rdPrefix_tmp[256] = {0};
	struct in6_addr pf_in6;

	if(NVRAM_MATCH("ipv6_6rd_dhcp", "0") || !NVRAM_MATCH("ipv6_wan_proto", "ipv6_6rd"))
		return 0;

	strcpy(_6rdPrefix_tmp, argv[3]);
	get_correct_in6_addr(_6rdPrefix_tmp, &pf_in6);
	inet_ntop(AF_INET6, &pf_in6, _6rdPrefix, sizeof(_6rdPrefix));

	nvram_set("ipv6_6rd_ipv4_mask_length", argv[1]);
	nvram_set("ipv6_6rd_prefix_length", argv[2]);
	nvram_set("ipv6_6rd_prefix", _6rdPrefix);
	nvram_set("ipv6_6rd_relay", argv[4]);

	if (access(SIT2_PID, F_OK) == 0)
		system("cli ipv6 6rd stop");
	if (access(RADVD_PID, F_OK) == 0)
		system("cli ipv6 radvd stop");
	if (access(DHCPD6_PID, F_OK) == 0)
		system("cli ipv6 dhcp6d stop");
	system("cli ipv6 6rd start");
	system("cli ipv6 6rd diagnose");
	system("cli ipv6 radvd start");
	system("cli ipv6 dhcp6d start");

	return 0;
}

static int v6rd_main_stop (int argc, char *argv[])
{
        system("ip link set dev sit2 down");
        system("ip tunnel del sit2");
        unlink(SIT2_PID);

	return 0;
}

int v6rd_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "start", v6rd_main_start, "start 6RD tunnel"},
                { "stop", v6rd_main_stop, "stop 6RD tunnel"},
		{ "diagnose", v6rd_main_diagnose, "Change 6rd LAN IP base on IPv4 current IP"},
		{ "dhcp", v6rd_main_dhcp, "start 6RD DHCPv4 Option"},
                { NULL, NULL}
        };
        int rev;
        rev = lookup_subcmd_then_callout(argc, argv, cmds);
        return rev;
}


