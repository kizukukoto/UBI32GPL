#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nvram.h>
#include <sutil.h>
#include "wlan.h"

extern void wlan_ath_setup(int , int);
extern void set_11bgn_rate(int, int, char *);
extern void set_11bg_rate(int, int, char *);
extern void set_11b_rate(int, int, char *);

/* Bridge to br0 interface with ahtX */
void start_activateVAP(int ath_count, int guest_zone, int netusb_guest_zone)
{
	char cmd[128];
	char *bridge = nvram_safe_get("lan_bridge");
	char *lan_ipadr = nvram_safe_get("lan_ipaddr");

	memset(cmd, '\0', sizeof(cmd));
	_system("ifconfig ath%d up", ath_count);
	sleep(1);
	_system("brctl addif %s ath%d", bridge, ath_count);
	//guest_zone=1 => Disable Disable Routing between LAN
	//netusb_guest_zone=1 => Guest Zone support NetWork USB Utility
	//setguestzone <bridge> <device>:<Enable_SharePort>:<Enable_RouteInLan> <bridge ip>
	if (guest_zone == 1) {
		if(netusb_guest_zone == 1)
			sprintf(cmd, "brctl setguestzone %s ath%d:1:0 %s",
				bridge, ath_count, lan_ipadr);
		else if(netusb_guest_zone == 0)
			sprintf(cmd, "brctl setguestzone %s ath%d:0:0 %s", 
				bridge, ath_count, lan_ipadr);
		else
			sprintf(cmd, "brctl setguestzone %s ath%d:1:0 %s", 
				bridge, ath_count, lan_ipadr);
	} else if (guest_zone == 0) {
		if (netusb_guest_zone == 1)
			sprintf(cmd, "brctl setguestzone %s ath%d:1:1 %s", 
				bridge, ath_count, lan_ipadr);
		else if (netusb_guest_zone == 0 )
			sprintf(cmd, "brctl setguestzone %s ath%d:0:1 %s", 
				bridge, ath_count, lan_ipadr);
		else
			sprintf(cmd, "brctl setguestzone %s ath%d:1:1 %s", 
				bridge, ath_count, lan_ipadr);
	}
	if (strlen(cmd) != 0)
		system(cmd);
}

static void create_wifi_dev(const char *op_mode, char *ifnum)
{
	char cmds[128];
	if(strcmp(op_mode, "AP")==0)
		sprintf(cmds, "wlanconfig ath create wlandev wifi%s wlanmode ap &", ifnum);
	else if(strcmp(op_mode, "APC")==0)
		sprintf(cmds, "wlanconfig ath create wlandev wifi%s wlanmode sta nosbeacon &", ifnum);
	system(cmds);
}

static void chk_Ch_mode(const char *Ch_Mode, char *ifnum, int ath)
{
	if (strstr(Ch_Mode, "11NG"))
		_system("iwpriv wifi%s ForBiasAuto 1", ifnum);

	if (strstr(Ch_Mode, "PLUS"))//channel 1~5
		_system("iwpriv ath%d extoffset 1", ath);

	if (strstr(Ch_Mode, "MINUS"))//channel 6~13
		_system("iwpriv ath%d extoffset -1", ath);

	/* 0 is static 20; 1 is dyn 2040; 2 is static 40 */
	if (strcmp(Ch_Mode, "11NGHT20") == 0 || strcmp(Ch_Mode, "11NAHT20") == 0)
		_system("iwpriv ath%d cwmmode 0", ath);
	else
		_system("iwpriv ath%d cwmmode 1", ath);

}

static void set_Freq(const char *op_mode, int ath, char *Freq)
{
	if (strcmp(op_mode, "AP") == 0)
		_system("iwconfig ath%d essid dlink mode master %s", ath, Freq);
	else if(strcmp(op_mode, "APC")==0)
		_system("iwconfig ath%d essid dlink mode managed %s", ath, Freq);
}

static void set_wds_status(int wds_en, const char *op_mode, int ath)
{
	if (wds_en) {
		_system("iwpriv ath%d wds 1", ath);
	} else {
		if(strcmp(op_mode, "APC") == 0)
			_system("iwpriv ath%d stafwd 1", ath);
	}
}

