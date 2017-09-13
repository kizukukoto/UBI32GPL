#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
//#include "../sutil/project.h"
#include <fcntl.h>
#include <artblock.h>

struct nvram_t {
        char name[40];
#ifdef sl3516
        char value[256];
#else
        char value[192];
#endif
};
#define HW_LAN_WAN_OFFSET(mtd_size)  ((mtd_size) - 100)
struct nvram_t SettingInFlash[4];

//#define NVRAM_DEBUG 1
#ifdef NVRAM_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

//#define ARTBLOCK_DEBUG 1 
#if ARTBLOCK_DEBUG
#define OTHER_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define OTHER_MSG(fmt, arg...) 
#endif
static char *ART_NAME = "artblock";
static int *get_mtd_partition_index(const char *name)
{
	FILE *fd;
	char *buf[128], p;
	int i;

	if (!name)
		goto out;
	if ((fd  = fopen("/proc/mtd", "r")) == NULL)
		goto out;
	bzero(buf, sizeof(buf));

	while (fgets(buf, sizeof(buf) - 1, fd)) {
		if (sscanf(buf, "mtd%d:", &i) && strstr(buf, name)) {
			fclose(fd);
			return i;
		}	
		bzero(buf, sizeof(buf));
	}
	fclose(fd);
out:
	return -1;
}
static int get_mtd_char_device(const char *name, char *buf)
{
	int i;
	
	if ((i = get_mtd_partition_index(name)) != -1) {
		sprintf(buf, "/dev/mtd%d", i);
		OTHER_MSG("get index:%d from %s\n", i, buf);
	}
	return i;
}

static int get_mtd_size(const char *name)
{
	FILE *fd;
	char *buf[128], p;
	int i, len = -1;

	if (!name)
		goto out;
	if ((fd  = fopen("/proc/mtd", "r")) == NULL)
		goto out;
	bzero(buf, sizeof(buf));

	while (fgets(buf, sizeof(buf) - 1, fd)) {
		if (sscanf(buf, "mtd%d: %X ", &i, &len) && strstr(buf, name)) {
			OTHER_MSG("get size: %d, from %s\n", len, name);
			fclose(fd);
			return len;
		}
		bzero(buf, sizeof(buf));
	}
	fclose(fd);
out:
	return -1;
}

calValue2ram(void)
{
	int i;	
	char *flash_value=NULL;	
	DEBUG_MSG("\n***********calValue2ram:***************\n");
	for(i = 0; i < sizeof(SettingInFlash)/sizeof(struct nvram_t); i++){
		memset(SettingInFlash[i].name, 0, sizeof(SettingInFlash[0].name));
		memset(SettingInFlash[i].value, 0, sizeof(SettingInFlash[0].value));
		strcpy(SettingInFlash[i].name, SettingInFlashName[i]);
		DEBUG_MSG("calValue2ram:SettingInFlash[%d].name=%s\n",i,SettingInFlash[i].name);
		flash_value=artblock_get(SettingInFlash[i].name);
		if(flash_value!=NULL){
			if(strlen(flash_value)){				
				strcpy(SettingInFlash[i].value, flash_value);
				DEBUG_MSG("calValue2ram:SettingInFlash[%d].name=%s,value=%s\n",i,SettingInFlash[i].name,SettingInFlash[i].value);
			}	
			else{
				strcpy(SettingInFlash[i].value, "");
				DEBUG_MSG("calValue2ram:SettingInFlash[%d].name=%s,artblock len value=NULL\n",i,SettingInFlash[i].name);
			}	
		}
		else{
			strcpy(SettingInFlash[i].value, "");
			DEBUG_MSG("calValue2ram:SettingInFlash[%d].name=%s,artblock value=NULL\n",i,SettingInFlash[i].name);
		}			
	}	
}			


static char get_tmp[4][20] = {0};	

