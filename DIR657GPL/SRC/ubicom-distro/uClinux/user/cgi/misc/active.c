#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define debug

#ifdef debug
#include "ssi.h"
#include "querys.h"
#include "log.h"
#endif

#define IP_CONNTRACK_FILE	"/proc/net/ip_conntrack"
#define LINE_PAGE	50

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
int _split(char *buf, const char *delim, char *tok[], int strip)
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

void parser(char *buf, char *string_buf)
{
	char *st[24], *src, *dst, *sport, *dport;
	int i;

	_split(buf, " \t", st, 1);

	switch (atoi(st[1])) {
		case 1:		/* ICMP ptotocol */
			for (i = 3; i < 8; i++) {
				if (strstr(st[i], "src") != 0){
					src = strchr(st[i], '=');
				}else if(strstr(st[i], "dst") != 0){
					dst = strchr(st[i], '=');
				}
			}
			sprintf(string_buf, "%s,%s,%s#", st[0], src + 1, dst + 1 );
			break;
		case 6:		/* TCP ptotocol */
			if (strstr(st[3], "ESTABLISHED") == 0){
				sprintf(string_buf, "");
				break;
			}
		case 17:	/* UDP ptotocol */
			for (i = 3; i < 8; i++) {
				if (strstr(st[i], "src") != 0){
					src = strchr(st[i], '=');
				}else if(strstr(st[i], "dst") != 0){
					dst = strchr(st[i], '=');
				}else if(strstr(st[i], "sport") != 0){
					sport = strchr(st[i], '=');
				}else if(strstr(st[i], "dport") != 0){
					dport = strchr(st[i], '=');
				}
			}
			sprintf(string_buf, "%s,%s:%s,%s:%s#", st[0], src + 1, sport + 1, dst + 1, dport + 1);
			break;
		default:
			//cprintf(":%d:%s:type[%s] No.[%s]\n", __LINE__, __func__, st[0], st[1]);
			sprintf(string_buf, "");
	}	
}

/*
 *	argv[1] = total --- count total page number
 *		  page  --- show page log infomation
 */

int active_main(int argc, char *argv[])
{
	FILE *fp;
	char buf[256], *s, *t;
        char *out_buf[LINE_PAGE], string_buf[1024];
	int cnt = 0, show_page = 0, i = LINE_PAGE, j = 0;
	div_t answer;
	
	if (argv[1] == NULL)
		return -1;

	s = getenv("QUERY_STRING");
	if (s != NULL)
		if ((s = strstr(s, "current_page"))) {
			t = strchr(s, '=');
			show_page = (atoi(++t) - 1);
		}
	
        fp = fopen(IP_CONNTRACK_FILE, "r");
        if (fp == (FILE *) 0) {
                fprintf(stderr, "Open %s record file FAIL\n", IP_CONNTRACK_FILE);
                return 0; 
        }

	cnt = 0;  
	while (fgets(buf, sizeof(buf),  fp) != NULL) {
		parser(buf, string_buf);
		if ( strcmp(string_buf, "") != 0) {
			cnt++;
			
			if ( strcmp(argv[1], "page") == 0) {
				if ( cnt > (show_page * LINE_PAGE)){
					printf("%s", string_buf);
				} else {
					if ( i == LINE_PAGE)
						i = 0;
					out_buf[i] = strdup(string_buf);
					i++;	
				}
				if ( cnt > ((show_page + 1) * LINE_PAGE) - 1)
					break;
			}
		}
		
	}
	fclose(fp);
	if ( cnt ==0 ) {
		cprintf("%d:No Data in File.\n", __LINE__);
		i = 0;
	}
	
	if ( (strcmp(argv[1], "page") == 0) && (i != LINE_PAGE)) {
		for (j = 0; j <= i; j++)
			printf("%s", out_buf[j]);
	}
	
	if ( strcmp(argv[1], "total") == 0) {
		answer = div(cnt, LINE_PAGE);
		if (answer.rem != 0 && answer.quot >= 0)
			answer.quot++;
		if (answer.quot == 0)
			answer.quot = 1;
		printf("%d", answer.quot);
	}

	return 1;
}
