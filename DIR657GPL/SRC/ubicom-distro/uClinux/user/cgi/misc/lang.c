#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ssi.h"
#include "lang.h"
#include "libdb.h"
#include "partition.h"

struct __lang_struct {
	const char *param;
	const char *desc;
	int (*fn)(int, char *[]);
};

static int check_lang_pack()
{
	if (access(LP_MTD_MOUNT, F_OK) != 0)
		system("mkdir /tmp/lang_pack");
	if (access(LP_TARGETNAME, F_OK) != 0)
		system("echo \"\" > /tmp/lang_pack/lang.js");
	return 0;
}

static int misc_lang_init(int argc, char *argv[])
{
	int ret = -1;
	char cmds[128];

	if (strcmp(nvram_safe_get(LP_SIZE_KEY), "0") == 0) {
		sprintf(cmds, "echo > %s", LP_LINKNAME);
		system(cmds);
		goto out;
	}

	unlink(LP_LINKNAME);
	lang_pack_umount();
	lang_pack_mount();

	if (access(LP_MTD_MOUNT, F_OK) != 0)
		goto out;
	if (access(LP_TARGETNAME, F_OK) != 0)
		goto out;

	sprintf(cmds, "ln -s %s %s", LP_TARGETNAME, LP_LINKNAME);
	system(cmds);

	ret = 0;
out:
	if (ret == -1)
		check_lang_pack();
	return ret;
}

static int misc_lang_info(int argc, char *argv[])
{
	char mtd[128];

	if (strcmp(nvram_safe_get(LP_SIZE_KEY), "0") == 0)
		goto out;

	if (get_mtd_char_device("language_pack", mtd) == -1)
		goto out;

	cprintf("partition: %s\n", mtd);
	cprintf("size: %s\n", nvram_safe_get(LP_SIZE_KEY));
	cprintf("version: %s\n", nvram_safe_get(LP_VERSION_KEY));
	cprintf("region: %s\n", nvram_safe_get(LP_REGION_KEY));
	cprintf("date: %s\n\n", nvram_safe_get(LP_DATE_KEY));

out:
	return 0;
}

static int show_language()
{
	int i = 0;
	char version[10], *ver, *buildNum;
	struct {
		const char *s;
		const char *lang;
	} *p, lang[] = {
		{ "ZH", "TAIWAN"},
		{ "JP", "JAPAN"},
		{ "EN", "ENGLISH"},
		{ "CH", "CHINA"},
		{ "GE", "GERMANY"},
		{ "FR", "FRANCH"},
		{ "IT", "ITALY"},
		{ "KO", "KOREA"},
		{ "ES", ""},
		{ NULL, NULL}
	};
	for ( p = lang; p->s; p++) {
		if (strcmp(nvram_safe_get("lang_region"), p->s) == 0) {
			break;
		}
	}

	if (p->lang == NULL)
		goto out;
	
	sprintf(version, "%s", nvram_safe_get("lang_version"));
	buildNum = version;
	ver = strsep(&buildNum, "b");
	printf("Language: %s<br>", p->lang);
	printf("Language-Version: ver%s%sb%s<br>", ver, p->s, buildNum);
	printf("Language-Checksum: ");
	fflush(stdout);
	system("/bin/checksum lang");
out:
	return 0;
}

static int misc_multi_lang(int argc, char *argv[])
{
	cprintf("Show Multi Language\n");
	char checksum[12];
		
	memset(checksum, '\0', sizeof(checksum));

	if (strcmp(nvram_safe_get("sys_lang_en"), "none") == 0) {
		printf("Language: DISABLE");
	} else {
		if ((nvram_safe_get("lang_region"), "0") == 0) 
			return -1;
		else {
			if (strcmp(nvram_safe_get("lang_region"), "0") == 0) {
				printf("Language: DEFAULT<br>");
				printf("Language-Version: ver0.00b00<br>");
				printf("Language-Checksum: 0x00000000");
				return 0;
			} else {
				show_language();
			}
		}
	}
	return 0;
}

static int misc_lang_erase(int argc, char *argv[])
{
	return lang_pack_erase();
}

static int misc_lang_help(struct __lang_struct *p)
{
	cprintf("Usage:\n");

	for (; p && p->param; p++)
		cprintf("\t%s: %s\n", p->param, p->desc);

	cprintf("\n");
	return 0;
}

int lang_pack_main(int argc, char *argv[])
{
	int ret = -1;
	struct __lang_struct *p, list[] = {
		{ "init", "Symbol linking into mtd/XX.js", misc_lang_init },
		{ "info", "Display language pack info", misc_lang_info },
		{ "multi-lang", "Show Multi-Language", misc_multi_lang },
		{ "erase", "Clear all language pack", misc_lang_erase },
		{ NULL, NULL, NULL }
	};

	if (argc == 1 || strcmp(argv[1], "help") == 0)
		goto help;

	for (p = list; p && p->param; p++) {
		if (strcmp(argv[1], p->param) == 0) {
			ret = p->fn(argc - 1, argv + 1);
			goto out;
		}
	}
help:
	ret = misc_lang_help(list);
out:
	return ret;
}
