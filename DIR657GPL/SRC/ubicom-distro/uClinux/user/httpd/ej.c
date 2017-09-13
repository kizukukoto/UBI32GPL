/*
 * Tiny Embedded JavaScript parser
 *
 *
 * $Id: ej.c,v 1.6 2009/04/29 07:51:52 yufa Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <linux/sockios.h>
#include <sutil.h>
#include <shvar.h>
#include <httpd.h>

#include "cmoapi.h"
#include <time.h>
extern const char VER_FIRMWARE[];
extern const char VER_BUILD[];
#ifdef MIII_BAR
#define var_tmp "\/mnt\/shared";
#define usb_mountpoint "/USB_USB2FlashStorage_45c0216_1";
#endif

#ifdef HTTPD_DEBUG
#define DEBUG_MSG(fmt, arg...)       printf("%s [%d]: "fmt, __FILE__, __LINE__, ##arg)
#else
#define DEBUG_MSG(fmt, arg...)
#endif


static char * get_arg(char *args, char **next);
static void call(char *func, FILE *stream);

static char *
unqstrstr(char *haystack, char *needle)
{
	char *cur;
	int q;

	for (cur = haystack, q = 0;
	     cur < &haystack[strlen(haystack)] && !(!q && !strncmp(needle, cur, strlen(needle)));
	     cur++) {
		if (*cur == '"')
			q ? q-- : q++;
	}
	return (cur < &haystack[strlen(haystack)]) ? cur : NULL;
}

static char *
get_arg(char *args, char **next)
{
	char *arg, *end;

	/* Parse out arg, ... */
	if (!(end = unqstrstr(args, ","))) {
		end = args + strlen(args);
		*next = NULL;
	} else
		*next = end + 1;

	/* Skip whitespace and quotation marks on either end of arg */
	for (arg = args; isspace((int)*arg) || *arg == '"'; arg++);
	for (*end-- = '\0'; isspace((int)*end) || *end == '"'; end--)
		*end = '\0';
	
	return arg;
}
static void
call(char *func, FILE *stream)
{
	char *args, *end, *next;
	int argc,i;
	char * argv[16];
	struct ej_handler *handler;

	/* Parse out ( args ) */
	if (!(args = strchr(func, '(')))
		return;
	if (!(end = unqstrstr(func, ")")))
		return;
	*args++ = *end = '\0';

	/* Set up argv list */
	for (argc = 0; argc < 16 && args && *args; argc++, args = next) {
		if (!(argv[argc] = get_arg(args, &next)))
			break;
	}
	
	//ej_cmo_get_cfg(0, stream, argc, argv);
	//ej_cmo_get_status(0, stream, argc, argv);   
	/* Call handler */
	handler = &ej_handlers[0];
	for(i = 0;i < 5 ;i++)
	{
		if(handler->pattern){
			if (strncmp(handler->pattern, func,strlen(handler->pattern)) == 0)
			{
				handler->output(0, stream, argc, argv);
				break;
			}
			if(i != 4)
			handler = handler + 1;
		}
	}
//	for (handler = &ej_handlers[0]; handler->pattern; handler++) {
//		printf("enter for loop");
//		if (strncmp(handler->pattern, func, strlen(handler->pattern)) == 0){
//			handler->output(0, stream, argc, argv);
//		}
//	}
}

 char *str_replace (char *source, char *find,  char *rep){  
     int find_L=strlen(find);  
    int rep_L=strlen(rep);  
    int length=strlen(source)+1;  
    int gap=0;  
     
    char *result = (char*)malloc(sizeof(char) * length);  
    strcpy(result, source);      
     
    char *former=source;  
    char *location= strstr(former, find);  
     
    while(location!=NULL){  
       gap+=(location - former);  
       result[gap]='\0';  
         
       length+=(rep_L-find_L);  
       result = (char*)realloc(result, length * sizeof(char));  
       strcat(result, rep);  
       gap+=rep_L;  
         
       former=location+find_L;  
       strcat(result, former);  
         
       location= strstr(former, find);  
    }      
  
    return result;  
  
} 


#ifdef MIII_BAR
const char *str_tok = "tok=";
const char *str_expires = "expires=";
const char *str_routerAccessKeyId = "routerAccessKeyId=";
const char *str_callback = "callback=";
const char *str_tx = "tx=";
const char *str_t = "t=";
extern char miii_usb_folder[];
extern int miiicasa_set_folder();
char miii_response[1024];

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (!((pstr[1] == '5') && (pstr[2] == 'C'))) {
    	*pbuf = 0x5C;
    	pbuf++;
    	}
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') { 
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}



void do_miii_getDeviceInfo(char *path, FILE *stream)
{
	
	char *pattern = NULL;
	char *p_pattern = NULL;
	char tok[256]={0};
	char callback[256]={0};
	char *p = NULL;
	char lan_mac[6]={0};
	char wan_mac[6]={0};
	char model_name[32]={0};
	char lan_ip[20]={0};
	//char wan_ip[20];
	char *wan_if;
	char *wan_ip;
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	//char str_vid[20];
	//char str_pid[20];
	char str_did[40]={0};
	char str_brand[20]={0};
	char str_wan_mac[32]={0};
	char str_lan_mac[32]={0};
	char str_user_mac[32]={0};
	char miiicasa_enabled[10]={0};
	int wifi_secure_enabled = 0;
    
        DEBUG_MSG("do_miii_getDeviceInfo(%d)\n",strlen(path));
        
    	strcpy(miiicasa_enabled, nvram_safe_get("miiicasa_enabled"));
    	
	get_macaddr( nvram_safe_get("lan_bridge"), lan_mac);
	get_macaddr( nvram_safe_get("wan_eth"), wan_mac);
    
	sprintf(str_wan_mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
		wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5]);
	sprintf(str_lan_mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
		lan_mac[0],lan_mac[1],lan_mac[2],lan_mac[3],lan_mac[4],lan_mac[5]);

  	strcpy(str_user_mac, &con_info.mac_addr[0]);
                
	strcpy(model_name, nvram_safe_get("model_number"));
 	strcpy(lan_ip, nvram_safe_get("lan_ipaddr"));
	//printf("########### str_user_mac = %s ###########\n",str_user_mac);
	wan_if= nvram_safe_get("wan_eth");
	wan_ip = get_ip_addr(wan_if);
	//printf("########### wan_ip= %s ###########\n",wan_ip);
	strcpy(str_status, "ok");
	//strcpy(str_vid, nvram_safe_get("manufacturer"));
	//strcpy(str_pid, "MUCHIII_PROTOTYPE");
	strcpy(str_did, nvram_safe_get("miiicasa_did"));
	strcpy(str_brand, nvram_safe_get("manufacturer"));
	strcpy(str_errno, "");

	memset(tok, 0, 256);
	memset(callback, 0, 256);

	pattern = malloc(strlen(path)+1);
	if (pattern == NULL)
	{
	        DEBUG_MSG("do_miii_getDeviceInfo(%d): can not alloc\n",strlen(path));
		return;
        }
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern, "&");
	// callback
	p = strtok(NULL, "&");
	strcpy(callback, p + 9);

