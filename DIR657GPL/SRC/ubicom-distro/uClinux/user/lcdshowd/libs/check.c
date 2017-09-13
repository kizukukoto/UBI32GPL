#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int __check_ip_format_v4(const char *unit)
{
	int i, res = -1;

	if (unit == NULL || *unit == '\0')
		goto out;
	for (i = 0; i < strlen(unit); i++)
		if (isdigit(unit[i]) == 0)
			goto out;
	if (atoi(unit) < 0 || atoi(unit) > 255)
		goto out;
	res = 0;
out:
	return res;
}

static int __check_netmask_format_v4(const char *str)
{
	char flag = 0;
	int res = -1, i = 0, v = atoi(str);

	for (; i < 8; i++) {
		if (((v >> (7 - i)) & 1) == 0)
			flag = 1;
		else if (((v >> (7 - i)) & 1) == 1 && flag == 1)
			goto out;
	}
	res = (flag == 1)? 0: 1;
out:
	return res;
}

static int __check_mac(const char *hc)
{
	int i;

	if (strlen(hc) != 2)
		goto out;
	for (i = 0; i < strlen(hc); i++) {
		if (!((hc[i] >= '0' && hc[i] <= '9') ||
			(hc[i] >= 'a' && hc[i] <= 'f') ||
			(hc[i] >= 'A' && hc[i] <= 'F')))
			goto out;
	}
	return 0;
out:
	return -1;
}

int check_ip_format_v4(const char *str)
{
	int i, res = -1;
	char *s, *p, tmp[1024];

	p = tmp;
	strcpy(tmp, str);

	for (i = 0; i < 4; i++) {
		if ((s = strsep(&p, ".")) == NULL)
			goto out;
		if (__check_ip_format_v4(s) != 0)
			goto out;
	}

	if (strsep(&p, ".") != NULL)
		goto out;

	res = 0;
out:
	return res;
}

int check_netmask_format_v4(const char *str)
{
	int i = 0, res = -1, flag = 0;
	char *p, tmp[1024];

	if (check_ip_format_v4(str) != 0)
		goto out;

	p = tmp;
	strcpy(tmp, str);

	for (; i < 4; i++) {
		int ret = __check_netmask_format_v4(strsep(&p, "."));	/* 0: find 0 bit */

		if (ret == -1)
			goto out;
		if (ret == 0)
			flag = 1;
		else if (ret == 1 && flag == 1)
			goto out;
	}

	res = 0;
out:
	return res;
}

int check_mac(const char *str)
{
	int i, res = -1;
	char *s, *p, buf[18];

	if (str == NULL || strlen(str) != 17)
		goto out;

	p = buf;
	bzero(buf, sizeof(buf));
	strcpy(buf, str);

	for (i = 0; (s = strsep(&p, ":")); i++) {
		if (__check_mac(s) != 0)
			goto out;
	}
	if (i != 6)
		goto out;
	res = 0;
out:
	return res;
}
