#ifndef DIRX30
#include <stdlib.h>
#include "ssc.h"
#include "public.h"
#include "debug.h"
#include "ssi.h"
#include "shvar.h"

void *wizard_ipv6(struct ssc_s *obj)
{
	char reboot_type[10] = {};
	unlink(WIZARD_IPV6);
	if(getenv("reboot_type") != NULL){
		strcpy(reboot_type, getenv("reboot_type"));
		if(strcmp(reboot_type, "all") == 0)
			rc_restart();
	}
	cprintf("reboot_type = %s\n",reboot_type);

        return get_response_page();
}
#endif
