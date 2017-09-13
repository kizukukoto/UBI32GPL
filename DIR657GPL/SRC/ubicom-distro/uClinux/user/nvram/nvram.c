#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#include <shvar.h>
#include <nvram.h>
#include <fcntl.h>
#include "project.h"

/*
 * NVRAM Debug Message Define
 */
//#define NVRAM_DEBUG 1
#ifdef NVRAM_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg);
#else
#define DEBUG_MSG(fmt, arg...)
#endif

//#define ARTBLOCK_DEBUG 1
#ifdef NVRAM_DEBUG_ARTBLOCK
#define OTHER_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg);
#else
#define OTHER_MSG(fmt, arg...)
#endif

#define NVRAM_DEFAULT "/var/etc/nvram.default"
#define NVRAM_DEFAULT_COUNTER "/var/etc/nvram_default_counter"
#define STORAGE_DEFAULT_FOLDER USBMOUNT_FOLDER

#ifdef CONFIG_NVRAM_SIZE
#define NVRAM_SIZE CONFIG_NVRAM_SIZE
#else
#define NVRAM_SIZE 1024
#endif

#define NVRAM_FILE_IDX_SIZE 5

#ifdef UBICOM_JFFS2
#define NVRAM_EACH_SIZE 256
#else
#define NVRAM_EACH_SIZE 296
#endif

static struct nvram_t nvram_array[NVRAM_SIZE]={0};
static struct nvram_fileidx_t nvram_fileidx[NVRAM_FILE_IDX_SIZE]={0};
void nvram_flag_set(void);
static void replace_special_char(char *sourcechar);
char *substitute_keyword(char *string,char *source_key,char *dest_key);
static int nvram_read_pid(char *file);
int nvram_init_flag = 0;
int nvram_save_flag = 0;

/*
* Name Albert Chen
* Date 2009-02-17
* Detail: direct access flash reduce CPU resrouce.
*         transfer flash data to ram
*/
#ifdef SET_GET_FROM_ARTBLOCK
struct nvram_t SettingInFlash[4];

char *SettingInFlashName[4] =
{
	"lan_mac",
	"wan_mac",
	"hw_version",
	"wlan0_domain"
};

void calValue2ram(void)
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
#endif

/* mac+1 */
char *mac_plus_1(char *lan_mac){
	char *index_t = NULL;
	static char wan_mac[20] = {};
	int i = 0;

	if(lan_mac == NULL)
		return "";

	memcpy(wan_mac,lan_mac,strlen(lan_mac));
	index_t = &wan_mac[0]+strlen(&wan_mac[0])-1;
	for(i=0;i<strlen(&wan_mac[0]);i++){
		/* i%3!=2 means the char ":" */
		if( i%3 != 2 ){
			if(*(index_t - i) == '9'){
				*(index_t - i) = 'a';
			}else if(*(index_t - i) == 'F' || *(index_t - i) == 'f'){
				*(index_t - i) = '0';
				continue;
			}else{
				*(index_t - i) = *(index_t - i)+1;
			}
			break;
		}
	}
	return wan_mac;
}


/* If nvram.conf doesn't exist, cp default to nvram.conf */
int check_file_exist(void)
{
	struct stat buf;
	int restore=0;

	if( stat(NVRAM_FILE, &buf) ==0 ) {
		if( buf.st_size ==0 )
			restore = 1;
	} else {
		restore = 1;
	}

	if( restore == 1) {
		printf("nvram.conf doesn't exist, cp nvram.default to nvram.conf\n");
		system("cp /etc/nvram.default /var/etc/nvram.conf");
	}
	return 0;
}

static char original_wlan0_domain[5]={0};

/* Function Name: nvram_init
 * Author: Solo Wu
 * Comment:
 *	Read the nvram.conf and copy into nvram_array
 */
int nvram_init(void)
{
	int  fp;
	/* jimmy modified , to let some compiler happy */
	//char *line, *tail, *head, *ptr;
	char line[NVRAM_EACH_SIZE] = {0};
	char *tail, *head, *ptr;
	/* ------------------------------------------- */
	char *data=calloc(1,sizeof(nvram_array));
	int i=0;
	int j=0;
	/* ---------------------- */

	/*
		NickChou, 2009/2/25
		Because cmd_nvram_get() in apps\sutil\scmd_nvram.c
		We can not add any debug message in "nvram get" command process
	*/
	//DEBUG_MSG("Read the nvram.conf and copy into nvram_array\n");

	if (data == NULL) {
		printf("nvram allocate memory failed\n");
		return -1;
    	}
	memset(data, 0, sizeof(nvram_array));

	if ((fp =open (NVRAM_FILE, O_RDWR ))==0 ) {
        	printf("nvram open %s failed\n", NVRAM_FILE);
		free(data);
		return -1;
	}

	if (read(fp, data, sizeof(nvram_array)) == 0) {
        	printf("nvram read from file failed\n");
		close(fp);
		free(data);
		return -1;
	}

	head = data;
	//line = (char *)malloc(NVRAM_EACH_SIZE);

	j = 0;
	while( (tail=strstr(head,"\n"))  )
	{
		memset(line,0,NVRAM_EACH_SIZE);
		if((tail-head) < NVRAM_EACH_SIZE)
		{
			strncpy(line, head, tail-head);
		 	//avoid buf overflow
                	if(i > (NVRAM_SIZE - 1))
                        	break;
                	if(j > (NVRAM_SIZE - 1))
                        	break;

			if ( (ptr=strstr(line,"=")) )
			{
				strncpy(nvram_array[i].name, line, ptr-line);
				strncpy(nvram_array[i].value, ptr+1, strlen(ptr));

				/*NickChou, keep original wlan0_domain*/
				if(strcmp(nvram_array[i].name, "wlan0_domain") == 0)
					strncpy(original_wlan0_domain, nvram_array[i].value, strlen(ptr));
				i++;
			}
			head = tail+1;
		}
		j++;
	}

/* jimmy added 20080623 to avoid lost the data if the last character of last line is not '\n',
ex:
aa=aa\n
b=b\0
 */
	if((strchr(head,'='))&&(i < (NVRAM_SIZE - 1))){
		memset(line,0,NVRAM_EACH_SIZE);
		j = 0;
		while(1){
			if((head[j] == '\0') || (head[j] == '\n')){
				break;
			}
			line[j] = head[j];
			j++;
		}
		line[j]='\0';

		if ( (ptr=strstr(line,"=")) )
		{
			strncpy(nvram_array[i].name, line, ptr-line);
			strncpy(nvram_array[i].value, ptr+1, strlen(ptr));
			/*NickChou, keep original wlan0_domain*/
			if(strcmp(nvram_array[i].name, "wlan0_domain") == 0)
				strncpy(original_wlan0_domain, nvram_array[i].value, strlen(ptr));
		}
	}
	/* -------------------------------------------------------------- */

	close(fp);
	/*
		NickChou, 2009/2/25
		Because cmd_nvram_get() in apps\sutil\scmd_nvram.c
		We can not add any debug message in "nvram get" command process
	*/
	//DEBUG_MSG("nvram init finished, size=%d\n", i);
	free(data);
	return i;
}


