#ifndef __UPDATE_LOG_TABLE_H
#define __UPDATE_LOG_TABLE_H

#define log_length 1024
#define MAX_LOG_LENGTH 250
#define LOG_INFO 16


struct __log_struct {
        const char *param;
        const char *desc;
        int (*fn)(int, char *[]);
};

typedef struct log_dynamic_table_s
{
        char time[30];
        char *type;
        char message[1024];
}log_dynamic_table_t;


extern log_dynamic_table_t log_dynamic_table[MAX_LOG_LENGTH];


extern int check_log_type(char *input,char *type, int index_t);
extern int update_log_table(int log_system_activity, int log_debug_information, int log_attacks, int log_dropped_packets, int log_notice);

#endif //__UPDATE_LOG_TABLE_H
