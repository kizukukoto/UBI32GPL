#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ctype.h>
#include <pure.h>
#include <nvram.h>
#include <time.h>
#include <unistd.h>
#include <linux_vct.h>

#include "sutil.h"
#include "shvar.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/netdevice.h>
#include <sys/ioctl.h>
#include "vct.h"
#include "ajax.h"
/*  Date: 2009-01-20
*   Name: jimmy huang
*   Reason: Avoid compiler warning
*   Note:
*/
#include "httpd_util.h"

/**************************************** prototype ****************************************/
int get_ajax_info(char *url);
static void add_ajax_headers(int status, char* title, char* extra_header, char* mime_type, long byte, char *mod );
static void ajax_add_to_buf( char* bufP, int* bufsizeP, int* buflenP, char* str, int len, int max_len);
static void ajax_add_to_response( char* str, int len );

static int create_err_xml(void);
static int create_device_status_xml(void);
static int create_internet_on_line_xml(void);
static int create_wireless_list_xml(void);

#define WIRELESS_LIST             2
#ifdef USER_WL_ATH_5GHZ
#define WIRELESS_5g_LIST          3
#endif

#define WAN_DETECT_STATUS_INIT    4
#define WAN_DETECT_STATUS         5
static int create_wan_detect_status_xml(int);

#define WIPNP_DETECT_STATUS       6
static int create_wipnp_status_xml(void);

#ifdef IPv6_SUPPORT
static int create_ipv6_status_xml(void);
static int create_ipv6_wizard_status_xml(void);
#endif

//#define HTTPD_DEBUG_AJAX 1
#ifdef HTTPD_DEBUG_AJAX
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

/**************************************** macro ****************************************/
#define DEVICE_STATUS       0
#define INTERNET_ON_LINE            1
#define NOT_SUPPORT         30
#ifdef IPv6_SUPPORT
#define IPV6_STATUS     16
#define IPV6_WIZARD_STATUS	17
#endif
/*  Date:   2009-04-17
*   Name:   jimmy huang
*   Reason: Add definition for miniupnpd-1.3 to use same features
*   Note:   This feature only works for libupnp or miniupnpd-1.3 (not include miniupnpd-1.0-RC7)
*/
//#if defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)
#if (defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)) || \
        (defined(UPNP_ATH_MINIUPNPD_1_3) && defined(UPNP_ATH_MINIUPNPD_1_3_LEASE_FILES))
static int create_upnp_dynamic_portmap_xml(void);
/*
*   Date: 2009-2-24
*   Name: Ken_Chiang
*   Reason: added for show upnp dynamic rule can up to 200 rules.
*   Notice :
*/
static int create_upnp_dynamic_portmappage_xml(int page_index);
#define UPNP_DYNAMIC_PORTMAP 8
#define UPNP_DYNAMIC_PORTMAP_PAGE0 9//page 0
#define UPNP_DYNAMIC_PORTMAP_PAGE1 10
#define UPNP_DYNAMIC_PORTMAP_PAGE2 11
#define UPNP_DYNAMIC_PORTMAP_PAGE3 12
#define UPNP_DYNAMIC_PORTMAP_PAGE4 13
#define UPNP_DYNAMIC_PORTMAP_PAGE5 14//page 5
#define UPNP_DYNAMIC_PORTMAP_PAGE6 15
#define UPNP_DYNAMIC_PORTMAP_PAGE7 16
#define UPNP_DYNAMIC_PORTMAP_PAGE8 17
#define UPNP_DYNAMIC_PORTMAP_PAGE9 18
#define UPNP_DYNAMIC_PORTMAP_PAGE10 19//page 10
#define UPNP_DYNAMIC_PORTMAP_PAGE11 20
#define UPNP_DYNAMIC_PORTMAP_PAGE12 21
#define UPNP_DYNAMIC_PORTMAP_PAGE13 22
#define UPNP_DYNAMIC_PORTMAP_PAGE14 23
#define UPNP_DYNAMIC_PORTMAP_PAGE15 24//page 15
#define UPNP_DYNAMIC_PORTMAP_PAGE16 25
#define UPNP_DYNAMIC_PORTMAP_PAGE17 26
#define UPNP_DYNAMIC_PORTMAP_PAGE18 27
#define UPNP_DYNAMIC_PORTMAP_PAGE19 28
#endif

/**************************************** global variable ****************************************/
pure_atrs_t ajax_atrs;

char *xmlHeader =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
"<env:Envelope xmlns:env=\"http://www.w3.org/2003/05/soap-envelop/\" env:encodingStyle=\"http://www.w3.org/2003/05/soap-encoding/\">"
"<env:Body>";

char *xmlTailer =
"</env:Body>"
"</env:Envelope>";

/**************************************** external variable ****************************************/
extern const char VER_FIRMWARE[];

/**************************************** function ****************************************/
char check_rc_status(void)
{
    int fd, size;
    char rc_state = 'i';

    fd = open(RC_FLAG_FILE, O_RDWR);
    if (fd)
    {
        read(fd, &rc_state, 1);
        close(fd);
    }
    else
        printf("Open %s failed\n", RC_FLAG_FILE);
    return rc_state;
}


void ajax_init(int conn_fd, char *protocol){

    ajax_atrs.conn_fd = conn_fd;
    ajax_atrs.protocol = protocol;
    return;
}
static void ajax_add_to_response( char* str, int len ){
    ajax_add_to_buf( ajax_atrs.response, &ajax_atrs.response_size, &ajax_atrs.response_len, str, len , MAX_RES_LEN_XML);
}


static void ajax_add_to_buf( char* bufP, int* bufsizeP, int* buflenP, char* str, int len, int max_len){
    if ( *bufsizeP == 0 ){
        *bufsizeP = MAX_RES_LEN_XML;
        *buflenP = 0;
    }
    if ( *buflenP + len >= *bufsizeP )
        return;

    memcpy( &bufP[*buflenP], str, len );
    *buflenP += len;
    bufP[*buflenP] = '\0';
}
static void add_ajax_headers(int status, char* title, char* extra_header, char* mime_type, long byte, char *mod ){
    char buf[200] = {};
    int buflen = 0;
    char timebuf[100];
    time_t now;

    ajax_atrs.status = status;
    ajax_atrs.bytes = byte;
    ajax_atrs.response_size = 0;

    sprintf( buf, "%s %d %s\r\n", ajax_atrs.protocol, ajax_atrs.status, title );
    ajax_add_to_response( buf, strlen(buf));

    sprintf( buf, "Server: Embedded HTTP Server ,Firmware Version %s\r\n", VER_FIRMWARE);
    ajax_add_to_response( buf, strlen(buf));

    now = time( (time_t*) 0 );
    strftime( timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime( &now ) );
    sprintf( buf, "Date: %s\r\n", timebuf );
    ajax_add_to_response( buf, strlen(buf));

    if ( extra_header != (char*) 0 ){
            sprintf( buf, "%s\r\n", extra_header );
            ajax_add_to_response( buf, strlen(buf));
    }

    if ( mime_type != (char*) 0 ){
            sprintf( buf, "Content-Type: %s\r\n", mime_type );
            ajax_add_to_response( buf, strlen(buf));
    }

    if ( ajax_atrs.bytes >= 0 ){
            sprintf( buf, "Content-Length: %ld\r\n", ajax_atrs.bytes );
            buflen = strlen(buf);
            ajax_add_to_response( buf, buflen );
    }

    if (mod){
            sprintf( buf, "Last-Modified: %s\r\n", "" );
            ajax_add_to_response( buf, strlen(buf));
    }

    sprintf( buf, "Connection: close\r\n\r\n" );
    sprintf( buf, "cache-Control: max-age=1\r\n\r\n" );
    ajax_add_to_response( buf, strlen(buf));

}


