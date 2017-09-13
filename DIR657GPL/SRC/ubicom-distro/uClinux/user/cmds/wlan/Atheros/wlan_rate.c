#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sutil.h>
#include <nvram.h>

static int bg_table[] = {54,48,36,24,18,12,9,6,11,5.5,2,1};
static int bg_rate_table[] = {12,8,13,9,14,10,15,11,24,25,26,27};
void set_11bgn_rate(int gidx, int ath_count, char *dot_mode)
{
	char *wlan0_11bgn_txrate, *mcs_start, *mcs_end,mcs_c[4], bgn_txrate[50];
	int mcs_i=0,mcs_i_next=0,len=0,i;

	if (gidx != 0) {
		sprintf(bgn_txrate, "wlan0_vap%d_11bgn_txrate", gidx);
		wlan0_11bgn_txrate = nvram_safe_get(bgn_txrate);
	} else {
		if (strcmp(dot_mode, "11bgn") == 0)
			wlan0_11bgn_txrate = nvram_safe_get("wlan0_11bgn_txrate");
		else if (strcmp(dot_mode, "11gn") == 0)
			wlan0_11bgn_txrate = nvram_safe_get("wlan0_11gn_txrate");
		else
			wlan0_11bgn_txrate = nvram_safe_get("wlan0_11n_txrate");
	}

	if (strcmp(wlan0_11bgn_txrate, "Best (automatic)") == 0) {
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
	} else if (!memcmp(wlan0_11bgn_txrate, "MCS", 3)) {
		memset(bgn_txrate, 0, 50);
		memset(mcs_c, 0, 4);
		mcs_start = wlan0_11bgn_txrate + 4;
		mcs_end = strstr(mcs_start, "-");
		if (mcs_end) {
			len = (unsigned long) mcs_start - (unsigned long) mcs_end - 1;
			len = 2;
			memcpy(mcs_c, mcs_start, len);
			mcs_i = 0x80 | atoi(mcs_c);
			if(mcs_i != 0x80) 
				mcs_i_next = mcs_i - 1;
			else 
				mcs_i_next = mcs_i;
			sprintf(bgn_txrate, "iwpriv ath%d set11NRates 0x%02X%02X%02X%02X", ath_count, mcs_i, mcs_i, mcs_i_next, mcs_i_next);
			_system(bgn_txrate);
			_system("iwpriv ath%d set11NRetries 0x04040404", ath_count);
			_system("iwpriv ath%d shortgi 0", ath_count);
			_system("iwpriv ath%d cwmmode 0", ath_count);
		}
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
	} else {
		mcs_i = atoi(wlan0_11bgn_txrate);
		for (i = 0; i < 12; i++) {
			if(mcs_i == bg_table[i]) 
				break;
		}
		if ((i == 12)&&(mcs_i != 1)) 
			mcs_i = 0;

		if (mcs_i != 0) {
			mcs_i = bg_rate_table[i];
			memset(bgn_txrate, 0, 50);
			sprintf(bgn_txrate, "iwpriv ath%d set11NRates 0x%02X%02X%02X%02X", ath_count, mcs_i, mcs_i, mcs_i, mcs_i);
			//DEBUG_MSG("set_11bgn_rate: set rate - %s\n",bgn_txrate);
			_system(bgn_txrate);
			_system("iwpriv ath%d set11NRetries 0x04040404", ath_count);
			_system("iwpriv ath%d shortgi 0", ath_count);
			_system("iwpriv ath%d cwmmode 0", ath_count);
		} else {
			//DEBUG_MSG("11bgn_txrate value error\n");
		}
		if (mcs_i == 1)
			_system("iwpriv ath%d mcast_rate 1000", ath_count);
		else
			_system("iwpriv ath%d mcast_rate 2000", ath_count);
	}
}

void set_11bg_rate(int gidx, int ath_count, char *dot_mode)
{
	char *wlan0_11bg_txrate, bg_txrate[30];
	int mcs_i = 0, i;

	if (gidx != 0) {
		sprintf(bg_txrate, "wlan0_vap%d_11bg_txrate", gidx);
		wlan0_11bg_txrate = nvram_safe_get(bg_txrate);
	} else {
		if (strcmp(dot_mode, "11bg")==0)
			wlan0_11bg_txrate = nvram_safe_get("wlan0_11bg_txrate");
		else
			wlan0_11bg_txrate = nvram_safe_get("wlan0_11g_txrate");
	}

	//DEBUG_MSG("set_11bg_rate: rate is %s\n",wlan0_11bg_txrate);
	if (strcmp(wlan0_11bg_txrate, "Best (automatic)") == 0) {
		//DEBUG_MSG("set_11bg_rate: rate = auto\n");
		//_system("iwconfig ath%d rate auto", ath_count);
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
	} else {
		mcs_i = atoi(wlan0_11bg_txrate);
		for ( i = 0; i < 12; i++) {
			if(mcs_i == bg_table[i]) 
				break;
		}
		if ((i == 12)&&(mcs_i != 1)) 
			mcs_i = 0;

		if (mcs_i != 0) {
			memset(bg_txrate, 0, 30);
			sprintf(bg_txrate, "iwconfig ath%d rate %dM", ath_count, mcs_i);
			//DEBUG_MSG("set_11bg_rate: set rate - %s\n",bg_txrate);
			_system(bg_txrate);
		} else 	{
			//DEBUG_MSG("11bg_txrate value error\n");
			//_system("iwconfig ath%d rate auto", ath_count);
		}
		if (mcs_i == 1)
			_system("iwpriv ath%d mcast_rate 1000", ath_count);
		else
			_system("iwpriv ath%d mcast_rate 2000", ath_count);
	}
}

void set_11b_rate(int gidx, int ath_count, char *dot_mode)
{
	char *wlan0_11b_txrate = NULL;
	char b_txrate[30];

	if (gidx != 0) {
		sprintf(b_txrate, "wlan0_vap%d_11b_txrate", gidx);
		wlan0_11b_txrate = nvram_safe_get(b_txrate);
	} else
		wlan0_11b_txrate = nvram_safe_get("wlan0_11b_txrate");

//	DEBUG_MSG("set_11b_rate: rate is %s\n",wlan0_11b_txrate);
	if (strcmp(wlan0_11b_txrate, "Best (automatic)") == 0) {
//		DEBUG_MSG("set_11b_rate: rate = auto\n");
		_system("iwconfig ath%d rate auto", ath_count);
	} else if (strcmp(wlan0_11b_txrate, "11") == 0)
		_system("iwconfig ath%d rate 11M", ath_count);
	else if (strcmp(wlan0_11b_txrate, "5.5") == 0)
		_system("iwconfig ath%d rate 5.5M", ath_count);
	else if (strcmp(wlan0_11b_txrate, "2") == 0)
		_system("iwconfig ath%d rate 2M", ath_count);
	else if (strcmp(wlan0_11b_txrate, "1") == 0)
		_system("iwconfig ath%d rate 1M", ath_count);
	else {
	//	DEBUG_MSG("11b_txrate value error\n");
		_system("iwconfig ath%d rate 11M", ath_count);
	}
	if (strcmp(wlan0_11b_txrate, "1") == 0)
		_system("iwpriv ath%d mcast_rate 1000", ath_count);
	else
		_system("iwpriv ath%d mcast_rate 2000", ath_count);
}

