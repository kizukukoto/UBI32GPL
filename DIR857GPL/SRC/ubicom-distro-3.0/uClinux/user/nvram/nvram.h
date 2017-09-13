#ifndef __NVRAM_H
#define __NVRAM_H
struct nvram_t {
	char name[40];
#ifdef UBICOM_JFFS2
	char value[192];
#else
#ifdef sl3516
	char value[256];
#else
	char value[256];
#endif
#endif
};

//#define NVRAM_FILEIDX_OFFSET 0xD800 //65535-10*1024
#define NVRAM_FILEIDX_OFFSET 0

// 0x10000 == 64*1024 == 65536
// file idx uses the first part of the last 10*1024
// 64-10=54*1024=0xD800

#define EOF_NVRAM "~end_of_nvram~" 

struct nvram_fileidx_t {
	char name[40];
	unsigned int  len;
	unsigned int  offset;
};

//extern int nvram_init(void);
int nvram_show(void);
int nvram_upgrade(void);
char *nvram_get(const char *name);
char *nvram_safe_get(const char *name);
int nvram_set(const char *name, const char *value);
int nvram_unset(const char *name);
int nvram_del(const char *name);
int nvram_count(const char *name);
int nvram_match(const char *name, const char *value);
int nvram_commit(void);
int nvram_restore_default(void);
int nvram_config2file(void);
int nvram_file_save(const char *name, const char *file);
int nvram_file_restore(const char *name, const char *file);
int nvram_replace(const char *name, const char *value);
int nvram_writefile(const char *name);
int nvram_readfile(const char *name);
int nvram_WiPNP(void);
char *substitute_keyword(char *string,char *source_key,char *dest_key);

/*
*   Date:	2009-05-26
*   Name:	jimmy huang
*   Reason:	To avoid compile warning
*			../nvram/nvram.h:42: warning: function declaration isn't a prototype
*   Notice : add void as for valid function declaration 
*/
void nvram_initfile(void);

void nvram_flag_reset(void);

#ifdef SET_GET_FROM_ARTBLOCK
void calValue2ram(void);
int artblock_set(char *name, char *value);
char *artblock_get(char *name);
char *artblock_safe_get(char *name);
char *artblock_fast_get(char *name); //for widget
#endif

#ifdef CONFIG_VLAN_ROUTER
#define CONF_BUF 30*1024
#else
#define CONF_BUF 30*1024
#endif  
  
#ifndef CONFIG_NVRAM_BLK
//#define CONFIG_NVRAM_BLK "/dev/mtdblock1"
#define CONFIG_NVRAM_BLK "/etc/nvram"
#endif

/*
* Albert Chen 2009-06-26 
* Detail take off not necessary define 
*/
//#ifdef ATHEROS_11N_DRIVER  
//#define SYS_CONF_MTDBLK CONFIG_NVRAM_BLK
//#else
//#define SYS_CONF_MTDBLK "/dev/mtdblock/1"
#define SYS_CONF_MTDBLK "/etc/nvram"
//#endif

#define NVRAM_FILE "/var/etc/nvram.conf"
#define NVRAM_SIZE_COUNT "/var/etc/nvram_count.txt"

#endif //__NVRAM_H