void do_ajax_action(char *url, FILE *stream, int len, char *boundary){

    switch (get_ajax_info(url))
    {
    case DEVICE_STATUS:
        create_device_status_xml();
        break;
    case INTERNET_ON_LINE:
        create_internet_on_line_xml();
        break;
    case WIRELESS_LIST:
        create_wireless_list_xml();
        break;
    case WAN_DETECT_STATUS_INIT:
         create_wan_detect_status_xml(1);
        break;          
    case WAN_DETECT_STATUS:
         create_wan_detect_status_xml(0);
        break;        
    case WIPNP_DETECT_STATUS:
         create_wipnp_status_xml();
        break;              
#ifdef IPv6_SUPPORT
    case IPV6_STATUS:
        create_ipv6_status_xml();
        break;
    case IPV6_WIZARD_STATUS:
       create_ipv6_wizard_status_xml();
        break;   
#endif
/*  Date:   2009-04-17
*   Name:   jimmy huang
*   Reason: Add definition for miniupnpd-1.3 to use same features
*   Note:   This feature only works for libupnp or miniupnpd-1.3 (not include miniupnpd-1.0-RC7)
*/
//#if defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)
#if (defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)) || \
        (defined(UPNP_ATH_MINIUPNPD_1_3) && defined(UPNP_ATH_MINIUPNPD_1_3_LEASE_FILES))
    case UPNP_DYNAMIC_PORTMAP:
        create_upnp_dynamic_portmap_xml();
        break;
/*
*   Date: 2009-2-24
*   Name: Ken_Chiang
*   Reason: added for show upnp dynamic rule can up to 200 rules.
*   Notice :
*/
    case UPNP_DYNAMIC_PORTMAP_PAGE0://page 0
        create_upnp_dynamic_portmappage_xml(0);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE1:
        create_upnp_dynamic_portmappage_xml(1);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE2:
        create_upnp_dynamic_portmappage_xml(2);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE3:
        create_upnp_dynamic_portmappage_xml(3);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE4:
        create_upnp_dynamic_portmappage_xml(4);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE5://page 5
        create_upnp_dynamic_portmappage_xml(5);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE6:
        create_upnp_dynamic_portmappage_xml(6);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE7:
        create_upnp_dynamic_portmappage_xml(7);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE8:
        create_upnp_dynamic_portmappage_xml(8);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE9:
        create_upnp_dynamic_portmappage_xml(9);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE10://page 10
        create_upnp_dynamic_portmappage_xml(10);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE11:
        create_upnp_dynamic_portmappage_xml(11);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE12:
        create_upnp_dynamic_portmappage_xml(12);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE13:
        create_upnp_dynamic_portmappage_xml(13);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE14:
        create_upnp_dynamic_portmappage_xml(14);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE15://page 15
        create_upnp_dynamic_portmappage_xml(15);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE16:
        create_upnp_dynamic_portmappage_xml(16);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE17:
        create_upnp_dynamic_portmappage_xml(17);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE18:
        create_upnp_dynamic_portmappage_xml(18);
        break;
    case UPNP_DYNAMIC_PORTMAP_PAGE19:
        create_upnp_dynamic_portmappage_xml(19);
        break;
#endif
      default:
        create_err_xml();
          break;
    }
    return;

}

void send_ajax_response(char *url,FILE *stream){
#if defined(HAVE_HTTPS)
    if (openssl_request == 1) {
        wfputs(ajax_atrs.response, stream);
    }
    else
#endif
        send( ajax_atrs.conn_fd, ajax_atrs.response, strlen(ajax_atrs.response), 0);

    memset(ajax_atrs.response,0,MAX_RES_LEN_XML);
    ajax_atrs.response_size = 0;

    return;
}

int get_ajax_info(char *url)
{
    char current_rc[2];

/*
 * Reason: remark this action. The status page will blank.
 * Modified: Yufa Huang
 * Date: 2010.03.26
 */
#if 0
    if (!strncmp(check_rc_status(current_rc), "b", 1)) {
        /*  Date:   2009-11-18
        *   Name:   Tina Tsao
        *   Reason: Fixed when device rebooting, open XML wireless status will case error.
        *   Note:
        */
        DEBUG_MSG("return 404\n");
        return 404;
    }
#endif

    if (strstr(url, "device_status"))
        return DEVICE_STATUS;
    else if (strstr(url, "internet_on_line"))
        return INTERNET_ON_LINE;
        /*  Date:   2009-11-12
        *   Name:   Tina Tsao
        *   Reason: Add for show XML wireless status
        *   Note:
        */
    else if (strstr(url, "wireless_list"))
        return WIRELESS_LIST;
#ifdef USER_WL_ATH_5GHZ
	else if (strstr(url, "wireless_5g_list"))
        return WIRELESS_5g_LIST; 	
#endif
    else if (strstr(url, "wan_detect_status_init"))
        return WAN_DETECT_STATUS_INIT;
    else if (strstr(url, "wan_detect_status"))
        return WAN_DETECT_STATUS;
    else if (strstr(url, "wipnp_detect"))
        return WIPNP_DETECT_STATUS;        
#ifdef IPv6_SUPPORT
    else if (strstr(url, "ipv6_status"))
        return IPV6_STATUS;
		else if (strstr(url, "ipv6_wizard_status"))
	  		return IPV6_WIZARD_STATUS;    
#endif
/* jimmy added 20080918 */
#ifdef UPNP_DYNAMIC_PORTMAP
/*
*   Date: 2009-2-24
*   Name: Ken_Chiang
*   Reason: modify for show upnp dynamic rule can up to 200 rules.
*   Notice :
*/
/*
    else if (strstr(url, "upnp_dynamic_portmap"))
        return UPNP_DYNAMIC_PORTMAP;
*/
    else if (strstr(url, "upnp_dynamic_portmap_num"))
        return UPNP_DYNAMIC_PORTMAP;
    else if (strstr(url, "upnp_dynamic_portmap_0"))//page 0
        return UPNP_DYNAMIC_PORTMAP_PAGE0;
    else if (strstr(url, "upnp_dynamic_portmap_1"))
        return UPNP_DYNAMIC_PORTMAP_PAGE1;
    else if (strstr(url, "upnp_dynamic_portmap_2"))
        return UPNP_DYNAMIC_PORTMAP_PAGE2;
    else if (strstr(url, "upnp_dynamic_portmap_3"))
        return UPNP_DYNAMIC_PORTMAP_PAGE3;
    else if (strstr(url, "upnp_dynamic_portmap_4"))
        return UPNP_DYNAMIC_PORTMAP_PAGE4;
    else if (strstr(url, "upnp_dynamic_portmap_5"))//page 5
        return UPNP_DYNAMIC_PORTMAP_PAGE5;
    else if (strstr(url, "upnp_dynamic_portmap_6"))
        return UPNP_DYNAMIC_PORTMAP_PAGE6;
    else if (strstr(url, "upnp_dynamic_portmap_7"))
        return UPNP_DYNAMIC_PORTMAP_PAGE7;
    else if (strstr(url, "upnp_dynamic_portmap_8"))
        return UPNP_DYNAMIC_PORTMAP_PAGE8;
    else if (strstr(url, "upnp_dynamic_portmap_9"))
        return UPNP_DYNAMIC_PORTMAP_PAGE9;
    else if (strstr(url, "upnp_dynamic_portmap_10"))//page 10
        return UPNP_DYNAMIC_PORTMAP_PAGE10;
    else if (strstr(url, "upnp_dynamic_portmap_11"))
        return UPNP_DYNAMIC_PORTMAP_PAGE11;
    else if (strstr(url, "upnp_dynamic_portmap_12"))
        return UPNP_DYNAMIC_PORTMAP_PAGE12;
    else if (strstr(url, "upnp_dynamic_portmap_13"))
        return UPNP_DYNAMIC_PORTMAP_PAGE13;
    else if (strstr(url, "upnp_dynamic_portmap_14"))
        return UPNP_DYNAMIC_PORTMAP_PAGE14;
    else if (strstr(url, "upnp_dynamic_portmap_15"))//page 15
        return UPNP_DYNAMIC_PORTMAP_PAGE15;
    else if (strstr(url, "upnp_dynamic_portmap_16"))
        return UPNP_DYNAMIC_PORTMAP_PAGE16;
    else if (strstr(url, "upnp_dynamic_portmap_17"))
        return UPNP_DYNAMIC_PORTMAP_PAGE17;
    else if (strstr(url, "upnp_dynamic_portmap_18"))
        return UPNP_DYNAMIC_PORTMAP_PAGE18;
    else if (strstr(url, "upnp_dynamic_portmap_19"))
        return UPNP_DYNAMIC_PORTMAP_PAGE19;
#endif
    else
      return NOT_SUPPORT;
}

char* get_internetonline_check(char *buf)
{
    char wan_interface[8] = {};
    char status[15] = {};
    char *wan_eth = NULL;
    char *wan_proto = NULL;

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
	char *usb_type=NULL;
	usb_type = nvram_safe_get("usb_type");
#endif
    wan_eth = nvram_safe_get("wan_eth");
    wan_proto = nvram_safe_get("wan_proto");

    if(strcmp(wan_proto, "dhcpc") == 0 || strcmp(wan_proto, "static") == 0
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
		|| strcmp(wan_proto, "usb3g_phone") == 0
#endif
	)
        strcpy(wan_interface, wan_eth);
    else
        strcpy(wan_interface, "ppp0");
/*  Date: 2009-01-20
*   Name: jimmy huang
*   Reason: for usb-3g detect status
*   Note:   0: disconnected
            1: connected
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
    if(strncmp(wan_proto, "usb3g",strlen("usb3g")) == 0
/*	Date:	2010-02-05
 *	Name:	Cosmo Chang
 *	Reason:	add usb_type=5 for Android Phone RNDIS feature
 */
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
		|| (strcmp(usb_type, "3") == 0)  || (strcmp(usb_type, "4") == 0 || (strcmp(usb_type, "5") == 0))
#endif
	){
        //DEBUG_MSG("%s,wan_proto = usb_3g\n",__FUNCTION__);
		DEBUG_MSG("%s,wan_proto = %s \n",__FUNCTION__,wan_proto);
        if(is_3g_device_connect()){
            DEBUG_MSG("%s,usb_3g is connect\n",__FUNCTION__);
            if(read_ipaddr(wan_interface) == NULL){
                strcpy(status,"disconnect");/*Internet Offline*/
            }else{
                strcpy(status,"connect");/*Internet Online*/
            }
        }else{
            strcpy(status,"disconnect");/*Internet Offline*/
            DEBUG_MSG("%s,usb_3g is not connect\n",__FUNCTION__);
        }
    }else{
        /* Check phy connect statue */
        VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);
    }

        if( strncmp("disconnect", status, 10) == 0)
            strcpy(buf,"0");/*Internet Offline*/
        else if(read_ipaddr(wan_interface) == NULL)
            strcpy(buf,"0"); /*Internet Offline*/
        else
            strcpy(buf,"1");/*Internet Online*/

