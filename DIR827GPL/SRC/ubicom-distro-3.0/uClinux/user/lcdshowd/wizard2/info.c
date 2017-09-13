#include <stdio.h>
#include <string.h>

static struct {
	const char *item;
	const char *txt;
} *p, itemlist[] = {
	{ "conntype", NULL },
	{ "timezone", NULL },
	{ NULL }
};

int info_save(const char *item, const char *txt)
{
	int ret = -1;

	p = itemlist;
	for (; p && p->item; p++) {
		if (strcmp(p->item, item) == 0) {
			p->txt = txt;
			ret = 0;
			break;
		}
	}

	return ret;
}

const char *info_load(const char *item)
{
	const char *ret = "";

	p = itemlist;
	for (; p && p->item; p++) {
		if (strcmp(p->item, item) == 0) {
			ret = p->txt;
			break;
		}
	}

	return ret;
}
