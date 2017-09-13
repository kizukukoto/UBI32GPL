#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "libdb.h"
#include "ssc.h"
#include "public.h"
#include "querys.h"
#include "netinf.h"
#include "project.h"
#include "sutil.h"
#include "libwlan.h"
#include "libdhcpd.h"

struct __wireless_struct {
        const char *param;
        const char *desc;
        int (*fn)(int, char *[]);
};

static int rssi_to_ratio(char *rssi)
{
        int nRSSI=atoi(rssi);
        int signalStrength=0;

    if (nRSSI >= 40)
    {
        signalStrength = 100;// 40 dB SNR or greater is 100%
    }
    else if (nRSSI >= 20)// 20 dB SNR is 80%
    {
        signalStrength = nRSSI - 20;
        signalStrength = (signalStrength * 26) / 30;
        signalStrength += 80; // Between 20 and 40 dB, linearize between 80 and 100
    }
    else
    {
        signalStrength = (nRSSI * 75) / 20;
    }

    return signalStrength;
}

static void wireless_reformat(char *buffer, char *stalist)
{
        char tmp[12], compute[4],
             mac[24], mode[12],
             status[24], rate[12], signal[12];

        bzero(mac, sizeof(mac)); bzero(mode, sizeof(mode));
        bzero(status, sizeof(status)); bzero(rate, sizeof(rate));
        bzero(signal, sizeof(signal));

        sscanf(buffer, "%s %[^' '] %s %s %s %s %s %s %s",
                tmp, mac, mode, status, tmp, rate, tmp, tmp, tmp);

        sprintf(compute, "%d", (100 - atoi(tmp)));
        sprintf(signal, "%d", rssi_to_ratio(compute));
        strcpy(stalist, mac); strcat(stalist, "/");
        strcat(stalist, mode); strcat(stalist, "/");
        strcat(stalist, status); strcat(stalist, "/");
        strcat(stalist, rate); strcat(stalist, "/");
        strcat(stalist, signal);

        cprintf("stalist :: [%s]\n", stalist);
        stalist += 45;
}

#ifdef MVL
static int wireless_list_info(int argc, char *argv[])
{

	FILE *fp;
        char cmd[512], buffer[1024];
        char stalist[1024], arp[1024], res[512], res1[512];
        char *ptr_sta, *iface,
             addr[24], tmp[12], mac[24], dev[12];
        int flag = 0;

        bzero(cmd, sizeof(cmd));
        bzero(buffer, sizeof(buffer));
        memset(stalist, '\0', sizeof(stalist));
        memset(arp, '\0', sizeof(arp));
        memset(res, '\0', sizeof(res));
        memset(res1, '\0', sizeof(res1));

        ptr_sta = stalist;

        if((iface = nvram_safe_get("wlan0_eth")) == NULL){
                return -1;
	 }else{
                sprintf(cmd, "iwpriv %sap0 getstalistext > /tmp/tmp/wlan_stalist", iface);
                system(cmd);
                sprintf(cmd, "cat /tmp/tmp/wlan_stalist");

                if(!(fp = popen(cmd, "r"))){
                        return -1;
                }else{
                        while(fgets(buffer, sizeof(buffer), fp) != NULL){
                                if(!flag){
                                        flag = 1;
                                        continue;
                                }
                                if(strlen(buffer) < 2)
                                        break;
                                wireless_reformat(buffer, stalist);

                                if(strlen(res1) == 0){
                                        sprintf(res1, "%s", stalist);
                                }else{
                                        sprintf(res1, "%s, %s", res1,stalist);
                                }
                        }
                        pclose(fp);
                }
		flag = 0;
                sprintf(cmd, "cat /proc/net/arp");
                if(!(fp = popen(cmd, "r"))){
                        return -1;
                }else{
                        while(fgets(buffer, sizeof(buffer), fp)){
                                if(!flag){
                                        flag = 1;
                                        continue;
                                }
                                sscanf(buffer, "%s %s %s %s %s %s",
                                addr, tmp, tmp, mac, tmp, dev);
                                if(strcmp(dev, "br0") != 0)
                                        continue;
                                else{
                                        strcpy(stalist, res1);
                                        ptr_sta = res1;
                                        char *ptr;
                                        while((ptr = strsep(&ptr_sta, ","))){
                                                char *p = NULL;
                                                p = strstr(ptr, mac);
                                                if(p != NULL){
                                                        sprintf(arp, "%s/%s", ptr, addr);
                                                        if(strlen(res) == 0){
                                                                sprintf(res, "%s", arp);
                                                        }else{
                                                                sprintf(res, "%s, %s", res,arp);
                                                        }
                                                }
                                        }
                                        strcpy(res1, stalist);
                                }
    			bzero(buffer, sizeof(buffer));
                        }
		pclose(fp);
                }
        }
	cprintf("wireless channel list:: %s", res);
        printf("%s", res);
	return 0;
}
#endif

