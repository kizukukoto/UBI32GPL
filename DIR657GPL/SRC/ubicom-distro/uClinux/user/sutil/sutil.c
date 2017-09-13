#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
/*  Date: 2009-3-12
*   Name: Albert Chen
*   Reason: modify for different kernel version had diferent mtd include file.
*   Notice : Ex:DIR-400 isn't same as DIR-615C1.
*/
#include <mtd-user.h>

/*
#include <linux/version.h>
#if (LINUX_VERSION_CODE) < KERNEL_VERSION(2,6,15)
#include <linux/mtd/mtd.h>
#else
#include <mtd-user.h>
#endif
*/

#include "shvar.h"
#include "sutil.h"
//#include "scmd_nvram.h"
/*
 * Shared utility Debug Messgae Define
 */
#ifdef SUTIL_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif

#ifdef SUTIL_DEBUG_FLASH
#define DEBUG_FLASH(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_FLASH(fmt, arg...)
#endif

int _system(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[512];

	va_start(args, fmt);
	vsprintf(buf, fmt,args);
	va_end(args);

	DEBUG_MSG("%s\n",buf);
	i = system(buf);
	return i;

}


int read_pid(char *file)
{
	FILE *pidfile;
	char pid[20];
	pidfile = fopen(file, "r");
	if(pidfile) {
		fgets(pid,20, pidfile);
		fclose(pidfile);
	} else
		return -1;
	return atoi(pid);
}

