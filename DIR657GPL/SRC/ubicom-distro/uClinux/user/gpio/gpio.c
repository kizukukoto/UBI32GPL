/*
 * CAMEO command-line utility for Linux GPIO layer
 *
 * Copyright 2005, CAMEO Corporation
 * All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "gpio.h"
#include "bsp.h"

//#define GPIO_DEBUG 1
#ifdef GPIO_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

static	int gpio_fd = -1;

#define sys_reboot() kill(1, SIGTERM)

#define ioctl gpio_driver

unsigned int gpio_driver( unsigned int fpid_value, unsigned int gpio_value, unsigned long state_value)
{
	FILE *fp;

	char cmd[64];
	char c[10];

	//fpid_value is useless, we are talking directly to proc node.
//	printf( "%d %d %d Do nothing\n",gpio_value, state_value, sizeof(unsigned int));

	// This is for LED toggling

	if((state_value == GPIO_LED_ON)||(state_value == GPIO_LED_OFF))
	{
		switch ( gpio_value )
		{
			case STATUS_LED:
			case USB_LED:
			case POWER_LED:
			case INTERNET_LED_1:
			case INTERNET_LED_2:
#ifdef WIRELESS_LED
			case WIRELESS_LED:
#endif
#ifndef SPECIAL_WPS_LED
			case WPS_LED:
#endif
				sprintf(cmd,"echo %d > /sys/class/gpio/gpio%d/value", state_value, gpio_value);
				system(cmd);
				return 1;
				break;
		}
	}
	// If not LED, then check if it is button
	else if(state_value == GPIO_LED_STATUS)
	{
		switch ( gpio_value )
		{
			case USB_LED:
#ifdef WIRELESS_LED
			case WIRELESS_LED:
#endif
			case PUSH_BUTTON:
#ifdef IP8000 
			case SD_CARD_DETECT:
#endif
				sprintf(cmd,"/sys/class/gpio/gpio%d/value", gpio_value);
				break;
		}
		fp = fopen(cmd, "r");

		if(fp==NULL) {
			printf("Error: can't open GPIO.\n");
	    		/* fclose(file); DON'T PASS A NULL POINTER TO fclose !! */
	    		return 1;
		}


		while(fgets(c, 10, fp)!=NULL) {
      		/* keep looping until NULL pointer... */
//      		printf("String: %s", c);
	      	/* print the file one line at a time  */
		}
		fclose(fp);

		return atoi(c);
	}
}

static void usage(void)
{
	printf("usage: gpio [get name] [set name value] [name = STATUS_LED | WLAN_LED | USB_LED | SYSTEM] [value = on | off | status | blink] \n");
	exit(0);
}

void gpio_create_pidfile(char *fpid)
{
    FILE *pidfile;

    syslog(LOG_DEBUG, "gpio create pidfile %s", fpid);
    if ((pidfile = fopen(fpid, "w")) != NULL)
    {
		fprintf(pidfile, "%d\n", getpid());
		(void) fclose(pidfile);
    }
    else
		syslog(LOG_NOTICE,"Failed to create pid file %s\n", fpid);
}

void gpio_cleanup_pid(char *fpid)
{
	FILE *pfilein;
	char pidnumber[32];
	char cmd[64];

	pfilein = fopen(fpid, "r");
	if (pfilein)
	{
		if (fscanf(pfilein, "%s", pidnumber) == 1)
		{
			fclose(pfilein);
			if (atoi(pidnumber) != getpid())
			{
				sprintf(cmd,"kill -9 %s",pidnumber);
				system(cmd);
			}
		}
		else
			fclose(pfilein);
	}
}

