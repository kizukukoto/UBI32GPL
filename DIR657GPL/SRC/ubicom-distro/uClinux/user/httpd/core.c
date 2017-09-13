#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <assert.h>
#include <search.h>
#include <signal.h>
#include <fcntl.h>

#include <sutil.h>
#include <nvram.h>
#include <shvar.h>

#include <file.h>
#include <httpd_util.h>
#include <log.h>
#include <bsp.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/shm.h>
#include <syslog.h>

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

#ifdef XML_AGENT
#include <widget.h>
#endif

#ifdef AJAX
#include <ajax.h>
#endif

//#define HTTPD_DEBUG 1
#ifdef HTTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define MP_DEBUG 1
#ifdef MP_DEBUG
#define MP_DEBUG_MSG(fmt, arg...)       syslog(LOG_NOTICE, fmt, ##arg)
#else
#define MP_DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#endif

//#define ATH_AR938X_WPS20 1

#define POST_BUF_SIZE   10000
#define MIN(a, b)           ((a) < (b)? (a): (b))
#define sys_restart()     kill(read_pid(RC_PID), SIGHUP)
#define dhcpc_renew()     _system("killall -SIGUSR1 udhcpc")
#define dhcpc_release()   _system("killall -SIGUSR2 udhcpc")
void sys_reboot()         { dhcpc_release(); sleep(3); _system("reboot -d2 &"); }
#ifdef CONFIG_USER_TC
#define remove_bandwidth_file() unlink("/var/tmp/bandwidth_result.txt")
#endif
#define USB_MASS_STORAGE    "/proc/scsi/usb-storage-0/0"
#define NVRAM_DEFAULT       "/var/etc/nvram.default"
#define NVRAM_CONF          "/var/etc/nvram.conf"
#define NVRAM_TMP           "/var/etc/nvram.tmp"
/* Firmware status*/
#define FIRMWARE_UPGRADE_FAIL     0
#define FIRMWARE_UPGRADE_SUCCESS  1
#define FIRMWARE_UPGRADE_LOADER_SUCCESS  2
#define FIRMWARE_SIZE_ERROR       3
#ifdef CONFIG_LP
#define	LP_UPGRADE_SUCCESS		  4
#define	LP_UPGRADE_FAIL			  5
#endif

/* html page */
#define HTML_RESPONSE_PAGE        1
#define HTML_RETURN_PAGE          2
#define NO_HTML_PAGE              3

/* action type */
#define F_UPLOAD                  0
#define F_REBOOT                  1
#define F_UPGRADE                 2
#define F_RESTORE                 3
#define F_INITIAL                 4
#define F_RC_RESTART              5
#define F_TYPE_COUNT              6 // if you add F_XXX type, don't forget to fix it !!
                                    // the total count of F_XXX
/* NVRAM */
#define NVRAM_SIZE              1024
#define NVRAM_FILE              "/var/etc/nvram.conf"
#define NVRAM_READ_BUF      64 * 1024

struct nvram_table {
    char name[40];
    char value[192];
};

/* jimmy added 20081226, for graphic auth */
#ifdef AUTHGRAPH
#include "authgraph.h"
#endif
/* -------------------------- */

/* Version */
extern const char VER_FIRMWARE[];
extern const char VER_DATE[];
extern const char HW_VERSION[];
extern const char VER_BUILD[];
extern const unsigned char __CAMEO_LINUX_VERSION__[];
extern const unsigned char __CAMEO_LINUX_BUILD__[];
extern const unsigned char __CAMEO_LINUX_BUILD_DATE__[];
extern const unsigned char __CAMEO_ATHEROS_VERSION__[];
extern const unsigned char __CAMEO_ATHEROS_BUILD__[];
extern const unsigned char __CAMEO_ATHEROS_BUILD_DATE__[];
extern const unsigned char __CAMEO_WIRELESS_VERSION__[];
extern const unsigned char __CAMEO_WIRELESS_BUILD__[];
extern const unsigned char __CAMEO_WIRELESS_DATE_[];
/* Certification file for SNMP*/
char certification_ptr[256];

extern char admin_userid[];
extern char admin_passwd[];
extern char user_userid[];
extern char user_passwd[];
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
extern sp_admin_userid[];
extern sp_admin_passwd[];
#endif

#ifdef HTTPD_USED_MUTIL_AUTH
extern int auth_login_find(char *mac_addr);
extern int auth_login_search(void);
extern int auth_login_search_long(void);
#endif
/*
*   Date: 2009-5-26
*   Name: Vic Chang
*   Reason:Fixed WLX-0191 FW upgrade issue.
*   Notice :
*/
int read_fw_success=0;


/*
*   Date: 2009-7-23
*   Name: Nick Chou
*   Reason:
        If user login fail at login.asp,
GUI redirect to login.asp again and show alert messages.
( GUI originally redirect to login_fail.asp or login_auth_fail.asp )
If you want to use this feature,
you also need to modify GUI to show alert messages.
    gui_loginfail_alert=1 => User Name or Password fail
    gui_loginfail_alert=2 => Graphical authentication fail
    abortive login_alert
*/
#ifdef GUI_LOGIN_ALERT
int gui_loginfail_alert=0;
#endif


/* Set system time by GUI while no NTP */
#ifdef AP_NOWAN_HASBRIDGE
int set_time_by_gui_flag = 0;
int do_only_once = 0;
#endif

/* Upgrade Paramter */
int firmware_upgrade        = 0;
int configuration_upload    = 0;
int restore_flag        = 1;
/* for reset bytes*/
int reset_lan_tx_flag           = 0;
int reset_wan_tx_flag           = 0;
int reset_wlan_tx_flag           = 0;
int reset_lan_rx_flag           = 0;
int reset_wan_rx_flag           = 0;
int reset_wlan_rx_flag           = 0;

/* D-Link Router Web-based Setup Wizard Specification */
int skip_auth = 0;

static int pppoe_connect(int pppoe_unit);
static void ipv6_pppoe_connect(void);
static void ipv6_pppoe_disconnect(void);

#ifdef IPV6_SUPPORT
static void do_wizard_autodetect_start_cgi(void);
static void do_wizard_autodetect_stop_cgi(void);
#endif

#define READ_BUF                    64 * 1024

#define HWID_LEN                    24
#define KERNEL_HWID_LEN             24
/*   Date: 2009-2-18
*   Name: Ken Chiang
*   Reason: To use different hardware country ID for US and other regions for DIR-615_C1.
*   Notice :
*/
#ifdef HWCOUNTRYID
#define HWCOUNTRYID_LEN             2
#endif

#define ARRAYSIZE(a)            (sizeof(a)/sizeof(a[0]))

enum {
    NOTHING,
    REBOOT,
    RESTART,
    RESTORE,
};

static int ret_code;
char post_buf[POST_BUF_SIZE];

struct hsearch_data vtab;
struct variable {
    char *name;
    char *longname;
    char *prefix;
    void (*validate)(webs_t wp, char *value, struct variable *v, char *);
    char **argv;
    int nullok;
    int ezc_flags;
};


struct variable variables[] = {
    /* basic settings */
    { "lan_ipaddr", "IP Address", 0, 0, 0, 0, 0 }
};

static const char * const redirecter_footer =
"<html>"
"<head>"
"<script>"
"function redirect(){"
"   document.location.href = '%s';"
"}"
"</script>"
"</head>"
"<script>"
"   redirect();"
"</script>"
"</html>"
;

static const char * const chklst_html =
"<html>"
"<head>"
"<title>Wireless Router : Check list</title>"
"<style type=\"text/css\">"
"<!--"
".style1 {font-family: Arial, Helvetica, sans-serif}"
"-->"
"</style>"
"</head>"
"<body>"
"<h4><span class=\"style1\">%s</span></h4>"
"</body>"
"</html>"
;

static const char * const ip_error_html =
"<html>"
"<head>"
"<title>Errror Page</title>"
"<style type=\"text/css\">"
"<!--"
".style1 {font-family: Arial, Helvetica, sans-serif}"
"-->"
"</style>"
"</head>"
"<body>"
"<h3><span class=\"style1\">%s</span></h3>"
"</body>"
"</html>"
;

static const char * wirelsee_driver_update_html =
"<html><body>"
"<form action=\"wireless_driver_update.cgi\" enctype=\"multipart/form-data\" method=post>"
"<input type=\"hidden\" name=\"html_response_return_page\" value=\"%s\">"
"<td>Update Wireless Driver Path : </td>"
"<td><input type=file name=file></td>"
"<td><input type=\"submit\" value=\"Upload\"></td>"
"</form>"
"</body></html>";

static const char * wlan_start_html =
"<html>"
"<head>"
"<title>Wireless Router</title>"
"</head>"
"<body>"
"<p>";

static const char * wlan_end_html =
"</body>"
"</html>";

char no_cache[] =
"Cache-Control: no-cache\r\n"
"Pragma: no-cache\r\n"
"Expires: 0"
;

char cached[] =
"Cache-Control: max-age=300"
;

/*  Date:   2009-05-06
 *  Name:   Yufa Huang
 *  Reason: Modify header for HTTPS.
 */
char download_nvram[] =
"Cache-Control: no-cache\r\n"
"Expires: 0\r\n"
"Content-Type: application/octet-stream\r\n"
"Content-Disposition: attachment ; filename=config.bin"
;

char log_to_hd[] =
"Cache-Control: no-cache\r\n"
"Expires: 0\r\n"
"Content-Type: application/octet-stream\r\n"
"Content-Disposition: attachment ; filename=messages.txt"
;

struct ej_handler ej_handlers[] = {
    { "CmoGetCfg", ej_cmo_get_cfg },
    { "CmoGetStatus", ej_cmo_get_status},
    { "CmoGetList", ej_cmo_get_list},
    { "CmoExecCgi", ej_cmo_exec_cgi},
    { "CmoGetLog", ej_cmo_get_log}
};

/* 0 : wireless
   1 : all */
static int reboot_type_flag = 0;

static char *core_itoa(int n)
{
    static char s[5];
    sprintf(s,"%d", n);
    return(s);
}

int checkServiceAlivebyName(char *service)
{
    FILE * fp;
    char buffer[1024] = {};
    if((fp = popen("ps -ax", "r"))){
        while( fgets(buffer, sizeof(buffer), fp) != NULL ) {
            if(strstr(buffer, service)) {
                pclose(fp);
                return 1;
            }
        }
        pclose(fp);
    }
    return 0;
}

/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
static void check_http_url_request(char *http_url,FILE *http_stream){
	assert(http_url);
	assert(http_stream);
}
#endif

/* message of count down page */
static char *show_message(char *response_message,int type,int status){
    char *s_message = " Success, system rebooting. ";
    char *f_message = " Fail, system rebooting. ";
    char *e_message = " Fail, firmware size error. ";
    char *i_message = " Success, uploading finished. ";
    char *c_message = " Fail, configuration file error. ";
    char *t_message = " The setting is saved. ";

    switch(type){
        case F_UPLOAD:
            response_message = s_message;
            break;
        case F_REBOOT:
            response_message = s_message;
            break;
        case F_UPGRADE:
            if(status == FIRMWARE_UPGRADE_SUCCESS || status == FIRMWARE_UPGRADE_LOADER_SUCCESS
#ifdef CONFIG_LP
				|| status == LP_UPGRADE_SUCCESS
#endif
)
                response_message = s_message;
            else if(status == FIRMWARE_UPGRADE_FAIL)
                response_message = f_message;
            else if(status == FIRMWARE_SIZE_ERROR)
                response_message = e_message;
#ifdef CONFIG_LP
            else if(status == LP_UPGRADE_FAIL)
            	response_message = " LP_Fail, LP size error. ";;
#endif
            break;
        case F_RESTORE:
            response_message = s_message;
            break;
        case F_INITIAL:
            if(status == 0)
                response_message = c_message;
            else
                response_message = i_message;
            break;
        case F_RC_RESTART:
            response_message = t_message;
            break;
    }
    return response_message;
}

static void rc_action_send_signal(char *reboot_type)
{
    //DEBUG_MSG("rc_action reboot_type=%s\n",reboot_type);
    if(strcmp(reboot_type, "all") ==0 )
        kill(read_pid(RC_PID), SIGHUP);
    if(strcmp(reboot_type, "lan") ==0 )
        kill(read_pid(RC_PID), SIGQUIT);
    if(strcmp(reboot_type, "wan") ==0 )
        kill(read_pid(RC_PID), SIGUSR1);
    if(strcmp(reboot_type, "wireless") ==0 )
        kill(read_pid(RC_PID), SIGTSTP);
    if(strcmp(reboot_type, "application") ==0 )
        kill(read_pid(RC_PID), SIGTERM);
    if(strcmp(reboot_type, "wlanapp") ==0 )
        kill(read_pid(RC_PID), SIGILL);
    if(strcmp(reboot_type, "filter") ==0 ){
        /*albert 080307 wait 1 sec for rc update nvram value*/  
	sleep(1);  
        kill(read_pid(RC_PID), SIGSYS);
    }
      //if(strcmp(reboot_type, "shutdown") ==0 ){
      //    sys_reboot();
      //}
#ifdef CONFIG_USER_TC
    if(strcmp(reboot_type, "qos") ==0 )
        kill(read_pid(RC_PID), SIGTTIN);
#endif
}

void rc_action(char *reboot_type)
{
    int send_signal_done = -1;
    FILE *fp = NULL;
    char data[2] = {0};

    /* Before qos sends signal to rc, we have to make sure rc is NOT restarting NOW.
     * If rc flag is set to busy, qos will wait for 3 seconds and query rc again.
     * Waiting interval prevents qos from consuming CPU resource.
     * If sleep_count > 10 times, qos will break the loop.
     */
    while(send_signal_done < 1)
    {
#ifdef CONFIG_USER_TC
        if(strcmp(reboot_type, "qos") ==0 )
        {
            system("killall -9 tracegw");
            _system("killall mbandwidth");
        }
#endif
        fp = fopen(RC_FLAG_FILE,"r");
        if(fp)
        {
            fread(data, 1, sizeof(data), fp);
            if(!strncmp(data, "i", 1))
            {
                printf("httpd SENT restart signal to rc\n");
                rc_action_send_signal(reboot_type);
                send_signal_done = 1;
            }
            else if (send_signal_done < 0)
            {
                printf("httpd is WAITING for rc to be idle\n");
                send_signal_done = 0;
                sleep(3);
            }
            else
                sleep(3);
            fclose(fp);
        }
        else
        {
            printf("httpd open %s failed\n",RC_FLAG_FILE);
            send_signal_done = 1;
        }
    }
}

