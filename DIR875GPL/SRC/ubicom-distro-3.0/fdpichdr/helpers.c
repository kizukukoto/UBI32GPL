/*
 * File:         helpers.c
 * Author:       Mike Frysinger <michael.frysinger@analog.com>
 * Description:  some common utility functions
 * Modified:     Copyright 2006-2008 Analog Devices Inc.
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 * Licensed under the GPL-2, see the file COPYING in this dir
 */

#include "headers.h"
#include "helpers.h"

void *xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL)
		errp("malloc(%zi) returned NULL!", size);
	return ret;
}

void *xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);
	if (ret == NULL)
		errp("realloc(%p, %zi) returned NULL!", ptr, size);
	return ret;
}

char *xstrdup(const char *s)
{
	char *ret = strdup(s);
	if (ret == NULL)
		errp("strdup(%p) returned NULL!", s);
	return ret;
}

int parse_bool(const char *boo)
{
	if (strcmp(boo, "1") == 0 || strcasecmp(boo, "yes") == 0 ||	
	    strcasecmp(boo, "y") == 0 || strcasecmp(boo, "true") == 0)
		return 1;
	if (strcmp(boo, "0") == 0 || strcasecmp(boo, "no") == 0 ||
	    strcasecmp(boo, "n") == 0 || strcasecmp(boo, "false") == 0)
		return 0;
	err("Invalid boolean: '%s'", boo);
}

size_t parse_int_hex(const char *str)
{
	char *endp;
	size_t ret;

	/* first we parse as an integer */
	ret = strtol(str, &endp, 10);
	if (!*endp)
		return ret;

	/* then we parse as a hex */
	ret = strtol(str, &endp, 16);
	if (!*endp)
		return ret;

	err("Value is not integer or hex: '%s'", str);
}

ssize_t read_retry(int fd, void *buf, size_t count)
{
	ssize_t ret = 0, temp_ret;
	while (count > 0) {
		temp_ret = read(fd, buf, count);
		if (temp_ret > 0) {
			ret += temp_ret;
			buf += temp_ret;
			count -= temp_ret;
		} else if (temp_ret == 0) {
			break;
		} else {
			if (errno == EINTR)
				continue;
			ret = -1;
			break;
		}
	}
	return ret;
}
