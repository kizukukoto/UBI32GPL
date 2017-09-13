#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include    <regex.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<signal.h>
#include	"httpd_util.h"
#include	"file.h"
#include	"lp.h"
#include	"lp_version.h"
#include	"project.h"
#include	"sutil.h"

//#define LP_DEBUG  1
#ifdef LP_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

/* Check LP information*/
const char LP_MODEL_FORMAT[] = "DIR-652[A-Z]{0,2}_E[0-9]";
const char LP_VER_FORMAT[] = "[0-9].[0-9][0-9]";
const char LP_REG_FORMAT[] = "[A-Z][A-Z]";
const char LP_DATE_FORMAT[] = "[A-Za-z]{3}, [0-9]{2} [A-Za-z]{3} [0-9]{4}";


#ifdef CONFIG_LP
#define	LP_UPGRADE_SUCCESS		  4
#define	LP_UPGRADE_FAIL			  5
#endif

char *get_lp_info(const char* file, char* lp_info, int len){
	FILE *fp;

	memset (lp_info, '\0', len);

	if((fp = fopen(file,"r"))){
        fgets(lp_info, len, fp);
        fclose(fp);
    }

	return lp_info;
}

#ifdef CONFIG_LP_FW_VER_REGION
void get_firmware_version_with_LP_reg(char *fw_ver){
    int i;
    char fw_ver_num[6] = {'\0'};
    char fw_ver_reg[6] = {'\0'};
    char *ptr = NULL;

    ptr = fw_ver;
    for(i=0; i<strlen(fw_ver);i++){
        if(isalpha(fw_ver[i])){
            strncpy(fw_ver_num,fw_ver,i);
            ptr = fw_ver+i;
            strcpy(fw_ver_reg,ptr);
            break;
        }
    }
    get_lp_info(LP_REG_PATH, fw_ver_reg,6);
}
#endif

int check_LP_info_format(char *str, char *lp_modelname, char *lp_ver, char *lp_reg, char *lp_date)
{
	char *p;

	p = strstr(str,CONFIG_MODEL_NAME);
	if (!p) return LP_INVALID;

	memcpy(lp_modelname, p, 12);
	if (strcmp(p+13, "0001")) {
		memcpy(lp_ver, p+20, 4);
		memcpy(lp_reg, p+25, 2);
		memcpy(lp_date, p+28, 16);
		printf("\n*** check lp_ver: %s ***\n", lp_ver);
		return strncmp( lp_ver, LP_VER, strlen(LP_VER)-1);
	}
	return 0;
}

