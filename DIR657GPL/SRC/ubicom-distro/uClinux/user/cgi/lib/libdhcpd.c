#include <stdio.h>
#include <signal.h>

#include "libdb.h"
#include "nvram.h"
#include "shvar.h"
#include "sutil.h"
#include "libdhcpd.h"

int get_dhcpd_leased_table(struct dhcpd_leased_table_s *client_list)
{
        FILE *fp, *fp_pid;
        int i = 0, j = 0;
        char res_rule[] = "dhcpd_reserve_XX";
        char *enable="", *name="", *ip="", *mac="";
        char reserve_ip[MAX_DHCPD_RESERVATION_NUMBER][16];
        char *p_dhcp_reserve = NULL;
        char fw_value[192]={0};

        struct html_leases_s leases[M_MAX_DHCP_LIST];
        memset(leases, 0, sizeof(struct html_leases_s)*M_MAX_DHCP_LIST);

        if(NVRAM_MATCH("wlan0_mode", "ap") != 0 || NVRAM_MATCH("dhcpd_enable","0") != 0)
                return -1;

        if ((fp_pid = fopen(UDHCPD_PID,"r")) == NULL) {
                cprintf("get_dhcp_client_list /var/run/udhcpd.pid error\n");
                return -1;
        } else { /*write dhcp list into file /var/misc/udhcpd.leases & html.leases */
                kill(read_pid(UDHCPD_PID), SIGUSR1);
                fclose(fp_pid);
        }

        cprintf("get_dhcp_client_list: wait for  open udhcpd.leases\n");
        usleep(1000);
        if ((fp = fopen("/var/misc/html.leases","r")) == NULL) {
                cprintf("get_dhcp_client_list: open html.leases error\n");
                return -1;
        }

        // Jery Lin Added for save to reserve_ip array from nvram dhcpd_reserve_XX
        memset(reserve_ip, 0, sizeof(reserve_ip));
        for (j = 0; j < MAX_DHCPD_RESERVATION_NUMBER; j++){
                snprintf(res_rule, sizeof(res_rule), "dhcpd_reserve_%02d",j);
                p_dhcp_reserve = nvram_safe_get(res_rule);
                if (p_dhcp_reserve == NULL || (strlen(p_dhcp_reserve) == 0))
                        continue;
                memcpy(fw_value, p_dhcp_reserve, strlen(p_dhcp_reserve));
                getStrtok(fw_value, "/", "%s %s %s %s ", &enable, &name, &ip, &mac);
                if (atoi(enable)==0)
                        continue;
                strcpy(reserve_ip[j], ip);
                cprintf("get_dhcp_client_list: reserve_ip[%d]=%s\n", j, reserve_ip[j]);
        }


        for( i = 0; i < M_MAX_DHCP_LIST; i++) {
                fread(&leases[i], sizeof(struct html_leases_s), 1, fp);
                if(leases[i].ip_addr[0] != 0x00) {
                        client_list->hostname = leases[i].hostname;
                        client_list->ip_addr = leases[i].ip_addr;
                        client_list->mac_addr = leases[i].mac_addr;

                        for (j = 0; j < MAX_DHCPD_RESERVATION_NUMBER; j++) {
                                if (strcmp(reserve_ip[j], client_list->ip_addr)==0)
                                        break;
			}

                        client_list->expired_at = (j<MAX_DHCPD_RESERVATION_NUMBER) ?
                                                    "Never":
                                                    leases[i].expires;
			client_list++;
                }
                else
                        break;
        }
        fclose(fp);
        return 0;
}

