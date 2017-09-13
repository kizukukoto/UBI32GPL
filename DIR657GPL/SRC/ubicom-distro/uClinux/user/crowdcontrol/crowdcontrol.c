/*
 * Crowd Control HTTP Proxy, version 0.4 beta
 *
 * Copyright (c) 2005 Tokachu. Portions copyright (c) 2004, 2005 Christopher
 * Devine.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 U.S.A.
 */
#include <assert.h> 
#ifndef WIN32

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#define recv(a,b,c,d) read(a,b,c)
#define send(a,b,c,d) write(a,b,c)

#else

#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <windows.h>

#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <regex.h>
#include <errno.h>
#include <string.h>
#include <sys/signal.h>

#ifndef uint32
#define uint32 unsigned long int
#endif

#define MAXIMUM_FILE_DESCRIPTORS 1024
#define URLBUF_SIZE 2049
#define URLBUF_STRL 2048
#define HOST_SIZE 128

#ifndef TCP_WINDOW_SIZE
#define TCP_WINDOW_SIZE 8192
#endif

#define child_exit(ret)     \
	shutdown(client_fd, 2); \
	shutdown(remote_fd, 2); \
	close(client_fd);       \
	close(remote_fd);       \
	return(ret);	

#ifdef __USE_DNS_RESOLVED__
#include "clist.h"
#else
#define add_domain2ip(str) do{}while(0)
#define search_addr(str) (0)
#endif //__USE_DNS_RESOLVED__

#define __USE_SYSLOG__
#ifdef __USE_SYSLOG__
#define SYSLOG(LOG_INFO, fmt, args...) syslog(LOG_INFO, fmt, ##args)
#else
#define SYSLOG(LOG_INFO, fmt, args...) do {} while(0)
#endif

//#define __USE_FPRINTF__
#ifdef __USE_FPRINTF__
#define FPRINTF(stderr, fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define FPRINTF(stderr, fmt, args...) do {} while(0)
#endif

#ifdef __DEBUG_USE_SYSLOG__
#define DEBUG(LOG_INFO, fmt, args...) DEBUG(LOG_INFO, fmt, ##args)
#else
#define DEBUG(LOG_INFO, fmt, args...) do {} while(0)
#endif

#define LAN_IFNAME	"br0"
#define DENY_REDIRECT_PAGE	"/reject.html"

/* Constants. */
const int REGFLAGS = REG_NOSUB | REG_EXTENDED | REG_ICASE;
const int PERMITTED = 0, BLOCKED = 1;

/* Structure to hold a linked list of regular expressions. */
struct regex_item
{
	//regex_t reg;
	int isdom;
	char *domain;
	struct regex_item *next;
};

/* Linked list of regular expressions. */
struct regex_item *permitted_urls = NULL, *blocked_urls = NULL;

/* Global identifiers. */
int _GLOBAL_MAX_CONNECTIONS = 16;
int _GLOBAL_IMPLICITACTION  = 0;
char *_GLOBAL_FILEMODE      = "rt";
char *_GLOBAL_ERRPARAM      = "%s: Parameter required for %s.\n";
char *_GLOBAL_ERRNOFILE     = "%s: Cannot open \"%s\".\n";
char *_GLOBAL_ERRNOMEM      = "%s: Out of memory.\n";
char *_GLOBAL_ERRREGEX      = "%s: Error converting \'%s\' to regex.\n";

/* Web-based error messages. */
int _GLOBAL_SIZEOF_BLOCKTEXT = 67 + 18;
char *_GLOBAL_BLOCKTEXT      =
"HTTP/1.0 200 OK\r\n"
"Content-type: text/plain\r\n"
"Content-length: 18\r\n"
"\r\n\r\n"
"<html>Block</html>";

int _GLOBAL_SIZEOF_BLOCKIMG  = 117;
char *_GLOBAL_BLOCKIMG       =
"HTTP/1.0 410 Gone\r\n"
"Content-type: image/gif\r\n"
"Content-length: 49\r\n\r\n"
"GIF89a\x01\x00\x01\x00\xa1\x01\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff"
"\xff\xff\x21\xf9\x04\x01\n\x00\x01\x00\x2c\x00\x00\x00\x00\x01\x00\x01\x00"
"\x00\x02\x02L\x01\x00;\r\n";

enum
{
	PARAM_PORT = 1,
	PARAM_BIND,
	PARAM_SUBNET,
	PARAM_MAXCONNECTIONS,
	PARAM_HTTPONLY,
	PARAM_HTTPWITHSSL,
	PARAM_HTTPTUNNEL,
	PARAM_IMPLICITLYDENY,
	PARAM_IMPLICITLYPERMIT,
	PARAM_PERMITDOMAINS,
	PARAM_PERMITURLS,
	PARAM_PERMITEXPRESSIONS,
	PARAM_DENYDOMAINS,
	PARAM_DENYURLS,
	PARAM_DENYEXPRESSIONS,
	PARAM_TESTURL,
/* 
 Date: 2009-1-05 
 Name: Ken_Chiang
 Reason: modify for the crowdcontrol can open and close used the log.
 Notice :
*/	
	PARAM_USEDLOG,

#ifdef MIII_BAR
	PARAM_MIII_BAR_ENABLE,
	PARAM_MIII_BAR_LAN_IP,
	PARAM_MIII_BAR_WAN_IP,
#endif

/* Date: 2009-01-08
 * Name: Fred Hung
 * Reason: let crowdcontrol identify schedule file
 */
	PARAM_SCHEDULE
};

struct thread_data
{
	int client_fd;
	int log_fd;
	uint32 server_ip;
	uint32 auth_ip;
	uint32 netmask;
	uint32 client_ip;
	int connect;
};

/* Date: 2009-01-08
 * Name: Fred Hung
 * Reason: This struct defined for reading schedule info from schedule config file
 */
static struct schedule_struct {
	char key[32];
	char content[64];
};

#define SYS_SCHEDULE	25	/* the number of system schedule rule */
#define URL_SCHEDULE	50	/* the number of urlfilter schedule rule */

static int do_schedule = 0;	/* 0: schedule checking unnecessary */
static struct schedule_struct sys_sch[SYS_SCHEDULE];
static struct schedule_struct url_sch[URL_SCHEDULE];
static int syssch_certain_exists = 0;
static int urlsch_certain_exists = 0;

/* 
 Date: 2009-1-05 
 Name: Ken_Chiang
 Reason: modify for the crowdcontrol can open and close used the log.
 Notice :
*/	
struct logip_item
{
	uint32 logip;
	uint32 isip;
	char mac[18];		
	struct logip_item *next;
};
struct logip_item *log_iplists = NULL;
struct logip_item *loadiplogfile(char *argv0, char *filename, struct logip_item *iplist);
static int mac_flag=0;
#define ARP_FILE "/proc/net/arp"

/* Function declarations. */
char *dgets(FILE *stream);
struct regex_item *loaddomainfile(char *argv0, char *filename,
                                  struct regex_item *urllist);
struct regex_item *loadurlfile(char *argv0, char *filename,
                                   struct regex_item *urllist);
struct regex_item *loadexpressionfile(char *argv0, char *filename,
                                      struct regex_item *urllist);
struct regex_item *add_url(char *url, int isdom, struct regex_item *list);
/* 
 Date: 2009-1-05 
 Name: Ken_Chiang
 Reason: modify for the crowdcontrol can open and close used the log.
 Notice :
*/	
struct logip_item *add_ip(char *ip, struct logip_item *list);
int domainmatches(char *domain, int domain_s, char *filter, int filter_s);
int check_url(char *url, int isdom, struct regex_item *item);
int check_urls(char *url, int isdom);
/* 
 Date: 2009-1-05 
 Name: Ken_Chiang
 Reason: modify for the crowdcontrol can open and close used the log.
 Notice :
*/	
int chk_logip(uint32 currentip, struct logip_item *ip_list, int allow_url, char *current_url);
int isregchar(char c);
int isurlchar(char c);
char x2c(char *what);
char *unescape_url(char *url, int len);
char *url2regex(char *domain);
void deny_access(int client_fd, int c_flag, char *url);
void log_request(uint32 client_ip, char *headers, int headers_len,
				 uint32 auth_ip, uint32 netmask);
int client_thread(struct thread_data *td);

#ifdef MIII_BAR
char miiicasa_enabled[4],miiicasa_server[50],miiicasa_server_js[80],miiicasa_UST_server[80],miiicasa_did[80],miiicasa_pre_key[30],miiicasa_prefix[30],miiicasa_forward_server[80];
void mac_encrypt ( char *mac );
char *md5_encrypt ( char *plain_text, char *password, int iv_len );
char *md5_decrypt ( char *enc_text, char *password, int iv_len );
int updateStatusTo = 0;
static int strnicmp(const char *s1, const char *s2, size_t n)
{
        if (n == 0)
                return (0);
        do {
                if ( toupper(*s1) != toupper(*s2++))
                        return (*(const unsigned char *)s1 -
                                *(const unsigned char *)(s2 - 1));
                if (*s1++ == 0)
                        break;
        } while (--n != 0);
        return (0);
}

static char *stristr(const char *in, const char *str)
{
    char c;
    size_t len;

    c = *str++;
    if (!c)
        return (char *) in;	// Trivial empty string case

    len = strlen(str);
    do {
        char sc;

        do {
            sc = *in++;
            if (!sc)
                return (char *) 0;
        } while (sc != c);
    } while (strnicmp(in, str, len) != 0);

    return (char *) (in - 1);
}
int get_ip_addr(char *if_name, char *ip_addr)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr;

	struct ifreq *p_ifr = &ifr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		return -1;
	}

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		return -1;
	}

	close(sockfd);

	sprintf(ip_addr, "%s", inet_ntoa( ( (struct sockaddr_in *)  &p_ifr->ifr_addr)->sin_addr) ); 

	return 0;
}

static char *str_replace (char *source, char *find,  char *rep){  
     int find_L=strlen(find);  
    int rep_L=strlen(rep);  
    int length=strlen(source)+1;  
    int gap=0;  
     
    char *result = (char*)malloc(sizeof(char) * length);  
    strcpy(result, source);      
     
    char *former=source;  
    char *location= strstr(former, find);  
     
    while(location!=NULL){  
       gap+=(location - former);  
       result[gap]='\0';  
         
       length+=(rep_L-find_L);  
       result = (char*)realloc(result, length * sizeof(char));  
       strcat(result, rep);  
       gap+=rep_L;  
         
       former=location+find_L;  
       strcat(result, former);  
         
       location= strstr(former, find);  
    }      
  
    return result;  
  
} 
// transfer / or = or + or : to %2F or %3D or %2B or %3A
static char *special_str_replace (char *source){  
    int length = 3 * strlen(source);  
    int i,j=0;  
     
    char *result = (char*)malloc(sizeof(char) * length);
    memset(result, 0, length);
    for(i=0;i<strlen(source);i++)
    {
    	if(*(source+i) == 0x2f)
    	{
    		*(result+j) = 0x25;
    		j++;
    		*(result+j) = 0x32;
    		j++;
    		*(result+j) = 0x46;
    		j++;
    	}
    	else if(*(source+i) == 0x3d)
    	{
    		*(result+j) = 0x25;
    		j++;
    		*(result+j) = 0x33;
    		j++;
    		*(result+j) = 0x44;
    		j++;
    	}
    	else if(*(source+i) == 0x2b)
    	{
    		*(result+j) = 0x25;
    		j++;
    		*(result+j) = 0x32;
    		j++;
    		*(result+j) = 0x42;
    		j++;
    	}
    	else if(*(source+i) == 0x3a)
    	{
    		*(result+j) = 0x25;
    		j++;
    		*(result+j) = 0x33;
    		j++;
    		*(result+j) = 0x41;
    		j++;
    	}
    	else
    	{
    		*(result+j) = *(source+i);
    		j++;
    	}
    }
    return result;    
} 
// remove
static char *special_str_remove(char *source){  
    int length = strlen(source);  
    int i,j=0;  
     
    char *result = (char*)malloc(sizeof(char) * length);
    memset(result, 0, length);
    for(i=0;i<strlen(source);i++)
    {
    	if(*(source+i) != 0x5c)
    	{
    		*(result+j) = *(source+i);
    		j++;
    	}
    }
    return result;    
} 
#endif



static char *chop(char *io)
{
	char *p;

	p = io + strlen(io) -1;
	while ( *p == '\n' || *p == '\r') {
		*p = '\0';
		p --;
	}
	return io;
}

/* This function used in check_url function for get schedule key.
 * Either url_domain_filter_schedule_X or schedule_ruleX.
 * The function name never changed because we don't change the
 * code behaviors. However, the schedule key contents obtain from
 * schedule config file, not from nvram. The schedule config file
 * used depend on '-S' parameters.
 *
 * Parameters:
 * 	@key := nvram key
 * 	@buf := content, send from caller
 * 	@len := length of @buf
 *
 */
static void nvram_safe_get(const char *key, char *buf, int len)
{
	int i = 0;
	
	bzero(buf, len);

	if (strncmp(key, "schedule_rule", 13) == 0) {
		for (; i < syssch_certain_exists; i++)
			if (strcmp(key, sys_sch[i].key) == 0) {
				strcpy(buf, sys_sch[i].content);
				goto normal;
			}
	} else {
		for (; i < urlsch_certain_exists; i++)
			if (strcmp(key, url_sch[i].key) == 0) {
				strcpy(buf, url_sch[i].content);
				goto normal;
			}
	}

normal:		
	return;
}

/*
 * char *dgets(FILE *stream, int &n)
 *
 * Reads a line from a file, allocates memory to hold the line, and returns a
 * pointer to the string.
 */
char *dgets(FILE *stream)
{
	register char c;
	int size = 1;
	char *mystr = malloc(size);

	char *tmp;
	tmp = malloc(100);
	bzero(tmp, 100);
	
	if(!fgets(tmp, 99, stream))
		return NULL;
	chop(tmp);	
	return tmp;
	
	/* Go through the stream until we hit the end of the file or line. */
	while (!feof(stream) && (c = fgetc(stream)) & 0xE0)
	{
		if (c == '#' || c == ' ')
		{
			while (!feof(stream) && (fgetc(stream) != '\n'));
			return mystr;
		}
		
		/* Skip funny characters. */
		if (feof(stream) || !(c & 0xE0))
			continue;
		
		if ((mystr = realloc(mystr, ++size)))
		{
			mystr[size - 2] = c;
		}
		else
		{
			free(mystr);
			return NULL;
		}
	}
	
	if (size > 1)
	{
		mystr[size - 1] = '\0';
		return mystr;
	}
	else
	{
		free(mystr);
		return NULL;
	}
	
	free(mystr);
	return NULL;
}

char *cmd_nvram_get(const char *name, char *value){
        char buffer[192] = {};
        char *nvram_template = "nvram get %s > %s";
        FILE *fp;
        char *nvram_value=NULL;
        char nvram[100];
        char rm[10];
        char *rm_template = "rm -rf %s ";
        char parse_file[64] = {"/var/etc/test"};
        int pid;
        
        pid=getpid();
        if(pid<0)
        	return NULL;
        	
        sprintf(parse_file,"/var/etc/%d",pid);
        sprintf(nvram,nvram_template,name,parse_file );
        system(nvram);
        
        fp = fopen(parse_file,"r");
        if(!fp) {
                printf("cmd_nvram_get: %s open fail\n",parse_file);
                return NULL;
        }
        fgets(buffer,sizeof(buffer),fp);
        fclose(fp);
        sprintf(rm,"rm -f %s",parse_file );
        system(rm);
        
        if(strlen(buffer)<1){
                printf("cmd_nvram_get:%s not exsit.\n", name);
                return NULL;
        }
        
        nvram_value = strstr(buffer,"=");
        if(nvram_value==NULL)
        	return NULL;
        
        nvram_value = nvram_value + 2;
        
        nvram_value[strlen(nvram_value)-1] = '\0';
        sprintf(value,"%s",nvram_value);
        return value;
}

static void
list_item(struct regex_item *p)
{
	while(p){
		DEBUG(LOG_INFO, "LIST:[%s][%d]", p->domain, p->isdom);
		FPRINTF(stderr, "LIST:[%s][%d]\n", p->domain, p->isdom);
		p = p->next;
	}
	FPRINTF(stderr, "----------------------------------------\n");
}

/* 
 	Date: 2009-1-05 
 	Name: Ken_Chiang
 	Reason: modify for the crowdcontrol can open and close used the log.
 	Notice : for list the all log ip
*/ 	
static void
list_ipitem(struct logip_item *p)
{
	while(p){
		if(p->isip){
			DEBUG(LOG_INFO, "LIST:logip=%x",p->logip);
			FPRINTF(stderr, "LIST:logip=%x\n", p->logip);
		}	
		else{
			DEBUG(LOG_INFO, "LIST:logmac=%s",p->mac);
			FPRINTF(stderr, "LIST:logmac=%s\n", p->mac);
		}		
		p = p->next;
	}	
	FPRINTF(stderr, "----------------------------------------\n");
	return;
}

