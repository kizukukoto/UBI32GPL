#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdarg.h>
struct ssi_helper {
	char name[16];
	void (*fn) (char *filename, FILE * fp, int argc, char *argv[]);
};

extern struct ssi_helper helpers[];

extern void make_back_msg(const char *fmt, ...);
#endif /* __HELPER_H__ */
