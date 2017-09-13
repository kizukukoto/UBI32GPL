#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>

#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef MPPPOE
int ppp0_idle_time = 0,ppp1_idle_time = 0,ppp2_idle_time = 0,ppp3_idle_time = 0,ppp4_idle_time = 0;
int ppp0_timer_count = 0;
int ppp0_timer_flag = 1;
int ppp1_timer_count = 0;
int ppp1_timer_flag = 1;
int ppp2_timer_count = 0;
int ppp2_timer_flag = 1;
int ppp3_timer_count = 0;
int ppp3_timer_flag = 1;
int ppp4_timer_count = 0;
int ppp4_timer_flag = 1;
int ppp0_file_flag = 0;
#endif

int check_flag = 1;

struct EthPacket {
	uint8_t dst_mac[6];
	uint8_t src_mac[6];
	uint8_t type[2];
	struct iphdr ip;	//sizeof( struct iphdr ) = 20
	uint16_t sport;
	uint16_t dport;
	uint8_t data[1500-38];
} __attribute__((packed));

struct route_info {
	struct in_addr network;
	struct in_addr mask;
};
struct route_info rout_table[MAX_STATIC_ROUTING_NUMBER];

int raw_socket(int ifindex)
{
	int fd;
	struct sockaddr_ll sock;

	if ((fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
	{
		DEBUG_MSG("socket call failed: \n");
		return -1;
	}

	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;

	if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0)
	{
		DEBUG_MSG("bind call failed: \n");
		close(fd);
		return -1;
	}

	return fd;

}

int show_eth_ip_address(char *interface_name,int ifindex)
{
	int socket_index;
	struct ifreq ifr;
	struct sockaddr_in *socket_addr;
	unsigned char mac[6] = "";
	memset(&ifr, 0, sizeof(struct ifreq));

	if ((socket_index = socket(AF_INET,SOCK_RAW,IPPROTO_RAW)) < 0)
		DEBUG_MSG("socket fail!!\n");
	else
	{
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name,interface_name);
		DEBUG_MSG("%s\n", ifr.ifr_name);
	}

	if (ioctl(socket_index, SIOCGIFADDR, &ifr) == 0)
	{
		socket_addr = (struct sockaddr_in *) &ifr.ifr_addr;
		DEBUG_MSG("%s ip = %s\n", ifr.ifr_name, inet_ntoa(socket_addr->sin_addr));
	}
	else
		DEBUG_MSG("ioctl fail!!\n");

	if (ioctl(socket_index, SIOCGIFHWADDR, &ifr) == 0) 
	{
		memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
		DEBUG_MSG("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x \n",\
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	if (ioctl(socket_index, SIOCGIFINDEX, &ifr) == 0)
		ifindex = ifr.ifr_ifindex;


	close(socket_index);
	return ifindex;
}

static int get_if_tx_pkts_diff(const char *if_name, int *modified)
{
	struct pkt_counter_s pkt_counters;
	static unsigned long last_tx_pkts = 0;
	int ret;
	
	if (if_name == NULL)	return (-1);
	if (modified == NULL)	return (-1);
	
	ret = RC_get_if_pkt_counters(if_name, &pkt_counters);
	
	if (ret == 0)
	{
		if (pkt_counters.tx_packets > last_tx_pkts)
		{
			last_tx_pkts = pkt_counters.tx_packets;
			*modified = 1;
			return (0);
		}
		else
			return (-1);
	}
	else if (ret == -2)	/* interface not found */
	{
		return (-2);
	}
	
	
	*modified = 0;
	return (-1);
	
}

/*
 * return 0: don't dial
 * return 1: dial now (normal PPTP/L2TP/PPPoE)
 * return 2: dial now (Russia PPTP/L2TP/PPPoE)
 * return 3: dial now (both PPTP/L2TP/PPPoE)
 */
#ifdef RPPPOE	
int listen_socket(int russia_version, int obtain_ip_done, char *ip, char *mask, int route_count, char *lan_netmask, char *lan_ipaddr,char* lan_bridge)
#else
int listen_socket(char *lan_netmask, char *lan_ipaddr, char* lan_bridge)
#endif
{
	int fd, bytes, ifindex=0;
	fd_set rfds;
	struct EthPacket packet;
	struct in_addr saddr, daddr, netmask, myaddr, multicast_mask, broadcast_mask;
	struct in_addr lower, upper;	
#ifdef RPPPOE	
	struct in_addr wan_phy_ip, wan_phy_mask;
	int i=0;
	int modified;
#endif
	
	inet_aton(lan_netmask, &netmask);
	inet_aton(lan_ipaddr, &myaddr);
	inet_aton("255.255.255.255", &broadcast_mask);
	inet_aton("255.0.0.0", &multicast_mask);
	inet_aton("224.0.0.0", &lower);
	inet_aton("240.0.0.0", &upper);
	ifindex = show_eth_ip_address(lan_bridge,ifindex);
	
	FD_ZERO(&rfds);
	fd = raw_socket(ifindex);

	if(fd < 0)	
	{
		printf("FAIL: couldn't listen on socket\n");
		return 0;
	} 
	else 
	{
		FD_SET(fd, &rfds);
		memset(&packet, 0, sizeof(struct EthPacket));
		bytes = read(fd, &packet, sizeof(struct EthPacket));
		
		if(bytes < 0)
		{
			printf("no packets\n");
			close(fd);
			return 0;
		}			
		else
		{
			saddr.s_addr = (packet.ip.saddr);
			daddr.s_addr = (packet.ip.daddr);

			/* Router checks if the destination of packet isn't LAN   */
			if( (netmask.s_addr & myaddr.s_addr) != (netmask.s_addr & daddr.s_addr)) 
			{
				/* Filter the multicast and broadcast packets */
				if( ((daddr.s_addr & multicast_mask.s_addr) >= lower.s_addr) && 
				    ((daddr.s_addr & multicast_mask.s_addr) <= upper.s_addr) ) 
				{
					DEBUG_MSG("Multicast Packet\n");
					close(fd);
					return 0;
				} 
				else if( daddr.s_addr == broadcast_mask.s_addr ) 
				{
					DEBUG_MSG("Broadcast Packet\n");
					close(fd);
					return 0;
				} 
				else 
				{
#ifdef RPPPOE	
					/* Russia PPTP/PPPoE/L2TP need check if the destination of packet is ISP internel network.
					 * If true, don't dial.
					 * Else, dial now!
					 */
					if( russia_version == 1 )
					{
						if( obtain_ip_done == 1 )
						{
							inet_aton(ip, &wan_phy_ip);
							inet_aton(mask, &wan_phy_mask);
								
							/* ISP internel network (directly-connected)*/				
							if( (wan_phy_mask.s_addr & wan_phy_ip.s_addr) == (wan_phy_mask.s_addr & daddr.s_addr) )
							{
								DEBUG_MSG("(RUSSIA) packet is to ISP internel network (directly-connected)\n");
								close(fd);
					            return 0;
							}
							else
							{
								/* ISP internel network (static route)*/
								for( i=0 ; i<route_count ; i++)
								{
									if( rout_table[i].network.s_addr == (rout_table[i].mask.s_addr & daddr.s_addr))
									{
										DEBUG_MSG("(RUSSIA) packet is to ISP internel network(static route)\n");
										close(fd);
										return 0;
									}
								}
								/* packet is to INTERNET */
								DEBUG_MSG("(RUSSIA)Outgoing packet, from %s to %s\n", inet_ntoa(saddr), inet_ntoa(daddr));
								close(fd);
								return 2;//dial now (russia)
							}
						}
						else
						{
							/* We have not obatined IP yet, so it's impossible to dial now*/
							DEBUG_MSG("(RUSSIA) Not obatined IP yet!\n");
							close(fd);							
							return 0;
						}
					}
					/* For Normal PPTP/PPPoE/L2TP, dial now!*/
					else
#endif
					{
						DEBUG_MSG("Outgoing packet, from %s to %s\n", inet_ntoa(saddr), inet_ntoa(daddr));
						close(fd);
						return 1;//dial now (normal)
					}
				}
			}
#ifdef RPPPOE
			modified = 0;
			if(get_if_tx_pkts_diff("ppp0", &modified) == 0)
			{
				if (modified)
				{
					close(fd);
					return 2;//keep connected
				}
			}										
#endif
			/* DNS query */
			if( myaddr.s_addr == daddr.s_addr && htons(packet.dport)==53 ) 
			{
				DEBUG_MSG("DNS Query from %s\n", inet_ntoa(saddr));
				close(fd);
				return 3; //dial now (both)
			}
			
			close(fd);
			return 0;
		}
	}
	close(fd);	
}

#ifndef MPPPOE
int monitor_main(int argc, char** argv)
{
	int ppp_ifunit=0, pppoe_disconnect=0, connected=0, not_LocalPacket=1;
	int dial_on_demand_mode=0, use_dynamic_ip=0;
	int i=0;
	char *wan_eth=NULL, *wan_proto=NULL, *lan_netmask=NULL, *lan_ipaddr=NULL, *lan_bridge=NULL; 
	char wan_ip[16]={0}, wan_netmask[16]={0};
	char *enable, *name, *value, *dest_addr, *dest_mask, *gateway, *ifname, *metric;
	char *tmp_ip=NULL, *tmp_netmask=NULL;
	char *ppp_ip=NULL, *ppp_netmask=NULL;
	char service_name[128]={0}, ac_name[128]={0};
	char *sn=NULL, *ac=NULL;
	FILE *fp_monitor; 
	FILE *fp_pppoe, *fp_pptp, *fp_l2tp;
	FILE *fp_wantimer;

	char ppp6_ip[MAX_IPV6_IP_LEN]={};
	char lan_ip[MAX_IPV6_IP_LEN]={};
	int autoconfig = 0;	

#ifdef RPPPOE
    char static_routing[]="static_routing_XYXXXXX";
    char route_value[256]={0};
	int  obtain_ip_done=0, route_count=0;
	int  russia_version = 0, russia_use_dynamic_ip=0;
	char *russia_ip = NULL, *russia_mask = NULL ,russia_ip_addr[16]={0}, russia_net_mask[16]={0} ;
#endif	
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/	
#ifdef CONFIG_USER_3G_USB_CLIENT		
	FILE *fp_usb3g;	
#endif 
	/* Collect the information according to wan protocol.
	 * Such as, wan_proto/wan_eth/russia_version/use_dynamic_ip/dial_on_demand_mode 
	 */

	wan_proto = strtok(argv[2], "/");
	wan_eth = strtok(NULL, "/");
	lan_netmask = strtok(NULL, "/");
	lan_ipaddr = strtok(NULL, "/");
	lan_bridge = strtok(NULL, "/");	

	if(strncmp(wan_proto, "pppoe", 5) == 0)
	{
		dial_on_demand_mode = atoi(strtok(argv[3], "/"));
#ifdef RPPPOE
		russia_version = atoi(strtok(NULL, "/"));
		russia_use_dynamic_ip = atoi(strtok(NULL, "/"));
		russia_ip = strtok(NULL, "/");
		russia_mask = strtok(NULL, "/");
#endif	
		strcpy(service_name, argv[4]);		
		strcpy(ac_name, argv[5]);
		sn = parse_special_char(service_name);
		ac = parse_special_char(ac_name);	
	}
	else if(strncmp(wan_proto, "pptp", 4) == 0)
	{
		dial_on_demand_mode = atoi(strtok(argv[3], "/"));
#ifdef RPPPOE	
		russia_version = atoi(strtok(NULL, "/"));
#endif		
		use_dynamic_ip = atoi(strtok(NULL, "/"));
		ppp_ip = strtok(NULL, "/");
		ppp_netmask = strtok(NULL, "/");
	}		
	else if(strncmp(wan_proto, "l2tp", 4) == 0)
	{
		dial_on_demand_mode = atoi(strtok(argv[3], "/"));
#ifdef RPPPOE	
		russia_version = atoi(strtok(NULL, "/"));
#endif
		use_dynamic_ip = atoi(strtok(NULL, "/"));
		ppp_ip = strtok(NULL, "/");
		ppp_netmask = strtok(NULL, "/");				
	}
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/	
#ifdef CONFIG_USER_3G_USB_CLIENT	
	else if(strncmp(wan_proto, "usb3g", 5) == 0)	
	{
		dial_on_demand_mode = atoi(strtok(argv[3], "/"));
	}
#endif

#ifdef RPPPOE
	/* Record the static routes of WAN_PHY */
	if( russia_version )
	{
		for(i=0; i<MAX_STATIC_ROUTING_NUMBER; i++) 
		{
			sprintf(static_routing, "static_routing_%02d", i);
			value = nvram_safe_get(static_routing);
	
			if ( strlen(value) == 0 ) 
				continue;
			else 
				strcpy(route_value, value);
	
			enable = strtok(route_value, "/");
			name = strtok(NULL, "/");
			dest_addr = strtok(NULL, "/");
			dest_mask = strtok(NULL, "/");
			gateway = strtok(NULL, "/");
			ifname = strtok(NULL, "/");
			metric = strtok(NULL, "/");
		
			if( !strcmp(enable, "1" ) && !strncmp(ifname, "WAN_PHY", 8))
			{	
				inet_aton(dest_addr, &rout_table[route_count].network);
				inet_aton(dest_mask, &rout_table[route_count].mask);
				route_count++;
			}
		}	
	}
#endif
/*Albert Chen: please do not remove this sleep, it will cause dail on demand fail */
	sleep(1);
	fp_wantimer = fopen(WANTIMER_PID, "r");
	if(fp_wantimer == NULL)
	{
		printf("********** wan_timer fail => stop wan monitor **********\n");
		return 0;
	}
	else
	{
		fclose(fp_wantimer);	
		printf("********** Monitor PPP Connection State ( In Dial on Demand Mode)**********\n");
	}

	for(;;) 
	{
		fp_monitor = fopen(WAN_MONITOR_PID, "r");
		
		if(fp_monitor == NULL)
		{
			fp_monitor = fopen(WAN_MONITOR_PID, "w+");
			fclose(fp_monitor);
#if defined(__uClinux__) && !defined(IP8000)
			if (vfork())
				_exit(0);				
#else
			if (fork())
				exit(0);				
#endif
		}
		else
			fclose(fp_monitor);
		
#ifdef RPPPOE		
		/* Russia need the information of WAN physical IP */
		if(strncmp(wan_proto, "pppoe", 5) == 0)
		{
			obtain_ip_done = 1;
		}
		
		if( russia_version && !obtain_ip_done)
		{
			if( use_dynamic_ip )
			{				
				if(tmp_ip = get_ipaddr(wan_eth))
					strcpy(wan_ip , tmp_ip);
				if(tmp_netmask = get_netmask(wan_eth))
					strcpy(wan_netmask , tmp_netmask);
			}
			else
			{		
				if(ppp_ip)
					strcpy(wan_ip , ppp_ip);
				if(ppp_netmask)
					strcpy(wan_netmask , ppp_netmask);
			}
			
			if( strlen(wan_ip) && strlen(wan_netmask))
			{
				DEBUG_MSG("RUSSIA: wan_ip=%s wan_netmask=%s\n", wan_ip, wan_netmask);
				obtain_ip_done = 1;
			}							
			else
				DEBUG_MSG("RUSSIA: wan_ip is empty\n");
		}
#endif
		
#if 0		
		if(connected)
			printf("monitor_main: WAN connected\n");
		else
			printf("monitor_main: WAN not connected\n");	
#endif
		
		if(connected) 
		{
			sleep(atoi(argv[1]));
			if( !strncmp(wan_proto,"pppoe", 5) ) 
			{
				fp_pppoe = fopen("/var/run/ppp0.pid", "r");
				if(!get_ipaddr("ppp0") && fp_pppoe==NULL)
				{
					printf("monitor_main: PPPoE ppp0 is disconnected\n");
					connected = 0;
					autoconfig = 0;
				}
				if(fp_pppoe)
					fclose(fp_pppoe);
			}
			else if( !strncmp(wan_proto, "pptp", 4) ) 		 
			{
				fp_pptp = fopen(PPTP_PID, "r");
				if(!get_ipaddr("ppp0") && fp_pptp==NULL)
				{
					printf("monitor_main: PPTP is disconnected\n");
					connected = 0;
				} 
				if(fp_pptp)
					fclose(fp_pptp);
			}
			else if( !strncmp(wan_proto, "l2tp", 4) ) 		 
			{
				fp_l2tp = fopen(L2TP_PID, "r");
				if(!get_ipaddr("ppp0") && fp_l2tp==NULL)
				{
					printf("monitor_main: L2TP is disconnected\n");
					connected = 0;
				}
				if(fp_l2tp)
					fclose(fp_l2tp);
			}
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/	
#ifdef CONFIG_USER_3G_USB_CLIENT	
			else if(strncmp(wan_proto, "usb3g", 5) == 0)	
			{
				fp_usb3g = fopen(USB_3G_PID, "r");
				if(!get_ipaddr("ppp0") && fp_usb3g==NULL)
				{
					printf("monitor_main: usb3g is disconnected\n");
					connected = 0;
				}
				if(fp_usb3g)
					fclose(fp_usb3g);
			}
#endif			
			if(check_flag )
			{
				kill(read_pid(WANTIMER_PID), SIGUSR1); /*start_idle_timer*/
				sleep(2);
				check_flag = 0;									
			}
	
			if(connected)
			{
#ifdef RPPPOE	
				not_LocalPacket = listen_socket(russia_version, obtain_ip_done, wan_ip, wan_netmask , route_count, lan_netmask, lan_ipaddr, lan_bridge);
#else
				not_LocalPacket = listen_socket(lan_netmask, lan_ipaddr, lan_bridge);
#endif
				/* if not_LocalPacket = 0, don't dial
				 * else if not_LocalPacket = 1, dial now for normal PPTP
				 * else if not_LocalPacket = 2, dial now for russia PPTP
				 * else if not_LocalPacket = 3, dial now for both (normal and russia)
				 */
				if( not_LocalPacket )
					kill(read_pid(WANTIMER_PID), SIGUSR2);/*reset_idle_timer*/
			} 
		} 
		else //no connected
		{ 
			if( !strncmp(wan_proto,"pppoe", 5) 
#ifdef CONFIG_USER_3G_USB_CLIENT
				|| !strncmp(wan_proto,"usb3g", 5)
#endif			
			) 
			{
				if( dial_on_demand_mode ) 
				{
					if(get_ipaddr("ppp0"))
					{
#ifdef CONFIG_USER_3G_USB_CLIENT
						printf("PPPoE or USB3G ppp0 is connected\n");
#else						
						printf("PPPoE ppp0 is connected\n");
#endif					
						connected = 1;
					}
				}
			} 
			else 
			{
				if(get_ipaddr("ppp0"))
				{
					printf("L2TP or PPTP ppp0 is connected\n");
					connected = 1;
				}
			}
				
			if(check_flag == 0 )
			{	
				check_flag = 1;
				kill(read_pid(WANTIMER_PID), SIGINT);/*stop_idle_timer*/
				sleep(2);
			}

			if(!connected)
			{
#ifdef RPPPOE
				not_LocalPacket = listen_socket(russia_version, obtain_ip_done, wan_ip, wan_netmask,route_count,lan_netmask, lan_ipaddr, lan_bridge);
#else
				not_LocalPacket = listen_socket(lan_netmask, lan_ipaddr, lan_bridge);
#endif	
				/* if not_LocalPacket = 0, don't dial
				 * else if not_LocalPacket = 1, dial now for normal PPTP
				 * else if not_LocalPacket = 2, dial now for russia PPTP
				 * else if not_LocalPacket = 3, dial now for both (normal and russia)
				 */
				if( not_LocalPacket ) 
				{
					if( !strncmp(wan_proto,"l2tp", 4) ) 
					{
						printf("monitor: Dial L2TP\n");
						stop_l2tp();
						sleep(1);
						start_l2tp();
						if (get_ipaddr("ppp0")) 	connected = 1;
					} 
					else if( !strncmp(wan_proto,"pptp", 4) ) 
					{
						printf("monitor: Dial PPTP\n");
						stop_pptp();
						sleep(1);
						start_pptp();
						sleep(5);
						if (get_ipaddr("ppp0")) 	connected = 1;
					} 
					else if( !strncmp(wan_proto,"pppoe", 5) ) 
					{
						printf("monitor: Dial PPPoE\n");
						stop_pppoe(0);
						sleep(5);
						start_pppoe(0, wan_eth, sn, ac);												
						if (get_ipaddr("ppp0")) 	connected = 1;

						if(nvram_match("ipv6_wan_proto", "ipv6_autodetect") == 0 && autoconfig != -1){
retry:
						sleep(5);
						read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip));
						read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));
						if(strlen(ppp6_ip) < 1 || strlen(lan_ip) < 1){
							if(autoconfig == 4){
								printf("IPv6 PPPoE share with IPv4 is Failed, Change to Autoconfiguration Mode\n");
								system("cli ipv6 rdnssd stop");
								system("cli ipv6 autoconfig stop");
								_system("cli ipv6 rdnssd start %s", nvram_safe_get("wan_eth"));
								autoconfig = -1;
							}else{
								autoconfig ++;
								goto retry;
							}
						}else
							autoconfig = 0;
		}
					}
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/	
#ifdef CONFIG_USER_3G_USB_CLIENT	
					else if(strncmp(wan_proto, "usb3g", 5) == 0)	
					{
						printf("monitor: Dial usb3g\n");
						/*
							Date: 2009-2-4
							Name: Nick Chou
							Reason: we do not rmmove "option.ko" and "usbserial.ko" 
							        to prevet "Connect script failed"
							        So we only kill pppd here to prevent "re-dial" error.
						*/
						KILL_APP_ASYNCH("pppd");//stop_usb3g();
						sleep(2);
						start_usb3g();
						connected = 1;
					}	
#endif							
					/* wait for connect */
					sleep(5);
				}
			}
		}
	}
	return 0;
}