/*
 * struct regex_item *loaddomainfile(char *argv0, char *filename,
 *                                   struct regex_item *urllist)
 *
 * Loads a list of domains from file "filename" into list "urllist". Returns a
 * pointer to the list upon success, NULL if it failed.
 */
struct regex_item *loaddomainfile(char *argv0, char *filename,
                                  struct regex_item *urllist)
{
	FILE *in;
	char *fileline = NULL;
	
	/* Process a list of domains. */
	if (filename)
	{
		DEBUG(LOG_INFO,"filename=%s",filename);
		if (!(in = fopen(filename, _GLOBAL_FILEMODE)))
		{
			DEBUG(LOG_INFO, _GLOBAL_ERRNOFILE, argv0, urllist);
			exit(1);
		}
		
		while (!feof(in))
		{
			DEBUG(LOG_INFO, "Loop:\n");
			list_item(urllist);
			if (!(fileline = dgets(in))) {
				DEBUG(LOG_INFO, "break\n");
				continue;
			}
			list_item(urllist);
			DEBUG(LOG_INFO, "load:[%s]\n", fileline);
			if (!(urllist = add_url(fileline, strlen(fileline), urllist)))
			{
				DEBUG(LOG_INFO, _GLOBAL_ERRNOMEM, argv0);
				free(fileline);
				exit(1);
			}
		}
		
		free(fileline);
		fclose(in);
	}
	
	return urllist;
}

/*
 * struct regex_item *loadurlfile(char *argv0, char *filename,
 *                                struct regex_item *urllist)
 *
 * Loads a list of URLs from file "filename" into list "urllist". Returns a
 * pointer to the list upon success, NULL if it failed.
 */
struct regex_item *loadurlfile(char *argv0, char *filename,
                               struct regex_item *urllist)
{
	FILE *in;
	char *fileline = NULL;
	
	if (filename)
	{
		DEBUG(LOG_INFO, "filename=%s",filename);
		if (!(in = fopen(filename, _GLOBAL_FILEMODE)))
		{
			DEBUG(LOG_INFO, _GLOBAL_ERRNOFILE, argv0, urllist);
			exit(1);
		}
		
		while (!feof(in))
		{
			if (!(fileline = dgets(in)))
				continue;
			/*
			if (!(fileline = url2regex(fileline)))
			{
				DEBUG(LOG_INFO, _GLOBAL_ERRNOMEM, argv0);
				free(fileline);
				return NULL;
			}
			*/
			if (!(urllist = add_url(fileline, 0, urllist)))
			{
				DEBUG(LOG_INFO, _GLOBAL_ERRNOMEM, argv0, fileline);
				free(fileline);
				exit(1);
			}
		}
		
		free(fileline);
		fclose(in);
	}
	
	return urllist;
}

/*
 * struct regex_item *loadexpressionfile(char *argv0, char *filename,
 *                                       struct regex_item *urllist)
 *
 * Loads a list of regular expressions from file "filename" into list
 * "urllist". Returns a pointer to the list upon success, NULL if it failed.
 */
struct regex_item *loadexpressionfile(char *argv0, char *filename,
                                      struct regex_item *urllist)
{
	FILE *in;
	char *fileline = NULL;
	
	if (filename)
	{
		DEBUG(LOG_INFO, "filename=%s",filename);
		if (!(in = fopen(filename, _GLOBAL_FILEMODE)))
		{
			DEBUG(LOG_INFO, _GLOBAL_ERRNOFILE, argv0, filename);
			exit(1);
		}
		
		while (!feof(in))
		{
			if (!(fileline = dgets(in)))
				continue;
			
			if (!(urllist = add_url(fileline, 0, urllist)))
			{
				DEBUG(LOG_INFO, _GLOBAL_ERRREGEX, argv0, fileline);
				free(fileline);
				exit(1);
			}
		}
		
		free(fileline);
		fclose(in);
	}
	
	return urllist;
}

/* 
 	Date: 2009-1-05 
 	Name: Ken_Chiang
 	Reason: modify for the crowdcontrol can open and close used the log.
 	Notice :
 	
 * struct logip_item *loadiplogfile(char *argv0, char *filename, *struct logip_item *iplist)
 *
 * Loads a list of log ip from file "filename" into list "iplist". Returns a
 * pointer to the list upon success, NULL if it failed.
 */
struct logip_item *loadiplogfile(char *argv0, char *filename,
                               struct logip_item *iplist)
{
	FILE *in;
	char *fileline = NULL;
	
	if (filename)
	{
		DEBUG(LOG_INFO, "filename=%s",filename);
		if (!(in = fopen(filename, _GLOBAL_FILEMODE)))
		{
			DEBUG(LOG_INFO, _GLOBAL_ERRNOFILE, argv0, iplist);
			exit(1);
		}
		list_ipitem(iplist);
		while (!feof(in))
		{			
			if (!(fileline = dgets(in)))
				continue;
			
			if (!(iplist = add_ip(fileline, iplist)))
			{
				DEBUG(LOG_INFO, _GLOBAL_ERRNOMEM, argv0, fileline);
				free(fileline);
				exit(1);
			}
		}
		
		free(fileline);
		fclose(in);
	}	
	list_ipitem(iplist);
	return iplist;
}

/*
 * struct regex_item *add_url(char *url, int isdom, struct regex_item *list)
 *
 * Add a regular expression to the linked list. It returns a pointer to the
 * beginning of the linked list if the addition succeeds, or NULL otherwise.
 */
struct regex_item *add_url(char *url, int isdom, struct regex_item *list)
{	
	if (!list)
	{
		DEBUG(LOG_INFO, "%s: If the list is empty",__func__);
		/* If the list is empty, create the first item in the linked list. */
		if ((list = malloc(sizeof(*list))))
		{
			if (isdom)
			{
				DEBUG(LOG_INFO, "isdome\n");
				list->isdom = strlen(url);
				
				/* If the memory cannot be allocated, bail out. */
				if (!(list->domain = malloc(list->isdom + 1))){
					DEBUG(LOG_INFO, "list->domain malloc fail\n");
					return NULL;
				}	
				bzero(list->domain, list->isdom + 1);
				strncpy(list->domain, url, list->isdom + 1);
			}
			else
			{
				DEBUG(LOG_INFO, "not isdome\n");
				/* If the regex can't be compiled, bail out. */
				//if (regcomp(&list->reg, url, REGFLAGS))
				//	return NULL;
				if (!(list->domain = strdup(url))){
					DEBUG(LOG_INFO, "list->domain strdup fail");
					return NULL;
				}	
				list->isdom = 0;
				//list->domain = NULL;
			}
			
			list->next = NULL;
			DEBUG(LOG_INFO, "New domain:[%s]\n", list->domain);
			return list;
		}
		else{
			DEBUG(LOG_INFO, "list malloc fail\n");
			return NULL;
		}	
	}
	else
	{
		/* If the list has items, add another item to the linked list. */
		struct regex_item *current;
		current = list;
		DEBUG(LOG_INFO, "%s: If the list isn't empty",__func__);
		/* Seek to the end of the list. */
		while (current->next) {
			current = current->next;
		}
		/* Create a new item. */
		if ((current->next = malloc(sizeof(*current))))
		{
			DEBUG(LOG_INFO, "Create a new item");
			current = current->next;
			if (isdom)
			{
				DEBUG(LOG_INFO, "isdom");
				current->isdom = strlen(url);
				/* If the memory cannot be allocated, bail out. */
				if (!(current->domain = malloc(list->isdom + 10))){
					DEBUG(LOG_INFO, "list->domain malloc fail");
					return NULL;
				}	
				bzero(current->domain, list->isdom + 10);
				DEBUG(LOG_INFO, "added url:[%s]", url);				
				strncpy(current->domain, url, current->isdom + 1);
			}
			else
			{
				DEBUG(LOG_INFO, "not isdome\n");
				/* If the regex can't be compiled, bail out. */
				//if (regcomp(&current->reg, url, REGFLAGS))
				//	return NULL;
				if (!(current->domain = strdup(url))){
					DEBUG(LOG_INFO, "current->domain strdup fail");
					return NULL;
				}	
				current->isdom = 0;
			}
			current->next = NULL;
			DEBUG(LOG_INFO, "current domain:[%s]\n", current->domain);
			return list;
		}
		else{
			DEBUG(LOG_INFO, "Create a new item fail");
			return NULL;
		}	
	}
	
	/* As a default measure, return NULL. */
	return NULL;
}

/* 
 	Date: 2009-1-05 
 	Name: Ken_Chiang
 	Reason: modify for the crowdcontrol can open and close used the log.
 	Notice :
 	
 * struct logip_item *add_ip(char *ip, struct logip_item *list)
 *
 * Add a log ip to the linked list. It returns a pointer to the
 * beginning of the linked list if the addition succeeds, or NULL otherwise.
 */
struct logip_item *add_ip(char *ip, struct logip_item *list)
{	
	struct in_addr myip;
	if (!list)
	{
		DEBUG(LOG_INFO, "%s: If the list is empty",__func__);
		/* If the list is empty, create the first item in the linked list. */
		if ((list = malloc(sizeof(*list))))
		{
			if(strchr(ip,':')){
				strcpy(list->mac,ip);
				list->isip = 0;
				mac_flag++;
				DEBUG(LOG_INFO, "first log_mac:%s[%s]",ip , list->mac);
			}	
			else{
				inet_aton(ip,&myip);	
				list->logip = myip.s_addr;
				list->isip = 1;				
				DEBUG(LOG_INFO, "first log_ip:%s[%x]",ip , list->logip);	
			}	
			list->next = NULL;	
			return list;
		}
		else{
			DEBUG(LOG_INFO, "%s: list malloc fail",__func__);
			return NULL;
		}	
	}
	else
	{
		/* If the list has items, add another item to the linked list. */
		struct logip_item *current;
		current = list;
		DEBUG(LOG_INFO, "%s: If the list isn't empty",__func__);
		/* Seek to the end of the list. */
		while (current->next) {
			current = current->next;
		}
		/* Create a new item. */
		if ((current->next = malloc(sizeof(*current))))
		{
			DEBUG(LOG_INFO, "Create a new item");
			current = current->next;
			if(strchr(ip,':')){
				strcpy(current->mac,ip);
				current->isip = 0;
				mac_flag++;
				DEBUG(LOG_INFO, "new log_mac:%s[%s]",ip , current->mac);
			}	
			else{
				inet_aton(ip,&myip);	
				current->logip = myip.s_addr;
				current->isip = 1;				
				DEBUG(LOG_INFO, "new log_ip:%s[%x]",ip , current->logip);	
			}						
			current->next = NULL;							
			return list;
		}
		else{
			DEBUG(LOG_INFO, "%s: Create a new item fail",__func__);
			return NULL;
		}	
	}
	
	/* As a default measure, return NULL. */
	return NULL;
}

#define ADD_DOMAIN2IP(args)  add_domain2ip(args)
#define search_addr_r(str) search_addr(str)
/*
 * check URL match
 * Checks to see if the string "url" is finded in "filter".
 * return 1 is finded
 */
int chk_special(char *url,int url_s, char *filter, int filter_s){
	DEBUG(LOG_INFO, "s: d[%s][%d]f[%s][%d]",__func__, url, url_s, filter, filter_s);
	/*
	char *p;
	if (!(p = strchr(url, '/')))	//check "www.foo.com/index.html"
		return 0;
	if (strlen(p) <= 1)
		return 0;	//return if format "www.foo.com/" ...
	*/
	if (url_s < filter_s){
		DEBUG(LOG_INFO, "url_s < filter_s");
		return 0;
	}	

/* 
 Date: 2009-10-26 
 Name: Ken_Chiang
 Reason: modify for the crowdcontrol maybe will trap because strstr func the argument used the int type.
 Notice :
*/	
/*
	if (strstr(url_s, filter))
*/

	if (strstr(url, filter))
		return 1;
	
	return 0;
}

/* 
 	Date: 2009-1-05 
 	Name: Ken_Chiang
 	Input : char mac_addr 
 	Output: char ip_addr, return 1 is find ip addr and return 0 no find ip addr
 	Description: input mac addr to find ip addr in arp file.
 	Sample :
*/ 	
int macaddrs_to_ipaddrs(char *macaddr, char *ipaddr)
{
	FILE *fp1;
	int i;
	char tmp[100],ip[16];	
	/* open arp file */
	fp1 = fopen(ARP_FILE, "rt");
	if (fp1 == NULL) 
	{
		DEBUG(LOG_INFO, "open %s fail",ARP_FILE);
		return 0;					
	}
			
	for(i=0; i<strlen(macaddr); i++){
		*(macaddr+i)= toupper(*(macaddr+i));
	}	
	DEBUG(LOG_INFO, "macaddr up=%s",macaddr);
		
	while (!feof(fp1)){
		bzero(tmp, 100);	
		if(!fgets(tmp, 99, fp1)){
			DEBUG(LOG_INFO,"fgets fail");
			continue;
		}	
		DEBUG(LOG_INFO,"tmp %s", tmp);
		if(strcasestr(tmp,macaddr)){			
			strncpy(ip,tmp,15);
			ip[15]='\0';
			strcpy(ipaddr,ip);
			DEBUG(LOG_INFO,"ipaddr %s", ipaddr);
			return 1;			
		}	
	}					
	
	return 0;
}	

/* 
 	Date: 2009-1-05 
 	Name: Ken_Chiang
 	Reason: modify for the crowdcontrol can open and close used the log.
 	Notice :
 * check ip addreses match
 * Checks to see if the ip is finded in "ip_list".
 * return 1 is finded
 */ 
int chk_logip(uint32 currentip, struct logip_item* ip_list, int allow_url, char *current_url)
{		
	char ipaddr[16];
	struct in_addr myip;
	DEBUG(LOG_INFO, "mac_flag=%d", mac_flag);
	while(ip_list){		
		if(mac_flag)//had mac addr
		{			
			if(ip_list->isip){//ip addr
				DEBUG(LOG_INFO, "ip addr=%x",ip_list->logip);
				if( currentip == ip_list->logip){
					DEBUG(LOG_INFO, "find logip=%x",currentip);			
					myip.s_addr = currentip;
					if(allow_url)
						SYSLOG(LOG_INFO, "[Website Filter]: %s accessed from %s (%s)\n", current_url, inet_ntoa(myip), ip_list->mac);
					else
						SYSLOG(LOG_INFO, "[Website Filter]: %s blocked for %s (%s)", current_url, inet_ntoa(myip), ip_list->mac);
								
					return 1;
				}
			}
			else{//mac addr
				DEBUG(LOG_INFO, "mac addr=%s",ip_list->mac);
				if(macaddrs_to_ipaddrs(ip_list->mac,ipaddr)){
					inet_aton(ipaddr,&myip);
					DEBUG(LOG_INFO, "find logmacip=%s[%x]",ipaddr,myip);	
					if( currentip == myip.s_addr){
						DEBUG(LOG_INFO, "find logip=%x",currentip);			
						if(allow_url)
							SYSLOG(LOG_INFO, "[Website Filter]: %s accessed from %s (%s)\n", current_url, ipaddr, ip_list->mac);
						else
							SYSLOG(LOG_INFO, "[Website Filter]: %s blocked for %s (%s)", current_url, ipaddr, ip_list->mac);
									
						return 1;
					}
				}	
			}			
		}
		else{//just had ip addr
			if( currentip == ip_list->logip){
				DEBUG(LOG_INFO, "find logip=%x",currentip);			
				myip.s_addr = currentip;
				if(allow_url)
					SYSLOG(LOG_INFO, "[Website Filter]: %s accessed from %s\n", current_url, inet_ntoa(myip));	
				else
					SYSLOG(LOG_INFO, "[Website Filter]: %s blocked for %s", current_url, inet_ntoa(myip));
							
				return 1;
			}	
		}		
		ip_list = ip_list->next;
	}	
	DEBUG(LOG_INFO, "no find logip=%x",currentip);
	return 0;
}

/*
 * int domainmatches(char *domain, int domain_s, char *filter, int filter_s)
 *
 * Checks to see if the string "domain" is the same as "filter".
 * return 1 is matched
 */
int domainmatches(char *domain, int domain_s, char *filter, int filter_s)
{
	int rev = 0;
	
	DEBUG(LOG_INFO, "%s: d[%s][%d]f[%s][%d]",__func__, domain, domain_s, filter, filter_s);	
	if (domain_s < filter_s){
		DEBUG(LOG_INFO, "domain_s < filter_s");
		return 0;
	}	

	//Follow cameo's SPEC
	if (strstr(domain, filter)) {
		ADD_DOMAIN2IP(domain);
		DEBUG(LOG_INFO, "domain[%s] is match filter[%s]\n",domain, filter);
		return 1;
	}
	/*
	 * Matched if format of @domain as "www.foo.com" and @filter as "www.foo." 
	 * or the sam length of both
	 */
	if (domain_s == filter_s || filter[filter_s - 1] == '.'){
		rev = strncmp(domain, filter, domain_s);
		
		if (rev == 0)
			ADD_DOMAIN2IP(domain);
		DEBUG(LOG_INFO, "domain[%s] is match filter[%s]\n",domain, filter);	
		return !rev;
	}

	/*
	 * Matched  if format of @domain as "www.foo.com" and @filter as "foo.com"
	 */
	domain_s -= filter_s;
	
	if (domain[domain_s - 1] == '.')
	{
		if (!(rev = strncmp((char *)(domain + domain_s), filter, filter_s))) {
			ADD_DOMAIN2IP(domain);
			DEBUG(LOG_INFO, "domain[%s] is match filter[%s]\n",domain, filter);
			return 1;
		}
	}
	if (search_addr_r(domain) ==1)
		return 1;
	return 0;
}

