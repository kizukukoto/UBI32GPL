#include <stdio.h>
#include <stdlib.h>
#include "libdb.h"

#define __DEBUG_MSG(fmt, arg...)
#define EMPTY(str)	(str == NULL)

extern char *nvram_get(const char*);

int nvram_safe_set(const char *key, const char *value)
{
	int ret = -1;

	if (EMPTY(key) || EMPTY(value))
		goto out;
	if (EMPTY(nvram_get(key)))
		goto out;

	ret = nvram_set(key, value);
out:
	return ret;
}

int update_record(const char *key, const char *value)
{
	if (value == NULL)
		value = "";

	if (nvram_get(key))
		return nvram_set(key, value) == 0 ? 0: -1;

	return -1;
}

int query_vars(char *tag, char *buf, int buf_len)
{
	char *s;
	int ret = -1;

	__DEBUG_MSG("%s, tag = %s, \n",__FUNCTION__,tag);
	s = getenv(tag);
	if (s) {
		strncpy(buf, s, buf_len);
	} else {
        /*
	 * FIXME: jimmy modified 20081023 , this cause seg fault
	 *                   * */
		//if ((s = nvram->get(tag)) == NULL){
		if ((s = nvram_get(tag)) == NULL)
			goto out;

		strncpy(buf, s, buf_len);
	}
	ret = 0;
out:
	__DEBUG_MSG("%s, return ret = %d\n",__FUNCTION__,ret);
	return ret;
}
