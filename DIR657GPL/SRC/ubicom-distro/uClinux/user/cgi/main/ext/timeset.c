#include <stdlib.h>
#include <unistd.h>
#include "ssc.h"
#include "public.h"

#ifdef LINUX_NVRAM
#include "rcctrl.h"


void *do_timeset(struct ssc_s *obj)
{
	char *value=NULL;
	char cmd[128]={0};
	int year = 2006,month = 4, date = 12, hour = 12, min = 24, sec = 12;
	__post2nvram(obj);
	nvram_commit();
	value=nvram_safe_get("system_time");
	sscanf(value,"%d/%d/%d/%d/%d/%d",&year,&month,&date,&hour,&min,&sec);	
	sprintf(cmd,"date -s %02d%02d%02d%02d%04d",month,date,hour,min,year);
	system(cmd);
	rc_restart();
	return get_response_page();
}
#else

void *do_timeset(struct ssc_s *obj)
{
        __post2nvram(obj);
        nvram_commit();
        system("/sbin/settime");
        system("/sbin/setdaylight");
        sleep(3);

        __do_reboot(obj);
        return get_response_page();
#if 0
        char *ntp;

        if ((ntp = get_env_value("ntp_client_enable")) == NULL)
                goto out;       // FIXME: error

        if (*ntp == '1') {
                __post2nvram(obj);
                save_db();
                do_reboot(obj);
        } else {
                __post2nvram(obj);
                /* TODO: mplement evel API... */
                system("cli sys set_time");
                do_apply_cgi(obj);

        }
out:
        return get_response_page();
#endif

}
#endif //LINUX_NVRAM