/* parse the countdown page */
static char *count_down_parse(FILE *page, char *asp_tmp, int type, int ret)
{
    const char *page_fct = "<% CmoGetCfg(\"html_response_return_page\",\"none\"); %>";
    const char *count_fct = "<% CmoGetCfg(\"countdown_time\",\"none\"); %>";
    const char *message_fct = "<% CmoGetCfg(\"html_response_message\",\"none\"); %>";
    const char *user_fct = "<% CmoGetStatus(\"get_current_user\"); %>";
    const char *ver_fct = "<% CmoGetStatus(\"version\"); %>";
    const char *ver_refwsuc = "<% CmoGetStatus(\"read_fw_success\"); %>";
    const char *hwver_fct = "<% CmoGetStatus(\"hw_version\"); %>";
    const char *title_fct = "<% CmoGetStatus(\"title\"); %>";
    const char *copyright_fct = "<% CmoGetStatus(\"copyright\"); %>";
    const char *lingual_fct = "<% CmoGetCfg(\"lingual\",\"none\"); %>";
    const char *lanip_fct = "<% CmoGetCfg(\"lan_ipaddr\",\"none\"); %>";
    const char *model_fct = "<% CmoGetCfg(\"model_number\",\"none\"); %>";

#ifdef DIR865
    const char *language_fct = "<% CmoGetStatus(\"language\"); %>";
#endif

#ifdef CONFIG_BRIDGE_MODE
#ifdef AUTO_MODE_SELECT
        const char *auto_mode_select_fct = "<% CmoGetCfg(\"auto_mode_select\",\"none\"); %>";
#endif
        const char *wlan0_mode_fct = "<% CmoGetCfg(\"wlan0_mode\",\"none\"); %>";
        const char *wanstip_fct = "<% CmoGetCfg(\"wan_static_ipaddr\",\"none\"); %>";
#endif

    char line[256]={}, line_tmp[256]={} ,response_message[100]={};
    char *line_fct_start = NULL;
    int index;

    DEBUG_MSG("count_down_parse: type = %d, ret = %d\n", type, ret);

    while (fgets(line, 256, page)) {
        //DEBUG_MSG("count_down_parse: line=%s\n",line);
        memset(line_tmp, 0, 256);

        /* parse "html_response_return_page" value */
        if ((line_fct_start = strstr(line, page_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: html_response_return_page\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("html_response_return_page"));
            strcat(line_tmp, line_fct_start + strlen(page_fct));
        }
        /* parse "response message */
        else if ((line_fct_start = strstr(line, message_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: html_response_message\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, show_message(response_message, type, ret));
            strcat(line_tmp, line_fct_start + strlen(message_fct));
        }
        /* parse "countdown_time" value */
        else if ((line_fct_start = strstr(line, count_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: countdown_time\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("countdown_time"));
            strcat(line_tmp, line_fct_start + strlen(count_fct));
        }
        /* parse "model_number" value */
        else if ((line_fct_start = strstr(line, model_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: model_number\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("model_number"));
            strcat(line_tmp, line_fct_start + strlen(model_fct));
        }
        /* parse "copyright" value */
        else if ((line_fct_start = strstr(line, copyright_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: copyright\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, WEB_COPYRIGHT);
            strcat(line_tmp, line_fct_start + strlen(copyright_fct));
        }
        /* parse "title" value */
        else if ((line_fct_start = strstr(line, title_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: title\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, WEB_TITLE);
            strcat(line_tmp, line_fct_start + strlen(title_fct));
        }
        /* parse "lingual" value */
        else if ((line_fct_start = strstr(line, lingual_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: lingual \n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("lingual"));
            strcat(line_tmp, line_fct_start + strlen(lingual_fct));
        }
        /* parse "hw_version" value */
        else if ((line_fct_start = strstr(line, hwver_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: hw_version\n");
            strncpy(line_tmp, line, line_fct_start - line);

#ifdef SET_GET_FROM_ARTBLOCK
            if (artblock_get("hw_version"))
            	strcat(line_tmp, artblock_get("hw_version"));
            else
#endif
            	strcat(line_tmp, HW_VERSION);
            strcat(line_tmp, line_fct_start + strlen(hwver_fct));
        }

#ifdef DIR865
        /* parse "language" value */
        else if ((line_fct_start = strstr(line, language_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: language\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("language"));
            strcat(line_tmp, line_fct_start + strlen(lingual_fct));
        }
#endif//#ifdef DIR865

        /* parse "current_user" value */
        else if ((line_fct_start = strstr(line, user_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: current_user\n");
            strncpy(line_tmp, line, line_fct_start - line);

#ifdef HTTPD_USED_MUTIL_AUTH
            DEBUG_MSG("count_down_parse: MUTIL_AUTH\n");
            if ((index = auth_login_find(&con_info.mac_addr[0])) >= 0) {
                DEBUG_MSG("count_down_parse: index=%d, curr_user=%s\n", index, auth_login[index].curr_user);
                strcat(line_tmp, auth_login[index].curr_user);
            }
            else {
                DEBUG_MSG("count_down_parse: no found\n");
                strcat(line_tmp, "admin");
            }
#else
            strcat(line_tmp, auth_login.curr_user);
#endif
            strcat(line_tmp, line_fct_start + strlen(user_fct));
        }
        /* parse "version" value */
        else if ((line_fct_start = strstr(line, ver_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: version\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, VER_FIRMWARE);
            strcat(line_tmp, line_fct_start + strlen(ver_fct));
        }
        /* parse "lan_ipaddr" value */
        else if ((line_fct_start = strstr(line, lanip_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: lan_ipaddr\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("lan_ipaddr"));
            strcat(line_tmp, line_fct_start + strlen(lanip_fct));
        }
        /* parse "read_fw_success" value */
        else if ((line_fct_start = strstr(line, ver_refwsuc)) != NULL)
        {
            DEBUG_MSG("count_down_parse: read_fw_success\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, read_fw_success);
            strcat(line_tmp, line_fct_start + strlen(ver_refwsuc));
        }

/*  Date: 2009-04-30
    *   Name: Ken Chiang
    *   Reason: Add support for enable auto mode select(router mode/ap mode) and modify for can support ap mode only.
    *   Note:
*/
#ifdef CONFIG_BRIDGE_MODE
#ifdef AUTO_MODE_SELECT
        /* parse "auto_mode_select" value */
        else if ((line_fct_start = strstr(line, auto_mode_select_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: auto_mode_select\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("auto_mode_select"));
            strcat(line_tmp, line_fct_start + strlen(auto_mode_select_fct));
        }
#endif

        /* parse "wlan0_mode" value */
        else if ((line_fct_start = strstr(line, wlan0_mode_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: wlan0_mode\n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("wlan0_mode"));
            strcat(line_tmp, line_fct_start + strlen(wlan0_mode_fct));
        }
        /* parse "wan_static_ipaddr" value */
        else if ((line_fct_start = strstr(line, wanstip_fct)) != NULL)
        {
            DEBUG_MSG("count_down_parse: \n");
            strncpy(line_tmp, line, line_fct_start - line);
            strcat(line_tmp, nvram_safe_get("wan_static_ipaddr"));
            strcat(line_tmp, line_fct_start + strlen(wanstip_fct));
        }
#endif

        if (strlen(line_tmp))
        	sprintf(asp_tmp, "%s%s", asp_tmp, line_tmp);
        else
            sprintf(asp_tmp, "%s%s", asp_tmp, line);
    }

    return asp_tmp;
}


/* set the countdown time and redirect to the countdown page*/
int redirect_count_down_page(FILE *stream,int type,char *count_down_sec,int ret_s){
    int ret = 0,index;
    char *tmp = NULL;
    char asp_path[30]={};
    char *response_page = NULL;

    DEBUG_MSG("redirect_count_down_page: type = %d, ret_s = %d\n", type, ret_s);

    if (skip_auth) {
        goto no_auth;	
    }
    
    /* check if admin && set countdown_time to nvram*/
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("redirect_count_down_page: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 ||
            nvram_match("sp_admin_username", auth_login[index].curr_user) ==0 )

#else
    if( nvram_match("admin_username", auth_login.curr_user) ==0 ||
        nvram_match("sp_admin_username", auth_login.curr_user) ==0 )
#endif
#else//HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("redirect_count_down_page: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0)
#else
    if(nvram_match("admin_username", auth_login.curr_user) ==0)
#endif
#endif//HTTPD_USED_SP_ADMINID
    {
#ifdef HTTPD_USED_MUTIL_AUTH
        DEBUG_MSG("count_down_parse: index=%d\n",index);
        DEBUG_MSG("count_down_parse: curr_user=%s\n",auth_login[index].curr_user);
#endif
        DEBUG_MSG("redirect_count_down_page: count_down_sec = %s\n", count_down_sec);
        /* save html_xxx_page to nvram for "reboot" and "restore" */
        if(type == F_REBOOT || type == F_RESTORE)   {
            set_basic_api(stream);
        }
        /* set count time to nvram*/
        nvram_set("countdown_time",count_down_sec);
        nvram_commit();
    }

no_auth:
    /* check the count down page */
    response_page = nvram_get("html_response_page");
    if(response_page == NULL && type < F_TYPE_COUNT){
        response_page = "count_down.asp";
    }
#ifdef PURE_NETWORK_ENABLE
    else if(response_page == NULL && type > 4){
        response_page = nvram_safe_get("pure_reboot_page");
    }
#endif

    sprintf(asp_path,"%s%s","/www/",response_page);
    FILE *fp = fopen(asp_path,"r");
    if(fp == NULL)
        return 0;

    /* NULL for parse error */
    tmp = (char *)malloc(10000);
    if (!tmp)
    {
        fclose(fp);
        return 0;
    }
    memset(tmp,0,10000);
    DEBUG_MSG("redirect_count_down_page: asp_path = %s\n", asp_path);
    tmp = count_down_parse(fp,tmp,type,ret_s);
    if(tmp == NULL || strlen(tmp) == 0)
    {
        DEBUG_MSG("redirect_count_down_page can not alloc memory\n");
        ret = 0;
    }
    else
        ret = 1;

/*  Date:   2009-04-09
 *  Name:   Yufa Huang
 *  Reason: Fixed: Web page can not send not completely.
 */
    wfputs(tmp, stream);
    fclose(fp);
    free(tmp);

    return ret;
}

/* parse the upload file header to get the information of
   html_respinse_page and html_response_return_page */
static int parse_upload_header(int *count_down, int *page_type, char *buf){
    char *tmp = NULL;
    char ret[40] = {};

    /* if count = 1 , then buf = "\n" , ignore it and return for next fget */
    if(*count_down > 0){
        (*count_down)--;
        return 0;
    }
    /* take off the space and "\n" */
    tmp = &buf[0];
    strncpy(ret,tmp,strlen(tmp)-2);
    /* save value to nvram */
    switch(*page_type){
        case HTML_RESPONSE_PAGE:
            nvram_set("html_response_page",ret);
            break;
        case HTML_RETURN_PAGE:
            nvram_set("html_response_return_page",ret);
            break;
        default:
            break;
    }
    /* initial page type */
    *page_type = NO_HTML_PAGE;
    return 1;
}

/*
*   Date: 2009-08-10
*   Name: Ken Chiang
*   Reason: Modify for GPL CmoCgi.
*   Notice :
*/
/*
static int save_html_page_to_nvram(webs_t wp)
*/
int save_html_page_to_nvram(webs_t wp)
{
    char *html_response_page=NULL;
    char *html_response_message = NULL;
    char *html_response_return_page = NULL;

    html_response_page = websGetVar(wp, "html_response_page", "");
    html_response_message = websGetVar(wp, "html_response_message", "");
    html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    nvram_flag_set();
    nvram_commit();
    return 0;
}


static void restore_default(void)
{
    char cmd[40];
    memset(cmd,0,sizeof(cmd));
    fprintf(stderr,"restore_default\n");
    sprintf(cmd,"nvram restore_default &" );
    fprintf(stderr,"%s",cmd);
    _system(cmd);
    restore_flag = 0;
    sys_reboot();
}


static void restore_wireless_setting(void)
{
    FILE *fp_src_1, *fp_src_2, *fp_dst;
    char buf_src_1[1024]={0},buf_src_2[1024]={0};
    char cmd[128]={0};

    fp_src_1 = fopen(NVRAM_DEFAULT, "r"); //"/var/etc/nvram.default"
    fp_src_2 = fopen(NVRAM_CONF, "r"); // "/var/etc/nvram.conf"
    fp_dst = fopen(NVRAM_TMP, "a");// "/var/etc/nvram.tmp"

    if ((fp_src_1 == NULL)|| (fp_src_2 == NULL)||(fp_dst == NULL))
    {
        printf("restore_wireless_setting : file open failed\n");
        return;
    }

    /* Assume the sequence of items in nvram.conf and nvram.default is the same */
    while(fgets(buf_src_1, 1024, fp_src_1) && fgets(buf_src_2, 1024, fp_src_2))
    {
        /* If the item is wlan0_domain, we use the value in nvram.conf */
        if( !strcmp(buf_src_1, "wlan0_domain") )
        {
            fwrite(buf_src_2, strlen(buf_src_2), 1, fp_dst);
        }
/*
*   Date: 2009-5-20
*   Name: Ken_Chiang
*   Reason: modify when we do reset to unconfigured the wps_pin and wps_default_pin is kepp it
*   to fixed the wps_pin and wps_default_pin is changed to 00000000 when we do reset to unconfigured.
*   Notice :
*/
// If the item is wlan0_XXX or wps_XXX we use the value in nvram.default but wps_pin and wps_default_pin is kepp it
#ifdef USER_WL_ATH_5GHZ //AG Concurrent
        else if( strstr(buf_src_1, "wlan0")||strstr(buf_src_1, "wlan1")||strstr(buf_src_1, "wps") )
#else
        else if( strstr(buf_src_1, "wlan0")||strstr(buf_src_1, "wps") )
#endif
        {
            if( strstr(buf_src_1, "wps_pin") || strstr(buf_src_1, "wps_default_pin") ){
                printf("%s: wps_pin or wps_default_pin =%s\n",__FUNCTION__,buf_src_2);
                fwrite(buf_src_2, strlen(buf_src_2), 1, fp_dst);
            }
            /*  Date:   2009-07-03
            *   Name:   jimmy huang
            *   Reason: Make restore wireless setting not reload
            *           "wlan0_enable" , "wps_enable"
            *   Note:   This because maybe some project's
            *           default wireless or wps is not enable
            */
            else if( strstr(buf_src_1,"wlan0_enable")
                        || strstr(buf_src_1,"wps_enable")
#ifdef USER_WL_ATH_5GHZ //AG Concurrent
                        || strstr(buf_src_1,"wlan1_enable")
#endif
                    )
            {
                fwrite(buf_src_2, strlen(buf_src_2), 1, fp_dst);
            }
            else{
                fwrite(buf_src_1, strlen(buf_src_1), 1, fp_dst);
            }
        }
        /* Else, we use the value in nvram.conf */
        else
        {
            fwrite(buf_src_2, strlen(buf_src_2), 1, fp_dst);
        }
    }

    fclose(fp_src_1);
    fclose(fp_src_2);
    fclose(fp_dst);

    unlink(NVRAM_CONF);

    sprintf(cmd,"mv %s %s",NVRAM_TMP,NVRAM_CONF);
    _system(cmd);
	nvram_commit();

//  _system("killall miniupnpd");
	rc_action("wireless");
//  _system("miniupnpd &");//ask miniupnpd sleep 10 secs before initializing upnp stack
}

#ifdef IPv6_TEST
static void restore_ipv6_setting()
{

    FILE *fp_src_1, *fp_src_2, *fp_dst;
    char buf_src_1[1024]={0},buf_src_2[1024]={0};
    char cmd[128]={0};

    fp_src_1 = fopen(NVRAM_DEFAULT, "r");
    fp_src_2 = fopen(NVRAM_CONF, "r");
    fp_dst = fopen(NVRAM_TMP, "a");

    if ((fp_src_1 == NULL)|| (fp_src_2 == NULL)||(fp_dst == NULL))
    {
        printf("restore_ipv6_setting : file open failed\n");
        return;
    }

    /* Assume the sequence of items in nvram.conf and nvram.default is the same */
    while(fgets(buf_src_1, 1024, fp_src_1) && fgets(buf_src_2, 1024, fp_src_2))
    {
        /* If the item is wlan0_XXX or wps_XXX, we use the value in nvram.default */
        if(strstr(buf_src_1, "ipv6_ra_"))
        {
            fwrite(buf_src_1, strlen(buf_src_1), 1, fp_dst);
        }
        /* Else, we use the value in nvram.conf */
        else
        {
            fwrite(buf_src_2, strlen(buf_src_2), 1, fp_dst);
        }
    }

    fclose(fp_src_1);
    fclose(fp_src_2);
    fclose(fp_dst);

    unlink(NVRAM_CONF);

    sprintf(cmd,"mv %s %s",NVRAM_TMP,NVRAM_CONF);
    _system(cmd);

    nvram_commit();
    rc_action("application");
}
#endif //#ifdef IPv6_TEST

#define TMPFS

/* System MTD Table */
static int sys_upgrade(char *url, FILE *stream, int *total)
{
    int  ret = FIRMWARE_UPGRADE_FAIL;
    int  count = 0, size_error = 0;
    char *read_buffer = NULL;
    char *file_buffer = NULL;
    int  image_length = 0;
    char imageHWID[HWID_LEN+2];
    char *p_hwid;
    int  i = 0;
    FILE *fp;
    int upgrade_bootloader=0;
    int free_memory_low = 1;

#ifdef HWCOUNTRYID
    char imageHWCOUNTRYID[HWCOUNTRYID_LEN+2] = {0};
    char *p_hwcountryid ;
#endif

    DEBUG_MSG("Image size are %02d\n", *total);

//#ifndef DIR865
#if 0
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char *free_mem, buf[32] = {0};
        while (!(ferror(stream) || feof(stream))) {
            fgets(buf, 32, fp);

            if (strstr(buf, "MemFree:")) {
                /* file format :
                   MemTotal:          58072 kB
                   MemFree:           22204 kB */
                free_mem = buf + 10;
                free_mem[strlen(free_mem) - 3] = 0;

                /* free memory size if large than 15M ? */
                DEBUG_MSG("free mem_size = %d\n", atol(free_mem));
                if (atol(free_mem) > 15360)
                    free_memory_low = 0;
                else
                    free_memory_low = 1;

                break;
            }
        }
        fclose(fp);
    }

    if (free_memory_low) {
        /* We must get more free memory when free memory size is low,
           so must send STOP signal to RC and wait RC finished. */
        printf("sys_upgrade: free_memory_low killall all apps \n");
        kill(read_pid(RC_PID), SIGINT);
        i = 0;
        while (++i < 10)
        {
            char data[2] = {0};

            fp = fopen(RC_FLAG_FILE, "r");
            if (fp) {
                printf("upgrade is waiting for rc to be idle.\n");
                fread(data, 1, 1, fp);
                fclose(fp);

                if (data[0] == 'i') {
                    break;
                }
            }
            sleep(3);
        }
    }
#endif

    read_buffer = (char *) malloc(READ_BUF);
    if (!read_buffer) {
        printf("sys_upgrade: malloc READ_BUF fail\n");
        goto _upgrade_fail;
    }

    /* check firmware size */
    if ((*total > IMG_LOWER_BOUND) && (*total < IMG_UPPER_BOUND)) {
        printf("IMG_UPPER_BOUND size %d,IMG_LOWER_BOUND size %d\n", IMG_UPPER_BOUND, IMG_LOWER_BOUND);
        printf("Upgrade Image size are %02d\n", *total);
        upgrade_bootloader = 0;
    }
    else if ((*total < BOOT_IMG_UPPER_BOUND) && ((*total > BOOT_IMG_LOWER_BOUND))) {
        printf("BOOT_IMG_UPPER_BOUND size %d,IMG_LOWER_BOUND size %d\n", BOOT_IMG_UPPER_BOUND, BOOT_IMG_LOWER_BOUND);
        printf("Upgrade BootLoader, File size %d\n", *total);
        upgrade_bootloader = 1;
    }
    else {
        printf("File size %d  > IMG_UPPER_BOUND size %d   or    < IMG_LOWER_BOUND size %d\n", *total, IMG_UPPER_BOUND, IMG_LOWER_BOUND);
        printf("File size %d  > BOOT_IMG_UPPER_BOUND size %d   or    < IMG_LOWER_BOUND size %d\n", *total, BOOT_IMG_UPPER_BOUND, BOOT_IMG_LOWER_BOUND);
        ret = FIRMWARE_SIZE_ERROR;
        size_error = 1;
    }

    DEBUG_MSG("sys_upgrade: check firmware size finished\n");

    fp = fopen(FW_UPGRADE_FILE, "w+");
    if (!fp) {
        printf("Open file %s failed\n", FW_UPGRADE_FILE);
        goto _upgrade_fail;
    }

    /* Read from HTTP file stream */
    fprintf(stderr, "start read http stream\n");
    while (*total > 0)
    {
        if (*total < READ_BUF) {
            count = safe_fread(read_buffer, 1, *total, stream);
            break;
        }

        count = safe_fread(read_buffer, 1, READ_BUF, stream);
        *total -= count;
        DEBUG_MSG("Read data is %02d, Total Reminder is %02d\n", count, *total);

        /* if size error, then just receive data and drop them */
        if (!size_error) {
            safe_fwrite(read_buffer, 1, count, fp);
            image_length = image_length + count;
            DEBUG_MSG("image_length %02d\n", image_length);
        }

        /* if receive the last data then break the loop */
        if (*total < READ_BUF) {
            count = safe_fread(read_buffer, 1, *total, stream);
            DEBUG_MSG("last data size %02d < MAX_BUF (%02d) and count is %02d \n", *total, READ_BUF, count);
            *total -= count;

            /* Copy to File Buffer */
            if (!size_error) {
				if(upgrade_bootloader ==1){
					count =  count - 49;
            		DEBUG_MSG("last data size %02d and  count is %02d \n", *total, count);
            	}
            	else{
                	char *end_ptr = strstr(read_buffer, "\n");
                	if (end_ptr) {
                    	count = end_ptr - read_buffer;
                    	DEBUG_MSG("end_ptr last data size %02d and  count is %02d \n", *total, count);
                	}
                }

                if (count > 0) {
                    safe_fwrite(read_buffer, 1, count, fp);
                }

                image_length = image_length + count;
                file_buffer = read_buffer + count;
                printf("image_length %02d\n", image_length);
                printf("file_buffer %x\n", file_buffer);
            }

            if (!*total && (ferror(stream) || feof(stream)))
                break;
        }
    }

    DEBUG_MSG("Image length is %d\n", image_length);

    /* return error after drop the receiveing data */
    if (size_error) {
        printf("sys_upgrade: return error after drop the receiveing data\n");
        goto _upgrade_fail;
    }

    /* Hardware ID Check */
    ret = FIRMWARE_UPGRADE_FAIL;
    count = 0;
    printf("Hardware ID check begin \n");

    for (i = 0; i < 24; i++)
    {
        if (upgrade_bootloader == 1) {
            printf("BOOT Hardware ID check begin \n");
            p_hwid = file_buffer - 20 - count;
            DEBUG_MSG("file_buffer now address 0x%x, image_lenght is %02d\n, Hardware ID is %s\n",file_buffer,image_length,p_hwid);
        }
        else {
            printf("Run time Hardware ID check begin \n");
#ifdef HWCOUNTRYID
            p_hwid = file_buffer - 20 - count;
            p_hwcountryid = file_buffer - 20 - count - HWCOUNTRYID_LEN;
            DEBUG_MSG("file_buffer now address 0x%x, image_lenght is %02d\n, Hardware ID address 0x%x, Hardware Country ID address 0x%x\n",file_buffer,image_length,p_hwid,p_hwcountryid);

#else
            p_hwid = file_buffer - 20 - count;
            DEBUG_MSG("file_buffer now address 0x%x, image_lenght is %02d\n, Hardware ID address 0x%x\n",file_buffer,image_length,p_hwid);
#endif
        }

#ifdef HWCOUNTRYID
        memcpy(imageHWID, p_hwid, HWID_LEN);
        imageHWID[HWID_LEN] = '\0';
        DEBUG_MSG("Image Hardware ID is %s, System Hardware ID is %s\n", imageHWID, HWID);

        if (upgrade_bootloader != 1) {
            //BootLoader don't must check HWCOUNTRYID
            memcpy(imageHWCOUNTRYID, p_hwcountryid, HWCOUNTRYID_LEN);
            imageHWCOUNTRYID[HWCOUNTRYID_LEN] = '\0';
            DEBUG_MSG("Image Hardware Country ID is %s\n", imageHWCOUNTRYID);
            DEBUG_MSG("System Hardware Country ID is %s\n", HWCOUNTRYID);
        }
#else
        memcpy(imageHWID, p_hwid, HWID_LEN);
        imageHWID[HWID_LEN] = '\0';
        DEBUG_MSG("Image Hardware ID is %s, len=%d\n", imageHWID, image_length);
        DEBUG_MSG("System Hardware ID is %s\n",HWID);
#endif

        /* Hardware ID Check */
        if (image_length > IMG_LOWER_BOUND ) {
        	/* Run time */
            if (!strncmp(imageHWID, HWID,KERNEL_HWID_LEN)) {
#ifdef HWCOUNTRYID
                printf("Hardware ID match \n");
                if (!strncmp(imageHWCOUNTRYID, HWCOUNTRYID, HWCOUNTRYID_LEN)) {
                    ret = FIRMWARE_UPGRADE_SUCCESS;
                    firmware_upgrade = 1;
                    printf("Hardware ID and Hardware Country ID match \n");
                    break;
                }
                else
                    printf("Hardware Country ID don't match \n");
#else
                ret = FIRMWARE_UPGRADE_SUCCESS;
                firmware_upgrade = 1;
                printf("Hardware ID match \n");
                break;
#endif
            } else
                count++;
        }
        else if ((image_length < BOOT_IMG_UPPER_BOUND) && (image_length > BOOT_IMG_LOWER_BOUND)) {
            /* BootLoader */
            if (!strncmp(imageHWID, HWID, KERNEL_HWID_LEN)) {
                ret = FIRMWARE_UPGRADE_LOADER_SUCCESS;
                firmware_upgrade = 1;
                printf("BootLoader Hardware ID match \n");
                break;
            }
            else
            	count++;
        }
    }//end for

_upgrade_fail:
    DEBUG_MSG("sys_upgrade: free buffer\n");
    if (read_buffer)
        free(read_buffer);

    if (fp)
        fclose(fp);

    if (ret == FIRMWARE_UPGRADE_SUCCESS || ret ==FIRMWARE_UPGRADE_LOADER_SUCCESS) {
#if 0
        if (!free_memory_low) {
            printf("sys_upgrade: killall all apps \n");
	 		kill(read_pid(RC_PID), SIGINT);
        }
#endif
#ifdef CONFIG_USER_WISH
        system("/etc/init.d/stop_wish &");
#endif

        system("killall wantimer");
        if (upgrade_bootloader == 1)
            system("/var/sbin/fwupgrade boot &");
        else
            system("/var/sbin/fwupgrade fw &");
    }
    else {
//#ifndef DIR865
#if 0
        if (free_memory_low) {
            DEBUG_MSG("START RC\n");
            kill(read_pid(RC_PID), SIGUSR2);
        }
#endif
        remove(FW_UPGRADE_FILE);
    }

    return ret;
}

#ifdef NVRAM_CONF_VER_CTL/*configuration version control*/
int check_conf_version(void)
{
	char *conf_ver = NULL;
	char conf_ver_string[8] = {};
	char con_cv[8]={0}, cv[8]={0};
	char *con_cv_major=NULL, *cv_major=NULL;
	char *con_cv_minor=NULL, *cv_minor=NULL;
	int ret=0;
		
	conf_ver = get_value_from_upload_config(NVRAM_UPGRADE_TMP, "configuration_version", conf_ver_string);
	if(!conf_ver){
			printf("Do not support configuration version control in configuration file\n");
	}
	else{
		strcpy(con_cv, conf_ver);
		strcpy(cv, nvram_safe_get("configuration_version"));
		printf("configuration_version=%s in configuration file\n", con_cv);
		printf("configuration_version=%s in System\n", cv);
		con_cv_major = strtok(con_cv, ".");
		con_cv_minor = strtok(NULL, ".");
		cv_major = strtok(cv, ".");
		cv_minor = strtok(NULL, ".");
		
		/* major.minor
			1. Udate nvram deault value, item was already in NVRAM
			ex. change wlan0_ssid=a to wlan0_ssid=b in nvram.default
				=> major + 1
			2. Sytem has new nvram item and value => Add it into NVRAM
				=> minor + 1
		*/	
		
		if((con_cv_major != NULL) && (cv_major != NULL) && (con_cv_minor != NULL) && (cv_minor != NULL))
		{		
			if( (atoi(cv_minor) < atoi(con_cv_minor) ) //=> there are new items in confing.bin  
			){
				/*ignore new items in configuration file, Do not upgrade new items to system */
				printf("upload_configuration using Configuration Version Control\n", cv);
				ret = 1;
			}
		}	
	}
	
	return ret;	
}
#endif

/* Upload_configuration File */
static int upload_configuration(char *url, FILE *stream, int *total)
{
    char *read_buffer;
    int  ret = 0, count = 0;
    FILE *fp;

    /* Received configuration file via http protocol */
    DEBUG_MSG("configuration file size = %02d \n",*total);

    if(*total > (NVRAM_READ_BUF))
    {
        printf("configration size too big \n");
        read_buffer = (char *) malloc((*total));
        count = safe_fread(read_buffer, 1, *total, stream);
        free(read_buffer);
        ret = 0;
        configuration_upload = ret;
        return ret;
    }

    read_buffer = (char *) malloc((NVRAM_READ_BUF));
    if (!read_buffer)
        return ret;
    DEBUG_MSG("read file via http protocol\n");
    fp = fopen(NVRAM_UPGRADE_TMP, "w+");
    if (!fp) {
        free(read_buffer);
        return ret;
    }
    count = safe_fread(read_buffer, 1, *total, stream);
    count = safe_fwrite(read_buffer,1,*total,fp);
    DEBUG_MSG("Write to tmp file size = %02d\n",count);
    fclose(fp);
    free(read_buffer);

    /* check upload file checksum */
    char upload_config_string[NVRAM_READ_BUF] = {};     // max size : 32K
    char checksum_value_string[64] = {};
    char checksum_string[32] = {};
    unsigned long checksum = 0;
    char *upload_config = NULL;
    char *checksum_value = NULL;
    int conf_ver_ctl = 0;
    
    /* get checksum from upload configuration file */
    checksum_value = get_value_from_upload_config(NVRAM_UPGRADE_TMP,"config_checksum",checksum_value_string);
    if(!checksum_value){
        ret = 0;
        configuration_upload = ret;
        return ret;
    }

#ifdef NVRAM_CONF_VER_CTL/*configuration version control*/
	conf_ver_ctl = check_conf_version();
#endif
	
    /* init upload configuration with format : A=B\nC=D\nE=F\n..... */
	upload_config = upload_configuration_init(NVRAM_UPGRADE_TMP, upload_config_string, "config_checksum", conf_ver_ctl);
	
    if(!upload_config){
        ret = 0;
        configuration_upload = ret;
        return ret;
    }

    /* calculate the checksum according to upload configuration */
 	char md5_checksum[64];

	bzero(md5_checksum, sizeof(md5_checksum));
	in_cksum_ex(upload_config, strlen(upload_config), md5_checksum);

	if (strcmp(checksum_value, md5_checksum) != 0) {
		ret = 0;
		configuration_upload = ret;
		return 0;
	}

	/* remove config_checkum from NVRAM_UPGRADE_TMP */
	unlink(NVRAM_UPGRADE_TMP);
	fp = fopen(NVRAM_UPGRADE_TMP, "w+");
	if (fp == NULL)
		return 0;

	fwrite(upload_config_string , 1 , sizeof(upload_config_string) , fp);
	fclose(fp);

    /* Read default configuration file */
    _system("nvram upgrade &");

    ret = 1;
    configuration_upload = 1;

    return ret;
}


int variables_arraysize(void)
{
    return ARRAYSIZE(variables);
}

int hash_vtab(struct hsearch_data *tab,struct variable *vblock,int num_items)
{
    ENTRY e, *ep=NULL;
    int count;

    assert(tab);
    assert(vblock);

    if (!num_items) return -1;

    if (!hcreate_r(num_items,tab))
    {
        return -1;
    }


    for (count=0; count < num_items; count++)
    {
        e.key = vblock[count].name;
        e.data = &vblock[count];
        if (!hsearch_r(e, ENTER, &ep, tab))
            return -1;
    }

    return 0;

}

int internal_init(void)
{
    /* Initialize hash table */
    if (hash_vtab(&vtab,variables,variables_arraysize()))
        return -1;
    return 0;
}

/*
*   Date: 2009-08-10
*   Name: Ken Chiang
*   Reason: Modify for GPL CmoCgi.
*   Notice :
*/
/*
static char *absolute_path(char *page)
*/
char *absolute_path(char *page)
{
    static char absolute_path_t[500] = {};
    struct in_addr addr;
/*  Date:   2009-04-09
 *  Name:   Yufa Huang
 *  Reason: add HTTPS function.
 */
    char *url = "http://";

#if defined(HAVE_HTTPS)
    if (openssl_request == 1)
        url = "https://";
#endif

    addr.s_addr = con_info.ip_addr;
    DEBUG_MSG("absolute_path: addr.s_addr=%x\n",addr.s_addr);
    if (check_ip_match(inet_ntoa(addr), nvram_get("lan_ipaddr")) == 0)
    {
        DEBUG_MSG("absolute_path: addr.s_addr=%s==lan_ipaddr=%s\n",inet_ntoa(addr),nvram_get("lan_ipaddr"));
        if(get_apply_config_flag() == 1){
            sprintf(absolute_path_t,"%s%s%s%s",url,get_old_value_by_file(NVRAM_CONFIG_PATH,"lan_ipaddr"),"/",page);
            page = absolute_path_t;
        }
        else {
            sprintf(absolute_path_t,"%s%s%s%s",url,nvram_get("lan_ipaddr"),"/",page);
            page = absolute_path_t;
        }
    }
    DEBUG_MSG("absolute_path: page=%s\n",page);
    return page;
}

/* save value to nvram with commit rc action */
static int apply_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", nvram_safe_get("default_html"));
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", nvram_safe_get("default_html"));
    char *reboot_type = websGetVar(wp, "reboot_type", "all");
#ifdef AP_NOWAN_HASBRIDGE
    char *count_down_time = nvram_safe_get("countdown_time");
#endif
#ifdef IPv6_SUPPORT
    char *page_type = websGetVar(wp, "page_type", "normal");
    char *ipv6_wan_proto = NULL;
#endif
    int index;

	/*
	 * Reason: Prevent DUT from saving setting when web GUI time out.
	 * Modified: Yufa Huang
	 * Date: 2010.12.22
	 */
    if (strstr(url, "login.asp")) {
    	websWrite(wp, redirecter_footer, absolute_path("login.asp"));
    	return 1;
    }

    /* jimmy added for DIR-400 , don't check account name */
#ifndef AthSDK_AP61
    /* accound = user only */
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 ){
        DEBUG_MSG("apply_cgi: index=%d\n",index);
        DEBUG_MSG("apply_cgi: curr_user=%s\n",auth_login[index].curr_user);
        if(nvram_match("user_username", auth_login[index].curr_user) ==0)
        {
            DEBUG_MSG("apply_cgi: curr_user=%s==user_username\n",auth_login[index].curr_user);
            nvram_set("html_response_return_page",html_response_return_page);
            websWrite(wp,redirecter_footer, absolute_path("user_page.asp"));
            return 1;
        }
    }
#else
    if(nvram_match("user_username", auth_login.curr_user) ==0)
    {
        nvram_set("html_response_return_page",html_response_return_page);
        websWrite(wp,redirecter_footer, absolute_path("user_page.asp"));
        return 1;
    }
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
#endif

#ifdef IPv6_SUPPORT
    if(strcmp(page_type,"IPv6") == 0)
    {
        ipv6_wan_proto = websGetVar(wp, "ipv6_wan_proto", "ipv6_static");
        if(strncmp(ipv6_wan_proto,"ipv6_stateless",14)==0){
            system("echo 1 > /proc/sys/net/ipv6/conf/all/accept_ra");
            _system("echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra", nvram_safe_get("wan_eth"));
        }else{
            system("echo 0 > /proc/sys/net/ipv6/conf/all/accept_ra");
            _system("echo 0 > /proc/sys/net/ipv6/conf/%s/accept_ra", nvram_safe_get("wan_eth"));
        }

        if(nvram_match("ipv6_wan_proto","ipv6_static") == 0 && strlen(nvram_safe_get("ipv6_static_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_static_lan_ip"), nvram_safe_get("lan_bridge"));
        else if(nvram_match("ipv6_wan_proto","ipv6_pppoe") == 0 && strlen(nvram_safe_get("ipv6_pppoe_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_pppoe_lan_ip"), nvram_safe_get("lan_bridge"));
        else if(nvram_match("ipv6_wan_proto","ipv6_6to4") == 0 && strlen(nvram_safe_get("ipv6_6to4_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_6to4_lan_ip"), nvram_safe_get("lan_bridge"));
        else if(nvram_match("ipv6_wan_proto","ipv6_6in4") == 0 && strlen(nvram_safe_get("ipv6_6in4_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_6in4_lan_ip"), nvram_safe_get("lan_bridge"));
        else if(nvram_match("ipv6_wan_proto","ipv6_pppoe") == 0 && strlen(nvram_safe_get("ipv6_pppoe_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_pppoe_lan_ip"), nvram_safe_get("lan_bridge"));
        else if(nvram_match("ipv6_wan_proto","ipv6_dhcp") == 0 && strlen(nvram_safe_get("ipv6_dhcp_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_dhcp_lan_ip"), nvram_safe_get("lan_bridge"));
#ifdef IPV6_STATELESS_WAN
        else if(nvram_match("ipv6_wan_proto","ipv6_stateless") == 0 && \
                        strlen(nvram_safe_get("ipv6_stateless_lan_ip")) != 0){
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_stateless_lan_ip"), nvram_safe_get("lan_bridge"));
                }
#endif

    }
    else    //for 6to4 changed IP caused by IPv4 IP changed
    {
        if(nvram_match("ipv6_wan_proto","ipv6_pppoe") == 0 && strlen(nvram_safe_get("ipv6_pppoe_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_pppoe_lan_ip"), nvram_safe_get("lan_bridge"));
        else if(nvram_match("ipv6_wan_proto","ipv6_6to4") == 0 && strlen(nvram_safe_get("ipv6_6to4_lan_ip")) != 0)
            _system("ip -6 addr del %s/64 dev %s", nvram_safe_get("ipv6_6to4_lan_ip"), nvram_safe_get("lan_bridge"));
    }
#endif

    /* save value only for admin */
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 ||
            nvram_match("sp_admin_username", auth_login[index].curr_user) ==0 )

#else
    if( nvram_match("admin_username", auth_login.curr_user) ==0 ||
        nvram_match("sp_admin_username", auth_login.curr_user) ==0 )
#endif
#else//HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 )
#else
    if(nvram_match("admin_username", auth_login.curr_user) ==0 )
#endif
#endif//HTTPD_USED_SP_ADMINID
    {
        set_basic_api(wp);
        nvram_commit();
    }


#ifdef IPv6_SUPPORT
    if(strcmp(page_type,"IPv6") == 0)
        system("killall -SIGPIPE timer"); //sync httpd & nvram in time
#endif

#if 0
//#ifdef IPv6_TEST
    if( strcmp("reboot",reboot_type) == 0 && redirect_count_down_page(wp,F_REBOOT,SUPERG_DOMAIN_CHANGED_REBOOT_COUNT_DOWN,1)){
        sys_reboot();
    }else{
        /* redirect to correct page */
        websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
#else
        /*
         * Reason: Update for Multi-Save
         * Modified: Yufa Huang
         * Date: 2011.07.04
         */
        //if( strcmp("shutdown",reboot_type) == 0 ){
        if (0) {
    		if (redirect_count_down_page(wp, F_REBOOT, REBOOT_COUNT_DOWN, 1)) {
        		fprintf(stderr, "redirect to count down page success\n");
        		sys_reboot();
    			/* Reset CGI */
    			init_cgi(NULL);
    			return 1;
    		}
    		else {
        		fprintf(stderr,"redirect to count down page fail\n");
        		websWrite(wp, redirecter_footer, absolute_path("login.asp"));
    		}
        }else{
#ifdef AP_NOWAN_HASBRIDGE
            if(strcmp(html_response_page,"count_down.asp"))
                websWrite(wp,redirecter_footer, absolute_path(html_response_page));
            else
                redirect_count_down_page(wp,F_RC_RESTART,count_down_time,F_RC_RESTART);
#else
            /*
             * Reason: Update for Multi-Save
             * Modified: Yufa Huang
             * Date: 2011.07.08
             */
            extern int webserver_reboot_needed;

            if (strcmp("none", reboot_type)) {
            	webserver_reboot_needed |= 1;
                websWrite(wp, redirecter_footer, absolute_path("reboot_needed.asp"));
            }
            else {
                websWrite(wp, redirecter_footer, absolute_path(html_response_page));
            }
#endif
        }
#endif
        /*
         * Reason: Update for Multi-Save
         * Modified: Yufa Huang
         * Date: 2011.07.04
         */
        //if (strcmp("none", reboot_type) != 0)
        //    rc_action(reboot_type);

#ifdef IPv6_SUPPORT
    // }
   // write_radvd_conf();
#endif
    return 1;
}

static void do_apply_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    apply_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

/*
 * Reason: Follw D-Link Router Web-based Setup Wizard Spec.
 * Modified: Yufa Huang
 * Date: 2011.02.25
 */
static void do_wizard_apply_cgi(char *url, FILE *stream)
{
    assert(url);
    assert(stream);
	
    if (nvram_match("nvram_default_value", "1") == 0) {
    	skip_auth = 1;

        set_basic_api(stream);
        nvram_commit();

        if (nvram_match("nvram_default_value", "1") == 0) {
            websWrite(stream, redirecter_footer, absolute_path("setup_wizard.asp"));
        }
        else {
            redirect_count_down_page(stream, F_REBOOT, REBOOT_COUNT_DOWN, 1);
            sys_reboot();
        }
    }

    /* Reset CGI */
    init_cgi(NULL);
}

static void do_wizard_cancel_cgi(char *url, FILE *stream)
{
    assert(url);
    assert(stream);

    nvram_set("nvram_default_value", "0");
    nvram_flag_set();
    nvram_commit();
    websWrite(stream, redirecter_footer, absolute_path("login.asp"));

    /* Reset CGI */
    init_cgi(NULL);
}

#ifdef MPPPOE
void stop_mpppoe_timer(int ifname,int reset_flag)
{
    char *file_path;
    init_file(PPP_IDLE_00);
    init_file(PPP_IDLE_01);
    init_file(PPP_IDLE_02);
    init_file(PPP_IDLE_03);
    init_file(PPP_IDLE_04);

    switch (ifname)
    {
        case 0://for ppp0
            file_path = PPP_IDLE_00;
            break;
        case 1://for ppp1
            file_path = PPP_IDLE_01;
            break;
        case 2://for ppp2
            file_path = PPP_IDLE_02;
            break;
        case 3://for ppp3
            file_path = PPP_IDLE_03;
            break;
        case 4://for ppp4
            file_path = PPP_IDLE_04;
            break;
    }

    if(reset_flag)
        init_file(file_path);
    else
        save2file(file_path,"stop\n");

}

static int mpppoe_apply_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{

    char *html_response_page = websGetVar(wp, "html_response_page", nvram_safe_get("default_html"));
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", nvram_safe_get("default_html"));
    char *reboot_type = websGetVar(wp, "reboot_type", "none");
    int ppp_ifunit,redial_flag = 0,monitor_flag = 0,manual_flag = 0;
    char ppp_if_name[4] = {};
    char connection_mode[32] = {};
    char pppoe_enable[32] = {};
    /* accound = user only */
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("mpppoe_apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 ){
        DEBUG_MSG("mpppoe_apply_cgi: index=%d\n",index);
        DEBUG_MSG("mpppoe_apply_cgi: curr_user=%s\n",auth_login[index].curr_user);
        if(nvram_match("user_username", auth_login[index].curr_user) ==0)
        {
            DEBUG_MSG("mpppoe_apply_cgi: curr_user=%s==user_username\n",auth_login[index].curr_user);
            nvram_set("html_response_return_page",html_response_return_page);
            websWrite(wp,redirecter_footer, absolute_path("user_page.asp"));
            return 1;
        }
    }
#else
    if(nvram_match("user_username", auth_login.curr_user) ==0)
    {
        nvram_set("html_response_return_page",html_response_return_page);
        websWrite(wp,redirecter_footer, absolute_path("user_page.asp"));
        return 1;
    }
#endif
    /* save value only for admin */
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 ||
            nvram_match("sp_admin_username", auth_login[index].curr_user) ==0 )

#else
    if( nvram_match("admin_username", auth_login.curr_user) ==0 ||
        nvram_match("sp_admin_username", auth_login.curr_user) ==0 )
#endif
#else//HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 )
#else
    if(nvram_match("admin_username", auth_login.curr_user) ==0 )
#endif
#endif//HTTPD_USED_SP_ADMINID
    {
        printf("start set value\n");
        set_basic_api(wp);
        printf("start nvram commit\n");
        nvram_commit();
    }
    /* redirect to correct page */
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    if(!nvram_match("wan_proto","pppoe"))
    {
        if(checkServiceAlivebyName("udhcpc"))
        {
            _system("killall udhcpc");
            _system("ifconfig %s 0.0.0.0",nvram_safe_get("wan_eth"));
        }

        modify_ppp_options();
        nvram_flag_reset();
        //To know what kind of dial up mode in MPPPOE
        for (ppp_ifunit=0; ppp_ifunit < MAX_PPPOE_CONNECTION; ppp_ifunit ++)
        {
            memset(pppoe_enable,0,sizeof(pppoe_enable));
            memset(ppp_if_name,0,sizeof(ppp_if_name));
            memset(connection_mode,0,sizeof(connection_mode));
            sprintf(ppp_if_name, "ppp%d", ppp_ifunit);
            sprintf(connection_mode, "wan_pppoe_connect_mode_%02d",ppp_ifunit);
            sprintf(pppoe_enable,"wan_pppoe_enable_%02d",ppp_ifunit);

            if(nvram_match(connection_mode,"always_on") == 0 && nvram_match(pppoe_enable,"1") == 0)
            {
                redial_flag = 1;
                if(checkServiceAlivebyName("ppptimer"))
                    stop_mpppoe_timer(ppp_ifunit,0);
            }
            if(nvram_match(connection_mode,"on_demand") == 0 && nvram_match(pppoe_enable,"1") == 0)
            {
                monitor_flag = 1;
                if(checkServiceAlivebyName("ppptimer"))
                    stop_mpppoe_timer(ppp_ifunit,1);
            }
            if(nvram_match(connection_mode,"manual") == 0 && nvram_match(pppoe_enable,"1") == 0)
            {
                manual_flag = 1;
                if(checkServiceAlivebyName("ppptimer"))
                    stop_mpppoe_timer(ppp_ifunit,0);
            }
        }
        printf("redial_flag = %d monitor_flag = %d manual_flag = %d\n",redial_flag,monitor_flag,manual_flag);

        //Can not use system call, it will cause HTTPD blocked.So,we use signal instead of system call
        if(manual_flag && redial_flag == 0 && monitor_flag == 0) //manual only
        {
            manual_flag = 0;
            _system("killall -SIGUSR1 lld2d");
        }
        else if(redial_flag && manual_flag == 0 && monitor_flag == 0) //always on only
        {
            redial_flag = 0;
            _system("killall -SIGUSR2 lld2d");
        }
        else if(monitor_flag && manual_flag == 0 && redial_flag == 0) //demand only
        {
            monitor_flag = 0;
            _system("killall -SIGHUP lld2d");
        }
        else if(manual_flag && redial_flag && monitor_flag == 0) //manual and always on == always on only
        {
            manual_flag = 0;
            redial_flag = 0;
            _system("killall -SIGUSR2 lld2d");
        }
        else if(manual_flag && redial_flag == 0 && monitor_flag) //manual and demand == demand only
        {
            manual_flag = 0;
            monitor_flag = 0;
            _system("killall -SIGHUP lld2d");
        }
        else if(manual_flag == 0 && redial_flag && monitor_flag) //always on and demand
        {
            redial_flag = 0;
            monitor_flag = 0;
            _system("killall -SIGTSTP lld2d");
        }
        else if(manual_flag && redial_flag && monitor_flag) //all types == always on and demand
        {
            manual_flag = 0;
            redial_flag = 0;
            monitor_flag = 0;
            _system("killall -SIGTSTP lld2d");
        }
    }

    if( strcmp("none", reboot_type) != 0 )
        rc_action(reboot_type);
    return 1;
}


static void do_mpppoe_apply_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    mpppoe_apply_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
#endif

/*
*   Date: 2009-08-10
*   Name: Ken Chiang
*   Reason: Modify for GPL CmoCgi.
*   Notice :
*/
void do_apply_post(char *url, FILE *stream, int len, char *boundary)
{
    assert(url);
    assert(stream);
/*  Date:   2009-04-09
 *  Name:   Yufa Huang
 *  Reason: add HTTPS function.
 */
    /* Get query */
    if (!wfgets(post_buf, MIN(len + 1, sizeof(post_buf)), stream))
        return;
    len -= strlen(post_buf);
    DEBUG_MSG("%s\n",post_buf);
    /* Initialize CGI */
    init_cgi(post_buf);

#if defined(HAVE_HTTPS)
    if (openssl_request == 1)
        return;
#endif

    /* Slurp anything remaining in the request */
    while (len--)
        (void) fgetc(stream);
}

static int dhcp_release_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page=NULL;
    html_response_page = websGetVar(wp, "html_response_page", "");

    dhcpc_release();
    sleep(1);
    /*NickChou, move to dhcpc.c, if wan ip change then remove_bandwidth_file
#ifdef CONFIG_USER_TC
remove_bandwidth_file();
#endif
*/
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_dhcp_release_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    dhcp_release_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int dhcp_renew_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page=NULL;
    html_response_page = websGetVar(wp, "html_response_page", "");

    dhcpc_renew();
    sleep(1);

    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_dhcp_renew_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    save_html_page_to_nvram(stream);
    dhcp_renew_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

/*  Date: 2009-2-17
*   Name: Nick Chou
*   Reason: added to change language from GUI.
*   Notice :
*/
#if defined(DIR865) || defined(EXVERSION_NNA)
static int switch_language_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_lang=NULL;
    char *language=NULL;

    html_response_lang = websGetVar(wp, "html_response_lang", "");
    language = websGetVar(wp, "language", "");
/*  Date: 2009-3-10
*   Name: Ken Chiang
*   Reason: modify to fix reboot or upgrade fw the language no change issue.
*   Notice :
*/
/*
    nvram_set("language",language);
*/
    //printf("language=%s\n",language);
    nvram_set("language",language);
    nvram_flag_set();
    nvram_commit();
    websWrite(wp,redirecter_footer, absolute_path(html_response_lang));

    return 1;
}

static void do_switch_language_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    save_html_page_to_nvram(stream);

    switch_language_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
#endif //#ifdef DIR865

static void do_restore_default_cgi(char *url, FILE *stream)
{
    assert(url);
    assert(stream);
    if(redirect_count_down_page(stream,F_RESTORE,RESTORE_COUNT_DOWN,1))
    {
        fprintf(stderr,"redirect to count down page success \n");
        if(restore_flag)
            restore_default();
    }
    else
    {
        restore_flag = 0;
        fprintf(stderr,"redirect to count down page fail \n");
        websWrite(stream,redirecter_footer, absolute_path("login.asp"));

    }

    /* Reset CGI */
    init_cgi(NULL);
}

static void do_restore_default_wireless_cgi(char *url, FILE *stream)
{
	assert(url);
	assert(stream);

#ifdef CONFIG_LP
	check_http_url_request(url,stream);
#endif

	if (redirect_count_down_page(stream,F_RESTORE,RESTORE_WIRELESS_COUNT_DOWN,1)) {
		printf("redirect to count down page success\n");
		restore_wireless_setting();
	}
	else {
		printf("redirect to count down page fail\n");
		websWrite(stream,redirecter_footer, absolute_path("login.asp"));
	}

    /* Reset CGI */
    init_cgi(NULL);
}

#ifdef IPv6_TEST
static void restore_default_ipv6_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    websWrite(wp,redirecter_footer, absolute_path(html_response_page));

    restore_ipv6_setting();

    return 1;
}

static void do_restore_default_ipv6_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    restore_default_ipv6_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
#endif //#ifdef IPv6_TEST



static int pppoe_disconnect_return_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}


static int pppoe_return_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *countdown_time = websGetVar(wp, "countdown_time", "");
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");
    char *html_response_message = websGetVar(wp, "html_response_message", "");
    nvram_set("countdown_time",countdown_time);
    nvram_set("html_response_return_page",html_response_return_page);
    nvram_set("html_response_message",html_response_message);
    nvram_commit();
    //sleep(3);
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static int pppoe_disconnect(int pppoe_unit)
{
#ifdef MPPPOE
    char *dns_file;
    char connect_mode[] = "wan_pppoe_connect_mode_XY";
    if(pppoe_unit == 0)
        dns_file = DNS_FILE_00;
    if(pppoe_unit == 1)
        dns_file = DNS_FILE_01;
    if(pppoe_unit == 2)
        dns_file = DNS_FILE_02;
    if(pppoe_unit == 3)
        dns_file = DNS_FILE_03;
    if(pppoe_unit == 4)
        dns_file = DNS_FILE_04;
    unlink(dns_file);
    struct mp_info mpppoe;
    mpppoe = check_mpppoe_info(pppoe_unit);
    if( mpppoe.pid > 0)
        _system("kill %d &",mpppoe.pid);
    sprintf(connect_mode,"wan_pppoe_connect_mode_%02d",pppoe_unit);
    if(nvram_match(connect_mode,"always_on") !=0 )
        _system("killall rc");//restart apps
    return 0;
#else
    _system("/var/sbin/ip-down pppoe %d &", pppoe_unit);
    return 0;
#endif
}


static int pppoe_connect(int pppoe_unit)
{
    char sn[128]={0};
    char *wan_eth=NULL, *service_name=NULL;
    FILE *fp_pppoe;
    char *wan_pppoe_connect_mode_00 = NULL;
//albert 20081027
    char ac[128]={0};
    char *ac_name=NULL;

#ifdef MPPPOE
    char connection_mode[64] = {};
    char service_name_m[128] = "";
    char tmp_service_name[128] = "";
    char *sn_m;
    char ppp_if_unit[8] = {};

    sprintf(connection_mode,"wan_pppoe_connect_mode_%02d",pppoe_unit);
    sprintf(service_name_m,"wan_pppoe_service_%02d",pppoe_unit);
    sprintf(ppp_if_unit,"ppp%d",pppoe_unit);
    //if wan already has an IP,ignore the connect button
    if(read_ipaddr_no_mask(ppp_if_unit))
        return 0;
    sn_m = nvram_safe_get(service_name_m);
    strcpy(tmp_service_name, sn_m);

    pppoe_disconnect(pppoe_unit);
    sleep(1);
    if(strcmp(sn_m,"") == 0 || strlen(sn_m) == 0)
    {
        _system("pppd linkname %02d file /var/etc/options_%02d unit %d pty \'pppoe -I %s\' &", pppoe_unit, pppoe_unit, pppoe_unit, nvram_get("wan_eth"));
    }
    else
    {
        if(nvram_match(connection_mode,"always_on") != 0)
            _system("pppd linkname %02d file /var/etc/options_%02d unit %d pty \'pppoe -I %s -U -S \"%s\"  -C \"%s\"\' &", pppoe_unit, pppoe_unit, pppoe_unit, nvram_get("wan_eth"), parse_special_char(tmp_service_name),parse_special_char(tmp_service_name));
    }
#else //#ifdef MPPPOE
        
            fp_pppoe = fopen("/var/run/ppp0.pid", "r");
            if(fp_pppoe==NULL)
            {
                wan_eth = nvram_safe_get("wan_eth");
                service_name = nvram_safe_get("wan_pppoe_service_00");
                if(strcmp(service_name, "") == 0)
                    strncpy(sn, "null_sn", strlen("null_sn"));
                else
                    strncpy(sn, service_name, strlen(service_name));
//albert add 20081027
                ac_name = nvram_safe_get("wan_pppoe_ac_00");
                if(strcmp(ac_name, "") == 0)
                    strncpy(ac, "null_ac", strlen("null_ac"));
                else
                    strncpy(ac, ac_name, strlen(ac_name));
                _system("/var/sbin/ip-up pppoe %d %s \"%s\" \"%s\" &",
                    pppoe_unit, wan_eth, parse_special_char(sn), parse_special_char(ac));
            }
            else
            {
                printf("PPPoE is already connecting\n");
                fclose(fp_pppoe);
            }
#endif
    return 0;
}

/*  Date: 2009-01-21
*   Name: jimmy huang
*   Reason: for usb-3g connect/disconnect via UI
*   Note:
*
*/
#ifdef CONFIG_USER_3G_USB_CLIENT
static int usb3g_connect(int pppoe_unit){
    char sn[128]={0};
    char *service_name=NULL;
    FILE *fp_pppoe;
    char *usb3g_reconnect_mode = NULL;

    DEBUG_MSG("%s\n",__FUNCTION__);
       strncpy(sn, "null_sn", strlen("null_sn"));

               fp_pppoe = fopen(USB_3G_PID, "r");// var/run/ppp0.pid
            if(fp_pppoe==NULL)
            {
                  _system("/var/sbin/ip-up usb3g &");
            }
            else
            {
                printf("3G USB is already connecting\n");
                fclose(fp_pppoe);
            }
      DEBUG_MSG("%s finish\n",__FUNCTION__);
    return 0;
}
#endif //#ifdef CONFIG_USER_3G_USB_CLIENT

#ifdef CONFIG_USER_3G_USB_CLIENT
static int usb3g_disconnect(int pppoe_unit){
    DEBUG_MSG("%s, calling ip-down usb3g\n",__FUNCTION__);
    _system("/var/sbin/ip-down usb3g %d &", pppoe_unit);
    return 0;
}
#endif //#ifdef CONFIG_USER_3G_USB_CLIENT

#ifdef CONFIG_USER_3G_USB_CLIENT
static int do_usb3g_connect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    int pid , status;
    int count = 0;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            usb3g_connect(0);
            pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(5);
#else
            exit(5);
#endif
        default:
            wait(&status);
    }
    /* Reset CGI */
    nvram_flag_reset();
    init_cgi(NULL);
}
#endif //#ifdef CONFIG_USER_3G_USB_CLIENT

#ifdef CONFIG_USER_3G_USB_CLIENT
static int do_usb3g_disconnect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            usb3g_disconnect(0);
#ifdef CONFIG_USER_TC
            remove_bandwidth_file();
#endif
            pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}
#endif//#ifdef CONFIG_USER_3G_USB_CLIENT

#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
/*
iphone start flow
1. driver init OK
2. execute iphone application to send ssl ... to enable internet sharing
3. ifconfig eth2 up
4. udhcpc -i eth2

iphone stop flow
1. kill udhcpc
2. ifconfig eth2 0.0.0.0
3. ifconfig eth2 down
4. execute iphone application to send ssl ... to disable internet sharing
*/
char cmd[96]={'\0'};
#ifdef CONFIG_USER_3G_USB_CLIENT_IPHONE
int init_iphone(void){
	FILE *fp;
	memset(cmd,'\0',sizeof(cmd));
	unlink(IPHONE_INIT_RESULT);
	sprintf(cmd,"[ -n \"`iphone | grep Success`\" ] && echo 1 > %s",IPHONE_INIT_RESULT);
	system(cmd);
	if((fp = fopen(IPHONE_INIT_RESULT,"r"))!=NULL){
		fclose(fp);
		return 1;
	}
	return 0;
}
#endif

void dhcpc_kill_renew(char *interface){
	int i = 0;

	memset(cmd,'\0',sizeof(cmd));
	DEBUG_MSG("interface = %s\n", interface);
	system("[ -n \"`ps | grep udhcpc`\" ] && killall udhcpc");
#ifdef CONFIG_USER_3G_USB_CLIENT_WM5
	if(strcmp(interface,"rndis0") == 0){
		DEBUG_MSG("bring up interface %s\n", interface);
		// if interface rndis0 is not up, bring it up
		system("[ -n \"`ifconfig | grep rndis0`\" ] || ifconfig rndis0 up");
	}
#endif
#ifdef CONFIG_USER_3G_USB_CLIENT_IPHONE
	if(strcmp(interface,"eth2") == 0){
		// other mobile phone will bring different interface
		if(init_iphone() == 1){
			DEBUG_MSG("init iphone Success\n");
		}else{
			DEBUG_MSG("init iphone failed, try again\n");
			init_iphone();
		}
		//sleep(1); // wait iphone to send init request to iphone device
		DEBUG_MSG("bring up interface %s\n", interface);
		// if interface rndis0 is not up, bring it up
		system("[ -n \"`ifconfig | grep eth2`\" ] || ifconfig eth2 up");

	}
#endif
	sprintf(cmd,"udhcpc -i %s -s %s &",interface,DHCPC_DNS_SCRIPT);
	DEBUG_MSG("cmd = %s\n", cmd);
	//cmd_tmp += sprintf(cmd_tmp,"-s %s %s ",wan_specify_dns ? DHCPC_NODNS_SCRIPT:DHCPC_DNS_SCRIPT,tmp_netbios);
	_system(cmd);
}

static int do_usb3g_phone_connect_cgi(char *url, FILE *stream)
{
	char *path=NULL, *query=NULL;
	int pid , status;
	int count = 0;

	DEBUG_MSG("usb3g_phone_connect\n");

	assert(url);
	assert(stream);

	save_html_page_to_nvram(stream);

	/* Parse path */
	query = url;
	path = strsep(&query, "?") ? : url;
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

	switch(pid)
	{
		case -1:
			printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
		case 0:
			dhcpc_kill_renew(nvram_safe_get("wan_eth"));
			//websWrite(wp,redirecter_footer, absolute_path(html_response_page));
			pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(5);
#else
            exit(5);
#endif
		default:
			wait(&status);
	}
	/* Reset CGI */
	nvram_flag_reset();
	init_cgi(NULL);
}

static int do_usb3g_phone_disconnect_cgi(char *url, FILE *stream)
{
	char *path=NULL, *query=NULL;
	assert(url);
	assert(stream);

	/* Parse path */
	query = url;
	path = strsep(&query, "?") ? : url;
	int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

	switch(pid)
	{
		case -1:
			printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
		case 0:
			system("killall udhcpc 2>&1 > /dev/null");

#ifdef CONFIG_USER_3G_USB_CLIENT_WM5
/*	Date:	2010-02-05
 *	Name:	Cosmo Chang
 *	Reason:	add usb_type=5 for Android Phone RNDIS feature
 */
			if(nvram_match("usb_type", "3")==0 || nvram_match("usb_type", "5")==0){
				system("ifconfig rndis0 0.0.0.0");
				system("ifconfig rndis0 down");
			}
#endif
#ifdef CONFIG_USER_3G_USB_CLIENT_IPHONE
			if(nvram_match("usb_type", "4")==0){
				system("ifconfig eth2 0.0.0.0");
				system("ifconfig eth2 down");
				if(init_iphone() != 0){
					/*
						application iphone initial OK to enable internet share feature
						exec again to disable
					*/
					init_iphone();
				}
			}
#endif
#ifdef CONFIG_TC
			remove_bandwidth_file();
#endif
			//websWrite(wp,redirecter_footer, absolute_path(html_response_page));
			pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
		default:
			wait(&status);
	}

	/* Reset CGI */
	init_cgi(NULL);
}
#endif


static void do_pppoe_connect_00_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    int pid , status;
    int count = 0;
    char ppp6_ip[64]={}; 

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_connect(0);

            while(count < 8)
            {
                if(wan_connected_cofirm("ppp0"))
                    break;
                else
                {
                    sleep(1);
                    count++;
                }
            }
						while (count < 18 && nvram_match("ipv6_wan_proto","ipv6_autodetect")==0) { 
										if(read_ipv6addr("ppp0", SCOPE_GLOBAL, ppp6_ip, sizeof(ppp6_ip))) 
														break; 
										else { 
														sleep(1); 
														count++; 
										} 
										if(count == 18){ 
														cprintf("IPv6 PPPoE share with IPv4 is Failed, Change to Autoconfiguration Mode\n"); 
														system("cli ipv6 rdnssd stop"); 
														system("cli ipv6 autoconfig stop"); 
														_system("cli ipv6 rdnssd start %s", nvram_safe_get("wan_eth")); 
										}
						}
            printf("httpd wait %d Secs for PPPoE dial\n", count);

            pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(5);
#else
            exit(5);
#endif
        default:
            wait(&status);
    }
    /* Reset CGI */
    nvram_flag_reset();
    init_cgi(NULL);
}


static void do_pppoe_disconnect_00_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
#ifdef IPV6_SUPPORT
						if (nvram_match("ipv6_wan_proto","ipv6_autodetect")==0) {
										system("cli ipv6 rdnssd stop");
										system("cli ipv6 autoconfig stop");
						}
#endif
            pppoe_disconnect(0);
            printf("wait 3 Secs for PPPoE Disconnect\n");
            sleep(3);
#ifdef CONFIG_USER_TC
            remove_bandwidth_file();
#endif
            pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}

#ifdef MPPPOE
static void do_pppoe_connect_01_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_connect(1);
            pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }
    nvram_flag_reset();
    /* Reset CGI */
    init_cgi(NULL);
}
#endif

#ifdef MPPPOE
static void do_pppoe_disconnect_01_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_disconnect(1);
#ifdef CONFIG_USER_TC
            remove_bandwidth_file();
#endif
            pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}
#endif

#ifdef MPPPOE
static void do_pppoe_connect_02_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_connect(2);
            pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }
    nvram_flag_reset();
    /* Reset CGI */
    init_cgi(NULL);
}
#endif

#ifdef MPPPOE
static void do_pppoe_disconnect_02_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_disconnect(2);
#ifdef CONFIG_USER_TC
            remove_bandwidth_file();
#endif
            pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}
#endif

#ifdef MPPPOE
static void do_pppoe_connect_03_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_connect(3);
            pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }
    nvram_flag_reset();
    /* Reset CGI */
    init_cgi(NULL);
}
#endif

#ifdef MPPPOE
static void do_pppoe_disconnect_03_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_disconnect(3);
#ifdef CONFIG_USER_TC
            remove_bandwidth_file();
#endif
            pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}
#endif

#ifdef MPPPOE
static void do_pppoe_connect_04_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_connect(4);
            pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }
    nvram_flag_reset();
    /* Reset CGI */
    init_cgi(NULL);
}
#endif

