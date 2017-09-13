#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scmd_nvram.h"
//#include "sutil.h"

#ifdef SUTIL_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

#ifdef LINUX_NVRAM
extern int _system(const char *fmt, ...);
char *cmd_nvram_get(const char *key, char *buf)
{
	FILE *fp;
	char tbuf[2048], cmds[128], *ptr_buf;

	if (key == NULL || *key == '\0')
		goto out;

	bzero(tbuf, sizeof(tbuf));
	sprintf(cmds, "nvram get %s", key);

	if ((fp = popen(cmds, "r")) == NULL)
		goto out;

	fgets(tbuf, sizeof(tbuf), fp);
	pclose(fp);

	if (*tbuf == '\0')
		goto out;
	if (tbuf[strlen(tbuf) -1] == '\r' || tbuf[strlen(tbuf) - 1] == '\n')
		tbuf[strlen(tbuf) - 1] = '\0';
	DEBUG_MSG("XXX cmd_nvram_get.[%s] -> [%s]\n", key, tbuf);
	ptr_buf = tbuf; ptr_buf += strlen(key) + 3;
        sprintf(cmds, "%s", ptr_buf);
        return strcpy(buf, cmds);
out:
	return NULL;
}
#else
char *cmd_nvram_get(const char *name, char *value){
        char buffer[NVRAM_VALUE_SIZE] = {};
        FILE *fp;
        char *nvram_value=NULL;
        char parse_file[64] = {"/var/etc/test"};
        int pid;
        
        pid=getpid();
        if(pid<0)
        	return NULL;
        	
        sprintf(parse_file,"/var/etc/%d",pid);
        _system("nvram get %s > %s",name,parse_file);
        
        fp = fopen(parse_file,"r");
        if(!fp) {
                printf("cmd_nvram_get: %s open fail\n",parse_file);
                return NULL;
        }
        fgets(buffer,sizeof(buffer),fp);
        fclose(fp);
        _system("rm -f %s",parse_file);
        
        if(strlen(buffer)<1){
                printf("cmd_nvram_get:%s not exsit.\n", name);
                return NULL;
        }
        
        nvram_value = strstr(buffer,"=");
        if(nvram_value==NULL)
        	return NULL;
        
        nvram_value = nvram_value + 2;
        
        nvram_value[strlen(nvram_value)-1] = '\0';
        sprintf(value,"%s",nvram_value);
        return value;
}
#endif

char *cmd_nvram_safe_get(const char *name, char *value){
        if(!cmd_nvram_get(name,value))
		value="";
        return value;
                
}

int cmd_nvram_set(const char *name, const char *value){
        return _system("nvram set %s=%s", name, value);
}

int cmd_nvram_commit(void){
        return _system("nvram commit");
}

#ifdef LINUX_NVRAM
int cmd_nvram_match(const char *key, const char *value)
{
	char *context, tbuf[NVRAM_VALUE_SIZE];

	bzero(tbuf, sizeof(tbuf));
	if (key == NULL || *key == '\0' || value == NULL)
		goto out;

	DEBUG_MSG("XXX cmd_nvram_match.key [%s]\n", key);
	context = cmd_nvram_get(key, tbuf);
	if (context == NULL)
		goto out;

	DEBUG_MSG("XXX cmd_nvram_match.context [%s] <-> tbuf [%s]\n", context, tbuf);
	if (strcmp(context, value) == 0)
		return 0;
out:
	return -1;
}
#else
int cmd_nvram_match(const char *name, const char*value){
	int result;
	char *INIT_NVRAM_VALUE(tmp_value);
	if(!cmd_nvram_get(name,tmp_value)){
		FREE_NVRAM_VALUE(tmp_value);
		return 1;
	}
	result = strcmp(tmp_value,value);
	FREE_NVRAM_VALUE(tmp_value);
	return result;
}
#endif

