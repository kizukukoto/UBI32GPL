#include <stdio.h>
#include <string.h>

#include "nvram.h"
#include "debug.h"

int check_wps_main(int argc, char *argv[])
{
	FILE *fp;
	char wps_status[6];

	memset(wps_status, '\0', sizeof(wps_status));

	if ((fp = popen("cat /var/tmp/wps_tmp", "r")) == NULL)
		return -1;
	
	fgets(wps_status, 2, fp);

	if (atoi(wps_status) == 1) {/* Fail */
		setenv("wps_push_button_result", "0", 1);
		printf("%d", atoi(wps_status));
	} else { /* Success */
		setenv("wps_push_button_result", "1", 1);
		printf("%d", atoi(wps_status));
	}
	pclose(fp);

	cprintf("wps_status :: %d\n", atoi(wps_status));
	return 0;
}