#ifdef MPPPOE
static void do_pppoe_disconnect_04_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            pppoe_disconnect(4);
#ifdef CONFIG_USER_TC
            remove_bandwidth_file();
#endif
            pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}
#endif

static int l2tp_connect_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    int pid, status;
    FILE *fp_l2tp;
    char *wan_l2tp_connect_mode = nvram_safe_get("wan_l2tp_connect_mode");
    int count=0;

    if(!wan_connected_cofirm("ppp0"))
                _system("/var/sbin/ip-up l2tp &");


            websWrite(wp,redirecter_footer, absolute_path(html_response_page));

    return 1;
}

static void do_l2tp_connect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    l2tp_connect_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int l2tp_disconnect_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");

    _system("var/sbin/ip-down l2tp &");
    printf("wait 3 Secs for L2TP Disconnect\n");
    sleep(3);
#ifdef CONFIG_USER_TC
    remove_bandwidth_file();
#endif
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));

    return 1;
}

static void do_l2tp_disconnect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    l2tp_disconnect_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int pptp_connect_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    int pid, status;
    FILE *fp_pptp;
    int count=0;


    if(!wan_connected_cofirm("ppp0"))
                _system("/var/sbin/ip-up pptp &");

            websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 0;
}

static void do_pptp_connect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    pptp_connect_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int pptp_disconnect_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    _system("/var/sbin/ip-down pptp &");
    printf("wait 3 Secs for PPTP Disconnect\n");
    sleep(3);
