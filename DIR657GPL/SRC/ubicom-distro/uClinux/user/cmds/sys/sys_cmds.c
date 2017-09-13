#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>
#include "cmds.h"
#include "meminfo.c"
#include "uptime.c"
#include "do_dns.c"
#include "cpuinfo.c"

#if 1
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

extern int dhcprelay_info_main(int argc, char *argv[]);
extern int restore_main(int argc, char *argv[]);
extern int meminfo_main(int argc, char *argv[]);
extern int uptime_main(int argc, char *argv[]);
extern int my_dns_main(int argc, char *argv[]);
extern int cpuinfo_main(int argc, char *argv[]);
extern int init_nvram_main(int argc, char *argv[]);
extern int param_main(int argc, char *argv[]);
extern int setmac_main(int argc, char *argv[]);
extern int getmac_main(int argc, char *argv[]);
extern int flash_write_main(int argc, char *argv[]);

static int time_main(argc, argv)
{
	printf("time_main\n");
	return 0;
}

#if 1//def KERNEL_2_6_15
#define GPIO_DRIVER_PATH	"/lib/modules/2.6.15/net/gpio_mod.ko"
#define WPS_LED_GPIO_DRIVER_PATH "/lib/modules/2.6.15/net/ar531xgpio.o"	
#else
#define GPIO_DRIVER_PATH	"/lib/modules/2.4.25-LSDK-5.2.0.47/misc/ar531x-gpio.o"
#define WPS_LED_GPIO_DRIVER_PATH "/lib/modules/2.4.25-LSDK-5.2.0.47/misc/ar531xgpio.o"	
#endif

static int sys_gpio_main(int argc, char *argv[])
{
	char cmd[40];
	
	DEBUG_MSG("gpio_main:\n");
	
#if 1//def KERNEL_2_6_15
	DEBUG_MSG("gpio_main:KERNEL_2_6_15\n");
	sprintf(cmd,"insmod %s",GPIO_DRIVER_PATH);
	DEBUG_MSG("gpio_main:cmd=%s\n",cmd);
	system(cmd);		
	//sprintf(cmd,"insmod %s",WPS_LED_GPIO_DRIVER_PATH);
	//DEBUG_MSG("gpio_main:cmd=%s\n",cmd);
	//system(cmd);	
#else
	DEBUG_MSG("gpio_main:not KERNEL_2_6_15\n");
	sprintf(cmd,"modprobe %s",GPIO_DRIVER_PATH);
	DEBUG_MSG("gpio_main:cmd=%s\n",cmd);
	system(cmd);		
	//sprintf(cmd,"modprobe %s",WPS_LED_GPIO_DRIVER_PATH);
	//DEBUG_MSG("gpio_main:cmd=%s\n",cmd);
	//system(cmd);		
#endif	
	system("/sbin/gpio SYSTEM check &");
//Albert add 2008/11/06	
	system("/sbin/gpio WPS_BUTTON check &");
	return 0;
}

static int sys_watchdog_main(int argc, char *argv[])
{
	char cmd[40];
	
	DEBUG_MSG("sys_watchdog_main:\n");
	system("watchdog &");
	return 0;
}

/*
static int version_main(int argc, char *argv[])
{
	__show_version();
}
*/
extern int reg_read_main(__u32 phyaddr, __u32 len, __u32 *out);
extern int set_timezone_main(int argc, char *argv[]);
extern int manual_time_main(int argc, char *argv[]);
extern int dump_default_nvram(int argc, char *argv[]);
extern int sysinfo_main(int argc, char *argv[]);
extern int account_main(int argc, char *argv[]);
extern int init_main(int argc, char *argv[]);
extern int vif_main(int argc, char *argv[]);

int sys_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
/*
		{ "restore", restore_main},
		{ "init_nvram", init_nvram_main},
		{ "time", time_main},
		{ "cpuinfo", cpuinfo_main},
		{ "meminfo", meminfo_main},
		{ "uptime", uptime_main},
		{ "dns", my_dns_main},
		{ "sys_gpio", sys_gpio_main},
		{ "watch_dog", sys_watchdog_main},
		{ "setmac", setmac_main},
		{ "getmac", getmac_main},
*/
		{ "account", account_main, "create httpd login account"},
		{ "flash_write", flash_write_main, "erase and write a file to flash"},
		{ "info", sysinfo_main, "manipuate system info"},
		{ "init", init_main, "system init"},
		{ "param", param_main, "save/resotre config"},
		{ "dhcprelay", dhcprelay_info_main, "record dhcprelay info"},
		{ "vif", vif_main, "virtual interface control" },
/*
		{ "reg_read", reg_read_main},
		{ "set_tmzone", set_timezone_main, "setup system time zone"},
		{ "set_time", manual_time_main, "setup system time and zone manually."},
		{ "version", version_main, "display firmware version infomation."},
		{ "dump_defaults", dump_default_nvram, "dump default build-in nvram keys"},
*/
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
