#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/klog.h>
#include "cmds.h"

#if 0
#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt, ## args); \
                fclose(fp); \
        } \
} while (0)
#else
#define cprintf(fmt, args...)
#endif

#define SYSLOG(fmt, ...) syslog(LOG_INFO, fmt, ## __VA_ARGS__)
#define IWLIST "/var/misc/wlanlist.txt"
#define IWCONF "/var/misc/iwconfig.txt"
/* @command : wlanconfig ath0 list sta 
 * @data:
 * ADDR               AID CHAN RATE RSSI IDLE  TXSEQ  RXSEQ CAPS ACAPS ERP    STATE HTCAPS   A_TIME NEGO_RATES
 * 00:16:6f:48:fc:d7    1    6  54M   62  165     22  11920 ESs          0       27 Q      00:02:14 12         WME
 */

static int get_wireless_info(const char *iface, const char *cmac, char *_atime) 
{
	FILE *fp;
	char buff[1024], mac[18], atime[9], *tmp, *ptr;
	int ret = 0;

	memset(buff, '\0', sizeof(buff));
	memset(mac, '\0', sizeof(mac));
	memset(atime, '\0', sizeof(atime));

	_system("wlanconfig %s list sta > %s", iface, IWLIST);

	if ((fp = fopen(IWLIST, "rb")) == NULL)
		goto out;

	while (fgets(buff, 1024, fp)) {
		if (strstr(buff, "ID CHAN RATE RSSI IDLE"))
		    continue;
		ptr = buff;
		memset(mac, '\0', sizeof(mac));
		memset(atime, '\0', sizeof(atime));
		tmp = strstr(ptr, " ");
		strncpy(mac, ptr, strlen(ptr)-strlen(tmp)); // get mac
		cprintf("%s(%d) : %s(%d)\n", mac,strlen(mac), cmac, strlen(cmac));
		if (strcasecmp(mac, cmac) != 0)
			continue;
		ptr += strlen(mac);
		tmp = strstr(ptr, ":");
		ptr += strlen(ptr) - strlen(tmp) - 2;
		tmp = strstr(ptr, " ");
		strncpy(atime, ptr, strlen(ptr) - strlen(tmp)); // get association time
		break;
	}
	fclose(fp);
	if (strlen(atime) != 0) {
		strcpy(_atime, atime);
		ret = 1;
	}
out:
	return ret;
}

static int get_ssid(const char *iface, char *_ssid)
{
	FILE *fp;
	char buff[1024], ssid[256], *tmp, *tmp1, *ptr;
	int ret = 0;
	
	memset(buff, '\0', sizeof(buff));
	memset(ssid, '\0', sizeof(ssid));
	
	_system("iwconfig %s > %s", iface, IWCONF);

	if ((fp = fopen(IWCONF, "rb")) == NULL)
		goto out;

	if (fgets(buff, 1024, fp) == NULL)
		goto out;
	else {
		ptr = buff;
		tmp = strstr(ptr, "SSID:");
		tmp1 = strstr(ptr, "Nickname:");
		tmp += 6; //remove 'SSID:"'
		strncpy(ssid, tmp, strlen(tmp) - strlen(tmp1));
		ssid[strlen(ssid)-3] = '\0';
		fclose(fp);
		ret = 1;
		strcpy(_ssid, ssid);
	}
out:
	return ret;
}

static int record_info_main(int argc, char *argv[])
{
	char iface[12], cmac[18], cip[18], atime[9], ssid[256];
	int i = 0, flag = 0;
	
	memset(ssid, '\0', sizeof(ssid));
	memset(atime, '\0', sizeof(atime));

	/* Get mac and ip address from env */
	sprintf(cmac, "%s", getenv("dhmac"));
	sprintf(cip, "%s", getenv("dhip"));
	cprintf("mac :: %s\n", cmac);
	cprintf("ip :: %s\n", cip);

	for (i = 0; i < 4; i++) {
		sprintf(iface, "ath%d", i);
		if (!(flag = get_wireless_info(iface, cmac, atime))) {
			cprintf("no match mac");
			continue;
		}

		if (!(flag = get_ssid(iface, ssid))) {
			cprintf("no ssid");
			continue;
		}
		if (flag)
			break;
		unlink(IWLIST);
		unlink(IWCONF);
	}

	if (flag) {
		cprintf("Association Time %s - MAC %s - IP Address %s - SSID %s\n", atime, cmac, cip, ssid);
		SYSLOG("Association Time %s - MAC %s - IP Address %s - SSID %s\n", atime, cmac, cip, ssid);
	} else {
		SYSLOG("MAC %s - IP Address %s\n", cmac, cip);
		cprintf("MAC %s - IP Address %s\n", cmac, cip);
	}

	return 0;
}

int dhcprelay_info_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "info", record_info_main},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
