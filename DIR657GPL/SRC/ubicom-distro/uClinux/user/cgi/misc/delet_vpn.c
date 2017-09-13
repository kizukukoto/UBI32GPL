#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "querys.h"
#include "libdb.h"

#define LINK		50


void delet_vpn(char *wan_ifname)
{
	char *p, *q;
	/* FIXME buffer limit */
	char tmp[1024], buf[1024];

	sprintf(tmp, "/sbin/ifconfig %s down", wan_ifname);
	system(tmp);

	query_vars("vpn_link", tmp, sizeof(tmp));
	if((p = strstr(tmp, wan_ifname))) {
		q = strchr(p, '#');
		q++;
		tmp[strlen(tmp) - strlen(p)] = '\0';
		sprintf(buf, tmp);
		strcat(buf, q);

		q--;
		query_vars("vpn_link", tmp, sizeof(tmp));
		tmp[strlen(tmp) - strlen(q)] = '\0';
		p = strrchr(tmp, ',');
		p++;
		sprintf(tmp, "/usr/sbin/iptables -D FORWARD -s %s -j ACCEPT", p);
		system(tmp);
		update_record("vpn_link", buf);
	}
}


int delete_vpn_main(int argc, char *argv[])
{
	int i, s; 
	char wan_ifname[8];
	struct ifreq ifr;

	close(2);
	for(i = 0; i < LINK; i++) {
		sprintf(wan_ifname, "ppp%d", i);
		strncpy(ifr.ifr_name, wan_ifname, sizeof(wan_ifname));
		if((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
			continue;

		if(ioctl(s, SIOCGIFFLAGS, &ifr) || !(ifr.ifr_flags & IFF_UP)) {
			delet_vpn(wan_ifname);
		}
		close(s);
	}
	return 0;
}
