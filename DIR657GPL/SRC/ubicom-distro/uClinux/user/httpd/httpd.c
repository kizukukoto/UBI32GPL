#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <error.h>
#include <sys/signal.h>
#include <search.h>
#include <assert.h>
#include <sys/time.h>
#include <httpd.h>
//#include <cmoapi.h>
#include <nvram.h>
#include "sutil.h"
#include "httpd_util.h"
#include <linux/sockios.h>

#ifndef _LINUX_IF_H
# include <net/if.h>
/* # define _LINUX_IF_H */
#endif

#ifndef _LINUX_IF_H
#define _LINUX_IF_H
#endif

#define MAXSOCKFD 10

#ifdef PURE_NETWORK_ENABLE
#include <pure.h>
#endif

/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
#include "lp.h"
#endif

#ifdef AJAX
#include <ajax.h>
#endif

/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
#include "authgraph.h"
#endif

#ifdef MIII_BAR
#include "project.h"
char miii_usb_folder[128];
char miii_usb_folder_t[128];
#endif

//#define HTTPD_DEBUG 1
#ifdef HTTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define SERVER_NAME "httpd"
#define SERVER_PORT 80
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define THREE_MINUTES 36

/* A multi-family sockaddr. */
typedef union {
        struct sockaddr sa;
        struct sockaddr_in sa_in;
} usockaddr;


#ifdef IPv6_SUPPORT

#define IPV6_V6ONLY             26

typedef union {
        struct sockaddr sa;
        struct sockaddr_in6 sa_in6;
        struct sockaddr_storage sa_stor;
} usockaddr6;

static usockaddr6 usa6;
int ipv6_request = 0;

#if defined(HAVE_HTTPS)
static usockaddr6 usa6_https;
#endif
#endif

/* Globals. */
static FILE *conn_fp;
static int conn_fd;
static usockaddr usa;

int change_admin_password_flag = 0;

#if defined(HAVE_HTTPS)
#define DEFAULT_HTTPS_PORT      443
int openssl_request = 0;
static usockaddr usa_https;

static SSL *ssl;
#define CERT_FILE       "/www/caCert.pem"
#define KEY_FILE        "/www/caKey.pem"
#endif

char admin_userid[AUTH_MAX];
char admin_passwd[AUTH_MAX];
char user_userid[AUTH_MAX];
char user_passwd[AUTH_MAX];

static char filepath[1024] = {0};
static char filename[1024] = {0};

/*
*       Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
char sp_admin_userid[AUTH_MAX];
char sp_admin_passwd[AUTH_MAX];
#endif
static char auth_realm[AUTH_MAX];
int auth_count = 0;

#if defined(CONFIG_WEB_404_REDIRECT) || defined(USB_STORAGE_HTTP_ENABLE)
int enable_404_redirect=0;
int lan_request=0;
char *device_name;
char lan_ip[17];
struct in_addr lan_subnet;

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

int get_netmask(char *if_name, struct in_addr *netmask)
{
        int sockfd;
        struct ifreq ifr;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return -1;

        strcpy(ifr.ifr_name, if_name);
        if( ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
                close(sockfd);
                return -1;
         }

        close(sockfd);

        netmask->s_addr = sin_addr(&ifr.ifr_netmask).s_addr;
        return 0;
}
#endif

#ifdef HTTPD_USED_MUTIL_AUTH
extern int auth_login_find(char *mac_addr);
extern int auth_login_search(void);
#endif

extern int internal_init(void);
/* Forwards. */
/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
static int initialize_listen_socket( usockaddr* usaP, unsigned short server_port );
static int auth_check( char* dirname, char* authorization );
//static void send_authenticate( char* realm ); // ==> seems no one use this function anymore
static void send_error( int status, char* title, char* extra_header, char* text );
static void send_headers( int status, char* title, char* extra_header, char* mime_type );

static int match( const char* pattern, const char* string );
static int match_one( const char* pattern, int patternlen, const char* string );
static void handle_request(void);
static void save_connect_info(int conn_fd_t, struct in_addr addr);
#ifdef IPv6_SUPPORT
static void save_connect6_info(int conn_fd_t, struct in6_addr addr6);
#endif
void logout(void);
static void init_login_list(void);

/* varible for logout */
static char mac_list[MAX_TMP] = {};
/* define for limit_ip_connect */
static int limit_connect_init_flag = 0;
static char default_limit_ip[50] = {};
static char default_limit_enable[2] = {};
static int limit_ip_connect(struct in_addr addr);

#ifdef PURE_NETWORK_ENABLE
int pure_mode_enable = 0;
#endif

#define LISTENQ 10

/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
static int initialize_listen_socket( usockaddr* usaP, unsigned short server_port ){
        int listen_fd;
        int on=1;

        memset( usaP, 0, sizeof(usockaddr) );

        usaP->sa.sa_family = AF_INET;
        usaP->sa_in.sin_addr.s_addr = htonl( INADDR_ANY );
        usaP->sa_in.sin_port = htons( server_port );

        listen_fd = socket( usaP->sa.sa_family, SOCK_STREAM, 0 );
        if ( listen_fd < 0 )
        {
                perror( "socket" );
                return -1;
        }
        (void) fcntl( listen_fd, F_SETFD, 1 );

        if ( setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0 )
        {
                perror( "setsockopt" );
                return -1;
        }
        if ( bind( listen_fd, &usaP->sa, sizeof(struct sockaddr_in) ) < 0 )
        {
                perror( "bind" );
                        return -1;
                }
        if ( listen( listen_fd, LISTENQ ) < 0 )
                {
                perror( "listen" );
                        return -1;
                }
        return listen_fd;
        }

/*      Date:   2010-01-25
 *      Name:   Jery Lin
 *      Reason: enable httpd support ipv6
 */
#ifdef IPv6_SUPPORT
static int initialize_listen6_socket( usockaddr6* usa6P, unsigned short server_port ){
        int listen6_fd;
        int on=1;

        memset( usa6P, 0, sizeof(usockaddr6) );

        usa6P->sa_in6.sin6_family = AF_INET6;
        usa6P->sa_in6.sin6_addr = in6addr_any;
        usa6P->sa_in6.sin6_port = htons( server_port );

        listen6_fd = socket( AF_INET6, SOCK_STREAM, 0);
        if ( listen6_fd < 0 )
        {
                perror( "socket6" );
                return -1;
        }
        (void) fcntl( listen6_fd, F_SETFD, 1 );

        if ( setsockopt( listen6_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0 )
        {
                perror( "setsockopt" );
                return -1;
        }

        if ( setsockopt( listen6_fd, SOL_IPV6, IPV6_V6ONLY, &on, sizeof(on) ) < 0 )
        {
                perror( "setsockopt" );
                return -1;
        }

        if ( bind( listen6_fd, &usa6P->sa_in6, sizeof(usa6P->sa_in6) ) < 0 )
        {
                perror( "bind" );
                return -1;
        }
        if ( listen( listen6_fd, LISTENQ ) < 0 )
        {
                perror( "listen" );
                return -1;
        }
        return listen6_fd;
}
#endif

struct loginList_t
{
    unsigned char mac_t[18];
    int pag;
    int flg;
};

struct loginList_t loginList[MAX_TMP];

void init_login_list(void)
{
        int i;
        for (i = 0; i < MAX_TMP; i++) {
           memset(loginList[i].mac_t, 0, 18);
           loginList[i].pag = 0;
           loginList[i].flg = 0;
        }
        DEBUG_MSG("init login finished \n");
        return;

}

/* implement logout function by mac */
int update_logout_list(char *mac, int type, int c)
{
        struct loginList_t *find;
        int i, findMac = 0;
        static int alloc_ptr = 0;

        for (i=0; i < MAX_TMP; i++) {
                find = &loginList[i];
                if ((find->mac_t[2] == ':') && (memcmp(find->mac_t, mac, 17) == 0)) {
                        DEBUG_MSG("loginList[%d].mac_t = %s\n", i, loginList[i].mac_t);
                        findMac = 1;
                        break;
                }
        }

        switch (type)
        {
        case ADD_MAC:
                DEBUG_MSG("ADD_MAC, findMac = %d\n", findMac);
                if (findMac == 0) {
                        find = &loginList[alloc_ptr];
                        memcpy(find->mac_t, mac, 17);

                        if (++alloc_ptr == MAX_TMP)
                                alloc_ptr = 0;
                }
                break;
        case GET_PAG:
                DEBUG_MSG("GET_PAG, findMac = %d\n", findMac);
                if (findMac) {
                        DEBUG_MSG("GET_PAG = %d\n", find->pag);
                        return find->pag;
                }
                break;
        case GET_FLG:
                DEBUG_MSG("GET_FLG, findMac = %d\n", findMac);
                if (findMac) {
                        DEBUG_MSG("GET_FLG = %d\n", find->flg);
                        return find->flg;
                }
                break;
        case SET_PAG:
                DEBUG_MSG("SET_PAG, findMac = %d\n", findMac);
                if (findMac) {
                        DEBUG_MSG("SET_PAG = %d\n", c);
                        find->pag = c;
                }
                break;
        case SET_FLG:
                DEBUG_MSG("SET_FLG, findMac = %d\n", findMac);
                if (findMac) {
                        DEBUG_MSG("SET_FLG %d\n", c);
                        find->flg = c;
                }
                break;
        case DEL_MAC:
                DEBUG_MSG("DEL_MAC, findMac = %d\n", findMac);
                if (findMac) {
                        memset(find->mac_t, 0, 17);
                        find->pag = 0;
                        find->flg = 0;
                }
                break;
        }

        return -1;
}

#ifdef PURE_NETWORK_ENABLE
static int auth_check( char* dirname, char* authorization ){
        char authinfo[500];
        char* authpass;
        int l,index;
        DEBUG_MSG("PURE MODE auth_check:dirname=%s,authorization=%s\n",dirname,authorization);

        /* Basic authorization info? */
        if ( !authorization || strncmp( authorization, "Basic ", 6 ) != 0 ) {
                DEBUG_MSG("auth_check fail. (1)\n");
                pure_authorization_error();
                return 0;
        }

        /* Decode it. */
        l = b64_decode( &(authorization[6]), authinfo, sizeof(authinfo) );
        authinfo[l] = '\0';

        /* Split into user and password. */
        authpass = strchr( authinfo, ':' );
        if ( authpass == (char*) 0 ) {
                /* No colon?  Bogus auth info. */
                DEBUG_MSG("auth_check fail. (2)\n");
                pure_authorization_error();
                return 0;
        }
        *authpass++ = '\0';

        //DEBUG_MSG("auth_check:authpass=%s,admin_passwd_tmp=%s\n",authpass,admin_passwd_tmp);
        /* Is this the right user and password? */
        /* Allow admin only */
        if ( (strcmp( admin_userid, authinfo ) == 0 && strcmp( admin_passwd, authpass ) == 0) ){
#ifdef HTTPD_USED_MUTIL_AUTH
                                DEBUG_MSG("auth_check: MUTIL_AUTH admin_userid\n");
                                if( (index = auth_login_find(&con_info.mac_addr[0])) < 0 ){
                                        if( (index = auth_login_search()) < 0){
                                                index=auth_login_search_long();
                                        }
                                }
                                DEBUG_MSG("auth_check: MUTIL_AUTH index=%d\n",index);
                                strcpy(auth_login[index].curr_user,admin_userid);
                                strcpy(auth_login[index].curr_passwd,admin_passwd);
                                strcpy(auth_login[index].mac_addr,con_info.mac_addr);
                                auth_login[index].logintime = time((time_t *)NULL);
                                DEBUG_MSG("auth_check: MUTIL_AUTH curr_user=%s,curr_passwd=%s,mac_addr=%s,time=%d\n",
                                                auth_login[index].curr_user,auth_login[index].curr_passwd,auth_login[index].mac_addr,auth_login[index].logintime);
#else
                                auth_login.curr_user = admin_userid;
                                auth_login.curr_passwd = admin_passwd;
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
                                update_logout_list(&con_info.mac_addr[0],ADD_MAC,NULL_LIST);
                                DEBUG_MSG("auth_check pass.\n\n");
                                return 1;
                }
#if 0
                else{
#ifdef HTTPD_USED_MUTIL_AUTH
                                DEBUG_MSG("auth_check: MUTIL_AUTH user_userid\n");
                                if( (index = auth_login_find(&con_info.mac_addr[0])) < 0 ){
                                        if( (index = auth_login_search()) < 0){
                                                index=auth_login_search_long();
                                        }
                                }
                                DEBUG_MSG("auth_check: MUTIL_AUTH index=%d\n",index);
                                strcpy(auth_login[index].curr_user,user_userid);
                                strcpy(auth_login[index].curr_passwd,user_passwd);
                                strcpy(auth_login[index].mac_addr,con_info.mac_addr);
                                auth_login[index].logintime = time((time_t *)NULL);
                                DEBUG_MSG("auth_check: MUTIL_AUTH curr_user=%s,curr_passwd=%s,mac_addr=%s,time=%d\n",
                                                auth_login[index].curr_user,auth_login[index].curr_passwd,auth_login[index].mac_addr,auth_login[index].logintime);
#else
                                auth_login.curr_user = user_userid;
                                auth_login.curr_passwd = user_passwd;
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
                /* update for login mac */
                update_logout_list(&con_info.mac_addr[0],ADD_MAC,NULL_LIST);
                DEBUG_MSG("auth_check pass.\n\n");
                return 1;
        }
#endif
        DEBUG_MSG("auth_check fail. (3)\n");
        pure_authorization_error();

        return 0;
}
#endif

static void send_authenticate( char* realm ){
        char header[10000];

        (void) snprintf(
                        header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", realm );
        send_error( 401, "Unauthorized", header, "Authorization required." );
}


static void send_error( int status, char* title, char* extra_header, char* text )
{
        send_headers( status, title, extra_header, "text/html" );
/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
        (void) wfprintf( conn_fp, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\"><H4>%d %s</H4>\n", status, title, status, title );
        (void) wfprintf( conn_fp, "%s\n", text );
        (void) wfprintf( conn_fp, "</BODY></HTML>\n" );
        (void) wfflush( conn_fp );
}

static void send_headers( int status, char* title, char* extra_header, char* mime_type ){
        time_t now;
        char timebuf[100];
/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
        (void) wfprintf( conn_fp, "%s %d %s\r\n", PROTOCOL, status, title );
        (void) wfprintf( conn_fp, "Server: %s\r\n", SERVER_NAME );
        now = time( (time_t*) 0 );
        (void) strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &now ) );
        (void) wfprintf( conn_fp, "Date: %s\r\n", timebuf );

        if ( extra_header != (char*) 0 )
                (void) wfprintf( conn_fp, "%s\r\n", extra_header );

        if ( mime_type != (char*) 0 )
                (void) wfprintf( conn_fp, "Content-Type: %s\r\n", mime_type );

        (void) wfprintf( conn_fp, "Connection: close\r\n" );
        (void) wfprintf( conn_fp, "\r\n" );
}

#ifdef MIII_BAR
static void miiicasa_replace_special_char(char *sourcechar)
{
        char value_tmp[1024];
        char value_tmp_amp[1024];	    
        char value_tmp_lt[1024];	
    
        memset(value_tmp, 0, 1024);
        memset(value_tmp_amp, 0, 1024);
        memset(value_tmp_lt, 0, 1024);								

	sprintf(value_tmp,"%s",sourcechar);
	sprintf(value_tmp_amp,"%s",substitute_keyword(value_tmp,"\(","\\("));		
	sprintf(value_tmp_lt,"%s",substitute_keyword(value_tmp_amp,"\)","\\)"));	
	sprintf(sourcechar,"%s",value_tmp_lt);
}

int miiicasa_set_folder()
{
	char filesystem[256]={0}, temp[256]={0}, *tmp_p=NULL, *tmp_end_p=NULL;
	FILE *fp=NULL;
	int i=0, miii_usb_found=-1;
	
	sprintf(filesystem, "df -k | grep %s > /var/miiicasa_mount.txt",USBMOUNT_FOLDER);
	system(filesystem);
	
	fp = fopen("/var/miiicasa_mount.txt","r");
	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/miiicasa_mount.txt");		
		return -1;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{
			tmp_p = temp;
			if((tmp_p = strstr(tmp_p, USBMOUNT_FOLDER)) != NULL)
			{
				DEBUG_MSG("miiicasa: usb storage - %s\n",tmp_p);
				tmp_end_p = tmp_p;
				for(i=0;i<100;i++)
				{
					if(*tmp_end_p == 0xa) 
					{
						//*tmp_end_p = "\\";
						break;
					}
					tmp_end_p ++;
				}
				memset(miii_usb_folder, 0, sizeof(miii_usb_folder));
				memset(miii_usb_folder_t, 0, sizeof(miii_usb_folder));
				memcpy(miii_usb_folder, tmp_p, tmp_end_p - tmp_p);
				memcpy(miii_usb_folder_t, tmp_p, tmp_end_p - tmp_p);
				miiicasa_replace_special_char(miii_usb_folder);
				miii_usb_found = 1;
				break;
			}
		}
		fclose(fp);
		return miii_usb_found;
	}
}

int miiicasa_check_file(char *file_name)
{
	FILE *fp=NULL;
	fp = fopen(file_name,"r");
	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,file_name);		
		return -1;
	}else{
		fclose(fp);
		return 1;
	}
}

static void miii_send_headers( int status, char* title, char* mime_Disposition, char* extra_header,int mime_Length, char* mime_type,char* mime_transfer,int flag ){
        time_t now;
        char timebuf[128];
/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
        (void) wfprintf( conn_fp, "%s %d %s\r\n", PROTOCOL, status, title );
        (void) wfprintf( conn_fp, "Server: %s\r\n", SERVER_NAME );
        now = time( (time_t*) 0 );
        (void) strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &now ) );
        (void) wfprintf( conn_fp, "Date: %s\r\n", timebuf );

        if ( extra_header != (char*) 0 )
                (void) wfprintf( conn_fp, "%s\r\n", extra_header );
      //if strcmp(flag,"1")
      
        printf("miii flag =%d\n",flag);
      	if(flag == 1){

        	if ( mime_transfer != (char*) 0 )
                	(void) wfprintf( conn_fp, "%s\r\n", mime_transfer );     
             
        	if ( mime_Disposition != (char*) 0 )        
			(void) wfprintf( conn_fp, "Content-Disposition: %s\r\n", mime_Disposition );

        	if ( mime_type != (char*) 0 )
                	(void) wfprintf( conn_fp, "Content-Type: %s\r\n", mime_type );
	}else{

       		if ( mime_Disposition != (char*) 0 )        
			(void) wfprintf( conn_fp, "content-Disposition: %s\r\n", mime_Disposition );
				
		if ( mime_Length != (int*) 0 )        
			(void) wfprintf( conn_fp, "content-Length: %d\r\n", mime_Length );
								
       	 	if ( mime_type != (char*) 0 )
                	(void) wfprintf( conn_fp, "Content-Type: %s\r\n", mime_type );
         	(void) wfprintf( conn_fp, "Connection: close\r\n" );
 
      }
               
      (void) wfprintf( conn_fp, "\r\n" );
}
#endif

/*
 * base64 encoder
 *
 * encode 3 8-bit binary bytes as 4 '6-bit' characters
 */
char *b64_encode( unsigned char *src ,int src_len, unsigned char* space, int space_len){
        static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        unsigned char *out = space;
        unsigned char *in=src;
        int sub_len,len;
        int out_len;

        out_len=0;

        if (src_len < 1 ) return NULL;
        if (!src) return NULL;
        if (!space) return NULL;
        if (space_len < 1) return NULL;


        /* Required space is 4/3 source length  plus one for NULL terminator*/
        if ( space_len < ((1 +src_len/3) * 4 + 1) )return NULL;

        memset(space,0,space_len);

        for (len=0;len < src_len;in=in+3, len=len+3)
        {

                sub_len = ( ( len + 3  < src_len ) ? 3: src_len - len);

                /* This is a little inefficient on space but covers ALL the
                   corner cases as far as length goes */
                switch(sub_len)
                {
                        case 3:
                                out[out_len++] = cb64[ in[0] >> 2 ];
                                out[out_len++] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ] ;
                                out[out_len++] = cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] ;
                                out[out_len++] = cb64[ in[2] & 0x3f ];
                                break;
                        case 2:
                                out[out_len++] = cb64[ in[0] >> 2 ];
                                out[out_len++] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ] ;
                                out[out_len++] = cb64[ ((in[1] & 0x0f) << 2) ];
                                out[out_len++] = (unsigned char) '=';
                                break;
                        case 1:
                                out[out_len++] = cb64[ in[0] >> 2 ];
                                out[out_len++] = cb64[ ((in[0] & 0x03) << 4)  ] ;
                                out[out_len++] = (unsigned char) '=';
                                out[out_len++] = (unsigned char) '=';
                                break;
                        default:
                                break;
                                /* do nothing*/
                }
        }
        out[out_len]='\0';
        return out;
}

/* Base-64 decoding.  This represents binary data as printable ASCII
 ** characters.  Three 8-bit binary bytes are turned into four 6-bit
 ** values, like so:
 **
 **   [11111111]  [22222222]  [33333333]
 **
 **   [111111] [112222] [222233] [333333]
 **
 ** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
 */

//static int b64_decode_table[256] = {
const static int b64_decode_table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
 ** Return the actual number of bytes generated.  The decoded size will
 ** be at most 3/4 the size of the encoded, and may be smaller if there
 ** are padding characters (blanks, newlines).
 */
