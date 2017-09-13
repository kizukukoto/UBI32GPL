#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "ssi.h"
#include "shvar.h"
#include "sutil.h"
#include "nvram.h"
#include "libdb.h"
#include "debug.h"
#include "libdhcpd.h"

struct cmo_get_list_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int misc_cmo_get_list_help(struct cmo_get_list_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

static int igmp_table(int argc, char *argv[])
{
	FILE *fp, *fp_pid;
        int i;
	char igmp_list[256];

        memset(igmp_list, '\0', sizeof(igmp_list));

        unlink(IGMP_GROUP_FILE);

        if ((fp_pid = fopen(IGMPPROXY_PID,"r")) == NULL ) {
		cprintf("get_igmp_list %s error\n",IGMPPROXY_PID);
		return -1;
	} else {
                /*write igmp member into file*/
                kill(read_pid(IGMPPROXY_PID), SIGUSR1);
                fclose(fp_pid);
        }
        sleep(1);

        if ((fp = fopen(IGMP_GROUP_FILE,"r")) == NULL) {
                cprintf("get_igmp_list: %s error\n",IGMP_GROUP_FILE);
                return -1;
        }

	while (fgets(igmp_list, sizeof(igmp_list), fp)) {
		if (strlen(igmp_list)) {
			printf("%s,",igmp_list);
		} else
			break;
	}
        fclose(fp);

	return 0;
}

static int client_list_table(int argc, char *argv[])
{
	int i = 0;
        FILE *fp, *fp1;
        char temp[128];
	
	memset(temp, '\0', sizeof(temp));

        if ((fp = fopen(CLIENT_LIST_FILE,"r")) == NULL) {
		cprintf("list file does not exist\n");
		return -1;
        }
	while(fgets(temp,sizeof(temp),fp)) {
		i++;
		printf("%s,", temp);
		memset(temp, '\0', sizeof(temp));
	}
	fclose(fp);

	if (!i) {
		if ((fp1 = fopen(CLIENT_LIST_TMP_FILE,"r")) == NULL) {
			cprintf("list tmp file does not exist\n");
			return -1;
		} else {
			while(fgets(temp,sizeof(temp),fp1)) {
				printf("%s,", temp);
				memset(temp,0,sizeof(temp));
			}
			fclose(fp1);
		}
	}
	return 0;
}

static int dhcpd_leased_table(int argc, char *argv[])
{
	int i = 0;
	struct dhcpd_leased_table_s client_list[M_MAX_DHCP_LIST];

	memset(client_list, '\0', sizeof(client_list));

	get_dhcpd_leased_table(client_list);
	for (i = 0; i < M_MAX_DHCP_LIST; i++) {
		if (client_list[i].hostname == NULL || strlen(client_list[i].hostname) == 0)
			break;
		printf("%s/%s/%s/%s,", 
			client_list[i].hostname, client_list[i].ip_addr,
			client_list[i].mac_addr, client_list[i].expired_at);
	}
	return 0;
}

int cmo_get_list_main(int argc, char *argv[])
{
	int ret = -1;
	struct cmo_get_list_struct *p, list[] = {
		{ "igmp_table", "Igmp Table List", igmp_table },
		{ "local_lan_ip", "Local Lan IP List", client_list_table },
		{ "dhcpd_leased_table", "Dhcpd Lease Table", dhcpd_leased_table },
		//{ "routing_table", return_routing_table },
		//{ "wireless_station_list", return_wireless_marvell_station_list},
		//{ "wireless_ap_scan_list",return_wireless_ap_scan_list},
		//{ "upnp_portmap_table", return_upnp_portmap_table},
		//{ "napt_session_table", return_napt_session_table },
		//{ "internet_session_table", return_internet_session_table },
		//{ "ipv6_client_list",return_ipv6_client_list_table},
		//{ "plc_device_scan",return_plc_device_scan},
		{ NULL, NULL, NULL }
	};

	if (argc == 1 || strcmp(argv[1], "help") == 0)
		goto help;

	for (p = list; p && p->param; p++) {
		if (strcmp(argv[1], p->param) == 0) {
			ret = p->fn(argc - 1, argv + 1);
			goto out;
		}
	}
help:
	ret = misc_cmo_get_list_help(list);
out:
	return ret;
}