#else //#ifndef MPPPOE

void reset_mpppoe_timer(int ifname)
{
	cprintf("IN monitor\n");
	init_file(PPP_RESET_00);
        init_file(PPP_RESET_01);
        init_file(PPP_RESET_02);
        init_file(PPP_RESET_03);
        init_file(PPP_RESET_04);

	switch (ifname)
        {
                case 0://for ppp0
			cprintf("reset ppp0\n");
			if(ppp0_file_flag)
			{
				save2file(PPP_RESET_00,"reset\n");
				ppp0_file_flag = 0;
			}
                        break;
                case 1://for ppp1
			cprintf("reset ppp1\n");
			save2file(PPP_RESET_01,"reset\n");
                        break;
                case 2://for ppp2
			cprintf("reset ppp2\n");
			save2file(PPP_RESET_02,"reset\n");
                        break;
                case 3://for ppp3
			cprintf("reset ppp3\n");
			save2file(PPP_RESET_03,"reset\n");
                        break;
                case 4://for ppp4
			cprintf("reset ppp4\n");
			save2file(PPP_RESET_04,"reset\n");
                        break;
        }
}

void start_mpppoe_timer(int ifname)
{
	init_file(PPP_START_IDLE_00);
	init_file(PPP_START_IDLE_01);
	init_file(PPP_START_IDLE_02);
	init_file(PPP_START_IDLE_03);
	init_file(PPP_START_IDLE_04);

	switch (ifname)
	{
		case 0://for ppp0
			save2file(PPP_START_IDLE_00,"start\n");
			break;
		case 1://for ppp1
			save2file(PPP_START_IDLE_01,"start\n");
			break;
		case 2://for ppp2
			save2file(PPP_START_IDLE_02,"start\n");
                        break;
                case 3://for ppp3
			save2file(PPP_START_IDLE_03,"start\n");
                        break;
		case 4://for ppp4
			save2file(PPP_START_IDLE_04,"start\n");
                        break;
	}
}