int b64_decode( const char* str, unsigned char* space, int size ){
        const char* cp;
        int space_idx, phase;
        int d, prev_d=0;
        unsigned char c;

        space_idx = 0;
        phase = 0;
        if(str==NULL)
                return space_idx;

        for ( cp = str; *cp != '\0'; ++cp )
        {
                d = b64_decode_table[(int)*cp];
                if ( d != -1 )
                {
                        switch ( phase )
                        {
                                case 0:
                                        ++phase;
                                        break;
                                case 1:
                                        c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
                                        if ( space_idx < size )
                                                space[space_idx++] = c;
                                        ++phase;
                                        break;
                                case 2:
                                        c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
                                        if ( space_idx < size )
                                                space[space_idx++] = c;
                                        ++phase;
                                        break;
                                case 3:
                                        c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
                                        if ( space_idx < size )
                                                space[space_idx++] = c;
                                        phase = 0;
                                        break;
                        }
                        prev_d = d;
                }
        }
        return space_idx;
}


/* Simple shell-style filename matcher.  Only does ? * and **, and multiple
 ** patterns separated by |.  Returns 1 or 0.
 */
int match( const char* pattern, const char* string ){
        const char* or;

        for (;;)
        {
                or = strchr( pattern, '|' );
                if ( or == (char*) 0 )
                        return match_one( pattern, strlen( pattern ), string );
                if ( match_one( pattern, or - pattern, string ) )
                        return 1;
                pattern = or + 1;
        }
}

static int match_one( const char* pattern, int patternlen, const char* string ){
        const char* p;

        for ( p = pattern; p - pattern < patternlen; ++p, ++string )
        {
                if ( *p == '?' && *string != '\0' )
                        continue;
                if ( *p == '*' )
                {
                        int i, pl;
                        ++p;
                        if ( *p == '*' )
                        {
                                /* Double-wildcard matches anything. */
                                ++p;
                                i = strlen( string );
                        }
                        else
                                /* Single-wildcard matches anything but slash. */
                                i = strcspn( string, "/" );
                        pl = patternlen - ( p - pattern );
                        for ( ; i >= 0; --i )
                                if ( match_one( p, pl, &(string[i]) ) )
                                        return 1;
                        return 0;
                }
                if ( *p != *string )
                        return 0;
        }
        if ( *string == '\0' )
                return 1;

#ifdef MIII_BAR
	if (*p == 0 && *string == '?')
		return 1;
#endif

        return 0;
}


#define DO_FILE_CHUNK_SIZE      (512)
void do_file(char *path, FILE *stream){
        FILE *fp;
        int c;
        char wwwPath[40] = "/www/";
        int numr,numw;
        char buffer[DO_FILE_CHUNK_SIZE + 1];
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
        if(strstr(path,"lingualMTD"))
                strcpy(wwwPath,"/var/www/");
#endif
        strcat(wwwPath, path);

        if (!(fp = fopen(wwwPath, "r")))
                return;

        /*
         * copy faster
         */
        while (feof(fp) == 0) {
                if ((numr = fread(buffer, 1, DO_FILE_CHUNK_SIZE, fp)) != DO_FILE_CHUNK_SIZE) {
                        if (ferror(fp) != 0) {
                                DEBUG_MSG("read file error.\n");
                                fclose(fp);
                                return;
                        }
                }

#if defined(HAVE_HTTPS)
                if (openssl_request == 1) {
                        if ((numw = BIO_write(stream, buffer, numr)) != numr) {
                                DEBUG_MSG("write file error.\n");
                                break;
                        }
                }
                else
#endif
                {
                        if ((numw = fwrite(buffer, 1, numr, stream)) != numr) {
                                DEBUG_MSG("write file error.\n");
                                break;
                        }
                }
        }

        fclose(fp);
}

/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
void do_auth_pic(char *path, FILE *stream){
        unsigned long auth_id;
        /* jimmy modified , modify auth codes from 4 to 5 */
        //unsigned short auth_code;
        unsigned long auth_code;
        /* ---------------------------------------------- */
        unsigned long afile_size;
        unsigned long afile_width;
        unsigned long afile_height;
        unsigned long font;
        unsigned char n;
        unsigned char *nfile = NULL;
        char bmp_file[64];
        int i, fd;

        if(strncmp(path,"auth.bmp",strlen("auth.bmp")) != 0){
                do_file(path,stream);
                return ;
        }

        DEBUG_MSG("Request coming for graphic authentication pic, auth.bmp ...\n");

        auth_id   = AuthGraph_GetId();
        auth_code = AuthGraph_GetCode();
        /* jimmy modified , modify auth codes from 4 to 5 */
        /*
        unsigned long 4 bytes:
           1 byte    1 byte    1 byte    1 byte
        |----|----|----|----|----|----|----|----|
           1   c    1    5    5    b    4    2
                we don't need the first 3 bytes
                so we shift the first 3 bytes to
        |----|----|----|----|----|----|----|----|
          5    5    b    4    2    0    0    0
        */
        auth_id = auth_id << 12;
        auth_code = auth_code << 12;
        /* ---------------------------------------- */
        /* malloc for pics */
        auth_bmp_data = malloc((unsigned char *)AUTH_BMP_LEN);
        /* reset the content ==> remove to here to avoid stack overflow ?? */
        for (i=0; i<(AUTH_BMP_LEN-54); i++){
                *(auth_bmp_data + 54 + i) = 0xff;
        }
        *(auth_bmp_data + 0) = 0x42; *(auth_bmp_data + 1) = 0x4d;*(auth_bmp_data + 2) = 0xcf;
        *(auth_bmp_data + 3) = 0x25; *(auth_bmp_data + 4) = 0x00;*(auth_bmp_data + 5) = 0x00;
        *(auth_bmp_data + 6) = 0x00; *(auth_bmp_data + 7) = 0x00;*(auth_bmp_data + 8) = 0x00;
        *(auth_bmp_data + 9) = 0x00; *(auth_bmp_data + 10) = 0x36;*(auth_bmp_data + 11) = 0x00;
        *(auth_bmp_data + 12) = 0x00; *(auth_bmp_data + 13) = 0x00;*(auth_bmp_data + 14) = 0x28;
        *(auth_bmp_data + 15) = 0x00; *(auth_bmp_data + 16) = 0x00;*(auth_bmp_data + 17) = 0x00;
        *(auth_bmp_data + 18) = 0x7d; *(auth_bmp_data + 19) = 0x00;*(auth_bmp_data + 20) = 0x00;
        *(auth_bmp_data + 21) = 0x00; *(auth_bmp_data + 22) = 0x19;*(auth_bmp_data + 23) = 0x00;
        *(auth_bmp_data + 24) = 0x00; *(auth_bmp_data + 25) = 0x00;*(auth_bmp_data + 26) = 0x01;
        *(auth_bmp_data + 27) = 0x00; *(auth_bmp_data + 28) = 0x18;*(auth_bmp_data + 29) = 0x00;
        *(auth_bmp_data + 30) = 0x00; *(auth_bmp_data + 31) = 0x00;*(auth_bmp_data + 32) = 0x00;
        *(auth_bmp_data + 33) = 0x00; *(auth_bmp_data + 34) = 0x00;*(auth_bmp_data + 35) = 0x00;
        *(auth_bmp_data + 36) = 0x00; *(auth_bmp_data + 37) = 0x00;*(auth_bmp_data + 38) = 0x61;
        *(auth_bmp_data + 39) = 0x0f; *(auth_bmp_data + 40) = 0x00;*(auth_bmp_data + 41) = 0x00;
        *(auth_bmp_data + 42) = 0x61; *(auth_bmp_data + 43) = 0x0f;*(auth_bmp_data + 44) = 0x00;
        *(auth_bmp_data + 45) = 0x00; *(auth_bmp_data + 46) = 0x00;*(auth_bmp_data + 47) = 0x00;
        *(auth_bmp_data + 48) = 0x00; *(auth_bmp_data + 49) = 0x00;*(auth_bmp_data + 50) = 0x00;
        *(auth_bmp_data + 51) = 0x00; *(auth_bmp_data + 52) = 0x00;*(auth_bmp_data + 53) = 0x00;

//      0               1  2    3    4          5       6       7       8       9          10   11      12              13 14  15
//      0x42,0x4d,0xcf,0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00, //16,
//16      17    18      19      20        21    22      23      24        25    26      27      28       29        30   31
//0x00,0x00,0x7d,0x00,0x00,0x00,0x19,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
///* --------------------------------------------------- */
//32     33             34      35      36              37 38   39      40        41    42      43      44        45    46      47
//0x00,0x00,0x00,0x00,0x00,0x00,0x61,0x0f,0x00,0x00,0x61,0x0f,0x00,0x00,0x00,0x00,
//48     49             50      51      52              53
//0x00,0x00,0x00,0x00,0x00,0x00


        afile_size = AuthGraph_GetBMPLength((unsigned char *) auth_bmp_data);
        //DEBUG_MSG("%s, auth_bmp_data size = %lu bytes\n",__FUNCTION__,afile_size);
        //DEBUG_MSG("%s, resetting auth_bmp_data content\n",__FUNCTION__);
        /* reset the content ==> remove to above to avoid stack overflow ?? */
        //AuthGraph_Reset((unsigned char *) auth_bmp_data);

        afile_width = AuthGraph_GetBMPWidth((unsigned char *) auth_bmp_data);
        afile_height = AuthGraph_GetBMPHeight((unsigned char *) auth_bmp_data);
        //DEBUG_MSG("%s, auth.bmp afile_width = %lu, afile_height = %lu\n"
        //              ,__FUNCTION__,afile_width,afile_height);

        /* read in base bmp data from file */
        nfile = (unsigned char *)malloc(BASE_BMP_LEN);
        if(nfile == NULL){
                printf("%s,line %d, malloc failed !\n",__FUNCTION__,__LINE__);
                if(auth_bmp_data)
                        free(auth_bmp_data);
                return ;
        }

/* (1) 1st number */
        memset(nfile,'\0',BASE_BMP_LEN);
        memset(bmp_file,'\0',sizeof(bmp_file));
        /* jimmy modified , modify auth codes from 4 to 5 */
        //n = (auth_code >> 12) & 0x000f;
        n = (auth_code >> 28) & 0x000f;
        /* ----------------------------------------------- */

        font = rand() % 4;
        sprintf(bmp_file,"%s/%s",PIC_PATH,authGraphFiles[font*16+n+1].pic_name);
        DEBUG_MSG("%s, font = %lu, n = %u !\n",__FUNCTION__,font,n);
        DEBUG_MSG("%s, using %s to generate 1st number !\n",__FUNCTION__,bmp_file);
        if((fd = open(bmp_file,O_RDONLY)) == -1){
                printf("%s,line %d, can't open %s !, errno = %d\n",__FUNCTION__,__LINE__,bmp_file,errno);
                free(nfile);
                if(auth_bmp_data)
                        free(auth_bmp_data);
                return ;
        }
        read(fd,nfile,BASE_BMP_LEN);
        close(fd);
        AuthGraph_CopyFromA2B((unsigned char *) auth_bmp_data, (unsigned char *) nfile, 1+rand()%5, 1+rand()%5);

/* (2) 2nd number*/
        memset(nfile,'\0',BASE_BMP_LEN);
        memset(bmp_file,'\0',sizeof(bmp_file));
        /* jimmy modified , modify auth codes from 4 to 5 */
        //n = (auth_code >> 8) & 0x000f;
        n = (auth_code >> 24) & 0x000f;
        /* ----------------------------------------------- */
        font = rand() % 4;
        sprintf(bmp_file,"%s/%s",PIC_PATH,authGraphFiles[font*16+n+1].pic_name);
        DEBUG_MSG("%s, 2nd font = %lu, n = %u !\n",__FUNCTION__,font,n);
        DEBUG_MSG("%s, using %s to generate 2nd number !\n",__FUNCTION__,bmp_file);
        if((fd = open(bmp_file,O_RDONLY)) == 0){
                printf("%s,line %d, can't open %s !, errno = %d\n",__FUNCTION__,__LINE__,bmp_file,errno);
                free(nfile);
                if(auth_bmp_data)
                        free(auth_bmp_data);
                return ;
        }
        read(fd,nfile,BASE_BMP_LEN);
        close(fd);
        AuthGraph_CopyFromA2B((unsigned char *) auth_bmp_data, (unsigned char *) nfile, 25+rand()%5, 1+rand()%5);

/* (3) 3rd number */
        memset(nfile,'\0',BASE_BMP_LEN);
        memset(bmp_file,'\0',sizeof(bmp_file));
        /* jimmy modified , modify auth codes from 4 to 5 */
        //n = (auth_code >> 4) & 0x000f;
        n = (auth_code >> 20) & 0x000f;
        /* ----------------------------------------------- */
        font = rand() % 4;
        sprintf(bmp_file,"%s/%s",PIC_PATH,authGraphFiles[font*16+n+1].pic_name);
        //DEBUG_MSG("%s, 3rd font = %lu, n = %u !\n",__FUNCTION__,font,n);
        //DEBUG_MSG("%s, using %s to generate 3rd number !\n",__FUNCTION__,bmp_file);
        if((fd = open(bmp_file,O_RDONLY)) == -1){
                printf("%s,line %d, can't open %s !, errno = %d\n",__FUNCTION__,__LINE__,bmp_file,errno);
                free(nfile);
                if(auth_bmp_data)
                        free(auth_bmp_data);
                return ;
        }

        read(fd,nfile,BASE_BMP_LEN);
        close(fd);
        AuthGraph_CopyFromA2B((unsigned char *) auth_bmp_data, (unsigned char *) nfile, 50+rand()%5, 1+rand()%5);

/* (4) 4th number */
        memset(nfile,'\0',BASE_BMP_LEN);
        memset(bmp_file,'\0',sizeof(bmp_file));
        /* jimmy modified , modify auth codes from 4 to 5 */
        //n = (auth_code    ) & 0x000f;
        n = (auth_code >> 16) & 0x000f;
        /* ----------------------------------------------- */
        font = rand() % 4;
        sprintf(bmp_file,"%s/%s",PIC_PATH,authGraphFiles[font*16+n+1].pic_name);
        //DEBUG_MSG("%s, 4th font = %lu, n = %u !\n",__FUNCTION__,font,n);
        //DEBUG_MSG("%s, useing %s to generate 4th number !\n",__FUNCTION__,bmp_file);
        if((fd = open(bmp_file,O_RDONLY)) == -1){
                printf("%s,line %d, can't open %s !, errno = %d\n",__FUNCTION__,__LINE__,bmp_file,errno);
                free(nfile);
                if(auth_bmp_data)
                        free(auth_bmp_data);
                return ;
        }
        read(fd,nfile,BASE_BMP_LEN);
        close(fd);
        AuthGraph_CopyFromA2B((unsigned char *) auth_bmp_data, (unsigned char *) nfile, 75+rand()%5, 1+rand()%5);

        /* jimmy modified , modify auth codes from 4 to 5 */
/* (5) 5th number */
        memset(nfile,'\0',BASE_BMP_LEN);
        memset(bmp_file,'\0',sizeof(bmp_file));
        n = (auth_code >> 12) & 0x000f;
        font = rand() % 4;
        sprintf(bmp_file,"%s/%s",PIC_PATH,authGraphFiles[font*16+n+1].pic_name);
        //DEBUG_MSG("%s, 4th font = %lu, n = %u !\n",__FUNCTION__,font,n);
        //DEBUG_MSG("%s, useing %s to generate 4th number !\n",__FUNCTION__,bmp_file);
        if((fd = open(bmp_file,O_RDONLY)) == -1){
                printf("%s,line %d, can't open %s !, errno = %d\n",__FUNCTION__,__LINE__,bmp_file,errno);
                free(nfile);
                if(auth_bmp_data)
                        free(auth_bmp_data);
                return ;
        }
        read(fd,nfile,BASE_BMP_LEN);
        close(fd);
        AuthGraph_CopyFromA2B((unsigned char *) auth_bmp_data, (unsigned char *) nfile, 100+rand()%5, 1+rand()%5);
        /* -------------------------------------------------------- */

        if(nfile)
                free(nfile);

        /* random pixel */
        for(i=0; i<200; i++) {
                unsigned long x, y, c_r, c_g, c_b;
                x = rand() % (afile_width);
                y = rand() % (afile_height);

                c_r = rand() & 0xff;
                c_g = rand() & 0xff;
                c_b = rand() & 0xff;

                AuthGraph_DrawPixel((unsigned char *) auth_bmp_data,
                                x, y, c_r, c_g, c_b);
        }

        for(i = 0 ; i < AUTH_BMP_LEN ; i++){
/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
                wfputc(auth_bmp_data[i], stream);
        }

        /* malloc for pics */
        if(auth_bmp_data){
                free(auth_bmp_data);
        }
        /* ------------------ */
}
#endif
/* --------------------------------------------------------------- */

static int check_file_exist(char *path){
        FILE *fp;
        char wwwPath[20] = "/www/";

  /* chklst.txt  : check list entry
     wlan.txt    : wireless check list entry
     HNAP1.txt   : pure network web entry
     ip_error.txt: limit connect ip error entry
     wireless_update.txt : wireless driver update entry
  */
/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
        if(strncmp(path,"auth.bmp",strlen("auth.bmp")) == 0){
                return 1;
        }
#endif

//				printf("gogogo1200 [%s]\n",path);    

		if( strchr(path,'.') )

/* ------------------------------------------ */
        if(!(strncmp(strchr(path,'.')+1,"cgi",3)) || !(strncmp(path,"chklst.txt",10)) \
                        || !(strncmp(path,"wlan.txt",8)) || !(strncmp(path,"HNAP1.txt",9)) \
                        || !(strncmp(path,"ip_error.txt",12)) || !(strncmp(path,"wireless_update.txt",19)) \
                        || !(strncmp(path,"router_info.xml", 15)) || !(strncmp(path,"post_login.xml", 14)) \
/* jimmy added 20080401 , Dlink Hidden Page V1.01 support */
                        || !(strncmp(path,"version.txt", strlen("version.txt")))
/* ------------------------------------------------------ */
                        || !(strncmp(path,"F2_info.txt", strlen("F2_info.txt")))
                        || !(strncmp(path,"device.xml=", 11)) \
                        || !(strncmp(path,"ping_test.xml=", 14)) \
        )
                return 1;
        if(!(fp = fopen(strcat(wwwPath, path),"r")))
                return 0;
        else{
                fclose(fp);
                return 1;
        }
}

static int checkQueryTimeout(int *prefix){
        struct timeval timeval;
        fd_set afdset;

        /* initial timeout */
        memset(&timeval, 0, sizeof(timeval));
        timeval.tv_sec = 3;
        timeval.tv_usec = 0;

        /* initial data stream */
        FD_ZERO(&afdset);
        FD_SET(conn_fd,&afdset);

        /* if session establish, quit after 5 sec. with no request */
        if( select(conn_fd + 1, &afdset, NULL, NULL, &timeval) <= 0)
                        return 0;

        /* if got request, quit after 5 sec. with no continue data */
        while((*prefix = fgetc(conn_fp)) != EOF){
        if(*prefix == 'G' || *prefix == 'P')   // G for GET , P for POST
                break;
        timeval.tv_sec = 3;
        timeval.tv_usec = 0;
        if( select(conn_fd+1, &afdset, NULL, NULL, &timeval) <= 0)
                        return 0;
  }

  return 1;
}

static int composeToOriinalRequest(int prefix,char *line){
        char line_tmp[1024] = {};

        /* get the first line remainder of the request. */
        if ( fgets( line_tmp, sizeof(line_tmp), conn_fp ) == (char*) 0 ) {
                send_error( 400, "Bad Request", (char*) 0, "No request found." );
                return 0;
        }

        /* add request remainder(ET / HTTP/1.1) to line ,
           original should be like (GET / HTTP/1.1) || (POST .......)*/
        sprintf(line,"%c%s",prefix,line_tmp);
        return 1;
}

/* for GPL used */
/**************************************************************/
/**************************************************************/
extern struct mime_handler customer_mime_handlers[];
extern int customer_mime_handlers_num;
/* for GPL used */
/**************************************************************/
/**************************************************************/

/*
*	Date: 2010-07-30
*   Name: Ken Chiang
*   Reason: Add usb storage upload and download file by http feature.
*   Notice :
*/
#ifdef USB_STORAGE_HTTP_ENABLE
//#define USB_STORAGE_HTTP_DEBUG 1
#ifdef USB_STORAGE_HTTP_DEBUG
#undef USB_STORAGE_DEBUG_MSG(fmt, arg...)
#define USB_STORAGE_DEBUG_MSG(fmt, arg...)      printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define USB_STORAGE_DEBUG_MSG(fmt, arg...)
#endif

#define htmlWrite(html_result, fmt, args...) ({sprintf(html_result, fmt, ## args);})
static const char *http_storage_header =
"<html>"
"<meta http-equiv=\"Content-Type\" content=\"text-html; charset=utf-8\">"
"<head>";

static const char *http_storage_script =
"<script language=\"JavaScript\">"
"  function get_by_id(id){"
"    with(document){"
"      return getElementById(id);"
"    }"
"  }"
"  function CreatDirectory(){"
"    if (get_by_id(\"directory\").value == \"\"){"
"      	alert(\"You must enter the name of a directory first.\");"
"      	return false;"
"    }"
"    get_by_id(\"form1\").submit();"
"  }"
"  function LoadFile(){"
"    if (get_by_id(\"file\").value == \"\"){"
"      alert(\"You must enter the name of a file first.\");"
"      return false;"
"    }"
"    get_by_id(\"form\").submit();"
"  }"
"  function DelFile(filename, dirflag){"
"	var del_path = get_by_id(\"del_path\");"
"	get_by_id(\"del_path\").value = del_path+filename;"
"   get_by_id(\"del_type\").value = dirflag;"		
"   get_by_id(\"form\").submit();"
"  }"
"</script>";

