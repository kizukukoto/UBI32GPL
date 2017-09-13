#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include "libdb.h"
#include "debug.h"
#include "hnap.h"
#include "shvar.h"

extern int do_xml_response(const char *);
extern int do_xml_mapping(const char *, hnap_entry_s *);

/* Return 0: idle, -1: busy */
static int rc_status()
{
	char st;
	int ret = -1;
	sleep(2); /* Wait rc ready for IsDeviceReady function*/

	FILE *fp = fopen(HNAP_RC_FLAG, "r");

	if (fp == NULL)
		goto out;
	fscanf(fp, "%c", &st);
	fclose(fp);

	if (st == 'b')
		goto out;
	ret = 0;
out:
	return ret;
}

int rc_restart()	/* copy from main/ext/rcctrl.c */
{
#if NOMMU
	pid_t pid;
	if ((pid = vfork()) == 0)
		execlp("/bin/sh", "/bin/sh", "-c", "(sleep 3; killall -SIGHUP rc) &", NULL);
	else if (pid >0)
		return 0;
	else
		cprintf("XXX %s[%d] vfotk error XXX",__func__,__LINE__);
#else
#ifdef LINUX_NVRAM
	return kill(read_pid(RC_PID), SIGHUP);
#else
	return kill(1, SIGTERM);
#endif
#endif
}


int do_unknown_call(const char *key, char *raw)
{
	char xml_resraw[8192];

	bzero(xml_resraw, sizeof(xml_resraw));

	do_xml_create_mime(xml_resraw);
	strcat(xml_resraw, unknown_call);

	return do_xml_response(xml_resraw);
}

int do_getdevice_settings(const char *key, char *raw)
{
	char xml_resraw[8192];
	char xml_resraw2[8192];

	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	sprintf(xml_resraw2, get_device_settings_result,
		nvram_safe_get("pure_type"),
		nvram_safe_get("pure_device_name"),
		nvram_safe_get("pure_vendor_name"),
		nvram_safe_get("pure_model_description"),
		nvram_safe_get("hostname"),
		"1.01",
		nvram_safe_get("pure_presentation_url"),
#ifdef AUTHGRAPH
		atoi(nvram_safe_get("graph_auth_enable")) == 0? "false": "true",
#endif
		nvram_safe_get("pure_wireless_url"),
		nvram_safe_get("pure_block_url"),
		nvram_safe_get("pure_parental_url"),
		nvram_safe_get("manufacturer"),
		nvram_safe_get("pure_support_url")
	);
	strcat(xml_resraw, xml_resraw2);

	return do_xml_response(xml_resraw);
}

int do_isdevice_ready(const char *key, char *raw)
{
	char xml_resraw[1024], xml_resraw2[1024];

	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);

	sprintf(xml_resraw2, is_device_ready_result,
		(rc_status() == 0)? "OK": "ERROR");
	strcat(xml_resraw, xml_resraw2);

	return do_xml_response(xml_resraw);
}

int do_reboot(const char *key, char *raw)
{
	char xml_resraw[1024];

	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);
	strcat(xml_resraw, reboot_result);

	do_xml_response(xml_resraw);
	return rc_restart();
}

static void replace_special_char(char *str)
{
	char replace_str[64];
	int i, j, len, find, count;
	
	len = strlen(str);
	memset(replace_str, 0, 64);
	find = 0;
	count = 0;
	cprintf("len=%d \n",len);
	for (i = 0, j = 0; i < len; i++, j++){
		count++;
		if (str[i] != '&'){
			replace_str[j] = str[i];
		
		}else{
			if (str[i+1] == 'a' && str[i+2] == 'm' && str[i+3] == 'p' 
				&& str[i+4] == ';'){    // replace '&'
				replace_str[j] = str[i];
				i += 4;
				find = 1;
				cprintf("&");
			}else if (str[i+1] == 'q' && str[i+2] == 'u' && str[i+3] == 'o'
				&& str[i+4] == 't' && str[i+5] == ';'){ // replace '"'
				replace_str[j] = '"';
				i += 5;
				find = 1;
			}else if (str[i+1] == 'a' && str[i+2] == 'p' && str[i+3] == 'o' 
				&& str[i+4] == 's' && str[i+5] == ';'){ // replace '''
				strcpy(&replace_str[j], "'");
				i += 5;
				find = 1;
			}else if (str[i+1] == 'l' && str[i+2] == 't' && str[i+3] == ';'){       // replace '<'
				replace_str[j] = '<';
				i += 3;
				find = 1;
                                cprintf("<");
			}else if (str[i+1] == 'g' && str[i+2] == 't' && str[i+3] == ';'){       // replace '>'
				replace_str[j] = '>';
				i += 3;
				find = 1;
				cprintf(">");
			}
		}
	}
	cprintf("count:%d \n",count);
	replace_str[count]='\0';
	if (find){
		memset(str, 0, len);
		memcpy(str, replace_str, count+1);
	}
}


static int do_set_nvram_key(const char *key, char *ct)
{
	//cprintf("XXX %s(%d) '%s=%s'\n", __FUNCTION__, __LINE__, key, ct);
	if(strcmp(key,"admin_password") == 0)
		replace_special_char(ct);
	nvram_set(key, ct);
	return 0;
}

int do_setdevice_settings(const char *key, char *raw)
{
	int ret;
	char xml_resraw[1024];
	hnap_entry_s ls[] = {
		{ "DeviceName", "pure_device_name", do_set_nvram_key },
		{ "AdminPassword", "admin_password", do_set_nvram_key },
		{ NULL, NULL }
	};

	ret=do_xml_mapping(raw, ls);
	
	bzero(xml_resraw, sizeof(xml_resraw));
	do_xml_create_mime(xml_resraw);
        strcat(xml_resraw, setdevice_settings_result);
	return do_xml_response(xml_resraw);	
}
