#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "cmds.h"

#define TMP_DIR "/tmp"

/* if the tail of filename is not .st, then return -1 */
static int utility_is_openvpnlog(const char *st)
{
	char __tf[128];

	if (st == NULL || strlen(st) < 3)
		return -1;

	bzero(__tf, sizeof(__tf));
	strncpy(__tf, st + strlen(st) - 3, sizeof(__tf));
	if (strcmp(__tf, ".st") == 0)
		return 0;

	return -1;
}

static int utility_openvpn_duration_time(const char *st_file)
{
	struct stat buf;
	time_t cur_time = time(NULL);

	stat(st_file, &buf);
	return cur_time - buf.st_mtime;
}

static void utility_openvpn_status(const char *st_file)
{
	FILE *fp;
	char __st_file[128];
	char log_hostnet[24];
	char log_remoteaddr[24];
	int dorig, dh, dm, ds;

	bzero(log_hostnet, sizeof(log_hostnet));
	bzero(log_remoteaddr, sizeof(log_remoteaddr));
	bzero(__st_file, sizeof(__st_file));
	sprintf(__st_file, "%s/%s", TMP_DIR, st_file);

	if ((fp = fopen(__st_file, "r")) == NULL)
		return;

	/* FIXME: tomorrow continue...don't forget!! */
	
	fscanf(fp, "%[^,],%s", log_hostnet, log_remoteaddr);
	fclose(fp);

	dorig = utility_openvpn_duration_time(st_file);

	dh = dorig/60/60;
	dorig -= (dh * 60 * 60);
	dm = dorig / 60;
	dorig -= (dm * 60);
	ds = dorig;

	printf("sslvpn,%s,%s,%02d:%02d:%02d#", log_hostnet, log_remoteaddr, dh, dm, ds);
}

int utility_openvpn_main(int argc, char *argv[])
{
	DIR *dp;
	int rev = -1;
	struct dirent *entry;
	struct stat statbuf;

	if ((dp = opendir(TMP_DIR)) == NULL)
		goto out;

	chdir(TMP_DIR);
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (!S_ISDIR(statbuf.st_mode) &&
			utility_is_openvpnlog(entry->d_name) != -1)
			utility_openvpn_status(entry->d_name);
	}

	rev = 0;
	chdir("..");
	closedir(dp);
out:
	return rev;
}
