/* serverpacket.c
 *
 * Construct and send DHCP server packets
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include "serverpacket.h"
#include "dhcpd.h"
#include "options.h"
#include "common.h"
#include "static_leases.h"
#ifdef UDHCPD_ALWAYS_BROADCAST
extern unsigned int always_bcast;
#endif
extern char dns_name[];
/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(struct dhcpMessage *payload)
{
	DEBUG(LOG_INFO, "Forwarding packet to relay");

	return kernel_packet(payload, server_config.server, SERVER_PORT,
			payload->giaddr, SERVER_PORT);
}

/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(struct dhcpMessage *payload, int force_broadcast)
{
	uint8_t *chaddr;
	uint32_t ciaddr;

#ifdef UDHCPD_ALWAYS_BROADCAST
	if (always_bcast)
		payload->flags |= htons(BROADCAST_FLAG);
#endif

	if (force_broadcast) {
		DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else if (payload->ciaddr) {
		DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
		ciaddr = payload->ciaddr;
		chaddr = payload->chaddr;
	} else if (ntohs(payload->flags) & BROADCAST_FLAG) {
		DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else {
		DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
		ciaddr = payload->yiaddr;
		chaddr = payload->chaddr;
	}

	return raw_packet(payload, server_config.server, SERVER_PORT,
			ciaddr, CLIENT_PORT, chaddr, server_config.ifindex);
			
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(struct dhcpMessage *payload, int force_broadcast)
{
	int ret;

	if (payload->giaddr)
		ret = send_packet_to_relay(payload);
	else ret = send_packet_to_client(payload, force_broadcast);
	return ret;
}


static void init_packet(struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type)
{
	init_header(packet, type);
	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, 16);
	packet->flags = oldpacket->flags;
	packet->giaddr = oldpacket->giaddr;
	packet->ciaddr = oldpacket->ciaddr;
	add_simple_option(packet->options, DHCP_SERVER_ID, server_config.server);
}

/* check if (lan_ip & netmask) == (lease_ip &netmask) */
static int check_ip_by_netmask(char *lan_ip,char *netmask,char *lease_ip){
	struct in_addr lan_addr;
	struct in_addr netmask_addr;
	struct in_addr lease_addr;
	struct in_addr result_lan_mask;
	struct in_addr result_lease_mask;
	char lan_mask[20] = {};
	char lease_mask[20] = {};
	
	if(lan_ip == NULL || netmask == NULL || lease_ip == NULL)
		return 0;
	
	inet_aton(lan_ip,&lan_addr);
	inet_aton(netmask,&netmask_addr);
	inet_aton(lease_ip,&lease_addr);
	
	result_lan_mask.s_addr = lan_addr.s_addr & netmask_addr.s_addr;
	result_lease_mask.s_addr = lease_addr.s_addr & netmask_addr.s_addr;		
	
	memcpy(lan_mask,inet_ntoa(result_lan_mask),strlen(inet_ntoa(result_lan_mask)));
	memcpy(lease_mask,inet_ntoa(result_lease_mask),strlen(inet_ntoa(result_lease_mask)));
	
	if( strcmp(lan_mask,lease_mask) == 0)
		return 1;	
	return 0;
}

/* add in the bootp options */
static void add_bootp_options(struct dhcpMessage *packet)
{
	packet->siaddr = server_config.siaddr;
	if (server_config.sname)
		strncpy(packet->sname, server_config.sname, sizeof(packet->sname) - 1);
	if (server_config.boot_file)
		strncpy(packet->file, server_config.boot_file, sizeof(packet->file) - 1);
}

void get_dns_name(char *dns_name_input){
	FILE *fp = fopen("/var/tmp/dhcpc_dns.tmp","r");
	if(fp == NULL)
		return;
	/* save lease value for dhcp client */						
	fgets(dns_name_input, 32, fp);
	fclose(fp);
}