static const char *http_storage_upload_form =
"<form id=\"form\" name=\"form\" method=POST action=\"upload_file.cgi\" enctype=multipart/form-data>"
"<input type=\"hidden\" id=\"upload_file_path\" name=\"upload_file_path\" value=\"%s\">"
"<input type=\"hidden\" id=\"del_path\" name=\"del_path\" value=\"%s\">"
"<input type=\"hidden\" id=\"del_type\" name=\"del_type\" value=\"0\">"
"  <tr>"
"    <td height=\"72\" align=right valign=\"top\" class=\"duple\">\"Load File From Local Hard Drive\"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;:&nbsp;</td>"
"    <td valign=\"top\">&nbsp;"
"    <input type=file id=\"file\" name=\"file\" size=20>&nbsp;"
"    <input name=\"load\" id=\"load\" type=\"button\" value=\"  Upload File  \" onClick=\"LoadFile()\">"
"    </td>"
"  </tr>"
"</form>";

static const char *http_storage_creat_form =
"<form id=\"form1\" name=\"form1\" method=POST action=\"creat_directory.cgi\" enctype=multipart/form-data>"
"<input type=\"hidden\" id=\"creat_directory_path\" name=\"creat_directory_path\" value=\"%s\">"
"  <tr>"
"    <td height=\"72\" align=right valign=\"top\" class=\"duple\">\"Creat Directory to Device Hard Drive\"&nbsp;&nbsp;:&nbsp;</td>"
"    <td valign=\"top\">&nbsp;"
"    <input type=text id=\"directory\" name=\"directory\" size=30 maxlength=40 value=\"\">&nbsp;"
"    <input name=\"creat\" id=\"creat\" type=\"button\" value=\"Creat Directory\" onClick=\"CreatDirectory()\">"
"    </td>"
"  </tr>"
"</form>";

static const char *http_storage_back_script =
"<script language=\"JavaScript\">"
"  function back(){"
"    window.location.href = \"http://%s%s\";"		
"  }"
"</script>"
;
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: Add for usb storage upload and download file can uesd https.
*   Notice :
*/
static const char *https_storage_back_script =
"<script language=\"JavaScript\">"
"  function back(){"
"    window.location.href = \"https://%s%s\";"		
"  }"
"</script>"
;
static const char *http_storage_back_body =
"<title>D-LINK CORPORATION, INC</title>"
"</head>"
"<body>"
"<tr><h2><td><p align=\"center\">%s</p></td></h2></tr>"
"<hr>"
"<tr><td><p align=\"center\"><input name=\"button\" id=\"button\" type=\"button\" value=\"Continue\" onClick=\"back( );\"></p></td></tr>";

static const char *http_storage_tailer =
"</body>"
"</html>";

// %20 --> Space" " ,%5E --> ^ and special char Space" ",$,&,' --> count
static int StrSpaceDecode(char *from, char *to)
{
	int ret = 0;
	char *pt = to;
	char *pf = NULL;	
	while (*from)
	{
		switch (*from)
		{
			case '%':
				pf = from+1;
				//printf("pf1=%c\n",*pf);
				if( *pf == '2'){
					pf = from+2;
					//printf("pf2=%c\n",*pf);
					if( *pf == '0'){
						from+=2;
						*pt++ = ' ';
						ret++;
					}
					else
						*pt++ = *from;	
				}
				else if( *pf == '5'){
					pf = from+2;
					//printf("pf2=%c\n",*pf);
					if( *pf == 'E'){
						from+=2;
						*pt++ = '^';
						ret++;
					}
					else
						*pt++ = *from;	
				}
				else
					*pt++ = *from;		
			break;
			case '$':
				ret++;
				*pt++ = *from;
			break;	
			case '&':
				ret++;
				*pt++ = *from;	
			break;
			case '\'':
				ret++;
				*pt++ = *from;	
			break;
			case '(':
				ret++;
				*pt++ = *from;	
			break;
			case ')':
				ret++;
				*pt++ = *from;	
			break;
			case ' ':
				ret++;
				*pt++ = *from;	
			break;
			
			// fall through to default
			default:
				*pt++ = *from;
			break;
		}
		from++;
	}
	*pt = '\0';

	return ret;
}

// Space" " --> \Space "\ " and special char $ --> \$,& --> \&,^ --> \^,' --> \'
static int StrSpaceEncode(char *from, char *to)
{
	int ret = 0;
	char *pt = to;
	//char *pf = NULL;
	while (*from)
	{
		switch (*from)
		{
			case ' ':				
				*pt++ = '\\';
				*pt++ = *from;		
				ret++;
			break;
			case '$':				
				*pt++ = '\\';
				*pt++ = *from;		
				ret++;
			break;
			case '&':				
				*pt++ = '\\';
				*pt++ = *from;		
				ret++;
			break;
			case '^':				
				*pt++ = '\\';
				*pt++ = *from;		
				ret++;
			break;
			case '\'':				
				*pt++ = '\\';
				*pt++ = *from;		
				ret++;
			break;
			case '(':				
				*pt++ = '\\';
				*pt++ = *from;		
				ret++;
			break;
			case ')':				
				*pt++ = '\\';
				*pt++ = *from;		
				ret++;
			break;
			// fall through to default
			default:
				*pt++ = *from;
			break;
		}
		from++;
	}
	*pt = '\0';

	return ret;
}
	
int usb_storge_http_mount_match(char *fileurl)
{
	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0};
	FILE *fp=NULL;

	USB_STORAGE_DEBUG_MSG("%s: fileurl=%s\n",__func__,fileurl);
	system("df -k |grep /tmp/sd > /var/httpdf.txt");
	system("df -k |grep /tmp/mm >> /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");		
		return -1;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			USB_STORAGE_DEBUG_MSG("temp=%s\n",temp);
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			USB_STORAGE_DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			if(!strcmp(mount,fileurl)){
				USB_STORAGE_DEBUG_MSG("find http file match\n");	
				fclose(fp);
				return 0;
			}	
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}		
	return -1;	
}
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
void usb_storge_http_handler(char *fileurl, FILE *stream, int lan_request, char *wan_ipaddrport)
{	
	int list_count=0,rl=0;
	char storge_temp[1024]={0};
	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0};
	char noused[20]={0},arrt[20]={0},size[12]={0},week[8],month[8]={0},day[8]={0},year[8]={0},time[16]={0};
	char name[128]={0},nametmp[128]={0},listtemp[512]={0};
	char *combuf=NULL;	
	FILE *fp=NULL,*fp1=NULL;

	assert(fileurl);
	assert(stream);
	USB_STORAGE_DEBUG_MSG("%s:fileurl=%s\n",__func__,fileurl);	
	system("df -k |grep /tmp/sd > /var/httpdf.txt");
	system("df -k |grep /tmp/mm >> /var/httpdf.txt");	
	fp = fopen("/var/httpdf.txt","r");	
	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");		
		return;
	}else{
		send_headers( 200, "Ok", no_cache, "text/html" );		
		websWrite(stream, "%s", http_storage_header);
		websWrite(stream, "%s", http_storage_script); 
		while(fgets(temp,sizeof(temp),fp))
		{	
			USB_STORAGE_DEBUG_MSG("temp=%s\n",temp);
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			USB_STORAGE_DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			if(!strcmp(mount,fileurl)){
				USB_STORAGE_DEBUG_MSG("mount=%s==fileurl=%s\n",mount,fileurl);
				if(!lan_request){
					websWrite(stream,"<title>%s - %s</title>",wan_ipaddrport, fileurl);
					websWrite(stream,"</head><body><h1>%s - %s</h1>",wan_ipaddrport, fileurl);
				}	
				else{
					websWrite(stream,"<title>%s - %s</title>",nvram_safe_get("lan_ipaddr"), fileurl);
					websWrite(stream,"</head><body><h1>%s - %s</h1>",nvram_safe_get("lan_ipaddr"), fileurl);	
				}				
				websWrite(stream,http_storage_upload_form, fileurl, fileurl);
				websWrite(stream,http_storage_creat_form, fileurl);							
				//_system("ls -le %s > /var/usbls.txt",fileurl);	
				combuf = (char *)malloc(strlen(fileurl)+100);
				if(combuf){
					sprintf(combuf,"ls -le \"%s\" > /var/usbls.txt",fileurl);
					system(combuf);
					free(combuf);
				}
				else{
					printf("combuf malloc fail\n");
					return;	
				}
				fp1 = fopen("/var/usbls.txt","r");
				if(fp1 == NULL){
					printf("%s:\"%s\" does not exist\n",__func__,"/var/usbls.txt");		
					break;
				}else{	
					//do list			
					while(fgets(listtemp,sizeof(listtemp),fp1))
					{	
						USB_STORAGE_DEBUG_MSG("listtemp=%s\n",listtemp);
						rl=sscanf(listtemp, "%s %s %s %s %s %s %s %s %s %s %s %s", arrt, noused, noused, noused, size, week, month, day, time , year, name, nametmp);
						USB_STORAGE_DEBUG_MSG("%s %s %s %s %s %s %s %s \n", arrt, size, week ,month, day, year, time , name, nametmp);
						DEBUG_MSG("######nametmp=%s\n",nametmp);
						USB_STORAGE_DEBUG_MSG("sscanf=%d\n",rl);
						if(rl > 11){
							char *name_ptr=NULL;							
							name_ptr = strstr(listtemp,year);
							if(name_ptr){
								name_ptr+=5;
								//USB_STORAGE_DEBUG_MSG("name_ptr=%s\n",name_ptr);
								strcpy(name,name_ptr);
								name[strlen(name)-1]='\0';
								DEBUG_MSG("name=%s..\n",name);
							}	
			 			}	
			 			//windows system dir or file
			 			if( strstr(name,"System Volume Information") || strstr(name,"$RECYCLE.BIN") || strstr(name,"RECYCLER") || strstr(name,"desktop.ini") ){
			 					USB_STORAGE_DEBUG_MSG("%s=windows system\n",name);
			 					continue;
			 			}			 						 			
			 			//special char don't list
			 			if( strstr(name,"#") || strstr(name,"%") ){
			 				USB_STORAGE_DEBUG_MSG("%s=special char\n",name);
			 				continue;
			 			}
						if(list_count){
							if(strstr(arrt,"d")){//dir
								websWrite(stream,"<br>    %-4s,%-4s%-3s%-5s,%-9s    &lt;dir&gt; <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',1)\">",week, month, day, year, time, fileurl, name, name, name);
							}
							else{	
								websWrite(stream,"<br>    %-4s,%-4s%-3s%-5s,%-9s    %s <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',0)\">",week, month, day, year, time, size, fileurl, name, name, name);
							}
						}	
						else{//first	
							if(strstr(arrt,"d")){//dir
								websWrite(stream,"<hr><pre><br><br>    %-4s,%-4s%-3s%-5s,%-9s    &lt;dir&gt; <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',1)\">",week, month, day, year, time, fileurl, name, name, name);
							}
							else{	
								websWrite(stream,"<hr><pre><br><br>    %-4s,%-4s%-3s%-5s,%-9s    %s <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',0)\">",week, month, day, year, time, size, fileurl, name, name, name);
							}
						}	
						list_count++;
						memset(listtemp,0,sizeof(listtemp));
					}//while
					if(list_count)//had list
						websWrite(stream, "%s","<br></pre><hr>");				
					fclose(fp1);
				}				
			}//strcmp(mount,fileurl)			
			memset(temp,0,sizeof(temp));			
		}//while		
		fclose(fp);
		websWrite(stream, "%s", http_storage_tailer);				
	}		
	/* Reset CGI */
	init_cgi(NULL);
}

int usb_storge_http_list_match(char *fileurl,int Space_flag)
{
	int rl=0, special_char_flag=0;
	char *fileurl_temp=NULL,*fileurl_url=NULL,*fileurl_file=NULL;
	char *combuf=NULL,*fileurl_sp_temp=NULL,*fileurl_sp_url=NULL;
	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0};
	char noused[20]={0},arrt[20]={0},size[12]={0},week[8],month[8]={0},day[8]={0},year[8]={0},time[16]={0};
	static char name[128]={0},nametmp[128]={0},listtemp[256]={0};
	FILE *fp=NULL, *fp1=NULL;

	USB_STORAGE_DEBUG_MSG("%s:\n",__func__);		
	USB_STORAGE_DEBUG_MSG("fileurl=%s\n",fileurl);
	fileurl_url = strdup(fileurl);
	// get file url
	if(fileurl_url){
		USB_STORAGE_DEBUG_MSG("fileurl_url=%s\n",fileurl_url);
		fileurl_temp=strrchr(fileurl_url,'/');
		if(fileurl_temp){
			USB_STORAGE_DEBUG_MSG("fileurl_temp=%s\n",fileurl_temp);		
			*fileurl_temp='\0';
			//USB_STORAGE_DEBUG_MSG("fileurl_temp=%s\n",fileurl_temp);
		}		
		USB_STORAGE_DEBUG_MSG("fileurl_url=%s\n",fileurl_url);
	}
	else{
		printf("strdup fileurl_url fail\n");
		fileurl_url=fileurl;
		USB_STORAGE_DEBUG_MSG("fileurl_url=%s\n",fileurl_url);
	}		
	//check Space and special char in fileurl_url
	if( Space_flag && ( strstr(fileurl_url," ") || strstr(fileurl_url,"$") || strstr(fileurl_url,"&") || strstr(fileurl_url,"^") || 
										strstr(fileurl_url,"'") || strstr(fileurl_url,"(") || strstr(fileurl_url,")") ) ){
		int Temp_space_flag = 0, Temp_space_count = 0;
		special_char_flag = 1;
		fileurl_sp_url = strdup(fileurl_url);
		USB_STORAGE_DEBUG_MSG("check Space and special char fileurl_sp_url=%s\n",fileurl_sp_url);
		Temp_space_flag=StrSpaceDecode(fileurl_url,fileurl_sp_url);
		USB_STORAGE_DEBUG_MSG("fileurl_sp_url=%s,Temp_space_flag=%d\n",fileurl_sp_url,Temp_space_flag);
		fileurl_sp_temp = (char *)malloc(strlen(fileurl_sp_url)+Temp_space_flag+2);
		if(fileurl_sp_temp){			
			Temp_space_count=StrSpaceEncode(fileurl_sp_url, fileurl_sp_temp);
			if(Temp_space_count!=Temp_space_flag)
				printf("Temp_space_count=%d != Temp_space_flag=%d\n",Temp_space_count, Temp_space_flag);
			USB_STORAGE_DEBUG_MSG("fileurl_sp_temp=%s\n",fileurl_sp_temp);				
		}
		else{
			printf("fileurl_sp_temp malloc fail\n");
			fileurl_sp_temp=fileurl_sp_url;
		}
		strcpy(fileurl_url,fileurl_sp_temp);
		USB_STORAGE_DEBUG_MSG("fileurl_url=%s..\n",fileurl_url);
	}					
	fileurl_file = strrchr(fileurl,'/');
	// get file name
	if(fileurl_file){
		USB_STORAGE_DEBUG_MSG("fileurl_file=%s\n",fileurl_file);
		fileurl_file ++;
		USB_STORAGE_DEBUG_MSG("fileurl_file=%s..\n",fileurl_file);		
	}
	else{
		printf("fileurl_file does not exist\n");
		if(fileurl_sp_temp)			
			free(fileurl_sp_temp);			
		return -1;
	}	
	system("df -k |grep /tmp/sd > /var/httpdf.txt");
	system("df -k |grep /tmp/mm >> /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");
	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");
		if(fileurl_sp_temp)			
			free(fileurl_sp_temp);		
		return -1;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			USB_STORAGE_DEBUG_MSG("temp=%s\n",temp);
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			USB_STORAGE_DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			if(strstr(fileurl,mount)){
				USB_STORAGE_DEBUG_MSG("find mount path match\n");
				USB_STORAGE_DEBUG_MSG("fileurl_url=%s\n",fileurl_url);
				//_system("ls -le %s > /var/usbls.txt",fileurl_url);
				combuf = (char *)malloc(strlen(fileurl_url)+100);
				if(combuf){
					USB_STORAGE_DEBUG_MSG("fileurl_url=%s..\n",fileurl_url);
					if(special_char_flag)
						sprintf(combuf,"ls -le %s > /var/usbls.txt",fileurl_url);
					else
						sprintf(combuf,"ls -le \"%s\" > /var/usbls.txt",fileurl_url);
					USB_STORAGE_DEBUG_MSG("combuf=%s\n",combuf);
					system(combuf);
					free(combuf);
				}
				else{
					printf("combuf malloc fail\n");
					if(fileurl_sp_temp)			
						free(fileurl_sp_temp);
					return;	
				}		
				fp1 = fopen("/var/usbls.txt","r");
				if(fp1 == NULL){
					printf("%s:\"%s\" does not exist\n",__func__,"/var/usbls.txt");		
					break;
				}else{				
					while(fgets(listtemp,sizeof(listtemp),fp1))
					{	
						int i;
						USB_STORAGE_DEBUG_MSG("listtemp=%s\n",listtemp);
						rl=sscanf(listtemp, "%s %s %s %s %s %s %s %s %s %s %s %s", arrt, noused, noused, noused, size, week, month, day, time , year, name, nametmp);
						USB_STORAGE_DEBUG_MSG("%s %s %s %s %s %s %s %s %s\n", arrt, size, week ,month, day, year, time , name ,nametmp);
						if(rl > 11){							
							char *name_ptr=NULL;							
							name_ptr = strstr(listtemp,year);
							if(name_ptr){
								name_ptr+=5;
								USB_STORAGE_DEBUG_MSG("name_ptr=%s\n",name_ptr);
								strcpy(name,name_ptr);
								name[strlen(name)-1]='\0';
								USB_STORAGE_DEBUG_MSG("name=%s..\n",name);
							}							
			 			}	
						if(!strcmp(fileurl_file,name)){
							USB_STORAGE_DEBUG_MSG("find mount list match\n");
							if(strstr(arrt,"d")){//dir
								USB_STORAGE_DEBUG_MSG("It's dir\n");
								fclose(fp);
								fclose(fp1);
								if(fileurl_sp_temp)			
									free(fileurl_sp_temp);
								return 1;
							}	
							fclose(fp);
							fclose(fp1);
							if(fileurl_sp_temp)			
								free(fileurl_sp_temp);
							return 0;
						}	
						memset(listtemp,0,sizeof(listtemp));					
					}//while										
				}
				fclose(fp1);				
			}	
			memset(temp,0,sizeof(temp));			
		}//while		
		fclose(fp);		
	}	
	if(fileurl_sp_temp)			
			free(fileurl_sp_temp);
	return -1;	
}

void do_get_file(char *path, FILE *stream)
{
	FILE *fp;
	int c;

	DEBUG_MSG("%s: path = %s\n",__func__,path);
	if (!(fp = fopen(path, "r"))){
		printf("\"%s\" can't open\n",path);
		return;
	}
	//send_headers( 200, "Ok", no_cache, "text/html" );
	send_headers( 200, "Ok", no_cache, "application/octet-stream");
	while ((c = fgetc(fp)) != EOF) {
		wfputc(c, stream);
	}
	USB_STORAGE_DEBUG_MSG("%s: finsh\n",__func__);
	fclose(fp);
}


