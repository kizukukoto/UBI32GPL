#include <stdio.h>
#include <stdlib.h>

#include "querys.h"
#include "ssc.h"
#include "nvram.h"
#include "debug.h"

#if ATH
char do_sta_enrollee_cgi(struct ssc_s *obj)
{
	int ath_count=0;
	char cmd[128]={0};
	int pid, status;
	char *wlan0_enable=nvram_safe_get("wlan0_enable");
	char *wlan1_enable=nvram_safe_get("wlan1_enable");
	char *enrollee_pin = get_env_value("wps_sta_enrollee_pin");

	if (enrollee_pin == NULL)
		enrollee_pin = nvram_safe_get("wps_default_pin");

	if (strcmp(wlan0_enable, "1") == 0) {
		sprintf(cmd,"wpatalk ath%d \'configthem pin=%s\'"
		" 2>&1 > /var/tmp/WSC_ath%d_wpatalk_log &", 
		ath_count, enrollee_pin, ath_count);
		ath_count++;
		system(cmd);
	}

	if (strcmp(wlan1_enable, "1") == 0) {
		sprintf(cmd,"wpatalk ath%d \'configthem pin=%s\'"
		" 2>&1 > /var/tmp/WSC_ath%d_wpatalk_log &",
		ath_count, enrollee_pin, ath_count);
		system(cmd);
	}

	return get_response_page();
}

char * do_sta_pbc_cgi(struct ssc_s *obj)
{
	int pid,status=0;
	pid = vfork();
	switch(pid) {
		case -1:
			cprintf("virtual_push_button_cgi fork error!!\n");
			exit(0);
		case 0:
			system("wpatalk ath0 configthem > /dev/null 2>&1 &");
			_exit(0);
	}
        return get_response_page();
}
#elif MVL
char *do_sta_enrollee_cgi(struct ssc_s *obj)
{
	char cmd[128];
	char *enrollee_pin = get_env_value("wps_sta_enrollee_pin");

	if (enrollee_pin == NULL)
		enrollee_pin = nvram_safe_get("wps_default_pin");

	sprintf(cmd,"wsc_cfg_pin %s", enrollee_pin);
	cprintf("set_sta_enrollee_pin=%s\n", enrollee_pin);
	system(cmd);

	return get_response_page();
}

char do_sta_pbc_cgi(struct ssc_s *obj)
{
	system("wlutils wps pbc &");
	return get_response_page();
}

#else
char *do_sta_enrollee_cgi(struct ssc_s *obj){

        char *wsc_pin = get_env_value("wps_sta_enrollee_pin");
        char buffer[200];

        sprintf(buffer, "cli net wl trigger_wsc wsc_pin %s &", wsc_pin);
        system(buffer);
        return get_response_page();
}

char * do_sta_pbc_cgi(struct ssc_s *obj){

        char buffer[200];

        sprintf(buffer, "cli net wl trigger_wsc pbc &");
        system(buffer);
        fprintf(stderr, "%s", buffer);

        return get_response_page();
}
#endif
