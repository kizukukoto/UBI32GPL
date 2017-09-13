#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

#include <linux_vct.h>
#include <nvram.h>

#include "debug.h"

#define WAN_CONNECT_FILE "/var/tmp/wan_connect_time.tmp"
#define WAN_CONNECT_DIFFERENCE "/var/tmp/wan_connect_time_diff.tmp"


char *wan_statue(const char *proto)
{
        char pid_file[40]="";
        char mpppoe_mode[40]="";
        char eth[10], ifnum='0';
        char status[15];
        int demand = 0, alwayson = 0;
        struct stat filest;
        char *ConnStatue[] = {"Connected", "Connecting", "Disconnected"};
        enum {Connected, Connecting, Disconnected};
        char *wan_eth = NULL;
        char ipaddr[32], netmask[32];

	memset(ipaddr, '\0', sizeof(ipaddr));
	memset(netmask, '\0', sizeof(netmask));

        wan_eth = nvram_safe_get("wan_eth");
        /* Check WAN eth phy connect */
        VCTGetPortConnectState(wan_eth, VCTWANPORT0, status);
        if (!strncmp("disconnect", status, 10))
                return ConnStatue[Disconnected];

        if (strcmp(proto, "dhcpc") == 0 || strcmp(proto, "static") == 0)
                strcpy(eth, wan_eth);
        else
                strcpy(eth, "ppp0");

        if (get_ip(eth, ipaddr) == 0) {
                if (strncmp(proto, "pppoe", 5) == 0) {
                        if(!strncmp(ipaddr, "10.64.64.64", 11))
                                return ConnStatue[Disconnected];
                }
                if (get_netmask(eth, netmask) == 0)
                	return ConnStatue[Connected];
        }

        if (strncmp(proto, "pppoe", 5) == 0) {
                sprintf(pid_file, "/var/run/ppp-0%c.pid", ifnum);
                sprintf(mpppoe_mode, "wan_pppoe_connect_mode_0%c", ifnum);
                if (nvram_match(mpppoe_mode,"on_demand") != 0)
                        demand = 1;
                else if (nvram_match(mpppoe_mode,"always_on") != 0)
                        alwayson = 1;
        } else if (strcmp(proto, "pptp") == 0) {
                strcpy(pid_file, "/var/run/ppp-pptp.pid");
                if (nvram_match("wan_pptp_connect_mode", "on_demand") != 0)
                        demand = 1;
                else if (nvram_match("wan_pptp_connect_mode", "always_on") != 0)
                        alwayson = 1;
        } else if (strcmp(proto, "l2tp") == 0){
                strcpy(pid_file, "/var/run/l2tp.pid");
                if (nvram_match("wan_l2tp_connect_mode", "on_demand") != 0)
                        demand = 1;
                else if (nvram_match("wan_l2tp_connect_mode", "always_on") != 0)
                        alwayson = 1;
	}

        if( stat(pid_file, &filest) == 0 || alwayson == 1)
                return ConnStatue[Connecting];

        return ConnStatue[Disconnected];
}

static unsigned long uptime(void){
        struct sysinfo info;
        sysinfo(&info);
        return info.uptime;
}

int get_wan_connect_time(int status)
{
	FILE *fp,*fd;
	char *wan_proto;
	char wan_status[16], tmp[10],tmp_time[10];
	int connect_time = 0;
	
	memset(wan_status, '\0' , sizeof(wan_status));
	memset(tmp, '\0', sizeof(tmp));
	memset(tmp_time, '\0', sizeof(tmp_time));

	wan_proto = nvram_safe_get("wan_proto");

	if (status == 1) {
		if(strcmp(wan_proto, "pppoe") == 0)
			strcpy(wan_status, wan_statue("pppoe-00"));
		else
			strcpy(wan_status, wan_statue(wan_proto));

                if(strncmp(wan_status, "Connected", 9) == 0) {
                        /* open read-only file to get wan connect time stamp */
                        if ((fp = fopen(WAN_CONNECT_FILE,"r+")) == NULL)
                                return 0;

                        if (fgets(tmp, sizeof(tmp), fp))
                                connect_time = atol(tmp);

                        fclose(fp);

			if((fd = fopen(WAN_CONNECT_DIFFERENCE,"r+")) != NULL) {
				fgets(tmp_time, sizeof(tmp), fd);
                        	fclose(fd);
			}

			
                        if (uptime() == connect_time)
                                return 1;
                        else {
				/* Connection up time display on status page ,need wait a few seconds.
				   So we add a temp file to record the last connection time.
				*/
				//cprintf("XXX %s[%d] tmp:%s tmp_time:%s XXX\n", __FUNCTION__, __LINE__, tmp,tmp_time);
				if(strcmp(tmp,tmp_time) == 0)
					return 0;
				else
                               		return connect_time ? uptime() - connect_time : 0;
			}
                } else {
			
			if ((fp = fopen(WAN_CONNECT_FILE,"r+")) == NULL)
				return 0;
			
			if((fd = fopen(WAN_CONNECT_DIFFERENCE,"w+")) == NULL)
                                return 0;

			fgets(tmp_time, sizeof(tmp_time), fp);
			fputs(tmp_time, fd);

                        fclose(fp);
                        fclose(fd);
			
                        return 0;
		}
        } else {
                /* open read-only file to get wan connect time stamp */
                if ((fp = fopen(WAN_CONNECT_FILE, "r+")) == NULL)
                        return 0;

                if (fgets(tmp, sizeof(tmp), fp))
                        connect_time = atol(tmp);

                fclose(fp);

                if( uptime() == connect_time )
                        return 1;
                else
                        return connect_time ? uptime() - connect_time : 0;
        }
}
