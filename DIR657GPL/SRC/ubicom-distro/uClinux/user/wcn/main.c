#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include "wcn.h"
#include <unistd.h>
#include <shvar.h>
 
#if 0
#define DEBUG_MSG(fmt, arg...)       printf(fmt, ##arg)
#else
#define DEBUG_MSG(fmt, arg...) 
#endif


char configId_wsetting[XML_LENGTH] = "";
char configId_wcn[XML_LENGTH] = "";
char cfgId_tmp[XML_LENGTH] = "";
char filename_wcn[XML_LENGTH] = "";
char filename_wcn_tmp[XML_LENGTH] = "";
char filename_raw[XML_LENGTH] = "";
char cfgId_buffer[XML_LENGTH] = "";
char name_buffer[XML_LENGTH] = "";
char mount_device[XML_LENGTH] = {0};
char *configId_s, *configId_e;

static int flag_nodo_while = 0;
static int flag_do_wcn = 0;
static int flag_usb_plugged = 0;
static int flag_usb_mounted = 0;
static int flag_wcn_do_not_remount = 0;
static int remount_retry_count = 0;
static int thresh_remount_retry_count = 10;
char lan_mac[17] ={};

int main(int argc, char* argv[])
{
	int mywcn_file_exist = 0;		
	int retry_count = 0;
	struct stat usb_status;		
	char usb_device[] = "/proc/scsi/usb-storage/";		
	int i;
	
	FILE *fp,*fp1;
	char filesystem[256]={0}, temp[256]={0}, *tmp_p=NULL, *tmp_end_p=NULL, tmp_mount_name[50]={0}, WFC_FILE_PATH[256]={0};
	int wcn_file_find = 0;
	
	write_pid_to_var();
	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='-')
		{
			fprintf(stderr, "111 Unknown option: %s\n", argv[i]);
		}
		else switch(argv[i][1])
		{
			case 'm':
				strncpy(lan_mac, argv[++i], 17);
				fprintf(stderr, "lan_mac: %s\n", lan_mac);
				break;
			default:
				fprintf(stderr, "222 Unknown option: %s\n", argv[i]);
		}
	}

