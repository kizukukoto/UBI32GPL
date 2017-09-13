#ifndef _PACKET_H
#define _PACKET_H

#include <netinet/udp.h>
#include <netinet/ip.h>
/*	Date: 2009-12-17
*	Name: jimmy huang
*	Reason:	Modified option length from 308 to 408
*	Note:	
*		1.	TEW-634GRU RUssia has the case, dhcpd offer both option 121 and 249, each contains 15 rules
*			And the total dhcp OFFER packet size is 601, which will cause our udhcpc can not handle the packet
*			thus we increase to 408 to let udhcpc can hanlde these kinds of packet
*		2.	We defined new definition "DHCP_OPTION_SIZE" in common.h, all related variable
*			will be replaced with this definition
*/
#include "common.h"


struct dhcpMessage {
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint32_t ciaddr;	/*Client IP address*/
	uint32_t yiaddr;	/*Your(Client) IP address*/
	uint32_t siaddr;
	uint32_t giaddr;
	uint8_t chaddr[16];	/*Client MAC address*/
	uint8_t sname[64];  /*server host name*/
	uint8_t file[128];  /*boot file name*/
	uint32_t cookie;
	
	/*	Date: 2009-12-17
	*	Name: jimmy huang
	*	Reason:	Modified option length from 308 to 408
	*	Note:	
	*		1.	RUssia has the case, dhcpd offer both option 121 and 249, each contains 15 rules
	*			And the total dhcp OFFER packet size is 601, which will cause udhcpc can not handle
	*			thus we increase to 408 to let udhcpc can hanlde these kinds of packet
	*		2.	We defined new definition "DHCP_OPTION_SIZE" in common.h, all related variable
	*			will be replaced with this definition
	*/
	uint8_t options[DHCP_OPTION_SIZE];
	
/* jimmy modified 20080416, for dhcpd offer 590 bytes */
	//uint8_t options[308];
//	uint8_t options[208]; //308 /* 312 - cookie */
//	/* NickChou, for support MTU=500 , device still can get wan ip. 
//		down size => options max. size form 308 to 208
//		=> sizeof(struct udp_dhcp_packet) from 576 to 476
//	*/
/*----------------------------------------------------- */
};

struct udp_dhcp_packet {
	struct iphdr ip;
	struct udphdr udp;
	struct dhcpMessage data;
};

void init_header(struct dhcpMessage *packet, char type);
int get_packet(struct dhcpMessage *packet, int fd);
uint16_t checksum(void *addr, int count);
int raw_packet(struct dhcpMessage *payload, uint32_t source_ip, int source_port,
		   uint32_t dest_ip, int dest_port, uint8_t *dest_arp, int ifindex);
int kernel_packet(struct dhcpMessage *payload, uint32_t source_ip, int source_port,
		   uint32_t dest_ip, int dest_port);


#endif
