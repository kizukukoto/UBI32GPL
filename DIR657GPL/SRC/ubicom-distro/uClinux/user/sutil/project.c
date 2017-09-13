#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
/*  Date: 2009-3-12
*   Name: Albert Chen
*   Reason: modify for different kernel version had diferent mtd include file. 
*   Notice : Ex:DIR-400 isn't same as DIR-615C1.
*/
#include <linux/version.h>
#include <mtd-user.h>
/*
#if (LINUX_VERSION_CODE) < KERNEL_VERSION(2,6,15)
#include <linux/mtd/mtd.h>
#else
#include <mtd/mtd-user.h>
#endif
*/
#include "shvar.h"
#include "sutil.h"
#include "scmd_nvram.h"
/*
 * Shared utility Debug Messgae Define
 */
#ifdef SUTIL_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

int _system(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[512];

	va_start(args, fmt);
	i = vsprintf(buf, fmt,args);
	va_end(args);

	DEBUG_MSG("%s\n",buf);
	system(buf);
	return i;

}

/* Init File and clear the content */
int init_file(char *file)
{
	FILE *fp;

	if ((fp = fopen(file ,"w")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	fclose(fp);
	return 0;
}


/* Save data into file	
 * Notice: You must call init_file() before save2file()
 */
void save2file(const char *file, const char *fmt, ...)
{
	char buf[10240];
	va_list args;
	FILE *fp;

	if ((fp = fopen(file ,"a")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	va_start(args, fmt);
	fprintf(fp, "%s", buf);
	va_end(args);

	fclose(fp);
}

/*
 * write2file()
 * Save data into file (non-append).
 * Typically to write a sysfs or procfs file.
 */
void write2file(const char *file, const char *fmt, ...)
{
	char buf[10240];
	va_list args;
	FILE *fp;

	if ((fp = fopen(file ,"w")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	fprintf(fp, "%s", buf);

	fclose(fp);
}

/*
 * wait4file()
 * Wait for a file to exist or become deleted
 */
int wait4file(const char *file, int until_exist, int timeout)
{
	FILE *fp;

	if (until_exist) {
		while (timeout && ((fp = fopen(file ,"r")) == NULL)) {
			sleep(1);
			timeout--;
		}
		if (fp) {
			fclose(fp);
		}
		return timeout;
	}

	while (timeout && ((fp = fopen(file ,"r")) != NULL)) {
		fclose(fp);
		sleep(1);
		timeout--;
	}
	return timeout;
}

char *add_token(char *mac_without_token)
{
	char temp[32] = {};
	char buf[4] = {};
	char *ptr;
	int i;
	ptr = mac_without_token;
	for(i=0;i<6;i++)
	{
		strncpy(buf,ptr,2);
		strcat(temp,buf);
		if(i == 5)
			break;
		strcat(temp,":");
		memset(buf,0,sizeof(buf));
		ptr = ptr + 2;
	}
	return temp;
}


void modify_ppp_options(void)
{
	int i = 0;
	char options_file[] = "/var/etc/options_XX";
	char username[] = "wan_pppoe_username_XX";
	char pppoe_type[] = "wan_pppoe_connect_mode_XX";
	char connect_type[] = "wan_pppoe_dynamic_XX";
	char ipaddr[] = "wan_pppoe_ipaddr_XX";
	int time;
	char default_gateway[16] = {};
	char use_peerdns[16] = {};
	char tmp[64] = {};

	memset(use_peerdns, '\0', sizeof(use_peerdns));
	memset(tmp, '\0', sizeof(tmp));

#ifdef RPPPOE
	if(	 !cmd_nvram_match("wan_specify_dns", "0") ||
			(!cmd_nvram_match("wan_pppoe_russia_enable", "1") && !cmd_nvram_match("wan_proto", "pppoe")) || 
			(!cmd_nvram_match("wan_pptp_russia_enable", "1") && !cmd_nvram_match("wan_proto", "pptp")) ||
			(!cmd_nvram_match("wan_l2tp_russia_enable", "1") && !cmd_nvram_match("wan_proto", "l2tp")) )
		sprintf(use_peerdns, "usepeerdns\n");
#else
	if(!cmd_nvram_match("wan_specify_dns", "0"))
		sprintf(use_peerdns, "usepeerdns\n");
#endif
	/* 2010/09/07 Robert
	 * Fix bug that use_peerdns could be null string, it will occur ppp-option error.
	 * So, I merge maxfail and usepeerdns columns to tmp variable.
         */
	sprintf(tmp, "%s%s\n", use_peerdns, "maxfail 3");

	char *wan_pppoe_main_session_value = NULL;
        char *username_value = NULL;
        char *wan_pppoe_mtu_value = NULL;
        char *ipaddr_value = NULL;
        char *wan_pptp_max_idle_time_value = NULL;
        char *wan_pptp_username_value = NULL;
        char *wan_pptp_mtu_value = NULL;
        char *wan_l2tp_max_idle_time_value = NULL;
        char *wan_l2tp_username_value = NULL;
        char *wan_l2tp_mtu_value = NULL;
#ifdef IPV6_PPPoE
	char *ipv6_pppoe_share = NULL;
#endif
	/* For PPPoE */
	if( cmd_nvram_match("wan_proto", "pppoe") ==0 ) 
	{
		INIT_NVRAM_VALUE(wan_pppoe_main_session_value);
		INIT_NVRAM_VALUE(username_value);
		INIT_NVRAM_VALUE(wan_pppoe_mtu_value);
		INIT_NVRAM_VALUE(ipaddr_value);
#ifdef IPV6_PPPoE
		INIT_NVRAM_VALUE(ipv6_pppoe_share);
#endif

		for(i=0; i<MAX_PPPOE_CONNECTION; i++)
		{
			memset(default_gateway,0,sizeof(default_gateway));
			if(atoi(cmd_nvram_safe_get("wan_pppoe_main_session",wan_pppoe_main_session_value)) == i)
				strcpy(default_gateway,"defaultroute");
			else
				strcpy(default_gateway,"nodefaultroute");

			sprintf(options_file, "/var/etc/options_%02d", i);
			sprintf(username, "wan_pppoe_username_%02d", i);
			sprintf(pppoe_type, "wan_pppoe_connect_mode_%02d", i);
			sprintf(connect_type, "wan_pppoe_dynamic_%02d", i);
			sprintf(ipaddr, "wan_pppoe_ipaddr_%02d", i);
			// debug option is need, i don't know why the pppd write to file need this option
			init_file(options_file);
			save2file(options_file,
					"plugin /lib/pppd/2.4.3/rp-pppoe.so\n"
					"login\n"
					"noauth\n"
					"debug\n"
					"refuse-eap\n"
					"noipdefault\n"
					"defaultroute\n"
					"passive\n"
					"lcp-echo-interval 30\n"
					"lcp-echo-failure 10\n"
					"%s"
					"user \"%s\"\n"
					"mtu  %s\n"
					"mru  %s\n"
					"%s:\n"
					"remotename %s\n"
					"novj \n"
					"nopcomp \n"
					"noaccomp \n"
					"-am\n"
					"noccp \n",
					tmp, \
					cmd_nvram_safe_get(username,username_value),\
					cmd_nvram_safe_get("wan_pppoe_mtu",wan_pppoe_mtu_value),\
					wan_pppoe_mtu_value,\
					cmd_nvram_match(connect_type, "1") ? cmd_nvram_get(ipaddr,ipaddr_value) : "",\
					CHAP_REMOTE_NAME);
#ifdef IPV6_PPPoE
			cmd_nvram_get("ipv6_pppoe_share", ipv6_pppoe_share);
			if((cmd_nvram_match("ipv6_wan_proto", "ipv6_pppoe") == 0 
				|| cmd_nvram_match("ipv6_wan_proto", "ipv6_autodetect") == 0) 
				&& strcmp(ipv6_pppoe_share,"1") == 0)
				save2file(options_file,"ipv6 , \n");
#endif
		}
	} 
	else if ( cmd_nvram_match("wan_proto", "pptp") ==0) 
	{
		INIT_NVRAM_VALUE(wan_pptp_username_value);
		INIT_NVRAM_VALUE(wan_pptp_mtu_value);
		INIT_NVRAM_VALUE(wan_pptp_max_idle_time_value);
		/* For PPTP */
		time = 60 * atoi( cmd_nvram_safe_get("wan_pptp_max_idle_time",wan_pptp_max_idle_time_value) );
		init_file(PPP_OPTIONS);
		/*  Date: 2009-6-26
		*   Name: Tina Tsao
		*   Reason: modify for fixed DNS can't be replace by DHCP DNS at RS-PPTP static mode. 
		*   Notice : Fixed as below
		*/
		save2file(PPP_OPTIONS,
				"noipdefault\n"
				"defaultroute\n"
				"passive\n"
				"debug\n"
				"lcp-echo-interval 30\n"
				"lcp-echo-failure 4\n"/*Set number of consecutive echo failures to indicate link failure*/
				"%s"
				"user \"%s\"\n"
				"mtu %s\n"
				"mru %s\n"
				"noauth\n"
				"refuse-eap\n"
				"nobsdcomp\n"/*BSD-Compress*/
				"nodeflate\n"/*Deflate compression*/
				"noaccomp\n"/*Address Control filed compression*/
				"nopcomp\n"	/*Protocol filed compression*/
				"novj\n"	/*VJ compression*/
				"novjccomp\n"/*Disable VJ connection-ID compression*/
				"ipcp-accept-remote\n"
				"ipcp-accept-local\n"
				"persist\n"/*Keep on reopening connection after close*/
				"remotename %s\n"
				"%s\n"		/*require MPPE 40-bit encryption*/
				"%s\n"		
				"%s\n",		/*Compression Control Protocol*/		
				tmp,\
				cmd_nvram_safe_get("wan_pptp_username",wan_pptp_username_value) ,\
				cmd_nvram_safe_get("wan_pptp_mtu",wan_pptp_mtu_value),\
				cmd_nvram_safe_get("wan_pptp_mtu",wan_pptp_mtu_value),\
				CHAP_REMOTE_NAME,\
				cmd_nvram_match("support_pptp_mppe", "1")==0 ? "require-mppe-40" : "",\
				cmd_nvram_match("support_pptp_mppe", "1")==0 ? "require-mppe-128" : "",\
				(cmd_nvram_match("support_pptp_mppe", "0")==0 || cmd_nvram_match("support_pptp_mppe", "")==0) ? "noccp" : ""
				);
	} 
	else if ( cmd_nvram_match("wan_proto", "l2tp") ==0) 
	{
		INIT_NVRAM_VALUE(wan_l2tp_max_idle_time_value);
		INIT_NVRAM_VALUE(wan_l2tp_username_value);
		INIT_NVRAM_VALUE(wan_l2tp_mtu_value);
		/* For L2TP */
		time = 60 * atoi( cmd_nvram_safe_get("wan_l2tp_max_idle_time",wan_l2tp_max_idle_time_value) );
		init_file(PPP_OPTIONS);
		/*  Date: 2009-6-26
		*   Name: Tina Tsao
		*   Reason: modify for fixed DNS can't be replace by DHCP DNS at RS-L2TP static mode. 
		*   Notice : Fixed as below
		*/
		save2file(PPP_OPTIONS,
				"noipdefault\n"
				"defaultroute\n"
				"passive\n"
				"debug\n"
				"refuse-eap\n"  /*Don't agree to authenticate to peer with EAP*/
				"lcp-echo-interval 30\n"
				"lcp-echo-failure 4\n"                               
				"%s"
				"user \"%s\"\n"
				"mtu  %s\n"
				"mru  %s\n"
				"noauth\n" /*"Don't require peer to authenticate"*/
				"noaccomp\n"/*Disable address/control compression*/
				"nopcomp\n"     /*Disable protocol field compression*/
				"noccp\n"       /*Disable CCP negotiation : ccp.c*/
				"-am\n"         /*Disable asyncmap negotiation*/
				"remotename %s\n",
				tmp,\
				cmd_nvram_safe_get("wan_l2tp_username",wan_l2tp_username_value) ,\
				cmd_nvram_safe_get("wan_l2tp_mtu",wan_l2tp_mtu_value),\
				cmd_nvram_safe_get("wan_l2tp_mtu",wan_l2tp_mtu_value),\
				CHAP_REMOTE_NAME);
	}
	FREE_NVRAM_VALUE(wan_pppoe_main_session_value);
	FREE_NVRAM_VALUE(username_value);
	FREE_NVRAM_VALUE(wan_pppoe_mtu_value);
	FREE_NVRAM_VALUE(ipaddr_value);
	FREE_NVRAM_VALUE(wan_pptp_max_idle_time_value);
	FREE_NVRAM_VALUE(wan_pptp_username_value);
	FREE_NVRAM_VALUE(wan_pptp_mtu_value);
	FREE_NVRAM_VALUE(wan_l2tp_max_idle_time_value);
	FREE_NVRAM_VALUE(wan_l2tp_username_value);
	FREE_NVRAM_VALUE(wan_l2tp_mtu_value);
#ifdef IPV6_PPPoE
	FREE_NVRAM_VALUE(ipv6_pppoe_share);
#endif
}

/*
 *	*st: dest string
 *	insert[]: insert string
 *	start: start location
 */
void string_insert(char *st, char insert[], int start) {
	int i, st_length, insert_length;

	st_length = strlen(st);
	insert_length = strlen(insert);
	if (st_length <= start) return;

	for (i = st_length ; i >= start ; i--) {
		st[i + insert_length] = st[i];
	}

	for (i = 0 ; i < insert_length ; i++) {
		st[start + i] = insert[i];
	}
}

/* multiple pppoe pid & logical interface*/
struct mp_info check_mpppoe_info(int pppoe_unit)
{
	static struct mp_info info;
	char file[] = "/var/run/ppp-XX.pid";
	char buf[32]="";
	FILE *fp;
	int i=0;

	sprintf(file, "/var/run/ppp-%02d.pid", pppoe_unit);
	fp = fopen(file, "r");
	if(fp) {
		for(i=0;i<2;i++) {
			fgets(buf,20, fp);
			if(i ==0)
				info.pid = atoi(buf);
			else
				strcpy(info.pppoe_iface, buf);
		}
		fclose(fp);
	} else
		info.pid = -1;

	return info;
}




int check_ip_match(const char *target, const char *model)
{
	int target_ip[4]={0,0,0,0};
	int model_ip[4]={0,0,0,0};

	sscanf(target,"%d.%d.%d.%d",&target_ip[0],&target_ip[1],&target_ip[2],&target_ip[3]);
	sscanf(model,"%d.%d.%d.%d",&model_ip[0],&model_ip[1],&model_ip[2],&model_ip[3]);
/*	Date: 2009-04-10
	*	Name: Ken Chiang
	*	Reason:	Modify for ap mode set ip=192.168.0.50 and Lan ip=192.168.0.1 the function
	            check it match.
	*	Note: 
*/		
/*
	if( (target_ip[0] == model_ip[0]) && (target_ip[1] == model_ip[1]) && 
			(target_ip[2] == model_ip[2]))
		return 0;
*/		
	if( (target_ip[0] == model_ip[0]) && (target_ip[1] == model_ip[1]) && 
			(target_ip[2] == model_ip[2]) && (target_ip[3] == model_ip[3]) )
		return 0;
	return -1;
}

char *parse_special_char(char *p_string)
{
	char hex[400], hex_tmp[200];
	int len = 0, c1 = 0, c2 = 0;

	memset(hex, 0, sizeof(hex));
	memset(hex_tmp, 0, sizeof(hex_tmp));
	len = strlen(p_string);
	strcpy(hex_tmp, p_string);

	for(c1 = 0; c1 < len; c1++)
	{
		/*   ` = 0x60, " = 0x22, \ = 0x5c, ' = 0x27, $ = 0x24       */		

		if((hex_tmp[c1] == 0x22) || (hex_tmp[c1] == 0x5c) || (hex_tmp[c1] == 0x60) || (hex_tmp[c1] == 0x24))
		{
			hex[c2] = 0x5c;
			c2++;
			hex[c2] = hex_tmp[c1];
		}	
		else
			hex[c2] = hex_tmp[c1];
		c2++;		
	}

	strcpy(p_string, hex);
	return p_string;
}

char *get_ip_addr(char *if_name)
{
	int sockfd;
	struct ifreq ifr;
	struct in_addr in_addr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		return 0;
	}

	close(sockfd);

	in_addr.s_addr = sin_addr(&ifr.ifr_addr).s_addr;
	return inet_ntoa(in_addr);
}

int get_mac_addr(char *if_name, char *mac)
{
        int sockfd;
        struct ifreq ifr;

        if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return -1;

        strcpy(ifr.ifr_name, if_name);
        if( ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
                close(sockfd);
                return -1;
        }
        close(sockfd);
        memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
        return 0;
}

/*  Date: 2009-01-16
*   Name: Cosmo Chang
*   Reason: updating missing function
*/
int char2int(char data) {
        int value = 0;
        switch(data) {
                default:
                case '0':
                        value = 0;
                break;
                case '1':
                        value = 1;
                break;
                case '2':
                        value = 2;
                break;
                case '3':
                        value = 3;
                break;
                case '4':
                        value = 4;
                break;
                case '5':
                        value = 5;
                break;
                case '6':
                        value = 6;
                break;
                case '7':
                        value = 7;
                break;
                case '8':
                        value = 8;
                break;
                case '9':
                        value = 9;
                break;
                case 'a':
                        value = 10;
                break;
                case 'b':
                        value = 11;
                break;
                case 'c':
                        value = 12;
                break;
                case 'd':
                        value = 13;
                break;
                case 'e':
                        value = 14;
                break;
                case 'f':
                        value = 15;
                break;
        }
        return value;
}


/*This function is used to remove backslash ('\' ascii code is 0x5c). In case the ssid of router or ap contain '\',
  the scan list of Broadcom wireless driver would add one more backslash in front of backslash, so we have to remove it to display ssid correctly*/
char * remove_backslash(char *input, int input_len)
{
	static char ssid[132];
	char ssid_tmp[66];
	int len;
	int data1 = 0;
	int data2 = 0;
	int data3 = 0;
	int data4 = 0;
	int p = 0;
	int k = 0;


	char data[4];
	char hex[66];
	int i;

	memset(hex,0,sizeof(hex));

	for(i = 0;i < input_len;i++)
	{
		sprintf(data,"%02x",*(input+i));
		strcat(hex,data);
	}

	memset(ssid, 0, sizeof(ssid));
	memset(ssid_tmp, 0, sizeof(ssid_tmp));
	len = strlen(hex);
	strcpy(ssid_tmp, hex);
	for(k = 0; k < len; k++) {
		data1 = char2int(ssid_tmp[k]);
		data2 = char2int(ssid_tmp[k+1]);
		data1 = data1 * 16 +  data2;
		sprintf(ssid, "%s%2x", ssid, data1);
		p++;
		k++;
	}
	return ssid;
}


/*This function is used to transfer hex ssid to ascii for wpasupplicant configuration file*/
char * ssid_transfer(char *wlan_ssid)
{
	static char ssid[132];
	char ssid_tmp[66];
	int len;
	int data1 = 0;
	int data2 = 0;
	int p = 0;
	int k = 0;

	memset(ssid, 0, sizeof(ssid));
	memset(ssid_tmp, 0, sizeof(ssid_tmp));
	len = strlen(wlan_ssid);
	DEBUG_MSG("ssid length = %02d \n", len);
	DEBUG_MSG("ssid_transfer = %s\n", wlan_ssid);
	strcpy(ssid_tmp, wlan_ssid);
	for(k = 0; k < len; k++) {
		data1 = char2int(ssid_tmp[k]);
		data2 = char2int(ssid_tmp[k+1]);
		data1 = data1 * 16 +  data2;
		ssid[p]= toascii(data1);
		p++;
		k++;
	}
	DEBUG_MSG("ssid (after) : %s\n", ssid);
	return ssid;
}

/*This function is used to check ssid which contains specific character (` ascii code is 0x64, " ascii code is 0x22, \ ascii code is 0x5c.
  Once we detect the three specific characters, we add backslash in front of it*/
char * ssid_check(char *wlan_ssid)
{
	static char ssid[132];
	char ssid_tmp[66];
	int len;
	int data1 = 0;
	int data2 = 0;
	int p = 0;
	int k = 0;

	memset(ssid, 0, sizeof(ssid));
	memset(ssid_tmp, 0, sizeof(ssid_tmp));
	len = strlen(wlan_ssid);
	DEBUG_MSG("ssid length = %02d \n", len);
	DEBUG_MSG("ssid_check = %s\n", wlan_ssid);
	strcpy(ssid_tmp, wlan_ssid);
	for(k = 0; k < len; k++) {
		data1 = char2int(ssid_tmp[k]);
		data2 = char2int(ssid_tmp[k+1]);
		data1 = data1 * 16 +  data2;
		if((data1 == 0x60) || (data1 == 0x22) || (data1 == 0x5c)) { // ` = 0x60, " = 0x22, \ = 0x5c 
			ssid[p] = 0x5c; //in order to display specfic character, we place 0x5c(\) in front of (`, ", \)
			p++;
		}
		ssid[p]= toascii(data1);		
		p++;
		k++;
	}		
	DEBUG_MSG("ssid_check (after) : %s\n", ssid);
	return ssid;
}

/**********************************************************************/
#ifdef CONFIG_USER_WL_ATH
char _ath_cipher_parser_buffer[BUFFER_LENGTH] = "";
char _ath_cipher_parser_mac_tmp[BUFFER_LENGTH] = "";
int ath_cipher_parser(int wireless_band)
{
	FILE *fp;
	char *buffer=_ath_cipher_parser_buffer;
	char *mac_tmp=_ath_cipher_parser_mac_tmp;
	char *mac_s, *mac_e;
	char wds_remote_mac[24] = "";
	char *wpa_ie;
	char *rsn_ie;
	int wpa_ie_len = 0;
	int rsn_ie_len = 0;
	int cipher_type = -1;
	char *INIT_NVRAM_VALUE(nvram_tmp_value);

	if(wireless_band == WL_BAND_24G) {
		strcpy(wds_remote_mac, add_token(cmd_nvram_safe_get("wlan0_wds_remote_mac",nvram_tmp_value)));
		if(strcmp(ATH_BRIDGE, cmd_nvram_safe_get("wlan0_op_mode",nvram_tmp_value)) == 0) {			
			_system("iwlist ath0 scanning > %s", AP_SCAN_LIST);
			sleep(3);
		}		
		else if(strcmp(ATH_REPEATER, cmd_nvram_safe_get("wlan0_op_mode",nvram_tmp_value)) == 0) {			
			_system("iwlist ath1 scanning > %s", AP_SCAN_LIST);
			sleep(3);
		}
	} else if(wireless_band == WL_BAND_5G) {
		strcpy(wds_remote_mac, add_token(cmd_nvram_safe_get("wlan1_wds_remote_mac",nvram_tmp_value)));
		if(strcmp(ATH_BRIDGE, cmd_nvram_safe_get("wlan1_op_mode",nvram_tmp_value)) == 0) {			
			_system("iwlist ath0 scanning > %s", AP_SCAN_LIST);
			sleep(3);
		}
		else if(strcmp(ATH_REPEATER, cmd_nvram_safe_get("wlan1_op_mode",nvram_tmp_value)) == 0) {			
			_system("iwlist ath1 scanning > %s", AP_SCAN_LIST);
			sleep(3);
		}
	}	

	if((fp = fopen(AP_SCAN_LIST, "r+")) == NULL){
		DEBUG_MSG("fail to open SCAN_LIST_FILE\n");
		FREE_NVRAM_VALUE(nvram_tmp_value);
		return -1;
		//return "fail to open SCAN_LIST_FILE";
	} else {
		while(fgets(buffer, BUFFER_LENGTH, fp)){
			if(strstr(buffer, "Address") != NULL){
				mac_s = strstr(buffer, ":") + 2;
				mac_e = strstr(buffer, ":") + 19;
				memset(mac_tmp, 0, BUFFER_LENGTH); 					
				strncpy(mac_tmp, mac_s, mac_e - mac_s);				
				if(strcmp(mac_tmp, wds_remote_mac) == 0){
					DEBUG_MSG("mac match\n");
					while(fgets(buffer, BUFFER_LENGTH, fp)) {
						if(strstr(buffer, "wpa_ie") != NULL) {	// keyword "wpa_ie", help us to check cipher auto of WPA
							wpa_ie = strstr(buffer, CIPHER_BEGIN_KEYWORD);
							wpa_ie_len = strlen(wpa_ie + 2);
							if(wpa_ie_len == 56) {
								cipher_type = CIPHER_AUTO;
								printf("WPA CIPHER_AUTO\n");
								break;							
							} else if(wpa_ie_len == 48) {
								if(strstr(wpa_ie, WPA_IE_TKIP)) {
									cipher_type = CIPHER_TKIP;
									printf("WPA CIPHER_TKIP\n");
									break;								
								} else if(strstr(wpa_ie, WPA_IE_AES)) {
									cipher_type = CIPHER_AES;
									printf("WPA CIPHER_AES\n");
									break;
								} else {
									cipher_type = -1;
									printf("cipher no match\n");
									break;
								}
							} else {
								printf("the length of wpa_ie isn't match\n");
							}
							break;
						} else if (strstr(buffer, "rsn_ie") != NULL) { // keyword "rsn_ie", help us to check cipher auto of WPA2
							rsn_ie = strstr(buffer, CIPHER_BEGIN_KEYWORD);
							rsn_ie_len = strlen(rsn_ie + 2);
							if(rsn_ie_len == 52) {
								cipher_type = CIPHER_AUTO;
								printf("WPA2 CIPHER_AUTO\n");
								break;
							} else if(rsn_ie_len == 44) {
								if(strstr(rsn_ie, RSN_IE_TKIP)) {
									cipher_type = CIPHER_TKIP;
									printf("WPA2 CIPHER_TKIP\n");
									break;								
								} else if(strstr(rsn_ie, RSN_IE_AES)) {
									cipher_type = CIPHER_AES;
									printf("WPA2 CIPHER_AES\n");
									break;								
								}else{
									cipher_type = -1;
									printf("cipher no match\n");
									break;
								}
							} else {
								printf("the length of rsn_ie isn't match\n");
							}
							break;
						}
					}
				}
			}	
		}
	}
	fclose(fp);
	FREE_NVRAM_VALUE(nvram_tmp_value);
	return cipher_type;
}

int wpa_sup_config(const int radio_mode,const char *key_mgmt, const char *proto)
{
	char wlan_psk_cipher_type[32] = {};
	char wlan_ssid[16] = {};
	char wlan_psk_pass_phrase[32] = {};
	char cipher[] = "TKIP AES BOTH XXXXXXX";
	char pairwise[] = "TKIP AES BOTH XXXXXXX";
	char group[] = "TKIP AES BOTH XXXXXXX";
	int cipher_type = CIPHER_AUTO;
	int retry_count = 0;
	char *INIT_NVRAM_VALUE(nvram_tmp_value);
	char *INIT_NVRAM_VALUE(nvram_tmp_value2);
	sprintf(wlan_psk_cipher_type,"wlan%d_psk_cipher_type",radio_mode);
	sprintf(wlan_ssid,"wlan%d_ssid",radio_mode);
	sprintf(wlan_psk_pass_phrase,"wlan%d_psk_pass_phrase",radio_mode);
	
	if( !cmd_nvram_match(wlan_psk_cipher_type, "tkip") )
		strcpy(cipher, "TKIP");
	else if ( !cmd_nvram_match(wlan_psk_cipher_type, "aes") )
		strcpy(cipher, "CCMP");
	else
		strcpy(cipher, "TKIP CCMP");

	cipher_type = ath_cipher_parser(radio_mode);
	while((cipher_type < 0) && (retry_count < 3)) {
		cipher_type = ath_cipher_parser(radio_mode);
		retry_count++;
		DEBUG_MSG(" ======= retry_count : %d =======\n", retry_count);
	}
	DEBUG_MSG("\n ======== %s ======== cipher_type : %d\n", __func__, cipher_type);
	switch (cipher_type) {
		case CIPHER_AUTO:			
			if(strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "both", 4) == 0) {				
				strcpy(pairwise, "CCMP");
				strcpy(group, "TKIP");				
			} else if (strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "tkip", 4) == 0) {
				strcpy(pairwise, "TKIP");
				strcpy(group, "TKIP");				
			} else if (strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "aes", 3) == 0) {
				strcpy(pairwise, "CCMP");
				strcpy(group, "TKIP");				
			}
			break;
		case CIPHER_TKIP:		
			if(strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "both", 4) == 0) {
				strcpy(pairwise, "TKIP");
				strcpy(group, "TKIP");				
			} else if (strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "tkip", 4) == 0) {
				strcpy(pairwise, "TKIP");
				strcpy(group, "TKIP");				
			} else if (strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "aes", 3) == 0) {
				strcpy(pairwise, "CCMP");
				strcpy(group, "CCMP");				
			}
			break;
		case CIPHER_AES:		
			if(strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "both", 4) == 0) {
				strcpy(pairwise, "CCMP TKIP");
				strcpy(group, "CCMP TKIP");				
			} else if (strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "tkip", 4) == 0) {
				strcpy(pairwise, "TKIP");
				strcpy(group, "TKIP");				
			} else if (strncmp(cmd_nvram_safe_get(wlan_psk_cipher_type,nvram_tmp_value), "aes", 3) == 0) {
				strcpy(pairwise, "CCMP");
				strcpy(group, "CCMP");				
			}
			break;
		default :
			strcpy(pairwise, "TKIP");
			strcpy(group, "TKIP");
			break;
	}
	init_file(WPASUP_CONFIG);
	save2file(WPASUP_CONFIG,
			"ap_scan=2\n"
			"network={\n"
			"   ssid=\"%s\"\n"
			"   key_mgmt=%s\n"
			"   proto=%s\n"
			"   pairwise=%s\n"
			"   group=%s\n"
			"   psk=\"%s\"\n"
			"}\n",
			ssid_transfer(cmd_nvram_get(wlan_ssid,nvram_tmp_value)),
			key_mgmt,
			proto,
			cipher,
			cipher,
			cmd_nvram_get(wlan_psk_pass_phrase,nvram_tmp_value2) );

	
	FREE_NVRAM_VALUE(nvram_tmp_value);
	FREE_NVRAM_VALUE(nvram_tmp_value2);
	return 0;
}