#if 1	
	sprintf(filesystem, "df -k | grep %s > /var/WiPNP_mount.txt",STORAGE_DEFAULT_FOLDER);
	system(filesystem);
	
	fp = fopen("/var/WiPNP_mount.txt","r");
	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/WiPNP_mount.txt");		
		return;
	}else{
		while((fgets(temp,sizeof(temp),fp)) && !wcn_file_find)
		{
			tmp_p = temp;
			while((tmp_p = strstr(tmp_p, STORAGE_DEFAULT_FOLDER)) != NULL)
			{
				memset(tmp_mount_name, 0, 50);
				tmp_end_p = tmp_p;
				for(i=0;i<100;i++)
				{
					//printf("WCND: %d-%x\n",i,*tmp_end_p);
					if(*tmp_end_p == 0xa) break;
					tmp_end_p ++;
				}
				memcpy(tmp_mount_name, tmp_p, tmp_end_p - tmp_p);
				tmp_p += strlen(tmp_mount_name);
				printf("WCND: usb storage - %s(%d)\n",tmp_mount_name,strlen(tmp_mount_name));
#ifdef MIII_BAR
                                int miiicasa_find=0;
				memset(filesystem, 0, 256);
				sprintf(filesystem, "%s/config_miiicasa.ini", tmp_mount_name);
				if((fp1 = fopen(filesystem,"r")) != NULL)
				{
					printf("find %s\n",filesystem);
					while(fgets(cfgId_buffer, XML_LENGTH, fp1))
					{
			                        configId_s = (strstr(cfgId_buffer, "<miiicasa_server>")); 
			                        if(configId_s)
			                        {
			                                miiicasa_find = 1;
				                        configId_s = strstr(configId_s, ">") + 1;
				                        configId_e = strstr(configId_s, "<");
				                        memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
				                        strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
				                        printf("get miiicasa server : %s\n", cfgId_tmp);
				                        nvram_set("miiicasa_server", cfgId_tmp);			
			                        }
			                        configId_s = (strstr(cfgId_buffer, "<miiicasa_server_js>")); 
			                        if(configId_s)
			                        {
			                                miiicasa_find = 1;
				                        configId_s = strstr(configId_s, ">") + 1;
				                        configId_e = strstr(configId_s, "<");
				                        memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
				                        strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
				                        printf("get miiicasa server js : %s\n", cfgId_tmp);
				                        nvram_set("miiicasa_server_js", cfgId_tmp);			
			                        }
			                        configId_s = (strstr(cfgId_buffer, "<miiicasa_UST_server>")); 
			                        if(configId_s)
			                        {
			                                miiicasa_find = 1;
				                        configId_s = strstr(configId_s, ">") + 1;
				                        configId_e = strstr(configId_s, "<");
				                        memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
				                        strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
				                        printf("get miiicasa UST server : %s\n", cfgId_tmp);
				                        nvram_set("miiicasa_UST_server", cfgId_tmp);			
			                        }
			                        configId_s = (strstr(cfgId_buffer, "<miiicasa_did>")); 
			                        if(configId_s)
			                        {
			                                miiicasa_find = 1;
				                        configId_s = strstr(configId_s, ">") + 1;
				                        configId_e = strstr(configId_s, "<");
				                        memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
				                        strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
				                        printf("get miiicasa did : %s\n", cfgId_tmp);
				                        nvram_set("miiicasa_did", cfgId_tmp);			
			                        }
			                        configId_s = (strstr(cfgId_buffer, "<miiicasa_prefix>")); 
			                        if(configId_s)
			                        {
			                                miiicasa_find = 1;
				                        configId_s = strstr(configId_s, ">") + 1;
				                        configId_e = strstr(configId_s, "<");
				                        memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
				                        strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
				                        printf("get miiicasa prefix : %s\n", cfgId_tmp);
				                        nvram_set("miiicasa_prefix", cfgId_tmp);			
			                        }
			                        configId_s = (strstr(cfgId_buffer, "<miiicasa_forward_server>")); 
			                        if(configId_s)
			                        {
			                                miiicasa_find = 1;
				                        configId_s = strstr(configId_s, ">") + 1;
				                        configId_e = strstr(configId_s, "<");
				                        memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
				                        strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
				                        printf("get miiicasa forward server : %s\n", cfgId_tmp);
				                        nvram_set("miiicasa_forward_server", cfgId_tmp);			
			                        }
			                }
			                if(miiicasa_find != 0) nvram_commit();
			                fclose(fp1);
				}
#endif
				memset(filesystem, 0, 256);
				sprintf(filesystem, "%s/smrtntky/wsetting.wfc", tmp_mount_name);
				if((fp1 = fopen(filesystem,"r")) != NULL)
				{
					printf("find %s\n",filesystem);
					while(fgets(cfgId_buffer, XML_LENGTH, fp1))
					{
						configId_s = (strstr(cfgId_buffer, "configAuthor>")); 
						if(configId_s)
						{
							configId_s = strstr(configId_s, ">") + 1;
							configId_e = strstr(configId_s, "<");
							memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
							strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
							printf("get_configId : %s\n", cfgId_tmp);
							break;			
						}
					}
					fclose(fp1);
					if(configId_s)
					{
						if(memcmp(cfgId_tmp, "D-Link", 6) == 0) return wcn_file_find;
					}
					//get configId from ./smrtntky/wsetting.wfc to configId_wsetting
					memset(cfgId_tmp, 0, sizeof(cfgId_tmp));
					get_configId(filesystem);
					strcpy(configId_wsetting, cfgId_tmp);
					printf("WCN: %s configId is %s\n", filesystem, configId_wsetting);									
					memset(WFC_FILE_PATH, 0, 256);
					sprintf(WFC_FILE_PATH, "%s/smrtntky/device/", tmp_mount_name);
					if((fp1 = fopen(filesystem, "r+")) == NULL) {
						printf("check_mywcn_file_exist: directory does not exist\n");
						memset(filesystem, 0, 256);
						sprintf(filesystem, "mkdir -p %s",WFC_FILE_PATH); 	
						system(filesystem);
					}
					else
					{
						printf("check_mywcn_file_exist: directory does exist\n");
						fclose(fp1);
						memset(filesystem, 0, 256);
						sprintf(filesystem, "%s/smrtntky/device/%s.wfc", tmp_mount_name,configId_wsetting);
						printf("check_mywcn_file_exist: %s\n",filesystem);
						if((fp1 = fopen(filesystem, "r+")) != NULL) {
							printf("check_mywcn_file_exist: file does exist\n");
							fclose(fp1);
							mywcn_file_exist = 1;
						}
					}
					
					wcn_file_find = 1;
					do_wcn(mywcn_file_exist, tmp_mount_name);
					break;
				}
				else
				{
					printf("not find %s\n",filesystem);
				}
			}
		}
		fclose(fp);
	}
	
	return wcn_file_find;
		
