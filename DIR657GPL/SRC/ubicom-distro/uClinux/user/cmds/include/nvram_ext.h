#ifndef __NVRAM_EXT_H
#define __NVRAM_EXT_H
#include "shutils.h"
#define foreach_nvram(i, max, p, prefix, buf, cont)	\
	for (; (i) < (max) && \
	(((p) = nvram_get(strcat_r((prefix), itoa(i), buf))) || (cont)); (i)++)
extern char *__nvram_map(const char *key1, const char *value, const char *key2);
extern char *nvram_map(const char *key1, const char *value, const char *key2, char *buf);
extern int nvram_find_index(const char *prefix, const char *value);
#endif //__NVRAM_EXT_H
