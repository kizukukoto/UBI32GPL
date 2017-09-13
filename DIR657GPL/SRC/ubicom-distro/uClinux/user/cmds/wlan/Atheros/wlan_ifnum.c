#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nvram.h>
#include "wlan.h"

static void init_24G_scan_module(const char *mode)
{
	if (strcmp(mode, "APC") == 0)
		system("insmod /lib/modules/wlan_scan_sta.ko");
	else if (strcmp(mode, "AP") == 0)
		system("insmod /lib/modules/wlan_scan_ap.ko");
}

static void set_5G_AP(char *Ap_Primary_Ch, char *Ap_Chmode, wlan *w)
{
	if (w->auto_channel) {
		strcpy(Ap_Primary_Ch, "11na");
		system("insmod /lib/modules/wlan_scan_ap.ko");
	} else if ((atoi(w->channel)>35) && (atoi(w->channel)<166)) {
		/* HT40MHz*/
		/* PLUS  => 36, 44, 52, 60, 100, 108, 116, 124, 132, 149, 157 */
		/* MINUS => 40, 48, 56, 64, 104, 112, 120, 128, 136, 153, 161 */
		/* HT20MHz => 165 */
		strcpy(Ap_Primary_Ch, w->channel);
	} else /*default*/
		strcpy(Ap_Primary_Ch, "40");

	if ((strstr(w->channel_mode, "n"))) { // N mode and include a mode ex:11na, 11n
		if (w->auto_channel) {
			if (strcmp(w->protection, "auto") == 0)
				strcpy(Ap_Chmode, "11NAHT40");
			else
				strcpy(Ap_Chmode, "11NAHT20");
		} else {
		 	if (strcmp(w->protection, "auto") != 0) {
		 		strcpy(Ap_Chmode, "11NAHT20");
		 	} else if (((atoi(w->channel)-28)%8 == 0) ||
		    		 (atoi(w->channel)==149) ||
		    		 (atoi(w->channel)==153) ||
		    		 (atoi(w->channel)==157)
		    		) {
		    	//36, 44, 52, 60, 100, 108, 116, 124, 132, 149, 153, 157
			    	strcpy(Ap_Chmode, "11NAHT40PLUS");
		   	} else if(atoi(w->channel) == 165)
		    		strcpy(Ap_Chmode, "11NAHT20");
		        else {
		       	//40, 48, 56, 64, 104, 112, 120, 128, 136, 161
		       		strcpy(Ap_Chmode, "11NAHT40MINUS");
		        }

		}
	} else if (strcmp(w->channel_mode, "11a") == 0)
		strcpy(Ap_Chmode, "11A");
	else /*default*/
		strcpy(Ap_Chmode, "11NAHT40MINUS");
}

static void set_24G_AP(char *Ap_Primary_Ch, char *Ap_Chmode, wlan *w)
{
	if (w->auto_channel)
		strcpy(Ap_Primary_Ch, "11ng");
	else
		strcpy(Ap_Primary_Ch, w->channel);

	if ((strstr(w->channel_mode, "n"))) { //Include 11n mode
		if (w->auto_channel) {
			if (strcmp(w->protection, "auto"))
				strcpy(Ap_Chmode, "11NGHT20");
			else
				strcpy(Ap_Chmode, "11NGHT40");
		} else {
			if (strcmp(w->protection, "auto")) {
			    if (atoi(w->channel) < 6)
			    	strcpy(Ap_Chmode, "11NGHT40PLUS");
			    else
			        strcpy(Ap_Chmode, "11NGHT40MINUS");
			} else
				strcpy(Ap_Chmode, "11NGHT20");
		}
	} else if (strstr(w->channel_mode, "g")) // Include g mode and no n mode
		strcpy(Ap_Chmode, "11G");
	else // Only b mode
		strcpy(Ap_Chmode, "11B");
}

static void set_24G_ifnum(wlan *w, char *Primary, char *Chmode, int gidx)
{
	char ifnum[30];

	if (!strcmp(w->channel_mode, "11n")) //set pure n
		sprintf(ifnum, "0:%s:%s:%s:1", 
		(gidx == 0) ? "RF" : "NO", Primary, Chmode);
	else if (!strcmp(w->channel_mode, "11g") || 
		!strcmp(w->channel_mode, "11gn")) //set pure g
		sprintf(ifnum, "0:%s:%s:%s:0",
		(gidx == 0) ? "RF" : "NO", Primary, Chmode);
	else
		sprintf(ifnum, "0:%s:%s:%s:2", 
		(gidx == 0) ? "RF" : "NO", Primary, Chmode);
	strcpy (w->ifnum, ifnum); // ifnum
}

static void set_5G_ifnum(wlan *w, char *Primary, char *Chmode, int gidx)
{
	char ifnum[30];
	if (strcmp(w->channel_mode, "11n") == 0) {
		sprintf(ifnum, "1:%s:%s:%s:1", 
			(gidx == 0) ? "RF" : "NO", Primary,  Chmode);
	} else {
		sprintf(ifnum, "1:%s:%s:%s:2",
			(gidx == 0) ? "RF" : "NO", Primary,  Chmode);
	}
}

void init_wlan0_if(char *op_mode, wlan *w)
{
	char Ap_Chmode[30], Ap_Primary_Ch[30], ifnum[30];

	memset(Ap_Chmode, '\0', sizeof(Ap_Chmode));
	memset(Ap_Primary_Ch, '\0', sizeof(Ap_Primary_Ch));
	memset(ifnum, '\0', sizeof(ifnum));

	init_24G_scan_module(op_mode);
	set_24G_AP(Ap_Primary_Ch, Ap_Chmode, w);
	for (;w != NULL; w = w->next) {
		set_24G_ifnum(w, Ap_Primary_Ch, Ap_Chmode, w->gidx);
	}
}

void init_wlan1_if(wlan *w)
{
	char Ap_Chmode[30], Ap_Primary_Ch[30], ifnum[30];

	memset(Ap_Chmode, '\0', sizeof(Ap_Chmode));
	memset(Ap_Primary_Ch, '\0', sizeof(Ap_Primary_Ch));
	memset(ifnum, '\0', sizeof(ifnum));

	set_5G_AP(Ap_Primary_Ch, Ap_Chmode, w);
	for (;w != NULL; w = w->next) {
		set_5G_ifnum(w, Ap_Primary_Ch, Ap_Chmode, w->gidx);
	}
}
