
typedef unsigned int u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "profile.h"

// for ntohl
#if defined(WIN32)
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

memblock::memblock(unsigned int start, unsigned int block_size, unsigned int *data_in, char *name, int in_type) 
{
	next = 0; 
	type = in_type;
	size = 4 * (block_size / 4);	// all data is per instruction, not per byte - ignore 1-3 bytes at end
	start_size = size;
	init_start_address = start;
	start_address = start;
	pmem = data_in;		// block size array of bytes
	gprof = (double *)malloc(size / 4 * sizeof(double));
	for (unsigned int i = 0; i < PROFILE_MAX_THREADS; ++i) {
		thprof[i] = (double *)malloc(size / 4 * sizeof(double));
		if (thprof[0] == 0) {
			fatal_error("thprof allocation error");
			size = 0;
		}
	}
	iblocked = (double *)malloc(size / 4 * sizeof(double));
	dblocked = (double *)malloc(size / 4 * sizeof(double));
	hazards = (double *)malloc(size / 4 * sizeof(double));
	haztype = (int *)malloc(size / 4 * sizeof(int));
	hazclocks = (double *)malloc(size / 4 * sizeof(double));
	touched = (short *)malloc(size / 4 * sizeof(short));
	inst_type = (int *)malloc(size / 4 * sizeof(int));
	target = (bool *)malloc(size / 4 * sizeof(bool));
	mispredicts = (double *)malloc(size / 4 * sizeof(double));
	taken = (double *)malloc(size / 4 * sizeof(double));
	targetcount = (double *)malloc(size / 4 * sizeof(double));
	targethazard = (double *)malloc(size / 4 * sizeof(double));
	seg_name = (char *)malloc(strlen(name) + 1);

	// set up default symbol table
	strings = (char *)malloc(strlen(name) + 1);
	strcpy(strings, name);
	symbol_table =(struct symbol *) malloc(sizeof(struct symbol));
	symbol_table->st_name = strings;
	symbol_table->st_value = init_start_address;
	symbol_table->st_size = block_size;
	symbol_table->st_info = INFO_FUNCTION;
	symbol_table->st_other = 0;
	symbol_count = 1;

	if (gprof == 0 ||
		iblocked == 0 ||
		dblocked == 0 ||
		taken == 0 ||
		inst_type == 0 ||
		hazards == 0 ||
		haztype == 0 ||
		hazclocks == 0 ||
		touched == 0 ||
		target == 0 ||
		mispredicts == 0 ||
		targetcount == 0 ||
		targethazard == 0 ||
		seg_name == 0) {
		size = 0;
		fatal_error("Memblock allocation failed");
	}
	strcpy(seg_name, name);

	int sz = (size / 4) * sizeof(double);
	memset(gprof, 0, sz);
	memset(iblocked, 0, sz);
	memset(dblocked, 0, sz);
	memset(hazards, 0, sz);
	memset(hazclocks, 0, sz);
	memset(mispredicts, 0, sz);
	memset(targetcount, 0, sz);
	memset(targethazard, 0, sz);
	for (int j = 0; j < PROFILE_MAX_THREADS; ++j) {
		memset(thprof[j], 0, sz);
	}
	memset(taken, 0, sz);
	sz = (size / 4) * sizeof(short);
	memset(touched, 0, sz);
	sz = (size / 4) * sizeof(bool);
	memset(target, 0, sz);
	sz = (size / 4) * sizeof(int);
	memset(inst_type, 0, sz);
	for (unsigned int i = 0; i < size/4; ++i) {
		haztype[i] = HAZ_NONE;
	}
	init_pmem_hazards();
}

void memblock::init_pmem_hazards(void)
{
	for (unsigned int i = 0; i < size / 4; i++) {
		unsigned int inst = pmem[i];
		unsigned int addr = start_address + i * 4;
		targetcount[i] += 1.0;	// safe initial value
		if ((inst >> 27) == INST_JMP) {
			int offset = offset21(pmem[i]);
			int targ = (int)(addr - start_address) / 4 + offset / 4;
			if (targ < size / 4 && targ >= 0) {	// don't support calls to another segment
				target[targ] = true;		// mark jump targets
				if (pmem[i] & 0x400000) {	// predicted taken
					targethazard[targ] += 3.0;
				}
			}
		} else if ((inst >> 27) == INST_CALL) {
			int targ = (int)(addr - start_address) / 4 + offset24(pmem[i]) / 4;
			if (targ < size / 4 && targ >= 0) {	// don't support calls to another segment
				targethazard[targ] += 3.0;
			}
			if (i + 1 < size / 4) {	// don't support calls to another segment
				targethazard[i + 1] += 4.0;
			}
		}
	}
}