int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space )
{
	int i, n=0;
	int table_index = 0; 
	int group = 0;
	int nChannel = 0;
	struct freq_table_s *wireless_freq_table;
	int table_size = 0;
	int low_channel = 0;
	int high_channel = 0;
	int channel = 0;
	int channel_list[100] ={0};
	int band_width;
	int tmp_channel;

	while (1)
	{
		if (domain_name[n].number == 0xffff)    
			break;    /* not found */
		if (domain_name[n].number == number)    
			break;    /* found */
		n++;
	}
	//	band = WIRELESS_BAND_24G; //set band to 2.4G
#if defined (USER_WL_ATH_5GHZ) //5G interface 2
	if (band == WIRELESS_BAND_50G)
	{
		table_index = domain_name[n].wireless5_freq_table;
		wireless_freq_table = wireless5_table;
		table_size = sizeof(wireless5_table) / sizeof(struct freq_table_s);
		band_width = 4;
	}
	else
	{
#endif
		table_index = domain_name[n].wireless2_freq_table;
		wireless_freq_table = wireless2_table;
		table_size = sizeof(wireless2_table) / sizeof(struct freq_table_s);
		band_width = 1;
#if defined (USER_WL_ATH_5GHZ) //5G interface 2
	}        
#endif
	        
	for (n=0; n<table_size; n++)
	{
		if (wireless_freq_table[n].regdomain_mode == table_index)
		{
			for (group=0; group< wireless_freq_table[n].no_group; group++)
			{
				low_channel = wireless_freq_table[n].group[group].low_channel;
				high_channel = wireless_freq_table[n].group[group].high_channel;

				for(channel=low_channel; channel<=high_channel; (channel+=(5*band_width)))
				{
#if defined (USER_WL_ATH_5GHZ) //5G interface 2
					if (band == WIRELESS_BAND_50G)
					{
#ifndef DFS_SUPPORT
						if((channel>=5250 && channel<=5350)|| (channel>=5470 && channel<=5725) || \
						   (channel>=5825 && !cmd_nvram_match("wlan1_11n_protection","auto"))){							
							continue;
						}	
#endif
						channel_list[nChannel++] = (channel - 5000) / 5;
					}
					else
					{
#endif //defined (USER_WL_ATH_5GHZ)
						if (channel == 2484)//for MKK 14 channel
							channel_list[nChannel++] = 14;
						else{
							//Albert modify for different cross compiler
							//channel_list[nChannel++] = (channel - 2412) / 5 + 1;
							tmp_channel = nChannel;
							if(channel == 2412)
								channel_list[nChannel++] = ((channel - 2412)) + 1;
							else
								channel_list[nChannel++] = ((channel - 2412)) - 4*tmp_channel + 1;
						}
#if defined (USER_WL_ATH_5GHZ) //5G interface 2
					}
#endif
				}
			}
		}
	}
	
	sprintf(wlan_channel_list, "%d", channel_list[0]);
/* 
 	Date: 2009-3-5 
 	Name: Ken_Chiang
 	Reason: modify for show chanel list used space.
 	Notice :
*/ 
/*
	for(i = 1 ; i < nChannel; i++)
		sprintf(wlan_channel_list,"%s,%d", wlan_channel_list, channel_list[i]);
*/			
	if(show_used_space){
		for(i = 1 ; i < nChannel; i++)
			sprintf(wlan_channel_list,"%s, %d", wlan_channel_list, channel_list[i]);
	}
	else{	
		for(i = 1 ; i < nChannel; i++)
			sprintf(wlan_channel_list,"%s, %d", wlan_channel_list, channel_list[i]);
	}		
	DEBUG_MSG("%s, wlan_channel_list=%s",__func__,wlan_channel_list);	
	return nChannel;
}
#endif//CONFIG_USER_WL_ATH
/**********************************************************************/

