#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "sutil.h"
#include "shvar.h"
#include <linux_vct.h>
#include <pure.h>
#include <nvram.h>
#include <fcntl.h>

#include <httpd_util.h>
#include <assert.h>

// debug
//#define PURE_DEBUG 1
#ifdef PURE_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define MAC_LENGTH  17

struct ethernetport{
	char  port[16];
	char  connected[16];
	char  speed[5];
	char  duplex[5];
};	

pure_atrs_t pure_atrs;
wan_settings_t wan_settings;
cnted_client_list_t cnted_client_table[4];    // max client number

static char *parse_action(int i);
static int return_pure_action(char *cp_t, const char *recieve_buf);
static int get_pure_settings(int action, const char *recieve_buf);
static int set_pure_settings(int action, const char *recieve_buf);
int get_pure_content(int pure_conn_fd,char *rec_buf, char *action_string, int length);
static void add_to_response( char* str, int len );
static void add_to_buf( char* bufP, int* bufsizeP, int* buflenP, char* str, int len , int max_len);
static void add_headers(int status, char* title, char* extra_header, char* mime_type, long byte, char *mod );

/* parse response from pure network & set value to system */
static int pure_SetDeviceSettings(char *source_xml);
static int pure_SetWLanSettings24(char *source_xml);
static int pure_SetWLanSecurity(char *source_xml);
static int pure_SetWLanRadioSettings(char *source_mxl);
static int pure_SetWLanRadioSecurity(char *source_mxl);
static int pure_SetWanSettings(char *source_xml);
static int pure_SetMACFilters2(char *source_xml);
static int pure_AddPortMappings(char *source_xml);
static int pure_DelPortMappings(char *source_xml);
static int pure_SetRouterLanSettings(char *source_xml);
static int pure_SetAccessPointMode(char *source_xml, char *dest_xml);
static int pure_SetRenewWanConnection(char *source_xml, char *dest_xml);
static int get_wan_settings(int type);
static int set_wan_settings(int type);
static int check_lan_settings(char *ip_t, char *mask_t);
static int check_mac_filter(char *mac_t);
static int check_wan_settings(int type);
static int get_pppoe_pptp_l2tp_settings(int w_index);
static int set_pppoe_pptp_l2tp_settings(int w_index);
static int mac_check(char *mac,int wan_flag);
static int address_check(char *address,int type);
static int ip_check(char *token_1, char *token_2, char *token_3, char *token_4);
static int netmask_check(char *address, char *token_1, char *token_2, char *token_3, char *token_4);
static int illegal_mask(char *mask_token);
static char *mac_complement(char *mac,char *result);

/* parse to nvram value to pure xml */
static int pure_GetDeviceSettings(char *dest_xml, char *general_xml);
static int pure_GetMACFilters2(char *dest_xml, char *general_xml, char *mac_info_xml);
static int pure_GetNetworkStats(char *dest_xml, char *general_xml);
static int pure_GetRouterLanSettings(char *dest_xml, char *general_xml);
static int pure_GetPortMappings(char *dest_xml, char *general_xml, char *vs_rule_xml);
static int pure_GetWLanSetting24(char *dest_xml, char *general_xml);
static int pure_GetWLanSecurity(char *dest_xml, char *general_xml);
static int pure_GetWLanRadios(char *dest_xml, char *general_xml);
static int pure_GetWLanRadioSettings(char *dest_xml, char *general_xml, char *source_xml);
static int pure_GetWLanRadioSecurity(char *dest_xml, char *general_xml, char *source_xml);
static int pure_GetConnectedDevices(char *dest_xml, char *general_xml, char *cnted_client_xml);
static int pure_GetWanSettings(char *dest_xml, char *gereral_xml);
static int pure_GetWanStatus(char *dest_xml, char *gereral_xml);
static int pure_GetEthernetPortSetting(char *dest_xml, char *gereral_xml);

static int pure_StartCableModemCloneWanMac(char *source_xml);
static int pure_GetCableModemCloneWanMac(char *dest_xml, char *gereral_xml);
static int pure_SetCableModemCloneWanMac(char *source_xml, char *dest_xml);

/* parse if more info of response of pure network*/
static int get_element_value(char *source_xml, char *xml_value, char *element_name);
static char *find_next_element(char *source_xml, char *element_name);

/* others */
static char *pure_dhcp_time_format(char *data,char *res);
static void parse_ascii_to_hex(char *ascii,char *ascii_tmp);
static char *parse_hex_to_ascii(char *hex_string);

/* xml file format, define in pure_xml.c*/
/* part of Setting */
extern char *SetDeviceSettingsResult_xml;
extern char *SetWLanSettings24Result_xml;
extern char *SetWanSettingsResult_xml;
extern char *SetWLanSecurityResult_xml;
extern char *SetWLanRadioSettingsResult_xml;
extern char *SetWLanRadioSecurityResult_xml;
extern char *SetMACFilters2Result_xml;
extern char *SetRouterLanSettingsResult_xml;
extern char *AddPortMappingResult_xml;
extern char *SetAccessPointModeResult_xml;
extern char *RenewWanConnectionResult_xml;
extern char *DeletePortMappingResult_xml;
extern char *StartCableModemCloneWanMac_xml;
extern char *SetCableModemCloneWanMac_xml;
extern char *UnkownCode_xml;
/* part of Getting */
extern char *GetConnectedDevicesResult_xml;
extern char *GetNetworkStatsResult_xml;
extern char *GetDeviceSettingsResult_xml;
extern char *GetWanSettingsResult_xml;
extern char *GetWanStatusResult_xml;
extern char *GetEthernetPortSettingResult_xml;
extern char *GetWLanSecurityResult_xml;
extern char *GetWLanSettings24Result_xml;
extern char *GetWLanRadiosResult_xml;
extern char *GetWLanRadioSettingsResult_xml;
extern char *GetWLanRadioSecurityResult_xml;
extern char *RadioInfo_24GHZ;
extern char *RadioInfo_5GHZ;
extern char *SecurityInfo;
extern char *WideChannels_5GHZ;
extern char *GetMACFilters2Result_xml;
extern char *GetPortMappingsResult_xml;
extern char *GetRouterLanSettingsResult_xml;
extern char *IsDeviceReady_xml;
extern char *RebootResult_xml;
extern char *MACInfo_xml;
extern char *PortMapping_xml;
extern char *CntedClient_xml;
extern char *GetCableModemCloneWanMac_xml;
char is_allow_list[6] = {};

static int GetWLanRadioSettings_24GHZ;
static int GetWLanRadioSecurity_24GHZ;
static char pure_secondary_channel[4] = "0";
static char gbl_ssid[WLA_ESSID_LEN+1] = {};
static int GetWanMacClone = 0;

// 2011.11.18
#ifdef OPENDNS
extern char *SetOpenDNS_xml; 
extern char *GetOpenDNS_xml;

static int pure_GetOpenDNS(char *dest_xml, char *general_xml);
static int pure_SetOpenDNS(char *source_xml);

#endif

static int pure_SetMultipleActions(char *source_xml, char *dest_xml);
extern char *SetMultipleActionResult_xml;
static int pure_SetTriggerADIC(char *source_xml, char *dest_xml);
extern char *SetTriggerADICResult_xml;

void replace_get_special_char(char *sourcechar)
{
    char value_tmp[512]={};
    char value_tmp_lt[512]={};	
    char value_tmp_gt[512]={};	
    char value_tmp_amp[512]={};		
    char value_tmp_apos[512]={};
    char value_tmp_quot[512]={};

	if( sourcechar == NULL){
		return;
	}
	if(strlen(sourcechar) == 0){
		return;
	}

	sprintf(value_tmp,"%s",sourcechar);
	sprintf(value_tmp_amp,"%s",substitute_keyword(value_tmp,"\&","&amp;"));	
	sprintf(value_tmp_lt,"%s",substitute_keyword(value_tmp_amp,"\<","&lt;"));
	sprintf(value_tmp_gt,"%s",substitute_keyword(value_tmp_lt,"\>","&gt;"));
	sprintf(value_tmp_apos,"%s",substitute_keyword(value_tmp_gt,"\'","&apos;"));		
	sprintf(value_tmp_quot,"%s",substitute_keyword(value_tmp_apos,"\"","&quot;"));
	sprintf(sourcechar,"%s",value_tmp_quot);
}

/* for MacFilter2 */
mac_filter_list_t mac_filter_table[] = {
	{"mac_filter_00"},{"mac_filter_01"},{"mac_filter_02"},{"mac_filter_03"},
	{"mac_filter_04"},{"mac_filter_05"},{"mac_filter_06"},{"mac_filter_07"},
	{"mac_filter_08"},{"mac_filter_09"},{"mac_filter_10"},{"mac_filter_11"},
	{"mac_filter_12"},{"mac_filter_13"},{"mac_filter_14"},{"mac_filter_15"},
	{"mac_filter_16"},{"mac_filter_17"},{"mac_filter_18"},{"mac_filter_19"},
	{"mac_filter_20"},{"mac_filter_21"},{"mac_filter_22"},{"mac_filter_23"}
};

/* for WanSettings*/
pppoe_pptp_l2tp_t pppoe_pptp_l2tp_table[] = {
	{	"wan_pppoe_connect_mode_00","wan_pppoe_username_00","wan_pppoe_password_00",
		"wan_pppoe_max_idle_time_00","wan_pppoe_service_00","wan_pppoe_ipaddr_00",
		"wan_pppoe_netmask_00","wan_pppoe_gateway_00","wan_pppoe_dynamic_00"},

	{	"wan_pptp_connect_mode","wan_pptp_username","wan_pptp_password",
		"wan_pptp_max_idle_time","wan_pptp_server_ip","wan_pptp_ipaddr",
		"wan_pptp_netmask","wan_pptp_gateway","wan_pptp_dynamic"},

	{	"wan_l2tp_connect_mode","wan_l2tp_username","wan_l2tp_password",
		"wan_l2tp_max_idle_time","wan_l2tp_server_ip","wan_l2tp_ipaddr",
		"wan_l2tp_netmask","wan_l2tp_gateway","wan_l2tp_dynamic",}
};

/* for PortMappings & PortMappings Count */
vs_rule_t vs_rule_table[] = {
	{"vs_rule_00"},{"vs_rule_01"},{"vs_rule_02"},{"vs_rule_03"},
	{"vs_rule_04"},{"vs_rule_05"},{"vs_rule_06"},{"vs_rule_07"},
	{"vs_rule_08"},{"vs_rule_09"},{"vs_rule_10"},{"vs_rule_11"},
	{"vs_rule_12"},{"vs_rule_13"},{"vs_rule_14"},{"vs_rule_15"},
	{"vs_rule_16"},{"vs_rule_17"},{"vs_rule_18"},{"vs_rule_19"},
	{"vs_rule_20"},{"vs_rule_21"},{"vs_rule_22"},{"vs_rule_23"}
};

void pure_init(int conn_fd,char *protocol,long content_length,char *content_type,char *modified,char *action_string)
{
	//DEBUG_MSG("pure_init: conn_fd=%x,protocol=%s,content_length=%d,content_type=%s,modified=%s,action=%s\n",conn_fd,protocol,content_length,content_type,modified,action_string);
	pure_atrs.reboot = 0;
	pure_atrs.set_nvram_error = 0;
	pure_atrs.conn_fd = conn_fd;
	pure_atrs.protocol = protocol;
	pure_atrs.content_length = content_length;
	pure_atrs.content_type = content_type;
	pure_atrs.if_modified_since = modified;
	pure_atrs.action_string = action_string;
}

int mapping_SoapAction_xml(int action_type, char *string)
{
	if (action_type == 0 || action_type == 24)
		return action_type;

	char *checkPtr = NULL;
	char action_string[32] ={'\0'};
	int actionCheckLen = 0;

	if (!(checkPtr = strcasestr(string, "<soap:body>")))
		return 0;

	checkPtr += strlen("<soap:body>");	//go through <soap:body>
	checkPtr = strstr(checkPtr,"<");	//go through <, the packet might has \r\n
	strcpy(action_string,parse_action(action_type));
	if (strncmp(checkPtr+1, action_string, strlen(action_string)))
		action_type = 0;

	return action_type;
}

static int check_rekey_time(char *key_time)
{
	int rekey_time = atoi(key_time);

	if ((rekey_time < 30) || (rekey_time > 65535))
		return 0;
	else
		return 1;
}

int HNAP_auth_flag = 0;
int IsDeviceReady_cunt = 0;
static int return_pure_action(char *cp_t, const char *recieve_buf)
{
	int p_action = -1;

	if (strstr(cp_t, "GetDeviceSettings")) {
		p_action = 1;
	}
	else if (strstr(cp_t, "GetWanSettings")) {
		p_action = 2;
	}
	else if (strstr(cp_t, "GetWLanSettings24")) {
		p_action = 3;
	}
	else if (strstr(cp_t, "GetWLanSecurity")) {
		p_action = 4;
	}
	else if (strstr(cp_t, "GetConnectedDevices")) {
		p_action = 5;
	}
	else if (strstr(cp_t, "GetNetworkStats")) {
		p_action = 6;
	}
	else if (strstr(cp_t, "GetMACFilters2")) {
		p_action = 7;
	}
	else if (strstr(cp_t, "GetPortMappings")) {
		p_action = 8;
	}
	else if (strstr(cp_t, "GetRouterLanSettings")) {
		p_action = 9;
	}
	else if (strstr(cp_t, "IsDeviceReady")) {
		IsDeviceReady_cunt++;
		p_action = 10;
	}
	else if (strstr(cp_t, "Reboot")) {
		p_action = 11;
	}
	else if (strstr(cp_t, "SetWLanSettings24")) {
		p_action = 12;
	}
	else if (strstr(cp_t, "SetWLanSecurity")) {
		p_action = 13;
	}
	else if (strstr(cp_t, "GetForwardedPorts")) { // not support
		p_action = 14;
	}
	else if (strstr(cp_t, "SetDeviceSettings")) {
		p_action = 15;
	}
	else if (strstr(cp_t, "SetForwardedPorts")) { // not support
		p_action = 16;
	}
	else if (strstr(cp_t, "SetWanSettings")) {
		p_action = 17;
	}
	else if (strstr(cp_t, "SetMACFilters2")) {
		p_action = 18;
	}
	else if (strstr(cp_t, "SetRouterLanSettings")) {
		p_action = 19;
	}
	else if (strstr(cp_t, "AddPortMapping")) {
		p_action = 20;
	}
	else if (strstr(cp_t, "RenewWanConnection")) {
		p_action = 21;
	}
	else if (strstr(cp_t, "DeletePortMapping")) {
		p_action = 22;
	}
	else if (strstr(cp_t, "SetAccessPointMode")) {
		p_action = 23;
	}
	else if (strstr(cp_t, "GetWLanRadios")) {
		p_action = 25;
	}
	else if (strstr(cp_t, "GetWLanRadioSettings")) {
		if (strstr(recieve_buf, "RADIO_24GHz"))
			GetWLanRadioSettings_24GHZ = 1;
		else
			GetWLanRadioSettings_24GHZ = 0;
		p_action = 26;
	}
	else if (strstr(cp_t, "GetWLanRadioSecurity")) {
		if (strstr(recieve_buf, "RADIO_24GHz"))
			GetWLanRadioSecurity_24GHZ = 1;
		else
			GetWLanRadioSecurity_24GHZ = 0;
		p_action = 27;
	}
	else if (strstr(cp_t, "SetWLanRadioSettings")) {
		p_action = 28;
	}
	else if (strstr(cp_t, "SetWLanRadioSecurity")) {
		p_action = 29;
	}
	else if (strstr(cp_t, "GetWanStatus")) {
		p_action = 30;
	}
	else if (strstr(cp_t, "GetEthernetPortSetting")) {
		p_action = 31;
	}	
	else if (strstr(cp_t, "StartCableModemCloneWanMac")) {
		p_action = 32;
	}	
	else if (strstr(cp_t, "GetCableModemCloneWanMac")) {
		p_action = 33;
	}	
	else if (strstr(cp_t, "SetCableModemCloneWanMac")) {
		p_action = 34;
	}
	else if (strstr(cp_t, "Method")) {
		p_action = 0;
		DEBUG_MSG("Unknow code string %s", cp_t);
	}

#ifdef OPENDNS
	else if (strstr(cp_t, "GetOpenDNS")) {
		p_action = 39;
	}	
	else if (strstr(cp_t, "SetOpenDNS")) {
		p_action = 40;
	}	

#endif
	else if (strstr(cp_t, "SetMultipleActions")) {
		p_action = 66;
	}
	else if (strstr(cp_t, "SetTriggerADIC")) {
		p_action = 67;
	}
	else {
		DEBUG_MSG("bad action %s\n", pure_atrs.action_string);
		p_action = 24;
	}

	if( HNAP_auth_flag == 0 && p_action != 1 && p_action != 10 && p_action != 0){
		DEBUG_MSG(" pure_mode_enable, !auth_check\n");
		p_action = 24;
		return p_action;
	}

DEBUG_MSG("p_action = %d\n", p_action);
	return p_action;
}

void send_pure_response(char *url,FILE *stream){

	send( pure_atrs.conn_fd, pure_atrs.response, strlen(pure_atrs.response), 0);
	/* "get function" can't do "reboot" except reboot function itself (action 11) */

	usleep(1000);
	memset(pure_atrs.response, 0, MAX_RES_LEN);

	/* just Reboot(11) && SetRouterLanSettings(19) && SetMACFilters2(18) will reboot system */
	if (pure_atrs.reboot == 1)
	{
		DEBUG_MSG("PureNetwork reboot\n");
		if (pure_atrs.action == 11)   /*Reboot*/
		{
			system("rc restart");
			if (nvram_match("wlan0_enable", "1") == 0)
				usleep(20000000);	/* 20 Secs*/
		}
		else if(pure_atrs.action == 12 || pure_atrs.action == 13 || pure_atrs.action == 28)
			kill(read_pid(RC_PID), SIGTSTP);	/*RESTART WLAN*/
		else if(pure_atrs.action == 17)
		{
		    system("reboot");
			//kill(read_pid(RC_PID), SIGUSR1);	/*RESTART RESTART WAN*/
	        }
		else if(pure_atrs.action == 18 || pure_atrs.action == 20)
			kill(read_pid(RC_PID), SIGSYS);		/*RESTART FIREWALL*/
		else if (pure_atrs.action == 19)
			kill(read_pid(RC_PID), SIGQUIT);	/* RESTART LAN*/
		else if(pure_atrs.action > 11)
			system("rc restart");
	}

	DEBUG_MSG("send_pure_response\n\n\n");
}

void do_pure_action(char *url, FILE *stream, int len, char *boundary)
{
	assert(stream);
	pure_atrs.stream = stream;                          // get connected fp (soap body if exist)
	//DEBUG_MSG("do_pure_action()\n");

	if (!pure_atrs.action_string) {
		DEBUG_MSG("!pure_atrs.action_string\n");
		return;
	}

	char recieve_buf[MAX_REC_LEN] = {'\0'};
	memset ( recieve_buf, MAX_REC_LEN, sizeof(recieve_buf));
	pure_atrs.action = get_pure_content( pure_atrs.conn_fd,recieve_buf, pure_atrs.action_string, len);

	if (pure_atrs.action >= 0) {
		DEBUG_MSG("action = %d, %s\n", pure_atrs.action, parse_action(pure_atrs.action));

#ifdef OPENDNS
		if( pure_atrs.action == 40) {

			if (!set_pure_settings(pure_atrs.action, recieve_buf))
				return;
		}
		else if( pure_atrs.action == 39) {
			if (!get_pure_settings(pure_atrs.action, recieve_buf))
				return;
		}
		else
#endif

		if (pure_atrs.action > 11 && pure_atrs.action!=25 && pure_atrs.action!=26 && pure_atrs.action!=27 && pure_atrs.action!=30 && pure_atrs.action!=31 && pure_atrs.action!=33) {	     									// action > 11 do setting
			if (!set_pure_settings(pure_atrs.action, recieve_buf))
				return;
		}
		else {	  // action <= 11 or action=25,26,27,30,31,33 => do getting
			if (!get_pure_settings(pure_atrs.action, recieve_buf))
				return;
		}
	}
}

void do_restart_cgi(char *url, FILE *stream)
{
	assert(stream);
	assert(url);

	nvram_set("html_response_page", nvram_get("pure_reboot_page"));
	nvram_flag_set();
	nvram_commit();

	if (redirect_count_down_page(stream,10, "40", 1)){  // 10 is for compare with ui (just > 4 is ok)
		fprintf(stderr,"redirect to count down page success\n");
		_system("rc restart");
	}

	init_cgi(NULL);
}

void read_rc_flag(char* current_state)
{
	int fd,size;
	fd = open(RC_FLAG_FILE, O_RDWR);
	if (fd) {
		size = read(fd, current_state, 1);
		close(fd);
	}
	else
		printf("Open %s failed\n", RC_FLAG_FILE);
}

