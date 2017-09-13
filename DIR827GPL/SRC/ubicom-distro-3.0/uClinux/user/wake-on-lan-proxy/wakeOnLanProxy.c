#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <errno.h>

//#define WOL_DEBUG

#ifdef WOL_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/* buf need to send */
//u_char outpack[150];
//int private_port;

/* A multi-family sockaddr. */
typedef union {
	struct sockaddr sa;
	struct sockaddr_in sa_in;
} usockaddr;

struct SubnetList {
	int value;
	int zero_count; 	
	int default_ip;
};
struct SubnetList subnet_list_t[] = {
	{255,0},
	{254,1},
	{252,2},
	{248,3},
	{240,4},
	{224,5},
	{192,6},
	{128,7},
	{0,8}
};

static char *get_lan_broadcast_ip(char *lan_ip, char *subnet_mask){
	int lanip[4];
	int lan_subnet[4];
	int lan_broadcast_ip[4];
	static char lan_broadcast_ip_str[16];
	int i=0,j=0,k=0;
	
	
	memset(lanip,0,sizeof(lanip));
	memset(lan_subnet,0,sizeof(lan_subnet));
	memset(lan_broadcast_ip,0,sizeof(lan_broadcast_ip));
	memset(lan_broadcast_ip_str,0,sizeof(lan_broadcast_ip_str));
	
	DEBUG_MSG("get_lan_broadcast_ip: lan_ip=%s, subnet_mask=%s\n", lan_ip, subnet_mask);
	
	sscanf(lan_ip,"%i.%i.%i.%i",&lanip[0],&lanip[1],&lanip[2],&lanip[3]);
	sscanf(subnet_mask,"%i.%i.%i.%i",&lan_subnet[0],&lan_subnet[1],&lan_subnet[2],&lan_subnet[3]);

	for(k=0;k<4;k++){
		for(i=0;i<9;i++){			
			if(lan_subnet[k] == subnet_list_t[i].value){
				lan_broadcast_ip[k] = lanip[k];				
				lan_broadcast_ip[k] = lan_broadcast_ip[k] >> subnet_list_t[i].zero_count;												
				for(j=0;j<subnet_list_t[i].zero_count;j++){					
					lan_broadcast_ip[k] = (lan_broadcast_ip[k] << 1) + 1;					
				}								
				if(lan_broadcast_ip[k] == 0 && lan_subnet[k] != 255){
					lan_broadcast_ip[k] = 255;
				}				
				break;
			}
		}
	}
	
	sprintf(lan_broadcast_ip_str,"%i.%i.%i.%i",\
		lan_broadcast_ip[0],lan_broadcast_ip[1],lan_broadcast_ip[2],lan_broadcast_ip[3]);
	
	DEBUG_MSG("get_lan_broadcast_ip: lan_broadcast_ip_str=%s\n", lan_broadcast_ip_str);
		
	return &lan_broadcast_ip_str[0];	
}

static int get_dest_mac_addr(const char *hostid, struct ether_addr *eaddr){
	/*
	ether_aton() converts the 48-bit Ethernet host address asc 
	from the standard hex-digits-and-colons notation into binary data in network byte order 
	and returns a pointer to it in a statically allocated buffer, 
	which subsequent calls will overwrite. 
	ether_aton returns NULL if the address is invalid.
	*/
	struct ether_addr *eap = (struct ether_addr*)ether_aton(hostid);
	
	/* mac transfer error */
	if(!eap){return -1;}	
	*eaddr = *eap;	
	return 0;
}

static int generate_WOL_context(unsigned char *pkt, struct ether_addr *eaddr){
	
	unsigned char *station_addr = eaddr->ether_addr_octet;
	int offset=0;
	int i=0;
	
	offset = 0;
	memset(pkt+offset, 0xff, 6);
	offset += 6;

	for(i = 0; i < 16; i++){
		memcpy(pkt+offset, station_addr, 6);
		offset += 6;
	}
		
	return offset;
}

