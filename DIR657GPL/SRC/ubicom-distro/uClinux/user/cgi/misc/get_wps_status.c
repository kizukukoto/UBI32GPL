#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ssi.h"
#include "libdb.h"
#include "debug.h"

struct __wlan_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int misc_wlan_help(struct __wlan_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

static int get_wps_status(int argc, char *argv[])
{
    FILE *fp, *fp_wps;
    unsigned char wps_enable[5];
    unsigned char cmd_buf[128];
    char buf[1024];

    memset(buf, '\0', sizeof(buf));
    memset(cmd_buf, '\0', sizeof(cmd_buf));

    _system("iwconfig %s > /var/wlan_config.txt", "ath0");

    if ((fp = fopen("/var/wlan_config.txt", "r")) == NULL)
        return 1;

    while(!feof(fp)) {
        if (fgets(cmd_buf, 128, fp))
        	sprintf(buf, "%s", cmd_buf);
    }
    fclose(fp);

    //show_wps_push_button_status
    //system("cat /proc/sys/dev/ath/wps_enable > /var/wps_tmp");
    if ((fp_wps = fopen("/var/tmp/wps_tmp", "r")) == NULL)
        sprintf(buf, "wps_enable=0 ");
    else {
        while(!feof(fp_wps))
        {
            if (fgets(wps_enable, 5, fp_wps))
                sprintf(buf, "wps_enable=%s", wps_enable);
        }
        fclose(fp_wps);
    }
    printf("%s", buf);
    return 0;
}

int wps_status_main(int argc, char *argv[])
{
	int ret = -1;
	struct __wlan_struct *p, list[] = {
		{ "wps", "Get wps status", get_wps_status},
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
	ret = misc_wlan_help(list);
out:
	return ret;
}