static int get_pure_settings(int action, const char *recieve_buf)
{
	char dest_xml[MAX_RES_LEN] = {};

	switch (action) {
#ifdef OPENDNS
		case 39:
			//
			pure_GetOpenDNS(dest_xml,GetOpenDNS_xml);
			break;

#endif
		case 1:
			pure_GetDeviceSettings(dest_xml, GetDeviceSettingsResult_xml);
			break;

		case 2:
			if (!pure_GetWanSettings(dest_xml, GetWanSettingsResult_xml)) {
				DEBUG_MSG("GetWanSettings fail\n");
				return 0;
			}
			break;

		case 3:
			if (!pure_GetWLanSetting24(dest_xml, GetWLanSettings24Result_xml)) {
				DEBUG_MSG("GetWLanSetting24 fail\n");
				return 0;
			}
			break;

		case 4:
			if (!pure_GetWLanSecurity(dest_xml, GetWLanSecurityResult_xml)) {
				DEBUG_MSG("GetWLanSecurity fail\n");
				return 0;
			}
			break;

		case 5:
			if (!pure_GetConnectedDevices(dest_xml, GetConnectedDevicesResult_xml, CntedClient_xml)) {
				DEBUG_MSG("GetConnectedDevices fail\n");
				return 0;
			}
			break;

		case 6:
			if (!pure_GetNetworkStats(dest_xml, GetNetworkStatsResult_xml)) {
				DEBUG_MSG("GetNetworkStats fail\n");
				return 0;
			}
			break;

		case 7:
			if (!pure_GetMACFilters2(dest_xml, GetMACFilters2Result_xml, MACInfo_xml)) {
				DEBUG_MSG("GetMACFilters2 fail\n");
				return 0;
			}
			break;

		case 8:
			if (!pure_GetPortMappings(dest_xml, GetPortMappingsResult_xml, PortMapping_xml)) {
				DEBUG_MSG("GetPortMappings fail\n");
				return 0;
			}
			break;

		case 9:
			if (!pure_GetRouterLanSettings(dest_xml, GetRouterLanSettingsResult_xml)) {
				DEBUG_MSG("GetRouterLanSettings fail\n");
				return 0;
			}
			break;

		case 10:
		{
			char current_rc[4] = {0};
			int rc_idle=1, rc_busy=1, rc_busy_count=5;
#ifdef IP8000
			int rc_idle_count = 5;
#else
			int rc_idle_count = 3;
#endif

			while (1)
			{
				read_rc_flag(current_rc);
				if (current_rc[0] == 'i')
					rc_idle++;
				else {
					rc_idle = 1;
					rc_busy++;
				}

				if (rc_idle > rc_idle_count) {
					rc_busy = 1;
					DEBUG_MSG("IsDeviceReady: OK\n");
					xmlWrite(dest_xml, IsDeviceReady_xml, "OK");
					break;
				}

				if (rc_busy > rc_busy_count) {
					rc_busy = 1;
					rc_idle = 1;
					DEBUG_MSG("IsDeviceReady: ERROR\n");
					xmlWrite(dest_xml, IsDeviceReady_xml, "ERROR");					
					break;
				}	

				usleep(1000000); /* 1 Secs*/
			}
		}
			break;

		case 11:
			DEBUG_MSG("Reboot\n");
			sprintf(dest_xml, "%s", RebootResult_xml);
			pure_atrs.reboot = 1;
			break;

		case 25:
			if (!pure_GetWLanRadios(dest_xml, GetWLanRadiosResult_xml)) {
				printf("GetWLanRadios fail\n");
				return 0;
			}
			break;

		case 26:
			if (!pure_GetWLanRadioSettings(dest_xml, GetWLanRadioSettingsResult_xml, recieve_buf)) {
				printf("GetWLanRadioSettings fail\n");
				return 0;
			}
			break;

		case 27:
			if (!pure_GetWLanRadioSecurity(dest_xml, GetWLanRadioSecurityResult_xml, recieve_buf)) {
				printf("GetWLanRadioSecurity fail\n");
				return 0;
			}
			break;

		case 30:
			if (!pure_GetWanStatus(dest_xml, GetWanStatusResult_xml)) {
				printf("GetWanStatus fail\n");
				return 0;
			}
			break;

		case 31:
			if (!pure_GetEthernetPortSetting(dest_xml, GetEthernetPortSettingResult_xml)) {
				printf("GetEthernetPortSetting fail\n");
				return 0;
			}
			break;			

		case 33:
			if (!pure_GetCableModemCloneWanMac(dest_xml, GetCableModemCloneWanMac_xml)) {
				printf("GetCableModemCloneWanMac fail\n");
				return 0;
			}
			break;			

		default:
			DEBUG_MSG("unkown code\n");
			sprintf(dest_xml, "%s", UnkownCode_xml);
			break;
	}

	add_headers(200, "OK", NULL, "text/xml", -1, NULL);
	add_to_response(dest_xml, strlen(dest_xml));
	return 1;
}

