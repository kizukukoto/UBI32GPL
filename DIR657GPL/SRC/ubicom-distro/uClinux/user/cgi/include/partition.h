#ifndef __PARTITION_H__
#define __PARTITION_H__


/* cgi/lib/libpartition.c */
extern int *get_mtd_partition_index(const char *name);
extern int get_mtd_char_device(const char *name, char *buf);
extern int get_mtd_size(const char *name);
#endif