#ifdef CONFIG_USER_TC
    remove_bandwidth_file();
#endif
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 0;
}

static void do_pptp_disconnect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    pptp_disconnect_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int bigpond_login_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            _system("/var/sbin/ip-up bigpond");
            websWrite(wp,redirecter_footer, absolute_path(html_response_page));
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }
    return 0;
}

static void do_bigpond_login_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    save_html_page_to_nvram(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    bigpond_login_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int bigpond_logout_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    _system("/var/sbin/ip-down bigpond");
#ifdef CONFIG_USER_TC
    remove_bandwidth_file();
#endif
    printf("html_response_page is : %s\n", html_response_page);
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));

    return 0;
}

static void do_bigpond_logout_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    bigpond_logout_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int online_firmware_check_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = NULL;
    char *html_response_message = NULL;
    char *html_response_return_page = NULL;

    html_response_page = websGetVar(wp, "html_response_page", "");
    html_response_message = "Connecting with the server for firmware information";
    html_response_return_page = websGetVar(wp, "html_response_return_page", "");
    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));

    return 1;
}

static void do_online_firmware_check_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    online_firmware_check_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int dhcp_voke_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *revoke_ip = NULL;
    char *revoke_mac = NULL;
    char *html_response_return_page = NULL;

    revoke_mac = websGetVar(wp, "revoke_mac", "");
    revoke_ip = websGetVar(wp, "revoke_ip", "");
    html_response_return_page = websGetVar(wp, "html_response_return_page", "");
    init_file(DHCPD_REVOKE_FILE);
    save2file(DHCPD_REVOKE_FILE, "#Sample udhcpd revoke\n"
            "ip %s\n" \
            "mac    %s\n" \
            ,revoke_ip, revoke_mac);
    _system("killall -SIGUSR2 udhcpd");
    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));

    return 1;
}

static void do_dhcp_voke_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    dhcp_voke_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);


}
static int ping_response_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = NULL;
    char *html_response_message = NULL;
    char *html_response_return_page = NULL;
    html_response_page = websGetVar(wp, "html_response_page", "");
    html_response_message = "Waiting for remote host response...";
    html_response_return_page = websGetVar(wp, "html_response_return_page", "");
    ping.ping_ipaddr = websGetVar(wp, "ping_ipaddr", "");

#ifdef IPv6_TEST
    ping.ping_ipaddr_v6 = websGetVar(wp, "ping6_addr", "");
    ping.interface = websGetVar(wp, "ifp", "");
    ping.ping6_size = atoi(websGetVar(wp, "ping6_size", "1500"));
    ping.ping6_count = atoi(websGetVar(wp, "ping6_count", "1"));
#endif
    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

#ifdef IPv6_TEST
    if ((strcmp(ping.ping_ipaddr,"") != 0 && ping.ping_ipaddr != NULL) || (strcmp(ping.ping_ipaddr_v6,"") != 0 && ping.ping_ipaddr_v6 != NULL))
#else
    if ((strcmp(ping.ping_ipaddr,"") != 0 && ping.ping_ipaddr != NULL))
#endif
        websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    else
        websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));  //graceyang 20070803

    return 1;
}

static void do_ping_response_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    ping_response_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

#ifdef IPv6_SUPPORT
static int ping6_response_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
                char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = NULL;
        char *html_response_message = NULL;
        char *html_response_return_page = NULL;
        html_response_page = websGetVar(wp, "html_response_page", "");
        html_response_message = "Waiting for remote host response...";
        html_response_return_page = websGetVar(wp, "html_response_return_page", "");

        ping.ping_ipaddr_v6 = websGetVar(wp, "ping6_ipaddr", "");

    nvram_set("html_response_message",html_response_message);
        nvram_set("html_response_page",html_response_page);
        nvram_set("html_response_return_page",html_response_return_page);

        if((strcmp(ping.ping_ipaddr_v6,"") != 0 && ping.ping_ipaddr_v6 != NULL))
            websWrite(wp,redirecter_footer, absolute_path(html_response_page));
        else
            websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));      //graceyang 20070803
        return 1;
}

static void do_ping6_response_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

        assert(url);
        assert(stream);

        /* Parse path */
        query = url;
        path = strsep(&query, "?") ? : url;

        ping6_response_cgi(stream, NULL, NULL, 0, url, path, query);

        /* Reset CGI */
        init_cgi(NULL);
}
static void do_ipv6_pppoe_connect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    int pid , status;
    int count = 0;
    assert(url);
    assert(stream);
    save_html_page_to_nvram(stream);
    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            ipv6_pppoe_connect();
            while(count < 8)
            {
                if(wan_connected_cofirm("ppp6"))
                    break;
                else
                {
                    sleep(1);
                    count++;
                }
            }
            printf("httpd wait %d Secs for IPv6 PPPoE dial\n", count);
            pppoe_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(5);
#else
            exit(5);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}

static void do_ipv6_pppoe_disconnect_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    int pid, status;

#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            ipv6_pppoe_disconnect();
            printf("wait 3 Secs for IPv6 PPPoE Disconnect\n");
            sleep(3);
            pppoe_disconnect_return_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        default:
            wait(&status);
    }

    /* Reset CGI */
    init_cgi(NULL);
}

static void ipv6_pppoe_connect()
{
	system("cli ipv6 pppoe start");
	if(nvram_match("ipv6_pppoe_connect_mode", "always_on")!=0)
system("cli ipv6 pppoe dial");
}

static void ipv6_pppoe_disconnect()
{
	system("cli ipv6 pppoe stop");
	system("killall -SIGILL wantimer");
}

static void set_percent(int value)
{
	if(value >= -1 && value <= 100)
		_system("echo %d > %s", value, WIZARD_IPV6);
}

static void do_wizard_autodetect_start_cgi()
{
	system("cli ipv6 wizard start &");
	init_cgi(NULL);
}

static void do_wizard_autodetect_stop_cgi(void)
{
	system("cli ipv6 wizard stop &");
	init_cgi(NULL);
}

#endif //ifdef IPv6_SUPPORT

/////////////////////////////////////// apc_set_sta_enrollee_pin ///////////////////////////////////////
////
////
#define APC_WSC_CONF_FILE  "/var/etc/stafwd-wsc.conf"
static void create_APC_wpa_conf_file(void) {
    FILE *fp=fopen(APC_WSC_CONF_FILE, "w+");
    if (!fp) {
        DEBUG_MSG("create_APC_wsc_conf: create %s fail\n", APC_WSC_CONF_FILE);
        return;
    }

    fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
    fprintf(fp, "update_config=1\n");
    fprintf(fp, "wps_property={\n");
    {
        fprintf(fp, "wps_disable=0\n");
        fprintf(fp, "wps_upnp_disable=1\n");
        fprintf(fp, "version=0x10\n");
//      uuid=
        fprintf(fp, "auth_type_flags=0x0023\n"); //OPEN/WPAPSK/WPA2PSK
        fprintf(fp, "encr_type_flags=0x000f\n"); //NONE/WEP/TKIP/AES
        fprintf(fp, "conn_type_flags=0x1\n");    //ESS
        fprintf(fp, "config_methods=0x84\n");    //PBC/PIN
        fprintf(fp, "wps_state=0x01\n");         //not configured
        fprintf(fp, "rf_bands=0x01\n");          //1:(2.4G), 2:(5G), 3:(2.4G+5G)
        fprintf(fp, "manufacturer=\"%s\"\n",  nvram_safe_get("manufacturer"));
        fprintf(fp, "model_name=\"%s\"\n",    nvram_safe_get("model_name"));
        fprintf(fp, "model_number=\"%s\"\n",  nvram_safe_get("model_number"));
        fprintf(fp, "serial_number=\"%s\"\n", nvram_safe_get("serial_number"));
        fprintf(fp, "dev_category=1\n");         //Computer
        fprintf(fp, "dev_sub_category=1\n");     //PC
        fprintf(fp, "dev_oui=0050f204\n");
        fprintf(fp, "dev_name=\"%s\"\n",      nvram_safe_get("model_name"));
        fprintf(fp, "os_version=0x00000001\n");
    }
    fprintf(fp, "}\n");

    fclose(fp);
//  system("cat /var/etc/stafwd-wsc.conf");printf("\n");
}

static void create_APC_topo_file() {
    FILE *fp=fopen("/var/tmp/stafwd-topology.conf", "w+");
    if(fp==NULL) {
        DEBUG_MSG("create_topo_file: create stafwd-topology.conf : fail\n");
        return;
    }

    fprintf(fp, "bridge %s\n", nvram_safe_get("lan_bridge"));
    fprintf(fp, "{\n");
    fprintf(fp, "\tipaddress %s\n", nvram_safe_get("lan_ipaddr"));
    fprintf(fp, "\tipmask %s\n", nvram_safe_get("lan_netmask"));
    fprintf(fp, "\tinterface %s\n", WLAN0_ETH);
    fprintf(fp, "\tinterface %s\n", nvram_safe_get("lan_eth"));
    fprintf(fp, "}\n");

    fprintf(fp, "radio wifi0\n");
    fprintf(fp, "{\n");
    fprintf(fp, "\tsta %s\n", WLAN0_ETH);
    fprintf(fp, "\t{\n");
    fprintf(fp, "\t\tconfig %s\n", APC_WSC_CONF_FILE);
    fprintf(fp, "\t}\n");
    fprintf(fp, "}\n");

    fclose(fp);
//  system("cat /var/tmp/stafwd-topology.conf");printf("\n");
}

extern int push_button_result;
static int apc_set_sta_enrollee_pin_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query) {
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *enrollee_pin = websGetVar(wp, "wps_pin", "");
#ifdef WPS_WPATALK
    char cmd[128]={0};
    int pid=0, status=0;
    char *wlan0_enable=nvram_safe_get("wlan0_enable");
#endif //WPS_WPATALK

    if (nvram_match("wps_enable", "1")) {
        websWrite(wp,redirecter_footer, absolute_path(""));
        DEBUG_MSG("WPS Disable!!\n");
        return 1;
    }

#ifdef WPS_WPATALK
    /* When we use IE(other browser) to set WPS PIN code,
       GUI would hang up when it redirect to 120Sec conut donw page.
       So we use "fork()" and "2>&1 > /var/tmp/" to prevent this issue.
    */
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid) {
    case -1:
        DEBUG_MSG("apc_set_sta_enrollee_pin_cgi fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
    case 0:
        close(con_info.conn_fd);
        if (0==strcmp(wlan0_enable, "1")) {
            DEBUG_MSG("Start wpatalk!!\n");
#ifdef DIR865 // WPS Ralink
       if (enrollee_pin && (strlen(enrollee_pin) == 8)) {
		_system("cmds wlan wps pin %s %s &", enrollee_pin, "ra00_0");
		_system("cmds wlan wps pin %s %s &", enrollee_pin, "ra01_0");
		_system("cmds wlan wps monitor %s &", "ra00_0");
		_system("cmds wlan wps monitor %s &", "ra01_0");
	    } else {
		printf("%s(%d) enrollee_pin :: %s, len :: %d\n", __func__, __LINE__, enrollee_pin, strlen(enrollee_pin));
	    }
#else
#ifdef ATH_AR938X_WPS20
            sprintf(cmd, "hostapd_cli -i ath0 wps_pin any %s 2>&1 > /var/tmp/WSC_apc_ath0_wpatalk_log &", enrollee_pin);
#else
            if (enrollee_pin && 8==strlen(enrollee_pin))
                sprintf(cmd, "wpatalk ath0 \'configme pin=%s clean\' 2>&1 > /var/tmp/WSC_apc_ath0_wpatalk_log &", enrollee_pin);
            else
                sprintf(cmd, "wpatalk ath0 \'configme  clean\' 2>&1 > /var/tmp/WSC_apc_ath0_wpatalk_log &", enrollee_pin);
#endif
            _system(cmd);
#endif
        }
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
    //default:
    //  wait(&status);
    }
#endif //WPS_WPATALK

    push_button_result = 0;
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_apc_set_sta_enrollee_pin(char *url, FILE *stream) {
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    apc_set_sta_enrollee_pin_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
////
////
/////////////////////////////////////// apc_set_sta_enrollee_pin ///////////////////////////////////////


static int set_sta_enrollee_pin_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *enrollee_pin =websGetVar(wp, "wps_sta_enrollee_pin", "");
#ifdef WPS_WPATALK
    int ath_count=0;
    char cmd[128]={0};
    int pid, status;
    char *wlan0_enable=nvram_safe_get("wlan0_enable");
    char *wlan1_enable=nvram_safe_get("wlan1_enable");
#else//WPS_WPATALK
    char cmd[32]={0};
#endif//WPS_WPATALK

    push_button_result = 0;

    if( enrollee_pin != NULL){
        enrollee.pin = enrollee_pin;
    }

#ifdef WPS_WPATALK
    /*
    NickChou. 2009/1/13
    When we use IE(other browser) to set WPS PIN code,
    GUI would hang up when it redirect to 120Sec conut donw page.
    So we use "fork()" and "2>&1 > /var/tmp/" to prevent this issue.
    */
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("set_sta_enrollee_pin_cgi fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            close(con_info.conn_fd);
#ifdef DIR865
            if (strcmp(wlan0_enable, "1")==0) {
		_system("cmds wlan wps pin %s %s &", enrollee_pin, "ra01_0");
		_system("cmds wlan wps monitor %s &", "ra01_0");
            }

            if (strcmp(wlan1_enable, "1")==0) {
		_system("cmds wlan wps pin %s %s &", enrollee_pin, "ra00_0");
		_system("cmds wlan wps monitor %s &", "ra00_0");
            }

#else
            if(strcmp(wlan0_enable, "1")==0)
            {
#ifdef ATH_AR938X_WPS20
                sprintf(cmd, "hostapd_cli -i ath0 wps_pin any %s 2>&1 > /var/tmp/WSC_ath0_wpatalk_log &", enrollee.pin);
#else
                sprintf(cmd,"wpatalk ath%d \'configthem pin=%s\' 2>&1 > /var/tmp/WSC_ath%d_wpatalk_log &",
                ath_count, enrollee.pin, ath_count);
#endif
                ath_count++;
                system(cmd);
            }

            if(strcmp(wlan1_enable, "1")==0)
            {
#ifdef ATH_AR938X_WPS20
                sprintf(cmd, "hostapd_cli -i ath1 wps_pin any %s 2>&1 > /var/tmp/WSC_ath1_wpatalk_log &", enrollee.pin);
#else
                sprintf(cmd,"wpatalk ath%d \'configthem pin=%s\' 2>&1 > /var/tmp/WSC_ath%d_wpatalk_log &",
                ath_count, enrollee.pin, ath_count);
#endif
                system(cmd);
            }
#endif
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        //default:
        //  wait(&status);
    }
#else//WPS_WPATALK
    sprintf(cmd,"wsc_cfg pin %s",enrollee.pin);
    printf("set_sta_enrollee_pin=%s\n",enrollee.pin);
    system(cmd);
#endif//WPS_WPATALK

    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_set_sta_enrollee_pin(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    set_sta_enrollee_pin_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int virtual_push_button_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
#ifdef WPS_WPATALK
    int pid,status=0;
#endif
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    printf("html_response_page = %s\n",html_response_page);
//#ifdef AR9100
//  _system("echo 1 >> /proc/ar531x/gpio/push_button");
//#else
//  _system("echo 1 >> /proc/sys/dev/wifi0/wsc_button");
//#endif

#ifdef WPS_WPATALK
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid) {
    case -1:
        DEBUG_MSG("virtual_push_button_cgi fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
    case 0:
	close(con_info.conn_fd);
#ifdef DIR865
	_system("cmds wps pbc &");
	_system("cmds wps monitor &");
#else
#ifdef ATH_AR938X_WPS20
        _system("hostapd_cli -i ath0 wps_pbc &");
        _system("hostapd_cli -i ath1 wps_pbc &");
#else
        _system("wpatalk ath0 configthem &");
#endif
#endif
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
    //default:
    //  wait(&status);
    }
#endif
    push_button_result = 0;
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));


    return 1;
}

