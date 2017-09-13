#ifndef __NVRAM_COMPATIBLE_H__
#define __NVRAM_COMPATIBLE_H__
extern int nvram_upgrade(void);
#define nvram_del(name)	nvram_unset(name)
extern int nvram_restore_default(void);
extern int nvram_eraseall(char **);
extern int nvram_config2file(void);


/* debug nvram ops */
#define DEBUG_NVRAM_NOP
#ifdef DEBUG_NVRAM_NOP
#define cprintf(fmt, args...) do {		\
	FILE *fp = fopen("/dev/console", "w");	\
	if (fp) {				\
		fprintf(fp, fmt, ## args);	\
		fclose(fp);			\
	}					\
} while (0)

#define NVRAM_NOP(ops, rev) ({					\
	cprintf("%s at pid %d:%s->%s:%d\n",			\
	ops, getpid(), __FILE__, __FUNCTION__, __LINE__);	\
	rev;							\
})

#define NVRAM_NOP_NOREV(ops)					\
	cprintf("%s at pid %d:%s->%s:%d\n",			\
	ops, getpid(), __FILE__, __FUNCTION__, __LINE__)

#else
#define NVRAM_NOP(ops, rev)	do { } while(0)
#define NVRAM_NOP_NOREV(ops)	do { } while(0)
#endif	//DEBUG_NVRAM_NOP

#define nvram_file_save(name, file) NVRAM_NOP("nvram_file_save", 0)
#define nvram_file_restore(name,file) NVRAM_NOP("nvram_file_restore", 0)
#define nvram_flag_reset()	NVRAM_NOP("nvram_flag_reset", 0)
#define nvram_writefile(name)	NVRAM_NOP("nvram_writefile", 0)
#define nvram_readfile(name)	NVRAM_NOP("nvram_readfile", 0)
//#define nvram_flag_set()	NVRAM_NOP_NOREV("nvram_flag_set")
#define nvram_flag_set()	0
#define nvram_count(name)	NVRAM_NOP("nvram_count", -1)

#define char2int(data) (data - '0')
#define EOF_NVRAM "~end_of_nvram~" 
#define nvram_replace(n, v) (nvram_get(n) == NULL) ? -1: nvram_set(n, v)


//#define NVRAM_FILE "/etc/nvram.conf"
#define NVRAM_DEFAULT "/etc/nvram.default"
#define NVRAM_UPGRADE_TMP    "/var/nvram.tmp" /* XXX: copy from shvar.h */
#define NVRAM_CONFIG_TMP     "/var/nvram.bin" /* XXX: copy from shvar.h */

/*
 * extension functions
 * */

/*
 *  read from @file
 *  each line will be formated as "<key>=<value>" then call @fn
 *  @data: by pass to @fn as extra parameter
 * */
extern int foreach_nvram_from(const char *file, void (*fn)(const char *key, const char *value, void *data), void *data);
/*
 *  read from flash/mtd
 *  each line will be formated as "<key>=<value>" then call @fn
 *  @data: by pass to @fn as extra parameter
 * */
extern int foreach_nvram_from_mtd(void (*fn)(const char *key, const char *value, void *data), void *data);
#endif // __NVRAM_COMPATIBLE_H__
