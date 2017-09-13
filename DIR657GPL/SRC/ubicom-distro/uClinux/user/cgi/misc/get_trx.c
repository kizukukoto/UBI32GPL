#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libdb.h"
#include "querys.h"

int do_get_trx(char *interface, char *type)
{
	FILE *fp;
	int i;
	char buf[128], val[8], tmp[8], *face, *r_byte, *r_packet, *t_byte, *t_packet;
	char lan_ifname[8], wan_ifname[8], wl_ifname[8], *ifname = NULL;

	if ((fp = fopen("/proc/net/dev", "r")) == NULL) {
		perror("open /proc/net/dev");
		exit(1);
	}
	if (strcmp(interface, "lan") == 0) {
		if (query_record("lan_ifname", lan_ifname, sizeof(lan_ifname)) != 0)
			return -1;
		ifname = lan_ifname;
	} else if (strcmp(interface, "wan") == 0) {
		if (query_record("wan0_ifname", wan_ifname, sizeof(wan_ifname)) != 0)
			return -1;
		ifname = wan_ifname;
	} else if (strcmp(interface, "wl") == 0){
		if (query_record("wl0_ifname", wl_ifname, sizeof(wl_ifname)) != 0)
			return -1;
		ifname = wl_ifname;
	}

	buf[0] = '\0';
	while(fgets(buf, sizeof(buf), fp)) {
		face = strtok(buf, " :");
		r_byte = strtok(NULL, " ");
		r_packet = strtok(NULL, " ");
		i = 0;
		while(i < 6) {
			strtok(NULL, " ");
			i++;
		}
		t_byte = strtok(NULL, " ");
		t_packet = strtok(NULL, " ");

		if (strcmp(face, ifname) == 0) {
			if (*type && strcmp(type, "set_trx") == 0) {
				if (strcmp(ifname, lan_ifname) == 0) {
					update_record("lan_tx", t_packet);
					update_record("lan_rx", r_packet);
					return 0;
				}
				if (strcmp(ifname, wan_ifname) == 0) {
					update_record("wan_tx", t_packet);
					update_record("wan_rx", r_packet);
					return 0;
				}
				if (strcmp(ifname, wl_ifname) == 0) {
					update_record("wl_tx", t_packet);
					update_record("wl_rx", r_packet);
					return 0;
				}
			}

			sprintf(val, "%s_%s", interface, type);
			tmp[0] = '\0';
			query_record(val, tmp , sizeof(tmp));
			fclose(fp);
			if (strcmp(type, "tx") == 0) {
				return (atoi(t_packet) - atoi(tmp));
			} else {
				return (atoi(r_packet) - atoi(tmp));
			}
		}
	}
	return 0;
}

int get_trx_main(int argc, char *argv[])
{
	printf("%d", do_get_trx(argv[1], argv[2]));
	return 0;
}
