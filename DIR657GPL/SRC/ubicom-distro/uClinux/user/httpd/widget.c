#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <linux_vct.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/netdevice.h>
#include <sys/ioctl.h>

#include "sutil.h"
#include "shvar.h"
#include "vct.h"
#include "widget.h"
#include <httpd_util.h>
#include <pure.h>
#include <nvram.h>

//#define WIDGET_DEBUG 1
#ifdef WIDGET_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

int get_widget_info(char *url);
static int create_system_xml(void);
static int create_dhcpd_xml(void);
static int create_lan_xml(void);
static int create_qos_xml(void);
static int create_misc_xml(void);
static int create_wlan_xml(void);
static int create_wps_xml(void);
static int create_wan_xml(void);
int create_scheduler_xml(void);
int create_virtual_server_xml(void);
int create_port_forwarding_xml(void);
int create_application_xml(void);
int create_filter_xml(void);
int create_time_xml(void);
int create_ddns_xml(void);
int create_dmz_xml(void);
int create_log_xml(void);
int create_routing_table_xml(void);
static int create_static_route_xml(void);
static int create_rip_config_xml(void);

static void widget_add_to_buf(char* bufP, int* bufsizeP, int* buflenP, char* str, int len, int max_len);
static void widget_add_to_response(char* str, int len );
extern const char VER_FIRMWARE[];
extern const char VER_DATE[];
extern const char VER_BUILD[];
extern const char HW_VERSION[];
extern char login_salt[];
extern char user_passwd[];
extern char admin_passwd[];
/*
* 	Date: 2009-2-04
* 	Name: Ken_Chiang
* 	Reason: modify for firtst time don't used cache.
* 	Notice :
*/
#define QUERY_TIMES 6

pure_atrs_t widget_atrs;

#define SYSTEM_INFO		0
#define DHCP_INFO		1
#define LAN_INFO		2
#define QOS_INFO		3
#define MISC_INFO		4
#define WLAN_INFO		5
#define WPS_INFO		6
#define WAN_INFO		7
#define SCHEDULER_INFO  	8
#define VIRTUAL_SERVER_INFO 	9
#define PORT_FORWARDING_INFO    10
#define APPLICATION_INFO	11
#define FILTER_INFO		12
#define TIME_INFO		13
#define DDNS_INFO		14
#define DMZ_INFO		15
#define LOG_INFO		16
#define ROUTER_TABLE_INFO	17
#define STATIC_ROUTE		18
#define RIP_CONFIG		19
#define NOT_SUPPORT     	25
static int system_ready = 0;
static int loginerror = 0;

struct statistics {
	char  rx_packets[20];
	char  rx_errors[20];
	char  rx_dropped[20];
	char  rx_bytes[20];
	char  tx_packets[20];
	char  tx_errors[20];
	char  tx_dropped[20];
	char  tx_collisions[20];
	char  tx_bytes[20];
};

