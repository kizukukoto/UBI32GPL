#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <mtd-user.h>

#include <sys/sysinfo.h>
#include <shvar.h>
#include <sutil.h>
#include <sys/shm.h>

//define  RC_DEBUG  1
#ifdef RC_DEBUG
#define DEBUG_MSG(fmt, arg...)       cprintf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define sys_reboot()		kill(1, SIGTERM)

int fwupgrade_main(int argc, char** argv)
{
	char cmd[256];
	printf("fwupgrade_main\n");
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef	CONFIG_LP
	int no_art=1,lingual_erase=0;
	FILE *fp = NULL;

	fp = fopen("/proc/mtd", "r");
	if (fp) {
		while (!feof(fp)) {
			memset(cmd, '\0', sizeof(cmd));
			fgets(cmd,sizeof(cmd), fp);
			if (strstr(cmd, "mtd5")) {
				no_art = 0;
				break;
			}
		}
		fclose(fp);
	}

	printf("fwupgrade_main - no_art=%d\n", no_art);
#endif

	if (strncmp(argv[1], "fw", 2) == 0) {
		cprintf("FW_UPGRADE_FILE=%s,SYS_KERNEL_MTDBLK=%s\n",FW_UPGRADE_FILE,SYS_KERNEL_MTDBLK);
		sprintf(cmd, "dd if=%s of=%s", FW_UPGRADE_FILE, SYS_KERNEL_MTDBLK);
		system("killall timer");
		system(cmd);
		cprintf("FW upgrade finished...wait to reboot...\n");
	}
	else if (strncmp(argv[1], "boot", 4) == 0) {
		cprintf("Loader_UPGRADE_FILE=%s,SYS_BOOT_MTDBLK=%s\n",FW_UPGRADE_FILE,SYS_BOOT_MTDBLK);
		sprintf(cmd, "dd if=%s of=%s", FW_UPGRADE_FILE, SYS_BOOT_MTDBLK);
		system("killall timer");
		system(cmd);
		cprintf("Loader upgrade finished...wait to reboot...\n");
	}
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef	CONFIG_LP
	else if (strncmp(argv[1], "lingual",7) == 0) {
		if (no_art==1) return EINVAL;

		fp = fopen(FW_UPGRADE_FILE, "r");
		if (!fp) {
			lingual_erase = 1;
			sprintf(cmd, "eraseall %s &", SYS_LP_MTD);
			system(cmd);
			//sleep(3);
			//return 0;
		}
		else {
			fclose(fp);
			cprintf("FW_UPGRADE_FILE=%s,SYS_KERNEL_MTDBLK=%s\n",FW_UPGRADE_FILE,SYS_LP_MTDBLK);
			system("killall timer");
			sprintf(cmd, "dd if=%s of=%s", FW_UPGRADE_FILE, SYS_LP_MTDBLK);
			system(cmd);
			cprintf("Loader language finished...wait to restart...\n");
			//sleep(3);
		}
	}

	if (!strncmp(argv[1], "lingual", 7)) {
		//sleep(10);
		sprintf(cmd, "rm -f %s", FW_UPGRADE_FILE);
		system(cmd);
		if(lingual_erase == 1)
		{
		printf("killall -SIGSYS httpd\n");
		system("killall -SIGSYS httpd");
	}
	else
			sys_reboot();
	}
	else
#endif
	{
		cprintf("FW upgrade finished...wait to reboot...\n");
		sys_reboot();
	}
	return 0;
}
