#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/compiler.h>
#include <mtd/mtd-user.h>
#include "nvram.h"

//#define NVRAM_DEBUG
#ifdef NVRAM_DEBUG
#undef DEBUG_MSG(fmt, arg...)
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#undef DEBUG_MSG(fmt, arg...)
#define DEBUG_MSG(fmt, arg...)
#endif

static char *chop(char *io)
{
	char *p;

	p = io + strlen(io) - 1;
	while ( *p == '\n' || *p == '\r') {
		*p = '\0';
		p--;
	}
	return io;
}

static int str2nvram(char *s, struct nvram_tuple *nv)
{
	char *p;
	
	chop(s);
	if (*s == '#' || *s == ' ' || *s == '\t' || *s == '/')
		goto out;
	if ((p = strsep(&s, "=")) == NULL)
		goto out;
	if (s == NULL)
		goto out;
	if ((nv->name = strdup(p)) == NULL)
		goto out;
	if ((nv->value = strdup(s)) == NULL)
		goto nomem2;
	return 0;
nomem2:
	free(nv->name);
out:
	return -1;
}

struct nvram_tuple *init_nvram_tuples(const char *filename)
{
	FILE *fd;
	char buf[4096];
	struct nvram_tuple *np, *n = NULL;

	if ((fd = fopen(filename, "r")) == NULL){
		fprintf(stderr, "%s: openfile :%s fail\n",__func__,filename);
		goto out;
	}	
	while (fgets(buf, sizeof(buf), fd) != NULL) {
		if ((np = malloc(sizeof(struct nvram_tuple))) == NULL){
			fprintf(stderr, "%s: malloc nvram_tuple fail\n",__func__);
			goto out;
		}	
		if (str2nvram(buf, np) == -1)
			continue;
		np->next = n;
		n = np;
	}
out:
	if (fd != NULL)
		fclose(fd);
		
	return n;
}

static void uninit_nvram_tuples(struct nvram_tuple *p)
{
	struct nvram_tuple *n;
	
	while (p != NULL) {
		n = p->next;
		free(p->name);
		free(p->value);
		free(p);
		p = n;
	}
	
}

static void fn_nvram_set(const char *n, const char *v, void *unused)
{
	DEBUG_MSG("%s: name=%s,value=%s\n",__func__,n,v);
	nvram_set(n, v);
}
/**********************************
 * extension helper
 * ********************************/

int foreach_nvram_from(const char *file, void (*fn)(const char *key, const char *value, void *data), void *data)
{
	struct nvram_tuple *n, *h;
	
	if ((h = init_nvram_tuples(file)) == NULL){
		fprintf(stderr, "init_nvram_tuples fail\n");
		return -1;
	}	
	printf("openfile :%s\n", file);
	for (n = h;n != NULL; n = n->next) {
		fn(n->name, n->value, data);
	}
	uninit_nvram_tuples(h);
	return 0;
}

int foreach_nvram_from_mtd(void (*fn)(const char *key, const char *value, void *data), void *data)
{
	char *name, *k, *v, *buf;
	int nvram_space, rev = -1;
	
	if ((nvram_space = nvram_get_nvramspace()) <= 0) {
		fprintf(stderr, "can not get space size from nvram device\n");
			return -1;
	}
	if ((buf = malloc(nvram_space)) == NULL) {
		fprintf(stderr, "can not malloc()\n");
			return -1;
	}

	//fprintf(stderr, "%s:%d, size: %d\n", __FUNCTION__, __LINE__, sizeof(buf));
	if (nvram_getall(buf, nvram_space) != 0)
		goto out;
	//for (name = buf; *name; name += strlen(name) + 1) {
	for (name = buf; *name; ) {
		v = name;
		name += strlen(name) + 1;	/* next key value */
		k = strsep(&v, "=");
		//fprintf(stderr, "%s:%d: [%s:%s]\n", __FUNCTION__, __LINE__, k, v);
		fn(k, v, data);
	}
	rev = 0;
out:
	free(buf);
	return rev;
}

/*************************************
 * public shared libraries
 * ***********************************/
/* @argv[0] := eraseall
 * @argv[1] := $(MTDDEV)
 * @argv[2] := NULL
 * */
