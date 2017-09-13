#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h> /* for sockaddr_in */
#include <sys/ioctl.h>
#include <unistd.h>

#include <sutil.h>
#include <nvram.h>
#include <shvar.h>
#include <rc.h>

#define WSC_LED "/sys/class/gpio/gpio34/value"
#define WLUTILS_WLANLED_PID "/var/run/wlanled.pid"

/*************************************************
 * WPS LED toggle stuff
 * **********************************************/
void toggle_wps_led(int on)
{
        if (on)
                _system("echo 0 > %s", WSC_LED);
        else
                _system("echo 1 > %s", WSC_LED);
}

void toggle_wps_led_blank(int on, int off, int timeout)
{
        if (on < 100 || off < 100)
                return;

        on *= 1000;
        off *= 1000;
        timeout *= 1000;
        while (timeout >= 0) {
                toggle_wps_led(1);
                usleep(on);
                toggle_wps_led(0);
                usleep(off);
                timeout -= on + off;
        }
}
/**************************************************
 * Follow Wi-Fi Alliance WPS 1.0 Sep, 2006 Page.81
 * ************************************************/
void wps_led_inprocess(int timeout)
{
        toggle_wps_led_blank(200, 100, timeout);
}

void wps_led_error(int timeout)
{
        toggle_wps_led_blank(100, 100, timeout);
}

void wps_led_success(int timeout) /* led on 300 sec by SPEC */
{
        if (timeout == 0)
                timeout = 300 * 1000;
        toggle_wps_led_blank(timeout, 0, timeout);
}

void wps_led_overlap(int timeout)
{
        while (timeout > 0) {
                toggle_wps_led_blank(100, 100, 1000);
                toggle_wps_led(0);
                usleep(500 * 1000);
                timeout -= 1500;
        }
}

void sig(int n)
{
        toggle_wps_led(0);
        printf("exit\n");
        exit(0);
}

int wps_led_main(int argc, char *argv[])
{
        int t = atoi(argv[2]);

        if (argc < 3) {
                printf("%s [inproc | error | overlap | success] [timeout(sec)]\n", argv[0]);
                return 0;
        }
        t *= 1000;

        signal(SIGINT, sig);
        signal(SIGTERM, sig);
        signal(SIGQUIT, sig);

        if (strcmp(argv[1], "inproc") == 0)
                wps_led_inprocess(t);
        else if (strcmp(argv[1], "error") == 0)
                wps_led_error(t);
        else if (strcmp(argv[1], "overlap") == 0)
                wps_led_overlap(t);
        else if (strcmp(argv[1], "success") == 0)
                wps_led_success(t);
        else
                printf("%s [inproc | error | overlap | success] [timeout(sec)]\n", argv[0]);
        return 0;
}

enum {
        WLAN_G_BAND = 0,
        WLAN_A_BAND = 1,
};

/*************************************************
 * WLAN LED toggle stuff
 * **********************************************/
void toggle_wlan_led(int on, int band)
{
        char str[64];

        sprintf(str, "gpio %s %s", (band == WLAN_G_BAND)?"WLAN_LED":"WLAN_LED_2", on?"on":"off");
        system(str);
}

void toggle_wlan_led_blink(int on, int off, int band)
{
        if (on < 100 || off < 100)
                return;

        on *= 1000;
        off *= 1000;
        while (band != -1) {
                toggle_wlan_led(1, band);
                usleep(on);
                toggle_wlan_led(0, band);
                usleep(off);
        }
}

/*************************************************************
 * Follow Dlink Led spec. high speed: flash every 2/3 seconds.
 *************************************************************/
void wlan_led_blink(int band)
{
        toggle_wlan_led_blink(333, 333, band); /* about 1/3 second on, 1/3 second off */
}

void _sig_(int n)
{
        toggle_wlan_led(0, WLAN_G_BAND);
        toggle_wlan_led(0, WLAN_A_BAND);
        printf("exit\n");
        exit(0);
}

void create_pidfile(char *fpid)
{
        FILE *pidfile;

        if ((pidfile = fopen(fpid, "w")) != NULL) {
                fprintf(pidfile, "%d\n", getpid());
                (void) fclose(pidfile);
        } else {
                printf("Failed to create pid file %s: %m", WLUTILS_WLANLED_PID);
        }
}

void kill_wlanled_pid(void)
{
        FILE *pfilein;
        char pidnumber[32];
        char cmd[64];
        pfilein = fopen(WLUTILS_WLANLED_PID, "r");
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

int wlan_led_main(int argc, char *argv[])
{

        int wlan_band = -1;

        if (argc < 3) {
                printf("%s [blink] [2.4G | 5G]\n", argv[0]);
                return 0;
        }


        signal(SIGINT, _sig_);
        signal(SIGTERM, _sig_);
        signal(SIGQUIT, _sig_);

        create_pidfile(WLUTILS_WLANLED_PID);

        if (strcmp(argv[1], "blink") == 0) {
                if (strcmp(argv[2], "2.4G") == 0)
                        wlan_band = WLAN_G_BAND;
                else if (strcmp(argv[2], "5G") == 0)
                        wlan_band = WLAN_A_BAND;
                else
                        wlan_band = -1; /* -1 as wirless disabled */

                wlan_led_blink(wlan_band);
        }
        else
                printf("%s [blink] [2.4G | 5G]\n", argv[0]);

        return 0;
}
