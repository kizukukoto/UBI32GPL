/* Date: 2008-12-19
*  Name: Albert Chen 
*  Description: this c code is base on Ralink WPS feature implement
*  		If we want to trigger Ralink WPS feature, we must setting some iwpriv commands  
*		to Ralink wireless driver, the setting commands as below list

*		iwpriv ra0 set WscConfMode=value //0x1 Enrollee; 0x2 Proxy; 0x4 Registar
*		iwpriv ra0 set WscConfStatus=value //1 is unconfig mode ; 2 is config mode
*		iwpriv ra0 set WscMode=value	// 1 mean use PIN code; 2 mean use PBC(push button connect)
*		iwpriv ra0 set WscPinCode=value (this value length is 8 charachers) //set PINCODE
*		iwpriv ra0 set WscGetConf=value	// triggle WPS AP to do simple config with client, 0 mean disable
*		
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* for sockaddr_in */
#include <fcntl.h>
#include <time.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <asm/types.h>
#include <sys/signal.h>
#include <sys/time.h>

#define IFNAMSIZ 16
#include </usr/include/linux/wireless.h>
#include <sys/ioctl.h>

#include <nvram.h>
#include "cmds.h"

#define RT_PRIV_IOCTL (SIOCIWFIRSTPRIV + 0x01)	// copy from raconfig.h
#define RT_OID_WSC_QUERY_STATUS 0x0751	// copy from webui src
#define RT_OID_WSC_PIN_CODE 0x0752	// copy from webui src
#define WSC_PUSHBUTTON "/proc/sys/wlan/wps_enable"
#define GPIO_DRIVER  		"/dev/gpio_ioctl"
#define WSC_RESULT  "/var/tmp/wps_result"
#define WSC_CONFIGFILE "/var/tmp/iNIC_ap.dat"
#define WSC_LED  17
static int led_status = 0;
int gpio_fd;
static int stop_wps_flag = 0;

typedef int WSC_STATE;
#define DEBUG_MSG //printf

WSC_STATE rt_oid_get_wsc_status(char *ifname, int skfd){
   struct iwreq wrq;
   int data;    

   memset(&wrq, 0, sizeof(wrq));

   strcpy(wrq.ifr_ifrn.ifrn_name, ifname);
   wrq.u.data.length = sizeof(int);
   wrq.u.data.pointer = &data;
   wrq.u.data.flags = RT_OID_WSC_QUERY_STATUS ;

   ioctl(skfd, RT_PRIV_IOCTL ,&wrq); 
   DEBUG_MSG("wsc query status = %d \n", data);

   return data;
}

int update_wireless_nvram(void)
{
	char *wsc_config=nvram_safe_get("wps_configured_mode");
	char tmp_security[32] = {};
	char tmp_cipher[32] = {};
	char tmp_passphrase[32] = {};
	char buf[512];
	char passphrase[65] = {0};
	FILE *fp;
	char *ptr;
	
	sprintf(tmp_security, "wlan%d_security", atoi(nvram_safe_get("wlan0_radio_mode")));
	sprintf(tmp_cipher, "wlan%d_psk_cipher_type", atoi(nvram_safe_get("wlan0_radio_mode")));
	sprintf(tmp_passphrase, "wlan%d_psk_pass_phrase", atoi(nvram_safe_get("wlan0_radio_mode")));
	
	if (!strcmp(wsc_config, "0"))// un-config
	{
		fp = fopen(WSC_CONFIGFILE, "r");
		if (!fp){
   			perror("wsc_result"); 
			return -1;
		}	
		while(fgets(buf, sizeof(buf), fp))
		{
			if ((ptr = strstr(buf, "WPAPSK1=")) > 0){
				memcpy(passphrase, (ptr+8), 64);
				passphrase[64] = '\0';
				nvram_set(tmp_passphrase, passphrase);
				break;
			}						
		}
		fclose(fp);
		nvram_set(tmp_security, "wpa2auto_psk");
		nvram_set(tmp_cipher, "auto");
		nvram_set("wps_configured_mode", "0");
		nvram_commit();					 
	}
	return 0;	
	
}


int monitor_wsc(void/*int argc, char *argv[]*/)
{
   unsigned num_sec = 0;
   int skfd, gpio_fd;
   int ret, value;
   int toggle = 0;
   FILE *fd;

   skfd = socket(AF_INET, SOCK_DGRAM, 0);
   if (skfd < 0)
      return -1;
      
   fd = fopen(WSC_RESULT,"w");    
   if (!fd)
   	perror("wsc_result"); 


   num_sec = 120;//atoi(argv[1]);
   
   DEBUG_MSG("monitor_wsc num_sec= %d \n", num_sec);

   while (num_sec-- > 0 && value != 34)
   {
      value = rt_oid_get_wsc_status("ra0", skfd);
      sleep(1);
   }

   
   if (value == 34){
   	fwrite("1", 1,1, fd);
   	update_wireless_nvram();
   }	
   else	
   	fwrite("0", 1,1, fd);

   stop_wps_flag = 1;   

   fclose(fd);
   return 0;
}