static void do_virtual_push_button(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    virtual_push_button_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int ntp_sync_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char cmd[32]={0};
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *ntp_server =websGetVar(wp, "ntp_server", "");

    if( ntp_server != NULL){
        sprintf(cmd,"ntpclient -h %s -s -i 5 -c 1",ntp_server);
        printf("ntp_sync_cgi: cmd=%s\n",cmd);
        _system(cmd);
    }

    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_ntp_sync(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    ntp_sync_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int mac_clone_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    // Do not implement this function //
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_mac_clone_cgi(char *url, FILE *stream)
{
    // Do not implement this function //
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    mac_clone_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static void do_reboot_cgi(char *url, FILE *stream)
{
    assert(stream);
    assert(url);

    if (redirect_count_down_page(stream, F_REBOOT, REBOOT_COUNT_DOWN, 1)) {
        fprintf(stderr, "redirect to count down page success\n");
        sys_reboot();
    }
    else {
        fprintf(stderr,"redirect to count down page fail\n");
        websWrite(stream, redirecter_footer, absolute_path("login.asp"));
    }

    /* Reset CGI */
    init_cgi(NULL);
}


#ifdef OPENDNS
static void do_dns_reboot_cgi(char *url, FILE *stream)
{
	do_reboot_cgi(url,stream);
}
#endif

static int nvram_deny_check(const char *key)
{
	int ret = -1;
	struct {
		const char *key;
		int len;
	} *p, deny_list[] = {
		{ "admin_", 6 },
		{ "wan_mac", -1 },
		{ "lan_mac", -1 },
		{ "lan_eth", -1 },
		{ "lan_bridge", -1 },
		{ "wan_eth", -1 },
		{ "asp_temp", 7 },
		{ "html_", 5 },
		{ "wlan0_domain", -1 },
		{ "configuration_version", 21},
		{ NULL }
	};

	p = deny_list;
	while (p && p->key) {
		if (p->len == -1 && strcmp(key, p->key) == 0)
			goto out;
		else if (strncmp(key, p->key, p->len) == 0)
			goto out;
		p++;
	}
	ret = 0;
out:
	return ret;
}

static int nvram_parser(FILE *fp, FILE *stream, char *chksum_buf)
{
	char *s, *pl, line[2048], buf[1024];

	while (fscanf(fp, "%[^\n]", line) != EOF) {
		pl = line;
		if ((s = strsep(&pl, "=")) && nvram_deny_check(s) == 0) {
#ifdef NVRAM_CONF_VER_CTL
			//if(strcmp(s, "configuration_version")==0)
			//break;
#endif   
			sprintf(buf, "%s=%s\n", s, pl);
			websWrite(stream, buf);
			strcat(chksum_buf, buf);
		}
		fseek(fp, 1, SEEK_CUR);
	}

	return 0;
}

static int get_file_size(const char *desc)
{
	int fd;
	int size = -1;
	struct stat filestat;

	if ((fd = open(desc, O_RDONLY)) < 0) {
		DEBUG_MSG("%s(%d) open file %s fail\n",
			__FUNCTION__,
			__LINE__,
			desc);
		goto out;
	}

	if (fstat(fd, &filestat) < 0) {
		DEBUG_MSG("%s(%d) fstat fd fail\n",
			__FUNCTION__,
			__LINE__);
		goto fsout;
	}

	size = filestat.st_size;
fsout:
	close(fd);
out:
	return size;
}

int in_cksum_ex(char *data, size_t len, char *crc)
{
	int i = 0;
#if 0
	/* MD5 checksum, need libssl support */
	unsigned char *md5_local = MD5(data, len, NULL);

	for (; i < MD5_DIGEST_LENGTH; i++)
		sprintf(crc, "%s%02X", crc, md5_local[i]);
#else
	sprintf(crc, "%lu", in_cksum((unsigned short *)data, len));
#endif
	return 0;
}

static void do_save_configuration_cgi(char *url, FILE *stream)
{
	FILE *fp;
	int total_size;
	char *chksum_buf, md5_checksum[64];

	assert(url);
	assert(stream);

	nvram_config2file();

	if ((fp = fopen(NVRAM_CONFIG_TMP, "r")) == NULL) {
		printf("%s: file %s open fail\n", __FUNCTION__, NVRAM_CONFIG_TMP);
		return;
	}

	total_size = get_file_size(NVRAM_CONFIG_TMP);
	if ((chksum_buf = (char *)malloc(total_size)) == NULL) {
		printf("%s(%d) malloc fail\n", __FUNCTION__, __LINE__);
		goto out;
	}

	bzero(chksum_buf, total_size);
	nvram_parser(fp, stream, chksum_buf);

	bzero(md5_checksum, sizeof(md5_checksum));
	in_cksum_ex(chksum_buf, strlen(chksum_buf), md5_checksum);
#ifdef NVRAM_CONF_VER_CTL 
/* Add configuration_version, checksum do not include configuration_version*/
	websWrite(stream, "configuration_version=%s\n", nvram_safe_get("configuration_version"));
#endif

	websWrite(stream, "config_checksum=%s\n", md5_checksum);
	websDone(stream, 200);

	free(chksum_buf);
out:
	fclose(fp);
	remove(NVRAM_CONFIG_TMP);

    /* Reset CGI */
    init_cgi(NULL);
}

#ifdef CONFIG_USER_WAN_8021X
static void parse_upload_file(char *url, FILE *stream, int *len, char *boundary) {
    char buf[1024];
    int html_page_type = NO_HTML_PAGE;
    int count_down_time = 0;

    assert(url);
    assert(stream);

    ret_code = EINVAL;
    printf("parse_upload_file\n");

    /* Look for our part */
    while (*len > 0) {
        if (!fgets(buf, MIN(*len + 1, sizeof(buf)), stream))
            return;
        *len -= strlen(buf);

        /* get html_XXX_page info */
        if(html_page_type != NO_HTML_PAGE && parse_upload_header(&count_down_time,&html_page_type,buf) == 0)
            continue;
        /* parse headr */
        if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_page\"")){
            count_down_time = 1;
            html_page_type = HTML_RESPONSE_PAGE;
        }else if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_return_page\"")){
            count_down_time = 1;
            html_page_type = HTML_RETURN_PAGE;
        }else if (!strncasecmp(buf, "Content-Disposition:", 20) &&  strstr(buf, "name=\"file\"")){
            /* write the certification path to the indicate file */
            strcpy(certification_ptr, strstr(buf,"filename="));
            break;
        }
        memset(buf,0,sizeof(buf));
    }

    /* Skip boundary and headers */
    printf("upload  file size = %02d \n", *len);
    while (*len > 0) {
        if (!fgets(buf, MIN(*len + 1, sizeof(buf)), stream))
            return;
        *len -= strlen(buf);
        if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
            break;
    }
}
#endif

#ifdef LINUX_NVRAM
static void do_upload_post(char *url, FILE *stream, int len, char *boundary)
{
    assert(url);
    assert(stream);
    DEBUG_MSG("XXX %s len:%d, boundary[%s]\n", __FUNCTION__, len, boundary);

    ret_code = __do_upload_post(stream, len, boundary);
}
#else
static void do_upload_post(char *url, FILE *stream, int len, char *boundary)
{
    char buf[1024];
    int html_page_type = NO_HTML_PAGE;
    int count_down_time = 0;

    assert(url);
    assert(stream);

    ret_code = EINVAL;
    printf("do_upload_post\n");

    /* Look for our part */

    while (len > 0) {
/*  Date:   2009-04-09
 *  Name:   Yufa Huang
 *  Reason: add HTTPS function.
 */
        if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
            return;
        printf("%i:%s\n",len,buf);

        len -= strlen(buf);

        /* get html_XXX_page info */
        if(html_page_type != NO_HTML_PAGE && parse_upload_header(&count_down_time,&html_page_type,buf) == 0)
            continue;
        /* parse headr */
        if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_page\""))
        {
            count_down_time = 1;
            html_page_type = HTML_RESPONSE_PAGE;
        }
        else if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_return_page\""))
        {
            count_down_time = 1;
            html_page_type = HTML_RETURN_PAGE;
        }
        //      else if (!strncasecmp(buf, "Content-Disposition:", 20) &&   strstr(buf, "name=\"file\""))
        else if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "filename="))
        {
            break;
        }
        memset(buf,0,sizeof(buf));
    }

    /* Skip boundary and headers */
    while (len > 0) {
        printf("%i:%s\n",len,buf);
/*  Date:   2009-04-09
 *  Name:   Yufa Huang
 *  Reason: add HTTPS function.
 */
        if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
            return;
        len -= strlen(buf);
        if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
            break;
    }

    DEBUG_MSG("do_upload_post start: configuration file size = %d\n", len);

    ret_code = upload_configuration(NULL, stream, &len);

    DEBUG_MSG("do_upload_post: ret_code=%d\n", ret_code);
}
#endif  /* ifdef LINUX_NVRAM */


static void do_upload_configuration_cgi(char *url, FILE *stream)
{
    assert(stream);
    assert(url);
    int count_check = 0;

    switch(ret_code)
    {
        case 0: /* upload fail */
            count_check = redirect_count_down_page(stream, F_INITIAL, RESET_COUNT_DOWN, ret_code);
            break;
        case 1: /* upload success */
            count_check = redirect_count_down_page(stream, F_INITIAL, UPLOAD_COUNT_DOWN, ret_code);
            break;
    }

    DEBUG_MSG("do_upload_configuration_cgi: count_check = %d\n", count_check);

    if(!count_check)
        websWrite(stream,redirecter_footer, absolute_path("index.asp"));

    /* Reset CGI   */
    init_cgi(NULL);
}

#ifdef CONFIG_USER_WAN_8021X
static void save_upload_file(FILE *stream,int *len,char *boundary,char *cer_file,int limit_len){
    char *read_buffer;
    int count = 0;
    FILE *fp;
    char *end_boundary;
    int filelen = 0;

    /* Received configuration file via http protocol */
    printf("save upload file size = %02d \n", *len);

    if(*len > (limit_len * 1024)) {
        printf("upload file size is too big \n");
        ret_code = 0;
        return;
    }

    /* read from buffer && save to device */
    read_buffer = (char *) malloc(limit_len*1024);
    fp = fopen(cer_file, "w+");
    count = safe_fread(read_buffer, 1, *len, stream);

    end_boundary = strstr(read_buffer,boundary);

    //count = safe_fwrite(read_buffer,1, *len - strlen(boundary) - 8, fp);  // 8 is for unknow size

    filelen=end_boundary-read_buffer;
    printf("read_buffer-endbound len: %d\n", filelen);

    count = safe_fwrite(read_buffer,1, filelen-4, fp);  // 4 is for the extra boundary bytes (\r\n--)

    printf("Write to tmp file size = %02d\n",count);
    fclose(fp);
    free(read_buffer);
    ret_code = 1;
}

static void save_certification_to_nvram(char *cer_file,char *cer_tag){
    char nvram_name[100];
    char cmd[100];
    int loop = 0;

    /* BCM5352 code */
    /* save data to nvram */
//  memset(nvram_name,0,sizeof(nvram_name));
//  memset(cmd,0,sizeof(cmd));
    /* clean all nvram tag data */
//  for(loop = 0;loop < 100;loop++) {
//      sprintf(cmd,"nvram unset %s_%02d",cer_tag,loop);
//      _system(cmd);
//  }
//  usleep(500);
//  sprintf(cmd,"nvram save %s %s",cer_tag,cer_file);

    /* CONFIG_USER_WAN_8021X code */
    sprintf(cmd,"nvram writefile %s", cer_file);
    _system(cmd);
    usleep(500);
}
#endif

static void do_wireless_update_cgi(char *url, FILE *stream){
    assert(url);
    assert(stream);
    websWrite(stream, (char_t *) wirelsee_driver_update_html, nvram_get("default_html"));
    /* Reset CGI */
    init_cgi(NULL);
}
/* recieve wireless driver */
static void do_wireless_upload_post(char *url, FILE *stream, int len, char *boundary){
#if 0
    char buf[1024];
    int html_page_type = NO_HTML_PAGE;
    int count_down_time = 0;
    char *read_buffer;
    int count = 0;
    FILE *fp;


    assert(url);
    assert(stream);

    ret_code = EINVAL;

    /* Look for our part */
    while (len > 0) {
        if (!fgets(buf, MIN(len + 1, sizeof(buf)), stream))
            return;
        len -= strlen(buf);

        /* get html_XXX_page info */
        if(html_page_type != NO_HTML_PAGE && parse_upload_header(&count_down_time,&html_page_type,buf) == 0)
            continue;
        /* parse headr */
        if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_page\"")){
            count_down_time = 1;
            html_page_type = HTML_RESPONSE_PAGE;
        }else if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_return_page\"")){
            count_down_time = 1;
            html_page_type = HTML_RETURN_PAGE;
        }else if (!strncasecmp(buf, "Content-Disposition:", 20) &&  strstr(buf, "name=\"file\"")){
            break;
        }
        memset(buf,0,sizeof(buf));
    }

    /* Skip boundary and headers */
    while (len > 0) {
        if (!fgets(buf, MIN(len + 1, sizeof(buf)), stream))
            return;
        len -= strlen(buf);
        if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
            break;
    }

    /* Received configuration file via http protocol */

    if(len > (500 * 1024) || len < (100 * 1024)) {
        ret_code = 0;
        return;
    }

    read_buffer = (char *) malloc(500*1024);

    fp = fopen(WIRELESS_DRIVER_TMP_FILE, "w+");
    count = safe_fread(read_buffer, 1, len , stream);
    count = safe_fwrite(read_buffer,1, len - strlen(boundary) - 8 , fp);
    fclose(fp);
    free(read_buffer);

    ret_code = 1;

    /* update wireless driver && re-act*/
    _system("rc stop");
    sleep(5);
    unlink(WIRELESS_DRIVER_KERNEL_FILE);
    _system("cp %s %s ",WIRELESS_DRIVER_TMP_FILE,WIRELESS_DRIVER_KERNEL_PATH);
    _system("rc start");
#endif
    return;
}
static void do_wireless_upload_cgi(char *url, FILE *stream){
    assert(stream);
    assert(url);

    if(redirect_count_down_page(stream,F_INITIAL,UPLOAD_COUNT_DOWN,ret_code) && ret_code){
        fprintf(stderr,"redirect to count down page success and upload success\n");
    }
    else{
        fprintf(stderr,"redirect to count down page fail or upload fail\n");
        websWrite(stream,redirecter_footer, absolute_path("index.asp"));
    }

    /* Reset CGI   */
    init_cgi(NULL);
}

#ifdef BCM5352
static void do_upload_certification_post(char *url, FILE *stream, int len, char *boundary,char *cer_file,char *cer_tag,int limit_len)
{
    parse_upload_file(url,stream,&len,boundary);
    save_upload_file(stream,&len,boundary,cer_file,limit_len);
    save_certification_to_nvram(cer_file,cer_tag);
}

static void do_upload_ca_certification_post(char *url, FILE *stream, int len, char *boundary)
{
    do_upload_certification_post(url,stream,len,boundary,CA_CERTIFICATION,CA_NVRAM_TAG,4);
    //save the path to ca_certification_file
    init_file(EAP_CA_FILE);
    save2file(EAP_CA_FILE,"%s\n",certification_ptr);
}

static void do_upload_client_certification_post(char *url, FILE *stream, int len, char *boundary)
{
    do_upload_certification_post(url,stream,len,boundary,CLIENT_CERTIFICATION,CLIENT_NVRAM_TAG,4);
    //save the path to client_certification_file
    init_file(EAP_CLIENT_FILE);
    save2file(EAP_CLIENT_FILE,"%s\n",certification_ptr);
}

static void do_upload_pkey_certification_post(char *url, FILE *stream, int len, char *boundary)
{
    do_upload_certification_post(url,stream,len,boundary,PRIVATE_KEY,PRIVATE_TAG,5);
    //save the path to pkey_certification_file
    init_file(EAP_PKEY_FILE);
    save2file(EAP_PKEY_FILE,"%s\n",certification_ptr);
}

static void do_upload_certification_cgi(char *url, FILE *stream)
{
    assert(stream);
    assert(url);

    if(redirect_count_down_page(stream,F_INITIAL,UPLOAD_CERTIFICATION_COUNT_DOWN,ret_code) && ret_code){
        fprintf(stderr,"redirect to count down page success and upload success\n");
    }
    else{
        fprintf(stderr,"redirect to count down page fail or upload fail\n");
        websWrite(stream,redirecter_footer, absolute_path("index.asp"));
    }

    /* Reset CGI   */
    init_cgi(NULL);
}
#endif

#ifdef CONFIG_USER_WAN_8021X
static void do_upload_certification_post(char *url, FILE *stream, int len, char *boundary,char *cer_file,char *cer_tag,int limit_len)
{
    parse_upload_file(url,stream,&len,boundary);
    save_upload_file(stream,&len,boundary,cer_file,limit_len);
    save_certification_to_nvram(cer_file,cer_tag);
}

static void do_upload_ca_certification_post(char *url, FILE *stream, int len, char *boundary)
{
    do_upload_certification_post(url,stream,len,boundary,CA_CERTIFICATION,CA_NVRAM_TAG,4);
    //save the path to ca_certification_file
    init_file(EAP_CA_FILE);
    save2file(EAP_CA_FILE,"%s\n",certification_ptr);
    rc_action("all");
}

static void do_upload_client_certification_post(char *url, FILE *stream, int len, char *boundary)
{
    do_upload_certification_post(url,stream,len,boundary,CLIENT_CERTIFICATION,CLIENT_NVRAM_TAG,4);
    //save the path to client_certification_file
    init_file(EAP_CLIENT_FILE);
    save2file(EAP_CLIENT_FILE,"%s\n",certification_ptr);
    rc_action("all");
}

static void do_upload_pkey_certification_post(char *url, FILE *stream, int len, char *boundary)
{
    do_upload_certification_post(url,stream,len,boundary,PRIVATE_KEY,PRIVATE_TAG,5);
    //save the path to pkey_certification_file
    init_file(EAP_PKEY_FILE);
    save2file(EAP_PKEY_FILE,"%s\n",certification_ptr);
    rc_action("all");
}

static void do_upload_certification_cgi(char *url, FILE *stream)
{
    assert(stream);
    assert(url);

//  if(redirect_count_down_page(stream,F_INITIAL,UPLOAD_CERTIFICATION_COUNT_DOWN,ret_code) && ret_code){
    if(websWrite(stream,redirecter_footer, absolute_path("wan_dhcp.asp"))) {
        fprintf(stderr,"redirect to count down page success and upload success\n");
    } else{
        fprintf(stderr,"redirect to count down page fail or upload fail\n");
        websWrite(stream,redirecter_footer, absolute_path("index.asp"));
    }

    /* Reset CGI   */
    init_cgi(NULL);
}

#endif

static void do_refresh_log_email_cgi(char *url, FILE *stream)
{
    char *html_response_page = websGetVar(stream, "html_response_page", "");
    assert(stream);
    assert(url);

    //system("logread | tac > /var/log/messages");
    system("cat /var/log/messages_bak | tac > /var/log/messages");
    websWrite(stream,redirecter_footer, absolute_path(html_response_page));

    /* Reset CGI   */
    init_cgi(NULL);
}

static void do_log_email_button_clicked_set0_cgi(char *url, FILE *stream)
{
    char *html_response_page = websGetVar(stream, "html_response_page", "");
    assert(url);
    assert(stream);
    nvram_set("log_email_button_clicked","0");
    system("nvram writefile /etc/nvram");
    websWrite(stream,redirecter_footer, absolute_path(html_response_page));
        /* Reset CGI   */
    init_cgi(NULL);
}

static void do_send_log_email_cgi(char *url, FILE *stream)
{
    char *html_response_page = websGetVar(stream, "html_response_page", "");
    //char *sender = websGetVar(stream, "log_email_sender", "");
    //char *server = websGetVar(stream, "log_email_server", "");
    int index;
    assert(url);
    assert(stream);
    nvram_set("log_email_button_clicked","1");

/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 ||
            nvram_match("sp_admin_username", auth_login[index].curr_user) ==0 )
#else
    if( nvram_match("admin_username", auth_login.curr_user) ==0 ||
        nvram_match("sp_admin_username", auth_login.curr_user) ==0 )
#endif
#else//HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("apply_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 )
#else
    if(nvram_match("admin_username", auth_login.curr_user) ==0 )
#endif
#endif//HTTPD_USED_SP_ADMINID
    {
        set_basic_api(stream);
    }

    /*NickChou, send signal to mailosd to call send_log_by_smtp,
      GUI can response more fast
      send_log_by_smtp();
      */
    system("killall -SIGUSR1 mailosd &");

    /* NickChou marked, to prevent net_puts error in SMTP
       syslog(LOG_INFO, "Sending log mail");
       */

    nvram_set("html_response_message","Send log message Success");

    websWrite(stream,redirecter_footer, absolute_path(html_response_page));

    /* Reset CGI */
    init_cgi(NULL);
}

#ifdef MV88F6281
static void do_upgrade_post(char *url, FILE *stream, int len, char *boundary)
{
    assert(url);
    assert(stream);
    fprintf(stderr, "len:%d, boundary[%s]\n", len, boundary);

    ret_code = __do_upgrade_post(stream, len, boundary);
    return;
}
#else
static void do_upgrade_post(char *url, FILE *stream, int len, char *boundary)
{
    char buf[1024] = {};
    int html_page_type = NO_HTML_PAGE;
    int count_down_time = 0;

    assert(url);
    assert(stream);

#ifdef MPPPOE
    _system("rc stop");
    sleep(10);
    _system("killall syslogd");
    _system("killall klogd");
    _system("killall gpio");
    _system("killall -9 rc");
    _system("killall wantimer");
    _system("killall -9 lld2d");
    _system("killall -9 wsccmd");
    _system("killall -9 hostapd");
#endif
    ret_code = EINVAL;
    DEBUG_MSG("do_upgrade_cgi\n");

    /* Look for our part */
    while (len > 0) {
/*  Date:   2009-04-09
 *  Name:   Yufa Huang
 *  Reason: add HTTPS function.
 */
        if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
            return;
        len -= strlen(buf);

        /* get html_XXX_page info */
        if(html_page_type != NO_HTML_PAGE && parse_upload_header(&count_down_time,&html_page_type,buf) == 0)
            continue;

        /* parse header */
        if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_page\"")){
            count_down_time = 1;
            html_page_type = HTML_RESPONSE_PAGE;
        }else if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_return_page\"")){
            count_down_time = 1;
            html_page_type = HTML_RETURN_PAGE;
        }else if (!strncasecmp(buf, "Content-Disposition:", 20) &&  strstr(buf, "name=\"file\"")){
            break;
        }
        memset(buf,0,sizeof(buf));
    }

    /* Skip boundary and headers */
    while (len > 0) {
/*  Date:   2009-04-09
 *  Name:   Yufa Huang
 *  Reason: add HTTPS function.
 */
        if (!wfgets(buf, MIN(len + 1, sizeof(buf)), stream))
            return;
        len -= strlen(buf);
        if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
            break;
    }

    /* count down value should be setted before upgrade */
    DEBUG_MSG("UPGRADE_COUNT_DOWN=%s\n", UPGRADE_COUNT_DOWN);
    nvram_set("countdown_time",UPGRADE_COUNT_DOWN);
    nvram_commit();

    printf("====================filesize:: %d\n", len);
    ret_code = sys_upgrade(NULL, stream, &len);

    read_fw_success=ret_code;
    printf("read_fw_success=%d\n",read_fw_success);
    printf("do_upgrade_post finished (%d)\n", ret_code);
}
#endif //MV88F6281



#ifdef MIII_BAR
static void do_miii_uploadFile_post(char *url, FILE *stream, int len, char *boundary)
{
				    char buf[1024] = {};
    				int html_page_type = NO_HTML_PAGE;
    				int count_down_time = 0;

					    assert(url);
					    assert(stream);
}
#endif

static void pre_process_fileupload(FILE *stream, int *len){
	char buf[1024] = {};
	int html_page_type = NO_HTML_PAGE;
	int count_down_time = 0;

	DEBUG_MSG("do_upgrade_cgi\n");

	/* Look for our part */
    while (*len > 0) {
        if (!fgets(buf, MIN(*len + 1, sizeof(buf)), stream))
			return;
        *len -= strlen(buf);

		/* get html_XXX_page info */
		if(html_page_type != NO_HTML_PAGE && parse_upload_header(&count_down_time,&html_page_type,buf) == 0)
			continue;

		/* parse header */
		if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_page\"")){
			count_down_time = 1;
			html_page_type = HTML_RESPONSE_PAGE;
		}else if (!strncasecmp(buf, "Content-Disposition:", 20) && strstr(buf, "name=\"html_response_return_page\"")){
			count_down_time = 1;
			html_page_type = HTML_RETURN_PAGE;
        }else if (!strncasecmp(buf, "Content-Disposition:", 20) &&  strstr(buf, "filename=")){
			break;
		}
		memset(buf,0,sizeof(buf));
	}
	/* Skip boundary and headers */
    while (*len > 0) {
        if (!fgets(buf, MIN(*len + 1, sizeof(buf)), stream))
			return;
        *len -= strlen(buf);
		if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
			break;
	}
}

#ifdef CONFIG_LP
static void do_lp_upgrade_post(char *url, FILE *stream, int len, char *boundary)
{
    DEBUG_MSG("do_lp_upgrade_post\n");
    check_http_url_request(url,stream);
    pre_process_fileupload(stream, &len);
    printf("========== file size:: %d\n", len);
	ret_code = lp_upgrade(NULL, stream, &len);
	printf("do_lp_upgrade_post finished\n");
}

static void do_lp_clear_cgi(char *url, FILE *stream)
{
    assert(url);
    assert(stream);
    int count_check = 0;

    ret_code = lp_clear(NULL, stream);
    count_check = redirect_count_down_page(stream,F_REBOOT,REBOOT_COUNT_DOWN,1);
    //sys_reboot();
}
#endif

static void do_firmware_upgrade_cgi(char *url, FILE *stream)
{
    assert(stream);
    assert(url);
    int count_check = 0;
    int count_down = 0;
    char buf[256] = {0};

    switch(ret_code){
        case FIRMWARE_UPGRADE_SUCCESS:
            count_check =  redirect_count_down_page(stream,F_UPGRADE,UPGRADE_COUNT_DOWN,ret_code);
            break;
        case FIRMWARE_UPGRADE_LOADER_SUCCESS:
        	DEBUG_MSG("UPGRADE_COUNT_DOWN=%s\n", UPGRADE_COUNT_DOWN);
        	count_down = atoi(UPGRADE_COUNT_DOWN);
        	DEBUG_MSG("count_down=%d\n", count_down);
        	sprintf(buf,"%d",count_down/2);
        	DEBUG_MSG("count_down buf=%s\n", buf);
            count_check =  redirect_count_down_page(stream,F_UPGRADE,buf,ret_code);
            break;
        case FIRMWARE_UPGRADE_FAIL:
#ifdef CONFIG_LP
        case LP_UPGRADE_SUCCESS:
#endif
            count_check =  redirect_count_down_page(stream,F_UPGRADE,REBOOT_COUNT_DOWN,ret_code);
            break;
        case FIRMWARE_SIZE_ERROR:
            count_check =  redirect_count_down_page(stream,F_UPGRADE,RESET_COUNT_DOWN,ret_code);
            break;
#ifdef CONFIG_LP
        case LP_UPGRADE_FAIL:
            nvram_set("html_response_page", "ustatus.asp");
            count_check =  redirect_count_down_page(stream,F_UPGRADE,UPGRADE_COUNT_DOWN,ret_code);
            break;
#endif                       
    }

    /* redirect count down page error and back to index.asp */
    if(!count_check)
        websWrite(stream,redirecter_footer, absolute_path("index.asp"));

}

static void do_check_new_firmware_cgi(char *url, FILE *stream){
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(stream,redirecter_footer, absolute_path(html_response_page));
}

/*
 * is_valid_subnet_mask()
 *  Verify that a subnet mask is valid.
 */
static int is_valid_subnet_mask(unsigned long mask)
{
    if (!mask || (mask == 0xffffffff)) {
        return 0;
    }

    /*
     * Ensure that we don't have any more significant zero bits after the least
     * significant one bit.
     */
    mask = (~mask) + 1;
    if ((mask & (-mask)) != mask) {
        return 0;
    }
    return 1;
}

/*
 * is_valid_host_ip
 *  Verify that an IP address is valid.
 */
static int is_valid_host_ip(unsigned long ip)
{
    /*
     * Don't allow non-unicast or loopback IP addresses.
     */
    return ((ip != 0) && (ip < 0xe0000000) && ((ip & 0x7f000000) != 0x7f000000));
}

