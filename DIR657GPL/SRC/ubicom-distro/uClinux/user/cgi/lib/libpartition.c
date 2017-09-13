#include <stdio.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ssi.h"
#include "libdb.h"
#include "partition.h"

int *get_mtd_partition_index(const char *name)
{
        FILE *fd;
        char buf[128], p;
        int i;

        if (!name)
                goto out;
        if ((fd  = fopen("/proc/mtd", "r")) == NULL)
                goto out;
        bzero(buf, sizeof(buf));

        while (fgets(buf, sizeof(buf) - 1, fd)) {
                if (sscanf(buf, "mtd%d:", &i) && strstr(buf, name))
                        return i;       
                bzero(buf, sizeof(buf));
        }
out:
        return -1;
}

int get_mtd_char_device(const char *name, char *buf)
{
        int i;
        
        if ((i = get_mtd_partition_index(name)) != -1) {
                sprintf(buf, "/dev/mtd%d", i);
        }
        return i;
}

int get_mtd_size(const char *name)
{
        FILE *fd;
        char buf[128], p;
        int i, len = -1;

        if (!name)
                goto out;
        if ((fd  = fopen("/proc/mtd", "r")) == NULL)
                goto out;
        bzero(buf, sizeof(buf));

        while (fgets(buf, sizeof(buf) - 1, fd)) {
                if (sscanf(buf, "mtd%d: %X ", &i, &len) && strstr(buf, name)) {
                        return len;
                }
                bzero(buf, sizeof(buf));
        }
out:
        return -1;
}


