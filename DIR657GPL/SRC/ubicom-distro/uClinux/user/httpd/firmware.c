#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <mtd/mtd-user.h>
#include <malloc.h>

#define PATH_MAX 1024
#define cprintf printf
//#define CAT_TO_FLASH
#define TIME_SURVEY

#ifdef TIME_SURVEY
#include <sys/time.h>
#endif

int mtd_open(const char *mtd, int flags)
{
	FILE *fp;
	char dev[PATH_MAX];
	int i;

	if ((fp = fopen("/proc/mtd", "r"))) {
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
	sprintf(cmd, "/sbin/flash_erase %s 0 %d", mtd, ((strcmp(mtd, "/dev/mtd1")==0)?10:36));
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

int flash_update(const char *fname, int ksize, int fsize)
{
	int ret = -1;
	char *kr_file = "/var/firm/kr.bin";
	char *fs_file = "/var/firm/fs.bin";

	file_split(fname, 0, ksize, kr_file);
	file_split(fname, ksize, fsize, fs_file);

	system("rm -rf /var/firm/image.bin");

	cprintf("Write kernel.bin into /dev/mtd1... (erase + write)\n");
	ret = flash_write(kr_file, "/dev/mtd1");
//	system("cat /var/firm/kr.bin > /dev/mtd1");
	cprintf("Write fs.bin into /dev/mtd2... (erase + write)\n");
	ret = flash_write(fs_file, "/dev/mtd2");
//	system("cat /var/firm/fs.bin > /dev/mtd2");

	return 1;
}
