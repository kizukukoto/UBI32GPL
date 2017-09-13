//#define I386
#include <stdio.h>
#include <string.h>
#ifndef I386
#include "libdb.h"
#endif

int get_os_date(int argc, char *argv[])
{
	char buf[80];
	char mon[10], day[10], years[10];

	bzero(buf, sizeof(buf));
	bzero(mon, sizeof(mon));
	bzero(day, sizeof(day));
	bzero(years, sizeof(years));

	query_record("os_date", buf, sizeof(buf));
	sscanf(buf, "%s %s %s", mon, day, years);
	printf("%s, %s, %s", day, mon, years);

	return 0;
}
