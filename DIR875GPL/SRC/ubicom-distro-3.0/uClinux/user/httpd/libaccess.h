#ifndef __LIBACCESS__
#define __LIBACCESS__

typedef struct access_list{
	char path[1024];
	char access[4];
	struct access_list *next;
}ACCESS_LIST; 

extern int get_access(ACCESS_LIST **list);
extern int free_access(ACCESS_LIST *list);
extern int check_access(char *path, ACCESS_LIST *list, int type);

#endif
