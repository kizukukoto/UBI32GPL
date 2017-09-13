#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>

//#include "middleware.h"
//#include "entry_util.h"
//#include "public_util.h"
#include "main.h"

#define ENCODING_LEN	25
char *encoding_code[] = {"20", "22", "23", "24", "25", "26", "2B", "2C", "2F", "3A", "3B", "3C", "3D", "3E", "3F", "40", "5B", "5C", "5D", "5E", "60", "7B", "7C", "7D", "7E"};
char encoding_char[] = {' ', '"', '#', '$', '%', '&', '+', ',', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '`', '{', '|', '}', '~'};

char *remote_addr = NULL;
char reading_file[20];
char current_id[33];
char current_tok[33];
char upload_source[4];
int total_count = 0;

/* everything except: ! ( ) * - . 0-9 A-Z _ a-z */
const char encoded_chars_uri[] = {
	/*      
        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
        */      
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  00 -  0F control chars */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  10 -  1F */
	1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1,  /*  20 -  2F space " # $ % & ' + , / */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,  /*  30 -  3F : ; < = > ? */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  40 -  4F @ */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,  /*  50 -  5F [ \ ] ^ */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  60 -  6F ` */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,  /*  70 -  7F { | } ~ DEL */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  80 -  8F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  90 -  9F */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  A0 -  AF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  B0 -  BF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  C0 -  CF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  D0 -  DF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  E0 -  EF */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /*  F0 -  FF */
};

void output_xml_header(){
	printf("Content-type: text/xml\r\n\r\n");	// need two \r\n to return page
	printf("<?xml version=\"1.0\"?>");
}

void send_reload_page(char *which_page){
	printf("Content-type: text/html\r\n\r\n");	// need two \r\n to return page
	printf("<script>location.href='%s'.htm;</script>", which_page);	
}

void set_redirect_page(char *which_page){	
	output_xml_header();
	printf("<root>");	
	printf("<redirect_page>%s</redirect_page>", which_page);	
	printf("</root>");	
}

void output_json_header(){
	printf("Content-type: application/json; charset=utf-8\r\n\r\n");	// need two \r\n to return page	
}

void replace_special_char(char *str){
	char replace_str[600];
	char temp_str[3];
	int i, j, k, len, find, count;
	
	len = strlen(str);
	memset(replace_str, 0, sizeof(replace_str));
	find = 0;
	count = 0;
	
	for (i = 0, j = 0; i < len; i++, j++){	
		memset(temp_str, 0, 3);
		count++;
		if (str[i] != '%'){
			replace_str[j] = str[i];
		}else{							
			sprintf(temp_str, "%c%c", str[i+1], str[i+2]);	
			for(k = 0; k < ENCODING_LEN; k++){
				if (strncmp(&str[i+1], encoding_code[k], 2) == 0){							
					replace_str[j] = encoding_char[k];				
					i += 2;
					find = 1;
					break;
				} 
			}			
		}
	}
	
	if (find){
		memset(str, 0, len);		
		memcpy(str, replace_str, count);
	}
}

int get_ws_entries(ware_entry *entries){
	char buf[1024];
	char read_byte = 0;
	int index = 0;
	int count = 0;
	int flag = 0;    
	int len = 0;
	int i;
	
	memset(&buf, 0, sizeof(buf));
	strcpy(buf, getenv("QUERY_STRING"));
	len = strlen(buf);
	
	for (i = 0; i < len; i++){
		read_byte = buf[i];		
			
  		if (read_byte == '='){	// when read value
  			flag = 1;  			
  			count = 0;
  			continue;
  		}else if (read_byte == '&'){	// when read next parameter
  			flag = 0;
  			count = 0;  			
  			index++;
  			continue;
  		}
  		
  		if (flag == 0){
  			entries[index].name[count++] = read_byte;
  		}else if (flag == 1){
  			entries[index].value[count++] = read_byte;
  		}  		  		
  	}
  	
  	index++;
  	
  	for (count = 0 ; count < index; count++){
  		replace_special_char(entries[count].value);
  	}
  	
  	return index;
}

char *query_encoded(const char *s, char *ret_path) {
	const char hex_chars[] = "0123456789ABCDEF";
	unsigned char *ds, *d;
	size_t d_len, ndx;
	const char *map = encoded_chars_uri;

	if (!s){
		return "undefined";
	}
	
	memset(ret_path, '\0', strlen(ret_path));
	for (ds = (unsigned char *)s, d = (unsigned char *)ret_path + strlen(ret_path), d_len = 0, ndx = 0; ndx < strlen(s); ds++, ndx++) {
		if (map[*ds]) {
			d[d_len++] = '%';
			d[d_len++] = hex_chars[((*ds) >> 4) & 0x0F];
			d[d_len++] = hex_chars[(*ds) & 0x0F];
		} else {
			d[d_len++] = *ds;
		}
	}
	d[d_len++] = '\0';

	return 0;
}

int get_webfile_state(){
	FILE *state_file = NULL;
	char buf[2];
	int result = 0;
	int byte_read = 0;
	
	state_file = fopen(WEBFILE_STATE_FILE, "r");
	if (state_file != NULL){
		memset(buf, 0, sizeof(buf));					
		byte_read = fread(buf, 1, sizeof(buf), state_file);
	
		if (byte_read > 0){
			result = atoi(buf);
		}
		
		fclose(state_file);
	}
	
	return result;
}

int file_ext(const char *which_file, const char *which_ext){
	char *end;	
	int file_len = strlen(which_file);
	int ext_len = strlen(which_ext);

	if(ext_len > file_len){	// when the length of the extension name is greater than the length of file name
		return 0;			// check fail
	}
	
 	end = which_file + file_len - ext_len;	// point to the extension name

	return (strcasecmp(end, which_ext) ? 0 : 1);
}

int is_audio(const char *file){
	return (file_ext(file, ".aac") || file_ext(file, ".mid") 
		|| file_ext(file, ".mp3") || file_ext(file, ".mpa") 
		|| file_ext(file, ".ra") || file_ext(file, ".wav")
		|| file_ext(file, ".wma") || file_ext(file, ".aac"));
		
	/* DLNA definitions
	return (file_ext(file, ".mp3") || file_ext(file, ".flac") 
		|| file_ext(file, ".wma") || file_ext(file, ".asf") 
		|| file_ext(file, ".fla") || file_ext(file, ".flc")
		|| file_ext(file, ".m4a") || file_ext(file, ".aac")
		|| file_ext(file, ".mp4") || file_ext(file, ".m4p") 
		|| file_ext(file, ".wav") || file_ext(file, ".ogg") 
		|| file_ext(file, ".pcm") || file_ext(file, ".3gp"));
	*/
}

int is_doc(const char *file){
	return (file_ext(file, ".doc") || file_ext(file, ".docx")
		|| file_ext(file, ".log") || file_ext(file, ".txt")
		|| file_ext(file, ".cvs") || file_ext(file, ".pdf")
		|| file_ext(file, ".pps") || file_ext(file, ".ppt") 
		|| file_ext(file, ".xml"));
}

int is_image(const char *file){
	return (file_ext(file, ".bmp") || file_ext(file, ".gif")
		|| file_ext(file, ".jpg") || file_ext(file, ".jpeg")
		|| file_ext(file, ".png") || file_ext(file, ".psd")
		|| file_ext(file, ".tif"));
}

int is_video(const char *file){
	return (file_ext(file, ".3g2") || file_ext(file, ".3gp")  
		|| file_ext(file, ".asf") || file_ext(file, ".asx")
		|| file_ext(file, ".avi") || file_ext(file, ".flv") 
		|| file_ext(file, ".mov") || file_ext(file, ".mp4") 
		|| file_ext(file, ".mpg") || file_ext(file, ".rm")  
		|| file_ext(file, ".swf") || file_ext(file, ".vob") 
		|| file_ext(file, ".wmv"));
		
	/*  DLNA definitions 
	return (file_ext(file, ".mpg") || file_ext(file, ".mpeg")  
		|| file_ext(file, ".avi") || file_ext(file, ".divx")
		|| file_ext(file, ".asf") || file_ext(file, ".wmv") 
		|| file_ext(file, ".mp4") || file_ext(file, ".m4v") 
		|| file_ext(file, ".mts") || file_ext(file, ".m2ts")  
		|| file_ext(file, ".m2t") || file_ext(file, ".mkv") 
		|| file_ext(file, ".vob") || file_ext(file, ".ts") 
		|| file_ext(file, ".flv") || file_ext(file, ".xvid") 
		|| file_ext(file, ".mov") || file_ext(file, ".3gp"));
		
	*/
}


int filter_audio(const struct dirent *d){
	return ((*d->d_name != '.') && ((d->d_type == DT_DIR) || (d->d_type == DT_LNK) 
		|| (d->d_type == DT_UNKNOWN) || ((d->d_type == DT_REG) && is_audio(d->d_name))));
}

int filter_video(const struct dirent *d){
	return ((*d->d_name != '.') && ((d->d_type == DT_DIR) || (d->d_type == DT_LNK) 
		|| (d->d_type == DT_UNKNOWN) || ((d->d_type == DT_REG) && is_video(d->d_name))));
}

int filter_images(const struct dirent *d){
	return ( (*d->d_name != '.') && ((d->d_type == DT_DIR) || (d->d_type == DT_LNK) 
		|| (d->d_type == DT_UNKNOWN) || ((d->d_type == DT_REG) && is_image(d->d_name))));
}

int filter_doc(const struct dirent *d){
	return ( (*d->d_name != '.') && ((d->d_type == DT_DIR) || (d->d_type == DT_LNK) 
		|| (d->d_type == DT_UNKNOWN) || ((d->d_type == DT_REG) && is_doc(d->d_name))));
	/*
	return ((*d->d_name != '.') && ((d->d_type == DT_DIR) || (d->d_type == DT_LNK) 
		|| (d->d_type == DT_UNKNOWN) || ((d->d_type == DT_REG) && !is_audio(d->d_name) 
		&& !is_video(d->d_name) && !is_image(d->d_name))));
	*/
}

int filter_all(const struct dirent *dir) {
	if (dir->d_type == 4 && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 && strstr(dir->d_name, "db_dir") == 0){
		return 1;
	}else if (dir->d_type != 4){
		return 1;	
	}else{
		return 0;
	}
}

int filter_folder(const struct dirent *dir) {
	if (dir->d_type == 4 && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 && strstr(dir->d_name, "db_dir") == 0){
		return 1;
	}else{
		return 0;
	}
}

int filter_file(const struct dirent *dir) {
	if (dir->d_type != 4){
		return 1;
	}else{
		return 0;
	}
}

void convert_file_unit(double st_size, int count, char *final_size){
	char *unit[] = {"Bytes", "KB", "MB", "GB", "TB", "PB", "EB"};

	while (st_size > 1024){
		st_size = st_size/1024;
		count++;
	}

	if (count == 0){
		sprintf(final_size, "%.0f %s", st_size, unit[count]);
	}else{
		sprintf(final_size, "%.2f %s", st_size, unit[count]);
	}	
}

void printf_folder_info(struct dirent *dir, char *path){
	struct stat buf;
	char url_path[MAX_PATH_LEN];
	char which_folder[MAX_PATH_LEN];
	char mtime[30];
	int len = 0;
		
	memset(&which_folder, 0, sizeof(which_folder));		
	sprintf(which_folder, "%s/%s", path, dir->d_name);		
	stat(which_folder, &buf);
	
	memset(mtime, 0, sizeof(mtime));
	strcpy(mtime, ctime(&buf.st_mtime));
	len = strlen(mtime);
	
	if (len > 6){
		mtime[len - 1] = '\0';
	}
	
	query_encoded(which_folder, url_path);
	printf(JSON_FILE_NODE, dir->d_name, url_path, 1, 0, 2, mtime, "folder", 0);
	
	//query_encoded(path, url_path);	// encode the path only, not to encode file name
	//memset(&which_folder, 0, sizeof(which_folder));
	//sprintf(which_folder, "%s/%s", url_path, dir->d_name);
	//printf(JSON_FILE_NODE, dir->d_name, which_folder, 1, 0, 2, mtime, "folder", 0);	
}

static int printf_file_info(struct dirent *file, char *path){
	struct stat buf;
	char *file_type;	
	char url_path[MAX_PATH_LEN];
	char which_file[MAX_PATH_LEN];		
	char file_format[256];
	char file_size[32];
	char mtime[30];
	int index = strlen(USB_MOUNT_DEV);
	int len = 0;

	if (file->d_type == 10) {
		printf(JSON_FILE_NODE, file->d_name, 1, 0, 1, ctime(&buf.st_mtime), "Symbolic link", 0);		
	} else{
		memset(&which_file, 0, sizeof(which_file));
		sprintf(which_file, "%s/%s", path, file->d_name);
		stat(which_file, &buf);
		
		memset(&file_format, 0, sizeof(file_format));
		if ((file_type = strrchr(file->d_name, '.')) != 0){
			sprintf(file_format, "%s", file_type + 1);
		}else{
			strcpy(file_format, "");
		}
		
		memset(&file_size, 0, sizeof(file_size));
		convert_file_unit(buf.st_size, 0, &file_size);
		
		memset(mtime, 0, sizeof(mtime));
		strcpy(mtime, ctime(&buf.st_mtime));		
		len = strlen(mtime);
	
		if (len > 6){
			mtime[len - 1] = '\0';
		}
		
		query_encoded(which_file, url_path);
		
		/*
		query_encoded(path, url_path);	// encode the path only, not to encode file name
		memset(&which_file, 0, sizeof(which_file));
		sprintf(which_file, "%s/%s", url_path, file->d_name);
		*/
	
		printf(JSON_FILE_NODE, file->d_name, url_path, 0, buf.st_size, 2, mtime, file_format, 0);		
	}

	return 0;
}

int printf_media_info(media_file *src_file){	
	char *file_type;	
	char url_path[MAX_PATH_LEN];
	char which_file[MAX_PATH_LEN];		
	char file_format[256];
	char file_size[32];
	char mtime[30];
	int index = strlen(USB_MOUNT_DEV);
	int len = 0;

	if (src_file->type == IS_LINK) {
		printf(JSON_FILE_NODE, src_file->name, 1, 0, 1, ctime(&src_file->mtime), "Symbolic link", 0);		
	}else{
		memset(&which_file, 0, sizeof(which_file));
		sprintf(which_file, "%s/%s", src_file->path, src_file->name);
		
		memset(&file_format, 0, sizeof(file_format));
		if ((file_type = strrchr(src_file->name, '.')) != 0){
			sprintf(file_format, "%s", file_type + 1);
		}else{
			strcpy(file_format, "");
		}
		
		memset(&file_size, 0, sizeof(file_size));
		convert_file_unit(src_file->size, 0, &file_size);
		
		memset(mtime, 0, sizeof(mtime));
		strcpy(mtime, ctime(&src_file->mtime));		
		len = strlen(mtime);
	
		if (len > 6){
			mtime[len - 1] = '\0';
		}
		
		query_encoded(which_file, url_path);
		/*
		query_encoded(src_file->path, url_path);	// encode the path only, not to encode file name
		memset(&which_file, 0, sizeof(which_file));
		sprintf(which_file, "%s/%s", url_path, src_file->name);
		*/
		if (src_file->type == IS_FOLDER) {
			printf(JSON_FILE_NODE, src_file->name, url_path, 1, 0, 2, mtime, "folder", 0);
			//printf(JSON_FILE_NODE, src_file->name, which_file, 1, 0, 2, mtime, "folder", 0);
		}else{
			printf(JSON_FILE_NODE, src_file->name, url_path, 0, src_file->size, 2, mtime, file_format, 0);		
			//printf(JSON_FILE_NODE, src_file->name, which_file, 0, src_file->size, 2, mtime, file_format, 0);		
		}
	}

	return 0;
}

void get_admin_node(root_node *my_node){
	struct dirent **dir_list;
	int total = 0;
	int i;
	
	if (get_webfile_state()){	
		strcpy(my_node->status, WS_STATUS1);
		strcpy(my_node->error_code, WS_ERRNO0);
		
		total = scandir(USB_MOUNT_DEV, &dir_list, filter_folder, alphasort);
		
		if (total > 0){
			
			for (i = 0; i < total; i++){
				if (strcmp("..", dir_list[i]->d_name) == 0) {				
					continue;
				}
				
				strcpy(my_node->node[i].volname, dir_list[i]->d_name);
				my_node->node[i].volid = (i+1);
				strcpy(my_node->node[i].status, "ok");
				strcpy(my_node->node[i].nodename, "");
				my_node->num++;
				//strcpy((path + i * MAX_PATH_LEN), dir_list[i]->d_name);				
			}
		}
	}else{
		strcpy(my_node->status, WS_STATUS2);
		strcpy(my_node->error_code, WS_ERRNO2);
	}
}

void get_node_by_user(root_node *my_node){
	
	char user_name[33];
	char user_pwd[33];	
	int i;
		
//		if (strcmp(current_id, user_name) == 0 && strcmp(current_tok, user_pwd) == 0){
			get_admin_node(my_node);
//		}
}

void check_sub_dir(char *src_path, char *nodename){	
	struct dirent **dir_list;	
	char real_path[MAX_PATH_LEN];	
	int count = 0;	
		
	memset(real_path, 0, sizeof(real_path));		
	sprintf(real_path, "%s/%s", DEFAULT_WEBFILE_PATH, src_path);
	
	count = scandir(real_path, &dir_list, filter_folder, alphasort);
		
	sprintf(nodename, "%d", count);	
}

void print_root_dir(){
	root_node my_node;	
	char url_path[MAX_PATH_LEN];
	char volname[MAX_NODE_NAME + 10];	// added usb_dev/
	int i;
		
	memset(&my_node, 0, sizeof(root_node));
	get_node_by_user(&my_node);		// get the path information by the current user name and password
		
	printf(JSON_ROOT_RESP, my_node.status, my_node.error_code);
	
	for (i = 0; i < my_node.num; i++){	
		memset(volname, 0, sizeof(volname));
		sprintf(volname, "%s/%s", ROOT_USB_DEV, my_node.node[i].volname);
		//sprintf(volname, "%s", my_node.node[i].volname);
		query_encoded(volname, url_path);
		
		//check_sub_dir(&volname, &my_node.node[i].nodename);
		
		//printf(JSON_ROOT_NODE, volname, my_node.node[i].volid, my_node.node[i].status, my_node.node[i].nodename);
		printf(JSON_ROOT_NODE, url_path, my_node.node[i].volid, my_node.node[i].status, my_node.node[i].nodename);				
	if (i < (my_node.num - 1)){
						printf(",");
					}
		printf("\n");
}	
		
	printf("]}");	// print out the end of response data
}

void print_dir_list(ware_entry *my_entry, int entry_num){	
	;
}

void save_media_info(FILE *my_media, struct dirent *file, char *path, char media_type){
	media_file which_media;
	struct stat buf;	
	char which_info[MAX_PATH_LEN];
		
	memset(&which_info, 0, sizeof(which_info));		
	sprintf(which_info, "%s/%s", path, file->d_name);		
	stat(which_info, &buf);
		
	memset(&which_media, 0, sizeof(which_media));
	
	if (file->d_type == DT_DIR){
		which_media.type = IS_FOLDER;
	}else if (file->d_type == DT_REG){ 
		which_media.type = IS_FILE;
	}else if (file->d_type == DT_LNK){ 
		which_media.type = IS_LINK;
	}
	
	which_media.media_type = media_type;
	strcpy(which_media.path, path);
	strcpy(which_media.name, file->d_name);
	which_media.mtime = buf.st_mtime;
	
	if (file->d_type == DT_REG){ 
		which_media.size = buf.st_size;
	}
	
	fwrite(&which_media, sizeof(media_file), 1, my_media); 
	
	total_count++;
}

void scan_directory(FILE *my_media, char *parent_path, char *child_path, char media_type){
	struct dirent **media_list;	
	char real_path[MAX_PATH_LEN];
	char sub_path[MAX_PATH_LEN];
	int total = 0;
	int i;
		
	memset(&real_path, 0, sizeof(real_path));
	if (strlen(child_path) > 0){
		sprintf(real_path, "%s/%s", parent_path, child_path);
	}else{
		strcpy(real_path, child_path);	
	}
		
	
	if (media_type == AUDIO_ONLY){
		total = scandir(real_path, &media_list, filter_audio, alphasort);
	}else if (media_type == VIDEO_ONLY){
		total = scandir(real_path, &media_list, filter_video, alphasort);
	}else if (media_type == IMAGES_ONLY){
		total = scandir(real_path, &media_list, filter_images, alphasort);
	}else if (media_type == DOC_ONLY){
		total = scandir(real_path, &media_list, filter_doc, alphasort);
	}
		
	for (i = 0; i < total; i++){
		if (strcmp("..", media_list[i]->d_name) == 0){				
			continue;
		}
		
		if (media_list[i]->d_type == DT_DIR){
			memset(&sub_path, 0, sizeof(sub_path));
			sprintf(sub_path, "%s/%s", child_path, media_list[i]->d_name);
			scan_directory(my_media, parent_path, sub_path, media_type);
		}else if (media_list[i]->d_type == DT_REG){
			save_media_info(my_media, media_list[i], child_path, media_type);
		}
	}	
}

char *start_scanning(char *path, char *filter, char *error){	
	FILE *my_media = NULL;
	struct dirent **media_list;	
	char real_path[MAX_PATH_LEN];
	char media_type;
	int count = 0;
	int i;
	
	total_count = 0;
		
	my_media = fopen(MEDIA_INFO_FILE, "w");
	
	if (my_media != NULL){
		if (strlen(filter) == 0){			
			memset(real_path, 0, sizeof(real_path));		
			sprintf(real_path, "%s/%s", DEFAULT_WEBFILE_PATH, path);
	    
			count = scandir(real_path, &media_list, filter_all, alphasort);
						
			for (i = 0; i < count; i++){
				if (strcmp("..", media_list[i]->d_name) == 0) {				
					continue;
				}
			
				save_media_info(my_media, media_list[i], path, ALL_FILES);
			}
		}else{
			if (strcmp(filter, "photo") == 0){	// filter by photo
				media_type = IMAGES_ONLY;
			}else if (strcmp(filter, "music") == 0){	// filter by music
				media_type = AUDIO_ONLY;
			}else if (strcmp(filter, "movie") == 0){	// filter by movie
				media_type = VIDEO_ONLY;				
			}else if (strcmp(filter, "document") == 0){	// filter by document
				media_type = DOC_ONLY;
			}
			
			if (strlen(path) == 0){
				strcpy(path,ROOT_USB_DEV);				
			}
			
			scan_directory(my_media, DEFAULT_WEBFILE_PATH, path, media_type);			
			
		}	
		
		fclose(my_media);
	}
	
	return WS_ERRNO0;
}

void print_file_list(ware_entry *my_entry, int entry_num){	
	FILE *my_media = NULL;
	media_file my_file;
	char volid[MAX_NODE_NAME];
	char path[MAX_PATH_LEN];	
	char sort[5];
	char filter[10];	// photo, music, movie, document
	char status[5];	// exist: ok, not exist: fail
	char *error = WS_ERRNO0;
	char volid_flag = 0;
	char path_flag = 0;
	int page = 0;
	int page_size = 0;
	int sort_order = WS_SORT_INCR;	
	int i;
		
			
	memset(&volid, 0, sizeof(volid));
	memset(&path, 0, sizeof(path));
	memset(&sort, 0, sizeof(sort));
	memset(&filter, 0, sizeof(filter));
	
	for (i = 0; i < entry_num; i++){
		if (strcmp(my_entry[i].name, "volid") == 0){
			strcpy(volid, my_entry[i].value);
			volid_flag = 1;
		}else if (strcmp(my_entry[i].name, "path") == 0){			
			strcpy(path, my_entry[i].value);
			path_flag = 1;
		}else if (strcmp(my_entry[i].name, "page") == 0){
			page = atoi(my_entry[i].value);			
		}else if (strcmp(my_entry[i].name, "pagesize") == 0){
			page_size = atoi(my_entry[i].value);			
		}else if (strcmp(my_entry[i].name, "sort") == 0){
			strcpy(sort, my_entry[i].value);
		}else if (strcmp(my_entry[i].name, "sortorder") == 0){
			if (strcmp(my_entry[i].value, "decrease") == 0){
				sort_order = WS_SORT_DECR;
			}
		}else if (strcmp(my_entry[i].name, "filter") == 0){
			strcpy(filter, my_entry[i].value);
		}
	}
	
	memset(&status, 0, sizeof(status));
		
	if (get_webfile_state() == 0){
		strcpy(status, WS_STATUS2);
		error = WS_ERRNO2;
	}else{
		if (volid_flag == 0 || path_flag == 0){	
			strcpy(status, WS_STATUS2);
			error = WS_ERRNO6;
		}else{	
			
			error = start_scanning(path, filter, error);
						
			if (strcmp(error, WS_ERRNO0) == 0){				
				strcpy(status, WS_STATUS1);
			}else{
				strcpy(status, WS_STATUS2);
			}
					
			/*
				int   access(const   char   *filename,   int   amode); 			
			*/
		}
	}
			
	printf(JSON_LIST_FILE_RESP, status, error, total_count);
	
	if (strcmp(error, WS_ERRNO0) == 0){
		my_media = fopen(MEDIA_INFO_FILE, "r");
		
		if (my_media != NULL){
			
			for (i = 0; i < total_count; i++){
				memset(&my_file, 0, sizeof(media_file));
		
				if (fread(&my_file, 1, sizeof(media_file), my_media) > 0){
										
					printf_media_info(&my_file);
				
					if (i < (total_count - 1)){
						printf(",");
					}
					
					printf("\n");
				}							
			}
		}		
	}
	
	printf("]}");	// print out the end of response data
	
}

void print_delete_file(ware_entry *my_entry, int entry_num){	
	char volid[MAX_NODE_NAME];
	char path[MAX_PATH_LEN];	
	char file_list[MAX_PATH_LEN];
	char file_name[MAX_NODE_NAME];
	char *name_ptr = NULL;
	char cmd[MAX_PATH_LEN];
	char status[5];	// exist: ok, not exist: fail	
	char *error = WS_ERRNO0;	
	char volid_flag = 0;
	char path_flag = 0;	
	int len;
	int i;
		
	memset(&volid, 0, sizeof(volid));
	memset(&path, 0, sizeof(path));
	memset(&file_list, 0, sizeof(file_list));
	
	for (i = 0; i < entry_num; i++){
		if (strcmp(my_entry[i].name, "volid") == 0){
			strcpy(volid, my_entry[i].value);
			volid_flag = 1;
		}else if (strcmp(my_entry[i].name, "path") == 0){			
			strcpy(path, my_entry[i].value);
			path_flag = 1;			
		}else if (strcmp(my_entry[i].name, "filenames") == 0){			
			memcpy(file_list, &my_entry[i].value[1], strlen(my_entry[i].value) - 1);
			len = strlen(file_list);
			file_list[len - 1] = '\0';	// remove the last ]
		}
	}
				
	memset(&status, 0, sizeof(status));
		
	if (get_webfile_state() == 0){
		strcpy(status, WS_STATUS2);
		error = WS_ERRNO2;
	}else{
		if (volid_flag == 0 || path_flag == 0){	
			strcpy(status, WS_STATUS2);
			error = WS_ERRNO6;
		}else{						
			name_ptr = strtok(file_list, ",");
			
			if (name_ptr != NULL){
				memset(&file_name, 0, sizeof(file_name));
				strcpy(file_name, name_ptr);
			
				while(1){
					//len = strlen(file_name);
					//file_name[len - 1] = '\0';
					
					memset(&cmd, 0, sizeof(cmd));
					//sprintf(cmd, "rm -rf %s/%s%s", DEFAULT_WEBFILE_PATH, path, &file_name[1]);
					sprintf(cmd, "rm -rf %s/%s%s", DEFAULT_WEBFILE_PATH, path, file_name);
					system(cmd);
					
					name_ptr = strtok(NULL, ",");
					
					if (name_ptr == NULL){
						break;
					}
					
					memset(&file_name, 0, sizeof(file_name));
					strcpy(file_name, name_ptr);									
				}
			}		
			strcpy(status, WS_STATUS1);
		}
	}
		
	printf(JSON_COMM_RESP, status, error);		
}

void print_upload_file(int result, int len){
	char status[5];	// exist: ok, not exist: fail	
	char *error = WS_ERRNO0;
	
	memset(&status, 0, sizeof(status));
	if (result == 1){
		strcpy(status, WS_STATUS1);						
	}else{
		strcpy(status, WS_STATUS2);
		
		if (result == 0){
			error = WS_ERRNO1;
		}else if (result == -1){	// missing parameters		
			error = WS_ERRNO6;
		}
	}
	
	output_json_header();
	printf(JSON_UPLOAD_FILE_RESP, status, error, len);
}

void print_add_dir(ware_entry *my_entry, int entry_num){	
	char volid[MAX_NODE_NAME];
	char path[MAX_PATH_LEN];	
	char dir_name[MAX_NODE_NAME];	
	char real_dir_name[MAX_PATH_LEN+MAX_NODE_NAME+2];
	char status[5];	// exist: ok, not exist: fail
	char *error = WS_ERRNO0;	
	char path_flag = 0;
	char name_flag = 0;	
	int err = 0;
	int i;
		
	memset(&volid, 0, sizeof(volid));
	memset(&path, 0, sizeof(path));
	memset(&dir_name, 0, sizeof(dir_name));

	for (i = 0; i < entry_num; i++){
		if (strcmp(my_entry[i].name, "volid") == 0){
			strcpy(volid, my_entry[i].value);			
		}else if (strcmp(my_entry[i].name, "path") == 0){			
			strcpy(path, my_entry[i].value);
			path_flag = 1;		
		}else if (strcmp(my_entry[i].name, "dirname") == 0){
			strcpy(dir_name, my_entry[i].value);
			name_flag = 1;
		}
	}
	
	memset(&status, 0, sizeof(status));
		
	if (get_webfile_state() == 0){
		strcpy(status, WS_STATUS2);
		error = WS_ERRNO2;
	}else{
		if (path_flag == 0 || name_flag == 0){	
			strcpy(status, WS_STATUS2);
			error = WS_ERRNO6;
		}else{	
			memset(&real_dir_name, 0, sizeof(real_dir_name));		
			sprintf(real_dir_name, "%s/%s", path, dir_name);
			err = mkdir(real_dir_name, 0777);
			
			if (err == 0){
				strcpy(status, WS_STATUS1);
			}else{
				strcpy(status, WS_STATUS2);
				switch(err){
					case EACCES:
						error = WS_ERRNO3;
						break;
					case EEXIST:	// the directory name is existing
						error = WS_ERRNO9;
						break;
					case ENOENT:	// the path for creating the directory name is not existed
						error = WS_ERRNO4;
						break;					
					case EROFS:		// the file system is read only
						error = WS_ERRNO10;
						break;
					case ENOSPC:	// disk full
						error = WS_ERRNO8;
						break;
					default:
						error = WS_ERRNO1;
						break;
				}
			}
		}
	}
		
	printf(JSON_COMM_RESP, status, error);	
}

int main(){	
	ware_entry entries[MAX_ENTRIES];	
	char *content_type = NULL;
	char *content_length = NULL;
	char *method = NULL;
	char ws_action[20];
	int len = 0;
	int count;
	int result = 0;	
	
	method = getenv("REQUEST_METHOD"); 	// get the incoming request method(POST/GET)
	remote_addr = getenv("REMOTE_ADDR"); 	// get the ip address of this incoming request. This ipaddr get from lighttpd/src/mod_cgi.c
	content_type = getenv ("CONTENT_TYPE");
	memset(&ws_action, 0, sizeof(ws_action));
	strcpy(ws_action, getenv("WS_ACTION"));
	len = atoi(getenv("CONTENT_LENGTH"));
				
	if (method == NULL){	// if we can not find the incoming request method, just ingore the request		
		return 0;
	}else if (strcmp(method, "GET") == 0){		
		memset(entries, 0, sizeof(entries));	// clear entries              		
		count = get_ws_entries(&entries);	// get input paramenters and values
		
		if (count > 0){				
			/*
			if (strcmp(entries[0].value, "login") != 0){
				if (update_login_time(remote_addr) == 0){
					set_redirect_page(LOGIN_HTM);
					return 0;
				}
			}
			*/
			if (strcmp(entries[0].value, "login") == 0){
				do_login(&entries[1], count - 1, remote_addr);
			}else if (strcmp(entries[0].value, "get_login_info") == 0){
				print_login_info(remote_addr);
			}else{
			
				memset(current_id, 0, sizeof(current_id));
				memset(current_tok, 0, sizeof(current_tok));
				
				if (count >= 2){
					if (strcasecmp(entries[0].name, "id") == 0){
						strcpy(current_id, entries[0].value);
					}
					
					if (strcasecmp(entries[1].name, "tok") == 0){
						strcpy(current_tok, entries[1].value);
					}
				}
		
				output_json_header();
	
				if (strcmp(method, "GET") == 0){ 	// if the incoming request method is GET, do nothing right now		
					if (strcmp(ws_action, "AddDir") == 0){
						print_add_dir(&entries[2], count - 2);
					}else if (strcmp(ws_action, "DelFile") == 0){
						print_delete_file(&entries[2], count - 2);					
					}else if (strcmp(ws_action, "ListDir") == 0){
						print_dir_list(&entries[2], count - 2);
					}else if (strcmp(ws_action, "ListFile") == 0){
						print_file_list(&entries[2], count - 2);
					}else if (strcmp(ws_action, "ListRoot") == 0){
						print_root_dir();
					}
					
					unlink(MEDIA_INFO_FILE);
				}
			}
		}
	}else{			
			/* when the incoming request is uploading a file */		
			if (content_type != NULL && (strstr(content_type, "multipart/form-data"))){				
				result = do_save_file(len);			
			if (strcmp(upload_source, "gui") == 0){	// upload files from GUI
				printf("Content-type: text/html\r\n\r\n");	// need two \r\n to return page					
				printf("<script>top.window.refresh_current_path();</script>");	
			}else{	// upload files from APP
					print_upload_file(result, len);
			}
		}
	}	
	return 0;
}