static int set_pure_settings(int action, const char *recieve_buf)
{
	char dest_xml[MAX_XML_LEN] = {};

	switch(action) {
		case 12  :
			if (!pure_SetWLanSettings24(recieve_buf)) {
				DEBUG_MSG("SetWLanSettings24 fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, SetWLanSettings24Result_xml, "ERROR");
			}
			else {
				DEBUG_MSG("SetWLanSettings24 OK\n");
				xmlWrite(dest_xml, SetWLanSettings24Result_xml, "REBOOT");
			}
			break;

		case 13  :
			if (!pure_SetWLanSecurity(recieve_buf)) {
				DEBUG_MSG("SetWLanSecurity fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, SetWLanSecurityResult_xml, "ERROR");
				pure_atrs.reboot = 0;
			}
			else {
				DEBUG_MSG("SetWLanSecurity OK\n");
				xmlWrite(dest_xml, SetWLanSecurityResult_xml, "REBOOT");
				pure_atrs.reboot = 1;
			}
			break;

		case 15 :
			if (!pure_SetDeviceSettings(recieve_buf)) {
				DEBUG_MSG("SetDevicesettings fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, SetDeviceSettingsResult_xml, "ERROR");
			}
			else {
				DEBUG_MSG("SetDevicesettings OK\n");
				xmlWrite(dest_xml, SetDeviceSettingsResult_xml, "OK");
			}
			break;

		case 17 :
			if (!pure_SetWanSettings(recieve_buf)) {
				DEBUG_MSG("SetWanSettings fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, SetWanSettingsResult_xml, "ERROR");
				pure_atrs.reboot = 0;
			}
			else {
				DEBUG_MSG("SetWanSettings OK\n");
				xmlWrite(dest_xml, SetWanSettingsResult_xml, "REBOOT");
				pure_atrs.reboot = 1;
			}
			break;

		case 18 :
			if (!pure_SetMACFilters2(recieve_buf)) {
				DEBUG_MSG("SetMACFilters2 fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml,SetMACFilters2Result_xml,"ERROR");
				pure_atrs.reboot = 0;
			}
			else {
				DEBUG_MSG("SetMACFilters2 OK\n");
				xmlWrite(dest_xml,SetMACFilters2Result_xml,"REBOOT");
				pure_atrs.reboot = 1;
			}
			break;

		case 19 :
			if (!pure_SetRouterLanSettings(recieve_buf)) {
				DEBUG_MSG("SetRouterLanSettings fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml,SetRouterLanSettingsResult_xml,"ERROR");
				pure_atrs.reboot = 0;
			}
			else {
				DEBUG_MSG("SetRouterLanSettings OK\n");
				xmlWrite(dest_xml,SetRouterLanSettingsResult_xml,"REBOOT");
				pure_atrs.reboot = 1;
			}
			break;

		case 20 :
			if (!pure_AddPortMappings(recieve_buf)) {
				DEBUG_MSG("AddPortMappings fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, AddPortMappingResult_xml, "ERROR");
			}
			else {
				DEBUG_MSG("AddPortMappings OK\n");
				xmlWrite(dest_xml, AddPortMappingResult_xml, "REBOOT");
			}
			break;
		case 21 :
			if (!pure_SetRenewWanConnection(recieve_buf, dest_xml)) {
				DEBUG_MSG("SetRenewWanConnection fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml,RenewWanConnectionResult_xml,"ERROR");
			}
			else {
				DEBUG_MSG("SetRenewWanConnection OK\n");
				xmlWrite(dest_xml,RenewWanConnectionResult_xml,"REBOOT");
			}
			break;

		case 22 :
			if (!pure_DelPortMappings(recieve_buf)) {
				DEBUG_MSG("DelPortMappings fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, DeletePortMappingResult_xml, "ERROR");
			}
			else {
				DEBUG_MSG("DelPortMappings OK\n");
				xmlWrite(dest_xml, DeletePortMappingResult_xml, "REBOOT");
			}
			break;

		case 23 :
			pure_SetAccessPointMode(recieve_buf, dest_xml);
			DEBUG_MSG("SetAccessPointMode OK\n");
			strcat(dest_xml, SetAccessPointModeResult_xml);
			break;

		case 28:
			if (!pure_SetWLanRadioSettings(recieve_buf)) {
				DEBUG_MSG("SetWLanRadioSettings fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, SetWLanRadioSettingsResult_xml, "ERROR");
			}
			else {
				DEBUG_MSG("SetWLanRadioSettings OK\n");
				xmlWrite(dest_xml, SetWLanRadioSettingsResult_xml, "REBOOT");
				pure_atrs.reboot = 1;
			}
			break;

		case 29:
			if (!pure_SetWLanRadioSecurity(recieve_buf)) {
				DEBUG_MSG("SetWLanRadioSecurity fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml,SetWLanRadioSecurityResult_xml,"ERROR");
				pure_atrs.reboot = 0;
			}
			else {
				DEBUG_MSG("SetWLanRadioSecurity OK\n");
				xmlWrite(dest_xml,SetWLanRadioSecurityResult_xml,"REBOOT");
				pure_atrs.reboot = 1;
			}
			break;

		case 32 :
			if (!pure_StartCableModemCloneWanMac(recieve_buf)) {
				DEBUG_MSG("StartCableModemCloneWanMac fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, StartCableModemCloneWanMac_xml, "ERROR");
			}
			else {
				DEBUG_MSG("StartCableModemCloneWanMac OK\n");
				xmlWrite(dest_xml, StartCableModemCloneWanMac_xml, "OK");
			}
			break;

		case 34 :
			if (!pure_SetCableModemCloneWanMac(recieve_buf, dest_xml)) {
				DEBUG_MSG("SetCableModemCloneWanMac fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, SetCableModemCloneWanMac_xml, "ERROR");
			}
			else {
				DEBUG_MSG("SetCableModemCloneWanMac OK\n");
				xmlWrite(dest_xml,SetCableModemCloneWanMac_xml,"OK");
			}
			break;

#ifdef OPENDNS
		case 40:
			if (!pure_SetOpenDNS(recieve_buf)) {
				DEBUG_MSG("pure_SetOpenDNS fail\n");
				pure_atrs.set_nvram_error = 1;
				xmlWrite(dest_xml, SetOpenDNS_xml, "ERROR");
			}
			else {
				DEBUG_MSG("SetOpenDNS_xml OK\n");
				xmlWrite(dest_xml, SetOpenDNS_xml, "REBOOT");
				pure_atrs.reboot = 1;
			}
			break;
#endif

		case 66:
			if (!pure_SetMultipleActions(recieve_buf, dest_xml)) {
				DEBUG_MSG("pure_SetMultipleActions fail\n");
				pure_atrs.set_nvram_error = 1;
			}
			else {
				DEBUG_MSG("pure_SetMultipleActions OK\n");
				pure_atrs.reboot = 1;
			}
			break;
			
		case 67:
			if (!pure_SetTriggerADIC(recieve_buf, dest_xml)) {
				DEBUG_MSG("pure_SetTriggerADIC fail\n");
				pure_atrs.set_nvram_error = 1;
			}
			else {
				DEBUG_MSG("pure_SetTriggerADIC OK\n");
			}
			break;
			
		default:
			DEBUG_MSG("not support this action %d\n", action);
			pure_atrs.set_nvram_error = 1;
			break;
	}

	if (!pure_atrs.set_nvram_error) {
		nvram_set("nvram_default_value", "0");
		nvram_flag_set();
		nvram_commit();
	}

	/* add setting header & reponse */
	add_headers(200, "OK", (char*) 0, "text/xml", -1, NULL);
	add_to_response( dest_xml, strlen(dest_xml));
	return 1;
}

static void add_to_response(char* str, int len)
{
	add_to_buf(pure_atrs.response, &pure_atrs.response_size, &pure_atrs.response_len, str, len, MAX_RES_LEN);
}

static void add_to_buf(char* bufP, int* bufsizeP, int* buflenP, char* str, int len, int max_len)
{
	if (*bufsizeP == 0) {
		*bufsizeP = MAX_RES_LEN;
		*buflenP = 0;
	}

	if (*buflenP + len >= *bufsizeP)
		return;

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

	pure_atrs.status = status;
	pure_atrs.bytes = byte;
	pure_atrs.response_size = 0;

	sprintf(buf, "%s %d %s\r\n", pure_atrs.protocol, pure_atrs.status, title);
	add_to_response(buf, strlen(buf));

	sprintf(buf, "Server: Embedded HTTP Server ,Firmware Version %s\r\n", VER_FIRMWARE);
	add_to_response(buf, strlen(buf));

	now = time( (time_t*) 0 );
	strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
	sprintf(buf, "Date: %s\r\n", timebuf);
	add_to_response(buf, strlen(buf));

	if (extra_header != (char*) 0) {
    	sprintf(buf, "%s\r\n", extra_header);
		add_to_response( buf, strlen(buf));
	}

	if (mime_type != (char*) 0) {
		sprintf(buf, "Content-Type: %s\r\n", mime_type);
		add_to_response(buf, strlen(buf));
	}

	if (pure_atrs.bytes >= 0) {
		sprintf(buf, "Content-Length: %ld\r\n", pure_atrs.bytes);
		buflen = strlen(buf);
		add_to_response(buf, buflen);
	}

	if (mod) {
    	sprintf(buf, "Last-Modified: %s\r\n", "");
    	add_to_response(buf, strlen(buf));
	}

	sprintf(buf, "Connection: close\r\n\r\n");
	add_to_response(buf, strlen(buf));
}

static int get_element_value(char *source_xml, char *xml_value, char *element_name){
	char *index1 = NULL, *index2 = NULL;
	char *ch = NULL;
	char temp_value[128]={};
	int find = 1;

	index1 = strstr(source_xml, element_name);

	if (index1 != 0)
	{

		while ( *(ch = index1 + strlen(element_name)) != '>'
				// && *(ch = index1 + strlen(element_name)) != ' '
				&& *(ch = index1 + strlen(element_name)) != '/'
				//&& *(ch = index1 - 1) != '<'  /* NickChou modify for check <ChannelWidth>20</ChannelWidth><Channel>0</Channel> */
			)
		{
			index1 += strlen(element_name);
			index1 = strstr(index1, element_name);

			if (index1 == NULL){
				xml_value[0] = '\0';
				return 0;
			}
		}

		index1 += strlen(element_name);

		while(*(ch = index1) != '>'){
			if (*(ch) == '/'){
				xml_value[0] = '\0';
				return 0;
			}
			index1++;
		}
		index1++;

		index2 = strstr(index1, element_name); // find the close element

		while (*(ch = index2 - 2) != '<'){
			index2 += strlen(element_name);
			index2 = strstr(index2, element_name);
		}

		if (index2 != 0)
		{
			index2 -= 2;
				memcpy(temp_value, index1, strlen(index1) - strlen(index2));
				memcpy(xml_value, temp_value, strlen(temp_value));
				xml_value[strlen(temp_value)] = '\0';
		}
	}else {
		find = 0;
	}

	return find;
}

static char *find_next_element(char *source_xml, char *element_name){
	char *index1 = NULL;
	char *ch = NULL;

	index1 = strstr(source_xml, element_name);
	if (index1 != 0){
		while ((*(ch = index1 - 1) != '<') || (*(ch = index1 + strlen(element_name)) != '>'
						&& *(ch = index1 + strlen(element_name)) != ' ' && *(ch = index1 + strlen(element_name)) != '/')){
			index1 += strlen(element_name);
			index1 = strstr(index1, element_name);

			if (index1 == NULL)
				return index1;
	}

		index1 += strlen(element_name) + 1;
		source_xml = index1;
	}else{
		source_xml = NULL;
	}

	return source_xml;
}

/*
  Modification: for password test
  Modified by: ken_chiang
  Date:2007/8/21
*/
static void replace_special_char(char *str)
{
	char replace_str[AUTH_MAX];
	int i, j, len, find, count;

	len = strlen(str);
	memset(replace_str, 0, AUTH_MAX);
	find = 0;
	count = 0;

	if( str == NULL){
		return;
	}
	if(strlen(str) == 0){
		return;
	}
	
	for (i = 0, j = 0; i < len; i++, j++) {
		count++;
		if (str[i] != '&') {
			replace_str[j] = str[i];

		}
		else {
			if (str[i+1] == 'a' && str[i+2] == 'm' && str[i+3] == 'p'
					&& str[i+4] == ';'){	// replace '&'
				replace_str[j] = str[i];
				i += 4;
				find = 1;
				//DEBUG_MSG("&\n");
			}
			else if (str[i+1] == 'q' && str[i+2] == 'u' && str[i+3] == 'o'
					&& str[i+4] == 't' && str[i+5] == ';'){	// replace '"'
				replace_str[j] = '"';
				i += 5;
				find = 1;
			}
			else if (str[i+1] == 'a' && str[i+2] == 'p' && str[i+3] == 'o'
					&& str[i+4] == 's' && str[i+5] == ';'){	// replace '''
				strcpy(&replace_str[j], "'");
				i += 5;
				find = 1;
			}
			else if (str[i+1] == 'l' && str[i+2] == 't' && str[i+3] == ';') {	// replace '<'
				replace_str[j] = '<';
				i += 3;
				find = 1;
				//DEBUG_MSG("<\n");
			}
			else if (str[i+1] == 'g' && str[i+2] == 't' && str[i+3] == ';') {	// replace '>'
				replace_str[j] = '>';
				i += 3;
				find = 1;
				//DEBUG_MSG(">\n");
			}
		}
	}

	replace_str[count]='\0';

	if (find) {
		memset(str, 0, len);
		memcpy(str, replace_str, count+1);
	}
}

static int pure_SetDeviceSettings(char *source_xml)
{
	char device_name[DEV_HOST_NAME_LEN+1] = {};
	char new_pwd[AUTH_MAX] = {};
#ifdef AUTHGRAPH
	static char graph_enable[8] = {0};
#endif

	get_element_value(source_xml, device_name, "DeviceName");
	get_element_value(source_xml, new_pwd, "AdminPassword");
#ifdef AUTHGRAPH
	get_element_value(source_xml, graph_enable, "CAPTCHA");
#endif

	replace_special_char(device_name);
	DEBUG_MSG("DeviceName= %s\n", device_name);
	replace_special_char(new_pwd);
	DEBUG_MSG("AdminPassword= %s\n", new_pwd);
    
	if ((strlen(new_pwd) > 15) || (strlen(device_name) > 32)) {
		DEBUG_MSG("AdminPassword length > 15 or DeviceName length > 32\n");
		return 0;
	}

	nvram_set("pure_device_name", device_name);
	nvram_set("admin_password", new_pwd);
#ifdef AUTHGRAPH
	nvram_set("graph_auth_enable", !strcmp(graph_enable, "true") ? "1" : "0");
#endif

	if (strcmp(new_pwd, "ActualPassword") == 0) {
		if (nvram_match("traffic_shaping", "1") == 0) {
			printf("Disable traffic shaping before pure test\n");
			nvram_set("traffic_shaping", "0");
			nvram_set("asp_temp_71", "1");
		}
		else
			nvram_set("asp_temp_71", "0");
	}

	return 1;
}

static int pure_SetWLanRadioSettings(char *source_xml)
{
	char radio_id[16]={};
	char *enable = "1";
	char enable_tmp[8] = {};
	char *mode = "11n";
	char mode_tmp[12] = {};
	char ssid[WLA_ESSID_LEN+1] = {};
	char *ssid_bcast = "1";
	char ssid_bcast_tmp[8];
	char *channel_width = "20";
	char channel_width_tmp[8]={};
	char channel[8] = {};
	char secondary_channel_tmp[8] = {};
	char *qos = "1";
	char qos_tmp[8] = {};

	get_element_value(source_xml, radio_id, "RadioID");
	get_element_value(source_xml, enable_tmp, "Enabled");
	get_element_value(source_xml, mode_tmp, "Mode");
	get_element_value(source_xml, ssid, "SSID");
	get_element_value(source_xml, ssid_bcast_tmp, "SSIDBroadcast");
	get_element_value(source_xml, channel_width_tmp, "ChannelWidth");
	get_element_value(source_xml, channel, "Channel");
	get_element_value(source_xml, secondary_channel_tmp, "SecondaryChannel");
	get_element_value(source_xml, qos_tmp, "QoS");

	DEBUG_MSG("RadioID = %s\n", radio_id);
	DEBUG_MSG("Enabled = %s\n", enable_tmp);
	DEBUG_MSG("Mode = %s\n", mode_tmp);
	replace_special_char(ssid);	
	DEBUG_MSG("SSID = %s\n", ssid);
	DEBUG_MSG("SSIDBroadcast = %s\n", ssid_bcast_tmp);
	DEBUG_MSG("ChannelWidth = %s\n", channel_width_tmp);
	DEBUG_MSG("Channel = %s\n", channel);
	DEBUG_MSG("SecondaryChannel = %s\n", secondary_channel_tmp);
	DEBUG_MSG("QoS = %s\n", qos_tmp);

#ifdef USER_WL_ATH_5GHZ
	if (strcasecmp(radio_id, "RADIO_24GHz") && strcasecmp(radio_id, "RADIO_5GHz")) {
#else
	if (strcasecmp(radio_id, "RADIO_24GHz")) {
#endif
		return 0;
	}

	/* enable/disable transfer */
	if (strlen(enable_tmp) && (strcasecmp(enable_tmp, "false") == 0)) {
		enable = "0";
	}

	if (strlen(mode_tmp)) {
		if (strcasecmp(mode_tmp, "802.11bgn") == 0)
			mode = "11bgn";
		else if (strcasecmp(mode_tmp, "802.11bg") == 0)
			mode = "11bg";
		else if (strcasecmp(mode_tmp, "802.11gn") == 0)
			mode = "11gn";
		else if (strcasecmp(mode_tmp, "802.11b") == 0)
			mode = "11b";
		else if (strcasecmp(mode_tmp, "802.11g") == 0)
			mode = "11g";
#ifdef USER_WL_ATH_5GHZ
		else if (strcasecmp(mode_tmp, "802.11an") == 0)
			mode = "11na";
		else if (strcasecmp(mode_tmp, "802.11a") == 0)
			mode = "11a";
#endif
		else
			mode = "11n";
	}

	if (strlen(ssid_bcast_tmp) && (strcasecmp(ssid_bcast_tmp, "false") == 0)) {
		ssid_bcast = "0";
	}

	if (strlen(channel_width_tmp)) {
		if (strcasecmp(channel_width_tmp, "0") == 0)
			channel_width = "auto";
		else if (strcasecmp(channel_width_tmp, "40") == 0)
			channel_width = "40";
	}

	if (strlen(secondary_channel_tmp)) {
		strcpy(pure_secondary_channel, secondary_channel_tmp);
	}
	else
		strcpy(pure_secondary_channel, "0");

	if (strlen(qos_tmp) && (strcasecmp(qos_tmp, "false") == 0)) {
		qos = "0";
	}

	if (strcasecmp(radio_id, "RADIO_24GHz") == 0) {
		nvram_set("wlan0_enable", enable);
		nvram_set("wlan0_ssid", ssid);
		nvram_set("wlan0_dot11_mode", mode);
		nvram_set("wlan0_ssid_broadcast", ssid_bcast);
		if (strcmp(channel, "0") == 0) {
			nvram_set("wlan0_auto_channel_enable", "1");
		}
		else {
			nvram_set("wlan0_auto_channel_enable", "0");
			nvram_set("wlan0_channel", channel);
		}
		nvram_set("wlan0_11n_protection", channel_width);
		nvram_set("wlan0_wmm_enable", qos);
	}
#ifdef USER_WL_ATH_5GHZ
	else if (strcasecmp(radio_id, "RADIO_5GHz") == 0) {
		nvram_set("wlan1_enable", enable);
		nvram_set("wlan1_ssid", ssid);
		nvram_set("wlan1_dot11_mode", mode);
		nvram_set("wlan1_ssid_broadcast", ssid_bcast);
		if (strcasecmp(channel, "0") == 0)
			nvram_set("wlan1_auto_channel_enable", "1");
		else {
			nvram_set("wlan1_auto_channel_enable", "0");
			nvram_set("wlan1_channel", channel);
		}
		nvram_set("wlan1_11n_protection", channel_width);
		nvram_set("wlan1_wmm_enable", qos);
	}
#endif

	nvram_set("disable_wps_pin", "1");
	nvram_set("wps_configured_mode", "5");
	return 1;
}

#ifdef OPENDNS
static int pure_SetOpenDNS(char *source_xml)
{
	
	char enable_str[32]={0};
	char mode_str[32]={0};
	char deviceid_str[32]={0};

	

	get_element_value(source_xml, enable_str, "EnableOpenDNS");
	get_element_value(source_xml, mode_str, "OpenDNSMode");
	get_element_value(source_xml, deviceid_str, "OpenDNSDeviceID");

	DEBUG_MSG("opendns_enable = %s\n", enable_str);
	DEBUG_MSG("opendns_mode = %s\n", mode_str);
	DEBUG_MSG("opendns_deviceid = %s\n", deviceid_str);

	
//
	if( strcmp(enable_str,"true")==0) {
		strcpy(enable_str,"1");
	}
	else {
		strcpy(enable_str,"0");
	}


	if( strcmp(enable_str,"1")==0) {
		if( strcmp(mode_str,"Advanced")==0) {
			strcpy(mode_str,"4"); 
		}
		else if( strcmp(mode_str,"FamilyShield")==0) {
			strcpy(mode_str,"3"); 
		}
		else if( strcmp(mode_str,"Parental")==0) {
			strcpy(mode_str,"2"); 
		}
		else {
			strcpy(mode_str,"1"); // DHCP
		}
	}
	else {
		strcpy(mode_str,"1"); // DHCP
	}


	nvram_set("opendns_enable", enable_str);
	nvram_set("opendns_mode", mode_str);

	nvram_set("dns_relay", "1");
	
	if( strcmp(mode_str,"3") == 0) {
		// special for FamilyShield
			nvram_set("wan_specify_dns","1");
			nvram_set("wan_primary_dns","208.67.222.123");
			nvram_set("wan_secondary_dns","208.67.220.123");
	
	}
	else if( strcmp(mode_str,"4") == 0) {
		// special for Advanced DNS
			nvram_set("wan_specify_dns","1");
			nvram_set("wan_primary_dns","204.194.232.200");
			nvram_set("wan_secondary_dns","204.194.234.200");
	
	}
	else if( strcmp(mode_str,"2") == 0) {
		nvram_set("opendns_deviceid", deviceid_str);
	}
	else {	
//		nvram_set("opendns_deviceid", deviceid_str);
	}

	
//	nvram_flag_set();
//	nvram_commit();
	
	
	
	return 1;
}
#endif


static int pure_SetWLanSettings24(char *source_xml)
{
	char enable_tmp[8] = {};
	char *enable = "1";
	char ssid[WLA_ESSID_LEN+1] = {};
	char ssid_bcast_temp[8] = {};
	char *ssid_bcast = "1";
	char channel[4] = {};

	get_element_value(source_xml, enable_tmp, "Enabled");
	get_element_value(source_xml, ssid, "SSID");
	get_element_value(source_xml, ssid_bcast_temp, "SSIDBroadcast");
	get_element_value(source_xml, channel, "Channel");

	DEBUG_MSG("Enabled = %s\n", enable_tmp);
	replace_special_char(ssid);
	DEBUG_MSG("SSID = %s\n", ssid);
	DEBUG_MSG("SSIDBroadcast = %s\n", ssid_bcast_temp);
	DEBUG_MSG("Channel = %s\n", channel);

	/* enable/disable transfer */
	if (enable_tmp != NULL && strcasecmp(enable_tmp,"false") == 0) {
		enable = "0";
	}
	if (ssid_bcast_temp != NULL && strcasecmp(ssid_bcast_temp,"false") == 0) {
		ssid_bcast = "0";
	}
	
	nvram_set("wlan0_enable", enable);
	nvram_set("wlan0_ssid", ssid);
	nvram_set("wlan0_ssid_broadcast", ssid_bcast);
	nvram_set("wlan0_channel", channel);
	nvram_set("disable_wps_pin", "1");
	nvram_set("wps_configured_mode","5");

	return 1;
}

static int pure_SetWanSettings(char *source_xml)
{
	int type = 0;
	char wan_protocol[20] = {};
#ifdef OPENDNS
	char open_dns[8] = {};
#endif
	char username[POE_NAME_LEN+1] = {};
	char password[POE_PASS_LEN+1] = {};
	char max_idle_time[8] = {};
	char service_name[POE_NAME_LEN+1] = {};
	char auto_reconnect[8] = {};
	char ip[20] = {};
	char mask[20] = {};
	char gateway[20] = {};
	char dns1[20] = {};
	char dns2[20] = {};
	char mac[18] = {};
	char mtu[8] = {};

	/* get from xml */
	get_element_value(source_xml, wan_protocol, "Type");
	get_element_value(source_xml, username, "Username");
	get_element_value(source_xml, password, "Password");
	get_element_value(source_xml, max_idle_time, "MaxIdleTime");
	get_element_value(source_xml, service_name, "ServiceName");
	get_element_value(source_xml, auto_reconnect, "AutoReconnect");
	get_element_value(source_xml, ip, "IPAddress");
	get_element_value(source_xml, mask, "SubnetMask");
	get_element_value(source_xml, gateway, "Gateway");
	get_element_value(source_xml, dns1, "Primary");
	get_element_value(source_xml, dns2, "Secondary");
#ifdef OPENDNS
	get_element_value(source_xml, open_dns, "enable");
#endif
	get_element_value(source_xml, mac, "MacAddress");
	get_element_value(source_xml, mtu, "MTU");

	DEBUG_MSG("Type = %s\n", wan_protocol);
	replace_special_char(username);
	DEBUG_MSG("Username = %s\n", username);
	replace_special_char(password);
	DEBUG_MSG("Password = %s\n", password);
	DEBUG_MSG("MaxIdleTime = %s\n", max_idle_time);
	DEBUG_MSG("ServiceName = %s\n", service_name);
	DEBUG_MSG("AutoReconnect = %s\n", auto_reconnect);
	DEBUG_MSG("IPAddress = %s\n", ip);
	DEBUG_MSG("SubnetMask = %s\n", mask);
	DEBUG_MSG("Gateway = %s\n", gateway);
	DEBUG_MSG("Primary = %s\n", dns1);
	DEBUG_MSG("Secondary = %s\n", dns2);
#ifdef OPENDNS
	DEBUG_MSG("enable = %s\n", open_dns);
#endif
	DEBUG_MSG("MacAddress = %s\n", mac);
	DEBUG_MSG("MTU = %s\n", mtu);

	wan_settings.name = username;
	wan_settings.password = password;
	wan_settings.idle_time = max_idle_time;
	wan_settings.service = service_name;
	wan_settings.mode = auto_reconnect;
	wan_settings.ip = ip;
	wan_settings.netmask = mask;
	wan_settings.gateway = gateway;
	wan_settings.primary_dns = dns1;
	wan_settings.secondary_dns = dns2;
#ifdef OPENDNS
	wan_settings.opendns = open_dns;
#endif
	if (strlen(mac) == 0)
		strcpy(mac, nvram_safe_get("wan_mac"));

	wan_settings.mac = mac;
	wan_settings.mtu = mtu;

	/* get wan protocol type */
	if (strcasecmp(wan_protocol, "Static") == 0) {
		type = 1;
		wan_settings.protocol = "static";
		wan_settings.dynamic = "0";
	}
	else if (strcasecmp(wan_protocol, "StaticPPPoE") == 0) {
		type = 2;
		wan_settings.protocol = "pppoe";
		wan_settings.dynamic = "0";
	}
	else if (strcasecmp(wan_protocol, "DHCPPPPoE") == 0) {
		type = 3;
		wan_settings.protocol = "pppoe";
		wan_settings.dynamic = "1";
	}
	else if (strcasecmp(wan_protocol, "DHCP") == 0) {
		type = 4;
		wan_settings.protocol = "dhcpc";
		wan_settings.dynamic = "1";
	}
	else if (strcasecmp(wan_protocol, "StaticPPTP") == 0) {
		type = 5;
		wan_settings.protocol = "pptp";
		wan_settings.dynamic = "0";
	}
	else if (strcasecmp(wan_protocol, "DynamicPPTP") == 0) {
		type = 6;
		wan_settings.protocol = "pptp";
		wan_settings.dynamic = "1";
	}
	else if (strcasecmp(wan_protocol, "StaticL2TP") == 0) {
		type = 7;
		wan_settings.protocol = "l2tp";
		wan_settings.dynamic = "0";
	}
	else if (strcasecmp(wan_protocol, "DynamicL2TP") == 0) {
		type = 8;
		wan_settings.protocol = "l2tp";
		wan_settings.dynamic = "1";
	}
	else {
		DEBUG_MSG("Unknown Wan Type\n");
		return 0;
	}

	/* check the value of xml */
	if (!check_wan_settings(type)) {
		DEBUG_MSG("check_wan_settings() error\n");
		return 0;
	}

	/* set to nvram & wan settings struct */
	if (!set_wan_settings(type)) {
		DEBUG_MSG("set_wan_settings() error\n");
		return 0;
	}

	/* set common value to nvram */
	nvram_set("wan_proto", wan_settings.protocol);
	if(GetWanMacClone == 0)
		SetCloneMac(mac);

	/* ken modfiy for dns can't show in status page */
	if (((strlen(dns1) != 0)&&(strcmp(dns1,"0.0.0.0") != 0)) || ((strlen(dns2) != 0)&&(strcmp(dns2,"0.0.0.0") != 0))) {
		nvram_set("wan_specify_dns", "1");
	}

	if ((wan_settings.dynamic == "1") && (strcmp(dns1,"0.0.0.0") == 0) && (strcmp(dns2,"0.0.0.0") == 0)) {
		nvram_set("wan_specify_dns", "0");
	}

	/* if dhcp , must have default dns1 */
	if (type != 4) {
		if (strlen(dns1) == 0)
			nvram_set("wan_primary_dns", "0.0.0.0");
		else
			nvram_set("wan_primary_dns", dns1);
	}
	else if ((strlen(dns1) == 0) && (strlen(dns2) == 0))
		nvram_set("wan_primary_dns",nvram_get("lan_ipaddr"));

	if (strlen(dns2) == 0)
		nvram_set("wan_secondary_dns", "0.0.0.0");
	else
		nvram_set("wan_secondary_dns", dns2);

#ifdef OPENDNS
	if (strcmp(open_dns,"1") == 0 || strcmp(open_dns, "true") == 0) {
		nvram_set("opendns_enable", "1");
		nvram_set("dns_relay", "1");
		nvram_set("wan_specify_dns", "1");

#if 0		
		nvram_set("wan_primary_dns", "204.194.232.200");
		nvram_set("wan_secondary_dns", "204.194.234.200");
#else
		nvram_set("opendns_mode", "3");

		nvram_set("wan_primary_dns","208.67.222.123");
		nvram_set("wan_secondary_dns","208.67.220.123");
#endif		
	}
	else {
		nvram_set("opendns_enable", "0");

#if 0
#else
		nvram_set("opendns_mode", "1");
#endif
		//Dynamic DNS that got from WAN protocol
		if ((strcmp(dns1,"0.0.0.0") == 0 && strcmp(dns2,"0.0.0.0") == 0) || (strlen(dns1) == 0 && strlen(dns2) == 0) ) {
			nvram_set("wan_specify_dns", "0");
			nvram_set("wan_primary_dns", "0.0.0.0");
            nvram_set("wan_secondary_dns", "0.0.0.0");
		}
		else {
			//Static DNS
			nvram_set("wan_specify_dns", "1");
			nvram_set("wan_primary_dns", dns1);
			nvram_set("wan_secondary_dns", dns2);
		}
	}
#endif

	return 1;
}

void SetCloneMac(char *mac)
{
	char *wan_protocol = NULL;	
	nvram_set("wan_mac", mac);
	nvram_set("mac_clone_addr", mac);

	wan_protocol = nvram_get("wan_proto");
	if(strcasecmp(wan_protocol, "DHCP") == 0)
	    _system("ifconfig %s 0.0.0.0",nvram_safe_get("wan_eth"));	
}

static void parse_ascii_to_hex(char *ascii,char *ascii_tmp)
{
	int i;

	for (i=0;i<strlen(&ascii_tmp[0]);i++) {
		sprintf(ascii, "%s%02x", ascii, ascii_tmp[i]);
	}
}

static int pure_SetWLanSecurity(char *source_xml)
{
	char enable[8] = {};
	char type[8] = {};
	char wep_key_len[8] = {};
	char key_tmp[512] = {};
	char radioip1[20] = {};
	char radioip2[20] = {};
	char radioport1[8] = {};
	char radioport2[8] = {};
	char wlan0_eap_radius_server_0[64] = {};
	char wlan0_eap_radius_server_1[64] = {};

	memset(key_tmp, 0, sizeof(key_tmp));

	get_element_value(source_xml, enable, "Enabled");
	get_element_value(source_xml, type, "Type");
	get_element_value(source_xml, wep_key_len, "WEPKeyBits");
	get_element_value(source_xml, key_tmp, "Key");
	get_element_value(source_xml, radioip1, "RadiusIP1");
	get_element_value(source_xml, radioport1, "RadiusPort1");
	get_element_value(source_xml, radioip2, "RadiusIP2");
	get_element_value(source_xml, radioport2, "RadiusPort2");

	if (strlen(radioip1) == 0)
		strcpy(radioip1, "0.0.0.0");

	if (strlen(radioport1) == 0)
		strcpy(radioport1, "0");

	if (strlen(radioip2) == 0)
		strcpy(radioip2, "0.0.0.0");

	if (strlen(radioport2) == 0)
		strcpy(radioport2, "0");
		
	if (strlen(wep_key_len) == 0)
		strcpy(wep_key_len, "0");

	DEBUG_MSG("Enabled = %s\n", enable);
	DEBUG_MSG("Type = %s\n", type);
	DEBUG_MSG("WEPKeyBits = %s\n", wep_key_len);
	replace_special_char(key_tmp);
	DEBUG_MSG("Key = %s\n", key_tmp);
	DEBUG_MSG("RadiusIP1 = %s\n", radioip1);
	DEBUG_MSG("RadiusPort1 = %s\n", radioport1);
	DEBUG_MSG("RadiusIP2 = %s\n", radioip2);
	DEBUG_MSG("RadiusPort2 = %s\n", radioport2);

	sprintf(wlan0_eap_radius_server_0, "%s/%s/0", radioip1, radioport1);
	sprintf(wlan0_eap_radius_server_1, "%s/%s/0", radioip2, radioport2);
	nvram_set("wlan0_eap_radius_server_0", wlan0_eap_radius_server_0);
	nvram_set("wlan0_eap_radius_server_1", wlan0_eap_radius_server_1);

	if (strcasecmp(enable, "false") == 0) {
		nvram_set("wlan0_security", "disable");
		return 1;
	}
	else if (strcasecmp(enable, "true") == 0)
		nvram_set("wlan0_wep_default_key", "1");		// set to index 1 for pure network (total is 4)
	else {
		return 0;
	}

	// wpa type need more data to config , use default here
	if (strcasecmp(type, "wep") == 0 && strcasecmp(wep_key_len, "64") == 0 && strlen(key_tmp) == 10) {
		nvram_set("wlan0_security", "wep_open_64");
		nvram_set("wlan0_wep64_key_1", key_tmp);
	}
	else if (strcasecmp(type, "wep") == 0 && strcasecmp(wep_key_len, "128") == 0 && strlen(key_tmp) == 26) {
		nvram_set("wlan0_security", "wep_open_128");
		nvram_set("wlan0_wep128_key_1", key_tmp);
	}
	else if (strcasecmp(type, "wep") == 0 && strcasecmp(wep_key_len, "152") == 0 && strlen(key_tmp) == 32) {
		nvram_set("wlan0_security", "wep_open_152");
		nvram_set("wlan0_wep152_key_1", key_tmp);
	}
	else if (strcasecmp(type, "wpa") == 0 &&
			( (strcasecmp(wep_key_len, "0") == 0) ||
			  (strcasecmp(wep_key_len, "64") == 0) ||
			  (strcasecmp(wep_key_len, "128") == 0) ||
			  (strcasecmp(wep_key_len, "152") == 0) ) ) {
		nvram_set("wlan0_security", "wpa2auto_psk");	// data miss, use default
		nvram_set("wlan0_psk_cipher_type", "aes");		// data miss, use default
		nvram_set("wlan0_psk_pass_phrase", key_tmp);
	}
	else
		return 0;

	if (strlen(nvram_safe_get("blank_status")) != 0)
		nvram_set("blank_status","1");

	nvram_set("disable_wps_pin", "1");
	nvram_set("wps_configured_mode", "5");
	return 1;
}

static int pure_SetWLanRadioSecurity(char *source_xml)
{
	char radioid[16] = {};
	char enable[8] = {};
	char type[20] = {};
	char encryption[16] = {};
	char key_tmp[512] = {};
	char rekey_time[32] ={};
	char radiusip1[20] = {};
	char radiusip2[20] = {};
	char radiusport1[8] = {};
	char radiusport2[8] = {};
	char radiussecret1[256] = {};
	char radiussecret2[256] = {};
	char wlan_eap_radius_server_0[64] = {};
	char wlan_eap_radius_server_1[64] = {};

	memset(key_tmp, 0, sizeof(key_tmp));
	
	get_element_value(source_xml, radioid, "RadioID");
	get_element_value(source_xml, enable, "Enabled");
	get_element_value(source_xml, type, "Type");
	get_element_value(source_xml, encryption, "Encryption");
    get_element_value(source_xml, key_tmp, "Key");
	get_element_value(source_xml, rekey_time, "KeyRenewal");
	get_element_value(source_xml, radiusip1, "RadiusIP1");
	get_element_value(source_xml, radiusport1, "RadiusPort1");
	get_element_value(source_xml, radiussecret1, "RadiusSecret1");
	get_element_value(source_xml, radiusip2, "RadiusIP2");
	get_element_value(source_xml, radiusport2, "RadiusPort2");
	get_element_value(source_xml, radiussecret2, "RadiusSecret2");

	DEBUG_MSG("RadioID = %s\n", radioid);
	DEBUG_MSG("Enabled = %s\n", enable);
	DEBUG_MSG("Type = %s\n", type);
	DEBUG_MSG("Encryption = %s\n", encryption);
	replace_special_char(key_tmp);
	DEBUG_MSG("Key = %s\n", key_tmp);
	DEBUG_MSG("KeyRenewal = %s\n", rekey_time);
	DEBUG_MSG("RadiusIP1 = %s\n", radiusip1);
	DEBUG_MSG("RadiusPort1 = %s\n", radiusport1);
	DEBUG_MSG("RadiusSecret1 = %s\n", radiussecret1);
	DEBUG_MSG("RadiusIP2 = %s\n", radiusip2);
	DEBUG_MSG("RadiusPort2 = %s\n", radiusport2);
	DEBUG_MSG("RadiusSecret2 = %s\n", radiussecret2);

	if (strlen(radiusip1) == 0)
		strcpy(radiusip1, "0.0.0.0");

	if (strlen(radiusport1) == 0)
		strcpy(radiusport1, "0");

	if (strlen(radiusip2) == 0)
		strcpy(radiusip2, "0.0.0.0");

	if (strlen(radiusport2) == 0)
		strcpy(radiusport2, "0");

	if (strcasecmp(radioid, "RADIO_24GHz") == 0)
	{
		if (strcasecmp(enable, "false") == 0) {
			nvram_set("wlan0_security", "disable");
			return 1;
		}
		else if (strcasecmp(enable, "true") == 0)
			nvram_set("wlan0_wep_default_key", "1");	// set to index 1 for pure network (total is 4)
		else
			return 0;

		if (strcasecmp(type, "WEP-OPEN") == 0)
		{
			if (strcasecmp(encryption, "WEP-64") == 0)
				nvram_set("wlan0_security", "wep_open_64");
			else if (strcasecmp(encryption, "WEP-128") == 0)
				nvram_set("wlan0_security", "wep_open_128");

			if (strlen(key_tmp) == 10) {
				nvram_set("wlan0_wep64_key_1", key_tmp);
			}
			else if (strlen(key_tmp) == 26) {
				nvram_set("wlan0_wep128_key_1", key_tmp);
			}
			else
				return 0;
		}
		else if (strcasecmp(type, "WEP-SHARED") == 0)
		{
			if (strcasecmp(encryption, "WEP-64") == 0)
				nvram_set("wlan0_security", "wep_share_64");
			else if (strcasecmp(encryption, "WEP-128") == 0)
				nvram_set("wlan0_security", "wep_share_128");

			if (strlen(key_tmp) == 10) {
				nvram_set("wlan0_wep64_key_1", key_tmp);
			}
			else if (strlen(key_tmp) == 26) {
				nvram_set("wlan0_wep128_key_1", key_tmp);
			}
			else
				return 0;
		}
		else if (strcasecmp(type, "WPA-PSK") == 0)
		{
			nvram_set("wlan0_security", "wpa_psk");
			if (strcasecmp(encryption,"AES") == 0)
				nvram_set("wlan0_psk_cipher_type", "aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan0_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 	nvram_set("wlan0_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan0_gkey_rekey_time", rekey_time);
			nvram_set("wlan0_psk_pass_phrase", key_tmp);
		}
		else if (strcasecmp(type, "WPA-RADIUS") == 0)
		{
			nvram_set("wlan0_security", "wpa_eap");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan0_psk_cipher_type", "aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan0_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 nvram_set("wlan0_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan0_gkey_rekey_time", rekey_time);

			sprintf(wlan_eap_radius_server_0, "%s/%s/%s", radiusip1, radiusport1, radiussecret1);
			sprintf(wlan_eap_radius_server_1, "%s/%s/%s", radiusip2, radiusport2, radiussecret2);
			nvram_set("wlan0_eap_radius_server_0", wlan_eap_radius_server_0);
			nvram_set("wlan0_eap_radius_server_1", wlan_eap_radius_server_1);
		}
		else if (strcasecmp(type, "WPA2-PSK") == 0)
		{
			nvram_set("wlan0_security", "wpa2_psk");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan0_psk_cipher_type", "aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan0_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 nvram_set("wlan0_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan0_gkey_rekey_time", rekey_time);
			nvram_set("wlan0_psk_pass_phrase", key_tmp);
		}
		else if (strcasecmp(type, "WPAORWPA2-PSK") == 0)
		{
			nvram_set("wlan0_security", "wpa2auto_psk");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan0_psk_cipher_type", "aes");
			else if (strcasecmp(encryption,"TKIP") == 0)
				nvram_set("wlan0_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption,"TKIPORAES") == 0)
				 nvram_set("wlan0_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan0_gkey_rekey_time", rekey_time);
			nvram_set("wlan0_psk_pass_phrase", key_tmp);
		}
		else if (strcasecmp(type, "WPA2-RADIUS") == 0)
		{
			nvram_set("wlan0_security", "wpa2_eap");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan0_psk_cipher_type","aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan0_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 nvram_set("wlan0_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan0_gkey_rekey_time", rekey_time);

			sprintf(wlan_eap_radius_server_0, "%s/%s/%s", radiusip1, radiusport1, radiussecret1);
			sprintf(wlan_eap_radius_server_1, "%s/%s/%s", radiusip2, radiusport2, radiussecret2);
			DEBUG_MSG("wlan0_eap_radius_server_0 = %s\n", wlan_eap_radius_server_0);
			DEBUG_MSG("wlan0_eap_radius_server_1 = %s\n", wlan_eap_radius_server_1);
			nvram_set("wlan0_eap_radius_server_0", wlan_eap_radius_server_0);
			nvram_set("wlan0_eap_radius_server_1", wlan_eap_radius_server_1);
		}
	}
#ifdef USER_WL_ATH_5GHZ
	else if(strcasecmp(radioid, "RADIO_5GHz") == 0)
	{
		if (strcasecmp(enable,"false") == 0) {
			nvram_set("wlan1_security", "disable");
			return 1;
		}
		else if (strcasecmp(enable, "true") == 0)
			nvram_set("wlan1_wep_default_key", "1");	// set to index 1 for pure network (total is 4)
		else
			return 0;

		if (strcasecmp(type,"WEP-OPEN") == 0)
		{
			if (strcasecmp(encryption, "WEP-64") == 0)
				nvram_set("wlan1_security", "wep_open_64");
			else if (strcasecmp(encryption, "WEP-128") == 0)
				nvram_set("wlan1_security", "wep_open_128");

			if (strlen(key_tmp) == 10) {
				nvram_set("wlan1_wep64_key_1", key_tmp);
			}
			else if (strlen(key_tmp) == 26) {
				nvram_set("wlan1_wep128_key_1", key_tmp);
			}
			else
				return 0;
		}
		else if (strcasecmp(type, "WEP-SHARED") == 0)
		{
			if (strcasecmp(encryption, "WEP-64") == 0)
				nvram_set("wlan1_security", "wep_share_64");
			else if (strcasecmp(encryption, "WEP-128") == 0)
				nvram_set("wlan1_security", "wep_share_128");

			if (strlen(key_tmp) == 10) {
				nvram_set("wlan1_wep64_key_1", key_tmp);
			}
			else if (strlen(key_tmp) == 26) {
				nvram_set("wlan1_wep128_key_1", key_tmp);
			}
			else
				return 0;
		}
		else if (strcasecmp(type,"WPA-PSK") == 0)
		{
			nvram_set("wlan1_security", "wpa_psk");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan1_psk_cipher_type", "aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan1_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 	nvram_set("wlan1_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan1_gkey_rekey_time", rekey_time);
			nvram_set("wlan1_psk_pass_phrase", key_tmp);
		}
		else if(strcasecmp(type, "WPA-RADIUS") == 0)
		{
			nvram_set("wlan1_security", "wpa_eap");
			if (strcasecmp(encryption,"AES") == 0)
				nvram_set("wlan1_psk_cipher_type", "aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan1_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 nvram_set("wlan1_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan1_gkey_rekey_time",rekey_time);

			sprintf(wlan_eap_radius_server_0, "%s/%s/%s", radiusip1, radiusport1, radiussecret1);
			sprintf(wlan_eap_radius_server_1, "%s/%s/%s", radiusip2, radiusport2, radiussecret2);
			nvram_set("wlan1_eap_radius_server_0", wlan_eap_radius_server_0);
			nvram_set("wlan1_eap_radius_server_1", wlan_eap_radius_server_1);
		}
		else if (strcasecmp(type, "WPA2-PSK") == 0)
		{
			nvram_set("wlan1_security", "wpa2_psk");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan1_psk_cipher_type", "aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan1_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 nvram_set("wlan1_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan1_gkey_rekey_time", rekey_time);
			nvram_set("wlan1_psk_pass_phrase", key_tmp);
		}
		else if (strcasecmp(type, "WPAORWPA2-PSK") == 0)
		{
			nvram_set("wlan1_security", "wpa2auto_psk");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan1_psk_cipher_type", "aes");
			else if (strcasecmp(encryption,"TKIP") == 0)
				nvram_set("wlan1_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption,"TKIPORAES") == 0)
				 nvram_set("wlan1_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan1_gkey_rekey_time", rekey_time);
			nvram_set("wlan1_psk_pass_phrase", key_tmp);
		}		
		else if(strcasecmp(type,"WPA2-RADIUS") == 0)
		{
			nvram_set("wlan1_security", "wpa2_eap");
			if (strcasecmp(encryption, "AES") == 0)
				nvram_set("wlan1_psk_cipher_type", "aes");
			else if (strcasecmp(encryption, "TKIP") == 0)
				nvram_set("wlan1_psk_cipher_type", "tkip");
			else if	(strcasecmp(encryption, "TKIPORAES") == 0)
				 nvram_set("wlan1_psk_cipher_type", "both");

			if (!check_rekey_time(rekey_time))
				return 0;

			nvram_set("wlan1_gkey_rekey_time", rekey_time);

			sprintf(wlan_eap_radius_server_0, "%s/%s/%s", radiusip1, radiusport1, radiussecret1);
			sprintf(wlan_eap_radius_server_1, "%s/%s/%s", radiusip2, radiusport2, radiussecret2);
			DEBUG_MSG("wlan1_eap_radius_server_0 = %s\n", wlan_eap_radius_server_0);
			DEBUG_MSG("wlan1_eap_radius_server_1 = %s\n", wlan_eap_radius_server_1);			
			nvram_set("wlan1_eap_radius_server_0", wlan_eap_radius_server_0);
			nvram_set("wlan1_eap_radius_server_1", wlan_eap_radius_server_1);
		}
	}
#endif
	else
		return 0;


	if (strlen(nvram_safe_get("blank_status")) != 0)
		nvram_set("blank_status", "1");

	nvram_set("disable_wps_pin", "1");
	nvram_set("wps_configured_mode", "5");
	return 1;
}


static int pure_SetMACFilters2(char *source_xml)
{
	char enable[6] = {};
	char tmp[62] = {};
	char name[32] = {};
	char mac[18] = {};
	int mac_index = 0;

	memset(is_allow_list, 0, sizeof(is_allow_list));
	/* get element value from response of pure */
	get_element_value(source_xml, enable, "Enabled");
	get_element_value(source_xml, is_allow_list, "IsAllowList");

	DEBUG_MSG("Enabled = %s\n", enable);
	DEBUG_MSG("IsAllowList = %s\n", is_allow_list);

	/* transfer type of pure value to nvram*/
	if (strcasecmp(enable, "false") == 0)								// [false , false/true ]
		nvram_set("mac_filter_type", "disable");
	else if (strcasecmp( enable, "true" ) == 0) {
		if (strcasecmp(is_allow_list, "true") == 0)        // [true , true ]
			nvram_set("mac_filter_type", "list_allow");
		else if (strcasecmp(is_allow_list , "false") == 0)	// [ true , false ]
			nvram_set("mac_filter_type", "list_deny");
		else
			return 0;
	}
	else
		return 0;

	/* initial mac filter */
	for (mac_index=0; mac_index<ARRAYSIZE(mac_filter_table); mac_index++) {
		nvram_set(mac_filter_table[mac_index].mac_address,"0/name/00:00:00:00:00:00/Always");
		usleep(10000);
	}

	/* set mac value of pure to nvram */
	for (mac_index=0; mac_index<ARRAYSIZE(mac_filter_table); mac_index++) {
		if ((source_xml = find_next_element(source_xml, "MACInfo")) != NULL) {
			get_element_value(source_xml, mac, "MacAddress");
			get_element_value(source_xml, name, "DeviceName");

			DEBUG_MSG("MacAddress = %s\n", mac);
			DEBUG_MSG("DeviceName = %s\n", name);

			/* check if invalid mac */
			if (!check_mac_filter(mac)) {
				return 0;
			}

			if (strcmp(name, "") == 0 || name == NULL) {
				DEBUG_MSG("replace name from empty to 0\n");
				strcpy(name, "0");
			}

			sprintf(tmp, "1/%s/%s/Always", name, mac);
			DEBUG_MSG("mac_filter_%02d = %s\n", mac_index, tmp);

			// make format , like 1/name/00:01:02:03:04:00/Always
			nvram_set(mac_filter_table[mac_index].mac_address,tmp);
            usleep(10000);

			/* initial mac tmp */
			memset(tmp, 0, 62);
			memset(name, 0, 32);
			memset(mac, 0, 18);
		}
		else
			break;
	}

	return 1;
}

static int pure_SetRouterLanSettings(char *source_xml)
{
	char dhcp_enable[8] = {};
	char ip[20] = {};
	char mask[20] = {};
	int rangeStart, rangeEnd;
 	struct in_addr start_addr, end_addr, myaddr, mymask;

	get_element_value(source_xml, ip, "RouterIPAddress");
	get_element_value(source_xml, mask, "RouterSubnetMask");
	get_element_value(source_xml, dhcp_enable, "DHCPServerEnabled");

	DEBUG_MSG("RouterIPAddress = %s\n", ip);
	DEBUG_MSG("RouterSubnetMask = %s\n", mask);
	DEBUG_MSG("DHCPServerEnabled = %s\n", dhcp_enable);

	if (strcasecmp(dhcp_enable, "true") == 0)
		nvram_set("dhcpd_enable", "1");
	else if (strcasecmp(dhcp_enable, "false") == 0)
		nvram_set("dhcpd_enable", "0");

	if (!check_lan_settings(ip,mask))
		return 0;

	nvram_set("lan_ipaddr", ip);
	nvram_set("lan_netmask", mask);
	inet_aton(ip, &myaddr);
	inet_aton(mask, &mymask);

	if (((myaddr.s_addr & 0x00FF) == 1) && ((mymask.s_addr & 0x00FF) == 0)) {
		rangeStart = 100;
		rangeEnd = 199;
	}
	else if (!strcmp(mask, "255.255.255.248")) {
		rangeStart = 2;
		rangeEnd = 6;
 	}
	else if (!strcmp(mask, "255.255.255.252")) {
		rangeStart = 1;
		rangeEnd = 2;
  	}
	else {
		rangeStart = 10;
		rangeEnd = 20;
	}

	start_addr.s_addr = (myaddr.s_addr & mymask.s_addr) + rangeStart;
	end_addr.s_addr = (myaddr.s_addr & mymask.s_addr) + rangeEnd;
	nvram_set("dhcpd_start", inet_ntoa(start_addr));
	nvram_set("dhcpd_end", inet_ntoa(end_addr));

	if ((strcmp(ip, "192.168.0.65") == 0) && (strcmp(mask, "255.255.255.252"))) {
	if (nvram_match("asp_temp_71", "1") == 0){
		printf("Enable traffic shaping after pure test\n");
		nvram_set("traffic_shaping", "1");
		nvram_set("asp_temp_71", "");	
		nvram_commit();
	}
	else if (nvram_match("asp_temp_71", "0") == 0){
		nvram_set("asp_temp_71", "");		
		nvram_commit();
	}
	}

	return 1;
}

static int pure_AddPortMappings(char *source_xml)
{
	char name[32] = {};
	char ip[16] = {};
	char protocol[5] = {};
	char exter_port[7] = {};
	char inter_port[7] = {};
	char vs_rule[300] = {};
	char vs_value[300] = {};
	int rule_index = 0;

	/* value in nvram */
	char *enable_n, *name_n, *protocol_n, *exter_port_n;
	char *ip_n, *inter_port_n, *schedule_n, *inbound_n;

	int full = 1;
	char *vs_rule_tt = NULL;

	/* get element value from response of pure */
	while(1) {
		if ((source_xml = find_next_element(source_xml, "AddPortMapping")) != NULL)
		{
			get_element_value(source_xml, name, "PortMappingDescription");
			get_element_value(source_xml, ip, "InternalClient");
			get_element_value(source_xml, protocol, "PortMappingProtocol");
			get_element_value(source_xml, exter_port, "ExternalPort");
			get_element_value(source_xml, inter_port, "InternalPort");

			if (strlen(name) == 0)
				strcpy(name, "allow_null_for_test");

			DEBUG_MSG("PortMappingDescription = %s\n", name);
			DEBUG_MSG("InternalClient = %s\n", ip);
			DEBUG_MSG("PortMappingProtocol = %s\n", protocol);
			DEBUG_MSG("ExternalPort = %s\n", exter_port);
			DEBUG_MSG("InternalPort = %s\n", inter_port);

			for (rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++) {
				vs_rule_tt = nvram_get(vs_rule_table[rule_index].rule);
				if (strcmp(vs_rule_tt, "0//TCP/0/0/0.0.0.0/Always/Allow_All") == 0 ||
					strcmp(vs_rule_tt, "0//6/0/0/0.0.0.0/Always/Allow_All" ) == 0 	||
					strlen(vs_rule_tt) == 0)
					full = 0;
			}

			if (full == 1)
				return 0;

			/* test if xml value is already in nvram */
			for (rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++) {
				sprintf(vs_rule, "vs_rule_%02d", rule_index);
				strcpy(vs_value, nvram_safe_get(vs_rule));
				//DEBUG_MSG("vs_rule_%02d = %s\n", rule_index, vs_value);

				/* if default value	, then ignore*/
				if (strcmp(vs_value, "0//6/0/0/0.0.0.0/Always/Allow_All") == 0 || strlen(vs_value) == 0)
					continue;

				getStrtok(vs_value, "/", "%s %s %s %s %s %s %s %s",
					&enable_n, &name_n, &protocol_n, &exter_port_n, &inter_port_n, &ip_n, &schedule_n, &inbound_n);

				/* compare xml value and nvram value */
				if (strcmp(exter_port, exter_port_n) == 0 && strcmp(protocol, protocol_n) == 0)
					return 0;
			}

			/* transfer xml value to vs_rule of nvram */
			sprintf(vs_rule,"1/%s/%s/%s/%s/%s/Always/Allow_All",
				name, protocol, exter_port, inter_port, ip);

			/* if vs_rule = 0//TCP/0/0/0.0.0.0/Always/Allow_All (default value) , then add to nvram */
			for (rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++){
				if (strcmp(nvram_get(vs_rule_table[rule_index].rule),"0//TCP/0/0/0.0.0.0/Always/Allow_All") == 0 ||
					strcmp(nvram_get(vs_rule_table[rule_index].rule),"0//6/0/0/0.0.0.0/Always/Allow_All") == 0 	||
					strlen(nvram_get(vs_rule_table[rule_index].rule)) == 0) {
					nvram_set(vs_rule_table[rule_index].rule,vs_rule);
					break;
				}
			}
		}
		else
			break;
	}

	return 1;
}

static int pure_DelPortMappings(char *source_xml)
{
	char protocol[5] = {};
	char exter_port[7] = {};
	int rule_index = 0;
	char vs_rule[300] = {};
	char vs_value[300] = {};

	/* value of nvram */
	char *enable_n, *name_n, *protocol_n, *exter_port_n;
	char *ip_n, *inter_port_n, *schedule_n, *inbound_n;

	/* get element value from response of pure */
	get_element_value(source_xml, protocol, "PortMappingProtocol");
	get_element_value(source_xml, exter_port, "ExternalPort");

	DEBUG_MSG("PortMappingProtocol = %s\n", protocol);
	DEBUG_MSG("ExternalPort = %s\n", exter_port);

	/* search vs_rule from nvram */
	for (rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++) {
		sprintf(vs_rule, "vs_rule_%02d", rule_index);
		strcpy(vs_value, nvram_safe_get(vs_rule));
		DEBUG_MSG("vs_rule_%02d = %s\n", rule_index, vs_value);

		/* if default value	, then ignore*/
		if (strcmp(vs_value, "0//6/0/0/0.0.0.0/Always/Allow_All") == 0 || strlen(vs_value) == 0)
			continue;

		getStrtok(vs_value, "/", "%s %s %s %s %s %s %s %s",
				&enable_n, &name_n, &protocol_n, &exter_port_n, &inter_port_n, &ip_n, &schedule_n, &inbound_n);

		/* delete vs_rule from nvram if match xml element */
		if (strcmp(protocol, protocol_n) == 0 && strcmp(exter_port, exter_port_n) == 0) {
			nvram_set(vs_rule_table[rule_index].rule, "0//6/0/0/0.0.0.0/Always/Allow_All");
			break;
		}
	}
	return 1;
}

static int pure_StartCableModemCloneWanMac(char *source_xml)
{
    char *wan_clone = NULL;
    wan_clone = nvram_safe_get("wan_eth");	
    
	DEBUG_MSG("pure_StartCableModemCloneWanMac \n");
	_system("/sbin/macclone %s &", wan_clone);
   	return 1;
}

static int pure_SetCableModemCloneWanMac(char *source_xml, char *dest_xml)
{
	char mac_addr[32];
	printf("pure_SetCableModemCloneWanMac \n");
	get_element_value(source_xml, mac_addr, "CableModemCloneWanMacAddr");
	SetCloneMac(mac_addr);
		
	/* insert settings */
	return 1;
}

static int pure_SetAccessPointMode(char *source_xml, char *dest_xml)
{
	char is_ap_mode[6] = {};

	/* get element value from response of pure */
	get_element_value(source_xml, is_ap_mode, "IsAccessPoint");

	/* insert settings */
	return 1;
}

static int pure_SetRenewWanConnection(char *source_xml, char *dest_xml)
{
	char renew_time[8] = {};

	/* get element value from response of pure */
	get_element_value(source_xml, renew_time, "RenewTimeout");

	/* This MUST stay distinct from the Reboot method.
	   RenewWanConnection keeps all LAN DHCP information intact
	   and has less impact on the device than the Reboot method would.
	*/
	if (atoi(renew_time) < 20)
		return 0;

	return 1;
}

static int pure_SetMultipleActions(char *source_xml, char *dest_xml)
{
	char SetMulAct_result[8] = {};
	char SetDev_result[8] = {};
	char SetWan_result[8] = {};
	char SetWlanSet_result[8] = {};
	char SetWlanSec_result[8] = {};

	int action_result_error = 0;
	char *source_tmp;
	
	source_tmp = strstr(source_xml, "SetDeviceSettings");
	if (source_tmp) {
		DEBUG_MSG("SetDeviceSettings ! \n");
			if (!pure_SetDeviceSettings(source_tmp)) {
				DEBUG_MSG("SetDevicesettings fail\n");
				strcpy(SetDev_result,"ERROR");
				pure_atrs.set_nvram_error = 1;
			}
			else {
				strcpy(SetDev_result,"OK");
				DEBUG_MSG("SetDevicesettings OK\n");
			}
	}

	source_tmp = strstr(source_xml, "SetWanSettings");
	if (source_tmp) {
		DEBUG_MSG("SetWanSettings ! \n");
			if (!pure_SetWanSettings(source_tmp)) {
				DEBUG_MSG("SetWanSettingsResult fail\n");
				strcpy(SetWan_result,"ERROR");
				pure_atrs.set_nvram_error = 1;
			}
			else {
				strcpy(SetWan_result,"REBOOT");
				DEBUG_MSG("SetWanSettingsResult REBOOT\n");
			}
	}
	
	source_tmp = strstr(source_xml, "SetWLanRadioSettings");
	if (source_tmp) {
		DEBUG_MSG("SetWLanRadioSettings ! \n");
			if (!pure_SetWLanRadioSettings(source_tmp)) {
				DEBUG_MSG("SetWanSettingsResult fail\n");
				strcpy(SetWlanSet_result,"ERROR");
				pure_atrs.set_nvram_error = 1;
			}
			else {
				strcpy(SetWlanSet_result,"REBOOT");
				DEBUG_MSG("SetWanSettingsResult REBOOT\n");
			}
	}
	
	source_tmp = strstr(source_xml, "SetWLanRadioSecurity");
	if (source_tmp) {
		DEBUG_MSG("SetWLanRadioSecurity ! \n");
			if (!pure_SetWLanRadioSecurity(source_tmp)) {
				DEBUG_MSG("SetWanSettingsResult fail\n");
				strcpy(SetWlanSec_result,"ERROR");
				pure_atrs.set_nvram_error = 1;
			}
			else {
				strcpy(SetWlanSec_result,"REBOOT");
				DEBUG_MSG("SetWanSettingsResult REBOOT\n");
			}
	}

	if(action_result_error == 0)
		strcpy(SetMulAct_result,"REBOOT");
	else
		strcpy(SetMulAct_result,"ERROR");
		
	/* set value to xml */
	xmlWrite(dest_xml, SetMultipleActionResult_xml, SetMulAct_result, SetDev_result, SetWan_result, SetWlanSet_result, SetWlanSec_result);
	return 1;
}

static int pure_SetTriggerADIC(char *source_xml, char *dest_xml)
{
    char wan_type[20];
	char ADIC_val[8] = {0};
	struct ethernetport wan_st;
	
	get_element_value(source_xml, ADIC_val, "TriggerADIC");
	DEBUG_MSG("ADIC_value = %s\n", ADIC_val);
	
    strcpy(wan_type, "ERROR");

	VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, wan_st.connected);
	strncpy(wan_st.port,"WAN",3);
	if (!strncmp(wan_st.connected,"connect",10))
	{
		if(strcmp(ADIC_val,"true")==0){
			_system("rm -f /var/tmp/pppoe_find /var/tmp/test_dhcp_res.txt");
			_system("pppoe -I %s -d -A -t 1", nvram_safe_get("wan_eth"));
			_system("killall -SIGALRM udhcpc"); //dhcp_discover 
			usleep(1000000);
			FILE *fp;
			char buf[20];
			fp = fopen("/var/tmp/pppoe_find", "r");
		    if (fp) {
		    	fgets(buf, 16, fp);
		    	if (buf[0] == '1') {
		    	    strcpy(wan_type, "OK_PPPoE");
		    	}
		    	fclose(fp);	
		    }
		    
			fp = fopen("/var/tmp/test_dhcp_res.txt", "r");
		    if (fp) {
		    	fgets(buf, 16, fp);		
		    	if (buf[0] == '1') {
		    	    strcpy(wan_type, "OK_DHCP");
		    	}
		    	fclose(fp);	
		    }
		}
	}

	/* set value to xml */
	xmlWrite(dest_xml, SetTriggerADICResult_xml, wan_type);
	return 1;
}

int get_pure_content(int pure_conn_fd,char *rec_buf, char *action_string, int length)
{
	char *tag = NULL;
	char tmp[MAX_ARRAY_TMP] = {};
	char *xml_content = NULL;
	int action_type = 0;

	/* session error */
	if (pure_atrs.conn_fd < 0) {
		printf("Session error\n");
		return action_type;
	}

	/*Since QRS API not add \r\n at the rear of the packet, we need to read the exactly size we wan for avoiding close the socket by reading EOF*/
	do {
		memset(tmp, 0, sizeof(tmp));
		tmp[ MAX_ARRAY_TMP-1 ] = '\0';
		length -= safe_fread( tmp, 1, MIN( MAX_ARRAY_TMP, length),pure_atrs.stream);

		if ((xml_content = strstr( tmp, "<soap:Envelope")) != NULL) {
			action_type = return_pure_action(pure_atrs.action_string, xml_content);
			action_type = mapping_SoapAction_xml(action_type, xml_content);
			snprintf( rec_buf, MAX_ARRAY_TMP, "%s", xml_content);
		}

		if (tag = strstr(rec_buf, "</soap:Envelope>")) {
			break;
		}
	} while (length > 0);

	if (tag == NULL) {	// soap Envelope not complete
		printf("Soap Envelope not complete\n");
		action_type = 0;
	}

	return action_type;
}

static int pure_GetMACFilters2(char *dest_xml, char *general_xml, char *mac_info_xml)
{
	char *enable = NULL;
	char *mac_tmp = NULL;
	char *mac_tmp_index = NULL;
	char name[32] = {};
	char mac[18] = {};
	char *mac_filter_type = NULL;
	int mac_index = 0;
	char mac_info_tmp[4000] = {};
	char mac_info_tmp_t[100] = {};
	char *mac_sub_index = NULL;

	memset(mac_info_tmp, 0, 4000);

	/* transfer nvram value of pure */
	mac_filter_type = nvram_safe_get("mac_filter_type");
	if (!strcmp(mac_filter_type, "disable")) {
		enable = "false";
	}
	else if (!strcmp(mac_filter_type, "list_allow")) {
		enable = "true";
	}
	else if (!strcmp(mac_filter_type, "list_deny")) {
		enable = "true";
	}

	/* transfer nvram value of mac to pure, is a loop for MACInfo_xml in pure_xml.c */
	for (mac_index=0; mac_index<ARRAYSIZE(mac_filter_table); mac_index++) {
		mac_tmp = nvram_safe_get(mac_filter_table[mac_index].mac_address);  /* 0/name/00:01:02:03:04:00/Always */
		if (strlen(mac_tmp) == 0)
		    continue;
		mac_sub_index = strstr(mac_tmp, "/");
		mac_tmp_index = strstr(mac_sub_index+1, "/");
		if (strcmp(mac_tmp, "0/name/00:00:00:00:00:00/Always")) {	// if not default value
			DEBUG_MSG("mac_filter = %s\n", mac_tmp);
			strncat(name, mac_sub_index+1, mac_tmp_index - mac_sub_index - 1);
			if (strcmp(name, "0") == 0) {
				printf("replace name from 0 to empty\n");
				strcpy(name, "");
			}
			strncat(mac, mac_tmp_index+1, 17);
			xmlWrite(mac_info_tmp_t, mac_info_xml, mac,name);
			strcat(mac_info_tmp, mac_info_tmp_t);
		}
		/* initial mac tmp */
		memset(mac_info_tmp_t,0,100);
		memset(mac,0,18);
		memset(name,0,32);
	}

	xmlWrite(dest_xml, general_xml, "OK", enable,is_allow_list, mac_info_tmp);
	return 1;
}

static int pure_GetNetworkStats(char *dest_xml, char *general_xml)
{
	FILE *fp;
	fp = fopen("/var/tmp/qos_estimation.tmp","r");
	if(nvram_match("traffic_shaping", "1") == 0 && nvram_match("auto_uplink", "1") == 0 &&
		fp != NULL ) {
			xmlWrite(dest_xml, general_xml,
			"0", "0", "0", "0",
			"0", "0", "0", "0");
			fclose(fp);
			return 1;
	}

	/* initial network stats */
	sprintf(pure_atrs.lanTxPkts, "%s", display_lan_pkts(TXPACKETS) ? : "0");
	sprintf(pure_atrs.lanRxPkts, "%s", display_lan_pkts(RXPACKETS) ? : "0");
	sprintf(pure_atrs.lanRxBytes, "%s", display_lan_bytes(RXBYTES) ? : "0");
	sprintf(pure_atrs.lanTxBytes, "%s", display_lan_bytes(TXBYTES) ? : "0");
	sprintf(pure_atrs.wlanTxPkts, "%s", display_wlan_pkts(TXPACKETS) ? : "0");
	sprintf(pure_atrs.wlanRxPkts, "%s", display_wlan_pkts(RXPACKETS) ? : "0");
	sprintf(pure_atrs.wlanTxBytes, "%s", display_wlan_bytes(TXBYTES) ? : "0");
	sprintf(pure_atrs.wlanRxBytes, "%s", display_wlan_bytes(RXBYTES) ? : "0");

	/* add stats to xml */
	xmlWrite(dest_xml, general_xml,
			pure_atrs.lanRxPkts, pure_atrs.lanTxPkts, pure_atrs.lanRxBytes, pure_atrs.lanTxBytes,
			pure_atrs.wlanRxPkts, pure_atrs.wlanTxPkts, pure_atrs.wlanRxBytes, pure_atrs.wlanTxBytes);

	return 1;
}

static int pure_GetRouterLanSettings(char *dest_xml, char *general_xml)
{
	char *nvram_dhcp_enable = NULL;
	char *dhcp_enable = NULL;

	DEBUG_MSG("RouterIPAddress = %s\n", nvram_get("lan_ipaddr"));
	DEBUG_MSG("RouterSubnetMask = %s\n", nvram_get("lan_netmask"));
	DEBUG_MSG("DHCPServerEnabled = %s\n", nvram_get("dhcpd_enable"));

	/* get from nvram and parse */
	nvram_dhcp_enable = nvram_get("dhcpd_enable");
	if (strncmp(nvram_dhcp_enable, "0", 1) == 0)
		dhcp_enable = "false";
	else if (strncmp(nvram_dhcp_enable, "1", 1) == 0)
		dhcp_enable = "true";

	xmlWrite(dest_xml, general_xml, nvram_get("lan_ipaddr"), nvram_get("lan_netmask"), dhcp_enable);
	return 1;
}

#ifdef OPENDNS

extern char opendns_token[32+1]; // 32 bytes
extern int opendns_token_time;
extern int opendns_token_failcode;

static int pure_GetOpenDNS(char *dest_xml, char *general_xml)
{
	int mode;
    char mode_str[32]={0};
	char enable_str[32];

	strcpy(mode_str,"");
	mode = 0;
	if( nvram_safe_get("opendns_mode")) {
		mode = atol( nvram_get("opendns_mode"));
	}

	if( mode == 4) {
		strcpy(mode_str,"Advanced");
	}
	else if( mode == 3) {
		strcpy(mode_str,"FamilyShield");
	}
	else if( mode == 2) {
		strcpy(mode_str,"Parental");
	}

	if (strcmp(nvram_get("opendns_enable"), "1") == 0) {
		strcpy(enable_str, "true");
	}
	else {
		strcpy(enable_str, "false");
	}

	DEBUG_MSG("opendns_enable = %s\n", enable_str);
	DEBUG_MSG("opendns_mode = %s\n", mode_str);
	DEBUG_MSG("opendns_deviceid = %s\n", nvram_get("opendns_deviceid"));

// devicekey issue
	int now_time = time((time_t *) NULL);

// find old use old
	int create_new = 0;
	
	if( now_time > (opendns_token_time + (30 * 60)) ) {
		// timeout 30*60 seconds
		create_new = 1;
	}

	// check token
	if( strlen(opendns_token) < 8) {
		// token fail
		create_new = 1;
	}

	if( create_new ) {
		char tmp[16];
		sprintf(tmp,"%08x",rand());
		strcpy(opendns_token,tmp);
		sprintf(tmp,"%08x",rand());
		strcat(opendns_token,tmp);
	}
	
	opendns_token_failcode = 0;
	opendns_token_time = time((time_t *) NULL);

	sprintf(dest_xml, general_xml, 
		"OK",
		enable_str,
		mode_str,
		nvram_get("opendns_deviceid"),
		opendns_token
	);
	
	return 1;
}
#endif

static int pure_GetDeviceSettings(char *dest_xml, char *general_xml)
{
    char device_name[128];

	DEBUG_MSG("Type = %s\n", nvram_get("pure_type"));
	DEBUG_MSG("DeviceName = %s\n", nvram_get("pure_device_name"));
	DEBUG_MSG("VendorName = %s\n", nvram_get("pure_vendor_name"));
	DEBUG_MSG("ModelDescription = %s\n", nvram_get("pure_model_description"));
	DEBUG_MSG("ModelName = %s\n", nvram_get("model_number"));
	DEBUG_MSG("FirmwareVersion = %s\n", VER_FIRMWARE);
	DEBUG_MSG("PresentationURL = %s\n", nvram_get("pure_presentation_url"));
#ifdef AUTHGRAPH	
	DEBUG_MSG("CAPTCHA = %s\n", nvram_safe_get("graph_auth_enable"));	
#endif

	strcpy(device_name, nvram_get("pure_device_name"));
	replace_get_special_char(device_name);

	sprintf(dest_xml, general_xml, nvram_get("pure_type"),
		device_name,
		nvram_get("pure_vendor_name"),
		nvram_get("pure_model_description"),
		nvram_get("model_number"),
		VER_FIRMWARE,
		nvram_get("pure_presentation_url")
#ifdef AUTHGRAPH
		, (atoi(nvram_safe_get("graph_auth_enable")) == 0) ? "false" : "true"
#endif
	);
	
	return 1;
}

static int pure_GetWanSettings(char *dest_xml, char *gereral_xml)
{
	char *wan_protocol = NULL;
	char *parse_result = "ERROR";
	int type = 0;

	if(nvram_match("asp_temp_71", "1") == 0 || nvram_match("asp_temp_71", "0") == 0)
		parse_result = "ERROR";
	else
		parse_result = "OK";
		
	/* get wan protocol type */
	wan_protocol = nvram_get("wan_proto");

#ifdef OPENDNS
	if (strlen(nvram_safe_get("opendns_enable")) != 0) {
		if (nvram_match("opendns_enable", "1") == 0)
			wan_settings.opendns = "true";
		else
			wan_settings.opendns = "false";
	}
	else
		wan_settings.opendns = "false";
#endif

	char pure_wan_mtu[32];
	strcpy(pure_wan_mtu, nvram_safe_get("wan_mtu"));
	if (strcasecmp(wan_protocol, "static") == 0) {
		strcpy(pure_wan_mtu, nvram_safe_get("wan_mtu"));
		wan_settings.protocol = "Static";
		type = 1;
	}
	else if (strcasecmp(wan_protocol, "pppoe") == 0){
		strcpy(pure_wan_mtu, nvram_safe_get("wan_pppoe_mtu"));
		if (strcmp(nvram_get("wan_pppoe_dynamic_00"), "0") == 0) {
			wan_settings.protocol = "StaticPPPoE";
			type = 2;
		}
		else if (strcmp(nvram_get("wan_pppoe_dynamic_00"), "1") == 0) {
			wan_settings.protocol = "DHCPPPPoE";
			type = 3;
		}
	}
	else if (strcasecmp(wan_protocol, "dhcpc") == 0) {
		strcpy(pure_wan_mtu, nvram_safe_get("wan_mtu"));
		wan_settings.protocol = "DHCP";
		type = 4;
	}
	else if (strcasecmp(wan_protocol, "pptp") == 0) {
		strcpy(pure_wan_mtu, nvram_safe_get("wan_pptp_mtu"));
		if (strcmp(nvram_get("wan_pptp_dynamic"), "0") == 0){
			wan_settings.protocol = "StaticPPTP";
			type = 5;
		}
		else if (strcmp(nvram_get("wan_pptp_dynamic"), "1") == 0){
			wan_settings.protocol = "DynamicPPTP";
			type = 6;
		}
	}
	else if (strcasecmp(wan_protocol, "l2tp") == 0) {
		strcpy(pure_wan_mtu, nvram_safe_get("wan_l2tp_mtu"));
		if (strcmp(nvram_get("wan_l2tp_dynamic"),"0") == 0) {
			wan_settings.protocol = "StaticL2TP";
			type = 7;
		}
		else if(strcasecmp(nvram_get("wan_l2tp_dynamic"),"1") == 0) {
			wan_settings.protocol = "DynamicL2TP";
			type = 8;
		}
	}
	else
		return 0;

	/* get wan settings from nvram to by wan_settings struction */
	if (get_wan_settings(type)) {
		parse_result = "OK";
	}

	char mac_addr[32];
#ifdef SET_GET_FROM_ARTBLOCK
	if (artblock_get("wan_mac") == NULL)
		strcpy(mac_addr, nvram_safe_get("wan_mac"));
	else
		strcpy(mac_addr, artblock_get("wan_mac"));
#else
	strcpy(mac_addr, nvram_safe_get("wan_mac"));
#endif	
	
	replace_get_special_char(wan_settings.name);
	replace_get_special_char(wan_settings.password);

	DEBUG_MSG("GetWanSettingsResult = %s\n", parse_result);
	DEBUG_MSG("Type = %s\n", wan_settings.protocol);
	DEBUG_MSG("Username = %s\n", wan_settings.name);
	DEBUG_MSG("Password = %s\n", wan_settings.password);
	DEBUG_MSG("MaxIdleTime = %s\n", wan_settings.idle_time);
	DEBUG_MSG("ServiceName = %s\n", wan_settings.service);
	DEBUG_MSG("AutoReconnect = %s\n", wan_settings.mode);
	DEBUG_MSG("IPAddress = %s\n", wan_settings.ip);
	DEBUG_MSG("SubnetMask = %s\n", wan_settings.netmask);
	DEBUG_MSG("Gateway = %s\n", wan_settings.gateway);
	DEBUG_MSG("Primary DNS = %s\n", wan_settings.primary_dns);
	DEBUG_MSG("Secondary DNS = %s\n", wan_settings.secondary_dns);
#ifdef OPENDNS
	DEBUG_MSG("OPENDNS enable = %s\n", wan_settings.opendns);
#endif
	DEBUG_MSG("MacAddress = %s\n", mac_addr);
	DEBUG_MSG("MTU = %s\n", pure_wan_mtu);

	/* set value to xml */
	sprintf(dest_xml, gereral_xml, parse_result,
		wan_settings.protocol, wan_settings.name, wan_settings.password,
		wan_settings.idle_time, wan_settings.service, wan_settings.mode,
		wan_settings.ip, wan_settings.netmask, wan_settings.gateway,
		wan_settings.primary_dns, wan_settings.secondary_dns
#ifdef OPENDNS
		,wan_settings.opendns
#endif
		,mac_addr ,pure_wan_mtu);

	if ((IsDeviceReady_cunt != 0) && !nvram_match("traffic_shaping", "1")) {
		printf("Disable traffic shaping before HNAP test\n");
		nvram_set("asp_temp_22", "1");	
		nvram_set("traffic_shaping", "0");		
		nvram_commit();
	}

	return 1;
}


static int pure_GetWanStatus(char *dest_xml, char *gereral_xml)
{
    char connection_status_buf[64]={0};
    char ConnStatue[64]={0};
  	get_wan_connection_status(connection_status_buf);

    if (strcmp(connection_status_buf, "Connected") == 0)
    	sprintf(ConnStatue,"CONNECTED");
    else if (strcmp(connection_status_buf, "Connecting") == 0)
    	sprintf(ConnStatue,"CONNECTING");
    else
    	sprintf(ConnStatue,"DISCONNECTED");

	/* set value to xml */
	xmlWrite(dest_xml, gereral_xml, "OK", ConnStatue);
	return 1;
}

static int pure_GetEthernetPortSetting(char *dest_xml, char *gereral_xml)
{
	struct ethernetport wan_st,lan_st0,lan_st1,lan_st2,lan_st3;	    
       
	VCTGetPortConnectState( nvram_safe_get("wan_eth"), VCTWANPORT0, wan_st.connected);
	strncpy(wan_st.port,"WAN",3);
	if (!strncmp(wan_st.connected,"connect",10))
	{
		strcpy(wan_st.connected,"true");
		VCTGetPortSpeedState( nvram_safe_get("wan_eth"), VCTWANPORT0, wan_st.speed, wan_st.duplex);   
		if (!strcmp(wan_st.speed,"giga"))
	         strcpy(wan_st.speed, "1000");
	}
	else
	{	
		strcpy(wan_st.connected,"false");
		strcpy(wan_st.speed,"10");		
		strcpy(wan_st.duplex,"half");		
	}				

	VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT3, lan_st0.connected);
	strncpy(lan_st0.port,"LAN_0",5);   
	if (!strncmp(lan_st0.connected,"connect",10))
	{
		strcpy(lan_st0.connected,"true");		
		VCTGetPortSpeedState( nvram_safe_get("lan_eth"), VCTLANPORT3, lan_st0.speed, lan_st0.duplex);
		if (!strcmp(lan_st0.speed,"giga"))
			 strcpy(lan_st0.speed, "1000");
	}
	else
	{	
		strcpy(lan_st0.connected,"false");
		strcpy(lan_st0.speed,"10");		
		strcpy(lan_st0.duplex,"half");					
	}
					    
	VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT2, lan_st1.connected);
	strncpy(lan_st1.port,"LAN_1",5);   
	if (!strncmp(lan_st1.connected,"connect",10))
	{
		strcpy(lan_st1.connected,"true");		
		VCTGetPortSpeedState( nvram_safe_get("lan_eth"), VCTLANPORT2, lan_st1.speed, lan_st1.duplex);   
		if (!strcmp(lan_st1.speed,"giga"))
			strcpy(lan_st1.speed, "1000");
	}
	else
	{	
		strcpy(lan_st1.connected,"false");
		strcpy(lan_st1.speed,"10");		
		strcpy(lan_st1.duplex,"half");					
	}
	
	VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT1, lan_st2.connected);
	strncpy(lan_st2.port,"LAN_2",5); 
	if (!strncmp(lan_st2.connected,"connect",10))
	{
		strcpy(lan_st2.connected,"true");		
		VCTGetPortSpeedState( nvram_safe_get("lan_eth"), VCTLANPORT1, lan_st2.speed, lan_st2.duplex);   
		if (!strcmp(lan_st2.speed,"giga"))
			 strcpy(lan_st2.speed, "1000");
	}
	else
	{	
		strcpy(lan_st2.connected,"false");
		strcpy(lan_st2.speed,"10");		
		strcpy(lan_st2.duplex,"half");					
	}
     	
	VCTGetPortConnectState( nvram_safe_get("lan_eth"), VCTLANPORT0, lan_st3.connected);
	strncpy(lan_st3.port,"LAN_3",5);   
	if (!strncmp(lan_st3.connected,"connect",10))
	{
		strcpy(lan_st3.connected,"true");		
		VCTGetPortSpeedState( nvram_safe_get("lan_eth"), VCTLANPORT0, lan_st3.speed, lan_st3.duplex);   
		if (!strcmp(lan_st3.speed,"giga"))
			 strcpy(lan_st3.speed, "1000");
	}
	else
	{	
		strcpy(lan_st3.connected,"false");
		strcpy(lan_st3.speed,"10");		
		strcpy(lan_st3.duplex,"half");					
	}
	
	/* set value to xml */
	xmlWrite(dest_xml, gereral_xml, "OK", wan_st.port,wan_st.connected,wan_st.speed,wan_st.duplex
	                                     ,lan_st0.port,lan_st0.connected,lan_st0.speed,lan_st0.duplex
	                                     ,lan_st1.port,lan_st1.connected,lan_st1.speed,lan_st1.duplex
	                                     ,lan_st2.port,lan_st2.connected,lan_st2.speed,lan_st2.duplex
	                                     ,lan_st3.port,lan_st3.connected,lan_st3.speed,lan_st3.duplex);
	                                     
	return 1;
}

static int pure_GetCableModemCloneWanMac(char *dest_xml, char *general_xml)
{
	char mac_addr[32];
	char temp[256]={0};
	FILE *fp=NULL;
		
	fp = fopen("/tmp/macaddress","r");

	if(fp == NULL){
		printf("Error get macclone");
		xmlWrite(dest_xml, general_xml, "ERROR", "");                                     
		return 0;	
	}
	else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			printf("temp=%s\n",temp);
			strcpy(mac_addr, temp);	
			xmlWrite(dest_xml, general_xml, "OK", mac_addr);                                     
			GetWanMacClone = 1;
			return 1;
		}//while
		fclose(fp);				
	}
	return 0;	
}

static int pure_GetPortMappings(char *dest_xml, char *general_xml, char *vs_rule_xml)
{
	int rule_index = 0;
	char vs_rule_tmp[8192] = {};
	char vs_rule[300] = {};
	char vs_value[300] = {};
	char *enable, *name, *protocol, *exter_port;
	char *ip, *inter_port, *schedule, *inbound;

    memset(vs_rule_tmp, 0, 8192);

	/* transfer nvram value of vs_rule to pure, is a loop for PortMapping_xml in pure_xml.c */
	for (rule_index=0; rule_index<ARRAYSIZE(vs_rule_table); rule_index++) {
		sprintf(vs_rule, "vs_rule_%02d", rule_index);
		strcpy(vs_value, nvram_safe_get(vs_rule));
		//DEBUG_MSG("vs_rule_%02d = %s\n", rule_index, vs_value);

		/* if default value	, then ignore*/
		if (strcmp(vs_value, "0//6/0/0/0.0.0.0/Always/Allow_All") == 0 || strlen(vs_value) == 0)
			continue;

		getStrtok(vs_value, "/", "%s %s %s %s %s %s %s %s",
				&enable, &name, &protocol, &exter_port, &inter_port, &ip, &schedule, &inbound);

		if (strcmp(name, "allow_null_for_test") == 0)
			name = NULL;

		DEBUG_MSG("PortMappingDescription = %s\n", name);
		DEBUG_MSG("InternalClient = %s\n", ip);
		DEBUG_MSG("PortMappingProtocol = %s\n", protocol);
		DEBUG_MSG("ExternalPort = %s\n", exter_port);
		DEBUG_MSG("InternalPort = %s\n", inter_port);

		xmlWrite(vs_rule, vs_rule_xml, name, ip, protocol, exter_port, inter_port);
		strcat(vs_rule_tmp, vs_rule);
	}

	xmlWrite(dest_xml,general_xml, "OK", vs_rule_tmp);
	return 1;
}

static char *parse_hex_to_ascii(char *hex_string)
{
	char hex[400];
	char hex_tmp[200];
	int len;
	int c1 = 0;
	int c2 = 0;

	memset(hex, 0, sizeof(hex));
	memset(hex_tmp, 0, sizeof(hex_tmp));
	len = strlen(hex_string);
	strcpy(hex_tmp, hex_string);

	for (c1 = 0; c1 < len; c1++) {
		/* ` = 0x60, " = 0x22, \ = 0x5c, ' = 0x27 */
		if ((hex_tmp[c1] == 0x22) || (hex_tmp[c1] == 0x5c) || (hex_tmp[c1] == 0x60)) {
			hex[c2] = 0x5c;
			c2++;
			hex[c2] = hex_tmp[c1];
		}
		else
			hex[c2] = hex_tmp[c1];
		c2++;
	}

	strcpy(hex_string, hex);
	return hex_string;
}

static int pure_GetWLanSetting24(char *dest_xml, char *general_xml)
{
	char *p_ssid;
	char *p_wEnable = NULL;
	char *p_mac = NULL;
	char *p_broadcast = NULL;
	char *p_channel = NULL;
	char *p_result = NULL;
	strcpy(gbl_ssid,nvram_get("wlan0_ssid"));
	//p_ssid = nvram_get("wlan0_ssid");
	p_wEnable = nvram_get("wlan0_enable");
#ifdef SET_GET_FROM_ARTBLOCK
	if(!(p_mac = artblock_get("lan_mac")))
		p_mac = nvram_get("lan_mac");
#else
	p_mac = nvram_get("lan_mac");
#endif
	p_broadcast = nvram_get("wlan0_ssid_broadcast");
	p_channel = nvram_get("wlan0_channel");

	replace_get_special_char(gbl_ssid);
	p_ssid = gbl_ssid;
	DEBUG_MSG("wlan0_ssid = %s\n", p_ssid);
	DEBUG_MSG("wlan0_enable = %s\n", p_wEnable);
	DEBUG_MSG("wlan0_mac = %s\n", p_mac);
	DEBUG_MSG("wlan0_ssid_broadcast = %s\n", p_broadcast);
	DEBUG_MSG("wlan0_channel = %s\n", p_channel);

	/* parse hex ssid to ascii */
	parse_hex_to_ascii(p_ssid);

	if (strcmp(p_wEnable, "0") == 0)
		p_wEnable = "false";
	else if (strcmp(p_wEnable, "1") == 0)
		p_wEnable = "true";

	if (strcmp(p_broadcast, "0") == 0)
		p_broadcast = "false";
	else if (strcmp(p_broadcast, "1") == 0)
		p_broadcast = "true";

	if ((p_wEnable == NULL) || (p_mac == NULL)|| (p_broadcast == NULL) || (p_channel == NULL)) {
		p_result = "FAIL";
		return 0;
	}
	else
		p_result = "OK";

	xmlWrite(dest_xml, general_xml, p_result, p_wEnable, p_mac, p_ssid, p_broadcast, p_channel);

	if (nvram_match("asp_temp_22", "1") == 0) {
		printf("Enable traffic shaping after HNAP test\n");
		nvram_set("asp_temp_22", "0");
		nvram_set("traffic_shaping", "1");
		nvram_flag_set();
		nvram_commit();
	}

#ifdef IP8000
	system("reboot -d2 &");
#endif

	return 1;
}

static int pure_GetWLanRadios(char *dest_xml, char *general_xml)
{
	char RadioInfo24[5120] = {};
	char g_channels[300] = {};
	char g_widechannel[2048] = {};
	char *g_channels_t;
	char g_channel_list[128];
	int  i;
	int  channel_count;
#ifdef USER_WL_ATH_5GHZ
	char RadioInfo5[5120] = {};
	char a_channels[300] = {};
	char *a_channels_t;
	char a_channel_list[512];
#endif
	char *wlan0_domain = NULL;
	int domain;

#ifdef SET_GET_FROM_ARTBLOCK
	wlan0_domain = artblock_get("wlan0_domain");
	if (wlan0_domain == NULL)
		wlan0_domain = nvram_get("wlan0_domain");
#else
	wlan0_domain = nvram_get("wlan0_domain");
#endif

	if (strlen(wlan0_domain))
		sscanf(wlan0_domain, "%x", &domain);
	else
		domain = 0x10;

	GetDomainChannelList(domain, WIRELESS_BAND_24G, g_channel_list, 0);

	/* generate channel */
	g_channels_t = strtok(g_channel_list, ",");
	sprintf(g_channels, "<int>0</int><int>%s</int>", g_channels_t);

	while (g_channels_t=strtok(NULL, ",")) {
		char tmp_buf[32];
		sprintf(tmp_buf, "<int>%s</int>", g_channels_t);
		strcat(g_channels, tmp_buf);
	}

	/* generate wide channel */
    if (domain == 0x30)
    	channel_count = 13;
    else
    	channel_count = 11;

	strcpy(g_widechannel, "<WideChannel><Channel>0</Channel><SecondaryChannels><int>0</int></SecondaryChannels></WideChannel>");
	for (i=3; i<=channel_count-2; i++) {
		char tmp_buf[160];
		sprintf(tmp_buf, "<WideChannel><Channel>%d</Channel><SecondaryChannels><int>%d</int><int>%d</int></SecondaryChannels></WideChannel>",
			i, i-2, i+2);
		strcat(g_widechannel, tmp_buf);
	}

	xmlWrite(RadioInfo24, RadioInfo_24GHZ, g_channels, g_widechannel, SecurityInfo);
#ifndef USER_WL_ATH_5GHZ
	xmlWrite(dest_xml, general_xml, "OK", RadioInfo24);
#endif

#ifdef USER_WL_ATH_5GHZ
	GetDomainChannelList(domain, WIRELESS_BAND_50G, a_channel_list, 0);
	
	/* generate channel */
	a_channels_t = strtok(a_channel_list, ",");
	sprintf(a_channels, "<int>0</int><int>%s</int>", a_channels_t);

	while(a_channels_t=strtok(NULL, ","))
	{
		char tmp_buf[32];
		sprintf(tmp_buf, "<int>%s</int>", a_channels_t);
		strcat(a_channels, tmp_buf);
	}
	xmlWrite(RadioInfo5, RadioInfo_5GHZ, a_channels, WideChannels_5GHZ, SecurityInfo);
	xmlWrite(dest_xml, general_xml, "OK", RadioInfo24, RadioInfo5);
#endif

	return 1;
}

static int pure_GetWLanRadioSettings(char *dest_xml, char *general_xml, char *source_xml)
{
	char *wlan_enable = "false";
	char *wlan_dot11_mode = NULL;
	char *mode = "11n";
	char *mac = "";
	char *ssid_broadcast = "true";
	char ssid[WLA_ESSID_LEN];
	char *wlan_11n_protection = NULL;
	char *protection = "20";
	char *channel = "0";
	char *qos = "true";
	char *p_wan_mac = NULL;
	int  wan_mac[6];
	char wlan1_mac[20];
	char *result = "OK";

	if (GetWLanRadioSettings_24GHZ == 1)
	{
		if (nvram_match("wlan0_enable", "1") == 0)
			wlan_enable = "true";

		wlan_dot11_mode=nvram_safe_get("wlan0_dot11_mode");
		if (strcmp(wlan_dot11_mode, "11bgn") == 0)
			mode = "802.11bgn";
		else if (strcmp(wlan_dot11_mode, "11g") == 0)
			mode = "802.11g";
		else if (strcmp(wlan_dot11_mode, "11bg") == 0)
			mode = "802.11bg";
		else if (strcmp(wlan_dot11_mode, "11gn") == 0)
			mode = "802.11gn";
		else if (strcmp(wlan_dot11_mode, "11b") == 0)
			mode = "802.11b";
		else
			mode = "802.11n";

#ifdef SET_GET_FROM_ARTBLOCK
		if (!(mac = artblock_get("lan_mac")))
			mac = nvram_safe_get("lan_mac");
#else
		mac = nvram_safe_get("lan_mac");
#endif
		bzero(ssid, sizeof(ssid));
		strcpy(ssid, nvram_get("wlan0_ssid"));
		/* parse hex ssid to ascii */
		//parse_hex_to_ascii(ssid);

		if (nvram_match("wlan0_ssid_broadcast", "0") == 0)
			ssid_broadcast = "false";

		wlan_11n_protection = nvram_safe_get("wlan0_11n_protection");
		if (strcmp(wlan_11n_protection, "40") == 0)
			protection = "40";
		else if (strcmp(wlan_11n_protection, "auto") == 0)
			protection = "0";

		if (nvram_match("wlan0_auto_channel_enable", "0") == 0)
        	channel = nvram_safe_get("wlan0_channel");

		if (nvram_match("wlan0_wmm_enable", "0") == 0)
			qos = "false";
	}
#ifdef USER_WL_ATH_5GHZ
	else if (GetWLanRadioSettings_24GHZ == 0)
	{
		if (nvram_match("wlan1_enable", "1") == 0)
			wlan_enable = "true";
		else
			wlan_enable = "false";

		wlan_dot11_mode=nvram_safe_get("wlan1_dot11_mode");
		if(strcmp(wlan_dot11_mode, "11a")==0)
			mode="802.11a";
		else if(strcmp(wlan_dot11_mode, "11n")==0)
			mode="802.11n";
		else if(strcmp(wlan_dot11_mode, "11na")==0)
			mode="802.11an";
		else
			mode="802.11an";

#ifdef SET_GET_FROM_ARTBLOCK
		if(artblock_get("wan_mac"))
			p_wan_mac = artblock_safe_get("wan_mac");
		else
			p_wan_mac = nvram_safe_get("wan_mac");
#else
		p_wan_mac = nvram_safe_get("wan_mac");
#endif

		sscanf(p_wan_mac,"%02x:%02x:%02x:%02x:%02x:%02x",&wan_mac[0],&wan_mac[1],&wan_mac[2],&wan_mac[3],&wan_mac[4],&wan_mac[5]);

		if (wan_mac[5] != 0xff)
			sprintf(wlan1_mac, "%02x:%02x:%02x:%02x:%02x:%02x", wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5]+1);
		else if (wan_mac[4] != 0xff)
			sprintf(wlan1_mac, "%02x:%02x:%02x:%02x:%02x:%02x", wan_mac[0], wan_mac[1], wan_mac[2], wan_mac[3], wan_mac[4]+1, 0);
		else if (wan_mac[3] != 0xff)
			sprintf(wlan1_mac, "%02x:%02x:%02x:%02x:%02x:%02x", wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3]+1, 0, 0);
		else if (wan_mac[2] != 0xff)
			sprintf(wlan1_mac, "%02x:%02x:%02x:%02x:%02x:%02x", wan_mac[0],wan_mac[1],wan_mac[2]+1, 0, 0, 0);
		else if (wan_mac[1] != 0xff)
			sprintf(wlan1_mac, "%02x:%02x:%02x:%02x:%02x:%02x", wan_mac[0],wan_mac[1]+1, 0, 0, 0, 0);
		else
			sprintf(wlan1_mac, "%02x:%02x:%02x:%02x:%02x:%02x", wan_mac[0]+1, 0, 0, 0, 0, 0);

		mac = wlan1_mac;

		strcpy(ssid, nvram_get("wlan1_ssid"));
		/* parse hex ssid to ascii */
		parse_hex_to_ascii(ssid);

		if (nvram_match("wlan1_ssid_broadcast", "1") == 0)
			ssid_broadcast = "true";
		else
			ssid_broadcast = "false";

		wlan_11n_protection = nvram_safe_get("wlan1_11n_protection");
		if (strcmp(wlan_11n_protection, "20") == 0)
			protection = "20";
		else if (strcmp(wlan_11n_protection, "40") == 0)
			protection = "40";
		else
			protection = "0";

		if (nvram_match("wlan1_auto_channel_enable", "1") == 0)
			channel = "0";
		else
			channel = nvram_safe_get("wlan1_channel");

		if (nvram_match("wlan1_wmm_enable", "1") == 0)
			qos = "true";
		else
			qos = "false";
	}
#endif
	else
	{
		printf("GetWLanRadioSettings: Bad RADIOID\n");
		result = "ERROR_BAD_RADIOID";
		return 0;
	}

	DEBUG_MSG("Enabled = %s\n", wlan_enable);
	DEBUG_MSG("Mode = %s\n", mode);
	DEBUG_MSG("MacAddress = %s\n", mac);
	replace_get_special_char(ssid);
	DEBUG_MSG("SSID = %s\n", ssid);
	DEBUG_MSG("SSIDBroadcast = %s\n", ssid_broadcast);
	DEBUG_MSG("ChannelWidth = %s\n", protection);
	DEBUG_MSG("Channel = %s\n", channel);
	DEBUG_MSG("SecondaryChannel = %s\n", pure_secondary_channel);
	DEBUG_MSG("QoS = %s\n", qos);

	xmlWrite(dest_xml, general_xml, result, wlan_enable, mode, mac, ssid, ssid_broadcast, protection, channel, pure_secondary_channel, qos);
	return 1;
}

static int pure_GetWLanRadioSecurity(char *dest_xml, char *general_xml, char *source_xml)
{
	char *enable = "true";
	char *wlan_security = NULL;
	char *type = "";
	char *encryption = "";
	char *key = "";
	char *wep64_key=NULL, *wep128_key=NULL, *wep_key_index=NULL;
	char *cipher_type = "";
	char *psk_pass_phrase="";
	char *keyrenewal="0";
	char *wlan_wpa_eap_radius_server_0=NULL;
	char *wlan_wpa_eap_radius_server_1=NULL;
	char *radiusip1="", *radiusport1="0", *radiussecret1="";
	char *radiusip2="", *radiusport2="0", *radiussecret2="";
	char *result;

	if (GetWLanRadioSecurity_24GHZ == 1)
	{
		wlan_security = nvram_safe_get("wlan0_security");
		if (strstr(wlan_security, "wep")) {
			char key64_name[32];
			char key128_name[32];

    		wep_key_index = nvram_safe_get("wlan0_wep_default_key");
    		sprintf(key64_name, "wlan0_wep64_key_%s", wep_key_index);
    		sprintf(key128_name, "wlan0_wep128_key_%s", wep_key_index);

    		wep64_key = nvram_safe_get(key64_name);
    		wep128_key = nvram_safe_get(key128_name);
	    }
	    else {
	    	cipher_type = nvram_safe_get("wlan0_psk_cipher_type");
	    	psk_pass_phrase = nvram_safe_get("wlan0_psk_pass_phrase");
	    	keyrenewal = nvram_safe_get("wlan0_gkey_rekey_time");
	    }

	    wlan_wpa_eap_radius_server_0 = nvram_safe_get("wlan0_eap_radius_server_0");
	    wlan_wpa_eap_radius_server_1 = nvram_safe_get("wlan0_eap_radius_server_1");

		if (strlen(wlan_wpa_eap_radius_server_0)) {
			radiusip1 = strtok(wlan_wpa_eap_radius_server_0, "/");
			radiusport1 = strtok(NULL, "/");
			radiussecret1 = strtok(NULL, "/");
			if (radiusip1 == NULL || radiusport1 == NULL || radiussecret1 == NULL) {
				radiusip1 = "";
			   	radiusport1 = "";
			  	radiussecret1 = "";
			}
		}

		if (strlen(wlan_wpa_eap_radius_server_1)) {
			radiusip2 = strtok(wlan_wpa_eap_radius_server_1, "/");
			radiusport2 = strtok(NULL, "/");
			radiussecret2 = strtok(NULL, "/");
			if (radiusip2 == NULL || radiusport2 == NULL  || radiussecret2 == NULL) {
				radiusip2 = "";
		      	radiusport2 = "";
		       	radiussecret2 = "";
		    }
		}

		result = "OK";
	}
#ifdef USER_WL_ATH_5GHZ
	else if (GetWLanRadioSecurity_24GHZ == 0)
	{
		wlan_security = nvram_safe_get("wlan1_security");
		if (strstr(wlan_security, "wep")) {
			char key64_name[32];
			char key128_name[32];

    		wep_key_index = nvram_safe_get("wlan1_wep_default_key");
    		sprintf(key64_name, "wlan1_wep64_key_%s", wep_key_index);
    		sprintf(key128_name, "wlan1_wep128_key_%s", wep_key_index);

    		wep64_key = nvram_safe_get(key64_name);
    		wep128_key = nvram_safe_get(key128_name);
	    }
	    else {
	    	cipher_type = nvram_safe_get("wlan1_psk_cipher_type");
	    	psk_pass_phrase = nvram_safe_get("wlan1_psk_pass_phrase");
	    	keyrenewal = nvram_safe_get("wlan1_gkey_rekey_time");
	    }

	    wlan_wpa_eap_radius_server_0 = nvram_safe_get("wlan1_eap_radius_server_0");
	    wlan_wpa_eap_radius_server_1 = nvram_safe_get("wlan1_eap_radius_server_1");

		if (strlen(wlan_wpa_eap_radius_server_0)) {
			radiusip1 = strtok(wlan_wpa_eap_radius_server_0, "/");
			radiusport1 = strtok(NULL, "/");
			radiussecret1 = strtok(NULL, "/");
			if (radiusip1 == NULL || radiusport1 == NULL || radiussecret1 == NULL) 
			{
				radiusip1 = "";
			   	radiusport1 = "";
			  	radiussecret1 = "";
			}
		}

		if (strlen(wlan_wpa_eap_radius_server_1)) {
			radiusip2 = strtok(wlan_wpa_eap_radius_server_1, "/");
			radiusport2 = strtok(NULL, "/");
			radiussecret2 = strtok(NULL, "/");
			if (radiusip2 == NULL || radiusport2 == NULL  || radiussecret2 == NULL) {
				radiusip2 = "";
		      	radiusport2 = "";
		       	radiussecret2 = "";
		    }
		}

		result = "OK";
	}
#endif
	else {
		printf("GetWLanRadioSecurity: Bad RADIOID\n");
		result = "ERROR_BAD_RADIOID";
		return 0;
	}

	if ((strcmp(wlan_security, "disable") == 0)) {
		enable = "false";
	}
	else if ((strcmp(wlan_security, "wep_open_64") == 0)) {
		type = "WEP-OPEN";
		encryption = "WEP-64";
		key = wep64_key;
	}
	else if ((strcmp(wlan_security, "wep_share_64") == 0)) {
		type = "WEP-SHARED";
		encryption = "WEP-64";
		key = wep64_key;
	}
	else if ((strcmp(wlan_security, "wep_open_128") == 0)) {
		type = "WEP-OPEN";
		encryption = "WEP-128";
		key = wep128_key;
    }
    else if ((strcmp(wlan_security, "wep_share_128") == 0)) {
    	type = "WEP-SHARED";
    	encryption = "WEP-128";
    	key = wep128_key;
    }
    else if (strcmp(wlan_security, "wpa_psk") == 0) {
        type = "WPA-PSK";
        if (strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if (strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";

		key = psk_pass_phrase;
    }
    else if (strcmp(wlan_security, "wpa2auto_psk") == 0) {
        type = "WPAORWPA2-PSK";
        if (strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if (strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";

		key = psk_pass_phrase;
    }
    else if ((strcmp(wlan_security, "wpa2_psk") == 0)) {
    	type = "WPA2-PSK";
    	if (strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if (strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";
    }
    else if ((strcmp(wlan_security, "wpa_eap") == 0) ||
    		 (strcmp(wlan_security, "wpa2auto_eap") == 0)) {
    	type = "WPA-RADIUS";
        if (strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if (strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";
    }
    else if (strcmp(wlan_security, "wpa2_eap") == 0) {
    	type = "WPA2-RADIUS";
    	if (strcmp(cipher_type, "tkip") == 0)
			encryption = "TKIP";
		else if (strcmp(cipher_type, "aes") == 0)
			encryption = "AES";
		else
			encryption = "TKIPORAES";
    }

	DEBUG_MSG("Enabled = %s\n", enable);
	DEBUG_MSG("Type = %s\n", type);
	DEBUG_MSG("Encryption = %s\n", encryption);
	replace_get_special_char(key);
	DEBUG_MSG("Key = %s\n", key);
	DEBUG_MSG("KeyRenewal = %s\n", keyrenewal);
	DEBUG_MSG("RadiusIP1 = %s\n", radiusip1);
	DEBUG_MSG("RadiusPort1 = %s\n", radiusport1);
	DEBUG_MSG("RadiusSecret1 = %s\n", radiussecret1);
	DEBUG_MSG("RadiusIP2 = %s\n", radiusip2);
	DEBUG_MSG("RadiusPort2 = %s\n", radiusport2);
	DEBUG_MSG("RadiusSecret2 = %s\n", radiussecret2);

	xmlWrite(dest_xml, general_xml, result,
		enable, type, encryption, key, keyrenewal, radiusip1, radiusport1, radiussecret1, radiusip2, radiusport2, radiussecret2);

	return 1;
}

static int pure_GetWLanSecurity(char *dest_xml, char *general_xml)
{
	char *wlan0_security_tmp;
	char *enable = "disable";
	char key_type[16] = {"WEP"};
	char sub_type[8] = {"open"};
	char wep_key_len[8] = {"64"};
	char key[256] = "0000000000000000000000000000000000000000000000000000000000000000";
	char *wlan0_wpa_eap_radius_server_0;
	char *wlan0_wpa_eap_radius_server_1;
	char *radiusip1 = "0.0.0.0";
	char *radiusip2 = "0.0.0.0";
	char *radiusport1 = "0";
	char *radiusport2 = "0";
	char key_xml[512] = {};
	char wlan0_security[100]= {};
	int i = 0;

	wlan0_security_tmp = nvram_safe_get("wlan0_security");
	strcpy(wlan0_security, wlan0_security_tmp);

	if (strcmp(wlan0_security, "disable") == 0) {
		enable = "false";
	}
	else {
		enable = "true";

		/* substitute "_" to " " , for sscanf function */
		for (i=0; i<strlen(wlan0_security); i++)
			if (*(wlan0_security+i) == '_')
				*(wlan0_security+i) = ' ';

		/* format scan in & write to xml */
		sscanf(wlan0_security, "%s %s %s", key_type, sub_type, wep_key_len);

		/* to Upper case */
		for (i=0; i<sizeof(key_type); i++)
			key_type[i] = toupper(key_type[i]);

		/* get key value */
		if ((strcasecmp(key_type, "WPA2AUTO") == 0) ||
			(strcasecmp(key_type, "WPA") == 0) ||
			(strcasecmp(key_type, "WPA2") == 0)) {
			strcpy(key, nvram_get("wlan0_psk_pass_phrase"));
			if (strcasecmp(key_type, "WPA2AUTO") == 0)
				strcpy(key_type, "WPA");
		}
		else if (strcasecmp(wep_key_len, "64") == 0)
			strcpy(key, nvram_safe_get("wlan0_wep64_key_1"));
		else if (strcasecmp(wep_key_len, "128") == 0)
			strcpy(key, nvram_safe_get("wlan0_wep128_key_1"));
		else if (strcasecmp(wep_key_len, "152") == 0)
			strcpy(key, nvram_safe_get("wlan0_wep152_key_1"));

		/* parse hex key to ascii */
		strcpy(key_xml, parse_hex_to_ascii(key));

		/* get eap config */
		wlan0_wpa_eap_radius_server_0 = nvram_safe_get("wlan0_eap_radius_server_0");
		wlan0_wpa_eap_radius_server_1 = nvram_safe_get("wlan0_eap_radius_server_1");
		if (strlen(wlan0_wpa_eap_radius_server_0) > 0) {
			radiusip1 = strtok(wlan0_wpa_eap_radius_server_0, "/");
			radiusport1 = strtok(NULL, "/");
			if (radiusip1 == NULL || radiusport1 == NULL) {
				radiusip1 = "0.0.0.0";
		    	radiusport1 = "0";
			}
		}
		else {
			radiusip1 = "0.0.0.0";
			radiusport1 = "0";
		}

		if (strlen(wlan0_wpa_eap_radius_server_1) > 0) {
			radiusip2 = strtok(wlan0_wpa_eap_radius_server_1, "/");
			radiusport2 = strtok(NULL, "/");
			if (radiusip2 == NULL || radiusport2 == NULL  || !strcmp(radiusip2, "not_set") || !strcmp(radiusport2, "not_set")) {
				radiusip2 = "0.0.0.0";
		    	radiusport2 = "0";
			}
		}
		else {
			radiusip2 = "0.0.0.0";
			radiusport2 = "0";
		}
	}

	DEBUG_MSG("enable = %s\n", enable);
	DEBUG_MSG("type = %s\n", key_type);
	DEBUG_MSG("key_len = %s\n", wep_key_len);
	replace_get_special_char(key_xml);
	DEBUG_MSG("key = %s\n", key_xml);
	DEBUG_MSG("radiusIP1 = %s\n", radiusip1);
	DEBUG_MSG("radiusPort1 = %s\n", radiusport1);
	DEBUG_MSG("radiusIP2 = %s\n", radiusip2);
	DEBUG_MSG("radiusPort2 = %s\n", radiusport2);

	if (strcasecmp(enable, "true") == 0) {
		xmlWrite(dest_xml, general_xml, "OK",
			enable, key_type, wep_key_len, key_xml, radiusip1, radiusport1, radiusip2, radiusport2);
	}
	else {	// for false case , for test-tool , default type=wep
		xmlWrite(dest_xml, general_xml, "OK",
			enable, "WEP", "64", "", radiusip1, radiusport1, radiusip2, radiusport2);
	}
	return 1;
}

static int pure_GetConnectedDevices(char *dest_xml, char *general_xml, char *cnted_client_xml)
{
	T_DHCPD_LIST_INFO* dhcpd_status = NULL;
	T_WLAN_STAION_INFO* wireless_status = NULL;
	int	i = 0, j = 0;
	char	clients_xml[MAX_RES_LEN] = {};
	char	clients_xml_tmp[1000] = {};
	char res[1024] = {};
	int find = 0;
	int wireless_enable=1;

	dhcpd_status = get_dhcp_client_list();
	wireless_status = get_stalist();

	if (wireless_status == NULL || wireless_status->num_sta == 0)
		wireless_enable=0;

	if ((dhcpd_status == NULL) && (wireless_status == NULL))
		return 0;
	else if (dhcpd_status->num_list > 0)
	{

		DEBUG_MSG("dhcpd_status->num_list = %d\n", dhcpd_status->num_list);
		if (wireless_enable)
			DEBUG_MSG("wireless_status->num_sta = %d\n", wireless_status->num_sta);

		for (i = 0 ; i < dhcpd_status->num_list; i++) {
			cnted_client_table[i].cnted_time = pure_dhcp_time_format(dhcpd_status->p_st_dhcpd_list[i].expired_at,res);
			cnted_client_table[i].mac_add = dhcpd_status->p_st_dhcpd_list[i].mac_addr;
			cnted_client_table[i].dev_name = dhcpd_status->p_st_dhcpd_list[i].hostname;

			if (wireless_enable) {
				find = 0;
				for (j = 0 ; j < wireless_status->num_sta; j++) {
					if (strncmp(dhcpd_status->p_st_dhcpd_list[i].mac_addr ,wireless_status->p_st_wlan_sta[j].mac_addr, 18) == 0) {
						cnted_client_table[i].port_name = "WLAN 802.11g";
						cnted_client_table[i].wireless = "true";
						cnted_client_table[i].active = "false";
						find = 1;
						break;
					}
				}

				if (!find) {
					cnted_client_table[i].port_name = "LAN";
					cnted_client_table[i].wireless = "false";
					cnted_client_table[i].active = "true";
				}
			}
			else {
				cnted_client_table[i].port_name = "LAN";
				cnted_client_table[i].wireless = "false";
				cnted_client_table[i].active = "true";
			}
		}
	}

	for (i = 0; i < dhcpd_status->num_list; i++) {
		xmlWrite(clients_xml_tmp, cnted_client_xml,
		cnted_client_table[i].cnted_time,
		cnted_client_table[i].mac_add,
		cnted_client_table[i].dev_name,
		cnted_client_table[i].port_name,
		cnted_client_table[i].wireless,
		cnted_client_table[i].active);

		sprintf(clients_xml, "%s%s", clients_xml, clients_xml_tmp);
		memset(clients_xml_tmp, 0, 1000);
	}

	xmlWrite(dest_xml, general_xml, "OK", clients_xml);
	return 1;
}

static char *pure_dhcp_time_format(char *data,char *res)
{
	char *year,*month,*temp_date,*temp_time;
	char date[16] = "";
	char time_tmp[32] = "";

	year = data;
	month = NULL;

	if((temp_date = strstr(data,"Jan")))
		month = "01";
	else if((temp_date = strstr(data,"Feb")))
		month = "02";
	else if((temp_date = strstr(data,"Mar")))
		month = "03";
	else if((temp_date = strstr(data,"Apr")))
		month = "04";
	else if((temp_date = strstr(data,"May")))
		month = "05";
	else if((temp_date = strstr(data,"Jun")))
		month = "06";
	else if((temp_date = strstr(data,"Jul")))
		month = "07";
	else if((temp_date = strstr(data,"Aug")))
		month = "08";
	else if((temp_date = strstr(data,"Sep")))
		month = "09";
	else if((temp_date = strstr(data,"Oct")))
		month = "10";
	else if((temp_date = strstr(data,"Nov")))
		month = "11";
	else if((temp_date = strstr(data,"Dec")))
		month = "12";

	temp_date = temp_date + 4;
	strncpy(date,temp_date,2);
	if(strcmp(date," 1") == 0)
		strcpy(date,"01");
	if(strcmp(date," 2") == 0)
		strcpy(date,"02");
	if(strcmp(date," 3") == 0)
		strcpy(date,"03");
	if(strcmp(date," 4") == 0)
		strcpy(date,"04");
	if(strcmp(date," 5") == 0)
		strcpy(date,"05");
	if(strcmp(date," 6") == 0)
		strcpy(date,"06");
	if(strcmp(date," 7") == 0)
		strcpy(date,"07");
	if(strcmp(date," 8") == 0)
		strcpy(date,"08");
	if(strcmp(date," 9") == 0)
		strcpy(date,"09");
	temp_time = temp_date + 3;
	strncpy(time_tmp,temp_time,8);
	year = year + 20;
	sprintf(res,"%s-%s-%sT%s",year,month,date,time_tmp);
	return res;
}

static int check_lan_settings(char *ip_t, char *mask_t){
	if(	strcmp(mask_t,"") == 0 || \
			strcmp(mask_t,"0.0.0.0") == 0 || \
			address_check(mask_t,2) == 0)
		return 0;

	if(	strcmp(ip_t,"") == 0 || \
			strcmp(ip_t,"0.0.0.0") == 0 || \
			address_check(ip_t,1) == 0)
		return 0;

	return 1;
}

static int check_mac_filter(char *mac_t)
{
	int index_t = 0;
	char *mac_tmp = NULL;
	char *mac_tmp_index = NULL;
	char mac[18] = {};

	/* condition protection , may add more conditions */
	if (strcasecmp(mac_t,"") == 0 || mac_check(mac_t,0) == 0) {
		return 0;
	}

	/* check if same mac in table */
	for (index_t=0; index_t<ARRAYSIZE(mac_filter_table); index_t++) {
		mac_tmp = nvram_get(mac_filter_table[index_t].mac_address);
		if (strcmp(mac_tmp,"name/00:00:00:00:00:00/Always") == 0)		// ignore default value
			continue;

		/* get true mac of nvram and compare to xml value */
		mac_tmp_index = strstr(mac_tmp, "/");
		strncat(mac, mac_tmp_index+1, 17);
		if (strcasecmp(mac,mac_t) == 0)
			return 0;

		memset(mac, 0, 18);
	}
	return 1;
}

static int check_wan_settings(int type)
{
	int mtu_tmp = atoi(wan_settings.mtu);
	switch(type) {
	case 1:  // static
		if (strcmp(wan_settings.name,"") || strcmp(wan_settings.password,"") ||
			strcmp(wan_settings.idle_time, "0") || strcasecmp(wan_settings.mode,"false") ||
			mtu_tmp < 1300 || mtu_tmp > 1500 ||
	  		strcmp(wan_settings.ip,"") == 0 ||
	  		address_check(wan_settings.ip,1) == 0 ||
	  		address_check(wan_settings.netmask,2) == 0 ||
	  		address_check(wan_settings.gateway,1) == 0 ||
	  		mac_check(wan_settings.mac, 1) == 0 ) {
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_mtu", wan_settings.mtu);
	  	break;

	case 2: // static pppoe
	  	if (strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 ||
	  		mtu_tmp < 1300 || mtu_tmp > 1492 ||
			atoi(wan_settings.idle_time) < 0 ||
	  		strcmp(wan_settings.ip,"") == 0 || strcmp(wan_settings.netmask,"") != 0 || strcmp(wan_settings.gateway,"") != 0 ||
	  		(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") ) ||
	  		address_check(wan_settings.ip,1) == 0 ) {
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_pppoe_mtu", wan_settings.mtu);
	  	break;

	case 3: // dynamic pppoe
	  	if (strcmp(wan_settings.name,"") == 0 ||
	  		strcmp(wan_settings.password,"") == 0 ||
	  		mtu_tmp < 1300 || mtu_tmp > 1492 ||
			(strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) ||
			atoi(wan_settings.idle_time) < 0 ||
			strcmp(wan_settings.ip,"") || strcmp(wan_settings.netmask,"") ||strcmp(wan_settings.gateway,"") ||
			(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false")) ) {
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_pppoe_mtu", wan_settings.mtu);
	  	break;

	case 4:	// dhcp
	  	if (strcmp(wan_settings.name,"") || strcmp(wan_settings.password,"") ||
			strcmp(wan_settings.idle_time, "0") || strcasecmp(wan_settings.mode, "false") ||
			mtu_tmp < 1300 || mtu_tmp > 1500 ||
	  		(strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) ||
	  		strcmp(wan_settings.ip,"") || strcmp(wan_settings.netmask,"") || strcmp(wan_settings.gateway,"") ||
	  		mac_check(wan_settings.mac, 1) == 0 ) {
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_mtu", wan_settings.mtu);
	  	break;

	case 5: // static pptp
	 	if (strcmp(wan_settings.ip,"") == 0 ||
	 		mtu_tmp < 1300 || mtu_tmp > 1400 ||
	 		atoi(wan_settings.idle_time) < 0 ||
	 		(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") ) ||
	 		strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 ||
	 		address_check(wan_settings.ip,1) == 0 ||
	 		address_check(wan_settings.gateway,1) == 0 ||
	 		address_check(wan_settings.netmask,2) == 0 ||
	  		address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ){
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_pptp_mtu", wan_settings.mtu);
	 	break;

	case 6:	// dynamic pptp
	  	if ((strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) ||
	  		strcmp(wan_settings.ip,"") != 0 || strcmp(wan_settings.netmask,"") != 0 || strcmp(wan_settings.gateway,"") != 0 ||
	  		mtu_tmp < 1300 || mtu_tmp > 1400 ||
	  		atoi(wan_settings.idle_time) < 0 ||
	  		(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") != 0 ) ||
	  		strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 ||
	  		address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ){
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_pptp_mtu", wan_settings.mtu);
	  	break;

	case 7: // static l2tp
	  	if (strcmp(wan_settings.ip,"") == 0 ||
	 		mtu_tmp < 1300 || mtu_tmp > 1400 ||
	 		atoi(wan_settings.idle_time) < 0 ||
	 		(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") != 0 ) ||
	 		strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 ||
	 		address_check(wan_settings.ip,1) == 0 ||
	 		address_check(wan_settings.gateway,1) == 0 ||
	 		address_check(wan_settings.netmask,2) == 0 ||
	  		address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ){
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_l2tp_mtu", wan_settings.mtu);
	  	break;

	case 8:	// dynamic l2tp
	  	if ((strcmp(wan_settings.primary_dns,"") == 0 && strlen(wan_settings.secondary_dns) != 0 ) ||
	  		strcmp(wan_settings.ip,"") != 0 || strcmp(wan_settings.netmask,"") != 0 || strcmp(wan_settings.gateway,"") != 0 ||
	  		mtu_tmp < 1300 || mtu_tmp > 1400 ||
	  		atoi(wan_settings.idle_time) < 0 ||
	  		(atoi(wan_settings.idle_time) == 180 && strcasecmp(wan_settings.mode,"false") != 0 ) ||
	  		strcmp(wan_settings.name,"") == 0 || strcmp(wan_settings.password,"") == 0 ||
	  		address_check(wan_settings.service,1) == 0 || strcmp(wan_settings.service,"") == 0 ){
	  		if(mtu_tmp == 0)
	  			return 1;
	  		else	  			
	  			return 0;
	  	}
	  	nvram_set("wan_l2tp_mtu", wan_settings.mtu);
	  	break;
	}

	return 1;
}

static int set_wan_settings(int type)
{
	switch(type) {
		case 1:		// static
			//DEBUG_MSG("set_wan_settings: static\n");
			nvram_set("wan_static_ipaddr",wan_settings.ip);
			nvram_set("wan_static_netmask",wan_settings.netmask);
			nvram_set("wan_static_gateway",wan_settings.gateway);
			break;

		case 2:		// static pppoe
			//DEBUG_MSG("set_wan_settings: static pppoe\n");
			if (!set_pppoe_pptp_l2tp_settings(0))
				return 0;
			nvram_set("wan_pppoe_username_00",wan_settings.name);
			nvram_set("wan_pppoe_password_00",wan_settings.password);
			nvram_set("wan_pppoe_max_idle_time_00",wan_settings.idle_time);
			nvram_set("wan_pppoe_service_00",wan_settings.service);
			nvram_set("wan_pppoe_connect_mode_00",wan_settings.mode);
			nvram_set("wan_pppoe_dynamic_00","0");
			nvram_set("wan_pppoe_ipaddr_00",wan_settings.ip);
			nvram_set("wan_pppoe_netmask_00",wan_settings.netmask);
			nvram_set("wan_pppoe_gateway_00",wan_settings.gateway);
			break;

		case 3:		// dynamic pppoe
			//DEBUG_MSG("set_wan_settings: dynamic pppoe\n");
			if (!set_pppoe_pptp_l2tp_settings(0))
				return 0;
			break;

		case 4:		// dhcpc
			//DEBUG_MSG("set_wan_settings: dhcpc\n");
			break;

		case 5:		// static pptp
			//DEBUG_MSG("static pptp\n");
			if (!set_pppoe_pptp_l2tp_settings(1))
				return 0;
			break;

		case 6:		// dynamic pptp
			//DEBUG_MSG("dynamic pptp\n");
			if (!set_pppoe_pptp_l2tp_settings(1))
				return 0;
			break;

		case 7:		// static l2tp
			//DEBUG_MSG("static l2tp\n");
			if (!set_pppoe_pptp_l2tp_settings(2))
				return 0;
			break;

		case 8:		// dynamic l2tp
			//DEBUG_MSG("dynamic l2tp\n");
			if (!set_pppoe_pptp_l2tp_settings(2))
				return 0;
			break;

		default:
			return 0;
	}
	return 1;
}
char wan_current_state[150] = {};
static int get_wan_settings(int type)
{
	switch(type) {
		case 1:		// static
			wan_settings.name = "";
			wan_settings.password = "";
			wan_settings.idle_time = "0";
			wan_settings.service = "";
			wan_settings.mode = "";
			wan_settings.ip = nvram_get("wan_static_ipaddr");
			wan_settings.netmask = nvram_get("wan_static_netmask");
			wan_settings.gateway = nvram_get("wan_static_gateway");
			wan_settings.primary_dns = nvram_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_get("wan_secondary_dns");
			break;

		case 2:  	// static pppoe
			if (!get_pppoe_pptp_l2tp_settings(0))
				return 0;
			wan_settings.primary_dns = nvram_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_get("wan_secondary_dns");
			break;

		case 3:  	// dynamic pppoe
			if (!get_pppoe_pptp_l2tp_settings(0))
				return 0;
			wan_settings.primary_dns = nvram_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_get("wan_secondary_dns");
			break;

		case 4: 	// dhcp
			/* initial wan current status */
			memset(wan_current_state, 0, sizeof(wan_current_state));
			sprintf(wan_current_state, "%s\n", read_current_ipaddr(0));
			/* save value to struct */
			wan_settings.name = "";
			wan_settings.password = "";
			wan_settings.idle_time = "0";
			wan_settings.service = "";
			wan_settings.mode = "";
			getStrtok(wan_current_state, "/", "%s %s %s %s %s", &wan_settings.ip, &wan_settings.netmask, &wan_settings.gateway, &wan_settings.primary_dns, &wan_settings.secondary_dns);
			break;

		case 5:		// static pptp
			if(!get_pppoe_pptp_l2tp_settings(1))
				return 0;
			wan_settings.primary_dns = nvram_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_get("wan_secondary_dns");
			break;

		case 6:		// dynamic pptp
			if (!get_pppoe_pptp_l2tp_settings(1))
				return 0;
			wan_settings.primary_dns = nvram_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_get("wan_secondary_dns");
			break;

		case 7:		// static l2tp
			if (!get_pppoe_pptp_l2tp_settings(2))
				return 0;
			wan_settings.primary_dns = nvram_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_get("wan_secondary_dns");
			break;

		case 8:		// dynamic l2tp
			if (!get_pppoe_pptp_l2tp_settings(2))
				return 0;
			wan_settings.primary_dns = nvram_get("wan_primary_dns");
			wan_settings.secondary_dns = nvram_get("wan_secondary_dns");
			break;
	}
	return 1;
}

static int get_pppoe_pptp_l2tp_settings(int w_index)
{
	char *mode_tmp = NULL;

	mode_tmp = nvram_get(pppoe_pptp_l2tp_table[w_index].mode);
	if (strcmp(mode_tmp, "on_demand") == 0)
		wan_settings.mode = "true";
	else if (strcmp(mode_tmp,"manual") == 0)
		wan_settings.mode = "false";
	else
		return 0;

	wan_settings.name = nvram_get(pppoe_pptp_l2tp_table[w_index].name);
	wan_settings.password = nvram_get(pppoe_pptp_l2tp_table[w_index].password);
	wan_settings.idle_time = nvram_get(pppoe_pptp_l2tp_table[w_index].idle_time);
	wan_settings.service = nvram_get(pppoe_pptp_l2tp_table[w_index].service);
	wan_settings.ip = nvram_get(pppoe_pptp_l2tp_table[w_index].ip);
	wan_settings.netmask = nvram_get(pppoe_pptp_l2tp_table[w_index].netmask);
	wan_settings.gateway = nvram_get(pppoe_pptp_l2tp_table[w_index].gateway);
	return 1;
}

static int set_pppoe_pptp_l2tp_settings(int w_index)
{
	if (strcasecmp(wan_settings.mode, "true") == 0)
		wan_settings.mode = "on_demand";
	else if (strcasecmp(wan_settings.mode,"false") == 0)
		wan_settings.mode = "manual";
	else
		return 0;

	nvram_set(pppoe_pptp_l2tp_table[w_index].name,wan_settings.name);
	nvram_set(pppoe_pptp_l2tp_table[w_index].password,wan_settings.password);
	nvram_set(pppoe_pptp_l2tp_table[w_index].idle_time,wan_settings.idle_time);
	nvram_set(pppoe_pptp_l2tp_table[w_index].service,wan_settings.service);
	nvram_set(pppoe_pptp_l2tp_table[w_index].mode,wan_settings.mode);
	nvram_set(pppoe_pptp_l2tp_table[w_index].ip,wan_settings.ip);
	nvram_set(pppoe_pptp_l2tp_table[w_index].netmask,wan_settings.netmask);
	nvram_set(pppoe_pptp_l2tp_table[w_index].gateway,wan_settings.gateway);
	nvram_set(pppoe_pptp_l2tp_table[w_index].dynamic,wan_settings.dynamic);
	return 1;
}

static int mac_check(char *mac,int wan_flag)
{
	char *start = NULL;
	char tmp[6] = {};
	int i=0;
	char res[20] = {};

	if (strlen(mac) < MAC_LENGTH) {			// if < 17 is likely "1:2:3:4:5:6"
		start = mac_complement(mac,res);
		if (start == NULL || strlen(start) != MAC_LENGTH) {
			DEBUG_MSG("mac_check error\n");
			return 0;
		}
	}
	else if (strlen(mac) == MAC_LENGTH)		// check len ( 17 is correct )
		start = mac;
	else {
		DEBUG_MSG("mac_check error\n");
		return 0;
	}

	memcpy(tmp, start, 3);
	memset(tmp, 0, 6);

	for(i=0; i<MAC_LENGTH; i++) {			// if format correct
		memcpy(tmp, start, 1);
		if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
			if (strcmp(tmp,":") == 0)		// check ":" position
				goto move;
			else {
				DEBUG_MSG("mac_check error\n");
				return 0;
			}
		}

		if (strcasecmp(tmp,"a") == 0 || strcasecmp(tmp,"b") == 0 || strcasecmp(tmp,"c") == 0 || \
			strcasecmp(tmp,"d") == 0 || strcasecmp(tmp,"e") == 0 || strcasecmp(tmp,"f") == 0)
			goto move;						// check "a to f"

		if (strcmp(tmp,"0") == 0)			// check "0"
			goto move;

		if (1 <= atoi(tmp) && atoi(tmp)<= 9)	// check "1~9"
			goto move;
		else {
			DEBUG_MSG("mac_check error\n");
			return 0;
		}
move:
		start++;
	}

	return 1;
}

static char *mac_complement(char *mac,char *result){
	char tmp_1[3] = {};
	char tmp_2[3] = {};
	char start[20] = {};
	int i=0;

	memcpy(start, mac, strlen(mac));
	strcat(start, ":");
	for(i=0; i<strlen(mac);){
		tmp_1[0] = start[i];
		tmp_2[0] = start[i+1];

		if(strcmp(tmp_1,":") == 0 )
			return NULL;
		else if(strcmp(tmp_2,":") == 0) {
			sprintf(result,"%s%s%s%s",result,"0",tmp_1,":");
			i = i + 2 ;
		}else {
			sprintf(result,"%s%s%s%s",result,tmp_1,tmp_2,":");
			i = i+3;
		}
		memset(tmp_1,0,3);
		memset(tmp_2,0,3);
	}

	*(strrchr(result,':')) = '\0';
	return result;

}

static int illegal_mask(char *mask_token){
	if( 0 >= strlen(mask_token) || strlen(mask_token) > 3 )  //	check length
		return 0;

	/* illegal netmask value */
	if((strcmp(mask_token,"255") ==0) || (strcmp(mask_token,"254") ==0) ||
	   (strcmp(mask_token,"252") ==0) || (strcmp(mask_token,"248") ==0) ||
	   (strcmp(mask_token,"240") ==0) || (strcmp(mask_token,"224") ==0) ||
	   (strcmp(mask_token,"192") ==0) || (strcmp(mask_token,"128") ==0) ||
	   (strcmp(mask_token,"0") ==0) )
		return 1;
	else
		return 0;
}

static int address_check(char *address,int type){
	char *start_1,*end_1;
	char *start_2,*end_2;
	char *start_3,*end_3;
	char *start_4;
	char mask_token_1[32] = "";
	char mask_token_2[32] = "";
	char mask_token_3[32] = "";
	char mask_token_4[32] = "";
	int i = 0;

	for(i=0; i<strlen(address); i++){						// check if it is "0 ~ 9", ignore "."
		if(*(address+i) == '.')
			continue;
		else
			if(isdigit(*(address+i)) == 0)
				return 0;
	}

	if(strcmp(address,"0.0.0.0") == 0)					// is not allowed for nvram
		return 0;

	start_1 = address;
	if((end_1 = strstr(address,".")) == NULL)
		return 0;
	start_2 = end_1 + 1;
	if((end_2 = strstr(start_2,".")) == NULL)
		return 0;
	start_3 = end_2 + 1;
	if((end_3 = strstr(start_3,".")) == NULL)
		return 0;
	start_4 = end_3 + 1;
	if((end_3 = strstr(start_3,".")) == NULL)
		return 0;

	/* get value from xml value */
	strncpy(mask_token_1,start_1,end_1 - start_1);
	strncpy(mask_token_2,start_2,end_2 - start_2);
	strncpy(mask_token_3,start_3,end_3 - start_3);
	strcpy(mask_token_4,start_4);

	switch(type){
		case 1:   // ip , gateway , service
			if(!ip_check(mask_token_1,mask_token_2,mask_token_3,mask_token_4))
				return 0;
			break;
		case 2:		// netmask
			if(!netmask_check(address,mask_token_1,mask_token_2,mask_token_3,mask_token_4))
			{
				return 0;
			}
			break;
	}
	return 1;
}

static int netmask_check(char *address, char *token_1, char *token_2, char *token_3, char *token_4){
	/* check length & illegal value */
	if(	(illegal_mask(token_1) == 0) || (illegal_mask(token_2) == 0) || \
	   	(illegal_mask(token_3) == 0) || (illegal_mask(token_4) == 0) )
  	return 0;
   if(strcmp(token_1,"255") == 0)
  {
  	if(strcmp(token_2,"255") == 0)
  	{
  		if(strcmp(token_3,"255") == 0)
  		{
  			return 1;

  		}
  		else
  		{
  			strcat(token_1,".");
  			strcat(token_1,token_2);
  			strcat(token_1,".");
  			strcat(token_1,token_3);
  			strcat(token_1,".0");
  			if(strcmp(token_1,address) != 0)   // should be 255.255.255.0
  				return 0;
  		}
  	}
  	else{
  		strcat(token_1,".");
  		strcat(token_1,token_2);
  		strcat(token_1,".0.0");							// should be 255.255.0.0
  		if(strcmp(token_1,address) != 0)
  			return 0;
  	}
  }
  else{
  	strcat(token_1,".0.0.0");							// should be 255.0.0.0
  	if(strcmp(token_1,address) != 0)
  		return 0;
  }

  return 1;
}

static int ip_check(char *token_1, char *token_2, char *token_3, char *token_4){
	/* check length */
	if(	strlen(token_1) <= 0 || strlen(token_1) > 3 || \
			strlen(token_2) <= 0 || strlen(token_2) > 3 || \
			strlen(token_3) <= 0 || strlen(token_3) > 3 || \
			strlen(token_4) <= 0 || strlen(token_4) > 3)
		return 0;
	/* check range */
	if( atoi(token_1) < 0 || atoi(token_1) > 255 || \
			atoi(token_2) < 0 || atoi(token_2) > 255 || \
			atoi(token_3) < 0 || atoi(token_3) > 255 || \
			atoi(token_4) < 0 || atoi(token_4) > 254 )
		return 0;

	return 1;
}


void do_pure_check(char *url, FILE *stream){
	assert(url);
	assert(stream);
	/*
	  Modification: DeviceSetting testting for different Model
      Modified by: ken_chiang
      Date:2007/9/21
	*/
	websWrite(stream, GetDeviceSettingsResult_xml,
				nvram_get("pure_type"),
				nvram_get("pure_device_name"),
				nvram_get("pure_vendor_name"),
				nvram_get("pure_model_description"),
				nvram_get("model_number"),
				VER_FIRMWARE,
				nvram_get("pure_presentation_url")
#ifdef AUTHGRAPH
				, (atoi(nvram_safe_get("graph_auth_enable")) == 0) ? "false" : "true"
#endif
				);
}

void pure_authorization_error(void)
{
	add_headers(401, "Unauthorized", NULL, "text/xml", -1, NULL);
	send(pure_atrs.conn_fd, pure_atrs.response, strlen(pure_atrs.response), 0);
}

static char *parse_action(int i)
{
	switch(i) {
		case 1:
			return "GetDeviceSettings";
		case 2:
			return "GetWanSettings";
		case 3:
			return "GetWLanSettings24";
		case 4:
			return "GetWLanSecurity";
		case 5:
			return "GetConnectedDevices";
		case 6:
			return "GetNetworkStats";
		case 7:
			return "GetMACFilters2";
		case 8:
			return "GetPortMappings";
		case 9:
			return "GetRouterLanSettings";
		case 10:
			return "IsDeviceReady";
		case 11:
			return "Reboot";
		case 12:
			return "SetWLanSettings24";
		case 13:
			return "SetWLanSecurity";
		case 14:
			return "GetForwardedPorts";
		case 15:
			return "SetDeviceSettings";
		case 16:
			return "SetForwardedPorts";
		case 17:
			return "SetWanSettings";
		case 18:
			return "SetMACFilters2";
		case 19:
			return "SetRouterLanSettings";
		case 20:
			return "AddPortMapping";
		case 21:
			return "RenewWanConnection";
		case 22:
			return "DeletePortMapping";
		case 23:
			return "SetAccessPointMode";
		case 24:
			return "UnknownCodeTest";
		case 25:
			return "GetWLanRadios";
		case 26:
			return "GetWLanRadioSettings";
		case 27:
			return "GetWLanRadioSecurity";
		case 28:
			return "SetWLanRadioSettings";
		case 29:
			return "SetWLanRadioSecurity";
		case 30:
			return "GetWanStatus";
		case 31:
			return "GetEthernetPortSetting";			
		case 32:
			return "StartCableModemCloneWanMac";			
		case 33:
			return "GetCableModemCloneWanMac";			
		case 34:
			return "SetCableModemCloneWanMac";			

#ifdef OPENDNS
		case 39:
			return "GetOpenDNS";
		case 40:
			return "SetOpenDNS";
#endif
		case 66:
			return "SetMultipleActions";
		case 67:
			return "SetTriggerADIC";
	}

	DEBUG_MSG("ERROR PURE ACTION %d\n", i);
	return "ERROR PURE ACTION";
}
