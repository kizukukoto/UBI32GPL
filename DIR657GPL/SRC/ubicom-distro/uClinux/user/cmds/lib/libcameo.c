#include "libcameo.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <stdarg.h>
#include <errno.h>
#include "shutils.h"
#include "cmds.h"
/*
 * split string @buf by deimitor, such as split utility
 * Input:
 * 	@buf:	string
 * 	@strip:	not zero for strip repeated delimitor
 * Output:
 * 	@tok:	Stored each of results from strsep()
 * Retrun:
 * 	Retrun length of @tok
 * 
 * FIXME:
 * 	buf = "1bbb2bb3", strip = 0
 * 	return [1],[],[],2,[],[3]:
 * 		which output each slice of delimitor - 1 while paser with @stip = 0
 * 		it seems due to the behavior of @strsep()
 * */

int split(char *buf, const char *delim, char *tok[], int strip)
{
	int i = 0;
	char *p;
	
	while ((p = strsep(&buf, delim)) != NULL) {
		if (strip && (strchr(delim, *p) != NULL)) {
			continue;
		}
		tok[i++] = p;
	}
	if (buf)
		tok[i++] = buf;
	return i;
}

char *chop(char *io)
{
	char *p;

	p = io + strlen(io) - 1;
	while ( *p == '\n' || *p == '\r') {
		*p = '\0';
		p--;
	}
	return io;
}

/* 
 * Parse netmask 255.255.255.0 to maskbit 24 
 * XXX: ignored protect function. such as 255.255.0.1 as error syntax.
 * */
int netmask2int(const char *netmask)
{
	struct in_addr sin_addr;
	u_int32_t maskbit = 0;
	int i;

	inet_aton(netmask, &sin_addr);
	for (i = 31; (int)i >= 0; i--) {
		maskbit += (u_int32_t) sin_addr.s_addr & 1 << i ? 1 : 0;
	}
	
	return maskbit;
}

/* compare ip1 and ip2 are in the same subnet or not */
/* in same subnet(return 1) Not in same subnet (return 0) */
int same_subnet(const char *ip1, const char *ip2, const char *netmask)
{
	if(((inet_addr(ip1) & inet_addr(netmask)) ==
		       	(inet_addr(ip2) & inet_addr(netmask)))) {
		fprintf(stderr, "in same_subnet \n");
		return 1;
	}
	fprintf(stderr, "NOT in same_subnet \n");
	return 0;
}

/* replace string tok with dst from str, and str length is slen */
int str_replace(char *str, const char *dst, const char *tok, const size_t slen)
{
	char *pt, *buf;

	if(strlen(dst) > slen)
		return -1;

	if((pt = strstr(str, tok)) == NULL)
		return 0;
	pt[0] = '\0';
	pt += strlen(tok);
	buf = malloc(slen);
	sprintf(buf, "%s%s%s", str, dst, pt);
	strcpy(str, buf);
	free(buf);
	return 0;
}


void __args(int argc, char *argv[], char **arg[])
{
	int i;
	char ***p;
	
	for (i = 0,p = arg; *p != NULL && i < argc ;p++, i++)
		**p = i < argc ? argv[i] : NULL;
}

int strlist_del(char *s, const char *delim)
{
	char *p, *n;
	int rev = -1;
	
	if ((p = strstr(s, delim)) == NULL)
		goto out;
	
	n = p + strlen(delim);
	
	/* @s = "123 456 789", error on: @delim = "45"
	 * @n must be SPACE char or '\0'
	 * */
	if (*n != '\0' && isspace(*n) == 0) {
		goto out;
	}
	
	if (p == s) {
		SKIP_SPACE(n);
		strcpy(s, n);
	} else {
		/* @s = "123 456 789", error on: @delim = "56"
		 * @p must be SPACE char
		 * */
		if (isspace(*(p - 1)) == 0)
			goto out;
		strcpy(p - 1, n);
	}
	
	rev = 0;
out:
	return rev;
}

int _system(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[1024];

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	i = system(buf);
	return i;

}