/*Read the nvram.default and copy into default_array*/
int nvram_default_init(struct nvram_t *default_array)
{
	FILE *fp;
	char *line, *tail, *head, *ptr;
	char *data=calloc(1,NVRAM_SIZE*sizeof(struct nvram_t));
	int i=0;
	/* jimmy added , 20080623 */
	int j=0;
	/* ---------------------- */
	FILE *fp_count;

	DEBUG_MSG("nvram_default_init\n");

	if (data == NULL) {
		printf("nvram_default_init: alloc data fail\n");
		return -1;
	}
	memset(data, 0, sizeof(nvram_array));

	if ((fp =fopen (NVRAM_DEFAULT, "r")) == 0) {
		printf("nvram_default_init: open NVRAM_DEFAULT fail\n");
		free(data);
		return -1;
	}

	if (fread(data, 1, NVRAM_SIZE*sizeof(struct nvram_t), fp) == 0) {
		printf("nvram_default_init: read NVRAM_DEFAULT fail\n");
		fclose(fp);
		free(data);
		return -1;
	}

	head = data;
	line = (char *)malloc(NVRAM_EACH_SIZE);
	if (!line) {
		printf("nvram_default_init: malloc fail\n");
		fclose(fp);
		free(data);
		return -1;
	}

	while( (tail=strstr(head,"\n"))  ) {
		memset(line,0,NVRAM_EACH_SIZE);
		strncpy(line, head, tail-head);

		if ( (ptr=strstr(line,"=")) ) {
			strncpy(default_array[i].name, line, ptr-line);
			strncpy(default_array[i].value, ptr+1, strlen(ptr));
		} else
			break;
		i++;
		head = tail+1;
	}

#if 0
	// avoid overflow
	fp_count=fopen(NVRAM_DEFAULT_COUNTER, "w+");
	if(fp_count)
	{
		fprintf(fp_count, "%d\n", i);
		printf("nvram_default_init: default nvram count=%d\n", i);
		fclose(fp_count);
	}
#endif

//jimmy added to avoid lost the data if the last character of last line is not '\n',
/* ex:
   aa=aa\n
   b=b\0
 */
	if(strchr(head,'=')){
		memset(line,0,NVRAM_EACH_SIZE);
		j = 0;
		while(1){
			if((head[j] == '\0') || (head[j] == '\n')){
				break;
			}
			line[j] = head[j];
			j++;
		}
		line[j]='\0';

		if ( (ptr=strstr(line,"=")) )
		{
			strncpy(default_array[i].name, line, ptr-line);
			strncpy(default_array[i].value, ptr+1, strlen(ptr));
		}
	}
	/* ------------------------------------------------------------- */

	fclose(fp);
	free(line);
	free(data);
//#ifdef SET_GET_FROM_ARTBLOCK
//	calValue2ram();
//#endif

	DEBUG_MSG("nvram_default_init finished\n");
	return 0;
}

int nvram_show(void)
{
	int i=0;

	if(nvram_init() < 0)
		return -1;
	for(i=0;i<NVRAM_SIZE;i++) {
		if(strlen(nvram_array[i].name)==0)
			break;
		printf(" %s\t%s\n", nvram_array[i].name, nvram_array[i].value);
	}
	return 0;
}

char *nvram_default_search(const char *name)
{
	int i;
	struct nvram_t *default_array=calloc(1, sizeof(nvram_array));
	static char value[256];

	if(default_array == NULL) return NULL;
	nvram_default_init(&default_array[0]);
	
	DEBUG_MSG("nvram_default_search - %s is not in /var/etc/nvram.conf\n",name);
	for (i=0;i<NVRAM_SIZE;i++) {
	    if((name[0] == default_array[i].name[0])&&(name[4] == default_array[i].name[4]))
	    {
		if (strcmp(name, default_array[i].name) == 0) {
			DEBUG_MSG("default_array - %s\t%s\n", default_array[i].name, default_array[i].value);
			strcpy(value, default_array[i].value);
			free(default_array);
			return value;
		}
	}
	}
	free(default_array);
	return NULL;
}

char *nvram_search(const char *name)
{
	int i;
	int ps_pid;

	ps_pid = getpid();
	if ((ps_pid == nvram_read_pid(HTTPD_PID)) || //((ps_pid == nvram_read_pid(RC_PID)) || (ps_pid == nvram_read_pid(HTTPD_PID)) ||
		(ps_pid == nvram_read_pid(DCC_PID))) {
		/*when nvram_get, rc only init nvram once*/
		if (nvram_init_flag == 0)
		{
			DEBUG_MSG("pid = %d, only init nvram once\n", ps_pid);
			nvram_init_flag = 1;
			if (nvram_init() < 0)
				exit(0);
		}
	}
	else {
	    if (nvram_init() < 0)
		    exit(0);
	}

	for (i=0;i<NVRAM_SIZE;i++) {
	    if((name[0] == nvram_array[i].name[0])&&(name[4] == nvram_array[i].name[4]))
	    {
		if (strcmp(name, nvram_array[i].name) == 0) {
			return  nvram_array[i].value;
		}
	    }
	}

	return nvram_default_search(name);
}


/* Function Name: nvram_count
 * Author: Solo Wu
 * Comment:
 *	Count and return the number of rules. e.g. fw_mac0, fw_mac1, ...
 */
int nvram_count(const char *name)
{
	int count=0;
	static char *data=NULL;
	char target[64];

	while(1) {
		sprintf(target, "%s%d", name, count);
		data = nvram_search(target);
		if( data )  {
			if( strlen(data) ==0)
				break;
			count += 1;
		} else
			break;
	}

	return count;
}


/* Function Name: nvram_save
 * Author: Solo Wu
 * Comment:
 *	Save the nvram value into nvram.conf
 */
