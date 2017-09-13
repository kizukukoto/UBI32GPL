#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <getopt.h>
#include <fcntl.h>
#include <nvram.h>
#include <sutil.h>
#include <shvar.h>
#include "libwlan.h"
#include "libdb.h"

typedef struct  iw_freq iwfreq;

#ifdef ATH
int iw_freq2channel(iwfreq *    in)
{
        int     freq = in->m;
        int channel;

        channel = (freq/100000 - 2412)/5 + 1;
        if(channel > 0 && channel < 14)
                return channel;
        else
                return 0;
}

int iw_freq2channel_5g(iwfreq * in)
{
        int     freq = in->m;
        int channel;

        channel = (freq/100000-5000)/5;

        return channel;
}
#endif

#ifdef MVL
int iw_freq2channel(iwfreq * in)
{
        int freq = in->m;
        int channel;

        channel = freq;
        if (channel > 0 && channel < 14)
                return channel;
        else
                return 0;
}

int iw_freq2channel_5g(iwfreq * in)
{
        return in->m;
}
#endif

/* get 2.4G Channel */
static void get_channel(char *wlan_channel)
{
        int skfd;
        struct iwreq wrq;
        int channel;

        /* Create a channel to the NET kernel. */
        skfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(skfd < 0) {
                perror("get_channel");
                strcpy(wlan_channel, "unknown");
                close(skfd);
                return;
        }

        /* Set device name */
        strncpy(wrq.ifr_name, WLAN0_ETH, IFNAMSIZ);

        /* Do the request */
        if( ioctl(skfd, 0x8B05/*SIOCGIWFREQ*/, &wrq) >= 0)
        {
                channel=iw_freq2channel(&(wrq.u.freq));
                if(channel>0)
                        sprintf(wlan_channel,"%d",channel);
                else
                        strcpy(wlan_channel, "unknown");
        } else {
                cprintf("get_channel: ioctl error\n");
                strcpy(wlan_channel, "unknown");
        }
        close(skfd);
        return;
}

/* get 5G channel */
static void get_5g_channel(char *wlan_channel)
{
        int skfd;
        struct iwreq wrq;
        int channel;

        /* Create a channel to the NET kernel. */
        skfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (skfd < 0) {
		perror("get_5g_channel");
		strcpy(wlan_channel, "unknown");
		close(skfd);
		return;
        }

        strncpy(wrq.ifr_name, nvram_safe_get("wlan0_eth"), IFNAMSIZ);
        /* Do the request */
        if( ioctl(skfd, 0x8B05/*SIOCGIWFREQ*/, &wrq) >= 0) {
                channel=iw_freq2channel_5g(&(wrq.u.freq));
                if(channel>0)
                        sprintf(wlan_channel,"%d",channel);
                else
                        strcpy(wlan_channel, "unknown");
        } else {
                cprintf("get_5g_channel: ioctl error\n");
                strcpy(wlan_channel, "unknown");
        }

        close(skfd);
        return;
}

int get_wlan_channel(int type, char *channel)
{
        char wlan_enable[32], wlan_channel[10];
        int ret = -1;

        memset(wlan_channel, '\0', sizeof(wlan_channel));

        if (type < 0 && type > 1)
                goto out;

        sprintf(wlan_enable, "wlan%d_enable", type);
        if (NVRAM_MATCH(wlan_enable, "0") != 0) {
                goto out;
        }

        if (type == 0)
                get_channel(wlan_channel);
        else
                get_5g_channel(wlan_channel);

        if (strcmp(wlan_channel, "unknown") == 0)
                goto out;
        else
                strcpy(channel, wlan_channel);
        return 0;
out:
        strcpy(channel, "NULL");
        return ret;
}
