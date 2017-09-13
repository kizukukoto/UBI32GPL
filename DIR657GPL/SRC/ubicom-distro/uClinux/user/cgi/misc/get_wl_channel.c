#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int get_wl_channel_main(int argc, char *argv[])
{
	FILE *fd;
	char *channel, buf[64];

	if ((fd =  popen("/usr/sbin/wl channel |grep target", "r")) == NULL)
		return -1;

	fgets(buf, sizeof(buf), fd);
	pclose(fd);
	channel = strrchr(buf, '\t');
	channel++;
	printf("%d", atoi(channel));

	return 0;
}