/**********************************************************************/
#ifdef CONFIG_USER_WL_RTL
int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[])
{
	int n=0;
	int table_index = 0; 
	int group = 0;
	int nChannel = 0;
	struct freq_table_s *wireless_freq_table;
	int table_size = 0;
	int low_channel = 0;
	int high_channel = 0;
	int channel = 0;	
	int band_width;	

	while (1){
		if (domain_name[n].number == NOT_FOUND || domain_name[n].number == number)    
			break;    /* not found */
		n++;
	}
	table_index = domain_name[n].wireless2_freq_table;
	wireless_freq_table = wireless2_table;
	table_size = sizeof(wireless2_table) / sizeof(struct freq_table_s);
	band_width = 1;
	        
	memset(wlan_channel_list,0,sizeof(wlan_channel_list));
	for (n=0; n<table_size; n++){
		if (wireless_freq_table[n].regdomain_mode == table_index){
			for (group=0; group< wireless_freq_table[n].no_group; group++){
				low_channel = wireless_freq_table[n].group[group].low_channel;
				high_channel = wireless_freq_table[n].group[group].high_channel;

				for(channel=low_channel; channel<=high_channel; channel++){
					nChannel++;										
					/* init format */
					if(strlen(wlan_channel_list)==0){						
						sprintf(wlan_channel_list, "%d", channel);
					}else{
						sprintf(wlan_channel_list,"%s, %d", wlan_channel_list, channel);
					}	
				}
			}
		}
	}
		
	DEBUG_MSG("%s, wlan_channel_list=%s",__func__,wlan_channel_list);	
	return nChannel;
}
#endif//CONFIG_USER_WL_RTL
/**********************************************************************/