int check_LP_ver(void)
{
	int mtd_block=0, loc=0, rtn=LP_INVALID;
	char lp_ver[12] = {'\0'};
	char lp_build[8] = {'\0'};
	char *buf,*p,*p_h;
	char reg[3]={'\0'};
	char lp_date[20]={'\0'};
	FILE *fp,*fp1, *fp2;
	int i,len_buf=0,j=0,lp_clear=0;
	
	char cmd[256];
	int no_art=1;
	
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
	
	if (no_art == 1)
	{
		printf("Open MTD5 fail!\n");
		return LP_INVALID;
	}
	mtd_block = open(SYS_LP_MTDBLK,O_RDONLY);
	if (!mtd_block) {
		printf("Open LP BLOCK %s fail!\n",SYS_LP_MTDBLK);
		return LP_INVALID;
	}

	system("rm -rf /var/www/lang_tmp.zip");
	fp = fopen("/var/www/lang_tmp.zip", "wb+");
	if (!fp) { 
		close(mtd_block);
		return rtn;
	}

	fp1 = fopen("/var/tmp/lp_clear", "r");
	if (fp1) {
		fscanf(fp1,"%d",&lp_clear);
		fclose(fp1);
		if (lp_clear == 1) {
			close(mtd_block);
			fclose(fp);
			return LP_INVALID;
		}
	}

	//printf("****MODEL name %s\n",CONFIG_MODEL_NAME);
	buf = malloc(LP_READ_BUF);
	do {
		memset(buf, 0, LP_READ_BUF);
		loc = read(mtd_block, buf, LP_READ_BUF);

		//printf("****find loc %d - %x %x %x %x\n",loc,buf[0],buf[1],buf[2],buf[3]);
		if ((p = strstr(buf, CONFIG_MODEL_NAME))) {
			strncpy(lp_build,p+strlen(CONFIG_MODEL_NAME)+6,2);
			//printf("****find MODEL name %s - %s\n",CONFIG_MODEL_NAME,p+strlen(CONFIG_MODEL_NAME)+4);
			if ((memcmp(p+strlen(CONFIG_MODEL_NAME)+4, "0001", 4))||((!memcmp(p+strlen(CONFIG_MODEL_NAME)+4, "0001", 4))&&(!memcmp(p+strlen(CONFIG_MODEL_NAME)+10, "_", 1)))) {
				printf("****find new language package\n");
				rtn = strncmp(p+strlen(CONFIG_MODEL_NAME)+11,LP_VER,strlen(LP_VER)-1);
				strncpy(lp_ver,p+strlen(CONFIG_MODEL_NAME)+11,strlen(LP_VER));
				//Get region by Language Pack
				strncpy(reg,p+strlen(CONFIG_MODEL_NAME)+12+strlen(LP_VER),2);
				strncpy(lp_date,p+strlen(CONFIG_MODEL_NAME)+15+strlen(LP_VER),16);
			}
			else {
				rtn = 0;
				//Get region by Language Pack
				strncpy(reg,p+strlen(CONFIG_MODEL_NAME)+8,2);
				strcpy(lp_ver,"1.00");
				strcpy(lp_date,"Fri, 19 Mar 2010");
			}
printf("lp_ver=%s\n",lp_ver);
printf("lp_build_ver=%s\n",lp_build);
#ifdef IP8000
			if(strncmp(lp_ver,"1.00",sizeof("1.00"))<=0 && strncmp(lp_build,"01",sizeof("01"))<=0)//because add new string tag, temporary change ui 
			{
				printf("change ui\n");
				system("mv -f /www/setup_wizard_tmp.asp /www/setup_wizard.asp");
			}
#else
			if(strncmp(lp_ver,"1.00",sizeof("1.00"))<=0 && strncmp(lp_build,"03",sizeof("03"))<=0)//because add new string tag, temporary change ui 
			{
				printf("change ui\n");
				system("mv -f /www/setup_wizard_tmp.asp /www/setup_wizard.asp");
			}
#endif
			if (loc == 0) {
				for (i=0; i<(LP_READ_BUF-strlen(CONFIG_MODEL_NAME)-3-strlen(LP_VER)); i++)
					fprintf(fp, "%c", buf[i]);
				len_buf = -1;
			}
			break;
		}
		else {
			for (i=0; i<LP_READ_BUF; i++)
				fprintf(fp, "%c", buf[i]);
			j++;
			if (j>192) {
				close(mtd_block);
				fclose(fp);
				free(buf);
				return rtn;
			}
		}
	}
	while (loc > 0);

	if (strlen(reg)) {
		_system("echo %s > %s", reg, LP_REG_PATH);
	}

	printf("****** check LP (ver status:%d)/Reg: %s ***********\n", rtn, reg);
	if ((rtn == 0) && (len_buf == 0)) {
		p = strstr(buf, CONFIG_MODEL_NAME);
		p_h = &buf[0];
		len_buf = p - p_h;
		for (i=0; i<len_buf; i++)
			fprintf(fp, "%c", buf[i]);
	}

	fclose(fp);
	free(buf);
	close(mtd_block);
	sleep(2);

	if (rtn == 0) {
		system("unzip /var/www/lang_tmp.zip");
		system("cp -rf /www/lingual_EN.js /www/lingual_EN_tmp.js");
		sleep(1);
		system("cp -rf lang_tmp.js /www/lingual_EN.js");
		system("rm -f lang_tmp.js");
	}

	if( lp_ver != NULL) {
		if(fp2 = fopen(LP_VER_PATH,"w")){
			fputs(lp_ver, fp2);
			fclose(fp2);
		}
	}
	if( lp_build != NULL) {
		if(fp2 = fopen(LP_BUILD_PATH,"w")){
			fputs(lp_build, fp2);
			fclose(fp2);
		}
	}
	if( reg != NULL) {
		if(fp2 = fopen(LP_REG_PATH,"w")){
			fputs(reg, fp2);
			fclose(fp2);
		}
	}
	if (lp_date != NULL) {
		if(fp2 = fopen(LP_DATE_PATH,"w")){
			fputs(lp_date, fp2);
			fclose(fp2);
		}
    }
	return rtn;
}

int init_LP(char *lingual)
{
	printf("\tinit_LP\n");
	int verFlag;

	_system("rm %s", VLP_MTD);
	mkdir("/var/www", 0777);

	verFlag = check_LP_ver();
	if (verFlag == LP_INVALID) {
		symlink("/dev/null", VLP_MTD);
	}

	return verFlag;
}

#ifdef CONFIG_LP
int lp_clear(char *url, FILE *stream)
{
	printf("lp_clear\n");
	system("echo 1 > /var/tmp/lp_clear");
	system("mount -t tmpfs tmpfs /var/firm");
	_system("rm -rf %s", FW_UPGRADE_FILE);
	_system("cp -rf /www/lingual_EN_tmp.js /www/lingual_EN.js");

	_system("rm -f %s &", LP_VER_PATH);
	_system("rm -f %s &", LP_REG_PATH);
	_system("rm -f %s &", LP_DATE_PATH);

	system("/var/sbin/fwupgrade lingual &");
    printf("lp_clear finished\n");
	return LP_UPGRADE_SUCCESS;
}

