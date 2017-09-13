#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmds.h"
#include "nvram.h"
#include "shutils.h"
#include "nvram_ext.h"

/*
 * ex: vdev_1 = "WAN:WAN_primary,WAN_slave"
 * */
#define VDEV_PREFIX	"vdev_"
#define VDEV_MAX	10
int vdev_get_group(const char *group, char *pbuf[], char *buf)
{
	char tmp[] = "vdev_XXX", *p, *s;
	int rev = -1, i;

	for (i = 0; i < 10; i++) {
		int len = 0;
		sprintf(tmp, "vdev_%d", i);
		p = nvram_safe_get(tmp);
		if (strncmp(group, p,  strlen(group)) != 0)
			continue;
		
		printf("p=%s\n", p);	       
		if (*(p += strlen(group)) != ':') // ex: "WAN:" != "WAN_xxx:"
			continue;
		if (*(++p) == '\0')
			continue;
		strcpy(buf, p);
		while (buf != NULL) {
			s = strsep(&buf, ",");
			/* Fix: 
			 * vdev_2="LAN:lan,,," to "lan"
			 * vdev_2="LAN:lan,lan2," to "lan" "lan2"
			 * vdev_2="LAN:lan,,lan2" to "lan" "lan2"
			 * */
			if (*s == '\0') 
				continue;
			printf("s=[%s]\n", s);
			pbuf[len++] = s;
		}
		pbuf[len] = NULL;
		rev = len;
	}
	return rev;
}

int group_main(int argc, char *argv[])
{
	char *s[10], buf[512];
	int i, r;
	
	r = vdev_get_group(argv[1], s, buf);
	for (i = 0; i < r; i++)
		printf("get vdev:[%s]\n", s[i]);
}
