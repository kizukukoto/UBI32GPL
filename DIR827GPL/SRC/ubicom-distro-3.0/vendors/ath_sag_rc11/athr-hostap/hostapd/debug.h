#ifndef __DEBUG_H__
#define __DEBUG_H__
#ifndef cprintf
#define cprintf(fmt, args...) do { \
        FILE *cfp = fopen("/dev/console", "w"); \
        if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
        } \
} while (0)
#endif
#endif
