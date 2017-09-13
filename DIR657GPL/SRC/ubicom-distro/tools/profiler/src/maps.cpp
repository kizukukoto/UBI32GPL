
#include "maps.h"
#include <ctype.h>
#include <stdlib.h>

unsigned int xtoi(char *p);

maps::maps(void)
{
	init();
}

void maps::init()
{
	map_count = 0;
	line[0] = 0;
	end = line;
	data_size = 0;
	stack_size = 0;
	other_size = 0;
	pid = 0;
	command[0] = 0;
	unknown_gprof = 0.0;
}

maps::~maps(void)
{
}

// find the library name
char *maps::get_name(int i) 
{
	char *name = linux_map[i].name;
	char *p = name;
	while (*p) {
		if (*p == '/')
			name = p + 1;
		p++;
	}
	return name;
}

bool maps::parse_map(char *in)
{
	while (*in) {
		// copy a line
		while (*in && *in != '\n') {
			*end++ = *in++;
		}

		// partial line, return and wait for more data
		if (*in == 0) {
			return true;		
		}

		*end++ = *in++;
		*end = 0;
		end = line;
		char *p = line;

		// parse one input line
		// skip white space
		while (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')  {
			p++;
		}
		if (strstr(p, "done") == p)
			return false;
		linux_map[map_count].ocm_start = 0;
		linux_map[map_count].gprof = 0.0;
		if (!isdigit(*p)) { 
			// kernel module map
			char *n = linux_map[map_count].name;
			int count = 0;
			while (*p != ' ' && *p != 0) {	// name
				if (count < MAX_NAME)
					*n++ = *p;
				p++;
				count++;
			}
			*n = 0;
			while (*p == ' ' && *p != 0)
				p++;
			int size = atoi(p);
			while (*p != ' ' && *p != 0)	// size
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			while (*p != ' ' && *p != 0)	// -
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			while (*p != ' ' && *p != 0)	// -
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			while (*p != ' ' && *p != 0)	// live
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			linux_map[map_count].start = xtoi(p + 2);
			linux_map[map_count].end = linux_map[map_count].start + size;
			linux_map[map_count].offset = 0;
			while (*p != ' ' && *p != 0)	// address
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			while (*p != '(' && *p != 0)	// OCM
				p++;
			if (*p != 0) {
				p++;
				size = atoi(p);
			}
			while (*p != ' ' && *p != 0)	// size
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			while (*p != ' ' && *p != 0)	// bytes
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			while (*p != ' ' && *p != 0)	// @
				p++;
			while (*p == ' ' && *p != 0)
				p++;
			linux_map[map_count].ocm_start = xtoi(p + 2);
			linux_map[map_count].ocm_end = linux_map[map_count].ocm_start + size;
			if (*p != 0)
				map_count++;
			continue;
		}
		// regular map
		linux_map[map_count].start = xtoi(p);
		while (isxdigit(*p)) {
			p++;
		}
		if (*p != '-')
			continue;
		p++;
		if (!isxdigit(*p)) 
			break;
		linux_map[map_count].end = xtoi(p);
		while (isxdigit(*p)) {
			p++;
		}
		if (*p != ' ')
			continue;
		p += 3;
		if (*p == 'x' && *(p + 1) == 'p') {	// stack
			stack_size += linux_map[map_count].end - linux_map[map_count].start;
			continue;
		} 
		if (*p == '-' && *(p + 1) == 'p') {	// data
			data_size += linux_map[map_count].end - linux_map[map_count].start;
			continue;
		}
		if (*p != 'x' || *(p + 1) != 's') {
			other_size += linux_map[map_count].end - linux_map[map_count].start;
			continue;
		}
		p += 3;
		if (!isxdigit(*p)) 
			continue;
		linux_map[map_count].offset = xtoi(p);
		while (isxdigit(*p)) {
			p++;
		}
		while (*p && *p != '/') {
			p++;
		}
		char *n = linux_map[map_count].name;
		int count = 0;
		while (*p && *p != '\r' && *p != '\n' && count < MAX_NAME) {
			*n++ = *p++;
			count++;
		}
		*n = 0;
		if (map_count < MAX_MAPS - 1) {
			map_count++;
		}
	}
	return true;
}