/*
 * int check_url(char *url, int isdom, struct regex_item *item)
 *
 * Go through a single list of regular expressions to find a match for "url".
 * Returns 1 (true) if a match was found, 0 (false) otherwise.
 */
int check_url(char *url, int isdom, struct regex_item *item)
{
	int i=0,ttime;
	time_t t;
	struct tm *tm;
	char key[32], value[32], *name, *week, *s_min, *e_min;
	struct regex_item *current = item;
	/* If the list is empty, just return 0. */
	DEBUG(LOG_INFO, "%s:url=%s,isdom=%d",__func__,url,isdom);
	if (!item){
		DEBUG(LOG_INFO, "item null");
		return 0;
	}	
	while (current) {

		/* Date: 2009-01-08
		 * Name: Fred Hung
		 * Reason: if crowdcontrol has '-S' schedule request,
		 *         do_schedule be 1, otherwise be 0.
		 */
		if (do_schedule) {
			DEBUG(LOG_INFO, "%s: do_schedule",__func__);
			sprintf(key, "url_domain_filter_schedule_%d", i);
			i++;
			nvram_safe_get(key, value, sizeof(value));
			if (strlen(value) == 0) {
				continue;
			} else if (strcmp(value, "Always") != 0) {
				sprintf(key, "schedule_rule%d", atoi(value));
				value[0] = '\0';
				nvram_safe_get(key, value, sizeof(value));
				if (strlen(value) == 0) {
					perror("schedule_rule hvae no value");
					exit(1);
				}
				name = strtok(value, "/");
				week = strtok(NULL, "/");
				s_min = strtok(NULL, "/");
				e_min = strtok(NULL, "/");
				time(&t);
				tm = localtime(&t);
				ttime = (tm->tm_hour * 60) + tm->tm_min;
				if (week[tm->tm_wday] == '1') {
					DEBUG(LOG_INFO, "today is in shedule\n"); 
					if (ttime < atoi(s_min) || ttime > (atoi(e_min) - 1)) {
						DEBUG(LOG_INFO, "Not shedule\n"); 
						current = current->next;
						continue;
					}
					DEBUG(LOG_INFO, "IN shedule\n"); 
				} else {
					DEBUG(LOG_INFO, "today is NOT in shedule\n"); 
					current = current->next;
					continue;
				}
			}
		}

		/* If we reach the end of the domain list with matches, return 1. */
		if (isdom)
		{
			DEBUG(LOG_INFO, "isdom\n");
			if (!(current->isdom)){
				current = current->next;
				continue;
			}
			DEBUG(LOG_INFO, "check current domain[%s][%d]\n",current->domain, current->isdom);
			if (domainmatches(url, strlen(url), current->domain, current->isdom)){
				DEBUG(LOG_INFO, "match current domain[%s][%d]\n", current->domain, current->isdom);	
				return 1;
			}	
		} else {			
			DEBUG(LOG_INFO, "not isdom");			       	
			if (current->isdom) {
				current = current->next;
				continue;
			}
			DEBUG(LOG_INFO, "chk current url[%s][%d]\n", current->domain, current->isdom);	
			if (chk_special(url, strlen(url), current->domain,
					       	strlen(current->domain))){
				DEBUG(LOG_INFO, "match current url[%s][%d]\n", current->domain, current->isdom);	
				return 1;
			}	
			/* If we find a match, return 1. */
		//	if (!regexec(&current->reg, url, 0, NULL, 0))
		//		return 1;
		}
		/* Seek to the next element. */
		current = current->next;
	}
	
	/* If no match is found, return 2. */
	return 2;
}

/*
 * int check_urls(char *url, int isdom)
 *
 * Checks to see if a URL should be permitted or blocked. Returns 1 (true) if
 * the URL should be permitted, or 0 (false) if it should be blocked.
 */
int allow_action;

int check_urls(char *url, int isdom)
{
	int match;

	DEBUG(LOG_INFO, "%s:url=%s,isdom=%d",__func__,url,isdom);

	if ((permitted_urls == NULL) && (allow_action == 1)) {
		/* deny all urls */
		DEBUG(LOG_INFO, "permitted no find match\n");
		return 0;		
	}

	if ((match = check_url(url, isdom, permitted_urls)))
	{
		DEBUG(LOG_INFO, "permitted match = %d", match);		
		if(match == 2){
			DEBUG(LOG_INFO, "permitted no find match\n");
			return 0;
		}
		DEBUG(LOG_INFO, "permitted");		
		return 1;
	}
	else
	{
		DEBUG(LOG_INFO, "No match permitted_urls\n");
		if ((_GLOBAL_IMPLICITACTION == BLOCKED) && isdom) {
			DEBUG(LOG_INFO, "Implicitation domain block[%s]", url);			
			return 0;
		}
		if ((match = check_url(url, isdom, blocked_urls))) {
			DEBUG(LOG_INFO, "blocked match = %d", match);			
			if(match == 2){
				DEBUG(LOG_INFO, "blocked no find match\n");
				return 1;
			}				
			DEBUG(LOG_INFO, "blocked");			
			return 0;
		}
		DEBUG(LOG_INFO, "others");		
	}	
	return 1;
}

/*
 * int isregchar(char c)
 *
 * Checks to see if a character should be preceeded with a blackslash for a
 * regular expression. Returns 1 (true) if it should, 0 (false) otherwise.
 */
int isregchar(char c)
{
	return (c == '.' || c == '?' || c == '+' || c == '[' || c == ']' ||
			c == '\\'|| c == '{' || c == '}' || c == '*' || c == '(' ||
			c == ')' || c == '$' || c == '^' || c == '|');
}

/*
 * int isurlchar(char c)
 *
 * Checks to see if a character (in a URL) should NOT be percent-encoded.
 * Returns 0 (false) if the character should be percent-encoded, 1 (true)
 * otherwise.
 */
int isurlchar(char c)
{
	return (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~');
}

/*
 * char x2c(char *what)
 *
 * Decode a percent-encoded character (routine from the original NCSA server).
 */
char x2c(char *what)
{
	register char c;
	
	c = (what[0] >= 'A' ? ((what[0] & 0xDF) - 'A') + 10 : (what[0] - '0'));
	c *= 16;
	c +=(what[1] >= 'A' ? ((what[1] & 0xDF) - 'A') + 10 : (what[1] - '0'));
	
	return isurlchar(c) ? c : 0;
}

/*
 * char *unescape_url(char *url, int len)
 *
 * Unescape a URL (routine from the original NCSA server).
 */
char *unescape_url(char *url, int len)
{
	int x, y;
	char c, *newurl = malloc(len + 1);
	
	for (x = 0, y = 0; url[x] && x < len; ++x, ++y)
	{
		if ((newurl[x] = url[y]) == '%')
		{
			if ((c = x2c(&url[y + 1])))
			{
				newurl[x] = c;
				y += 2;
			}
		}
	}
	
	newurl[x] = '\0';
	return newurl;
}

/*
 * char *url2regex(char *domain)
 *
 * Converts a URL segment to a valid regular expression string.
 */
char *url2regex(char *domain)
{
	int i, domsize = 20, chars = 19;
	char *regex;
	
	for (i = 0; domain[i]; i++)
	{
		if (isregchar(domain[i]))
			domsize++;
		domsize++;
	}
	
	if (!(regex = malloc(domsize)))
		return NULL;
	
	strncpy(regex, "^([a-z0-9\\._-]+\\.)?", domsize);
	
	for (i = 0; domain[i] && chars < domsize; i++)
	{
		if (isregchar(domain[i]))
			regex[chars++] = '\\';
		regex[chars++] = domain[i];
	}
	
	regex[chars] = '\0';
	return regex;
}

static char *
get_lan_ip(char *buf, int len)
{
	int s, r;
	struct ifreq ifr;
	struct sockaddr_in in;
	
	s = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ifr,0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, LAN_IFNAME);
	r = ioctl(s, SIOCGIFADDR,&ifr);
	if (r < 0){
		DEBUG(LOG_INFO, "ioctl fail");
		return NULL;
	}
	memcpy(&in, &ifr.ifr_addr, sizeof(struct sockaddr_in));
	strncpy(buf, inet_ntoa(in.sin_addr), len);
	return buf;
}

static char *
redirect(char *buf, int len, char *url){
	char ip[16];
	int i = 0;

	if (get_lan_ip(ip, sizeof(ip)) == NULL){
		DEBUG(LOG_INFO, "get_lan_ip fail");
		return NULL;
	}
	while(url[i] != '\0'){
		if(url[i] == '/'){
			url[i] = '\0';
			break;
		}
		i++;
	}
	snprintf(buf, len,
	"HTTP/1.1 302 found\r\n"
#ifdef BUSYBOX_HTTPD
	"Location: http://%s%s?url=%s\r\n"
#else
	"Location: http://%s%s\r\n"
#endif
	"Content-Length: 0\r\n"
	"Connection: close\r\n\r\n", 
	ip, DENY_REDIRECT_PAGE, url);

	return buf;
}

/*
 * void deny_access(struct thread_data *td, int c_flag)
 *
 * Returns a "410 Gone" error with an appropriate file (for HTTP requests), or
 * just closes the connection (for HTTPS/tunnel connections).
 */
 
void deny_access(int client_fd, int c_flag, char *url)
{
	if (!c_flag)
	{
		DEBUG(LOG_INFO, "%s: !c_flag\n",__func__);
		/* Quick and dirty way to find the file extension. */
		int i = 0, loc = 0;
		
		while (url[i++])
		{
			if (url[i] == '.')
			{
				loc = i + 1;
			}
			
			if (url[i] == '?')
				break;
		}
		
		char *ext = malloc(4);
		strncpy(ext, (char *)(url + loc), 3);
		ext[3] = '\0';
		
		if (!strcasecmp(ext, "gif") || !strcasecmp(ext, "jpg") ||
			!strcasecmp(ext, "png") || !strcasecmp(ext, "jpe"))
		{
			DEBUG(LOG_INFO, "blocked[%s] is picture",url);
			DEBUG(LOG_INFO, "No redirect[%s]\n",_GLOBAL_BLOCKIMG);
			send(client_fd, _GLOBAL_BLOCKIMG, _GLOBAL_SIZEOF_BLOCKIMG, 0);
		}
		else
		{
			char *buf;
			DEBUG(LOG_INFO, "blocked[%s] isn't picture",url);		
			buf = malloc(1024);
			if(!buf){
				DEBUG(LOG_INFO, "buf malloc fail\n");
				return ;
			}	
			bzero(buf, 1024);
			
			if (redirect(buf, 1024, url)) {
				DEBUG(LOG_INFO, "Redirect[%s]", buf);					
				send(client_fd, buf, strlen(buf), 0);
			} else {
				DEBUG(LOG_INFO, "No redirect[%s]",_GLOBAL_BLOCKTEXT);
				send(client_fd, _GLOBAL_BLOCKTEXT,	_GLOBAL_SIZEOF_BLOCKTEXT, 0);
			}
			free(buf);
		}
	}
}

/*
 * void log_request(uint32 client_ip, char *headers, int headers_len,
 *                  uint32 auth_ip, uint32 netmask)
 *
 * Logs a request.
 * TODO: Change this to make an SNMP broadcast.
 */
void log_request(uint32 client_ip, char *headers, int headers_len,
				 uint32 auth_ip, uint32 netmask)
{
	int i;
	time_t t;
	struct tm *lt;
	char hyphen[2] = "-";
	char *buffer;
	char strbuf[16];
	char *ref, *u_a;
	
	buffer = malloc(headers_len + 1);
	memcpy(buffer, headers, headers_len + 1);
	
	/* Find the Referer: and User-Agent: headers. */
	strncpy(strbuf, "Referer: ", sizeof(strbuf));
	ref = strstr(buffer, strbuf);
	ref = ((ref) ? ref + 9 : hyphen);
	
	strncpy(strbuf, "User-Agent: ", sizeof(strbuf));
	u_a = strstr(buffer, strbuf);
	u_a = ((u_a) ? u_a + 12 : hyphen);
	
	/* Replace special characters with a space. */
	for (i = 0; i < headers_len; i++)
	{
		if (buffer[i] < ' ' || buffer[i] > '~')
		{
			if (buffer[i] == '\r' && buffer[i + 1] == '\n')
				buffer[i] = '\0';
			else
				buffer[i] = ' ';
		}
	}
	
	/* Put the information together and send it out. */
	t = time(NULL);
	lt = localtime(&t);
	lt->tm_year += 1900;
	lt->tm_mon++;
	
	int quads[4] = {
		(int)(client_ip)       & 0xFF,
		(int)(client_ip >>  8) & 0xFF,
		(int)(client_ip >> 16) & 0xFF,
		(int)(client_ip >> 24) & 0xFF
	};
	
	free(buffer);
	
	char *logstr = malloc(headers_len + 1);
	sprintf(logstr, "[%04d-%02d-%02d %02d:%02d:%02d] "
			"%d.%d.%d.%d \"%s\" \"%s\" \"%s\"", lt->tm_year,
			lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec,
		    quads[0], quads[1], quads[2], quads[3], buffer, ref, u_a);
	if(logstr) free(logstr);
	/* Insert SNMP-sending thing here. */
	auth_ip |= (~netmask);
}

#ifdef WIN32
HANDLE tdSem;
#define close(fd) closesocket(fd)
#endif

#define SKIP_SPACE(p) while (isspace(*(p)) && *(p) != '\0'){(p)++;}
#define JUMP_SPACE(p) while (!isspace(*(p)) && *(p) != '\0'){(p)++;}
/*
 * Get HTTP head's context
 * @header - Header string, such as "Host", case insensitive.
 * @s - Point to buffer of HTTP where MUST start with string @header
 *	such as "Host: foo.com\r\nOtherHead: ...."
 * Return:
 *	Return a string buffer. Caller should free() the buffer.
 *	Return NULL on error.
 */
static char *
header_from_line(const char *header, char *s)
{
	char *p, *e, *new = NULL;
	
	if (strncasecmp(s, header, strlen(header)))
		return NULL;
	
	p = s + strlen(header);
	SKIP_SPACE(p);
	if (*p != ':')		//header should be "Host:" 
		return NULL;
	
	p++;
	SKIP_SPACE(p);
	
	if (!(e = strchr(p, '\n'))) //header shoule end with a newline '\n'
		return NULL;
	new = strndup(p, (e - p));
	chop(new);
	return new;
	
}

static char *
fakeheader(char *buffer, int len)
{
	char *ptmp;
	
	ptmp = strstr(buffer, "\nHost:");

	if (ptmp) {
		char *h_host, *p, *stmp = NULL;
		char cmd[20];
		
		ptmp++;
		
		//get header "www.foo.com" from 'Host' header such as "Host: www.foo.com"
		h_host = header_from_line("Host", ptmp);
		if (!h_host){
			DEBUG(LOG_INFO, "header_from_line fail");
			return NULL;
		}	
		stmp = strdup(buffer);
		p = stmp;
		JUMP_SPACE(p);	//jump "GET" commmand
		bzero(cmd,sizeof(cmd));
		strncpy(cmd, stmp, p - stmp); //store command, "GET", "POST" ...
		SKIP_SPACE(p);	//skip space to "/url/..."
		
		
		if (*p != '/') {
			DEBUG(LOG_INFO, "Err HTTP Cmd: GET: [%s]", buffer);			
			free(h_host);
			free(stmp);
			return NULL;
			//child_exit(15);
		}
		
		DEBUG(LOG_INFO, "Old:[%s]", buffer);		
		sprintf(buffer, "%s http://%s%s", cmd, h_host, p);
		DEBUG(LOG_INFO, "New:[%s]", buffer);		
		free(h_host);
		free(stmp);
	} else {
		DEBUG(LOG_INFO, "Err HTTP header: Host: [%s]", buffer);		
		return NULL;
		//child_exit(15);
	}
	return buffer;
}

static int _GetMemInfo(char *strKey, char *strOut)
{
    FILE *fp;
    char buf[80], mem[32], *ptr;
    int ret = 0, index = 0;

    memset(mem, '\0', sizeof(mem));

    if (strlen(strKey) == 0)
        goto out;

    if ((fp = popen("cat /proc/meminfo", "r")) == NULL)
        goto out;

    while (fgets(buf, sizeof(buf), fp)) {
        if (ptr = strstr(buf, strKey)) {
            ptr += (strlen(strKey) + 2);

            /* Skip space */
            while (*ptr == ' ')  ptr++;

            /* Get value */
            while (*ptr != ' ') {
                mem[index] = *ptr;
                ptr++;
                index++;
            }

            break;
        }
    }

    pclose(fp);
    sprintf(strOut, "%d", atol(mem)/1024);
    ret = 1;
out:
    return ret;
}

