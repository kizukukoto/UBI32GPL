#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ssi.h"
#include "libdb.h"
#include "debug.h"
#include "nvram.h"
#include "linux_vct.h"

struct __tc_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

extern char *get_ipaddr(char *);

static int misc_tc_help(struct __tc_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

#if CONFIG_USER_STREAMENGINE
static int check_wan_type()
{
	if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 ))
		return 1;
	else
		return 0;
}

static void measure_uplink_bandwidth(char *bandwidth)
{
        FILE *fp;
        char returnVal[32] = {0};

        if ((fp = fopen("/sys/devices/system/ubicom_streamengine/ubicom_streamengine0/ubicom_streamengine_calculated_rate","r")) != NULL) {
                fread(returnVal, sizeof(returnVal), 1, fp);
                if (strlen(returnVal) > 1) {
                        unsigned long int kbps;
                        sscanf(returnVal, "%lu", &kbps);
                        kbps /= 1000;           /* bps to kbps */
                        sprintf(bandwidth, "%lu", kbps);
                } else
                        strcpy(bandwidth, "0");
                fclose(fp);
        } else
                strcpy(bandwidth, "0");
}

static int not_measure_yet()
{
        FILE *fp = NULL;

        char rate[32] = {0};
        char code[32] = {0};

        if((fp = fopen("/sys/devices/system/ubicom_streamengine/ubicom_streamengine0/ubicom_streamengine_start_estimation","r")) == NULL) 
                return 0;

	measure_uplink_bandwidth(rate);
        if (atoi(rate) != 0) {
                fclose(fp);
                return 0;       /* Measured */
        }
        /*
         * No measured bandwidth but if ubicom_streamengine_start_estimation reports error then we have tried and failed.
         */
        fread(code, sizeof(code), 1, fp);
        fclose(fp);
        if (atoi(code) < 0) {
                return 0;               /* We tried and failed so we have attempted a measurement */
        }

        /*
         * No measured rate and not attempted before
         */
        return 1;
}

static int measured_uplink_speed(int argc, char *argv[])
{
        char bandwidth[32];
        FILE *fp;
	memset(bandwidth, '\0', sizeof(bandwidth));
        measure_uplink_bandwidth(bandwidth);
        if (!strcmp(bandwidth, "0")) {
                if ((fp = fopen("/var/tmp/ubicom_streamengine_calculated_rate.tmp","r")) != NULL) {
                        fclose(fp);
                        printf("Your broadband internet connection has surpassed<br>&nbsp; the uplink measurement requirement.");
                }
                else
			printf("Not Estimated");
        }
        else
                printf("%s kbps", bandwidth);
	return 0;
}
#else

static int check_wan_type()
{
        if (( nvram_match("wan_proto", "dhcpc") ==0 ) || ( nvram_match("wan_proto", "static") ==0 ) || ( nvram_match("wan_proto", "usb3g_phone") ==0 ))
		return 1;
	else
		return 0;
}

static void measure_uplink_bandwidth(char *bandwidth)
{
        FILE *fp;
        char returnVal[32] = {0};

        if ((fp = fopen("/var/tmp/bandwidth_result.txt","r")) != NULL) {
                fread(returnVal, sizeof(returnVal), 1, fp);
                if(strlen(returnVal) > 1)
                        strcpy(bandwidth, returnVal);
                else
                        strcpy(bandwidth, "0");
                fclose(fp);
        } else
                strcpy(bandwidth, "0");
        return ;
}

static int not_measure_yet()
{
        FILE *fp = NULL;
        if ((fp = fopen("/var/tmp/bandwidth_result.txt","r")) == NULL)
                return 1;
        fclose(fp);
        return 0;
}

static int measured_uplink_speed(int argc, char *argv[])
{
        char bandwidth[32];
	memset(bandwidth, '\0', sizeof(bandwidth));
        measure_uplink_bandwidth(bandwidth);
        if (!strcmp(bandwidth, "0")) 
			printf("Not Estimated");
        else
                printf("%s kbps", bandwidth);
	return 0;
}
#endif