/*
 * is_valid_host_ip_with_subnet
 *  Verify that an IP address is valid.
 */
int is_valid_host_ip_with_subnet(unsigned long ip, unsigned long mask)
{
    if (!is_valid_host_ip(ip)) {
        return 0;
    }

    /*
     * Network address.
     */
    if ((ip & mask) == ip) {
        return 0;
    }

    /*
     * Broadcast address.
     */
    unsigned long wildcard = ~mask;
    if ((ip & wildcard) == wildcard) {
        return 0;
    }
    return 1;
}

/*
 * is_valid_network_ip_with_subnet
 *  Verify that an IP network address is valid.
 */
static int is_valid_network_ip_with_subnet(unsigned long ip, unsigned long mask)
{
    if (!is_valid_host_ip(ip)) {
        return 0;
    }

    /*
     * Mask must be valid (may also be a network of one host)
     */
    if (!is_valid_subnet_mask(mask) && (mask != 0xffffffff)) {
        return 0;
    }

    /*
     * Must not be to a host (except where mask defines a single host).
     */
    if ((ip & mask) != ip) {
        return 0;
    }

    return 1;
}

/*
 * ip_get_netural_ip_netmask()
 *      Returns the natural NETmask of the given address.
 */
unsigned long ip_get_natural_ip_netmask(unsigned long addr)
{
    /*
     * Special default address?
     */
    if (!addr) {
        return 0;
    }

    /*
     * Special broadcast?
     */
    if (addr == 0xffffffff) {
        return 0xffffffff;
    }

    unsigned char msbyte = (unsigned char)(addr >> 24);

    /*
     * Check if class A.
     */
    if ((msbyte & 0x80) == 0) {
        return 0xff000000;
    }

    /*
     * Check if class B.
     */
    if ((msbyte & 0xc0) == 0x80) {
        return 0xffff0000;
    }

    /*
     * Check if class C.
     */
    if ((msbyte & 0xe0) == 0xc0) {
        return 0xffffff00;
    }

    /*
     * Check if class D.
     */
    if ((msbyte & 0xf0) == 0xe0) {
        return 0xf0000000;
    }

    /*
     * Don't know which class!
     */
    return 0;
}

static int system_time_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *set_time = websGetVar(wp, "date", "");
    char *return_page = websGetVar(wp, "html_response_return_page", "");

#ifdef AP_NOWAN_HASBRIDGE
    time_t time_new;
    unsigned long time_diff;
    char set_time_tmp[14];

    /* set_time from gui format  : xxxxxxxxxxxxx (len=13, micro sec) */
    /* time_new from linux format: xxxxxxxxxx    (len=10, sec) */
    time(&time_new);
    snprintf(set_time_tmp,11,"%s",set_time);
    DEBUG_MSG("system time(sec) from gui    : %s\n",set_time_tmp);
    DEBUG_MSG("system time(sec) from device : %d\n",time_new);
    time_diff = strtoul(set_time_tmp,NULL,10) - time_new;
    DEBUG_MSG("system time(sec) difference  : %10d\n",time_diff);
    memset(set_time_tmp,0,sizeof(set_time_tmp));
    sprintf(set_time_tmp,"%d%s",time_diff,"000");
    nvram_set("system_time_difference",set_time_tmp);
    nvram_commit();
    do_only_once = 1;
#else
    _system("date -s %s ",set_time);
#endif
    websWrite(wp,redirecter_footer, absolute_path(return_page));

    return 1;
}

static void do_system_time_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    system_time_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int wlan_config_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    unsigned char cmd_buf[128];
#ifdef CONFIG_USER_WCN //for MP
    struct stat usb_status;
    char usb_device[] = "/proc/scsi/usb-storage/";
    int usb_attached=0, sd_attached=0;
    char *usb_type = nvram_safe_get("usb_type");
#ifdef IP8000
		char card_reader_dev[4];
		char attached[4];
		int card_reader_stat=0; //get card reader dev - yes:1 no:0
		int sd_gpio=1; //sd detect gpio - on:0  off:1
#else
    FILE *fp_mount_info;
#endif //IP8000
    char mount_info[256];
#endif
    FILE *fp, *fp_wps;
    unsigned char wps_enable[5];

    _system("iwconfig %s > /var/wlan_config.txt", WLAN0_ETH);

    fp = fopen("/var/wlan_config.txt", "r");
    if (fp == NULL)
        return 1;

    while(!feof(fp))
    {
        if (fgets(cmd_buf, 128, fp))
        websWrite(wp, "%s", cmd_buf);
    }

    fclose(fp);

    //show_wps_push_button_status
    //system("cat /proc/sys/dev/ath/wps_enable > /var/wps_tmp");
    fp_wps = fopen("/var/tmp/wps_tmp", "r");
    if (fp_wps == NULL)
        websWrite(wp, "wps_enable=0 ");
    else
    {
        while(!feof(fp_wps))
        {
            if(fgets(wps_enable, 5, fp_wps))
                websWrite(wp, "wps_enable=%s", wps_enable);
        }

        fclose(fp_wps);
    }

#ifdef CONFIG_USER_WCN //for MP
    if(strcmp("0", usb_type)==0) //Using Silex USB Utility
    {
#ifdef USE_SILEX
        /* create in drivers\deviceserver-1.1.1\sxuptp_generic.c*/
        MP_DEBUG_MSG("MP:USB Attached Detect: using Silex\n");
#endif //USE_SILEX

#ifdef USE_KCODES
        /* create in NetUSB.ko by Kcodes RD*/
        printf("USB Attached Detect: using Kcodes\n");
#endif
#ifdef USE_SILEX
#ifdef IP8000
				system("/usr/bin/sxstorageinfo -c&");
				MP_DEBUG_MSG("MP:Get Card Reader Dev\n");
				sleep(1);
	
				fp = fopen("/var/tmp/cardreader_dev", "r");
				if(fp == NULL)
				{
    			MP_DEBUG_MSG("MP:open /var/tmp/cardreader_dev fail\n");
    			card_reader_stat=0;
				}
				else
				{
    			while(!feof(fp))
    			{
        		if(fgets(card_reader_dev, 4, fp))
							break;
    			}
    			fclose(fp);
    			if(strncmp(card_reader_dev, "sd", 2)==0)
    				card_reader_stat=1;
    			else
    				card_reader_stat=0;
				}
				MP_DEBUG_MSG("MP:Card Reader Dev on (%s), stat (%d)\n", card_reader_dev, card_reader_stat);

				fp = fopen("/sys/class/gpio/gpio155/value", "r");
				if(fp == NULL)
				{
					MP_DEBUG_MSG( "MP:open /sys/class/gpio/gpio155/value fail\n");
					sd_gpio=1;
				}
				else
				{
					while(!feof(fp))
    			{
        		if(fgets(attached, 5, fp))
            	sd_gpio=atoi(attached);
    			}
    			fclose(fp);
				}
				MP_DEBUG_MSG("MP:SD card detect gpio on:0 off:1 (%d)\n", sd_gpio);

				fp = fopen("/proc/mounts", "r");
				while(fgets(mount_info, sizeof(mount_info), fp)!= NULL)
				{
					if(strstr(mount_info, card_reader_dev) && (sd_gpio == 0) && (card_reader_stat==1))
					{
						sd_attached = 1;
						MP_DEBUG_MSG("MP:SD attached\n");
						continue;
					}
					else if(strstr(mount_info, "/tmp/sd"))
					{
						usb_attached = 1;
						MP_DEBUG_MSG("MP:USB attached\n");
						continue;
					}
				}
				fclose(fp);

#else
				fp_mount_info = fopen("/proc/mounts", "r");
				while(fgets(mount_info, sizeof(mount_info), fp_mount_info)!= NULL)
				{
					if(strstr(mount_info, "/tmp/mm"))
					{
						sd_attached = 1;
					}
					else if(strstr(mount_info, "/tmp/sd"))
					{
						usb_attached = 1;
					}
				}
				fclose(fp_mount_info);
#endif //IP8000
#endif //USE_SILEX
    }
        /*  Date: 2009-01-22
    *   Name: Nick Chou
    *   Reason: for usb-3g MP
    *   Note: add usb_type=2
    */
    else if(strcmp("1", usb_type)==0 || strcmp("2", usb_type)==0) //Using WCN_UFD or 3G USB Adapter
    {
    if(stat(usb_device,&usb_status) ==0)
    {
        usb_attached=1;
            //printf("usb_config_cgi: %s Plug in\n", usb_device);
        }
        else
        {
            usb_attached=0;
            //printf("usb_config_cgi: %s don't Plug in\n", usb_device);
        }
    }
    else
    {
        printf("nvram does not has usb_type item\n");
        usb_attached=0;
    }

    websWrite(wp, "usb_connect=%d ", usb_attached);
    websWrite(wp, "sd_connect=%d ", sd_attached);
#endif

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf,"/sys/class/gpio/gpio%d/value", PUSH_BUTTON);
    websWrite(wp, "reset_button=%d ", read_pid(cmd_buf));
    
    return 1;
}

void get_ath_macaddr(char *strMac)
{
    FILE *fp;
    unsigned char mac[6];
    fp = fopen(SYS_CAL_MTD, "r");
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
    if(!fp)
    {
#ifdef CONFIG_LP
			fp = fopen(SYS_LP_MTD, "r");
			if(!fp)
#endif
    	return;
  	}
    fseek(fp, ATH_MAC_OFFSET, SEEK_SET);
    fread(mac, 1, 6, fp);
    sprintf(strMac,"%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    fclose(fp);
}

#ifdef USER_WL_ATH_5GHZ
void get_ath_5g_macaddr(char *strMac)
{
    char *p_lan_mac;
    int  lan_mac[6],i=0;

#ifdef SET_GET_FROM_ARTBLOCK
    if (artblock_get("lan_mac"))
        p_lan_mac = artblock_safe_get("lan_mac");
    else
#endif    	
        p_lan_mac = nvram_safe_get("lan_mac");

    sscanf(p_lan_mac,"%02x:%02x:%02x:%02x:%02x:%02x",&lan_mac[0],&lan_mac[1],&lan_mac[2],&lan_mac[3],&lan_mac[4],&lan_mac[5]);

    if(lan_mac[5] == 0xff) i=1;
    if ((lan_mac[5] != 0xff)&&(lan_mac[5] != 0xfe))
        sprintf(strMac, "%02X:%02X:%02X:%02X:%02X:%02X",lan_mac[0],lan_mac[1],lan_mac[2],lan_mac[3],lan_mac[4],lan_mac[5]+2);
    else if (lan_mac[4] != 0xff)
        sprintf(strMac, "%02X:%02X:%02X:%02X:%02X:%02X",lan_mac[0], lan_mac[1], lan_mac[2], lan_mac[3], lan_mac[4]+1, i);
    else if (lan_mac[3] != 0xff)
        sprintf(strMac, "%02X:%02X:%02X:%02X:%02X:%02X",lan_mac[0],lan_mac[1],lan_mac[2],lan_mac[3]+1, 0, i);
    else if (lan_mac[2] != 0xff)
        sprintf(strMac, "%02X:%02X:%02X:%02X:%02X:%02X",lan_mac[0],lan_mac[1],lan_mac[2]+1, 0, 0, i);
    else if (lan_mac[1] != 0xff)
        sprintf(strMac, "%02X:%02X:%02X:%02X:%02X:%02X",lan_mac[0],lan_mac[1]+1, 0, 0, 0, i);
    else
        sprintf(strMac, "%02X:%02X:%02X:%02X:%02X:%02X",lan_mac[0]+1, 0, 0, 0, 0, i);
}
#endif

/* jimmy added 20080401 , Dlink Hidden Page V1.01 support */
/* NickChou added 20090331 , Dlink Hidden Page V1.03 support */
static void do_version_cgi(char *url, FILE *stream){
    assert(url);
    assert(stream);
    char *p_chklst = NULL;
    p_chklst = (char*)malloc(512);
    if (!p_chklst)
        return;
    memset(p_chklst,0,512);
    print_version( p_chklst );
    websWrite(stream, "%s", p_chklst);
    free(p_chklst);
    /* Reset CGI */
    init_cgi(NULL);
}

#define CHKLST_SIZE 1024

//#define VER_FIRMWARE_LEN 7
/*
    Note:
    When add new item, please check if total length longer than CHKLST_SIZE
*/
static void do_chklst_cgi(char *url, FILE *stream){
    	assert(url);
    	assert(stream);
    	/* Parse path */
    	char *p_chklst = NULL;
	char *p_chklst_tmp = NULL;
	p_chklst = (char*)malloc(CHKLST_SIZE);
    	if (!p_chklst)
        	return;
    	memset(p_chklst,0,CHKLST_SIZE);
    	p_chklst_tmp = p_chklst;
	print_chklst( p_chklst_tmp );

	if(p_chklst_tmp > p_chklst + CHKLST_SIZE){
        	printf("httpd: line %d, Error, chklst length overflow !!!",__LINE__);
    	}
    	websWrite(stream, "%s", p_chklst);
    	free(p_chklst);
    	/* Reset CGI */
    	init_cgi(NULL);
}


#ifdef OPENDNS


static void do_parentalcontrols_cgi(char *url, FILE *stream){
    	assert(url);
    	assert(stream);

		DEBUG_MSG("%s\n",url);
		DEBUG_MSG("%s\n",stream);
		
#if 0
    	/* Parse path */
    	char *p_chklst = NULL;
	char *p_chklst_tmp = NULL;
	p_chklst = (char*)malloc(CHKLST_SIZE);
    	if (!p_chklst)
        	return;
    	memset(p_chklst,0,CHKLST_SIZE);
    	p_chklst_tmp = p_chklst;
	print_chklst( p_chklst_tmp );

	if(p_chklst_tmp > p_chklst + CHKLST_SIZE){
        	printf("httpd: line %d, Error, chklst length overflow !!!",__LINE__);
    	}
    	websWrite(stream, "%s", p_chklst);
    	free(p_chklst);
#endif
    	/* Reset CGI */
    	init_cgi(NULL);
}

#endif




/*
 	Date: 2010-01-26
 	Name: Jerry Chen
 	Reason: add this page for MP pin-code use.
 	Notice: None
*/
static void do_f2info_cgi(char *url, FILE *stream) {
    assert(url);
    assert(stream);

	/* Parse path */
    FILE *fp_pin;

    char default_pin[32];
   	char cmd[256];
   	int check_lp=0;
/*
 * Reason: Let MP smooth.
 * Modified: John Huang
 * Date: 2010.02.09
 */
#if 0
    char pin_code[9];
    char *str_main = NULL, *str_sub = NULL;

    system("cat /var/tmp/wps_default_pin.txt > /var/mp_default_pin.txt");

    memset(default_pin, 0, 32);
    fp_pin = fopen("/var/mp_default_pin.txt", "r");
    if (fp_pin)
    {
        while(!feof(fp_pin))
        {
            if (fgets(default_pin, 32, fp_pin))
            {
            	  str_main= strtok(default_pin,"=");
            	  str_sub = strtok(NULL,".");
            	  sprintf(str_sub,"%c%c%c%c%c%c%c%c",str_sub[5],str_sub[6],str_sub[7],str_sub[8],str_sub[1],str_sub[2],str_sub[3],str_sub[4]);
                websWrite(stream, "%s: %s\n", str_main, str_sub);
        	  }
        }
        fclose(fp_pin);
    }
    else
    {
        websWrite(stream, "wps_default_pin = FFFFFFFF\n");
    }

    system("rm /var/tmp/wps_default_pin.txt");
#endif
#ifdef UBICOM_MP_SPEED
    memset(default_pin, 0, 32);
    fp_pin = fopen("/var/tmp/mp_cmd.tmp", "r");
    if (fp_pin)
    {
        if(!feof(fp_pin))
        {
            if (fgets(default_pin, 32, fp_pin))
            {
        	websWrite(stream, "mp_testing_cmd = %c\n",default_pin[0]);
      	    }
      	    else
      	        websWrite(stream, "mp_testing_cmd = #\n");
        }
        fclose(fp_pin);
    }
    else
    {
        websWrite(stream, "mp_testing_cmd = #\n");
    }

    memset(default_pin, 0, 32);
    fp_pin = fopen("/var/tmp/rc_idle.tmp", "r");
    if (fp_pin)
    {
        if(!feof(fp_pin))
        {
            if (fgets(default_pin, 32, fp_pin))
            {
        	websWrite(stream, "mp_rc_idle = %c\n",default_pin[0]);
      	    }
      	    else
      	        websWrite(stream, "mp_rc_idle = #\n");
        }
        fclose(fp_pin);
    }
    else
    {
        websWrite(stream, "mp_rc_idle = #\n");
    }
#endif

#ifdef	CONFIG_LP
    printf("**mp check pre-code mtd5 language-pack \n");
    FILE *fp = NULL;
    fp = fopen("/proc/mtd","r");
    if(fp)
    {
        while(!feof(fp)){
            memset(cmd,'\0',sizeof(cmd));
            fgets(cmd,sizeof(cmd),fp);
            if(strstr(cmd, "language_pack"))
            {
              check_lp |= 1;
            }
            if(strstr(cmd, "mtd5"))
            {
              check_lp |= 2;
            }
        }
        fclose(fp);
    }

    if(check_lp == 3)
        websWrite(stream, "mp_check_language_page = 1\n");
    else
        websWrite(stream, "mp_check_language_page = 0\n");


#endif

    /* Reset CGI */
    init_cgi(NULL);
}

static void do_ip_error_cgi(char *url, FILE *stream){
    assert(url);
    assert(stream);
    websWrite(stream, (char_t *) ip_error_html, "404 Error - File Not Found");
    /* Reset CGI */
    init_cgi(NULL);
}

static void do_wlan_config_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    wlan_config_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int vct_wan_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    // Do not implement this function //
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}
static void do_vct_wan_cgi(char *url, FILE *stream)
{
    // Do not implement this function //
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    vct_wan_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
static int vct_lan_00_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    // Do not implement this function //
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}
static void do_vct_lan_00_cgi(char *url, FILE *stream)
{
    // Do not implement this function //
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    vct_lan_00_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
static int vct_lan_01_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    // Do not implement this function //
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}
static void do_vct_lan_01_cgi(char *url, FILE *stream)
{
    // Do not implement this function //
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    vct_lan_01_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
static int vct_lan_02_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    // Do not implement this function //
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}
static void do_vct_lan_02_cgi(char *url, FILE *stream)
{
    // Do not implement this function //
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    vct_lan_02_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
static int vct_lan_03_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    // Do not implement this function //
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}
static void do_vct_lan_03_cgi(char *url, FILE *stream)
{
    // Do not implement this function //
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    vct_lan_03_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}
static int reset_packet_counter_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
#ifndef MPPPOE
    get_resetting_bytes();
#else
    eByteType tx = TXBYTES;
    eByteType rx = RXBYTES;
    char buf[128] = {};
    //reset average bytes
    init_file(AVERAGE_BYTES);
    save2file(AVERAGE_BYTES,"0/0/0/0/0/0");
    //reset tx bytes
    strcpy(buf,display_lan_bytes(tx,LAN_TX_BYTES));
    init_file(TX_BYTES);
    save2file(TX_BYTES,"%s\n",buf);
    memset(buf,0,128);
    strcpy(buf,display_wan_bytes(tx,WAN_TX_BYTES));
    save2file(TX_BYTES,"%s\n",buf);
    if(nvram_match("wlan0_enable","1") == 0)
    {
        memset(buf,0,128);
        strcpy(buf,display_wlan_bytes(tx,WLAN_TX_BYTES));
        save2file(TX_BYTES,"%s\n",buf);
        reset_wlan_tx_flag = 1;
    }
    reset_lan_tx_flag = 1;
    reset_wan_tx_flag = 1;
    //reset rx bytes
    memset(buf,0,128);
    strcpy(buf,display_lan_bytes(rx,LAN_RX_BYTES));
    init_file(RX_BYTES);
    save2file(RX_BYTES,"%s\n",buf);
    memset(buf,0,128);
    strcpy(buf,display_wan_bytes(rx,WAN_RX_BYTES));
    save2file(RX_BYTES,"%s\n",buf);
    if(nvram_match("wlan0_enable","1") == 0)
    {
        memset(buf,0,128);
        strcpy(buf,display_wlan_bytes(rx,WLAN_RX_BYTES));
        save2file(RX_BYTES,"%s\n",buf);
        reset_wlan_rx_flag = 1;
    }
    reset_lan_rx_flag = 1;
    reset_wan_rx_flag = 1;
    //reset average bytes , reset rc -> apps
    _system("killall rc");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
#endif
    return 1;
}
static void do_reset_packet_counter_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    reset_packet_counter_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int dns_query_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{

    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *dns_query_name = websGetVar(wp, "dns_query_name", "");
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");
    char *countdown_time = websGetVar(wp, "countdown_time", "5");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    nvram_set("html_response_return_page",html_response_return_page);
    nvram_set("html_response_message","Waiting for remote host response...");
    nvram_set("countdown_time",countdown_time);
    _system("nslookup %s > %s",dns_query_name,DNS_QUERY_RESULT);
    return 1;
}

static int do_dns_query_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    dns_query_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int reset_ifconfig_packet_counter_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    get_ifconfig_resetting_bytes();
    return 1;
}
static void do_reset_ifconfig_packet_counter_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    reset_ifconfig_packet_counter_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}


static int save_log_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    int i, total_logs;
    char *p_log;

    total_logs = update_log_table(atoi( nvram_get("log_system_activity")),\
            atoi( nvram_get("log_debug_information")),\
            atoi( nvram_get("log_attacks")),\
            atoi( nvram_get("log_dropped_packets")),\
            atoi( nvram_get("log_notice")));

    p_log = (char *)malloc(512);
    if (!p_log)
        return 0;
    memset(p_log, 0, 512);

    for(i = 0; i < total_logs; i++) {
        sprintf(p_log, "%s\t %s\t %s\n", log_dynamic_table[i].time, log_dynamic_table[i].type, log_dynamic_table[i].message);
        websWrite(wp, "%s", p_log);
        memset(p_log, 0, 512);
    }
    free(p_log);
    websDone(wp, 200);

}

static void do_save_log_cgi(char *url, FILE *stream)
{

    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    save_log_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}



static int log_first_page_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    logs_info.cur_page = 1;

    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;

}

static void do_log_first_page_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    log_first_page_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int log_last_page_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{

    logs_info.cur_page = logs_info.total_page;
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_log_last_page_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    log_last_page_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int log_previous_page_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{

    if(logs_info.cur_page == 1)
        logs_info.cur_page = 1;
    else
        logs_info.cur_page -= 1;

    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_log_previous_page_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    log_previous_page_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

#ifdef IPv6_TEST
static int radvd_enable_disable_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{

    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    if(checkServiceAlivebyName("radvd"))
    {
        _system("killall radvd");
        nvram_set("ipv6_ra_status","off");
    }
    else
    {
        _system("killall radvd");
        _system("radvd -C %s &",RADVD_CONF_FILE);
        nvram_set("ipv6_ra_status","on");
        sleep(1);
        if(checkServiceAlivebyName("radvd") == 0)
            nvram_set("ipv6_ra_status","off");

    }
    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    return 1;
}

static void do_radvd_enable_disable_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    int pid , status;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            radvd_enable_disable_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(5);
#else
            exit(5);
#endif
        default:
            wait(&status);
    }

    nvram_flag_reset();
    /* Reset CGI */
    init_cgi(NULL);
}

static int del_lan_gateway_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *default_gateway = websGetVar(wp, "ipv6_default_gateway", "");
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    _system("route -A inet6 del default gw %s dev %s", default_gateway, nvram_safe_get("lan_bridge"));
    return 1;
}

static void do_del_lan_gateway_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    del_lan_gateway_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int add_lan_gateway_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *default_gateway = websGetVar(wp, "ipv6_default_gateway", "");
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);
    nvram_set("ipv6_default_gateway_l",default_gateway);
    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    _system("route -A inet6 add default gw %s dev %s", default_gateway, nvram_safe_get("lan_bridge"));
    return 1;
}

static void do_add_lan_gateway_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    add_lan_gateway_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int del_wan_gateway_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *default_gateway = websGetVar(wp, "ipv6_default_gateway", "");
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    _system("route -A inet6 del default gw %s dev %s", default_gateway, nvram_safe_get("wan_eth"));
    return 1;
}

static void do_del_wan_gateway_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    del_wan_gateway_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int add_wan_gateway_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *default_gateway = websGetVar(wp, "ipv6_default_gateway", "");
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);
    nvram_set("ipv6_default_gateway_w",default_gateway);
    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    _system("route -A inet6 add default gw %s dev %s", default_gateway, nvram_safe_get("wan_eth"));
    return 1;
}

static void do_add_wan_gateway_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    add_wan_gateway_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int set_wan_link_mtu_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *ipv6_link_mtu = websGetVar(wp, "ipv6_link_mtu", "");
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    _system("ifconfig %s mtu %s", nvram_safe_get("wan_eth"), ipv6_link_mtu);
    return 1;
}

static void do_set_wan_link_mtu_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    set_wan_link_mtu_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int set_lan_link_mtu_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *ipv6_link_mtu = websGetVar(wp, "ipv6_link_mtu", "");
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    _system("ifconfig %s mtu %s", nvram_safe_get("lan_bridge"), ipv6_link_mtu);
    return 1;
}

static void do_set_lan_link_mtu_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    set_lan_link_mtu_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int start_service_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    _system("rc start &");
    _system("dcc&");
    _system("lld2d %s &", nvram_get("lan_bridge"));
    return 1;
}

