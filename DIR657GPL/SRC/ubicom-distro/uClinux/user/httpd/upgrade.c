#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "bsp.h"
#include "mime.h"
#include "sutil.h"
#include "shvar.h"

#ifdef HTTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#define FIRMWARE_UPGRADE_FAIL     0
#define FIRMWARE_UPGRADE_SUCCESS  1
#define FIRMWARE_SIZE_ERROR       2

#define PATH_MAX	1024

#define FW_ID_LEN		24
#define FW_DOMAIN_ID_LEN	2

static int upgrade_status;
static int upgrade_mutex = 0;

static void upstream(FILE *stream)
{
	char buf[4096];
	int n = 0;

	/* eat out all of stream where not read yet */
	while (fgets(buf, sizeof(buf), stream) != NULL) {
		n += strlen(buf);
		DEBUG_MSG("X:%d: %s", n, buf);
	}
}

static inline int check_firmware_id(const char *s)
{
	const char *id;
	
	/* 
	 * where @s is buffer from the last 26 bytes from tail of stream.
	 * offset 2 bytes for get 24 bytes id.
	 * */
	id = s + FW_DOMAIN_ID_LEN;
	if (strncmp(id, HWID, FW_ID_LEN) == 0)
		return 0; /* id match */
	
	return -1;
}

static inline int check_firmware_domain_id(const char *s)
{
#ifdef HWCOUNTRYID
	return strncmp(s, HWCOUNTRYID, FW_DOMAIN_ID_LEN);
#else
	return 0;
#endif
}

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

#if 0
static int upgrade_write(
	struct mime_desc *md,
	size_t start,
	size_t len,
	const char *mtd)
{
	int c, ret = -1;
	FILE *fp = fopen(mtd, "w");

	if (md == NULL || mtd == NULL)
		goto out;
	if (len != -1 && start + len > md->size)
		goto out;
	if (start > md->size)
		goto out;
	if (fp == NULL)
		goto out;

	DEBUG_MSG("XXX md->size [%d]\n", md->size);
	DEBUG_MSG("XXX start [%d]\n", start);
	DEBUG_MSG("XXX len [%d]\n", len);
	DEBUG_MSG("XXX mtd [%s]\n", mtd);

	c = safe_fwrite(md->fd + start, 1, (len == -1)?(md->size - start):len, fp);
	DEBUG_MSG("XXX safe_fwrite [%d]\n\n", c);

	ret = 0;
	fclose(fp);
out:
	return ret;
}
#else
static int mtd_open(const char *mtd, int flags)
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


static int flash_write(const char *fname, const char *mtd)
{
	int mtd_fd = -1;
	int firm_fd = -1;
	char *buf = NULL;
	int ret = 1, len = -1, wlen = -1;
	struct stat filestat;
	const size_t buffer_size = 1024;

	if ((mtd_fd = mtd_open(mtd, O_SYNC | O_RDWR)) < 0)
		return -1;
	if((firm_fd = open(fname, O_RDONLY)) < 0) {
		ret = -3;
		goto out;
	}

	if(fstat (firm_fd, &filestat) < 0) {
		DEBUG_MSG("While trying to get the file status of %s: %s\n", fname, mtd);
		goto out;
	}

	buf = (char *)malloc(buffer_size);
	bzero(buf, buffer_size);
	while((len = safe_read(firm_fd, buf, buffer_size)) == buffer_size) {
		if((wlen = safe_write(mtd_fd, buf, buffer_size)) != buffer_size) {
			cprintf("wlen: %d, erase_info.length: %d\n", wlen, buffer_size);
			ret = -5;
			goto out;
		}
		bzero(buf, buffer_size);
	}

	if (len > 0) {
		if((wlen = safe_write(mtd_fd, buf, buffer_size)) != buffer_size) {
			DEBUG_MSG("wlen: %d, erase_info.length: %d\n", wlen, buffer_size);
			ret = -6;
			goto out;
		}
	}

out:
	if (mtd_fd >= 0) close(mtd_fd);
	if (firm_fd >= 0) close(firm_fd);
	if (buf) free(buf);

	return ret;
}

static size_t find_max_block(size_t max_buf, size_t size)
{
	for(; max_buf > 0; max_buf--)
		if(size % max_buf == 0)
			return max_buf;

	return 1;
}