static int check_mem()
{
	char mfree[32];
	int memfree = 0, ret = 0;

	if (!_GetMemInfo("MemFree:", mfree))
		goto out;
	if ((memfree = strtol(mfree, NULL, 10)) < 50) {
		DEBUG(LOG_INFO, "Memory free is lower than 50 : %d \n", memfree);
		goto out;
	}
	DEBUG(LOG_INFO, "Memory free : %d \n", memfree);
	ret = 1;
out:
	return ret;
}

static char buffer_client[URLBUF_SIZE+512];
int client_thread(struct thread_data *td)
{
	int remote_fd = -1, remote_port, state, c_flag, n, client_fd, loc;
	uint32 client_ip;	
	char *str_end, *current_url, c;
	char *url_host, *url_port, last_host[HOST_SIZE];
	
	struct sockaddr_in remote_addr;
	struct hostent *remote_host;
	struct timeval timeout;
	struct in_addr addr;
	
	fd_set rfds;	
#ifdef MIII_BAR
	struct sockaddr_in client_addr;
	int is_miii_file = 0;
	char *miii_p = NULL, *miii_pp = NULL, *miii_page=NULL;
	char lan_ip[20];
//	uint32 lan_ip;
	char eth[10];
	char wan_ipaddr[20];
	char wan_mac[40];
	char wan_mac_tmp[80];
	char *wmacencrypt =NULL ;
	struct in_addr in_lan_ip;
	int m=0,packet_num=0;
	char miii_html_header[512],*miii_html_header_t;
	char *p_get = NULL;
	
	in_lan_ip.s_addr = td->server_ip;
		//nvram_safe_get("lan_ipaddr", lan_ip, sizeof(lan_ip));
	//strcpy(lan_ip, "192.168.0.1:80");
	strcpy(lan_ip, inet_ntoa(in_lan_ip));
	//strcpy(wan_ipaddr, "172.21.33.123");
	DEBUG(LOG_INFO, "xxxxxxxxxxxx lan_ip=%s xxxxxxxxxxxxxxxxxxx\n",lan_ip);
	//nvram_safe_get("wan_eth", eth, sizeof(eth));
	//cmd_nvram_get("ipv6_pppoe_share", ipv6_pppoe_share);
	cmd_nvram_get("wan_eth",eth);
	get_ip_addr(eth,wan_ipaddr);
	
#if 0
	cmd_nvram_get("wan_mac",wan_mac);
	DEBUG(LOG_INFO, "000000000wan_mac=%s\n",wan_mac);	
	strcpy(wan_mac_tmp,wan_mac);	
	mac_encrypt(wan_mac_tmp);
	wmacencrypt = special_str_replace(wan_mac_tmp);
	memset(wan_mac_tmp, 0, 80);
	if(wmacencrypt != NULL)
	{
	        strcpy(wan_mac_tmp, wmacencrypt);
	        free(wmacencrypt);
	}
	else
	        strcpy(wan_mac_tmp, "00%3A03%3A2F%3A11%3A22%3A33");
#endif
	wmacencrypt = wan_mac_tmp;
	DEBUG(LOG_INFO, "11111 wan_mac=%s\n",wan_mac);		
	
	DEBUG(LOG_INFO, "22222 wmacencrypt=%s\n",wmacencrypt);	

	strcpy(wmacencrypt, "gyxmO2xfm0bV5h3UHMDBXlbXOfTTvAGuMns0LedDJJsxZsaf4eYnpB6dTnJVvft3");
	
#endif
	
	client_fd = td->client_fd;
	client_ip = td->client_ip;
	
#ifdef WIN32
	/* Let the master thread continue. */
	ReleaseSemaphore(tdSem, 1, NULL);
#endif
	
	/* Fetch the HTTP request headers. */
	FD_ZERO(&rfds);
	FD_SET((unsigned int)client_fd, &rfds);
	
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	DEBUG(LOG_INFO, "%s start",__func__);
	if (select(client_fd + 1, &rfds, NULL, NULL, &timeout) <= 0)
	{
		DEBUG(LOG_INFO, "select fail");
		child_exit(11);
	}
	
	if ((n = recv(client_fd, buffer_client, URLBUF_STRL, 0)) <= 0)
	{
		DEBUG(LOG_INFO, "recv fail");
		child_exit(12);
	}
	memset(last_host, 0, sizeof(last_host));
#ifdef MIII_BAR
	/* change HTTP/1.1 to HTTP/1.0 */
	if((miiicasa_enabled != NULL) && (miiicasa_enabled[0] == '1'))
	{
		DEBUG(LOG_INFO, "miiicasa_enabled run HTTP\n");
		if(check_mem() == 0)
		{
		        //miii_p = strstr(buffer_client, "User-Agent: BitTorrent");
		        //if (miii_p != NULL) 
		                child_exit(12);
		}
		miii_p = strstr(buffer_client, "HTTP/1.1");
		if (miii_p != NULL) {
			memcpy(miii_p, "HTTP/1.0", 8);
		}
		else {
			miii_p = strstr(buffer_client, "HTTP/1.0");
		}
		if(miii_p != NULL)
		{
		        miii_pp = strstr(buffer_client, "GET");
		        if(miii_pp != NULL)
		        {
		                //miii_page = strstr(buffer_client, "miiicasa");
		                if(1) //(miii_page == NULL)
		                {
		                        memset(miii_html_header, 0, sizeof(miii_html_header));
		                        if(((miii_p - miii_pp) > 0) && ((miii_p - miii_pp) < (sizeof(miii_html_header) -2)))
		                                strncpy(miii_html_header, miii_pp, miii_p - miii_pp);
		                        else
		                                strncpy(miii_html_header, miii_pp, sizeof(miii_html_header) - 2);
		                        if(((strstr(miii_html_header, ".jpg")) != NULL)||((strstr(miii_html_header, ".gif")) != NULL)||((strstr(miii_html_header, ".swf")) != NULL)||((strstr(miii_html_header, ".png")) != NULL))
		                        {
		                                miii_p = NULL;
		                        }
		                }
		        }
		        //else
		        //        miii_p = NULL;
	        }
	}

	/* remove Accept-Encoding:, Keep-Alive:, Connection:... */
	if(miii_p != NULL)
	{
		char *remove_line_str[] = {
			/* "Accept-Encoding:", */
			"Keep-Alive:",
			"Connection:",
			"Transfer-Encoding:",
			"Range:",
			//"If-Range:",
			//"Accept-Encoding:",
//"Cookie:",
			NULL };
		char *p1 = NULL;
		char *p2 = NULL;	
		char *tmp_buf;
		unsigned int new_len;
		int i = 0;
		char *p_remove_line = remove_line_str[0];
		char *keep_close_line = "Alpha-Mask: 115\r\nAlpha-Mask: closeclose\r\n";

		while (p_remove_line) {
			p1 = NULL;
			p1 = strstr(buffer_client, p_remove_line);
			if (p1) {
				p2 = NULL;
				p2 = strstr(p1, "\r\n");
				if (p2) {
					p2 += 2;
					new_len = (unsigned long) p2 - (unsigned long) p1;
					tmp_buf = malloc(n);
					if (tmp_buf) {
						int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
						if(!strcmp(p_remove_line, "Connection:"))
						{
							memcpy(tmp_buf, p2, shift_len);
							memcpy(p1, keep_close_line, strlen(keep_close_line));
							memcpy(p1+strlen(keep_close_line), tmp_buf, shift_len);
							n = n - new_len + strlen(keep_close_line);
					  }
					  else
					  {
							memcpy(tmp_buf, p2, shift_len);
							memcpy(p1, tmp_buf, shift_len);
							n = n - new_len;
						}
					}
					free (tmp_buf);
				}
			}
			p_remove_line = remove_line_str[++i];;
		}
	}	

	/* change Cookie: GZ=Z=1 to GZ=Z=0 */
	if(miii_p != NULL)
	{
		char *remove_line_str[] = {
			"Cookie:",
			NULL };
		char *p1 = NULL;
		char *p2 = NULL;	
		int i = 0;
		char *p_remove_line = remove_line_str[0];

		while (p_remove_line) {
			p1 = NULL;
			p1 = strstr(buffer_client, p_remove_line);

			if (p1) {
					p2 = NULL;
					p2 = strstr(p1, "GZ=Z=");
					if(p2 && (*(p2+5) == '1')) 
						*(p2+5) = '0';
					else
					{
						p2 = NULL;
						p2 = strstr(p1, "gz=1");
						if(p2) 
							*(p2+3) = '0';
					}
			}
			p_remove_line = remove_line_str[++i];;
		}
	}	

	/* change Accept-Encoding: xxx to "None" */
	if(miii_p != NULL)
	{
		char *remove_line_str[] = {
			"Accept-Encoding:",
			NULL };
		char *p1 = NULL;
		char *p2 = NULL;	
		char *tmp_buf;
		unsigned int new_len, old_len;
		int i = 0;
		char *p_remove_line = remove_line_str[0];
		char *embedded_str = "alpha";

		while (p_remove_line) {
			p1 = NULL;
			p1 = strstr(buffer_client, p_remove_line);

			if (p1) {
				p1 += strlen(p_remove_line) + 1;
				p2 = NULL;
				p2 = strstr(p1, "\r\n");
				if (p2) {
					old_len = (unsigned long) p2 - (unsigned long) p1;
					new_len = strlen(embedded_str);
					tmp_buf = malloc(n);
					if (tmp_buf) {
						int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
						memcpy(tmp_buf, p2, shift_len);
						memcpy(p1, embedded_str, new_len);
						p1 += new_len;
						memcpy(p1, tmp_buf, shift_len);
						n = n + new_len - old_len;
					}
					free (tmp_buf);
				}
			}
			p_remove_line = remove_line_str[++i];;
		}
	}	

	if((miiicasa_enabled != NULL) && (miiicasa_enabled[0] == '1'))
	{
		int is_map_remote = 0, is_miiicasa_prefix = 0;
		char *p1 = NULL;
		
		p_get = strstr(buffer_client, "GET");
		
		if(p_get)
		{
		        if(!memcmp(p_get + 4, miiicasa_prefix, 18))
		        {
		                DEBUG(LOG_INFO, "client_thread:GET %s\n",miiicasa_prefix);
		                is_miiicasa_prefix = 1;
		        }
		        else
		        {
		                p1 = strstr(p_get, "frameset=");
		                if (p1) {
			                p1 = NULL;
			                p1 = strstr(p_get, "wsip=");
			                if (p1) {
				                p1 = NULL;
				                p1 = strstr(p_get, "wip=");
				                if (p1) {
					                is_map_remote = 1;
				                }
			                }
		                }
		        }
		}


		if (is_miiicasa_prefix) {
			/* change URI-MAP */

			char *p1 = NULL;
			char *p2 = NULL;
			char *tmp_buf;
			unsigned int new_len, old_len;
			int i = 0;
			char embedded_str[100]; 
			memset(embedded_str, 0, 100);
			sprintf(embedded_str, "GET %s", miiicasa_forward_server);
			if(p_get)
			{
			        p1 = p_get;
				p2 = NULL;
				p2 = p_get + strlen("GET ") + strlen(miiicasa_prefix);
				if (p2) {
					old_len = (unsigned long) p2 - (unsigned long) p1;
					new_len = strlen(embedded_str);
					tmp_buf = malloc(n);
					if (tmp_buf) {
						int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
						memcpy(tmp_buf, p2, shift_len);
						memcpy(p1, embedded_str, new_len);
						p1 += new_len;
						memcpy(p1, tmp_buf, shift_len);
						n = n + new_len - old_len;
					}
					free (tmp_buf);
				}
				p1 = strstr(p_get, "HTTP/1.1");
		                if (p1 != NULL) {
			                memcpy(p1, "HTTP/1.0", 8);
		                }
			}
		}

		if (is_map_remote) {
			/* change URI-MAP */
			char *remove_line_str[] = {
			"/655",
			NULL };

			char *p1 = NULL;
			char *p2 = NULL;
			char *tmp_buf;
			unsigned int new_len, old_len;
			int i = 0;
			char *p_remove_line = remove_line_str[0];
			char embedded_str[100]; // = "http://img1.alpha.staging.miiicasa.com/i/toolbar.js HTTP /1.0"; 
			//"http://img1.qa.staging.muchiii.com/miii_miiibar_a42ac13a16c85d38e82fcaeb56e55da4.js";
			//"http://img1.alpha.corp.miiicasa.com/i/toolbar.js HTTP /1.0";
			//"http://a.mimgs.com/i/toolbar.js  HTTP /1.0";
			char *miii_UST_H_template = "Host: %s\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\np=%s&did=";
			char *miii_UST_template = "did=&brand=D-Link&miiicasa_enabled=1&wifi_secure_enabled=1&mac=%s&model=DIR-657&ws_ip=%s&ws_port=80&external_ip=%s&external_port=5555&thumbnail_supported=0&authorized=0";
			memset(embedded_str, 0, 100);
			DEBUG(LOG_INFO, "updateStatusTo:%d miiicasa_did:%s-%d\n",updateStatusTo,miiicasa_did,strlen(miiicasa_did));
			if(0) //((updateStatusTo == 0)&&strstr(buffer_client, "/65500000?")&&((((strlen(miiicasa_did)) == 1)&&(miiicasa_did[0] == '0'))||((strlen(miiicasa_did)) == 0)))
			{
				if(miiicasa_UST_server)
					sprintf(embedded_str, "POST http://%s/service/device/updateStatusTo HTTP/1.0\r\n",miiicasa_UST_server);
				else
					sprintf(embedded_str, "%s", "POST http://w1.partner.staging.miiicasa.com/service/device/updateStatusTo HTTP/1.0\r\n");
				updateStatusTo = 1;
				DEBUG(LOG_INFO, "updateStatusTo:%s\n",embedded_str);
			}
			else
			{
				updateStatusTo = 0;
				if(miiicasa_server_js)
					sprintf(embedded_str, "%s HTTP /1.0", miiicasa_server_js);
				else
					sprintf(embedded_str, "%s HTTP /1.0", "http://img1.partner.staging.miiicasa.com/i/toolbar.js");
				DEBUG(LOG_INFO, "updateStatusTo nono:%s\n",embedded_str);
			}
			while (p_remove_line) {

				p1 = NULL;
				p1 = strstr(buffer_client, p_remove_line);
	
				if (p1) {
					if(updateStatusTo != 0)
					{
						p1 = NULL;
						p1 = strstr(buffer_client, "GET");
						if(p1)
						{
							old_len = (unsigned long) p1 - (unsigned long) buffer_client;
							memset(p1, 0, n-old_len);
							memcpy(p1, embedded_str, strlen(embedded_str));
							//tmp_buf = malloc(1024);
							//memset(tmp_buf, 0, 1024);
							memset(miii_html_header, 0, sizeof(miii_html_header));
							//p2 = special_str_replace(cmd_nvram_get("wan_mac",wan_mac));
							//sprintf(tmp_buf, miii_UST_template, p2, lan_ip, wan_ipaddr);
							//if(p2) free(p2);
							//DEBUG(LOG_INFO, "d:%s\n",tmp_buf);
							//p2 = special_str_replace(md5_encrypt(tmp_buf, miiicasa_pre_key, 16));
							//strcpy(miii_html_header, p2);
							//DEBUG(LOG_INFO, "e:%s\n",p2);
							strcpy(miii_html_header, "ABYn4la0yU2c2nnxKWldALB06OSpbdNn1LFKvd%2B5AzjxLs2fuCWBRdq2LKutygdb51y17KliwGWH5DTcwbQrEL874bbrZd45rclbtdrAd1ijbtOw%2Fz2DNMexObbY1nNYo27TqeByxjnRsC%2F9lIwnDLp84IbmcI813rInqdvWelDjO%2Bmh%2B2XAaojsVuiDijZD4Si57Klt222A42jrjacnELV%2F4LzrPYMinulv8bOLJx2hb%2BmG6m7TZoXlbaXd3jYWoXDut%2B5p3lua9Xnog4o2G7AgvMqPALIE6YAJmOz4Qn4%3D");
							//if(p2) free(p2);
							//strcpy(miii_html_header, tmp_buf);
							if(miiicasa_UST_server)
							        sprintf(p1+strlen(embedded_str),miii_UST_H_template,miiicasa_UST_server,strlen(miii_html_header)+2+5,miii_html_header);
							else
							        sprintf(p1+strlen(embedded_str),miii_UST_H_template,"w1.partner.staging.miiicasa.com",strlen(miii_html_header)+2+5,miii_html_header);
							DEBUG(LOG_INFO, "e:%s\n",p1+strlen(embedded_str));
							n = old_len + strlen(p1);
							//if(tmp_buf) free (tmp_buf);
						}
					}
					else
					{
						p2 = NULL;
						p2 = strstr(p1, "\r\n");
						if (p2) {
							old_len = (unsigned long) p2 - (unsigned long) p1;
							new_len = strlen(embedded_str);
							tmp_buf = malloc(n);
							if (tmp_buf) {
								int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
								memcpy(tmp_buf, p2, shift_len);
								memcpy(p1, embedded_str, new_len);
								p1 += new_len;
								memcpy(p1, tmp_buf, shift_len);
								n = n + new_len - old_len;
							}
							free (tmp_buf);
						}
					}
				}
				p_remove_line = remove_line_str[++i];;
			}
		}

		if ((is_map_remote || is_miiicasa_prefix) && (updateStatusTo == 0)) {
			/* change URI-MAP and Host */
			char *remove_line_str[] = {
			"Host:",
			NULL };

			char *p1 = NULL;
			char *p2 = NULL;	
			char *tmp_buf;
			unsigned int new_len, old_len;
			int i = 0;
			char *p_remove_line = remove_line_str[0];
			//char *embedded_str = "img1.qa.staging.muchiii.com";
			//char *embedded_str = "img1.alpha.corp.miiicasa.com/i/toolbar.js";
			//char *embedded_str = "a.mimgs.com/i/toolbar.js";
			char embedded_str[100]; // = "img1.alpha.staging.miiicasa.com";
			memset(embedded_str, 0, 100);
		        if(is_map_remote)
		        {
			        if(miiicasa_server)
				        sprintf(embedded_str, "%s", miiicasa_server);
			        else
				        sprintf(embedded_str, "%s", "img1.partner.staging.miiicasa.com");
	                }
	                else
	                {
			        sprintf(embedded_str, "%s", "a.mimgs.com");
	                }
			while (p_remove_line) {
				p1 = NULL;
				p1 = strstr(buffer_client, p_remove_line);
	
				if (p1) {
					p1 += strlen(p_remove_line) + 1;
					p2 = NULL;
					p2 = strstr(p1, "\r\n");
					if (p2) {
						old_len = (unsigned long) p2 - (unsigned long) p1;
						new_len = strlen(embedded_str);
						tmp_buf = malloc(n);
						if (tmp_buf) {
							int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
							memcpy(tmp_buf, p2, shift_len);
							memcpy(p1, embedded_str, new_len);
							p1 += new_len;
							memcpy(p1, tmp_buf, shift_len);
							n = n + new_len - old_len;
						}
						free (tmp_buf);
					}
				}
				p_remove_line = remove_line_str[++i];;
			}
		}
	}
#endif
	
process_request:
	buffer_client[n] = '\0';
	
	/* Log the client request. */
	/*
	if (td->logfile)
		log_request(td->client_ip, buffer_client, n, td->auth_ip, td->netmask);
	*/

	/* Look to see if the HTTP request is a CONNECT method or not. */
	c_flag = 0;
	
	if (!strncmp(buffer_client, "CONNECT ", 8))
	{
		if (!td->connect)
		{
			DEBUG(LOG_INFO, "!td->connect fail");
			child_exit(13);
		}
		
		c_flag = 1;
	}
	
	/* Skip the HTTP method. */
	
	url_host = buffer_client;
	DEBUG(LOG_INFO, "url_host=%s",url_host);
	while (*url_host != ' ')
	{
		if ((url_host - buffer_client) > 10 || !(*url_host))
		{
			DEBUG(LOG_INFO, "url_host fail");
			child_exit(14);
		}		
		url_host++;
	}
	
	url_host++;
	
	/* Get the HTTP server hostname. */
	if (!c_flag)
	{
		if (!strncmp(url_host, "http://", 7))
			url_host += 7;
		else
		{
			DEBUG(LOG_INFO, "fake header");			
			if (!fakeheader(buffer_client, strlen(buffer_client))) {
				DEBUG(LOG_INFO,"fakeheader():NULL fail");				
				child_exit(15);
			}
			if (!strncmp(url_host, "http://", 7))
				url_host += 7;
		}
	}
	
	/* Copy the requested URL to a temporary buffer_client for access control. */
	for (loc = 0; url_host[loc] != (c_flag ? ':' : ' ') &&
	     url_host[loc]; loc++);
	current_url = malloc(loc + 1);
	strncpy(current_url, url_host, loc);
	DEBUG(LOG_INFO, "current_url=%s",current_url);	
	char *new_url;
	new_url = unescape_url(current_url, loc);
	DEBUG(LOG_INFO, "new_url=%s",new_url);
	free(current_url);
	current_url = new_url;
	/* Resolve the HTTP server hostname. */
	str_end = url_host;
	
	while (*str_end != ':' && *str_end != '/' && *str_end != '\0')
	{
		if ((str_end - url_host) >= HOST_SIZE || (!(*str_end)))
		{
			DEBUG(LOG_INFO, "url_host size fail");
			child_exit(16);
		}
		
		str_end++;
	}

	c = *str_end;
	*str_end = '\0';
	//if (!check_urls(url_host, 1) || (!c_flag && !check_urls(current_url, 0)))
	if (!check_urls(url_host, 1))
	{
/* 
 Date: 2009-1-05 
 Name: Ken_Chiang
 Reason: modify for the crowdcontrol can open and close used the log.
 Notice :
*/	
		//list_ipitem(log_iplists);
		if(chk_logip(client_ip, log_iplists, 0, current_url)){
			addr.s_addr = client_ip;
			/*
				NickChou move syslog into chk_logip() 20090610
				To show MAC in GUI Log Page
				When using Access Control -> MAC Address filter for Block Some Access
			*/
			//DEBUG(LOG_INFO, "%s blocked for %s", current_url, inet_ntoa(addr));
		}		
			
		DEBUG(LOG_INFO, "blocked[%s]", current_url);		
		deny_access(client_fd, c_flag, current_url);
		free(current_url);
		//if (!c_flag)
		if (c_flag) // I think that is right way to exit socket, peter		
		{
			DEBUG(LOG_INFO, "c_flag close");			
			child_exit(17);
		}
		DEBUG(LOG_INFO, "close\n");
		child_exit(17); //add by peter
		DEBUG(LOG_INFO, "process_request\n");
		goto process_request;
	}
/* 
 Date: 2009-1-05 
 Name: Ken_Chiang
 Reason: modify for the crowdcontrol can open and close used the log.
 Notice :
*/	
	if(chk_logip(client_ip, log_iplists, 1, current_url)){
		addr.s_addr = client_ip;
		/*
			NickChou move syslog into chk_logip() 20090610
			To show MAC in GUI Log Page
			When using Access Control -> MAC Address filter for Block Some Access
		*/
		//DEBUG(LOG_INFO, "%s accessed from %s\n", current_url, inet_ntoa(addr));
	}		
	if (!(remote_host = gethostbyname(url_host)))
	{
		DEBUG(LOG_INFO, "gethostbyname[%s] error\n", url_host);				
		child_exit(18);
	}
	DEBUG(LOG_INFO, "remote_host->h_addr=%x\n",remote_host->h_addr);
	
	*str_end = c;
	
	/* Get the HTTP server port. */
	if (c != ':')
	{
		DEBUG(LOG_INFO, "c != :");
		c           = *str_end;
		*str_end    = '\0';
		remote_port = 80;
	}
	else
	{
		DEBUG(LOG_INFO, "c = :");
		url_port = ++str_end;
		while (*str_end != ' ' && *str_end != '/')
		{
			if (*str_end < '0' || *str_end > '9')
			{
				DEBUG(LOG_INFO, "str_end fail");
				child_exit(19);
			}
			
			str_end++;
			
			if (str_end - url_port > 5)
			{
				DEBUG(LOG_INFO, "str_end - url_port > 5 fail");
				child_exit(20);
			}
		}
		
		c           = *str_end;
		*str_end    = '\0';
		remote_port = atoi(url_port);
	}
	DEBUG(LOG_INFO, "remote_port=%d",remote_port);
	if (c_flag)
	{
		if ((td->connect == 1 && remote_port != 443) ||
		    (remote_port < 1 || remote_port > 65535))
		{
			DEBUG(LOG_INFO, "remote_port=%d fail",remote_port);
			child_exit(21);
		}
	}
	
	/* Connect to the remote server, if not already connected. */
	if (strcmp(url_host, last_host))
	{
		shutdown(remote_fd, 2);
		close(remote_fd);
		
		remote_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		
		if (remote_fd < 0)
		{
			DEBUG(LOG_INFO, "remote_fd socket fail");
			child_exit(22);
		}
		
		remote_addr.sin_family = AF_INET;
		remote_addr.sin_port   = htons((unsigned short)remote_port);
		
		memcpy((void *)&remote_addr.sin_addr, (void *)remote_host->h_addr,
		       remote_host->h_length);
		
		if (connect(remote_fd, (struct sockaddr *)&remote_addr,
			sizeof(remote_addr)) < 0)
		{
			DEBUG(LOG_INFO, "remote_fd connect fail");
			child_exit(23);
		}
		
		memset(last_host, 0, sizeof(last_host));
		strncpy(last_host, url_host, sizeof(last_host) - 1);
	}
	
	*str_end = c;
	DEBUG(LOG_INFO, "session[%s]", current_url);		
	
	if (c_flag)
	{
		DEBUG(LOG_INFO, "c_flag Accept[%s]", current_url);				
		strncpy(buffer_client, "HTTP/1.0 200 Connection Established\r\n\r\n",
				sizeof(buffer_client));
		
		if (send(client_fd, buffer_client, 39, 0) != 39)
		{
			DEBUG(LOG_INFO, "accept[%s] send fail",current_url);
			child_exit(24);
		}
	}
	else
	{
		int m_len;

		/* Remote "http://hostname[:port]" and send headers. */
		DEBUG(LOG_INFO, "!c_flag u[%s]h[%s]e[%s]", url_host, buffer_client, str_end);		
		m_len = url_host - 7 - buffer_client;
		n -= 7 + (str_end - url_host);
		memcpy(str_end -= m_len, buffer_client, m_len);
		DEBUG(LOG_INFO, "send:[%s]", str_end);		
		
		if (n != strlen(str_end)) {
			DEBUG(LOG_INFO, "length differ:%d:%d", n, strlen(str_end));			
			n = strlen(str_end);	
		}
		
		if (send(remote_fd, str_end, n, 0) != n)
		{
			DEBUG(LOG_INFO, "str_end[%s] send fail",str_end);
			child_exit(25);
		}
	}
	
	/* Tunnel the data between the client and the server. */
	state = 0;
#ifdef MIII_BAR			
	packet_num=0;
#endif
	
	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET((unsigned int)client_fd, &rfds);
		FD_SET((unsigned int)remote_fd, &rfds);
		
		n = (client_fd > remote_fd) ? client_fd : remote_fd;
		
		if (select(n + 1, &rfds, NULL, NULL, NULL) < 0)
		{
			DEBUG(LOG_INFO, "select fail");
			child_exit(26);
		}
		
		if (FD_ISSET(remote_fd, &rfds))
		{
			DEBUG(LOG_INFO, "remote_fd FD_ISSET");
			if ((n = recv(remote_fd, buffer_client, URLBUF_STRL, 0)) <= 0)
			{
				DEBUG(LOG_INFO, "remote_fd recv fail");
				child_exit(27);
			}
			state = 1;
#ifdef MIII_BAR
                        packet_num ++;		
			if((miiicasa_enabled != NULL) && (miiicasa_enabled[0] == '1') && (miii_p != NULL))
			{
		                DEBUG(LOG_INFO, "xxxxxxxxxxxMIII_BAR startxxxxxxxxxxxxxxxxxxxx\n");			
				char *miii_toolbar_js_template = 
				//"<script type=\"text/javascript\" src=\"http://a.mimgs.com/i/toolbar.js?frameset=0&wsip=%s&wip=%s\"></script>";
				//"<div id=\"miii-root\"></div><script>(function () { var e = document.createElement(\"script\"); e.type = \"text/javascript\"; e.id = \"miii-seed\"; e.src = \"/%08x?frameset=%d&wsip=%s&wip=%s\"; e.async = true; document.getElementById(\"miii-root\").appendChild(e); }()); </script>";  
				"<div id=\"miii-root\"></div>\r\n<script>\r\nmiii_url = \"/%08x?frameset=%d&wsip=%s:%d&wip=%s&m=%s&did=%s\";\r\n(function () {\r\n    var e = document.createElement(\"script\");\r\n    e.type = \"text/javascript\";\r\n    e.id = \"miii-seed\";\r\n    e.src = miii_url;\r\n    e.async = true;\r\n    document.getElementById(\"miii-root\").appendChild(e);\r\n    // dev2\r\n}());\r\n</script>";  
				//"<script type=\"text/javascript\" src=\"/%08x?frameset=%d&wsip=%s&wip=%s\"></script>"; //47bce5c74f589f4867dbd57e9ca9f808
				char *content_len_str = "Content-Length: ";
				char *body_end = "<body";
				char *html_end = "</html>";
				char *tmp_buf = NULL;
				char *p1 = NULL, *p2 = NULL;
				#define MAX_CONTENT_LEN_STR_LEN 10
				char len_str[MAX_CONTENT_LEN_STR_LEN];
				unsigned int len = 0;
				unsigned int old_len = 0;
				unsigned int new_len = 0;
				int miii_done,miii_iframe=0;
				unsigned int randsalt; // = random();
			        srand((unsigned int)time(NULL));
				randsalt = rand();
			        
			        memset(miii_html_header, 0, sizeof(miii_html_header));
			        if(0) //(strlen(buffer_client) > 600)
			        {
			                memset(miii_html_header, 0, sizeof(miii_html_header));
			                memcpy(miii_html_header, buffer_client, sizeof(miii_html_header)-2);
			                miii_html_header_t = miii_html_header;
			        }
			        else
			        {
			                miii_html_header_t = buffer_client;
			        }
			        
				bzero(len_str, MAX_CONTENT_LEN_STR_LEN);
			        if(strstr(buffer_client, "<iframe")) miii_iframe = 1;
			        memset(miii_html_header, 0, sizeof(miii_html_header));
                                //"<div id=\"miii-root\"></div>\r\n<script>\r\nmiii_url = \"/%08x?frameset=%d&wsip=%s:%d&wip=%s&m=gyxmO2xfm0bV5h3UHMDBXlbXOfTTvAGuMns0LedDJJsxZsaf4eYnpB6dTnJVvft3&did=47bce5c74f589f4867dbd57e9ca9f808\";\r\n(function () {\r\n    var e = document.createElement(\"script\");\r\n    e.type = \"text/javascript\";\r\n    e.id = \"miii-seed\";\r\n    e.src = miii_url;\r\n    e.async = true;\r\n    document.getElementById(\"miii-root\").appendChild(e);\r\n    // dev2\r\n}());\r\n</script>";  
				sprintf(miii_html_header, miii_toolbar_js_template, 
			                0x65500000 | (randsalt & 0xfffff), miii_iframe, lan_ip,remote_port, wan_ipaddr, wmacencrypt, miiicasa_did); /* will be replaced with correct value ...*/
		                DEBUG(LOG_INFO, "xxxxxxxxxxx miii_toolbar_js_template=%s xxxxxxxxxxxxxxxxxxxx\n",miii_toolbar_js_template);	
				if ((is_miii_file == 0)&&(packet_num <= 1)) {
					miii_done = 0;
					/* check header */
					p1 = strstr(miii_html_header_t, "HTTP/1.1 200");
                			if (p1 == NULL)
						p1 = strstr(miii_html_header_t, "HTTP/1.0 200");
                    
					if (p1 != NULL) {
						p2 = NULL;
						p2 = strstr(miii_html_header_t, "Content-Type: text/html");
						if(p2 == NULL)
						        is_miii_file = 0;
						else
						{
#if 0
						        p2 = NULL;
						        p2 = strstr(miii_html_header_t, "Content-Type: image/");
						        if(p2 == NULL)
						                p2 = strstr(miii_html_header_t, "Content-Type: application/");
						        //if ((strstr(miii_html_header_t, "muchiii"))||(strstr(miii_html_header_t, "x-javascript"))) {
						        if (((strstr(miii_html_header_t, "muchiii"))&&(p1 ==NULL)) || ((strstr(miii_html_header_t, "miiicasa"))&&(p1 ==NULL))||(strstr(miii_html_header_t, "x-javascript"))||(p2!=NULL)) {
			                                //if (((strstr(miii_html_header_t, "muchiii"))&&(p1 ==NULL))||(strstr(miii_html_header_t, "x-javascript")) ||(strstr(miii_html_header_t, "miiicasa"))) {
							        is_miii_file = 0;
						        }
						        else {
							        is_miii_file = 1;
						        }
						        //p1 = NULL;
						        //p1 = strstr(buffer_client, "Content-Encoding");
						        //if (p1 != NULL) {
						        //	is_miii_file = 0;
						        //}
#else
                                                        is_miii_file = 1;
#endif
						        p2 = NULL;
						        p2 = strstr(miii_html_header_t, "X-RADID: ");
						        if(p2 != NULL) is_miii_file = 0;
						}
					}

        				if (is_miii_file) {
						/* len */
                    				p1 = NULL;
                    				p1 = strstr(buffer_client, content_len_str);
                        
                   				if (p1 != NULL) {
							p1 += strlen (content_len_str);
							p2 = strstr(p1, "\r\n");
							old_len = (unsigned long) p2 - (unsigned long) p1;
							strncpy(len_str, p1, old_len);
							len = atoi(len_str);

							tmp_buf = malloc(n);

							if (tmp_buf != NULL) {
								int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
								memcpy(tmp_buf, p2, shift_len);
								
 								sprintf(len_str, "%u", len + strlen(miii_html_header));
 								new_len = strlen(len_str);
  								memcpy(p1, len_str, strlen(len_str));
								p1 += strlen(len_str);
                            
								memcpy(p1, tmp_buf, shift_len);
 								free (tmp_buf);

								n = n + new_len - old_len;
							}
						}
					}

					/* remove Connection: ... */
					if (is_miii_file) {
						char *connection_str = "Connection:";			
						p1 = NULL;
						p1 = strstr(buffer_client, connection_str);
						if (p1 != NULL) {
							p2 = NULL;
							p2 = strstr(p1, "\r\n");
							if (p2 != NULL) {
								p2 += 2;
								new_len = (unsigned long) p2 - (unsigned long) p1;
								tmp_buf = malloc(n);
								if (tmp_buf != NULL) {
									int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
									memcpy(tmp_buf, p2, shift_len);
									memcpy(p1, tmp_buf, shift_len);
									n = n - new_len;
								}
								free (tmp_buf);
							}
						}
					}

					/* remove Keep-Alive: ... */
					if (is_miii_file) {
						char *connection_str = "Keep-Alive:";			
						p1 = NULL;
						p1 = strstr(buffer_client, connection_str);
						if (p1 != NULL) {
							p2 = NULL;
							p2 = strstr(p1, "\r\n");
							if (p2 != NULL) {
								p2 += 2;
								new_len = (unsigned long) p2 - (unsigned long) p1;
								tmp_buf = malloc(n);
								if (tmp_buf != NULL) {
									int shift_len = n - ((unsigned long) p2 - (unsigned long) buffer_client);
									memcpy(tmp_buf, p2, shift_len);
									memcpy(p1, tmp_buf, shift_len);
									n = n - new_len;
								}
								free (tmp_buf);
                        				}
                        			}
                    			}
                    
					/* add Connection: Close\r\n" */
					if (is_miii_file) {
						char *end_header = "\r\n\r\n";
						char *close_str = "Connection: close\r\n";
						/* len */

						p1 = NULL;
						p1 = strstr(buffer_client, end_header);
                        
						if (p1 != NULL) {
							p1 += 2;
							tmp_buf = malloc(n);
                 
							if (tmp_buf != NULL) {
								int shift_len = n - ((unsigned long) p1 - (unsigned long) buffer_client);	
								memcpy(tmp_buf, p1, shift_len);
								
								new_len = strlen(close_str);
								memcpy(p1, close_str, strlen(close_str));
								p1 += strlen(close_str);
                            
								memcpy(p1, tmp_buf, shift_len);
								free (tmp_buf);

								n = n + new_len;
							}
						}
					}
				}
			
				if (is_miii_file && miii_done == 0) {
					p1 = NULL;
					p1 = strstr(buffer_client, body_end);
			
					if ((p1 != NULL)&&(((unsigned long)p1 - (unsigned long)buffer_client) > 30)) {
					        char document_tmp[40];
					        memset(document_tmp, 0, sizeof(document_tmp));
					        memcpy(document_tmp, p1-30, 30);
						p2 = NULL;
						p2 = strstr(document_tmp, "document.write");
						if(p2 != NULL)
						{
				                        p1 = NULL;
				                        p1 = strstr(p1+6, body_end);
					        }
					}
			
					if (p1 != NULL) {
						  p1 = strstr(p1, ">");
						  if (p1 != NULL) {
						  	p1++;
        	    					tmp_buf = malloc(URLBUF_SIZE+512);
        	    					if (tmp_buf != NULL) {
									int shift_len = n - ((unsigned long) p1 - (unsigned long) buffer_client);
									memcpy(tmp_buf, p1, shift_len);
									memcpy(p1, miii_html_header, strlen(miii_html_header));
									memcpy(p1+strlen(miii_html_header), tmp_buf, shift_len);
									n += strlen(miii_html_header);
									free (tmp_buf);
									miii_done = 1;
								}
							}
					}
				}
			}
#endif
//                        DEBUG(LOG_INFO, "client_fd send - %d - %d",n,packet_num);
			if (send(client_fd, buffer_client, n, 0) != n)
			{
				DEBUG(LOG_INFO, "client_fd send fail");
				child_exit(28);
			}
		}
		
		if (FD_ISSET(client_fd, &rfds))
		{
			DEBUG(LOG_INFO, "client_fd FD_ISSET");
			if ((n = recv(client_fd, buffer_client, URLBUF_STRL, 0)) <= 0)
			{
				DEBUG(LOG_INFO, "client_fd recv fail");
				child_exit(29);
			}
			
			if (state && !c_flag)
			{
				/* New HTTP request. */
				DEBUG(LOG_INFO, "New request process_request");
				goto process_request;
			}
			
			if (send(remote_fd, buffer_client, n, 0) != n)
			{
				DEBUG(LOG_INFO, "remote_fd send fail");
				child_exit(30);
			}
		}
	}	
	/* Not reached. */
	return -1;
}