#ifdef ATH
static int wireless_list_info(int argc, char *argv[])
{
        FILE *fp,*fp1;
        char *ptr, *rssi_ptr, *rate_ptr,*nego_rate,*ptr1;
        char buff[1024], addr[20], rssi[4], mode[16], time[16],rate[8], ip_addr[16],arpaddr[16],buff1[1024];
        int i;
        char *ptr_htcaps = NULL;
        int sta_is_n_mode = 0;
	struct dhcpd_leased_table_s dhcpd_list[M_MAX_DHCP_LIST];

        memset(dhcpd_list, '\0', sizeof(dhcpd_list));

        get_dhcpd_leased_table(dhcpd_list);
        if (NVRAM_MATCH("wlan0_enable", "0") != 0)
                return NULL;

	_system("wlanconfig %s list sta > /var/misc/stalist.txt", WLAN0_ETH);
        if (NVRAM_MATCH("wlan0_vap1_enable", "1") != 0) {
		system("wlanconfig ath1 list sta >> /var/misc/stalist.txt"); //2.4GHz GuestZone
        }

        if ((fp=fopen("/var/misc/stalist.txt","rb")) == NULL) {
                cprintf("stalist file open failed (No station connected). \n");
                return -1;
        }

        while (fgets(buff, 1024, fp)) {
		if (strstr(buff, "ID CHAN RATE RSSI IDLE"))
		    continue;
                ptr_htcaps = NULL;
                sta_is_n_mode = 0;
		ptr = buff;
/*UAPSD QoSInfo: 0x0f, (VO,VI,BE,BK) = (1,1,1,1), MaxSpLimit = NoLimit*/
		if(strstr(buff, "UAPSD QoSInfo")) {
                  //      DEBUG_MSG("get_stalist(): Having VISTA Client QoSInfo.\n");
                        continue;
                }
		memset(addr, 0, 20);
		memset(rssi, 0, 4);
		memset(mode, 0, 16);
		memset(time, 0, 16);
		memset(rate, 0, 8);
		memset(ip_addr, 0, 16);

//00:13:46:11:6b:bd 1 4 54M 55 135 25 18464 ESs 0 27 open none Normal 00:00:56 12 WME
//%s %4u %4d %3dM %4d %4d %6d %6d %4s %5s %3x %8x %6s %7s %6s %-8.8s %-10d
//AR9100(AP81), AR7161(AP94)
//00:19:d2:36:f7:98 3 11 130M 62 120 451 96 ES 0 27 Q 01:27:06 12 WME
//%s %4u %4d %3dM %4d %4d %6d %6d %4s %5s %3x %8x %6s %-8.8s %-10d

		strncpy(addr, ptr, 17);
		ptr += strlen(addr);
		ptr += 5; /* skip one space + AID */
		ptr += 5; /* skip one space + CHAN */
		ptr += 1; /* skip one space */
		rate_ptr = ptr;
		while (*rate_ptr == ' ')
			rate_ptr++;
		strncpy(rate, rate_ptr, 4-(rate_ptr-ptr) );

		ptr += 4; /* skip RATE */
		/* check rate value is valid */
		char rate_val[10];
		strcpy(rate_val, rate);
		rate_val[strlen(rate) - 1] = 0;   /* del 'M' */
		if (atoi(rate_val) >= 300)
			strcpy(rate, "300M");
		else if (atof(rate_val) <= 5)
			strcpy(rate, "5.5M");

		ptr += 1; /* skip one space */
		rssi_ptr = ptr;

		while (*rssi_ptr == ' ')
			rssi_ptr++;
		strncpy(rssi, rssi_ptr, 4-(rssi_ptr-ptr) );

		ptr += 4; /* skip RSSI */
		ptr += 5; /* skip one space + IDLE */
		ptr += 7; /* skip one space + TXSEQ */
		ptr += 7; /* skip one space + RXSEQ */
		ptr += 5; /* skip one space + CAPS */
		ptr += 6; /* skip one space + ACAPS */
		ptr += 4; /* skip one space + ERP */
		ptr += 9; /* skip one space + STATE */

		ptr_htcaps = ptr + 1;
                ptr += 7; /* skip one space + HTCAPS */

		ptr += 1; /* skip one space */
		strncpy(time, ptr, 8);

		ptr += strlen(time);
		ptr += 1; /* skip one space */
		if(!strlen(time))
			strcpy(time, "0");

		nego_rate = ptr;
		while (*nego_rate != ' ')
                nego_rate++;
		/*NEGO_RATES = 12*/
		if (nego_rate - ptr == 2) {
			strcpy(mode, "802.11g");
                        while (*ptr_htcaps != ' ') {
                                switch(ptr_htcaps[0]) {
                                        case 'G':
                                        case 'M':
                                        case 'P':
                                        case 'S':
                                        case 'W':
                                                memset(mode,'\0',sizeof(mode));
                                                strcpy(mode, "802.11n");
                                                sta_is_n_mode = 1;
                                                break;
                                        default:
                                                break;
                                }
                                if(sta_is_n_mode){
                                        break;
                                }
                                ptr_htcaps++;
                        }
		} else if(nego_rate - ptr == 1) /*NEGO_RATES = 4*/
			strcpy(mode, "802.11b");

                _system("rm -rf /tmp/var/arpinfo.txt");
                sleep(1);
                _system("/bin/cat /proc/net/arp | grep '%s'| cut -d ' ' -f 1  >> /tmp/var/arpinfo.txt",addr);
                if ((fp1=fopen("/tmp/var/arpinfo.txt","rb")) == NULL) {
                        cprintf("arpinfo file open failed . \n");
                        return -1;
                }

                memset(buff1, 0, 1024);
                memset(arpaddr, 0, 16);
                strcpy(arpaddr, "0.0.0.0");
                while( fgets(buff1, 1024, fp1)) {
                        if(buff1) {
                                ptr1 = buff1;
                                memset(arpaddr, 0, 16);
                                strncpy(arpaddr, ptr1, 14);//192.168.0.100
                        }
                }
                fclose(fp1);
                strcpy(ip_addr,arpaddr);

		for ( i = 0; i < M_MAX_DHCP_LIST; i++) {
			if (dhcpd_list[i].ip_addr == NULL)
				break;
			if (strcmp(dhcpd_list[i].mac_addr, addr) == 0) {
					strcpy(ip_addr, dhcpd_list[i].ip_addr);
					break;
			}
		}
		printf("%s/%s/%s/%s/%d/%s, ", addr, mode, time, rate, rssi_to_ratio(rssi), ip_addr);// for GUI
        }
        fclose(fp);

        return 0;
}
#endif

