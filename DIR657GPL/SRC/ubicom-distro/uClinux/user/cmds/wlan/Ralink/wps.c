#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h> /* for sockaddr_in */
#include <sys/ioctl.h>

#include "wireless.h"
#include "nvram.h"

static int wl_debug;
#define wlmsg(fmt, a...) { wl_debug ? fprintf(stderr, fmt, ##a) : 0;}

#define RT_PRIV_IOCTL (SIOCIWFIRSTPRIV + 0x01)
#define RT_OID_WSC_QUERY_STATUS 0x0751
#define WSC_RESULT_0  "/var/tmp/wps_result0"
#define WSC_RESULT_1  "/var/tmp/wps_result1"
#define WSC_CONFIGFILE_0 "/var/tmp/iNIC_ap1.dat"
#define WSC_CONFIGFILE_1 "/var/tmp/iNIC_ap.dat"
#define WSC_PBC "/sys/class/gpio/gpio33/value"
#define WSC_LED "/sys/class/gpio/gpio34/value"
#define WPS_PID "/var/run/wps.pid"

extern int wps_led_inprocess(int);
extern int wps_led_success(int);
extern int toggle_wps_led(int);
extern int wps_led_error(int);

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

/***************************************************
 * wireless, nvram, independence helper.
 * ************************************************/
#define dbg(fmt, a...)  fprintf(stderr, fmt, ##a)
typedef int (*action_t)(char *, int, void *);
typedef int WSC_STATE;

/* Query WPS Status */
WSC_STATE rt_oid_get_wsc_status(char *ifname, int skfd){
   struct iwreq wrq;
   int data;

   memset(&wrq, 0, sizeof(wrq));

   strcpy(wrq.ifr_ifrn.ifrn_name, ifname);
   wrq.u.data.length = sizeof(int);
   wrq.u.data.pointer = &data;
   wrq.u.data.flags = RT_OID_WSC_QUERY_STATUS ;

   ioctl(skfd, RT_PRIV_IOCTL ,&wrq);

   return data;
}

static int kill_wps_pid_file(const char *iface)
{
	char path[128];
	sprintf(path, "%s.%s", WPS_PID, "ra01_0");
	if (!access(path, F_OK))
        	unlink(path);
	return 0;
}

static int stop_wps_monitor(const char *iface)
{
        DEBUG_MSG("stop_wps_monitor\n");
        FILE *fp = NULL;
        char wps_pid[12], path[128], tmp_pid[12];

        memset(path, '\0', sizeof(path));
        memset(wps_pid, '\0', sizeof(wps_pid));

        sprintf(path, "%s.%s", WPS_PID, iface);
        fp = fopen(path, "r");
        if (fp == NULL)
                return -1;

	fgets(tmp_pid, sizeof(tmp_pid), fp);
        fclose(fp);
	strncpy(wps_pid, tmp_pid, strlen(tmp_pid));
        _system("kill -9 %s", wps_pid);
        _system("echo 1 > %s", WSC_LED);

        /* rmmove wps.pid file */
	kill_wps_pid_file("ra00_0");
	kill_wps_pid_file("ra01_0");
        return 0;
}

static int set_nvram_key(const char *ssid, const char *psk)
{
        nvram_set("wlan0_ssid", ssid);
        nvram_set("wlan1_ssid", ssid);
        nvram_set("wlan0_security", "wpa2auto_psk");
        nvram_set("wlan1_security", "wpa2auto_psk");
        nvram_set("wlan0_psk_pass_phrase", psk);
        nvram_set("wlan1_psk_pass_phrase", psk);
        nvram_set("wlan0_psk_cipher_type", "both");
        nvram_set("wlan1_psk_cipher_type", "both");
        nvram_set("wps_configured_mode", "0");
        nvram_commit();
        _system("cmds wlan wconfig ra01");
        _system("cmds wlan wconfig ra00");
        return 0;
}

/*
 * update wireless nvram
 * @path : /var/tmp/iNIC_apX.dat
 * @iface : 0 express 2.4G , 1 express 5G
 */