unsigned int xtoi(char *p);

// find the library name
char *map_segment::get_name(void) 
{
	char *name = path;
	char *p = name;
	while (*p) {
		if (*p == '/')
			name = p + 1;
		p++;
	}
	return name;
}


memory_map::memory_map(int c_pid, char *com, int time)
{
	next = NULL;
	first_seg = NULL;
	pid = c_pid;
	strncpy(command, com, MAX_NAME);
	command[MAX_NAME - 1] = 0;
	gprof = 0.0;
	data_size = 0;
	stack_size = 0;
	other_size = 0;
	code_size = 0;
	mapped = false;
	last_sample_time = time;
}

memory_map::~memory_map()
{
	if (next) {
		delete next;
		next = 0;
	}
	if (first_seg) {
		delete first_seg;
		first_seg = 0;
	}
}

memory::memory()
{
	first_memblock = 0;
	first_map = 0;
	init();
}

memory::~memory()
{
	delete first_map;
	first_map = 0;
	delete first_memblock;
	first_memblock = 0;
	dummy_memblock = 0;
}

void memory::init()
{
	if (first_map) {
		delete first_map;
		first_map = 0;
	}
	if (first_memblock) {
		delete first_memblock;
		first_memblock = 0; 
	}
	num_maps = 0;
	dummy_memblock = add_dummy_memblock(0, MEM_UNKNOWN_LABEL);
	touched_error = 0; 
	target_error = false;
	uint_error = 0;
	int_error = 0;
	double_error = 0.0;
	haztype_error = 0;
	line[0] = 0;
	end = line;
}

void memory::clean_maps(int time, double total_rate)
{
	int count = 0;
	memory_map *pmap;
	pmap = first_map;
	if (!pmap)
		return;
	while (pmap->next) {
		count++;
		if (count <= MIN_MAPS || 
			pmap->next->last_sample_time + MAX_AGE >= time || 
			pmap->next->gprof >= total_rate * MIN_RATE) {
			pmap = pmap->next;
			continue;
		}
		memory_map *tmp = pmap->next;
		pmap->next = tmp->next;
		tmp->next = 0;
		delete tmp;
	}
}

memory_map *memory::add_memory_map(int pid, char *command, int time, double total_rate) {
	for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
		if (pmap->pid == pid) {
			return NULL;
		}
	}
	memory_map *pmap = new memory_map(pid, command, time);
	pmap->next = first_map;
	first_map = pmap;
	num_maps++;

	// every map needs a dummy segment
	char name[MAX_NAME];
	strcpy(name, command);
	strcat(name, ":");
	strcat(name, "Unknown Functions");
	map_segment *ms = new map_segment(0, 8, 0, 0, "Unknown applications");
	ms->seg = add_dummy_memblock(0, name);
	ms->next = pmap->first_seg;
	pmap->first_seg = ms;

	// clear out old maps
	if (num_maps >= MAX_MAPS) {
		clean_maps(time, total_rate);
	}

	return pmap;
}

memory_map *memory::get_memory_map(int pid) {
	for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
		if (pmap->pid == pid) {
			return pmap;
		}
	}
	return NULL;
}


// called whenever any memory map changes, so may be multiple map updates for the same app or library
void memory::update_memory_map(memory_map *mm)
{
	char *n, name[MAX_NAME], dummy_name[MAX_NAME];

	for (map_segment *ms = mm->first_seg; ms; ms = ms->next) {
		if (ms->seg)
			continue;
		n = ms->get_name();
		if (!*n) {
			continue;
		}
		strcpy(name, n);
		while (*n) {		// find the separator
			if (*n == ':') {
				break;
			}
			n++;
		}
		char buf[500];
		sprintf(buf, "%s.ko", name);
		if (strstr(name, "busybox:")) {
			sprintf(name, "busybox:%s", n);
		} 
		memblock *m;
		strcpy(dummy_name, name);
		strcat(dummy_name, ":");
		strcat(dummy_name, MEM_UNKNOWN_LABEL);
		for (m = get_first_memblock(); m != NULL; m = m->next) {
			char segname[500];
			if (!strstr(m->seg_name, ".text") && !strstr(m->seg_name, ".plt")) {
				continue;
			}
			strcpy(segname, m->seg_name);
			n = segname;
			while (*n) {
				if (*n == ':') {
					*n = 0;
					break;
				}
				n++;
			}
			if (strcmp(m->seg_name, name) == 0 || strcmp(segname, name) == 0 || strcmp(segname, buf) == 0 || strcmp(m->seg_name, dummy_name) == 0) {
				ms->seg = m;
				ms->start_address = ms->elf_start_address + ms->seg->start_address;
				ms->end_address = ms->start_address + ms->seg->size;
				break;
			}
		}
		if (m == NULL) {
			ms->seg = add_dummy_memblock(ms->start_address, dummy_name);
		}
	}
}

