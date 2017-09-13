#include <stdio.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ssi.h"

#include "libdb.h"
#include "querys.h"
#define DEBUG(fmt, a...) do { }while(0)
#define WL_CLIENT_MAX 256
static char WL_MAC[WL_CLIENT_MAX][24];

int init_wireless_clients_mac()
{
	FILE *fp;
	char line[128], *token;
	int i = 0 ,rev = -1;

	if (!(fp = popen("/usr/sbin/wl authe_sta_list 2>/dev/null", "r"))) {
		perror("popen /usr/sbin/wl authe_sta_list");
		goto err_exit;
	}
	DEBUG("Init wireless MAC\n");
	while (fgets(line, sizeof(line), fp)) {

		token = strtok(line, " \t\n");
		if (!token)
			goto err_exit;
		if (!(token = strtok(NULL, " "))){
			goto err_exit;
		}
		strcpy(WL_MAC[i], token);
		WL_MAC[i][strlen(WL_MAC[i])-1] = '\0';
		DEBUG("WL_MAC:[%s]\n", WL_MAC[i]);
		if (i++ >= WL_CLIENT_MAX -1)
			break;
	}
	pclose(fp);
	return i - 1;
err_exit:
	return rev;
}

int is_wireless_client(const char *mac)
{
	int i;
	
	DEBUG("mac [%s] in wireless ?\n", mac);
	
	for (i = 0; WL_MAC[i][0] != '\0' && i < WL_CLIENT_MAX; i++) {
		DEBUG("%d: [%s] == [%s] ?\n", i, mac, WL_MAC[i]);
		if (strcasecmp(mac, WL_MAC[i]) == 0)
			return 1;
	}
	return 0;
}

#if 0
int arp_query()
{
	char *ipaddr, *hwtype, *flag, *hwaddr, *mask, *token;
	char line[512];
	FILE *fp1;
	int cnt = 0;

	if (!(fp1 = fopen("/proc/net/arp", "r"))) {
		return errno;
	}
	while (fgets(line, sizeof(line), fp1)) {
		if ( cnt == 0) {
			cnt++;
			continue;
		}
		token = strtok(line, " \t\n");
		ipaddr = token;
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		hwtype = token;
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		flag = token;	
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		hwaddr = token;	
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		mask = token;	
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		token[strlen(token)-1] = '\0';
	
		cprintf("%d::%s/%s/%s/%s/%s/%s//\n", cnt++, ipaddr, hwtype, flag, hwaddr, mask, token);	
	}
	fclose(fp1);
	return 0;
}
#endif
int arp_alive(const char *ip)
{
	char cmd[128], dev[16];
	int rev;
	
	if (query_record("lan_ifname", dev, sizeof(dev)) != 0)
		return -1;
	sprintf(cmd, "arping -f -w 1 -c 1 -q -i %s %s", dev, ip);
	rev = system(cmd);
	return WIFEXITED(rev) ? WEXITSTATUS(rev): -1;
}

int dynamicdhcp_main(int argc, char *argv[])
{
	char line[512], hostname[128], temp[128];
       	char *token, *expired_time, *mac, *ip;
	time_t exp_time;
	FILE *fp;
	int wireless = 1, ret = -1;

	ret = system("/usr/bin/dumpleases -f /tmp/udhcpd.leases -c > /tmp/dnsmasq.lease");
	if (ret < 0 || ret ==127){
		fprintf(stdout, "");	
		return errno;
	}
	
	if (!(fp = fopen("/tmp/dnsmasq.lease", "r"))) {
		fprintf(stdout, "");	
		return errno;
	}
	if (argc > 1) {
	       	if (strcmp(argv[1], "lan") == 0) {
			//arp_query();
			wireless = 0;
			init_wireless_clients_mac();
		} else {
			fprintf(stderr, "Usage: %s [lan] to display lan of wire\n", argv[0]);
			goto out;
		}
	}
	while (fgets(line, sizeof(line), fp)) {

		token = strtok(line, " \t\n");
		expired_time = token;
		exp_time = atoi(expired_time);
		strcpy(temp,asctime(localtime(&exp_time)));
		temp[strlen(temp)-1] = '\0';
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		mac = token;
		if (wireless == 0 && (is_wireless_client(mac) == 1))
			continue;
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		ip = token;	
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		if (strcmp(token, "") ==0)
			strcpy(hostname, "Unknow-host");
		else if (strcmp(token, "*") != 0)
				strcpy(hostname, token);
		else
			strcpy(hostname, ip);
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		token[strlen(token)-1] = '\0';
		if (arp_alive(ip) == 0)
			fprintf(stdout, "%s/%s/%s/%s/%s,", hostname, ip, mac, temp, token);	
	}
out:
	fclose(fp);
	return 0;
}