/*
MIIICASA.hpDevice.afterGetDeviceInfo({
"status":"ok","errno":"","errmsg":"",
"device":{"did":"47bce5c74f589f4867dbd57e9ca9f808",
"brand":"D-Link","miiicasa_enabled":"1","wifi_secure_enabled":"0",
"mac":"00:26:5A:AD:BD:95","model":"DIR-685","ws_ip":"192.168.0.1",
"ws_port":"80","external_ip":"172.21.33.80","external_port":"5555"},
"user_mac":"00:1D:92:52:07:CF"});
*/

	memset(miii_response, 0, sizeof(miii_response));
		
	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\","
		"\"device\":{\"did\":\"%s\","
		"\"brand\":\"%s\",\"miiicasa_enabled\":\"%c\",\"wifi_secure_enabled\":\"%d\","
		"\"mac\":\"%s\",\"model\":\"%s\",\"ws_ip\":\"%s\","
		"\"ws_port\":\"%d\",\"external_ip\":\"%s\","
		"\"external_port\":\"%d\"},\"user_mac\":\"%s\"});", 
		callback, str_status, str_errno, str_errmsg,
		str_did,
		str_brand, miiicasa_enabled[0], wifi_secure_enabled,
		str_wan_mac, model_name, lan_ip,
		80, wan_ip, 5555,
		 str_user_mac);

	wfputs(miii_response, stream);
	//system("ls -le /var/usb/sda/sda1/Photos > /var/miii_usbls.txt");
	//combuf = (char *)malloc(strlen(mount)+100);
			
	//sprintf(combuf,"ls -le %s/Photos > /var/miii_usbls.txt",mount);
	//printf("combuf = %s\n",combuf);
	//system(combuf);
	//free(combuf);
	free (pattern);
	
	return;
}

void do_miii_listStorages(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char tok[256];
	char callback[256];
	char *p = NULL;
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	int count = 1;
	int type =2;
	char model[20] = "FlashStorage";
	char name[20] = "HDDREG";
	char mountpoint[128] = usb_mountpoint;
	
	//char mountpoint[128]={0};
	unsigned long long used_space ;
	unsigned long long storage ;
 	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0};
	
	FILE *fp=NULL;
	FILE *fp2=NULL;
	char listtemp1[1024]={0};
	char usb0[128]={0};
	char usb1[128]={0};
	char usb2[128]={0};
	char usb3[128]={0};
	char usb4[128]={0};
	char usb5[128]={0};
	char usb6[128]={0};
	char usb7[128]={0};
	char usb8[128]={0};
	char usb9[128]={0};
	char usb10[128]={0};
	char usb11[128]={0};
	
	if(miiicasa_set_folder() == -1) return;
	
	system("dmesg | grep \"Direct-Access\" >> /var/tmp/456.txt");
	fp2 = fopen("/var/tmp/456.txt","r");
	while(fgets(listtemp1,sizeof(listtemp1),fp2))
	{
		sscanf(listtemp1, "%s %s %s %s %s %s %s %s %s %s %s %s", usb0,usb1,usb2,usb3,usb4,usb5,usb6,usb7,usb8,usb9,usb10,usb11);
  		//printf("XXXX=%s %s %s %s %s %s %s %s %s %s %s %s\n", usb0,usb1,usb2,usb3,usb4,usb5,usb6,usb7,usb8,usb9,usb10,usb11);
		//printf("usb5=%s\n",usb5);
		if(usb5)
		{
			memset(model, 0, sizeof(model));
			strcpy(model,usb5);
		}
	}
	fclose(fp2);

	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		strcpy(mountpoint,"");
		return;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			// printf("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));
			break;		
		}//while
		fclose(fp);				
	}		
    
	strcpy(str_status, "ok");
	storage = (unsigned long long)atol(totalsize);
	storage = storage * 1024;
	used_space = (unsigned long long)atol(usedsize);
	used_space = used_space * 1024;
	strcpy(str_errno, "");
	p = strtok(filesystem, "/");
	p = strtok(NULL, "/");
	if(p)
	{
		memset(name, 0, sizeof(name));
		strcpy(name, p);
	}
 
	memset(tok, 0, 256);
	memset(callback, 0, 256);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
 	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,str_callback))
		strcpy(callback, p + 9);
	}

	memset(miii_response, 0, sizeof(miii_response));
	
	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\","
		"\"count\":%d,\"storages\":[{\"model\":\"%s\","
		"\"type\":\"%d\",\"mountpoints\":[{\"mountpoint\":\"%s\",\"name\":\"%s\","
		"\"storage\":%llu,\"used_space\":%llu}]}]});",                                         
		callback, str_status, str_errno, str_errmsg,
		count,model,type, mountpoint, name,
		storage, used_space);
			
	wfputs(miii_response, stream);

	free (pattern);
	return;
  }

int GMTtoEPOCH(char *year, char *month, char *day, char *time) {
	FILE *fp;
	char mon[12][5] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	char buff[256], buf[256];
	int i, d_mon = 0, mtime = 0;
	for(i = 1; i <= 12 ; i++){
			if(strcmp(mon[i-1],month) == 0){
				d_mon = i;
			}
	}
	// date +%s "YYYY-MM-DD hh:mm:ss"
	sprintf(buff,"date +%%s \"%s-%d-%s %s\"",year,d_mon,day,time);
	if((fp = popen(buff, "r")) != NULL)
	{
			memset(buf,'\0',sizeof(buf));
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				mtime = atoi(buf);
			}
	}
	pclose(fp);
	
	DEBUG_MSG("Miii_mtime = %d \n",mtime);
	return mtime;
}

