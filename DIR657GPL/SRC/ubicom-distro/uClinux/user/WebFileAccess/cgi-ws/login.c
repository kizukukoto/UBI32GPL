#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>

#include "main.h"
#include "nvram.h"
//#include "middleware.h"
//#include "public_util.h"

extern char reading_file[20];

//#ifdef CONFIG_BASE64_LOGIN
/*
 * base64 encoder
 *
 * encode 3 8-bit binary bytes as 4 '6-bit' characters
 */                                                                               
char *b64_encode( unsigned char *src ,int src_len, unsigned char* space, int space_len){
	static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned char *out = space;
	unsigned char *in=src;
	int sub_len,len;  
	int out_len;

	out_len=0;   

	if (src_len < 1 ) return NULL;
	if (!src) return NULL;
	if (!space) return NULL;
	if (space_len < 1) return NULL;


	/* Required space is 4/3 source length  plus one for NULL terminator*/
	if ( space_len < ((1 +src_len/3) * 4 + 1) )return NULL;

	memset(space,0,space_len);

	for (len=0;len < src_len;in=in+3, len=len+3)
	{

		sub_len = ( ( len + 3  < src_len ) ? 3: src_len - len);

		/* This is a little inefficient on space but covers ALL the 
		   corner cases as far as length goes */
		switch(sub_len)
		{
			case 3:
				out[out_len++] = cb64[ in[0] >> 2 ]; 
				out[out_len++] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ] ;
				out[out_len++] = cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] ;
				out[out_len++] = cb64[ in[2] & 0x3f ];
				break;
			case 2:
				out[out_len++] = cb64[ in[0] >> 2 ];
				out[out_len++] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ] ;
				out[out_len++] = cb64[ ((in[1] & 0x0f) << 2) ];
				out[out_len++] = (unsigned char) '=';
				break;
			case 1:
				out[out_len++] = cb64[ in[0] >> 2 ];
				out[out_len++] = cb64[ ((in[0] & 0x03) << 4)  ] ;
				out[out_len++] = (unsigned char) '=';
				out[out_len++] = (unsigned char) '=';
				break;
			default:
				break;
				/* do nothing*/
		}
	}
	out[out_len]='\0'; 
	return out;
}

