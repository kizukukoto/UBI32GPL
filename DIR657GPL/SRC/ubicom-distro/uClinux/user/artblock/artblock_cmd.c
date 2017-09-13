#include <stdio.h>
#include <string.h>

extern int artblock_set(char *name, char *value);
extern char *artblock_get(char *key);

static struct artblock_cmd_list {
	const char *cmd;
	const char *desc;
	int (*fn)(int, char **);
};

static int artblock_cmd_lookable(const char *data)
{
	int ret = -1, i = 0;

	for (i; i < strlen(data); i++) {
		if (data[i] < 32 || data[i] > 126)
			goto out;
	}

	ret = 0;
out:
	return ret;
}

static int artblock_cmd_set(int argc, char *argv[])
{
	int ret = -1;

	if (argc != 3)
		goto out;

	ret = artblock_set(argv[1], argv[2]);
out:
	return ret;
}

static int artblock_cmd_get(int argc, char *argv[])
{
	char *data;
	int ret = -1;

	if (argc != 2)
		goto out;

	if ((data = artblock_get(argv[1])) == NULL)
		goto out;
	if (artblock_cmd_lookable(data) == 0)
		ret = printf("%s\n", data);
	else
		printf("empty\n");

out:
	return ret;
}

static int artblock_cmd_usage(struct artblock_cmd_list *p)
{
	printf("usage:\n");
	for (; p && p->cmd; p++)
		printf("\t%s: %s\n", p->cmd, p->desc);

	return -1;
}

static int artblock_cmd_help()
{
	printf("set example:\n");
	printf("\tartblock_cmd set lan_mac 00:11:22:33:44:55\n");
	printf("\tartblock_cmd set wan_mac 00:12:34:56:78:90\n");
	printf("\tartblock_cmd set wlan0_domain 0x10\n");
	printf("\tartblock_cmd set hw_version A1\n\n");
	printf("get example:\n");
	printf("\tartblock_cmd get lan_mac\n");
	printf("\tartblock_cmd get wan_mac\n");
	printf("\tartblock_cmd get wlan0_domain\n");
	printf("\tartblock_cmd get hw_version\n\n");

	return 0;
}

int main(int argc, char *argv[])
{
	struct artblock_cmd_list *p, list[] = {
		{ "set", "set artblock value into cal partition", artblock_cmd_set},
		{ "get", "get artblock value from cal partition", artblock_cmd_get},
		{ "help", "help", artblock_cmd_help},
		{ NULL, NULL, NULL}
	};

	if (argc <= 1)
		goto out;

	for (p = list; p && p->cmd; p++) {
		if (strcmp(p->cmd, argv[1]) == 0 && p->fn)
			return p->fn(--argc, ++argv);
	}

out:
	return artblock_cmd_usage(list);
}