static void file_split(
	const char *fname,
	size_t start,
	size_t len,
	const char *ofname)
{
	int fdin = open(fname, O_RDONLY);
	int fdout = open(ofname, O_WRONLY | O_CREAT);
	int max_blocksize, blocks;
	char *buf = NULL;

	DEBUG_MSG("* Fast File Split *\n");

	if(fdin < 0) {
		DEBUG_MSG("%s can not open.\n", fname);
		goto out;
	}

	if(fdout < 0) {
		DEBUG_MSG("%s can not create.\n", ofname);
		goto out;
	}

	max_blocksize = find_max_block(10000, len);
	buf = (char *)malloc(max_blocksize);
	if(!buf) {
		DEBUG_MSG("can not allocate enough memory size.\n");
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

static int upgrade_write(struct mime_desc *md)
{
	int ret = -1;
	char buf[1024];
	FILE *tmpfs = fopen("/tmp/firmware.bin", "w");

	if (tmpfs == NULL)
		goto out;

	fseek(md->fd, 0L, SEEK_SET);
	while (safe_fread(buf, 1, sizeof(buf) - 1, md->fd) > 0)
		safe_fwrite(buf, 1, sizeof(buf) - 1, tmpfs);

	fclose(tmpfs);
	ret = 0;

	file_split("/tmp/firmware.bin", 0, (1536 * 1024), "/tmp/kr.bin");
	file_split("/tmp/firmware.bin", (1536 * 1024), md->size - (1536 * 1024), "/tmp/fs.bin");
	system("rm -rf /tmp/firmware.bin");
	DEBUG_MSG("XXX write kernel...!!\n");
	flash_write("/tmp/kr.bin", "/dev/mtdblock2");
	DEBUG_MSG("XXX write filesystem...!!\n");
	flash_write("/tmp/fs.bin", "/dev/mtdblock3");
out:
	if (ret != 0)
		DEBUG_MSG("XXX fileopen fail.\n");
	return ret;
}
#endif
static int mime_element(struct mime_desc *md)
{
	int ret = -1;
	char response_page[128];

	if (md->size <= 0)
		goto out;

	bzero(response_page, sizeof(response_page));
	fread(response_page, 1, md->size, md->fd);
	nvram_set(md->name, response_page);

	ret = 0;
out:
	return ret;
}

static int mime_file(struct mime_desc *md)
{
	pid_t pid;
	char buf[28];

	bzero(buf, sizeof(buf));

	if (fseek(md->fd, -26, SEEK_END) == -1)
		goto sys_err;

	/*
	 * XXX: @buf always get the last 26 bytes from tail
	 *      of stream, even if we did not support country id...
	 * */
	if (fread(buf, 1, sizeof(buf) - 1, md->fd) == -1)
		goto sys_err;

	/* 
	 * check firmware's hw and country id
	 * eg: buf[26] = "00MVL-88F6281-RT-090520-00"
	 * */
	if (check_firmware_id(buf) != 0)
		goto id_err;

	/*
	 * check file size to detect upgrade types
	 * is a normal file or bootcode file.
	 * */
	if ((BOOT_IMG_LOWER_BOUND < md->size) && (md->size < BOOT_IMG_UPPER_BOUND)) {
		fprintf(stderr, "bootcode firmware upgrade\n");
	} else if ((IMG_LOWER_BOUND < md->size) && (md->size < IMG_UPPER_BOUND)) {
		fprintf(stderr, "normal firmware upgrade\n");
	if (check_firmware_domain_id(buf) != 0)
		goto id_err;
	} else
		goto size_err;

	DEBUG_MSG("XXX upgrade mutex [%d]\n", upgrade_mutex);

	if (upgrade_mutex != 0)
		goto out;
	upgrade_mutex = getpid();
	DEBUG_MSG("XXX upgrade mutex goin [%d]\n", upgrade_mutex);

	DEBUG_MSG("FW ID from upload file is [%s]\n", buf);
	DEBUG_MSG("start upgrade stream\n");

	system("killall wantimer");
	system("killall timer");
	system("killall syslogd");
	system("killall klogd");
	system("killall udhcpc");
	system("killall udhcpd");
	system("killall dnsmasq");
	system("killall tftpd");
	system("killall nmbd");
	system("killall redial");
	system("killall monitor");

	if ((pid = vfork()) == 0)	/* child gout */
		goto out;
	sleep(2);
	upgrade_write(md);
	system("reboot");
	_exit(-1);

out:
	upgrade_status = FIRMWARE_UPGRADE_SUCCESS;
	return 0;
sys_err:
	fprintf(stderr, "%s->%d: sys_err:%s\n",
		__FUNCTION__, __LINE__, strerror(errno));
	upgrade_status = FIRMWARE_UPGRADE_FAIL;
	return -1;
id_err:
	fprintf(stderr, "%s->%d: firmware id check error: "
			"%s(from file) %s(expect)\n",
		__FUNCTION__, __LINE__, buf, HWID);
	upgrade_status = FIRMWARE_UPGRADE_FAIL;
	return -1;
size_err:
	fprintf(stderr, "Invalid size check\n");
	upgrade_status = FIRMWARE_SIZE_ERROR;
	return -1;
}

static int upgrade_stream(struct mime_desc *md)
{
	struct {
		char *name;
		int (*fn)(struct mime_desc *);
	} *s, mime_list[] = {
		{ "html_response_page", mime_element },
		{ "html_response_return_page", mime_element },
		{ "countdown_time", mime_element },
		{ "file", mime_file },
		{ NULL, NULL }
	};

	for (s = mime_list; s && s->name && s->fn; s++)
		if (strcmp(md->name, s->name) == 0)
			return s->fn(md);

	return 0;
}

int __do_upgrade_post(FILE *stream, int len, const char *boundary)
{
	//int ret = FIRMWARE_UPGRADE_SUCCESS;
	char buf[32];

	sprintf(buf, "%d", len);

	DEBUG_MSG("X%s:%d, conetent-lenght:%d\n", __FUNCTION__, __LINE__, len);
	/*
	 * need by mime.c library
	 * */
	setenv("CONTENT_LENGTH", buf, 1);
	setenv("BOUNDARY", boundary, 1);
	
	register_mime_handler(upgrade_stream);
	/* 
	 * @http_post_upgrade will call @upgrade_stream in each one
	 * file stream within @boundary 
	 * */
	http_post_upload(stream);
	upstream(stream);

	DEBUG_MSG("XXX upgrade_status: [%d]\n", upgrade_status);

	return upgrade_status;
}