/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct dhcpOfferedAddr *lease = NULL;
	uint32_t req_align, lease_time_align = server_config.lease;
	uint8_t *req, *lease_time;
	struct option_set *curr;
	struct in_addr addr;
	unsigned char *hname = NULL;
	uint32_t static_lease_ip;
	//unsigned char *dname = NULL;

	init_packet(&packet, oldpacket, DHCPOFFER);

	static_lease_ip = getIpByMac(server_config.static_leases, oldpacket->chaddr);

	/* ADDME: if static, short circuit */
	if(!static_lease_ip)
	{
		int ip_validity_flag = 0;
		struct in_addr lease_ip; 
		
		lease = find_lease_by_chaddr(oldpacket->chaddr);
		if(lease != NULL)
		{
			lease_ip.s_addr = lease->yiaddr;							    
			/* Chun modify for XBOX-upnp-test */					    
			LOG(LOG_INFO, "UDHCPD sendOffer : device_lan_ip=%s , device_lan_subnet_mask=%s", nvram_lan_ip, nvram_lan_mask);					    
			ip_validity_flag = check_ip_by_netmask(nvram_lan_ip,nvram_lan_mask,inet_ntoa(lease_ip));
			/* Chun end */		 			
		}
		
	  /* the client is in our lease/offered table && is in the same subnet with router ip */	
	    if ((ip_validity_flag && lease != NULL ) 
	    	&&	(!reservedIp(server_config.static_leases, htonl(lease->yiaddr)))
	    	)
    {
			LOG(LOG_DEBUG, "UDHCPD sendOffer : client is in lease/offered table");	
		if (!lease_expired(lease))
			lease_time_align = lease->expires - time(0);
		packet.yiaddr = lease->yiaddr;
			if(ntohl(packet.yiaddr) < ntohl(server_config.start) || ntohl(packet.yiaddr) > ntohl(server_config.end)){				
				goto find_free_ip;
			}
		} 			
		else if (
	/* Or the client has a requested ip */
				(req = get_option(oldpacket, DHCP_REQUESTED_IP)) &&

		   /* Don't look here (ugly hackish thing to do) */
		   memcpy(&req_align, req, 4) &&

		   /* and the ip is in the lease range */
		   ntohl(req_align) >= ntohl(server_config.start) &&
		   ntohl(req_align) <= ntohl(server_config.end) &&
		
				/* Check that its not a static lease */
				!static_lease_ip &&
		
				/* and is not already taken/offered  or its taken, but expired */
		   		((!(lease = find_lease_by_yiaddr(req_align)) || lease_expired(lease))) &&

		   		/*NickChou, 2007.10.02, check offer ip isn't in static leases*/
		   		(!reservedIp(server_config.static_leases, htonl(req_align)))
		   		
				/* ADDME: or maybe in here */
				) 
		{
				LOG(LOG_DEBUG, "UDHCPD sendOffer : client has a requested ip and there is not a host using this IP");
				packet.yiaddr = req_align; /* FIXME: oh my, is there a host using this IP? */
		} 
		else /* otherwise, find a free IP */
		{
find_free_ip:			
			LOG(LOG_DEBUG, "UDHCPD sendOffer : find a free IP");
			/* Is it a static lease? (No, because find_address skips static lease) */
		packet.yiaddr = find_address(0);

		/* try for an expired lease */
			if (!packet.yiaddr) 
				packet.yiaddr = find_address(1);
	}

	if(!packet.yiaddr) {
		LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
		return -1;
	}
		
	if ((hname = get_option(oldpacket, DHCP_HOST_NAME)) == NULL)
             hname = blank_hostname;
	             
	if (!add_lease(packet.chaddr, packet.yiaddr, server_config.offer_time, hname)) {
		LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
		return -1;
	}

	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config.lease)
			lease_time_align = server_config.lease;
	}

	/* Make sure we aren't just using the lease time from the previous offer */
	if (lease_time_align < server_config.min_lease)
		lease_time_align = server_config.lease;
	}
	else/* ADDME: end of short circuit */
	{
		/* It is a static lease... use it */
		packet.yiaddr = static_lease_ip;
	}

	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}
	if (dns_name[0] != 0)
		add_simple_option(packet.options, DHCP_DOMAIN_NAME, dns_name);	
	add_bootp_options(&packet);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "UDHCPD sending OFFER of %s", inet_ntoa(addr));
	return send_packet(&packet, 0);
}


int sendNAK(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;

	init_packet(&packet, oldpacket, DHCPNAK);

	DEBUG(LOG_INFO, "sending NAK");
	return send_packet(&packet, 1);
}


int sendACK(struct dhcpMessage *oldpacket, uint32_t yiaddr)
{
	struct dhcpMessage packet;
	struct option_set *curr;
	uint8_t *lease_time;
	uint32_t lease_time_align = server_config.lease;
	struct in_addr addr;
        unsigned char *hname = NULL;
        //unsigned char *dname = NULL;

	init_packet(&packet, oldpacket, DHCPACK);
	packet.yiaddr = yiaddr;

	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config.lease)
			lease_time_align = server_config.lease;
		else if (lease_time_align < server_config.min_lease)
			lease_time_align = server_config.lease;
	}

	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}
	if (dns_name[0] != 0)
		add_simple_option(packet.options, DHCP_DOMAIN_NAME, dns_name);
	add_bootp_options(&packet);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_DEBUG, "UDHCPD sending ACK to %s", inet_ntoa(addr));

	if (send_packet(&packet, 0) < 0)
		return -1;

	if ((hname = get_option(oldpacket, DHCP_HOST_NAME)) == NULL)
             hname = blank_hostname;

	add_lease(packet.chaddr, packet.yiaddr, lease_time_align, hname);

	return 0;
}


int send_inform(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct option_set *curr;

	uint32_t lease_time_align = server_config.lease;
	uint8_t *lease_time;
	unsigned char *hname = NULL;
	struct in_addr addr;
	
	init_packet(&packet, oldpacket, DHCPACK);

	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	if (dns_name[0] != 0)
		add_simple_option(packet.options, DHCP_DOMAIN_NAME, dns_name);
	add_bootp_options(&packet);

	/*NickChou add for dhcp client list*/
	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) 
	{
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config.lease)
			lease_time_align = server_config.lease;
		else if (lease_time_align < server_config.min_lease)
			lease_time_align = server_config.lease;
	}
		
	if ((hname = get_option(oldpacket, DHCP_HOST_NAME)) == NULL)
		hname = blank_hostname;
		
	addr.s_addr = packet.ciaddr;
	LOG(LOG_INFO, "UDHCPD Inform: add_lease %s", inet_ntoa(addr));	
		
	add_lease(packet.chaddr, packet.ciaddr, lease_time_align, hname);	
	/*NickChou end*/
	
	return send_packet(&packet, 0);
}