//Albert add 20081029

int generic_wps_pin()
{
        
        unsigned long int rnumber = 0;
        unsigned long int pin = 0;
        unsigned long int accum = 0;
        int digit = 0;
        int checkSumValue =0 ;
        char mac[32];
        char *p_mac = NULL;
        char *mac_ptr = NULL;
        int i = 0;
        char *nvram_wps_pin = NULL, *nvram_wps_default_pin = NULL;
        char wps_pin[8]={};
        
        //get mac
       
        nvram_wps_pin = nvram_safe_get("wps_pin");
        nvram_wps_default_pin = nvram_safe_get("wps_default_pin");

	if(!strncmp(nvram_wps_pin, nvram_wps_default_pin, 8) || (strcmp(nvram_wps_pin, "") == 0) || (strcmp(nvram_wps_default_pin, "00000000") == 0))
	{
	        get_mac(nvram_safe_get("wlan0_eth"), mac);	       
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
	        nvram_set("wps_default_pin", wps_pin);
   nvram_set("wps_pin", wps_pin);
   	_system("iwpriv ra0 set WscVendorPinCode=%s", wps_pin);
	}else{
		_system("iwpriv ra0 set WscVendorPinCode=%s", nvram_wps_pin);
	}		
   
    DEBUG_MSG("generic_wps_pin finished\n");  
        return 0;

}
int init_wps_main(int argc, char *argv[])
{
	return generic_wps_pin();
}

void check_led(int sig)
{
	int ret;
	ret = ioctl(gpio_fd, WSC_LED, led_status);	
	led_status ^= 1;
	if (!stop_wps_flag)	
		signal(SIGALRM, check_led);
	else
		ret = ioctl(gpio_fd, WSC_LED, 0);	
	return;	
   	
}

static void set_wps(char *pin_value)
{
	char *wps_enable, *wps_config;
	struct itimerval value;
	struct sigaction act;
		
	// init check timer
	act.sa_handler=check_led; 
	act.sa_flags=SA_RESTART; 
	sigaction(SIGALRM, &act,NULL);
	
	value.it_value.tv_sec = 1;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 1;
	value.it_interval.tv_usec = 0;
	
	gpio_fd = open( GPIO_DRIVER, O_RDONLY);
	if (gpio_fd < 0 ) 
	{
		printf( "ERROR : cannot open gpio driver\n");
		return;
	}	
		
	DEBUG_MSG("set_wps\n");
	wps_enable = nvram_safe_get("wps_enable");
	wps_config = nvram_safe_get("wps_configured_mode");
	if (!strcmp(wps_enable, "1")){
		setitimer(ITIMER_REAL, &value, NULL);
		_system("iwpriv ra0 set WscConfMode=7"); //0x1 Enrollee; 0x2 Proxy; 0x4 Registar
		DEBUG_MSG("iwpriv ra0 set WscConfMode=7\n"); //0x1 Enrollee; 0x2 Proxy; 0x4 Registar
		if(pin_value){ //wsc_pin
			if(!strcmp(wps_config, "0"))
				_system("iwpriv ra0 set WscConfStatus=1");
			else
				_system("iwpriv ra0 set WscConfStatus=2");
		
			_system("iwpriv ra0 set WscMode=1");
			_system("iwpriv ra0 set WscPinCode=%s",pin_value);
			DEBUG_MSG("iwpriv ra0 set WscConfStatus=2\n");
			DEBUG_MSG("iwpriv ra0 set WscMode=1\n");
			DEBUG_MSG("iwpriv ra0 set WscPinCode=%s\n",pin_value);							
		}else{ //pbc			
			if(!strcmp(wps_config, "0")){
				_system("iwpriv ra0 set WscConfStatus=1"); //WscConfStatus=2 config; 
				DEBUG_MSG("iwpriv ra0 set WscConfStatus=1\n");
			}	
			else{
				_system("iwpriv ra0 set WscConfStatus=2");
				DEBUG_MSG("iwpriv ra0 set WscConfStatus=2\n");
			}
			_system("iwpriv ra0 set WscMode=2");	//WscMode=1 PIN ;WscMode=2 PBC	
			DEBUG_MSG("iwpriv ra0 set WscMode=2\n");
		}	
		_system("iwpriv ra0 set WscGetConf=1");
		DEBUG_MSG("iwpriv ra0 set WscGetConf=1\n");
	}else // wps_disable
		return;
		
	unlink(WSC_RESULT);
	
	monitor_wsc();	
	
	ioctl(gpio_fd, WSC_LED, 1);	
	close(gpio_fd);
	return;

}