static int misc_wireless_list_help(struct __wireless_struct *p)
{
        cprintf("Usage:\n");

        for (; p && p->param; p++)
                cprintf("\t%s: %s\n", p->param, p->desc);

        cprintf("\n");
        return 0;
}

static int get_domain()
{
	char *value=NULL;
        char buf[128];
        int domain=0x3a;

	/* XXX: nvram get instead of artblock get. We found a bug which
	 * two processes access flash simultaneous may cause kernel panic.
	 * In original, we use artblock get here, but chklst.txt has another
	 * flash accessing code. When user view the chklst.txt refresh faster,
	 * kernel panic maybe occur.
	 *
	 * 2010/9/2 Fredhung */
	value = buf;
	strcpy(buf, nvram_safe_get("sys_wlan0_domain"));

	if (*value == '\0')
		domain = 0x10;
	else
		sscanf(value, "%x", &domain);
#if 0
	if ((fp = popen("/bin/get_artblock wlan0_domain", "r")) != NULL) {
		fgets(buf, 127, fp);
		pclose(fp);
	}

	value = buf;

	if (value != NULL)
		 sscanf(value, "%x", &domain);
        else
                domain = 0x10;
#endif
	return domain;
}

static int wlan0_channel_list(int argc, char *argv[])
{
	char channel_list[128];
	int domain = 0x30;

	domain = get_domain();
 	GetDomainChannelList(domain, WIRELESS_BAND_24G, channel_list, 0);
	printf("%s", channel_list);

	return 0;
}

static int wlan1_channel_list(int argc, char *argv[])
{
	char channel_list[128];
	int domain = 0x30;

	domain = get_domain();
	GetDomainChannelList(domain, WIRELESS_BAND_50G, channel_list ,0);
	printf("%s", channel_list);
	return 0;
}


int get_wireless_list_main(int argc, char *argv[])
{
	int ret = -1;
        struct __wireless_struct *p, list[] = {
                { "wireless_station_list", "Show wireless connected status", wireless_list_info},
		{ "wlan0_channel_list", "show wlan0 channel list", wlan0_channel_list},
		{ "wlan1_channel_list", "show wlan1 channel list", wlan1_channel_list},
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
        ret = misc_wireless_list_help(list);
out:
        return ret;

}
