#ifndef __LIB_CAMEO_H__
#define __LIB_CAMEO_H__

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#define SKIP_SPACE(p) while (isspace(*(p)) && *(p) != '\0'){(p)++;}
#define JUMP_SPACE(p) while (!isspace(*(p)) && *(p) != '\0'){(p)++;}


/*
 * args: initial args by @argc, @argv.
 * ex:
 * 	char *a1, *a2, *a3
 * 	args(argc, argv, &a1, &a2, &a3);
 * 	// @a1 = argv[0], @a2 = argv[1] ...
 * */
extern void __args(int argc, char *argv[], char **arg[]);
#define args(argc, argv, s1, s...)  ({ \
         char **v[] = {s1, ## s, NULL }; \
         __args(argc, argv, v); \
	 })
extern int split(char *buf, const char *delim, char *tok[], int strip);
extern char *chop(char *io);
extern int netmask2int(const char *netmask);
extern int same_subnet(const char *ip1, const char *ip2, const char *netmask);
extern int str_replace(char *str, const char *dst, const char *tok, const size_t slen);
/*
 * Deleted a string @delim from @s, the string list @s are separate by
 * SPACE (test by isspace()). the @delim MUST match by string list @s
 * EX:
 * 	@s = "123 456 789", @delim = "456",
 * 		then output: @s = "123 789", 0 return
 * 	@s = "123 456 789", @delim = "45",
 * 		then output: @s = "123 456 789", -1 return
 * 	@s = "123 456 789", @delim = "56",
 * 		then output: @s = "123 456 789", -1 retrun
 * 	@s = "123 456  789 " @delim = "456",
 * 		then output: @s = "123  789 ", 0 return
 * */
extern int strlist_del(char *s, const char *delim);
extern int _system(const char *fmt, ...);
/*
 * replace string from a opened file
 * ex: stream_replace_line(fd, "abc", "ABC");
 * 	context:
 * 		01234
 * 		abcdefg
 * 		hijklm
 * 	file fd will replaced as:
 * 		01234
 * 		ABC
 * 		hijklm
 *
 * */
extern int stream_replace_line(int fd, const char *from, const char *to);

#define DNS_CONF	"/tmp/resolv.conf"
extern int dns_append(const char *ip);
extern int dns_insert(const char *ip);
extern int dns_delete(const char *ip);
extern int dns_get(char *buf);	// split by space charactor, ex: 172.21.1.1 172.21.1.2 172.21.1.3

#define MTD_PARTITION   "/dev/mtd3"

extern int getmac(int cnt, char *macaddr);	/* Ex: getmac(1, buf) */
extern int clone_mac(const char *prefix, const char *alias, const char *dev);
extern int kill_from_pidfile(int sig, const char *fmt, ...);
extern pid_t get_pid_from_pidfile(const char *fmt, ...);
#endif // __LIB_CAMEO_H__

