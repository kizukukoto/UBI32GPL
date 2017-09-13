#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ssi.h"
#include "libdb.h"
#include "debug.h"

struct __wps_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int misc_wps_help(struct __wps_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

static int generate_pin_by_random(int argc, char *argv[])
{
        unsigned long int rnumber = 0;
        unsigned long int pin = 0;
        unsigned long int accum = 0;
        int digit = 0;
        int checkSumValue =0 ;
	char wps_pin[32];

	memset(wps_pin, '\0', sizeof(wps_pin));

        //generate random 7 digits
        srand((unsigned int)time(NULL));
        rnumber = rand()%10000000;
        pin = rnumber;

        //Compute the checksum
        rnumber *= 10;
        accum += 3 * ((rnumber / 10000000) % 10);
        accum += 1 * ((rnumber / 1000000) % 10);
        accum += 3 * ((rnumber / 100000) % 10);
        accum += 1 * ((rnumber / 10000) % 10);
        accum += 3 * ((rnumber / 1000) % 10);
        accum += 1 * ((rnumber / 100) % 10);
        accum += 3 * ((rnumber / 10) % 10);
        digit = (accum % 10);
        checkSumValue = (10 - digit) % 10;

        pin = pin*10 + checkSumValue;
        sprintf( wps_pin, "%08d", pin );
	printf("%s", wps_pin);
}

static int get_default_pin(int argc, char *argv[])
{
        unsigned long int rnumber = 0;
        unsigned long int pin1 = 0,pin2 = 0;
        unsigned long int accum = 0;
        int i,digit = 0;
        char wan_mac[32], wps_pin[32];
        char *mac_ptr = NULL;

	memset(wan_mac, '\0', sizeof(wan_mac));
	memset(wps_pin, '\0', sizeof(wps_pin));

	mac_ptr = nvram_safe_get("wan_mac");
        cprintf("mac_ptr=%s\n", mac_ptr);
        strncpy(wan_mac, mac_ptr, 18);
        mac_ptr = wan_mac;
        //remove ':'
        for(i =0 ; i< 5; i++ ) 
                memmove(wan_mac+2+(i*2), wan_mac+3+(i*3), 2);

        memset(wan_mac+12, 0, strlen(wan_mac)-12);
        sscanf(wan_mac,"%06X%06X",&pin1,&pin2);

        pin2 ^= 0x55aa55;
        pin1 = pin2 & 0x00000F;
        pin1 = (pin1<<4) + (pin1<<8) + (pin1<<12) + (pin1<<16) + (pin1<<20) ;
        pin2 ^= pin1;
        pin2 = pin2 % 10000000; // 10^7 to get 7 number

        if( pin2 < 1000000 )
                pin2 = pin2 + (1000000*((pin2%9)+1)) ;

        //ComputeChecksum
        rnumber = pin2*10;
        accum += 3 * ((rnumber / 10000000) % 10);
        accum += 1 * ((rnumber / 1000000) % 10);
        accum += 3 * ((rnumber / 100000) % 10);
        accum += 1 * ((rnumber / 10000) % 10);
        accum += 3 * ((rnumber / 1000) % 10);
        accum += 1 * ((rnumber / 100) % 10);
        accum += 3 * ((rnumber / 10) % 10);
        digit = (accum % 10);
        pin1 = (10 - digit) % 10;

        pin2 = pin2 * 10;
        pin2 = pin2 + pin1;
        sprintf(wps_pin, "%08d", pin2 );
        printf("%s", wps_pin);
	return 0;
}

int wps_pin_generate_main(int argc, char *argv[])
{
	int ret = -1;
	struct __wps_struct *p, list[] = {
		{ "default_pin", "Get WPS Default PIN", get_default_pin},
		{ "random_pin", "Get WPS Random PIN", generate_pin_by_random},
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
	ret = misc_wps_help(list);
out:
	return ret;
}
