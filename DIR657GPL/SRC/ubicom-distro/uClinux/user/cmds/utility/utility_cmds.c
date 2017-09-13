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
#if 1//set/get port status
#include <net/if.h>
#endif
#include "cmds.h"

static int dump_remote_mac(const char *device, const char *ip)
{
	int s, rev = -1;
	struct arpreq r;
	struct sockaddr *mac;
	struct sockaddr_in *in = (struct sockaddr_in *)&r.arp_pa;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("socket");
		goto out;
	}
	memset(&r, 0, sizeof(r));
	if (inet_aton(ip, &in->sin_addr) == 0)
		goto out;

	strncpy((char *)r.arp_dev, device, sizeof(r.arp_dev));
	r.arp_pa.sa_family = AF_INET;

	if (ioctl(s, SIOCGARP, &r) < 0)
		goto out;
	mac = &r.arp_ha;
	printf("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", 
			(uint8_t)mac->sa_data[0], (uint8_t)mac->sa_data[1], 
			(uint8_t)mac->sa_data[2], (uint8_t)mac->sa_data[3], 
			(uint8_t)mac->sa_data[4], (uint8_t)mac->sa_data[5]);
	rev = 0;
out:
	return rev;
}

int arp_main(int argc, char *argv[])
{
	char *remote = NULL;
	int rev = -1;
	
	remote = getenv("REMOTE_ADDR");
	if (argc >= 2)
		remote = argv[1];
	if (remote == NULL)
		goto out;
	if ((rev = dump_remote_mac("br0", remote)) == -1)
		rev = dump_remote_mac("eth2", remote);
out:
	return rev;
}

extern int utility_ipsec_main(int argc, char *argv[]);
extern int utility_openvpn_main(int argc, char *argv[]);
extern int utility_dns_main(int argc, char *argv[]);
extern int utility_pppd_main(int argc, char *argv[]);
extern int utility_vpn_main(int argc, char *argv[]);
extern int utility_dump_main(int argc, char *argv[]);
extern int utility_wclient_main(int argc, char *argv[]);
extern int vct_main(int argc, char *argv[]);
int utility_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		//{ "arp", arp_main},
		{ "ipsec", utility_ipsec_main},
		{ "openvpn", utility_openvpn_main},
		//{ "dns", utility_dns_main},
		{ "pppdvpn", utility_pppd_main},
		{ "time", utility_vpn_main},
		//{ "dump", utility_dump_main},
		//{ "wlan", utility_wclient_main},
		//{ "vct", vct_main, "set/get ports status or info"},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}