static int _update_wlan_nvram(const char *path, int iface)
{
        char buf[512], ssid[64], passphrase[65], cmds[128];
        FILE *fp;
        char *ptr, *dev;
        int search_key = 0 ;
        char *wsc_config = nvram_safe_get("wps_configured_mode");

        memset(ssid, '\0', sizeof(ssid));
        memset(passphrase, '\0', sizeof(passphrase));
        memset(buf, '\0', sizeof(buf));
        memset(cmds, '\0', sizeof(cmds));

        dev = (iface == 0) ? "ra00_0" : "ra01_0";

        // un-config
        if (!strcmp(wsc_config, "1")) {
                fp = fopen(path, "r");
                if (!fp) {
                        perror("wsc_result");
                        return -1;
                }

                while (fgets(buf, sizeof(buf), fp) && search_key < 2) {
                        if ((ptr = strstr(buf, "WPAPSK1=")) > 0){
                                memcpy(passphrase, (ptr+8), 64);
                                passphrase[64] = '\0';
                                search_key++;
                        }
                        if ((ptr = strstr( buf, "SSID1=")) > 0) {
                                memcpy(ssid, (ptr+6), 9);
                                ssid[9] = '\0';
                                search_key++;
                        }
                        memset(buf, '\0', sizeof(buf));
                }

                fclose(fp);
                _system("iwpriv %s set AuthMode=%s", dev, "WPAPSKWPA2PSK");
                _system("iwpriv %s set EncrypType=%s", dev, "TKIPAES");
                _system("iwpriv %s set IEEE8021X=0", dev);
                _system("iwpriv %s set WPAPSK=%s", dev, passphrase);
                _system("iwpriv %s set SSID=%s", dev, ssid);
                set_nvram_key(ssid, passphrase);
        }
        return 0;
}

int update_wireless_nvram(int argc, char *argv[])
{
        if (strcmp(argv[1], "ra00_0") == 0)
                return _update_wlan_nvram(WSC_CONFIGFILE_1, 1);
        else
                return _update_wlan_nvram(WSC_CONFIGFILE_0, 0);
}

/***************************************************
 * main functions. Wireless, WPS...
 * ************************************************/
int pin_main(int argc, char *argv[])
{
        char *wps_enable, *wps_config, *inface;
        char *pin_value;

        if (argv[2] != NULL)
                inface = argv[2];
        else
                inface = "ra01_0";

        if (argc < 2) {
                printf("args: <pin>: set up PIN code\n");
                return -1;
	}

        wps_enable = nvram_safe_get("wps_enable");
        wps_config = nvram_safe_get("wps_configured_mode");
        pin_value = argv[1];
        if (!strcmp(wps_enable, "1")){
                _system("iwpriv %s set WscConfMode=7", inface); //0x1 Enrollee; 0x2 Proxy; 0x4 Registar
                if (argv[1]) { //wsc_pin
                        if(!strcmp(wps_config, "1"))
                                _system("iwpriv %s set WscConfStatus=1", inface); // unconfig
                        else
                                _system("iwpriv %s set WscConfStatus=2", inface); // config

                        _system("iwpriv %s set WscMode=1", inface);
                        _system("iwpriv %s set WscPinCode=%s", inface, argv[1]);
                }
                _system("iwpriv %s set WscGetConf=1", inface);
        }

        return 0;
}

static void xmit_httpd_wps_result(int success)
{
        if (success)
                system("killall -USR1 httpd");
        else
                system("killall -QUIT httpd");
}

int pbc_main(int argc, char *argv[])
{
        char *wps_enable, *wps_config, *inface;

        if (argv[1] != NULL)
                inface = argv[1];
        else
                inface = "ra01_0";

        /* gpio WPS on|off */
        if (nvram_match("wps_enable", "1") != 0)
                return 0;

        wps_enable = nvram_safe_get("wps_enable");
        wps_config = nvram_safe_get("wps_configured_mode");

        if (!strcmp(wps_enable, "1")){
                _system("iwpriv %s set WscConfMode=7", inface); //0x1 Enrollee; 0x2 Proxy; 0x4 Registar
                //pbc
                 if(!strcmp(wps_config, "1"))
                        _system("iwpriv %s set WscConfStatus=1", inface); //WscConfStatus=2 config; 
                 else
                        _system("iwpriv %s set WscConfStatus=2", inface);
                _system("iwpriv %s set WscMode=2", inface);    //WscMode=1 PIN ;WscMode=2 PBC  
                _system("iwpriv %s set WscGetConf=1", inface);
        }

        return 0;
}

