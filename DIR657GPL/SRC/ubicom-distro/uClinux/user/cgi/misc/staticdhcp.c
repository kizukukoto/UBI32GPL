#include <stdio.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "libdb.h"
#include "ssi.h"
#include "querys.h"

#define max_static 20

int dynamic_compare(char *static_ip)
{
	char line[512];
	char *token, *expired_time, *mac, *ip;
	FILE *fp;
	int ret = -1;

	if (!(fp = fopen("/tmp/dnsmasq.lease", "r"))) {
		perror("/tmp/dnsmasq.lease");
		return errno;
	}

	while (fgets(line, sizeof(line), fp)) {

		token = strtok(line, " \t\n");
		expired_time = token;
		if (!token)
			break;
		if (!(token = strtok(NULL, " "))){
			continue;
		}
		mac = token;
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
		if (strcmp(static_ip, ip) == 0 ){
			ret = 1;
			break;
		}
	}
	fclose(fp);
	return ret;
}

int connect_compare(char **ip)
{
	char line[512], *token;
	int ret = -1;
	FILE *fp;
	
	if (!(fp = fopen("/proc/net/arp", "r"))) {
		perror("/proc/net/arp");
		goto compare_err;
	}
	
	if (!(fgets(line, sizeof(line), fp)))
		goto compare_err;
	
	while (fgets(line, sizeof(line), fp)) {
		token = strtok(line, " ");
		if (!token)
			goto compare_err;
		if (strcmp(token, *ip) == 0){
			ret = 1;
		}
	}
compare_err:
	if (fp)
		fclose(fp);
	return ret;
}

int static_paser(char *string, char **enable, char **name, char **ip, char **mac)
{
	char *token;
	token = strtok(string, "/");
	if (!token)
		goto paser_err;
	*enable = token;
	token = strtok(NULL, "/");
	if (!token)
		goto paser_err;
	*name = token;
	token = strtok(NULL, "/");
	if (!token)
		goto paser_err;
	*ip = token;
	token = strtok(NULL, "/");
	if (!token)
		goto paser_err;
	*mac = token;
	
	return 1;
paser_err:
	return -1;
}

int staticdhcp_main(int argc, char *argv[])
{
	char temp[128], value[256];
       	char *enable, *name, *ip, *mac;
	int i, ret = -1;

	if (nvram_init(NULL) < 0)
		return 0;
	
 	for (i = 0; i < max_static; i++) {
		ret = -1;

		if ( i < 10)
			sprintf(temp,"dhcpd_reserve_0%d", i);
		else
			sprintf(temp,"dhcpd_reserve_%d", i);
						
		if (query_record(temp, value, sizeof(value)) < 0)
			break;
		if ((static_paser(value, &enable, &name, &ip, &mac)) < 0)
			break;
		if ((dynamic_compare(ip)) > 0)
			break;
		if (!strcmp(enable,"1")){
			if (connect_compare(&ip) == 1)
				fprintf(stdout, "%s/%s/%s,", ip, name, mac);
		}
	}

	return ret;
}
