#define NVRAM_VALUE_SIZE        192	//the same as nvram.h
#define INIT_NVRAM_VALUE(s)     s=(char *)malloc(NVRAM_VALUE_SIZE);memset(s,0,NVRAM_VALUE_SIZE)
#define FREE_NVRAM_VALUE(s)	if(s) free(s)

char *cmd_nvram_get(const char *name, char *value);
char *cmd_nvram_safe_get(const char *name, char *value);
int cmd_nvram_set(const char *name, const char *value);
int cmd_nvram_commit(void);
int cmd_nvram_match(const char *name, const char*value);
