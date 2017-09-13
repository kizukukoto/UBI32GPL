#ifndef _SUTIL_H
#define _SUTIL_H

//#include "project.h"

extern int _system(const char *fmt, ...);
extern int read_pid(char *file);
extern int init_file(char *file);
extern void save2file(const char *file, const char *fmt, ...);
extern void write2file(const char *file, const char *fmt, ...);
extern int wait4file(const char *file, int until_exist, int timeout);
extern char *replace(char *input, const char target, const char rechar );
extern int get_blk_region(char* region);
extern int get_blk_domain(char* domain);
extern int get_blk_hwver(char* hwver);
extern int get_blk_mac(char* mac);
extern int write_macblock(char* mac, char* hwver, char* domain, char* region);
extern int init_macblock(void);
extern int _GetMemInfo(char *strKey, char *strOut);

#ifdef CONFIG_USER_WL_ATH
extern int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space);
#endif
#ifdef CONFIG_USER_WL_RTL
int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[]);
#endif
#ifdef CONFIG_USER_WL_MVL
extern int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space);
#endif
#ifdef CONFIG_USER_WL_RALINK
extern int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space);
#endif

extern char *time_format_for_log(void);
extern int wan_connected_cofirm(char *if_name);
extern int check_ip_match(const char *target, const char *model);
extern char *parse_special_char(char *p_string);
extern int getStrtok(char *input, char *token, char *fmt, ...);
extern void flash_get_checksum(void);
#ifdef CONFIG_LP
extern void lp_get_checksum(void);
#endif
extern void string_insert(char *st, char insert[], int start);
extern char *ssid_transfer(char *wlan_ssid);
extern void generate_pin_by_mac(char *wps_pin);

#ifdef CONFIG_LP
#define FIRMWARE_UPGRADE_FAIL     0
#define FIRMWARE_UPGRADE_SUCCESS  1
#define FIRMWARE_SIZE_ERROR       2

/* html page */
#define HTML_RESPONSE_PAGE        1
#define HTML_RETURN_PAGE          2
#define NO_HTML_PAGE              3

/* upgrade fw/loader */
#define ERROR_UPGRADE             0
#define FW_UPGRADE                1
#define LD_UPGRADE                2
/* action type */
#define F_UPLOAD                  0
#define F_REBOOT                  1
#define F_UPGRADE                 2
#define F_RESTORE                 3
#define F_INITIAL                 4
#define F_RC_RESTART              5
#define F_TYPE_COUNT              6 // if you add F_XXX type, don't forget to fix it !!
#endif

/*  Date: 2010-04-23
*   Name: Cosmo Chang
*   Reason: get the usb type
*   Notice: 
*/
extern int get_usb_type(void);

/* IPv6 scope*/
#ifdef IPv6_SUPPORT
#define SCOPE_LOCAL     0
#define SCOPE_GLOBAL    1

#define MAX_IPV6_IP_LEN	45
#endif

void modify_ppp_options(void);
char* return_ap_rt_info(void);
struct mp_info check_mpppoe_info(int pppoe_unit);

struct mp_info {
	int pid;
	char pppoe_iface[5];
};

struct IpRange{
	char addr[38];
}; 

struct OtherMachineInfo{
	char filter[14];
	char log[4];
	char schedule[64];
	int index;
}; 

#define foreach(word, wordlist, next, match) \
	for (next = &wordlist[strspn(wordlist, match)], \
			strncpy(word, next, sizeof(word)), \
			word[strcspn(word, match)] = '\0', \
			word[sizeof(word) - 1] = '\0', \
			next = strstr(next, match); \
			strlen(word); \
			next = next ? &next[strspn(next, match)] : "", \
			strncpy(word, next, sizeof(word)), \
			word[strcspn(word, match)] = '\0', \
			word[sizeof(word) - 1] = '\0', \
			next = strstr(next, match))

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)
#define run_rc_action(action) write_rc_action(action);kill(read_pid(RC_PID), SIGHUP)

#define cprintf(fmt, args...) do { \
	FILE *cfp = fopen("/dev/console", "w"); \
	if (cfp) { \
		fprintf(cfp, fmt, ## args); \
		fclose(cfp); \
	} \
} while (0)

/* Mac Block Struct */
typedef struct block_data {
	int init_flag;
	char mac[20];
	char hwver[4];
	char domain[4];
	char region[4];
};

/************************** Channel Parameter move to project.h in WWW model**************************/
//struct channel_list_s {
//};

//struct freq_table_s {
//};

//static struct freq_table_s wireless2_table[] = {
//};

//struct domain_name_s {
//};

//static struct domain_name_s  domain_name[] = {
//};

//struct country_name_s {
//};

//static struct country_name_s  country_name[] = {
//};

//struct region_s{
//};

//static struct region_s region[] = {
//};

/************************** Channel Parameter **************************/

#endif //#ifndef _SUTIL_H