/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
void do_get_dir_list(char *fileurl, FILE *stream, int Space_flag, int lan_request, char *wan_ipaddrport)
{	
	int list_count=0,rl=0,special_char_flag=0;
	char storge_temp[1024]={0};
	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0};
	char noused[20]={0},arrt[20]={0},size[12]={0},week[8],month[8]={0},day[8]={0},year[8]={0},time[16]={0};
	char name[128]={0},nametmp[128]={0},listtemp[512]={0};
	char *combuf=NULL,*fileurl_parent=NULL,*fileurl_temp=NULL,*fileurl_sp_url=NULL;
	FILE *fp=NULL,*fp1=NULL;	
	
	assert(fileurl);
	assert(stream);	
	USB_STORAGE_DEBUG_MSG("%s: fileurl=%s\n",__func__ ,fileurl);
	fileurl_parent = strdup(fileurl);
	//get parent dir
	if(fileurl_parent){
		USB_STORAGE_DEBUG_MSG("fileurl_parent=%s\n",fileurl_parent);		
		fileurl_temp=strrchr(fileurl_parent,'/');
		if(fileurl_temp){
			USB_STORAGE_DEBUG_MSG("fileurl_temp=%s\n",fileurl_temp);		
			*fileurl_temp='\0';					
		}
		else
		{
			USB_STORAGE_DEBUG_MSG("fileurl_temp NULL\n");
		}
		USB_STORAGE_DEBUG_MSG("fileurl_parent=%s\n",fileurl_parent);	
	}
	else{
		printf("strdup fileurl_parent fail\n");
		fileurl_parent=fileurl;
		USB_STORAGE_DEBUG_MSG("fileurl_parent=%s\n",fileurl_parent);
	}	
	//check Space and special char in fileurl 
		if( Space_flag && ( strstr(fileurl," ") || strstr(fileurl,"$") || strstr(fileurl,"&") || strstr(fileurl,"^") || 
										strstr(fileurl,"'") || strstr(fileurl,"(") || strstr(fileurl,")") ) ){
		int Temp_space_flag = 0, Temp_space_count = 0;
		special_char_flag = 1;
		fileurl_sp_url = strdup(fileurl);
		USB_STORAGE_DEBUG_MSG("check Space and special char fileurl_sp_url=%s\n",fileurl_sp_url);
		Temp_space_flag=StrSpaceDecode(fileurl,fileurl_sp_url);
		USB_STORAGE_DEBUG_MSG("fileurl_sp_url=%s,Temp_space_flag=%d\n",fileurl_sp_url,Temp_space_flag);
		fileurl_temp = (char *)malloc(strlen(fileurl)+Temp_space_flag+2);
		if(fileurl_temp){			
			Temp_space_count=StrSpaceEncode(fileurl_sp_url, fileurl_temp);
			if(Temp_space_count!=Temp_space_flag)
				printf("Temp_space_count=%d != Temp_space_flag=%d\n",Temp_space_count, Temp_space_flag);
			USB_STORAGE_DEBUG_MSG("fileurl_temp=%s\n",fileurl_temp);				

//		int Space_count=0;
//		fileurl_temp = (char *)malloc(strlen(fileurl)+Space_flag+2);
//		if(fileurl_temp){			
//			Space_count=StrSpaceEncode(fileurl, fileurl_temp);
//			if(Space_count!=Space_flag)
//				printf("Space_count=%d != Space_flag=%d\n",Space_count, Space_flag);
//			USB_STORAGE_DEBUG_MSG("fileurl_temp=%s\n",fileurl_temp);				
		}
		else{
			printf("fileurl_temp malloc fail\n");
			fileurl_temp=fileurl;
		}		
	}
	else{
		fileurl_temp=fileurl;	
	}					
	system("df -k |grep /tmp/sd > /var/httpdf.txt");
	system("df -k |grep /tmp/mm >> /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");
	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");
		if(fileurl_temp)			
			free(fileurl_temp);			
		return;
	}else{
		send_headers( 200, "Ok", no_cache, "text/html" );
		websWrite(stream, "%s", http_storage_header);
		websWrite(stream, "%s", http_storage_script);	 		
		while(fgets(temp,sizeof(temp),fp))
		{		
			USB_STORAGE_DEBUG_MSG("temp=%s\n",temp);
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			USB_STORAGE_DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			if(strstr(fileurl,mount)){
				USB_STORAGE_DEBUG_MSG("find mount path match\n");
				if(!lan_request){					
					websWrite(stream,"<title>%s - %s</title>",wan_ipaddrport, fileurl);
					websWrite(stream,"</head><body><h1>%s - %s</h1>",wan_ipaddrport, fileurl);
				}	
				else{
					websWrite(stream,"<title>%s - %s</title>",nvram_safe_get("lan_ipaddr"), fileurl);				
					websWrite(stream,"</head><body><h1>%s - %s</h1>",nvram_safe_get("lan_ipaddr"), fileurl);
				}
				websWrite(stream,http_storage_upload_form, fileurl, fileurl);
				websWrite(stream,http_storage_creat_form, fileurl);
				websWrite(stream,"<br><a href='%s'>[To Parent Directory]</a>",fileurl_parent);
				//_system("ls -le %s > /var/usbls.txt",fileurl_temp);
				combuf = (char *)malloc(strlen(fileurl_temp)+100);
				if(combuf){
					USB_STORAGE_DEBUG_MSG("fileurl_url=%s..\n",fileurl_temp);
					if(special_char_flag)
						sprintf(combuf,"ls -le %s > /var/usbls.txt",fileurl_temp);
					else
						sprintf(combuf,"ls -le \"%s\" > /var/usbls.txt",fileurl_temp);
					USB_STORAGE_DEBUG_MSG("combuf=%s\n",combuf);
					system(combuf);
					free(combuf);
				}
				else{
					printf("combuf malloc fail\n");
					if(fileurl_temp)			
						free(fileurl_temp);	
					return;	
				}	
				fp1 = fopen("/var/usbls.txt","r");
				if(fp1 == NULL){
					printf("%s:\"%s\" does not exist\n",__func__,"/var/usbls.txt");		
					break;
				}else{	
					//do list			
					while(fgets(listtemp,sizeof(listtemp),fp1))
					{	
						USB_STORAGE_DEBUG_MSG("listtemp=%s\n",listtemp);
						rl=sscanf(listtemp, "%s %s %s %s %s %s %s %s %s %s %s %s", arrt, noused, noused, noused, size, week, month, day, time , year, name, nametmp);
						USB_STORAGE_DEBUG_MSG("%s %s %s %s %s %s %s %s %s\n", arrt, size, week ,month, day, year, time , name ,nametmp);
						if(rl > 11){
							char *name_ptr=NULL;							
							name_ptr = strstr(listtemp,year);
							if(name_ptr){
								name_ptr+=5;
								//USB_STORAGE_DEBUG_MSG("name_ptr=%s\n",name_ptr);
								strcpy(name,name_ptr);
								name[strlen(name)-1]='\0';
								USB_STORAGE_DEBUG_MSG("name=%s..\n",name);
							}	
			 			}		
			 			//windows system dir or file
			 			if( strstr(name,"System Volume Information") || strstr(name,"$RECYCLE.BIN") || strstr(name,"RECYCLER") || strstr(name,"desktop.ini") ){
			 				USB_STORAGE_DEBUG_MSG("%s=windows system\n",name);
			 				continue;
			 			}				 				
			 			//special char don't list
			 			if( strstr(name,"#") || strstr(name,"%") ){
			 				USB_STORAGE_DEBUG_MSG("%s=special char\n",name);
			 				continue;
			 			}			 			
						if(list_count){
							if(strstr(arrt,"d")){//dir
								websWrite(stream,"<br>    %-4s,%-4s%-3s%-5s,%-9s    &lt;dir&gt; <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',1)\">",week, month, day, year, time, fileurl, name, name, name);
							}
							else{	
								websWrite(stream,"<br>    %-4s,%-4s%-3s%-5s,%-9s    %s <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',0)\">",week, month, day, year, time, size, fileurl, name, name, name);
							}
						}	
						else{//first	
							if(strstr(arrt,"d")){//dir
								//websWrite(stream,"<hr><pre><a href='%s'>[To Parent Directory]</a><br><br>    %-4s,%-4s%-3s%-5s,%-9s    &lt;dir&gt; <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',1)\">",fileurl_parent,week, month, day, year, time, fileurl, name, name, name);
								websWrite(stream,"<hr><pre><br><br>    %-4s,%-4s%-3s%-5s,%-9s    &lt;dir&gt; <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',1)\">",week, month, day, year, time, fileurl, name, name, name);
							}
							else{	
								//websWrite(stream,"<hr><pre><a href='%s'>[To Parent Directory]</a><br><br>    %-4s,%-4s%-3s%-5s,%-9s    %s <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',0)\">",fileurl_parent,week, month, day, year, time, size, fileurl, name, name, name);
								websWrite(stream,"<hr><pre><br><br>    %-4s,%-4s%-3s%-5s,%-9s    %s <a href=\"%s/%s\">%s</a>&nbsp;<input name=\"delete\" id=\"delete\" type=\"button\" value=\"Delete\" onClick=\"DelFile(\'%s\',0)\">",week, month, day, year, time, size, fileurl, name, name, name);
							}
						}	
						list_count++;
						memset(listtemp,0,sizeof(listtemp));
					}//while
					if(list_count)//had list
						websWrite(stream, "%s","<br></pre><hr>");				
					fclose(fp1);
				}	
				memset(temp,0,sizeof(temp));
			}//	strstr(fileurl,mount)			
		}//while		
		fclose(fp);
		websWrite(stream, "%s", http_storage_tailer);		
	}
	if(fileurl_temp)			
		free(fileurl_temp);	
	/* Reset CGI */
	init_cgi(NULL);
}
	
static int do_uploadfile_post(char *url, FILE *stream, int len, char *boundary)
{
	char buf[1024] = {0},buf1[1024] = {0};		
	char filename[1024] = {0};
	char delpath[1024] = {0};
	char file[2048] = {0};
	char *start=NULL,*end=NULL, *flag=NULL;
	char *combuf=NULL;
	int deltype = 0;
	FILE *fp;
	int count = 0;
	
	assert(url);
	assert(stream);
	USB_STORAGE_DEBUG_MSG("%s: len=%d,url=%s,boundary=%s\n",__func__, len, url, boundary);
	memset(filepath,0,sizeof(filepath));	
	/* Skip First Boundary and get we need info */	
	while (len > 0) {
		memset(buf,0,sizeof(buf));
		if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
			printf("No boundary\n");
			return -1;
		}	
		len -= strlen(buf);
		//Get upload file path
		if ( !strncasecmp(buf, "Content-Disposition:", 20) && (strstr(buf, "name=\"upload_file_path\"")) ){
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No upload_file_path\n");
				return -1;
			}
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("filepath buf1=%s..\n",buf);	
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No upload_file_path\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("filepath buf2=%s..\n",buf);
			end = strstr(buf,"\r");
			*end='\0';
			strcpy(filepath,buf);
			USB_STORAGE_DEBUG_MSG("filepath=%s..\n",filepath);
		}
		//Get delete file
		if ( !strncasecmp(buf, "Content-Disposition:", 20) && (strstr(buf, "name=\"del_path\"")) ){
			USB_STORAGE_DEBUG_MSG("del_path buf=%s\n",buf);
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No del_path\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("del_path buf1=%s..\n",buf);	
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No del_path\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("del_path buf2=%s..\n",buf);			
			//[object]filename or [object HTMLInputElement]filename
			start = strstr(buf,"]");
			if(start){
				start++;
				end = strstr(buf,"\r");
				*end='\0';
				strcpy(delpath,start);
				USB_STORAGE_DEBUG_MSG("del_path=%s..\n",delpath);			
				if(strlen(delpath)){
					deltype ++;
					USB_STORAGE_DEBUG_MSG("Had delpath deltype=%d\n",deltype);				
				}	
			}
			else{
				end = strstr(buf,"\r");
				*end='\0';
				strcpy(delpath,buf);
				USB_STORAGE_DEBUG_MSG("del_path=%s..\n",delpath);
			}			
		}
		//Get delete type
		if ( !strncasecmp(buf, "Content-Disposition:", 20) && (strstr(buf, "name=\"del_type\"")) ){
			char deltmp[1024] = {0};
			USB_STORAGE_DEBUG_MSG("del_type buf=%s\n",buf);
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No del_type\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("del_type buf1=%s..\n",buf);	
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No del_path\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("del_type buf2=%s..\n",buf);
			end = strstr(buf,"\r");
			*end='\0';
			strcpy(deltmp,buf);
			USB_STORAGE_DEBUG_MSG("del_type=%s..\n",deltmp);
			if(strlen(deltmp)){
				deltype = deltype+atoi(deltmp);
				USB_STORAGE_DEBUG_MSG("Had del_type deltype=%d\n",deltype);				
			}	
		}
		//Get file name
		if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"file\"")){
			USB_STORAGE_DEBUG_MSG("filename buf=%s\n",buf);
			start=strstr(buf,"filename=");
			USB_STORAGE_DEBUG_MSG("filename start=%s\n",start);	
			if(start)
				start=strstr(start,"\"");
			if(start)
				start++;
			flag = strrchr(start, '\\');
			if(flag){
				start = strrchr(start, '\\');
				start++;
			}
			USB_STORAGE_DEBUG_MSG("filename start=%s..\n",start);		
			end = strstr(start,"\"");
			*end='\0';
			strcpy(filename,start);
			USB_STORAGE_DEBUG_MSG("filename=%s..\n",filename);
			break;
		}		
	}//while
	
	if(deltype){//do del
		int Temp_space_flag = 0, Temp_space_count = 0;		
		char *file_sp=NULL, *file_sp_temp=NULL;
		if(!strlen(delpath)){
			printf("Delete no file find\n");
			return -1;
		}	
		sprintf(file, "%s/%s", filepath, delpath);
		
		if(!strlen(file)){
			printf("Delete no path and file find\n");
			return -1;
		}
		//check Space and special char
		file_sp = strdup(file);
		Temp_space_flag=StrSpaceDecode(file,file_sp);
		if(Temp_space_flag){
			USB_STORAGE_DEBUG_MSG("file_sp=%s,Temp_space_flag=%d\n",file_sp,Temp_space_flag);
			file_sp_temp = (char *)malloc(strlen(file_sp)+Temp_space_flag+2);
			if(file_sp_temp){			
				Temp_space_count=StrSpaceEncode(file_sp, file_sp_temp);
				if(Temp_space_count!=Temp_space_flag)
				printf("Temp_space_count=%d != Temp_space_flag=%d\n",Temp_space_count, Temp_space_flag);
				USB_STORAGE_DEBUG_MSG("file_sp_temp=%s\n",file_sp_temp);				
			}
			else{
				printf("file_sp_temp malloc fail\n");
				return -1;
			}	
			file_sp = file_sp_temp;
		}			
		while (len > 0) {			
			memset(buf,0,sizeof(buf));			
			if(len < 1024){ 
					count = safe_fread(buf, 1, len, stream);
					len -= count;	
					USB_STORAGE_DEBUG_MSG("file len=%d\n",len);
					//USB_STORAGE_DEBUG_MSG("file buf=%s..\n",buf);			
					if( (len != 0) ){
						printf("fread file fail !!\n");
						free(file_sp_temp);
						return -1;
					}
					count = 0; //-= (strlen(boundary)+8);
					USB_STORAGE_DEBUG_MSG("file count=%d\n",count);
			}	
			else{
				count = safe_fread(buf, 1, sizeof(buf), stream);
				len -= count;	
				USB_STORAGE_DEBUG_MSG("file len1=%d\n",len);
				//USB_STORAGE_DEBUG_MSG("file buf1=%s..\n",buf);			
				if(count < 1024 && (len != 0) ){
					printf("fread file fail !!\n");
					free(file_sp_temp);
					return -1;
				}	
			}				
		}//while
		if(deltype > 1){//dir
			USB_STORAGE_DEBUG_MSG("Del Dir=%s..\n",file_sp);
			//_system("rm -rf %s", file);
			combuf = (char *)malloc(strlen(file_sp)+100);
			if(combuf){
				sprintf(combuf,"rm -rf %s",file_sp);
				USB_STORAGE_DEBUG_MSG("####combuf####=%s..\n",combuf);
				system(combuf);
				free(combuf);
			}
			else{
				printf("combuf malloc fail\n");
				free(file_sp_temp);
				return -1;	
			}					
		}else{//file
			USB_STORAGE_DEBUG_MSG("Del File=%s..\n",file_sp);
			//_system("rm -f %s", file);
			combuf = (char *)malloc(strlen(file_sp)+100);
			if(combuf){
				sprintf(combuf,"rm -f %s",file_sp);
				system(combuf);
				free(combuf);
			}
			else{
				printf("combuf malloc fail\n");
				free(file_sp_temp);
				return -1;	
			}	
		}		
		free(file_sp_temp);
	}
	else{//do upload
		if(!strlen(filename)){
			printf("Upload no file find\n");
			return -1;
		}	
		sprintf(file, "%s/%s", filepath, filename);
		if(!strlen(file)){
			printf("Upload no path and file find\n");
			return -1;
		}			
		/* Skip Content-Type */
		while (len > 0) {
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No Content-Type\n");
				return -1;
			}	
			len -= strlen(buf);
			if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
				break;
		}//while
		USB_STORAGE_DEBUG_MSG("file=%s,len=%d\n", file, len);
		fp = fopen(file, "w+");
		if(fp){		
			while (len > 0) {			
				memset(buf,0,sizeof(buf));			
				if(len < 1024){ 
					count = safe_fread(buf, 1, len, stream);
					len -= count;	
					USB_STORAGE_DEBUG_MSG("file len=%d\n",len);
					//USB_STORAGE_DEBUG_MSG("file buf=%s..\n",buf);			
					if( (len != 0) ){
						printf("fread file fail !!\n");
						return -1;
					}
					count -= (strlen(boundary)+8);
					USB_STORAGE_DEBUG_MSG("file count=%d\n",count);
				}	
				else{
					count = safe_fread(buf, 1, sizeof(buf), stream);
					len -= count;	
					USB_STORAGE_DEBUG_MSG("file len1=%d\n",len);
					//USB_STORAGE_DEBUG_MSG("file buf1=%s..\n",buf);			
					if(count < 1024 && (len != 0) ){
						printf("fread file fail !!\n");
						return -1;
					}	
				}		
				if( safe_fwrite(buf,1,count,fp) != count){
					printf("fwrite file fail !!\n");
					return -1;
				}	
			}//while
			fflush(fp);
			fclose(fp);
		}		
		else{
			USB_STORAGE_DEBUG_MSG("open %s file fail\n",file);
			return -1;
		} 
	}	
	USB_STORAGE_DEBUG_MSG("%s: finished \n",__func__);
	return 0;	
}

static int do_creatdir_post(char *url, FILE *stream, int len, char *boundary)
{
	char buf[1024] = {0},buf1[1024] = {0};		
	char dirname[1024] = {0};
	char dir[2048] = {0};
	char *start=NULL,*end=NULL,*dirname_sp=NULL,*dirname_sp_temp=NULL;
	char *combuf=NULL;
	FILE *fp;
	int count = 0,Temp_space_flag = 0,Temp_space_count = 0; 	
	
	assert(url);
	assert(stream);
	USB_STORAGE_DEBUG_MSG("%s: len=%d,url=%s,boundary=%s\n",__func__, len, url, boundary);
	memset(filepath,0,sizeof(filepath));	
	/* Skip First Boundary and get we need info */	
	while (len > 0) {
		memset(buf,0,sizeof(buf));
		if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
			printf("No boundary\n");
			return -1;
		}	
		len -= strlen(buf);
		//Get creat_directory path
		if ( !strncasecmp(buf, "Content-Disposition:", 20) && (strstr(buf, "name=\"creat_directory_path\"")) ){
			USB_STORAGE_DEBUG_MSG("dirpath buf=%s\n",buf);
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No creat_directory_path\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("dirpath buf1=%s..\n",buf);	
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No creat_directory_path\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("dirpath buf2=%s..\n",buf);
			end = strstr(buf,"\r");
			*end='\0';
			strcpy(filepath,buf);
			USB_STORAGE_DEBUG_MSG("filepath=%s..\n",filepath);
		}
		//Get directory name
		if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"directory\"")){
			USB_STORAGE_DEBUG_MSG("dirname buf=%s\n",buf);
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No directory\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("dirname buf1=%s..\n",buf);	
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No directory\n");
				return -1;
			}	
			len -= strlen(buf);
			USB_STORAGE_DEBUG_MSG("dirname buf2=%s..\n",buf);
			end = strstr(buf,"\r");
			*end='\0';
			strcpy(dirname,buf);
			USB_STORAGE_DEBUG_MSG("dirname=%s..\n",dirname);
		}		
	}//while
	
	{//do Creat
		if(!strlen(dirname)){
			printf("Upload no directory find\n");
			return -1;
		}	
/*
*	Date: 2010-08-09
*   Name: Ken Chiang
*   Reason: modify for usb storage http uplaod/download feature can't support special char in creat dir.
*   Notice :
*/
		//check Space and special char
		dirname_sp = strdup(dirname);
		Temp_space_flag=StrSpaceDecode(dirname,dirname_sp);
		if(Temp_space_flag){
			USB_STORAGE_DEBUG_MSG("dirname_sp=%s,Temp_space_flag=%d\n",dirname_sp,Temp_space_flag);
			dirname_sp_temp = (char *)malloc(strlen(dirname_sp)+Temp_space_flag+2);
			if(dirname_sp_temp){			
				Temp_space_count=StrSpaceEncode(dirname_sp, dirname_sp_temp);
				if(Temp_space_count!=Temp_space_flag)
				printf("Temp_space_count=%d != Temp_space_flag=%d\n",Temp_space_count, Temp_space_flag);
				USB_STORAGE_DEBUG_MSG("dirname_sp_temp=%s\n",dirname_sp_temp);				
			}
			else{
				printf("dirname_sp_temp malloc fail\n");
				return -1;
			}	
			dirname_sp = dirname_sp_temp;
		}		
		sprintf(dir, "%s/%s", filepath, dirname_sp);
		if(!strlen(dir)){
			printf("Upload no path and directory find\n");
			return -1;
		}			
		/* Skip Content-Type */
		while (len > 0) {
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No Content-Type\n");
				return -1;
			}	
			len -= strlen(buf);
			if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
				break;
		}//while
		USB_STORAGE_DEBUG_MSG("Creat dir=%s,len=%d\n", dir, len);
		//_system("mkdir %s", dir);
		combuf = (char *)malloc(strlen(dir)+100);
		if(combuf){
			sprintf(combuf,"mkdir \"%s\"",dir);
			system(combuf);
			free(combuf);
		}
		else{
			printf("combuf malloc fail\n");
			return -1;	
		}			
	}	
	USB_STORAGE_DEBUG_MSG("%s: finished \n",__func__);
	return 0;
	
}
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file can uesd https.
*   Notice :
*/
static int do_upload_cgi(char *url, FILE *stream, int flag, int lan_request, char *wan_ipaddrport)
{
	assert(url);
	assert(stream);	
	USB_STORAGE_DEBUG_MSG("%s: filepath=%s,flag=%d\n",__func__ , filepath, flag);	
	if(flag){//fail		
		send_headers( 200, "Ok", no_cache, "text/html" );
		websWrite(stream, "%s", http_storage_header);
		if(!lan_request){				
#if defined(HAVE_HTTPS)
				if(openssl_request)
					websWrite(stream, https_storage_back_script, wan_ipaddrport, filepath);
				else	
#endif		
					websWrite(stream, http_storage_back_script, wan_ipaddrport, filepath);
		}
		else{
#if defined(HAVE_HTTPS)
				if(openssl_request)
					websWrite(stream, https_storage_back_script, nvram_safe_get("lan_ipaddr"), filepath);
				else	
#endif	
					websWrite(stream, http_storage_back_script, nvram_safe_get("lan_ipaddr"), filepath);
		}		
		websWrite(stream, http_storage_back_body, "Upload File Fail !!");
		websWrite(stream, "%s", http_storage_tailer);	
	}
	else{
		send_headers( 200, "Ok", no_cache, "text/html" );
		websWrite(stream, "%s", http_storage_header);	 
		if(!lan_request){			
#if defined(HAVE_HTTPS)
				if(openssl_request)
					websWrite(stream, https_storage_back_script, wan_ipaddrport, filepath);
				else	
#endif	
					websWrite(stream, http_storage_back_script, wan_ipaddrport, filepath);
		}
		else{
#if defined(HAVE_HTTPS)
				if(openssl_request)
					websWrite(stream, https_storage_back_script, nvram_safe_get("lan_ipaddr"), filepath);
				else	
#endif	
					websWrite(stream, http_storage_back_script, nvram_safe_get("lan_ipaddr"), filepath);
		}		
		websWrite(stream, http_storage_back_body, "Upload File Success !!");		
		websWrite(stream, "%s", http_storage_tailer);					
	}	
	init_cgi(NULL);
	USB_STORAGE_DEBUG_MSG("%s: finished \n",__func__);
	return;
}