const static int b64_decode_table[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
 ** Return the actual number of bytes generated.  The decoded size will
 ** be at most 3/4 the size of the encoded, and may be smaller if there
 ** are padding characters (blanks, newlines).
 */
int b64_decode( const char* str, unsigned char* space, int size ){
	const char* cp;
	int space_idx, phase;
	int d, prev_d=0;
	unsigned char c;

	space_idx = 0;
	phase = 0;
	if(str==NULL)
		return space_idx;

	for ( cp = str; *cp != '\0'; ++cp )
	{
		d = b64_decode_table[(int)*cp];
		if ( d != -1 )
		{
			switch ( phase )
			{
				case 0:
					++phase;
					break;
				case 1:
					c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 2:
					c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 3:
					c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
					if ( space_idx < size )
						space[space_idx++] = c;
					phase = 0;
					break;
			}
			prev_d = d;
		}
	}
	return space_idx;
}
//#endif	// CONFIG_BASE64_LOGIN

void get_admin_path(storage_user *which_user){
	struct dirent **dir_list;
	int total = 0;
	int i;
	
	if (get_webfile_state()){	
	
		total = scandir(USB_MOUNT_DEV, &dir_list, filter_folder, alphasort);
		
		which_user->path_num = total;
		
		if (total > 0){
			
			for (i = 0; i < total; i++){
				if (strcmp("..", dir_list[i]->d_name) == 0) {				
					continue;
				}
				
				sprintf(which_user->access_path[i].path, "%s/%s", ROOT_USB_DEV, dir_list[i]->d_name);								
			}
		}
	}else{
		which_user->path_num = 0;
	}
}

int add_storage_user(char *addr, storage_user *which_user){
	FILE *user_file = NULL;
	time_t current;
	char file_name[50];	
	int success = 0;
		
	
		
	memset(file_name, 0, sizeof(file_name));
	sprintf(file_name, "%s/%s", WEBFILE_USER_PATH, addr);
			
	user_file = fopen(file_name, "w");
	if (user_file != NULL){
		time(&current);		
		
		which_user->login_time = current;
			
		fwrite(which_user, sizeof(storage_user), 1, user_file); 
		fclose(user_file);
		
		success = 1;		
	}
		
	return success;						
}

//int find_storage_user(midware_obj *my_obj, midware_entry *login_info, char *remote_addr){
//	storage_user which_user;
//	int result = MIDWARE_ERR;		// assume update or insert login information failed
//	int find = 0;		
//	int success = 0;
//	
//	memset(&which_user, 0, sizeof(storage_user));
//	strcpy(which_user.user_name, login_info->entry[0][0].value);
//	strcpy(which_user.user_pwd, login_info->entry[0][1].value);
//	
//	result = my_obj->get_entry(reading_file, "admin_user", login_info, my_obj->entry_list);			
//		
//	if (result == MIDWARE_OK){			
//		if (my_obj->entry_list->col_num > 0){	// when we can find the login name and password in database
//			find = 1;
//			which_user.level = 0;	// admin
//			get_admin_path(&which_user);
//		}
//	}
//		
//	switch(find){
//		case 1:		// admin 
//			success = add_storage_user(remote_addr, &which_user);
//			break;
//			
//	}
//	
//	return success;
//}

int find_storage_user(ware_entry *my_entry, char *remote_addr){
	storage_user which_user;
	//int result = MIDWARE_ERR;		// assume update or insert login information failed
	int find = 0;		
	int success = 0;
	//get nvram value
	char *user_name = nvram_safe_get("admin_username");
	//char *remote_ipaddr = nvram_safe_get("remote_ipaddr_key");
	char *password = nvram_safe_get("admin_password");
	 
	memset(&which_user, 0, sizeof(storage_user));
	strcpy(which_user.user_name, my_entry[0].value);
	strcpy(which_user.user_pwd, my_entry[1].value);
		
	if(strcmp(which_user.user_name,user_name) == 0 && strcmp(which_user.user_pwd,password) == 0)
	{
			find = 1;
			which_user.level = 0;	// admin
			get_admin_path(&which_user);
			
	}
			
	switch(find){
		case 1:		// admin 
			success = add_storage_user(remote_addr, &which_user);
			break;
			
	}
	
	return success;
}

void do_login(ware_entry *my_entry, int entry_num, char *remote_addr){	
	//midware_obj my_obj;
	//midware_entry *login_info = NULL;
	char *redirect_page;
	char login_name[30];
	char login_pwd[30];	
	int len = 0;
	int find = 0;	
	
	//create_midware_obj(&my_obj);
	//my_obj.entry_list = malloc(sizeof(midware_entry));
	//if (my_obj.entry_list != NULL){
	
		memset(login_name, 0, sizeof(login_name));
		memset(login_pwd, 0, sizeof(login_pwd));
		
//#ifdef CONFIG_BASE64_LOGIN
		len = b64_decode(my_entry[0].value, login_name, sizeof(login_name));
		login_name[len] = '\0';
		len = b64_decode(my_entry[1].value, login_pwd, sizeof(login_pwd));
		login_pwd[len] = '\0';	

		memset(&my_entry[0].value, 0, ENTRY_VALUE_LEN+1);
		strcpy(my_entry[0].value, login_name);	

		memset(&my_entry[1].value, 0, ENTRY_VALUE_LEN+1);	
		strcpy(my_entry[1].value, login_pwd);
		
		cprintf("login_name=%s  login_passwd=%s  remote_addr=%s\n",login_name,login_pwd,remote_addr);
//#endif	// CONFIG_BASE64_LOGIN	
				
	//	login_info = malloc(sizeof(midware_entry));
		
	//	if (login_info != NULL){			
	//		login_info->col_num = 2;
	//		memcpy(&login_info->entry[0], my_entry, sizeof(ware_entry) * 2);
						
			//find = find_storage_user(&my_obj, login_info, remote_addr);
			find = find_storage_user(my_entry, remote_addr);
			//find = 1;
			
			if (find){
				redirect_page = CATEGORY_HTM;
				//redirect_page = FOLDER_HTM;
			}
			//add by Allen
			else
				redirect_page = LOGIN_HTM;
				
				cprintf("redirect_page=%s\n",redirect_page);
			//end by Allen
			/*
			result = my_obj->get_entry(reading_file, "admin_user", login_info, my_obj->entry_list);			

			if (result == MIDWARE_OK){
				if (my_obj->entry_list->col_num > 0){	// when we can find the login name and password in database
												
					if (add_login_user(remote_addr, level)){			
						redirect_page = DEFAULT_HTM;	
					}else{
						add_msg(my_obj, strings[LOGIN_FAILED], "login");	// add login failed message to the message table
						redirect_page = BACK_HTM;
					}
				}else{
					add_msg(my_obj, strings[INVALID_PWD], "login");	// add login failed message to the message table
					redirect_page = BACK_HTM;
				}
			}
			*/
			
			//free(login_info);
		//}

		//free(my_obj.entry_list);
		
		set_redirect_page(redirect_page);	
	//}
}

void delete_existed_user(char *addr){
	char file_name[50];
	
	memset(file_name, 0, sizeof(file_name));
	sprintf(file_name, "%s/%s", WEBFILE_USER_PATH, addr);
	unlink(file_name);
}

int check_expire_time(unsigned long current_time, unsigned long pre_time){
	int no_expire = 0;
		
	if ((current_time - pre_time) < LOGIN_IDLE_TIMEOUT){
		no_expire = 1;
	}
		
	return no_expire;
}

int update_login_time(char *remote_addr){	
	FILE *user_file = NULL;	
	time_t current;
	storage_user which_user;
	char file_name[50];			
	int success = 0;

	memset(&which_user, 0, sizeof(storage_user));
	
	memset(file_name, 0, sizeof(file_name));
	sprintf(file_name, "%s/%s", WEBFILE_USER_PATH, remote_addr);
			
	user_file = fopen(file_name, "r");
	if (user_file != NULL){
		fread(&which_user, sizeof(storage_user), 1, user_file);		
		fclose(user_file);
	}

	if (which_user.login_time > 0){
		time(&current);	
		
		if (check_expire_time(current, which_user.login_time)){	//  when the current ip address is not expired 
			success = add_storage_user(remote_addr, &which_user);						
		}
	}
	
	if (success == 0){	// remove the ip address which is expired
		unlink(file_name);
	}

	return success;						
}

void print_login_info(char *remote_addr){	
	FILE *user_file = NULL;	
	time_t current;
	storage_user which_user;
	char file_name[50];
	int i;

	memset(&which_user, 0, sizeof(storage_user));
	
	memset(file_name, 0, sizeof(file_name));
	sprintf(file_name, "%s/%s", WEBFILE_USER_PATH, remote_addr);
	
	user_file = fopen(file_name, "r");
	if (user_file != NULL){
		fread(&which_user, sizeof(storage_user), 1, user_file);

		output_xml_header();
		printf("<root>");	
		printf("<user_name>%s</user_name>", which_user.user_name);	
		printf("<user_pwd>%s</user_pwd>", which_user.user_pwd);	
		printf("<volid>usb_A1</volid>");
		printf("</root>");
	
		fclose(user_file);
	}
}

//void print_login_info(char *remote_addr){	
//	FILE *user_file = NULL;	
//	time_t current;
//	storage_user which_user;
//	char file_name[50];
//	int i;
//
//	memset(&which_user, 0, sizeof(storage_user));
//	
//	memset(file_name, 0, sizeof(file_name));
//	sprintf(file_name, "%s/%s", WEBFILE_USER_PATH, remote_addr);
//	
//	//user_file = fopen(file_name, "r");
//	//if (user_file != NULL){
//	//	fread(&which_user, sizeof(storage_user), 1, user_file);
//
//		output_xml_header();
//		printf("<root>");	
//		printf("<user_name>%s</user_name>", "admin");	
//		printf("<user_pwd>%s</user_pwd>", "");	
//		printf("<volid>usb_A1</volid>");
//		printf("</root>");
//	
//		//fclose(user_file);
//	//}
//}
