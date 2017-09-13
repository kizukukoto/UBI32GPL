#ifndef __OPTIONS_H__
#define __OPTIONS_H__

enum orayconfigoptions {
	ORAY_INVALID = 0,
	ORAY_USERNAME = 1,
	ORAY_PASSWD,
	ORAY_REGISTERNAME
};
/* readoptionsfile()
 * parse and store the option file values
 * returns: 0 success, -1 failure */
int
readoptionsfile(const char * fname);
#define MAX_OPTION_VALUE_LEN (80)
struct option
{
	enum orayconfigoptions id;
	char value[MAX_OPTION_VALUE_LEN];
};

extern struct option * ary_options;
int num_options;
#endif