void execute_mpppoe(int ifname)
{
	stop_pppoe(ifname);
    sleep(2);
	start_pppoe(ifname);
    start_mpppoe_timer(ifname);
}

//For MPPPOE
int monitor_main(int argc, char** argv) 
{
        int fd, bytes, ifindex=0;
        int ppp_ifunit=0, pppoe_disconnect=0, connected =0, LocalPacket=1, i;
        char pppoe_type[] = "wan_pppoe_connect_mode_XX";
        char pppoe_enable[] = "wan_pppoe_enable_XX";
        char ppp_con[] = "pppXX";
        char process_name[32] = {};
        char route_value[256]={};
        char multiple_routing[] = "multiple_routing_table_XYXXXXX";
        char connect_mode[] = "wan_pppoe_connect_mode_XYXXXXX";
        char *interface_name,*type,*content;
        char *wan_proto,*value;
        struct EthPacket packet;
        struct in_addr saddr, daddr, netmask, myaddr, multicast_mask;
        fd_set rfds;

        inet_aton(nvram_safe_get("lan_netmask"), &netmask);
        inet_aton(nvram_safe_get("lan_ipaddr"), &myaddr);
        inet_aton("255.0.0.0", &multicast_mask);

        ifindex = show_eth_ip_address(nvram_safe_get("lan_bridge"),ifindex);
        wan_proto = nvram_get("wan_proto");

        for(;;)
        {

                sleep(atoi(argv[1]));
                FD_ZERO(&rfds);
                fd = raw_socket(ifindex);

                if(fd < 0)
                {
                        DEBUG_MSG("FATAL: couldn't listen on socket\n");
                        return -1;
                }
                else
                {
                        FD_SET(fd, &rfds);
                        memset(&packet, 0, sizeof(struct EthPacket));
                        bytes = read(fd, &packet, sizeof(struct EthPacket));

                        if(bytes > 0)
                        {
                                saddr.s_addr = (packet.ip.saddr);
                                daddr.s_addr = (packet.ip.daddr);
                                /* Router forward packet to wan interface */
                                if( (netmask.s_addr & myaddr.s_addr) != (netmask.s_addr & daddr.s_addr))
                                {

                                        /* Multicast Packet */
                                        /* Big Endian */
                                        if( ((daddr.s_addr & multicast_mask.s_addr)) >=224 && ((daddr.s_addr & multicast_mask.s_addr) ) <=240 )
                                                LocalPacket = 1;
                                        else
                                                LocalPacket = 0;


                                        /* DNS query */
                                        if( myaddr.s_addr == daddr.s_addr && htons(packet.dport)==53 )
                                        {
                                                cprintf("DNS Query from %s\n", inet_ntoa(saddr));
                                                LocalPacket = 0;
                                        }
                                }
                        }

                        if(!LocalPacket)
                        {
                                if(strncmp(wan_proto,"pppoe", 5) == 0)
                                {
                                        for(i=0; i<MAX_MULTIPLE_ROUTING_NUMBER; i++)
                                        {
                                                memset(route_value,0,sizeof(route_value));
                                                memset(process_name,0,sizeof(process_name));
                                                sprintf(multiple_routing, "multiple_routing_table_%02d", i);
                                                value = nvram_safe_get(multiple_routing);
                                                if ( strlen(value) == 0 )
                                                        break;
                                                else
                                                        strcpy(route_value, value);

                                                interface_name = strtok(route_value, "/");
                                                type = strtok(NULL, "/");
                                                content = strtok(NULL, "/");
                                                interface_name = interface_name + 3;
                                                sprintf(process_name,"pppd linkname %02d",atoi(interface_name));
                                                sprintf(connect_mode,"wan_pppoe_connect_mode_%02d",atoi(interface_name));
                                                if(strcmp(type,"0") == 0 && strcmp(inet_ntoa(saddr),content) == 0 && nvram_match(connect_mode,"on_demand") == 0)
                                                {
                                                        if(checkServiceAlivebyName(process_name) == 0)
                                                        	execute_mpppoe(atoi(interface_name)); 
							reset_mpppoe_timer(atoi(interface_name));
							sleep(2);
                                                }
                                                if(strcmp(type,"1") == 0 && strcmp(inet_ntoa(daddr),content) == 0 && nvram_match(connect_mode,"on_demand") == 0)
                                                {
                                                        if(checkServiceAlivebyName(process_name) == 0)
								execute_mpppoe(atoi(interface_name));
							reset_mpppoe_timer(atoi(interface_name));
							sleep(2);
                                                }
                                                /* not implement type = 2 (domain name)*/
                                                if(strcmp(type,"3") == 0 && (htons(packet.dport) == atoi(content)) && nvram_match(connect_mode,"on_demand") == 0)
                                                {
                                                        if(checkServiceAlivebyName(process_name) == 0)
								execute_mpppoe(atoi(interface_name));
							reset_mpppoe_timer(atoi(interface_name));
							sleep(2);
                                                }
                                        }
                                        // not match type = 0 - 3 , dial up main ppp session
                                        memset(process_name,0,sizeof(process_name));
                                        memset(connect_mode,0,sizeof(connect_mode));
                                        sprintf(connect_mode,"wan_pppoe_connect_mode_%02d",atoi(nvram_safe_get("wan_pppoe_main_session")));
                                        sprintf(process_name,"pppd linkname %02d",atoi(nvram_safe_get("wan_pppoe_main_session")));
                                        if(checkServiceAlivebyName(process_name) == 0 && nvram_match(connect_mode,"on_demand") == 0)
						execute_mpppoe(atoi(nvram_safe_get("wan_pppoe_main_session")));
					ppp0_file_flag = 1;
					reset_mpppoe_timer(atoi(nvram_safe_get("wan_pppoe_main_session")));
					sleep(2);
                                }
                        }
                        LocalPacket = 1;
                }
                if(fd)
                        close(fd);
        }
}