int trigger_wsc(int argc, char *argv[])
{
	//unsigned char pin[8];
	DEBUG_MSG("%s argv[1] =%s argv[2] =%s\n", __FUNCTION__, argv[1], argv[2]);
//	if (argv[2])
//		memcpy(pin, argv[2], 8);
	
	if(!strcmp(argv[1], "wsc_pin")) {
		set_wps(argv[2]);
	} else if(!strcmp(argv[1], "pbc")){
		set_wps(NULL);		
	}
	else {
		printf("\nusage:\n\t\t%s wsc_pin | pbc\n", argv[0]);
	}

}

int start_wps(void)
{
	DEBUG_MSG("start_wps\n");
	char *wps_enable, *wps_config, *wlan_enable;
	
	wps_enable = nvram_safe_get("wps_enable");
	wps_config = nvram_safe_get("wps_configured_mode");
	wlan_enable = nvram_safe_get("wlan0_enable");
	
	if (!strcmp(wps_enable, "1") && !strcmp(wps_config, "0") && !strcmp(wlan_enable, "1")){
		_system("wscd -i ra0 -w /etc/xml -m 1 -a %s -p 60001 &", nvram_safe_get("lan_ipaddr"));
		DEBUG_MSG("wscd -i ra0 -w /etc/xml -m 1 -a %s -p 60001\n", nvram_safe_get("lan_ipaddr"));		
		_system("iwpriv ra0 set WscConfMode=7"); //0x1 Enrollee; 0x2 Proxy; 0x4 Registar
		DEBUG_MSG("iwpriv ra0 set WscConfMode=7\n");
		_system("iwpriv ra0 set WscConfStatus=1"); //1 is unconfig mode ; 2 is config mode
		DEBUG_MSG("iwpriv ra0 set WscConfStatus=1\n");
		
		_system("iwpriv ra0 set WscMode=1"); // 1 mean use PIN code; 2 mean use PBC(push button connect)
		_system("iwpriv ra0 set WscPinCode=%s",nvram_safe_get("wps_pin")); //set PINCODE
			
			
		DEBUG_MSG("iwpriv ra0 set WscMode=1\n");
		DEBUG_MSG("iwpriv ra0 set WscPinCode=%s\n",nvram_safe_get("wps_pin"));							

		_system("iwpriv ra0 set WscGetConf=1"); // triggle WPS AP to do simple config with client
		DEBUG_MSG("iwpriv ra0 set WscGetConf=1\n");
		_system("iwpriv ra0 set WscStatus=0"); //get WPS config methods 0 mean not used
		DEBUG_MSG("iwpriv ra0 set WscStatus=0\n");			
		
	}else { // wps_disable
		DEBUG_MSG("wps_disable upnp \n");
		return;
	}		
	DEBUG_MSG("start_wps finished\n");
	
	return 0;
	
}

int stop_wps(void)
{
	DEBUG_MSG("stop_wps\n");
	FILE *fp = NULL;
	char wps_pid[5]={};
	
	fp = fopen("/var/run/wscd.pid", "r");
	if (fp == NULL){
		printf("wps open file fail\n");
		return -1;
	}
	fread(wps_pid, 5, 1, fp);
	fclose(fp);
	DEBUG_MSG("wps pid value = %s", wps_pid);
	_system("kill -9 %s", wps_pid);
		

	
	return 0;
	
}

int trigger_wsc_main(int argc, char *argv[])
{
	return trigger_wsc(argc, argv);
}

int wsc_button_main(int argc, char *argv[])
{
	FILE *fp;
	char value[2] ={};
	
	fp = fopen(WSC_PUSHBUTTON, "r");
	if (!fp)
	{
		perror("wsc");
		return;
	}
	   fgets(value, sizeof(value), fp);
	fclose(fp);
	return printf("%s", value);
}

int wsc_getstatus_main(int argc, char *argv[])
{
	FILE *fp;
	char value[2] ={};
	
	fp = fopen(WSC_RESULT, "r");
	if (!fp)
		return printf("2");

	   fgets(value, sizeof(value), fp);
	fclose(fp);
	return printf("%s", value);	
	
	
}
static int start_wps_main(int argc, char *argv[])
{
	printf("start_wps_main\n");
	return start_wps();
}
static int stop_wps_main(int argc, char *argv[])
{
	return stop_wps();
}

int wps_main(int argc, char *argv[])
{
	printf("wps_main\n");
	struct subcmd cmds[] = {
		{ "start", start_wps_main},
		{ "stop", stop_wps_main},
//Albert add
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}