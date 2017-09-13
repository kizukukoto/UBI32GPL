#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "hnap.h"

int do_xml_create_mime(char *xml_resraw)
{
	int idx;
	char ctime_str[128];
	time_t lt = time(NULL);

	strcpy(ctime_str, ctime(&lt));
	idx = strlen(ctime_str) - 1;

	for (; isspace(ctime_str[idx]); idx--)
		ctime_str[idx] = '\0';

	strcat(xml_resraw, "Server: Embedded HTTP Server ,Firmware Version 1.00\r\n");
	sprintf(xml_resraw, "%sDate: %s\r\n", xml_resraw, ctime_str);
	strcat(xml_resraw, "Content-type: text/xml\r\n");
	strcat(xml_resraw, "Connection: close\r\n\r\n");

	return 0;
}
