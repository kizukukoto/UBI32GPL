#include <string.h>
#include <stdio.h>
#include <sys/types.h>

#include "libdb.h"
#include "ssc.h"
#include "public.h"
#include "querys.h"
#include "netinf.h"

struct __ddns_struct {
        const char *param;
        const char *desc;
        int (*fn)(int, char *[]);
};

unsigned char *get_ping_app_stat(unsigned char *host)
{
        FILE *fp;
        char *ping_result;
	char cmd[128]={0};
        unsigned char returnVal[256];

        memset(returnVal, 0, sizeof(returnVal));
 	//parse_special_char(host);
        //_system("ping -c 1 \"%s\" > /var/misc/ping_app.txt", host);
        sprintf(cmd,"ping -c 1 \"%s\" > /var/misc/ping_app.txt", host);
	system(cmd);
        fp = fopen("/var/misc/ping_app.txt", "r");
        if(fp == NULL) {
                cprintf("open /var/misc/ping_app.txt error\n");
                return NULL;
        }
        fread(returnVal, sizeof(returnVal), 1, fp);
        cprintf("%s", returnVal);
        if(strlen(returnVal) > 1)
        {
                if(strstr(returnVal, "transmitted")){//transmitted
                if(strstr(returnVal, "100%")) //100% packet loss
                        ping_result = "Fail";
                else
                        ping_result = "Success";
            }
            else
                    ping_result = "Unknown Host";
        }
        else
                ping_result = "Unknown Host";

        fclose(fp);
        return ping_result;
}



int ddns_get_info(int argc, char *argv[])
{
	FILE *fp;
	FILE *fp2;
	char wan_interface[8] = {0};
	char ddns_status[64] = {0};
	unsigned char web[] = "168.95.1.1";
	char buf[256]={0}, host_ip[16]={0},ipaddr[16]={0},cmd[128]={0};
	char *ip_s=NULL, *ip_e=NULL;
	char *wan_proto= nvram_safe_get("wan_proto");
	char *ddns_type = nvram_safe_get("ddns_type");
	
	if( strcmp(wan_proto,"pppoe") == 0 
		|| strcmp(wan_proto,"pptp") == 0 
		|| strcmp(wan_proto,"l2tp") == 0 
#ifdef CONFIG_3G_USB_CLIENT		
		|| strcmp(wan_proto,"usb3g") == 0
#endif		
		)	      
  		strcpy(wan_interface,"ppp0");
	else
		strcpy(wan_interface,nvram_safe_get("wan_eth"));
	
	if(NVRAM_MATCH("ddns_enable","1") == 0)
		printf("Dynamic DNS service is not enabled.");	
	else {
		get_ip(wan_interface,ipaddr);
		if(ipaddr< 0)
			printf("No update action.There is no WAN IP address.");
		else {
			if(strcmp(get_ping_app_stat(web), "Success") != 0)
				printf("Internet Offline");
			else {
				fp = fopen("/var/tmp/ddns_status","r"); /*if we have updated, then this file is exist*/
				if(fp) {
				 	fgets(ddns_status, sizeof(ddns_status), fp);
                			fclose(fp);
					if(strstr(ddns_status, "SUCCESS"))
						printf("success");
					else if(strstr(ddns_status, "Authentication failed"))
						printf("Update Fail!");
					else
						printf("The Host Name is invalid");	
				} else {
        				//_system("ping -c 1 \"%s\" > /var/etc/ddns_check.txt 2>&1", nvram_safe_get("ddns_hostname"));
        				sprintf(cmd,"ping -c 1 \"%s\" > /var/etc/ddns_check.txt 2>&1", nvram_safe_get("ddns_hostname"));
					system(cmd);
					fp2 = fopen("/var/etc/ddns_check.txt", "r");
					if(fp2) {		
						memset(buf, 0, 80);
						fgets(buf, sizeof(buf), fp2);
						ip_s = strstr(buf, "(");
						ip_e = strstr(buf, ")");
						if(ip_s && ip_e) {
							strncpy(host_ip, ip_s+1, ip_e-(ip_s+1));
							if (strcasecmp( ddns_type, "DynDns.org" ) == 0 || strncasecmp(ddns_type, "dyndns.com", 10) ==0 ) {
								FILE *fp_checkip = fopen("/var/etc/dyndns_checkip", "r");
								char buf_checkip[20]={0};
								if(fp_checkip) { 
									fgets(buf_checkip, sizeof(buf_checkip), fp_checkip);
									cprintf("dyndns_checkip=%s=, host_ip=%s=\n", buf_checkip, host_ip);
									if(strncmp(buf_checkip, host_ip, strlen(host_ip))==0)
										printf("success");	
									fclose(fp_checkip);	
								} else
									printf("fail");
							} else if(strcmp(ipaddr, host_ip)==0) {
								printf("success");
							}
						} else
							printf("fail");

						fclose(fp2);
					} else {
						printf("open /var/tmp/ddns_check.txt fail");
						printf("fail");
					}	
        			}					
			}
		}
	}
	return 0;
}


static int misc_ddns_help(struct __ddns_struct *p)
{
        cprintf("Usage:\n");

        for (; p && p->param; p++)
                cprintf("\t%s: %s\n", p->param, p->desc);

        cprintf("\n");
        return 0;
}


int get_ddns_status_main(int argc, char *argv[])
{
	int ret = -1;
        struct __ddns_struct *p, list[] = {
                { "ddns_status", "Show ddns connected status", ddns_get_info},
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
        ret = misc_ddns_help(list);
out:
        return ret;

}
