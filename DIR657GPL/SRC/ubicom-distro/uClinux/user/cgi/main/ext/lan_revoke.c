#include <stdio.h>
#include "debug.h"

#ifdef LINUX_NVRAM

#include "sutil.h"
#include "shvar.h"

void do_lan_revoke(const char *revoke_ip, const char *revoke_mac) 
{
        init_file(DHCPD_REVOKE_FILE);
        save2file(DHCPD_REVOKE_FILE, "#Sample udhcpd revoke\n"
        "ip %s\n" \
        "mac    %s\n" \
        ,revoke_ip, revoke_mac);
	system("killall -SIGUSR2 udhcpd");
	return;
}
#else

void do_lan_revoke(const char *ip, const char *mac)
{
	cprintf("Nothing tod do!\n");
	return;
}

#endif
