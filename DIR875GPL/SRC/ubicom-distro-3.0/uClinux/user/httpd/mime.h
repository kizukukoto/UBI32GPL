#ifndef __MIME_H
#define __MIME_H

struct mime_desc{
	char name[64];
	char filename[256];
	char mime_type[64];
	int size;
	FILE *fd;
	void (*fn)(FILE *, const char *, struct mime_desc *);
};
extern int register_mime_handler(int (*)(struct mime_desc *));
extern int http_post_upload(FILE *);
#endif //__MIME_H