int nvram_save(void)
{
	int fp;
	char *data=calloc(1, sizeof(nvram_array));
	int i = 0;
	int ps_pid;
	FILE *fp_2;
	char buf[NVRAM_EACH_SIZE] = {};
	//int default_nvram_counter = 0;
	int data_len=0;

	if (data == NULL) {
		printf("nvram_save: malloc data fail\n");
		return 0;
	}
	memset(data,'\0',sizeof(data));

	//default_nvram_counter=nvram_read_pid(NVRAM_DEFAULT_COUNTER);
	//if(default_nvram_counter<0)
	//{
	//	default_nvram_counter=NVRAM_SIZE;
	//	printf("nvram_save: read NVRAM_DEFAULT_COUNTER fail\n");
	//}
	//DEBUG_MSG("nvram_save: default_nvram_counter=%d\n", default_nvram_counter);

	ps_pid = getpid();
	if ((ps_pid == nvram_read_pid(HTTPD_PID)) || (ps_pid == nvram_read_pid(DCC_PID)))
	{
		if (nvram_save_flag == 1)
		{
			fp =open (NVRAM_FILE, O_RDWR);
			if(fp)
			{
				for(i=0; i<NVRAM_SIZE; i++) {
					//if(i >= default_nvram_counter)//avoid overflow
					//	break;
					if(strlen(nvram_array[i].name) ==0 )
			                        break;

					data_len+= strlen(nvram_array[i].name);
					data_len+= strlen(nvram_array[i].value);
					data_len+= 2; // for '\n' and '=' chars

					memset(buf, 0, NVRAM_EACH_SIZE);
					sprintf(buf,"%s=%s\n", nvram_array[i].name, nvram_array[i].value);
					if(i == 0)
					{
						strcpy(data, buf);
					}
					else
					{
						strcat(data, buf);
					}
				}
				DEBUG_MSG("NVRAM HTTPD/DCCD save %d line\n",i);
				write(fp, data, strlen(data));
				ftruncate(fp, data_len);
				close(fp);
				free(data);
				return 0;
			}
			else {
				free(data);
				return 0;
			}
		}
		else {
			free(data);
			return 0;
		}
	}
	else if (ps_pid == nvram_read_pid(NVRAM_UPGRADE_PID))
	{
		if (nvram_save_flag == 1)
		{
			fp =open (NVRAM_FILE, O_RDWR);
			if(fp)
			{
				for(i=0; i<NVRAM_SIZE; i++) {
					//if(i >= default_nvram_counter)//avoid overflow
					//	break;
					if(strlen(nvram_array[i].name) ==0 )
			                        break;

					data_len+= strlen(nvram_array[i].name);
					data_len+= strlen(nvram_array[i].value);
					data_len+= 2; // for '\n' and '=' chars

					memset(buf, 0, NVRAM_EACH_SIZE);
					sprintf(buf,"%s=%s\n", nvram_array[i].name, nvram_array[i].value);
					if(i == 0)
					{
						strcpy(data, buf);
					}
					else
					{
						strcat(data, buf);
					}
				}
				DEBUG_MSG("NVRAM UPGRADE save %d line\n",i);
				write(fp, data, strlen(data));
				ftruncate(fp, data_len);
				close(fp);
				free(data);
        	    		return 0;
			}
			else {
				free(data);
				return 0;
			}
		}
		else {
			free(data);
			return 0;
		}
	}
	else
	{
		DEBUG_MSG("PID %d using nvram_save\n", getpid());
		fp =open (NVRAM_FILE, O_RDWR);

		if(fp) {
			for(i=0; i<NVRAM_SIZE; i++) {
				if(strlen(nvram_array[i].name) ==0 ) {
					DEBUG_MSG("nvram_save: Last NVRAM Item : %d: %s\n", i-1, nvram_array[i-1].name);
					break;
				}
				data_len+= strlen(nvram_array[i].name);
				data_len+= strlen(nvram_array[i].value);
				data_len+= 2; // for '\n' and '=' chars
				memset(buf, 0, NVRAM_EACH_SIZE);
				sprintf(buf,"%s=%s\n", nvram_array[i].name, nvram_array[i].value);
				if(i == 0)
				{
					strcpy(data, buf);
				}
				else
				{
					strcat(data, buf);
				}
			}
			DEBUG_MSG("NVRAM UPGRADE save %d line\n",i);
			write(fp, data, strlen(data));
			ftruncate(fp, data_len);

			close(fp);
		} else {
			printf("nvram_save: open NVRAM_FILE fail\n");
		}
		free(data);
		return 0;
	}
	free(data);
}

int nvram_commit(void)
{
#ifdef UBICOM_JFFS2
	int fp;
	struct stat f_stat;
	if ((fp=open(NVRAM_FILE, O_RDONLY)) == 0) {
		printf("%s not exits \n", NVRAM_FILE);
		system("cp -f /var/etc/nvram.default /etc/nvram");
	}
	else
	{
		if (stat(NVRAM_FILE, &f_stat) == -1) {
			printf("read existing file failed\n");
			return -1;
		}
		close(fp);
		system("cp -rf /var/etc/nvram.conf /etc/nvram");
	}
	system("echo -n ~end_of_nvram~ >> /etc/nvram");
#else
	int fp;
	int mtd_block;
	int result = 0, count = 0;
	char *file_buffer;
	struct stat f_stat;

	DEBUG_MSG("nvram_commit start\n");

	file_buffer = malloc(CONF_BUF);
	if (!file_buffer) {
		printf("nvram memory not enough\n", NVRAM_FILE);
		return -1;
    }
	memset(file_buffer, 0, CONF_BUF);

	if ((fp=open(NVRAM_FILE, O_RDWR)) == 0) {
		free(file_buffer);
		printf("%s not exits \n", NVRAM_FILE);
		return 0;
	}

	if (stat(NVRAM_FILE, &f_stat) == -1) {
		printf("read existing file failed\n");
		return -1;
	}

	count = read(fp, file_buffer, f_stat.st_size);
	DEBUG_MSG("nvram_commit: read file size is %02d, MTDBLK = %s\n", count, SYS_CONF_MTDBLK);

	// add terminate char '~'
	// add terminate string "~end_of_nvram~"
	strcpy(file_buffer+f_stat.st_size, EOF_NVRAM);

	mtd_block = open(SYS_CONF_MTDBLK, O_WRONLY);
	if (!mtd_block) {
		fsync(fp);
		close(fp);
		free(file_buffer);
		printf("nvram open MTDBLK(%s) failed\n", SYS_CONF_MTDBLK);
		return 0;
	}

	result = write(mtd_block, file_buffer, f_stat.st_size + sizeof(EOF_NVRAM)); // add len for terminate string in flash

	if (result <= 0)
		printf("write to %s fail\n", SYS_CONF_MTDBLK);
	else
		printf("%02d data write to %s success\n", CONF_BUF, SYS_CONF_MTDBLK);

	close(mtd_block);
	fsync(fp);
	close(fp);
	free(file_buffer);
#endif
	return 0;
}


int nvram_replace(const char *name, const char *value)
{
	int i;
	for (i=0;i<NVRAM_SIZE;i++) {
	    if((name[0] == nvram_array[i].name[0])&&(name[4] == nvram_array[i].name[4]))
	    {
		if (strcmp(name, nvram_array[i].name) == 0) {
			strcpy(nvram_array[i].value, value);
			DEBUG_MSG("nvram_replace %s=%s\n", name, value);
			return 0;
		}
	    }
	}

	return -1;
}

int nvram_add(const char *name, const char *value)
{
	int i;
	if(nvram_default_search(name) != NULL) return -1;
	for (i=0; i<NVRAM_SIZE; i++) {
		if (strlen(nvram_array[i].name)==0) {
			strcpy(nvram_array[i].name, name);
			strcpy(nvram_array[i].value, value);
			DEBUG_MSG("nvram_add %s=%s\n", name, value);
			return 0;
		}
	}

	return -1;
}

char *nvram_get(const char *name)
{
	if (name == NULL)
		return NULL;
	/*
		NickChou, 2009/2/25
		Because cmd_nvram_get() in apps\sutil\scmd_nvram.c
		We can not add any debug message in "nvram get" command process
	*/
	return nvram_search(name);
}

/* Function Name: nvram_set
 * Author: Solo Wu
 * Comment:
 *	Set nvram setting into nvram.conf. If the setting already existed,
 *	replace it by nvram_replace(), else call nvram_add()
 */
