#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>
#include "cmds.h"

struct sysver {
	int orig_major, orig_minor;
	int new_major, new_minor;
};
static int commit;

static void nvram_new_version_fn(const char *k, const char *v, void *data)
{
	struct sysver *ver = data;
	
	if (strcmp(k, "sys_config_version_major") == 0)
		ver->new_major = atoi(v);
	else if (strcmp(k, "sys_config_version_minor") == 0)
		ver->new_minor = atoi(v);
	else
		return;
}

static int check_nvram_config_version(const char *sys_info, int *commit)
{
	char *m,*n;
	struct sysver sysver;
	
	m = nvram_safe_get("sys_config_version_major");
	n = nvram_safe_get("sys_config_version_minor");
	if (strlen(m) == 0|| strlen(n) == 0)
		goto do_restore_deafult; /* witout config version in my f/w */
	memset(&sysver, -1, sizeof(sysver));
	sysver.orig_major = atoi(m);
	sysver.orig_minor = atoi(n);

	foreach_nvram_from(sys_info, nvram_new_version_fn, &sysver);
	printf("new ver: %d.%d\nold ver: %d.%d\n",
		sysver.new_major, sysver.new_minor,
		sysver.orig_major, sysver.orig_minor);
	if (sysver.new_major == -1 || sysver.new_minor == -1)
		return 0; 	/* without config version in new f/w. */
	printf("new ver: %d.%d\nold ver: %d.%d\n",
		sysver.new_major, sysver.new_minor,
		sysver.orig_major, sysver.orig_minor);
	/* tes all cases of major/minor */
	if (sysver.new_major > sysver.orig_major)
		goto do_restore_deafult; /* restore, make sure major/minor key is set. */
	if (sysver.new_major < sysver.orig_major)
		goto undefine; /* '<' download... undfine  */
	if (sysver.new_minor > sysver.orig_minor)
		goto add_new_items;
	if (sysver.new_minor < sysver.orig_minor)
		goto  undefine; /* '<' download... undfine */
	return 0;
do_restore_deafult:
	nvram_restore_default();
	nvram_set("restore_default", "0");
	return 0;
add_new_items:
	nvram_set("sys_nvram_version_minor", n);
	*commit += 1;
	/* pass to nvram_sanity_check to do it */
	return 0;
undefine:
	printf("new ver: %d.%d\nold ver: %d.%d\n",
		sysver.new_major, sysver.new_minor,
		sysver.orig_major, sysver.orig_minor);
	printf("nvram version is undefined");
	return 0;
}

/**
 * Init static nvram from keys sys_XXXX, and from artblock.
 */


static void fn_nvram_set(const char *k, const char *v, void *unused)
{
	
	if (strncmp("sys_", k, strlen("sys_")) == 0 ||
	    strncmp("feature_", k, strlen("feature_")) == 0)
	{
		nvram_set(k, v);
		return;
	}
	fprintf(stderr, "XXX key:%s, value:%s was not initial\n", k, v);
	return;
}

static int fn_is_valid_wlan_domain(const char *k, char *domain)
{
	if (domain == NULL)
		return 0;
	if (strlen(domain) != 4) //eg: 0x10,
		return 0;
	/* artblock_get("wlan0_domain") will return "   " when mtd is empty */
	if (strncmp(domain, "0x", 2) != 0)
		return 0;
	return 1;
}

static int fn_is_valid_hw_ver(const char *k, char *ver)
{
	if (ver == NULL)
		return 0;
	if (strlen(ver) != 2)	//eg: "A1"
		return 0;
	return 1;
}
static int fn_is_valid_macaddr(const char *k, char *mac)
{
	char *hex, buf[20], *e = NULL;
	int i;

	if (mac == NULL)
		goto fmt_err;
	if (mac[17] != '\0') // "00:00:00:00:00:00";
		goto fmt_err;
	strcpy(buf, mac);
	mac = buf;
	for (i = 0; i < 6; i++) {
		if ((hex = strsep(&mac, ":")) == NULL)
			goto fmt_err;

		if (strlen(hex) > 2)	/* err on hex: "123", "#@", "!o".. ? */
			goto fmt_err;
		e = NULL;
       		if ((strtoul(hex, &e, 16)) > 0xFF)
			goto fmt_err;
		if (*e != '\0')
			goto fmt_err;
	}
	return 1;	//true in valid
fmt_err:
	printf("%s have a error MAC fromat\n", k);
	return 0;
}

static int art2nvram()
{
	int rev = 0, _true;
	char *s;
	struct {
		char *art;
		char *nvram;
		char *_default;
		int (*fn_is_valid)(const char *, char *); //return valid if true(!=0)
	} a2n[] = {
		{"lan_mac",	"sys_lan_mac",	"00:01:23:45:67:89", fn_is_valid_macaddr},
		{"wan_mac",	"sys_wan_mac",	"00:01:23:45:67:8a", fn_is_valid_macaddr},
		{"hw_version",	"sys_hw_version", "XX", fn_is_valid_hw_ver},
		{"wlan0_domain","sys_wlan0_domain", "0x00", fn_is_valid_wlan_domain},
		{NULL, NULL, NULL, NULL},
	}, *p;

	for (p = a2n; p->art != NULL; p++) {
		s = artblock_get(p->art);
		if (!s && p->_default) {
			s = p->_default;
			printf("artblock err, reset:%s=%s\n", p->art, s);
			artblock_set(p->art, s);
		}
		//printf("get %s=%s from artblock\n", p->art, s);
		_true = p->fn_is_valid ? p->fn_is_valid(p->art, s): 1;

		if (!_true && p->_default) {
			s = p->_default;
			printf("artblock fmt err: reset:%s=%s\n", p->art, s);
			artblock_set(p->art, s);
		}
		if (s) {
			printf("init artblock :%s=%s to \"%s\"\n",
						p->art, s, p->nvram);
			nvram_set(p->nvram, s);
		} else {
			printf("unknow how to init key:%s\n", p->nvram);
			rev = -1;
		}
	}
	return rev;
}
static int init_main(int argc, char *argv[])
{
	char *sys_info = "/etc/sysinfo";
	int rev = -1;
	int commit = 0;
	if (argc >= 2)
		sys_info = argv[1];
	
	check_nvram_config_version(sys_info, &commit);
	if ((rev = foreach_nvram_from(sys_info, fn_nvram_set, NULL)) == -1)
		goto help;
	if (commit)
		nvram_commit();
	return art2nvram();
help:
	printf("use %s [file], default: %s\n", argv[0], "/etc/sysinfo");
	return 0;
		
}

static int show_main(int argc, char *argv[])
{
	return 0;
}
int sysinfo_main(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "show", show_main, "info of system"},
		{ "init", init_main, "used to init info from file and artblock at booting."},
		{ NULL, NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
