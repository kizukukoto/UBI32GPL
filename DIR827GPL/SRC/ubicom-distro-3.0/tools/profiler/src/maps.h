#pragma once
#include "string.h"

/*
 * record the linux memory map for all executable segments 
 */

#define MAX_MAPS 500
#define MAX_NAME 256
#define MAX_LINE 500

struct linux_memory {
	unsigned int start;
	unsigned int end;
	unsigned int offset;
	unsigned int ocm_start;
	unsigned int ocm_end;
	double gprof;	// hit count for this segment
	char name[MAX_NAME];
};

class maps
{
public:
	maps(void);
	~maps(void);
	void init();
	bool parse_map(char *);

	char *get_name(int i);	// get the name portion of the path

	int map_count;
	linux_memory linux_map[MAX_MAPS];
	char line[MAX_LINE];	// hold one line of input
	char *end;		// end of line
	unsigned int data_size;
	unsigned int stack_size;
	unsigned int other_size;
	double unknown_gprof;
	int pid;
	char command[MAX_LINE];
};
