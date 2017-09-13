#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>

#include "main.h"

extern char upload_source[4];

#define MAX_BUF_LEN		1024 * 1024
char id[16];
char tok[16];
char volid[MAX_NODE_NAME];
char path[MAX_PATH_LEN];
char file_name[MAX_NODE_NAME];

int get_boundary(char *boundary, int boundary_len){
	char *content_type = NULL;
	char *ptr = NULL;
	int len = 0;

	if (boundary == NULL){
		return -1;
	}
	
	content_type = getenv("CONTENT_TYPE");
	if ((ptr = strstr(content_type, "boundary=")) == NULL){
		return -1;
	}else{
		ptr = ptr + strlen("boundary=");
		len = strlen(ptr);

		if (len < boundary_len) {
			memset(boundary, 0, boundary_len);
			strcpy(boundary, ptr);
		}else{
			return -1;
		}
	}
	
	return 0;
}

char *get_start_position(char *buf, char *boundary){
	char *b_ptr = NULL;
	char *d_ptr = NULL;
	char *t_ptr = NULL;
	char *start_ptr = NULL;
	char *var_ptr = NULL;
	char var_flag;
	int i;
	
	b_ptr = strstr(buf, boundary);
 	
	memset(&id, 0, sizeof(id));
	memset(&tok, 0, sizeof(tok));
	memset(&volid, 0, sizeof(volid));
	memset(&path, 0, sizeof(path));
	memset(&file_name, 0, sizeof(file_name));	
	memset(&upload_source, 0, sizeof(upload_source));		// app : from APP, gui: from GUI
	
	while (b_ptr != NULL){ 		
		d_ptr = b_ptr + strlen(boundary) + 2;	 // 2 = \r\n	
				
		if (strncmp(d_ptr, "Content-Disposition", strlen("Content-Disposition")) == 0){
			d_ptr += 32;		
						
			if (strncmp(d_ptr, "name=", 5) == 0){
				d_ptr += 6;
				
				var_flag = 0;
				if (strncmp(d_ptr, "id", 2) == 0){
					var_flag = 1;
					var_ptr = id;
				}else if (strncmp(d_ptr, "tok", 3) == 0){
					var_flag = 1;
					var_ptr = tok;
				}else if (strncmp(d_ptr, "volid", 5) == 0){
					var_flag = 1;
					var_ptr = volid;
				}else if (strncmp(d_ptr, "path", 4) == 0){
					var_flag = 1;
					var_ptr = path;
				}else if (strncmp(d_ptr, "filename", 8) == 0){
					var_flag = 1;
					var_ptr = file_name;
				}else if (strncmp(d_ptr, "upload_source", 13) == 0){
					var_flag = 1;
					var_ptr = upload_source;
				}
				
				if (var_flag){
					while (1){
						if (*(d_ptr) == '\r' && *(d_ptr+1) == '\n'){		
							d_ptr += 2;
							if (*(d_ptr) == '\r' && *(d_ptr+1) == '\n'){	
								d_ptr += 2;
							}		 
							break;
						}
						d_ptr++;
					}
				
					i = 0;
					while(1){
						if (*(d_ptr) == '\r' && *(d_ptr+1) == '\n'){	
							break;
						} 
						var_ptr[i++] = *(d_ptr++);
					}																	
				}
			}			
					
			while (1){
				if (*(d_ptr) == '\r' && *(d_ptr+1) == '\n'){					
					break;
				}else if (*(d_ptr) == ';' && *(d_ptr+1) == ' '){
					d_ptr += 2;
					break;
				}
				
				d_ptr++;
			}
				
			if (strncmp(d_ptr, "filename=", 9) == 0){
				while (1){
					if (*(d_ptr) == '\r' && *(d_ptr+1) == '\n'){					
						t_ptr = d_ptr + 3;
						break;
					}
					d_ptr++;
				}
				break;
			}
		}
	
		b_ptr = strstr(d_ptr, boundary);			
	}
 	 	
	while (1){
		if (*(t_ptr) == '\r' && *(t_ptr+1) == '\n'){					
			start_ptr = t_ptr + 5;
			break;
		}
		t_ptr++;
	}
 	
	return start_ptr;
}