#else
    /* Check phy connect statue */
    VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);

    if( strncmp("disconnect", status, 10) == 0)
        strcpy(buf,"0");/*Internet Offline*/
    else if(read_ipaddr(wan_interface) == NULL)
        strcpy(buf,"0"); /*Internet Offline*/
    else
        strcpy(buf,"1");/*Internet Online*/
#endif
    return buf;
}
#ifdef IPv6_SUPPORT

char* get_ipv6_internetonline_check(char *buf)
{
        char wan_interface[8] = {};
        char status[15] = {};
        char *wan_eth = NULL;
        char *wan_proto = NULL;
    char wanip[MAX_IPV6_IP_LEN]={};

        wan_eth = nvram_safe_get("wan_eth");
        wan_proto = nvram_safe_get("ipv6_wan_proto");

        if(strcmp(wan_proto, "ipv6_6to4") == 0)
                strcpy(wan_interface, "tun6to4");
    	else if(strcmp(wan_proto, "ipv6_6in4") == 0)
        	strcpy(wan_interface, "sit1");
        else if(strcmp(wan_proto, "ipv6_pppoe") == 0)
                strcpy(wan_interface, "ppp6");
        else
                strcpy(wan_interface, wan_eth);

        /* Check phy connect statue */
        VCTGetPortConnectState( wan_eth, VCTWANPORT0, status);

        if( strncmp("disconnect", status, 10) == 0)
                strcpy(buf,"disconnect");/*Internet Offline*/
        else if(read_ipv6addr(wan_interface,SCOPE_GLOBAL,wanip,sizeof(wanip)) == NULL)
                strcpy(buf,"disconnect"); /*Internet Offline*/
        else
                strcpy(buf,"disconnect");/*Internet Online*/
        return buf;
}

#endif

char* get_wan_connection_status(char *buf){

    char wan_proto[32]={0};
    strcpy(wan_proto, nvram_safe_get("wan_proto"));

    if(strlen(wan_proto))
        strcpy(buf, wan_statue(wan_proto));

    return buf;
}

#ifdef IPv6_SUPPORT
static int create_ipv6_status_xml(void){
    char network_status[15]={0};
    char global_wanip[5*MAX_IPV6_IP_LEN+4]={0};
    char local_wanip[5*MAX_IPV6_IP_LEN+4]={0};
    char global_lanip[MAX_IPV6_IP_LEN]={0};
    char local_lanip[MAX_IPV6_IP_LEN]={0};
    char default_gateway[MAX_IPV6_IP_LEN]={0};
    char lan_default_gateway[MAX_IPV6_IP_LEN]={0};
    char *primary_dns=NULL, *secondary_dns=NULL;
    char dns_stream[2*MAX_IPV6_IP_LEN+1]={0};
    char ajax_xml[MAX_RES_LEN_XML + sizeof(global_wanip) + sizeof(local_wanip) + sizeof(global_lanip) + 
		sizeof(local_lanip) + sizeof(default_gateway) +sizeof(lan_default_gateway)+sizeof(dns_stream)] = {0};
char lan_bridge[20] = {};
    char wan_interface[8] = {};
    char *wan_eth = NULL;
    char wan_proto[50];

	char dhcp_pd_enable[10]={0};
	char dhcp_pd[100]={0};
	char ppp6_ip[64] = {}; 
	char autoconfig_ip[64] = {}; 

    strcpy(wan_proto, nvram_safe_get("ipv6_wan_proto"));
strcpy(lan_bridge, nvram_safe_get("lan_bridge"));
    wan_eth = nvram_safe_get("wan_eth");

    if (strcmp(wan_proto, "ipv6_6to4") == 0)
        strcpy(wan_interface, "tun6to4");
    else if (strcmp(wan_proto, "ipv6_6in4") == 0)
        strcpy(wan_interface, "sit1");
    else if (strcmp(wan_proto, "ipv6_pppoe") == 0)
    		if(nvram_match("ipv6_pppoe_share", "1") == 0)
						strcpy(wan_interface, "ppp0");
				else
        strcpy(wan_interface, "ppp6");
        else if(strcmp(wan_proto, "ipv6_6rd") == 0)
        	strcpy(wan_interface, "sit2");
        else if(strcmp(wan_proto, "ipv6_autodetect") == 0) 
		if(read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip))){ 
			strcpy(wan_interface, "ppp0"); 
			strcpy(wan_proto, "ipv6_pppoe"); 
		}else if(read_ipv6addr(wan_eth, SCOPE_GLOBAL, autoconfig_ip, sizeof(autoconfig_ip))){ 
			 strcpy(wan_interface, wan_eth); 
			 strcpy(wan_proto, "ipv6_autoconfig"); 
		 }else{ 
			 strcpy(wan_interface, wan_eth); 
			 strcpy(wan_proto, "ipv6_autodetect"); 
		} 
        else{
	strcpy(wan_interface, wan_eth);
	}
    //Phy interface connect state
    VCTGetPortConnectState( wan_eth, VCTWANPORT0, network_status);
    if (strncmp("connect", network_status, 7) == 0) {
        DEBUG_MSG("phy connect\n");
        if (read_ipv6addr(wan_interface, SCOPE_GLOBAL, global_wanip, sizeof(global_wanip))==NULL)
            strcpy(network_status, "disconnect");
		// Jery Lin added for 6rd special case, 
		// although wan have no global ip address it need show connect
#ifdef IPV6_6RD
		if (strcmp(wan_proto, "ipv6_6rd") == 0 && access(SIT2_PID, F_OK) == 0)
			strcpy(network_status,"connect");
#endif


	if(strcmp(wan_proto, "ipv6_static") == 0 && nvram_match("ipv6_use_link_local", "1") == 0)
		strcpy(network_status,"connect");
		read_ipv6addr(wan_interface, SCOPE_LOCAL, local_wanip, sizeof(local_wanip));
	/*PPPoE connect or not is depand on link local address*/		
	if(strcmp(wan_proto, "ipv6_pppoe") == 0 && strlen(local_wanip) > 0)
		strcpy(network_status,"connect");


	}else{
         DEBUG_MSG("phy disconnect\n");
    }

    if (strncmp("connect", network_status, 7) == 0) {
        read_ipv6defaultgateway(wan_interface, default_gateway);
        memcpy(dns_stream, read_dns(16), sizeof(dns_stream));
        getStrtok(dns_stream, "/", "%s %s", &primary_dns, &secondary_dns);
    }

    if (read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_GLOBAL, global_lanip, sizeof(global_lanip))==NULL)
        strcpy(global_lanip,"(null)"); //avoiding ui get empty string cause show error

    if (read_ipv6addr(nvram_safe_get("lan_bridge"), SCOPE_LOCAL, local_lanip, sizeof(local_lanip))==NULL)
        strcpy(local_lanip,"(null)");

    if (strlen(global_wanip) < 1)
        strcpy(global_wanip,"(null)");

		if (strlen(local_wanip) < 1)
				strcpy(local_wanip,"(null)");    

    if (strlen(default_gateway) < 1)
        strcpy(default_gateway,"(null)");

	read_ipv6defaultgateway(lan_bridge,lan_default_gateway);
	if(strlen(lan_default_gateway)<1)
		strcpy(lan_default_gateway,"(null)");

	if ( strcmp(wan_proto, "ipv6_static")==0 || strcmp(wan_proto, "ipv6_6to4")==0 || strcmp(wan_proto, "link_local")==0 || strcmp(wan_proto, "ipv6_6rd")==0)
		strcpy(dhcp_pd_enable, "Disabled");
	else {
		if (atoi(nvram_safe_get("ipv6_dhcp_pd_enable")))
			strcpy(dhcp_pd_enable, "Enabled");
		else
			strcpy(dhcp_pd_enable, "Disabled");
	}

	if (strcmp(dhcp_pd_enable, "Enabled")==0) {
		FILE *fp_dhcppd;
		// Jery modify get dhcp-pd form /var/tmp/dhcp_pd
		// /var/tmp/dhcp_pd generated by dhcp6c
		fp_dhcppd = fopen ( "/var/tmp/dhcp_pd" , "r" );
		if ( fp_dhcppd )
		{
			char buffer[128] = {};
			if ( fgets(buffer, sizeof(buffer), fp_dhcppd ) ) {
				buffer[strlen(buffer)-1] = '\0'; //replace '\n'
				strcpy(dhcp_pd, buffer);
			}

			if ( fgets(buffer, sizeof(buffer), fp_dhcppd ) ) {
				buffer[strlen(buffer)-1] = '\0'; //replace '\n'
				strcat(dhcp_pd, "/");
				strcat(dhcp_pd, buffer);
			}
			fclose(fp_dhcppd);
		}

		if (strlen(dhcp_pd)==0)
			strcpy(dhcp_pd,"(null)");
	} else 
		strcpy(dhcp_pd,"(null)");

    xmlWrite(ajax_xml,xmlHeader);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_proto>%s</ipv6_wan_proto>", wan_proto);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_network_status>%s</ipv6_wan_network_status>", network_status);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_ip>%s</ipv6_wan_ip>", global_wanip);
	xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_ip_local>%s</ipv6_wan_ip_local>", local_wanip);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_lan_ip_global>%s</ipv6_lan_ip_global>", global_lanip);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_lan_ip_local>%s</ipv6_lan_ip_local>", local_lanip);
	xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_lan_default_gateway>%s</ipv6_lan_default_gateway>", lan_default_gateway);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_default_gateway>%s</ipv6_wan_default_gateway>", default_gateway);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_primary_dns>%s</ipv6_wan_primary_dns>", primary_dns);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_secondary_dns>%s</ipv6_wan_secondary_dns>", secondary_dns);
	
	xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_dhcp_pd_enable>%s</ipv6_dhcp_pd_enable>", dhcp_pd_enable);
	xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_dhcp_pd>%s</ipv6_dhcp_pd>", dhcp_pd);
	
    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);

    DEBUG_MSG("\n%s\n",ajax_xml);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));
    return 0;
}

