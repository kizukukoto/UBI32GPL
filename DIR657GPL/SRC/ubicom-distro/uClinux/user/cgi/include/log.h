#ifndef __LOG_H
#define __LOG_H
#include <stdio.h>

//#define __SSI_LOGGER__
#ifdef __SSI_LOGGER__
#define SSI_LOGGER_FILE	"/tmp/ssi.log"
extern FILE *LOGFD;
#define syslog(pri, fmt, ...) fprintf(LOGFD, fmt, ## __VA_ARGS__)
#else
#include <syslog.h>
#endif //__SSI_LOGGER__

#ifdef ADD_SYSLOG
#define SYSLOG(fmt, ...) syslog(LOG_INFO, fmt, ## __VA_ARGS__)
#define LOGERR(fmt, ...) syslog(LOG_ERR, fmt, ## __VA_ARGS__)
#define STDOUT(fmt, ...) do {} while(0)
#else
#define SYSLOG(fmt, ...) do {} while(0)
#define LOGERR(fmt, ...) do {} while(0)
#define STDOUT(fmt, ...) do {} while(0)
#endif
#endif //__LOG_H