int lp_upgrade(char *url, FILE *stream, int *total)
{
	int  ret = LP_UPGRADE_FAIL;
	int  count = 0,size_error = 0;
	char *read_buffer = NULL;
	char *file_buffer = NULL;
	int  image_length = 0;
	FILE *fp;
	char *end_ptr = NULL;

	printf("LP upgrade\n");

	/* check firmware size */
	if (*total < LP_BUF_SIZE) {
		file_buffer = (char*) malloc(LP_BUF_SIZE);
		if (!file_buffer) {
			return ret;
		}
		memset(file_buffer, 0, sizeof(LP_BUF_SIZE));
	}
	else {
		printf("Wrong file size are %02d\n", *total);
		size_error = 1;
	}

	read_buffer = (char *) malloc(LP_READ_BUF);
	if (!read_buffer) {
		if (file_buffer)
			free(file_buffer);			
		return ret;
	}
	memset(read_buffer, 0, sizeof(read_buffer));

	/* Read from HTTP file stream */
	printf("Start to read http stream\n");
	while (*total > 0)
	{
		if (*total < LP_READ_BUF) {
			count = safe_fread(read_buffer, 1, *total, stream);
			break;
		}
		count = safe_fread(read_buffer, 1, LP_READ_BUF, stream);

		if (*total < count) {
			count = *total;
			*total = 0;
		}
		else
			*total -= count;

		/* if size error, then just receive data and drop them */
		if (!size_error) {
			memcpy(file_buffer+image_length, read_buffer, count);
			memset(read_buffer, 0, sizeof(read_buffer));
			image_length += count;
		}
		//DEBUG_MSG("Read data is %02d, Total Reminder is %02d, image_length is %d\n", count, *total, image_length);

		/* if receive the last data then break the loop */
		if ((*total < LP_READ_BUF)&&(*total > 0)) {
			count = safe_fread(read_buffer, 1, *total,stream);
			//DEBUG_MSG("last data size %02d < MAX_BUF (%02d) and count is %02d \n", *total, LP_READ_BUF, count);

			if (*total < count) {
				count = *total;
				*total = 0;
			}
			else
				*total -= count;

			/* Copy to File Buffer */
			if (!size_error) {
				if (count > 0) {
					memcpy(file_buffer+image_length, read_buffer, count);
					image_length += count;
				}
			}

			if (!*total && (ferror(stream) || feof(stream)))
				break;
		}
	}
	printf("Upload file size is %d\n", image_length);

	/* return error after drop the receiveing data */
	if ((size_error) || (image_length > LP_BUF_SIZE)) {
		if (read_buffer)
			free(read_buffer);

		if (file_buffer)
			free(file_buffer);

		printf("lp_upgrade: return error after drop the receiveing data\n");
		return ret;
	}

	/* LP Validation */
	for (count=0; count < LP_VER_LEN; count++) {
		end_ptr = strstr(file_buffer + image_length - 50 - count, CONFIG_MODEL_NAME);
		if (end_ptr) {
			char *ptr;

			/* Check zip file size is 1024 * n (no include LP header) */
			image_length = end_ptr - file_buffer;
			printf("ZIP file size = %d\n", image_length);
			if ((image_length & 0x3FF) != 0) {
				printf("Invalid ZIP file size\n");
				end_ptr = NULL;
				break;
			}

			/* Search HTTP header */
			ptr = strstr(end_ptr, "\r\n------------");
			/* Drop HTTP header */
			*ptr = 0;
			image_length = ptr - file_buffer;
			ret = LP_UPGRADE_SUCCESS;
			printf("LP file size is %d\n", image_length);
			printf("LP information: %s\n", end_ptr);
			break;
		}
	}

	if (end_ptr == NULL) {
		printf("*** LP info format does not match ***\n");
	}

	if (ret == LP_UPGRADE_SUCCESS) {
		system("mount -t tmpfs tmpfs /var/firm");
		fp = fopen(FW_UPGRADE_FILE,"w+");
		if (fp) {
			count = safe_fwrite(file_buffer, 1, image_length, fp);
			fclose(fp);
		}
	}

	/* free buffer */
	if (read_buffer)
		free(read_buffer);

	if (file_buffer)
		free(file_buffer);

	system("rm -rf /var/tmp/lp_clear");
	sleep(1);
	printf("start to fwupgrade lingual\n");

	if (ret == LP_UPGRADE_SUCCESS) {
		system("/var/sbin/fwupgrade lingual &");
	}

	return ret;
}
#endif
