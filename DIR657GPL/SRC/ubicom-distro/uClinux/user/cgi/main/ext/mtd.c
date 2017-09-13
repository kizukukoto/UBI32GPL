#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include "mime.h"
#include "ssi.h"
#include "partition.h"
/*
 * Helper copy from artblock.c, thanks
 * */

/***************************************************/
/* The function move to /cgi/lib , because /cgi/misc/lang.c will be used */
#if 0
int *get_mtd_partition_index(const char *name)
{
	FILE *fd;
	char buf[128], p;
	int i;

	if (!name)
		goto out;
	if ((fd  = fopen("/proc/mtd", "r")) == NULL)
		goto out;
	bzero(buf, sizeof(buf));

	while (fgets(buf, sizeof(buf) - 1, fd)) {
		if (sscanf(buf, "mtd%d:", &i) && strstr(buf, name))
			return i;	
		bzero(buf, sizeof(buf));
	}
out:
	return -1;
}

int get_mtd_char_device(const char *name, char *buf)
{
	int i;
	
	if ((i = get_mtd_partition_index(name)) != -1) {
		sprintf(buf, "/dev/mtd%d", i);
	}
	return i;
}

int get_mtd_size(const char *name)
{
	FILE *fd;
	char buf[128], p;
	int i, len = -1;

	if (!name)
		goto out;
	if ((fd  = fopen("/proc/mtd", "r")) == NULL)
		goto out;
	bzero(buf, sizeof(buf));

	while (fgets(buf, sizeof(buf) - 1, fd)) {
		if (sscanf(buf, "mtd%d: %X ", &i, &len) && strstr(buf, name)) {
			return len;
		}
		bzero(buf, sizeof(buf));
	}
out:
	return -1;
}
#endif
/***************************************************/

/*
 * Write date from stream to /dev/mtdX
 * @stream: file stream
 * @stream_len: date length of @stream
 * Retrun: 0 on sucess, -1 on fail.
 * */

//int stream2mtd(FILE *stream, int stream_len, const char *mtd_name)
//int stream2mtd(const char *file, int stream_len, const char *mtd_name)
int stream2mtd(struct mime_desc *dc, int stream_len, const char *mtd_name)
{
	int mtd_len, ret = -1;
	char mtd[128], cmd[128];

	if (get_mtd_char_device(mtd_name, mtd) == -1)
		goto out;
	if ((mtd_len = get_mtd_size(mtd_name)) == -1)
		goto out;
	if (stream_len > mtd_len) {
		goto out;
	}

	if (dc->opt)
		sprintf(cmd, "cli sys flash_write %s %s \"reboot -d 3\" &", dc->tpath, mtd);
	else {
		FILE *fp;
		int c, total = 0;
		char *bin = "/tmp/flash.bin";

		if((fp = fopen(bin, "w")) == NULL)
			goto sys_err;
		while ((c = fgetc(dc->fd)) != EOF) {
	                fputc(c, fp);
			total++;
		}
		fclose(fp);
		sprintf(cmd, "cli sys flash_write %s %s \"reboot -d 3\" &", bin, mtd);
	}

	cprintf(cmd, "XXX %s(%d) cmd: '%s'\n", __FUNCTION__, __LINE__, cmd);
	system(cmd);
	return 0;
sys_err:
	perror("sys_err");
out:
	return ret;
}