void do_miii_getFileList(char *path, FILE *stream)
{
	int list_count=0,rl=0;
	char *pattern = NULL;
	char *temppath = NULL;
	char *miii_response_t = NULL;
	char usblsbuff[4096]={0};
	char usblsbuff1[2048]={0};
	char *p_pattern = NULL;
	char tok[256];
	char dirname[256];
	char callback[256];
	char *p = NULL;
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char url_addr[256]={0};
	char files[20]={0};
	char filepath[128]={0};
	char *tempfilepath = NULL ;
	char label_name[20] = "\"UsbStorage\"";
	int cover = 0 ;
	char files_name[128]={0} ;
	char list_files_name[40]={0} ;
	int file_type =0;
	int file_mtime = 0;
	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0};
	char listtemp[1024]={0}, name[128]={0};
	char noused[20]={0},arrt[20]={0},size[12]={0},week[32],month[32]={0},day[32]={0},year[32]={0},time[32]={0},usbname[16]={0},nametmp[32]={0};
	char ext_value[128] = {0};
	//char *combuf=NULL;
	FILE *fp=NULL,*fp1=NULL;

	if(miiicasa_set_folder() == -1) return;
	
	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		strcpy(filepath,"");
		return;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			printf("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}	
//labe_name	
	char *sp = NULL;
	char *p3 = NULL;
	char *p4 = NULL;
	char *p5 = NULL;
	char labename[50], buf[50]={0};
	FILE *fp3 = NULL;
	memset(labename, '\0', sizeof(labename));
	sprintf(buf,"blkid -s LABEL %s",filesystem);
	if ((fp3 = popen(buf, "r")) == NULL)
		return;
	fgets(labename, sizeof(labename), fp3);
	if(labename) 
	{
		DEBUG_MSG("####labename = %s",labename);
		char devname[128];
		char devlabel[128];
		//sscanf(labename,"%s %s",devname,devlabel);
		//printf("devname = %s, devlabel = %s\n",devname,devlabel);
	
		p3 = strstr(labename,"LABEL=");
		if(p3 != NULL) 
		{
			p4 = strstr(p3,"\"");
			//p4 = strtok(p3,"LABEL=");
			//printf("p4=%s\n",p4);
			//p4 = strstr(p3,"\"");

			// while(p4 = strtok(NULL,"\""))
			//p5 = strtok(p4,"\"");
			//  printf("p5=%s\n",p5);
			// p = strtok(filesystem, "/");
			//p = strtok(NULL, "/");
			if(p4 != NULL)
			{
				memset(label_name, 0, sizeof(label_name));
				strcpy(label_name, p4);
			}
		}
	}
    
	pclose(fp3);
	strcpy(str_status, "ok");
	strcpy(str_errno, "");
	strcpy(list_files_name,"");
	strcpy(files, "");
		
	memset(tok, 0, 256);
	memset(dirname, 0, 256);
	memset(callback, 0, 256);
	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	DEBUG_MSG("pattern = %s\n",pattern);
	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"path"))
		{
			strcpy(dirname,p+5);
			temppath = url_decode(dirname);
			sprintf(filepath,"%s",temppath);
		}
		else if(strstr(p,str_callback))
			strcpy(callback,p+9);
	}

	//getFileList
	char *ep = NULL;
	char path_t[128]={0};
	char cmdd[128]={0};
	
	ep = strstr(temppath, "miiiCasa_Photos");
	if((strstr(temppath,"/USB_USB2FlashStorage_45c0216_1"))||(strcmp(temppath,"/") == 0)){
		char *str1 = str_replace(temppath,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);
		strcpy(path_t,"miiiCasa_Photos");
		if((fp3 = fopen(str1,"r")) == NULL)
		{
			memset(cmdd, 0, sizeof(cmdd));
			sprintf(cmdd,"mkdir \"%s\"",str1);
			system(cmdd);
		}
		else
			fclose(fp3);
		memset(cmdd, 0, sizeof(cmdd));
		sprintf(cmdd,"ls -le %s > /var/miii_usbls.txt",str1);
		if(str1) free(str1);
	}else{		
		if(ep) strcpy(path_t, ep);
		else strcpy(path_t, "miiiCasa_Photos");
		memset(cmdd, 0, sizeof(cmdd));
		sprintf(cmdd,"%s/%s",mount,path_t);
		if((fp3 = fopen(cmdd,"r")) == NULL)
		{
			memset(cmdd, 0, sizeof(cmdd));
			sprintf(cmdd,"mkdir \"%s/%s\"",mount,path_t);
			system(cmdd);
		}
		else
			fclose(fp3);
		memset(cmdd, 0, sizeof(cmdd));
		sprintf(cmdd,"ls -le %s/%s > /var/miii_usbls.txt",mount,path_t);
		DEBUG_MSG("### ls -le %s/%s > /var/miii_usbls.txt \n",mount,path_t);
	}
	system(cmdd);
	
	fp1 = fopen("/var/miii_usbls.txt","r");
		
	if(fp1 == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/miii_usbls.txt");		
		return;
	}else{ 
		while(fgets(listtemp,sizeof(listtemp),fp1))
		{	
			sscanf(listtemp, "%s %s %s %s %s %s %s %s %s %s %s %s", arrt, noused, noused, noused, size, week, month, day, time , year, name, nametmp);
			//printf("XXgetFileXX=%s %s %s %s %s %s %s %s %s %s %s %s\n", arrt, noused, noused, noused, size, week, month, day, time , year, name, nametmp);
			if(strstr(arrt,"drw"))
				file_type = 2;
			else if(strstr(arrt,"-rw"))	
				file_type = 1;
			else if(strstr(arrt,"lrw"))
				file_type = 3;
			else
			        file_type = 1;
			DEBUG_MSG("YYgetFile arrt=%s,name=%s,file_type=%d,size=%s\n",arrt,name,file_type,size);
						
			if(file_type == 1)
			{
				sprintf(url_addr,"http:\\/\\/%s\\/da%s\\/%s",nvram_safe_get("lan_ipaddr"),temppath,name); 
				DEBUG_MSG("getFileList url_addr = %s\n",url_addr);
				char *ext =	rindex(name,'.');
				strcpy(ext_value,(ext+1));
				DEBUG_MSG("ext_value value =%s\n",ext_value);													
			}else{
				strcpy(url_addr,"");
			}
			file_mtime = GMTtoEPOCH(year,month,day,time);
			memset(listtemp,0,sizeof(listtemp));
			sprintf(usblsbuff1 , "{\"name\":\"%s\",\"type\":%d,\"thumb\":%d,\"uri\":\"%s\",\"ext\":\"%s\",\"size\":%s,\"mtime\":%d},",
				name,file_type,1288248332,url_addr,ext_value,size,file_mtime);
							
			if(strstr(callback,"photosMain")){
				if(strstr(arrt,"d")){	
					list_count++;
					strcat(usblsbuff, usblsbuff1);
				}
			}else{
				list_count++;
				strcat(usblsbuff, usblsbuff1);
			}	
			DEBUG_MSG("@@usblsbuff =%s@@\n",usblsbuff);
		}//while
		usblsbuff[strlen(usblsbuff)-1]=0;
		DEBUG_MSG("### Miii_list_count =%d \n",list_count);
		DEBUG_MSG("### Miii_usblsbuff =%s \n",usblsbuff);
		fclose(fp1);		
	}
	
	DEBUG_MSG("### Miii_FilePath = %s \n", filepath);
	DEBUG_MSG("!!!!!!!!!!!!mount = %s!!!!!!!!!!\n",mount); 
	DEBUG_MSG("!!!!!!!!!!!!temppath = %s!!!!!!!!!!\n",temppath); 

	DEBUG_MSG("^^^^ Miii_FilePath = %s \n", filepath);
	
/*
({"status":"ok","errno":"","errmsg":"",
"count":1,"filepath":"\/USB_USB2FlashStorage_45c0216_1\/Photos","label_name":"HDDREG",
"cover":0,"files":[{"name":"Matt_test_miii_folder","type":2,"thumb":0,"uri":"","ext":"","size":4096,"mtime":1285833302}]});

*/
	miii_response_t = malloc((list_count+1)*1024);

	if(strcmp(temppath,"/") == 0){
		char usblsbuff2[1024];
		//sprintf(miii_response_t,"%s","miiiAPI.showDir({\"status\":\"ok\",\"errno\":"",\"errmsg\":"",\"count\":1,\"filepath\":\"/\",\"label_name\":"",\"cover\":0,\"files\":[{\"name\":\"USB_USB2FlashStorage_45c0216_1\",\"type\":2,\"thumb\":0,\"uri\":"",\"ext\":"",\"size\":4096,\"mtime\":0}]});");
		//	list_count =1 ;
		//filepath ="\/";
		strcpy(label_name,"\"\"");
		sprintf(usblsbuff2 , "{\"name\":\"%s\",\"type\":%d,\"thumb\":%d,\"uri\":\"\",\"ext\":\"\",\"size\":%d,\"mtime\":%d}",
			"USB_USB2FlashStorage_45c0216_1",2,0,4096,0);
		sprintf(miii_response_t, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\","
			"\"count\":%d,\"filepath\":\"%s\",\"label_name\":%s,"
			"\"cover\":%d,\"files\":[%s]});",  
			callback, str_status, files, "",1,filepath,label_name
			,1292469185, usblsbuff2);
	}else{	

		sprintf(miii_response_t, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\","
			"\"count\":%d,\"filepath\":\"%s\",\"label_name\":%s,"
			"\"cover\":%d,\"files\":[%s]});",                                         
			callback, str_status, files, "",list_count,filepath, label_name,cover, usblsbuff);
	}	
	
	wfputs(miii_response_t, stream);

	free (pattern);
	free(miii_response_t);

	return;
}

