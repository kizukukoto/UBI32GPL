#include <stdio.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ssi.h"
#include "lang.h"
#include "libdb.h"
#include "partition.h"

/* Porting from DIR-615 apps/httpd/lp.c */
static int __lang_pack_get_info(char *hdr, char *result, const char *pattern)
{
	regex_t preg;
	int regex_flag = REG_EXTENDED | REG_ICASE;
	size_t nmatch = 10;
	regmatch_t pmatch[ nmatch ];
	int len = -1, i = 0, rc = 0;

	if (regcomp( & preg, pattern ,regex_flag)!=0) /* Regexp compare fail */
		goto out;

	rc = regexec(&preg, hdr, nmatch, pmatch, 0);
	regfree(&preg);

	if (rc != 0)    /* Not Match */
		goto out;

	for (i = 0; i < nmatch && pmatch[i].rm_so >= 0; ++i) {
		len = pmatch[i].rm_eo - pmatch[i].rm_so;
		if (len > 0) {
			strncpy(result, hdr + pmatch[i].rm_so, len);
			result[len] ='\0';
		}
	}
out:
	return len;
}

int lang_pack_umount()
{
	char cmds[128];

	sprintf(cmds, "umount %s", LP_MTD_MOUNT);
	system(cmds);
	sprintf(cmds, "rm -rf %s", LP_MTD_MOUNT);
	system(cmds);

	return 0;
}

int lang_pack_mount()
{
	char cmds[128],mtd[128];
	int mtd_num;
	/* for check language_pack partition */
        if ((mtd_num=get_mtd_char_device("language_pack", mtd)) == -1)
                goto out;

	lang_pack_umount();
	sprintf(mtd,"/dev/mtdblock%d", mtd_num);

	sprintf(cmds, "mkdir %s", LP_MTD_MOUNT);
	system(cmds);
	//sprintf(cmds, "mount -t squashfs %s %s", LP_MTD_BLOCK, LP_MTD_MOUNT);
	sprintf(cmds, "mount -t squashfs %s %s", mtd, LP_MTD_MOUNT);
	system(cmds);

	return 0;
out:
	cprintf("XXX %s[%d] The language_pack partition error XX\n", __FUNCTION__, __LINE__);
	return -1;
}

int lang_pack_erase()
{
	char cmds[128],mtd[128];
	int mtd_num;
	/* for check language_pack partition */
	if ((mtd_num=get_mtd_char_device("language_pack", mtd)) == -1)
		goto out;

        sprintf(mtd,"/dev/mtdblock%d",mtd_num);

	unlink(LP_LINKNAME);
	lang_pack_umount();
	sprintf(cmds, "echo > %s", mtd);

	nvram_set(LP_SIZE_KEY, "0");
	nvram_set(LP_VERSION_KEY, "0");
	nvram_set(LP_REGION_KEY, "0");
	nvram_set(LP_DATE_KEY, "0");
	nvram_commit();

	return system(cmds);
out:
	cprintf("XXX %s[%d] The language_pack partition error XX\n", __FUNCTION__, __LINE__);
	return -1;
}

int lang_pack_get_info(
	char *hdr,
	char *info_model,
	char *info_version,
	char *info_region,
	char *info_date)
{
	int ret = -1;

	if ((ret = __lang_pack_get_info(hdr, info_model, LP_MODEL_REGEX)) == -1)
		goto out;
	if ((ret = __lang_pack_get_info(hdr += ret, info_version, LP_VER_REGEX)) == -1)
		goto out;
	if ((ret = __lang_pack_get_info(hdr += ret, info_region, LP_REG_REGEX)) == -1)
		goto out;
	if ((ret = __lang_pack_get_info(hdr += ret, info_date, LP_DATE_REGEX)) == -1)
		goto out;
out:
	return ret;
}


int lang_pack_get_header(const char *fdesc, char *hdr)
{
	FILE *fp;
	int ret = -1;

	if (fdesc == NULL || (fp = fopen(fdesc, "rb")) == NULL)
		goto out;
	if (fseek(fp, -LP_HDR_SIZE, SEEK_END) == -1)
		goto leave;
	if (fread(hdr, 1, LP_HDR_SIZE, fp) != LP_HDR_SIZE)
		goto leave;

	ret = 0;
leave:
	fclose(fp);
out:
	return ret;
}

int lang_pack_update_info(const char *fdesc)
{
	int fd, ret = -1;
	char hdr[LP_HDR_SIZE + 1];
	char size[8], model[24], version[24], region[24], date[24];
	struct stat buf;

	if (lang_pack_get_header(fdesc, hdr) != 0)
		goto out;
	if ((ret = lang_pack_get_info(hdr, model, version, region, date)) == -1)
		goto out;
	if ((fd = open(fdesc, O_RDONLY)) == -1)
		goto out;
	if ((ret = fstat(fd, &buf)) != 0)
		goto leave;

	sprintf(size, "%ld", buf.st_size);
	nvram_set(LP_SIZE_KEY, size);
	nvram_set(LP_VERSION_KEY, version);
	nvram_set(LP_REGION_KEY, region);
	nvram_set(LP_DATE_KEY, date);

	nvram_commit();
leave:
	close(fd);
out:
        return ret;
}

int lang_pack_verify(const char *fdesc)
{
	int ret = -1;
	char hdr[LP_HDR_SIZE + 1];
	char model[24], version[24], region[24], date[24];

	if (lang_pack_get_header(fdesc, hdr) != 0)
		goto out;
	ret = lang_pack_get_info(hdr, model, version, region, date);
out:
	return ret;
}
