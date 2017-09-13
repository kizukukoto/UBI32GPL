/* leases.h */
#ifndef _LEASES_H
#define _LEASES_H


struct dhcpOfferedAddr {
	uint8_t chaddr[16];
	uint32_t yiaddr;	/* network order */
	uint32_t expires;	/* host order */
        unsigned char hostname[64];
};

struct html_leases_s
{
     unsigned char hostname[64];
     unsigned char mac_addr[20];
     unsigned char ip_addr[16];
     unsigned char expires[32];
};

extern uint8_t blank_chaddr[];
extern unsigned char blank_hostname[];
void clear_lease(uint8_t *chaddr, uint32_t yiaddr);
struct dhcpOfferedAddr *add_lease(uint8_t *chaddr, uint32_t yiaddr, unsigned long lease, unsigned char *hname);
int lease_expired(struct dhcpOfferedAddr *lease);
struct dhcpOfferedAddr *oldest_expired_lease(void);
struct dhcpOfferedAddr *find_lease_by_chaddr(uint8_t *chaddr);
struct dhcpOfferedAddr *find_lease_by_yiaddr(uint32_t yiaddr);
uint32_t find_address(int check_expired);


#endif
