#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libdb.h"

int auth_local(int argc, char *argv[])
{
	int ret = -1;
	char *group, *gp, *p, _group[1024];
	char gname[128];
	
	if((getenv("USERNAME") == NULL) || 
	   (strcmp(getenv("AUTH_METHOD"), "DB")) != 0)
		goto out;

	sprintf(gname, "auth_group%s", getenv("DB_INDEX"));
	if ((group = nvram_safe_get(gname)) == NULL)
		goto out;
	
	strcpy(_group, group);
	p = _group;

	while ((gp = strsep(&p, "/")) != NULL) {
		char uid[128], pwd[128];

		sscanf(gp, "%[^,],%s", uid, pwd);
		if (strcmp(getenv("USERNAME"), uid) == 0 &&
			strcmp(getenv("PASSWORD"), pwd) == 0)
			return 0;
	}

out:
	return ret;
}