static int create_ipv6_wizard_status_xml(void)
{
	FILE *fd;
	int test;
	char ajax_xml[MAX_RES_LEN_XML],buf[128];
	char cmd[32];

	sprintf(cmd, "cat %s", WIZARD_IPV6);
	if ((fd =  popen(cmd, "r")) == NULL)
                goto out;

        while (fgets(buf, sizeof(buf), fd)) {
                buf[strlen(buf) - 1] = '\0';
        }
        pclose(fd);	
	
	xmlWrite(ajax_xml, xmlHeader);
	xmlWrite(ajax_xml+strlen(ajax_xml),"<ipv6_wan_connect_percentage>%s</ipv6_wan_connect_percentage>", buf);
	xmlWrite(ajax_xml+strlen(ajax_xml), xmlTailer);
	add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
	DEBUG_MSG("\n%s\n",ajax_xml);
	ajax_add_to_response( ajax_xml, strlen(ajax_xml));
out:
	return 0;
}
#endif

static int create_wan_detect_status_xml(int init)
{
	char ajax_xml[MAX_RES_LEN_XML];
    char wan_ip[192]={0};
    char wan_type[20];
    char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;

    strcpy(wan_type, "detecting");

    if (init) {
    	_system("pppoe -I %s -d -A -t 1", nvram_safe_get("wan_eth"));
    }
    else {
        /* wan info */
        memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
        getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);

        if (strcmp(wanip, "0.0.0.0")) {
            strcpy(wan_type, "dhcpc");
        }
        else {
        	FILE *fp;
        	char buf[20];

        	fp = fopen("/var/tmp/pppoe_find", "r");
            if (fp) {
            	fgets(buf, 16, fp);

            	if (buf[0] == '1') {
            	    strcpy(wan_type, "pppoe");
            	}
            	fclose(fp);
            }
        }
    }

	xmlWrite(ajax_xml, xmlHeader);
	xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_detect_type>%s</wan_detect_type>", wan_type);
	xmlWrite(ajax_xml+strlen(ajax_xml), xmlTailer);
	add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
	ajax_add_to_response( ajax_xml, strlen(ajax_xml));

	return 0;
}

static int create_wipnp_status_xml(void)
{
	char ajax_xml[MAX_RES_LEN_XML];

	xmlWrite(ajax_xml, xmlHeader);
	xmlWrite(ajax_xml+strlen(ajax_xml),"<wipnp_status>%d</wipnp_status>", nvram_WiPNP());
	xmlWrite(ajax_xml+strlen(ajax_xml), xmlTailer);
	add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
	ajax_add_to_response( ajax_xml, strlen(ajax_xml));

	return 0;
}

/*  Date:   2009-04-17
*   Name:   jimmy huang
*   Reason: Add definition for miniupnpd-1.3 to use same features
*   Note:   This feature only works for libupnp or miniupnpd-1.3 (not include miniupnpd-1.0-RC7)
*/
//#if defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)
#if (defined(UPNP_ATH_LIBUPNP) && defined(UPNP_PORTMAPPING_SAVE)) || \
        (defined(UPNP_ATH_MINIUPNPD_1_3) && defined(UPNP_ATH_MINIUPNPD_1_3_LEASE_FILES))
#ifdef UPNP_ATH_MINIUPNPD_RC
#error UPNP_ATH_MINIUPNPD_RC and UPNP_ATH_LIBUPNP can not be selected simultaneously
#endif
/*
*   Date: 2009-2-24
*   Name: Ken_Chiang
*   Reason: modify for show upnp dynamic rule can up to 200 rules.
*   Notice :
*/
/*
static int create_upnp_dynamic_portmap_xml(void){
    int i = 0;
    char ajax_xml[MAX_RES_LEN_XML] = {0};

//
    These codes and definitions
    are related to
    - libupnp-1.6.3/upnp/sample/linuxigd-1.0/
        main.c
        gatedevice.c
        gatedevice.h
    - httpd
        cmobasicapi.c

//

//#define UPNP_RULES_SHOW_UI_MAX 200
#define UPNP_FORWARD_RULES_FILE "/tmp/upnp_portmapping_rules"
#define UPNP_FORWARD_RULES_FILE_TMP "/tmp/upnp_portmapping_rules.tmp"
#define PRO_LEN 5
#define HOST_LEN 16
#define PORT_LEN 6
#define DESC_LEN 64
#define DURATION_LEN 12
#define NAME_SIZE  256
#define FILE_LINE_LEN (PRO_LEN + 2*HOST_LEN + 2*PORT_LEN + DESC_LEN + 2*DURATION_LEN + NAME_SIZE)

    char buf[FILE_LINE_LEN] = {0};
    char *protocol_in = NULL, *desc_in = NULL;
    char *remoteHost_in = NULL, *externalPort_in = NULL;
    char *internalPort_in = NULL, *internalClient_in = NULL;
    char *duration_in = NULL;
    char *p_head = NULL, *p_tail = NULL;

    FILE *fp;

    xmlWrite(ajax_xml,xmlHeader);

    if((fp = fopen(UPNP_FORWARD_RULES_FILE,"r"))!=NULL){
        DEBUG_MSG("upnp list file open\n");
        while(!feof(fp)){
            fgets(buf,sizeof(buf),fp);
            if(!feof(fp) && (buf[0] != '\n')){
                p_head = buf;
                p_tail = strchr(buf,'/');

                if(p_head == p_tail){
                    p_tail = p_head;
                    desc_in = NULL;
                }else{
                    p_tail[0]='\0';
                    desc_in = p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    protocol_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0]='\0';
                    protocol_in = p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    remoteHost_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0]='\0';
                    remoteHost_in= p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    externalPort_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    externalPort_in = p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    internalClient_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    internalClient_in = p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    internalPort_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    internalPort_in = p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    duration_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    duration_in = p_head;
                }
//
These codes can not handle the situation
/TCP/172.21.35.200/3000/192.168.0.101/2000/3600/123456789/uuid:33333333
Formal format:
test1/TCP/172.21.35.200/3000/192.168.0.101/2000/3600/123456789/uuid:33333333
                getStrtok(buf,"/","%s %s %s %s %s %s %s %s %s"
                            ,&desc_in,&protocol_in
                            ,&remoteHost_in,&externalPort_in
                            ,&internalClient_in,&internalPort_in
                            ,&duration_in,&expireTime_in,&devudn_in);
//
//
*   Date: 2009-1-19
*   Name: Ken_Chiang
*   Reason: modify for show upnp dynamic rule.
*   Notice :
//
                xmlWrite(ajax_xml+strlen(ajax_xml),"<Description_%d>%s</Description_%d>"
                        ,i,desc_in ? desc_in : "(null)",i);
                xmlWrite(ajax_xml+strlen(ajax_xml),"<Protocol_%d>%s</Protocol_%d>"
                        ,i,protocol_in,i);
                xmlWrite(ajax_xml+strlen(ajax_xml),"<RemoteHost_%d>%s</RemoteHost_%d>"
                        ,i,remoteHost_in ? remoteHost_in : "(null)",i);
                xmlWrite(ajax_xml+strlen(ajax_xml),"<PublicPort_%d>%s</PublicPort_%d>"
                        ,i,externalPort_in,i);
                xmlWrite(ajax_xml+strlen(ajax_xml),"<InternalClient_%d>%s</InternalClient_%d>"
                        ,i,internalClient_in,i);
                xmlWrite(ajax_xml+strlen(ajax_xml),"<PrivatePort_%d>%s</PrivatePort_%d>"
                        ,i,internalPort_in ? internalPort_in : "(null)",i);
                xmlWrite(ajax_xml+strlen(ajax_xml),"<Duration_%d>%s</Duration_%d>"
                        ,i,duration_in ? duration_in : "(null)",i);

                i++;
            }
        }
        //desc, protocol, remoteHost, externalPort, internalClient, internalPort,duration, desc,devudn
        //we need to show
        xmlWrite(ajax_xml+strlen(ajax_xml),"<Listnumber>%d</Listnumber>",i);
        fclose(fp);
    }else{
        DEBUG_MSG("no upnp list\n");
        //xmlWrite(ajax_xml+strlen(ajax_xml),"<Description_%d></Description_%d>",i,i);
        //xmlWrite(ajax_xml+strlen(ajax_xml),"<Protocol_%d></Protocol_%d>",i,i);
        //xmlWrite(ajax_xml+strlen(ajax_xml),"<RemoteHost_%d></RemoteHost_%d>",i,i);
        //xmlWrite(ajax_xml+strlen(ajax_xml),"<PublicPort_%d></PublicPort_%d>",i,i);
        //xmlWrite(ajax_xml+strlen(ajax_xml),"<InternalClient_%d></InternalClient_%d>",i,i);
        //xmlWrite(ajax_xml+strlen(ajax_xml),"<PrivatePort_%d></PrivatePort_%d>",i,i);
        //xmlWrite(ajax_xml+strlen(ajax_xml),"<Duration_%d></Duration_%d>",i,i);
        xmlWrite(ajax_xml+strlen(ajax_xml),"<Listnumber>0</Listnumber>");
    }

    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));

    return 0;
}
*/

