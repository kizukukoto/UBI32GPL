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
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
/* "__user" is defined by kernel?, user space can not recognize */
#ifndef __user
#define __user
#endif
#include <mtd/mtd-user.h>
#define TIME_SURVEY

#ifdef TIME_SURVEY
#include <sys/time.h>
#endif

#define cprintf(fmt, args...) do { \
	FILE *fp = fopen("/dev/console", "w"); \
	if (fp) { \
		fprintf(fp, fmt, ## args); \
		fclose(fp); \
	} \
} while (0)

#if 1
#define DEBUG_MSG(fmt, arg...)	cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

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

static float
get_timeval(struct timeval tpstart, struct timeval tpend)
{
	float timeuse;

	timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) +
			tpend.tv_usec - tpstart.tv_usec;
	timeuse /= 1000000;

	return timeuse;
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

static int mtd_open(const char *mtd, int flags)
{
	FILE *fp;
	char dev[128];
	int i;

	if ((fp = fopen("/proc/mtd", "rb"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) {
				snprintf(dev, sizeof(dev), "/dev/mtd/%d", i);
				fclose(fp);
				return open(dev, flags);
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}
/*
 * erase and write @fname to @mtd
 * return: 1 on sucess
 * */
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

#ifdef TIME_SURVEY
        gettimeofday(&t2, NULL);
#endif

        bzero(cmd, sizeof(cmd));
        sprintf(cmd, "/bin/cat %s > %s", fname, mtd);
        system(cmd);
#ifdef TIME_SURVEY
        gettimeofday(&t3, NULL);

        DEBUG_MSG("Erase time: %.3f s\n", get_timeval(t1, t2));
        DEBUG_MSG("Write time: %.3f s\n", get_timeval(t2, t3));
        DEBUG_MSG("Total time: %.3f s\n", get_timeval(t1, t3));
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

        DEBUG_MSG("* Time Survey *\n");
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
                DEBUG_MSG("While trying to get the file status of %s: %s\n", fname, mtd);
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
        DEBUG_MSG("Prepare Time: %f s + %f s = %f s\n",
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
                        DEBUG_MSG("ioctl error (1).\n");
                        goto out;
                }
#ifdef TIME_SURVEY
                gettimeofday(&t6, NULL);
#endif
                if((wlen = safe_write(mtd_fd, buf, erase_info.length)) != erase_info.length) {
                                DEBUG_MSG("wlen: %d, erase_info.length: %d\n", wlen, erase_info.length);
                                ret = -5;
                                goto out;
                }

#ifdef TIME_SURVEY
                gettimeofday(&t7, NULL);
#endif
                erase_info.start += len;
                bzero(buf, erase_info.length);
                total_write_size += len;
                DEBUG_MSG("%d / %d", total_write_size, filestat.st_size);
#ifdef TIME_SURVEY
                gettimeofday(&t8, NULL);

                DEBUG_MSG("\tread: %.3f, ", get_timeval(t3, t4));
                DEBUG_MSG("MEMUNLOCK: %.3f, ", get_timeval(t4, t5));
                DEBUG_MSG("MEMERASE: %.3f, ", get_timeval(t5, t6));
                DEBUG_MSG("write: %.3f, ", get_timeval(t6, t7));
                DEBUG_MSG("others: %.3f\n", get_timeval(t7, t8));

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
                        DEBUG_MSG("ioctl error. (2)");
                        goto out;
                }

#ifdef TIME_SURVEY
                gettimeofday(&t10, NULL);
#endif
                if((wlen = safe_write(mtd_fd, buf, erase_info.length)) != erase_info.length) {
                        DEBUG_MSG("wlen: %d, erase_info.length: %d\n", wlen, erase_info.length);
                        ret = -6;
                        goto out;
                }

        }
#ifdef TIME_SURVEY
        gettimeofday(&t11, NULL);
#endif
	DEBUG_MSG("File Size: %d bytes\n", fsize(fname));
        DEBUG_MSG("Total Write Size: %d bytes (%s: %d bytes)\n",
                        total_write_size, fname, filestat.st_size);

#ifdef TIME_SURVEY
        if(len > 0) {
                erase_time += get_timeval(t12, t10);
                write_time += get_timeval(t10, t11);
        }

        DEBUG_MSG("Total read: %.3f s\n", read_time);
        DEBUG_MSG("Total erase: %.3f s\n", erase_time);
        DEBUG_MSG("Total write: %.3f s\n", write_time);
        DEBUG_MSG("Total time: %.3f s\n", get_timeval(t1, t11));
#endif

out:
        if (mtd_fd >= 0) close(mtd_fd);
        if (firm_fd >= 0) close(firm_fd);
        if (buf) free(buf);
#endif
        return ret;
}

/* INPUT:
 * 	@desc := filename string
 * RETURN:
 * 	zero    := is not charactor device
 * 	non-zeo := is charactor device
 *
 * */
static int is_charactor_dev(const char *desc)
{
	struct stat st;

	if (desc == NULL)
		goto out;
	if (access(desc, F_OK) != 0)
		goto out;
	if (stat(desc, &st) != 0)
		goto out;

	return S_ISCHR(st.st_mode);
out:
	return 0;
}