/*
*	Date: 2010-08-04
*   Name: Ken Chiang
*   Reason: modiyf for usb storage upload and download file don't login issue.
*   Notice :
*/
static int usb_storge_http_login_check(void)
{
	/* check if logout request */
	if( update_logout_list(&con_info.mac_addr[0], GET_FLG, NULL_LIST) == 1 &&
		update_logout_list(&con_info.mac_addr[0], GET_PAG, NULL_LIST) == 1 )
	{
		USB_STORAGE_DEBUG_MSG("Find user in login list[ever login], but already logout, so need to re-login\n");
		//file = "login.asp";
		return -1;
	}
	else
	{
		DEBUG_MSG("User maybe [already login] or [never login]\n");
		if( update_logout_list(&con_info.mac_addr[0], GET_FLG, NULL_LIST) == -1 && 
			update_logout_list(&con_info.mac_addr[0], GET_PAG, NULL_LIST) == -1 )
		{
			USB_STORAGE_DEBUG_MSG("Can not find user in login list => never login\n");
			//file = "login.asp";
			return -2;
		}
		else
		{
			USB_STORAGE_DEBUG_MSG("User is in login list\n");
		}
	}
	return 0;
}

/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: Add for usb storage upload and download file remote side feature.
*   Notice :
*/
void usb_storge_http_get_wanip(char *wan_ipaddr)
{
	char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;
	char wan_ip[192]={0};
	/* wan info */
	memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
	getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);	    
	USB_STORAGE_DEBUG_MSG("wanip=%s\n",wanip);
	strcpy(wan_ipaddr,wanip);
	USB_STORAGE_DEBUG_MSG("wan_ipaddr=%s\n",wan_ipaddr);	
	return ;
}
#endif//USB_STORAGE_HTTP_ENABLE

#ifdef MIII_BAR
void do_miii_download_file(char *path, FILE *stream)
{
	FILE *fp;
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *p = NULL;
	char filenames[256];
	char base_path[256] ={0};
	char targetname[256] = {0};
	char temppath[256] = {0};
	char tmpfilenames[256] ={0}; 
	int c;
	char *pp =NULL;
	char tmep_pp[256]={0};
	char image_path[256] = {0};
	char isdir_image_path[1024] = {0};
	char more_file_path[1024] = {0};
	char sendtype[128] = {0};
	char senddispo[128] = {0};
	char sendtran[128] = {0};
	char morefile[1024] ={0};
	char onlyfile[1024] ={0};
	int total_size ;
	struct stat sizebuf;
	int flag ;
	int moreflag ;
	
	DEBUG_MSG("%s:path=%s\n",__func__,path);
	
	if(miiicasa_set_folder() == -1) return;
	        
	//GET /ws/api/downloadFiles?tok=4d1060fb8e6ea:3b9a05eb1cfcf6d66c5258e7ee716ca5&base_path=%2FUSB_USB_FLASH_DRIVE_0380206_1%2FmiiiCasa_Photos%2Ftest&filenames=%5B%22Hydrangeas.jpg%22%5D&targetname=miiiCasa_1292919049111.zip&tx=129291904911187746
	// /var/usb/sda/sda1/test.jpg
	//printf("%s:path=%s\n",__func__,path);
	memset(filenames, 0, 256);	
	
	pattern = malloc(strlen(path)+1);
	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));
	p_pattern = strchr (pattern, '?') + 1;
	// tok
	p = strtok(p_pattern,"&");
	
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"filenames"))
		{
			strcpy(filenames,p+10);
			DEBUG_MSG("filenames=%s\n",filenames);
			strcpy(tmpfilenames ,url_decode(filenames));
			DEBUG_MSG("tmpfilenames=%s\n",tmpfilenames);
			//tmpfilenames=\[\"Hydrangeas.jpg\"\]	
		}	
		else if(strstr(p,"base_path"))
		{
			strcpy(base_path,p+10);
			DEBUG_MSG("base_path=%s\n",base_path);
			char *str1 = str_replace(base_path,"%2F","/");	
			strcpy(temppath ,str1);
			DEBUG_MSG("temppath=%s\n",temppath);
		}
		else if(strstr(p,"targetname"))
		{
			strcpy(targetname,p+11);	
			DEBUG_MSG("targetname=%s\n",targetname);
		}
	}	
		
	if(strstr(temppath,"\/USB_USB2FlashStorage_45c0216_1"))
	{
		char *str1 = str_replace(temppath,"\/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);	
		strcpy(filepath,str1);
		if(str1) free(str1);
		str1 = str_replace(temppath,"\/USB_USB2FlashStorage_45c0216_1",miii_usb_folder_t);
		memset(filename, 0, sizeof(filename));	
		strcpy(filename,str1);
		if(str1) free(str1);
	}	
				
	pp=(strstr(tmpfilenames,"\"")+1);
	DEBUG_MSG("pp=%s\n",pp);
	strcpy(tmep_pp,pp);
	DEBUG_MSG("tmep_pp=%s\n",tmep_pp);
	tmep_pp[strlen(tmep_pp)-4]=0;
	DEBUG_MSG("tmep_pp=%s\n",tmep_pp);				
						
	strcpy(image_path,tmep_pp);
	DEBUG_MSG("image_path=%s\n",image_path);
	DEBUG_MSG("1 tmep_pp =%s\n" ,tmep_pp);

        if(strstr(tmep_pp,"\,"))
	{
		//char str3[1024] = {0} ;
		moreflag = 1;
		//sprintf(str3," %s/",filepath);
		//DEBUG_MSG("str3 = %s\n",str3);
		DEBUG_MSG("tmep_pp=%s\n",tmep_pp);
		DEBUG_MSG("image_path = %s\n",tmep_pp);
		char *str2 = str_replace(image_path,"\\\"\\\,\\\""," ");
		DEBUG_MSG("## str2 = %s ##\n",str2);	
		strcpy(morefile,str2);
		DEBUG_MSG("## morefile = %s ##\n",morefile);
		if(str2) free(str2);	
	}else{
		
		moreflag = 0;
		sprintf(onlyfile,"%s/%s",filename,image_path);
		DEBUG_MSG("onlyfile=%s\n",onlyfile);	
	}
	
	char path_pp[256]={0};
	strcpy(path_pp,tmep_pp);

	stat(image_path,&sizebuf);
	total_size = sizebuf.st_size;
	DEBUG_MSG("image_path size = %d\n",total_size);
						
	FILE *fp2 = NULL;
	char buf2[256] = {0};
	char isdir[256] = {0};
        if(strstr(tmep_pp,"\,"))
		sprintf(buf2,"ls -ld %s/%s",filepath,morefile);
        else			
		sprintf(buf2,"ls -ld %s/%s",filepath,tmep_pp);
	DEBUG_MSG("buf2=%s\n",buf2);
	if ((fp2 = popen(buf2, "r")) == NULL)
		return -1;
	fgets(isdir, sizeof(isdir), fp2);
	DEBUG_MSG("isdir = %s\n",isdir);
	pclose(fp2);
				
	DEBUG_MSG("isdir tmep_pp=%s\n",tmep_pp);		
					
	if(strstr(isdir,"drw")){
		flag = 1;
				
	        DEBUG_MSG("isdir tmep_pp=%s\n",tmep_pp);
	        DEBUG_MSG("xxxx image_path=%s\n",image_path);
	        DEBUG_MSG("isdir filepath=%s\n",filepath);
	        DEBUG_MSG("isdir path_pp=%s\n",path_pp);
	        DEBUG_MSG("isdir targetname=%s\n",targetname);	
					
	        char zippath[1024] = {0};
	        if(strstr(image_path,"\,"))	{
		        sprintf(zippath,"cd %s ; zip -r %s %s",filepath,targetname,morefile);
		        DEBUG_MSG("morefile zippath=%s\n",zippath);
	        }else{
	                sprintf(zippath,"cd %s ; zip -r %s %s",filepath,targetname,path_pp);
		        DEBUG_MSG("zippath=%s\n",zippath);
	        }
	        system(zippath);
						
	        //_system("cd %s ; zip -r %s /%s",filepath,targetname,path_pp);
						
	        sprintf(isdir_image_path,"%s/%s",filename,targetname);
	        DEBUG_MSG("isdir_image_path=%s\n",isdir_image_path);
						
	        if (!(fp = fopen(isdir_image_path, "r"))){
	                printf("\"%s\" can't open\n",isdir_image_path);
		        return 0;
	        }

	        DEBUG_MSG("flag=%d\n",flag);
	        sprintf(sendtran,"%s","Transfer-Encoding: chunked");	
	        sprintf(senddispo," filename=\"%s\"",targetname);	
	        sprintf(sendtype,"%s","application/zip");
						
   	        miii_send_headers( 200, "Ok",senddispo,no_cache,total_size,sendtype,sendtran,1);
   	        while ((c = fgetc(fp)) != EOF) {
		        wfputc(c, stream);
	        }
	        fclose(fp);
	        _system("rm -rf %s",isdir_image_path);				
						
	}else{	
		flag = 0;				
		DEBUG_MSG("flag=%d\n",flag);		
		if(strstr(image_path,"\,"))
		{			
		        DEBUG_MSG("777 filepath=%s\n",filepath);
			DEBUG_MSG("777 targetname=%s\n",targetname);	
			DEBUG_MSG("777 morefile=%s\n",morefile);
			//_system("zip -j %s/%s /%s",filepath,targetname,morefile);
			_system("cd %s ; zip -r %s %s",filepath,targetname,morefile);
			sprintf(more_file_path,"%s/%s",filename,targetname);
			DEBUG_MSG("more_file_path=%s\n",more_file_path);
						
			if (!(fp = fopen(more_file_path, "r"))){
				printf("\"%s\" can't open\n",more_file_path);
				return 0;
			}

			DEBUG_MSG("flag=%d\n",flag);
			sprintf(sendtran,"%s","Transfer-Encoding: chunked");	
			sprintf(senddispo," filename=\"%s\"",targetname);	
			sprintf(sendtype,"%s","application/zip");
						
   			miii_send_headers( 200, "Ok",senddispo,no_cache,total_size,sendtype,sendtran,1);
   			while ((c = fgetc(fp)) != EOF) {
				wfputc(c, stream);
			}
			fclose(fp);
			_system("rm -rf %s",more_file_path);
					
		}
						
		if (!(fp = fopen(onlyfile, "r"))){					        
			DEBUG_MSG("\"%s\" can't open\n",image_path);
			return 0;
		}
		DEBUG_MSG("2 tmep_pp =%s\n" ,tmep_pp);	
		DEBUG_MSG("\nfilepath %s, not found pattern\n",image_path);
						 
		sprintf(senddispo,"attachment; filename=\"%s\"",path_pp);
		DEBUG_MSG("senddispo = %s \n",senddispo);
		DEBUG_MSG("3 tmep_pp =%s\n" ,tmep_pp);
				
		sprintf(sendtype,"application/octet-stream; name=\"%s\"",path_pp);
		DEBUG_MSG("4 tmep_pp =%s\n" ,tmep_pp);
		DEBUG_MSG("sendtype = %s \n",sendtype);
		//send_headers( 200, "Ok", no_cache,sendtype);
		miii_send_headers( 200, "Ok",senddispo,no_cache,total_size,sendtype,"NULL",0);
										
		while ((c = fgetc(fp)) != EOF) {
			wfputc(c, stream);
		}
						
		fclose(fp);
	}	
        DEBUG_MSG("%s: finsh\n",__func__);
}

void do_miii_get_file(char *path, FILE *stream)
{
	FILE *fp;
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *p = NULL;
	char callback[256];
	char fullfilename[256];
	int c;
	//do_miii_get_file:path=/ws/api/getFile?tok=4c5fbaf0796ba:1a13ce642f0b06727ec7d8992b3de782&fullfilename=/USB_USB2FlashStorage_45c0216_1/test.jpg
	// /var/usb/sda/sda1/test.jpg
	DEBUG_MSG("%s:path=%s\n",__func__,path);
	
	if(miiicasa_set_folder() == -1) return;
		
	memset(callback, 0, 256);	
	memset(fullfilename, 0, 256);	
	
	pattern = malloc(strlen(path)+1);
	if (pattern == NULL)
			return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));
	p_pattern = strchr (pattern, '?') + 1;
	// tok
	p = strtok(p_pattern,"&");
	
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"fullfilename"))
		{
			strcpy(fullfilename,p+13);
			DEBUG_MSG("fullfilename=%s\n",fullfilename);
		}	
	}	
		
	if(strstr(fullfilename,"/USB_USB2FlashStorage_45c0216_1"))
	{
		char *str1 = str_replace(fullfilename,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder_t);	
		strcpy(filepath,str1);
		if(str1) free(str1);
	}	
		
	char dirbasepath[256] ;
	char buf1[256] = {0};
	FILE *fp2 = NULL;
	memset(dirbasepath, '\0', sizeof(dirbasepath));
	sprintf(buf1,"basename %s",fullfilename);
	if ((fp2 = popen(buf1, "r")) == NULL)
		return -1;
	fgets(dirbasepath, sizeof(dirbasepath), fp2);
	pclose(fp2);
	dirbasepath[strlen(dirbasepath) - 1] = 0;
	DEBUG_MSG("dirbasepath = %s\n",dirbasepath);
				
	if (!(fp = fopen(filepath, "r"))){
		printf("\"%s\" can't open\n",filepath);
		return 0;
	}
	DEBUG_MSG("\nfilepath %s, not found pattern\n",filepath);
	char sendtype[128] = {0};
	char senddispo[128] = {0};
	sprintf(senddispo,"attachment; filename=\"%s\"",dirbasepath);
	sprintf(sendtype,"application/octet-stream; name=\"%s\"",dirbasepath);
	DEBUG_MSG("sendtype = %s ",sendtype);
	//send_headers( 200, "Ok", no_cache,sendtype);
	//miii_send_headers( 200, "Ok",senddispo,sendtype);
			
					
	while ((c = fgetc(fp)) != EOF) {
		wfputc(c, stream);
	}
	DEBUG_MSG("%s: finsh\n",__func__);
	fclose(fp);
}


static int do_miii_uploadfile_post(char *url, FILE *stream, int len, char *boundary)
{
	char buf[1024] = {0},buf1[1024] = {0};		
	//char filename[1024] = {0};
	char delpath[1024] = {0};
	char file[2048] = {0};
	char *start=NULL,*end=NULL;
	char *combuf=NULL;
	int deltype = 0;
	FILE *fp;
	int count = 0;  	

 	assert(url);
	assert(stream);
	DEBUG_MSG("%s: len=%d,url=%s,boundary=%s\n",__func__, len, url, boundary);
	
	if(miiicasa_set_folder() == -1) return -1;
	
	memset(filepath,0,sizeof(filepath));	
		/* Skip First Boundary and get we need info */	
	while (len > 0) {
		memset(buf,0,sizeof(buf));
		if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
			printf("No boundary\n");
			return -1;
		}	
		len -= strlen(buf);
		//Get upload file path
		if ( !strncasecmp(buf, "Content-Disposition:", 20) && (strstr(buf, "name=\"path\"")) ){
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No upload_file_path\n");
				return -1;
			}	
			len -= strlen(buf);
			DEBUG_MSG("filepath buf=%s..\n",buf);	
			memset(buf,0,sizeof(buf));
			if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
				printf("No upload_file_path\n");
				return -1;
			}	
			len -= strlen(buf);
			DEBUG_MSG("filepath buf2=%s..\n",buf);
			end = strstr(buf,"\r");
			*end='\0';
			if(strstr(buf,"/USB_USB2FlashStorage_45c0216_1"))
			{
				char *str1 = str_replace(buf,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);	
				strcpy(filepath,str1);
				if(str1) free(str1);
				str1 = str_replace(buf,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder_t);	
				strcpy(buf1,str1);
				if(str1) free(str1);
			}else{
				strcpy(filepath,buf);
			}

		}
	//Get file name
		if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"uploadedfile\"")){
			start=strstr(buf,"filename=");
			if(start)
				start=strstr(start,"\"");
			if(start)
				start++;
			end = strstr(start,"\"");
			*end='\0';
			strcpy(filename,start);
			break;
		}	
	}

	//do upload
	if(!strlen(filename)){
		printf("Upload no file find\n");
		return -1;
	}
	if(strcmp(filepath,"/")==0){
		strcpy(filepath,miii_usb_folder);
		strcpy(buf1,miii_usb_folder_t);
	}
	sprintf(file, "%s/%s", buf1, filename);
	if(!strlen(file)){
		printf("Upload no path and file find\n");
		return -1;
	}
	/* Skip Content-Type */
	while (len > 0) {
		memset(buf,0,sizeof(buf));
		if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream)){
			printf("No Content-Type\n");
			return -1;
		}	
		len -= strlen(buf);
		if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
			break;
	}//while			
	DEBUG_MSG("file=%s,len=%d\n", file, len);	
	fp = fopen(file, "w+");
	if(fp){		
		while (len > 0) {			
			memset(buf,0,sizeof(buf));			
			if(len < 1024){ 
				count = safe_fread(buf, 1, len, stream);
				len -= count;	
				//USB_STORAGE_DEBUG_MSG("file buf=%s..\n",buf);			
				if( (len != 0) ){
					printf("fread file fail !!\n");
					return -1;
				}
				count = 0; // -= (strlen(boundary)+8);
			}	
			else{
				count = safe_fread(buf, 1, sizeof(buf), stream);
				len -= count;	
				if(count < 1024 && (len != 0) ){
					printf("fread file fail !!\n");
					return -1;
				}	
			}		
			if( safe_fwrite(buf,1,count,fp) != count){
				printf("fwrite file fail !!\n");
				return -1;
			}	
		}//while
		fflush(fp);
		fclose(fp);
	}	
	else{
		printf("open %s file fail\n",file);
		return -1;
	} 
	DEBUG_MSG("%s: finished \n",__func__);
	return 0;	
}


static int do_miii_upload_cgi(char *url, FILE *stream, int flag, int lan_request, char *wan_ipaddrport)
{
	char miii_response[1024];
	FILE *fp = NULL;
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char filesize[32]={0};
	char listtemp[1024]={0};
	char buf[50]={0};
 	char filesystem[128]={0},linenum[12]={0},user1[12]={0},user2[12]={0};
 	char month[32]={0},day[32]={0},week[32]={0},year[32]={0},time[32]={0},filepwd[128]={0};
	int file_mtime = 0;

	//strcpy(str_errno,"");
	//strcpy(str_errmsg,"");
	//strcpy(uploadmtime,"");
	assert(url);
	assert(stream);	
	DEBUG_MSG("%s: filepath=%s,flag=%d,filename=%s\n",__func__ , filepath, flag,filename);	

	if(miiicasa_set_folder() == -1) return -1;
		
	sprintf(buf,"ls -el %s | grep %s >>  /var/tmp/mountusb.txt",filepath,filename);
	system (buf);
	fp = fopen("/var/tmp/mountusb.txt","r");
	while(fgets(listtemp,sizeof(listtemp),fp))
	{
		sscanf(listtemp, "%s %s %s %s %s %s %s %s %s %s %s", filesystem, linenum, user1, user2, filesize,week,month,day,time,year,filepwd);
  		DEBUG_MSG("listtemp=%s %s %s %s %s %s %s %s %s %s %s\n", 
  			filesystem, linenum, user1,user2,filesize,week,month,day,time,year,filepwd);
		DEBUG_MSG("filesize=%s\n",filesize);
		file_mtime = GMTtoEPOCH(year,month,day,time);
		DEBUG_MSG("file_mtime=%d\n",file_mtime);
		//	strcpy(filesize,totalsize);
		//	uploadmtime = file_mtime;
	}
	fclose(fp);

	
	if(flag){//fail	
		strcpy(str_status,"fail");
		send_headers( 200, "Ok", no_cache, "application/json; charset=utf-8" );
		sprintf(miii_response, 	"{\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\",\"mtime\":%d,\"size\":\"%s\"};" 
																	,str_status,str_errno, str_errmsg, file_mtime, filesize);
		websWrite(stream, "%s", miii_response);
	}else{
		strcpy(str_status,"ok");
		send_headers( 200, "Ok", no_cache, "application/json; charset=utf-8" );
		sprintf(miii_response, 	"{\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\",\"mtime\":%d,\"size\":\"%s\"};"
																	,str_status,str_errno, str_errmsg, file_mtime, filesize);
		websWrite(stream, "%s", miii_response);								
	}
	init_cgi(NULL);
	USB_STORAGE_DEBUG_MSG("%s: finished \n",__func__);
	return;
}
#endif

static char *ddns_status(char *parse_value, char *status)
{
	FILE *fp;
	char wan_interface[8] = {};
	char web[] = "168.95.1.1";
	char *wan_proto = nvram_safe_get("wan_proto");

	if (strcmp(wan_proto, "pppoe") == 0 ||
		strcmp(wan_proto, "l2tp") == 0 ||
		strcmp(wan_proto, "pptp") == 0
#ifdef CONFIG_USER_3G_USB_CLIENT
		|| strcmp(wan_proto, "usb3g") == 0
#endif
		)
  		strcpy(wan_interface, "ppp0");
	else
		strcpy(wan_interface, nvram_safe_get("wan_eth"));

	if (nvram_match("ddns_enable", "1") != 0)
		strcpy(status, "not_configured");
	else {
		if (!read_ipaddr(wan_interface))
			strcpy(status, "disconnected");
		else {
			if (strcmp(get_ping_app_stat(web), "Success") != 0)
				strcpy(status, "disconnected");
			else {
				fp = fopen("/var/tmp/ddns_status", "r");
				if (fp) {
				 	fgets(parse_value, 128, fp);
                	fclose(fp);
					if (strstr(parse_value, "SUCCESS"))
						strcpy(status, "connected");
					else
						strcpy(status, "disconnected");
				}
        		else
					strcpy(status, "connected");
			}
		}
	}

	return status;
}