int do_save_file(unsigned long len){
	FILE *image = NULL;			
	char boundary[128] = {0}; 	
	char *buf;
	char *start_ptr;	
	char save_path[MAX_PATH_LEN];
	unsigned long start_pos = 0;	
	unsigned long count = 0;
	unsigned long total_size = 0;
	int result = 0;
	int find_boundary = 0;
	int i;	
	
	//cprintf("Into do_save_file,len=%d\n",len);
		
	if (get_boundary(boundary, sizeof(boundary)) != 0){
		//printf("unable to get boundary\r\n");			
		return 0;
	}
  //cprintf("Into do_save_file 1,boundary=%s\n",boundary);
	if((buf = malloc(MAX_BUF_LEN)) == NULL){		// allocate a set of memory for saving image
		printf("image malloc fail\r\n");			
		return 0;
	}  
  	//cprintf("Into do_save_file 2\n");
  
  	if (len < MAX_BUF_LEN){
		count = len;
	}else{
		count = MAX_BUF_LEN;
	}
		
 	for (i = 0; i < count; i++){
 		buf[i] = fgetc(stdin); 	
 	}
	//cprintf("Into do_save_file 2.1,buf len=%d\n",i);
 	start_ptr = get_start_position(buf, boundary);	// get the start position
 	start_pos = start_ptr - buf - 1; 
		//cprintf("Into do_save_file 3\n");	

#if 0	
	cprintf("echo id=%s >> /var/tmp/cgi", id);
	cprintf("echo tok=%s >> /var/tmp/cgi", tok);
	cprintf("echo volid=%s >> /var/tmp/cgi", volid);
	cprintf("echo path=%s >> /var/tmp/cgi", path);
	cprintf("echo file_name=%s >> /var/tmp/cgi", file_name);
#endif
	
	if (strlen(path) == 0 || strlen(file_name) == 0){
	        free(buf);
		return -1;
	}
		//cprintf("Into do_save_file 4\n");
	replace_special_char(&path);
	memset(&save_path, 0, sizeof(save_path));
	sprintf(save_path, "%s%s", path, file_name);
	
	if (result == 0){			
	 	if ((image = fopen(save_path, "w")) == NULL){
			free(buf);
			return 0;
		}
	
		/* write the first block of data */
		for (i = start_pos; i < count; i++){
				
			if (memcmp(&buf[i+4], boundary, strlen(boundary)) == 0){  	
				total_size = len;
				break;
			}
			
			fputc(buf[i], image);
			total_size++;
		}
		
		if (total_size == len){	// when we save all data in the first block
			result = 1;
		}else{
			total_size += start_pos;
			
			//_system("echo len=%d >> /www/webfile_access/usb_dev/usb_A1/cgi", len);
			//_system("echo count=%d >> /www/webfile_access/usb_dev/usb_A1/cgi", count);
			//_system("echo total_size=%d >> /www/webfile_access/usb_dev/usb_A1/cgi", total_size);
			//_system("echo -------------- >> /www/webfile_access/usb_dev/usb_A1/cgi");
			
			if ((count == total_size) && (total_size < len)){
			
				while(1){
					if ((total_size + MAX_BUF_LEN) > len){
						count = len - total_size;
					}else{
						count = MAX_BUF_LEN;					
					}
					
					total_size += count;
					
					memset(buf, 0, MAX_BUF_LEN);
					for (i = 0; i < count; i++){
						buf[i] = fgetc(stdin); 	
					}
					
					for (i = 0; i < count; i++){
	
						if (memcmp(&buf[i+4], boundary, strlen(boundary)) == 0){ 
							find_boundary = 1;
							break;
						}
	 		
						fputc(buf[i], image);
		}
	
					//_system("echo find_boundary=%d >> /www/webfile_access/usb_dev/usb_A1/cgi", find_boundary);
					//_system("echo total_size=%d >> /www/webfile_access/usb_dev/usb_A1/cgi", total_size);
					//system("free >> /www/webfile_access/usb_dev/usb_A1/cgi");
					//_system("echo -------------- >> /www/webfile_access/usb_dev/usb_A1/cgi");
					if (find_boundary || total_size == len){
						break;
					}
				}			
				result = 1;	// success	
			}else{
				result = 0;
			}
		}
 	 	
		fclose(image);		
	 			
	}
	
 	free(buf);
	
	if (result){
		sync();	// write data in system buffer into disk		
	}
	
	return result;	
}