int start_wps_main(int argc, char *argv[])
{
        char *wps_enable, *wps_config, *wlan_enable, *inface;

        if (argv[1] != NULL || strcmp(argv[1], "ra00_0") == 0)
                inface = argv[1];
        else
                inface = "ra01_0";

        /* stop wscd deamon */
        _system("cmds wlan wps stop %s", inface);

        wps_enable = nvram_safe_get("wps_enable");
        wps_config = nvram_safe_get("wps_configured_mode");
        wlan_enable = nvram_safe_get("wlan0_enable");

        if (!strcmp(wps_enable, "1") && !strcmp(wlan_enable, "1")){
                _system("wscd -i %s -w /etc/xml -m 1 -a %s -p 60001 &", inface, nvram_safe_get("lan_ipaddr"));
                _system("iwpriv %s set WscConfMode=7", inface); //0x1 Enrollee; 0x2 Proxy; 0x4 Registar

                if (!strcmp(wps_config, "1"))
                        _system("iwpriv %s set WscConfStatus=1", inface); //1 is unconfig mode ; 2 is config mode
                else
                        _system("iwpriv %s set WscConfStatus=2", inface); //1 is unconfig mode ; 2 is config mode

                _system("iwpriv %s set WscMode=1", inface); // 1 mean use PIN code; 2 mean use PBC(push button connect)
                _system("iwpriv %s set WscPinCode=%s", inface, nvram_safe_get("wps_pin")); //set PINCODE
                _system("iwpriv %s set WscGetConf=1", inface); // triggle WPS AP to do simple config with client
                _system("iwpriv %s set WscStatus=0", inface); //get WPS config methods 0 mean not used
        }else  // wps_disable
                return -1;

        return 0;
}

int stop_wps_main(int argc, char *argv[])
{
        DEBUG_MSG("stop_wps\n");
        FILE *fp = NULL;
        char wps_pid[5], path[128];
        char *iface = NULL;

        memset(path, "\0", sizeof(path));

        if (argv[1] != NULL)
                iface = argv[1];
        else
                iface = "ra01_0";

        sprintf(path, "/var/run/wscd.pid.%s", iface);

        fp = fopen(path, "r");
        if (fp == NULL)
                return -1;

        fread(wps_pid, 5, 1, fp);

        fclose(fp);

        _system("kill -9 %s", wps_pid);

        return 0;
}

int wps_ok_main(int argc, char *argv[])
{
	printf("Nothing to do\n");
        return 0;
}

int wps_pbc_monitor_main(int argc , char *argv[])
{
	printf("Nothing to do\n");
        return 0;
}

int wps_monitor_main(int argc , char *argv[])
{
        unsigned int num_sec = 0;
        int skfd, value, pid = 0;
        FILE *fd;
        char *iface = NULL, path[128];

        if (argv[1] != NULL)
                iface = argv[1];
        else
                iface = "ra01_0";

        skfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (skfd < 0)
                return -1;

        if (strcmp(iface, "ra00_0") == 0)
                fd = fopen(WSC_RESULT_0, "w+");
        else
                fd = fopen(WSC_RESULT_1, "w+");

        if (!fd)
                perror("wsc_result");

        num_sec = 120;

        DEBUG_MSG("monitor_wsc num_sec= %d \n", num_sec);

        sprintf(path, "/var/run/wps.pid.%s", iface);
        pid = getpid();
        _system("echo %d > %s", pid, path);

        while ((num_sec-- > 0) && (value != 34)) {
                value = rt_oid_get_wsc_status(iface, skfd);
                wps_led_inprocess(1 * 1000);
        }

        if (value == 34) {
                fwrite("1", 1, 1, fd);
                update_wireless_nvram(argc, argv);
                wps_led_success(5 * 1000);
                xmit_httpd_wps_result(1);
                system("killall -SIGUSR2 httpd");
        } else {
                fwrite("0", 1, 1, fd);
                wps_led_error(10 * 1000);
                xmit_httpd_wps_result(0);
        }

        fclose(fd);

        if (strcmp(iface, "ra00_0") == 0){
                unlink(WSC_RESULT_0);
                stop_wps_monitor("ra01_0");
        } else {
                unlink(WSC_RESULT_1);
                stop_wps_monitor("ra00_0");
        }

        return 0;
}

int init_wps_main(int argc, char *argv[])
{
	printf("Nothing to do!\n");
        return 0;
}

void start_wps(void)
{
        if (nvram_match("wps_enable", "1") == 0) {
                system("cmds wlan wps start ra00_0 &");
                system("cmds wlan wps start ra01_0 &");
                system("killall monitor_pbc &");
                sleep(1);
                system("monitor_pbc &");
        }
}

void stop_wps(void)
{
        system("killall monitor_pbc &");
        if (nvram_match("wps_enable", "0") == 0) {
                system("cmds wlan wps stop ra00_0 &");
                system("cmds wlan wps stop ra01_0 &");
        }
}