int is_wan_ip(const char *host)
{
	char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;
	char wan_ip[192]={0};
	char parse_ddns[128] = {0};
	char status[64] = {0};
	/* wan info */
	memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
	getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);	    
	if(strcmp(host, wan_ip) == 0)
		return 1;
		
	if(nvram_match("ddns_enable", "1") == 0){
		DEBUG_MSG("DDNS Enabled \n");
		ddns_status(parse_ddns, status);
		if(strstr(status, "connected"))		
			return 1;
		DEBUG_MSG("DDNS enabled but disconnect. \n");
	}
	
	return 0;
}

static int change_savecgi = 0;
static void handle_request(void){
#ifdef CONFIG_USER_FASTNAT_APPS
        char line[1460], *cur;
#else
        char line[1500], *cur;
#endif
        char *method, *path, *protocol, *authorization, *boundary, *content_type;
        char *cp;
        char *file,file_tmp[50];
        int len;
        struct mime_handler *handler;
        int cl = 0, unauth_flag = 0,flags;
        int pattern_found = 0;
        /* for check request timeout */
        int method_prefix;

#ifdef CONFIG_WEB_404_REDIRECT
        char host[256]={0};  // dns max length is 255
#endif

/*
*       Date: 2009-08-10
*   Name: Ken Chiang
*   Reason: Modify for GPL CmoCgi.
*   Notice :
*/
        int customer_handlers_chack_flag = 0;

#ifdef PURE_NETWORK_ENABLE
  /* for pure network initial */
        int ignore_password, pure_mode_enable;
        char *action_string, *if_modified_since;

        pure_mode_enable = ignore_password = 0;
        action_string = content_type = if_modified_since = NULL;
#endif

        /* Initialize the request variables. */
        authorization = boundary = NULL;
        bzero( line, sizeof (line) );

#if defined(HAVE_HTTPS)
        if (openssl_request) {
                /* Parse the first line of the request. */
                if (wfgets(line, sizeof(line), conn_fp) == (char *) 0) {
                        send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );
                        return;
                }
        }
        else
#endif
        {
        /* prevent httpd hold from no data request (like telnet 192.168.0.1 80)*/
        if(!checkQueryTimeout(&method_prefix))
                return;

        /* compose original request */
        if(!composeToOriinalRequest(method_prefix,line))
                return;
        }

        method = path = line;
        strsep(&path, " ");

        /* easy head format check (should be Method Path Proto,is like GET /a.asp HTTP/1.1)*/
        /* prvent "telnet 192.168.0.1 80" cause httpd "crash" */
        if(path == NULL)
                return;

        while (*path == ' ') path++;
        protocol = path;
        strsep(&protocol, " ");

        /* easy head format check (should be Method Path Proto,is like GET /a.asp HTTP/1.1)*/
        if(protocol == NULL)
                return;

        while (*protocol == ' ') protocol++;
        cp = protocol;
        strsep(&cp, " ");
        if ( !method || !path || !protocol ) {
                send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );
                return;
        }
        cur = protocol + strlen(protocol) + 1;
    
        /* Parse the rest of the request headers. */
/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
        while ( wfgets(cur, line + sizeof(line) - cur, conn_fp) != (char*) 0 ){
                if ( strcmp( cur, "\n" ) == 0 || strcmp( cur, "\r\n" ) == 0 ){
                        break;
                }else if ( strncasecmp( cur, "Authorization:", 14 ) == 0 ){
                        cp = &cur[14];
                        cp += strspn( cp, " \t" );
                        authorization = cp;
                        cur = cp + strlen(cp) + 1;
                }else if (strncasecmp( cur, "Content-Length:", 15 ) == 0) {
                        cp = &cur[15];
                        cp += strspn( cp, " \t" );
                        cl = strtoul( cp, NULL, 0 );
                        //content_length = cl;
                }else if ( strncasecmp( cur, "Content-Type:", 13 ) == 0 ){
                        cp = &cur[13];
                        cp += strspn( cp, " \t" );
                        content_type = cp;
                        boundary = 0;
                        if (strncasecmp(cp, "multipart/form-data;", 20) == 0){
                                char *cp1;
                                cp1 = &cp[20];
                                cp1 += strspn( cp1, " \t" );
                                if (strncasecmp(cp1, "boundary=", 9) == 0){
                                        cp = &cp1[9];
                                        boundary = cp;
                                        for( cp = cp + 9; *cp && *cp != '\r' && *cp != '\n'; cp++ );
                                        *cp = '\0';
                                        cur = ++cp;
                                }
                        }
                }
#ifdef PURE_NETWORK_ENABLE
                else if ( strncasecmp( cur, "If-Modified-Since:", 18 ) == 0 ){
                        DEBUG_MSG("If-Modified-Since\n");
                        cp = &line[18];
                        cp += strspn( cp, " \t" );
                        if_modified_since = cp;
                }
                else if (strncasecmp( cur, "SOAPAction:", 11 ) == 0 ){
                        DEBUG_MSG("SOAPAction\n");
                        cp = &cur[11];
                        cp += strspn( cp, " \t" );
                        if(strstr(cp, "HNAP1")){
                                DEBUG_MSG("HNAP1 (pure mode)\n");
                                action_string = (char *) malloc(1024);
                                memset(action_string, 0, 1024);
                                memcpy(action_string, cp, strlen(cp));
                                pure_mode_enable = 1;
                        }
                }
#endif

#ifdef CONFIG_WEB_404_REDIRECT
                else if (lan_request) {
                        if (strncasecmp( cur, "Host:", 5 ) == 0 ){
                                int len;
#define isspace(c) ((c)==' ' || (c)=='\t' || (c)=='\r' || (c)=='\n')
                                DEBUG_MSG("Host\n");
                                cp = &cur[5];
                                while (isspace(*cp)) cp++;
                                strcpy(host,cp);
                                len=strlen(host)-1; // skip '\0'
                                while (isspace(host[len])) len--;
                                host[len+1]='\0';
                        }
                }
#endif
        }

        if ( strcasecmp( method, "get" ) != 0 && strcasecmp(method, "post") != 0 ) {
                send_error( 501, "Not Implemented", (char*) 0, "That method is not implemented." );
                return;
        }

        if ( path[0] != '/' ) {
                send_error( 400, "Bad Request", (char*) 0, "Bad filename." );
                return;
        }
#ifdef CONFIG_WEB_404_REDIRECT
        if (lan_request) {
                DEBUG_MSG("http request host=%s, lan_ipaddr=%s, lan_device_name=%s\n", host,lan_ip,device_name);
#ifdef IPv6_SUPPORT
                if (!ipv6_request) {
#endif
/*
        Date: 2010-2-9
        Name: Ken_Chiang
        Reason: Modify to fix the 404 redirect can't support new feature the http://router.dlink.com redirect to device login page issue.
        Notice :
*/
#ifdef CONFIG_USER_DLINK_ROUTER_URL

                if ( !(strcmp(host, lan_ip)==0 || strcmp(host, device_name)==0 || strcmp(host, "router.dlink.com")==0) )
                {
                	if(!is_wan_ip(host)){
                	memset(file_tmp, 0, 50);
                	strcpy(file_tmp, "error-404.htm");
                        file = file_tmp;
                        goto get_404_file;
                    }
                }
#else
                if ( !(strcmp(host, lan_ip)==0 || strcmp(host, device_name)==0))
                {
                	memset(file_tmp, 0, 50);
                	strcpy(file_tmp, "error-404.htm");
                        file = file_tmp;
                        goto get_404_file;
                }
#endif
#ifdef IPv6_SUPPORT
                } // end if (!ipv6_request)
#endif
        }
#endif

        file = &(path[1]);
        DEBUG_MSG("path = %s\n", path);
        DEBUG_MSG("file 1 = %s\n", file);

        len = strlen( file );
        if ( file[0] == '/' || strcmp( file, ".." ) == 0 || strncmp( file, "../", 3 ) == 0 ||
                 strstr( file, "/../" ) != (char*) 0 || strcmp( &(file[len-3]), "/.." ) == 0 )
        {
                send_error( 400, "Bad Request", (char*) 0, "Illegal filename." );
                return;
        }

#ifdef PURE_NETWORK_ENABLE
  /* for pure mode GetPage (test-tool version 4.0 ) */
        DEBUG_MSG("authorization=%s\n", authorization);
        DEBUG_MSG("action_string=%s\n", action_string);
        DEBUG_MSG("file 2 = %s\n", file);

        if (authorization == NULL && action_string != NULL)
        {
                /* Allow SoapAction: GetDeviceSettings get information without Authorization.*/
                if (strstr(action_string,"GetDeviceSettings") != NULL )
                {
                        DEBUG_MSG("GetDeviceSettings\n");
                        ignore_password = 1;
                }
        		else if (strstr(action_string,"SetDeviceSettings"))
                {
                        ignore_password = 0;
                }
                else
                        ignore_password = 1; // Allow unknow soapactions , for TestDevice.
        }
        else if( strncmp(file, "HNAP1/", 6) == 0 && action_string == NULL )
        {
                /* for case http://192.168.0.1/HNAP1/ from web-page */
                DEBUG_MSG("http://192.168.0.1/HNAP1/ from web-page\n");
                ignore_password = 1;
               	memset(file_tmp, 0, 50);
               	strcpy(file_tmp, "HNAP1.txt");
                file = file_tmp;
        }
        else if( file[0] == '\0' || file[len-1] == '/' )
        {
                /* for others */
                DEBUG_MSG("others\n");
                
#ifdef OPENDNS
			if( strstr(file, "parentalcontrols/")) {
				DEBUG_MSG("find opendns parnetalcontrols\n");
			}
			else

			
#endif                
                
                if( (action_string != NULL) && (strstr(action_string,"GetDeviceSettings") != NULL) ){
            			DEBUG_MSG("Don't care authorization of GetDeviceSettings\n");
                        ignore_password = 1;
                }
                else if(pure_mode_enable)
                {
                        DEBUG_MSG("Other HNAP Action\n");
                }
                else
                {
                        DEBUG_MSG("login.asp\n");
               		memset(file_tmp, 0, 50);
               		if (nvram_match("nvram_default_value", "1") == 0) {
               			strcpy(file_tmp, "/setup_wizard.asp");
               		}
               		else {
               		    strcpy(file_tmp, "/login.asp");
               		}
                	file = file_tmp;
                        /* modify by ken for widget login issue
                           when widget run login continue then get from IE will can't get other page
                           because will goto login.asp page after run logout
                        */
#ifdef AthSDK_AP61//just for DIR-400
                        logout();
#endif
                }
        }

#if defined(HAVE_HTTPS)
        if (openssl_request == 0)
#endif
        if( pure_mode_enable){
                DEBUG_MSG("PURE MODE: pure_init\n");
       		memset(file_tmp, 0, 50);
       		strcpy(file_tmp, "index.xml");
              	file = file_tmp;
                pure_init(conn_fd,"HTTP/1.1", cl, content_type, if_modified_since, action_string);
        }
#else
        if( file[0] == '\0' || file[len-1] == '/' )
        {
       		memset(file_tmp, 0, 50);
       		strcpy(file_tmp, nvram_safe_get("default_html"));
              	file = file_tmp;
        }
#endif

#ifdef CONFIG_WEB_404_REDIRECT
        get_404_file:
#endif

#ifdef XML_AGENT
#if defined(HAVE_HTTPS)
        if (openssl_request == 0)
#endif
        widget_init(conn_fd, "HTTP/1.1");
#endif

#ifdef AJAX
        ajax_init(conn_fd, "HTTP/1.1");
#endif


        /* check permission of ip */
        if(limit_ip_connect(usa.sa_in.sin_addr))
        {
       		memset(file_tmp, 0, 50);
       		strcpy(file_tmp, "ip_error.txt");
              	file = file_tmp;
        }

/*
 *      Date: 2009-10-09
 *      Name: Gareth Williams <gareth.williams@ubicom.com>
 *      Reason: Streamengine support - when rating show the please wait page.  Only force a file change if the file requested was a HTML or ASP page
 *      all other files should be allowed since the web browser is probably in the middle of loading a page already and we don't want to disturb that.
 */
#ifdef CONFIG_USER_STREAMENGINE
        if (if_measure_now() && wan_ip_is_obtained()) {
                if (strstr(file, ".asp") || strstr(file, ".htm")) {
       			memset(file_tmp, 0, 50);
       			strcpy(file_tmp, "please_wait.asp");
              		file = file_tmp;
                }
        }
#endif

        pattern_found = 0;

		/*
		 * Reason: Follw D-Link Router Web-based Setup Wizard Spec.
		 * Modified: Yufa Huang
		 * Date: 2011.02.25
		 */
        if (nvram_match("nvram_default_value", "1") == 0) {
            if (strlen(file) && strstr(file, ".asp")) {
                strcpy(file, "setup_wizard.asp");
            }
        }
        else {
            if (strlen(file) && strcmp(file, "setup_wizard.asp") == 0) {
                strcpy(file, "login.asp");
            }
        }

        /*
         * Reason: Follow the Spec. of D-Link Hidden page. An admin privilege
         *         SHOULD be gained before the accessibility to version.txt.
         * Modified: Yufa Huang
         * Date: 2010.07.05
         */
        if (strcmp(file, "version.txt") == 0) {
            if ((update_logout_list(&con_info.mac_addr[0], GET_FLG, NULL_LIST) != 0) &&
                (update_logout_list(&con_info.mac_addr[0], GET_PAG, NULL_LIST) != 0)) {
                // no login
                memset(file_tmp, 0, 50);
               	if (nvram_match("nvram_default_value", "1") == 0) {
               		strcpy(file_tmp, "/setup_wizard.asp");
               	}
               	else {
               	    strcpy(file_tmp, "/login.asp");
               	}
                file = file_tmp;
            }
            else {
                int user_index = auth_login_find(&con_info.mac_addr[0]);
                if (strcmp(auth_login[user_index].curr_user, "user") == 0) {
                    // user login
                    memset(file_tmp, 0, 50);
               		if (nvram_match("nvram_default_value", "1") == 0) {
               			strcpy(file_tmp, "/setup_wizard.asp");
               		}
               		else {
               		    strcpy(file_tmp, "/login.asp");
               		}
       	            file = file_tmp;
                }
            }
        }

        DEBUG_MSG("httpd file name = %s\n", file);

