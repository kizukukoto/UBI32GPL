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
#include <errno.h>
#include <mtd/mtd-user.h>
#include <header.h>
#include <bsp.h>
#define TIME_SURVEY
#define CAT_TO_FLASH

#ifdef TIME_SURVEY
#include <sys/time.h>
#endif

#include "ssi.h"
#include "log.h"
#include "mime.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
#define TRUE	1
#define FALSE	0

int upgrade_allow;
extern int mtd_open(const char *mtd, int flags);

static char *flash_type[7] = {
	"ABSENT",
	"Undefinition",
	"Undefinition",
	"NOR Flash",
	"NAND Flash",
	"Undefinition",
	"Data Flash",
	"Generic Type"
};

static int
safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

static ssize_t
safe_write(int fd, const void *buf, size_t count)
{
	ssize_t n;

	do {
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

static ssize_t
safe_read(int fd, void *buf, size_t count)
{
	ssize_t n;

	do {
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

static size_t fsize(const char *fname)
{
        size_t size = 0;
        FILE *fp = fopen(fname, "rb");

        if(!fp) return 0;

        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);

        fclose(fp);

        return size;
}

static size_t find_max_block(size_t max_buf, size_t size)
{
	for(; max_buf > 0; max_buf--)
		if(size % max_buf == 0)
			return max_buf;

	return 1;
}

static void
file_split(const char *fname, size_t start, size_t len, const char *ofname)
{
	int fdin = open(fname, O_RDONLY);
	int fdout = open(ofname, O_WRONLY | O_CREAT | O_EXCL);
	int max_blocksize, blocks;
	char *buf = NULL;

	cprintf("* Fast File Split *\n");

	if(fdin < 0) {
		cprintf("%s can not open.\n", fname);
		goto out;
	}

	if(fdout < 0) {
		cprintf("%s can not create.\n", ofname);
		goto out;
	}

	max_blocksize = find_max_block(10000, len);
	buf = (char *)malloc(max_blocksize);
	if(!buf) {
		cprintf("can not allocate enough memory size.\n");
		goto out;
	}

	lseek(fdin, start, SEEK_SET);

	for(blocks = 0; blocks < len / max_blocksize; blocks++) {
		safe_read(fdin, buf, max_blocksize);
		safe_write(fdout, buf, max_blocksize);
	}

	free(buf);
out:
	if(fdin >= 0) close(fdin);
	if(fdout >= 0) close(fdout);
#if 0
	FILE *fin = fopen(fname, "rb");
        FILE *fout = NULL;
        char buf;
        int i = 0;

        if(!fin) return;

        fout = fopen(ofname, "wb+");

        if(!fout) return;

        fseek(fin, start, SEEK_SET);

        for(; i < len; i++) {
                fread(&buf, sizeof(char), 1, fin);
                fwrite(&buf, sizeof(char), 1, fout);
        }

        fclose(fout);
        fclose(fin);
#endif
}

static float
get_timeval(struct timeval tpstart, struct timeval tpend)
{
	float timeuse;

	timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) +
			tpend.tv_usec - tpstart.tv_usec;
	timeuse /= 1000000;

	return timeuse;
}

static int flash_write(const char *fname, const char *mtd)
{
#ifdef CAT_TO_FLASH
	char cmd[128];
	int ret = 1;
#ifdef TIME_SURVEY
	struct timeval t1, t2, t3;

	gettimeofday(&t1, NULL);
#endif
	bzero(cmd, sizeof(cmd));
	sprintf(cmd, "/sbin/flash_erase %s 0 10", mtd);
	system(cmd);

#ifdef TIME_SURVEY
	gettimeofday(&t2, NULL);
#endif

	bzero(cmd, sizeof(cmd));
	sprintf(cmd, "/bin/cat %s > %s", fname, mtd);
	system(cmd);
#ifdef TIME_SURVEY
	gettimeofday(&t3, NULL);

	cprintf("Erase time: %.3f s\n", get_timeval(t1, t2));
	cprintf("Write time: %.3f s\n", get_timeval(t2, t3));
	cprintf("Total time: %.3f s\n", get_timeval(t1, t3));
#endif
#else
	int mtd_fd = -1;
	int firm_fd = -1;
	char *buf = NULL;
	int ret = 1, len = -1, wlen = -1;
	struct stat filestat;
	size_t total_write_size = 0;
#ifdef TIME_SURVEY
	struct timeval t1, t2, t3, t4;
	struct timeval t5, t6, t7, t8;
	struct timeval t9, t10, t11, t12;

	float read_time = 0.0, write_time = 0.0, erase_time = 0.0;

	cprintf("* Time Survey *\n");
#endif
	mtd_info_t mtd_info;
	erase_info_t erase_info;
#ifdef TIME_SURVEY
	gettimeofday(&t1, NULL);
#endif
	if ((mtd_fd = mtd_open(mtd, O_SYNC | O_RDWR)) < 0) {
		return -1;
	}

	if(ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0) {
		ret = -2;
		goto out;
	}

	if((firm_fd = open(fname, O_RDONLY)) < 0) {
		ret = -3;
		goto out;
	}

	if(fstat (firm_fd, &filestat) < 0) {
		cprintf("While trying to get the file status of %s: %s\n", fname, mtd);
		goto out;
	}
#ifdef TIME_SURVEY
        gettimeofday(&t2, NULL);
#endif
	/* Erase and Write */
	erase_info.start = 0;
	erase_info.length = mtd_info.erasesize;
	buf = (char *)malloc(erase_info.length);
	bzero(buf, erase_info.length);
#ifdef TIME_SURVEY
        gettimeofday(&t3, NULL);
	cprintf("Prepare Time: %f s + %f s = %f s\n",
		get_timeval(t1, t2), get_timeval(t2, t3), get_timeval(t1, t3));
#endif

	while((len = safe_read(firm_fd, buf, erase_info.length)) == erase_info.length) {
#ifdef TIME_SURVEY
		gettimeofday(&t4, NULL);
#endif
		(void)ioctl(mtd_fd, MEMUNLOCK, &erase_info);

#ifdef TIME_SURVEY
		gettimeofday(&t5, NULL);
#endif
		if(ioctl(mtd_fd, MEMERASE, &erase_info) != 0) {
			cprintf("ioctl error (1).\n");
			goto out;
		}
#ifdef TIME_SURVEY
		gettimeofday(&t6, NULL);
#endif
		if((wlen = safe_write(mtd_fd, buf, erase_info.length)) != erase_info.length) {
				cprintf("wlen: %d, erase_info.length: %d\n", wlen, erase_info.length);				
				ret = -5;
				goto out;
		}

#ifdef TIME_SURVEY
		gettimeofday(&t7, NULL);
#endif
		erase_info.start += len;
		bzero(buf, erase_info.length);
		total_write_size += len;
		cprintf("%d / %d", total_write_size, filestat.st_size);
#ifdef TIME_SURVEY
		gettimeofday(&t8, NULL);

		cprintf("\tread: %.3f, ", get_timeval(t3, t4));
		cprintf("MEMUNLOCK: %.3f, ", get_timeval(t4, t5));
		cprintf("MEMERASE: %.3f, ", get_timeval(t5, t6));
		cprintf("write: %.3f, ", get_timeval(t6, t7));
		cprintf("others: %.3f\n", get_timeval(t7, t8));

		read_time += get_timeval(t3, t4);
		erase_time += get_timeval(t4, t6);
		write_time += get_timeval(t6, t7);

		t3 = t8;
#endif
	}

	if(len > 0) {

#ifdef TIME_SURVEY
		gettimeofday(&t12, NULL);
#endif
		total_write_size += len;
		(void)ioctl(mtd_fd, MEMUNLOCK, &erase_info);
#ifdef TIME_SURVEY
		gettimeofday(&t9, NULL);
#endif
		if(ioctl(mtd_fd, MEMERASE, &erase_info) != 0) {
			cprintf("ioctl error. (2)");
			goto out;
		}

#ifdef TIME_SURVEY
		gettimeofday(&t10, NULL);
#endif
		if((wlen = safe_write(mtd_fd, buf, erase_info.length)) != erase_info.length) {
			cprintf("wlen: %d, erase_info.length: %d\n", wlen, erase_info.length);
			ret = -6;
			goto out;
		}

	}
#ifdef TIME_SURVEY
	gettimeofday(&t11, NULL);
#endif
	cprintf("Total Write Size: %d bytes (%s: %d bytes)\n",
			total_write_size, fname, filestat.st_size);

#ifdef TIME_SURVEY
	if(len > 0) {
		erase_time += get_timeval(t12, t10);
		write_time += get_timeval(t10, t11);
	}

	cprintf("Total read: %.3f s\n", read_time);
	cprintf("Total erase: %.3f s\n", erase_time);
	cprintf("Total write: %.3f s\n", write_time);
	cprintf("Total time: %.3f s\n", get_timeval(t1, t11));
#endif

out:
	if (mtd_fd >= 0) close(mtd_fd);
	if (firm_fd >= 0) close(firm_fd);
	if (buf) free(buf);
#endif
	return ret;
}

#if 0
static int
flash_update(const char *fname)
{
	int ret = 0;
	firmware_header fh;
	int ksize;

	ksize = atoi(fh.mb + KOFF);
	file_split(fname, 0, ksize, "/tmp/kernel.bin");
	file_split(fname, ksize, fsize(fname) - ksize - sizeof(firmware_header), "/tmp/fs.bin");

	system("/bin/rm -rf /tmp/flash.bin");

	cprintf("Write kernel.bin into /dev/mtd1... (erase + write)\n");
	ret = flash_write("/tmp/kernel.bin", "/dev/mtd1");
	cprintf("return: %d\n", ret);
	cprintf("Write fs.bin into /dev/mtd2... (erase + write)\n");
	ret = flash_write("/tmp/fs.bin", "/dev/mtd2");
	cprintf("return: %d\n", ret);

	return 1;
}

static void flash_write(const char *mtd, const char *fname)
{
	char cmds[128];

	sprintf(cmds, "flash_write %s %s", mtd, fname);
	system(cmds);
}
#endif

static void ips_update_information(const char *fname)
{
	char cmds[128];

	sprintf(cmds, "%d", fsize(fname));
	nvram_set("ips_size", cmds);
	nvram_commit();
}

static int mime_ipsupdate(struct mime_desc *dc)
{
	char *c;
	FILE *fw;
	int ret = -1;

	if (strcmp("file", dc->name) != 0)
		goto out;
	if ((fw = fopen("/tmp/ips.bin", "w")) == NULL)
		goto out;

	cprintf("Begin ips upgrade...\n");
	while ((c = fgetc(dc->fd)) != EOF)
		fputc(c, fw);
	fclose(fw);
#if 0
	ret = flash_update("/tmp/flash.bin");
	if (ret == -1) {
		upgrade_allow = FALSE;
		cprintf("upgrade denied %s\n", (upgrade_allow == TRUE)?"TRUE":"FALSE");
	}
#endif
	flash_write("/tmp/ips.bin", "/dev/mtd6");
	ips_update_information("/tmp/ips.bin");
	ret = 0;
out:
	return ret;
}

char* update_ips() {
	char *t = "error.asp";
	pid_t pid;

	register_mime_handler(mime_ipsupdate);
	http_post_upload(stdin);

	pid = fork();

	if(pid < 0) {
		setenv("html_response_return_page", "tools_system.asp", 1);
		setenv("html_response_error_message", "The ips upgrade FAIL!", 1);
		setenv("result_timer", "10", 1);

		goto out;

	} else if (pid == 0) {
		close(0);
		close(1);
		close(2);
		sleep(1);
#if 0
		kill(1, SIGTERM);
		exit(0);
#endif
	} else {
		t = "count_down.asp";
		cprintf("allow upgrade, return %s page.\n", t);
		setenv("html_response_return_page", "tools_system.asp", 1);
		setenv("html_response_message", "The ips upgrading ", 1);
		setenv("result_timer", "10", 1);

		nvram_set("ips_date", __DATE__);
		sleep(10);
		nvram_commit();
out:
		return t;
	}
}
