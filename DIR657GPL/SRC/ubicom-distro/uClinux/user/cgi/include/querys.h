#ifndef __QUERY_STRING_H
#define __QUERY_STRING_H

#define get_env_value(tag)	getenv(tag)
int query_vars(char *tag, char *buf, int buf_len);
void put_querystring_env(void);
#endif