static void do_start_service_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    int pid , status;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
#if defined(__uClinux__) && !defined(IP8000)
    pid = vfork();
#else
    pid = fork();
#endif

    switch(pid)
    {
        case -1:
            printf("fork error!!\n");
#if defined(__uClinux__) && !defined(IP8000)
            _exit(0);
#else
            exit(0);
#endif
        case 0:
            start_service_cgi(stream, NULL, NULL, 0, url, path, query);
#if defined(__uClinux__) && !defined(IP8000)
            _exit(5);
#else
            exit(5);
#endif
        default:
            wait(&status);
    }
    /* Reset CGI */
    init_cgi(NULL);
}

static int stop_service_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);

    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    _system("rc stop &");
    _system("killall dcc");
    _system("killall lld2d");
    return 1;
}

static void do_stop_service_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    stop_service_cgi(stream, NULL, NULL, 0, url, path, query);
    /* Reset CGI */
    init_cgi(NULL);
}

static int execute_cmd_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");
    char *command = websGetVar(wp, "command", "");
    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);
    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    _system("%s &",command);
    return 1;
}

static void do_execute_cmd_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    execute_cmd_cgi(stream, NULL, NULL, 0, url, path, query);
    /* Reset CGI */
    init_cgi(NULL);
}
static int restart_network_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    char *html_response_message = "Waiting for remote host response...";
    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");
    int lan_mac[6] = {0};
    int wan_mac[6] = {0};
#ifdef SET_GET_FROM_ARTBLOCK
    if(artblock_get("lan_mac"))
        sscanf(artblock_safe_get("lan_mac"),"%02x:%02x:%02x:%02x:%02x:%02x", &lan_mac[0], &lan_mac[1], &lan_mac[2], &lan_mac[3], &lan_mac[4], &lan_mac[5]);
    else
        sscanf(nvram_safe_get("lan_mac"),"%02x:%02x:%02x:%02x:%02x:%02x", &lan_mac[0], &lan_mac[1], &lan_mac[2], &lan_mac[3], &lan_mac[4], &lan_mac[5]);
    if(artblock_get("wan_mac"))
        sscanf(artblock_safe_get("wan_mac"),"%02x:%02x:%02x:%02x:%02x:%02x", &wan_mac[0], &wan_mac[1], &wan_mac[2], &wan_mac[3], &wan_mac[4], &wan_mac[5]);
    else
        sscanf(nvram_safe_get("wan_mac"),"%02x:%02x:%02x:%02x:%02x:%02x", &wan_mac[0], &wan_mac[1], &wan_mac[2], &wan_mac[3], &wan_mac[4], &wan_mac[5]);
#else

    sscanf(nvram_safe_get("lan_mac"),"%02x:%02x:%02x:%02x:%02x:%02x", &lan_mac[0], &lan_mac[1], &lan_mac[2], &lan_mac[3], &lan_mac[4], &lan_mac[5]);
    sscanf(nvram_safe_get("wan_mac"),"%02x:%02x:%02x:%02x:%02x:%02x", &wan_mac[0], &wan_mac[1], &wan_mac[2], &wan_mac[3], &wan_mac[4], &wan_mac[5]);

#endif

    nvram_set("html_response_message",html_response_message);
    nvram_set("html_response_page",html_response_page);
    nvram_set("html_response_return_page",html_response_return_page);
    _system("ip neigh flush dev %s",nvram_safe_get("lan_bridge"));
    _system("ip neigh flush dev %s",nvram_safe_get("wan_eth"));
    _system("route -A inet6 del default gw %s dev %s", nvram_safe_get("ipv6_default_gateway_w"), nvram_safe_get("wan_eth"));
    _system("route -A inet6 del default gw %s dev %s", nvram_safe_get("ipv6_default_gateway_l"), nvram_safe_get("lan_eth"));
    _system("ifconfig %s inet6 del %s",nvram_safe_get("lan_eth"),get_link_local_ip_l("LAN"));
    usleep(100);
    _system("ifconfig %s inet6 del %s",nvram_safe_get("lan_bridge"),get_link_local_ip_l("LAN"));
    usleep(100);
    _system("ifconfig %s inet6 del %s",nvram_safe_get("lan_bridge"),nvram_safe_get("ipv6_static_lan_ip"));
    usleep(100);
    _system("ifconfig %s inet6 del %s",nvram_safe_get("wan_eth"),get_link_local_ip_l("WAN"));
    usleep(100);
    _system("ifconfig %s inet6 del %s",nvram_safe_get("wan_eth"),nvram_safe_get("ipv6_static_wan_ip"));
    usleep(100);
    _system("ifconfig %s down",nvram_safe_get("lan_eth"));
    usleep(500);
    _system("ifconfig %s down",nvram_safe_get("lan_bridge"));
    usleep(500);
    _system("ifconfig %s down",nvram_safe_get("wan_eth"));
    usleep(500);
    _system("ifconfig %s hw ether %02x%02x%02x%02x%02x%02x",nvram_safe_get("lan_eth"),lan_mac[0], lan_mac[1], lan_mac[2], lan_mac[3], lan_mac[4], lan_mac[5]);
    usleep(100);
    _system("ifconfig %s hw ether %02x%02x%02x%02x%02x%02x",nvram_safe_get("wan_eth"),wan_mac[0], wan_mac[1], wan_mac[2], wan_mac[3], wan_mac[4], wan_mac[5]);
    usleep(100);
    _system("ifconfig %s up",nvram_safe_get("lan_bridge"));
    usleep(500);
    _system("ifconfig %s up",nvram_safe_get("lan_eth"));
    usleep(500);
    _system("ifconfig %s up",nvram_safe_get("wan_eth"));
    usleep(500);
    _system("ifconfig %s inet6 add %s",nvram_safe_get("lan_bridge"),nvram_safe_get("ipv6_static_lan_ip"));
    usleep(100);
    _system("ifconfig %s inet6 add %s",nvram_safe_get("wan_eth"),nvram_safe_get("ipv6_static_wan_ip"));
    usleep(100);
    rc_action("lan");
    if (strcmp(nvram_safe_get("wan_proto"), "static"))
        rc_action("wan");
    websWrite(wp,redirecter_footer, absolute_path(html_response_return_page));
    return 1;
}

static void do_restart_network_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;
    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;
    restart_network_cgi(stream, NULL, NULL, 0, url, path, query);
    /* Reset CGI */
    init_cgi(NULL);
}


#endif

static int log_next_page_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{

    logs_info.cur_page += 1;
    if(logs_info.cur_page > logs_info.total_page)
        logs_info.cur_page = logs_info.total_page;

    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_log_next_page_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    log_next_page_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static int log_clear_page_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg,
        char_t *url, char_t *path, char_t *query)
{

    logs_info.cur_page = -1;
    init_file(LOG_FILE_HTTP);

#if 1
/*
 Reason: add clear log function;
         cause one issue(not solved), first syslog will not show, need check lib/libc/misc/syslog_bb.c
 Modified: aaron
 Date: 2010.03.01
*/

//	_system("logread -c");
        init_file(LOG_FILE_BAK);
//	syslog(LOG_INFO, "Log cleared by Administrator");
	logs_info.cur_page = 1;
	logs_info.total_page = 1;
	syslog(LOG_INFO, "Log init");
	syslog(LOG_INFO, "Log cleared by Administrator");
#endif

    char *html_response_page = websGetVar(wp, "html_response_page", "");
    websWrite(wp,redirecter_footer, absolute_path(html_response_page));
    return 1;
}

static void do_log_clear_page_cgi(char *url, FILE *stream)
{
    char *path=NULL, *query=NULL;

    assert(url);
    assert(stream);

    /* Parse path */
    query = url;
    path = strsep(&query, "?") ? : url;

    log_clear_page_cgi(stream, NULL, NULL, 0, url, path, query);

    /* Reset CGI */
    init_cgi(NULL);
}

static void do_usb_test_cgi(char *url, FILE *stream){
    assert(url);
    assert(stream);

    char *html_response_return_page = websGetVar(wp, "html_response_return_page", "");

    /* for test plug-in */
    FILE *usb_plugin = NULL;
    char read_usb_info[512] = {};
    int size = 0, rtn = 0;
    char *usb_plug = "Yes";
    /* for test mount & writable */
    FILE *fp = NULL;
    FILE *usb_write = NULL;
    int usb_write_len = 100;
    char line[1024] = {};
    char const *mnt = "/var/tmp/mnt.txt";
    char const *sda1 = "/dev/sda1";
    char const *usb = "/mnt/usb";
    char const *test = "/mnt/usb/test.txt";
    char *line_found_sda1 = NULL;
    char *line_found_usb = NULL;
    int usb_flag = 0;
    int count_limit = 3;

    /* test usb plug-in success or not */
    while(count_limit > 0) {
        if((usb_plugin = fopen(USB_MASS_STORAGE,"r")) == 0)
            usb_test.result = "Fail : please plug-out & plug-in usb device again";
        else {
            size = fread(read_usb_info,1,512,usb_plugin);
            if(size < 10) {
                usb_test.result = "Fail : please plug-out & plug-in usb device again";
            } else {
                if(strstr(read_usb_info,usb_plug)) { //Plug in Success
                    usb_flag = 1;
                    break;
                    /* Light ON USB LED */
                    //system("gpio USB_LED on");
                } else {
                    usb_test.result = "Fail : please plug-out & plug-in usb device again";
                    /* Light OFF USB LED*/
                    //system("gpio USB_LED off");
                }
            }
            size = 0;
            memset(read_usb_info,0,sizeof(read_usb_info));
            fseek(usb_plugin,0,SEEK_SET);
            fclose(usb_plugin);
        }
        sleep(1);
        count_limit --;
        usb_flag = 0;
    }

    /* plug-in fail */
    if(!usb_flag)
        goto usb_end;

    /* mount usb device */
    _system("umount %s",usb);
    _system("mount %s %s &",sda1,usb);
    sleep(1);

    /* mount result file access */
    fp = fopen(mnt,"w+");
    if(fp == NULL){
        printf("mnt file : %s open error\n",mnt);
        return;
    }

    /* check mount success or fail */
    _system("mount > %s",mnt);
    while(fgets(line,1024,fp)){
        line_found_sda1 = strstr(line,sda1);
        line_found_usb = strstr(line,usb);
        if(line_found_sda1 != NULL || line_found_usb != NULL){
            usb_flag = 1;
            break;
        }
        memset(line,0,1024);
    }

    /* mount error */
    if(!usb_flag){
        printf("mount %s on %s error \n",sda1,usb);
        usb_test.result = "Fail : mount error";
        goto usb_end;
    }

    /* mount success */
    /* create file to test if usb is writable */
    printf("mount %s on %s success \n",sda1,usb);
    usb_write = fopen(test,"w+");
    if(usb_write == NULL){
        printf("test file : %s open error\n",test);
        usb_test.result = "Fail : create file in usb device error";
        goto usb_end;
    }

    while(usb_write_len > 0){
        fputs(line,usb_write);
        usb_write_len-- ;
    }

    /* len = 0 for unwritable */
    fseek(usb_write, 0, SEEK_END);
    usb_write_len = ftell(usb_write);
    if(usb_write_len == 0){
        printf("write to usb fail\n");
        usb_test.result = "Fail : write file in usb device error";
    }else{
        printf("created file size is %i \n",usb_write_len);
        usb_test.result = "Success";
    }

    /* umonut usb device    */
    _system("umount %s",usb);

usb_end:
    if(fp){
        fclose(fp);
        unlink(mnt);
    }
    if(usb_write){
        fclose(usb_write);
        unlink(test);
    }

    /* umonut usb device    */
    umount("/mnt/usb");
    websWrite(stream,redirecter_footer, absolute_path(html_response_return_page));

    if(rtn > 0)
        kill(rtn, SIGUSR2);

}
/* restore nvram tmp default */
static void do_apply_discard_cgi(char *url, FILE *stream){
    assert(stream);
    char *html_response_page = websGetVar(stream, "html_response_page", nvram_safe_get("default_html"));
    /* redirect to correct page */
    websWrite(stream,redirecter_footer, absolute_path(html_response_page));
    /* inital cgi */
    init_cgi(NULL);
}
static void do_apply_discard_post(char *url, FILE *stream, int len, char *boundary){
    assert(url);
    assert(stream);
    discard_nvram_config();
    do_apply_post(url,stream,len,boundary);
}

static void do_login_cgi(char *url, FILE *stream){
    assert(stream);
    char *html_response_page = websGetVar(stream, "html_response_page", "");
    char *login_pass = websGetVar(stream, "login_pass", NULL);
    char *login_name = websGetVar(stream, "login_name", NULL);
    char authName[20];//char authName[250];
    char authPwd[20];//char authPwd[250];
    int len;
#if defined(HTTPD_USED_MUTIL_AUTH)
    int index_multi_auth = 0;
#endif

/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
    char *graph_auth_enable = NULL;
    char *graph_id = websGetVar(stream, "graph_id", NULL);
    char *graph_code = websGetVar(stream, "graph_code", NULL);
    unsigned char GraphID[20];
    unsigned char GraphCode[20];
/*
*   Date: 2009-2-25
*   Name: Albert_chen
*   Reason: Add for support Auth Graph with Gadget
*   Notice :
*/
    unsigned int AuthGraphFlag = 0;
#endif

#ifdef GUI_LOGIN_ALERT
    gui_loginfail_alert=0;
#endif

#ifdef AUTHGRAPH
    graph_auth_enable = nvram_safe_get("graph_auth_enable");
    if(strcmp(graph_auth_enable,"1") == 0){
        memset(GraphID,'\0',sizeof(GraphID));
        memset(GraphCode,'\0',sizeof(GraphCode));

        DEBUG_MSG("(%s, 1st graph_id = [%s], graph_code = [%s])\n",__FUNCTION__,graph_id,graph_code);

        len = b64_decode(graph_id, GraphID, sizeof(GraphID));
        GraphID[len] = '\0';
        len = b64_decode(graph_code, GraphCode, sizeof(GraphCode));
        GraphCode[len] = '\0';

        DEBUG_MSG("(%s, b64_decode GraphID = [%s] GraphCode = [%s])\n",__FUNCTION__,GraphID,GraphCode);

        if (AuthGraph_ValidateIdCodeByStr(GraphID, GraphCode) != 0){
#ifdef GUI_LOGIN_ALERT
            gui_loginfail_alert=2;//html_response_page=login.asp
            websWrite(stream,redirecter_footer, absolute_path(html_response_page));
#else
            websWrite(stream,redirecter_footer, "login_auth_fail.asp");
#endif
            DEBUG_MSG("(%s, Graph Auth failed !)\n",__FUNCTION__);
            init_cgi(NULL);
            return;
        }
        AuthGraphFlag = 1;
    }
#endif
/* ---------------------------------------------------- */
    //  /* inital cgi */
    //DEBUG_MSG("do_login_cgi:login_pass=%s,login_name=%s\n",login_pass,login_name);

    len = b64_decode(login_name, authName, sizeof(authName));
    authName[len] = '\0';
    len = b64_decode(login_pass, authPwd, sizeof(authPwd));
    authPwd[len] = '\0';

    DEBUG_MSG("do_login_cgi:authPwd=%s,len=%d,authName=%s,len=%d\n",authPwd,strlen(authPwd),authName,strlen(authName));
    DEBUG_MSG("do_login_cgi:admin_passwd=%s,admin_userid=%s\n",admin_passwd,admin_userid);
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
    DEBUG_MSG("do_login_cgi:sp_admin_passwd=%s,len=%d,sp_admin_userid=%s,len=%d\n",sp_admin_passwd,strlen(sp_admin_passwd),sp_admin_userid,strlen(sp_admin_userid));
#endif
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("do_login_cgi: MUTIL_AUTH admin_userid=%s,user_userid=%s\n",admin_userid,user_userid);
#endif

    /* Is this the right user and password? */
    /* jimmy added for DIR-400 , don't check account name */
#ifndef AthSDK_AP61
    if ( (strcmp( admin_userid, authName ) == 0 && strcmp( admin_passwd, authPwd ) == 0) ||
#ifdef HTTPD_USED_SP_ADMINID
        (strcmp( sp_admin_userid, authName ) == 0 && strcmp( sp_admin_passwd, authPwd ) == 0) ||
#endif
        (strcmp( user_userid, authName ) == 0 && strcmp( user_passwd, authPwd ) == 0) ){

        if(strcmp( admin_userid, authName ) == 0 && strcmp( admin_passwd, authPwd ) == 0){
#ifdef HTTPD_USED_MUTIL_AUTH
            DEBUG_MSG("do_login_cgi: MUTIL_AUTH is admin_userid ok\n");
            if( (index_multi_auth = auth_login_find(&con_info.mac_addr[0])) < 0 ){
                if( (index_multi_auth = auth_login_search()) < 0){
                    index_multi_auth=auth_login_search_long();
                }
            }
            DEBUG_MSG("do_login_cgi: MUTIL_AUTH index_multi_auth=%d\n",index_multi_auth);
            strcpy(auth_login[index_multi_auth].curr_user,admin_userid);
            strcpy(auth_login[index_multi_auth].curr_passwd,admin_passwd);
            strcpy(auth_login[index_multi_auth].mac_addr,con_info.mac_addr);
            auth_login[index_multi_auth].logintime = time((time_t *)NULL);
            DEBUG_MSG("do_login_cgi: MUTIL_AUTH curr_user=%s,curr_passwd=%s,mac_addr=%s,time=%d\n",
                        auth_login[index_multi_auth].curr_user,auth_login[index_multi_auth].curr_passwd,auth_login[index_multi_auth].mac_addr,auth_login[index_multi_auth].logintime);
#else
            DEBUG_MSG("do_login_cgi: is admin_userid ok\n");
            auth_login.curr_user = admin_userid;
            auth_login.curr_passwd = admin_passwd;
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
        }else{
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
            if(strcmp( sp_admin_userid, authName ) == 0 && strcmp( sp_admin_passwd, authPwd ) == 0){
#ifdef HTTPD_USED_MUTIL_AUTH
                DEBUG_MSG("do_login_cgi: MUTIL_AUTH is sp_admin_userid\n");
                if( (index_multi_auth = auth_login_find(&con_info.mac_addr[0])) < 0 ){
                    if( (index_multi_auth = auth_login_search()) < 0){
                        index_multi_auth=auth_login_search_long();
                    }
                }
                DEBUG_MSG("do_login_cgi: MUTIL_AUTH index_multi_auth=%d\n",index_multi_auth);
                strcpy(auth_login[index_multi_auth].curr_user,sp_admin_userid);
                strcpy(auth_login[index_multi_auth].curr_passwd,sp_admin_passwd);
                strcpy(auth_login[index_multi_auth].mac_addr,con_info.mac_addr);
                auth_login[index_multi_auth].logintime = time((time_t *)NULL);
                DEBUG_MSG("do_login_cgi: MUTIL_AUTH curr_user=%s,curr_passwd=%s,mac_addr=%s,time=%d\n",
                        auth_login[index_multi_auth].curr_user,auth_login[index_multi_auth].curr_passwd,auth_login[index_multi_auth].mac_addr,auth_login[index_multi_auth].logintime);
#else
                 DEBUG_MSG("do_login_cgi: is sp_admin_userid\n");
                 auth_login.curr_user = sp_admin_userid;
                 auth_login.curr_passwd = sp_admin_passwd;
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
             }
             else{
#endif

                DEBUG_MSG("do_login_cgi: is user_userid\n");
#ifdef HTTPD_DENY_USERID
                websWrite(stream,redirecter_footer, absolute_path(html_response_page));
                init_cgi(NULL);
                return;
#endif
#ifdef HTTPD_USED_MUTIL_AUTH
                if( (index_multi_auth = auth_login_find(&con_info.mac_addr[0])) < 0 ){
                    if( (index_multi_auth = auth_login_search()) < 0){
                        index_multi_auth=auth_login_search_long();
                    }
                }
                DEBUG_MSG("do_login_cgi: MUTIL_AUTH index_multi_auth=%d\n",index_multi_auth);
                strcpy(auth_login[index_multi_auth].curr_user,user_userid);
                strcpy(auth_login[index_multi_auth].curr_passwd,user_passwd);
                strcpy(auth_login[index_multi_auth].mac_addr,con_info.mac_addr);
                auth_login[index_multi_auth].logintime = time((time_t *)NULL);
                DEBUG_MSG("do_login_cgi: MUTIL_AUTH curr_user=%s,curr_passwd=%s,mac_addr=%s,time=%d\n",
                        auth_login[index_multi_auth].curr_user,auth_login[index_multi_auth].curr_passwd,auth_login[index_multi_auth].mac_addr,auth_login[index_multi_auth].logintime);
#else
                auth_login.curr_user = user_userid;
                auth_login.curr_passwd = user_passwd;
#endif//#ifdef HTTPD_USED_MUTIL_AUTH
#ifdef HTTPD_USED_SP_ADMINID
            }
#endif
        }
        /* update for login mac */
#ifdef  AUTHGRAPH
        if(strcmp(graph_auth_enable,"1") == 0){
            if (AuthGraphFlag)
                update_logout_list(&con_info.mac_addr[0],ADD_MAC,NULL_LIST);
        }else
#endif
        update_logout_list(&con_info.mac_addr[0],ADD_MAC,NULL_LIST);


#ifdef AP_NOWAN_HASBRIDGE
        if(!do_only_once){
            websWrite(stream,redirecter_footer, "SystemTime.asp");
        }else
#endif
        {
            if (nvram_match("nvram_default_value", "1") == 0) {
            	DEBUG_MSG("do_login_cgi: redirecter_footer setup_wizard.asp\n");
            	websWrite(stream, redirecter_footer, "setup_wizard.asp");
            }
		    else {
                char buf[8] = {0};		    	
                get_internetonline_check(buf);

                if (buf[0] == '1') {
                    DEBUG_MSG("do_login_cgi: redirecter_footer st_device.asp\n");
                    websWrite(stream, redirecter_footer, "st_device.asp");
                }
                else {
                    DEBUG_MSG("do_login_cgi: redirecter_footer default_html\n");
                    websWrite(stream, redirecter_footer, absolute_path(nvram_safe_get("default_html")));
                }    
            }
        }
    }
    else {
#ifdef GUI_LOGIN_ALERT
        gui_loginfail_alert=1;
#endif
        DEBUG_MSG("do_login_cgi: redirecter_footer %s\n", absolute_path(html_response_page));
        websWrite(stream,redirecter_footer, absolute_path(html_response_page));
    }
#else//#ifndef AthSDK_AP61

    if ( (strcmp( admin_userid, authName ) == 0) && (strcmp( admin_passwd, authPwd ) == 0) ){
        auth_login.curr_user = admin_userid;
        auth_login.curr_passwd = admin_passwd;
        /* update for login mac */
#ifdef  AUTHGRAPH
        if(strcmp(graph_auth_enable,"1") == 0)
            if (AuthGraphFlag)
#endif
        update_logout_list(&con_info.mac_addr[0],ADD_MAC,NULL_LIST);
        websWrite(stream,redirecter_footer, absolute_path(nvram_get("default_html")));
    }else{
        websWrite(stream,redirecter_footer, absolute_path(html_response_page));
        DEBUG_MSG("do_login_cgi: redirecter_footer html_response_page\n");
    }
#endif//#ifndef AthSDK_AP61

    init_cgi(NULL);
}


/* logout */
static void do_logout_cgi(char *url, FILE *stream){
    assert(stream);

    /* redirect to correct page */
    //websWrite(stream,redirecter_footer, absolute_path(html_response_page));
    websWrite(stream,redirecter_footer, "login.asp");

    /* inital cgi */
    init_cgi(NULL);
    logout();
}

#ifdef BCM5352
/* check internet connection */
static void do_check_wan_connection_cgi(char *url, FILE *stream){
    assert(stream);
    char *html_response_page = websGetVar(wp, "html_response_page", "");
    int index;
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
    /* Apply values */
#ifdef HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("do_check_wan_connection_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 ||
            nvram_match("sp_admin_username", auth_login[index].curr_user) ==0 )
#else
    if( nvram_match("admin_username", auth_login.curr_user) ==0 ||
        nvram_match("sp_admin_username", auth_login.curr_user) ==0 )
#endif
#else
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("do_check_wan_connection_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 )
#else
    if(nvram_match("admin_username", auth_login.curr_user) == 0 )
#endif
#endif
    {
        DEBUG_MSG("do_check_wan_connection_cgi: set_basic_api\n");
        set_basic_api(stream);
    }

    /* Test internet connection */
    check_internet_connection(websGetVar(stream, "pingto_addr", ""));

    /* redirect to correct page */
    if (html_response_page)
        websWrite(stream,redirecter_footer, absolute_path(html_response_page));
    else
        websWrite(stream,redirecter_footer, absolute_path(nvram_get("default_html")));
}

static void do_apply_config_post(char *url, FILE *stream, int len, char *boundary){
    assert(url);
    assert(stream);
    do_apply_post(url,stream,len,boundary);
    set_nvram_config();
}

/* save value to nvram without rc action and commit */
static int do_apply_config_cgi(char *url, FILE *stream){
    assert(stream);
    char *html_response_page = websGetVar(stream, "html_response_page", nvram_safe_get("default_html"));
    int index;
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
    /* save value only for admin */
#ifdef HTTPD_USED_SP_ADMINID
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("do_apply_config_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 ||
            nvram_match("sp_admin_username", auth_login[index].curr_user) ==0 )
#else
    if( nvram_match("admin_username", auth_login.curr_user) ==0 ||
        nvram_match("sp_admin_username", auth_login.curr_user) ==0 )
#endif
#else
#ifdef HTTPD_USED_MUTIL_AUTH
    DEBUG_MSG("do_apply_config_cgi: MUTIL_AUTH\n");
    if( (index = auth_login_find(&con_info.mac_addr[0])) >= 0 )
        if( nvram_match("admin_username", auth_login[index].curr_user) ==0 )
#else
    if(nvram_match("admin_username", auth_login.curr_user) ==0 )
#endif
#endif
    {
        DEBUG_MSG("do_apply_config_cgi: set_basic_api\n");
        set_basic_api(stream);
    }
    /* redirect to correct page */
    websWrite(stream,redirecter_footer, absolute_path(html_response_page));
    /* inital cgi */
    init_cgi(NULL);
    return 1;
}