/**********************************************************************/
#ifdef CONFIG_USER_WL_MVL
int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space )
{
	int i, n=0;
	int table_index = 0; 
	int group = 0;
	int nChannel = 0;
	struct freq_table_s *wireless_freq_table;
	int table_size = 0;
	int low_channel = 0;
	int high_channel = 0;
	int channel = 0;
	int channel_list[100] ={0};
	int band_width;
	int tmp_channel;

	DEBUG_MSG("%s, wlan_channel_list=%s",__func__,wlan_channel_list);	
	return nChannel;
}
#endif//CONFIG_USER_WL_MVL
/**********************************************************************/

/**********************************************************************/
#ifdef CONFIG_USER_WL_RALINK
int GetDomainChannelList(unsigned short number, int band, char wlan_channel_list[], int show_used_space )
{
	int i, n=0;
	int table_index = 0; 
	int group = 0;
	int nChannel = 0;
	struct freq_table_s *wireless_freq_table;
	int table_size = 0;
	int low_channel = 0;
	int high_channel = 0;
	int channel = 0;
	int channel_list[100] ={0};
	int band_width;
	int tmp_channel;

	while (1)
	{
		if (domain_name[n].number == 0xffff)    
			break;    /* not found */
		if (domain_name[n].number == number)    
			break;    /* found */
		n++;
	}
	//	band = WIRELESS_BAND_24G; //set band to 2.4G
#if defined (USER_WL_RALINK_5GHZ)//5G interface 2
	if (band == WIRELESS_BAND_50G)
	{
		table_index = domain_name[n].wireless5_freq_table;
		wireless_freq_table = wireless5_table;
		table_size = sizeof(wireless5_table) / sizeof(struct freq_table_s);
		band_width = 4;
	}
	else
	{
#endif
		table_index = domain_name[n].wireless2_freq_table;
		wireless_freq_table = wireless2_table;
		table_size = sizeof(wireless2_table) / sizeof(struct freq_table_s);
		band_width = 1;
#if defined(USER_WL_RALINK_5GHZ)//5G interface 2
	}        
#endif
	        
	for (n=0; n<table_size; n++)
	{
		if (wireless_freq_table[n].regdomain_mode == table_index)
		{
			for (group=0; group< wireless_freq_table[n].no_group; group++)
			{
				low_channel = wireless_freq_table[n].group[group].low_channel;
				high_channel = wireless_freq_table[n].group[group].high_channel;

				for(channel=low_channel; channel<=high_channel; (channel+=(5*band_width)))
				{
#if defined(USER_WL_RALINK_5GHZ)//5G interface 2
					if (band == WIRELESS_BAND_50G)
					{
#ifndef DFS_SUPPORT
						if((channel>=5250 && channel<=5350)|| (channel>=5470 && channel<=5725) || \
						   (channel>=5825 && !cmd_nvram_match("wlan1_11n_protection","auto"))){							
							continue;
						}	
#endif
						channel_list[nChannel++] = (channel - 5000) / 5;
					}
					else
					{
#endif //USER_WL_RALINK_5GHZ)
						if (channel == 2484)//for MKK 14 channel
							channel_list[nChannel++] = 14;
						else{
							//Albert modify for different cross compiler
							//channel_list[nChannel++] = (channel - 2412) / 5 + 1;
							tmp_channel = nChannel;
							if(channel == 2412)
								channel_list[nChannel++] = ((channel - 2412)) + 1;
							else
								channel_list[nChannel++] = ((channel - 2412)) - 4*tmp_channel + 1;
						}
#if defined(USER_WL_RALINK_5GHZ)//5G interface 2
					}
#endif
				}
			}
		}
	}
	
	sprintf(wlan_channel_list, "%d", channel_list[0]);
/* 
 	Date: 2009-3-5 
 	Name: Ken_Chiang
 	Reason: modify for show chanel list used space.
 	Notice :
*/ 
/*
	for(i = 1 ; i < nChannel; i++)
		sprintf(wlan_channel_list,"%s,%d", wlan_channel_list, channel_list[i]);
*/			
	if(show_used_space){
		for(i = 1 ; i < nChannel; i++)
			sprintf(wlan_channel_list,"%s, %d", wlan_channel_list, channel_list[i]);
	}
	else{	
		for(i = 1 ; i < nChannel; i++)
			sprintf(wlan_channel_list,"%s, %d", wlan_channel_list, channel_list[i]);
	}		
	DEBUG_MSG("%s, wlan_channel_list=%s",__func__,wlan_channel_list);	
	return nChannel;
}
#endif//CONFIG_USER_WL_RALINK
/**********************************************************************/

