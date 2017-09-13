
#ifndef __CMDS_H
#define __CMDS_H
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
struct subcmd {
	char *action;
	int (*fn)(int argc, char *argv[]);
	char *help_string;
};
extern struct subcmd *lookup_subcmd(int argc, char *argv[], struct subcmd *cmds);
extern int lookup_subcmd_then_callout(int argc, char *argv[], struct subcmd *cmds);
extern int lock_prog(int argc, char *argv[], int force);
extern void unlock_prog(void);
extern int init_cmds_conf(const char *dbg_path, int lock_en);
extern void uninit_cmds_conf();
//#define debug(fmt, a...)	__rd__("###<%s.fmt, ##a)
extern void __rd__(const char *fmt, ...);
#define rd(rev, fmt, a...)	{ if (rev != 0) __rd__("RD :%s(%d): " fmt, \
		__FUNCTION__, __LINE__,##a);}
#define err_msg(fmt, a...)	__rd__("#ERR#<%s.%d>: " fmt, __FUNCTION__, __LINE__, ##a)
#define debug(fmt, a...)	__rd__("#DBG#<%s.%d>: " fmt, __FUNCTION__, __LINE__, ##a)
#define msg(fmt, a...)		__rd__(fmt, ##a)
#define trace(fmt, a...)	__rd__("#TAC#<%s.%d>: " fmt, __FUNCTION__, __LINE__, ##a)

#endif  //__CMDS_H
