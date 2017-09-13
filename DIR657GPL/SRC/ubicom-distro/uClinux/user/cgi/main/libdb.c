#include <stdio.h>
#include "libdb.h"

extern char *nvram_get(const char*);

int update_record(const char *key, const char *value)
{
        if (value == NULL)
                value = "";

        if (nvram_get(key))
                return nvram_set(key, value) == 0 ? 0: -1;
        return -1;
}