char *stream_search(int fd, const char *value, char *buf, size_t len)
{
	FILE *stream;
	
	lseek(fd, 0, SEEK_SET);
	stream = fdopen(fd, "r");
	
	while (fgets(buf, len, stream) != NULL) {
		if (strncmp(buf, value, strlen(value)) == 0)
			return buf;
	}
	return NULL;
}
/*
 * stream_replace_line: replace string @from to @to
 * return 0 on success -1 as fail.
 * 
 * XXX: @fd _MUST_ open(2) with O_RDWR flags as read/write operation
 * the @to will auto append '\n' to as new line.
 * ex: @to "to_string" will be "to_string\n"
 * 
 * XXX: 1. if no @from has be found within @fd, the string @to will
 * 	   append to the end of file @fd. otherwise, the @to will replace
 * 	   in the line of @from.
 *	2. if multiple @from has be found, it will _ONLY_ replaced
 *	   in the first time.
 *
 *
 * TODO: 1. support stream_replace(fd, from, NULL) to delete line from @fd
 * 	 2. support multiple replace in the same string with @form
 * 	 3. support optional to no replace if @from is not found!
 * */
 
int stream_replace_line(int fd, const char *from, const char *to)
{
	char buf[BUFSIZ];
	FILE *tmpfd, *stream;
	int rev = -1, hit = 0; /* only replace once */
	
	lseek(fd, 0, SEEK_SET);
	stream = fdopen(fd, "r");
	
	if ((tmpfd = tmpfile()) == NULL)
		goto out;
	while (fgets(buf, sizeof(buf), stream) != NULL) {
		if (strncmp(buf, from, strlen(from)) == 0) {
			if (hit++ == 0)
				sprintf(buf, "%s\n", to);
		}
		fputs(buf, tmpfd);
	}
	if (hit == 0)
		fprintf(tmpfd, "%s\n", to);
		
	rewind(tmpfd);
	lseek(fd, 0, SEEK_SET);
	ftruncate(fd, 0);
	while (fgets(buf, sizeof(buf), tmpfd) != NULL) {
		if (write(fd, buf, strlen(buf)) == -1)
			perror("");
	}
	/* need not close stream where come from fdopen */
	//fclose(stream);
	
	fclose(tmpfd);
	rev = 0;
out:
	return rev;
}
#if 0 
/* Demo code with stream_replace_line() */
int main(int argc, char *argv[])
{
	int fd;
	char buf[BUFSIZ], *from, *to;
	
	if (argc < 4) {
		printf("usage: %s <replaced_file> <from_string> <to_string>\n", argv[0]);
		goto out;
	}
	from = argv[2];
	to = argv[3];
	
	if ((fd = open(argv[1], O_RDWR)) == -1)
		goto out;
	
	if (stream_search(fd, from,  buf, sizeof(buf)) == NULL) {
		printf("no such string line in file %s\n", argv[1]);
	}
	
	/* do some things with @buf ...  */
	
	printf("replace [%s] to [%s]\n", from, to);
	/* remember to add '\n' at end of string line */
	stream_replace_line(fd, from, to);
	printf("pos:%d\n", lseek(fd, 0, SEEK_CUR));
	close(fd);
out:
	
	return 0;
}
#endif


pid_t get_pid_from_pidfile(const char *fmt, ...)
{
	va_list args;
	FILE *fd;
	char buf[1024];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	
	if ((fd = fopen(buf, "r")) == NULL)
		goto sys_err;
	if (fgets(buf, sizeof(buf), fd) == NULL)
		goto sys_err;
	fclose(fd);
	chomp(buf);
	return atoi(buf);
sys_err:
	err_msg( "%s", strerror(errno));
	return -1;

}

int kill_from_pidfile(int sig, const char *fmt, ...)
{
	va_list args;
	FILE *fd;
	char buf[1024];
	pid_t pid;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	
	if ((pid = get_pid_from_pidfile(buf)) == -1)
		goto out;
	if (kill(pid, sig) == -1)
		goto sys_err;
	return 0;
sys_err:
	err_msg("can not kill pid(%d): %s", strerror(errno));
out:
	return -1;
}