#else
	while(!flag_nodo_while) {
		retry_count++;
		if(stat(usb_device,&usb_status) != NULL)			
		{
			flag_wcn_do_not_remount = 0;
			if (flag_usb_plugged == 0) {
				// no usb device exists
				//continue;
			}
			// an existing usb device was unplugged
			flag_usb_plugged = 0;

			DEBUG_MSG("WCN: %s don't Plug in\n", usb_device);
			//USB_LED off
			//_system("/sbin/gpio USB_LED on &");//off

			unmount_usb_path();
#if 1 //configId equal do wcn
			memset(mount_device, 0, XML_LENGTH);
			sprintf(mount_device, "rm -f %s", WCN_SET_FILE); 	
			system(mount_device);
			//_system("rm -f %s",WCN_SET_FILE);
#endif
		}
		else
		{
			if ((flag_usb_plugged == 1) && (flag_wcn_do_not_remount == 1)) {
				// a usb device has already been plugged in and a mount was attempted previously
				// this function will check if the usb device has previously been mounted
				// and mount it if it hasnt.
				mount_usb_path();
				continue;
			}
			// a new usb device was plugged in
			flag_usb_plugged = 1;
			flag_wcn_do_not_remount = 0;

			if(!check_set_wcn()) //NickChou and Ken
			{
				DEBUG_MSG("WCN: %s Plug in\n", usb_device);
				//USB_LED on
				//_system("/sbin/gpio USB_LED off &");//on

				mount_usb_path();
				//get configId from /var/usb/smrtntky/wsetting.wfc to configId_wsetting
				get_configId(WFC_SETTING_FILE);
				strcpy(configId_wsetting, cfgId_tmp);
				DEBUG_MSG("WCN: %s configId is %s\n", WFC_SETTING_FILE, configId_wsetting);									
				if(check_mywcn_file_exist()){
					DEBUG_MSG("WCN: mywcn_file does not exist\n");
					mywcn_file_exist = 0;
					if(strcmp(configId_wsetting, "") == 0 || strcmp(configId_wsetting, "none") == 0)	
					{
						DEBUG_MSG("WCN: don't find wsetting.wfc configId\n");
						unmount_usb_path();
						remount_retry_count++;
						if (remount_retry_count > thresh_remount_retry_count) {
							flag_wcn_do_not_remount = 1;
							remount_retry_count = 0;	
						}
					}
					else{
						DEBUG_MSG("WCN: find wsetting.wfc configId===>do wcn\n");
						flag_nodo_while = 1;
						flag_do_wcn = 1;
						break;
					}
				}//check_mywcn_file_exist
				else{
					DEBUG_MSG("mywcn_file exist\n");
					mywcn_file_exist =1;
					if(strcmp(configId_wsetting, "") == 0 || strcmp(configId_wsetting, "none") == 0)	
					{
						DEBUG_MSG("WCN: don't find wsetting.wfc configId \n");
						unmount_usb_path();
						remount_retry_count++;
						if (remount_retry_count > thresh_remount_retry_count) {
							flag_wcn_do_not_remount = 1;
							remount_retry_count = 0;
						}
					}
					else {
						//get configId from /var/usb/smrtntky/device/mywcn_file to configId_wcn
						get_mywcn_filename(cfgId_tmp);
						strcpy(filename_raw, cfgId_tmp);
						DEBUG_MSG("WCN: mywcn_file raw is %s\n", filename_raw);							
						strncpy(filename_wcn_tmp, filename_raw, 8);
						sprintf(filename_wcn, "%s%s.wfc",WFC_FILE_PATH , filename_wcn_tmp);							
						DEBUG_MSG("WCN:  mywcn_file is %s\n", filename_wcn);	
						get_configId(filename_wcn);
						strcpy(configId_wcn, cfgId_tmp);
						DEBUG_MSG("WCN: get configId is %s from %s\n", configId_wcn, filename_wcn);
						//check configId
						if(strcmp(configId_wsetting, configId_wcn) == 0){
							DEBUG_MSG("WCN: configId equal\n");
#if 1//configId equal do wcn							
							if(!check_set_wcn()){
								DEBUG_MSG("WCN: NO set wcn\n"); 
								flag_nodo_while = 1;
								flag_do_wcn = 1;
							}
							else {
								DEBUG_MSG("WCN: set wcn\n");
								unmount_usb_path();
								remount_retry_count++;
								if (remount_retry_count > thresh_remount_retry_count) {
									flag_wcn_do_not_remount = 1;
									remount_retry_count = 0;
								}
							}
#else
							unmount_usb_path();
#endif
						}
						else
						{
							DEBUG_MSG("WCN: configId not equal====> do wcn\n");
							flag_nodo_while = 1;
							flag_do_wcn = 1;
							break;
						}
					}
				} //check_mywcn_file_exist
			} //check_set_wcn()
		}
		sleep(1);
	}

	DEBUG_MSG("exit while\n");	
	
	if(flag_do_wcn)
	{		
		do_wcn(mywcn_file_exist);
	}
	else		
	{			
		printf("fail go here!!\n");
		_system("reboot -d2 &");
	}	
		
	return 1;	
