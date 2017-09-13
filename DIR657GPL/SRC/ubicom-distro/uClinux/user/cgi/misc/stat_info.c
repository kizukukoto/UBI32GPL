#include <stdio.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#include "querys.h"
#include "netinf.h"
#include "ssi.h"

extern void wan_status(char *buf, int wan_type);

static int russia(char *buf)
{
	char proto[16], type[32];
	char *dhcp = "<br><input type='button' value='DHCP Release'\
			      name='status' onClick=russia_dhcp_release()>\
			      &nbsp;<input type='button' value='DHCP Renew'\
			      name='status' onClick=russia_dhcp_renew()>";
	
        query_vars("wan0_proto", (char *)proto, (int)sizeof(proto));
	if(strcmp(proto, "rupppoe") == 0) {
		strcpy(type, "wan0_russiapppoe_dynamic");
	} else if(strcmp(proto, "rupptp") == 0) {
		strcpy(type, "wan0_rupptp_dynamic");
	}
	proto[0] = '\0';
        query_vars(type, (char *)proto, (int)sizeof(proto));

	strcat(buf, proto);
        switch(atoi(proto)) {
	        case 0:
	                strcpy(buf, "fixed IP");
	                break;
	        case 1:
	                strcpy(buf, "DHCP Client ");
	                break;
	}

        if (atoi(proto) == 1) {
        	wan_status(buf, 0);
        	strcat(buf, dhcp);
        }
        return 0;

}

static int wan_connect(char *buf)
{
	char val[32];
	int wan_type = 0;
	char *dhcp = "<br><input type='button' id='dhcp_release_button' value='DHCP Release'\
			      name='status' onClick=dhcp_release()>\
			      &nbsp;<input type='button' id='dhcp_renew_button' value='DHCP Renew'\
			      name='status' onClick=dhcp_renew()>";
	char *ppp = "<input type='button' name='status' \
			  value='Connect' onClick=pppoe_connect()>\
			  &nbsp;<input type='button' name='status' \
			  value='Disconnect' onClick=pppoe_disconnect()>";
	/* Modify by Tony at 2007/4/23 */
	char *big = "<br><input type='button' name='status' \
			  value='BigPond Login' onClick=pppoe_connect()>\
			  &nbsp;<input type='button' name='status' \
			  value='BigPond Logout' onClick=pppoe_disconnect()>";
	memset(val, '\0', sizeof(val));

        if (query_vars("wan0_proto", (char *)val, (int)sizeof(val)) >= 0) {
		if (strcmp(val, "dhcpc") == 0) {
                	wan_type = 0;
		} else if (strcmp(val, "static") == 0) {
                	wan_type = 1;
		} else if (strcmp(val, "pppoe") == 0) {
                	wan_type = 2;
		} else if (strcmp(val, "pptp") == 0) {
                	wan_type = 3;
		} else if (strcmp(val, "l2tp") == 0) {
                	wan_type = 5;
		} else if (strcmp(val, "bigpond") == 0) {
			wan_type = 6;
		} else if (strcmp(val, "rupppoe") == 0) {
                	wan_type = 7;
		} else if (strcmp(val, "rupptp") == 0) {
			wan_type = 8;
		}
	} else {
		return -1;
	}

        switch (wan_type){
	        case 0:
	                strcpy(buf, "DHCP Client ");
	                break;
	        case 1:
	                strcpy(buf, "fixed IP");
	                break;
	        case 2:
	                strcpy(buf,"PPPoE ");
	                break;
	        case 3:
	                strcpy(buf,"PPTP ");
	                break;
	        case 5:
	                strcpy(buf,"L2TP ");
	                break;
		case 6:
	                strcpy(buf,"BigPond ");
			break;
	        case 7:
	                strcpy(buf,"Russia PPPoE ");
	                break;
		case 8:
	                strcpy(buf,"Russia PPTP ");
			break;
        }

        wan_status(buf, wan_type);
        if (wan_type == 0) {
               strcat(buf, dhcp);
	} else if (wan_type == 6 ){ /*	Modify by Tony at 2007/4/23 */
               strcat(buf, big);
        } else if (wan_type > 1){
               strcat(buf, ppp);
        }
        return 0;
}

extern int get_wan_name(char *tmp);


int stat_info_main(int argc, char *argv[])
{
	char lan_ifname[8], ifname[8];
	char wan_ifname[8], value[16];
	char buf[256],  *string;
	int i = 1, j, index, ppp = 1;
	FILE *fp;

	const static char *name[] = {
		"lan_ip",
		"lan_mac",
		"lan_netmask",
		"wan_ip",
		"wan_mac",
		"wan_netmask",
		"def_gw",
		"dns_server",
		"wan_connect",
		"russia",
		NULL
	};

	if (argc != 2) {
		printf("this program need args");
		return -1;
	}
	nvram_init(NULL);
	string = malloc(sizeof(char) *256);

	/*if the connection is break don't show the informantion*/
	memset(value, 0, sizeof(value));
	query_vars("wan0_proto", value, sizeof(value));
	if(strcmp(value, "pppoe") == 0) {
		fp = fopen("/tmp/ppp/link.ppp0", "r");
		if (fp == (FILE *) 0) {
			ppp = 0;
		}
	}


	while(argv[i]) {
		index = -1;
		j = 0;
		memset(buf, '\0', sizeof(buf));
		memset(wan_ifname, '\0', sizeof(wan_ifname));
		get_wan_name(wan_ifname);
		query_vars("lan_ifname", lan_ifname, sizeof(lan_ifname));
		query_vars("wan_ifname", ifname, sizeof(ifname));
		while(name[j]) {
			if(!strcmp(argv[i], name[j])) {
				index = j;
				break;
			}
			j++;
		}
		switch (index) {
			case 0:
				get_ip(lan_ifname, buf);
				break;
			case 1:
				get_mac(lan_ifname, buf);
				break;
			case 2:
				get_netmask(lan_ifname, buf);
				break;
			case 3:
				if(ppp)
					get_ip(wan_ifname, buf);
				break;
			case 4:
				if(ppp)
					get_mac(ifname, buf);
				break;
			case 5:
				if(ppp)
					get_netmask(wan_ifname, buf);
				break;
			case 6:
				if(ppp)
					get_def_gw(buf);
				break;
			case 7:
				if(ppp)
					get_dns_srv(buf);
				break;
			case 8:
				wan_connect(buf);
				break;
			case 9:
				russia(buf);
				break;
			default:
				break;
		}
		strcat(string, buf);
		strcat(string, " ");
		i++;
	}

	printf("%s", string);

	return 0;
}