static void do_apply_confirm_post(char *url, FILE *stream, int len, char *boundary){
    assert(url);
    assert(stream);
    do_apply_post(url,stream,len,boundary);
    commit_nvram_config();
}
/* nvram commit and rc action for new config */
static int do_apply_confirm_cgi(char *url, FILE *stream){
    assert(stream);
    char *html_response_page = websGetVar(stream, "html_response_page", nvram_safe_get("default_html"));

    /* special case: set default limit ip flag */
    set_limit_connect_init_flag(0);
    /* commit nvram tmp in apply_tmp.cgi */
    nvram_commit();
    /* rc restart for all */
    rc_action("all");
    /* redirect to correct page */
    websWrite(stream,redirecter_footer, absolute_path(html_response_page));
    /* inital cgi */
    init_cgi(NULL);
    return 1;
}

#endif

#ifdef HTTPD_USED_SP_ADMINID
/*
*   Date: 2009-6-09
*   Name: Ken_Chiang
*   Reason: added for spamin pasword generate by wan mac addreses.
*   Notice :
*/
void generate_spamin_pasword_by_wan_mac(char *password)
{
    unsigned long int rnumber = 0;
    unsigned long int pin1 = 0,pin2 = 0;
    unsigned long int accum = 0;
    int i,digit = 0;
    char wan_mac[32]={0};
    char *mac_ptr = NULL;

    //printf("%s:\n",__func__);
    //get mac
#ifdef SET_GET_FROM_ARTBLOCK
    if(artblock_get("wan_mac"))
        mac_ptr = artblock_safe_get("wan_mac");
    else
        mac_ptr = nvram_safe_get("wan_mac");
#else
    mac_ptr = nvram_safe_get("wan_mac");
#endif
    //printf("%s:mac_ptr=%s\n",__func__,mac_ptr);
    strncpy(wan_mac, mac_ptr, 18);
    mac_ptr = wan_mac;
    //remove ':'
    for(i =0 ; i< 5; i++ )
    {
        memmove(wan_mac+2+(i*2), wan_mac+3+(i*3), 2);
    }
    memset(wan_mac+12, 0, strlen(wan_mac)-12);
    //printf("%s:wan_mac=%x%x%x%x%x%x%x%x%x%x%x%x\n",__func__,
            //wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5],
            //wan_mac[6],wan_mac[7],wan_mac[8],wan_mac[9],wan_mac[10],wan_mac[11]);
    sscanf(wan_mac,"%06X%06X",&pin1,&pin2);

    pin2 ^= 0x55aa55;
    pin1 = pin2 & 0x00000F;
    pin1 = (pin1<<4) + (pin1<<8) + (pin1<<12) + (pin1<<16) + (pin1<<20) ;
    pin2 ^= pin1;
    pin2 = pin2 % 10000000; // 10^7 to get 7 number

    if( pin2 < 1000000 )
        pin2 = pin2 + (1000000*((pin2%9)+1)) ;

    //ComputeChecksum
    rnumber = pin2*10;
    accum += 3 * ((rnumber / 10000000) % 10);
    accum += 1 * ((rnumber / 1000000) % 10);
    accum += 3 * ((rnumber / 100000) % 10);
    accum += 1 * ((rnumber / 10000) % 10);
    accum += 3 * ((rnumber / 1000) % 10);
    accum += 1 * ((rnumber / 100) % 10);
    accum += 3 * ((rnumber / 10) % 10);
    digit = (accum % 10);
    pin1 = (10 - digit) % 10;

    pin2 = pin2 * 10;
    pin2 = pin2 + pin1 + 1;
    sprintf( password, "%08d", pin2 );
    //printf("%s:password=%s\n",__func__,password);
    return;
}
#endif

/*
*   Date: 2009-08-10
*   Name: Ken Chiang
*   Reason: Modify for GPL CmoCgi.
*   Notice :
*/
/*
static void do_auth(char *admin_userid, char *admin_passwd, char *user_userid, char *user_passwd, char *realm){
*/
void do_auth(char *admin_userid, char *admin_passwd, char *user_userid, char *user_passwd, char *realm){
    assert(admin_userid);
    assert(admin_passwd);
    assert(user_userid);
    assert(user_passwd);
    assert(realm);
    strcpy(admin_userid, nvram_safe_get("admin_username"));
    strcpy(admin_passwd, nvram_safe_get("admin_password"));
    strcpy(user_userid, nvram_safe_get("user_username"));
    strcpy(user_passwd, nvram_safe_get("user_password"));
/*
*   Date: 2009-4-24
*   Name: Ken Chiang
*   Reason: Added to support the spcial admin used the MAC as the password.
*   Notice :
*/
#ifdef HTTPD_USED_SP_ADMINID
    strcpy(sp_admin_userid, nvram_safe_get("sp_admin_username"));
    generate_spamin_pasword_by_wan_mac(sp_admin_passwd);
#endif

    if (strlen(nvram_safe_get("hostname")) > 1)
        strncpy(realm, nvram_safe_get("hostname"), AUTH_MAX);
    else
        strncpy(realm, nvram_safe_get("model_number"), AUTH_MAX);
}


#ifdef OPENDNS
void do_parentalcontrols(char *path, FILE *stream);

#endif

#ifdef MIII_BAR
void do_miii_getDeviceInfo(char *path, FILE *stream);
void do_miii_listStorages(char *path, FILE *stream);
void do_miii_getFileList(char *path, FILE *stream);
void do_miii_getFile(char *path, FILE *stream);
void do_miii_uploadFile(char *path, FILE *stream);
void do_miii_downloadFile(char *path, FILE *stream);
void do_miii_createDirectory(char *path, FILE *stream);
void do_miii_getRouter(char *path, FILE *stream);
void do_miii_listDevices(char *path, FILE *stream);
void do_miii_deleteFiles(char *path, FILE *stream);
void do_miii_renameFile(char *path, FILE *stream);
void do_miii_copyFilesTo(char *path, FILE *stream);
void do_miii_moveFilesTo(char *path, FILE *stream);
void do_miii_getVersion(char *path, FILE *stream);
void do_miii_uploadTo(char *path, FILE *stream);
void do_miii_updateRid(char *path, FILE *stream);
void do_miii_updateStatusTo(char *path, FILE *stream);
#endif

struct mime_handler mime_handlers[] = {
    { "**.asp", "text/html", no_cache, NULL, do_ej, do_auth },
    { "**.html", "text/html", no_cache, NULL, do_ej, do_auth },
#ifdef OPENDNS
    { "parentalcontrols/**", "text/html", no_cache, NULL, do_parentalcontrols, do_auth },
    { "opendns_ok.asp*", "text/html", no_cache, NULL, do_ej, do_auth },
    { "opendns_okUI.asp*", "text/html", no_cache, NULL, do_ej, do_auth },
    { "dns_reboot.cgi*", "text/html", no_cache, do_apply_post, do_dns_reboot_cgi, do_auth },
#endif

    { "**.htm", "text/html", no_cache, NULL, do_ej, do_auth },
    { "**.css", "text/css", cached, NULL, do_file, do_auth },
    { "**.gif", "image/gif", cached, NULL, do_file, NULL },
    { "**.jpg", "image/jpeg", cached, NULL, do_file, NULL },
    { "**.js", "text/javascript", cached, NULL, do_file, do_auth },
#ifdef MIII_BAR
	{"ws/api/getDeviceInfo", 		"application/javascript", NULL, NULL, do_miii_getDeviceInfo, NULL },
	{"ws/api/listStorages", 		"application/javascript", NULL, NULL, do_miii_listStorages, NULL },
	{"ws/api/getFileList", 		"application/javascript", NULL, NULL, do_miii_getFileList, NULL },
//	{"ws/api/getFile", 		"application/javascript", NULL, NULL, do_miii_getFile, NULL },
	//{"ws/api/uploadFile*", 		"application/javascript", do_miii_uploadFile_post, NULL, do_miii_uploadFile, NULL },
//	{ "firmware_upgrade.cgi*", "text/html", no_cache, do_upgrade_post, do_firmware_upgrade_cgi, do_auth },
//	{ "ws/api/downloadFile*", "application/javascript", no_cache, do_miii_uploadFile_post, do_miii_uploadFile, do_auth },
//	{"ws/api/downloadFile", 	"application/javascript", NULL, NULL, do_miii_downloadFile, NULL },
	{"ws/api/createDirectory", 	"application/javascript", NULL, NULL, do_miii_createDirectory, NULL },
	{"ws/api/getRouter", 		"application/javascript", NULL, NULL, do_miii_getRouter, NULL },
	{"ws/api/listDevices", 		"application/javascript", NULL, NULL, do_miii_listDevices, NULL },
	{"ws/api/deleteFiles", 		"application/javascript", NULL, NULL, do_miii_deleteFiles, NULL },
	{"ws/api/renameFile", 		"application/javascript", NULL, NULL, do_miii_renameFile, NULL },
	{"ws/api/copyFilesTo", 		"application/javascript", NULL, NULL, do_miii_copyFilesTo, NULL },
	{"ws/api/moveFilesTo", 		"application/javascript", NULL, NULL, do_miii_moveFilesTo, NULL },
	{"ws/api/getVersion", 		"application/javascript", NULL, NULL, do_miii_getVersion, NULL },
	{"ws/api/uploadTo", 		"application/javascript", NULL, NULL, do_miii_uploadTo, NULL },
	{"ws/api/updateRid", 		"application/javascript", NULL, NULL, do_miii_updateRid, NULL },
	{"ws/api/updateStatusTo", 	"application/javascript", NULL, NULL, do_miii_updateStatusTo, NULL },
#endif
    { "**.xsl", "text/xml", NULL, NULL, do_file, do_auth },
    { "default_classifier_state.xml*", "text/xml", no_cache, NULL, do_file, do_auth },
    { "apply.cgi*", "text/html", no_cache, do_apply_post, do_apply_cgi, do_auth },
    { "wizard_apply.cgi*", "text/html", no_cache, do_apply_post, do_wizard_apply_cgi, NULL },
    { "wizard_cancel.cgi*", "text/html", no_cache, do_apply_post, do_wizard_cancel_cgi, NULL },    
    { "dhcp_release.cgi*", "text/html", no_cache, do_apply_post, do_dhcp_release_cgi, do_auth },
    { "dhcp_renew.cgi*", "text/html", no_cache, do_apply_post, do_dhcp_renew_cgi, do_auth },
/* jimmy added 20081121, for graphic auth */
#ifdef AUTHGRAPH
    { "**.bmp", "image/bmp", no_cache, NULL, do_auth_pic, NULL },
#endif
/* ---------------------------------------- */
    { "pppoe_00_connect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_connect_00_cgi, do_auth },
    { "pppoe_00_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_disconnect_00_cgi, do_auth },
#ifdef MPPPOE
    { "pppoe_01_connect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_connect_01_cgi, do_auth },
    { "pppoe_01_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_disconnect_01_cgi, do_auth },
    { "pppoe_02_connect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_connect_02_cgi, do_auth },
    { "pppoe_02_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_disconnect_02_cgi, do_auth },
    { "pppoe_03_connect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_connect_03_cgi, do_auth },
    { "pppoe_03_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_disconnect_03_cgi, do_auth },
    { "pppoe_04_connect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_connect_04_cgi, do_auth },
    { "pppoe_04_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_pppoe_disconnect_04_cgi, do_auth },
#endif
    /*  Date: 2009-01-21
    *   Name: jimmy huang
    *   Reason: for usb-3g connect/disconnect via UI
    *   Note:
    *
    */
#ifdef CONFIG_USER_3G_USB_CLIENT
/*
* Name: Albert Chen
* Date 2009-02-19
* Detail Chnage name for Widget will get this usb3g_connect.cgi
*/
    { "usb3g_init_connect.cgi*", "text/html", no_cache, do_apply_post, do_usb3g_connect_cgi, do_auth },
    { "usb3g_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_usb3g_disconnect_cgi, do_auth },
#endif
#if defined(CONFIG_USER_3G_USB_CLIENT_WM5) || defined(CONFIG_USER_3G_USB_CLIENT_IPHONE)
    { "usb3g_phone_init_connect.cgi*", "text/html", no_cache, do_apply_post, do_usb3g_phone_connect_cgi, do_auth },
    { "usb3g_phone_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_usb3g_phone_disconnect_cgi, do_auth },
#endif
    { "l2tp_connect.cgi*", "text/html", no_cache, do_apply_post, do_l2tp_connect_cgi, do_auth },
    { "l2tp_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_l2tp_disconnect_cgi, do_auth },
    { "pptp_connect.cgi*", "text/html", no_cache, do_apply_post, do_pptp_connect_cgi, do_auth },
    { "pptp_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_pptp_disconnect_cgi, do_auth },
    { "bigpond_connect.cgi*", "text/html", no_cache, do_apply_post, do_bigpond_login_cgi, do_auth },
    { "bigpond_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_bigpond_logout_cgi, do_auth },
    { "ping_response.cgi*", "text/html", no_cache, do_apply_post, do_ping_response_cgi, do_auth },
#ifdef IPv6_SUPPORT
    { "ping6_response.cgi*", "text/html", no_cache, do_apply_post, do_ping6_response_cgi, do_auth },
    { "ipv6_pppoe_connect.cgi*", "text/html", no_cache, do_apply_post, do_ipv6_pppoe_connect_cgi, do_auth },
    { "ipv6_pppoe_disconnect.cgi*", "text/html", no_cache, do_apply_post, do_ipv6_pppoe_disconnect_cgi, do_auth },
		{ "wizard_autodetect_start.cgi*", "text/html", no_cache, do_apply_post, do_wizard_autodetect_start_cgi, do_auth },
		{ "wizard_autodetect_stop.cgi*", "text/html", no_cache, do_apply_post, do_wizard_autodetect_stop_cgi, do_auth },
#endif
    { "mac_clone.cgi*", "text/html", no_cache, do_apply_post, do_mac_clone_cgi, do_auth },
    { "restore_default.cgi*", "text/html", no_cache, do_apply_post, do_restore_default_cgi, do_auth },
    { "restore_default_wireless.cgi*", "text/html", no_cache, do_apply_post, do_restore_default_wireless_cgi, do_auth },
#ifdef IPv6_TEST
    { "restore_default_ipv6.cgi*", "text/html", no_cache, do_apply_post, do_restore_default_ipv6_cgi, do_auth },
#endif
    { "reboot.cgi*", "text/html", no_cache, do_apply_post, do_reboot_cgi, do_auth },
    { "save_configuration.cgi*", NULL, download_nvram, do_apply_post, do_save_configuration_cgi, do_auth },
    { "save_log.cgi*", NULL, log_to_hd, do_apply_post, do_save_log_cgi, do_auth },
    { "upload_configuration.cgi*", "text/html", no_cache, do_upload_post, do_upload_configuration_cgi, do_auth },
    { "firmware_upgrade.cgi*", "text/html", no_cache, do_upgrade_post, do_firmware_upgrade_cgi, do_auth },
#if 0 //def MIII_BAR
    { "ws/api/ws/api/uploadFile*", NULL, no_cache, do_miii_uploadFile_post, do_miii_uploadFile, NULL },
    { "ws/api/downloadFile*", NULL, no_cache, do_miii_uploadFile_post, do_miii_uploadFile, NULL },
#endif
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
	{ "lp_upgrade.cgi*", "text/html", no_cache, do_lp_upgrade_post, do_firmware_upgrade_cgi, do_auth },
	{ "lp_clear.cgi*", "text/html", no_cache, do_apply_post, do_lp_clear_cgi, do_auth },
#endif
    { "send_log_email.cgi*", "text/html", no_cache, do_apply_post, do_send_log_email_cgi, do_auth },
    { "log_email_button_clicked_set0.cgi*", "text/html", no_cache, do_apply_post, do_log_email_button_clicked_set0_cgi, do_auth },
    { "log_refresh_page.cgi*", "text/html", no_cache, do_apply_post, do_refresh_log_email_cgi, do_auth },
    { "system_time.cgi*", "text/html", no_cache, do_apply_post, do_system_time_cgi, do_auth },
    /* jimmy added 20080401 , Dlink Hidden Page V1.01 suport */
    { "version.txt", "text/html", no_cache, NULL, do_version_cgi, do_auth },
    /* ---------------------------------------------- */
    { "chklst.txt", "text/html", no_cache, NULL, do_chklst_cgi, do_auth },
#ifdef OPENDNS
//    { "parentalcontrols", "text/html", no_cache, NULL, do_parentalcontrols_cgi, do_auth },
#endif
    { "F2_info.txt", "text/html", no_cache, NULL, do_f2info_cgi, do_auth },
    { "wlan.txt", "text/html", no_cache, NULL, do_wlan_config_cgi, do_auth },
    { "log_first_page.cgi*", "text/html", no_cache, do_apply_post, do_log_first_page_cgi, do_auth },
    { "log_last_page.cgi*", "text/html", no_cache, do_apply_post, do_log_last_page_cgi, do_auth },
    { "log_previous_page.cgi*", "text/html", no_cache, do_apply_post, do_log_previous_page_cgi, do_auth },
    { "log_next_page.cgi*", "text/html", no_cache, do_apply_post, do_log_next_page_cgi, do_auth },
    { "log_clear_page.cgi*", "text/html", no_cache, do_apply_post, do_log_clear_page_cgi, do_auth },
    { "vct_wan", "text/html", no_cache, do_apply_post, do_vct_wan_cgi, do_auth },
    { "vct_lan_00", "text/html", no_cache, do_apply_post, do_vct_lan_00_cgi, do_auth },
    { "vct_lan_01", "text/html", no_cache, do_apply_post, do_vct_lan_01_cgi, do_auth },
    { "vct_lan_02", "text/html", no_cache, do_apply_post, do_vct_lan_02_cgi, do_auth },
    { "vct_lan_03", "text/html", no_cache, do_apply_post, do_vct_lan_03_cgi, do_auth },
    { "reset_packet_counter.cgi*", "text/html", no_cache, do_apply_post, do_reset_packet_counter_cgi, do_auth },
    { "reset_ifconfig_packet_counter.cgi*", "text/html", no_cache, do_apply_post, do_reset_ifconfig_packet_counter_cgi, do_auth },
    { "save_log", "text/html", no_cache, do_apply_post, do_save_log_cgi, do_auth },
    { "usb_test.cgi*", "text/html", no_cache, do_apply_post, do_usb_test_cgi, do_auth },
    { "check_new_firmware.cgi*", "text/html", no_cache, do_apply_post, do_check_new_firmware_cgi, do_auth },
    { "apply_discard.cgi*", "text/html", no_cache, do_apply_discard_post, do_apply_discard_cgi, do_auth },
    { "login.cgi*", "text/html", no_cache, do_apply_post, do_login_cgi, do_auth },
    { "set_sta_enrollee_pin.cgi*", "text/html", no_cache, do_apply_post, do_set_sta_enrollee_pin, do_auth },
    { "virtual_push_button.cgi*", "text/html", no_cache, do_apply_post, do_virtual_push_button, do_auth },
    { "client_pin.cgi*","text/html",no_cache, do_apply_post, do_set_sta_enrollee_pin, do_auth },
    { "apc_client_pin.cgi*","text/html",no_cache, do_apply_post, do_apc_set_sta_enrollee_pin, do_auth },
    { "ntp_sync.cgi*", "text/html", no_cache, do_apply_post, do_ntp_sync, do_auth },
    { "logout.cgi*", "text/html", no_cache, NULL, do_logout_cgi, do_auth },
    { "online_firmware_check.cgi*", "text/html", no_cache, do_apply_post, do_online_firmware_check_cgi, do_auth },
    { "dhcp_revoke.cgi*", "text/html", no_cache, do_apply_post, do_dhcp_voke_cgi, do_auth },
    { "dns_query.cgi*", "text/html", no_cache, do_apply_post, do_dns_query_cgi, do_auth},
#ifdef MPPPOE
    { "mpppoe_apply.cgi*", "text/html", no_cache, do_apply_post, do_mpppoe_apply_cgi, do_auth},
#endif
#ifdef IPv6_TEST
    { "radvd_enable_disable.cgi*", "text/html", no_cache, do_apply_post, do_radvd_enable_disable_cgi, do_auth},
    { "add_wan_gateway.cgi*", "text/html", no_cache, do_apply_post, do_add_wan_gateway_cgi, do_auth },
    { "add_lan_gateway.cgi*", "text/html", no_cache, do_apply_post, do_add_lan_gateway_cgi, do_auth },
    { "del_wan_gateway.cgi*", "text/html", no_cache, do_apply_post, do_del_wan_gateway_cgi, do_auth },
    { "del_lan_gateway.cgi*", "text/html", no_cache, do_apply_post, do_del_lan_gateway_cgi, do_auth },
    { "set_wan_link_mtu.cgi*", "text/html", no_cache, do_apply_post, do_set_wan_link_mtu_cgi, do_auth },
    { "set_lan_link_mtu.cgi*", "text/html", no_cache, do_apply_post, do_set_lan_link_mtu_cgi, do_auth },
    { "stop_service.cgi*", "text/html", no_cache, do_apply_post, do_stop_service_cgi, do_auth },
    { "start_service.cgi*", "text/html", no_cache, do_apply_post, do_start_service_cgi, do_auth },
    { "execute_cmd.cgi*", "text/html", no_cache, do_apply_post, do_execute_cmd_cgi, do_auth },
    { "restart_network.cgi*", "text/html", no_cache, do_apply_post, do_restart_network_cgi, do_auth },
#endif
#ifdef BCM5352
    { "check_wan_connection.cgi*", "text/html", no_cache, do_apply_post, do_check_wan_connection_cgi, do_auth },
    { "upload_ca_certification.cgi*", "text/html", no_cache, do_upload_ca_certification_post, do_upload_certification_cgi, do_auth },
    { "upload_client_certification.cgi*", "text/html", no_cache, do_upload_client_certification_post, do_upload_certification_cgi, do_auth },
    { "upload_pkey_certification.cgi*", "text/html", no_cache, do_upload_pkey_certification_post, do_upload_certification_cgi, do_auth },
    { "apply_config.cgi*", "text/html", no_cache, do_apply_config_post, do_apply_config_cgi, do_auth },
    { "apply_confirm.cgi*", "text/html", no_cache, do_apply_confirm_post, do_apply_confirm_cgi, do_auth },
    //  { "apply_discard.cgi*", "text/html", no_cache, do_apply_discard_post, do_apply_discard_cgi, do_auth },
    { "logout.cgi*", "text/html", no_cache, do_apply_post, do_logout_cgi, do_auth },
    { "ip_error.txt", "text/html", no_cache, NULL, do_ip_error_cgi, do_auth },
#endif
#ifdef CONFIG_USER_WAN_8021X
    { "upload_ca_certification.cgi*", "text/html", no_cache, do_upload_ca_certification_post, do_upload_certification_cgi, do_auth },
    { "upload_client_certification.cgi*", "text/html", no_cache, do_upload_client_certification_post, do_upload_certification_cgi, do_auth },
    { "upload_pkey_certification.cgi*", "text/html", no_cache, do_upload_pkey_certification_post, do_upload_certification_cgi, do_auth },
    { "ip_error.txt", "text/html", no_cache, NULL, do_ip_error_cgi, do_auth },
#endif
    { "wireless_update.txt", "text/html", no_cache, NULL, do_wireless_update_cgi, do_auth },
    { "wireless_driver_update.cgi*", "text/html", no_cache, do_wireless_upload_post, do_wireless_upload_cgi, do_auth },
#ifdef MIII_BAR
    { "crossdomain.xml", "text/xml", NULL, NULL, do_file, NULL },
#endif
#ifdef PURE_NETWORK_ENABLE
    { "**.xml", "text/xml", no_cache, do_pure_action, send_pure_response, do_auth },
    { "restart.cgi*", "text/html", no_cache, do_apply_post, do_restart_cgi, do_auth },
    { "HNAP1.txt", "text/xml", no_cache, NULL, do_pure_check, do_auth },
#endif
#ifdef XML_AGENT
    { "router_info.xml?section=*", "text/xml", no_cache, do_widget_action, send_widget_response, do_auth },
    { "post_login.xml?hash=*", "text/xml", no_cache, do_agentLogin_action, send_widget_response, NULL },
#endif
#ifdef AJAX
    { "device.xml=*", "text/xml", no_cache, do_ajax_action, send_ajax_response, NULL },
    { "ping_test.xml=**", "text/xml", no_cache, do_ping_action, send_ajax_response, NULL },
#endif
/*
    Date: 2009-1-05
    Name: Ken_Chiang
    Reason: modify for blocked url to redirect the reject page by crowdcontrol(http proxy).
    Notice :
*/
#ifdef HTTP_FILTER_PROXY
    { "reject.html", "text/html", NULL, NULL, do_file, NULL },
#endif
#if defined(DIR865) || defined(EXVERSION_NNA)
    {"switch_language.cgi*", "text/html", no_cache, do_apply_post, do_switch_language_cgi, do_auth},
#endif
    { NULL, NULL, NULL, NULL, NULL, NULL }
};