static void get_statistics(char *interface, struct statistics *st)
{
	#define FILE_SIZE 600
    FILE *fp = NULL;
    char *data[FILE_SIZE], *now_ptr, *p_pkts_s = NULL, *p_pkts_e = NULL;

    if ((strcmp(interface, WLAN0_ETH) == 0) && (nvram_match("wlan0_enable", "0") == 0))
        return;

    _system("ifconfig %s > /var/misc/pkts.txt", interface);
    fp = fopen("/var/misc/pkts.txt", "r");
    if (fp == NULL)
        return;

    fread(data, FILE_SIZE, 1, fp);
    fclose(fp);
    memset(st, 0, sizeof(struct statistics));

    /* RX packets */
    p_pkts_s = strstr(data, "RX packets:") + strlen("RX packets:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("RX pkts not found\n");
        return;
    }
    memcpy(st->rx_packets, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* RX errors */
    p_pkts_s = strstr(now_ptr, "errors:") + strlen("errors:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("RX errors not found\n");
        return;
    }
    memcpy(st->rx_errors, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* RX dropped */
    p_pkts_s = strstr(now_ptr, "dropped:") + strlen("dropped:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("RX dropped not found\n");
        return;
    }
    memcpy(st->rx_dropped, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* TX packets */
    p_pkts_s = strstr(data, "TX packets:") + strlen("TX packets:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("TX pkts not found\n");
        return;
    }
    memcpy(st->tx_packets, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* TX dropped */
    p_pkts_s = strstr(now_ptr, "dropped:") + strlen("dropped:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("TX dropped not found\n");
        return;
    }
    memcpy(st->tx_dropped, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* TX collisions */
    p_pkts_s = strstr(now_ptr, "collisions:") + strlen("collisions:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("TX collisions not found\n");
        return;
    }
    memcpy(st->tx_collisions, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* RX bytes */
    p_pkts_s = strstr(now_ptr, "RX bytes:") + strlen("RX bytes:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("RX bytes not found\n");
        return;
    }
    memcpy(st->rx_bytes, p_pkts_s, p_pkts_e - p_pkts_s);
    now_ptr = p_pkts_s;

    /* TX bytes */
    p_pkts_s = strstr(now_ptr, "TX bytes:") + strlen("TX bytes:");
    p_pkts_e = strstr(p_pkts_s, " ");
    if (p_pkts_s == NULL || p_pkts_e == NULL){
        printf("TX bytes not found\n");
        return;
    }
    memcpy(st->tx_bytes, p_pkts_s, p_pkts_e - p_pkts_s);
}

struct xml_cache_t xml_cache[10];

char *xmlpattenHeader =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>"
"<env:Envelope xmlns:env=\"http://www.w3.org/2003/05/soap-envelop/\" env:encodingStyle=\"http://www.w3.org/2003/05/soap-encoding/\">"
"<env:Body>";

char *xmlpattenTailer =
"</env:Body>"
"</env:Envelope>";

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

static char *time_format(void)
{
	static char buf[256]={};
	time_t widget_clock;
	struct tm *tm;

	time(&widget_clock);
	tm = localtime(&widget_clock) ;
	strftime(buf,sizeof(buf), "%Y, %m, %d, %k, %M, %S ", tm);
	return buf;
}

static char *wlan0_channel_list(char *channel_list)
{
	char *value = NULL;
	int domain=0x10;

#ifdef SET_GET_FROM_ARTBLOCK
	value = artblock_fast_get("wlan0_domain");
	if (value == NULL)
		value = artblock_get("wlan0_domain");
	if (value == NULL)
		value = nvram_get("wlan0_domain");
#else
	value = nvram_get("wlan0_domain");
#endif

	if (value != NULL)
		sscanf(value, "%x", &domain);
	else
		domain = 0x10;
/*
 	Date: 2009-3-5
 	Name: Ken_Chiang
 	Reason: modify for show chanel list used the space gap.
 	Notice :
*/
	GetDomainChannelList(domain, WIRELESS_BAND_24G, channel_list, 0);
	return channel_list;
}

//5G interface 1
static char *wlan1_channel_list(char *channel_list)
{
	char *value = NULL;
	int domain=0x37;

#ifdef SET_GET_FROM_ARTBLOCK
	value = artblock_fast_get("wlan0_domain");
	if (value == NULL)
		value = artblock_get("wlan0_domain");
	if (value == NULL)
		value = nvram_get("wlan0_domain");
#else
	value = nvram_get("wlan0_domain");
#endif

	if (value != NULL)
		sscanf(value, "%x", &domain);
	else
		domain = 0x37;
/*
 	Date: 2009-3-5
 	Name: Ken_Chiang
 	Reason: modify for show chanel list used the space gap.
 	Notice :
*/
	GetDomainChannelList(domain, WIRELESS_BAND_50G, channel_list, 0);
	return channel_list;
}

void widget_init(int conn_fd, char *protocol)
{
	widget_atrs.conn_fd = conn_fd;
	widget_atrs.protocol = protocol;
	return;
}

static void widget_add_to_response(char* str, int len)
{
	widget_add_to_buf(widget_atrs.response, &widget_atrs.response_size, &widget_atrs.response_len, str, len , MAX_RES_LEN_XML);
}

static void widget_add_to_buf( char* bufP, int* bufsizeP, int* buflenP, char* str, int len, int max_len)
{
	if (*bufsizeP == 0) {
		*bufsizeP = MAX_RES_LEN_XML;
		*buflenP = 0;
	}

	if (*buflenP + len >= *bufsizeP) {
		printf("widget_add_to_buf over size\n");
		return;
	}

	memcpy(&bufP[*buflenP], str, len);
	*buflenP += len;
	bufP[*buflenP] = '\0';
}

static void add_headers(int status, char* title, char* extra_header, char* mime_type, long byte, char *mod)
{
	char buf[200] = {};
	int buflen = 0;
	char timebuf[100];
	time_t now;

	widget_atrs.status = status;
	widget_atrs.bytes = byte;
	widget_atrs.response_size = 0;

	sprintf(buf, "%s %d %s\r\n", widget_atrs.protocol, widget_atrs.status, title);
	widget_add_to_response(buf, strlen(buf));

	sprintf(buf, "Server: Embedded HTTP Server ,Firmware Version %s\r\n", VER_FIRMWARE);
	widget_add_to_response(buf, strlen(buf));

	now = time((time_t*) 0);
	strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
	sprintf(buf, "Date: %s\r\n", timebuf);
	widget_add_to_response(buf, strlen(buf));

	if (extra_header != (char*) 0) {
		sprintf(buf, "%s\r\n", extra_header);
		widget_add_to_response(buf, strlen(buf));
	}

	if (mime_type != (char*) 0) {
		sprintf(buf, "Content-Type: %s\r\n", mime_type);
		widget_add_to_response(buf, strlen(buf));
	}

	if (widget_atrs.bytes >= 0) {
		sprintf(buf, "Content-Length: %ld\r\n", widget_atrs.bytes);
		buflen = strlen(buf);
		widget_add_to_response(buf, buflen);
	}

	if (mod) {
		strcpy(buf, "Last-Modified: \r\n");
		widget_add_to_response(buf, strlen(buf));
	}

	//sprintf(buf, "Connection: close\r\n\r\n");
	sprintf(buf, "cache-Control: max-age=1\r\n\r\n");
	widget_add_to_response(buf, strlen(buf));
}

void do_agentLogin_action(char *url, FILE *stream, int len, char *boundary)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	unsigned char input[64];
	unsigned int cmp;
	char login_hash[33];
	unsigned int digest[4];
	char *hash = strstr(url, "=") + 1 ;

	if (strncasecmp(login_salt, hash, 8)) {
		/* modify by ken for widget ipaddr error message
		   when widget run login fast continue sometime to check timeout is fail
		   for the issue we bypass the check now.
		*/
		if (loginerror) {
			add_headers(200, "OK", NULL, "text/xml", -1, NULL);
			xmlWrite(widget_xml, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
			xmlWrite(widget_xml + strlen(widget_xml), "<login>timeout</login>");
			widget_add_to_response(widget_xml, strlen(widget_xml));
		}
		else {
			goto pass_timeout;
		}
	}

	memset(input, 1, 64);
	memcpy(input, login_salt, 8);
	memcpy(input + 8, admin_passwd, strlen(admin_passwd));	// XXX
	md5_create_digest(input, 64, digest);
	sprintf(login_hash, "%08x%08x%08x%08x", digest[0], digest[1], digest[2], digest[3]);
	cmp = strncasecmp(login_hash, hash + 8, 32);

	if (cmp) {
		DEBUG_MSG("password error\n");
		/* modify by ken for widget password error message */
		add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		xmlWrite(widget_xml, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
		xmlWrite(widget_xml + strlen(widget_xml),"<login>error</login>");
		widget_add_to_response(widget_xml, strlen(widget_xml));
		loginerror = 1;
	}
	else {
pass_timeout:
		DEBUG_MSG("password ok\n");
		add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		xmlWrite(widget_xml, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
		xmlWrite(widget_xml+strlen(widget_xml), "<login>%s</login>", nvram_safe_get("default_html"));
		widget_add_to_response(widget_xml, strlen(widget_xml));
		loginerror = 0;
	}
}

void do_widget_action(char *url, FILE *stream, int len, char *boundary)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};

	DEBUG_MSG("do_widget_action\n");
	if (loginerror) {
		/* modify by ken for widget password error message */
		add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		xmlWrite(widget_xml, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
		xmlWrite(widget_xml + strlen(widget_xml), "<login>error</login>");
		widget_add_to_response(widget_xml, strlen(widget_xml));
	}

	switch(get_widget_info(url))
	{
	case SYSTEM_INFO:
		create_system_xml();
		break;
	case DHCP_INFO:
		create_dhcpd_xml();
		break;
	case LAN_INFO:
		create_lan_xml();
		break;
	case QOS_INFO:
		create_qos_xml();
		break;
	case MISC_INFO:
		create_misc_xml();
		break;
	case WLAN_INFO:
		create_wlan_xml();
		break;
	case WPS_INFO:
		create_wps_xml();
		break;
	case WAN_INFO:
		create_wan_xml();
		break;
	case SCHEDULER_INFO:
		create_scheduler_xml();
		break;
	case VIRTUAL_SERVER_INFO:
		create_virtual_server_xml();
		break;
	case PORT_FORWARDING_INFO:
		create_port_forwarding_xml();
		break;
	case APPLICATION_INFO:
		create_application_xml();
		break;
	case FILTER_INFO:
		create_filter_xml();
		break;
	case TIME_INFO:
		create_time_xml();
		break;
	case DDNS_INFO:
		create_ddns_xml();
		break;
	case DMZ_INFO:
		create_dmz_xml();
		break;
	case LOG_INFO:
		create_log_xml();
		break;
	case ROUTER_TABLE_INFO:
	  	create_routing_table_xml();
		break;
	case STATIC_ROUTE:
	  	create_static_route_xml();
		break;
	case RIP_CONFIG:
	  	create_rip_config_xml();
		break;
	}
}

void send_widget_response(char *url,FILE *stream)
{
/*
*  Name Albert Chen
*  Date 2009-02-17
*  Detail: set socket option using no buffer
*/
	int nSendBuf = 0;
	setsockopt(widget_atrs.conn_fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	send( widget_atrs.conn_fd, widget_atrs.response, strlen(widget_atrs.response), 0);
	memset(widget_atrs.response,0,MAX_RES_LEN_XML);
	widget_atrs.response_size = 0;

	return;
}

int get_widget_info(char *url)
{
	system_ready = 0;
	if (check_rc_status() != 'i')
		return 25; //NOT_SUPPORT	

	system_ready = 1;
/*
*  Name Albert Chen
*  Date 2009-02-17
*  Detail reduce strstr effort
*/
	char *xml_name = url + 24;
	if (strstr(xml_name, "system_info"))
		return SYSTEM_INFO;
	else if (strstr(xml_name, "dhcp_server"))
		return DHCP_INFO;
	else if (strstr(xml_name, "wlan_stats"))
		return  WLAN_INFO;
	else if (strstr(xml_name, "qos"))
		return QOS_INFO;
	else if (strstr(xml_name, "misc_config"))
		return MISC_INFO;
	else if (strstr(xml_name, "lan_stats"))
		return LAN_INFO;
	else if (strstr(xml_name, "wps"))
		return WPS_INFO;
	else if (strstr(xml_name, "wan_stats"))
		return WAN_INFO;
	else if (strstr(xml_name, "scheduler"))
		return SCHEDULER_INFO;
	else if (strstr(xml_name, "virtual_server"))
		return VIRTUAL_SERVER_INFO;
	else if (strstr(xml_name, "port_forwarding"))
		return PORT_FORWARDING_INFO;
	else if (strstr(xml_name, "application_rule"))
		return APPLICATION_INFO;
	else if (strstr(xml_name, "filter_config"))
		return FILTER_INFO;
	else if (strstr(xml_name, "time"))
		return TIME_INFO;
	else if (strstr(xml_name, "ddns"))
		return DDNS_INFO;
	else if (strstr(xml_name, "dmz"))
		return DMZ_INFO;
	else if (strstr(xml_name, "system_log"))
		return LOG_INFO;
	else if (strstr(xml_name, "routing_table"))
		return ROUTER_TABLE_INFO;
	else if (strstr(xml_name, "static_route"))
		return STATIC_ROUTE;
	else if (strstr(xml_name, "rip_config"))
		return RIP_CONFIG;
	else
		return NOT_SUPPORT;
}

char *mon[] =
{
	  "Jan",
	  "Feb",
	  "Mar",
	  "Apr",
	  "May",
	  "Jun",
	  "Jul",
	  "Aug",
	  "Sep",
	  "Oct",
	  "Nov",
	  "Dec",
};

static int create_system_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	char speed[5] = {0};
	char duplex[5]= {0};
	char status[15]= {0};
	char *fw_major="", *fw_minor="";
	int port;
	char ver_firm[5] = {0};
	char *tmp_day="0", *tmp_month="", *tmp_year="";
	char tmp_ver_date1[20]={0};
	int month = 0;
#ifdef SET_GET_FROM_ARTBLOCK
	char *art_lan_mac=artblock_fast_get("lan_mac");
	char *art_hw_version=artblock_fast_get("hw_version");
	char *art_wan_mac=artblock_fast_get("wan_mac");
#endif //#ifdef SET_GET_FROM_ARTBLOCK
	static int time_tmp = 0;

/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 2) {
		char *offset;
		if (offset=strstr(xml_cache[SYSTEM_INFO].data,"<system_time>")) {
			xmlWrite(offset, "<system_time>%s</system_time>", time_format());
  			xmlWrite(xml_cache[SYSTEM_INFO].data + strlen(xml_cache[SYSTEM_INFO].data), "</system_info>");
			xmlWrite(xml_cache[SYSTEM_INFO].data + strlen(xml_cache[SYSTEM_INFO].data), xmlpattenTailer);
			add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		}
		widget_add_to_response(xml_cache[SYSTEM_INFO].data, strlen(xml_cache[SYSTEM_INFO].data));
		return 0;
	}

	time_tmp = time_sec;
	memcpy(ver_firm, VER_FIRMWARE, 4);
	strcat(ver_firm, ".");
	getStrtok(ver_firm, ".", "%s %s", &fw_major, &fw_minor);

	memcpy(tmp_ver_date1, VER_DATE, 19);
	getStrtok(tmp_ver_date1, ",", "%s %s %s", &tmp_day, &tmp_month, &tmp_year);

	for (month = 12; month > 0; month--) {
		if (strstr(tmp_month, mon[month - 1])) {
			break;
		}
	}

	xmlWrite(widget_xml, xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<system_info>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<vendor_name>%s</vendor_name>", nvram_safe_get("manufacturer"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<product_name>%s</product_name>", nvram_safe_get("model_number"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<device_name>%s</device_name>", nvram_safe_get("pure_model_description"));
#ifdef SET_GET_FROM_ARTBLOCK
	if(art_hw_version!=NULL)
		xmlWrite(widget_xml+strlen(widget_xml), "<hardware_version>%s</hardware_version>", art_hw_version);
	else
		xmlWrite(widget_xml+strlen(widget_xml), "<hardware_version>%s</hardware_version>", HW_VERSION);
#else
  	xmlWrite(widget_xml+strlen(widget_xml), "<hardware_version>%s</hardware_version>", HW_VERSION);
#endif
  	xmlWrite(widget_xml+strlen(widget_xml), "<firmware_version_major>%s</firmware_version_major>", fw_major);
  	xmlWrite(widget_xml+strlen(widget_xml), "<firmware_version_minor>%s</firmware_version_minor>", fw_minor);
  	xmlWrite(widget_xml+strlen(widget_xml), "<firmware_version_build>%s</firmware_version_build>", VER_BUILD);
  	xmlWrite(widget_xml+strlen(widget_xml), "<firmware_version_date>%s/%02d/%s</firmware_version_date>", tmp_year, month, tmp_day);
  	xmlWrite(widget_xml+strlen(widget_xml), "<firmware_url>%s</firmware_url>", nvram_safe_get("model_url"));
  	{
  		/*
		 * Reason: url value remove ","
		 * Modified: Yufa Huang
		 * Date: 2010.05.03
		 */
  		char url[256], *url1=NULL;
  		strcpy(url, nvram_safe_get("check_fw_url"));
  		if (strlen(url)) {
  			url1 = strstr(url, ",");
  			if (url1) {
  				*url1 = 0;
  				url1++;
  			}
  		}

 	 	xmlWrite(widget_xml+strlen(widget_xml),"<firmware_update_info_url>http://%s%s</firmware_update_info_url>", url, url1);
	}
  	xmlWrite(widget_xml+strlen(widget_xml), "<firmware_notify_enable>0</firmware_notify_enable>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<firmware_notify_interval>0</firmware_notify_interval>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<multi_language>US</multi_language>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<multi_language_selection>US</multi_language_selection>");
#ifdef SET_GET_FROM_ARTBLOCK
	if (art_lan_mac != NULL)
		xmlWrite(widget_xml+strlen(widget_xml), "<lan_mac>%s</lan_mac>", art_lan_mac);
	else if (artblock_get("lan_mac")!= NULL)
		xmlWrite(widget_xml+strlen(widget_xml), "<lan_mac>%s</lan_mac>", artblock_safe_get("lan_mac"));
	else
		xmlWrite(widget_xml+strlen(widget_xml), "<lan_mac>%s</lan_mac>", nvram_safe_get("lan_mac"));

  	if (art_wan_mac != NULL)
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_mac>%s</wan_mac>", art_wan_mac);
  	else if (artblock_get("wan_mac") != NULL) 
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_mac>%s</wan_mac>", artblock_safe_get("wan_mac"));
  	else
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_mac>%s</wan_mac>", nvram_safe_get("wan_mac"));

	if (art_wan_mac != NULL && art_lan_mac != NULL) {
		if (strncasecmp(art_lan_mac, art_wan_mac, 10)) {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>1</wan_clone_mac_enable>");
  			xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>%s</wan_cloned_mac>", art_wan_mac);
		}
		else {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>0</wan_clone_mac_enable>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>00:00:00:00:00:00</wan_cloned_mac>");
		}
	}
	else {
		if (artblock_get("lan_mac") != NULL && artblock_get("wan_mac") != NULL) {
			if (strncasecmp(artblock_safe_get("lan_mac"), artblock_safe_get("wan_mac"), 10)) {
				xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>1</wan_clone_mac_enable>");
  				xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>%s</wan_cloned_mac>", artblock_safe_get("wan_mac"));
			}
			else {
				xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>0</wan_clone_mac_enable>");
				xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>00:00:00:00:00:00</wan_cloned_mac>");
			}
		}
		else {
			if (strncasecmp(nvram_safe_get("lan_mac"), nvram_safe_get("wan_mac"), 10)) {
				xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>1</wan_clone_mac_enable>");
	  			xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>%s</wan_cloned_mac>", nvram_safe_get("wan_mac"));
			}
			else {
				xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>0</wan_clone_mac_enable>");
				xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>00:00:00:00:00:00</wan_cloned_mac>");
	  		}
	  	}
	}
#else	//#ifdef SET_GET_FROM_ARTBLOCK
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_mac>%s</lan_mac>", nvram_safe_get("lan_mac"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<wan_mac>%s</wan_mac>", nvram_safe_get("wan_mac"));
	if (strncasecmp(nvram_safe_get("lan_mac"), nvram_safe_get("wan_mac"), 10)) {
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>1</wan_clone_mac_enable>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>%s</wan_cloned_mac>", nvram_safe_get("wan_mac"));
	}
	else {
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_clone_mac_enable>0</wan_clone_mac_enable>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_cloned_mac>00:00:00:00:00:00</wan_cloned_mac>");
	}
#endif//#ifdef SET_GET_FROM_ARTBLOCK

	xmlWrite(widget_xml+strlen(widget_xml), "<wan_config_port_speed>%s</wan_config_port_speed>", nvram_safe_get("wan_port_speed"));

  	for (port = 0; port < 4; port++) {
  		int port_tmp;
  		port_tmp = 3 - port;
		xmlWrite(widget_xml+strlen(widget_xml), "<switch_port_%d_status>", port_tmp);
  		xmlWrite(widget_xml+strlen(widget_xml), "<name>LAN%d</name>", port_tmp);
  		VCTGetPortConnectState(nvram_safe_get("lan_eth"), port+2, status);
  		if (strcmp(status, "disconnect") == 0)
  			xmlWrite(widget_xml+strlen(widget_xml), "<status>Disconnected</status>");
  		else {
  			VCTGetPortSpeedState(nvram_safe_get("wan_eth"), port+2, speed, duplex);
  			if (strcmp(speed, "giga") == 0)
  				duplex[0] = 0;
  			xmlWrite(widget_xml+strlen(widget_xml), "<status>%s%s</status>", speed, duplex);
  		}
  		xmlWrite(widget_xml+strlen(widget_xml), "</switch_port_%d_status>", port_tmp);
  		memset(speed, 0, 5);
  		memset(duplex, 0 , 5);
  		memset(status,0, 15);
	}

	xmlWrite(widget_xml+strlen(widget_xml), "<switch_port_4_status>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<name>WAN0</name>");
  	VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, status);
  	if (strcmp(status, "disconnect") == 0)
  		xmlWrite(widget_xml+strlen(widget_xml),"<status>Disconnected</status>");
  	else {
  		VCTGetPortSpeedState(nvram_safe_get("wan_eth"), VCTWANPORT0, speed, duplex);
  		if (strcmp(speed, "giga") == 0)
  			duplex[0] = 0;
  		xmlWrite(widget_xml+strlen(widget_xml), "<status>%s%s</status>", speed, duplex);
  	}
  	xmlWrite(widget_xml+strlen(widget_xml), "</switch_port_4_status>");

  	xmlWrite(widget_xml+strlen(widget_xml), "<system_time>%s</system_time>", time_format());
  	xmlWrite(widget_xml+strlen(widget_xml), "</system_info>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[SYSTEM_INFO].data, 0 , sizeof(widget_xml));
	memcpy(xml_cache[SYSTEM_INFO].data, widget_xml, sizeof(widget_xml));
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_dhcpd_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	int i;
	char buf[32];
	char *p_dhcp_reserve;
	char fw_value[192]={0};
	char *enable="", *name="", *ip="", *mac="";
	char *wanip="", *netmask="", *gateway="", *primary_dns="", *secondary_dns="";
	char wan_ip[192]={0};
	T_DHCPD_LIST_INFO* dhcpd_status = NULL;
	static int time_tmp = 0;

/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 2) {
		widget_add_to_response(xml_cache[DHCP_INFO].data, strlen(xml_cache[DHCP_INFO].data));
		return 0;
	}

	time_tmp = time_sec;
	dhcpd_status =  get_dhcp_client_list();
  	if (dhcpd_status == NULL) {
		xmlWrite(widget_xml, xmlpattenHeader);
		xmlWrite(widget_xml+strlen(widget_xml), "<dhcp_server>");
		xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_enable>%s</dhcpd_enable>", nvram_safe_get("dhcpd_enable"));
	  	xmlWrite(widget_xml+strlen(widget_xml), "</dhcp_server>");
		xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
		add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		widget_add_to_response(widget_xml, strlen(widget_xml));
		return 0;
  	}

	memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
	getStrtok(wan_ip, "/", "%s %s %s %s %s", &wanip, &netmask, &gateway, &primary_dns, &secondary_dns);
	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<dhcp_server>");
	xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_enable>%s</dhcpd_enable>", nvram_safe_get("dhcpd_enable"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_start_ip>%s</dhcpd_start_ip>", nvram_safe_get("dhcpd_start"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_end_ip>%s</dhcpd_end_ip>", nvram_safe_get("dhcpd_end"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_lease_time>%s</dhcpd_lease_time>", nvram_safe_get("dhcpd_lease_time"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_domain_name>%s</dhcpd_domain_name>", nvram_safe_get("dhcpd_domain_name"));
	if (!strcmp(nvram_safe_get("wan_primary_dns"), "0.0.0.0") || !strcmp(nvram_safe_get("wan_primary_dns"), "")) {
  		xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_primary_dns>%s</dhcpd_primary_dns>", primary_dns);
  		xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_secondary_dns>%s</dhcpd_secondary_dns>", secondary_dns);
	}
	else {
  		xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_primary_dns>%s</dhcpd_primary_dns>", nvram_safe_get("wan_primary_dns"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_secondary_dns>%s</dhcpd_secondary_dns>", nvram_safe_get("wan_secondary_dns"));
	}
  	xmlWrite(widget_xml+strlen(widget_xml),"<dhcpd_reservation>%s</dhcpd_reservation>", nvram_safe_get("dhcpd_reservation"));

  	for (i=0; i < 25; i++) {
		sprintf(buf, "dhcpd_reserve_%02d", i);
		p_dhcp_reserve = nvram_safe_get(buf);
		if (p_dhcp_reserve == NULL || (strlen(p_dhcp_reserve) == 0))
		   continue;

		memcpy(fw_value, p_dhcp_reserve, sizeof(fw_value));
		getStrtok(fw_value, "/", "%s %s %s %s ", &enable, &name, &ip, &mac);
		xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_reserve_entry_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", enable);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", name);
		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<mac>%s</mac>", mac);
		xmlWrite(widget_xml+strlen(widget_xml), "</dhcpd_reserve_entry_%d>", i);
	}

  	for (i = 0; i < dhcpd_status->num_list; i++) {
		xmlWrite(widget_xml+strlen(widget_xml), "<dhcpd_leased_entry_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", dhcpd_status->p_st_dhcpd_list[i].hostname);
		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", dhcpd_status->p_st_dhcpd_list[i].ip_addr);
		xmlWrite(widget_xml+strlen(widget_xml), "<mac>%s</mac>", dhcpd_status->p_st_dhcpd_list[i].mac_addr);
	  	xmlWrite(widget_xml+strlen(widget_xml), "</dhcpd_leased_entry_%d>", i);
	}

  	xmlWrite(widget_xml+strlen(widget_xml), "</dhcp_server>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[DHCP_INFO].data, 0 , sizeof(widget_xml));
	memcpy(xml_cache[DHCP_INFO].data, widget_xml, sizeof(widget_xml));
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_lan_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	struct statistics  lan_st;
	get_statistics(nvram_safe_get("lan_eth"), &lan_st);
	static int time_tmp = 0;

/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 2) {
		char *offset;
		if (offset=strstr(xml_cache[LAN_INFO].data,"<lan_tx_bytes>")) {
			xmlWrite(offset, "<lan_tx_bytes>%s</lan_tx_bytes>", lan_st.tx_bytes);
  			xmlWrite(xml_cache[LAN_INFO].data+strlen(xml_cache[LAN_INFO].data), "<lan_rx_bytes>%s</lan_rx_bytes>", lan_st.rx_bytes);
  			xmlWrite(xml_cache[LAN_INFO].data+strlen(xml_cache[LAN_INFO].data), "</lan_stats>");
			xmlWrite(xml_cache[LAN_INFO].data+strlen(xml_cache[LAN_INFO].data), xmlpattenTailer);
			add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		}
		widget_add_to_response(xml_cache[LAN_INFO].data, strlen(xml_cache[LAN_INFO].data));
		return 0;
	}

	time_tmp = time_sec;
	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<lan_stats>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_ip>%s</lan_ip>", nvram_safe_get("lan_ipaddr"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_netmask>%s</lan_netmask>", nvram_safe_get("lan_netmask"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_default_gateway>%s</lan_default_gateway>", nvram_safe_get("lan_ipaddr"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<dns_relay_enable>%s</dns_relay_enable>", nvram_safe_get("dns_relay"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_tx_packets>%s</lan_tx_packets>", lan_st.tx_packets);
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_rx_packets>%s</lan_rx_packets>", lan_st.rx_packets);
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_lost_packets>%s</lan_lost_packets>", lan_st.tx_dropped);
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_collision_packets>%s</lan_collision_packets>", lan_st.tx_collisions);
  	xmlWrite(widget_xml+strlen(widget_xml), "<timestamp>%d</timestamp>", uptime());
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_tx_bytes>%s</lan_tx_bytes>", lan_st.tx_bytes);
  	xmlWrite(widget_xml+strlen(widget_xml), "<lan_rx_bytes>%s</lan_rx_bytes>", lan_st.rx_bytes);
  	xmlWrite(widget_xml+strlen(widget_xml), "</lan_stats>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[LAN_INFO].data, 0 , sizeof(widget_xml));
	memcpy(xml_cache[LAN_INFO].data, widget_xml, sizeof(widget_xml));
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_qos_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	char buf_qos[20] = {0};
	int i = 0;
	char qos_value[192]={0};
	char *enable, *name="", *priority="", *protocol_type="", *local_start_ip="", *local_stop_ip="", *local_start_port="", *local_stop_port="";
	char *remote_start_ip="", *remote_stop_ip="", *remote_start_port="", *remote_stop_port="", *var1="", *var2="";
	char local_ip_buf[200] = {0};
	char local_port_buf[200] = {0};
	char remote_ip_buf[200] = {0};
	char remote_port_buf[200] = {0};
	char buf_wlan_qos[20] = {0};
	char wlan_qos_value[192]={0};
	static int time_tmp = 0;

/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 2) {
		widget_add_to_response(xml_cache[QOS_INFO].data, strlen(xml_cache[QOS_INFO].data));
		return 0;
	}

	time_tmp = time_sec;
	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<qos>");

	if (nvram_match("qos_enable", "1")== 0 && nvram_match("traffic_shaping", "1") == 0)
		xmlWrite(widget_xml+strlen(widget_xml), "<qos_enable>1</qos_enable>");
	else
		xmlWrite(widget_xml+strlen(widget_xml), "<qos_enable>0</qos_enable>");

  	xmlWrite(widget_xml+strlen(widget_xml), "<qos_traffic_shapping_enable>%s</qos_traffic_shapping_enable>", nvram_safe_get("traffic_shaping"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<qos_auto_classify_enable>%s</qos_auto_classify_enable>", nvram_safe_get("auto_classification"));

	memset(buf_qos, 0, sizeof(buf_qos));
	memset(qos_value, 0, sizeof(qos_value));
	memset(local_ip_buf, 0, sizeof(local_ip_buf));
	memset(local_port_buf, 0, sizeof(local_port_buf));
	memset(remote_ip_buf, 0, sizeof(remote_ip_buf));
	memset(remote_port_buf, 0, sizeof(remote_port_buf));

    //qos_rule_00=1/aaaa/1/0/0/6/192.168.0.23/255.255.255.255/0/65535/192.168.0.33/255.255.255.255/10/65535
	for (i = 0; i< 9; i++) {
		sprintf(buf_qos, "qos_rule_%02d", i);
		if (!strcmp(nvram_safe_get(buf_qos), "0//1/0/0/6/0.0.0.0/255.255.255.255/0/65535/0.0.0.0/255.255.255.255/0/65535"))
		   continue;

		strncpy(qos_value, nvram_safe_get(buf_qos), sizeof(qos_value));
		getStrtok(qos_value, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s %s", &enable, &name, &priority, &var1, &var2, &protocol_type, &local_start_ip
			, &local_stop_ip, &local_start_port, &local_stop_port, &remote_start_ip, &remote_stop_ip, &remote_start_port, &remote_stop_port);

		if (enable == NULL)
			continue;

		xmlWrite(widget_xml+strlen(widget_xml), "<qos_rule_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", enable);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", name);
		xmlWrite(widget_xml+strlen(widget_xml), "<priority>%s</priority>", priority);

		if (strcmp(protocol_type, "256") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>Any</protocol_type>");
		else if (strcmp(protocol_type, "6") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>TCP</protocol_type>");
		else if (strcmp(protocol_type, "17") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>UDP</protocol_type>");
		else if (strcmp(protocol_type, "257") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>Both</protocol_type>");
		else if (strcmp(protocol_type, "1") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>ICMP</protocol_type>");
		else
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>Other</protocol_type>");
		
		sprintf(local_ip_buf, "%s-%s", local_start_ip, local_stop_ip);
		sprintf(local_port_buf, "%s-%s", local_start_port, local_stop_port);
		sprintf(remote_ip_buf, "%s-%s", remote_start_ip, remote_stop_ip);
		sprintf(remote_port_buf, "%s-%s", remote_start_port, remote_stop_port);

		xmlWrite(widget_xml+strlen(widget_xml),"<local_ip_range>%s</local_ip_range>", local_ip_buf);
		xmlWrite(widget_xml+strlen(widget_xml),"<local_port_range>%s</local_port_range>", local_port_buf);
		xmlWrite(widget_xml+strlen(widget_xml),"<remote_ip_range>%s</remote_ip_range>", remote_ip_buf);
		xmlWrite(widget_xml+strlen(widget_xml),"<remote_port_range>%s</remote_port_range>", remote_port_buf);
		xmlWrite(widget_xml+strlen(widget_xml),"</qos_rule_%d>", i);
	}

  	xmlWrite(widget_xml+strlen(widget_xml), "<wmm_enable>%s</wmm_enable>", nvram_safe_get("wlan0_wmm_enable"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_qos_auto_classify_enable>0</wlan_qos_auto_classify_enable>");

	memset(local_ip_buf, 0, sizeof(local_ip_buf));
	memset(local_port_buf, 0, sizeof(local_port_buf));
	memset(remote_ip_buf, 0, sizeof(remote_ip_buf));
	memset(remote_port_buf, 0, sizeof(remote_port_buf));
	memset(buf_wlan_qos, 0, sizeof(buf_wlan_qos));
	memset(wlan_qos_value, 0, sizeof(wlan_qos_value));

    //wish_rule_00=1/ccccc/4/0/0/6/192.168.0.66/255.255.255.255/50/65535/192.168.0.77/255.255.255.255/55/65535
	for (i = 0; i< 24; i++) {
		sprintf(buf_wlan_qos, "wish_rule_%02d", i);
		if (!strcmp(nvram_safe_get(buf_wlan_qos), "0//1/0/0/6/0.0.0.0/255.255.255.255/0/65535/0.0.0.0/255.255.255.255/0/65535"))
		   continue;

		strncpy(wlan_qos_value, nvram_safe_get(buf_wlan_qos), sizeof(wlan_qos_value));
		getStrtok(wlan_qos_value, "/", "%s %s %s %s %s %s %s %s %s %s %s %s %s %s", &enable, &name, &priority, &var1, &var2, &protocol_type, &local_start_ip
			, &local_stop_ip, &local_start_port, &local_stop_port, &remote_start_ip, &remote_stop_ip, &remote_start_port, &remote_stop_port);

		if (enable == NULL)
			continue;

		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_qos_rule_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", enable);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", name);
		xmlWrite(widget_xml+strlen(widget_xml), "<priority>%s</priority>", priority);

		if (strcmp(protocol_type, "256") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>Any</protocol_type>");
		else if (strcmp(protocol_type, "6") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>TCP</protocol_type>");
		else if (strcmp(protocol_type, "17") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>UDP</protocol_type>");
		else if (strcmp(protocol_type, "257") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>Both</protocol_type>");
		else if (strcmp(protocol_type, "1") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>ICMP</protocol_type>");
		else
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>Other</protocol_type>");

		sprintf(local_ip_buf, "%s-%s", local_start_ip, local_stop_ip);
		sprintf(local_port_buf, "%s-%s", local_start_port, local_stop_port);
		sprintf(remote_ip_buf, "%s-%s", remote_start_ip, remote_stop_ip);
		sprintf(remote_port_buf, "%s-%s", remote_start_port, remote_stop_port);

		xmlWrite(widget_xml+strlen(widget_xml), "<local_ip_range>%s</local_ip_range>", local_ip_buf);
		xmlWrite(widget_xml+strlen(widget_xml), "<local_port_range>%s</local_port_range>", local_port_buf);
		xmlWrite(widget_xml+strlen(widget_xml), "<remote_ip_range>%s</remote_ip_range>", remote_ip_buf);
		xmlWrite(widget_xml+strlen(widget_xml), "<remote_port_range>%s</remote_port_range>", remote_port_buf);
		xmlWrite(widget_xml+strlen(widget_xml), "</wlan_qos_rule_%d>",i);
	}

  	xmlWrite(widget_xml+strlen(widget_xml), "</qos>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[QOS_INFO].data, 0 , sizeof(widget_xml));
	memcpy(xml_cache[QOS_INFO].data, widget_xml, sizeof(widget_xml));
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_misc_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	static int time_tmp = 0;
/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 2) {
		widget_add_to_response(xml_cache[MISC_INFO].data, strlen(xml_cache[MISC_INFO].data));
		return 0;
	}

	time_tmp = time_sec;
	xmlWrite(widget_xml, xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<misc_config>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<wan_port_ping_response_enable>%s</wan_port_ping_response_enable>", nvram_safe_get("wan_port_ping_response_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<remote_http_management>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", nvram_safe_get("remote_http_management_enable"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<port>%s</port>", nvram_safe_get("remote_http_management_port"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<ip_range>%s</ip_range>", strlen(nvram_safe_get("remote_http_management_ipaddr_range"))==0? "*": nvram_safe_get("remote_http_management_ipaddr_range"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<schedule_name>%s</schedule_name>", nvram_safe_get("remote_http_management_schedule_name"));
  	xmlWrite(widget_xml+strlen(widget_xml), "</remote_http_management>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<multicast_stream_enable>%s</multicast_stream_enable>", nvram_safe_get("multicast_stream_enable"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<pptp_pass_through>%s</pptp_pass_through>", nvram_safe_get("pptp_pass_through"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<ipsec_pass_through>%s</ipsec_pass_through>", nvram_safe_get("ipsec_pass_through"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<l2tp_pass_through>%s</l2tp_pass_through>", nvram_safe_get("l2tp_pass_through"));
	xmlWrite(widget_xml+strlen(widget_xml), "<upnp>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", nvram_safe_get("upnp_enable"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<friendly_name>%s</friendly_name>", nvram_safe_get("pure_model_description"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<manufacturer>%s</manufacturer>", nvram_safe_get("pure_vendor_name"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<manufacturer_url>%s</manufacturer_url>", nvram_safe_get("manufacturer_url"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<model_name>%s</model_name>", nvram_safe_get("model_number"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<model_url>%s</model_url>", nvram_safe_get("model_url"));
  	xmlWrite(widget_xml+strlen(widget_xml), "</upnp>");
  	xmlWrite(widget_xml+strlen(widget_xml), "</misc_config>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[MISC_INFO].data, 0 , sizeof(widget_xml));
	memcpy(xml_cache[MISC_INFO].data, widget_xml, sizeof(widget_xml));
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

/*
* Name Albert Chen
* Date 2009-02-19
* Detail modify cache behavior for dual band Widget access
*/
#define WLAN1_INFO 9
static int create_wlan_xml(void)
{
	char replace_str[256]={0};	
	char widget_xml[MAX_RES_LEN_XML] = {0};
	char fw_value[192]={0};
	char *eap="";
	char *host="", *port="", *secret="";
	char channel_list[512]="";
	T_WLAN_STAION_INFO* wireless_status = NULL;
	T_DHCPD_LIST_INFO* dhcpd_status = NULL;
	int i = 0;
	int j = 0;
	static int time_tmp = 0;

	/* 2010/05/10, Yufa, bad solution for widget bug (request many wlan_stats in one seconds */
	static struct statistics  wlan_st;
#ifdef USER_WL_ATH_5GHZ
	static struct statistics  wlan1_st;
#endif

/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 3) {
		char *offset;
		if (offset = strstr(xml_cache[WLAN_INFO].data, "<wlan_tx_bytes>")) {
			xmlWrite(offset,"<wlan_tx_bytes>%s</wlan_tx_bytes>", wlan_st.tx_bytes);
  			xmlWrite(xml_cache[WLAN_INFO].data+strlen(xml_cache[WLAN_INFO].data), "<wlan_rx_bytes>%s</wlan_rx_bytes>", wlan_st.rx_bytes);
  			xmlWrite(xml_cache[WLAN_INFO].data+strlen(xml_cache[WLAN_INFO].data), "</wlan_interface_0>");

#ifdef USER_WL_ATH_5GHZ
			if (offset = strstr(xml_cache[WLAN1_INFO].data, "<wlan_tx_bytes>")) {
				xmlWrite(offset,"<wlan_tx_bytes>%s</wlan_tx_bytes>", wlan1_st.tx_bytes);
  				xmlWrite(xml_cache[WLAN1_INFO].data+strlen(xml_cache[WLAN1_INFO].data), "<wlan_rx_bytes>%s</wlan_rx_bytes>", wlan1_st.rx_bytes);
  				xmlWrite(xml_cache[WLAN1_INFO].data+strlen(xml_cache[WLAN1_INFO].data), "</wlan_interface_0>");
			}
		
			strcat(xml_cache[WLAN_INFO].data, xml_cache[WLAN1_INFO].data);
#endif  			
  			xmlWrite(xml_cache[WLAN_INFO].data+strlen(xml_cache[WLAN_INFO].data), "</wlan_stats>");
			xmlWrite(xml_cache[WLAN_INFO].data+strlen(xml_cache[WLAN_INFO].data), xmlpattenTailer);
			add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		}

		widget_add_to_response(xml_cache[WLAN_INFO].data, strlen(xml_cache[WLAN_INFO].data));
		return 0;
	}

	time_tmp = time_sec;

	xmlWrite(widget_xml, xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_stats>");

	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_interface_0>");
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_enable>%s</wlan_enable>", nvram_safe_get("wlan0_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_name>WLAN0</wlan_name>");
	StrEncode(replace_str, nvram_safe_get("wlan0_ssid"), 0);
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_ssid>%s</wlan_ssid>", replace_str);
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_dot11d_enable>%s</wlan_dot11d_enable>", nvram_safe_get("wlan0_11d_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_channel_list>%s</wlan_channel_list>", wlan0_channel_list(channel_list));

	/* wlan info */
	/*
	 * Reason: Sometime can not get channel info
	 * Modified: Yufa Huang
	 * Date: 2010.05.04
	 */
	char channel[32];
	memset(channel, 0, 32);
	if (nvram_match("wlan0_enable", "1") == 0) {
		get_channel(channel);
		if (strcmp(channel, "unknown") == 0) {
       		if (check_rc_status() == 'b')
				strcpy(channel, nvram_safe_get("wlan0_channel"));
		}
	}
   	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_channel>%s</wlan_channel>", channel);
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_auto_channel_enable>%s</wlan_auto_channel_enable>", nvram_safe_get("wlan0_auto_channel_enable"));

	if (strcmp(nvram_safe_get("wlan0_dot11_mode"), "11gn") == 0)
	    xmlWrite(widget_xml+strlen(widget_xml), "<wlan_dot11_mode>11ng</wlan_dot11_mode>");
	else
	    xmlWrite(widget_xml+strlen(widget_xml), "<wlan_dot11_mode>%s</wlan_dot11_mode>", nvram_safe_get("wlan0_dot11_mode"));

	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_ssid_broadcast>%s</wlan_ssid_broadcast>", nvram_safe_get("wlan0_ssid_broadcast"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_mode>ap</wlan_mode>");
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_idle_time>5</wlan_idle_time>");

	if (strstr(nvram_safe_get("wlan0_security"), "disable")) {
    	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan0_security"));
	}
	else if (strstr(nvram_safe_get("wlan0_security"), "64")) {
		if (strstr(nvram_safe_get("wlan0_security"), "open"))
	    	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan0_security"));
	    else
		  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>wep_shared_64</wlan_security>");

  		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wep64_key>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<key0>%s</key0>", nvram_safe_get("wlan0_wep64_key_1"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<key1>%s</key1>", nvram_safe_get("wlan0_wep64_key_2"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<key2>%s</key2>", nvram_safe_get("wlan0_wep64_key_3"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<key3>%s</key3>", nvram_safe_get("wlan0_wep64_key_4"));
  		xmlWrite(widget_xml+strlen(widget_xml), "</wlan_wep64_key>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wep_display>%s</wlan_wep_display>", nvram_safe_get("wlan0_wep_display"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wep_default_key>%s</wlan_wep_default_key>", nvram_safe_get("wlan0_wep_default_key"));
	}
	else if (strstr(nvram_safe_get("wlan0_security"), "128")) {
		if (strstr(nvram_safe_get("wlan0_security"), "open"))
	    	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan0_security"));
	   	else
		  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>wep_shared_128</wlan_security>");

  		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wep128_key>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<key0>%s</key0>", nvram_safe_get("wlan0_wep128_key_1"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<key1>%s</key1>", nvram_safe_get("wlan0_wep128_key_2"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<key2>%s</key2>", nvram_safe_get("wlan0_wep128_key_3"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<key3>%s</key3>", nvram_safe_get("wlan0_wep128_key_4"));
  		xmlWrite(widget_xml+strlen(widget_xml), "</wlan_wep128_key>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wep_display>%s</wlan_wep_display>", nvram_safe_get("wlan0_wep_display"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wep_default_key>%s</wlan_wep_default_key>", nvram_safe_get("wlan0_wep_default_key"));
	}
	else if (strstr(nvram_safe_get("wlan0_security"), "psk")) {
		if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_psk") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>wpa_auto_psk</wlan_security>");
   	    else
   	        xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan0_security"));

		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_psk_cipher_type>%s</wlan_psk_cipher_type>", nvram_safe_get("wlan0_psk_cipher_type"));
		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_psk_pass_phrase>%s</wlan_psk_pass_phrase>", nvram_safe_get("wlan0_psk_pass_phrase"));
		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wpa_groupkey_rekey_time>%s</wlan_wpa_groupkey_rekey_time>", nvram_safe_get("wlan0_gkey_rekey_time"));
	}
	else if (strstr(nvram_safe_get("wlan0_security"), "eap")){
	    if (strcmp(nvram_safe_get("wlan0_security"), "wpa2auto_eap") == 0)
  	       	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>wpa_auto_eap</wlan_security>");
  	    else
  	       	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan0_security"));

		eap = nvram_safe_get("wlan0_eap_radius_server_0");
		memcpy(fw_value, eap, sizeof(fw_value));
		getStrtok(fw_value, "/", "%s %s %s", &host, &port, &secret);
		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_eap_radius_server>");
		xmlWrite(widget_xml+strlen(widget_xml), "<host>%s</host>", host);
		xmlWrite(widget_xml+strlen(widget_xml), "<port>%s</port>", port);
		xmlWrite(widget_xml+strlen(widget_xml), "<secret>%s</secret>", secret);
		xmlWrite(widget_xml+strlen(widget_xml), "</wlan_eap_radius_server>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wpa_groupkey_rekey_time>%s</wlan_wpa_groupkey_rekey_time>", nvram_safe_get("wlan0_gkey_rekey_time"));
	}

	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_auto_txrate>auto</wlan_auto_txrate>");
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_txrate>Auto</wlan_txrate>");
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_txpower>full</wlan_txpower>");
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_beacon_interval>%s</wlan_beacon_interval>", nvram_safe_get("wlan0_beacon_interval"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_rts_threshold>%s</wlan_rts_threshold>", nvram_safe_get("wlan0_rts_threshold"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_fragmentation_threshold>%s</wlan_fragmentation_threshold>", nvram_safe_get("wlan0_fragmentation"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_dtim>%s</wlan_dtim>", nvram_safe_get("wlan0_dtim"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_short_preamble>1</wlan_short_preamble>");
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_cts>1</wlan_cts>");
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_wmm_enable>%s</wlan_wmm_enable>", nvram_safe_get("wlan0_wmm_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_protection>%s</wlan_protection>", nvram_safe_get("wlan0_partition"));

#ifdef ATHEROS_11N_DRIVER
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_11n_protection>%s</wlan_11n_protection>", nvram_safe_get("wlan0_11n_protection"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_amsdu>4K</wlan_amsdu>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_channel_bandwidth>%s</wlan_channel_bandwidth>", nvram_safe_get("wlan0_11n_protection"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_guard_interval>short</wlan_guard_interval>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_ext_subchannel>auto</wlan_ext_subchannel>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_antenna_control>2</wlan_antenna_control>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_partition_enable>%s</wlan_partition_enable>", nvram_safe_get("wlan0_partition"));
#endif

	/* 2010/05/10, Yufa, bad solution for widget bug (request many wlan_stats in one seconds */
	get_statistics(WLAN0_ETH, &wlan_st);

  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_tx_packets>%s</wlan_tx_packets>", wlan_st.tx_packets);
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_rx_packets>%s</wlan_rx_packets>", wlan_st.rx_packets);
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_lost_packets>%s</wlan_lost_packets>", wlan_st.tx_dropped);
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_error_packets>%s</wlan_error_packets>", wlan_st.tx_errors);
  	xmlWrite(widget_xml+strlen(widget_xml), "<timestamp>%d</timestamp>", uptime());
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_tx_bytes>%s</wlan_tx_bytes>", wlan_st.tx_bytes);
  	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_rx_bytes>%s</wlan_rx_bytes>", wlan_st.rx_bytes);

  	//wlan0_associated_station
	xmlWrite(widget_xml+strlen(widget_xml), "<wlan_associated_station>");
	wireless_status = get_stalist();
	dhcpd_status = get_dhcp_client_list();
	if (wireless_status != NULL) {
		for(i = 0 ; i < wireless_status->num_sta; i++) {
			if (dhcpd_status != NULL) {
	  			for (j = 0; j < dhcpd_status->num_list; j++) {
	  				if (strcmp(dhcpd_status->p_st_dhcpd_list[j].mac_addr, wireless_status->p_st_wlan_sta[i].mac_addr) == 0) {
						xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", dhcpd_status->p_st_dhcpd_list[j].hostname ? dhcpd_status->p_st_dhcpd_list[j].hostname : "");
	  				}
	  			}
			}
  			xmlWrite(widget_xml+strlen(widget_xml), "<mac>%s</mac>", wireless_status->p_st_wlan_sta[i].mac_addr ? wireless_status->p_st_wlan_sta[i].mac_addr : "");
  			xmlWrite(widget_xml+strlen(widget_xml), "<type>%s</type>", wireless_status->p_st_wlan_sta[i].mode ? wireless_status->p_st_wlan_sta[i].mode : "");
  			xmlWrite(widget_xml+strlen(widget_xml), "<rate>%s</rate>", wireless_status->p_st_wlan_sta[i].rate ? wireless_status->p_st_wlan_sta[i].rate : "");
		}
	}

	xmlWrite(widget_xml+strlen(widget_xml), "</wlan_associated_station>");
	xmlWrite(widget_xml+strlen(widget_xml), "</wlan_interface_0>");
	
#ifdef USER_WL_ATH_5GHZ
	char widget_xml_1[MAX_RES_LEN_XML] = {0};	

	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_interface_1>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_enable>%s</wlan_enable>", nvram_safe_get("wlan1_enable"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_name>WLAN1</wlan_name>");
	StrEncode(replace_str, nvram_safe_get("wlan1_ssid"), 0);
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_ssid>%s</wlan_ssid>", replace_str);
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_dot11d_enable>%s</wlan_dot11d_enable>", nvram_safe_get("wlan1_11d_enable"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_channel_list>%s</wlan_channel_list>", wlan1_channel_list(channel_list));
	
	/* wlan info */
	/*
	 * Reason: Sometime can not get channel info
	 * Modified: Yufa Huang
	 * Date: 2010.05.04
	 */
	memset(channel, 0, 32);
	if (nvram_match("wlan1_enable", "1") == 0) {
		get_5g_channel(channel);
		if (strcmp(channel, "unknown") == 0) {
       		if (check_rc_status() == 'b')
				strcpy(channel, nvram_safe_get("wlan1_channel"));
		}
	}
   	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_channel>%s</wlan_channel>", channel);
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_auto_channel_enable>%s</wlan_auto_channel_enable>", nvram_safe_get("wlan1_auto_channel_enable"));	
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_dot11_mode>%s</wlan_dot11_mode>", nvram_safe_get("wlan1_dot11_mode"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_ssid_broadcast>%s</wlan_ssid_broadcast>", nvram_safe_get("wlan1_ssid_broadcast"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_mode>ap</wlan_mode>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_idle_time>5</wlan_idle_time>");

	if (strstr(nvram_safe_get("wlan1_security"), "disable")) {
    	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan1_security"));
	}
	else if (strstr(nvram_safe_get("wlan1_security"), "64")) {
		if (strstr(nvram_safe_get("wlan1_security"), "open"))
	    	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan1_security"));
	    else
		  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>wep_shared_64</wlan_security>");

  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wep64_key>");
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key0>%s</key0>", nvram_safe_get("wlan1_wep64_key_1"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key1>%s</key1>", nvram_safe_get("wlan1_wep64_key_2"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key2>%s</key2>", nvram_safe_get("wlan1_wep64_key_3"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key3>%s</key3>", nvram_safe_get("wlan1_wep64_key_4"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "</wlan_wep64_key>");
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wep_display>%s</wlan_wep_display>", nvram_safe_get("wlan1_wep_display"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wep_default_key>%s</wlan_wep_default_key>", nvram_safe_get("wlan1_wep_default_key"));
	}
	else if (strstr(nvram_safe_get("wlan1_security"), "128")) {
		if (strstr(nvram_safe_get("wlan1_security"), "open"))
	    	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan1_security"));
	   	else
		  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>wep_shared_128</wlan_security>");

  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wep128_key>");
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key0>%s</key0>", nvram_safe_get("wlan1_wep128_key_1"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key1>%s</key1>", nvram_safe_get("wlan1_wep128_key_2"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key2>%s</key2>", nvram_safe_get("wlan1_wep128_key_3"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<key3>%s</key3>", nvram_safe_get("wlan1_wep128_key_4"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "</wlan_wep128_key>");
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wep_display>%s</wlan_wep_display>", nvram_safe_get("wlan1_wep_display"));
  		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wep_default_key>%s</wlan_wep_default_key>", nvram_safe_get("wlan1_wep_default_key"));
	}
	else if (strstr(nvram_safe_get("wlan1_security"), "psk")) {
		if (strcmp(nvram_safe_get("wlan1_security"), "wpa2auto_psk") == 0)
			xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>wpa_auto_psk</wlan_security>");
   	    else
   	        xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan1_security"));

		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_psk_cipher_type>%s</wlan_psk_cipher_type>", nvram_safe_get("wlan1_psk_cipher_type"));
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_psk_pass_phrase>%s</wlan_psk_pass_phrase>", nvram_safe_get("wlan1_psk_pass_phrase"));
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wpa_groupkey_rekey_time>%s</wlan_wpa_groupkey_rekey_time>", nvram_safe_get("wlan1_gkey_rekey_time"));
	}
	else if (strstr(nvram_safe_get("wlan1_security"), "eap")){
	    if (strcmp(nvram_safe_get("wlan1_security"), "wpa2auto_eap") == 0)
  	       	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>wpa_auto_eap</wlan_security>");
  	    else
  	       	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_security>%s</wlan_security>", nvram_safe_get("wlan1_security"));

		eap = nvram_safe_get("wlan1_eap_radius_server_0");
		memcpy(fw_value, eap, sizeof(fw_value));
		getStrtok(fw_value, "/", "%s %s %s", &host, &port, &secret);
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_eap_radius_server>");
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<host>%s</host>", host);
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<port>%s</port>", port);
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<secret>%s</secret>", secret);
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "</wlan_eap_radius_server>");
		xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wpa_groupkey_rekey_time>%s</wlan_wpa_groupkey_rekey_time>", nvram_safe_get("wlan1_gkey_rekey_time"));
	}

	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_auto_txrate>auto</wlan_auto_txrate>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_txrate>Auto</wlan_txrate>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_txpower>full</wlan_txpower>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_beacon_interval>%s</wlan_beacon_interval>", nvram_safe_get("wlan1_beacon_interval"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_rts_threshold>%s</wlan_rts_threshold>", nvram_safe_get("wlan1_rts_threshold"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_fragmentation_threshold>%s</wlan_fragmentation_threshold>", nvram_safe_get("wlan1_fragmentation"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_dtim>%s</wlan_dtim>", nvram_safe_get("wlan1_dtim"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_short_preamble>1</wlan_short_preamble>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_cts>1</wlan_cts>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_wmm_enable>%s</wlan_wmm_enable>", nvram_safe_get("wlan1_wmm_enable"));
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_protection>%s</wlan_protection>", nvram_safe_get("wlan1_partition"));

#ifdef ATHEROS_11N_DRIVER
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_11n_protection>%s</wlan_11n_protection>", nvram_safe_get("wlan1_11n_protection"));
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_amsdu>4K</wlan_amsdu>");
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_channel_bandwidth>%s</wlan_channel_bandwidth>", nvram_safe_get("wlan1_11n_protection"));
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_guard_interval>short</wlan_guard_interval>");
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_ext_subchannel>auto</wlan_ext_subchannel>");
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_antenna_control>2</wlan_antenna_control>");
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_partition_enable>%s</wlan_partition_enable>", nvram_safe_get("wlan1_partition"));
#endif

	/* 2010/05/10, Yufa, bad solution for widget bug (request many wlan_stats in one seconds */
	get_statistics(WLAN1_ETH, &wlan1_st);

  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_tx_packets>%s</wlan_tx_packets>", wlan1_st.tx_packets);
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_rx_packets>%s</wlan_rx_packets>", wlan1_st.rx_packets);
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_lost_packets>%s</wlan_lost_packets>", wlan1_st.tx_dropped);
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_error_packets>%s</wlan_error_packets>", wlan1_st.tx_errors);
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<timestamp>%d</timestamp>", uptime());
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_tx_bytes>%s</wlan_tx_bytes>", wlan1_st.tx_bytes);
  	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_rx_bytes>%s</wlan_rx_bytes>", wlan1_st.rx_bytes);

  	//wlan1_associated_station
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "<wlan_associated_station>");
	wireless_status = get_5g_stalist();
	if (wireless_status != NULL) {
		for(i = 0 ; i < wireless_status->num_sta; i++) {
			if (dhcpd_status != NULL) {
	  			for (j = 0; j < dhcpd_status->num_list; j++) {
	  				if (strcmp(dhcpd_status->p_st_dhcpd_list[j].mac_addr, wireless_status->p_st_wlan_sta[i].mac_addr) == 0) {
						xmlWrite(widget_xml_1+strlen(widget_xml_1), "<name>%s</name>", dhcpd_status->p_st_dhcpd_list[j].hostname ? dhcpd_status->p_st_dhcpd_list[j].hostname : "");
	  				}
	  			}
			}
  			xmlWrite(widget_xml_1+strlen(widget_xml_1), "<mac>%s</mac>", wireless_status->p_st_wlan_sta[i].mac_addr ? wireless_status->p_st_wlan_sta[i].mac_addr : "");
  			xmlWrite(widget_xml_1+strlen(widget_xml_1), "<type>%s</type>", wireless_status->p_st_wlan_sta[i].mode ? wireless_status->p_st_wlan_sta[i].mode : "");
  			xmlWrite(widget_xml_1+strlen(widget_xml_1), "<rate>%s</rate>", wireless_status->p_st_wlan_sta[i].rate ? wireless_status->p_st_wlan_sta[i].rate : "");
		}
	}

	xmlWrite(widget_xml_1+strlen(widget_xml_1), "</wlan_associated_station>");
	xmlWrite(widget_xml_1+strlen(widget_xml_1), "</wlan_interface_1>");
	
	strcat(widget_xml, widget_xml_1);
#endif
	
	xmlWrite(widget_xml+strlen(widget_xml), "</wlan_stats>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[WLAN_INFO].data, 0 , sizeof(widget_xml));
	memcpy(xml_cache[WLAN_INFO].data, widget_xml, sizeof(widget_xml));
#ifdef USER_WL_ATH_5GHZ
	memset(xml_cache[WLAN1_INFO].data, 0 , sizeof(widget_xml_1));
	memcpy(xml_cache[WLAN1_INFO].data, widget_xml_1, strlen(widget_xml_1));		
#endif	
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_wps_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	static int time_tmp = 0;
/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 2) {
		widget_add_to_response(xml_cache[WPS_INFO].data, strlen(xml_cache[WPS_INFO].data));
		return 0;
	}

	time_tmp = time_sec;
	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<wps>");
  	xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", nvram_safe_get("wps_enable"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<ap_locked>%s</ap_locked>", nvram_safe_get("wps_lock"));
  	xmlWrite(widget_xml+strlen(widget_xml), "<configured>%s</configured>", (nvram_match("wps_configured_mode", "5") == 0) ? "1" : "0");
  	xmlWrite(widget_xml+strlen(widget_xml), "<pin>%s</pin>", nvram_safe_get("wps_pin"));
  	xmlWrite(widget_xml+strlen(widget_xml), "</wps>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[WPS_INFO].data, 0 , sizeof(widget_xml));
	memcpy(xml_cache[WPS_INFO].data, widget_xml, sizeof(widget_xml));
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_wan_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	char *ip="", *netmask="", *gateway="", *primary_dns="", *secondary_dns="";
	char wan_ip[192]={0};
	struct statistics  wan_st;
	static int time_tmp = 0;
	FILE *fd;
	char sessions[32];

	get_statistics(nvram_safe_get("wan_eth"), &wan_st);
	
	memset(sessions, 0, sizeof(sessions));
	if (fd = popen("cat /proc/sys/net/netfilter/nf_conntrack_count", "r")) {
		fgets(sessions, sizeof(sessions), fd);
		pclose(fd);
	}

/*
*	Date: 2010-05-17
*   Name: Yufa Huang
*   Reason: Modify for the system time no real-time update issue.
*   Notice :
*/
	int time_sec = time((time_t *) NULL);
	if (time_sec - time_tmp < 2) {
		char *offset;
		if (offset=strstr(xml_cache[WAN_INFO].data,"<wan_tx_bytes>")) {
			xmlWrite(offset, "<wan_tx_bytes>%s</wan_tx_bytes>", wan_st.tx_bytes);
  			xmlWrite(xml_cache[WAN_INFO].data+strlen(xml_cache[WAN_INFO].data), "<wan_rx_bytes>%s</wan_rx_bytes>", wan_st.rx_bytes);
  			xmlWrite(xml_cache[WAN_INFO].data+strlen(xml_cache[WAN_INFO].data), "<current_Internet_sessions>%s</current_Internet_sessions>", sessions);
  			xmlWrite(xml_cache[WAN_INFO].data+strlen(xml_cache[WAN_INFO].data), "</wan_stats>");
			xmlWrite(xml_cache[WAN_INFO].data+strlen(xml_cache[WAN_INFO].data), xmlpattenTailer);
			add_headers(200, "OK", NULL, "text/xml", -1, NULL);
		}
		widget_add_to_response(xml_cache[WAN_INFO].data, strlen(xml_cache[WAN_INFO].data));
		return 0;
	}

	time_tmp = time_sec;
	xmlWrite(widget_xml, xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_stats>");

	memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
	getStrtok(wan_ip, "/", "%s %s %s %s %s", &ip, &netmask, &gateway, &primary_dns, &secondary_dns);
 	xmlWrite(widget_xml+strlen(widget_xml),"<wan_mtu>%s</wan_mtu>",nvram_safe_get("wan_mtu"));

#ifdef OPENDNS
	xmlWrite(widget_xml+strlen(widget_xml),"<OpenDNS_enable>%s</OpenDNS_enable>", nvram_match("opendns_enable", "1") ? "false" : "true");
#endif
	if (strcmp(nvram_safe_get("wan_proto"), "dhcpc") == 0) {
		if (!strcmp(nvram_safe_get("wan_primary_dns"), "0.0.0.0") || !strcmp(nvram_safe_get("wan_primary_dns"), "")) {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>0</wan_specify_dns>");
	 		xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", primary_dns);
	  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", secondary_dns);
  		}
  		else {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>1</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", nvram_safe_get("wan_primary_dns"));
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", nvram_safe_get("wan_secondary_dns"));
		}
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_protocol>dhcp</wan_protocol>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_ip>%s</wan_ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_netmask>%s</wan_netmask>", netmask);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_default_gateway>%s</wan_default_gateway>", gateway);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcp_server_ip>%s</wan_dhcp_server_ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcpc_expired_time>%lu</wan_dhcpc_expired_time>", lease_time_execute(0));
	}
  	else if (strcmp(nvram_safe_get("wan_proto"), "static") == 0) {
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>%s</wan_specify_dns>", nvram_safe_get("wan_specify_dns"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", nvram_safe_get("wan_primary_dns"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", nvram_safe_get("wan_secondary_dns"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_protocol>static</wan_protocol>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_ip>%s</wan_ip>", nvram_safe_get("wan_static_ipaddr"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_netmask>%s</wan_netmask>", nvram_safe_get("wan_static_netmask"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_default_gateway>%s</wan_default_gateway>", nvram_safe_get("wan_static_gateway"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcp_server_ip>0.0.0.0</wan_dhcp_server_ip>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcpc_expired_time></wan_dhcpc_expired_time>");
	}
	else if (strcmp(nvram_safe_get("wan_proto"), "pptp") == 0) {
		if (!strcmp(nvram_safe_get("wan_primary_dns"), "0.0.0.0") || !strcmp(nvram_safe_get("wan_primary_dns"), "")) {
			xmlWrite(widget_xml+strlen(widget_xml),"<wan_specify_dns>0</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml),"<wan_primary_dns>%s</wan_primary_dns>", primary_dns);
			xmlWrite(widget_xml+strlen(widget_xml),"<wan_secondary_dns>%s</wan_secondary_dns>", secondary_dns);
		}
		else {
			xmlWrite(widget_xml+strlen(widget_xml),"<wan_specify_dns>1</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml),"<wan_primary_dns>%s</wan_primary_dns>", nvram_safe_get("wan_primary_dns"));
			xmlWrite(widget_xml+strlen(widget_xml),"<wan_secondary_dns>%s</wan_secondary_dns>", nvram_safe_get("wan_secondary_dns"));
		}
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_protocol>pptp</wan_protocol>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_ip>%s</wan_ip>", ip);
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_netmask>%s</wan_netmask>", netmask);
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_default_gateway>%s</wan_default_gateway>", gateway);
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcp_server_ip>0.0.0.0</wan_dhcp_server_ip>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcpc_expired_time></wan_dhcpc_expired_time>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_pptp_session>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<dynamic>%s</dynamic>", nvram_safe_get("wan_pptp_dynamic"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<username>%s</username>", nvram_safe_get("wan_pptp_username"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<password>%s</password>", nvram_safe_get("wan_pptp_password"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<max_idle_time>%s</max_idle_time>", nvram_safe_get("wan_pptp_max_idle_time"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<connect_mode>%s</connect_mode>", nvram_safe_get("wan_pptp_connect_mode"));
		xmlWrite(widget_xml+strlen(widget_xml), "<ppp_auth_type></ppp_auth_type>");
  		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", ip);
  		xmlWrite(widget_xml+strlen(widget_xml), "<netmask>%s</netmask>", netmask);
  		xmlWrite(widget_xml+strlen(widget_xml), "<gateway>%s</gateway>", gateway);
  		xmlWrite(widget_xml+strlen(widget_xml), "<server_ip>%s</server_ip>", nvram_safe_get("wan_pptp_server_ip"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<mtu>%s</mtu>", nvram_safe_get("wan_pptp_mtu"));
  		xmlWrite(widget_xml+strlen(widget_xml), "<connection_status>%s</connection_status>", wan_statue("pptp"));
		xmlWrite(widget_xml+strlen(widget_xml), "</wan_pptp_session>");
	}
	else if (strcmp(nvram_safe_get("wan_proto"), "l2tp") == 0) {
  		if (!strcmp(nvram_safe_get("wan_primary_dns"), "0.0.0.0") || !strcmp(nvram_safe_get("wan_primary_dns"), "")) {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>0</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", primary_dns);
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", secondary_dns);
		}
		else {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>1</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", nvram_safe_get("wan_primary_dns"));
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", nvram_safe_get("wan_secondary_dns"));
		}
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_protocol>l2tp</wan_protocol>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_ip>%s</wan_ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_netmask>%s</wan_netmask>", netmask);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_default_gateway>%s</wan_default_gateway>", gateway);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcp_server_ip>0.0.0.0</wan_dhcp_server_ip>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcpc_expired_time></wan_dhcpc_expired_time>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_l2tp_session>");
		xmlWrite(widget_xml+strlen(widget_xml), "<dynamic>%s</dynamic>", nvram_safe_get("wan_l2tp_dynamic"));
		xmlWrite(widget_xml+strlen(widget_xml), "<username>%s</username>", nvram_safe_get("wan_l2tp_username"));
		xmlWrite(widget_xml+strlen(widget_xml), "<password>%s</password>", nvram_safe_get("wan_l2tp_password"));
		xmlWrite(widget_xml+strlen(widget_xml), "<max_idle_time>%s</max_idle_time>", nvram_safe_get("wan_l2tp_max_idle_time"));
		xmlWrite(widget_xml+strlen(widget_xml), "<connect_mode>%s</connect_mode>", nvram_safe_get("wan_l2tp_connect_mode"));
		xmlWrite(widget_xml+strlen(widget_xml), "<ppp_auth_type></ppp_auth_type>");
		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<netmask>%s</netmask>", netmask);
		xmlWrite(widget_xml+strlen(widget_xml), "<gateway>%s</gateway>", gateway);
		xmlWrite(widget_xml+strlen(widget_xml), "<server_ip>%s</server_ip>", nvram_safe_get("wan_l2tp_server_ip"));
		xmlWrite(widget_xml+strlen(widget_xml), "<mtu>%s</mtu>", nvram_safe_get("wan_l2tp_mtu"));
		xmlWrite(widget_xml+strlen(widget_xml), "<connection_status>%s</connection_status>", wan_statue("l2tp"));
		xmlWrite(widget_xml+strlen(widget_xml),"</wan_l2tp_session>");
	}
	else if (strcmp(nvram_safe_get("wan_proto"), "pppoe") == 0) {
		if (!strcmp(nvram_safe_get("wan_primary_dns"), "0.0.0.0") || !strcmp(nvram_safe_get("wan_primary_dns"), "")) {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>0</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", primary_dns);
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", secondary_dns);
		}
		else{
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>1</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", nvram_safe_get("wan_primary_dns"));
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", nvram_safe_get("wan_secondary_dns"));
		}
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_protocol>pppoe</wan_protocol>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_ip>%s</wan_ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_netmask>%s</wan_netmask>", netmask);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_default_gateway>%s</wan_default_gateway>", gateway);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcp_server_ip>0.0.0.0</wan_dhcp_server_ip>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_dhcpc_expired_time></wan_dhcpc_expired_time>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_pppoe_session_0>");
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", nvram_safe_get("wan_pppoe_enable_00"));
		xmlWrite(widget_xml+strlen(widget_xml), "<dynamic>%s</dynamic>", nvram_safe_get("wan_pppoe_dynamic_00"));
		xmlWrite(widget_xml+strlen(widget_xml), "<username>%s</username>", nvram_safe_get("wan_pppoe_username_00"));
		xmlWrite(widget_xml+strlen(widget_xml), "<password>%s</password>", nvram_safe_get("wan_pppoe_password_00"));
		xmlWrite(widget_xml+strlen(widget_xml), "<max_idle_time>%s</max_idle_time>", nvram_safe_get("wan_pppoe_max_idle_time_00"));
		xmlWrite(widget_xml+strlen(widget_xml), "<connect_mode>%s</connect_mode>", nvram_safe_get("wan_pppoe_connect_mode_00"));
		xmlWrite(widget_xml+strlen(widget_xml), "<ppp_auth_type></ppp_auth_type>");
		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<netmask>%s</netmask>", netmask);
		xmlWrite(widget_xml+strlen(widget_xml), "<gateway>%s</gateway>", gateway);
		xmlWrite(widget_xml+strlen(widget_xml), "<mtu>%s</mtu>", nvram_safe_get("wan_pppoe_mtu"));
		xmlWrite(widget_xml+strlen(widget_xml), "<connection_status>%s</connection_status>", wan_statue("pppoe"));
		xmlWrite(widget_xml+strlen(widget_xml), "</wan_pppoe_session_0>");
	}
	else if (strcmp(nvram_safe_get("wan_proto"), "usb3g") == 0) {
		if (!strcmp(nvram_safe_get("wan_primary_dns"), "0.0.0.0") || !strcmp(nvram_safe_get("wan_primary_dns"), "")) {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>0</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", primary_dns);
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", secondary_dns);
		}
		else {
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_specify_dns>1</wan_specify_dns>");
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_primary_dns>%s</wan_primary_dns>", nvram_safe_get("wan_primary_dns"));
			xmlWrite(widget_xml+strlen(widget_xml), "<wan_secondary_dns>%s</wan_secondary_dns>", nvram_safe_get("wan_secondary_dns"));
		}
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_protocol>3G MODEM</wan_protocol>");
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_ip>%s</wan_ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_netmask>%s</wan_netmask>", netmask);
		xmlWrite(widget_xml+strlen(widget_xml), "<wan_default_gateway>%s</wan_default_gateway>", gateway);

		xmlWrite(widget_xml+strlen(widget_xml), "<wan_wwan>");
		xmlWrite(widget_xml+strlen(widget_xml), "<country>%s</country>", nvram_safe_get("usb3g_country"));
		xmlWrite(widget_xml+strlen(widget_xml), "<isp>%s</isp>", nvram_safe_get("usb3g_isp"));
		xmlWrite(widget_xml+strlen(widget_xml), "<username>%s</username>", nvram_safe_get("usb3g_username"));
		xmlWrite(widget_xml+strlen(widget_xml), "<password>%s</password>", nvram_safe_get("usb3g_password"));
		xmlWrite(widget_xml+strlen(widget_xml), "<dial_number>%s</dial_number>", nvram_safe_get("usb3g_dial_num"));
		xmlWrite(widget_xml+strlen(widget_xml), "<connect_mode>%s</connect_mode>", nvram_safe_get("usb3g_reconnect_mode"));
		xmlWrite(widget_xml+strlen(widget_xml), "<wwan_auth_type>%s</wwan_auth_type>", nvram_safe_get("usb3g_auth"));
		xmlWrite(widget_xml+strlen(widget_xml), "<apn>%s</apn>", nvram_safe_get("usb3g_apn_name"));
		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<netmask>%s</netmask>", netmask);
		xmlWrite(widget_xml+strlen(widget_xml), "<gateway>%s</gateway>", gateway);
		xmlWrite(widget_xml+strlen(widget_xml), "<mtu>%s</mtu>", nvram_safe_get("usb3g_wan_mtu"));
		xmlWrite(widget_xml+strlen(widget_xml), "<connection_status>%s</connection_status>", wan_statue("usb3g"));
		xmlWrite(widget_xml+strlen(widget_xml), "</wan_wwan>");
	}

	xmlWrite(widget_xml+strlen(widget_xml), "<wan_tx_packets>%s</wan_tx_packets>", wan_st.tx_packets);
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_rx_packets>%s</wan_rx_packets>", wan_st.rx_packets);
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_lost_packets>%s</wan_lost_packets>", wan_st.tx_dropped);
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_collision_packets>%s</wan_collision_packets>", wan_st.tx_collisions);
	xmlWrite(widget_xml+strlen(widget_xml), "<timestamp>%d</timestamp>", uptime());
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_uptime>%lu</wan_uptime>", get_wan_connect_time(nvram_safe_get("wan_proto"), 1));
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_total_uptime>%lu</wan_total_uptime>", get_wan_connect_time(nvram_safe_get("wan_proto"), 1));
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_tx_bytes>%s</wan_tx_bytes>", wan_st.tx_bytes);
	xmlWrite(widget_xml+strlen(widget_xml), "<wan_rx_bytes>%s</wan_rx_bytes>", wan_st.rx_bytes);
	xmlWrite(widget_xml+strlen(widget_xml), "<current_Internet_sessions>%s</current_Internet_sessions>", sessions);
	xmlWrite(widget_xml+strlen(widget_xml), "</wan_stats>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);

	memset(xml_cache[WAN_INFO].data, 0, sizeof(widget_xml));
	memcpy(xml_cache[WAN_INFO].data, widget_xml, sizeof(widget_xml));
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_scheduler_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	int i;
	char buf[32];
	char fw_value[192]={0};
	char *name="", *weekdays="", *start_time="", *end_time="";
	int set_tag = 0;

	xmlWrite(widget_xml,xmlpattenHeader);

	//11/1010000/00:34/23:56
	for (i = 0; i< 24; i++) {
		sprintf(buf, "schedule_rule_%02d", i);
		if (nvram_safe_get(buf) == NULL || strlen(nvram_safe_get(buf)) == 0) {
			if (!set_tag) {
				xmlWrite(widget_xml+strlen(widget_xml), "<scheduler_enable>disable</scheduler_enable>");
				set_tag = 1;
			}
			continue;
		}
		else {
			if (!set_tag) {
				xmlWrite(widget_xml+strlen(widget_xml), "<scheduler_enable>enable</scheduler_enable>");
				set_tag = 1;
			}
		}

		strncpy(fw_value, nvram_safe_get(buf), sizeof(fw_value));
		getStrtok(fw_value, "/", "%s %s %s %s", &name, &weekdays, &start_time, &end_time);

		xmlWrite(widget_xml+strlen(widget_xml), "<schedule_rule_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", name);
		xmlWrite(widget_xml+strlen(widget_xml), "<weekdays>%s</weekdays>", weekdays);
		xmlWrite(widget_xml+strlen(widget_xml), "<start_time>%s</start_time>", start_time);
		xmlWrite(widget_xml+strlen(widget_xml), "<end_time>%s</end_time>", end_time);
		xmlWrite(widget_xml+strlen(widget_xml), "</schedule_rule_%d>",i);
		memset(fw_value, 0, sizeof(fw_value));
	}
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_virtual_server_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	int i;
	char buf[32];
	char fw_value[192]={0};
	char *enable="", *name="", *protocol_type="", *pub_port="", *pri_port="", *ip="", *sch="", *inbound="";

	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml),"<virtual_server>");

	//vs_rule_00 = 1/TELNET/6/23/23/192.168.10.103/Never/Allow_All
	for (i = 0; i< 24; i++) {
		sprintf(buf, "vs_rule_%02d", i);
		strncpy(fw_value, nvram_safe_get(buf), sizeof(fw_value));
		if ((strlen(fw_value) == 0) || !strcmp(fw_value, "0//6/0/0/0.0.0.0/Always/Allow_All"))
		   	continue;

		getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s", &enable, &name, &protocol_type, &pub_port \
			, &pri_port, &ip, &sch, &inbound);
		xmlWrite(widget_xml+strlen(widget_xml), "<virtual_server_entry_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", enable);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", name);

  		if (strcmp(protocol_type, "6") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>TCP</protocol_type>");
  		else if (strcmp(protocol_type, "17") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>UDP</protocol_type>");
  		else if (strcmp(protocol_type, "256") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>Both</protocol_type>");
		else
			xmlWrite(widget_xml+strlen(widget_xml), "<protocol_type>%s</protocol_type>", protocol_type);

		xmlWrite(widget_xml+strlen(widget_xml), "<public_port>%s</public_port>", pub_port);
		xmlWrite(widget_xml+strlen(widget_xml), "<private_port>%s</private_port>", pri_port);
		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<schedule_name>%s</schedule_name>", sch);
		xmlWrite(widget_xml+strlen(widget_xml), "</virtual_server_entry_%d>", i);
		memset(fw_value, 0, sizeof(fw_value));
	}
	xmlWrite(widget_xml+strlen(widget_xml), "</virtual_server>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_port_forwarding_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	char buf_forward[32];
	char fw_value[192]={0};
	char *enable="", *name="",*ip="", *sch="";
	int i = 0;

	char *tcp_port_range="", *udp_port_range="", *inbound="";

	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<port_forwarding>");

	for (i = 0; i< 24; i++) {
		/* 1/Far Cry/192.168.1.103/900/49001,49002/Always/Allow_All */
		sprintf(buf_forward, "port_forward_both_%02d",i);
		strncpy(fw_value, nvram_safe_get(buf_forward), sizeof(fw_value));
		if ((strlen(fw_value) == 0) || !strcmp(fw_value, "0//0.0.0.0/0/0/Always/Allow_All"))
			continue;
		getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s", &enable, &name, &ip, &tcp_port_range \
			, &udp_port_range, &sch, &inbound);
		if (enable == NULL)
			continue;
		if (!strcmp(enable,"0"))
			continue;
		xmlWrite(widget_xml+strlen(widget_xml), "<port_forward_entry_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", enable);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", name);
		xmlWrite(widget_xml+strlen(widget_xml), "<tcp_port>%s</tcp_port>", tcp_port_range);
		xmlWrite(widget_xml+strlen(widget_xml), "<udp_port>%s</udp_port>", udp_port_range);

		xmlWrite(widget_xml+strlen(widget_xml), "<ip>%s</ip>", ip);
		xmlWrite(widget_xml+strlen(widget_xml), "<schedule_name>%s</schedule_name>", sch);
		xmlWrite(widget_xml+strlen(widget_xml), "</port_forward_entry_%d>", i);
		memset(fw_value, 0, sizeof(fw_value));
	}
	xmlWrite(widget_xml+strlen(widget_xml), "</port_forwarding>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_application_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	int i;
	char buf_app[15];
	char fw_value[192]={0};
	char *app_enable="", *app_name="", *trigger_protocol="", *trigger_port="", *pub_protocol="", *pub_port="", *sch="", *inbound="";

	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml),"<application_rule>");
	for (i = 0; i< 24; i++) {
		sprintf(buf_app, "application_%02d",i);
		strncpy(fw_value, nvram_safe_get(buf_app), sizeof(fw_value));
		if ((strlen(fw_value) == 0) || !strcmp(fw_value, "0//TCP/0/TCP/0/Always"))
		   continue;

		getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s", &app_enable, &app_name, &trigger_protocol, &trigger_port \
			, &pub_protocol, &pub_port, &sch, &inbound);
		xmlWrite(widget_xml+strlen(widget_xml), "<application_entry_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", app_enable);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", app_name);
		xmlWrite(widget_xml+strlen(widget_xml), "<trigger_protocol>%s</trigger_protocol>", trigger_protocol);
		xmlWrite(widget_xml+strlen(widget_xml), "<trigger_ports>%s</trigger_ports>", trigger_port);
		xmlWrite(widget_xml+strlen(widget_xml), "<public_protocol>%s</public_protocol>", pub_protocol);
		xmlWrite(widget_xml+strlen(widget_xml), "<public_ports>%s</public_ports>", pub_port);
		xmlWrite(widget_xml+strlen(widget_xml), "<schedule_name>%s</schedule_name>", sch);
		xmlWrite(widget_xml+strlen(widget_xml), "</application_entry_%d>", i);
		memset(fw_value, 0, sizeof(fw_value));
	}
	xmlWrite(widget_xml+strlen(widget_xml), "</application_rule>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_filter_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	int i;
	char buf_mac[32];
	char buf_url[32];
	char fw_value[192]={0};
	char filter_type[32];
	char *macfilter_enable="", *macfilter_name="", *macfilter_mac="", *macfilter_sch="";

	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml),"<filter_config>");
	xmlWrite(widget_xml+strlen(widget_xml),"<filter_spi_enable>%s</filter_spi_enable>", nvram_safe_get("spi_enable"));

	for (i = 0; i< 24; i++) {
		sprintf(buf_mac, "mac_filter_%02d",i);
		if (strlen(buf_mac) == 0 || !strcmp(nvram_safe_get(buf_mac), "0/name/00:00:00:00:00:00/Always"))
			continue;

		strncpy(fw_value, nvram_safe_get(buf_mac), sizeof(fw_value));
		getStrtok(fw_value, "/", "%s %s %s %s", &macfilter_enable, &macfilter_name, &macfilter_mac, &macfilter_sch);
	
		xmlWrite(widget_xml+strlen(widget_xml), "<mac_filter>");
		strcpy(filter_type, nvram_safe_get("mac_filter_type"));
		if (strcmp(filter_type, "list_allow") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<mac_filter_type>%s</mac_filter_type>", "allow");
		else if (strcmp(filter_type, "list_deny") == 0)
			xmlWrite(widget_xml+strlen(widget_xml), "<mac_filter_type>%s</mac_filter_type>", "deny");
		else
			xmlWrite(widget_xml+strlen(widget_xml), "<mac_filter_type>%s</mac_filter_type>", filter_type);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%d</name>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<mac>%s</mac>", macfilter_mac);
		xmlWrite(widget_xml+strlen(widget_xml), "<schedule_name>%s</schedule_name>", macfilter_sch);
		xmlWrite(widget_xml+strlen(widget_xml), "</mac_filter>");

		memset(fw_value, 0, sizeof(fw_value));
	}
	xmlWrite(widget_xml+strlen(widget_xml), "<protocol_filter_0>");
	xmlWrite(widget_xml+strlen(widget_xml), "</protocol_filter_0>");

	for (i = 0; i< 24; i++) {
		sprintf(buf_url, "url_domain_filter_%02d", i);
		if (strlen(nvram_safe_get(buf_url)) == 0)
			continue;

		xmlWrite(widget_xml+strlen(widget_xml), "<url_domain_filter>");
		xmlWrite(widget_xml+strlen(widget_xml), "<filter_type>%s</filter_type>", nvram_safe_get("url_domain_filter_type"));
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%d</name>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<url_string>%s</url_string>", nvram_safe_get(buf_url));
		xmlWrite(widget_xml+strlen(widget_xml), "<schedule_name></schedule_name>");
		xmlWrite(widget_xml+strlen(widget_xml), "</url_domain_filter>");

	}
	xmlWrite(widget_xml+strlen(widget_xml), "</filter_config>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_time_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	div_t time_zone;
	time_zone = div(atoi(nvram_safe_get("time_zone")), 16);

	xmlWrite(widget_xml, xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<time>");
	xmlWrite(widget_xml+strlen(widget_xml), "<time_zone_index>%d</time_zone_index>", time_zone.quot);
	xmlWrite(widget_xml+strlen(widget_xml), "<time_daylight_saving>");
	xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", nvram_safe_get("time_daylight_saving_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<start_month>%s</start_month>", nvram_safe_get("time_daylight_saving_start_month"));
	xmlWrite(widget_xml+strlen(widget_xml), "<start_week>%s</start_week>", nvram_safe_get("time_daylight_saving_start_week"));
	xmlWrite(widget_xml+strlen(widget_xml), "<start_day_of_week>%s</start_day_of_week>", nvram_safe_get("time_daylight_saving_start_day_of_week"));
	xmlWrite(widget_xml+strlen(widget_xml), "<start_time>%s</start_time>", nvram_safe_get("time_daylight_saving_start_time"));
	xmlWrite(widget_xml+strlen(widget_xml), "<end_month>%s</end_month>", nvram_safe_get("time_daylight_saving_end_month"));
	xmlWrite(widget_xml+strlen(widget_xml), "<end_week>%s</end_week>", nvram_safe_get("time_daylight_saving_end_week"));
	xmlWrite(widget_xml+strlen(widget_xml), "<end_day_of_week>%s</end_day_of_week>", nvram_safe_get("time_daylight_saving_end_day_of_week"));
	xmlWrite(widget_xml+strlen(widget_xml), "<end_time>%s</end_time>", nvram_safe_get("time_daylight_saving_end_time"));
	xmlWrite(widget_xml+strlen(widget_xml), "</time_daylight_saving>");
	xmlWrite(widget_xml+strlen(widget_xml), "<ntp>");
	xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", nvram_safe_get("ntp_client_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<sync_interval>%s</sync_interval>", nvram_safe_get("ntp_sync_interval"));
	xmlWrite(widget_xml+strlen(widget_xml), "<server>%s</server>", nvram_safe_get("ntp_server"));
	xmlWrite(widget_xml+strlen(widget_xml), "</ntp>");
	xmlWrite(widget_xml+strlen(widget_xml), "</time>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_ddns_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	char parse_ddns[128] = {0};
	char *ddns_workable ="", *last_host_registered ="", *last_ip_registered ="", *last_server_registered ="";
	char *ip="", *netmask="", *gateway="", *primary_dns="", *secondary_dns="";
	char wan_ip[192]={0};
	char status[64] = {0};

	ddns_status(parse_ddns, status);

	if (strlen(parse_ddns) != 0)
		getStrtok(parse_ddns, "|", "%s %s %s %s", &ddns_workable, &last_host_registered, &last_ip_registered, &last_server_registered);

	memcpy(wan_ip, read_current_ipaddr(0), sizeof(wan_ip));
	getStrtok(wan_ip, "/", "%s %s %s %s %s", &ip, &netmask, &gateway, &primary_dns, &secondary_dns);

	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<ddns>");
	xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", nvram_safe_get("ddns_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<hostname>%s</hostname>", nvram_safe_get("ddns_hostname"));
	xmlWrite(widget_xml+strlen(widget_xml), "<server>%s</server>", nvram_safe_get("ddns_type"));
	xmlWrite(widget_xml+strlen(widget_xml), "<username>%s</username>", nvram_safe_get("ddns_username"));
	xmlWrite(widget_xml+strlen(widget_xml), "<password>%s</password>", nvram_safe_get("ddns_password"));
	xmlWrite(widget_xml+strlen(widget_xml), "<kinds>%s</kinds>", nvram_safe_get("ddns_dyndns_kinds"));
	xmlWrite(widget_xml+strlen(widget_xml), "<last_host_registered>%s</last_host_registered>", last_host_registered == NULL? nvram_safe_get("ddns_hostname") : last_host_registered);
	xmlWrite(widget_xml+strlen(widget_xml), "<last_ip_registered>%s</last_ip_registered>", last_ip_registered == NULL? ip: last_ip_registered);
	xmlWrite(widget_xml+strlen(widget_xml), "<last_server_registered>%s</last_server_registered>", last_server_registered == NULL ? nvram_safe_get("ddns_type") : last_server_registered);
	xmlWrite(widget_xml+strlen(widget_xml), "<timeout>%s</timeout>", nvram_safe_get("ddns_sync_interval"));
	xmlWrite(widget_xml+strlen(widget_xml), "<status>%s</status>", status);
	xmlWrite(widget_xml+strlen(widget_xml), "</ddns>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_dmz_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};

	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<dmz>");
	xmlWrite(widget_xml+strlen(widget_xml), "<dmz_enable>%s</dmz_enable>", nvram_safe_get("dmz_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<dmz_ip>%s</dmz_ip>", nvram_safe_get("dmz_ipaddr"));
	xmlWrite(widget_xml+strlen(widget_xml), "<dmz_schedule>Always</dmz_schedule>");
	xmlWrite(widget_xml+strlen(widget_xml), "</dmz>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_log_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	char *syslog_enable="0", *syslog_ip="0.0.0.0";

	getStrtok(nvram_safe_get("syslog_server"), "/", "%s %s", &syslog_enable, &syslog_ip);

	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<system_log>");
	xmlWrite(widget_xml+strlen(widget_xml), "<log_critical>1</log_critical>");
	xmlWrite(widget_xml+strlen(widget_xml), "<log_system>%s</log_system>", nvram_safe_get("log_system_activity"));
	xmlWrite(widget_xml+strlen(widget_xml), "<log_debug>%s</log_debug>", nvram_safe_get("log_debug_information"));
	xmlWrite(widget_xml+strlen(widget_xml), "<log_firewall>1</log_firewall>");
	xmlWrite(widget_xml+strlen(widget_xml), "<log_warning>1</log_warning>");
	xmlWrite(widget_xml+strlen(widget_xml), "<log_status>1</log_status>");
	xmlWrite(widget_xml+strlen(widget_xml), "<log_syslog_enable>%s</log_syslog_enable>", syslog_enable);
	xmlWrite(widget_xml+strlen(widget_xml), "<log_syslog_server>%s</log_syslog_server>", syslog_ip);
	xmlWrite(widget_xml+strlen(widget_xml), "<log_email_enable>%s</log_email_enable>", nvram_safe_get("log_email_enable"));
	xmlWrite(widget_xml+strlen(widget_xml), "<log_email_sender>%s</log_email_sender>", nvram_safe_get("log_email_sender"));
	xmlWrite(widget_xml+strlen(widget_xml), "<log_email_recipient>%s</log_email_recipient>", nvram_safe_get("log_email_recipien"));
	xmlWrite(widget_xml+strlen(widget_xml), "<log_email_server>%s</log_email_server>", nvram_safe_get("log_email_server"));
	xmlWrite(widget_xml+strlen(widget_xml), "<log_email_username>%s</log_email_username>", nvram_safe_get("log_email_username"));
	xmlWrite(widget_xml+strlen(widget_xml), "<log_email_password>%s</log_email_password>", nvram_safe_get("log_email_password"));
	xmlWrite(widget_xml+strlen(widget_xml), "</system_log>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

int create_routing_table_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	int i = 0;
	routing_table_t *routing_table_list;

	routing_table_list = read_route_table();
	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<routing_table>");

	for (i=0; strlen(routing_table_list[i].dest_addr) >0; i++) {
		if (strcmp(routing_table_list[i].interface, "lo")) {
			if (strcmp(routing_table_list[i].interface, "br0") == 0)
				strcpy(routing_table_list[i].interface, "LAN");

			xmlWrite(widget_xml+strlen(widget_xml), "<routing_entry_%d>", i);
			xmlWrite(widget_xml+strlen(widget_xml), "<dest_addr>%s</dest_addr>", routing_table_list[i].dest_addr ? routing_table_list[i].dest_addr : "");
			xmlWrite(widget_xml+strlen(widget_xml), "<dest_mask>%s</dest_mask>", routing_table_list[i].dest_mask ? routing_table_list[i].dest_mask : "");
			xmlWrite(widget_xml+strlen(widget_xml), "<gateway>%s</gateway>", routing_table_list[i].gateway ? routing_table_list[i].gateway : "");
			xmlWrite(widget_xml+strlen(widget_xml), "<Interface>%s</Interface>", routing_table_list[i].interface ? routing_table_list[i].interface : "");
			xmlWrite(widget_xml+strlen(widget_xml), "<metric>%d</metric>", routing_table_list[i].metric);
			xmlWrite(widget_xml+strlen(widget_xml), "</routing_entry_%d>", i);
		}
	}

  	xmlWrite(widget_xml+strlen(widget_xml), "</routing_table>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_static_route_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	int i;
	char buf_app[32];
	char fw_value[192]={0};
	char *routing;
	char *enable="", *name="", *dest_addr="", *dest_mask="", *dest_gateway="", *iface="", *metric="";

	//static_routing_01      1/Name/192.168.2.0/255.255.255.0/192.168.2.252/WAN_PHY/0
	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<static_route>");

	for (i = 0; i< 32; i++) {
		sprintf(buf_app, "static_routing_%02d", i);
		routing = nvram_safe_get(buf_app);
		if ((strcmp("0//0.0.0.0/0.0.0.0/0.0.0.0/WAN/1", routing) == 0) ||
			(strlen(routing) == 0))
			continue;

		strncpy(fw_value, routing, sizeof(fw_value));
		getStrtok(fw_value, "/", "%s %s %s %s %s %s %s %s", &enable, &name, &dest_addr, &dest_mask \
		, &dest_gateway, &iface, &metric);

		xmlWrite(widget_xml+strlen(widget_xml), "<static_routing_%d>", i);
		xmlWrite(widget_xml+strlen(widget_xml), "<enable>%s</enable>", enable);
		xmlWrite(widget_xml+strlen(widget_xml), "<name>%s</name>", name);
		xmlWrite(widget_xml+strlen(widget_xml), "<dest_addr>%s</dest_addr>", dest_addr);
		xmlWrite(widget_xml+strlen(widget_xml), "<dest_mask>%s</dest_mask>", dest_mask);
		xmlWrite(widget_xml+strlen(widget_xml), "<gateway>%s</gateway>", dest_gateway);
		xmlWrite(widget_xml+strlen(widget_xml), "<interface>%s</interface>", iface);
		xmlWrite(widget_xml+strlen(widget_xml), "<metric>%s</metric>", metric);
		xmlWrite(widget_xml+strlen(widget_xml), "</static_routing_%d>", i);
		memset(fw_value, 0, sizeof(fw_value));
	}

  	xmlWrite(widget_xml+strlen(widget_xml),"</static_route>");
	xmlWrite(widget_xml+strlen(widget_xml),xmlpattenTailer);
	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}

static int create_rip_config_xml(void)
{
	char widget_xml[MAX_RES_LEN_XML] = {0};
	xmlWrite(widget_xml,xmlpattenHeader);
	xmlWrite(widget_xml+strlen(widget_xml), "<rip_config>");
	xmlWrite(widget_xml+strlen(widget_xml), "<rip_enable></rip_enable>");
	xmlWrite(widget_xml+strlen(widget_xml), "<rip_rx_version></rip_rx_version>");
	xmlWrite(widget_xml+strlen(widget_xml), "<rip_tx_version></rip_tx_version>");
	xmlWrite(widget_xml+strlen(widget_xml), "<rip_v2_type></rip_v2_type>");
  	xmlWrite(widget_xml+strlen(widget_xml), "</rip_config>");
	xmlWrite(widget_xml+strlen(widget_xml), xmlpattenTailer);

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	widget_add_to_response(widget_xml, strlen(widget_xml));
	return 0;
}
