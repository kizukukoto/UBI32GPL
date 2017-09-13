#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>

#include "querys.h"
#include "ssc.h"
#include "ssi.h"
#include "log.h"
#include "libdb.h"
#include "rcctrl.h"
#define SYS_SERVICE	"sys_service"	/* what services need restart? */
#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif
extern void make_back_msg(const char *, ...);	/* main/core/helper.c */
extern void updown_services(int, const char *);  /* main/libssc.c */
extern char *get_response_page();	/* main/ext/sscinf.c */
extern void __do_reboot(struct ssc_s *obj);

static void wscPinGen()
{
	unsigned long PIN;
	unsigned long int accum = 0;
	int digit;
	char devPwd[32];

	srand(time((time_t *)NULL));
	PIN = rand();
	sprintf( devPwd, "%08u", PIN );
	devPwd[7] = '\0';

	PIN = strtoul( devPwd, NULL, 10 );

	PIN *= 10;
	accum += 3 * ((PIN / 10000000) % 10);
	accum += 1 * ((PIN / 1000000) % 10);
	accum += 3 * ((PIN / 100000) % 10);
	accum += 1 * ((PIN / 10000) % 10);
	accum += 3 * ((PIN / 1000) % 10);
	accum += 1 * ((PIN / 100) % 10);
	accum += 3 * ((PIN / 10) % 10);

	digit = (accum % 10);
	accum = (10 - digit) % 10;

	PIN += accum;
	sprintf( devPwd, "%08u", PIN );
	devPwd[8] = '\0';

	cprintf("Generate WPS PIN = %s\n", devPwd);
	nvram_set("wsc_device_pin", devPwd);
}

static void RandAsciiKey(int len, char* key)
{
	char buf[64]="";
	int i=0, nkey=0;

	if(len<8 || len>63)
		len=10;

	memset(buf, 0, sizeof(buf));
	srand((int)time(0));
	for(i=0; i<len; i++)
	{
		nkey =  (rand() % 0x7A) | 0x21;
		if( ispunct((char)nkey) || (nkey>0x7A)) {
			buf[i] = (char)( 0x41 + i*7%25 );
		} else {
			buf[i] = (char)nkey;
		}
	}
	buf[len]='\0';
	strcpy(key, buf);
}

static int wscPincheck(char *pin_string)
{
	unsigned long PIN = strtoul(pin_string, NULL, 10 );;
	unsigned long int accum = 0;

	accum += 3 * ((PIN / 10000000) % 10);
	accum += 1 * ((PIN / 1000000) % 10);
	accum += 3 * ((PIN / 100000) % 10);
	accum += 1 * ((PIN / 10000) % 10);
	accum += 3 * ((PIN / 1000) % 10);
	accum += 1 * ((PIN / 100) % 10);
	accum += 3 * ((PIN / 10) % 10);
	accum += 1 * ((PIN / 1) % 10);

	if (0 == (accum % 10))
		return 0;
	else
		return 1;
}

