#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cmds.h"

#if 1
#define DEBUG_MSG(fmt, arg...)       cprintf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif

extern ipconvert_main(int, char **);
extern misc_getmac_main(int, char **);

#define SYS_BAK_MTDBLK    "/dev/mtdblock8"
#define SYS_BAK_BUF_SIZE  0x10000//64K
//ex cli misc backrw w mtdblock8isthebackblock
int backrw_main(int argc, char *argv[])
{
	int rev = -1;
	int wflag=0,count=0;
	int mtd_block;	
	char *file_buffer;
	
	printf("%s:argc=%d,argv[0,1]=%s,%s \n",__func__,argc,argv[0],argv[1]);

	if (argc < 2){
		printf("%s:arg fail\n",__func__);
		return -1;
	}	
	
	if(argv[1]){
		if(strstr(argv[1],"w")){
			wflag=1;
			printf("write cmd\n");
			if(argv[2]){
				printf("write data %s\n",argv[2]);
			}	
			else{
				printf("write cmd no data\n");
				goto out;
			}
		}
		else
			printf("%s:read cmd\n",__func__);	
	}	
	else
		printf("%s:no argv read cmd\n",__func__);	
	
	file_buffer = malloc(SYS_BAK_BUF_SIZE);
	
	if (!file_buffer){
		printf("file_buffer alloc fail!\n");
		goto out;
	}	
	
	if(!wflag){//read	
		mtd_block = open(SYS_BAK_MTDBLK, O_RDONLY);		
		if(!mtd_block){
			printf("read mtd_block opem fail!\n");
			free(file_buffer);
			goto out;
		}
			
		count = read(mtd_block, file_buffer, SYS_BAK_BUF_SIZE);		
		if(count <= 0){ 
			printf("read from %s fail\n",SYS_BAK_MTDBLK);
			close(mtd_block);	
			free(file_buffer);
			goto out;
		}	
		else{ 
			if(count!=SYS_BAK_BUF_SIZE){
				printf("read from %s size=%d fail!\n",SYS_BAK_MTDBLK,count);
				close(mtd_block);	
				free(file_buffer);
				goto out;
			}			
			printf("%02d data read from %s success\n",SYS_BAK_BUF_SIZE,SYS_BAK_MTDBLK);
			if(strlen(file_buffer))
				printf("block data %s\n",file_buffer);
			else
				printf("block data is null\n");
				
			close(mtd_block);			
			free(file_buffer);							
		}
	}else{//write	
		mtd_block = open(SYS_BAK_MTDBLK, O_WRONLY);	
		if(!mtd_block){
			printf("write mtd_block opem fail!\n");			
			goto out;
		}
		
		count = write(mtd_block, argv[2], strlen(argv[2]));	
		if(count <= 0) {
			printf("write to %s fail!\n",SYS_BAK_MTDBLK);
			close(mtd_block);			
			goto out;
		}	
		else{ 
			if(count!=strlen(argv[2])){
				printf("write to %s size=%d fail!\n",SYS_BAK_MTDBLK,count);
				close(mtd_block);	
				free(file_buffer);
				goto out;
			}	
			printf("%02d data write to %s success\n",strlen(argv[2]),SYS_BAK_MTDBLK);
			close(mtd_block);	
			free(file_buffer);			
		}		
	}
	
	rev = 0;
out:
	return rev;
}

extern int hw_vpn_main(int argc, char *argv[]);
extern int usb_test_main(int arg, char *argv[]);
extern int hotplug_status_main(int argc, char *argv[]);
int misc_subcmd(int argc, char *argv[])
{
	struct subcmd cmds[] = {
		{ "ipconvert", ipconvert_main},
		{ "getmac", misc_getmac_main},
		{ "backrw", backrw_main},
		{ "hw_vpn", hw_vpn_main, "show (storm 3516) vpn config from [proc_file]"},
		{ "usb_test", usb_test_main, "test usb status"},
		{ "hotplug_status", hotplug_status_main, "check status of hotplug devices:nic,usb"},
		{ NULL, NULL}
	};
	return lookup_subcmd_then_callout(argc, argv, cmds);
}