/*	Date:	2009-03-31
*	Name:	jimmy huang
*	Reason:	Reduce the frquency we use strcmp()
*			Parse only once, then convert to int for later used
*	Note:	Add the function below
*			1.	parameters are the transfer from main()'s argv[],
*				argv[] has been left shift 1, which means
*				parameters[0] here are argv[1] in main()
*			2.	other parameters please refer to gpio.h
*/
static int parse_parameter(char **parameters,
			pin_gpio_name_s *gpio_name_t,
			pin_gpio_action_s *gpio_action_t,
			pin_gpio_parameter *pin_gpio_parameter_t){
	// gpio type
	if(parameters[0]){
		if(!strcmp("INTERNET_LED",parameters[0])){
			*gpio_name_t = GPIONAME_INTERNET_LED;
		}else if(!strcmp("POWER_LED",parameters[0])){
			*gpio_name_t = GPIONAME_POWER_LED;
		}else if(!strcmp("STATUS_LED",parameters[0])){
			*gpio_name_t = GPIONAME_STATUS_LED;
		}else if(!strcmp("USB_LED",parameters[0])){
			*gpio_name_t = GPIONAME_USB_LED;
#ifdef WIRELESS_LED
		}else if(!strcmp("WLAN_LED",parameters[0])){
			*gpio_name_t = GPIONAME_WLAN_LED;
#endif
		}else if(!strcmp("SYSTEM",parameters[0])){
			*gpio_name_t = GPIONAME_SYSTEM;
		}else if(!strcmp("SWITCH_CONTROL",parameters[0])){
			*gpio_name_t = GPIONAME_SWITCH_CONTROL;
		}else if(!strcmp("TEST",parameters[0])){
			*gpio_name_t = GPIONAME_TEST;
		}else if(!strcmp("WPS",parameters[0])){
			*gpio_name_t = GPIONAME_WPS;
		}else if(!strcmp("WPS_BUTTON",parameters[0])){
			*gpio_name_t = GPIONAME_WPS_BUTTON;
#ifdef IP8000 
		}else if(!strcmp("SD_CHECK",parameters[0])){
			*gpio_name_t = GPIONAME_SD_CHECK;
#endif
		}else{
			printf("gpio: ERROR_GPIO_TYPE:%s ###\n",parameters[0]);
			*gpio_name_t = GPIONAME_ERROR_TYPE;
			return GPIO_ERROR;
		}
	}else{
		printf("gpio: Error ! no gpio type has been specify.\n");
		*gpio_name_t = GPIONAME_ERROR_TYPE;
		return GPIO_ERROR;
	}

	// gpio status type
	if(parameters[1]){
		if(!strcmp("on",parameters[1])){
			*gpio_action_t = GPIOSTATE_PIN_LED_ON;
		}else if(!strcmp("off",parameters[1])){
			*gpio_action_t = GPIOSTATE_PIN_LED_OFF;
		}else if(!strcmp("blink",parameters[1])){
			*gpio_action_t = GPIOSTATE_PIN_LED_BLINK;
		}else if(!strcmp("s_blink",parameters[1])){
			*gpio_action_t = GPIOSTATE_PIN_LED_SBLINK;
		}else if(!strcmp("status",parameters[1])){
			*gpio_action_t = GPIOSTATE_PIN_STATUS;
		}else if(!strcmp("check",parameters[1])){
			*gpio_action_t = GPIOSTATE_PIN_CHECK;
		}else{
			printf("gpio: ERROR_GPIO_ACTION_TYPE:%s ###\n",parameters[1]);
			*gpio_action_t = GPIOSTATE_ERROR_GPIO_ACTION_TYPE;
			return GPIO_ERROR;
		}
	}else{
		printf("gpio: Error ! no gpio action has been specify.\n");
		*gpio_action_t = GPIOSTATE_ERROR_GPIO_ACTION_TYPE;
		return GPIO_ERROR;
	}

	// gpio other parameters, optional ?
	if(parameters[2]){
		if(!strcmp("amber",parameters[2])){
			*pin_gpio_parameter_t = GPIOPARAMETER_AMBER;
		}else if(!strcmp("off",parameters[2])){
			*pin_gpio_parameter_t = GPIOPARAMETER_AMBER;
		}else if(!strcmp("green",parameters[2])){
			*pin_gpio_parameter_t = GPIOPARAMETER_GREEN;
		}else{
			*pin_gpio_parameter_t = GPIOPARAMETER_ERROR_PARAMETER;
		}
	}else{
			*pin_gpio_parameter_t = GPIOPARAMETER_ERROR_PARAMETER;
	}
	//printf("[info]GPIO TYPE: %s, GPIO STATUS TYPE: %s\n",gpio_name_str,gpio_state_str);
	return GPIO_SUCCESS;
}

#if 1 //paser /proc/net/dev
#define _PATH_PROCNET_DEV               "/proc/net/dev"
static const char *const ss_fmt[] = {
	"%n%llu%u%u%u%u%n%n%n%llu%u%u%u%u%u",
	"%llu%llu%u%u%u%u%n%n%llu%llu%u%u%u%u%u",
	"%llu%llu%u%u%u%u%u%u%llu%llu%u%u%u%u%u%u"
};

