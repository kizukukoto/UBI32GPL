#include <stdio.h>
#include "cmds.h"
#include "libcameo.h"

static int utility_dns_append(int argc ,char *argv[])
{
	if (argc != 2)
		return -1;

	dns_append(argv[1]);
	return 0;
}

static int utility_dns_delete(int argc ,char *argv[])
{
	if (argc != 2)
		return -1;

	dns_delete(argv[1]);
	return 0;
}

static int utility_dns_insert(int argc ,char *argv[])
{
	if (argc != 2)
		return -1;

	dns_insert(argv[1]);
	return 0;
}

static int utility_dns_get(int argc, char *argv[])
{
	char buf[1024];
	int num;

	bzero(buf, sizeof(buf));
	num = dns_get(buf);
	printf("%d: %s\n", num, buf);

	return 0;
}

int utility_dns_main(int argc, char *argv[])
{
        struct subcmd cmds[] = {
                { "add", utility_dns_append},
                { "del", utility_dns_delete},
		{ "ins", utility_dns_insert},
		{ "get", utility_dns_get},
                { NULL, NULL}
        };
        return lookup_subcmd_then_callout(argc, argv, cmds);
}
