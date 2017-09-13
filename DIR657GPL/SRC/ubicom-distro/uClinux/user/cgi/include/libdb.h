#ifndef __LIBDB__
#define __LIBDB__
#include <string.h>
#include <nvram.h>


#ifdef LINUX_NVRAM	/* we promise NVRAM_NVRAM always return @ture if matched */
#define NVRAM_MATCH(name, match) 	(!nvram_match(name, match))
#else
#define NVRAM_MATCH(name, match)	nvram_match(name, match)
#endif

static inline int query_record(const char *key, char *value, int size)
{
	char *p;
	if ((p = nvram_get(key)) == NULL)
		return -1;
	strncpy(value, p, size);
	return 0;
}

extern int update_record(const char *key, const char *value);
extern int nvram_safe_set(const char *key, const char *value);
#endif
