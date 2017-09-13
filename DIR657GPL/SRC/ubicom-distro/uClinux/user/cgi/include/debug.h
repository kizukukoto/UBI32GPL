#ifndef __DEBUG_H__
#define __DEBUG_H__
#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt, ## args); \
                fclose(fp); \
        } \
} while (0)
#endif
