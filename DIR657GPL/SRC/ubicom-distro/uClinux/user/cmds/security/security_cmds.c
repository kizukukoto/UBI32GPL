#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cmds.h"
#include "shutils.h"
#include "nvram.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif
#if 0
/*	Enable HW NAT settings
	 *	Add by Ken_Chiang 20081128	 
	 *	TO support the funtion Just by StomLink Chipset now	 
	 * #/sbin/nat_cfg del eth0
	 * #/sbin/nat_cfg del eth1
	 * /sbin/nat_cfg add ip eth0 ${inip} ${inmask}
	 * /sbin/nat_cfg add ip eth1 ${outip} ${outmask}
	 * #/sbin/nat_cfg set eth0 lan
	 * #/sbin/nat_cfg set eth1 wan
	 * /sbin/nat_cfg set enable
	 */
int hwnat_start_main(int argc, char *argv[])
{
	int i,sockfd, rev = -1;
	char *lan_eth;
	char buf[512];	
	char wan_eth[32] = {0};
	char wan_ip[32] = {0},wan_mask[32] = {0};
	char lan_ip[32] = {0},lan_mask[32] = {0};
	struct ifreq ifr;
	struct in_addr in_addr, netmask;		
			
	DEBUG_MSG("%s:\n",__func__);	
	get_wan_iface(wan_eth);		
	DEBUG_MSG("wan:%s\n",wan_eth);
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0){
		printf("socket fail!\n");
		goto hw_nat_fail;
	}				
				
	strcpy(ifr.ifr_name, wan_eth);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		printf("ioctl fail!\n");
		goto hw_nat_fail;
	}	
	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	sprintf(wan_ip, "%s", inet_ntoa(in_addr));

	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0){ 
		close(sockfd);
		printf("ioctl mask fail!\n");
		goto hw_nat_fail;
	}	
	netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
	sprintf(wan_mask, "%s", inet_ntoa(netmask));
	close(sockfd);	
		
	lan_eth = nvram_safe_get("if_dev0");		
	DEBUG_MSG("lan:%s\n",lan_eth);	
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0){
		printf("socket fail!\n");
		goto hw_nat_fail;
	}
		
	strcpy(ifr.ifr_name, lan_eth);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		printf("ioctl fail!\n");
		goto hw_nat_fail;
	}
	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	sprintf(lan_ip, "%s", inet_ntoa(in_addr));

	if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0){ 
		close(sockfd);
		printf("ioctl mask fail!\n");
		goto hw_nat_fail;
	}	
	netmask.s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
	sprintf(lan_mask, "%s", inet_ntoa(netmask));
	close(sockfd);					
		
	DEBUG_MSG("lan:%s ip=%s mask=%s\n",lan_eth,lan_ip,lan_mask);
	DEBUG_MSG("wan:%s ip=%s mask=%s\n",wan_eth,wan_ip,wan_mask);		
	//_system("/sbin/nat_cfg add ip eth0 %s %s",lan_ip,lan_mask);
	sprintf(buf, "/sbin/nat_cfg add ip eth0 %s %s" ,lan_ip ,lan_mask);
	DEBUG_MSG("buf:%s\n",buf);	
	system(buf);
	//_system("/sbin/nat_cfg add ip eth1 %s %s",wan_ip,wan_mask);
	sprintf(buf, "/sbin/nat_cfg add ip eth1 %s %s" ,wan_ip ,wan_mask);
	DEBUG_MSG("buf:%s\n",buf);	
	system(buf);
	system("/sbin/nat_cfg set enable");
	rev = 0;
	
hw_nat_fail:
	return rev;
}

int hwnat_stop_main(int argc, char *argv[])
{
	
	int rev = -1;			
	DEBUG_MSG("%s:\n",__func__);	
	//sbin/nat_cfg set disable
	system("/sbin/nat_cfg set disable");		
	rev = 0;	
	
	return rev;
}

int hwnat_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "start", hwnat_start_main}, //@dev
		{ "stop", hwnat_stop_main}, //@dev
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
#endif
extern int forward_port_main(int, char *[]);
extern int adv_firewall_main(int, char *[]);
extern int mac_filter_main(int, char *[]);
extern int ipsec_main(int, char *[]);
extern int vpn_subcmd(int, char *[]);
extern int alg_main(int argc, char *argv[]);
extern int inbound_filter_main(int argc, char *argv[]);

int security_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
//		{ "init", sec_init_main},		//@dev
//		{ "forward_port", forward_port_main},	//@dev
//		{ "adv_firewall", adv_firewall_main},	//no args
//		{ "mac_filter", mac_filter_main},	//no args
//		{ "inbound_filter", inbound_filter_main},
//		{ "app", alg_main},			//no args
		{ "vpn", vpn_subcmd},
//		{ "hwnat", hwnat_subcmd},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
