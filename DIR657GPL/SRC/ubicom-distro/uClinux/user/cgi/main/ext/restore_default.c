#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "libdb.h"
#include "ssc.h"

void *do_restore_defaults(struct ssc_s *obj)
{
#ifdef LINUX_NVRAM
	system("nvram restore_default");
#else
	/* DIRX30 style */
        int ret = -1;
        ret = nvram_set("restore_defaults", "1");
        nvram_commit();
#endif //LINUX_NVRAM
        __do_reboot(obj);

        return get_response_page();
}

/* XXX: porting from 130/330, keep pptp client terminate */
static void terminate_ppp_before_reboot()
{
        system("killall ifmonitor");
        system("killall l2tpd");
        system("killall pppd");
        system("killall pptp");
	/* 
	 * some region eg: RU, it force PPP MUST send TERM to peer
 	 * otherwise, server in RU will not allow device connect to after
 	 * reboot.
 	 */
        system("killall pptpd");
        system("killall xl2tpd");
        sleep(2);	/* make sure PPP packet FIN */
}

/* XXX porting from 130/330,
 * __do_reboot() and do_reboot() are the same behavior? */
void __do_reboot(struct ssc_s *obj)
{
        pid_t pid;

#if NOMMU
        pid = vfork();
#else
        pid = fork();
#endif
        terminate_ppp_before_reboot();

        if (pid == -1)
                return; /*FIXME: should returned error page*/

        if (pid == 0) {
                close(0);
                close(1);
                close(2);
                system("reboot -d 5 &");
                exit(0);
        }
	waitpid(pid, NULL, 0);
}

void *do_reboot(struct ssc_s *obj)
{
	__do_reboot(obj);
        return get_response_page();
}