struct user_net_device_stats {
	unsigned long rx_packets;	/* total packets received       */
	unsigned long tx_packets;	/* total packets transmitted    */
	unsigned long rx_bytes;	/* total bytes received         */
	unsigned long tx_bytes;	/* total bytes transmitted      */
	unsigned long rx_errors;	/* bad packets received         */
	unsigned long tx_errors;	/* packet transmit problems     */
	unsigned long rx_dropped;	/* no space in linux buffers    */
	unsigned long tx_dropped;	/* no space available in linux  */
	unsigned long rx_multicast;	/* multicast packets received   */
	unsigned long rx_compressed;
	unsigned long tx_compressed;
	unsigned long collisions;

	/* detailed rx_errors: */
	unsigned long rx_length_errors;
	unsigned long rx_over_errors;	/* receiver ring buff overflow  */
	unsigned long rx_crc_errors;	/* recved pkt with crc error    */
	unsigned long rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long rx_fifo_errors;	/* recv'r fifo overrun          */
	unsigned long rx_missed_errors;	/* receiver missed packet     */
	/* detailed tx_errors */
	unsigned long tx_aborted_errors;
	unsigned long tx_carrier_errors;
	unsigned long tx_fifo_errors;
	unsigned long tx_heartbeat_errors;
	unsigned long tx_window_errors;
};

//#define isspace(c) ((((c) == ' ') || (((unsigned int)((c) - 9)) <= (13 - 9))))
#define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#define isspace(c) (ISBLANK(c) || (c) == '\n' || (c) == '\r' \
		    || (c) == '\f' || (c) == '\v')

static void get_dev_fields(char *bp, struct user_net_device_stats *ife_stats, int procnetdev_vsn)
{
#if 1
	memset(ife_stats, 0, sizeof(struct user_net_device_stats));

	//printf("string(%d):%s\n",procnetdev_vsn,bp);

	sscanf(bp, "%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u",
		   &ife_stats->rx_bytes, /* missing for 0 */
		   &ife_stats->rx_packets,
		   &ife_stats->rx_errors,
		   &ife_stats->rx_dropped,
		   &ife_stats->rx_fifo_errors,
		   &ife_stats->rx_frame_errors,
		   &ife_stats->rx_compressed, /* missing for <= 1 */
		   &ife_stats->rx_multicast, /* missing for <= 1 */
		   &ife_stats->tx_bytes, /* missing for 0 */
		   &ife_stats->tx_packets,
		   &ife_stats->tx_errors,
		   &ife_stats->tx_dropped,
		   &ife_stats->tx_fifo_errors,
		   &ife_stats->collisions,
		   &ife_stats->tx_carrier_errors,
		   &ife_stats->tx_compressed /* missing for <= 1 */
		   );
#else
	sscanf(bp, ss_fmt[procnetdev_vsn],
		   &ife_stats->rx_bytes, /* missing for 0 */
		   &ife_stats->rx_packets,
		   &ife_stats->rx_errors,
		   &ife_stats->rx_dropped,
		   &ife_stats->rx_fifo_errors,
		   &ife_stats->rx_frame_errors,
		   &ife_stats->rx_compressed, /* missing for <= 1 */
		   &ife_stats->rx_multicast, /* missing for <= 1 */
		   &ife_stats->tx_bytes, /* missing for 0 */
		   &ife_stats->tx_packets,
		   &ife_stats->tx_errors,
		   &ife_stats->tx_dropped,
		   &ife_stats->tx_fifo_errors,
		   &ife_stats->collisions,
		   &ife_stats->tx_carrier_errors,
		   &ife_stats->tx_compressed /* missing for <= 1 */
		   );
#endif

	if (procnetdev_vsn <= 1) {
		if (procnetdev_vsn == 0) {
			ife_stats->rx_bytes = 0;
			ife_stats->tx_bytes = 0;
		}
		ife_stats->rx_multicast = 0;
		ife_stats->rx_compressed = 0;
		ife_stats->tx_compressed = 0;
	}
}

static int procnetdev_version(char *buf)
{
	if (strstr(buf, "compressed"))
		return 2;
	if (strstr(buf, "bytes"))
		return 1;
	return 0;
}

static char *get_name(char *name, char *p)
{
	/* Extract <name> from nul-terminated p where p matches
	   <name>: after leading whitespace.
	   If match is not made, set name empty and return unchanged p */
	int namestart = 0, nameend = 0;

	while (isspace(p[namestart]))
		namestart++;
	nameend = namestart;
	while (p[nameend] && p[nameend] != ':' && !isspace(p[nameend]))
		nameend++;
	if (p[nameend] == ':') {
		if ((nameend - namestart) < IFNAMSIZ) {
			memcpy(name, &p[namestart], nameend - namestart);
			name[nameend - namestart] = '\0';
			p = &p[nameend];
		} else {
			/* Interface name too large */
			name[0] = '\0';
		}
	} else {
		/* trailing ':' not found - return empty */
		name[0] = '\0';
	}
	return p + 1;
}