#endif //#ifndef MPPPOE

void write_pid_to_var(void)
{
	int pid;
	FILE *fp;
	char temp[8];
	pid = getpid();
	sprintf(temp,"%d\n",pid);

	if((fp = fopen(WAN_MONITOR_PID,"w")) != NULL)
	{
		fputs(temp,fp);
	}

	fclose(fp);
}

int redial_main(int argc, char **argv)
{
	int need_redial = 0;
	int status;
	pid_t pid;
	int ppp_ifunit=0;
	char ppp_con[] = "pppXX";
	int pppoe_disconnect=0;
	char *wan_eth=NULL, *wan_proto=NULL;
	char ac_name[128]={0}, service_name[128]={0};
	char *ac=NULL, *sn=NULL;
	FILE *fp_pppoe, *fp_pptp, *fp_l2tp;
	int pptp_need_redial_count = 0;
	int pppoe_need_redial_count = 0;
	int l2tp_need_redial_count = 0;

	char ppp6_ip[MAX_IPV6_IP_LEN]={};
	char lan_ip[MAX_IPV6_IP_LEN]={};
	int autoconfig = 0;
	int ipv4_state = 0; //0:disconnect 1:connect
	
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/	
#ifdef CONFIG_USER_3G_USB_CLIENT		
	FILE *fp_usb3g;
	int usb3g_need_redial_count = 0;
#endif
	
#ifndef MPPPOE	
	wan_proto = argv[2];
	if(strncmp(wan_proto, "pppoe", 5) == 0)
	{	
		wan_eth = argv[3];
		strcpy(service_name, argv[4]);
		strcpy(ac_name, argv[5]);
		sn = parse_special_char(service_name);
		ac = parse_special_char(ac_name);
	}
#else
    char process_name[32] = {};
    char pppoe_type[] = "wan_pppoe_connect_mode_XX";
	char pppoe_enable[] = "wan_pppoe_enable_XX";
#endif //#ifndef MPPPOE
	
	for(;;)
	{
/* Ubicom: Modified 20090826
	avoid generating multiple redial processes
#ifdef __uClinux__
		if (vfork()) exit(0);
#else
		if (fork()) exit(0);
#endif
*/
		/*NickChou, move to the end, 
		  because when WAN in always_on mode, we using redial deamon to dial ppp.
		sleep(atoi(argv[1])); 
		*/
		
		if(strncmp(wan_proto, "pppoe", 5)==0 ) 
		{
			pppoe_disconnect=0;
			for (ppp_ifunit=0; ppp_ifunit < MAX_PPPOE_CONNECTION; ppp_ifunit ++) 
			{
#ifndef MPPPOE					
				if(!get_ipaddr("ppp0"))
				{
					printf("redial_main: ppp0 IP = NULL\n");
					fp_pppoe = fopen("/var/run/ppp0.pid", "r");
					if(fp_pppoe==NULL || pppoe_need_redial_count == 2)
					{
						printf("redial_main: PPPoE need_redial\n");
						need_redial = 1;
						pppoe_need_redial_count = 0;
						//form connect to disconnect
						if(ipv4_state == 1 && nvram_match("ipv6_wan_proto", "autodetect") == 0){ 
							autoconfig = 0;
							ipv4_state = 0;
						}
					}
					else
					{
						printf("redial_main: PPPoE deaman is running : %d\n", pppoe_need_redial_count);
						pppoe_need_redial_count++;
						fclose(fp_pppoe);	
					}
				}else if(ipv4_state == 0){
					ipv4_state = 1;
					autoconfig = 0;
				}
#else //#ifndef MPPPOE
				sprintf(ppp_con, "ppp%d", ppp_ifunit);		
				sprintf(pppoe_enable, "wan_pppoe_enable_%02d", ppp_ifunit);
				sprintf(pppoe_type, "wan_pppoe_connect_mode_%02d", ppp_ifunit);
				sprintf(ppp_con, "ppp%d", ppp_ifunit);
				memset(process_name,0,sizeof(process_name));
				sprintf(process_name,"pppd linkname %02d",ppp_ifunit);

				if( nvram_match(pppoe_enable, "1") == 0 ) 
				{
					if(nvram_match(pppoe_type,"always_on") == 0) {
						if(!get_ipaddr(ppp_con) && checkServiceAlivebyName(process_name) == 0){
							need_redial = 1;
						    printf("redial_main: PPPoE need_redial\n");
							pppoe_disconnect += 0x1<<ppp_ifunit;
							DEBUG_MSG("pppoe %d need redial\n", ppp_ifunit);
						}
					}
				}
#endif //#ifndef MPPPOE	
			}	
		}
		else if(strncmp(wan_proto, "pptp", 4) ==0 ) 		 
		{
			/* ken added to clean wan addrs for wrong source IP for PPTP / L2TP / Russia PPTP/PPPoE/L2TP when Dynamic IP chang to Static IP */
			sleep(8);
			if(!get_ipaddr("ppp0"))
			{
				printf("redial_main: ppp0 IP = NULL nvram_test: %s\n",
				nvram_safe_get("wan_proto"));
				fp_pptp = fopen(PPTP_PID, "r");
				if(fp_pptp==NULL || pptp_need_redial_count == 2)
				{
					printf("redial_main: PPTP need_redial\n");
					need_redial = 1;
					pptp_need_redial_count = 0;
				}
				else
				{
					printf("redial_main: PPTP deaman is running : %d\n", pptp_need_redial_count);
					pptp_need_redial_count++;
					fclose(fp_pptp);	
				}		
			}
		}
		else if(strncmp(wan_proto, "l2tp", 4) ==0 ) 		 
		{
			/* ken added to clean wan addrs for wrong source IP for PPTP / L2TP / Russia PPTP/PPPoE/L2TP when Dynamic IP chang to Static IP */
			sleep(8);
			if(!get_ipaddr("ppp0"))
			{
				printf("redial_main: ppp0 IP = NULL\n");
				fp_l2tp = fopen(L2TP_PID, "r");
				if(fp_l2tp==NULL || l2tp_need_redial_count == 2)
				{
					printf("redial_main: L2TP need_redial\n");
					need_redial = 1;
					l2tp_need_redial_count = 0;
				}
				else
				{
					printf("redial_main: L2TP deaman is running : %d\n", l2tp_need_redial_count);
					l2tp_need_redial_count++;
					fclose(fp_l2tp);	
				}	
			}
		}
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/		
#ifdef CONFIG_USER_3G_USB_CLIENT	
		else if(strncmp(wan_proto, "usb3g", 5) ==0 ) 		 
		{
			/* ken added to clean wan addrs for wrong source IP for PPTP / L2TP / Russia PPTP/PPPoE/L2TP when Dynamic IP chang to Static IP */
			sleep(8);
			if(!get_ipaddr("ppp0"))
			{
				printf("redial_main: usb3g ppp0 IP = NULL\n");
				fp_usb3g = fopen(USB_3G_PID, "r");
				if(fp_usb3g==NULL || usb3g_need_redial_count == 2)
				{
					printf("redial_main: usb3g need_redial\n");
					need_redial = 1;
					usb3g_need_redial_count = 0;
				}
				else
				{
					printf("redial_main: usb3g deaman is running : %d\n", usb3g_need_redial_count);
					usb3g_need_redial_count++;
					fclose(fp_usb3g);	
				}	
			}
		}
#endif				
		if(need_redial)
		{
#if defined(__uClinux__) && !defined(IP8000)
		/*
		 * FIXME:
		 * program can not get nvram from /dev/nvram sytle
		 * after kill pppd...(NOTE: frist time to vfork() is ok)
		 *
		 * it seems to a be stack issues by vfork(), but still not
		 * find the root cause yet...
		 *
		 * I find to get any key from nvram before vfork() can avoid
		 * this problem...??????
		 *
 		 * XXX: whitout this line [nvram_safe_get("wan_proto")],
 		 * child protocess [vfork()] can not get nvram anymore...
 		 *
 		 * this case is found by pptp protocol, how about l2tp?
 		 * */
			printf("Help me: wan_proto %s redial have a nvram bug there!\n",
				nvram_safe_get("wan_proto"));
			pid = vfork();
#else
			pid = fork();
#endif
		/*
		 * FIXME: if we have no call nvram_safe_get above, we can not
		 * get nvram anymore from now on..., both parent and child
		 * */
			switch(pid)
			{
				case -1:
					perror("fork failed");
#if defined(__uClinux__) && !defined(IP8000)
					_exit(1);
#else    	
					exit(1);
#endif					
				case 0:
					if(strncmp(wan_proto, "pppoe", 5) == 0 ){
						DEBUG_MSG("redial pppoe\n");	
#ifdef MPPPOE						
						for(ppp_ifunit = 0; ppp_ifunit< MAX_PPPOE_CONNECTION; ppp_ifunit++) 
						{
							if(  ( pppoe_disconnect & (0x1<<ppp_ifunit ) ) ) 
							{
								DEBUG_MSG("redial pppoe %d\n", ppp_ifunit);
								stop_pppoe(ppp_ifunit);
								sleep(2);
								start_pppoe(ppp_ifunit);
							}
						}
#else
						stop_pppoe(0);
						sleep(1);
						start_pppoe(0, wan_eth, sn, ac);
#endif						
					} else if(strncmp(wan_proto, "pptp", 4) == 0 ){
						DEBUG_MSG("redial pptp\n");
						stop_pptp();
						sleep(1);
						start_pptp();
					} else if(strncmp(wan_proto, "l2tp", 4) == 0 ){
						DEBUG_MSG("redial l2tp\n");
						stop_l2tp();
						sleep(1);
						start_l2tp();
					}
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/		
#ifdef CONFIG_USER_3G_USB_CLIENT	
					else if(strncmp(wan_proto, "usb3g", 5) ==0 ) {
						DEBUG_MSG("redial usb3g\n");
						/*
							Date: 2009-2-4
							Name: Nick Chou
							Reason: we do not rmmove "option.ko" and "usbserial.ko" 
							        to prevet "Connect script failed"
							        So we only kill pppd here to prevent "re-dial" error.
						*/
						KILL_APP_ASYNCH("pppd");//stop_usb3g();
						sleep(2);
						start_usb3g();
					}
#endif
#if defined(__uClinux__) && !defined(IP8000)
					_exit(0);
#else
					exit(0);
#endif
					break;
				default:
					waitpid(pid, &status, 0);
					break;
			}
			need_redial = 0;
		} 		
		sleep(atoi(argv[1]));//NickChou

		if(nvram_match("wan_proto", "pppoe") == 0 && autoconfig != -1 &&
			nvram_match("ipv6_wan_proto", "ipv6_autodetect") == 0){
			read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip));
			read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, lan_ip, sizeof(lan_ip));
			if(strlen(ppp6_ip) < 1 || strlen(lan_ip) < 1){
				if(autoconfig == 4){	
					printf("IPv6 PPPoE share with IPv4 if Failed, Change to Autoconfiguration Mode\n");
					system("cli ipv6 rdnssd stop");
					system("cli ipv6 autoconfig stop");
					_system("cli ipv6 rdnssd start %s", nvram_safe_get("wan_eth"));
					autoconfig = -1;
				}else{
					autoconfig ++;
				}
			}else
				autoconfig = 0 ;
		}
	}
}