#define UPNP_RULES_SHOW_UI_MAX 200
#define UPNP_FORWARD_RULES_FILE "/tmp/upnp_portmapping_rules"
#define UPNP_FORWARD_RULES_FILE_TMP "/tmp/upnp_portmapping_rules.tmp"
#define PRO_LEN 5
#define HOST_LEN 16
#define PORT_LEN 6
#define DESC_LEN 64
#define DURATION_LEN 12
#define NAME_SIZE  256
#define FILE_LINE_LEN (PRO_LEN + 2*HOST_LEN + 2*PORT_LEN + DESC_LEN + 2*DURATION_LEN + NAME_SIZE)

/*
    These codes and definitions
    are related to
    - libupnp-1.6.3/upnp/sample/linuxigd-1.0/
        main.c
        gatedevice.c
        gatedevice.h
    - httpd
        cmobasicapi.c
*/

static int create_upnp_dynamic_portmap_xml(void){
    int i = 0;
    char ajax_xml[MAX_RES_LEN_XML] = {0};
    char buf[FILE_LINE_LEN] = {0};
    FILE *fp;

    xmlWrite(ajax_xml,xmlHeader);

    if((fp = fopen(UPNP_FORWARD_RULES_FILE,"r"))!=NULL){
        DEBUG_MSG("upnp list file open\n");
        while(!feof(fp)){
            fgets(buf,sizeof(buf),fp);
            if(!feof(fp) && (buf[0] != '\n')){
                i++;
            }
        }
        //we need to show
        DEBUG_MSG("%s: list=%d\n",__func__,i);
        if(i>UPNP_RULES_SHOW_UI_MAX){
            i=UPNP_RULES_SHOW_UI_MAX;
        }
        xmlWrite(ajax_xml+strlen(ajax_xml),"<Listnumber>%d</Listnumber>",i);
        if(i)
            xmlWrite(ajax_xml+strlen(ajax_xml),"<Pagenumber>%d</Pagenumber>",i%10 ? 1+(i/10) : (i/10));
        else
            xmlWrite(ajax_xml+strlen(ajax_xml),"<Pagenumber>0</Pagenumber>");

        fclose(fp);
    }else{
        DEBUG_MSG("no upnp list\n");
        xmlWrite(ajax_xml+strlen(ajax_xml),"<Listnumber>0</Listnumber>");
        xmlWrite(ajax_xml+strlen(ajax_xml),"<Pagenumber>0</Pagenumber>");
    }

    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));

    return 0;
}

static int create_upnp_dynamic_portmappage_xml(int page_index){
    int i = 0,j = 0,page_star=0,rule_page_max_num=10;
    char ajax_xml[MAX_RES_LEN_XML] = {0};
    char buf[FILE_LINE_LEN] = {0};
    char *protocol_in = NULL, *desc_in = NULL;
    char *remoteHost_in = NULL, *externalPort_in = NULL;
    char *internalPort_in = NULL, *internalClient_in = NULL;
    char *duration_in = NULL;
    char *expireTime_in = NULL;
    char *p_head = NULL, *p_tail = NULL;
    long int shown_time = 0;
    FILE *fp;

    xmlWrite(ajax_xml,xmlHeader);

    page_star=rule_page_max_num*page_index;
    DEBUG_MSG("%s: page_star=%d\n",__func__,page_star);
/*
Formats for
miniupnpd-1.3:      (upnpredirect.c, lease_file_add())
description / remote ip / ex-port / proto / int-ip / int-port / leasetime / expire time / "Reserved"

libupnp+linux-igd:
description / proto / remote ip  / ex-port/ int-ip     / int-port / leasetime / expire time / udev
test1      / TCP   / 61.61.1.61 / 3000   / 10.10.0.10  /  2000   /  3600    / 123456789 / uuid:33333333
*/
    if((fp = fopen(UPNP_FORWARD_RULES_FILE,"r"))!=NULL){
        DEBUG_MSG("upnp list file open\n");
        while(!feof(fp)){
            fgets(buf,sizeof(buf),fp);
            if(!feof(fp) && (buf[0] != '\n')){
                p_head = buf;
                p_tail = strchr(buf,'/');

                if(p_head == p_tail){
                    p_tail = p_head;
                    desc_in = NULL;
                }else{
                    p_tail[0]='\0';
                    desc_in = p_head;
                }
#ifdef UPNP_ATH_LIBUPNP
                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    protocol_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0]='\0';
                    protocol_in = p_head;
                }
#endif
                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    remoteHost_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0]='\0';
                    remoteHost_in= p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    externalPort_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    externalPort_in = p_head;
                }
#ifdef UPNP_ATH_MINIUPNPD_1_3
                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    protocol_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0]='\0';
                    protocol_in = p_head;
                }
#endif
                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    internalClient_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    internalClient_in = p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    internalPort_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    internalPort_in = p_head;
                }

                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    duration_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    duration_in = p_head;
                }

                /*  Date:   2009-04-22
                *   Name:   jimmy huang
                *   Reason: If the rules is dynamic rule (lease duration time != 0)
                *           Time shown on Web GUI should be remain time,
                *           not the Duration Time
                *   Note:   Refer to UPnP_IGD_WANIPConnection 1.0.pdf
                *           page 14, section 2.2.21
                */
                p_head = p_tail + 1;
                if(p_head[0] == '/'){
                    p_tail = p_head;
                    expireTime_in = NULL;
                }else{
                    p_tail = strchr(p_head,'/');
                    p_tail[0] = '\0';
                    expireTime_in = p_head;
                }

                if((duration_in != NULL) && (atoi(duration_in) > 0)){
                    // this is a dynamic rule (lease duration time > 0)
                    if(expireTime_in && (atoi(expireTime_in) > 0)){
                        // remain time > 0 (rule still valid)
                        shown_time = atol(expireTime_in) - time(NULL);
                        if(shown_time < 0)
                            shown_time = 0;
                    }else{
                        // remain time <= 0 (rule is expired)
                        shown_time = 0;
                    }
                }else{
                    // this is static rule (lease duration time = 0)
                    shown_time = atol(duration_in);
                }
