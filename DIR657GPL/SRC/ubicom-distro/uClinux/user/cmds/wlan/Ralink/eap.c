#include <stdio.h>
#include <string.h>
#include <nvram.h>

#define RT2880APD "/var/run/rt2880apd.pid"

static int check_cmd(const char *iface, char *dev)
{
	int res = -1;
	if (iface == NULL) {
		printf("No interface!!\n" \
		       "Usage : cmds eap start interface[ra00_0|ra01_1]\n");
		goto out;
	} else {
		if (!strcmp(iface, "ra00_0")) {
			res = 1;
			strcpy(dev, "ra00_0");
		} else if (!strcmp(iface, "ra01_0")){
			res = 0;
			strcpy(dev, "ra01_0");
		} else {
			printf("The interface is error, please re-input!\n");
			goto out;
		}
	}
	return res;
out:
	return -1;

}

/*
 * @wlan express wireless interface
 * When wlan is 0, it express 2.4G.
 * Or, it express 5G.
 */
int start_rt2880apd(int argc, char *argv[])
{
	int iface = 0, ret = -1;
        char wlan_security[128], enable[128], wlan_vap1_security[128];
	char dev[24];
	
	memset(dev, '\0', sizeof(dev));

	if ((iface = check_cmd(argv[1], dev)) == -1)
		goto out;

        sprintf(enable, "wlan%d_enable", iface);
        sprintf(wlan_security, "wlan%d_security", iface);
        sprintf(wlan_vap1_security, "wlan%d_vap1_security", iface);

        if (((strcmp(nvram_safe_get(wlan_security), "wpa_eap") == 0) || \
             (strcmp(nvram_safe_get(wlan_security), "wpa2_eap") == 0) || \
             (strcmp(nvram_safe_get(wlan_security), "wpa2auto_eap") == 0) || \
             (strcmp(nvram_safe_get(wlan_vap1_security), "wpa_eap") == 0) || \
             (strcmp(nvram_safe_get(wlan_vap1_security), "wpa2_eap") == 0) || \
             (strcmp(nvram_safe_get(wlan_vap1_security), "wpa2auto_eap") == 0))\
          && (strcmp(nvram_safe_get(enable), "1") == 0)) {
                _system("rt2880apd -i %s &", (iface == 0) ? "2" : "1");
        }
	ret = 0;
out:
	return ret;
}

int stop_rt2880apd(int argc, char *argv[])
{
	int ret = -1;
	FILE *fp = NULL;
        char pid[12], path[128], tmp_pid[12], dev[24];

	memset(path, '\0', sizeof(path));
        memset(pid, '\0', sizeof(pid));
        memset(dev, '\0', sizeof(dev));

	if ((check_cmd(argv[1], dev)) == -1)
		goto out;
        
        sprintf(path, "%s.%s", RT2880APD, dev);
        fp = fopen(path, "r");

        if (fp == NULL)
                goto out;

	fgets(tmp_pid, sizeof(tmp_pid), fp);
        fclose(fp);
	strncpy(pid, tmp_pid, strlen(tmp_pid));
        _system("kill -9 %s", pid);
	ret = 0;
out:
	return ret;
}