void *do_wps_action()
{
	char status[15]="";
	char str[64]="", strSSID[32]="";
	char *rPage;
	int nFlag=0;
	char *pincode;

	query_vars("status", status, sizeof(status));

	query_vars("pci/1/1/il0macaddr", str, sizeof(str));
	sprintf(strSSID, "dlink%c%c%c%c", str[12], str[13], str[15], str[16]);
	if( strcmp(nvram_safe_get("wsc_config_state"), "0")==0 &&
		strcmp(status, "unconfigured")!=0 &&
		strcmp(status, "reset")!=0 &&
		strlen(status)>0 )
	{
		memset(str, 0, sizeof(str));
		query_vars("wl0_ssid", str, sizeof(str));
		if( strcmp("dlink", str)!=0 )
			nFlag = 1;

		memset(str, 0, sizeof(str));
		query_vars("wl0_channel", str, sizeof(str));
		if( strcmp("6", str)!=0 )
			nFlag = 1;
		memset(str, 0, sizeof(str));
		query_vars("wl0_wep", str, sizeof(str));
		if( strcmp("disabled", str)!=0 )
			nFlag = 1;

		memset(str, 0, sizeof(str));
		query_vars("wl0_auth", str, sizeof(str));
		if( strcmp("0", str)!=0 )
			nFlag = 1;

		memset(str, 0, sizeof(str));
		query_vars("wl0_akm", str, sizeof(str));
		if( strcmp("", str)!=0 )
			nFlag = 1;

		if(nFlag==0)
		{
			cprintf("*****Reset SSID to dlinkXXX*****\n");

			nFlag = 1;
			nvram_set("wl0_ssid", strSSID);
			memset(str, 0, sizeof(str));
			RandAsciiKey(63, str);

			nvram_set("wl0_wep", "disabled");
			nvram_set("wl_wep", "disabled");
			nvram_set("wl_akm", "psk");
			nvram_set("wl0_akm", "psk");
			nvram_set("wl0_crypto", "tkip");
			nvram_set("wl_crypto", "tkip");
			nvram_set("wl0_channel", "6");
			nvram_set("wl0_wpa_psk", str);
			nvram_set("wl_wpa_psk", str);
		}

	}

	cprintf( "WPS status=%s, nFlag=%d\n", status, nFlag);

	nvram_set("wsc_timeout_enable", "1");
	if(strcmp(status, "new")==0 ) {
		nFlag=1;
		wscPinGen();
		nvram_set("wsc_mode", "enabled");
		nvram_set("wl0_wsc_mode", "enabled");
		nvram_set("wsc_method", "1");
		nvram_set("wsc_config_state", "1");
		nvram_set("wsc_addER", "1");
		nvram_set("wl_wsc_reg", "enabled");
		nvram_set("wl0_wsc_reg", "enabled");
		nvram_set("wsc_restart", "yes");
		nvram_commit();
		nvram_set("wsc_config_command", "1");
		sleep(10);
		nvram_set("wps_pre_restart", "5");
		nvram_commit();
		kill(1, SIGHUP);
	}
	else if(strcmp(status, "pin")==0 ) {
		query_vars("wsc_sta_pin", str, sizeof(str));
		str[8]='\0';
		cprintf("PIN code From STA:%s\n", str);
		if( wscPincheck(str) ) {
			rPage = "error.asp";
			setenv("html_response_return_page",
					"wps.asp", 1);
			setenv("html_response_error_message",
					"Invalid PIN number.", 1);
			setenv("result_timer", "0", 1);
			return rPage;
		} else {
			nvram_set("wsc_sta_pin", str);
			nvram_set("wsc_sta_pin_tmp", str);
			nvram_set("wsc_mode", "enabled");
			nvram_set("wl0_wsc_mode", "enabled");
			nvram_set("wsc_method", "1");
			nvram_set("wsc_config_state", "1");
			nvram_set("wsc_addER", "0");
			nvram_set("wl_wsc_reg", "enabled");
			nvram_set("wl0_wsc_reg", "enabled");
			nvram_set("wsc_restart", "yes");
			nvram_commit();
			if(strcmp(nvram_safe_get("wsc_config_command"), "0")==0 && nFlag==0) {
				nvram_set("wsc_config_command", "1");
			} else {
				nvram_set("wps_pre_restart", "7");
				nvram_commit();
				sleep(3);
				kill(1, SIGHUP);
			}
		}
	}
	else if(strcmp(status, "pcb")==0 ) {
		nvram_set("wsc_mode", "enabled");
		nvram_set("wl0_wsc_mode", "enabled");
		nvram_set("wsc_method", "2");
		nvram_set("wsc_config_state", "1");
		nvram_set("wsc_addER", "0");
		nvram_set("wl_wsc_reg", "enabled");
		nvram_set("wl0_wsc_reg", "disabled");
		nvram_set("wsc_restart", "yes");
		nvram_commit();
		if(strcmp(nvram_safe_get("wsc_config_command"), "0")==0 && nFlag==0){
			nvram_set("wsc_config_command", "1");
		} else {
			nvram_set("wps_pre_restart", "6");
			nvram_commit();
			sleep(3);
			kill(1, SIGHUP);
		}

	}
	else if(strcmp(status, "reset")==0 ) {
		cprintf("\n***Reset***\n");
		pincode = nvram_safe_get("wsc_default_pin");
		nvram_set("wsc_timeout_enable", "0");
		/* nvram_set("wsc_device_pin", "12345670"); */
		nvram_set("wsc_device_pin", pincode);
		nvram_set("wsc_proc_status", "0");
		nvram_set("wsc_status", "0");
		nvram_set("wsc_mode", "enabled");
		nvram_set("wl0_wsc_mode", "enabled");
		nvram_set("wsc_method", "1");
		nvram_set("wsc_config_state", "0");
		nvram_set("wsc_addER", "1");
		nvram_set("wl_wsc_reg", "enabled");
		nvram_set("wl0_wsc_reg", "enabled");
		nvram_set("wsc_restart", "yes");

		nvram_set("wl0_radio", "1");
		nvram_set("wl0_ssid", "dlink");
		nvram_set("wl0_channel", "6");
		nvram_set("wlan0_auto_channel_enable", "0");
		nvram_set("wl0_gmode", "1");
		nvram_set("wl0_closed", "0");
		nvram_set("wl0_wep", "disabled");
		nvram_set("wl0_auth", "");
		nvram_set("wep_key_len", "5");
		nvram_set("wlan0_wep_display", "hex");
		nvram_set("wl0_key", "1");
		nvram_set("wl0_key1", "");
		nvram_set("wl0_key2", "");
		nvram_set("wl0_key3", "");
		nvram_set("wl0_key4", "");
		nvram_set("wlan0_wep64_key_1", "");
		nvram_set("wlan0_wep64_key_2", "");
		nvram_set("wlan0_wep64_key_3", "");
		nvram_set("wlan0_wep64_key_4", "");
		nvram_set("wlan0_wep128_key_1", "");
		nvram_set("wlan0_wep128_key_2", "");
		nvram_set("wlan0_wep128_key_3", "");
		nvram_set("wlan0_wep128_key_4", "");
		nvram_set("wl0_akm", "");
		nvram_set("wl0_crypto", "tkip");
		nvram_set("wl0_radius_ipaddr", "");
		nvram_set("wl0_radius_port", "1812");
		nvram_set("wl0_radius_key", "");
		nvram_set("wl0_wpa_psk", "");
		/* nvram_commit(); */
		nvram_set("wps_pre_restart", "5");
		nvram_commit();
		sleep(3);
		kill(1, SIGHUP);
	}
	else if( strcmp(status, "unconfigured")==0 ) {
		cprintf("\n***Unconfigured***\n");
		nvram_set("wsc_timeout_enable", "0");
		nvram_set("wsc_addER", "0");
		nvram_set("wl0_wsc_reg", "disabled");
		nvram_set("wsc_restart", "yes");
		nvram_set("wsc_config_state", "0");
		nvram_set("wps_pre_restart", "5");
		nvram_commit();
		sleep(3);
		kill(1, SIGHUP);
	}
	else {
		cprintf("\n***RC Restart***\n");
		if (fork()) {
			nvram_commit();
			sleep(3);
			system("killall wsccmd");
			system("rmmod wl");
			__do_reboot(NULL);
			exit(0);
		}
	}
	return get_response_page();
}

