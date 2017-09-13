#ifndef __CONVERT_H
#define __CONVERT_H
struct zone_convert_struct{
	char *dest;
	char *src;
	void *priv;
	int flags;
	int (*cf)(struct zone_convert_struct *zc);
};
enum {
	ZF_COPY_PRIV = 1,
	ZF_NO_NULL = ZF_COPY_PRIV << 1,
};
extern int convert_env(struct zone_convert_struct *z);
#endif
