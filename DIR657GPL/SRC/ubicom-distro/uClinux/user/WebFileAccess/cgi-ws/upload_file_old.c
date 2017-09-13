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

int do_save_file(long len){
	FILE *image = NULL;			
	char boundary[128] = {0}; 	
	char *buf;
	char *start_ptr;	
	char save_path[MAX_PATH_LEN];
	long start_pos = 0;
	int i;
	int result = 0;
		
	if (get_boundary(boundary, sizeof(boundary)) != 0){
		//printf("unable to get boundary\r\n");			
		return 0;
	}
  
	if((buf = malloc(len)) == NULL){		// allocate a set of memory for saving image
		//printf("image malloc fail\r\n");			
		return 0;
	}  
  	
 	for (i = 0; i < len; i++){
 		buf[i] = fgetc(stdin); 	
 	}
	
 	start_ptr = get_start_position(buf, boundary);	// get the start position
 	start_pos = start_ptr - buf - 1; 
		
#if 0	
	_system("echo id=%s >> /var/tmp/cgi", id);
	_system("echo tok=%s >> /var/tmp/cgi", tok);
	_system("echo volid=%s >> /var/tmp/cgi", volid);
	_system("echo path=%s >> /var/tmp/cgi", path);
	_system("echo file_name=%s >> /var/tmp/cgi", file_name);
#endif
	
	if (strlen(path) == 0 || strlen(file_name) == 0){
		return -1;
	}
	
	replace_special_char(&path);
	memset(&save_path, 0, sizeof(save_path));
	sprintf(save_path, "%s%s", path, file_name);
	
	
	
	if (result == 0){			
	 	if ((image = fopen(save_path, "w")) == NULL){
	 		
			return 0;
		}
	
		for (i = start_pos; i < len; i++){
			
	 		if (memcmp(&buf[i+4], boundary, strlen(boundary)) == 0){  			
	 			break;
	 		}
	 		fputc(buf[i], image);
	 	}
 	 	
		fclose(image);		
	 	result = 1;	// success		
	}
	
 	free(buf);
	
	if (result){
		sync();	// write data in system buffer into disk		
	}
	
	return result;	
}
