#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "cmds.h"

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

static void utility_dump(const char *data, size_t baseidx, size_t len)
{
	size_t i;

	cprintf("\n## DUMP INFO ##\n");
	cprintf("len: %d\n", len);
	cprintf("## DUMP PACKET ##\n");

	while(1) {
		size_t display_len = ((len - baseidx > 16)?16:len - baseidx);
		size_t remain_len;

		if (baseidx > len)
			break;

		for (i = baseidx; i < baseidx + display_len; i++)
			cprintf("%02X ", *(data + i));

		remain_len = (16 - (i % 16)) % 16;
		for (i = 0; i < remain_len; i++)
			cprintf("   ");
		cprintf("\t");

		for (i = baseidx; i < baseidx + display_len; i++)
			cprintf("%c", (*(data + i) >= 32 && *(data + i) <= 126)?*(data + i):'.');

		baseidx += 16;
		cprintf("\n");
	}

	printf("\n## DUMP END ##\n");
}

static int utility_filesize(const char *fname)
{
	int fd, ret = -1;
	struct stat filestat;

	if ((fd = open(fname, O_RDONLY)) == -1)
		goto out;

	fstat(fd, &filestat);
	close(fd);

	ret = filestat.st_size;
out:
	return ret;
}

static int utility_create_buffer(const char *filename, char *buffer)
{
	FILE *fp;
	int size, ret = -1;

	if ((size = utility_filesize(filename)) == 0)
		goto out;

	if ((fp = fopen(filename, "r")) == NULL)
		goto out;

	fread(buffer, 1, size, fp);

	fclose(fp);
	ret = 0;
out:
	return ret;
}

int utility_dump_main(int argc, char *argv[])
{
	FILE *fp;
	int fd, filesize, ret = -1;
	struct stat filestat;
	char *buffer, *filename, *start, *length;

	int _start, _length;

	if (get_std_args(argc, argv, &filename, &start, &length) == -1)
		goto out;

	if ((filesize = utility_filesize(filename)) == -1)
		goto out;
	else
		buffer = (char *)malloc(filesize);

	_start = atoi(start);
	_length = atoi(length);

	if (_start >= filesize || _start < 0 ||
		_length <= 0 || _start + _length > filesize)
	{
		DEBUG_MSG("range error.\n");
		goto out;
	}

	DEBUG_MSG("filename: %s\n", filename);
	DEBUG_MSG("filesize: %d\n", filesize);
	DEBUG_MSG("start: %s\n", start);
	DEBUG_MSG("length: %s\n", length);

	utility_create_buffer(filename, buffer);
	utility_dump(buffer, atoi(start), atoi(length));

	if (buffer) free(buffer);
	ret = 0;
out:
	return ret;
}
