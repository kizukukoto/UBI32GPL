#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "helper.h"
#include "ssi.h"
#include "log.h"
#include "querys.h"
#include <sys/wait.h>
#include <ctype.h>

#ifdef BUF_SIZE
#undef BUF_SIZE
#endif

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#define BUF_SIZE 4096
static void html_tran(char *s)
{
	struct {
		char from;
		char *to;
	} TRAN[] = {
		{' ', "&#160;"},
		{'"', "&#34;"},
		{'&', "&#38;"},
		{'\'', "&#39;"},
		{'\\', "&#92;"},
	};
	int l = strlen(s);
	int i, j;


	for (i = 0; i < l; i++) {
		for (j = 0; j < sizeof(TRAN)/sizeof(TRAN[0]); j++) {
			if (TRAN[j].from == s[i]) {
				printf("%s", TRAN[j].to);
				break;
			}
		};
		if (j == sizeof(TRAN)/sizeof(TRAN[0]))
				putchar(s[i]);
	}
}
/*
 * Do wireless wep key chang form hex to ascii
 */
static void do_echok(char *filename, FILE * fp, int argc, char *argv[])
{
	char value1[BUF_SIZE], temp[3] = "xx";
       	unsigned long j;
	char *p;
	int i;

	DEBUG_MSG("do_echok:\n");
	if (query_vars(argv[1], value1, sizeof(value1)) < 0) {
		value1[0] = '\0';
		goto err_exit;
	}

	/* hex/ascii  */
	if (strcmp(argv[2], "ascii") == 0 ){
		p = value1;
		for (i = 0; i < (strlen(value1)/2); i++){
			strncpy(temp, p, 2);
			j = strtoul(temp, NULL, 16);
			fprintf(stdout, "%c", (unsigned char)j);
			p = p + 2;
		}
		return;
	}

err_exit:
	fputs(value1, stdout);
}

static void do_echot(char *filename, FILE * fp, int argc, char *argv[])
{
	char value[BUF_SIZE];
	DEBUG_MSG("do_echot:\n");
	if (query_vars(argv[1], value, sizeof(value)) >= 0) {
		DEBUG_MSG("do_echot:value=%s\n",value);
		html_tran(value);
	} else {
		/* XXX: argc <= 2 ? */
		if (argv[2] != NULL){
			DEBUG_MSG("do_echot:argv[2]=%s\n",argv[2]);
			fputs(argv[2], stdout);
		}	
	}
}

static void do_echo(char *filename, FILE * fp, int argc, char *argv[])
{
	char value[BUF_SIZE];

	DEBUG_MSG("do_echo:\n");
	if (query_vars(argv[1], value, sizeof(value)) >= 0) {
		DEBUG_MSG("do_echo:value=%s\n",value);
		fputs(value, stdout);
	} else {
		/* XXX: argc <= 2 ? */
		if (argv[2] != NULL){
			DEBUG_MSG("do_echo:argv[2]=%s\n",argv[2]);
			fputs(argv[2], stdout);
		}	
	}
}

static void do_match(char *filename, FILE * fp, int argc, char *argv[])
{
	char value[BUF_SIZE];

	DEBUG_MSG("do_match:\n");
	if (query_vars(argv[1], value, sizeof(value)) >= 0) {
		SYSLOG("DO_MATCH %s: [%s] == [%s]\n", argv[1], argv[2], value);
		if (strcmp(value, argv[2]) == 0){
			DEBUG_MSG("do_match:argv[3]=%s\n",argv[3]);
			(void) fputs(argv[3], stdout);
		}	
	} else {
		DEBUG_MSG("do_match:not found %s\n",argv[1]);
		SYSLOG("DO_MATCH NOT FOUND [%s]\n", argv[1]);
	}
}

static void do_choice(char *filename, FILE * fp, int argc, char *argv[])
{
	char value[BUF_SIZE];
	DEBUG_MSG("do_choice:\n");
	if (query_vars(argv[1], value, sizeof(value)) >= 0 &&
		       	argc >= (atoi(value) + 2)) {
		DEBUG_MSG("do_choice:arg=%s\n",argv[atoi(value) + 2]);       		
		fputs(argv[atoi(value) + 2], stdout);
	}
}

