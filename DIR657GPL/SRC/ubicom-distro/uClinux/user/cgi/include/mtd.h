#ifndef __MTD_H__
#define __MTD_H__

extern int *get_mtd_partition_index(const char *name);
extern int get_mtd_char_device(const char *name, char *buf);
extern int get_mtd_size(const char *name);
extern int fd2mtd_erase_write(FILE *fd, int fd_len, const char *mtd_name);


#endif