int nvram_set(const char *name, const char *value)
{
	char *data=NULL;
	int ps_pid;

	if (name == NULL)
		return -1;

	DEBUG_MSG("nvram_set %s=%s\n", name, value);

	ps_pid = getpid();

	if ((ps_pid == nvram_read_pid(HTTPD_PID)) || (ps_pid == nvram_read_pid(NVRAM_UPGRADE_PID)) ||
		(ps_pid == nvram_read_pid(DCC_PID))) {
		if (nvram_replace(name, value) == -1)
			nvram_add(name, value);
	}
	else
	{
		DEBUG_MSG("PID %d nvram_set %s\n", getpid(), name);
	    data = nvram_search(name);

	    if (data)
		    nvram_replace(name, value);
	    else
		    nvram_add(name, value);
	}

	nvram_save();
	return 0;
}

int nvram_unset(const char *name)
{
	DEBUG_MSG("nvram unset\n");
	nvram_set(name, NULL);
	return 0;
}

char *nvram_safe_get(const char *name)
{
	static char *data=NULL;
	if( ( data = nvram_get(name)) )
		return data;
	else
		return "";
}

int nvram_match(const char *name, const char *value)
{
	char *data;

	if(name==NULL || value==NULL)
		return -1;

	data = nvram_get(name);

	if(data)
		return strcmp(data, value);

	return -1;
}

void nvram_flag_set()
{
	nvram_save_flag = 1;
	nvram_save();
	nvram_save_flag = 0;
}

char _upgrade_line[1024]={};
#ifdef UBICOM_JFFS2
char _upgrade_nvvalue[192]={0};
#else
char _upgrade_nvvalue[256]={0};
#endif
int nvram_upgrade(void)
{
	int decount = 0, reboot=0;
	int nvcount=0, upgrade = 1, save = 0, nvram_line=0;
	char *line=_upgrade_line, *ptr=NULL;
	FILE *fp;
	FILE *pid_fp;
	struct nvram_t *default_array=calloc(1, sizeof(nvram_array));
#ifndef UBICOM_JFFS2
	char nvname[40]={0}, nvvalue[256]={0};
#else
	char nvname[40]={0}, *nvvalue=_upgrade_nvvalue;
#endif

#ifdef NVRAM_CONF_VER_CTL
	int restore_default = 0;
#endif

	DEBUG_MSG(" !! NVRAM UPGRADE !! NVRAM_SIZE=%d\n", NVRAM_SIZE);
	if (default_array == NULL) {
		printf("alloc default_array fail %d\n", __LINE__);
		return ENOMEM;
	}
	memset(default_array, 0, sizeof(nvram_array));
	nvram_default_init(&default_array[0]);

	/* Check nvram.conf exist */
	check_file_exist();
	nvram_line = nvram_init();
	nvram_init_flag = 1;
	DEBUG_MSG("The line of NVRAM is %d\n", nvram_line);
	
	/* upgrade from web/file */
	if ((fp=fopen(NVRAM_UPGRADE_TMP, "r")) )
	{
		sleep(5);

		//XXX! Commented out for now. no daemon call support in uClinux.
		//if (daemon(1, 1) == -1) {
		//	perror("daemon");
		//	exit(errno);
		//}
		if (!(pid_fp = fopen("/var/run/nvram_upgrade.pid", "w"))) {
			perror("/var/run/nvram_upgrade.pid");
			free(default_array); //John 2010.04.23 free default_array
			return errno;
		}
		fprintf(pid_fp, "%d", getpid());
		fclose(pid_fp);

		DEBUG_MSG("NVRAM upgrade from web/file start\n");

		while(fgets(line,1024,fp)){
			if ( (ptr=strstr(line,"=")) ) {
				strncpy(nvname, line, ptr-line);
				if( strncmp(nvname, "asp_temp_", 9)!=0) {
					strncpy(nvvalue, ptr+1, strlen(ptr)-2);
					strcat(nvvalue, "\0");
					nvram_set(nvname, nvvalue);
				}
			}
			memset(line, 0, 1024);
			memset(nvname, 0, 40);
#ifndef UBICOM_JFFS2
			memset(nvvalue, 0, 256);
#else
			memset(nvvalue, 0, 192);
#endif
		}
		nvram_flag_set();
		fclose(fp);
		remove(NVRAM_UPGRADE_TMP);
		reboot =1;
	}
	else
	{
		/* upgrade new nvram.default to nvram.conf */
		for( decount = 0; decount<NVRAM_SIZE; decount++ )
		{
			for( nvcount=0; nvcount<NVRAM_SIZE; nvcount++)
			{
				if((default_array[decount].name[0] == nvram_array[nvcount].name[0])&&(default_array[decount].name[4] == nvram_array[nvcount].name[4]))
				{
					if( !strcmp( default_array[decount].name, nvram_array[nvcount].name) )
					{
						upgrade = 0;
						break;
					}
				}
				else
					upgrade = 1;
			}
			if((upgrade == 1)&&(nvram_line > 0)&&(nvram_line < (NVRAM_SIZE - 1)))
			{
				DEBUG_MSG("NVRAM lacks something-%s\n",default_array[decount].name);
				if(strcmp(default_array[decount].name, "wlan0_domain") == 0)
				{
					/*NickChou, keep original wlan0_domain*/
					strcpy(nvram_array[nvram_line].name, default_array[decount].name);
					strcpy(nvram_array[nvram_line].value, original_wlan0_domain);
					save = 1;
				}
				else
				{
					strcpy(nvram_array[nvram_line].name, default_array[decount].name);
					strcpy(nvram_array[nvram_line].value, default_array[decount].value);
					DEBUG_MSG("new nvram item: %d : %s=%s\n", decount, nvram_array[decount].name, nvram_array[decount].value);
					save = 1;
#ifdef NVRAM_CONF_VER_CTL
					/*NickChou, Device using Configuration Version Control first time*/
					if(strcmp(default_array[decount].name, "configuration_version") == 0){
						printf("Device using Configuration Version Control => restore_default\n ");
						restore_default = 1;
					}	
#endif					

				}
				nvram_line++;
			}
		}

		if(save==1)
			nvram_save();
	}

	nvram_commit();

#ifdef NVRAM_CONF_VER_CTL
	if(restore_default){		
		reboot =1;
		nvram_restore_default();
	}
#endif

	if(reboot)
		kill(1, SIGTERM);
		
	free(default_array); //John 2010.04.23 free default_array
	return 0;
}


int nvram_restore_default(void)
{
	struct nvram_t *nv;
	struct nvram_t reservation[] = {
		{ "wan_mac", ""},
		{ "lan_mac", ""},
#ifndef UBICOM_MP_SPEED
		{ "lan_eth", ""},
		{ "lan_bridge", ""},
		{ "wan_eth", ""},
#endif
#ifndef RESERVATE_WIRELESS_DOMAIN
		{ "wlan0_domain", ""},
#endif
	};

	printf("nvram_restore_default start\n");

	/* Get reservation value first */
	for(nv=reservation; nv < &reservation[ sizeof(reservation)/ sizeof(reservation[0]) ] ; nv++) {
		if(strcmp(nv->name,"wan_mac") == 0)
			strcpy( nv->value, mac_plus_1(nvram_get("lan_mac")) );
		else
			strcpy( nv->value, nvram_safe_get(nv->name) );
	}

	memset(nvram_array, 0, sizeof(nvram_array));
	system("cp -f /var/etc/nvram.default /var/etc/nvram.conf");

    /* restore reservation value */
	for(nv=reservation; nv < &reservation[ sizeof(reservation)/ sizeof(reservation[0]) ] ; nv++) 	    {
		nvram_set( nv->name, nv->value );
	}

	nvram_commit();

	/* Remove nvram.conf */
	printf("nvram_restore_default finished\n");
	return 0;
}