int artblock_set(char *name, char *value)
{	
	int mtd_block;
	int i,result = 0, count = 0;
	char *file_buffer;
	char *offset;
	struct hw_lan_wan_t *header_start;
	char mtd_path[128];	
	int mtd_size;

	if( name==NULL || value==NULL){
		printf("artblock_set name/value null\n");
		return 0;
	}	
	

	if ((mtd_size = get_mtd_size(ART_NAME))  == -1) {
		printf("can not get artblock size form /proc/mtd\n");
		return 0;
	}
	file_buffer = malloc(mtd_size);
	
	if (!file_buffer){
		printf("artblock_set file_buffer alloc fail!\n");
		return 0;
	}	
	
	if (get_mtd_char_device(ART_NAME, mtd_path) == -1) {
		printf("can not find artblock from /proc/mtd\n");
		return 0;
	}
	mtd_block = open(mtd_path, O_RDONLY);
		
	if(!mtd_block){
		printf("artblock_set read mtd_block opem fail!\n");
		free(file_buffer);
		return 0;
	}
	
	count = read(mtd_block, file_buffer, mtd_size);
		
	if(count <= 0){ 
		printf("read from %s fail\n",mtd_path);
		close(mtd_block);	
		free(file_buffer);
		return 0;
	}	
	else{ 
		if(count!= mtd_size){
			printf("read from %s size=%d fail!\n", mtd_path, count);
			close(mtd_block);	
			free(file_buffer);
			return 0;
		}	
		close(mtd_block);
		OTHER_MSG("%02d data read from %s success\n", mtd_size, mtd_path);
		
	}
		
	offset = file_buffer+HW_LAN_WAN_OFFSET(mtd_size);	
#if 0//ARTBLOCK_DEBUG
	OTHER_MSG("header_start%x: ",offset);
	for(i=0;i<sizeof(struct hw_lan_wan_t);i++){
		OTHER_MSG("%x-",offset[i]);		
	}	
	OTHER_MSG("\n");	
#endif	
	OTHER_MSG("offset artblock: %p + (mtd_size: %d) %d ,name: %s\n",
		file_buffer, mtd_size, HW_LAN_WAN_OFFSET(mtd_size), name);
	header_start = (struct hw_lan_wan_t*)offset;	
	if(strcmp(name, "hw_version") == 0) {
		strcpy(header_start->hwversion, value);
		OTHER_MSG("hwversion=%s\n",header_start->hwversion);		
	}
	else if(strcmp(name, "lan_mac") == 0) {
		strcpy(header_start->lanmac, value);		
		OTHER_MSG("lanmac=%s\n",header_start->lanmac);
	}	
	else if(strcmp(name, "wan_mac") == 0) {
		strcpy(header_start->wanmac, value);		
		OTHER_MSG("wanmac=%s\n",header_start->wanmac);
	}
	else if(strcmp(name, "wlan0_domain") == 0) {
		strcpy(header_start->domain, value);
		printf("wlan0_domain=%s\n",header_start->domain);	
	}	
	else if(strcmp(name, "clear") == 0) {
		memset(offset,0,100);		
		printf("clear------\n");
	}		
	else{
		printf("unknow name=%s fail!\n",name);
		free(file_buffer);
		return 0;		
	}		
	mtd_block = open(mtd_path, O_WRONLY);

	/*
 	 * XXX: the code from orignal does not erase mtd partition 
 	 * before write to flash, why?
 	 * So. here I erase done here.  otherwise, it will be a
 	 * confuse char in mtd without erase before update data.
 	 * */	
	{
	char erasecmd[128];

	sprintf(erasecmd, "eraseall %s", mtd_path);
	system(erasecmd);
	}
	if(!mtd_block){
		printf("artblock_set write mtd_block opem fail!\n");
		free(file_buffer);
		return 0;
	}
	//printf("ART[%s], %d\n", file_buffer, mtd_size);
	result = write(mtd_block, file_buffer, mtd_size);
	
	if(result <= 0) {
		printf("write to %s fail!\n", mtd_path);
		close(mtd_block);	
		free(file_buffer);
		return 0;
	}	
	else{ 
		if(result!= mtd_size){
			printf("read from %s size=%d fail!\n", mtd_path,count);
			close(mtd_block);	
			free(file_buffer);
			return 0;
		}	
		OTHER_MSG("%02d data write to %s success\n", mtd_size, mtd_path);
		
	}		
		
	close(mtd_block);	
	free(file_buffer);

	return 1;
}

