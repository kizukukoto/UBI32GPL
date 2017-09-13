#ifndef __MIME_H
#define __MIME_H

struct mime_opt {
	const char *name;
	const char *desc;
};

struct mime_desc{
	char name[64];
	char filename[256];
	char mime_type[64];
	int size;
	char tpath[128];
	FILE *fd;
	struct mime_opt *opt;
	void (*fn)(FILE *, const char *, struct mime_desc *);
};
extern int register_mime_handler(int (*)(struct mime_desc *));
extern int http_post_upload(FILE *);
extern int http_post_upload_ext(FILE *, struct mime_opt *);
#endif //__MIME_H