#ifdef MPPPOE
void reload_nvram(int sig)
{
        ppp0_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_00")) * 60;
        ppp1_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_01")) * 60;
        ppp2_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_02")) * 60;
        ppp3_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_03")) * 60;
        ppp4_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_04")) * 60;
        return ;
}

void check_start_flag(int ifname)
{
	char *file_path;
        FILE *fp;
        char buf[16] = {};

        switch(ifname)
        {
                case 0:
                        file_path = PPP_START_IDLE_00;
                        break;
                case 1:
                        file_path = PPP_START_IDLE_01;
                        break;
                case 2:
                        file_path = PPP_START_IDLE_02;
                        break;
                case 3:
                        file_path = PPP_START_IDLE_03;
                        break;
                case 4:
                        file_path = PPP_START_IDLE_04;
                        break;
        }

        fp = fopen(file_path,"r");

        if(fp)
        {
                if(fgets(buf,16,fp))
                {
                        if(strncmp(buf,"start",5) == 0)
                        {
                                switch(ifname)
                                {
                                        case 0:
                                                ppp0_timer_flag = 1;
                                                break;
                                        case 1:
                                                ppp1_timer_flag = 1;
                                                break;
                                        case 2:
                                                ppp2_timer_flag = 1;
                                                break;
                                        case 3:
                                                ppp3_timer_flag = 1;
                                                break;
                                        case 4:
                                                ppp4_timer_flag = 1;
                                                break;
                                }
                        }
                }
                fclose(fp);
	}
	else
		return;
}
void check_stop_flag(int ifname)
{
	char *file_path;
	FILE *fp;
	char buf[16] = {};
	
	switch(ifname)
	{
		case 0:
			file_path = PPP_IDLE_00;
			break;
		case 1:
                	file_path = PPP_IDLE_01;
			break;
		case 2:
                	file_path = PPP_IDLE_02;
			break;
		case 3:
                	file_path = PPP_IDLE_03;
			break;
		case 4:
                	file_path = PPP_IDLE_04;
			break;
	}
	
	fp = fopen(file_path,"r");

	if(fp)
	{
		if(fgets(buf,16,fp))
		{
			if(strncmp(buf,"stop",4) == 0)
			{
				switch(ifname)
			        {
                			case 0:
              					ppp0_timer_flag = 0;
                        			break;
                			case 1:
                        			ppp1_timer_flag = 0;
                        			break;
                			case 2:
						ppp2_timer_flag = 0;
                        			break;
                			case 3:
						ppp3_timer_flag = 0;
                        			break;
                			case 4:
						ppp4_timer_flag = 0;
                        			break;
        			}
			}	
		}
		fclose(fp);
	}
	else
		return;
}
void check_reset_flag(int ifname)
{
	char *file_path;
        FILE *fp;
        char buf[16] = {};

        switch(ifname)
        {
                case 0:
                        file_path = PPP_RESET_00;
                        break;
                case 1:
                        file_path = PPP_RESET_01;
                        break;
                case 2:
                        file_path = PPP_RESET_02;
                        break;
                case 3:
                        file_path = PPP_RESET_03;
                        break;
                case 4:
                        file_path = PPP_RESET_04;
                        break;
        }

        fp = fopen(file_path,"r");

        if(fp)
        {
                if(fgets(buf,16,fp))
                {
                        if(strncmp(buf,"reset",5) == 0)
                        {
                                switch(ifname)
                                {
                                        case 0:
                                                ppp0_timer_count = 0;
                                                break;
                                        case 1:
                                                ppp1_timer_count = 0;
                                                break;
                                        case 2:
                                                ppp2_timer_count = 0;
                                                break;
                                        case 3:
                                                ppp3_timer_count = 0;
                                                break;
                                        case 4:
                                                ppp4_timer_count = 0;
                                                break;
                                }
                        }
		}
		fclose(fp);
	}
	else
		return;
}

