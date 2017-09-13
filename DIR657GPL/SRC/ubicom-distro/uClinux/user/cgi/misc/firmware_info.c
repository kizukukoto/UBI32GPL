#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "firmware.h"
#include "linux/cramfs_fs.h"

void get_signature(struct signature *s)
{
	struct cramfs_super cramfs;
	int fd;

	fd = open(SAVE_FILE, O_RDONLY);
	read(fd, &cramfs, sizeof(cramfs));
	lseek(fd, cramfs.size, SEEK_SET);
	read(fd, s, sizeof(*s));
	close(fd);

	s->signature=ntohl(s->signature);
	if (s->signature != IMAGE_SIGNATURE) {
		//SYSLOG("%s: Unkown image!", __FUNCTION__);
		s->signature = IMAGE_UNKOWN;
		return;
	}

	s->version = ntohl(s->version);
	s->time = ntohl(s->time);
}

void firmware_info(char *val)
{
	struct signature s;

	if (s.signature == 0x0ul)
		get_signature(&s);

	if (s.signature == IMAGE_UNKOWN) {
		printf("Unkown&#160firmware!!!");
		return;
	}

	if (strcmp(val, "version") == 0) {
		printf("%.2d.%.1d%c(%.4d)", \
				(s.version >> 24) / 10, \
				(s.version >> 24) % 10, \
				(s.version >> 16) & 0xff, \
				s.version & 0xffff);
	} else {
		printf("%s", ctime((const time_t *)&s.time));
	}

}

int firmware_info_main(int argc, char *argv[])
{
	firmware_info(argv[1]);
	return 0;
}