static int wan_ip_is_obtained(void)
{
        char *tmp_wan_ip1=NULL, *tmp_wan_ip2=NULL;
        char wan_ipaddr[2][16]={0};
        char status[16];
        char *wan_proto=NULL;

        wan_proto = nvram_safe_get("wan_proto");

        /* Check phy connect statue */
        VCTGetPortConnectState(nvram_safe_get("wan_eth"), VCTWANPORT0, status);
        if (!strncmp("disconnect", status, 10))
                return 0;
        if (check_wan_type())
        {
                tmp_wan_ip1 = get_ipaddr( nvram_safe_get("wan_eth") );
                if(tmp_wan_ip1)
                        strncpy(wan_ipaddr[0],tmp_wan_ip1,strlen(tmp_wan_ip1));
        } else {
                tmp_wan_ip1 = get_ipaddr( "ppp0" );
                if(tmp_wan_ip1)
                        strncpy(wan_ipaddr[0],tmp_wan_ip1,strlen(tmp_wan_ip1));

                /* if Russia is enabled */
                if ((( nvram_match("wan_proto", "pppoe") ==0 ) && ( nvram_match("wan_pppoe_russia_enable", "1") ==0 ))
                ||(( nvram_match("wan_proto", "pptp") ==0 ) && ( nvram_match("wan_pptp_russia_enable", "1") ==0 ))
                ||(( nvram_match("wan_proto", "l2tp") ==0 ) && ( nvram_match("wan_l2tp_russia_enable", "1") ==0 )))
                {
                        tmp_wan_ip2 = get_ipaddr( nvram_safe_get("wan_eth") );
                        if(tmp_wan_ip2)
                                strncpy(wan_ipaddr[1],tmp_wan_ip2,strlen(tmp_wan_ip2));
                }
        }

        if (!(strlen(wan_ipaddr[0])) && !(strlen(wan_ipaddr[1]))) {
                cprintf("wan_ipaddr == NULL, can't measure bandwidth\n");
                return 0;
        } else
                return 1;
}


static int if_measure_now(void)
{
        char status[15] = {0};
        VCTGetPortConnectState(nvram_safe_get("wan_eth"), VCTWANPORT0,status);

        if((NVRAM_MATCH("traffic_shaping", "1") != 0 && 
	    NVRAM_MATCH("auto_uplink", "1") != 0) && 
	    not_measure_yet() && 
	   (strncmp("disconnect", status, 10) != 0))
                return 1;
        else
                return 0;

}

static int if_measuring_uplink_now(int argc, char *argv[])
{
	int  measure_now = 0;
        measure_now = if_measure_now();
        printf("%d", measure_now);
	return 0;
}

static int if_wan_ip_obtained(int argc, char *argv[])
{
	int  obtain = 0;
        obtain = wan_ip_is_obtained();
        printf("%d", obtain);

	return 0;
}

static int detected_line_type(void)
{
	FILE *fp;
        int type = -1;
        char returnVal[32] = {0};

        fp = fopen("/sys/devices/system/ubicom_streamengine/ubicom_streamengine0/ubicom_streamengine_frame_relay","r");
        if (fp) {
                fread(returnVal, sizeof(returnVal), 1, fp);
                cprintf("%s", returnVal);
                if(strlen(returnVal) >= 1) {
                        sscanf(returnVal, "%d", &type);
                }

                fclose(fp);
        }

        cprintf("%d", type);
        return type;

}

static int qos_detected_line_type(int argc, char *argv[])
{
	int type;
        type = detected_line_type();
        switch(type) {
        case 0:
                printf("Cable Or Other Broadband Network");
                return;
        case 1:
                printf("xDSL Or Other Frame Relay Network");
                return;
        }

        printf("Not detected");

	return 0;
}

int tc_main(int argc, char *argv[])
{
	int ret = -1;
	struct __tc_struct *p, list[] = {
		{ "measured_uplink_speed", "Measured Uplink Speed", measured_uplink_speed},
		{ "if_measuring_uplink_now", "If Measuring Uplink Now", if_measuring_uplink_now},
 		{ "if_wan_ip_obtained", "If Wan IP Obtained", if_wan_ip_obtained},
		{ "detected_line_type", "Qos detected line type", qos_detected_line_type},
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
	ret = misc_tc_help(list);
out:
	return ret;
}