void generate_pin_by_mac(char *wps_pin)
{
	unsigned long int rnumber = 0;
	unsigned long int pin = 0;
	unsigned long int accum = 0;
	int digit = 0;
	int checkSumValue =0 ;
	char mac[32];
	char *mac_ptr = NULL;
	int i = 0;
	char *INIT_NVRAM_VALUE(nvram_tmp_value);

	//get mac
	strncpy(mac, cmd_nvram_safe_get("lan_mac",nvram_tmp_value), 18);
	mac_ptr = mac;
	//remove ':'
	for(i =0 ; i< 5; i++ )
	{
		memmove(mac+2+(i*2), mac+3+(i*3), 2);
	}
	memset(mac+12, 0, strlen(mac)-12);

	/* Generate a default pin by mac.
	 * Since we only need 7 numbers, the last 6 numbers
	 * of mac is enough! (16^6-1 =  16777275)
	 */
	mac_ptr = mac_ptr + 6;
	rnumber = strtoul( mac_ptr, NULL, 16 );
	if( rnumber > 10000000 )
		rnumber = rnumber - 10000000;
	pin = rnumber;

	//Compute the checksum
	rnumber *= 10;
	accum += 3 * ((rnumber / 10000000) % 10);
	accum += 1 * ((rnumber / 1000000) % 10);
	accum += 3 * ((rnumber / 100000) % 10);
	accum += 1 * ((rnumber / 10000) % 10);
	accum += 3 * ((rnumber / 1000) % 10);
	accum += 1 * ((rnumber / 100) % 10);
	accum += 3 * ((rnumber / 10) % 10);
	digit = (accum % 10);
	checkSumValue = (10 - digit) % 10;

	pin = pin*10 + checkSumValue;
	sprintf( wps_pin, "%08d", pin );
	FREE_NVRAM_VALUE(nvram_tmp_value);
	return;

}

