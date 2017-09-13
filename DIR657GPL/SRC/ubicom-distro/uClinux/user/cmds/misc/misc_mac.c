#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nvram.h"

int misc_getmac_main(int argc, char *argv[])
{
	int idx;
	int rev = -1;
	int mac[128];

	if (argc != 2)
		return -1;

	bzero(mac, sizeof(mac));

	get_mac(nvram_get("wlan0_eth"), mac);
	printf("%s", mac);

	rev = 0;
out:
	return rev;
}