int nvram_config2file(void)
{
	FILE *fp;
	int protect=0;
#ifdef BCM5352
	char buf[NVRAM_SPACE];
	char *value;
#else
	int nvcount=0;
#endif
	struct nvram_t *nv;
	struct nvram_t protection[] = {
		{ "wan_mac", ""},
		{ "lan_mac", ""},
		{ "lan_eth", ""},
		{ "lan_bridge", ""},
		{ "wan_eth", ""},
		{ "wlan0_domain", ""},
		{ "asp_temp_", ""},
		{ "html_", ""},
	};

	if( (fp =fopen (NVRAM_CONFIG_TMP, "w+"))==0 )
		return -1;

#ifdef BCM5352
	nvram_getall(buf, sizeof(buf));
	for (value = buf; *value; value += strlen(value) + 1) {
		for(nv=protection; nv < &protection[ sizeof(protection)/ sizeof(protection[0]) ] ; nv++) {
			if( strncmp(value, nv->name, strlen(nv->name)) ==0)  {
				protect =1;
				break;
			} else
				protect =0;
		}

		if( !protect )
			fprintf(fp, "%s\n", value);
	}

#else
	nvram_init();
	for( nvcount=0; strlen( nvram_array[nvcount].name)!=0; nvcount++) {
		for(nv=protection; nv < &protection[ sizeof(protection)/ sizeof(protection[0]) ] ; nv++) {
			if( strncmp(nvram_array[nvcount].name, nv->name, strlen(nv->name)) == 0) {
				protect = 1;
				break;
			} else
				protect =0;
		}

		if (!protect)
			fprintf(fp, "%s=%s\n", nvram_array[nvcount].name, nvram_array[nvcount].value);
	}
#endif

	fclose(fp);
	return 0;
}

int char2int(char data) {
        int value = 0;
        switch(data) {
                default:
                case '0':
                        value = 0;
                break;
                case '1':
                        value = 1;
                break;
                case '2':
                        value = 2;
                break;
                case '3':
                        value = 3;
                break;
                case '4':
                        value = 4;
                break;
                case '5':
                        value = 5;
                break;
                case '6':
                        value = 6;
                break;
                case '7':
                        value = 7;
                break;
                case '8':
                        value = 8;
                break;
                case '9':
                        value = 9;
                break;
                case 'a':
                        value = 10;
                break;
                case 'b':
                        value = 11;
                break;
                case 'c':
                        value = 12;
                break;
                case 'd':
                        value = 13;
                break;
                case 'e':
                        value = 14;
                break;
                case 'f':
                        value = 15;
                break;
        }
        return value;
}

void ascii_to_hex(char *output, unsigned char *input,int len)
{
        unsigned char data[4];
        int i;
        output[0] = '\0';
        for(i = 0;i < len;i++)
        {
            sprintf(data,"%02x",*(input+i));
            strcpy(&output[i * 2],data);
        }
}

int nvram_file_save(const char *name, const char* file) {
	FILE *fp;
	unsigned char nvram_name[100];
	unsigned char ascii_data[16];
	char hex_data[32];
	int  len = 0;
	int  i = 0;
	unsigned char data[4];
        char hex[32];
        int j = 0;
	memset(nvram_name,0,sizeof(nvram_name));
	memset(ascii_data,0,sizeof(ascii_data));
	memset(hex_data,0,sizeof(hex_data));

	fp=fopen(file,"r");
	if(fp) {
		while(1) {
			len = fread(ascii_data,1,sizeof(ascii_data),fp);
			if(len != 0) {
			  sprintf(nvram_name,"%s_%02d",name,i);
			  ascii_to_hex(hex_data, ascii_data,len);
			  DEBUG_MSG("nvram_name = %s, data = %s \n",nvram_name,hex_data);
		          printf(".");
			  nvram_set(nvram_name,hex_data);
			  i++;
			  len = 0;
			} else {
				break;
			}
			memset(ascii_data,0,sizeof(ascii_data));
		        memset(hex_data,0,sizeof(hex_data));
		}
	fclose(fp);
	} else {
		printf("%s open fail\n",name);
		return 0;
	}
	return 1;
}

int nvram_file_restore(const char *name, const char* file) {
	FILE *fp;
	unsigned char nvram_name[100];
	unsigned char ascii_data[16];
	unsigned char hex_data[32];
	int i = 0;
	int data_len = 0;
	unsigned char data[4];
        char* c;
        unsigned int data1 = 0;
        unsigned int data2 = 0;
	int j = 0;
        int k = 0;
        memset(nvram_name,0,sizeof(nvram_name));
        memset(ascii_data,0,sizeof(ascii_data));
        memset(hex_data,0,sizeof(hex_data));
	fp=fopen(file,"wb+");
	if(fp) {
		while(1) {
			sprintf(nvram_name,"%s_%02d",name,i);
			if(nvram_get(nvram_name) != NULL) {
				strcpy(hex_data,nvram_get(nvram_name));
				for(j = 0; j < 32; j++) {
					data1 = char2int(hex_data[j]);
					data2 = char2int(hex_data[j+1]);
					j++;
					data1 = data1 * 16 + data2;
					if(data1 != 0) {
						sprintf(data,"%c",data1);
						c = data;
						ascii_data[k] = *c;
					} else {
						ascii_data[k] = 0;
					}
					k++;
				}
				data_len = fwrite(ascii_data,1,sizeof(ascii_data),fp);
				DEBUG_MSG("save to file length = %d\n",data_len);
				i++;
				k = 0;
			} else {
				printf("no data and break \n");
				break;
			}
			memset(ascii_data,0,sizeof(ascii_data));
		        memset(hex_data,0,sizeof(hex_data));
		}
	printf("fclose file point \n");
	fclose(fp);
	} else {
		printf("%s write fail\n",name);
		return 0;
	}

	return 1;
}

void nvram_flag_reset(void)
{
	nvram_init_flag = 0;
}

static int nvram_read_pid(char *file)
{
	FILE *pidfile;
	char pid[20];

	pidfile = fopen(file, "r");

	if(pidfile == NULL)
	{
		DEBUG_MSG("nvram_read_pid(): Open %s Fail\n", file);
		return -1;
	}
	else
	{
		if(fgets(pid, 20, pidfile) == NULL)
		{
			DEBUG_MSG("nvram_read_pid(): Read %s Fail\n", file);
			fclose(pidfile);
			return -1;
		}
	}
	fclose(pidfile);
	return atoi(pid);
}

#ifdef SET_GET_FROM_ARTBLOCK
#define HW_LAN_WAN_OFFSET  (FW_CAL_BUF_SIZE-100)

struct hw_lan_wan_t {
	char hwversion[4];
	char lanmac[20];
	char wanmac[20];
//Albert added
	char domain[4];
};
//Albert added
static char get_tmp[5][21] = {0};

