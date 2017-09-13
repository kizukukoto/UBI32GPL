#include <stdio.h>
#include <string.h>

int get_wl_domain(int argc, char *argv[])
{
	FILE *fp = popen("/usr/sbin/wl channels", "r");
	char channel_list[128];

	if (fp == NULL)
		return 0;
	/* 
	 * This definition is for MP, 2.4G only have 2 channel type now.
	 * 0x10, 0x30 is our MP define.
	 * */
	struct {
		char *channels;
		char *domain;
	} *p, dm_list[] = {
		{"1 2 3 4 5 6 7 8 9 10 11 ", "0x10"},
		{"1 2 3 4 5 6 7 8 9 10 11 12 13 ", "0x30"},
		{"default", "0x10"},
		{NULL, NULL}
	};

	bzero(channel_list, sizeof(channel_list));
	fscanf(fp, "%[^\n]", channel_list);

	for (p = dm_list; p != NULL && p->channels != NULL; p++) {
		if (strcmp(channel_list, p->channels) == 0 ||
			strcmp("default", p->channels) == 0) {
			printf("%s", p->domain);
			break;
		}
	}

	pclose(fp);
	return 1;
}
