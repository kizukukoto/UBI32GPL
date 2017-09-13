#include <stdio.h>
#include <string.h>

char *get_local_lan(char *data, int *i)
{
	FILE *fp;
	char path[128];
	char buf[256], tmp[24];

	char *ptr;
	ptr = data;

	struct pc {
		char ipaddr[18];
		char hostname[256];
		char macaddr[18];
	} pc_info;

#ifndef PC
	sprintf(path, "cat %s", "/var/tmp/local_lan_ip_tmp");
#else
	sprintf(path, "cat %s", "local_lan_ip");
#endif
	fp = popen(path, "r");

	if (fp == NULL) {
		fprintf(stderr, "popen error");
		goto out;		
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		sscanf(buf, "%[^/]%[\\/]%s %[\\/]%s", pc_info.ipaddr, tmp, 
						      pc_info.hostname, tmp,
						      pc_info.macaddr);
		if (i == 0)
			sprintf(ptr, "%s", pc_info.macaddr);
		else
			sprintf(ptr, "%s%s", ptr, pc_info.macaddr);
		ptr += 24;
		(*i)++;
	}

	ptr = data;

	pclose(fp);
out:
	return data;
}
