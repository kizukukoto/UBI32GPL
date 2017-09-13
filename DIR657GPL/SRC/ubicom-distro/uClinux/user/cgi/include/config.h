#ifndef nvram_h_
#define nvram_h_
typedef struct _nvram_table {
        char name[40];
        char value[256];
} nvram_table;
#endif