/* Init File and clear the content */
int init_file(char *file)
{
	FILE *fp;

	if ((fp = fopen(file ,"w")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	fclose(fp);
	return 0;
}

/*
*   Date: 2009-5-20
*   Name: Ken_Chiang
*   Reason: modify for New WPS algorithm.
*   Notice :
*/
#ifdef WPS_PIN_GENERATE
void generate_pin_with_wan_mac(char *wps_pin)
{
    unsigned long int rnumber = 0;
    unsigned long int pin1 = 0,pin2 = 0;
    unsigned long int accum = 0;
    int i,digit = 0;
    char wan_mac[32]={0};
    char *mac_ptr = NULL;

    mac_ptr = cmd_nvram_safe_get("wan_mac");
    //printf("%s:mac_ptr=%s\n",__func__,mac_ptr);
    strncpy(wan_mac, mac_ptr, 18);
    mac_ptr = wan_mac;
    //remove ':'
    for(i =0 ; i< 5; i++ )
    {
        memmove(wan_mac+2+(i*2), wan_mac+3+(i*3), 2);
    }
    memset(wan_mac+12, 0, strlen(wan_mac)-12);
    //printf("%s:wan_mac=%x%x%x%x%x%x%x%x%x%x%x%x\n",__func__,
            //wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5],
            //wan_mac[6],wan_mac[7],wan_mac[8],wan_mac[9],wan_mac[10],wan_mac[11]);
    sscanf(wan_mac,"%06X%06X",&pin1,&pin2);

    pin2 ^= 0x55aa55;
    pin1 = pin2 & 0x00000F;
    pin1 = (pin1<<4) + (pin1<<8) + (pin1<<12) + (pin1<<16) + (pin1<<20) ;
    pin2 ^= pin1;
    pin2 = pin2 % 10000000; // 10^7 to get 7 number

    if( pin2 < 1000000 )
        pin2 = pin2 + (1000000*((pin2%9)+1)) ;

    //ComputeChecksum
    rnumber = pin2*10;
    accum += 3 * ((rnumber / 10000000) % 10);
    accum += 1 * ((rnumber / 1000000) % 10);
    accum += 3 * ((rnumber / 100000) % 10);
    accum += 1 * ((rnumber / 10000) % 10);
    accum += 3 * ((rnumber / 1000) % 10);
    accum += 1 * ((rnumber / 100) % 10);
    accum += 3 * ((rnumber / 10) % 10);
    digit = (accum % 10);
    pin1 = (10 - digit) % 10;

    pin2 = pin2 * 10;
    pin2 = pin2 + pin1;
    sprintf( wps_pin, "%08d", pin2 );
    //printf("%s:wps_pin=%s\n",__func__,wps_pin);
    return;

}
#endif

/* Save data into file
 * Notice: You must call init_file() before save2file()
 */
void save2file(const char *file, const char *fmt, ...)
{
	char buf[10240];
	va_list args;
	FILE *fp;

	if ((fp = fopen(file ,"a")) == NULL) {
		printf("Can't open %s\n", file);
		exit(1);
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	va_start(args, fmt);
	fprintf(fp, "%s", buf);
	va_end(args);

	fclose(fp);
}

int getStrtok(char *input, char *token, char *fmt, ...)
{
	va_list  ap;
	int arg, count = 0;
	char *c, *tmp;

	if (!input)
		return 0;
	tmp = input;
	for(count=0; tmp=strpbrk(tmp, token); tmp+=1, count++);
	count +=1;

	va_start(ap, fmt);
	for (arg = 0, c = fmt; c && *c && arg < count;) {
		if (*c++ != '%')
			continue;
		switch (*c) {
			case 'd':
				if(!arg)
					*(va_arg(ap, int *)) = atoi(strtok(input, token));
				else
					*(va_arg(ap, int *)) = atoi(strtok(NULL, token));
				break;
			case 's':
				if(!arg) {
					*(va_arg(ap, char **)) = strtok(input, token);
				} else
					*(va_arg(ap, char **)) = strtok(NULL, token);
				break;
		}
		arg++;
	}
	va_end(ap);

	return arg;
}


/*	return 0 means disconnect
 *	return 1 means connected
 */
int wan_connected_cofirm(char *if_name)
{
	int sockfd;
	struct ifreq ifr;

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return 0;

	strcpy(ifr.ifr_name, if_name);
	if( ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
		close(sockfd);
		return 0;
	}

	close(sockfd);
	return 1;
}

struct mtdDecs_t
{
	unsigned char *mtdBlockName;
	unsigned char *mtdCharName;
	unsigned int  size;
	unsigned int  start;
}mlist[2];


void flash_get_checksum()
{

	int mtd_fd = 0, count= 0, readsize = 0,totalSize = 0;
	int ret=0;
	char mtd[40]= {};
	char ImgCheckSum[10] = {};
	char *file_buffer = NULL;
	struct mtd_info_user mtd_info;
	int addr=0, end_addr=0;
	unsigned long check_sum = 0;
	FILE *fp;

	DEBUG_FLASH("Start Get CheckSum\n\n");
	//mtd 0 KERNEL
	mlist[0].mtdBlockName = SYS_KERNEL_MTD;
	mlist[0].mtdCharName  = SYS_KERNEL_MTD;
	mlist[0].size	      = IMG_LOWER_BOUND;
	mlist[0].start	      = SYS_KERNEL_START_ADDR;


	DEBUG_FLASH("mlist[0].size =%x\n",mlist[0].size);


	readsize=(64*1024);
	DEBUG_FLASH("readsize =%x\n",readsize);

	//mtd 0 KERNEL
	strcpy(mtd, mlist[0].mtdBlockName);
	if( (mtd_fd = open(mtd, O_RDWR)) < 0 )
	{
		printf("open mtd 0 fail\n");
		//close(mtd_fd); //need to close unopened file?
		return;
	}
	DEBUG_FLASH("open mtd0 finised\n");

	/* Read mtd0 from flash */
	totalSize = mlist[0].size;
	check_sum = 0;
	DEBUG_FLASH("totalSize0 =%x\n",totalSize);
	for( count=0; count < totalSize; ) {
		ret = 0;
		file_buffer = (char *)malloc(readsize);
		if(!file_buffer){
			close(mtd_fd);
			printf("malloc fail\n");
			return;
		}
		memset(file_buffer, 0, sizeof(file_buffer) );
		ret = read(mtd_fd,file_buffer,readsize);
		if(ret == -1){
			printf("read fail\n");
			close(mtd_fd);
			if(file_buffer)
				free(file_buffer);
			return;
		}
		else if( ret != readsize){
			count += ret;
			DEBUG_FLASH("read end\n");
			addr=0;
			end_addr=ret;
			DEBUG_FLASH("end_addr=%x\n",end_addr);
			DEBUG_FLASH("check_sum=%x\n",check_sum);
			while(addr < end_addr)
			{

				check_sum += *(unsigned char *) (file_buffer + addr);
				addr ++;
			}
			if(file_buffer)
				free(file_buffer);
			DEBUG_FLASH("check_sum=%x\n",check_sum);
			break;
		}
		else {
			if (count == 0) {
				unsigned long size = *(unsigned long *) (file_buffer + 12);
			    DEBUG_FLASH("\nChecksum Image Size = 0x%x\n", size);
			    if (size > totalSize)
			    	totalSize = size;
			}
			if (count == 0) {
          unsigned long size = *(unsigned long *) (file_buffer + 12);
          DEBUG_FLASH("\nChecksum Image Size = 0x%x\n", size);
          if (size > totalSize)
              totalSize = size;
      }
			count += readsize;
			DEBUG_FLASH("read\n");
			addr=0;
			end_addr=readsize;
			DEBUG_FLASH("end_addr=%x\n",end_addr);
			DEBUG_FLASH("check_sum=%x\n",check_sum);
			while(addr < end_addr)
			{
				check_sum += *(unsigned char *) (file_buffer + addr);
				addr ++;
			}
			DEBUG_FLASH("check_sum=%x\n",check_sum);
			if(file_buffer)
				free(file_buffer);
		}
		DEBUG_FLASH("count=%x\n",count);
		DEBUG_FLASH("totalSize=%x\n",totalSize);
	}
	if(mtd_fd)
		close(mtd_fd);
	DEBUG_FLASH("check_sum0=%x\n",check_sum);

	//NickChou, modify format for Chklst.txt Format v1.1
	sprintf(ImgCheckSum,"%08x",check_sum);
	DEBUG_FLASH("ImgCheckSum=%s\n",ImgCheckSum);

	/* ken modify save ImgCheckSum value from nvram to file
	   because some nvram setting will clear ImgCheckSum nvram value */
	fp = fopen("/var/tmp/checksum.txt","w");
	if (!fp)
		return;
	if(fputs(ImgCheckSum,fp)!= EOF)//kbps
		DEBUG_FLASH("write the checksum successfully!\n");
	else
		printf("write the checksum fail!\n");

	fclose(fp);
	//nvram_set("ImgCheckSum",ImgCheckSum);

	return;

}

#ifdef CONFIG_LP
void lp_get_checksum()
{
	int mtd_fd = 0;
	unsigned long count, readsize, totalSize;
	int ret=0;
	char mtd[40]= {};
	char ImgLPCheckSum[10] = {};
	char *file_buffer = NULL;
	struct mtd_info_user mtd_info;
	int addr=0, end_addr=0;
	unsigned long check_sum = 0;
	FILE *fp;

	DEBUG_FLASH("Get LP CheckSum\n\n");

	mlist[0].mtdBlockName = SYS_LP_MTD;
	mlist[0].mtdCharName  = SYS_LP_MTD;
	mlist[0].size	      = LP_BUF_SIZE;
	mlist[0].start	      = SYS_LP_START_ADDR;

	DEBUG_FLASH("mlist[0].size = 0x%x\n", mlist[0].size);

	readsize = (32*1024);
	DEBUG_FLASH("readsize = 0x%x\n", readsize);

	file_buffer = (char *)malloc(readsize);
	if (!file_buffer) {
		close(mtd_fd);
		DEBUG_FLASH("malloc fail\n");
		return;
	}

	strcpy(mtd, mlist[0].mtdBlockName);
	if( (mtd_fd = open(mtd, O_RDWR)) < 0 )
	{
		DEBUG_FLASH("open mtd 0 fail\n");
		//close(mtd_fd); //need to close unopened file?
		return;
	}

	/* Read mtd0 from flash, only read 96k */
	totalSize = 0x18000;
	DEBUG_FLASH("totalSize = 0x%x\n", totalSize);

	for (count=0; count < totalSize; ) {
		ret = 0;
		memset(file_buffer, 0, sizeof(file_buffer));
		DEBUG_FLASH("\ncount = 0x%x\n", count);

		ret = read(mtd_fd, file_buffer, readsize);
		if (ret == -1) {
			DEBUG_FLASH("read fail\n");
			close(mtd_fd);
			if (file_buffer)
				free(file_buffer);
			return;
		}
		else if (ret != readsize) {
			count += ret;
			DEBUG_FLASH("read end\n");
			addr = 0;
			end_addr = ret;
			DEBUG_FLASH("end_addr = 0x%x\n", end_addr);
			DEBUG_FLASH("check_sum = 0x%x\n", check_sum);
			while (addr < end_addr) {
				check_sum += *(unsigned char *) (file_buffer + addr);
				addr ++;
			}
			DEBUG_FLASH("LP check_sum = 0x%x\n",check_sum);
			break;
		}
		else {
			count += readsize;
			DEBUG_FLASH("read\n");
			addr = 0;
			end_addr = readsize;
			DEBUG_FLASH("end_addr = 0x%x\n", end_addr);
			DEBUG_FLASH("check_sum = 0x%x\n", check_sum);
			while (addr < end_addr) {
				check_sum += *(unsigned char *) (file_buffer + addr);
				addr++;
			}
			DEBUG_FLASH("LP check_sum = 0x%x\n",check_sum);
		}
	}

	if (mtd_fd)
		close(mtd_fd);
		
	if (file_buffer)
		free(file_buffer);

	//NickChou, modify format for Chklst.txt Format v1.1
	sprintf(ImgLPCheckSum, "%08x", check_sum);
	printf("\nImgLPCheckSum = 0x%s\n", ImgLPCheckSum);

	/* ken modify save ImgLPCheckSum value from nvram to file
	   because some nvram setting will clear ImgLPCheckSum nvram value */
	fp = fopen("/var/tmp/LP_checksum.txt","w");
	if (fp) {
		if (fputs(ImgLPCheckSum, fp) != EOF)
			DEBUG_FLASH("write the LP checksum successfully!\n");
		else
			DEBUG_FLASH("write the LP checksum fail!\n");

		fclose(fp);
	}
	return;
}
#endif

int checkServiceAlivebyName(char *service)
{
	FILE * fp;
	char buffer[1024];
	if((fp = popen("ps -ax", "r"))){
		while( fgets(buffer, sizeof(buffer), fp) != NULL ) {
			if(strstr(buffer, service)) {
				pclose(fp);
				return 1;
			}
		}
	}
	pclose(fp);
	return 0;
}

/* Date: 2009-01-21
*  Name: Jackey Chen
*  Reason: SNMP has many Makefile. In order to fit new sutil contructure. I copy codes from project.*		   c
*/

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

char * remove_backslash(char *input, int input_len)
{
    static char ssid[132];
    char ssid_tmp[70]; // ssid_tmp[66] is not enough for some ssid
    int len;
    int data1 = 0;
    int data2 = 0;
    int data3 = 0;
    int data4 = 0;
    int p = 0;
    int k = 0;


    char data[4];
    char hex[70]; // hex[66] is not enough for some ssid
    int i;

    memset(hex,0,sizeof(hex));

    for(i = 0;i < input_len;i++)
    {
        sprintf(data,"%02x",*(input+i));
        strcat(hex,data);
    }

    memset(ssid, 0, sizeof(ssid));
    memset(ssid_tmp, 0, sizeof(ssid_tmp));
    len = strlen(hex);
    strcpy(ssid_tmp, hex);
    for(k = 0; k < len; k++) {
        data1 = char2int(ssid_tmp[k]);
        data2 = char2int(ssid_tmp[k+1]);
        data1 = data1 * 16 +  data2;
        sprintf(ssid, "%s%2x", ssid, data1);
        p++;
        k++;
    }
    return ssid;
}

int _GetMemInfo(char *strKey, char *strOut)
{
    FILE *fp;
    char buf[80], mem[32], *ptr;
    int ret = 0, index = 0;

    memset(mem, '\0', sizeof(mem));

    if (strlen(strKey) == 0)
        goto out;

    if ((fp = popen("cat /proc/meminfo", "r")) == NULL)
        goto out;

    while (fgets(buf, sizeof(buf), fp)) {
        if (ptr = strstr(buf, strKey)) {
            ptr += (strlen(strKey) + 2);

            /* Skip space */
            while (*ptr == ' ')  ptr++;

            /* Get value */
            while (*ptr != ' ') {
                mem[index] = *ptr;
                ptr++;
                index++;
            }

            break;
        }
    }

    pclose(fp);
    sprintf(strOut, "%d", atol(mem)/1024);
    ret = 1;
out:
    return ret;
}
