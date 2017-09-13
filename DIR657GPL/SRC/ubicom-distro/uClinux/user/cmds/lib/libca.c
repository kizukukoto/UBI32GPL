#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvramcmd.h>
#include "cmds.h"

int https_pem(const char *ca)
{
	FILE *fd;
	char buf[128], pem[4096], *pub, *priv;
	int rev = -1, i;

	if (ca == NULL || *ca == '\0')
		goto out;
	if (strcmp(nvram_safe_get("https_pem"), "") == 0)
		goto out;
	strcpy(buf, nvram_safe_get("https_pem"));

	i = atoi(buf + strlen("x509_"));
	sprintf(buf, "x509_ca_%d", i);

	if (strcmp(nvram_safe_get(buf), "") == 0)
		goto out;
	strcpy(pem, nvram_safe_get(buf));

	pub = pem;
	priv =  strsep(&pub, ",");

	if ((fd = fopen(ca, "w")) == NULL)
		goto out;
	fwrite(pub, strlen(pub), 1, fd);
	fwrite(priv, strlen(priv), 1, fd);
	rev = 0;
	fclose(fd);
out:
	return rev;
}
