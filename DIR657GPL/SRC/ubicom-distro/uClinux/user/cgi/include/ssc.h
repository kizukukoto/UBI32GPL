#ifndef __SSC_H
#define __SSC_H
#include <stdio.h>
#include "nvram.h"
#include "public.h"
#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
		fprintf(fp, fmt, ## args); \
		fclose(fp); \
	} \
} while (0)

#define MAGIC_IV_NONULL	((char *)-1)
struct items{
	char *key;
	char *_default;
};
#define IRFG_NV_OFFSET	1

struct items_range{
	char *key;
	int start;
	int end;
	char *_default;
	unsigned int _flags;
	char *_base;
};

extern inline void __do_apply(struct ssc_s *obj);

extern void __post2nvram_range(struct ssc_s *obj);
extern void __post2nvram(struct ssc_s *obj);

extern void *do_apply_cgi_range(struct ssc_s *obj);
extern void *do_apply_cgi(struct ssc_s *obj);
#endif
