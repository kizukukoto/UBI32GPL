#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int nvram_i = 0;
static struct {
	const char *key;
	char *txt;
} nvram_data[256];

int wz_nvram_reset()
{
	int i = 0;

	for (; i < nvram_i; i++)
		free(nvram_data[i].txt);
	nvram_i = 0;

	return 0;
}

int wz_nvram_set(const char *key, const char *txt)
{
	int  i = 0;

	printf("wz_nvram_set %s=%s\n", key, txt);

	for (; i < nvram_i; i++) {
		if (strcmp(nvram_data[i].key, key) == 0) {
			free(nvram_data[i].txt);
			nvram_data[i].txt = strdup(txt);
			goto out;
		}
	}

	nvram_data[i].key = key;
	nvram_data[i].txt = strdup(txt);
	nvram_i++;
out:
	return 0;
}

char *wz_nvram_get(const char *key)
{
	return nvram_get(key);
}

char *wz_nvram_safe_get(const char *key)
{
	return nvram_safe_get(key);
}

char *wz_nvram_get_local(const char *key)
{
	int i = 0;

	for (; i < nvram_i; i++)
		if (strcmp(nvram_data[i].key, key) == 0)
			return nvram_data[i].txt;

	return "";
}

int wz_nvram_commit()
{
	int i = 0;

	for (; i < nvram_i; i++)
		nvram_set(nvram_data[i].key, nvram_data[i].txt);
	nvram_commit();
	return 0;
}