/*
These codes can not handle the situation
/TCP/172.21.35.200/3000/192.168.0.101/2000/3600/123456789/uuid:33333333
Formal format:
test1/TCP/172.21.35.200/3000/192.168.0.101/2000/3600/123456789/uuid:33333333
                getStrtok(buf,"/","%s %s %s %s %s %s %s %s %s"
                            ,&desc_in,&protocol_in
                            ,&remoteHost_in,&externalPort_in
                            ,&internalClient_in,&internalPort_in
                            ,&duration_in,&expireTime_in,&devudn_in);
*/
                if(i>=page_star && i< (page_star+rule_page_max_num)){
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<Description_%d>%s</Description_%d>"
                            ,i,desc_in ? desc_in : "(null)",i);
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<Protocol_%d>%s</Protocol_%d>"
                            ,i,protocol_in,i);
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<RemoteHost_%d>%s</RemoteHost_%d>"
                            ,i,remoteHost_in ? remoteHost_in : "(null)",i);
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<PublicPort_%d>%s</PublicPort_%d>"
                            ,i,externalPort_in,i);
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<InternalClient_%d>%s</InternalClient_%d>"
                            ,i,internalClient_in,i);
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<PrivatePort_%d>%s</PrivatePort_%d>"
                            ,i,internalPort_in ? internalPort_in : "(null)",i);
                    /*  Date:   2009-04-22
                    *   Name:   jimmy huang
                    *   Reason: If the rules is a dynamic rule (lease duration time != 0)
                    *           Time shown on Web GUI should be remain time,
                    *           not the Duration Time
                    *   Note:   Refer to UPnP_IGD_WANIPConnection 1.0.pdf
                    *           page 14, section 2.2.21
                    */
                    /*
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<Duration_%d>%s</Duration_%d>"
                            ,i,duration_in ? duration_in : "(null)",i);
                    */
                    xmlWrite(ajax_xml+strlen(ajax_xml),"<Duration_%d>%ld</Duration_%d>"
                            ,i,shown_time,i);

                    j++;
                }
                i++;
            }
        }
        //desc, protocol, remoteHost, externalPort, internalClient, internalPort,duration, desc,devudn
        //we need to show
        xmlWrite(ajax_xml+strlen(ajax_xml),"<Pagelistnumber>%d</Pagelistnumber>",j);
        fclose(fp);
    }else{
        DEBUG_MSG("no upnp list\n");
        xmlWrite(ajax_xml+strlen(ajax_xml),"<Pagelistnumber>0</Pagelistnumber>");
    }

    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));

    return 0;
}
#endif
/* ----------------------------------------------------- */

static int create_device_status_xml(void){
    char ajax_xml[MAX_RES_LEN_XML] = {0};
    char *wanip, *netmask, *gateway, *primary_dns, *secondary_dns;
    char wan_ip[192]={0};
    char cable_status_buf[64]={0},network_status_buf[64]={0},connection_status_buf[64]={0},wlan_channel_buf[64]={0};
char *lan_default_gateway=NULL;
#ifdef RPPPOE
    char *phy_wanip="0.0.0.0", *phy_netmask="0.0.0.0", *phy_gateway="0.0.0.0", *phy_primary_dns="0.0.0.0", *phy_secondary_dns="0.0.0.0";
    char phy_wan_ip[192]={0};
#endif

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
	char *usb_type=NULL;
	usb_type = nvram_safe_get("usb_type");
#endif

    /* wan info */
    memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
    getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);
    xmlWrite(ajax_xml,xmlHeader);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_ip>%s</wan_ip>", wanip);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_netmask>%s</wan_netmask>", netmask);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_default_gateway>%s</wan_default_gateway>", gateway);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_primary_dns>%s</wan_primary_dns>", primary_dns);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_secondary_dns>%s</wan_secondary_dns>", secondary_dns);
    /*
    VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, cable_status_buf);
    */
    /*  Date: 2009-01-20
    *   Name: jimmy huang
    *   Reason: for usb-3g detect status
    *   Note:   0: disconnected
                1: connected
    */
#ifdef CONFIG_USER_3G_USB_CLIENT
    if(strncmp(nvram_safe_get("wan_proto"), "usb3g",strlen("usb3g")) == 0
/*	Date:	2010-02-05
 *	Name:	Cosmo Chang
 *	Reason:	add usb_type=5 for Android Phone RNDIS feature
 */
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
		|| (strcmp(usb_type, "3") == 0)  || (strcmp(usb_type, "4") == 0 || (strcmp(usb_type, "5") == 0))
#endif
	){
        DEBUG_MSG("%s,wan_proto = usb_3g\n",__FUNCTION__);
        if(is_3g_device_connect()){
            strcpy(cable_status_buf,"connect");
            DEBUG_MSG("%s,usb_3g is %s\n",__FUNCTION__,cable_status_buf);
        }else{
            strcpy(cable_status_buf,"disconnect");
            DEBUG_MSG("%s,usb_3g is %s\n",__FUNCTION__,cable_status_buf);
        }
    }else
#endif
    VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, cable_status_buf);

#ifdef IPV6_DSLITE
	int aftr_enable = 0;
	char aftr_address[256] = {};
	FILE *fp;
    
	if(strcmp(nvram_safe_get("wan_proto"), "dslite") == 0 && access(DSLITE_PID, F_OK) == 0 &&
	   strcmp(cable_status_buf, "disconnect") != 0)
		strcpy(network_status_buf, "1");
	else
		strcpy(network_status_buf, "0");

        fp = fopen("/var/tmp/aftr_address","r");
        if(fp){
		if(fgets(aftr_address, 256, fp))
			aftr_address[strlen(aftr_address) - 1] = '\0';
                fclose(fp);
        }else
		strncpy(aftr_address, "NULL", sizeof(aftr_address));

	aftr_enable = atoi(nvram_safe_get("wan_dslite_dhcp"));    
#endif

    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_cable_status>%s</wan_cable_status>", cable_status_buf);
	xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_network_status>%s</wan_network_status>", 
#ifdef IPV6_DSLITE
		strcmp(nvram_safe_get("wan_proto"), "dslite") == 0 ? network_status_buf : get_internetonline_check(network_status_buf));
#else
		get_internetonline_check(network_status_buf));
#endif
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_connection_status>%s</wan_connection_status>", get_wan_connection_status(connection_status_buf));
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_uptime>%lu</wan_uptime>", get_wan_connect_time(nvram_safe_get("wan_proto"), 1));
#ifdef IPV6_DSLITE
		xmlWrite(ajax_xml+strlen(ajax_xml), "<aftr_address>%s</aftr_address>", aftr_address); 
		xmlWrite(ajax_xml+strlen(ajax_xml), "<aftr_enable>%d</aftr_enable>", aftr_enable);
#endif
#ifdef RPPPOE
    /* wan physical info for RUSSIA*/
    memcpy(phy_wan_ip, read_current_wanphy_ipaddr(), sizeof(phy_wan_ip));
    getStrtok(phy_wan_ip, "/", "%s %s %s %s %s", &phy_wanip, &phy_netmask, &phy_gateway, &phy_primary_dns, &phy_secondary_dns);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<phy_wan_ip>%s</phy_wan_ip>", phy_wanip);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<phy_wan_netmask>%s</phy_wan_netmask>", phy_netmask);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<phy_wan_default_gateway>%s</phy_wan_default_gateway>", phy_gateway);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<phy_wan_primary_dns>%s</phy_wan_primary_dns>", phy_primary_dns);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<phy_wan_secondary_dns>%s</phy_wan_secondary_dns>", phy_secondary_dns);
#endif

#if defined(ATH_AR938X)
    FILE *fd;
    char config_buf[256];
    int  wlan0 = 0;
    int  wlan1 = 0;

    if (fd = popen("ifconfig", "r")) {
        while (!feof(fd)) {
            fgets(config_buf, sizeof(config_buf), fd);

            if (!wlan0) {
                if (strstr(config_buf, WLAN0_ETH))
                    wlan0 = 1;
            }

            if (!wlan1) {
            	if(nvram_match("wlan0_enable", "1")==0){
                	if (strstr(config_buf, WLAN1_ETH))
                    	wlan1 = 1;
		        }
		        else{
                	if (strstr(config_buf, WLAN0_ETH))
                    	wlan1 = 1;
		        }
            }        
            
            if (wlan0 && wlan1)
            	break;
        }
        pclose(fd);
    }
#endif

    /* wlan info */
    if (nvram_match("wlan0_enable", "0") == 0)
        xmlWrite(ajax_xml+strlen(ajax_xml), "<wlan_channel>(null)</wlan_channel>");
    else
    {
        get_channel(wlan_channel_buf);
        if (strcmp(wlan_channel_buf, "unknown") == 0) {
            if (check_rc_status() == 'i')
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan_channel>(null)</wlan_channel>");
            else
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan_channel>%s</wlan_channel>", nvram_safe_get("wlan0_channel"));
        }
        else {
#if defined(ATH_AR938X)
            if (!wlan0)
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan_channel>(null)</wlan_channel>");
            else
#endif
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan_channel>%s</wlan_channel>", wlan_channel_buf);
        }
    }