int nvram_eraseall(char **argv)
{
	int i = 0, fd = -1, ret = -1;
	mtd_info_t mtd_info;
	erase_info_t erase_info;

	if (argv[1] == NULL){
		fprintf(stderr, "%s: loss argv\n",__func__);
		goto out;
	}	
	if (access(argv[1], F_OK) != 0){
		fprintf(stderr, "%s: access %s fail\n",__func__,argv[1]);
		goto out;
	}	

	if ((fd = open(argv[1], O_SYNC | O_RDWR)) < 0){
		fprintf(stderr, "%s: open %s fail\n",__func__,argv[1]);
		goto out;
	}	
	
	if (ioctl(fd, MEMGETINFO, &mtd_info) != 0){
		fprintf(stderr, "%s: ioctl MEMGETINFO fail\n",__func__);
		goto out;
	}	

	erase_info.start = 0;
	erase_info.length = mtd_info.erasesize;

	for (; i < mtd_info.size / mtd_info.erasesize; i++) {
		if (ioctl(fd, MEMERASE, &erase_info) != 0){
			fprintf(stderr, "%s: ioctl MEMERASE fail\n",__func__);
			goto out;
		}	
		erase_info.start += mtd_info.erasesize;
	}

	ret = 0;
out:
	printf("mtd block=%s Size: %08X (%08X erased)\n",
		argv[1],
		mtd_info.size,
		erase_info.start);

	if (fd != -1) close(fd);
	if (errno) perror("eraseall");
	return ret;
}

int nvram_restore_default(void)
{
	int res = 0;

	printf("%s\n",__func__);
	if((res = foreach_nvram_from(NVRAM_DEFAULT, fn_nvram_set, NULL)) == 0)
		nvram_commit();
	return res;
}

int nvram_compatible_args(char **argv, int *rev)
{
	if (!strncmp(*argv, "restore_default", 15)) {
		*rev = nvram_restore_default();
	} else if (!strncmp(*argv, "eraseall", 8)) {
		nvram_eraseall(argv++);
	} else if (!strncmp(*argv, "upgrade", 7)) {
		nvram_upgrade();
		nvram_commit();
	} else
		return -1;	//no args processed.
	return 0;
}

int nvram_upgrade(void)
{
	int rev = -1;

	if ((rev = foreach_nvram_from(NVRAM_UPGRADE_TMP, fn_nvram_set, NULL)) == -1)
		rev = nvram_restore_default();
	else	/*XXX: The file should be removed by caller */
		if (rev == 0)
			remove(NVRAM_UPGRADE_TMP);	
	
	return rev;
}

static void fn_nvram_download(const char *k, const char *v, void *data)
{
	char  *protection[] = {
		NULL,
		"wan_mac",
		"lan_mac",
		"lan_eth",
		"lan_bridge",
		"wan_eth",
		"wlan0_domain",
		"asp_temp_",
		"html_",
		NULL
	}, **p;
	FILE *fd = data;
	int protect = 0;
	
	//fprintf(fd, "%s=:%s\n", k, v == NULL ? "" : k);
	for (p = protection; *p != NULL; p++) {
		if (strcmp(k, *p) != 0)
			continue;
		protect = 1;
		break;
	}
	if (!protect)
		fprintf(fd, "%s=%s\n", k, v == NULL ? "" : v);
	else
		fprintf(stderr, "ignored : [%s:%s]\n", k, v);
}

int nvram_config2file(void)
{
	FILE *fd;
	int rev = -1;
	
	/* XXX: should not be hard code! */
	if ((fd = fopen(NVRAM_CONFIG_TMP, "a")) == NULL)
		return -1;
	rev = foreach_nvram_from_mtd(fn_nvram_download, fd);
	fclose(fd);
	return rev;
}

#if 0
int main(int argc, char *argv[])
{
	struct nvram_tuple *n;
	
	//if ((n = init_nvram_tuples("/etc/nvram.default")) == NULL)
	if ((n = init_nvram_tuples(argv[1])) == NULL)
		return -1;
	for (;n != NULL; n = n->next) {
		printf("[%s], [%s]\n", n->name, n->value);
	}
}
#endif