int artblock_set(char *name, char *value)
{
	int mtd_block;
	int i,result = 0, count = 0;
	char *file_buffer;
	char *offset;
	struct hw_lan_wan_t *header_start;
#ifdef CONFIG_LP
	FILE *fp;
	int no_art=1;
	char cmd[256];
#endif
	printf("artblock_set start\n");

	if( name==NULL || value==NULL){
		printf("artblock_set name/value null\n");
		return 0;
	}

	file_buffer = malloc(FW_CAL_BUF_SIZE);

	if (!file_buffer){
		printf("artblock_set file_buffer alloc fail!\n");
		return 0;
	}

#ifdef CONFIG_LP
	fp = fopen("/proc/mtd", "r");
	if (fp) {
		while (!feof(fp)) {
			memset(cmd, '\0', sizeof(cmd));
			fgets(cmd,sizeof(cmd), fp);
			if (strstr(cmd, "mtd5")) {
				no_art = 0;
				break;
			}
		}
		fclose(fp);
	}
	
	if (no_art == 0)
	{
#endif
	//read
	mtd_block = open(SYS_CAL_MTDBLK, O_RDONLY);

	if(!mtd_block){
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
		mtd_block = open(SYS_LP_MTDBLK, O_RDONLY);
		if(!mtd_block){
#endif
		printf("artblock_set read mtd_block open fail!\n");
		free(file_buffer);
		return 0;
#ifdef CONFIG_LP
		}
#endif
	}
#ifdef CONFIG_LP
	}
	else
	{
		mtd_block = open(SYS_LP_MTDBLK, O_RDONLY);
		if(!mtd_block){
			printf("artblock_set read mtd_block open fail!\n");
			free(file_buffer);
		return 0;
		}
	}
#endif

	count = read(mtd_block, file_buffer, FW_CAL_BUF_SIZE);

	if(count <= 0){
		printf("read from %s fail\n",SYS_CAL_MTDBLK);
		close(mtd_block);
		free(file_buffer);
		return 0;
	}
	else{
		if(count!=FW_CAL_BUF_SIZE){
			printf("read from %s size=%d fail!\n",SYS_CAL_MTDBLK,count);
			close(mtd_block);
			free(file_buffer);
			return 0;
		}
		close(mtd_block);
		OTHER_MSG("%02d data read from %s success\n",FW_CAL_BUF_SIZE,SYS_CAL_MTDBLK);

	}

	offset = file_buffer+HW_LAN_WAN_OFFSET;
#if 0//ARTBLOCK_DEBUG
	OTHER_MSG("header_start%x: ",offset);
	for(i=0;i<sizeof(struct hw_lan_wan_t);i++){
		OTHER_MSG("%x(%d)\n",offset[i],i);
	}
	OTHER_MSG("\n");
#endif
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

#ifdef CONFIG_LP
	if (no_art == 0)
	{
	//printf("artblock_set mtd5!\n");
#endif
	//write
	mtd_block = open(SYS_CAL_MTDBLK, O_WRONLY);

	if(!mtd_block){
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
		mtd_block = open(SYS_LP_MTDBLK, O_WRONLY);
		if(!mtd_block){
#endif
		printf("artblock_set write mtd_block open fail!\n");
		free(file_buffer);
		return 0;
#ifdef CONFIG_LP
		}
#endif
	}
#ifdef CONFIG_LP
	}
	else
	{
		//printf("artblock_set mtd4!\n");
		mtd_block = open(SYS_LP_MTDBLK, O_WRONLY);
		if(!mtd_block){
			printf("artblock_set read mtd_block open fail!\n");
			free(file_buffer);
		return 0;
		}
	}
#endif
	result = write(mtd_block, file_buffer, FW_CAL_BUF_SIZE);

	if(result <= 0) {
		DEBUG_MSG("write to %s fail!\n",SYS_CAL_MTDBLK);
		close(mtd_block);
		free(file_buffer);
		return 0;
	}
	else{
		if(result!=FW_CAL_BUF_SIZE){
			printf("read from %s size=%d fail!\n",SYS_CAL_MTDBLK,count);
			close(mtd_block);
			free(file_buffer);
			return 0;
		}
		OTHER_MSG("%02d data write to %s success\n",FW_CAL_BUF_SIZE,SYS_CAL_MTDBLK);

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
#ifdef CONFIG_LP
	FILE *fp;
	int no_art=1;
	char cmd[256];
#endif

	OTHER_MSG("artblock_get start\n");

	if( name==NULL ){
		printf("artblock_get name null\n");
		return NULL;
	}

	file_buffer = malloc(FW_CAL_BUF_SIZE);

	if (!file_buffer){
		printf("artblock_get file_buffer alloc fail!\n");
		return NULL;
	}

#ifdef CONFIG_LP
	fp = fopen("/proc/mtd", "r");
	if (fp) {
		while (!feof(fp)) {
			memset(cmd, '\0', sizeof(cmd));
			fgets(cmd,sizeof(cmd), fp);
			if (strstr(cmd, "mtd5")) {
				no_art = 0;
				break;
			}
		}
		fclose(fp);
	}
	
	if (no_art == 0)
	{
	//printf("artblock_get mtd5!\n");
#endif
	//read
	mtd_block = open(SYS_CAL_MTDBLK, O_RDONLY);

	if(!mtd_block){
/*
 * Reason: Support language pack.
 * Modified: John Huang
 * Date: 2010.02.11
 */
#ifdef CONFIG_LP
		mtd_block = open(SYS_LP_MTDBLK, O_RDONLY);
		if(!mtd_block){
#endif
		printf("artblock_get read mtd_block open fail!\n");
		goto err;
#ifdef CONFIG_LP
		}
#endif
	}
#ifdef CONFIG_LP
	}
	else
	{
		//printf("artblock_get mtd4!\n");
		mtd_block = open(SYS_LP_MTDBLK, O_RDONLY);
		if(!mtd_block){
			printf("artblock_set read mtd_block open fail!\n");
			free(file_buffer);
		return 0;
		}
	}
#endif

	count = read(mtd_block, file_buffer, FW_CAL_BUF_SIZE);

	if(count <= 0){
		DEBUG_MSG("read from %s fail\n",SYS_CAL_MTDBLK);
		close(mtd_block);
		goto err;
	}
	else{
		if(count!=FW_CAL_BUF_SIZE){
			printf("read from %s size=%d fail!\n",SYS_CAL_MTDBLK,count);
			close(mtd_block);
			goto err;
		}
		close(mtd_block);
		OTHER_MSG("%02d data read from %s success\n",FW_CAL_BUF_SIZE,SYS_CAL_MTDBLK);
	}

	offset = file_buffer+HW_LAN_WAN_OFFSET;
#if 0//ARTBLOCK_DEBUG
	OTHER_MSG("header_start%x: ",offset);
	for(i=0;i<sizeof(struct hw_lan_wan_t);i++){
		OTHER_MSG("%x(%x)\n",offset[i],i);
	}
	OTHER_MSG("\n");
#endif
	header_start = (struct hw_lan_wan_t*)offset;
	if(strcmp(name, "hw_version") == 0) {
		memcpy(get_tmp[0],header_start->hwversion,sizeof(header_start->hwversion));
		get_tmp[0][sizeof(header_start->hwversion)] = '\0';
		if(strlen(get_tmp[0])){
        	if(get_tmp[0][0]==0xff && get_tmp[0][1]==0xff){
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
		get_tmp[1][sizeof(header_start->lanmac)] = '\0';

		if(strlen(get_tmp[1])){
			if(get_tmp[1][0]==0xff && get_tmp[1][1]==0xff){
            	OTHER_MSG("lanmac=NULL\n");
            	goto err;
            }
            else
            {
                OTHER_MSG("lanmac=%s\n",get_tmp[1]);
                free(file_buffer);
                return get_tmp[1];
            }
		}
		else
        	goto err;
	}
	else if (strcmp(name, "wan_mac") == 0) {
		memcpy(get_tmp[2],header_start->wanmac,sizeof(header_start->wanmac));
		get_tmp[2][sizeof(header_start->wanmac)] = '\0';
		OTHER_MSG("art wan mac=%s\n", get_tmp[2]);
		nvram_wmac = nvram_safe_get("wan_mac");
		free(file_buffer);

		if (strcmp(nvram_wmac, "00:01:23:45:67:8a") == 0) {
			if (strlen(get_tmp[2]) && (get_tmp[2][0] != 0xff) && (get_tmp[2][1] != 0xff)) {
				return get_tmp[2];
			}
		}

		return nvram_wmac;
	}
	else if (strcmp(name, "art_wan_mac") == 0) {
		memcpy(get_tmp[4],header_start->wanmac,sizeof(header_start->wanmac));
		get_tmp[4][sizeof(header_start->wanmac)] = '\0';
		OTHER_MSG("art wan mac=%s\n", get_tmp[4]);
		nvram_wmac = nvram_safe_get("wan_mac");
		free(file_buffer);

		if (strlen(get_tmp[4]) && (get_tmp[4][0] != 0xff) && (get_tmp[4][1] != 0xff)) {
			return get_tmp[4];
		}

		return nvram_wmac;
	}
	else if(strcmp(name, "wlan0_domain") == 0) {
		memcpy(get_tmp[3],header_start->domain,sizeof(header_start->domain));
		get_tmp[3][sizeof(header_start->domain)] = '\0';
		if(strlen(get_tmp[3])){
			if(get_tmp[3][0]==0xff && get_tmp[3][1]==0xff){
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
#endif

void nvram_initfile() {
	int file_idx;
	FILE *mtd_block, *fp;
	int mtd_block_idx;
	int count = 0;

	mtd_block = fopen(SYS_CONF_MTDBLK, "r+b");

	if (!mtd_block) {
        printf("nvram_initfile: can not open mtd_block\n");
		return;
	}

	mtd_block_idx = fseek (mtd_block, NVRAM_FILEIDX_OFFSET, SEEK_SET);
	count = fwrite(&nvram_fileidx, sizeof(nvram_fileidx), 1, mtd_block);

	if (!count)
		printf("nvram initfile failed\n");

	fclose (mtd_block);
}

/*
 * nvram_writefile: write file in filesystem into flash
 */
int nvram_writefile(const char *name) {

	int file_idx;
	FILE *mtd_block, *fp;
	struct stat f_stat;
	int mtd_block_idx;
	int result = 0, count = 0;
	char *file_buffer;
	int f_match=0, f_hasspace=0;

	mtd_block = fopen(SYS_CONF_MTDBLK, "r+b");

	if (!mtd_block) {
        printf("nvram_writefile: can not open mtd_block\n");
		return 0;
	}

	mtd_block_idx = fseek (mtd_block, NVRAM_FILEIDX_OFFSET, SEEK_SET);

	count = fread (&nvram_fileidx, sizeof (nvram_fileidx), 1, mtd_block);

	DEBUG_MSG("count: %d, readbuf %d\n", count, sizeof (nvram_fileidx) );

	for (file_idx=0; file_idx<NVRAM_FILE_IDX_SIZE; file_idx++) {
		if ( strcmp (name, nvram_fileidx[file_idx].name) == 0 ) {
			f_match = 1;
			break;
		}
		if ( nvram_fileidx[file_idx].len == 0 ) {
			f_hasspace = 1;
			break;
		}
	} // for

	if (f_match || f_hasspace) {

		// read source file
		if (stat (name, &f_stat) == -1) {
			printf("read existing file failed \n");
				return -1;
		}

		file_buffer = malloc(f_stat.st_size);

		if (!file_buffer)
			return -1;

		memset(file_buffer, 0, f_stat.st_size);

		if( (fp = fopen(name, "r+b")) == 0) {
			free(file_buffer);
			printf("cannot open %s\n", name);
			return 0;
		}

		result = fread(file_buffer, f_stat.st_size, 1, fp);

		fclose (fp);

		// update file_idx
		strcpy (nvram_fileidx[file_idx].name, name);
		nvram_fileidx[file_idx].len = f_stat.st_size;

		if (file_idx != 0)
			nvram_fileidx[file_idx].offset = nvram_fileidx[file_idx-1].offset + nvram_fileidx[file_idx-1].len;
		else
			nvram_fileidx[file_idx].offset = 0;

		// update file_idx into flash
		rewind(mtd_block);
		fseek (mtd_block, NVRAM_FILEIDX_OFFSET, SEEK_SET);
		count = fwrite(&nvram_fileidx, sizeof(nvram_fileidx), 1, mtd_block);

		// write dest file in nvram
		fseek (mtd_block, nvram_fileidx[file_idx].offset, SEEK_CUR);

		count = fwrite(file_buffer, f_stat.st_size, 1, mtd_block);

		if(result <= 0)
			printf("write %s to %s fail\n", name, SYS_CONF_MTDBLK);
		else
			printf("write %s: %02d data: to %s success\n", name, f_stat.st_size, SYS_CONF_MTDBLK);

		// finished operation
		fclose(mtd_block);
		fclose(fp);
		free(file_buffer);
	} else {
		printf("no space and no match\n");
		fclose (mtd_block);
	}
}

int nvram_readfile(const char *name) {

	int file_idx;
	FILE *fp;
	FILE *mtd_block;
	int mtd_block_idx;
	int result = 0, count = 0;
	char *file_buffer;
	int f_match=0;

	mtd_block = fopen(SYS_CONF_MTDBLK, "rb");

	if (!mtd_block) {
		return 0;
	}

	mtd_block_idx = fseek (mtd_block, NVRAM_FILEIDX_OFFSET, SEEK_SET);

	count = fread (&nvram_fileidx, sizeof (nvram_fileidx), 1, mtd_block);

	DEBUG_MSG ("count: %d, readbuf %d\n", count, sizeof (nvram_fileidx) );

	for (file_idx=0; file_idx<NVRAM_FILE_IDX_SIZE; file_idx++) {

		if (strcmp(name, nvram_fileidx[file_idx].name) == 0) {
			f_match=1;

			file_buffer = malloc(nvram_fileidx[file_idx].len);
			if (!file_buffer) {
				printf("can not alloc memory\n");
				return -1;
			}

			memset(file_buffer,0,nvram_fileidx[file_idx].len);

			if( (fp =fopen(name, "wb")) == 0) {
				free(file_buffer);
				printf("cannot open %s\n", name);
				return 0;
			}

			fseek (mtd_block, nvram_fileidx[file_idx].offset, SEEK_CUR);
			count = fread(file_buffer, nvram_fileidx[file_idx].len, 1, mtd_block);
			result = fwrite(file_buffer, nvram_fileidx[file_idx].len, 1, fp);

			if (result <= 0) {
				DEBUG_MSG("write to %s fail\n", name);
			}
			else {
				DEBUG_MSG("%02d data write to %s success\n",result, name);
			}

			fflush(fp);
			fclose(fp);
			free(file_buffer);

			fclose(mtd_block);
			break;

		} // if strcmp

	} // for

	if (f_match==0)
		fclose (mtd_block);
}
char *substitute_keyword(char *string,char *source_key,char *dest_key){
        char *find = NULL;
        char head[1024] = {0};
        char result[1024] = {0};
        int flag=0;

        while(1){
          if((find = strstr(string,source_key)) != NULL){
                /* substitute source_key for dest_key */

                if(flag==0)
                snprintf(head,find-string+1,"%s%s",head,string);
                else
                        snprintf(head,find-string+1+strlen(result),"%s%s",result,string);

                if(flag==0)
                sprintf(result,"%s%s%s",result,head,dest_key);
                else
                        sprintf(result,"%s%s",head,dest_key);

                string = find + strlen(source_key);
                flag++;
                continue;
          }else{
                /* remainder part */
                sprintf(result,"%s%s",result,string);
                break;
                }
        }
        string = &result[0];
        return string;
}

static void replace_special_char(char *sourcechar)
{
    char value_tmp[1024];
    char value_tmp_amp[1024];	    
    char value_tmp_lt[1024];	
    
    memset(value_tmp, 0, 1024);
    memset(value_tmp_amp, 0, 1024);
    memset(value_tmp_lt, 0, 1024);								

	sprintf(value_tmp,"%s",sourcechar);
	sprintf(value_tmp_amp,"%s",substitute_keyword(value_tmp,"\(","\\("));		
	sprintf(value_tmp_lt,"%s",substitute_keyword(value_tmp_amp,"\)","\\)"));	
	sprintf(sourcechar,"%s",value_tmp_lt);
}

int nvram_WiPNP()
{
	char filesystem[256]={0}, temp[256]={0}, *tmp_p=NULL, tmp_mountname[50]={0}, *tmp_end_p=NULL;
	char easyconnect_security[10]={0}, easyconnect_cypher[10]={0}, easyconnect_key[65]={0}, wlan0_psk_cipher_type[10]={0};
	char wlan_security[20]={0}, *tmp_wlan=NULL, *ptr=NULL;
	FILE *fp=NULL;
	int i=0;
	
	sprintf(filesystem, "df -k | grep %s > /var/WiPNP_mount.txt",STORAGE_DEFAULT_FOLDER);
	system(filesystem);
	
	fp = fopen("/var/WiPNP_mount.txt","r");
	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/WiPNP_mount.txt");		
		return -1;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{
			tmp_p = temp;
			while((tmp_p = strstr(tmp_p, STORAGE_DEFAULT_FOLDER)) != NULL)
			{
				printf("WiPNP: usb storage - %s\n",tmp_p);
				tmp_end_p = tmp_p;
				for(i=0;i<100;i++)
				{
					//printf("WCND: %d-%x\n",i,*tmp_end_p);
					if(*tmp_end_p == 0xa) break;
					tmp_end_p ++;
				}
				memset(tmp_mountname, 0, 50);
				memcpy(tmp_mountname, tmp_p, tmp_end_p - tmp_p);
				replace_special_char(tmp_mountname);				
				tmp_p += strlen(tmp_mountname);
				memset(wlan_security, 0, 20);
				strcpy(wlan_security, nvram_get("wlan0_security"));
				if(!strcmp(wlan_security, "disable"))
				{
					strcpy(easyconnect_security, "open");
					strcpy(easyconnect_cypher, "none");
					strcpy(easyconnect_key, "");
				}
				else if(!memcmp(wlan_security, "wep", 3))
				{
					strcpy(easyconnect_cypher, "WEP");
					tmp_wlan = &wlan_security[4];
					if(!memcmp(tmp_wlan, "shared", 6))
						strcpy(easyconnect_security, "shared");
					else 
						strcpy(easyconnect_security, "open");
					if(tmp_wlan = strstr(tmp_wlan, "_"))
					{
						if(!memcmp(tmp_wlan+1, "128", 3))
							strcpy(easyconnect_key, nvram_get("wlan0_wep128_key_1"));
						else
							strcpy(easyconnect_key, nvram_get("wlan0_wep64_key_1"));
					}
					else
						strcpy(easyconnect_key, nvram_get("wlan0_wep64_key_1"));
				}
				else if(!memcmp(wlan_security, "wpa", 3))
				{
					tmp_wlan = wlan_security;
					tmp_wlan = strstr(tmp_wlan, "_");
					if(tmp_wlan = strstr(tmp_wlan, "_"))
					{
						if(!memcmp(tmp_wlan+1, "eap", 3))
						{
							if(!memcmp(wlan_security, "wpa2", 4))
								strcpy(easyconnect_security, "WPA2");
							else
								strcpy(easyconnect_security, "WPA");
							memset(filesystem, 0, 256);
							strcpy(filesystem, nvram_safe_get("wlan0_eap_radius_server_0"));
    							ptr = strtok(filesystem, "/");
							ptr = strtok(NULL, "/");
							ptr = strtok(NULL, "/");
							if (ptr)
								strcpy(easyconnect_key, ptr);
						}
						else
						{
							if(!memcmp(wlan_security, "wpa2", 4))
								strcpy(easyconnect_security, "WPA2PSK");
							else
								strcpy(easyconnect_security, "WPAPSK");
							strcpy(easyconnect_key, nvram_get("wlan0_psk_pass_phrase"));
						}
					}
					else
					{
						strcpy(easyconnect_security, "WPAPSK");
						strcpy(easyconnect_key, nvram_get("wlan0_psk_pass_phrase"));
					}
					strcpy(wlan0_psk_cipher_type,nvram_get("wlan0_psk_cipher_type"));
					if (strcmp(wlan0_psk_cipher_type, "tkip")== 0) {
					        strcpy(easyconnect_cypher, "TKIP");
				        }else if (strcmp(wlan0_psk_cipher_type, "aes")== 0) {
					        strcpy(easyconnect_cypher, "AES");
				        }else {
					        strcpy(easyconnect_cypher, "TKIP");
				        }
				}
				else
				{
					strcpy(easyconnect_security, "open");
					strcpy(easyconnect_cypher, "none");
					strcpy(easyconnect_key, "");
				}

				memset(filesystem, 0, 256);
				sprintf(filesystem, "easyconnect \"D-Link\" \"%s\" %s %s \"%s\" \"%s\"", nvram_get("wlan0_ssid"), easyconnect_security, easyconnect_cypher, easyconnect_key, tmp_mountname);
				//printf(filesystem);
				system(filesystem);
				//sleep(1);
				memset(filesystem, 0, 256);
				sprintf(filesystem, "mv -f %s/test.exe %s/WiPnP.exe",tmp_mountname,tmp_mountname);
				//printf(filesystem);
				system(filesystem);
				system("sync &");
			}
			printf("WiPNP: usb storage search end\n");
		}
		fclose(fp);
	}
}