static void send_WOL_packet(char *WOL_remote_mac, char *lan_ip, char *subnet_mask, int private_port){
	int sockfd;
	int broadcast=1;
	struct sockaddr_in recvaddr;
	int numbytes=0;
	struct ether_addr eaddr;
	int pktsize=0;
	char outpack[150];
	
	if((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1){
		perror("### send_WOL_packet:sockfd");return;
	}
	
	if((setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast)) == -1){
		perror("### send_WOL_packet:setsockopt");return;
	}
	
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = private_port;
	DEBUG_MSG("send_WOL_packet:lan_ip=%s, subnet_mask=%s, port=%d\n", lan_ip, subnet_mask, private_port);
	recvaddr.sin_addr.s_addr = inet_addr(get_lan_broadcast_ip(lan_ip, subnet_mask));
	memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);

	/* get WOL dest mac */
	if (get_dest_mac_addr(WOL_remote_mac, &eaddr) != 0){
		printf("### WOL Destination Address ERROR ###\n");return;
	}
	
	/* generate WOL packet context */
	pktsize = generate_WOL_context(outpack, &eaddr);	
		
	if((numbytes = sendto(sockfd, outpack, pktsize , 0,(struct sockaddr *)&recvaddr, sizeof recvaddr)) != -1){
   		DEBUG_MSG("### Sent WOL packet success , packets size=%d###\n", numbytes);
   	}else{
   		printf("### Sent WOL packet error ###\n");
   	}	
   		
	shutdown(sockfd, 2);	
}

int main(int argc, char *argv[]){
	int sockfd = -1;
	int receive_pkt_len = 0;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t len = 0;
	u_char mesg[110] = {};
	char remote_mac[18] = {};
	char *wan_ip = NULL;
	struct in_addr addr;
	char *lan_ip=NULL;
	char *subnet_mask=NULL;
	int private_port=9;
	
	if(!argv[1] || !argv[2] || !argv[3] || !argv[4]){
		printf("\n");
		printf("### wakeOnLanProxy format error! \n");
		printf("### - usage: wakeOnLanProxy LAN-IP Subnet-Mask Proxy-IP Port\n");
		printf("### If WakeOnLan need to support Schedule or Inbound-Filter => Proxy-IP = LAN-IP\n");
		printf("### -example: wakeOnLanProxy 192.168.0.1 255.255.255.0 192.168.0.1 9 \n\n");
		printf("### Else => Proxy-IP = WAN-IP\n");
		printf("### -example: wakeOnLanProxy 192.168.0.1 255.255.255.0 172.21.65.88 9 \n\n");
		return 0;
	}
	
	lan_ip = argv[1];
	subnet_mask = argv[2];
	wan_ip = argv[3];
	private_port = atoi(argv[4]);	
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;		
	if( inet_aton(wan_ip, &addr) == 0 )
	{
		perror("### WAN IP format error");
		return 0;
	}
	servaddr.sin_addr = addr;
	servaddr.sin_port=htons(private_port);
	
	if((sockfd=socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("### listen WOL_packet fail");
		return 0;
	}
	
	if( bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1 )
	{
		perror("### bind() WAN IP fail");
		shutdown(sockfd, 2);
		return 0;
	}	
				
	len = sizeof(cliaddr); 
	
	for (;;){	
		memset(mesg, 0, sizeof(mesg));	     
		receive_pkt_len = recvfrom(sockfd, mesg, sizeof(mesg), 0, (struct sockaddr *)&cliaddr, &len);            
		
		DEBUG_MSG("### Received WOL packet len: %d ###\n", receive_pkt_len);
		/* If data len is not 102, it maybe not the WOL packet */
		if(receive_pkt_len != 102){	
			printf("### Received WOL packet len should be 102, check if the packet is legal ###\n");
			continue;
		}
		
		mesg[receive_pkt_len] = '\0';
		memset(remote_mac, 0, sizeof(remote_mac));
		sprintf(remote_mac,"%02x:%02x:%02x:%02x:%02x:%02x",\
				mesg[6],mesg[7],mesg[8],mesg[9],mesg[10],mesg[11]);
		DEBUG_MSG("### Received WOL packet mac = %s ###\n", remote_mac);
		send_WOL_packet(remote_mac, lan_ip, subnet_mask, private_port);
	}
	shutdown(sockfd, 2);
	return 0;
}




