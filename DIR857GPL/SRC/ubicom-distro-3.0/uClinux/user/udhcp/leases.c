/*
 * leases.c -- tools to manage DHCP leases
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"
#include "arpping.h"
#include "common.h"

#include "static_leases.h"


uint8_t blank_chaddr[] = {[0 ... 15] = 0};
unsigned char blank_hostname[] = {0x07, 0x55, 0x4e, 0x4b, 0x4e, 0x4f, 0x57 ,0x4e};//" UNKNOWN  ";
//static unsigned char win2003_keyword = 0x3c;

/* clear every lease out that chaddr OR yiaddr matches and is nonzero */
void clear_lease(uint8_t *chaddr, uint32_t yiaddr)
{
	unsigned int i, j;

	for (j = 0; j < 16 && !chaddr[j]; j++);

	for (i = 0; i < server_config.max_leases; i++)
		if ((j != 16 && !memcmp(leases[i].chaddr, chaddr, 16)) ||
		    (yiaddr && leases[i].yiaddr == yiaddr)) {
			memset(&(leases[i]), 0, sizeof(struct dhcpOfferedAddr));
		}
}


/* add a lease into the table, clearing out any old ones */
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease, unsigned char *hname)
{
	struct dhcpOfferedAddr *oldest;
        int tmpVal;
    
	/* clean out any old ones */
	clear_lease(chaddr, yiaddr);

	oldest = oldest_expired_lease();

	if (oldest) {
		memcpy(oldest->chaddr, chaddr, 16);
		DEBUG(LOG_INFO,"oldest->chaddr =%02x:%02x:%02x:%02x:%02x:%02x\n",oldest->chaddr[0], oldest->chaddr[1], oldest->chaddr[2], oldest->chaddr[3], oldest->chaddr[4], oldest->chaddr[5]);
		oldest->yiaddr = yiaddr;
		oldest->expires = time(0) + lease;
		//DEBUG(LOG_INFO,"oldest->expires =%s\n", oldest->expires);
		if(hname != NULL){
            /* modify by ken for hoat name issue the hostname fist byte is length */
            tmpVal = hname[0];    		          
            //memcpy(oldest->hostname, hname, strlen(hname)-2);
			//tmpVal = strcspn(oldest->hostname, &win2003_keyword);
            //memset(oldest->hostname+tmpVal, 0, MAX_HOSTNAME_LEN-tmpVal); 
            if(tmpVal >= MAX_HOSTNAME_LEN)
            	tmpVal = MAX_HOSTNAME_LEN -1;            
            memcpy(oldest->hostname, hname+1, tmpVal);
                memset(oldest->hostname+tmpVal, 0, MAX_HOSTNAME_LEN-tmpVal);   
//        	DEBUG(LOG_INFO,"tmpVal=%d,oldest->hostname =%s\n",tmpVal, oldest->hostname);        		          
	}
	}

	return oldest;
}


/* true if a lease has expired */
int lease_expired(struct dhcpOfferedAddr *lease)
{
	return (lease->expires < (unsigned long) time(0));
}


/* Find the oldest expired lease, NULL if there are no expired leases */
struct dhcpOfferedAddr *oldest_expired_lease(void)
{
	struct dhcpOfferedAddr *oldest = NULL;
	unsigned long oldest_lease = time(0);
	unsigned int i;


	for (i = 0; i < server_config.max_leases; i++)
		if (oldest_lease > leases[i].expires) {
			oldest_lease = leases[i].expires;
			oldest = &(leases[i]);
		}
	return oldest;

}


/* Find the first lease that matches chaddr, NULL if no match */
struct dhcpOfferedAddr *find_lease_by_chaddr(uint8_t *chaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
	{
		/*NickChou, 07.09.06, from 16 to ETH_10MB_LEN*/
         if (!memcmp(leases[i].chaddr, chaddr, ETH_10MB_LEN))
			return &(leases[i]);
	}

	return NULL;
}


/* Find the first lease that matches yiaddr, NULL is no match */
struct dhcpOfferedAddr *find_lease_by_yiaddr(uint32_t yiaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
		if (leases[i].yiaddr == yiaddr) 
			return &(leases[i]);

	return NULL;
}


/* check is an IP is taken, if it is, add it to the lease table */
static int check_ip(uint32_t addr)
{
	struct in_addr temp;

	if (arpping(addr, server_config.server, server_config.arp, server_config.interface) == 0) {
		temp.s_addr = addr;
	// mark LOG to pass cd router test item : cdrouter_scale_10 	
	//	LOG(LOG_INFO, "%s belongs to someone, reserving it for %ld seconds",
	//		inet_ntoa(temp), server_config.conflict_time);
		DEBUG(LOG_INFO, "arpping find used");
		/* ken remark to fix when had device used static ip UI show mac 00:00:00:00:00:00 and unknown neme */
		//add_lease(blank_chaddr, addr, server_config.conflict_time, blank_hostname);
		return 1;
	} else 
		return 0;
}


/* find an assignable address, it check_expired is true, we check all the expired leases as well.
 * Maybe this should try expired leases by age... */
uint32_t find_address(int check_expired)
{
	uint32_t addr, ret;
	struct dhcpOfferedAddr *lease = NULL;

	addr = ntohl(server_config.start); /* addr is in host order here */
	for (;addr <= ntohl(server_config.end); addr++) {

		/* ie, 192.168.55.0 */
		if (!(addr & 0xFF)) continue;

		/* ie, 192.168.55.255 */
		if ((addr & 0xFF) == 0xFF) continue;

		/* Only do if it isn't an assigned as a static lease */
		if(!reservedIp(server_config.static_leases, htonl(addr)))
		{

		/* lease is not taken */
		ret = htonl(addr);
		if ((!(lease = find_lease_by_yiaddr(ret)) ||

		     /* or it expired and we are checking for expired leases */
		     (check_expired  && lease_expired(lease))) &&

		     /* and it isn't on the network */
	    	     !check_ip(ret)) {
			return ret;
			break;
		}
	}
	}
	return 0;
}