/* Access LAN MAC / Domain / Region / HW Revsion */
int init_macblock(void) 
{
	/* Erase Mac Block */
	int mtd_block = 0,result = 0;
	char *mac_buffer;
	mtd_block = open(SYS_MAC_MTDBLK, O_WRONLY);
	if(!mtd_block){
		DEBUG_MSG("MAC BLOCK %s open fail!",SYS_MAC_MTDBLK);
		return 0;
	}
	mac_buffer = malloc(MAC_BUF_SIZE);
	bzero(mac_buffer,MAC_BUF_SIZE);
	result = write(mtd_block, mac_buffer, MAC_BUF_SIZE);
	if(result < 0) {
		DEBUG_MSG("MAC BLOCK Erase fail!!");
		free(mac_buffer);
		close(mtd_block);
		return 0;
	} 
	DEBUG_MSG("Initial Success length is %u(0x%x)",result,result);
	free(mac_buffer);
	close(mtd_block);
	return 1;
}

int write_macblock(char* mac, char* hwver, char* domain, char* region) 
{
	struct block_data macblock;
	int mtd_block = 0, result = 0;
	mtd_block = open(SYS_MAC_MTDBLK, O_WRONLY);
	if(!mtd_block){
		DEBUG_MSG("MAC BLOCK %s open fail!",SYS_MAC_MTDBLK);
		return 0;
	}
	bzero(&macblock,sizeof(macblock));
	macblock.init_flag = 1;
	strcpy(macblock.mac,mac);
	strcpy(macblock.hwver,hwver);
	strcpy(macblock.domain,domain);
	strcpy(macblock.region,region);
	DEBUG_MSG("mac = 0x%s, HW ver = %s, domain = %s, region = %s",macblock.mac,macblock.hwver,macblock.domain,macblock.region);
	result = write(mtd_block,&macblock,sizeof(macblock));
	if(result < 0) {
		DEBUG_MSG("Write macblock fail!!");
		close(mtd_block);
		return 0;
	}
	DEBUG_MSG("Write macblock success!!");
	close(mtd_block);
	return 1;
}