#endif	
}	

void write_pid_to_var(void)
{
	int pid;
	FILE *fp;
	char temp[8];
	pid = getpid();
	sprintf(temp,"%d\n",pid);

	if((fp = fopen(WCND_PID,"w")) != NULL)
	{
		fputs(temp,fp);
	}
	else{
		DEBUG_MSG("write_pid : open fial failed\n");
		return;
	}
	fclose(fp);
}

#if 1//configId equal do wcn
int check_set_wcn(void)
{
	FILE *fp;
	
	if((fp = fopen(WCN_SET_FILE, "r+")) != NULL)
	{
		DEBUG_MSG("check_set_wcn : open\n");
		fclose(fp);	
		return 1;
	}
	else
	{
		DEBUG_MSG("check_set_wcn : fail to open\n");
		return 0;
	}		
}
#endif

void unmount_usb_path(void)
{
	if (flag_usb_mounted == 0) {
		return;
	}
	memset(mount_device, 0, XML_LENGTH);
	flag_usb_mounted = 0;
}

void mount_usb_path(void)
{
	if (flag_usb_mounted == 1) {
		return;
	}
	//mount -t type device dir
	memset(mount_device, 0, XML_LENGTH);

	flag_usb_mounted = 1;
}

#if 0
int check_mywcn_file_exist(void)
{
	FILE *fp;
	FILE *fr;
	get_mywcn_filename(cfgId_tmp);
	if((fr = fopen(WFC_FILE_PATH, "r+")) == NULL) {
		DEBUG_MSG("check_mywcn_file_exist: directory does not exist\n");
		memset(mount_device, 0, XML_LENGTH);
		sprintf(mount_device, "mkdir -p %s",WFC_FILE_PATH); 	
		system(mount_device);
		return 1;
	}
	memset(mount_device, 0, XML_LENGTH);
	sprintf(mount_device, "ls %s >> %s",WFC_FILE_PATH,MYWFC_TEMP_FILE); 	
	system(mount_device);	
	//_system("ls %s >> %s",WFC_FILE_PATH,MYWFC_TEMP_FILE);
	if((fp = fopen(MYWFC_TEMP_FILE, "r+")) != NULL)
	{
		while(fgets(name_buffer,XML_LENGTH,fp))
		{
			DEBUG_MSG("check_mywcn_file_exist : buffer is %s\n", name_buffer);
			fclose(fp);
			memset(mount_device, 0, XML_LENGTH);
			sprintf(mount_device, "rm -f %s",MYWFC_TEMP_FILE);
			system(mount_device);
			//_system("rm -f %s",MYWFC_TEMP_FILE);

			if(strstr(name_buffer,cfgId_tmp)){
				DEBUG_MSG("check_mywcn_file_exist : find my file name is %s\n", cfgId_tmp);
				return 0;
			}
			else {
				DEBUG_MSG("check_mywcn_file_exist : did not find my file name is %s\n", cfgId_tmp);
				return 1;
			}
		}
		fclose(fp);
	}
	else
	{
		DEBUG_MSG("check_mywcn_file_exist : fail to open %s\n", MYWFC_TEMP_FILE);
		memset(mount_device, 0, XML_LENGTH);
		sprintf(mount_device, "rm -f %s",MYWFC_TEMP_FILE); 	
		system(mount_device);	
		//_system("rm -f %s",MYWFC_TEMP_FILE);
		return 1;
	}		
}
#endif

