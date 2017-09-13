#include <stdio.h>
#include <string.h>
#include "libdb.h"

int get_os_version(int argc, char *argv[])
{
	char os_version[20];
	char os_version_[20];
	char *tmp = NULL;

	bzero(os_version, sizeof(os_version));
	bzero(os_version_, sizeof(os_version_));
	query_record("os_version", os_version, sizeof(os_version));

	tmp = strstr(os_version, "B");
	if(tmp == NULL)
		printf("%s", os_version);
	else {
		memcpy(os_version_, os_version, strstr(os_version, "B") - os_version);
		printf("%s", os_version_);
	}

	return 1;
}