void do_miii_getFile(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char tok[256];
	char callback[256];
	char fullfilename[256];
	char *p = NULL;
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[40]={0};
	char files[20];
	char *temppath = NULL;
	char filepath[128]={0};
	int tr ;
	strcpy(str_status, "fail");
	strcpy(str_errno, "5005");
	strcpy(str_errmsg, "No such file or directory");
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(fullfilename, 0, 256);

	if(miiicasa_set_folder() == -1) return;
	
	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));


	p_pattern = strchr (pattern, '?') + 1;

	//path=/ws/api/getFile?tok=4c5fbaf0796ba:1a13ce642f0b06727ec7d8992b3de782&fullfilename=/USB_USB2FlashStorage_45c0216_1/test.jpg
	//GET /ws/api/getFile?tok=4ca442f3eea04:b2ac7f6f06c026d13603e055cdf6cd4b&fullfilename=%2FUSB_USB2FlashStorage_45c0216_1%2FPhotos%2FMatt_test_miii_folder%2F.miiithumbs%2Fdesc.txt&callback=MIIICASA.photosList.afterDownloadDesc&tx=1285833368750524 HTTP/1.1

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"fullfilename"))
		{
			strcpy(fullfilename,p+13);
			DEBUG_MSG("fullfilename=%s\n",fullfilename);
			temppath = url_decode(fullfilename);
	 		sprintf(filepath,"\\%s",temppath);
	 		DEBUG_MSG("getFile filepath =%s\n",filepath);
		}
		else if(strstr(p,"tr=")){
			strcpy(tr,p+3);
			DEBUG_MSG("tr=%d\n",tr);
		}
		else if(strstr(p,str_callback)){
			strcpy(callback,p+9);
			DEBUG_MSG("callback=%s\n",callback);
		}
	}
	
	memset(miii_response, 0, sizeof(miii_response));
	
	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\"});",                                         
		callback, str_status, str_errno, str_errmsg);
	wfputs(miii_response, stream);
	free (pattern);
	
	return;
}

void do_miii_uploadFile(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char tok[256];
	char callback[256];
	char dirname[256];
	char *p = NULL;
	char model_name[32]={0};
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char files[20];
	char filesystem[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0};
	char *combuf=NULL;
	FILE *fp=NULL;

	if(miiicasa_set_folder() == -1) return;
	
	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		return;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}	


	strcpy(str_status, "ok");
	strcpy(str_errno, "");
	strcpy(files, "");
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(dirname, 0, 256);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));


	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern, "&");
	// callback
	p = strtok(NULL, "&");
	p = strtok(NULL, "&");
	strcpy(dirname, p + 8);
	p = strtok(NULL, "&");
	strcpy(callback, p + 9);

	memset(miii_response, 0, sizeof(miii_response));

	sprintf(miii_response, 	"%s{\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\",\"mtime\":%s,\"size\":\"%s\"};", 
		callback, str_status, files, str_errmsg);
			
	wfputs(miii_response, stream);

	free (pattern);
	
	return;
}
	
void do_miii_downloadFile(char *path, FILE *stream)
{
	if(miiicasa_set_folder() == -1) return;
	
	return;
}

void do_miii_deleteFiles(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *temppath = NULL;
	char tok[256];
	char callback[256];
	char tmppath[256];
	char dirname[128];
	char tx[20];
	char *p = NULL;
	char rootdir[128] = var_tmp;
	char model_name[32]={0};
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char filesystem[128]={0},filepath[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0},tempmount[128] = {0};
	char *combuf=NULL;
	FILE *fp=NULL;

	if(miiicasa_set_folder() == -1) return;
	
	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		return -1;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}

	strcpy(tempmount, mount);
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(tmppath, 0, 256);
	memset(dirname, 0, 128);
	memset(filepath, 0, 256);
	memset(tx, 0, 20);
	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));
	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"fullfilenames"))
		{
			strcpy(tmppath,p+14);
			temppath = url_decode(tmppath);
			//tmppath[strlen(tmppath)-1]='\0';
			sprintf(filepath,"%s",temppath );
		}
		else if(strstr(p,"dirname"))
			strcpy(dirname,p+8);	
		else if(strstr(p,str_callback))
			strcpy(callback,p+9);	
	}
	DEBUG_MSG("####deldir-callback = %s\n",dirname);
	DEBUG_MSG("####deldir-tmppath = %s\n",tmppath);
	DEBUG_MSG("####deldir-filepath = %s\n",filepath);
	DEBUG_MSG("####deldir-callback = %s\n",callback);
		
	char *pp =NULL;
	char tmep_pp[]={};
	char *pp1 =NULL;
	if(strstr(filepath,"/"))
	{
		//  \[\"\/USB_USB2FlashStorage_45c0216_1\/Photos\/test\"\]
				
		pp = strstr(filepath,"/");
		strcpy(tmep_pp,pp);
		tmep_pp[strlen(tmep_pp)-3]=0;
	}
	//Do Make Dir
	combuf = (char *)malloc(strlen(mount)+256);
	if(strcmp(filepath,"/") == 0)
	{
		sprintf(filepath,"%s",tempmount);
		sprintf(combuf,"rm -rf %s",tmep_pp);
	
	}else if(strstr(tmep_pp,"/USB_USB2FlashStorage_45c0216_1"))
	{
		char *str1 = str_replace(tmep_pp,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);
		if(strstr(str1,","))
		{
			char str2_tmp[256]={};
		        char *str2 = str_replace(str1,"\\\"\\\,\\\"\\"," ");
			char *delim ="  ";
			char *delim2 =".";
			char *str3 = NULL;
			char *str4 = NULL;
			if(strcmp(callback,"MIIICASA.photosList.afterDeleteFiles")==0){			
				strcpy(str2_tmp,str2);
				str3 = strtok(str2_tmp,delim);
										
				char dirpath[128], buf[128]={0},basepath[128]={0},tmp_basepath[128]={0};
				FILE *fp2=NULL;
				FILE *fp3=NULL;
				char *sp =NULL;
				memset(dirpath, '\0', sizeof(dirpath));
				sprintf(buf,"dirname %s",str3);
				if ((fp2 = popen(buf, "r")) == NULL)
					return -1;
				fgets(dirpath, sizeof(dirpath), fp2);
				pclose(fp2);
				dirpath[strlen(dirpath) - 1] = 0;
												
				memset(basepath, '\0', sizeof(basepath));
				sprintf(buf,"basename %s",str3);
				if ((fp3 = popen(buf, "r")) == NULL)
					return -1;
				fgets(basepath, sizeof(basepath), fp3);
				sp = strstr(basepath,delim2);
				*sp='\0';
				strcpy(tmp_basepath,basepath);
				pclose(fp3);
											
				sprintf(combuf,"rm -rf %s/.miiithumbs/%s*",dirpath,tmp_basepath);
				system(combuf);
				free(combuf);
												
				while((str3 = strtok(NULL,delim))){
					memset(dirpath, '\0', sizeof(dirpath));
					sprintf(buf,"dirname %s",str3);
					if ((fp2 = popen(buf, "r")) == NULL)
						return -1;
					fgets(dirpath, sizeof(dirpath), fp2);
					pclose(fp2);
					dirpath[strlen(dirpath) - 1] = 0;
													
					memset(basepath, '\0', sizeof(basepath));
					sprintf(buf,"basename %s",str3);
					if ((fp3 = popen(buf, "r")) == NULL)
						return -1;
					fgets(basepath, sizeof(basepath), fp3);
					pclose(fp3);
					sp = strstr(basepath,delim2);
					*sp='\0';
					strcpy(tmp_basepath,basepath);
					sprintf(combuf,"rm -rf %s/.miiithumbs/%s*",dirpath,tmp_basepath);
					system(combuf);
					free(combuf);
				}
			}else{
				sprintf(combuf,"rm -rf %s/",str2);
				system(combuf);
				free(combuf);
			}
	        }else{
		        sprintf(combuf,"rm -rf %s/",str1);
		        if(str1) free(str1);
		}
	}else{
		char patchbuf[128]={0};
		//patchbuf = strcat(var_tmp,filepath);
		sprintf(combuf,"rm -rf %s%s/%s/.miiithumbs",rootdir,filepath,dirname);
	}
	system(combuf);
	free(combuf);
	strcpy(str_status,"ok");
	strcpy(str_errno, "");
	strcpy(str_errmsg, "");

	memset(miii_response, 0, sizeof(miii_response));

	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\"});", 
		callback, str_status,str_errno,str_errmsg);
	wfputs(miii_response, stream);

	free (pattern);
	
	return;
}	
		