void get_mywcn_filename(char* name_tmp)
{
	int i, j = 0;
	char wfc_name[9];
	memset(name_tmp, 0, XML_LENGTH);	
	
	for(i = 6; i < 17; i++){
		if(*(lan_mac + i) != ':'){
			wfc_name[j] = *(lan_mac + i);
			j++;
		}
	}
	wfc_name[8] = '\0';
	strcpy(name_tmp, wfc_name);
	DEBUG_MSG("get_mywcn_filename : name is %s\n", name_tmp);	
}

void get_filename(char *file)
{
	FILE *fp;
	memset(cfgId_tmp, 0, XML_LENGTH);
	
	if((fp = fopen(file, "r+")) != NULL)
	{
		while(fgets(name_buffer,XML_LENGTH,fp))
		{			
			strcpy(cfgId_tmp, name_buffer);
			DEBUG_MSG("get_filename : name is %s\n", cfgId_tmp);
		}
		fclose(fp);
	}
	else
	{
		DEBUG_MSG("get_filename: fail to open %s\n", file);
		return;
	}	
}

void get_configId(char *file)
{
	FILE *fp;
	memset(cfgId_tmp, 0, XML_LENGTH);
	
	if((fp = fopen(file, "r+")) != NULL)
	{
		while(fgets(cfgId_buffer, XML_LENGTH, fp))
		{
			configId_s = (strstr(cfgId_buffer, "configId")); 
			if(configId_s)
			{
				configId_s = strstr(cfgId_buffer, ">") + 1;
				configId_e = strstr(configId_s, "<");
				strncpy(cfgId_tmp, configId_s, configId_e - configId_s);
				DEBUG_MSG("get_configId : %s\n", cfgId_tmp);			
			}
		}
		fclose(fp);	
	}
	else
	{
		DEBUG_MSG("get_configId : fail to open %s\n", file);
		strncpy(cfgId_tmp, "none", 4);
	}
}