/*
*	Date: 2010-07-30
*   Name: Ken Chiang
*   Reason: Modify for the *.jpg,*.bmp... file can't get by http download featue. 
*   Notice :
*/				
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
#ifdef USB_STORAGE_HTTP_ENABLE	
	if(!nvram_match("usb_storage_http_enable", "1")){
		int ret = 0,Space_flag = 0;
		char *path_url=NULL;
		char wan_ipaddr[34]={0},wan_ipaddrport[40]={0};		
		USB_STORAGE_DEBUG_MSG("usb_storage_http_enable\n");
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
		if(!lan_request){
			usb_storge_http_get_wanip(wan_ipaddr);
			sprintf(wan_ipaddrport,"%s:%s",wan_ipaddr, nvram_safe_get("remote_http_management_port"));	
		}	
		path_url = strdup(path);
		if(strstr(path,"upload_file.cgi") ){//upload_file.cgi
			USB_STORAGE_DEBUG_MSG("\n\nupload_file.cgi=%s\n\n",path_url);
			pattern_found = 1;
/*
*	Date: 2010-08-04
*   Name: Ken Chiang
*   Reason: modiyf for usb storage upload and download file don't login issue.
*   Notice :
*/
			if(usb_storge_http_login_check() < 0){
				memset(file_tmp, 0, 50);
               	if (nvram_match("nvram_default_value", "1") == 0) {
               		strcpy(file_tmp, "/setup_wizard.asp");
               	}
               	else {
               	    strcpy(file_tmp, "/login.asp");
               	}
       			file = file_tmp;
				USB_STORAGE_DEBUG_MSG("usb_storge_http_login_check = %s\n", file);				
				send_headers( 200, "Ok", no_cache, "text/html" );
				websWrite(conn_fp, "%s", http_storage_header);	 
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file can uesd https.
*   Notice :
*/
				if(!lan_request){
#if defined(HAVE_HTTPS)
					if(openssl_request)
						websWrite(conn_fp, https_storage_back_script, wan_ipaddrport, file);
					else	
#endif	
						websWrite(conn_fp, http_storage_back_script, wan_ipaddrport, file);
				}	
				else{
#if defined(HAVE_HTTPS)
					if(openssl_request)
						websWrite(conn_fp, https_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
					else	
#endif	
						websWrite(conn_fp, http_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
				}
				websWrite(conn_fp, http_storage_back_body, "Please Login First !!");		
				websWrite(conn_fp, "%s", http_storage_tailer);
				return;				
			}
			else{	
				if(do_uploadfile_post(path_url, conn_fp, cl, boundary)){//fail
					do_upload_cgi(path_url, conn_fp, 1, lan_request, wan_ipaddrport);									
				}
				else{
					do_upload_cgi(path_url, conn_fp, 0, lan_request, wan_ipaddrport);					
				}	
			}					
		} 	
		else if(strstr(path,"creat_directory.cgi") ){//creat_directory.cgi
			USB_STORAGE_DEBUG_MSG("\n\creat_directory.cgi=%s\n\n",path_url);
			pattern_found = 1;
/*
*	Date: 2010-08-04
*   Name: Ken Chiang
*   Reason: modiyf for usb storage upload and download file don't login issue.
*   Notice :
*/
			if(usb_storge_http_login_check() < 0){
				memset(file_tmp, 0, 50);
               	if (nvram_match("nvram_default_value", "1") == 0) {
               		strcpy(file_tmp, "/setup_wizard.asp");
               	}
               	else {
               	    strcpy(file_tmp, "/login.asp");
               	}
       			file = file_tmp;
				USB_STORAGE_DEBUG_MSG("usb_storge_http_login_check = %s\n", file);				
				send_headers( 200, "Ok", no_cache, "text/html" );
				websWrite(conn_fp, "%s", http_storage_header);	 
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file can uesd https.
*   Notice :
*/
				if(!lan_request){
#if defined(HAVE_HTTPS)
					if(openssl_request)
						websWrite(conn_fp, https_storage_back_script, wan_ipaddrport, file);
					else	
#endif	
						websWrite(conn_fp, http_storage_back_script, wan_ipaddrport, file);
				}
				else{
#if defined(HAVE_HTTPS)
					if(openssl_request)
						websWrite(conn_fp, https_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
					else	
#endif					
						websWrite(conn_fp, http_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
				}
				websWrite(conn_fp, http_storage_back_body, "Please Login First !!");		
				websWrite(conn_fp, "%s", http_storage_tailer);
				return;				
			}	
			else{
				if(do_creatdir_post(path_url, conn_fp, cl, boundary)){//fail
					do_upload_cgi(path_url, conn_fp, 1, lan_request, wan_ipaddrport);									
				}
				else{
					do_upload_cgi(path_url, conn_fp, 0, lan_request, wan_ipaddrport);					
				}
			}						
		} 	
		else{//others 
			//check Space and special char 
			if(strstr(path,"%20") || strstr(path,"$") || strstr(path,"&") || strstr(path,"%5E")){
				USB_STORAGE_DEBUG_MSG("path_url=%s\n",path_url);
				Space_flag = StrSpaceDecode(path,path_url);			
				USB_STORAGE_DEBUG_MSG("path_url=%s,Space_flag=%d\n",path_url,Space_flag);
			}			
			if(!usb_storge_http_mount_match(path_url)){
				pattern_found = 1;
/*
*	Date: 2010-08-04
*   Name: Ken Chiang
*   Reason: modiyf for usb storage upload and download file don't login issue.
*   Notice :
*/
				if(usb_storge_http_login_check() < 0){
					memset(file_tmp, 0, 50);
               		if (nvram_match("nvram_default_value", "1") == 0) {
               			strcpy(file_tmp, "/setup_wizard.asp");
               		}
               		else {
               		    strcpy(file_tmp, "/login.asp");
               		}
       				file = file_tmp;
					USB_STORAGE_DEBUG_MSG("usb_storge_http_login_check = %s\n", file);				
					send_headers( 200, "Ok", no_cache, "text/html" );
					websWrite(conn_fp, "%s", http_storage_header);	 
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file can uesd https.
*   Notice :
*/
					if(!lan_request){
#if defined(HAVE_HTTPS)
					if(openssl_request)
						websWrite(conn_fp, https_storage_back_script, wan_ipaddrport, file);
					else	
#endif
						websWrite(conn_fp, http_storage_back_script, wan_ipaddrport, file);
					}	
					else{
#if defined(HAVE_HTTPS)
					if(openssl_request)
						websWrite(conn_fp, https_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
					else	
#endif
						websWrite(conn_fp, http_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
					}
					websWrite(conn_fp, http_storage_back_body, "Please Login First !!");		
					websWrite(conn_fp, "%s", http_storage_tailer);
					return;					
				}
				else{	
					usb_storge_http_handler(path_url, conn_fp, lan_request, wan_ipaddrport);
				}					
			}
			else{
				ret=usb_storge_http_list_match(path_url, Space_flag);
				if(!ret){//find file
					pattern_found = 1;
/*
*	Date: 2010-08-04
*   Name: Ken Chiang
*   Reason: modiyf for usb storage upload and download file don't login issue.
*   Notice :
*/
					if(usb_storge_http_login_check() < 0){
						memset(file_tmp, 0, 50);
               			if (nvram_match("nvram_default_value", "1") == 0) {
               				strcpy(file_tmp, "/setup_wizard.asp");
               			}
               			else {
               		    	strcpy(file_tmp, "/login.asp");
               			}
       					file = file_tmp;
						USB_STORAGE_DEBUG_MSG("usb_storge_http_login_check = %s\n", file);				
						send_headers( 200, "Ok", no_cache, "text/html" );
						websWrite(conn_fp, "%s", http_storage_header);	 
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file can uesd https.
*   Notice :
*/
						if(!lan_request){
#if defined(HAVE_HTTPS)
							if(openssl_request)
								websWrite(conn_fp, https_storage_back_script, wan_ipaddrport, file);
							else	
#endif
								websWrite(conn_fp, http_storage_back_script, wan_ipaddrport, file);
						}
						else{
#if defined(HAVE_HTTPS)
							if(openssl_request)
								websWrite(conn_fp, https_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
							else	
#endif							
								websWrite(conn_fp, http_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
						}
						websWrite(conn_fp, http_storage_back_body, "Please Login First !!");		
						websWrite(conn_fp, "%s", http_storage_tailer);
						return;								
					}
					else{
						do_get_file(path_url, conn_fp);
					}					
				}
				else if(ret == 1){//find dir
					pattern_found = 1;				
/*
*	Date: 2010-08-04
*   Name: Ken Chiang
*   Reason: modiyf for usb storage upload and download file don't login issue.
*   Notice :
*/
					if(usb_storge_http_login_check() < 0){
						memset(file_tmp, 0, 50);
               			if (nvram_match("nvram_default_value", "1") == 0) {
               				strcpy(file_tmp, "/setup_wizard.asp");
               			}
               			else {
               		    	strcpy(file_tmp, "/login.asp");
               			}
       					file = file_tmp;
						USB_STORAGE_DEBUG_MSG("usb_storge_http_login_check = %s\n", file);				
						send_headers( 200, "Ok", no_cache, "text/html" );
						websWrite(conn_fp, "%s", http_storage_header);	 
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
/*
*	Date: 2010-08-06
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file can uesd https.
*   Notice :
*/
						if(!lan_request){
#if defined(HAVE_HTTPS)
							if(openssl_request)
								websWrite(conn_fp, https_storage_back_script, wan_ipaddrport, file);
							else	
#endif
								websWrite(conn_fp, http_storage_back_script, wan_ipaddrport, file);
						}	
						else{
#if defined(HAVE_HTTPS)
							if(openssl_request)
								websWrite(conn_fp, https_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
							else	
#endif							
								websWrite(conn_fp, http_storage_back_script, nvram_safe_get("lan_ipaddr"), file);
						}
						websWrite(conn_fp, http_storage_back_body, "Please Login First !!");		
						websWrite(conn_fp, "%s", http_storage_tailer);
						return;								
					}
					else{	
						do_get_dir_list(path_url, conn_fp, Space_flag, lan_request, wan_ipaddrport);	
					}										
				}	
				else{//no find
					USB_STORAGE_DEBUG_MSG("path_url=%s no find\n",path_url);
#ifdef MIII_BAR
					if(strstr(path,"/ws/api/getFile?") )
					{
						do_miii_get_file(path_url, conn_fp);
					}
#endif //MIII_BAR
				}		
			}	
		}//upload_file.cgi 
		Space_flag = 0;				
	}	
	USB_STORAGE_DEBUG_MSG("httpd file name = %s\n", file);	
#endif//USB_STORAGE_HTTP_ENABLE

/*
*       Date: 2009-08-10
*   Name: Ken Chiang
*   Reason: Modify for GPL CmoCgi.
*   Notice :
*/
/*
        for (handler = &mime_handlers[0]; handler->pattern; handler++)
*/
    handler = &mime_handlers[0];
customer_mime_handlers_chack:

        for (; handler->pattern; handler++)
        {
                if (match(handler->pattern, file))
                {
                        pattern_found = 1;
                        auth_count    = 0;
                        unauth_flag = 0;
                        if (handler->auth)
                        {
                                DEBUG_MSG("handler->auth\n");
                                handler->auth(admin_userid, admin_passwd, user_userid, user_passwd, auth_realm);

#ifdef OPENDNS
			// parcontrol enable and 30 minutes use only
								int pass_check;

int check_opendns_timeout(char *token);
								
								pass_check = 0;
								if( strcmp(file,"opendns_fail.asp")==0) {
									// pass auth check
									if( check_opendns_timeout(NULL) >= 0) {
										pass_check = 1;
									}
								}
								else
								if( strstr(file,"opendns_okUI.asp?") ) {
									pass_check = 1;
									file = "opendns_okUI.asp";
								}
								else
								if( strstr(file,"opendns_failUI.asp") ) {
									pass_check = 1;
									file = "opendns_failUI.asp";
								}
								else
								if( strstr(file,"opendns_okWait.asp") )  {
									pass_check = 1;
									file = "opendns_okWait.asp";
								}
								else
								if( strstr(file,"opendns_ok.asp?") ) {
									// pass auth check
									// maybe check token id									
									if( check_opendns_timeout(strstr(file,"?")+1) >= 0) {
										pass_check = 1;
									}
									file = "opendns_ok.asp";
								}
								else
								if( strstr(file,"dns_reboot.cgi?") ) {
									// pass auth check
									// maybe check token id?
									if( check_opendns_timeout(strstr(file,"?")+1) >= 0) {
										pass_check = 1;
									}
									file = "dns_reboot.cgi";
								}
				
									
			
								
								if( pass_check) {
									// pass check auth
								}
								else									
#endif

                                /* check if logout request */
                                if( update_logout_list(&con_info.mac_addr[0],GET_FLG,NULL_LIST) == 1 &&\
                                    update_logout_list(&con_info.mac_addr[0],GET_PAG,NULL_LIST) == 1 &&\
                                    !pure_mode_enable )
                                {
                                        DEBUG_MSG("update_logout_list 1\n");
                                        if( (strstr(file, ".asp") && strcmp(file, "login_fail.asp") && strcmp(file, "logout.asp")) || strstr(file, ".cgi") ) //NickChou
                                        {
											memset(file_tmp, 0, 50);
               								if (nvram_match("nvram_default_value", "1") == 0) {
               									strcpy(file_tmp, "/setup_wizard.asp");
               								}
               								else {
               		    						strcpy(file_tmp, "/login.asp");
               								}						
       										file = file_tmp;
                                            DEBUG_MSG("update_logout_list\n");
                                        }

                                        if(!strstr(file, ".css") && !strstr(file, ".js") && strcmp(file, "logout.asp")) {
                                                /*
                                                *   Date: 2010-07-07
                                                *   Name: Nick Chou
                                                *   Reason: 
                                                *   1. Bugs Steps:
                                                                1. User login
                                                                2. wait 3 min to logout automatically
                                                                3. Enter any asp => redirect to login.asp => correct
                                                                4. Enter any asp again => User can login without auth => incorrect
                                                */
                                                update_logout_list(&con_info.mac_addr[0], SET_PAG, -1);
                                                update_logout_list(&con_info.mac_addr[0], SET_FLG, -1);
                                        }
                                }
                                else
                                {
#ifdef PURE_NETWORK_ENABLE
                                        if(!ignore_password)
                                        {
                                                DEBUG_MSG("!ignore_password\n");
                                                if(pure_mode_enable) {
                                                	extern int HNAP_auth_flag;
                                                        HNAP_auth_flag = auth_check(auth_realm, authorization);
                                                        DEBUG_MSG("HNAP_auth_flag  = %d\n",HNAP_auth_flag);
                                                        //{
                                                        		//HNAP_auth_flag = 1;
                                                                //DEBUG_MSG(" pure_mode_enable, !auth_check\n");
                                                                //return;
                                                        //}
                                               }         
                                        }
#endif
                                        DEBUG_MSG("! update_logout_list\n");
                                        if (update_logout_list(&con_info.mac_addr[0],GET_FLG,NULL_LIST) == -1 &&
                                                update_logout_list(&con_info.mac_addr[0],GET_PAG,NULL_LIST) == -1)
                                        {
                                                DEBUG_MSG("update_logout_list -1\n");
#ifdef AUTHGRAPH
                                                if(strstr(file, ".cgi") || (strstr(file, ".asp") && strcmp(file, "login_fail.asp") && strcmp(file, "login_auth_fail.asp")))
#else
                                                if(strstr(file, ".cgi") || (strstr(file, ".asp") && strcmp(file, "login_fail.asp")))
#endif
                                                {
                                                        if(strcmp(file, "login.cgi") != 0
#if defined(DIR865) || defined(EXVERSION_NNA) //for multi-language in login page
                                                        && strcmp(file, "switch_language.cgi") != 0
#endif
                                                        )
                                                        {
                                                                DEBUG_MSG("unauth_flag==1\n");
                                                                //handler->input = NULL;
                                                                //handler->output = do_ej;
                                                                unauth_flag = 1;
                                                        }
                                                        /*      Date:   2009-05-21
                                                        *       Name:   jimmy huang
                                                        *       Reason: To avoid user to hack router's login behavior
                                                        *       Note:   When in the same pc, firest login successfully with browser 1,
                                                        *                       then use browser 2 with incorrect password or graph code,
                                                        *                       now the first browser will also be logout
                                                        */
                                                        if(strcmp(file, "login_fail.asp") || strcmp(file, "login_auth_fail.asp")){
                                                                logout();
                                                        }
                                                        // ----------
                                                        if( strstr(file, "save_configuration.cgi") || strstr(file, "save_log.cgi") )
                                                        {
                                                                /*NickChou, if unauth, redirect to login.asp*/
                                                                if(unauth_flag)
                                                                {
                                                                        /*
                                                                        we need to change extra_header and mime_type,
                                                                        then GUI redirect to login.asp.
                                                                        if we don't change,
                                                                        user still can download file.
                                                                        */
                                                                        handler->extra_header = no_cache;
                                                                        handler->mime_type = "text/html";
                                                                        change_savecgi = 1;
                                                                }
                                                        }

								memset(file_tmp, 0, 50);
               					if (nvram_match("nvram_default_value", "1") == 0) {
               						strcpy(file_tmp, "/setup_wizard.asp");
               					}
               					else {
               		    			strcpy(file_tmp, "/login.asp");
               					}
       							file = file_tmp;
                                                }
                                        }
                                }
                        }
                        /*
                        *       Date:   2009-06-17
                        *       Name:   Jimmy Huang
                        *       Reason: For Sorenson used, only acoount is sorenson-admin
                        *                       can view the page heartbeatinterval.asp
                        *       Notice: Both HTTPD_USED_MUTIL_AUTH and HTTPD_USED_SP_ADMINID
                        *                       need to be defined
                        */
#if defined(HTTPD_USED_MUTIL_AUTH) && defined(HTTPD_USED_SP_ADMINID)
                        if(strncmp(file,"heartbeatinterval.asp",21) == 0){
                                int index_sp = 0;
                                if( (index_sp = auth_login_find(&con_info.mac_addr[0])) < 0 ){
                                        if( (index_sp = auth_login_search()) < 0){
                                                index_sp=auth_login_search_long();
                                        }
                                }
                                if(sp_admin_userid){
                                        DEBUG_MSG("login name = %s\n",auth_login[index_sp].curr_user);
                                        DEBUG_MSG("sp_admin_userid = %s\n",sp_admin_userid);
                                        if(strncmp(auth_login[index_sp].curr_user,sp_admin_userid,strlen(sp_admin_userid)) != 0){
											memset(file_tmp, 0, 50);
               								if (nvram_match("nvram_default_value", "1") == 0) {
               									strcpy(file_tmp, "/setup_wizard.asp");
               								}
               								else {
               		    						strcpy(file_tmp, "/login.asp");
               								}
       										file = file_tmp;
                                        }
                                }
                        }
#endif

                        if (!strstr(file, ".cgi") && strcasecmp(method, "post") == 0 && !handler->input)
                        {
                                send_error(501, "Not Implemented", NULL, "That method is not implemented.");
                                DEBUG_MSG("send_error 501 method=%s\n",method);
                                return;
                        }

                        if(change_savecgi)
                        {
                                if( strstr(file, "save_configuration.cgi") )
                                {
                                        /*NickChou, if auth, restore extra_header and mime_type*/
                                        if(!unauth_flag)
                                        {
                                                handler->extra_header = download_nvram;
                                                handler->mime_type = NULL;
                                                change_savecgi = 0;
                                        }
                                }
                                if( strstr(file, "save_log.cgi") )
                                {
                                        /*NickChou, if auth, restore extra_header and mime_type*/
                                        if(!unauth_flag)
                                        {
                                                handler->extra_header = log_to_hd;
                                                handler->mime_type = NULL;
                                                change_savecgi = 0;
                                        }
                                }
                        }

                        if (handler->input && unauth_flag==0)
                        {
                                DEBUG_MSG("handler->input\n");
                                handler->input(file, conn_fp, cl, boundary);
#if defined(linux)
#if defined(HAVE_HTTPS)
        if (openssl_request == 0)
#endif
        {
                                                if ((flags = fcntl(fileno(conn_fp), F_GETFL)) != -1 &&
                                                        fcntl(fileno(conn_fp), F_SETFL, flags | O_NONBLOCK) != -1)
                                                {
                                                        /* Read up to two more characters */
                                                        if (fgetc(conn_fp) != EOF)
                                                                (void) fgetc(conn_fp);
                                                        fcntl(fileno(conn_fp), F_SETFL, flags);
                                                }
        }
#endif
                        }
                       

#ifdef OPENDNS


extern char opendns_token[32+1]; // 32 bytes
			int find_parental=0;
//				printf("gogogo100\n");    
			if( strstr(file, "parentalcontrols/")) {
				if( strlen(file+17) > 16) 
				
				if( opendns_token[0])
				if( strstr(file+17, opendns_token)) {
				    if( *(file+17+16) == '?') {
				    	find_parental = 1;
				    }
// UI issue  token + UI ?
				    if( *(file+17+18) == '?' && 
						*(file+17+17) == 'I' &&
						*(file+17+16) == 'U') {
				    	find_parental = 1;
				    }


				}
				
				if( !find_parental) {
					memset(file_tmp, 0, 50);

              		strcpy(file_tmp, "parentalcontrols");
					file = file_tmp;
				}
			}			

//				printf("gogogo100\n");    
			if( find_parental ) {
//			if (strstr(file, "parentalcontrols?")) {
//				printf("gogogo100\n");    

				send_headers( 200, "Ok", handler->extra_header, handler->mime_type );
				if (handler->output)
				{
					handler->output(file, conn_fp);
				}
			}
			else 


#endif 	
						
#ifdef MIII_BAR
			if (strstr(file, "ws/api/")) {
				send_headers( 200, "Ok", handler->extra_header, handler->mime_type );
				if (handler->output)
				{
					handler->output(file, conn_fp);
				}
			}
			else 
#endif                        	


                        if(!check_file_exist(file))
                        {
#ifdef MIII_BAR
 			        ///da/USB_USB2FlashStorage_45c0216_1/Photos/dfddd/.miiithumbs/Tulips_1291286416_620993.jpg?1288248332	
				if(strstr(path,"/da/") ){   // /da/												
					char *sp = NULL;
					char *sp1 = NULL;
					char *sp2 =NULL;
					char cmds[128]={0};
					//sp2=strchr (path, '?') + 1;
					sp = strstr(file, "da");
					sp = strtok(sp+2, "?");
					if(miiicasa_set_folder() == -1)
					{
                                        	DEBUG_MSG("!check_file_exist: send_error 404\n");
                                        	send_error( 404, "Not Found", (char*) 0, "File not found." );
					}
					else
					{
						char *str1 = str_replace(sp,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder_t);	
						do_get_file(str1, conn_fp);
						if(strstr(str1, ".miiithumbs"))
						{
					        }
						if(str1) free(str1);
					}
				}else{										
                                        DEBUG_MSG("!check_file_exist: send_error 404\n");
                                        send_error( 404, "Not Found", (char*) 0, "File not found." );
                                }
#else
                                DEBUG_MSG("!check_file_exist: send_error 404\n");
                                send_error( 404, "Not Found", (char*) 0, "File not found." );
#endif
                        }
                        else
                        {
#ifdef PURE_NETWORK_ENABLE
                                if(pure_mode_enable == 0 ) // if pure netwok(1), send its header in pure.c
#endif

                                if(1
#ifdef XML_AGENT
                                && !strstr(file, "router_info.xml?")
                                && !strstr(file, "post_login.xml?")
#endif
#ifdef AJAX
                                && !strstr(file, "device.xml=")
                                && !strstr(file, "ping_test.xml=")
#endif
                                )
                                        send_headers( 200, "Ok", handler->extra_header, handler->mime_type );

                                DEBUG_MSG("send ok\n");
                                if (handler->output)
                                {
                                        if(unauth_flag)
                                        {
                                                do_ej(file, conn_fp);
                                                DEBUG_MSG("handler->output do_ej\n");
                                        }
                                        else
                                        {
                                                DEBUG_MSG("handler->output\n");
                                                handler->output(file, conn_fp);
                                        }
                                }
                        }
                        DEBUG_MSG("break for loop\n");
                        break;
                }
        }
/*
*       Date: 2009-08-10
*   Name: Ken Chiang
*   Reason: Modify for GPL CmoCgi.
*   Notice :
*/
        if(customer_mime_handlers_num && !customer_handlers_chack_flag && !pattern_found){
                DEBUG_MSG("%s: customer_mime_handlers_chack\n",__func__);
                handler = &customer_mime_handlers[0];
                customer_handlers_chack_flag = 1;
                goto customer_mime_handlers_chack;
        }

        if (!pattern_found)
        {
#ifdef MIII_BAR
       		char *path_url = NULL;
              	if(strstr(path,"/ws/api/uploadFile") )//ws/api/uploadFile
              	{	
              		char wan_ipaddr[34]={0},wan_ipaddrport[40]={0};

              		path_url = strdup(path);
              		pattern_found = 1;
              		if(!lan_request){
				usb_storge_http_get_wanip(wan_ipaddr);
				sprintf(wan_ipaddrport,"%s:%s",wan_ipaddr, nvram_safe_get("remote_http_management_port"));	
			}	
			if(miiicasa_set_folder() == -1)
			{
                		DEBUG_MSG("!pattern_found: send_error 404\n");
                		send_error( 404, "Not Found", (char*) 0, "File not found." );
                	}
                	else
                	{
              			if(do_miii_uploadfile_post(path_url, conn_fp, cl, boundary)){//fail
					do_miii_upload_cgi(path_url, conn_fp, 1, lan_request, wan_ipaddrport);									
				}
				else{
					do_miii_upload_cgi(path_url, conn_fp, 0, lan_request, wan_ipaddrport);					
				}
			}
              	}
              	else if(strstr(path,"/ws/api/downloadFiles") )//ws/api/downloadFiles
              	{
			path_url = strdup(path);
       			pattern_found = 1;
       			do_miii_download_file(path_url, conn_fp);              			
              	}
              	else
              	{	
              		//	do_upload_cgi(path_url, conn_fp, 1, lan_request, wan_ipaddrport);
                	DEBUG_MSG("!pattern_found: send_error 404\n");
                	send_error( 404, "Not Found", (char*) 0, "File not found." );
                 }
#else
                DEBUG_MSG("!pattern_found: send_error 404\n");
                send_error( 404, "Not Found", (char*) 0, "File not found." );
#endif
       }

#ifdef CONFIG_TC
    if ((change_admin_password_flag == 1) && !strcmp(file, "please_wait.asp"))
#else 
    if ((change_admin_password_flag == 1) && !strcmp(file, "back.asp"))
#endif
    {
        init_login_list();
        change_admin_password_flag=0; 
    }   
}

/* set logout flag */
void logout(void){
        update_logout_list(&con_info.mac_addr[0],DEL_MAC, NULL_LIST);
}

/* Signal handling */
static void http_signal(int sig)
{
        switch (sig)
        {
                case SIGUSR2:
                        nvram_flag_reset();
#ifdef CONFIG_WEB_404_REDIRECT
/*      Date:   2010-01-27
*       Name:   jimmy huang
*       Reason: When change usb type to 3 or 4 (windows mobile 5 or iPhone)
*                       We will change lan ip to 192.168.99.1 and reboot device
*                       But before reboot, br0 is still 192.168.0.1, thus
*                       host will not match nvram_safe_get(lan_bridge)
*                       If we do redirect to 404 to 192.168.99.1/error-404.htm, the count down page
*                       will not success show out
*                       So... we using real br0's ip instead of using nvram's value
*/
                        strncpy(lan_ip,read_ipaddr_no_mask(nvram_safe_get("lan_bridge")),sizeof(lan_ip));
                        device_name = nvram_safe_get("lan_device_name");

                        struct in_addr lan_ip_in_addr;
                        struct in_addr lan_mask_in_addr;

                        inet_aton(lan_ip, &lan_ip_in_addr);
                        get_netmask(nvram_safe_get("lan_bridge"),&lan_mask_in_addr);
                        lan_subnet.s_addr = lan_ip_in_addr.s_addr & lan_mask_in_addr.s_addr;
#endif
                        break;
                case SIGUSR1:/* PBC SUCCCESS */
                        set_push_button_result(1);
                        break;
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
                case SIGSYS:
                        printf("***************httpd SIGSYS\n");
                        init_LP(nvram_safe_get("lingual"));
                        break;
#endif
                case SIGQUIT:/* PBC FAIL */
                        set_push_button_result(2);
                        break;
#ifdef CONFIG_WEB_404_REDIRECT
                case SIGILL:
                        DEBUG_MSG("httpd recv SIGILL\n");
                        enable_404_redirect = 0;
                        break;
                case SIGINT:
                        DEBUG_MSG("httpd recv SIGINT\n");
                        enable_404_redirect = 1;
                        break;
#endif

        }
}

void auth_check_time(int sig)
{
#ifdef HTTP_TIMEOUT
/*
*       Date: 2010-04-23
*   Name: Yufa Huang
*   Reason: Use default value
*   Notice :
*/
    DEBUG_MSG("auth_count = %d\n", auth_count);
    if (auth_count++ == THREE_MINUTES) {
        DEBUG_MSG("login timeout\n");
        update_logout_list(&con_info.mac_addr[0],SET_FLG,1);
        update_logout_list(&con_info.mac_addr[0],SET_PAG,1);
        auth_count = 0;
    }
#endif

    /*2007.10.19 NickChou,  prvent "telnet 192.168.0.1 80" cause httpd "hangup" */
    signal(SIGALRM, auth_check_time);
}

/*
    Albert add for GPL customer request
    2009-04-15
*/
//int main(int argc, char **argv)
extern int httpd_main(int argc, char **argv)
{
        #define MAX(a, b) ((a)>(b)) ? (a) : (b)
        int listen_fd, max_fd=-1;
        struct sockaddr_in cliaddr;
        socklen_t sz = sizeof(cliaddr);
        struct itimerval value;
        struct sigaction act;
        int server;
        fd_set readfd, portfd;
        extern int firmware_upgrade;

        struct timeval  no_wait;
        no_wait.tv_sec = 0;
        no_wait.tv_usec = 0;

/*      Date:   2009-05-06
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
        struct timeval  *http_timeout = NULL;

#if defined(HAVE_HTTPS)
        int listen_fd_https;
        fd_set portfd_https;
        struct sockaddr_in cliaddr_https;
        socklen_t sz_https = sizeof(cliaddr_https);

        BIO *sbio;
        SSL_CTX *ctx = NULL;
        int r;
        BIO *ssl_bio;

        printf("Support HTTPS\n");
#endif

#ifdef IPv6_SUPPORT
        int listen6_fd;
        fd_set portfd6;
        struct sockaddr_in6 cliaddr6;
        socklen_t sz6 = sizeof(cliaddr6);

#if defined(HAVE_HTTPS)
        int listen6_fd_https;
        fd_set portfd6_https;
        struct sockaddr_in6 cliaddr6_https;

        printf("Support IPv6 HTTPS\n");
#endif
#endif

        /* Ignore broken pipes */
        signal(SIGPIPE, SIG_IGN);
        signal(SIGUSR2, http_signal);
        signal(SIGUSR1, http_signal);
#ifdef CONFIG_WEB_404_REDIRECT
        signal(SIGILL, http_signal);
        signal(SIGINT, http_signal);
#endif
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
        signal(SIGSYS, http_signal);
#endif
        signal(SIGQUIT, http_signal);
// init check timer
        act.sa_handler=auth_check_time;
        act.sa_flags=SA_RESTART;
        sigaction(SIGALRM, &act,NULL);

        value.it_value.tv_sec = 5;
        value.it_value.tv_usec = 0;
        value.it_interval.tv_sec = 5;
        value.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &value, NULL);
        init_login_list();

#if defined(CONFIG_WEB_404_REDIRECT) || defined(USB_STORAGE_HTTP_ENABLE)
        struct in_addr lan_ip_in_addr;
        struct in_addr lan_mask_in_addr;

        memset(lan_ip,'\0',sizeof(lan_ip));
        strncpy(lan_ip,read_ipaddr_no_mask(nvram_safe_get("lan_bridge")),sizeof(lan_ip));
        //lan_ip = read_ipaddr_no_mask(nvram_safe_get("lan_bridge"));
        inet_aton(lan_ip, &lan_ip_in_addr);
        get_netmask(nvram_safe_get("lan_bridge"),&lan_mask_in_addr);
        lan_subnet.s_addr = lan_ip_in_addr.s_addr & lan_mask_in_addr.s_addr;
        device_name = nvram_safe_get("lan_device_name");
#endif
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
        init_LP(nvram_safe_get("lingual"));
#endif
        /* Initialize listen socket */
/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
        if ((listen_fd = initialize_listen_socket(&usa, SERVER_PORT)) < 0) {
                fprintf(stderr, "httpd: can't bind to any address\n");
                exit(errno);
        }

        max_fd = MAX(listen_fd, max_fd);
        FD_ZERO(&portfd);
        FD_SET(listen_fd, &portfd);

/*      Date:   2009-05-06
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
#if defined(HAVE_HTTPS)
        /* Initialize listen socket */
        if ((listen_fd_https = initialize_listen_socket(&usa_https, DEFAULT_HTTPS_PORT)) < 0) {
                fprintf(stderr, "https: can't bind to any address\n");
                exit(errno);
        }

        max_fd = MAX(listen_fd_https, max_fd);
        //FD_ZERO(&portfd_https);
        FD_SET(listen_fd_https, &portfd);
#endif

#ifdef IPv6_SUPPORT
        if ((listen6_fd = initialize_listen6_socket(&usa6, SERVER_PORT)) < 0) {
                fprintf(stderr, "httpd: can't bind to any ipv6 address\n");
                exit(errno);
        }

        max_fd = MAX(listen6_fd, max_fd);
        //FD_ZERO(&portfd6);
        FD_SET(listen6_fd, &portfd);

#if defined(HAVE_HTTPS)
        /* Initialize listen socket */
        if ((listen6_fd_https = initialize_listen6_socket(&usa6_https, DEFAULT_HTTPS_PORT)) < 0) {
                fprintf(stderr, "https: can't bind to any ipv6 address\n");
                exit(errno);
        }

        max_fd = MAX(listen6_fd_https, max_fd);
        //FD_ZERO(&portfd6_https);
        FD_SET(listen6_fd_https, &portfd);
#endif
#endif

        /* Initialize internal structures */
        if (internal_init()){
                fprintf(stderr, "%s:error initializing internal structures\n",argv[0] );
                exit(errno);
        }
#if !defined(DEBUG)
        {
                FILE *pid_fp;
                /* Daemonize and log PID */
//              if (daemon(1, 1) == -1) {
//                      perror("daemon");
//                      exit(errno);
//              }
                if (!(pid_fp = fopen("/var/run/httpd.pid", "w")))
                {
                        perror("/var/run/httpd.pid");
                        return errno;
                }
                fprintf(pid_fp, "%d", getpid());
                fclose(pid_fp);
        }
#endif

#if defined(HAVE_HTTPS)
        SSLeay_add_ssl_algorithms ();
        SSL_load_error_strings ();

        ctx = SSL_CTX_new (SSLv23_server_method ());
        if (SSL_CTX_use_certificate_file (ctx, CERT_FILE, SSL_FILETYPE_PEM) == 0)
        {
                fprintf(stderr, "Can't read %s\n", CERT_FILE);
                exit(errno);
        }
        if (SSL_CTX_use_PrivateKey_file (ctx, KEY_FILE, SSL_FILETYPE_PEM) == 0)
        {
                fprintf(stderr, "Can't read %s\n", KEY_FILE);
                exit(errno);
        }
        if (SSL_CTX_check_private_key (ctx) == 0)
        {
                fprintf(stderr, "Check private key fail\n");
                exit(errno);
        }
#endif

        /* Loop forever handling requests */
        for (;;)
        {
        readfd = portfd;
                if (select(max_fd + 1, &readfd, 0, 0, &no_wait) > 0) {
                        if (FD_ISSET(listen_fd, &readfd)) {
                                if ((conn_fd = accept(listen_fd, &cliaddr, &sz)) < 0)
                                {
                                        perror("accept");
                                        return errno;
                                }
                                fcntl(conn_fd, F_SETFD, FD_CLOEXEC);
                                if (!(conn_fp = fdopen(conn_fd, "r+")))
                                {
                                        perror("fdopen");
                                        return errno;
                                }

                                save_connect_info(conn_fd, cliaddr.sin_addr);
#if defined(HAVE_HTTPS)
                                openssl_request = 0;
#endif

#ifdef IPv6_SUPPORT
                                ipv6_request = 0;
#endif
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
#if defined(CONFIG_WEB_404_REDIRECT) || defined(USB_STORAGE_HTTP_ENABLE)
                                if((cliaddr.sin_addr.s_addr & lan_mask_in_addr.s_addr) == lan_subnet.s_addr) {
                                        lan_request = 1;
					DEBUG_MSG("lan_request=1\n");					
                                }
                                else {
                                        lan_request = 0;
					DEBUG_MSG("lan_request=0\n");					
                                }
#endif
                                handle_request();
                                fclose(conn_fp);
                                close(conn_fd);
                        }
                //}

#if defined(HAVE_HTTPS)
                //readfd = portfd_https;
                //if (select(max_fd + 1, &readfd, 0, 0, &no_wait) > 0) {
                else
                        if (FD_ISSET(listen_fd_https, &readfd)) {
                                if ((conn_fd = accept(listen_fd_https, &cliaddr_https, &sz_https)) < 0)
                                {
                                        perror("accept");
                                        return errno;
                                }

				fcntl(conn_fd, F_SETFD, FD_CLOEXEC);
				
                                sbio = BIO_new_socket (conn_fd, BIO_NOCLOSE);
                                ssl = SSL_new (ctx);
                                SSL_set_bio (ssl, sbio, sbio);

                                if ((r = SSL_accept (ssl) <= 0))
                                {
                                        printf("SSL accept error\n");
                                        close (conn_fd);
                                        continue;
                                }

                                conn_fp = (webs_t) BIO_new (BIO_f_buffer ());
                                ssl_bio = BIO_new (BIO_f_ssl ());
                                BIO_set_ssl (ssl_bio, ssl, BIO_CLOSE);
                                BIO_push ((BIO *) conn_fp, ssl_bio);

                                save_connect_info(conn_fd, cliaddr_https.sin_addr);
                                openssl_request = 1;
#ifdef IPv6_SUPPORT
                                ipv6_request = 0;
#endif
/*
*	Date: 2010-08-05
*   Name: Ken Chiang
*   Reason: modify for usb storage upload and download file remote side feature.
*   Notice :
*/
#if defined(CONFIG_WEB_404_REDIRECT) || defined(USB_STORAGE_HTTP_ENABLE)
				if((cliaddr.sin_addr.s_addr & lan_mask_in_addr.s_addr) == lan_subnet.s_addr) {
					lan_request = 1;
				}
				else {
                                lan_request = 0;
				}
#endif
                                handle_request();
                                wfflush(conn_fp);
                                wfclose(conn_fp);
                                close(conn_fd);
                        }
                //}
#endif

#ifdef IPv6_SUPPORT
                //readfd = portfd6;
                //if (select(max_fd + 1, &readfd, 0, 0, &no_wait) > 0) {
                  else      if (FD_ISSET(listen6_fd, &readfd)) {

                                if ((conn_fd = accept(listen6_fd, &cliaddr6, &sz6)) < 0)
                                {
                                        perror("accept");
                                        return errno;
                                }
                                fcntl(conn_fd, F_SETFD, FD_CLOEXEC);
                                if (!(conn_fp = fdopen(conn_fd, "r+")))
                                {
                                        perror("fdopen");
                                        return errno;
                                }

#if defined(HAVE_HTTPS)
                                openssl_request = 0;
#endif
                                save_connect6_info(conn_fd, cliaddr6.sin6_addr);
                                handle_request();
                                fclose(conn_fp);
                                close(conn_fd);
                        }
                //}

#if defined(HAVE_HTTPS)
                //readfd = portfd6_https;
                //if (select(max_fd + 1, &readfd, 0, 0, &no_wait) > 0) {
                  else      if (FD_ISSET(listen6_fd_https, &readfd)) {
                                if ((conn_fd = accept(listen6_fd_https, &cliaddr6_https, &sz6)) < 0)
                                {
                                        perror("accept");
                                        return errno;
                                }

				fcntl(conn_fd, F_SETFD, FD_CLOEXEC);
				
                                sbio = BIO_new_socket (conn_fd, BIO_NOCLOSE);
                                ssl = SSL_new (ctx);
                                SSL_set_bio (ssl, sbio, sbio);

                                if ((r = SSL_accept (ssl) <= 0))
                                {
                                        printf("SSL accept error\n");
                                        close (conn_fd);
                                        continue;
                                }

                                conn_fp = (webs_t) BIO_new (BIO_f_buffer ());
                                ssl_bio = BIO_new (BIO_f_ssl ());
                                BIO_set_ssl (ssl_bio, ssl, BIO_CLOSE);
                                BIO_push ((BIO *) conn_fp, ssl_bio);

                                ipv6_request = 1;
                                save_connect6_info(conn_fd, cliaddr6_https.sin6_addr);
                                openssl_request = 1;
                                handle_request();
                                wfflush(conn_fp);
                                wfclose(conn_fp);
                                close(conn_fd);
                        }
#endif
#ifdef CONFIG_WEB_404_REDIRECT
                lan_request = 0;
#endif
#endif
		}
		else
                	usleep(500);  // Jery Lin modify for httpd free cpu resource
                ///Yufa Huang 2010.04.22 only for upgrading
                if (firmware_upgrade == 1) {
                        sleep(1); //John Huang 2010.04.21 add sleep to let upgrade function work fine
                }
        }

        shutdown(listen_fd, 2);
        close(listen_fd);

#if defined(HAVE_HTTPS)
        shutdown(listen_fd_https, 2);
        close(listen_fd_https);
#endif

#ifdef IPv6_SUPPORT
        shutdown(listen6_fd, 2);
        close(listen6_fd);

#if defined(HAVE_HTTPS)
        shutdown(listen6_fd_https, 2);
        close(listen6_fd_https);
#endif
#endif

        return 0;
}

#ifdef IPv6_SUPPORT
#define in6_addr_comp(a, b) ((a).s6_addr32[0]==(b).s6_addr32[0] && \
                             (a).s6_addr32[1]==(b).s6_addr32[1] && \
                             (a).s6_addr32[2]==(b).s6_addr32[2] && \
                             (a).s6_addr32[3]==(b).s6_addr32[3] )

struct in6_addr old_ip6_addr;

void save_connect6_info(int conn_fd_t, struct in6_addr addr6)
{
        FILE *pp;
        char buff[1024], *pos;
        int update_mac = 0, i;
        char remote_ppp_client_mac[18]={0};

        if ( !in6_addr_comp(old_ip6_addr, addr6) )
        {
                con_info.conn_fd = (int)conn_fd_t;
                con_info.ip6_addr = addr6;

                sprintf(buff, "ip -6 neigh show to %x:%x:%x:%x:%x:%x:%x:%x",
                        addr6.s6_addr16[0], addr6.s6_addr16[1], addr6.s6_addr16[2],
                        addr6.s6_addr16[3], addr6.s6_addr16[4], addr6.s6_addr16[5],
                        addr6.s6_addr16[6], addr6.s6_addr16[7]);

                if((pp = popen(buff, "r")) != NULL)
                {
                        DEBUG_MSG("open \"ip -6 neigh show\" success \n");
                        memset(buff, 0, 1024);

                        if( fgets(buff, 1024, pp))
                        {
                                // ex. "fe80::2c0:26ff:fe6d:72d3 dev br0 lladdr 00:c0:26:6d:72:d3 STALE"
                                if (pos = strstr(buff, "lladdr") ) {
                                        pos = pos+7;
                                        for (i=0; i<17; i++)
                                                con_info.mac_addr[i] = toupper(*pos++);
                                        update_mac = 1;
                                }
                        }
                        pclose(pp);

                        if( ! update_mac )
                        {
                                unsigned char *ptr =  &con_info.ip6_addr;
                                sprintf(remote_ppp_client_mac, "00:00:%02x:%02x:%02x:%02x"
                                        , *(ptr+12), *(ptr+13), *(ptr+14), *(ptr+15));
                                memcpy(con_info.mac_addr, remote_ppp_client_mac , 17);
                        }
                }
                old_ip6_addr = addr6;
        }
}
#endif

struct in_addr old_ip_addr;

void save_connect_info(int conn_fd_t, struct in_addr addr)
{
        FILE *fp;
        char buff[1024];
        int update_mac = 0;
        char remote_ppp_client_mac[17]={0};

        if(old_ip_addr.s_addr != addr.s_addr)
        {
        /*  Date: 2009-10-22
        *   Name: jimmy huang
        *   Reason: Fixed some Bad WPS
        *   Note: Patch from DIR-655 (revision 781)
        */
                //con_info.conn_fd = (int)conn_fp;
                con_info.conn_fd = (int)conn_fd;
                con_info.ip_addr = addr.s_addr;
/*
*  Name Albert
*  Date 2009-02-17
*  Detail: using system call will reduce CPU resrouce, why do not access directly?
*/
                //_system("cat /proc/net/arp > /var/tmp/arp.tmp");
                //if((fp = fopen("/var/tmp/arp.tmp", "r")) != NULL)
                if((fp = fopen("/proc/net/arp", "r")) != NULL)
                {
                        DEBUG_MSG("open arp.tmp success \n");
                        memset(buff, 0, 1024);
                        while( fgets(buff, 1024, fp))
                        {
                                if (strstr(buff, (char *)inet_ntoa(addr)) ) {
                                        memcpy(con_info.mac_addr, (buff + 41),17);
                                        update_mac = 1;
                                }
                        }
                        fclose(fp);

                        if( ! update_mac )
                        {
                                /*NickChou, 20091027, add for
                                 1. do not keep last mac
                                 2. PPP clinet with remote login
                                 => Because we can not find MAC of PPP remote client,
                                 => Dispatch garbage MAC to it.
                                */
                                unsigned char *ptr =  &con_info.ip_addr;
                                sprintf(remote_ppp_client_mac, "00:00:%02x:%02x:%02x:%02x"
                                        , *ptr, *(ptr+1), *(ptr+2), *(ptr+3));
                                memcpy(con_info.mac_addr, remote_ppp_client_mac , 17);
                        }
                }
                old_ip_addr.s_addr = addr.s_addr;
        }
}

/* set default limit ip when rebooting && apply_confirm */
void set_limit_connect_init_flag(int value){
        limit_connect_init_flag = value;
}

/* check if ip is allowd or not (ip is defined in limit_adm_ip_connect of nvram)
   return value:
      0: allow to access device
      1: isn't allow to access device */
static int limit_ip_connect(struct in_addr addr){
        char *limit_enable = NULL;
        char *limit_ip = NULL;
        char limit_ip_1[20] = {};
        char limit_ip_2[20] = {};
        char *find = NULL;

        /* limit_connect_init_flag = 0 , initial default limit ip */
        if(limit_connect_init_flag == 0){
                memset(default_limit_ip,0,sizeof(default_limit_ip));
                memset(default_limit_enable,0,sizeof(default_limit_enable));
                memcpy(default_limit_ip,nvram_safe_get("limit_adm_ip_connect"),strlen(nvram_safe_get("limit_adm_ip_connect")));
                memcpy(default_limit_enable,nvram_safe_get("limit_adm_enable"),strlen(nvram_safe_get("limit_adm_enable")));
                set_limit_connect_init_flag(1);
        }

        /* check if enable && default value */
        limit_ip = &default_limit_ip[0];
        limit_enable = &default_limit_enable[0];
        if(strncmp(limit_enable,"0",1) == 0 || strlen(limit_enable) == 0 || \
           limit_ip == NULL || strlen(limit_ip) == 0)
                return 0;

        /* find first ip */
        find = strstr(limit_ip,"/");
        if(find == NULL)
                return 1;
        memcpy(limit_ip_1,limit_ip,find - limit_ip);
        /* find second ip */
        memcpy(limit_ip_2,find + 1,strlen(find) - 1);

        if( strcmp((char *)inet_ntoa(addr),limit_ip_1) == 0 || \
            strcmp((char *)inet_ntoa(addr),limit_ip_2) == 0 ){
                return 0;
        }
        return 1;
}

/*      Date:   2009-04-09
 *      Name:   Yufa Huang
 *      Reason: add HTTPS function.
 */
char *wfgets(char *buf, int len, FILE *fp)
{
#if defined(HAVE_HTTPS)
        if (openssl_request) {
                int ret = BIO_gets((BIO *) fp, buf, len);
                if (ret < 0)  ret = 0;
                return (char *) ret;
        }
#endif

        return fgets(buf, len, fp);
}

int wfputc (char c, FILE *fp)
{
#if defined(HAVE_HTTPS)
        if (openssl_request)
                return BIO_printf((BIO *) fp, "%c", c);
#endif

        return fputc(c, fp);
}

int wfputs (char *buf, FILE *fp)
{
#if defined(HAVE_HTTPS)
        if (openssl_request) {
                return BIO_puts((BIO *) fp, buf);
        }
#endif

        return fputs(buf, fp);
}

int wfprintf (FILE *fp, char *fmt, ...)
{
        #include <stdarg.h>

        va_list args;
        char buf[4096];
        int ret;

        va_start(args, fmt);
        vsnprintf(buf, sizeof (buf), fmt, args);

#if defined(HAVE_HTTPS)
        if (openssl_request)
                ret = BIO_printf((BIO *) fp, "%s", buf);
        else
#endif
                ret = fprintf(fp, "%s", buf);

        va_end (args);
        return ret;
}

int wfflush (FILE *fp)
{
#if defined(HAVE_HTTPS)
        if (openssl_request) {
                BIO_flush((BIO *) fp);
                return 1;
        }
        else
#endif
                return fflush(fp);
}

int wfclose (FILE *fp)
{
#if defined(HAVE_HTTPS)
        if (openssl_request) {
                BIO_free_all((BIO *) fp);
                return 1;
        }
        else
#endif
                return fclose(fp);
}