int ppptimer_main(int argc, char **argv)
{
	ppp0_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_00")) * 60;
        ppp1_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_01")) * 60;
        ppp2_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_02")) * 60;
        ppp3_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_03")) * 60;
        ppp4_idle_time = atoi(nvram_safe_get("wan_pppoe_max_idle_time_04")) * 60;

	while(1)
	{
		cprintf("IN ppptimer\n");
		signal(SIGSYS, reload_nvram);
		
		if(ppp0_timer_flag && !nvram_match("wan_pppoe_connect_mode_00","on_demand") && get_ipaddr("ppp0"))
                {
			check_start_flag(0);
                        cprintf("ppp0_timer_count = %d\nppp0_idle_time = %d\n",ppp0_timer_count,ppp0_idle_time);
                        ppp0_timer_count++;
			if(ppp0_file_flag == 0)
				check_reset_flag(0);
			check_stop_flag(0);
                        if((ppp0_idle_time > 0) && (ppp0_timer_count >= ppp0_idle_time))
                        {
                                stop_pppoe(0);
                                ppp0_timer_flag = 1;
                                ppp0_timer_count = 0;
                        }
			init_file(PPP_RESET_00);
                }
		if(ppp1_timer_flag && !nvram_match("wan_pppoe_connect_mode_01","on_demand") && get_ipaddr("ppp1"))
                {
			check_start_flag(1);
                        cprintf("ppp1_timer_count = %d\nppp1_idle_time = %d\n",ppp1_timer_count,ppp1_idle_time);
                        ppp1_timer_count++;
			check_reset_flag(1);
			check_stop_flag(1);
                        if((ppp1_idle_time > 0) && (ppp1_timer_count >= ppp1_idle_time))
                        {
                                stop_pppoe(1);
                                ppp1_timer_flag = 1;
                                ppp1_timer_count = 0;
                        }
			init_file(PPP_RESET_01);
                }
		if(ppp2_timer_flag && !nvram_match("wan_pppoe_connect_mode_02","on_demand") && get_ipaddr("ppp2"))
                {
			check_start_flag(2);
                        cprintf("ppp2_timer_count = %d\nppp2_idle_time = %d\n",ppp2_timer_count,ppp2_idle_time);
                        ppp2_timer_count++;
			check_reset_flag(2);
			check_stop_flag(2);
                        if((ppp2_idle_time > 0) && (ppp2_timer_count >= ppp2_idle_time))
                        {
                                stop_pppoe(2);
                                ppp2_timer_flag = 1;
                                ppp2_timer_count = 0;
                        }
			init_file(PPP_RESET_02);
                }
		if(ppp3_timer_flag && !nvram_match("wan_pppoe_connect_mode_03","on_demand") && get_ipaddr("ppp3"))
                {
			check_start_flag(3);
                        cprintf("ppp3_timer_count = %d\nppp3_idle_time = %d\n",ppp3_timer_count,ppp3_idle_time);
                        ppp3_timer_count++;
			check_reset_flag(3);
			check_stop_flag(3);
                        if((ppp3_idle_time > 0) && (ppp3_timer_count >= ppp3_idle_time))
                        {
                                stop_pppoe(3);
                                ppp3_timer_flag = 1;
                                ppp3_timer_count = 0;
                        }
			init_file(PPP_RESET_03);
                }
		if(ppp4_timer_flag && !nvram_match("wan_pppoe_connect_mode_04","on_demand") && get_ipaddr("ppp4"))
                {
			check_start_flag(4);
                        cprintf("ppp4_timer_count = %d\nppp4_idle_time = %d\n",ppp4_timer_count,ppp4_idle_time);
                        ppp4_timer_count++;
			check_reset_flag(4);
			check_stop_flag(4);
                        if((ppp4_idle_time > 0) && (ppp4_timer_count >= ppp4_idle_time))
                        {
                                stop_pppoe(4);
                                ppp4_timer_flag = 1;
                                ppp4_timer_count = 0;
                        }
			init_file(PPP_RESET_04);
                }		
                sleep(1);
	}
}