#ifdef USER_WL_ATH_5GHZ
    if (nvram_match("wlan1_enable", "0") == 0)
        xmlWrite(ajax_xml+strlen(ajax_xml), "<wlan1_channel>(null)</wlan1_channel>"); 
    else
    {	//5G interface 2
        get_5g_channel(wlan_channel_buf);
        if (strcmp(wlan_channel_buf, "unknown") == 0) {
            if (check_rc_status() == 'i')
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan1_channel>(null)</wlan1_channel>");
            else
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan1_channel>%s</wlan1_channel>", nvram_safe_get("wlan1_channel"));
        }
        else {
#if defined(ATH_AR938X)

            if (!wlan1)
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan1_channel>(null)</wlan1_channel>");
            else
#endif
                xmlWrite(ajax_xml+strlen(ajax_xml),"<wlan1_channel>%s</wlan1_channel>", wlan_channel_buf);
        }
    } 
#endif

    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));

    return 0;
}

static int create_internet_on_line_xml(void){
    char ajax_xml[MAX_RES_LEN_XML] = {0};
    char buf[64]={0};

    xmlWrite(ajax_xml,xmlHeader);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<wan_network_status>%s</wan_network_status>", get_internetonline_check(buf));
    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));

    return 0;
}


static int create_err_xml(void)
{
    char ajax_xml[MAX_RES_LEN_XML] = {0};

    xmlWrite(ajax_xml,xmlHeader);

    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    /*  Date:   2009-11-18
    *   Name:   Tina Tsao
    *   Reason: Fixed when device rebooting, open XML wireless status will case error.
    *   Note:
    */
    add_ajax_headers(404, "Not Found", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));
    return 0;
}

static int create_wireless_list_xml(void)
{
    char ajax_xml[MAX_RES_LEN_XML] = {0};
    T_WLAN_STAION_INFO* wireless_status = NULL;
    int i;
	char buff[128];

    memset(ajax_xml, 0, MAX_RES_LEN_XML);
    xmlWrite(ajax_xml, xmlHeader);

    wireless_status = get_stalist();
	if (wireless_status) {
        sprintf(buff, "<num_2>%d</num_2>", wireless_status->num_sta ? : 0);
        strcat(ajax_xml, buff);

        if (wireless_status != NULL) {
            for(i = 0 ; i < wireless_status->num_sta; i++) {
            	sprintf(buff, "<mac_2>%s</mac_2>", wireless_status->p_st_wlan_sta[i].mac_addr ? wireless_status->p_st_wlan_sta[i].mac_addr : "");
                strcat(ajax_xml, buff);

                sprintf(buff, "<ip_2>%s</ip_2>", wireless_status->p_st_wlan_sta[i].ip_addr ? wireless_status->p_st_wlan_sta[i].ip_addr : "");
                strcat(ajax_xml, buff);
                
                sprintf(buff, "<type_2>%s</type_2>", wireless_status->p_st_wlan_sta[i].mode ? wireless_status->p_st_wlan_sta[i].mode : "");
                strcat(ajax_xml, buff);

                sprintf(buff, "<rate_2>%s</rate_2>", wireless_status->p_st_wlan_sta[i].rate ? wireless_status->p_st_wlan_sta[i].rate : "");
                strcat(ajax_xml, buff);

                sprintf(buff, "<rsi_2>%s</rsi_2>", wireless_status->p_st_wlan_sta[i].rssi ? wireless_status->p_st_wlan_sta[i].rssi : "");
                strcat(ajax_xml, buff);
            }
        }
    }

#ifdef USER_WL_ATH_5GHZ
    wireless_status = get_5g_stalist();
	if (wireless_status) {
        sprintf(buff, "<num_5>%d</num_5>", wireless_status->num_sta ? : 0);
        strcat(ajax_xml, buff);

        if (wireless_status != NULL) {
            for(i = 0 ; i < wireless_status->num_sta; i++) {
            	sprintf(buff, "<mac_5>%s</mac_5>", wireless_status->p_st_wlan_sta[i].mac_addr ? wireless_status->p_st_wlan_sta[i].mac_addr : "");
                strcat(ajax_xml, buff);

                sprintf(buff, "<ip_5>%s</ip_5>", wireless_status->p_st_wlan_sta[i].ip_addr ? wireless_status->p_st_wlan_sta[i].ip_addr : "");
                strcat(ajax_xml, buff);

                sprintf(buff, "<type_5>%s</type_5>", wireless_status->p_st_wlan_sta[i].mode ? wireless_status->p_st_wlan_sta[i].mode : "");
                strcat(ajax_xml, buff);

                sprintf(buff, "<rate_5>%s</rate_5>", wireless_status->p_st_wlan_sta[i].rate ? wireless_status->p_st_wlan_sta[i].rate : "");
                strcat(ajax_xml, buff);

                sprintf(buff, "<rsi_5>%s</rsi_5>", wireless_status->p_st_wlan_sta[i].rssi ? wireless_status->p_st_wlan_sta[i].rssi : "");
                strcat(ajax_xml, buff);
            }
        }
    }
#endif

    strcat(ajax_xml, xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));
        
    return 0;
}

