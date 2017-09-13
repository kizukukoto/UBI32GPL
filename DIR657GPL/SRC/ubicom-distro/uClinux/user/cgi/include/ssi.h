#ifndef __SSI_H
#define __SSI_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include "mibinf.h"
//#include "socketlib.h"
#include "libsw4.h"
//#include "virtual_file.h"

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
				fprintf(fp, fmt, ## args); \
				fclose(fp); \
			} \
} while (0)

#define HTML_TITLE_HEAD "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n\n"
#define HTML_END        "</HTML>\n"

void internal_error( char* reason );
void not_found( char* filename );
void debug_message( char* message );

char *do_ssc();

struct subcmd {
	char *action;
	int (*fn)(int argc, char *argv[]);
};

#endif //__SSI_H