void *do_wireless_cgi(struct ssc_s *obj)
{
	struct items *it;
	char wep_en[32], wep_type[32], key_len[32], wps_en[32];
	char *s;

	/*
 * 	 * XXX:if name can present type, why we need a type field?
 * 	 	 */
	static struct {
		char *name;
	} *p, key[] = {
		{"wlan0_wep64_key_1"},
		{"wlan0_wep64_key_2"},
		{"wlan0_wep64_key_3"},
		{"wlan0_wep64_key_4"},
		{"wlan0_wep128_key_1"},
		{"wlan0_wep128_key_2"},
		{"wlan0_wep128_key_3"},
		{"wlan0_wep128_key_4"},
		{ NULL}
	};

	query_vars("wl0_wep", wep_en, sizeof(wep_en));
	query_vars("wlan0_wep_display", wep_type, sizeof(wep_type));
	query_vars("wep_key_len", key_len, sizeof(key_len));

	for (it = obj->data; it != NULL && it->key != NULL; it++) {
		int i;
		char hex_value[32], temp[3];
		hex_value[0]='\0';

		s = getenv(it->key);
		if (s == NULL || s[0] == '\0') {
			if (it->_default == NULL)
				continue;
			s = it->_default;
			STDOUT("Update default %s: %s\n",
					it->key, s);
		} else if (strcmp(wep_en, "disabled") != 0 &&
				strcmp(wep_type, "ascii") == 0) {
			for (p = key; p->name != NULL; p++) {
				if (strcmp(it->key, p->name) == 0){
					if ((strncmp(it->key, "wlan0_wep64", 11) == 0)
							&& (strcmp(key_len, "5") != 0))
						break;
					if ((strncmp(it->key, "wlan0_wep128", 12) == 0)
							&& (strcmp(key_len, "13") != 0))
						break;

					hex_value[0] = '\0';
					for ( i = 0; i < strlen(s); i++) {
						sprintf(temp, "%x", s[i]);
						strcat(hex_value, temp);
					}
					s = hex_value;
					break;
				}
			}
		}
		update_record(it->key, s);
	}

	query_vars("wl0_wsc_mode", wps_en, sizeof(wps_en));
	if ( strcmp(wps_en, "disabled") == 0)
		update_record("wps_pre_restart", "0");
	if ( strcmp(wps_en, "enabled") == 0 ) {
		do_wps_action();
	} else {
		if (fork()) {
			nvram_commit();
			system("killall wsccmd");
			system("rmmod wl");
			sleep(1);
			__do_reboot(obj);
			exit(0);
		}
	}
	return get_response_page();
}
