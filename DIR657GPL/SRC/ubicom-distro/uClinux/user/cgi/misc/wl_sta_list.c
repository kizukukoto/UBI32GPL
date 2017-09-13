#include <stdio.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#define dnsmasq_file "/tmp/dnsmasq.lease"

int wl_sta_list_main(int argc, char *argv[])
{
	char line[128], line2[128], temp[128], tmp[128];
	char mac_a[64], connect_time_a[64], idle_time_a[64];
       	char *token, *mac, *connect_time, *idle_time, *ipaddr, *hostname;
	FILE *fp, *fp2;
	int i = 0, j, find_fg = 0, ret = -1;
	
	ret = system("/usr/bin/dumpleases -f /tmp/udhcpd.leases -c > /tmp/dnsmasq.lease");
        if (ret < 0 || ret ==127){
                fprintf(stdout, "");
                return errno;
        }

	if (!(fp = popen("/usr/sbin/wl authe_sta_list", "r"))) {
		perror("popen /usr/sbin/wl authe_sta_list");
		goto err_exit;
	}
 
	while (fgets(line, sizeof(line), fp)) {

		token = strtok(line, " \t\n");
		if (!token)
			goto err_exit;
		if (!(token = strtok(NULL, " "))){
			goto err_exit;
		}
		mac = token;
		strcpy(mac_a, mac);
		mac_a[strlen(mac_a)-1] = '\0';

		strcpy(temp, "/usr/sbin/wl sta_info " );
		strcat(temp, mac_a);
		
		if (!(fp2 = popen(temp, "r"))) {
			perror("popen /usr/sbin/wl sta_info");
			continue;
		}

		while (fgets(line2, sizeof(line2), fp2 )) {
			if ((token = strstr(line2, "idle")) != 0 ){
				token = strtok(token, " ");
				token = strtok(NULL, " ");
				idle_time = token;
				strcpy( idle_time_a, idle_time);
				idle_time_a[strlen(idle_time_a)] = '\0';
			}else{
			       	if ((token = strstr(line2, "network")) != 0){
					token = strtok(token, " ");
					token = strtok(NULL, " ");
					connect_time = token;
					strcpy(connect_time_a, connect_time);
					connect_time_a[strlen(connect_time_a)] = '\0';
				}
			}
		}
		pclose(fp2);

		if(!(fp2 = fopen(dnsmasq_file, "r")))
			fprintf(stdout, "fopen fail\n");

		while(fgets(temp, sizeof(temp), fp2)) {
			token = strtok(temp, " ");
			mac = strtok(NULL, " ");
			ipaddr = strtok(NULL, " ");
			hostname = strtok(NULL, " ");
			bzero(tmp, sizeof(tmp));
			for(j = 0; j < strlen(mac_a); j++)
				tmp[j] = tolower(mac_a[j]);

			if(strcmp(tmp, mac) == 0) {
				find_fg = 1;
				break;
			}
		}
		fclose(fp2);

		if (find_fg) {
			if (i != 0)
				fprintf(stdout, ",");
				
			fprintf(stdout, "%s/%s/%s/%s/%s", ipaddr, hostname, mac_a,
					connect_time_a, idle_time_a);
		}
		find_fg = 0;
		i++;
	}
	pclose(fp);
	return 0;
err_exit:
	fprintf(stdout, " ");
	return -1;	
}