int send_log_by_smtp(void)
{
        /*
           msmtp -d test_recipient@testingsmtp.com     //recipient
           --auth=login                                 //authentication method
           --user=test                                          //username
           --host=testing.smtp.com                              //smtp server
           --from=test_sender@testingsmtp.com           //sender
           --passwd test                                        //password
           --port => NickChou Add for setting SMTP Server Port
           */
        FILE *fp;
        char log_email_server[64]={0}, log_email_username[64]={0}, log_email_password[64]={0};
        char log_email_recipien[64]={0}, log_email_sender[64]={0}, log_email_auth[64]={0};
        char log_email_server_port[6]={0};
        char buffer[256];
        char *ptr;

        fp=fopen(SMTP_CONF, "r");
        if(fp==NULL)
                return 0;

        while(fgets(buffer, 256, fp))
        {
                ptr = strstr(buffer, "log_email_auth=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_auth=");
                        strncpy(log_email_auth, ptr, strlen(buffer)-strlen("log_email_auth=")-1);
                        continue;
                }

                ptr = strstr(buffer, "log_email_recipien=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_recipien=");
                        strncpy(log_email_recipien, ptr, strlen(buffer)-strlen("log_email_recipien=")-1);
                        if(strlen(log_email_recipien) == 0)
                                return 0;
                        continue;
                }

                ptr = strstr(buffer, "log_email_username=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_username=");
                        strncpy(log_email_username, ptr, strlen(buffer)-strlen("log_email_username=")-1);
                        continue;
                }

                ptr = strstr(buffer, "log_email_password=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_password=");
                        strncpy(log_email_password, ptr, strlen(buffer)-strlen("log_email_password=")-1);
                        continue;
                }

                ptr = strstr(buffer, "log_email_server=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_server=");
                        strncpy(log_email_server, ptr, strlen(buffer)-strlen("log_email_server=")-1);
                        if(strlen(log_email_server) == 0)
                                return 0;
                        continue;
                }

                ptr = strstr(buffer, "log_email_sender=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_sender=");
                        strncpy(log_email_sender, ptr, strlen(buffer)-strlen("log_email_sender=")-1);
                        if(strlen(log_email_sender) == 0)
                                return 0;
                        continue;
                }

                ptr = strstr(buffer, "log_email_server_port=");
                if(ptr)
                {
                        ptr = ptr + strlen("log_email_server_port=");
                        strncpy(log_email_server_port, ptr, strlen(buffer)-strlen("log_email_server_port=")-1);
                        if(strlen(log_email_server_port) == 0)
                                return 0;
                        continue;
                }

        }

        fclose(fp);
        DEBUG_MSG("send_log_by_smtp\n");

        if( (strcmp(log_email_auth, "1") == 0) )
        {
                _system("msmtp %s --auth=  --user=%s  --passwd %s --host=%s --from=%s --port=%s &",
                                log_email_recipien, log_email_username, log_email_password, log_email_server, log_email_sender, log_email_server_port);
        }
        else
        {
                _system("msmtp %s --host=%s --from=%s --port=%s &",
                                log_email_recipien, log_email_server, log_email_sender, log_email_server_port);               
        }

        return 1;
}