#ifdef USER_WL_ATH_5GHZ
static int create_wireless_5g_list_xml(void)
{
    char ajax_xml[MAX_RES_LEN_XML] = {0};
    T_WLAN_STAION_INFO* wireless_status = NULL;
	int i = 0, j;
	FILE *fp;
	char buff[1024],ip_temp[24] = {0},mac_addr[18] = {0},ip[24] = {0};
	char tmp[12], mac[24], dev[12];

    wireless_status = get_5g_stalist();
	if (wireless_status == NULL)
		return 0;

    memset(ajax_xml, 0, MAX_RES_LEN_XML);
    xmlWrite(ajax_xml,xmlHeader);
    xmlWrite(ajax_xml+strlen(ajax_xml),"<num>%d</num>", wireless_status->num_sta? : 0);

    if (wireless_status != NULL) {
        for(i = 0 ; i < wireless_status->num_sta; i++) {
            xmlWrite(ajax_xml+strlen(ajax_xml),"<mac>%s</mac>", wireless_status->p_st_wlan_sta[i].mac_addr ? wireless_status->p_st_wlan_sta[i].mac_addr : "");
            xmlWrite(ajax_xml+strlen(ajax_xml),"<ip>%s</ip>",wireless_status->p_st_wlan_sta[i].ip_addr ? wireless_status->p_st_wlan_sta[i].ip_addr : "");
            xmlWrite(ajax_xml+strlen(ajax_xml),"<type>%s</type>",wireless_status->p_st_wlan_sta[i].mode ? wireless_status->p_st_wlan_sta[i].mode : "");
            xmlWrite(ajax_xml+strlen(ajax_xml),"<rate>%s</rate>",wireless_status->p_st_wlan_sta[i].rate ? wireless_status->p_st_wlan_sta[i].rate : "");
            xmlWrite(ajax_xml+strlen(ajax_xml),"<rsi>%s</rsi>",wireless_status->p_st_wlan_sta[i].rssi ? wireless_status->p_st_wlan_sta[i].rssi : "");
        }
    }

    xmlWrite(ajax_xml+strlen(ajax_xml),xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response( ajax_xml, strlen(ajax_xml));

    return 0;
}
#endif

static int  start_ping4(char *host, int *ttl);
static int  start_ping6(char *host, int *ttl);

void do_ping_action(char *url, FILE *stream, int len, char *boundary)
{
    char ajax_xml[MAX_RES_LEN_XML] = {0};
    int reply_time = -1, ttl = 0;
    char *ptr;

    ptr = strstr(url, "=");
    if (ptr) {
        ptr++;
        if (*ptr == '1') {
            /* IPv6 */
            reply_time = start_ping6(ptr + 2, &ttl);
        }
        else {
            /* IPv4 */
            reply_time = start_ping4(ptr + 2, &ttl);
        }
    }

    xmlWrite(ajax_xml, xmlHeader);

    xmlWrite(ajax_xml+strlen(ajax_xml), "<reply_millis>%d</reply_millis>", reply_time);
    xmlWrite(ajax_xml+strlen(ajax_xml), "<ttl>%d</ttl>", ttl);

    xmlWrite(ajax_xml+strlen(ajax_xml), xmlTailer);
    add_ajax_headers(200, "OK", NULL, "text/xml", -1, NULL);
    ajax_add_to_response(ajax_xml, strlen(ajax_xml));

    return;
}


//
// ping.c
//
// Send ICMP echo request to network host
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#define ICMP_ECHO       8
#define ICMP_ECHOREPLY  0
#define ICMP_MIN        8        // Minimum 8 byte icmp packet (just header)

//
// IP header
//
struct iphdr
{
  unsigned int h_len:4;          // length of the header
  unsigned int version:4;        // Version of IP
  unsigned char tos;             // Type of service
  unsigned short total_len;      // total length of the packet
  unsigned short ident;          // unique identifier
  unsigned short frag_and_flags; // flags
  unsigned char  ttl;
  unsigned char proto;           // protocol (TCP, UDP etc)
  unsigned short checksum;       // IP checksum

  unsigned int source_ip;
  unsigned int dest_ip;
} __attribute__ ((packed));

//
// ICMP header
//
struct icmphdr
{
  unsigned char i_type;
  unsigned char i_code; // type sub code
  unsigned short i_cksum;
  unsigned short i_id;
  unsigned short i_seq;
} __attribute__ ((packed));

#define DEF_PACKET_SIZE 32
#define MAX_PACKET_SIZE 256

#include <sys/socket.h>
#include <sys/timeb.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

static unsigned short ping_id = 0;
static unsigned short seq_no = 0;
static struct timeb start, end;

static unsigned short checksum(unsigned short *buffer, int size)
{
    unsigned long cksum = 0;

    while (size > 1) {
        cksum += *buffer++;
        size -= sizeof(unsigned short);
    }

    if (size) cksum += *(unsigned char *) buffer;

    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (unsigned short) (~cksum);
}

static void fill_icmp_data(char *icmp_data, int datasize)
{
    struct icmphdr *icmp_hdr;
    char *datapart;

    icmp_hdr = (struct icmphdr *) icmp_data;

    icmp_hdr->i_type = ICMP_ECHO;
    icmp_hdr->i_code = 0;
    icmp_hdr->i_id = ++ping_id;
    icmp_hdr->i_cksum = 0;
    icmp_hdr->i_seq = ++seq_no;

    datapart = icmp_data + sizeof(struct icmphdr);
    memset(datapart, 'E', datasize - sizeof(struct icmphdr));
}

int start_ping4(char *host, int *ttl)
{
    int sockraw;
    struct sockaddr_in dest, from;
    int datasize = DEF_PACKET_SIZE;
    socklen_t fromlen = sizeof(from);
    struct timeval timeout;
    char icmp_data[MAX_PACKET_SIZE];
    char recvbuf[MAX_PACKET_SIZE];

    struct addrinfo hints, *res;
    struct in_addr addr;

    ftime(&start);
    memset(&dest, 0, sizeof(dest));
    memset(&hints, 0, sizeof(hints));
    memset(icmp_data, 0, MAX_PACKET_SIZE);
    memset(recvbuf, 0, MAX_PACKET_SIZE);

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        printf("Ping: getaddrinfo error\n");
        return -1;
    }

    addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    DEBUG_MSG("Ping ip address : %s\n", inet_ntoa(addr));
    freeaddrinfo(res);

    dest.sin_addr.s_addr = addr.s_addr;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);

    sockraw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockraw < 0) {
        printf("Ping: Error socket\n");
        return -1;
    }

    timeout.tv_sec = 2000;
    timeout.tv_usec = 0;
    if (setsockopt(sockraw, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout)) < 0) {
        printf("Set recv socket error\n");
        return -1;
    }

    if (setsockopt(sockraw, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout)) < 0) {
        printf("Set send socket error\n");
        return -1;
    }

    DEBUG_MSG("PING %s : %d data bytes\n\n", host, datasize);

    struct icmphdr *icmphdr = (struct icmphdr *) &icmp_data;
    fill_icmp_data(icmp_data, datasize + sizeof(struct icmphdr));
    icmphdr->i_cksum = checksum((unsigned short *) icmp_data, datasize + sizeof(struct icmphdr));

    if (sendto(sockraw, icmp_data, datasize + sizeof(struct icmphdr), 0, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
        printf("Sendto() timed out\n");
        return -1;
    }

    if (recvfrom(sockraw, recvbuf, MAX_PACKET_SIZE, 0, (struct sockaddr *) &from, &fromlen) < 0) {
        printf("Recvfrom() timed out\n");
        return -1;
    }

    close(sockraw);

    /* decode packet */
    struct iphdr *iphdr = (struct iphdr *) recvbuf;
    icmphdr = (struct icmphdr *) (recvbuf + sizeof(struct iphdr));

    if (icmphdr->i_type == ICMP_ECHO) {
        if (iphdr->source_ip != iphdr->dest_ip) {
            DEBUG_MSG("Invalid echo pkt recvd\n");
            return -1;
        }
    }
    else if (icmphdr->i_type != ICMP_ECHOREPLY) {
        printf("Non-echo type %d recvd\n", icmphdr->i_type);
        return -1;
    }

    if (icmphdr->i_id != ping_id) {
        printf("Wrong ID : %d   %d\n", icmphdr->i_id, ping_id);
        return -1;
    }

    DEBUG_MSG("Ping: TTL=%d\n", iphdr->ttl);
    *ttl = iphdr->ttl;
    ftime(&end);

    DEBUG_MSG("Ping: time=%d ms\n", (end.time - start.time) * 1000 + (end.millitm - start.millitm));
    return ((end.time - start.time) * 1000 + (end.millitm - start.millitm));
}

#include <netinet/icmp6.h>
#include <netinet/ip6.h>

static void fill_icmp6_data(char *icmp_data, int datasize)
{
    struct icmp6_hdr *icmp_hdr;
    char *datapart;

    icmp_hdr = (struct icmp6_hdr *) icmp_data;

    icmp_hdr->icmp6_type = ICMP6_ECHO_REQUEST;
    icmp_hdr->icmp6_code = 0;
    icmp_hdr->icmp6_id = ++ping_id;
    icmp_hdr->icmp6_cksum = 0;
    icmp_hdr->icmp6_seq = ++seq_no;

    datapart = icmp_data + sizeof(struct icmp6_hdr);
    memset(datapart, 'E', datasize - sizeof(struct icmp6_hdr));
}

int start_ping6(char *host, int *ttl)
{
    struct sockaddr_in6 dest, from;
    int datasize = DEF_PACKET_SIZE;
    socklen_t fromlen = sizeof(from);
    struct timeval timeout;
    char icmp_data[MAX_PACKET_SIZE];
    char recvbuf[MAX_PACKET_SIZE];

    struct addrinfo hints, *res;
    int pingsock, sockopt;

    ftime(&start);
    memset(&dest, 0, sizeof(dest));
    memset(&hints, 0, sizeof(hints));
    memset(icmp_data, 0, MAX_PACKET_SIZE);
    memset(recvbuf, 0, MAX_PACKET_SIZE);

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET6;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        printf("Ping6: getaddrinfo error\n");
        return -1;
    }

    memcpy(dest.sin6_addr.s6_addr, ((struct sockaddr_in6 *)(res->ai_addr))->sin6_addr.s6_addr, sizeof(struct in6_addr));
    freeaddrinfo(res);

#ifdef HTTPD_DEBUG_AJAX
    char str[INET6_ADDRSTRLEN];

    if (inet_ntop(AF_INET6, dest.sin6_addr.s6_addr, str, INET6_ADDRSTRLEN) == NULL) {
        printf("inet_ntop\n");
    }

    DEBUG_MSG("Ping6 ip address : %s\n", str);
#endif

    pingsock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (pingsock < 0) {
        printf("Error socket\n");
        return -1;
    }

    timeout.tv_sec = 2000;
    timeout.tv_usec = 0;
    if (setsockopt(pingsock, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout, sizeof(timeout)) < 0) {
        printf("Set recv socket error\n");
        return -1;
    }

    timeout.tv_sec = 1000;
    timeout.tv_usec = 0;
    if (setsockopt(pingsock, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout, sizeof(timeout)) < 0) {
        printf("Set send socket error\n");
        return -1;
    }

    DEBUG_MSG("PING %s : %d data bytes\n", host, datasize);

    struct icmp6_hdr *icmphdr = (struct icmp6_hdr *) &icmp_data;
    fill_icmp6_data(icmp_data, datasize + sizeof(struct icmp6_hdr));
    icmphdr->icmp6_cksum = checksum((unsigned short *) icmp_data, datasize + sizeof(struct icmp6_hdr));

    if (sendto(pingsock, icmp_data, datasize + sizeof(struct icmphdr), 0, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
        printf("Sendto() timed out\n");
        return -1;
    }

    if (recvfrom(pingsock, recvbuf, MAX_PACKET_SIZE, 0, (struct sockaddr *) &from, &fromlen) < 0) {
        printf("Recvfrom() timed out\n");
        return -1;
    }

    close(pingsock);

    /* decode packet */
    icmphdr = (struct icmp6_hdr *) recvbuf;

    if ((icmphdr->icmp6_type != ICMP6_ECHO_REQUEST) && (icmphdr->icmp6_type != ICMP6_ECHO_REPLY)) {
        printf("Invalid type %d recvd\n", icmphdr->icmp6_type);
        return -1;
    }

    if (icmphdr->icmp6_id != ping_id) {
        printf("Wrong ID : %d   %d\n", icmphdr->icmp6_id, ping_id);
        return -1;
    }

    *ttl = 64;
    ftime(&end);

    DEBUG_MSG("Ping: time=%d ms\n", (end.time - start.time) * 1000 + (end.millitm - start.millitm));
    return ((end.time - start.time) * 1000 + (end.millitm - start.millitm));
}
