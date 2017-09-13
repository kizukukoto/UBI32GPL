#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include "querys.h"
#include "nvram.h"
#include "debug.h"

static void get_mac_clone_addr(char *device, char *ip)
{
	int s;
	struct arpreq r;
	struct sockaddr *mac;
	struct sockaddr_in *in = (struct sockaddr_in *)&r.arp_pa;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("socket");
		return;
	}
	memset(&r, 0, sizeof(r));
	if(inet_aton(ip, &in->sin_addr) == 0)
		return;

	strncpy((char *)r.arp_dev, device, sizeof(r.arp_dev));
	r.arp_pa.sa_family = AF_INET;

	if(ioctl(s, SIOCGARP, &r) < 0) {
		//perror("ioctl");
		return;
	}
	mac = &r.arp_ha;
	printf("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", 
			(uint8_t)mac->sa_data[0], (uint8_t)mac->sa_data[1], 
			(uint8_t)mac->sa_data[2], (uint8_t)mac->sa_data[3], 
			(uint8_t)mac->sa_data[4], (uint8_t)mac->sa_data[5]);
}

static void mac_clone_addr(const char *device)
{
	char ip[128], tmp_ip[128], *tmp, *ptr, *tmp1;
	/* REMOTE_ADDR == [::ffff:192.168.1.100] */
	sprintf(tmp_ip, "%s", getenv("REMOTE_ADDR"));
	tmp = tmp_ip;
	while((ptr = strsep(&tmp, ":")) != NULL) {
		if (tmp == NULL) 
			break;
	}
	sprintf(ip, "%s", ptr);
	ip[strlen(ip)-1] = '\0';
	get_mac_clone_addr(device, ip);
}

int clone_main(int argc, char *argv[])
{
	char lan_ifname[8];

	memset(lan_ifname, '\0', sizeof(lan_ifname));

	query_vars("lan_ifname", lan_ifname, sizeof(lan_ifname));

/* Ubicom platform*/
	if (strcmp(lan_ifname, "") == 0 || strlen(lan_ifname) == 0) {
		sprintf(lan_ifname, "%s", nvram_safe_get("lan_bridge"));
		mac_clone_addr(lan_ifname);
		return 0;
	}
/* end ubicom */

	get_mac_clone_addr(lan_ifname, getenv("REMOTE_ADDR"));

	return 0;
}