/*
 * eat all of the possibly multiline input buffer.
 * input may end with a partial line that needs to be saved in line buffer
 * continue until "done" received (return false if partial input after done)
 */
bool memory::parse_map(char *in, int *new_pid, int time, double total_rate)
{
	unsigned int start, end_adr, ocm_start, ocm_end, offset;
	char name[MAX_NAME];
	char seg_name[MAX_NAME];
	memory_map *pmap = NULL;
	bool done = false;

	
	while (*in) {
		if (!pmap && *new_pid != -1) {
			for (pmap = first_map; pmap; pmap = pmap->next) {
				if (pmap->pid == *new_pid) {
					break;
				}
			}
			if (!pmap) {
				pmap = add_memory_map(*new_pid, "Unknown Applications", time, total_rate);
			}
		}
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
		done = false;

		// parse one full input line up to \n
		// skip white space at start of line
		while (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')  {
			p++;
		}
		if (strstr(p, "done") == p) {
			if (pmap) {
				update_memory_map(pmap);
				pmap->mapped = true;
			}
			done = true;
			*new_pid = -1;
			pmap = NULL;
			continue;
		}
		if (strstr(p, "PID:") == p) {
			assert(*new_pid == -1);
			*new_pid = atoi(p + 4);
			continue;
		}
		if (pmap && pmap->mapped) {
			continue;	/* redundant map send or new command - TODO - check the command for a match */
		}
		ocm_start = 0;
		if (!isdigit(*p)) { 
			// kernel module map, or command for userland map
			char *n = name;
			int count = 0;
			if (*new_pid != 0) {
				for (p = line; *p != 0 && *p != '\n' && *p != '\r'; p++) 
					;
				*p = 0;
				if (pmap)
					strcpy(pmap->command, line);
				continue;
			}
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
			start = xtoi(p + 2);
			end_adr = start + size;
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
			ocm_start = xtoi(p + 2);
			ocm_end = ocm_start + size;
			if (*p != 0) {
				assert(pmap);
				map_segment *ms = pmap->add_segment(start, end_adr, ocm_start, ocm_end, name);
				ms->elf_start_address = start;
			}
			continue;
		}
		// regular map
		start = xtoi(p);
		while (isxdigit(*p)) {
			p++;
		}
		if (*p != '-')
			continue;
		p++;
		if (!isxdigit(*p)) 
			break;
		end_adr = xtoi(p);
		while (isxdigit(*p)) {
			p++;
		}
		if (*p != ' ')
			continue;
		p += 2;
		if (*p == 'w' && *(p + 1) == 'x' && *(p + 2) == 'p') {	// stack
			if (pmap)
				pmap->stack_size += end_adr - start;
			continue;
		}
		if (*p == 'w' && *(p + 1) == '-' && *(p + 2) == 'p') {	// data
			if (pmap)
				pmap->data_size += end_adr - start;
			continue;
		}
		if (*(p + 1) != 'x') {
			if (pmap)
				pmap->other_size += end_adr - start;
			continue;
		}
		p += 4;
		if (!isxdigit(*p)) 
			continue;
		if (pmap)
			pmap->code_size += end_adr - start;
		offset = xtoi(p);
		while (isxdigit(*p)) {
			p++;
		}
		while (*p && *p != '/') {
			p++;
		}
		char *n = name;
		int count = 0;
		while (*p && *p != '\r' && *p != '\n' && count < MAX_NAME) {
			*n++ = *p++;
			count++;
		}
		*n = 0;
		/* add the segment */
		map_segment *ms;
		strcpy(seg_name, name);
		strcat(seg_name, ":.plt");
		ms = pmap->add_segment(start, end_adr, ocm_start, ocm_end, seg_name);
		if (offset) {
			ms->elf_start_address = offset;
		} else {
			ms->elf_start_address = start;
		}
		strcpy(seg_name, name);
		strcat(seg_name, ":.text");
		ms = pmap->add_segment(start, end_adr, ocm_start, ocm_end, seg_name);
		if (offset) {
			ms->elf_start_address = offset;
		} else {
			ms->elf_start_address = start;
		}
	}
	return !done;
}


