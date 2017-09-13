#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#include "libdb.h"
#include "ssc.h"
#include "public.h"
#include "querys.h"
#include "netinf.h"

struct __firmware_check_struct {
        const char *param;
        const char *desc;
        int (*fn)(int, char *[]);
};

char* online_firmware_check(char *iface, char *fw_url, char *result_file)
{
        FILE *fp;
        char result[1024]={0};
	char cmd[128];
	sprintf(cmd,"fwqd -i %s -u %s -r %s", iface, fw_url, result_file);
	system(cmd);
        if((fp = fopen(result_file,"r+"))==NULL){
                cprintf("online_firmware_check");
                return "ERROR";
        }
        if(!fgets(result, sizeof(result), fp)){
                cprintf("fw_check: fgets error\n");
                return "ERROR";
        }
        fclose(fp);
        return result;
}

int return_online_firmware_check()
{

        char result[1024];
        char *check_fw_url = NULL;
        char *wan_eth = nvram_safe_get("wan_eth");

        check_fw_url = nvram_safe_get("check_fw_url");
        wan_eth = nvram_safe_get("wan_eth");

        sprintf(result, "%s",online_firmware_check(wan_eth, check_fw_url, "/var/tmp/fw_check_result.txt"));
	printf("%s", result);
}

int return_online_lp_check()
{
	char result[1024]={'\0'};
	char check_lp_url[1024];
	char *wan_eth = nvram_safe_get("wan_eth");

#if 0
	/* In order to allow user to download local language pack from the UI,
 	   when user remove local language pack.
 	*/
	char lang_region[10];
	struct {
		const char *region;
		const char *lang;
	} *p, lang[] = {
		{ "EN", "EN"},
		{ "NB", "JP"},
                { NULL, NULL}
	};
	
	memset(result,"\0",sizeof(result));
	memset(check_lp_url,"\0",sizeof(check_lp_url));

	if(NVRAM_MATCH("lang_region","0")) {

		for ( p = lang; p->region; p++) {
			if (strcasecmp(nvram_safe_get("sys_region"), p->region) == 0)
                        	break;
        	}
		
		sprintf(lang_region,"%s",p->lang);

	} else {

		sprintf(lang_region,"%s",nvram_safe_get("lang_region"));
	}

	sprintf(check_lp_url,"wrpd.dlink.com.tw,/router/firmware/query.asp?model=%s_%s_%sLP",
					nvram_safe_get("model_number"),nvram_safe_get("sys_hw_version"),lang_region);
#endif	
	sprintf(check_lp_url,"wrpd.dlink.com.tw,/router/firmware/query.asp?model=%s_%s_%sLP",
					nvram_safe_get("model_number"),nvram_safe_get("sys_hw_version"),nvram_safe_get("lang_region"));
	sprintf(result, "%s",online_firmware_check(wan_eth, check_lp_url, "/var/tmp/fw_check_result.txt"));
	printf("%s", result);

}

static int misc_firmware_check_help(struct __firmware_check_struct *p)
{
        cprintf("Usage:\n");

        for (; p && p->param; p++)
                cprintf("\t%s: %s\n", p->param, p->desc);

        cprintf("\n");
        return 0;
}

static int misc_download_fw_lp_help(void)
{
        cprintf("Usage:\n");

	cprintf("\tdownload_fw_lp file_link file_name update_type\n");
	cprintf("Example:\n");
	cprintf("\tdownload_fw_lp http://wrpd.dlink.com.tw/router/firmware/DIR-652/AX/NB/DIR652A1_FW100NBB15.bin DIR652A1_FW100NBB15.bin fw\n");
	cprintf("\tdownload_fw_lp http://wrpd.dlink.com.tw/router/firmware/DIR-652/AX/JPLP/DIR652A1_JPLP100NBB14.jplp DIR652A1_JPLP100NBB14.jplp lp\n");

        cprintf("\n");
        return 0;
}


int online_firmware_check_main(int argc, char *argv[])
{
	int ret = -1;
        struct __firmware_check_struct *p, list[] = {
                { "check_fw", "check firmware version", return_online_firmware_check},
                { "check_lp", "check Language version", return_online_lp_check},
                { NULL, NULL, NULL }
        };

	if (argc == 1 || strcmp(argv[1], "help") == 0)
                	goto help;

        for (p = list; p && p->param; p++) {
                if (strcmp(argv[1], p->param) == 0) {
                        ret = p->fn(argc - 1, argv + 1);
                        goto out;
                }
        }
help:
        ret = misc_firmware_check_help(list);
out:
        return ret;

}
// Matt add - 2010/11/08
int download_fw_lp(int argc, char *argv[])
{
	int ret = -1;
	char cmds[256];
	char file[128];
	FILE *fp;
	int retry = 3;

	if (argc == 1 || strcmp(argv[1], "help") == 0)
                	goto help;

	// downloag update file
	system("echo \"\{status : \\\"0\\\"\}\" > /www/auto_update_st.js"); //status = 0 : downloading...
	//sleep(5);
	memset(cmds, '\0', sizeof(cmds));
	sprintf(cmds, "wget -P /tmp/ %s", argv[1]);
	//system(cmds);

	memset(file, '\0', sizeof(file));
	sprintf(file, "/tmp/%s", argv[2]);
	
	for(;retry > 0;) {
		system(cmds);
		fp = fopen(file, "r");
		if(fp == NULL) {
			retry--;
			continue;
		}
		fclose(fp);
		break;
	}
	if(retry == 0) {
		system("echo \"\{status : \\\"-2\\\"\}\" > /www/auto_update_st.js"); //status = -2 : download fail...
		goto out;

	}
	
	if(!strcmp("fw", argv[3]))
		system("echo \"\{status : \\\"1\\\"\}\" > /www/auto_update_st.js"); //status = 1 : download success, and start update firmware.

	else if(!strcmp("lp", argv[3]))
		system("echo \"\{status : \\\"2\\\"\}\" > /www/auto_update_st.js"); //status = 2 : download success, and start update language pack.

	else
		system("echo \"\{status : \\\"-3\\\"\}\" > /www/auto_update_st.js"); //status = -3 : update type error.


	//sleep(10);
	
	goto out;
	
help:
        ret = misc_download_fw_lp_help();
out:
        return ret;

}
/*-------------------------------*/
