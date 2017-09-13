#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "convert.h"
#include "shutils.h"
#include "cmds.h"
int convert_env(struct zone_convert_struct *z)
{
	char *e;
	int rev = -1;

	for (; z->dest != NULL; z++) {

		if (z->src == NULL && z->flags & ZF_COPY_PRIV) {
			debug("setenv([%s], [%s])", z->dest, z->priv);
			setenv1(z->dest, z->priv);
			continue;
		}
		
		if ((e = getenv(z->src)) == NULL && z->flags & ZF_NO_NULL) {
			debug("%s:no env: %s\n", __FUNCTION__, z->src);
			break;
		}
		debug("setenv([%s], [%s])", z->dest, e);
		setenv1(z->dest, e);
	}
	rev = 0;
out:
	return rev;
}

