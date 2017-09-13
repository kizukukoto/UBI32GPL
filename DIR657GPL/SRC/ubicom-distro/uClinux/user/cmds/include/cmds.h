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
//#define debug(fmt, a...)	__rd__("###<%s.fmt, ##a)
#define rd(rev, fmt, a...)	{ if (rev != 0) __rd__("RD :%s(%d): " fmt, \
		__FUNCTION__, __LINE__,##a);}
#define err_msg(fmt, a...)	__rd__("#ERR#<%s.%d>: " fmt, __FUNCTION__, __LINE__, ##a);
#define debug(fmt, a...)	__rd__("#DBG#<%s.%d>: " fmt, __FUNCTION__, __LINE__, ##a);
#define trace(fmt, a...)	__rd__("#TAC#<%s.%d>: " fmt, __FUNCTION__, __LINE__, ##a);



#ifndef CONFIG_BCM_IPSEC
/* Eagle nvram not implement! */
#include "nvram.h"
#ifndef CONFIG_UBICOM_ARCH
/*
 * Inversely match an NVRAM variable.
 * @param       name    name of variable to match
 * @param       match   value to compare against value of variable
 * @return      TRUE if variable is defined and its value is not string
 *              equal to invmatch or FALSE otherwise
 */
static __inline__ int
nvram_invmatch(char *name, char *invmatch) {
        const char *value = nvram_get(name);
        return (value && strcmp(value, invmatch));
}
#endif //CONFIG_UBICOM_ARCH
#endif

#endif  //__CMDS_H