static int read_schedule_from_config(const char *sch_cfg)
{
	int ret = -1;
	char *p, line[128];
	FILE *fp = fopen(sch_cfg, "r");

	if (fp == NULL)
		goto out;

	while (fscanf(fp, "%s", line) != EOF) {
		char *key;

		p = line;
		key = strsep(&p, "=");

		if (strncmp(key, "schedule_rule", 13) == 0) {
			strcpy(sys_sch[syssch_certain_exists].key, key);
			strcpy(sys_sch[syssch_certain_exists++].content, p);
		} else {
			strcpy(url_sch[urlsch_certain_exists].key, key);
			strcpy(url_sch[urlsch_certain_exists++].content, p);
		}
	}

	fclose(fp);
	do_schedule = 1;	/* open schedule checking procedure */
	ret = 0;
out:
	return ret;
}

static int parse_parameters(int parse_argc, char **argv, struct thread_data *td, int *proxy_port, char **testurl)
{
	int i, param = 0, rp = 0;
	char *permitdomains = NULL, *permiturls = NULL, *permitexpressions = NULL;
	char *denydomains = NULL, *denyurls = NULL, *denyexpressions = NULL;
	/*
	Date: 2009-1-05
	Name: Ken_Chiang
	Reason: modify for the crowdcontrol can open and close used the log.
	Notice :
	*/
	char *logips = NULL;
	/* Date: 2009-01-08
	 * Name: Fred Hung
	 * Reason: let crowdcontrol identify schedule rule
	 */
	char *sch_cfg = NULL;
	/* Read the program arguments. */
	td->server_ip = INADDR_ANY;
	td->auth_ip   = INADDR_ANY;
	td->netmask   = 0xFFFFFFFF;
	td->connect   = 2;

	for (i = 1; i < parse_argc; i++)
	{
		if (argv[i][0] == '-')
		{
			FPRINTF(stderr, "argv[%d][0]==\'-\'\n",i);
			if (rp)
			{
				FPRINTF(stderr, _GLOBAL_ERRPARAM, argv[0], argv[i - rp]);
			}
			
			if (argv[i][1] == '-')
			{
				FPRINTF(stderr, "argv[%d][1]==\'-\'\n",i);
				if (!strcmp(argv[i], "--port"))
				{
					FPRINTF(stderr, "argv[%d] --port\n",i);
					param = PARAM_PORT;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--bind"))
				{
					FPRINTF(stderr, "argv[%d] --bind\n",i);
					param = PARAM_BIND;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--subnet"))
				{
					FPRINTF(stderr, "argv[%d] --subnet\n",i);
					param = PARAM_SUBNET;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--max-connections"))
				{
					FPRINTF(stderr, "argv[%d] --max-connections\n",i);
					param = PARAM_MAXCONNECTIONS;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--http-only"))
				{
					FPRINTF(stderr, "argv[%d] --http-only\n",i);
					param = PARAM_HTTPONLY;
					continue;
				}
				
				if (!strcmp(argv[i], "--http-with-ssl"))
				{
					FPRINTF(stderr, "argv[%d] --http-with-ssl\n",i);
					param = PARAM_HTTPWITHSSL;
					continue;
				}
				
				if (!strcmp(argv[i], "--http-tunnel"))
				{
					FPRINTF(stderr, "argv[%d] --http-tunnel\n",i);
					param = PARAM_HTTPTUNNEL;
					continue;
				}
				
				if (!strcmp(argv[i], "--implicitly-deny"))
				{
					FPRINTF(stderr, "argv[%d] --implicitly-deny\n",i);					
					param = PARAM_IMPLICITLYDENY;
					_GLOBAL_IMPLICITACTION = BLOCKED;
					continue;
				}
				
				if (!strcmp(argv[i], "--implicitly-permit"))
				{
					FPRINTF(stderr, "argv[%d] --implicitly-permit\n",i);
					param = PARAM_IMPLICITLYPERMIT;
					continue;
				}
				
				if (!strcmp(argv[i], "--permit-domains"))
				{
					allow_action = 1;
					FPRINTF(stderr, "argv[%d] --permit-domains\n",i);
					param = PARAM_PERMITDOMAINS;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--permit-urls"))
				{
					allow_action = 1;
					FPRINTF(stderr, "argv[%d] --permit-urls\n",i);
					param = PARAM_PERMITURLS;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--permit-expressions"))
				{
					allow_action = 1;
					FPRINTF(stderr, "argv[%d] --permit-expressions\n",i);
					param = PARAM_PERMITEXPRESSIONS;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--deny-domains"))
				{
					allow_action = 0;
					FPRINTF(stderr, "argv[%d] --deny-domains\n",i);
					param = PARAM_DENYDOMAINS;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--deny-urls"))
				{
					allow_action = 0;
					FPRINTF(stderr, "argv[%d] --deny-urls\n",i);
					param = PARAM_DENYURLS;
					rp = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--deny-expressions"))
				{
					allow_action = 0;
					FPRINTF(stderr, "argv[%d] --deny-expressions\n",i);
					param = PARAM_DENYEXPRESSIONS;
					rp    = 1;
					continue;
				}
				
				if (!strcmp(argv[i], "--test"))
				{
					FPRINTF(stderr, "argv[%d] --test\n",i);
					param = PARAM_TESTURL;
					rp    = 1;
					continue;
				}				
			}
			else
			{
				FPRINTF(stderr, "argv[%d][1]!=\'-\'\n",i);
				if (argv[i][1] == 'p')
				{
					FPRINTF(stderr, "argv[%d] -p\n",i);
					param = PARAM_PORT;
					rp    = 1;
					continue;
				}
				
				if (argv[i][1] == 'a')
				{
					FPRINTF(stderr, "argv[%d] -a\n",i);
					param = PARAM_BIND;
					rp    = 1;
					continue;
				}
				
				if (argv[i][1] == 's')
				{
					FPRINTF(stderr, "argv[%d] -s\n",i);
					param = PARAM_SUBNET;
					rp    = 1;
					continue;
				}
				
				if (argv[i][1] == 'm')
				{
					FPRINTF(stderr, "argv[%d] -m\n",i);
					param = PARAM_MAXCONNECTIONS;
					rp    = 1;
					continue;
				}
				/* 
 				Date: 2009-1-05 
 				Name: Ken_Chiang
 				Reason: modify for the crowdcontrol can open and close used the log.
 				Notice:
 				*/
 				if (argv[i][1] == 'l')
				{
					FPRINTF(stderr, "argv[%d] -l\n",i);									
					param = PARAM_USEDLOG;
					rp    = 1;
					continue;
				}
				/* Date: 2009-01-08
				 * Name: Fred Hung
				 * Reason: let crowdcontrol identify the schedule file
				 */
				if (argv[i][1] == 'S') {
					FPRINTF(stderr, "argv[%d] -S\n", i);
					param = PARAM_SCHEDULE;
					rp = 1;
					continue;
				}
			}
		}
		else
		{
			FPRINTF(stderr, "argv[%d][0]!=\'-\'\n",i);
			switch (param)
			{
				case 0:
					break;
					
				case PARAM_PORT:
					*proxy_port = atoi(argv[i]);
					FPRINTF(stderr, "proxy_port=%x\n",*proxy_port);
					--rp;
					break;
					
				case PARAM_BIND:
					td->auth_ip = td->server_ip = inet_addr(argv[i]);
					FPRINTF(stderr, "td->server_ip=%x\n",td->server_ip);
					--rp;
					break;
					
				case PARAM_SUBNET:
					td->netmask = inet_addr(argv[i]);
					FPRINTF(stderr, "td->netmask=%x\n",td->netmask);
					--rp;
					break;
					
				case PARAM_MAXCONNECTIONS:
					_GLOBAL_MAX_CONNECTIONS = atoi(argv[i]);
					if (!_GLOBAL_MAX_CONNECTIONS)
						_GLOBAL_MAX_CONNECTIONS = 16;
					FPRINTF(stderr, "_GLOBAL_MAX_CONNECTIONS=%x\n",_GLOBAL_MAX_CONNECTIONS);	
					--rp;
					break;
					
				case PARAM_HTTPONLY:
					td->connect = 0;
					FPRINTF(stderr, "td->connect=%x\n",td->connect);
					break;
					
				case PARAM_HTTPWITHSSL:
					td->connect = 1;
					FPRINTF(stderr, "td->connect=%x\n",td->connect);
					break;
					
				case PARAM_HTTPTUNNEL:
					td->connect = 2;
					FPRINTF(stderr, "td->connect=%x\n",td->connect);
					break;
					
				case PARAM_IMPLICITLYDENY:
					_GLOBAL_IMPLICITACTION = BLOCKED;
					FPRINTF(stderr, "_GLOBAL_IMPLICITACTION=%x\n",_GLOBAL_IMPLICITACTION);
					break;
					
				case PARAM_IMPLICITLYPERMIT:
					break;
					
				case PARAM_PERMITDOMAINS:
					permitdomains = argv[i];
					FPRINTF(stderr, "permitdomains=%s\n",permitdomains);
					--rp;
					break;
					
				case PARAM_PERMITURLS:
					permiturls = argv[i];
					FPRINTF(stderr, "permiturls=%s\n",permiturls);
					--rp;
					break;
					
				case PARAM_PERMITEXPRESSIONS:
					permitexpressions = argv[i];
					FPRINTF(stderr, "permitexpressions=%s\n",permitexpressions);
					--rp;
					break;
					
				case PARAM_DENYDOMAINS:
					denydomains = argv[i];
					FPRINTF(stderr, "denydomains=%s\n",denydomains);
					--rp;
					break;
					
				case PARAM_DENYURLS:
					denyurls = argv[i];
					FPRINTF(stderr, "denyurls=%s\n",denyurls);
					--rp;
					break;
					
				case PARAM_DENYEXPRESSIONS:
					denyexpressions = argv[i];
					FPRINTF(stderr, "PARAM_DENYEXPRESSIONS=%s\n",PARAM_DENYEXPRESSIONS);
					--rp;
					break;
					
				case PARAM_TESTURL:
					*testurl = argv[i];
					FPRINTF(stderr, "testurl=%s\n",*testurl);
					--rp;
					break;
				/* 
 				Date: 2009-1-05 
 				Name: Ken_Chiang
 				Reason: modify for the crowdcontrol can open and close used the log.
 				Notice:
 				*/	
				case PARAM_USEDLOG:
					logips = argv[i];
					FPRINTF(stderr, "logips=%s\n",logips);						
					--rp;
					break;
				/* Date: 2009-01-08
				 * Name: Fred Hung
				 * Reason: let crowdcontrol identify schedule file
				 */
				case PARAM_SCHEDULE:
					sch_cfg = argv[i];
					FPRINTF(stderr, "sch_cfg=%s\n", sch_cfg);
					--rp;
					break;
			}
			
			param = 0;
		}
	}
	
	if (rp)
	{
		FPRINTF(stderr, _GLOBAL_ERRPARAM, argv[0], argv[i - rp]);
		return 1;
	}
	/* Process a list of permitted domains. */
	permitted_urls = loaddomainfile(argv[0], permitdomains, permitted_urls);
	
	/* Process a list of permitted URLs. */
	permitted_urls = loadurlfile(argv[0], permiturls, permitted_urls);
	
	/* Process a list of permitted expressions. */
	permitted_urls = loadexpressionfile(argv[0], permitexpressions,
		                                      permitted_urls);
	
	/* Process a list of blocked domains. */
	blocked_urls = loaddomainfile(argv[0], denydomains, blocked_urls);
	
	/* Process a list of blocked URLs. */
	blocked_urls = loadurlfile(argv[0], denyurls, blocked_urls);
	
	/* Process a list of blocked expressions. */
	blocked_urls = loadexpressionfile(argv[0], denyexpressions, blocked_urls);
	
	/* 
 	Date: 2009-1-05 
 	Name: Ken_Chiang
 	Reason: modify for the crowdcontrol can open and close used the log.
 	Notice :
	*/	
	/* Process a list of log ip. */
	log_iplists = loadiplogfile(argv[0], logips, log_iplists);

	/* Date: 2009-01-08
	 * Name: Fred Hung
	 * Reason: read schedule info from schedule config file
	 */
	sch_cfg ? read_schedule_from_config(sch_cfg):0;

	if (*testurl)
	{
		if (!check_urls(*testurl, 1))
		{
			FPRINTF(stderr, "%s Domain blocked.\n", *testurl);
			return 0;
		}
		if (!check_urls(*testurl, 0))
		{
			FPRINTF(stderr, "%s URL/regex blocked.\n", *testurl);
			return 0;
		}

		FPRINTF(stderr, "%s Permitted.\n", *testurl);
		return 0;
	}
	td->auth_ip  &= td->netmask;
	td->client_ip = 0;

	return 0;
}

#ifdef MIII_BAR
int miiicasa_UST_sned()
{
	int	fd, ret=0;
	char recv_buf[2048]={0};
	char send_buf[1024]={0};
	char miii_UST_tmp[1024]={0},*tmp_buf,*p2;
	struct  in_addr saddr;
	struct  sockaddr_in addr;
	struct  hostent *host;
	fd_set afdset;
	struct timeval timeval;
	char wan_mac[40];
	char eth[10];
	char lan_ip[20];
	char wan_ipaddr[20];
	int miiicasa_did_is=0;
	char *fw_host = "w1.partner.staging.miiicasa.com";
	//char *miii_UST_H_template = "POST http://%s/service/device/updateStatusTo HTTP/1.0\r\nHost: %s\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\np=%s";
	char *miii_UST_H_template = "POST /service/device/updateStatusTo HTTP/1.0\r\nHost: %s\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\np=%s";
	char *miii_UST_template = "did=&brand=D-Link&miiicasa_enabled=1&wifi_secure_enabled=1&mac=%s&model=DIR-657&ws_ip=%s&ws_port=80&external_ip=%s&external_port=5555&thumbnail_supported=0&authorized=0";
	char *miii_UST_is_H_template = "POST /service/device/updateStatusTo HTTP/1.0\r\nHost: %s\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: close\r\n\r\np=%s&did=%s";
	char *miii_UST_is_template = "did=%s&brand=D-Link&miiicasa_enabled=1&wifi_secure_enabled=1&mac=%s&model=DIR-657&ws_ip=%s&ws_port=80&external_ip=%s&external_port=5555&thumbnail_supported=0&authorized=0";

        system("echo 1 > /var/tmp/miiicasa_pre_key_tmp");
	if(miiicasa_UST_server)
	        host = gethostbyname(miiicasa_UST_server);
	else
	        host = gethostbyname(fw_host);
	if( host == NULL )
	{
		perror("miiicasa_UST:gethostbyname()");
		return -1;
	}
	
	memcpy(&saddr.s_addr, host->h_addr_list[0], 4);
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = saddr.s_addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("fw_query:socket()");
		close(fd);
		return -1;
	}

	DEBUG(LOG_INFO, "miiicasa_UST:connect()...\n");
	if (connect(fd, &addr, sizeof(addr)) < 0)
	{
		perror("miiicasa_UST:connect()");
		close(fd);
		return -1;
	}

	nvram_safe_get("lan_ipaddr", lan_ip, sizeof(lan_ip));
	DEBUG(LOG_INFO, "xxxxxxxxxxxx lan_ip=%s xxxxxxxxxxxxxxxxxxx\n",lan_ip);
	cmd_nvram_get("wan_eth",eth);
	get_ip_addr(eth,wan_ipaddr);

        if((((strlen(miiicasa_did)) == 1)&&(miiicasa_did[0] == '0'))||((strlen(miiicasa_did)) == 0))
                miiicasa_did_is = 0;
        else
        {
                miiicasa_did_is = 1;
                return -1;
        }
	tmp_buf = malloc(1024);
	memset(tmp_buf, 0, 1024);
	memset(miii_UST_tmp, 0, 512);
	p2 = special_str_replace(cmd_nvram_get("wan_mac",wan_mac));
	if(miiicasa_did_is == 0)
	        sprintf(tmp_buf, miii_UST_template, p2, lan_ip, wan_ipaddr);
	else
	        sprintf(tmp_buf, miii_UST_is_template, miiicasa_did, p2, lan_ip, wan_ipaddr);
	if(p2) free(p2);
	DEBUG(LOG_INFO, "d:%s\n",tmp_buf);
	p2 = special_str_replace(md5_encrypt(tmp_buf, miiicasa_pre_key, 16));
	strcpy(miii_UST_tmp, p2);
	DEBUG(LOG_INFO, "e:%s\n",p2);
	//strcpy(miii_UST_tmp, "ABYn4la0yU2c2nnxKWldABk90LGUMhX2iN4Tq%2FjZhVyeXvQwhHC7Wf3Cc9UlNwcXf2i31SAUgJTwaTOukuM7LMuO%2Byi1guFnS65TQbvnELZ8rBlb2EkkEvVs%2FLlIOwiNczrhnb5T94F3pJXbL%2BuFyDhV8zcuUN64f7F2xmyCRUA8SlugHwfYRyvk7sPpmZoxv0vbi72VnUi5f4U8DJQcQU6bQhe5c67yfoNiEQUda2nkYq0OBTEUBk%2FOPBqS56nCZOJYkF4zyPMvX6ttCZZuzU4N3N%2FIdkD5YhpCu3pn%2Fns%3D");
	if(p2) free(p2);
	//strcpy(miii_UST_tmp, tmp_buf);

        if(miiicasa_did_is == 0)
        {
                if(miiicasa_UST_server)
	                sprintf(send_buf, miii_UST_H_template,
	                        miiicasa_UST_server, strlen(miii_UST_tmp)+2,miii_UST_tmp);
                else
	                sprintf(send_buf, miii_UST_H_template,
	                        "w1.partner.staging.miiicasa.com", strlen(miii_UST_tmp)+2,miii_UST_tmp);
	}
	else
        {
                if(miiicasa_UST_server)
	                sprintf(send_buf, miii_UST_is_H_template,
	                        miiicasa_UST_server, strlen(miii_UST_tmp)+2+5+strlen(miiicasa_did),miii_UST_tmp,miiicasa_did);
                else
	                sprintf(send_buf, miii_UST_is_H_template,
	                        "w1.partner.staging.miiicasa.com", strlen(miii_UST_tmp)+2+5+strlen(miiicasa_did),miii_UST_tmp,miiicasa_did);
	}

	DEBUG(LOG_INFO, "miiicasa_UST:send_buf =\n%s\n", send_buf);

	if( send(fd, send_buf, strlen(send_buf), 0 ) < 0)
	{
		perror("miiicasa_UST:send()");
		close(fd);
		return -1;
	}

	memset(recv_buf, 0, sizeof(recv_buf));
	DEBUG(LOG_INFO, "miiicasa_UST:recv()...\n");

	FD_ZERO(&afdset);
	FD_SET(fd, &afdset);

	timeval.tv_sec = 3;
	timeval.tv_usec = 0;

	/* timeout error */
	ret = select(fd + 1, &afdset, NULL, NULL, &timeval);
	if(ret == -1)
	{
		perror("miiicasa_UST:select()");
		close(fd);
		return -1;
	}

	/* receive fail */
	if(FD_ISSET(fd, &afdset))
		read(fd, recv_buf, sizeof(recv_buf));
	else
	{
		DEBUG(LOG_INFO, "miiicasa_UST: recv_buf = NULL\n");
		return -1;
	}

	DEBUG(LOG_INFO, "miiicasa_UST:recv_buf size = %d\n", strlen(recv_buf));
	DEBUG(LOG_INFO, "miiicasa_UST:recv_buf = \n%s\n", recv_buf);

	char *UST_t=NULL, tmp_buf1[1024], tmp_buf2[1024], *tmp_buf3, tmp_buf4[200];
	ret = 0;
	DEBUG(LOG_INFO, "UST111\n");
	UST_t = strstr(recv_buf, "status");
	if(UST_t != NULL)
	{
	        DEBUG(LOG_INFO, "UST:status\n");
	        UST_t = strstr(recv_buf, "ok");
		if(UST_t != NULL)
		{
		        DEBUG(LOG_INFO, "UST:ok\n");
			UST_t = strstr(UST_t, "p");
			if(UST_t != NULL)
			{
			        UST_t = strstr(UST_t, ":");
			        if(UST_t != NULL)
			        {
			                memset(tmp_buf1, 0, 1024);
			                strcpy(tmp_buf1, UST_t + 2);
			                tmp_buf1[strlen(tmp_buf1) - 1] = '\0';
			                tmp_buf1[strlen(tmp_buf1) - 1] = '\0';
			                DEBUG(LOG_INFO, "UST:%s\n",tmp_buf1);
			                tmp_buf3 = special_str_remove(tmp_buf1);
			                if(tmp_buf3 != NULL)
			                {
			                        memset(tmp_buf2, 0, 1024);
			                        strcpy(tmp_buf2, md5_decrypt(tmp_buf3, miiicasa_pre_key, 16));
				                DEBUG(LOG_INFO, "UST:%s\n",tmp_buf2);
				                free(tmp_buf3);
				                memset(tmp_buf1, 0, 1024);
				                UST_t = strstr(tmp_buf2, "did=");
				                if(UST_t != NULL)
				                {
				                        tmp_buf3 = strstr(UST_t, "&");
				                        if((tmp_buf3 != NULL) && ((tmp_buf3 - UST_t) > 0))
				                        {
				                                ret = 1;
				                                strncpy(tmp_buf1, UST_t + 4, tmp_buf3 - UST_t - 4);
				                                DEBUG(LOG_INFO, "UST did:%s\n",tmp_buf1);
				                                memset(tmp_buf4, 0, 200);
				                                sprintf(tmp_buf4, "nvram set miiicasa_did=%s", tmp_buf1);
				                                system(tmp_buf4);
				                                tmp_buf3 = strstr(tmp_buf3, "key=");
				                                if(tmp_buf3 != NULL)
				                                {
				                                        memset(tmp_buf1, 0, 1024);
				                                        strncpy(tmp_buf1, tmp_buf3 + 4, 32);
				                                        DEBUG(LOG_INFO, "UST key:%s\n",tmp_buf1);				                                
				                                        memset(tmp_buf4, 0, 200);
				                                        sprintf(tmp_buf4, "nvram set miiicasa_pre_key=%s", tmp_buf1);
				                                        system(tmp_buf4);
				                                        memset(tmp_buf4, 0, 200);
				                                        sprintf(tmp_buf4, "echo %s > /var/tmp/miiicasa_pre_key", tmp_buf1);
				                                        system(tmp_buf4);
				                                        memset(miiicasa_pre_key, 0, sizeof(miiicasa_pre_key));
				                                        strcpy(miiicasa_pre_key, tmp_buf1);
				                                        //memset(tmp_buf4, 0, 200);
				                                        //sprintf(tmp_buf4, "cp -rf /var/tmp/miiicasa_pre_key /etc/miiicasa_pre_key");
				                                        //system(tmp_buf4);
				                                }
				                                memset(tmp_buf4, 0, 200);
				                                sprintf(tmp_buf4, "nvram commit");
				                                system(tmp_buf4);
				                        }
				                }
				        }
				}
			}
		}
	}

        if(ret == 0)
        {
                memset(tmp_buf4, 0, 200);
                sprintf(tmp_buf4, "echo %s > /var/tmp/miiicasa_pre_key", miiicasa_pre_key);
                system(tmp_buf4);
	}
	
	if( shutdown(fd, 2) < 0)
	{
		perror("miiicasa_UST:shutdown()");
		return -1;
	}

	DEBUG(LOG_INFO, "Parsing recv_buf...\n");

        return 0;
}
#endif

int main(int argc, char **argv)
{
	int i, proxy_port = 32800, proxy_fd, log_fd;
	char *testurl = NULL;
	int pid;

	char *c_argv[20];
	int c_argc = 0;

	socklen_t n;
	struct sockaddr_in proxy_addr, client_addr;
	struct thread_data td;
	struct thread_data c_td;

#ifdef MIII_BAR
        FILE *fp;
	int m=0;
	char *str_end=NULL;
	
	memset(miiicasa_enabled, 0, 4);
	memset(miiicasa_server, 0, 50);
	memset(miiicasa_server_js, 0, 80);
	memset(miiicasa_UST_server, 0, 80);
	memset(miiicasa_did, 0, 80);
	memset(miiicasa_pre_key, 0, 80);
	memset(miiicasa_prefix, 0, 30);
	memset(miiicasa_forward_server, 0, 80);
	
	cmd_nvram_get("miiicasa_enabled", miiicasa_enabled);
	DEBUG(LOG_INFO, "miiicasa_enabled:%c\n",miiicasa_enabled[0]);
	if((miiicasa_enabled != NULL) && (miiicasa_enabled[0] == '1'))
	{
		cmd_nvram_get("miiicasa_did", miiicasa_did);
		str_end = miiicasa_did;
		for(m=0; m<80; m++)
		{
			if(*(str_end+m) == 0xa) 
			{
				*(str_end+m) = 0;
				break;
			}
		}
#if 0
		cmd_nvram_get("miiicasa_server", miiicasa_server);
		str_end = miiicasa_server;
		for(m=0; m<50; m++)
		{
			if(*(str_end+m) == 0xa) 
			{
				*(str_end+m) = 0;
				break;
			}
		}
		cmd_nvram_get("miiicasa_server_js", miiicasa_server_js);
		str_end = miiicasa_server_js;
		for(m=0; m<80; m++)
		{
			if(*(str_end+m) == 0xa) 
			{
				*(str_end+m) = 0;
				break;
			}
		}
		cmd_nvram_get("miiicasa_UST_server", miiicasa_UST_server);
		str_end = miiicasa_UST_server;
		for(m=0; m<80; m++)
		{
			if(*(str_end+m) == 0xa) 
			{
				*(str_end+m) = 0;
				break;
			}
		}
		cmd_nvram_get("miiicasa_prefix", miiicasa_prefix);
		str_end = miiicasa_prefix;
		for(m=0; m<80; m++)
		{
			if(*(str_end+m) == 0xa) 
			{
				*(str_end+m) = 0;
				break;
			}
		}
		if(strlen(miiicasa_prefix)==0)
		        strcpy(miiicasa_prefix, "/miiicasa_bar_api/");
		cmd_nvram_get("miiicasa_forward_server", miiicasa_forward_server);
		str_end = miiicasa_forward_server;
		for(m=0; m<80; m++)
		{
			if(*(str_end+m) == 0xa) 
			{
				*(str_end+m) = 0;
				break;
			}
		}
		if(strlen(miiicasa_forward_server)==0)
		        strcpy(miiicasa_forward_server, "http://a.mimgs.com/bar/api/");
#else
                strcpy(miiicasa_server, "img1.partner.staging.miiicasa.com");
                strcpy(miiicasa_server_js, "http://img1.partner.staging.miiicasa.com/i/toolbar.js");
                strcpy(miiicasa_UST_server, "w1.partner.staging.miiicasa.com");
                strcpy(miiicasa_prefix, "/miiicasa_bar_api/");
                strcpy(miiicasa_forward_server, "http://a.mimgs.com/bar/api/");
#endif
		fp = fopen("/etc/miiicasa_pre_key", "r");
		if(fp != NULL)
		{
		        fscanf(fp,"%s",&miiicasa_pre_key);
		        fclose(fp);
		}
		else
		{
		        cmd_nvram_get("miiicasa_pre_key", miiicasa_pre_key);
		        str_end = miiicasa_pre_key;
		        for(m=0; m<80; m++)
		        {
			        if(*(str_end+m) == 0xa) 
			        {
				        *(str_end+m) = 0;
				        break;
			        }
		        }
		}
		fp = fopen("/var/tmp/miiicasa_pre_key_tmp", "r");
		if(fp != NULL)
		        fclose(fp);
		else
		        miiicasa_UST_sned();
	}
#endif

	/*
	 * Prevent from zombie processes.
	 */
	signal(SIGCHLD,SIG_IGN);

	if (!strcmp(argv[argc-1], "child")) {
		DEBUG(LOG_INFO, "I am the child argc = %d", argc);
		parse_parameters(argc - 1, argv, &td, &proxy_port, &testurl);

		/* Close all file descriptors. */
		while (!close(n) && n++ < MAXIMUM_FILE_DESCRIPTORS);

		/* Create a socket. */
		proxy_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

		if (proxy_fd < 0){
			DEBUG(LOG_INFO, "Create proxy a socket fail\n");
			return 4;
		}

		log_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (log_fd < 0){
			DEBUG(LOG_INFO, "Create log a socket fail\n");
			return 4;
		}

		/* Bind the proxy on the local port and listen. */
#ifndef WIN32
		n = 1;

		if (setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&n, sizeof(n))< 0){
			DEBUG(LOG_INFO, "bind proxy a socket SO_REUSEADDR fail\n");
			return 5;
		}
		n = TCP_WINDOW_SIZE;

		if (setsockopt(proxy_fd, SOL_SOCKET, SO_SNDBUF, (void *)&n, sizeof(n)) < 0){
			DEBUG(LOG_INFO, "bind proxy a socket SO_SNDBUF fail\n");
			return 5;
		}

		if (setsockopt(proxy_fd, SOL_SOCKET, SO_RCVBUF, (void *)&n, sizeof(n)) < 0){
			DEBUG(LOG_INFO, "bind proxy a socket SO_RCVBUF fail\n");
			return 5;
		}
#endif
/*
		n = 1;
		if (setsockopt(log_fd, SOL_SOCKET, SO_BROADCAST, (void *)&n, sizeof(n))< 0){
			DEBUG(LOG_INFO, "bind log a socket SO_BROADCAST fail\n");
			return 5;
		}
		td.log_fd = log_fd;
*/
		td.auth_ip &= td.netmask;
		DEBUG(LOG_INFO, "proxy_port=%d,proxy.server_ip=%x\n",proxy_port,td.server_ip);
		memset(&proxy_addr, 0, sizeof(proxy_addr));
		proxy_addr.sin_family	   = AF_INET;
		proxy_addr.sin_port	   = htons((unsigned short)proxy_port);
		proxy_addr.sin_addr.s_addr = td.server_ip;

		if (bind(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
			DEBUG(LOG_INFO, "bind() fail:%s", strerror(errno));
			return 6;
		}

		if (listen(proxy_fd, _GLOBAL_MAX_CONNECTIONS)){
			DEBUG(LOG_INFO, "proxy_fd listen fail\n");
			return 7;
		}

		while (1)
		{
			n = sizeof(client_addr);

			/* Wait for an inbound connection. */

			if ((td.client_fd = accept(proxy_fd, (struct sockaddr *)&client_addr,
				 &n)) < 0) {
				DEBUG(LOG_INFO, "proxy_fd accept():%s", strerror(errno));
				return 8;
			}
			td.client_ip = client_addr.sin_addr.s_addr;
		
			/* Verify that the client is authorized. */

			if ((td.client_ip & td.netmask) != td.auth_ip)
			{
				DEBUG(LOG_INFO, "client_fd client_ip=%x fail close\n", td.client_ip);
				close(td.client_fd);
				continue;
			}
			DEBUG(LOG_INFO, "accept():ip=%x", td.client_ip);

			/* Fork a child process to handle the connection. */
			if ((pid = vfork()) < 0)
			{
				DEBUG(LOG_INFO, "client_fd fork fail close\n");
				close(td.client_fd);
				continue;
			}

			if (pid)
			{
				/* In parent thread: wait for child to finish. */
				DEBUG(LOG_INFO, "client_fd waitpid close\n");
				close(td.client_fd);
				//waitpid(pid, NULL, 0);
				continue;
			}

#if 0
			/* In child: fork and exit so that the parent thread becomes init. */
			if ((pid = vfork()) < 0){
				DEBUG(LOG_INFO, "In child: fork fail\n");
				return 9;
			}
			if (pid){
				DEBUG(LOG_INFO, "In child: pid parent pid = %d ########\n", pid);
				//return 0;
				_exit(0);
			}
#endif

			c_argc = argc - 1; // remove the "child" argument at the end of the arguments

			for (i = 0; i < c_argc; i++) {
				c_argv[i] = argv[i];
			}
			
			char client_fd[2];
			char log_fd[2];
			char server_ip[10];
			char auth_ip[10];
			char netmask[10];
			char client_ip[10];
			char connect[2];

			sprintf(client_fd, "%u", td.client_fd);
			sprintf(log_fd, "%u", td.log_fd);
			sprintf(server_ip, "%x", td.server_ip);
			sprintf(auth_ip, "%x", td.auth_ip);
			sprintf(netmask, "%x", td.netmask);
			sprintf(client_ip, "%x", td.client_ip);
			sprintf(connect, "%u", td.connect);

			c_argv[c_argc++] = client_fd;
			c_argv[c_argc++] = log_fd;
			c_argv[c_argc++] = server_ip;
			c_argv[c_argc++] = auth_ip;
			c_argv[c_argc++] = netmask;
			c_argv[c_argc++] = client_ip;
			c_argv[c_argc++] = connect;

			/*
			 * Invoke the subchild exec. We are passing a special argument to identify the subchild
			 */
			c_argv[c_argc++] = "subchild"; 
			c_argv[c_argc++] = NULL;

			execvp(c_argv[0], c_argv);
			_exit(1);

		} // while

		/* Not reached. */
		return -1;


	} else if (!strcmp(argv[argc-1], "subchild")) {		// subchild

		DEBUG(LOG_INFO, "I am the subchild argc = %d", argc);
		parse_parameters(argc - 8, argv, &td, &proxy_port, &testurl);

		argc -= 1;	// this is for skipping "subchild" string
		
		c_td.connect = atoi(argv[--argc]);
		c_td.client_ip = strtoul(argv[--argc], NULL, 16);
		c_td.netmask = strtoul(argv[--argc], NULL, 16);
		c_td.auth_ip = strtoul(argv[--argc], NULL, 16);
		c_td.server_ip = strtoul(argv[--argc], NULL, 16);
		c_td.log_fd = atoi(argv[--argc]);
		c_td.client_fd = atoi(argv[--argc]);

		DEBUG(LOG_INFO, "client_thread\n");
		_exit(client_thread(&c_td));


	} else {	// parent
		DEBUG(LOG_INFO, "I am the parent argc = %d", argc);
		openlog("urlblock", LOG_PID, LOG_USER);
		parse_parameters(argc, argv, &td, &proxy_port, &testurl);

		/* If inetd mode is enabled, use stdin. */
		if (!proxy_port)
		{
			DEBUG(LOG_INFO, "no proxy_port\n");
			td.client_fd = 0;
			return (client_thread(&td));
		}

		/*
		 * The parent vforks and execs a child creating
		 * a deamon.
		 */
		/* Fork into background. */
		if ((pid = vfork()) < 0)
		{
			DEBUG(LOG_INFO, "fork fail\n");
			return 2;
		}

		if (pid){
			DEBUG(LOG_INFO, "pid parent pid\n");
			//return 0;
			_exit(0);
		}

		/* Create a new session. */
		DEBUG(LOG_INFO, "child process: %d\n", pid);
		if (setsid() < 0){
			DEBUG(LOG_INFO, "setsid fail\n");
			return 3;
		}
		DEBUG(LOG_INFO, "setsid success\n");

		c_argc = argc;

		for (i = 0; i < c_argc; i++) {
			c_argv[i] = argv[i];
		}
	
		/*
		 * Invoke the child exec. We are passing a special argument to identify the child
		 */
		c_argv[c_argc++] = "child";
		c_argv[c_argc++] = NULL;

		execvp(c_argv[0], c_argv);
		_exit(1);
	}
}
#ifdef MIII_BAR
char *share_key_mac = "a5797fcce39d0e942841031a1c85ab8d";


char *get_rand_iv ( int iv_len )
{
    int i = 0;
    char *retval = (char*) malloc(iv_len+1);

    srandom((unsigned int)time(NULL));
    for (i=0; i<iv_len; i++)
    {
	retval[i] = (random() % 255)+1;
    }

    retval[i] = 0;

    return retval;
}


char *XOR (char* dest, int dest_len, char *value, int vlen, char *key, int klen)
{
    char *retval = dest;
    
    short unsigned int k = 0;
    short unsigned int v = 0;

    for (v; v<vlen && v<klen; v++ )
    {
	retval[v] = value[v] ^ key[k];
	k=(++k<vlen?k:0);
    }

    return retval;
}

char *substr (char *dest, int dest_len, char *string, int start, int length )
{
    char *retval = dest;

    memcpy(retval, string+start, dest_len);

    if ( dest_len <= length )
    {
        // dest[dest_len-1] = '\0';
        // printf("dest_len(%d) <= length(%d)\n", dest_len, length);
    }
    else
    {
        dest[length] = '\0';
    }

    return retval;
}

char *packH ( char *dest, int dest_len, char* string ) 
{
    char *result = dest;
    int i = 0;


    for (i=0; i<dest_len-1; i++)
    {
	char hex[16];
	int temp = 0;
	substr(hex, 16, string, i*2, 2);
	sscanf(hex, "%02X", &temp);
	result[i] = temp;
    }

    return result;
}

static unsigned char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64encode(const unsigned char *input, int input_length, unsigned char *output, int output_length)
{
	int	i = 0, j = 0;
	int	pad;

	//assert(output_length >= (input_length * 4 / 3));

	while (i < input_length) {
		pad = 3 - (input_length - i);
		if (pad == 2) {
			output[j  ] = basis_64[input[i]>>2];
			output[j+1] = basis_64[(input[i] & 0x03) << 4];
			output[j+2] = '=';
			output[j+3] = '=';
		} else if (pad == 1) {
			output[j  ] = basis_64[input[i]>>2];
			output[j+1] = basis_64[((input[i] & 0x03) << 4) | ((input[i+1] & 0xf0) >> 4)];
			output[j+2] = basis_64[(input[i+1] & 0x0f) << 2];
			output[j+3] = '=';
		} else{
			output[j  ] = basis_64[input[i]>>2];
			output[j+1] = basis_64[((input[i] & 0x03) << 4) | ((input[i+1] & 0xf0) >> 4)];
			output[j+2] = basis_64[((input[i+1] & 0x0f) << 2) | ((input[i+2] & 0xc0) >> 6)];
			output[j+3] = basis_64[input[i+2] & 0x3f];
		}
	        output[j+4] = '\0';
		i += 3;
		j += 4;
	}
	return j;
}

/* This assumes that an unsigned char is exactly 8 bits. Not portable code! :-) */
static unsigned char index_64[128] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   62, 0xff, 0xff, 0xff,   63,
      52,   53,   54,   55,   56,   57,   58,   59,   60,   61, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff,    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
      15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
      41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51, 0xff, 0xff, 0xff, 0xff, 0xff
};

#define char64(c)  ((c > 127) ? 0xff : index_64[(c)])

int base64decode(const unsigned char *input, int input_length, unsigned char *output, int output_length)
{
	int		i = 0, j = 0, pad;
	unsigned char	c[4];

	//printf("input: %s\n", input);

	if (!(output_length >= (input_length * 3 / 4)) || !((input_length % 4) == 0)) return -1;
	while ((i + 3) < input_length) {
		pad  = 0;
		c[0] = char64(input[i  ]); pad += (c[0] == 0xff);
		c[1] = char64(input[i+1]); pad += (c[1] == 0xff);
		c[2] = char64(input[i+2]); pad += (c[2] == 0xff);
		c[3] = char64(input[i+3]); pad += (c[3] == 0xff);
		if (pad == 2) {
			output[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			output[j]   = (c[1] & 0x0f) << 4;
		} else if (pad == 1) {
			output[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			output[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
			output[j]   = (c[2] & 0x03) << 6;
		} else {
			output[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			output[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
			output[j++] = ((c[2] & 0x03) << 6) | (c[3] & 0x3f);
			//printf("output[%d]: %c(%d)\n", j-3, output[j-3], output[j-3]);
			//printf("output[%d]: %c(%d)\n", j-2, output[j-2], output[j-2]);
			//printf("output[%d]: %c(%d)\n", j-1, output[j-1], output[j-1]);
		}
		i += 4;
		//printf("output: %s\n", output);
	}
	return j;
}

char *md5_encrypt ( char *plain_text, char *password, int iv_len )
{
    static char result[2048];
    int i = 0, n = 0;
    char iv[512], strEncText[2048], strXOR[512];
    char my_plain_text[2048];
    char *tmp = NULL;
    int strEncText_len = 0;
    int block_iv_len = 32;
    int old_iv_len = 16;

    memset(result, 0, sizeof(result));
    memset(strEncText, 0, sizeof(strEncText));
    memset(my_plain_text, 0, sizeof(my_plain_text));

    strcpy(my_plain_text, plain_text);

    n = strlen(my_plain_text);

    if ( n % 16 )
    {
	int i = 0;

	for (i=0; i< 16 - (n % 16); i++ )
	{
	    my_plain_text[n+i] = ' ';
	}
    }

    tmp = get_rand_iv(iv_len);
    memcpy(strEncText, tmp, iv_len);
    strEncText_len += iv_len;
    free(tmp);

    XOR(strXOR, 512, password, 16, strEncText, 16);
    substr(iv, sizeof(iv), strXOR, 0, 512);

    while ( i < n )
    {
        char block[1024], strSubStr[512], *strXOR = NULL, *strMD5_iv = NULL;
        char pack[64], *block_iv = NULL;
        memset(strSubStr, 0, sizeof(strSubStr));
        substr(strSubStr, 512, my_plain_text, i, 16);

        strMD5_iv = MDString(iv, old_iv_len);
        packH(pack, sizeof(pack), strMD5_iv);
        XOR(block, 16, strSubStr, 16, pack, 16);

        memcpy(strEncText+strEncText_len, block, 16);
	strEncText_len += 16;

        block_iv = (char*) malloc(16+32+1);
	memcpy(block_iv, block, 16);
	memcpy(block_iv+16, iv, old_iv_len);
	old_iv_len = 32;

        substr(strSubStr, 512, block_iv, 0, 512);
        XOR(iv, 512, strSubStr, 32, password, 32);

        i += 16;
    }

    base64encode(strEncText, strEncText_len, result, sizeof(result));

    return result;
}

char *md5_decrypt ( char *enc_text, char *password, int iv_len )
{
    static char my_plain_text[2048];
    int i = 0, n = 0;
    char iv[512], base64_dec_text[2048], strXOR[512];
    char base64_enc_substr[1024];
    char base64_enc_block[1024];
    char base64_enc_xor[1024];
    char base64_enc_iv[1024];
    char base64_enc_block_iv[1024];
    char base64_enc_pack[1024];
    char strSubStr[1024];;
    char *tmp = NULL;
    int block_iv_len = 32;
    int old_iv_len = 16;
    
    i = iv_len;
    n = base64decode(enc_text, strlen(enc_text), base64_dec_text, sizeof(base64_dec_text));
    memset(my_plain_text, 0, sizeof(my_plain_text));

    substr(strSubStr, sizeof(strSubStr), base64_dec_text, 0, iv_len);
    base64encode(strSubStr, iv_len, base64_enc_substr, sizeof(base64_enc_substr));

    XOR(strXOR, iv_len, password, 16, strSubStr, sizeof(strSubStr));
    base64encode(strXOR, iv_len, base64_enc_xor, sizeof(base64_enc_xor));

    substr(iv, sizeof(iv), strXOR, 0, 512);
    base64encode(iv, strlen(iv), base64_enc_iv, sizeof(base64_enc_iv));

    i = iv_len;

    while ( i < n )
    {
        char block[1024], strSubStr[512], strXOR[512], *strMD5_iv = NULL;
        char pack[512], block_iv[512];

        memset(strSubStr, 0, sizeof(strSubStr));
        substr(block, sizeof(block), base64_dec_text, i, iv_len);
        base64encode(block, iv_len, base64_enc_block, sizeof(base64_enc_block));

        strMD5_iv = MDString(iv, old_iv_len);
        base64encode(iv, old_iv_len, base64_enc_iv, sizeof(base64_enc_iv));
        old_iv_len = 32;

        packH(pack, sizeof(pack) , strMD5_iv);
        base64encode(pack, iv_len, base64_enc_pack, sizeof(base64_enc_pack));


        XOR(strXOR, sizeof(strXOR), block, sizeof(block), pack, sizeof(pack));
        base64encode(strXOR, iv_len, base64_enc_xor, sizeof(base64_enc_xor));

        strncat(my_plain_text, strXOR, strlen(my_plain_text)+strlen(strXOR)+1);

        memcpy(block_iv, block, iv_len);
        memcpy(block_iv+iv_len, iv, iv_len*2);
        base64encode(block_iv, block_iv_len, base64_enc_block_iv, sizeof(base64_enc_block_iv));
        block_iv_len = 48;

        substr(strSubStr, 32, block_iv, 0, 512);
        base64encode(strSubStr, 32, base64_enc_substr, sizeof(base64_enc_substr));
        memset(iv, 0, sizeof(iv));
        XOR(iv, 32, strSubStr, sizeof(strSubStr), password, 32);
        base64encode(iv, old_iv_len, base64_enc_iv, sizeof(base64_enc_iv));

        i += 16;
    }

    return my_plain_text;
}


void mac_encrypt ( char *mac )
{
	//static char result[128];
	  DEBUG(LOG_INFO, "mac_encrypt_1 mac=%s\n",mac);	
    //char *md5_encrypted_mac = md5_encrypt(mac, share_key_mac, 16);
    strcpy(mac,md5_encrypt(mac, share_key_mac, 16));
  DEBUG(LOG_INFO, "mac_encrypt_2 mac=%s\n",mac);	
   // printf("\nmac: %s\n", mac);
    //printf("md5_encrypt(%d): %s\n", strlen(md5_encrypted_mac), md5_encrypted_mac);
    //DEBUG(LOG_INFO, "mac_encrypt_3 md5_encrypt(%d): %s\n", strlen(md5_encrypted_mac), md5_encrypted_mac);	
    //return result;
}


#endif