/*
NickChop add
For widget get lan_mac more fast
To reduce CPU resrouce 
*/
char *artblock_fast_get(char *name)
{
	int i;
	char *flash_value=NULL;	
	for(i = 0; i < sizeof(SettingInFlash)/sizeof(struct nvram_t); i++)
	{	      
	   if(!strlen(SettingInFlash[i].name)){
			calValue2ram();
			if(!strlen(SettingInFlash[i].name))
			{
				DEBUG_MSG("artblock_fast_get: %s in SettingInFlash is empty\n",name);
				return NULL;
	   }	
		}
		
		DEBUG_MSG("artblock_fast_get:SettingInFlash[%d].name=%s\n",i,SettingInFlash[i].name);
		
	   if(strcmp(name, SettingInFlash[i].name) == 0) 
       {  
            if(strcmp(SettingInFlash[i].value, "") == 0)
            {
            	DEBUG_MSG("artblock_fast_get: %s value in SettingInFlash is NULL\n", name);
				if(flash_value=artblock_safe_get(name))
				{
					if(strlen(flash_value))
					{
						strcpy(SettingInFlash[i].value, flash_value);
						DEBUG_MSG("artblock_fast_get:SettingInFlash[%d].name=%s.artblock value=%s\n",i,SettingInFlash[i].name,SettingInFlash[i].value);
						return  SettingInFlash[i].value;
					}
					else
					{				
						DEBUG_MSG("artblock_fast_get: artblock_safe_get: %s value is empty\n", name);		
						return NULL;
					}	
				}
				else
				{			
					DEBUG_MSG("artblock_fast_get: artblock_safe_get: %s value is NULL\n", name);			
					return NULL;
				}	
			}	
			else
			{	
				DEBUG_MSG("artblock_fast_get:SettingInFlash[%d].name=%s.value=%s\n",i,SettingInFlash[i].name,SettingInFlash[i].value);
				return  SettingInFlash[i].value;
			}	
	   } 
	}
	return NULL;
}