static int split_ifnum(char *pifnum, char *ifnum, 
			char *RF, char *PriCh, 
			char *Chmode, char *puremode)
{
	char *Ifnum = NULL, *set_RF = NULL, *Pri_Ch = NULL, 
		*Ch_Mode = NULL, *pure_mode = NULL, ap_ifnum[30];

	sprintf(ap_ifnum, "%s", pifnum);
	Ifnum = strtok(ap_ifnum, ":");
	set_RF = strtok(NULL, ":");
	Pri_Ch = strtok(NULL, ":");
	Ch_Mode = strtok(NULL, ":");
	pure_mode = strtok(NULL, ":");

	if (Ifnum == NULL || Pri_Ch == NULL || Ch_Mode == NULL) {
		printf("start_makeVAP error !!\n");
		return 1;
	}
	strcpy(ifnum, Ifnum);
	strcpy(RF, set_RF);
	strcpy(PriCh, Pri_Ch);
	strcpy(Chmode, Ch_Mode);
	strcpy(puremode, pure_mode);
	return 0;
}

static void set_puremode(const char *pure_mode, int ath)
{
	if (atoi(pure_mode)<2) { /* 2 => do not set pure mode */
		if (atoi(pure_mode) == 0)
			_system("iwpriv ath%d pureg 1", ath);
		else //if (atoi(pure_mode) == 1)
			_system("iwpriv ath%d puren 1", ath);
	}
}

static void set_rate(char *dot11_mode, int ath, int gidx)
{
	if (strstr(dot11_mode, "n"))
		set_11bgn_rate(gidx, ath, dot11_mode);
	else if (strstr(dot11_mode, "g"))
		set_11bg_rate(gidx, ath, dot11_mode);
	else if (strstr(dot11_mode, "b"))
		set_11b_rate(gidx, ath, dot11_mode);
	else 
		set_11bgn_rate(gidx, ath, dot11_mode);
}

static void set_wifi(const char *Ifnum, int ath)
{
	_system("ifconfig ath%d txqueuelen 250", ath);
	_system("ifconfig wifi%s txqueuelen 250", Ifnum);
	_system("iwpriv wifi%s AMPDU 1", Ifnum);
	_system("iwpriv wifi%s AMPDUFrames 32", Ifnum);
	_system("iwpriv wifi%s txchainmask 3", Ifnum);
	_system("iwpriv wifi%s rxchainmask 3", Ifnum);
	_system("iwpriv wifi%s AMPDULim 50000", Ifnum);
	system("echo 1 > /proc/sys/dev/ath/htdupieenable");
}

void start_makeVAP(wlan *w, const char *op_mode)
{
	char Ifnum[24], set_RF[24], Pri_Ch[24], Ch_Mode[24], pure_mode[24],
	     Freq[10], dot11_mode[12];
	int ath = 0, gidx = 0;
	
	memset(Ifnum, '\0', sizeof(Ifnum));
	memset(set_RF, '\0', sizeof(set_RF));
	memset(Pri_Ch, '\0', sizeof(Pri_Ch));
	memset(Ch_Mode, '\0', sizeof(Ch_Mode));
	memset(pure_mode, '\0', sizeof(pure_mode));
	memset(Freq, '\0', sizeof(Freq));
	memset(dot11_mode, '\0', sizeof(dot11_mode));

	sprintf(dot11_mode, "%s", w->channel_mode);
	for (;w != NULL; w = w->next) {
		ath = w->ath;
		gidx = w->gidx;
		if (split_ifnum(w->ifnum, Ifnum, set_RF, Pri_Ch, Ch_Mode, pure_mode)) {
			printf("XX Error ifnum :: %s\n", w->ifnum);
			break;
		}
		create_wifi_dev(op_mode, Ifnum);
		sleep(1);
		set_wds_status(w->wds_en, op_mode, ath);
		_system("iwpriv ath%d dbgLVL 0x100", ath);
		_system("iwpriv ath%d bgscan 0", ath); //Disable Background Scan
		if (strcmp(Pri_Ch, "11na") && strcmp(Pri_Ch, "11ng"))
			sprintf(Freq, "freq %s", Pri_Ch);
		if (strcmp(set_RF, "RF") == 0) { // only 2.4G or 5G will come in ..
			_system("iwpriv ath%d mode %s", ath, Ch_Mode);
			set_puremode(pure_mode, ath);
			chk_Ch_mode(Ch_Mode, Ifnum, ath);
			set_Freq(op_mode, ath, Freq);
			set_rate(dot11_mode, ath, gidx);
			set_wifi(Ifnum, ath);
			if (strcmp(Ifnum, "0") == 0) /* Like 2.4G */
				wlan_ath_setup(0, ath);
			else if (strcmp(Ifnum, "1") == 0) /* 5G */
				wlan_ath_setup(1, ath);
		} else {
			_system("iwpriv ath%d mode %s", ath, Ch_Mode);
			_system("iwconfig ath%d essid dlink mode master %s", ath, Freq);
		}
	}
}