static int flash_write_stdin(const char *dev)
{
	const char *mtd = dev;
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

        DEBUG_MSG("* Time Survey *\n");
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
#if 0
        if ((firm_fd = open(fname, O_RDONLY)) < 0) {
                ret = -3;
                goto out;
        }
#else
	firm_fd = fileno(stdin);
#endif
        if(fstat (firm_fd, &filestat) < 0) {
                DEBUG_MSG("While trying to get the file status of STDIN: %s\n", mtd);
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
        DEBUG_MSG("Prepare Time: %f s + %f s = %f s\n",
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
                        DEBUG_MSG("ioctl error (1).\n");
                        goto out;
                }
#ifdef TIME_SURVEY
                gettimeofday(&t6, NULL);
#endif
                if((wlen = safe_write(mtd_fd, buf, erase_info.length)) != erase_info.length) {
                                DEBUG_MSG("wlen: %d, erase_info.length: %d\n", wlen, erase_info.length);
                                ret = -5;
                                goto out;
                }

#ifdef TIME_SURVEY
                gettimeofday(&t7, NULL);
#endif
                erase_info.start += len;
                bzero(buf, erase_info.length);
                total_write_size += len;
                DEBUG_MSG("%d / %d", total_write_size, filestat.st_size);
#ifdef TIME_SURVEY
                gettimeofday(&t8, NULL);

                DEBUG_MSG("\tread: %.3f, ", get_timeval(t3, t4));
                DEBUG_MSG("MEMUNLOCK: %.3f, ", get_timeval(t4, t5));
                DEBUG_MSG("MEMERASE: %.3f, ", get_timeval(t5, t6));
                DEBUG_MSG("write: %.3f, ", get_timeval(t6, t7));
                DEBUG_MSG("others: %.3f\n", get_timeval(t7, t8));

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
                        DEBUG_MSG("ioctl error. (2)");
                        goto out;
                }

#ifdef TIME_SURVEY
                gettimeofday(&t10, NULL);
#endif
                if((wlen = safe_write(mtd_fd, buf, erase_info.length)) != erase_info.length) {
                        DEBUG_MSG("wlen: %d, erase_info.length: %d\n", wlen, erase_info.length);
                        ret = -6;
                        goto out;
                }

        }
#ifdef TIME_SURVEY
        gettimeofday(&t11, NULL);
#endif
        //DEBUG_MSG("File Size: %d bytes\n", fsize(fname));
        //DEBUG_MSG("Total Write Size: %d bytes (%s: %d bytes)\n",
        //                total_write_size, fname, filestat.st_size);

#ifdef TIME_SURVEY
        if(len > 0) {
                erase_time += get_timeval(t12, t10);
                write_time += get_timeval(t10, t11);
        }

        DEBUG_MSG("Total read: %.3f s\n", read_time);
        DEBUG_MSG("Total erase: %.3f s\n", erase_time);
        DEBUG_MSG("Total write: %.3f s\n", write_time);
        DEBUG_MSG("Total time: %.3f s\n", get_timeval(t1, t11));
#endif

out:
        if (mtd_fd >= 0) close(mtd_fd);
        if (firm_fd >= 0) close(firm_fd);
        if (buf) free(buf);

        return ret;
}

static void help()
{
	printf("arguments: [filename] [mtd_dev] <post command>\n"
		"  filename: the file write to flash\n"
		"  mtd_dev : the target device name to be erase and wrote\n"
		"  post command: exect a <command> after flash write completely\n\n"
		"  eg: <cli ..> flash_write /tmp/upgrade.bin /dev/mtd1\n"
		"      <cli ..> flash_write /tmp/upgrade.bin /dev/mtd1 \"reboot -d 2\"\n"
		"      <cli ..> flash_write /dev/mtd1 \"reboot -d 2\"  (stdin input)\n");
}

int flash_write_main(int argc, char *argv[])
{
	int fw_ret, ret = -1;
	char *dev, *filename = NULL, *pst_cmds = NULL;

	if (argc < 2 || argc > 4)
		goto help;
	if (argc == 2)	/* filename = stdin */
		dev = argv[1];
	else if (argc == 3) {
		if (is_charactor_dev(argv[1])) {
			dev = argv[1];
			pst_cmds = argv[2];
		} else {
			filename = argv[1];
			dev = argv[2];
		}
	} else {
		filename = argv[1];
		dev = argv[2];
		pst_cmds = argv[3];
	}

	cprintf("XXX %s: %s(%d) filename: '%s'\n",
		__FILE__, __FUNCTION__, __LINE__, (filename == NULL)? "STDIN": filename);
	cprintf("XXX %s: %s(%d) dev: '%s' pst_cmds: '%s'\n",
		__FILE__, __FUNCTION__, __LINE__, dev, pst_cmds);

	if (filename == NULL)
		fw_ret = flash_write_stdin(dev);
	else
		fw_ret = flash_write(filename, dev);

	if (fw_ret != 1)
		goto out;
	if (pst_cmds)
		system(pst_cmds);
	ret = 0;
#if 0
	if (flash_write(filename, dev) == 1) {
		if (pst_cmds)
			system(pst_cmds);
		ret = 0;
	}
#endif
out:
	return ret;
help:
	help();
	return ret;
}