void do_miii_getRouter(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char sig[20];
	char expires[20];
	char routerAccessKeyId[20];
	char callback[30];
	char t[20];
	char *p = NULL;
	char *p_end = NULL;
	char lan_mac[6]={};
	char wan_mac[6]={};
	char model_name[20];
	char lan_ip[20];
	char wan_ip[20];
	char sn[20];
	char str_status[20];
	char str_errno[20];
	char str_errmsg[20];
	char str_vid[20];
	char str_pid[20];
	char str_wan_mac[20];
	char str_lan_mac[20];
    
	get_macaddr( nvram_safe_get("lan_bridge"), lan_mac);
	get_macaddr( nvram_safe_get("wan_eth"), wan_mac);
    
	sprintf(str_wan_mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
		wan_mac[0],wan_mac[1],wan_mac[2],wan_mac[3],wan_mac[4],wan_mac[5]);
	sprintf(str_lan_mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
		lan_mac[0],lan_mac[1],lan_mac[2],lan_mac[3],lan_mac[4],lan_mac[5]);
                
	strcpy(model_name, nvram_safe_get("model_number"));
 	strcpy(lan_ip, nvram_safe_get("lan_ipaddr"));
	strcpy(wan_ip, nvram_safe_get("wan_eth"));
    
	strcpy(str_status, "ok");
	strcpy(str_vid, nvram_safe_get("manufacturer"));
	strcpy(str_pid, "MUCHIII_PROTOTYPE");
    
	strcpy(sn, wan_ip);
    
	strcpy(str_errno, "");

	memset(sig, 0, 20);
	memset(expires, 0, 20);
	memset(routerAccessKeyId, 0, 20);
	memset(callback, 0, 30);
	memset(t, 0, 20);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;

	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	{
		p = NULL;
		p = strstr(p_pattern, str_tok);
		if (p) {
			p += strlen (str_tok);
			p_end = NULL;
			p_end = strchr (p, '&');
			if (p_end) {
				memcpy(sig, p, (unsigned long) p_end - (unsigned long) p);
			}
		}

		p = NULL;
		p = strstr(p_pattern, str_expires);
		if (p) {
			p += strlen (str_expires);
			p_end = NULL;
			p_end = strchr (p, '&');
			if (p_end) {
				memcpy(expires, p, (unsigned long) p_end - (unsigned long) p);
			}
		}
	
		p = NULL;
		p = strstr(p_pattern, str_routerAccessKeyId);
		if (p) {
			p += strlen (str_routerAccessKeyId);
			p_end = NULL;
			p_end = strchr (p, '&');
			if (p_end) {
				memcpy(routerAccessKeyId, p, (unsigned long) p_end - (unsigned long) p);
			}
		}		

		p = strstr(p_pattern, str_t);
		if (p) {
			p += strlen (str_t);
			p_end = NULL;
			p_end = strchr (p, '&');
			if (p_end) {
				memcpy(t, p, (unsigned long) p_end - (unsigned long) p);
			}
		}	

		p = strstr(p_pattern, str_callback);
		if (p) {
			p += strlen (str_callback);
			p_end = NULL;
			p_end = strchr (p, '&');
			if (p_end) {
				memcpy(callback, p, (unsigned long) p_end - (unsigned long) p);
			}
			else
				memcpy(callback, p, strlen(p));
		}	

		memset(miii_response, 0, sizeof(miii_response));

/*

MIIICASA.hpDevice.afterGetDeviceInfo({"status":"ok","errno":"","errmsg":"","device":{"did":"47bce5c74f589f4867dbd57e9ca9f808","brand":"D-Link","miiicasa_enabled":"1","wifi_secure_enabled":"0","mac":"00:26:5A:AD:BD:95","model":"DIR-685","ws_ip":"192.168.0.1","ws_port":"80","external_ip":"172.21.33.80","external_port":"5555"},"user_mac":"00:1D:92:52:07:CF"});

*/


		DEBUG_MSG(	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\","
			"\"router\":{\"vid\":\"%s\",\"pid\":\"%s\","
			"\"sn\":\"%s\",\"mac\":\"%s\","
			"\"model\":\"%s\",\"ws_ip\":\"%s\",\"ws_port\":\"%d\","
			"\"external_ip\":\"%s\","
			"\"external_port\":\"%d\"},\"user_mac\":\"%s\"});", 
			callback, str_status, str_errno, str_errmsg,
			str_vid, str_pid,
			sn, str_wan_mac, 
			model_name, lan_ip, 80,
			wan_ip, 
			5555, str_wan_mac);

		sprintf(miii_response,
			"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\","
			"router:{\"vid\":\"%s\",\"pid\":\"%s\","
			"\"sn\":\"%s\",\"mac\":\"%s\","
			"\"model\":\"%s\",\"ws_ip\":\"%s\",\"ws_port\":\"%d\","
			"\"external_ip\":\"%s\","
			"\"external_port\":\"%d\"},\"user_mac\":\"%s\"});", 
			callback, str_status, str_errno, str_errmsg,
			str_vid, str_pid,
			sn, str_wan_mac, 
			model_name, lan_ip, 80,
			wan_ip, 
			5555, str_wan_mac);

		wfputs(miii_response, stream);

	}

	free (pattern);

	return;	
}