static int if_readlist_proc(char *target, struct user_net_device_stats *ife_stats)
{
	static int proc_read=0;

	FILE *fh;
	char buf[512];
	int err, procnetdev_vsn;

	if (proc_read)
		return 0;
	if (!target)
		proc_read = 1;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		return -1;
	}
	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);

	procnetdev_vsn = procnetdev_version(buf);

	err = 0;
	while (fgets(buf, sizeof buf, fh)) {
		char *s, name[128];

		s = get_name(name, buf);
		if (target && !strcmp(target, name))
		{
			get_dev_fields(s, ife_stats, procnetdev_vsn);
			break;
		}
	}
	if (ferror(fh)) {
		perror(_PATH_PROCNET_DEV);
		err = -1;
		proc_read = 0;
	}
	fclose(fh);
	return err;
}

#endif

int read_pid(char *file)
{
	FILE *pidfile;
	char pid[20];
	pidfile = fopen(file, "r");
	if(pidfile) {
		fgets(pid,20, pidfile);
		fclose(pidfile);
	} else
		return -1;
	return atoi(pid);
}

struct user_net_device_stats ife_w_stats;
unsigned long w_rx_packets=0;	/* previous total packets received       */
unsigned long w_tx_packets=0;	/* previous total packets transmitted    */

struct user_net_device_stats ife_w_stats_guest;
unsigned long w_rx_packets_guest=0;	/* previous total packets received       */
unsigned long w_tx_packets_guest=0;	/* previous total packets transmitted    */