char *artblock_get(char *name)
{
	int mtd_block=0;
	int i,result = 0, count = 0;
	char *file_buffer=NULL;
	char *offset=NULL;
	struct hw_lan_wan_t *header_start;
	char *nvram_wmac = NULL;
	char mtd_path[128];
	int mtd_size;
	OTHER_MSG("artblock_get start\n");

	if( name==NULL ){
		printf("artblock_get name null\n");
		return NULL;
	}	
	
	if ((mtd_size = get_mtd_size(ART_NAME))  == -1) {
		printf("can not get artblock size form /proc/mtd\n");
		return NULL;
	}
	file_buffer = malloc(mtd_size);
		
	if (!file_buffer){
		printf("artblock_get file_buffer alloc fail!\n");
		return NULL;
	}	
	
	if (get_mtd_char_device(ART_NAME, mtd_path) == -1) {
		printf("can not find artblock from /proc/mtd\n");
		goto err;
	}
	mtd_block = open(mtd_path, O_RDONLY);
		
	if(!mtd_block){
		printf("artblock_get read mtd_block opem fail!\n");
		goto err;
	}
	
	count = read(mtd_block, file_buffer, mtd_size);
		
	if(count <= 0){ 
		printf("read from %s fail\n", mtd_path);
		close(mtd_block);	
		goto err;
	}	
	else{ 
		if(count != mtd_size){
			printf("read from %s size=%d fail!\n", mtd_path, count);
			close(mtd_block);	
			goto err;
		}	
		close(mtd_block);
		OTHER_MSG("%02d data read from %s success\n", mtd_size, mtd_path);
	}
		
	offset = file_buffer+HW_LAN_WAN_OFFSET(mtd_size);	
#if 0//ARTBLOCK_DEBUG
	OTHER_MSG("header_start%x: ",offset);
	for(i=0;i<sizeof(struct hw_lan_wan_t);i++){
		OTHER_MSG("%x-",offset[i]);		
	}	
	OTHER_MSG("\n");	
#endif	
	OTHER_MSG("offset artblock: %p + (mtd_size: %d) %d ,name: %s\n",
		file_buffer, mtd_size, HW_LAN_WAN_OFFSET(mtd_size), name);
	header_start = (struct hw_lan_wan_t*)offset;	
	if(strcmp(name, "hw_version") == 0) {		
		memcpy(get_tmp[0],header_start->hwversion,sizeof(header_start->hwversion));
		if(strlen(get_tmp[0])){
        	if(get_tmp[0][0]==0xffffffff && get_tmp[0][1]==0xffffffff){
             	OTHER_MSG("hw_version=NULL\n");
             	goto err;
            }
            else
            {
             	OTHER_MSG("hw_version=%s\n",get_tmp[0]);
             	free(file_buffer);	
             	return get_tmp[0];
            }
		}	
		else
             goto err;  
	}
	else if(strcmp(name, "lan_mac") == 0) {		
		memcpy(get_tmp[1],header_start->lanmac,sizeof(header_start->lanmac));
            	OTHER_MSG("%s:%d tmp [%s]\n", __FUNCTION__, __LINE__, get_tmp[1]);
		if(strlen(get_tmp[1])){
			if(get_tmp[1][0]==0xffffffff && get_tmp[1][1]==0xffffffff){
            	OTHER_MSG("lanmac=NULL\n");
            	goto err;
            }
            else
            {
            	OTHER_MSG("%s:%d\n", __FUNCTION__, __LINE__);
                OTHER_MSG("lanmac=%s\n",get_tmp[1]);
                free(file_buffer);
                return get_tmp[1];
            }
		}
		else
        	goto err;					
	}	
	else if(strcmp(name, "wan_mac") == 0) {			
		memcpy(get_tmp[2],header_start->wanmac,sizeof(header_start->wanmac));
		if(strlen(get_tmp[2])){
			if(get_tmp[2][0]==0xffffffff && get_tmp[2][1]==0xffffffff){
				OTHER_MSG("wanmac=NULL\n");
				goto err;
			} else {
				OTHER_MSG("wanmac=%s\n",get_tmp[2]);
				free(file_buffer);
				/* FIXME: have no idea how to merge, caller should check, and determine
				 * which mac they want by themselves, eg: from artblock, or nvram...		
				 * artblock should just care about artblock partition
				 */
#if 0
				nvram_wmac = nvram_safe_get("wan_mac");
				if (!strcmp(nvram_wmac, "00:01:23:45:67:8a"))
					return get_tmp[2];
				else 
				   	return nvram_wmac;
//#else
				FILE *fp = popen("nvram get wan_mac", "r");
				char wan_tmp[128], wan_key_nvram[18], eq[12], wan_mac_nvram[18];

				if (fp == NULL)
					goto err;

				bzero(wan_mac_nvram, sizeof(wan_mac_nvram));
				fscanf(fp, "%[^\n]", wan_tmp);
				pclose(fp);

				if (strlen(wan_tmp) > strlen("wan_mac = "))
					sscanf(wan_tmp, "%s %s %s", wan_key_nvram, eq, wan_mac_nvram);
				else
					strcpy(wan_mac_nvram, "00:01:23:45:67:8a");

				if ((strcmp(wan_mac_nvram, "00:01:23:45:67:8a") == 0 &&
					get_tmp[2][0] == 0xFF && get_tmp[2][1] == 0xFF) ||
					strcmp(wan_mac_nvram, "00:01:23:45:67:8a") != 0)
					strcpy(get_tmp[2], wan_mac_nvram);
#endif
				return get_tmp[2];
//------------------				   	
			}	
		}	
		else 
             goto err;
	}	
	else if(strcmp(name, "wlan0_domain") == 0) {			
		memcpy(get_tmp[3],header_start->domain,sizeof(header_start->domain));
		if(strlen(get_tmp[3])){
			if(get_tmp[3][0]==0xffffffff && get_tmp[3][1]==0xffffffff){
            	OTHER_MSG("wlan0_domain=NULL\n");
            	goto err;
			}
			else
			{
				OTHER_MSG("wlan0_domain=%s\n",get_tmp[3]);
				free(file_buffer);
				return get_tmp[3];
			}	
		}	
		else 
        	goto err;
	}	
	else{
		printf("unknow name=%s fail!\n",name);
		goto err;
	}		
err:	
	free(file_buffer);	
	
	return NULL;
}

char *artblock_safe_get(char *name)
{
	static char *data=NULL;
	if( ( data = artblock_get(name)) )
		return data;
	else 
		return "";
}
