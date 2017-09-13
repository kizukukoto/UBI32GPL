#define head_interval 16
#define catch_time 1
#define log_length 1024
#define MAX_LOG_LENGTH 250

typedef struct log_dynamic_table_s
{
	char time[30];
	char *type;
	char message[log_length];
}log_dynamic_table_t;


extern log_dynamic_table_t log_dynamic_table[MAX_LOG_LENGTH];

void fill_log_into_structure(char *time, char *type, char *messages, int index_t);
int check_log_type(char *input,char *type, int index_t);
int update_log_table(int log_system_activity, int log_debug_information, int log_attacks, int log_dropped_packets, int log_notice);