void do_miii_listDevices(char *path, FILE *stream)
{
	return;
}

void do_miii_createDirectory(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *temppath = NULL;
	char tok[256];
	char callback[256];
	char tmppath[256];
	char dirname[128];
	char *p = NULL;
	char rootdir[128] = var_tmp;
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char files[20];
  	char filesystem[128]={0},filepath[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0},tempmount[128] = {0};
	char *combuf=NULL,*combuf_t=NULL;
	FILE *fp=NULL;

	if(miiicasa_set_folder() == -1) return;
	
	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		return;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}

	strcpy(tempmount, mount);
	strcpy(str_status, "ok");
	strcpy(str_errno, "");
	strcpy(files, "");
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(tmppath, 0, 256);
	memset(dirname, 0, 128);
	memset(filepath, 0, 256);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"path"))
		{
			strcpy(tmppath,p+5);
			temppath = url_decode(tmppath);
			sprintf(filepath,"%s",temppath );
		}
		else if(strstr(p,"dirname"))
			strcpy(dirname,p+8);	
		else if(strstr(p,str_callback))
			strcpy(callback,p+9);	
	}
	//Do Make Dir
	combuf = (char *)malloc(strlen(mount)+256);
	combuf_t = (char *)malloc(strlen(mount)+256);
	if(strcmp(filepath,"/") == 0)
	{
		sprintf(filepath,"%s",tempmount);
		sprintf(combuf,"mkdir -p %s/%s/.miiithumbs",filepath,dirname);
		sprintf(combuf_t,"cp -rf /www/cover_100.jpg %s/%s/.miiithumbs/cover_100.jpg",filepath,dirname);
	
	}else if(strstr(filepath,"/USB_USB2FlashStorage_45c0216_1"))
	{
		char *str1 = str_replace(filepath,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);	
		sprintf(combuf,"mkdir -p %s/%s/.miiithumbs",str1,dirname);
		sprintf(combuf_t,"cp -rf /www/cover_100.jpg %s/%s/.miiithumbs/cover_100.jpg",str1,dirname);
		if(str1) free(str1);
	}else{
		char patchbuf[128]={0};
		//patchbuf = strcat(var_tmp,filepath);
		sprintf(combuf,"mkdir -p %s%s/%s/.miiithumbs",rootdir,filepath,dirname);
		sprintf(combuf_t,"cp -rf /www/cover_100.jpg %s%s/%s/.miiithumbs/cover_100.jpg",rootdir,filepath,dirname);
	}
	system(combuf);
	free(combuf);
	//system(combuf_t);
	free(combuf_t);

	memset(miii_response, 0, sizeof(miii_response));

	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\"});", 
		callback, str_status, files, str_errmsg);
			
	wfputs(miii_response, stream);

	free (pattern);
	
	return;
}	

void do_miii_renameFile(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *temppath = NULL;
	char tok[256];
	char callback[256];
	char tmppath[256];
	char src_fullfilename[128];
	char dst_filename[128];
	char *p = NULL;
	char rootdir[128] = var_tmp;
	char model_name[32]={0};
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char files[20];
  	char filesystem[128]={0},filepath[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0},tempmount[128] = {0};
	char *combuf=NULL;
	FILE *fp=NULL;

	if(miiicasa_set_folder() == -1) return;
	
	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		return;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}

	strcpy(tempmount, mount);
	strcpy(str_status, "ok");
	strcpy(str_errno, "");
	strcpy(files, "");
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(tmppath, 0, 256);
	memset(src_fullfilename, 0, 128);
	memset(dst_filename, 0, 128);
	memset(filepath, 0, 256);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"src_fullfilename"))
			strcpy(src_fullfilename,p+17);
		else if(strstr(p,"dst_filename"))
			sprintf(dst_filename,"/%s",p+13);		
		else if(strstr(p,str_callback))
				strcpy(callback,p+9);	
	}

	//Do Make Dir
	combuf = (char *)malloc(strlen(mount)+256);
        if(strstr(src_fullfilename,"/USB_USB2FlashStorage_45c0216_1"))
       {
		char *str1 = str_replace(src_fullfilename,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);
													
		char dirpath[128], buf[128]={0};
		FILE *fp = NULL;
		memset(dirpath, '\0', sizeof(dirpath));
		sprintf(buf,"dirname %s",str1);
		if ((fp = popen(buf, "r")) == NULL)
			return;
		fgets(dirpath, sizeof(dirpath), fp);
		pclose(fp);
		dirpath[strlen(dirpath) - 1] = 0;
		char *pathdirbuf= NULL ;
		strcat(dirpath,dst_filename);
		sprintf(combuf,"mv %s %s",str1,dirpath);
		if(str1) free(str1);
	}
	system(combuf);
	free(combuf);

	memset(miii_response, 0, sizeof(miii_response));

	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\"});", 
		callback, str_status, files, str_errmsg);
			
	wfputs(miii_response, stream);

	free (pattern);
	
	return;
}	

void do_miii_copyFilesTo(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *temppath = NULL;
	char tok[256];
	char callback[256];
	char tmppath[256];
	char src_path[128];
	char dst_path[128];
	char *p = NULL;
	char rootdir[128] = var_tmp;
	char model_name[32]={0};
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char files[20];
	char filesystem[128]={0},filepath[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0},tempmount[128] = {0};
		char *combuf=NULL;
	FILE *fp=NULL;

	if(miiicasa_set_folder() == -1) return;
	
	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		return;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			printf("temp=%s\n",temp);
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			printf("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}

	strcpy(tempmount, mount);
	strcpy(str_status, "ok");
	strcpy(str_errno, "");
	strcpy(files, "");
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(tmppath, 0, 256);
	memset(src_path, 0, 128);
	memset(dst_path, 0, 128);
	memset(filepath, 0, 256);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"src_filenames"))
		{
			strcpy(tmppath,p+14);
			temppath = url_decode(tmppath);
			//tmppath[strlen(tmppath)-1]='\0';
			sprintf(filepath,"%s",temppath );
		}
		else if(strstr(p,"src_path"))
			strcpy(src_path,p+9);
		else if(strstr(p,"dst_path"))
			strcpy(dst_path,p+9);		
		else if(strstr(p,str_callback))
			strcpy(callback,p+9);	
	}
	char *pp0 =NULL;
	char *pp =NULL;
	char *pp1 =NULL;
	
	 //[\"inject.txt\"]
	pp0 = strchr (filepath, '"') +1;
	pp = strtok(pp0,"\\");
		
	//Do Make Dir
	combuf = (char *)malloc(strlen(mount)+256);
        if(strstr(src_path,"/USB_USB2FlashStorage_45c0216_1"))
       {
		char *str1 = str_replace(src_path,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);
		char *str2 = str_replace(dst_path,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);	
		sprintf(combuf,"cp %s%s %s",str1,pp,str2);
		if(str1) free(str1);
		if(str2) free(str2);
	}
	system(combuf);
	free(combuf);

	memset(miii_response, 0, sizeof(miii_response));

	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\"});", 
		callback, str_status, files, str_errmsg);
			
	wfputs(miii_response, stream);

	free (pattern);
	
	return;
}	