/*  Date: 2010-05-04
*   Name: Cosmo Chang
*   Reason: get the usb type
*   Notice: 
*/
#ifdef USB_GENERAL_API_SUPPORT
static int if_usb_3g_mode(void){
	if(	cmd_nvram_match("wan_proto",USB_GENERAL_PROTO)==0 &&\
		cmd_nvram_match("wlan0_mode","rt")==0)
		return USB_IN_3G_MODE;
	else
		return USB_OUT_3G_MODE;
}
#ifdef USB_3G_PHONE_SUPPORT	
static int if_usb_3g_phone_mode(void){
	if(	cmd_nvram_match("wan_proto",USB_3G_PHONE_PROTO)==0 &&\
		cmd_nvram_match("wlan0_mode","rt")==0)
		return USB_IN_3G_MODE;
	else
		return USB_OUT_3G_MODE;
}
#endif	//	USB_3G_PHONE_SUPPORT
#endif	//	USB_GENERAL_API_SUPPORT
int get_usb_type(void){	
	if (0){		
		// make define flag reasonable
	}
#ifdef USB_GENERAL_API_SUPPORT	
#ifdef USB_SHARE_PORT_SUPPORT	
	else if(cmd_nvram_match("usb_type",USB_SHARE_TYPE)==0 && !if_usb_3g_mode() 
#ifdef USB_3G_PHONE_SUPPORT		
		&& !if_usb_3g_phone_mode()
#endif		
		){		
		return atoi(USB_SHARE_TYPE);
	}
#endif
#ifdef CONFIG_WCN
	else if(cmd_nvram_match("usb_type",USB_WCN_TYPE)==0 && !if_usb_3g_mode()
#ifdef USB_3G_PHONE_SUPPORT		
		&& !if_usb_3g_phone_mode()
#endif		
		){		
		return atoi(USB_WCN_TYPE);
	}
#endif	
	else if(cmd_nvram_match("usb_type",USB_3G_ADAPTER_TYPE)==0 && if_usb_3g_mode()){		
		return atoi(USB_3G_ADAPTER_TYPE);
	}	
#ifdef USB_3G_PHONE_SUPPORT	
#ifdef CONFIG_USER_3G_USB_CLIENT_WM5	
	else if(cmd_nvram_match("usb_type",USB_WIN_MOBILE_5_TYPE)==0 && if_usb_3g_phone_mode()){		
		return atoi(USB_WIN_MOBILE_5_TYPE);
	}
#endif
#ifdef CONFIG_USER_3G_USB_CLIENT_IPHONE		
	else if(cmd_nvram_match("usb_type",USB_I_PHONE_TYPE)==0 && if_usb_3g_phone_mode()){
		return atoi(USB_I_PHONE_TYPE);
	}
#endif
#ifdef CONFIG_USER_3G_USB_CLIENT_ADR	
	else if(cmd_nvram_match("usb_type",USB_ANDROID_PHONE_TYPE)==0 && if_usb_3g_phone_mode()){		
		return atoi(USB_ANDROID_PHONE_TYPE);
	}
#endif	
#endif	//	USB_3G_PHONE_SUPPORT
#endif	//	USB_GENERAL_API_SUPPORT
	else{		
		return USB_UNKNOWN_MODE;
	}	
}

char *schedule2iptables(char *schedule_name)
{
        static char  schedule_value[100]={0};
        char value[128] = {0}, week[32]={0};
        char *nvram_ptr=NULL;
        char *name = NULL, *weekdays = NULL, *start_time = NULL, *end_time = NULL;
        char schedule_list[]="schedule_rule_00";
        char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        int i=0, day, count=0;

        if ( (schedule_name == NULL) || (strcmp ("Always", schedule_name ) == 0) || (strcmp(schedule_name, "") == 0) )
                return "";

        /*      Schedule format
		name/weekdays/start_time/end_time
		Test/1101011/08:00/18:00
		only valid for PRE_ROUTING, LOCAL_IN, FORWARD and OUTPUT
	*/
        for(i=0; i<MAX_SCHEDULE_NUMBER; i++)
        {
                snprintf(schedule_list, sizeof(schedule_list), "schedule_rule_%02d", i);
                cmd_nvram_safe_get(schedule_list,nvram_ptr);
                if ( nvram_ptr == NULL || strcmp(nvram_ptr, "") == 0)
                        continue;
                strncpy(value,nvram_ptr,sizeof(value));
                if( getStrtok(value, "/", "%s %s %s %s", &name, &weekdays, &start_time, &end_time) !=4)
                        continue;

                /* Add , symbol between date */
                if ( strcmp(name, schedule_name) == 0) {

                        for(day=0; day<7; day++) {
                                if ( *(weekdays + day) == '1') {
                                        count += 1;

                                        if( count >1 ) {
                                                strcat(week, ",");
                                                count -=1;
                                        }
                                        strcat(week, weekday[day] );
                                }
                        }

                        /* If start_time = end_time or start_tiem=00:00 and end_time=24:00, it means all day */
                        if((strcmp(start_time, end_time) == 0) || ( !strcmp(start_time,"00:00") && !strcmp(end_time, "24:00")))
                                sprintf(schedule_value, "-m time --weekdays %s", week);
                        else
                                sprintf(schedule_value, "-m time --timestart %s --timestop %s --weekdays %s", start_time, end_time, week);
                        return schedule_value;
                }
        }
        return "";
}