void memory::write_histograms(FILE *f, double samples_per_second) 
{
	unsigned char tag = 0;
	struct {
		unsigned int lopc;
		unsigned int  hipc;
		unsigned int size;
		unsigned int rate;
		char name[15];
		char abrv;
	} header;

	unsigned short *gprofout;
	unsigned int min_addr = 0xffffffff, max_addr = 0;
	for ( memblock *seg = first_memblock; seg != 0; seg = seg->next) {
		if (seg->start_address < min_addr) {
			min_addr = seg->start_address;
		}
		if (seg->start_address + seg->size > max_addr) {
			max_addr = seg->start_address + seg->size;
		}
	}
	if (max_addr == 0) {
		return;
	}
	int bins = (max_addr-min_addr)/4;
	gprofout = (unsigned short *)malloc(bins * sizeof(unsigned short));
	if (gprofout == 0) {
		fatal_error("Can't allocate gprofout");
		return;
	}
	memset(gprofout, 0, (bins * sizeof(unsigned short)));
	fwrite(&tag, 1, 1, f);	/* histogram record next */

	/* reverse endianness, since elf file says data is from a big endian machine */
	
	header.lopc = ntohl(min_addr);
	header.hipc = ntohl(max_addr - 4);
	header.size = ntohl(bins);
	header.rate = ntohl((int)samples_per_second); 
	strcpy(header.name, "Seconds");
	header.abrv = 's';

	for (memblock *seg = first_memblock; seg != 0; seg = seg->next) {

		for (unsigned int i = 0; i < seg->size/4; ++i) {
			unsigned short gp;
			if (seg->gprof[i] > 65500.) {
				gp = 65500;			// clip it
			} else {
				gp = (int)seg->gprof[i];
			}
			gprofout[i+(seg->start_address - min_addr)/4] = ntohs(gp);
		}
	}
	fwrite(&header, sizeof(header), 1, f);		/* histogram records */
	fwrite(gprofout, 2, bins, f);
	free(gprofout);
}

// return a symbol table pointer for the symbol that matches this
// name, or null if not found

struct symbol *memory::find_symbol_by_name(const char *name) 
{
	for ( memblock *seg = first_memblock; seg != 0; seg = seg->next) {
		for (int i = 0; i < seg->symbol_count; ++i) {
			if (strcmp(seg->symbol_table[i].st_name, name) == 0) {
				return &seg->symbol_table[i];
			}
		}
	}
	return NULL;
}

/*
 * return a symbol table index for the symbol with highest address
 * less than or equal to addr
 * symbol table is already sorted by address
 * NULL for error
 */
struct symbol *memory::find_symbol_by_address(unsigned int addr, unsigned int pid, unsigned int *sym_addr, int *type_out) 
{
	struct symbol *ret;
	for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
		if (pmap->pid == pid) {
			for (map_segment *pseg = pmap->first_seg; pseg != 0; pseg = pseg->next) {
				if (addr >= pseg->start_address && addr < pseg->end_address && pseg->seg) {
					ret = pseg->seg->find_symbol_by_address(addr - pseg->elf_start_address, sym_addr, type_out);
					if (ret) {
						*sym_addr += pseg->elf_start_address;
						return ret;
					}
				}
			}
		}
	}
	return NULL;
}

/*
 * translate a pid and address to an elf absolute address
 */
unsigned int memory::translate(unsigned int pid, unsigned int addr)
{
	for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
		if (pmap->pid == pid) {
			for (map_segment *pseg = pmap->first_seg; pseg != 0; pseg = pseg->next) {
				if (addr >= pseg->start_address && addr < pseg->end_address) {
					return addr - pseg->elf_start_address;
				}
			}
		}
	}
	return NULL;
}