static void do_cgi(int argc, char *argv[]) 
{
	int i, j, k;
	int p[2];
	char buf[2][128];
	char *prev = buf[0], *curr = buf[1];

	argv[argc] = (char *)0;
	DEBUG_MSG("do_cgi:\n");
	if (pipe(p)) {
		perror("pipe");		
		return;
	}

	fflush(stdout);
#if NOMMU
	k = vfork();
#else
	k = fork();
#endif
	if (k < 0) {
		LOGERR("%s fork fail\n", __func__);
		DEBUG_MSG("do_cgi:fork fail!\n");
		return;
	}

	if (k == 0) {
		close(p[0]);		// close read pipe
		dup2(p[1], 1);		// replace pipe to STDIN

		execvp(argv[0], argv);
		LOGERR("execv fail");
		DEBUG_MSG("do_cgi:execv fail!\n");
		exit(-1); /* execv fail */
	}
	
	close(p[1]);			// close write pipe
	DEBUG_MSG("do_cgi:DO EXEC PIPE\n");
	j = 0;
	do {
		char *t;

		if ((i = read(p[0], curr, sizeof(buf[0]))) <= 0)
			break;

		// Write previous read data
		if (j)
			write(1, prev, j);
		//Swap buffer
		j = i;
		t = curr;
		curr = prev;
		prev = t;
	} while(1);
	DEBUG_MSG("do_cgi:DO EXEC PIPE EOF\n");
	// Remove "CR" or SPACE charactor for HTML happy
	while ((j && (prev[j-1] == '\n' || prev[j-1] == ' ')))
			j--;
	if (j)
		write(1, prev, j);

	waitpid(k, &i, 0);
	if (!WIFEXITED(i)){
		LOGERR("child process abnormal");
		DEBUG_MSG("do_cgi:child process abnormal\n");
	}	

	close(p[0]);
}

static void do_exec(char *filename, FILE * fp, int argc, char *argv[])
{
	int i;
	struct {
		char *name;
		void (*fn)(int argc, char *argv[]);
	} cmd[] = {
		{ "cgi", do_cgi},
		{NULL, NULL}
	};
	
	DEBUG_MSG("do_exec:\n");
	if (argc < 3){
		DEBUG_MSG("do_exec: argc fail!\n");
		return;
	}	

	
	for (i = 0; cmd[i].name != NULL && strcmp(argv[1], 
				cmd[i].name) != 0; i++);

	if (cmd[i].fn != NULL)
		cmd[i].fn(argc - 2, &argv[2]);

}
/*
 * Get a value from a offseted key base on argv
 * argv[1]: prefix of key name.
 * argv[2]: offset base from argv[3]
 * argv[3]: base
 * URL=GET http://192.168.0.1/fw.asp?fw_base=75
 * EX:
 * <!--# offset firewall fw_base 14 --!>
 * argv[1] = "firewall_rule"
 * argv[2] = "fw_base" key of nvram
 * argv[3] = "14"
 * 
 *  Where will return value of "firewall_rule39" if "fw_base" = 25 from nvram.
 *  Where will return value of "firewall_rule89" if "fw_base" = 75 from nvram.
 *  Where will return value of "firewall_rule14" if "fw_base" not found.
 * */
static void do_offset(char *filename, FILE * fp, int argc, char *argv[])
{
	char value[BUF_SIZE] = "";
	char key[256] = "";
	int i;
	if (argc < 4){
		DEBUG_MSG("do_offset:argc fail!\n");
		return;
	}	
	
	i = query_vars(argv[2], value, sizeof(value)) < 0 ? 0 : atoi(value);
	sprintf(key, "%s%d", argv[1], atoi(argv[3]) + i);
	
	if (query_vars(key, value, sizeof(value)) >= 0){
		DEBUG_MSG("do_offset:value=%s\n",value);
		fputs(value, stdout);
	}	
}

struct ssi_helper helpers[] = {
	{"echo", do_echo},
	{"echot", do_echot},
	{"echok", do_echok},
	{"match", do_match},
	{"choice", do_choice},
	{"exec", do_exec},
	{"offset", do_offset},
	{"",NULL}
};
#define HTML_RESPONSE_PAGE "html_response_page"
#define HTML_RESPONSE_MESSAGE "html_response_message"
#define HTML_RESPONSE_ERROR_MESSAGE "html_response_error_message"
#define HTML_RETURN_PAGE "html_response_return_page"
void make_back_msg(const char *fmt, ...)
{
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	setenv(HTML_RESPONSE_ERROR_MESSAGE, buf, 1);
}
