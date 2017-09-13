#ifndef __PUBLIC_H
#define __PUBLIC_H

struct method_plugin {
	char file_path[128];
	int (*func)();	
};

struct nvram_ops{
	int   (*init)(void *);
	void  (*exit)(void *);
	char *(*get) (const char *);
	int   (*set) (const char *, const char *);
	int   (*unset)(const char *);
	int   (*commit)(void);
};

struct ssc_s{
	char *action;
	char *shell;
	char *back_html;
	void *(*fn)(struct ssc_s *);
	void *data;
};

extern int nvram_register(struct nvram_ops *);
extern int ssi_get_method_register(struct method_plugin *m);
extern int ssc_action_register(struct ssc_s *ss);
extern int start_cgi(int argc, char *argv[]);
extern int do_nvram_register();
extern int init_plugin();
#endif //__PUBLIC_H
