#include <stdio.h>
#include "libcameo.h"

#define LINE_LEN	256

static int dns_getline(const char *file)
{
	int lines = -1;
	char cmds[128];
	FILE *fp = NULL;

	bzero(cmds, sizeof(cmds));
	sprintf(cmds, "wc %s|awk '{print $1}'", DNS_CONF);
	if ((fp = popen(cmds, "r")) == NULL)
		return -1;

	fscanf(fp, "%d", &lines);
	pclose(fp);

	return lines;
}

static void dns_conf2buf(const char *fname, char buf[][LINE_LEN])
{
	int idx = 0;
	char line[LINE_LEN];
	FILE *fp = fopen(fname, "r");

	if (fp == NULL)
		return;

	bzero(line, sizeof(line));
	while (fscanf(fp, "%[^\n]\n", line) != EOF) {
		if (*line == '\0') {
			fseek(fp, 1, SEEK_CUR);
			continue;
		}
		strcpy(buf[idx++], line);
		bzero(line, sizeof(line));
	}

	fclose(fp);
}

static void
dns_del_ip(const char *ip, char buf[][LINE_LEN], int entrys)
{
	int idx = 0;
	char conf_item[64], conf_ip[16];

	for (; idx < entrys; idx++) {
		bzero(conf_item, sizeof(conf_item));
		bzero(conf_ip, sizeof(conf_ip));

		sscanf(buf[idx], "%[^ ] %[^\n]\n", conf_item, conf_ip);
		if (strcmp(ip, conf_ip) == 0)
			bzero(buf[idx], LINE_LEN);
	}
}

static void
dns_ins_ip(const char *ip, char buf[][LINE_LEN], int entrys)
{
	int idx = 0;

	if (entrys == 1) {	// resolv.conf is empty
		sprintf(buf[0], "nameserver %s", ip);
		return;
	}

	for (; idx < entrys; idx++) {
		if (strstr(buf[idx], "nameserver")) {
			int __idx = entrys - 2;

			for (; __idx >= idx; __idx--) {
				strcpy(buf[__idx + 1], buf[__idx]);
			}
			sprintf(buf[idx], "nameserver %s", ip);

			return;
		}
	}

	sprintf(buf[0], "nameserver %s", ip);
}

static void dns_buf2conf(const char *fname, char buf[][LINE_LEN], int entrys)
{
	int idx = 0;
	FILE *fp = fopen(fname, "w");

	if (fp == NULL)
		return;

	for (; idx < entrys; idx++) {
		if (strlen(buf[idx]) > 0)
			fprintf(fp, "%s\n", buf[idx]);
	}

	fclose(fp);
}

static int dns_buf2list(const char buf[][LINE_LEN], char *list, int entrys)
{
	int idx = 0;
	int cnt = 0;
	char conf_item[64], conf_ip[16];

	for (; idx < entrys; idx++) {
		bzero(conf_item, sizeof(conf_item));
		bzero(conf_ip, sizeof(conf_ip));

		sscanf(buf[idx], "%[^ ] %[^\n]\n", conf_item, conf_ip);
		if (*conf_item != NULL && *conf_ip != NULL && strcmp(conf_item, "nameserver") == 0) {
			strcat(list, conf_ip);
			strcat(list, " ");
			cnt++;
		}
	}

	return cnt;
}

static int ip_format_correct(const char *addr)
{
	int i = 0, ret = -1;
	char *p, *ip[4], taddr[16];

	strncpy(taddr, addr, sizeof(taddr));
	p = taddr;

	ip[0] = strsep(&p, ".");
	ip[1] = strsep(&p, ".");
	ip[2] = strsep(&p, ".");
	ip[3] = p;

	if (ip[3] != NULL && strstr(p, "."))
		goto out;

	for (; i < 4; i++) {
		if (ip[i] == NULL || *ip[i] == '\0')
			goto out;

		if (atoi(ip[i]) < 0 || atoi(ip[i]) > 255)
			goto out;
	}

	ret = 0;
out:
	return ret;
}

int dns_append(const char *ip)
{
	int rev = -1;
	FILE *fp = fopen(DNS_CONF, "a");

	if (fp == NULL)
		goto append_error;

	if (ip == NULL || *ip == '\0' || ip_format_correct(ip) != 0)
		goto append_error;

	fprintf(fp, "nameserver %s\n", ip);
	fprintf(stderr, "XXXXXXX DNS APPEND:%s\n", ip);
	fclose(fp);
	rev = 0;

append_error:
	return rev;
}

int dns_delete(const char *ip)
{
	int idx = 0;
	int entrys = dns_getline(DNS_CONF);
	char tmp_entry[entrys][LINE_LEN];

	for(; idx < entrys; idx++)
		bzero(tmp_entry[idx], sizeof(tmp_entry[idx]));

	if (strcmp(ip, "all") != 0) {
		dns_conf2buf(DNS_CONF, tmp_entry);
		dns_del_ip(ip, tmp_entry, entrys);
	}
	dns_buf2conf(DNS_CONF, tmp_entry, entrys);

	return -1;	
}

int dns_insert(const char *ip)
{
	int idx = 0;
	int entrys = dns_getline(DNS_CONF) + 1;
	char tmp_entry[entrys][LINE_LEN];

	for(; idx < entrys; idx++)
		bzero(tmp_entry[idx], sizeof(tmp_entry[idx]));

	dns_conf2buf(DNS_CONF, tmp_entry);
	dns_ins_ip(ip, tmp_entry, entrys);
	dns_buf2conf(DNS_CONF, tmp_entry, entrys);

	return -1;
}

int dns_get(char *buf)
{
	int idx = 0;
	int entrys = dns_getline(DNS_CONF);
	char tmp_entry[entrys][LINE_LEN];

	dns_conf2buf(DNS_CONF, tmp_entry);
	return dns_buf2list(tmp_entry, buf, entrys);
}
