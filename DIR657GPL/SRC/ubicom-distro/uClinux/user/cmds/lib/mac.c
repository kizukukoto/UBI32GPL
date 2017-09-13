#include <sys/types.h>
#include "libcameo.h"
#include "shutils.h"

#if 0
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

int getmac(int cnt, char *macaddr)
{
	int idx = 0;
	char buf[1024];
	char mkey[5];
	FILE *fp = NULL;

	if (cnt < 1 || cnt > 3)
		return -1;
	if ((fp = fopen(MTD_PARTITION, "r")) == NULL)
		return -1;

	bzero(buf, sizeof(buf));
	bzero(mkey, sizeof(mkey));

	sprintf(mkey, "MAC%d", cnt);
	fread(buf, sizeof(buf), 1, fp);

	for (; idx < sizeof(buf) - strlen(mkey); idx++) {
		char tmpkey[5];

		bzero(tmpkey, sizeof(tmpkey));
		memcpy(tmpkey, &buf[idx], strlen(mkey));
		if (strcmp(tmpkey, mkey) == 0) {
			memcpy(macaddr, &buf[idx] + 7, 12);
			break;
		}
	}

	fclose(fp);
}

int clone_mac(const char *prefix, const char *alias, const char *dev)
{
	int idx = -1;
	char compl_key[128];
	char compl_mac[128];

	if (!strstr(dev, "eth"))
		return;

	bzero(compl_key, sizeof(compl_key));
	bzero(compl_mac, sizeof(compl_mac));
	sprintf(compl_key, "%s_alias", prefix);
	sprintf(compl_mac, "%s_mac", prefix);

	DEBUG_MSG("prefix: %s, alias: %s, dev: %s\n", prefix, alias, dev);
	DEBUG_MSG("compl_key: %s, compl_mac: %s\n", compl_key, compl_mac);

	if ((idx = nvram_find_index(compl_key, alias)) != -1) {
		DEBUG_MSG("idx: %d\n", idx);
		sprintf(compl_mac, "%s%d", compl_mac, idx);
		set_clone_mac(dev, nvram_get(compl_mac));
	}

	return 1;
}

int set_mtu(const char *prefix, const char *alias, const char *dev)
{
	int idx = -1;
	char compl_key[128];
	char compl_mtu[128];

	bzero(compl_key, sizeof(compl_key));
	bzero(compl_mtu, sizeof(compl_mtu));
	sprintf(compl_key, "%s_alias", prefix);
	sprintf(compl_mtu, "%s_mtu", prefix);

	DEBUG_MSG("prefix: %s, alias: %s, dev: %s\n", prefix, alias, dev);
	DEBUG_MSG("compl_key: %s, compl_mtu: %s\n", compl_key, compl_mtu);

	if ((idx = nvram_find_index(compl_key, alias)) != -1) {
		DEBUG_MSG("idx: %d\n", idx);
		sprintf(compl_mtu, "%s%d", compl_mtu, idx);
		set_dev_mtu(dev, 0, atoi(nvram_get(compl_mtu)));
	}

	return 1;
}