void do_miii_moveFilesTo(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *temppath = NULL;
	char tok[256];
	char callback[256];
	char tmppath[256];
	char src_path[128];
	char dst_path[128];
	char *p = NULL;
	char rootdir[128] = var_tmp;
	char model_name[32]={0};
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char files[20];
	char filesystem[128]={0},filepath[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0},tempmount[128] = {0};
	char *combuf=NULL;
	FILE *fp=NULL;

	if(miiicasa_set_folder() == -1) return;

	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		return;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			printf("temp=%s\n",temp);
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			printf("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}

	strcpy(tempmount, mount);
	strcpy(str_status, "ok");
	strcpy(str_errno, "");
	strcpy(files, "");
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(tmppath, 0, 256);
	memset(src_path, 0, 128);
	memset(dst_path, 0, 128);
	memset(filepath, 0, 256);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"src_filenames"))
		{
			strcpy(tmppath,p+14);
			temppath = url_decode(tmppath);
			//tmppath[strlen(tmppath)-1]='\0';
			sprintf(filepath,"%s",temppath );
		}
		else if(strstr(p,"src_path"))
			strcpy(src_path,p+9);
		else if(strstr(p,"dst_path"))
			strcpy(dst_path,p+9);		
		else if(strstr(p,str_callback))
			strcpy(callback,p+9);	
	}
	char *pp0 =NULL;
	char *pp =NULL;
	char *pp1 =NULL;
		
	 //[\"inject.txt\"]
	pp0 = strchr (filepath, '"') +1;
	pp = strtok(pp0,"\\");

	//Do Make Dir
	combuf = (char *)malloc(strlen(mount)+256);
        if(strstr(src_path,"/USB_USB2FlashStorage_45c0216_1"))
	{
		char *str1 = str_replace(src_path,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);
		char *str2 = str_replace(dst_path,"/USB_USB2FlashStorage_45c0216_1",miii_usb_folder);	
		sprintf(combuf,"mv %s%s %s",str1,pp,str2);
		if(str1) free(str1);
		if(str2) free(str2);
	}
	system(combuf);
	free(combuf);

	memset(miii_response, 0, sizeof(miii_response));

	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\"});", 
		callback, str_status, files, str_errmsg);
			
	wfputs(miii_response, stream);

	free (pattern);
	
	return;
}	

void do_miii_getVersion(char *path, FILE *stream)
{
	char *pattern = NULL;
	char *p_pattern = NULL;
	char *temppath = NULL;
	char tok[256];
	char callback[256];
	char dirname[128];
	char *p = NULL;
	char model_name[32]={0};
	char str_status[20]={0};
	char str_errno[20]={0};
	char str_errmsg[20]={0};
	char files[20];
	char api_ver[20] ={0};
	char filesystem[128]={0},filepath[128]={0},totalsize[12]={0},usedsize[12]={0},freesize[12]={0};
	char usedscale[12]={0},mount[128]={0},temp[256]={0},tempmount[128] = {0};
	char *combuf=NULL;
	FILE *fp=NULL;

	system("df -k |grep sd > /var/httpdf.txt");
	fp = fopen("/var/httpdf.txt","r");

	if(fp == NULL){
		printf("%s:\"%s\" does not exist\n",__func__,"/var/httpdf.txt");	
		return -1;
	}else{
		while(fgets(temp,sizeof(temp),fp))
		{		
			sscanf(temp, "%s %s %s %s %s %s",  filesystem, totalsize, usedsize, freesize, usedscale, mount);
			// /dev/sda1 1970372 1129008 841364 57% /var/usb/sda/sda1
			DEBUG_MSG("%s %s %s %s %s %s\n", filesystem, totalsize, usedsize, freesize, usedscale, mount);
			memset(temp,0,sizeof(temp));			
		}//while
		fclose(fp);				
	}

	strcpy(tempmount, mount);
	strcpy(str_status, "ok");
	strcpy(str_errno, "");
	strcpy(files, "");
	memset(tok, 0, 256);
	memset(callback, 0, 256);
	memset(dirname, 0, 128);
	memset(filepath, 0, 256);

	pattern = malloc(strlen(path)+1);

	if (pattern == NULL)
		return;
	memset(pattern, 0, strlen(path)+1);
	memcpy(pattern, path, strlen(path));

	p_pattern = strchr (pattern, '?') + 1;

	// tok
	p = strtok(p_pattern,"&");
	while((p=strtok(NULL,"&")))
	{
		if(strstr(p,"path"))
		{
			strcpy(dirname,p+5);
			temppath = url_decode(dirname);
			sprintf(filepath,"\\%s",temppath);
		}
		else if(strstr(p,str_callback))
			strcpy(callback,p+9);
	}
		
	char ver_firm[5] = {0};
	char *fw_major="", *fw_minor="";
	memcpy(ver_firm, VER_FIRMWARE, 4);
	strcat(ver_firm,".");
	getStrtok(ver_firm, ".", "%s %s", &fw_major, &fw_minor);
	
	sprintf(api_ver,"%s.%s",VER_FIRMWARE,VER_BUILD); 
	
	memset(miii_response, 0, sizeof(miii_response));

	sprintf(miii_response, 	"%s({\"status\":\"%s\",\"errno\":\"%s\",\"errmsg\":\"%s\",\"version\":\"%s\"});", 
		callback, str_status, files, str_errmsg, api_ver);
			
	wfputs(miii_response, stream);

	free (pattern);
	
	return;
}

void do_miii_photosUpload(char *path, FILE *stream)
{
	return;
}

void do_miii_uploadTo(char *path, FILE *stream)
{
	return;
}
void do_miii_updateRid(char *path, FILE *stream)
{
	return;
}
void do_miii_updateStatusTo(char *path, FILE *stream)
{
	return;
}
#endif