int start_ppptimer(void)
{
        _system("/var/sbin/ppptimer &");
        return 0;
}

int stop_ppptimer(void)
{
		KILL_APP_SYNCH("ppptimer");
        return 0;
}
#endif //MPPPOE

int start_monitor(int ppp_ifunit, char *wan_proto, char *wan_eth)
{
	char wan_info[128] = {0};
	char ppp_info[64] = {0};
	int  dial_on_demand_mode = 0;
	char *service_name = NULL, pppoe_service_name[20] = {0}, sn[128] = {0};
	char *pptp_ip=NULL, *pptp_netmask=NULL;
	char *l2tp_ip=NULL, *l2tp_netmask=NULL;
    int  use_dynamic_ip = 0;
//Albert add 20081024
//---
	char *ac_name = NULL;
	char pppoe_ac_name[20] = {0};
	char ac[128] = {0};
//---	    
#ifdef RPPPOE		
	int  russia_version = 0; 
	char *russia_ipaddr=NULL, *russia_netmask=NULL;
#endif 

	sprintf(wan_info, "%s/%s/%s/%s/%s", wan_proto, wan_eth, nvram_safe_get("lan_netmask"), 
				nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_bridge"));


	if( strncmp(wan_proto, "pptp", 4) == 0)
	{
#ifdef RPPPOE		
		if( nvram_match("wan_pptp_russia_enable", "1") == 0)
			russia_version = 1;
#endif				
		if(nvram_match("wan_pptp_connect_mode", "on_demand") == 0)
			dial_on_demand_mode = 1;
			
		if( nvram_match("wan_pptp_dynamic", "1") == 0)
		{
			pptp_ip = "0.0.0.0";
			pptp_netmask = "0.0.0.0";
			use_dynamic_ip = 1;
		}	
		else
		{
			pptp_ip = nvram_safe_get("wan_pptp_ipaddr");		
			pptp_netmask = nvram_safe_get("wan_pptp_netmask");
		}
		
#ifdef RPPPOE
		sprintf(ppp_info, "%d/%d/%d/%s/%s", dial_on_demand_mode, russia_version, use_dynamic_ip, 
							pptp_ip, pptp_netmask);	
#else
		sprintf(ppp_info, "%d/%d/%s/%s", dial_on_demand_mode, use_dynamic_ip, 
							pptp_ip, pptp_netmask);	
#endif									
	}
	else if( strncmp(wan_proto, "pppoe", 5) == 0)
	{
#ifdef RPPPOE		
        if( nvram_match("wan_pppoe_russia_enable", "1") == 0)
			russia_version = 1;
			
		if( nvram_match("wan_pppoe_russia_dynamic", "1") == 0)
		{
			russia_ipaddr = "0.0.0.0";
			russia_netmask = "0.0.0.0";
			use_dynamic_ip = 1;
		}	
		else
		{
			russia_ipaddr = nvram_safe_get("wan_pppoe_russia_ipaddr");
			russia_netmask = nvram_safe_get("wan_pppoe_russia_netmask");
		}			
#endif
		if(nvram_match("wan_pppoe_connect_mode_00", "on_demand") == 0)
			dial_on_demand_mode = 1;
		
		sprintf(pppoe_service_name, "wan_pppoe_service_%02d", ppp_ifunit); 
		service_name = nvram_safe_get(pppoe_service_name);
		if(strcmp(service_name, "") == 0)
			strncpy(sn, "null_sn", strlen("null_sn"));
		else
			strncpy(sn, service_name, strlen(service_name));	

		sprintf(pppoe_ac_name, "wan_pppoe_ac_%02d", ppp_ifunit); 
		ac_name = nvram_safe_get(pppoe_ac_name);
		if(strcmp(ac_name, "") == 0)
			strncpy(ac, "null_ac", strlen("null_ac"));
		else
			strncpy(ac, ac_name, strlen(ac_name));

#ifdef RPPPOE			
		sprintf(ppp_info, "%d/%d/%d/%s/%s", dial_on_demand_mode, russia_version, use_dynamic_ip,	
					russia_ipaddr, russia_netmask);														
#else	
		sprintf(ppp_info, "%d", dial_on_demand_mode);					
#endif	
	}
	else if(strncmp(wan_proto, "l2tp", 4) == 0)
	{
#ifdef RPPPOE		
		if( nvram_match("wan_l2tp_russia_enable", "1") == 0)
			russia_version = 1;
#endif			
		if(nvram_match("wan_l2tp_connect_mode", "on_demand") == 0)
			dial_on_demand_mode = 1;
			
		if( nvram_match("wan_l2tp_dynamic", "1") == 0)
		{
			l2tp_ip = "0.0.0.0";
			l2tp_netmask = "0.0.0.0";
			use_dynamic_ip = 1;
		}	
		else
		{
			l2tp_ip = nvram_safe_get("wan_l2tp_ipaddr");		
			l2tp_netmask = nvram_safe_get("wan_l2tp_netmask");
		}
			
#ifdef RPPPOE
		sprintf(ppp_info, "%d/%d/%d/%s/%s", dial_on_demand_mode, russia_version, use_dynamic_ip, 
							l2tp_ip, l2tp_netmask);	
#else
		sprintf(ppp_info, "%d/%d/%s/%s", dial_on_demand_mode, use_dynamic_ip, 
							l2tp_ip, l2tp_netmask);	
#endif					
	}
/*
* 	Date: 2009-1-19
* 	Name: Ken_Chiang
* 	Reason: modify for 3g usb client card to used.
* 	Notice :
*/	
#ifdef CONFIG_USER_3G_USB_CLIENT	
	else if(strncmp(wan_proto, "usb3g", 5) == 0)
	{
		if(nvram_match("usb3g_reconnect_mode", "on_demand") == 0)
			dial_on_demand_mode = 1;		
		sprintf(ppp_info, "%d", dial_on_demand_mode);			
	}
#endif	
	
	DEBUG_MSG("star_monitor: wan_info = %s\n", wan_info);
	DEBUG_MSG("star_monitor: ppp_info = %s\n", ppp_info);

	_system("/var/sbin/monitor 5 %s %s \"%s\" \"%s\" &", 
		wan_info, ppp_info, parse_special_char(sn), parse_special_char(ac));

	return 0;
}

int stop_monitor(void)
{
	KILL_APP_ASYNCH("monitor");
	unlink("/var/run/monitor.pid");
	return 0;
}

int start_redial(char *wan_proto, char *wan_eth, char *service_name, char *ac_name )
{
	char sn[128]={0};
	char ac[128]={0};
	
	strcpy(sn, service_name);
	strcpy(ac, ac_name);
	
	if(strncmp(wan_proto, "pppoe", 5) ==0)
	{
		_system("/var/sbin/redial 5 %s %s \"%s\" \"%s\" &", 
			wan_proto, wan_eth, parse_special_char(sn), parse_special_char(ac));
	}
	else
		_system("/var/sbin/redial 5 %s &", wan_proto);
		
	return 0;
}

int stop_redial(void)
{
	KILL_APP_ASYNCH("redial");
	return 0;
}