/* GPIO utility */
int main(int argc, char* argv[])
{
	unsigned int ret;
	unsigned long status = 0;
	char *fpid;
	int count = 0;
	unsigned int reboot = 0;
	unsigned int restore = 0;
    char cmd[40];
	int i = 0;
	int status_led = 0;
	int wlan_led = 0;
	unsigned long gpio_pin = 0;
#ifdef SPECIAL_ONEINTERNET_LED
	unsigned long gpio_pin_other = 0;
#endif
	FILE *fp_internet;
	// jimmy added 20080723, for monitor phisical reset button
	int first_time = 0;
	int last_time = 0;
#ifdef IP8000 
	unsigned int sd_card_detect = 0;  // 0: no sd card, 1: sd card plugged, -1: can't detect dev path
	FILE *fp_sd;
	char cardreader_dev[5];
#endif
	memset(cmd,0,sizeof(cmd));

	if(argc < 3)
	{
		usage();
		DEBUG_MSG("gpio:argc fail!\n");
		return -1;
	}
#ifndef BSP_SETUP
	DEBUG_MSG("gpio:Not BSP_SETUP\n");
#else
	DEBUG_MSG("gpio:BSP_SETUP\n");
#endif
	/* Open GPIO Driver */
//	gpio_fd = open( GPIO_DRIVER, O_RDONLY);
//	if (gpio_fd < 0 )
//	{
//		printf( "ERROR : cannot open gpio driver\n");
//		return	-1;
//	}

	pin_gpio_name_s gpio_name;
	pin_gpio_action_s gpio_state;
	pin_gpio_parameter gpio_parameter;

	if(GPIO_ERROR == parse_parameter(argv+1,&gpio_name,&gpio_state,&gpio_parameter)){
		printf( "gpio: ERROR cannot parsing parameters\n");
		return -1;
	}

	switch(gpio_name){
		// STATUS_LED Control = MV_GPP12
		case GPIONAME_STATUS_LED:
			DEBUG_MSG("gpio:STATUS_LED\n");
			fpid = GPIO_SYSTEM;
			gpio_cleanup_pid(fpid);
			gpio_create_pidfile(fpid);
#ifdef SPECIAL_POWER_STATUS_LED
	ret = ioctl(gpio_fd, POWER_LED, GPIO_LED_OFF);
#endif
			switch(gpio_state){
				case GPIOSTATE_PIN_LED_ON:
					status = GPIO_LED_ON;
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = GPIO_LED_OFF;
					break;
				case GPIOSTATE_PIN_LED_BLINK:
					while(1) {
						if(status_led) {
							status = GPIO_LED_ON;
							status_led = 0;
						} else {
							status = GPIO_LED_OFF;
							status_led = 1;
						}
						ret = ioctl(gpio_fd,STATUS_LED, status);
						usleep(500000);
					}
					break;
				default:
					goto exit;
			}

			ret = ioctl(gpio_fd, STATUS_LED , status);
			break;

#ifdef WIRELESS_LED
		case GPIONAME_WLAN_LED:
			DEBUG_MSG("gpio:WLAN_LED\n");
			/* The GPIO_WLAN_PID must same as uClinux/user/sutil/shvar.h */
			#define GPIO_WLAN_PID  "/var/run/gpio_wlan.pid"
			gpio_cleanup_pid(GPIO_WLAN_PID);
			gpio_create_pidfile(GPIO_WLAN_PID);

			switch (gpio_state) {
				case GPIOSTATE_PIN_LED_ON:
					status = GPIO_LED_ON;
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = GPIO_LED_OFF;
					break;
				case GPIOSTATE_PIN_STATUS:
					status = GPIO_LED_STATUS;
					break;
				case GPIOSTATE_PIN_LED_BLINK:
					while(1) {
						if (!wlan_led) {
							ret = if_readlist_proc("ath1", &ife_w_stats_guest);
							if (if_readlist_proc("ath0", &ife_w_stats) != -1) {
								DEBUG_MSG("wlan(1) tx-%d-%d rx-%d-%d\n", ife_w_stats.tx_packets,w_tx_packets,ife_w_stats.rx_packets,w_rx_packets);
								if ((ife_w_stats.tx_packets > w_tx_packets) || (ife_w_stats.rx_packets > w_rx_packets) ||
									(ife_w_stats_guest.tx_packets > w_tx_packets_guest) || (ife_w_stats_guest.rx_packets > w_rx_packets_guest)) {
									wlan_led = 0;
								}
								else {
									wlan_led = 1;
								}
								w_tx_packets = ife_w_stats.tx_packets;
								w_rx_packets = ife_w_stats.rx_packets;
								
								if (ret != -1) {
									w_tx_packets_guest = ife_w_stats_guest.tx_packets;
									w_rx_packets_guest = ife_w_stats_guest.rx_packets;
								}									
							}
							else {
								printf("if_readlist_proc fails\n");
								wlan_led = 1;
							}
						}

						if (wlan_led) {
							status = GPIO_LED_ON;
							wlan_led = 0;
						}
						else {
							status = GPIO_LED_OFF;
							wlan_led = 1;
						}

						ret = ioctl(gpio_fd, WIRELESS_LED, status);
						// Blink once every 0.5 sec(defined in the v1.09 specification) 
						usleep(250000);
					}
				default:
					goto exit;
			}

			ret = ioctl(gpio_fd, WIRELESS_LED, status);
			DEBUG_MSG("gpio:WLAN_LED, ret = %02d\n",ret);
			break;
#endif

		case GPIONAME_USB_LED:
			DEBUG_MSG("gpio:USB_LED\n");
			switch(gpio_state){
				case GPIOSTATE_PIN_LED_ON:
					status = GPIO_LED_ON;
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = GPIO_LED_OFF;
					break;
				case GPIOSTATE_PIN_STATUS:
					status = GPIO_LED_STATUS;
					break;
				default:
					goto exit;
			}

			ret = ioctl(gpio_fd, USB_LED, status);
			DEBUG_MSG("ret = %02d\n", ret);
			break;

		case GPIONAME_SYSTEM:
			//SYSTEM for Restore Default and Reboot = MV_GPP13
			first_time = 0;
			last_time = 0;
			DEBUG_MSG("gpio:SYSTEM\n");
			#define GPIO_SYSTEM_PID  "/var/run/gpio_system.pid"
			gpio_cleanup_pid(GPIO_SYSTEM_PID);
			gpio_create_pidfile(GPIO_SYSTEM_PID);
			status = GPIO_LED_STATUS;
			while(1){
				// Every 1 sec check gpio status
				sleep(1);
				ret = ioctl(gpio_fd, PUSH_BUTTON, status);
				if(ret == PUSH_VALUE){ // button is dirty
					printf("PUSH_BUTTON gpio = %02d at %ld\n", ret, time((time_t *)NULL));
					if(count == 0){
						first_time = time((time_t *)NULL);
					}else{
						last_time = time((time_t *)NULL);
						//if((count > 2) || (last_time - first_time > 3)){
						//	restore = 1;
						//	printf("PUSH_BUTTON lasts for %d secs, going to restore !!\n",last_time - first_time);
						//	break;
						//}
					}
					count ++;
				}else{ // button is clean
					if(count != 0){
						last_time = time((time_t *)NULL);
						if((count >= 5) || (last_time - first_time > 5)){
							printf("PUSH_BUTTON lasts for %d secs, going to reStore !!\n",last_time - first_time);
							restore = 1;
							break;
						}else{
							printf("PUSH_BUTTON lasts for %d secs, no reBoot for new spec !!\n",last_time - first_time);
							// reboot = 1;
						}
						count = 0;
					}
				}
			}
#ifdef DLINK_ROUTER_LED_DEFINE
			ret = ioctl(gpio_fd, STATUS_LED, GPIO_LED_OFF);
			ret = ioctl(gpio_fd, POWER_LED, GPIO_LED_ON);
#endif
			break;

#ifdef ATHEROS_11N_DRIVER
		case GPIONAME_INTERNET_LED:
			DEBUG_MSG("gpio:INTERNET_LED\n");
			switch(gpio_state){
				case GPIOSTATE_PIN_LED_ON:
					status = GPIO_LED_ON;
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = GPIO_LED_OFF;
					break;
				case GPIOSTATE_PIN_LED_SBLINK: //slow speed
					if(gpio_parameter == GPIOPARAMETER_GREEN)
					{
					    #define GPIO_WAN_GREEN_PID  "/var/run/gpio_wan_green.pid"
						gpio_cleanup_pid(GPIO_WAN_GREEN_PID);
						gpio_create_pidfile(GPIO_WAN_GREEN_PID);	
					}	
					sleep(1);
					fp_internet = fopen("/var/tmp/internet_led_blink.tmp", "w+");
					if(fp_internet){
						fputs("1", fp_internet);
						fclose(fp_internet);
						memset(&ife_w_stats, 0, sizeof(struct user_net_device_stats));
					}else{
						perror("create /var/tmp/internet_led_blink.tmp");
						goto exit;
					}
					printf("gpio: create /var/tmp/internet_led_blink.tmp finished\n");
					while(1){
						fp_internet=fopen("/var/tmp/internet_led_blink.tmp", "r+");
						if(fp_internet){
							fclose(fp_internet);
							if((!status_led)&&(gpio_parameter!=GPIOPARAMETER_AMBER))
							{
#ifdef UBICOM_TWOMII
								if(if_readlist_proc("eth1", &ife_w_stats)!=-1)
#else
								if(if_readlist_proc("eth0.1", &ife_w_stats)!=-1)
#endif
								{
									//printf("wan(1) tx-%d-%d rx-%d-%d\n",ife_w_stats.tx_packets,w_tx_packets,ife_w_stats.rx_packets,w_rx_packets);
									if((ife_w_stats.tx_packets > w_tx_packets)||(ife_w_stats.rx_packets > w_rx_packets))
									{
										status_led = 0;
									}
									else
									{
										status_led = 1;
									}
									w_tx_packets = ife_w_stats.tx_packets;
									w_rx_packets = ife_w_stats.rx_packets;
								}
								else
								{
									printf("if_readlist_proc fails\n");
									status_led = 1;
								}
							}
							if(status_led){
								status = GPIO_LED_ON;
								status_led = 0;
							}else{
								status = GPIO_LED_OFF;
								status_led = 1;
							}
							//printf("internet_led_blinking\n");
							switch(gpio_parameter){ //argv[3]
								case GPIOPARAMETER_AMBER:
									if((fp_internet = fopen("/var/run/gpio_wan_green.pid", "r")) != NULL)
									{
										ret = ioctl(gpio_fd,INTERNET_LED_1, GPIO_LED_OFF);
										fclose(fp_internet);
										goto exit;
									}
									ret = ioctl(gpio_fd,INTERNET_LED_1, status);
									#ifdef SPECIAL_ONEINTERNET_LED
									ret = ioctl(gpio_fd,INTERNET_LED_2, GPIO_LED_OFF);
									#endif
									if(status_led == 1) /*slow speed*/
										sleep(2);
									else
										sleep(1);
									break;
								default:
									ret = ioctl(gpio_fd,INTERNET_LED_2, status);
									#ifdef SPECIAL_ONEINTERNET_LED
									ret = ioctl(gpio_fd,INTERNET_LED_1, GPIO_LED_OFF);
									#endif
									// Blink once every 0.5 sec(defined in the v1.09 specification) 
									usleep(250000);
							}
						}else{
							perror("check /var/tmp/internet_led_blink.tmp");
							goto exit;
						}
					}
					break;
				default:
					goto exit;
			}
			switch(gpio_parameter){ //argv[3]
				case GPIOPARAMETER_AMBER:
					gpio_pin = INTERNET_LED_1;
#ifdef SPECIAL_ONEINTERNET_LED
					gpio_pin_other = INTERNET_LED_2;
#endif
					break;
				case GPIOPARAMETER_GREEN:
					gpio_pin = INTERNET_LED_2;
#ifdef SPECIAL_ONEINTERNET_LED
					gpio_pin_other = INTERNET_LED_1;
#endif
					break;
				default:
					goto exit;
			}
			if(status == GPIO_LED_ON)
			{
#ifdef SPECIAL_ONEINTERNET_LED
			        ret = ioctl(gpio_fd, gpio_pin_other, GPIO_LED_OFF);
#endif
			        ret = ioctl(gpio_fd, gpio_pin, status);
			}
			else
			{
			        ret = ioctl(gpio_fd, gpio_pin, status);
		        }
			break;

		case GPIONAME_POWER_LED:
			DEBUG_MSG("gpio:POWER_LED\n");
			fpid = GPIO_POWER;
			gpio_cleanup_pid(fpid);
			gpio_create_pidfile(fpid);
#ifdef SPECIAL_POWER_STATUS_LED
					ret = ioctl(gpio_fd, STATUS_LED, GPIO_LED_OFF);
#endif
			switch(gpio_state){
				case GPIOSTATE_PIN_LED_ON:
					status = GPIO_LED_ON;
					ret = ioctl(gpio_fd, POWER_LED, status);
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = GPIO_LED_OFF;
				  ret = ioctl(gpio_fd, POWER_LED, status);
					break;
				case GPIOSTATE_PIN_LED_BLINK:
					while(1) {
						if(status_led) {
							status = GPIO_LED_ON;
							status_led = 0;
						} 
						else {
							status = GPIO_LED_OFF;
							status_led = 1;
						}
						ret = ioctl(gpio_fd, POWER_LED, status);
						usleep(500000);
					}
					break;

				default:
					status = GPIO_LED_STATUS;
#ifdef AR7161
			ret = ioctl(gpio_fd, POWER_LED, GPIO_LED_OFF);
			ret = ioctl(gpio_fd, STATUS_LED, GPIO_LED_OFF);
			sleep(1);
#else
			ret = ioctl(gpio_fd, POWER_LED, GPIO_LED_ON);
			ret = ioctl(gpio_fd, STATUS_LED, GPIO_LED_ON);
			sleep(2);
#endif
			ret = ioctl(gpio_fd, POWER_LED, GPIO_LED_ON);
			}
			break;

		case GPIONAME_SWITCH_CONTROL:
			DEBUG_MSG("gpio:SWITCH_CONTROL\n");
			switch(gpio_state){
				case GPIOSTATE_PIN_LED_ON:
					status = GPIO_LED_ON;
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = GPIO_LED_OFF;
					break;
				default:
					goto exit;
			}
			ret = ioctl(gpio_fd, SWITCH_CONTROL, status);
			break;

#ifdef AR7161
		case GPIONAME_WPS:
			DEBUG_MSG("gpio:WPS\n");
			switch(gpio_state){
				case GPIOSTATE_PIN_LED_ON:
					status = GPIO_LED_ON;
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = GPIO_LED_OFF;
			                #define GPIO_WPS_PID  "/var/run/gpio_wps.pid"
					i = read_pid(GPIO_WPS_PID);
			                if((i != -1)&&(i != 0)) kill(i, SIGHUP);
					break;
				case GPIOSTATE_PIN_LED_BLINK:
			                #define GPIO_WPS_PID  "/var/run/gpio_wps.pid"
			                gpio_cleanup_pid(GPIO_WPS_PID);
			                gpio_create_pidfile(GPIO_WPS_PID);
					while(1) {
						if(status_led) {
							status = GPIO_LED_ON;
							status_led = 0;
						} 
						else {
							status = GPIO_LED_OFF;
							status_led = 1;
						}
#ifdef SPECIAL_WPS_LED
			                        ret = ioctl(gpio_fd,WPS_LED, status);
			                        ret = ioctl(gpio_fd,STATUS_LED, GPIO_LED_OFF);
#else
			                        ret = ioctl(gpio_fd, WPS_LED, status);
#endif
						usleep(500000);
					}
					break;
				default:
					goto exit;
			}
#ifdef SPECIAL_WPS_LED
			ret = ioctl(gpio_fd,WPS_LED, status);
			ret = ioctl(gpio_fd,STATUS_LED, GPIO_LED_OFF);
			usleep(500000);								        
#else
			ret = ioctl(gpio_fd, WPS_LED, status);
#endif
			break;
#endif
		case GPIONAME_TEST:
			DEBUG_MSG("gpio:TEST\n");//NickChou add
			switch(gpio_state){
				case GPIOSTATE_PIN_LED_ON:
					status = 1;;
					break;
				case GPIOSTATE_PIN_LED_OFF:
					status = 0;
					break;
				default:
					goto exit;
			}
				{
					int offset=0;
					offset=atoi(argv[3]);
					DEBUG_MSG("gpio test BIT%d\n", offset);
					ret = ioctl(gpio_fd, (1 << offset), status);
				}
			break;
#endif	  //end ATHEROS_11N_DRIVER
#ifdef sl3516
		case GPIONAME_WPS_BUTTON:
			printf("gpio:WPS_BUTTON\n");
			status = GPIO_LED_STATUS;
			while(1){
				// Every 0.5 sec check gpio status
				usleep(500000);
				ret = ioctl(gpio_fd, WPS_BUTTON, status);
				if(ret == 0){ // button is dirty
					DEBUG_MSG("WPS_BUTTON gpio = %02d \n", ret);
					sprintf(cmd,"cli net wl trigger_wsc pbc");
					system(cmd);
					fprintf(stderr,"%s",cmd);
				}
			}
			break;
#endif
#ifdef IP8000 
		case GPIONAME_SD_CHECK:
			sd_card_detect = 0;
			DEBUG_MSG("gpio:SDCARD\n");
			status = GPIO_LED_STATUS;
			
			for(i=0;i<3;i++){
				fp_sd = fopen("/var/tmp/cardreader_dev", "r");
				if(fp_sd == NULL)
				{
    			syslog(LOG_NOTICE,"Can't detect SD card reader!\n");
    			sd_card_detect = -1;
				}
				else
				{
    			while(!feof(fp_sd))
    			{
      			if(fgets(cardreader_dev, 5, fp_sd))
      				continue;
    			}
    			fclose(fp_sd);
				}
			}

			if(sd_card_detect==-1)
				break;

			while(1){
				// Every 2 sec check gpio status
				sleep(3);
				ret = ioctl(gpio_fd, SD_CARD_DETECT, status);
				DEBUG_MSG("SD_CARD_DETECT = (%02d)\n",ret);
				if(ret == 0){ // sd card detect
					if(sd_card_detect != 1){
						DEBUG_MSG("SD_CARD_DETECT gpio = %02d at %ld\n", ret, time((time_t *)NULL));
						sprintf(cmd, "/usr/sbin/sxmount mount %s", cardreader_dev);
						system(cmd);
						sleep(3);
						system("/sbin/hotplug_init&");
						sd_card_detect = 1;
					}
				}
				else{ // not sd card detect
					if(sd_card_detect != 0){
						DEBUG_MSG("SD_CARD_DETECT gpio = %02d at %ld\n", ret, time((time_t *)NULL));
						sprintf(cmd, "/usr/sbin/sxmount umount %s", cardreader_dev);
						system(cmd);
						system(cmd);
						system(cmd);
						system("killall minidlna");
						system("rm -rf /var/run/minidlna.pid");
						sd_card_detect = 0;
					}
				}
			}
			break;
#endif //CONFIG_GPIO_SD_DETECT
		default:
			printf("Input Type Error (%d)\n",gpio_name);
			break;
	}//end switch() case

	if(reboot)
	{
		printf("system rebooting \n");
		sys_reboot();
	}
	else if(restore)
	{
#ifdef sl3516
		printf("sl3516 restore_default and rebooting \n");
		sprintf(cmd,"cli sys restore");
		fprintf(stderr,"%s",cmd);
		system(cmd);
#else
#ifdef CONFIG_USER_WISH
        	system("/etc/init.d/stop_wish &");
#endif
#ifdef CONFIG_USER_STREAMENGINE
        	system("/etc/init.d/streamengine_stop &");
        	sleep(2);
#endif
		printf("restore_default and rebooting \n");
		sprintf(cmd,"nvram restore_default");
		fprintf(stderr,"%s",cmd);
		//system("rm /etc/wcn/*.wfc");
		//system("cp -f /etc/wcn_default/*.wfc /etc/wcn/");
		system(cmd);
		sleep(2);
#endif
		sys_reboot();
	}
exit:
	//close(gpio_fd);
	return 1;
}

