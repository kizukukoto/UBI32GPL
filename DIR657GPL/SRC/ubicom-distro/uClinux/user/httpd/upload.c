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

#define PATH_MAX	1024
static int upload_status;
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
	int c, ret = -1;
	char buf[1024];
	FILE *tmpfs = fopen(NVRAM_UPGRADE_TMP, "w");

	upload_status = 0;
	bzero(buf, sizeof(buf));

	if (tmpfs == NULL)
		goto out;

	fseek(md->fd, 0L, SEEK_SET);
	while ((c = safe_fread(buf, 1, sizeof(buf) - 1, md->fd)) > 0)
		safe_fwrite(buf, 1, c, tmpfs);

	fclose(tmpfs);

	upload_status = 1;
	ret = 0;

	if (vfork() == 0)	/* child go out */
		goto out;
	sleep(2);
	system("nvram upgrade");
	system("reboot");
	_exit(-1);
out:
	return ret;
}

static int upload_stream(struct mime_desc *md)
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

int __do_upload_post(FILE *stream, int len, const char *boundary)
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
	
	register_mime_handler(upload_stream);
	/* 
	 * @http_post_upgrade will call @upgrade_stream in each one
	 * file stream within @boundary 
	 * */
	http_post_upload(stream);
	upstream(stream);

	DEBUG_MSG("XXX upload_status: [%d]\n", upload_status);

	return upload_status;
}