void
do_ej(char *path, FILE *stream)
{
	FILE *fp;
	int c;
	char pattern[1000], *asp = NULL, *func = NULL, *end = NULL, *dynamic_page = NULL;
	int len = 0;
	int if_get_dyn_type=0;
	char dynamic_page_type[32]={0};

	char wwwPath[20] = "/www/";
	
	strcat(wwwPath, path);
	
	if (!(fp = fopen(wwwPath, "r")))
		return;
	
	while ((c = getc(fp)) != EOF) {

		/* Add to pattern space */
		pattern[len++] = c;
		pattern[len] = '\0';
		if (len == (sizeof(pattern) - 1))
			goto release;
		
		/* Look for <@ ... */
		if (!dynamic_page && !strncmp(pattern, "<@", len)) {
			if (len == 2)
			{
				dynamic_page = pattern + 2;
				len = 0;
			}
			continue;
		}

		if (dynamic_page) {
			/* Look for dynamic page type, ie <@ "XXX"..., which end with another '<'  */
			if( !if_get_dyn_type)
			{
				if (unqstrstr(pattern, "<"))
				{
					func = pattern;
					
					/* Skip initial whitespace */
					for (; isspace((int)*func); func++);
				
					strncpy(dynamic_page_type,func,len-1);
					dynamic_page_type[len]='\0';
					
					pattern[0]='<';
					len = 1;
					if_get_dyn_type =1;
					
				}
			}
			/* Look for ... @> */
			else if (end = unqstrstr(dynamic_page, "@>")) {
				func = pattern;
				*end = '\0';
				
				/* Skip initial whitespace */
				for (; isspace((int)*func); func++);

				if(
#ifdef CONFIG_USER_TC 
					!strncmp( dynamic_page_type,"qos",strlen("qos")) || 
#endif
#ifdef CONFIG_USER_WISH
					!strncmp( dynamic_page_type,"wish",strlen("wish")) ||
#endif
#ifdef IPv6_SUPPORT
                                        !strncmp( dynamic_page_type,"ipv6",strlen("ipv6")) ||
#endif
 
					0)
				{
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
					wfputs(pattern, stream);
				}	
				
				dynamic_page = NULL;
				len = 0;
				if_get_dyn_type = 0;
			}
			continue;
		}
		
		/* Look for <% ... */
		if (!asp && !strncmp(pattern, "<%", len)) {
			if (len == 2)
				asp = pattern + 2;
			continue;
		}

		/* Look for ... %> */
		if (asp) {
			if (unqstrstr(asp, "%>")) {
				for (func = asp; func < &pattern[len]; func = end) {
					/* Skip initial whitespace */
					for (; isspace((int)*func); func++);
					if (!(end = unqstrstr(func, ";")))
						break;
					*end++ = '\0';
					/* Call function */
					call(func, stream);
				}
				asp = NULL;
				len = 0;
			}
			continue;
		}
		
	release:
/*	Date:	2009-04-09
 *	Name:	Yufa Huang
 *	Reason: add HTTPS function.
 */
		/* Release pattern space */
		wfputs(pattern, stream);
		len = 0;
	}

	fclose(fp);
}

int
ejArgs(int argc, char **argv, char *fmt, ...)
{
	va_list	ap;
	int arg;
	char *c;

	if (!argv)
		return 0;

	va_start(ap, fmt);
	for (arg = 0, c = fmt; c && *c && arg < argc;) {
		if (*c++ != '%')
			continue;
		switch (*c) {
		case 'd':
			*(va_arg(ap, int *)) = atoi(argv[arg]);
			break;
		case 's':
			*(va_arg(ap, char **)) = argv[arg];
			break;
		}
		arg++;
	}
	va_end(ap);

	return arg;
}

#ifdef OPENDNS

static const char * const redirecter_footer =
"<html>"
"<head>"
"<script>"
"function redirect(){"
"   document.location.href = '%s';"
"}"
"</script>"
"</head>"
"<script>"
"   redirect();"
"</script>"
"</html>"
;

static char *get_path_value(char *path,char *name,char *value,int maxlen)
{
	char *tmp;
	char *tmp2;
	char pattern[128];
	int len;

	strcpy(pattern,name);
	strcat(pattern,"=");

	tmp = strstr(path,pattern);

	if( !tmp)
		return NULL;

	tmp += strlen(pattern);

	tmp2 = strchr(tmp,'&');

	if( tmp2) {
		// tmp2 to  tmp is our value
		len = tmp2-tmp;

	}
	else {
		len = strlen(tmp);
	}

	if( len >= maxlen)
		len = maxlen - 1;

	memcpy(value,tmp,len);
	value[len] = 0;

	return tmp;

}

extern char opendns_token[32+1]; // 32 bytes
extern int opendns_token_time;
extern int opendns_token_failcode;


int check_opendns_timeout(char *token)
{
	int now_time;

	now_time = time((time_t *) NULL);

	DEBUG_MSG("check_opendns_timeout\n");
	DEBUG_MSG("now_time=%d\n",now_time);
	DEBUG_MSG("opendns_token_time=%d\n",opendns_token_time);
	DEBUG_MSG("token=%s\n",token);
	DEBUG_MSG("opendns_token=%s\n",opendns_token);

	if( now_time > (opendns_token_time + (30 * 60)) ) {
		// timeout 30*60 seconds
		return -1;
	}

	// check token
	if( strlen(opendns_token) < 8) {
		// token fail
		return -2;
	}

// no token, not to check 
	if( token != NULL)
	if( strncmp(token,opendns_token,16) != 0) {
		// token fail
		return -3;
	}


	return 0;

}


void do_parentalcontrols(char *path, FILE *stream)
{

	int UI=0;
	char str_response[512];
	char *tmp;

	DEBUG_MSG("do_parentalcontrols\n");

	DEBUG_MSG("%s\n",path);

	char id[32]={0};
	char ip1[32]={0};
	char ip2[32]={0};
	char token[32]={0};

	
	if( strstr(path,"UI") )
		UI = 1;


	get_path_value(path,"deviceid",id,32);
	get_path_value(path,"dnsip1",ip1,32);
	get_path_value(path,"dnsip2",ip2,32);
//	get_path_value(path,"token",token,32);

//parentalcontrols?deviceid=000251879117BD20&dnsip1=208.67.222.222&dns
//ip2=208.67.220.220&token=token

	DEBUG_MSG("id=[%s]\n",id);
	DEBUG_MSG("ip1=[%s]\n",ip1);
	DEBUG_MSG("ip2=[%s]\n",ip2);
//	DEBUG_MSG("token=[%s]\n",token);

	opendns_token_failcode = 0;

	int ret;

//	ret = check_opendns_timeout(token);
	ret = check_opendns_timeout(NULL);

	if( ret < 0) {
		opendns_token_failcode = ret;
		
		if( UI) 
			sprintf(str_response,redirecter_footer,"/opendns_failUI.asp");	
		else
			sprintf(str_response,redirecter_footer,"/opendns_fail.asp");	

		wfputs(str_response, stream);
		return;
	}
//	strcpy(opendns_token,""); // reset opendns_token
// token will be used

	nvram_set("opendns_enable","1");
	nvram_set("opendns_mode","2");
	nvram_set("opendns_deviceid",id);
	nvram_set("wan_specify_dns","1");
	nvram_set("wan_primary_dns",ip1);
	nvram_set("wan_secondary_dns",ip2);
// save dns info.
	nvram_set("opendns_dns1",ip1);
	nvram_set("opendns_dns2",ip2);

//
	nvram_set("html_response_return_page","dns_setting.asp");

// must add dns_relay
	nvram_set("dns_relay", "1");

	nvram_flag_set();
	nvram_commit();

// ???
    rc_action("wan");


	char url[128];

	if( UI ) {
		sprintf(url,"/opendns_okUI.asp?%s",opendns_token);
	}
	else {
		sprintf(url,"/opendns_ok.asp?%s",opendns_token);
	}

//	sprintf(url,"/reboot.asp");

	sprintf(str_response,redirecter_footer,url);
	
	wfputs(str_response, stream);


//            websWrite(wp,redirecter_footer, absolute_path("user_page.asp"));


}

#endif