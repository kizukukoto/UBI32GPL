
#if !defined(MEMORY_INCLUDED)
#define MEMORY_INCLUDED

#define INFO_INIT 1
#define INFO_FUNCTION 2
#define INFO_EXEC 4
#define INFO_DSR 8

#define MAX_NAME 256
#define MAX_LINE 500

#define MEM_UNKNOWN_LABEL "Unknown functions"

#define MEM_TYPE_UNKNOWN 0
#define MEM_TYPE_ULTRA 1
// unmapped linux code
#define MEM_TYPE_LINUX 2
// mapped linux executable
#define MEM_TYPE_EXEC 3
// linux DLL
#define MEM_TYPE_DLL 4
// mapped kernel loadable module
#define MEM_TYPE_KLM 5
// dummy segment with no actual memory allocated
#define MEM_TYPE_DUMMY 6

#define NUMHAZARD 11
#define HAZ_NONE NUMHAZARD
#define HAZ_JMP 0
#define HAZ_MISSPREDJMP 1
#define HAZ_JMPTT 2
#define HAZ_JMPTF 3
#define HAZ_CALL 4
#define HAZ_CALLI 5
#define HAZ_RET 6
#define HAZ_AREG 7
#define HAZ_MAC 8
#define HAZ_IMISS 9
#define HAZ_DMISS 10

#define MEM_ADDR_ERROR "Not Found"

extern void fatal_error(const char *);

// memory model, with per-instruction data

// every text segment read from an elf file is represented by a memblock object, which contains:
//	start address (offset from start of elf file) and size
//	many counters for each instruction
//	the text data (instructions)
//	a symbol table of all labels in this text segment
// one extra special segment at address 0 for samples that have no loaded code

// every interesting process has an associated memory map, by PID
// PID 0 is unmapped space, or all of memory if there is no MMU
//	one map_segment per mmaped text segment
//	if the segment has been read from an elf file there is a pointer to the memblock


/* symbol table entry */
struct symbol {
	char *st_name;
	unsigned long st_value;
	unsigned long st_size;
	unsigned char st_info;
	unsigned char st_other;
	unsigned short st_shndx;
};

// a text memory region from an elf file, with counters per word and a symbol table
class memblock {
public:
	memblock(unsigned int start, unsigned int block_size, unsigned int *data_in, char *name, int in_type);
	~memblock(void) 
	{
		if (next != 0) {
			delete next;
			next = 0;
		}
		free(pmem);
		free(touched);
		free(target);
		free(gprof);
		free(iblocked);
		free(dblocked);
		free(hazards);
		free(haztype);
		free(hazclocks);
		free(mispredicts);
		free(targetcount);
		free(targethazard);
		free(seg_name);
		free(inst_type);
		free(taken);
		free(strings);
		free(symbol_table);
		for (int i = 0; i < PROFILE_MAX_THREADS; ++i) {
			free(thprof[i]);
		}
	}

	void memblock::init_pmem_hazards(void);

	bool add_symbol_table(struct elf_file_instance *elf_file, char *name, int section);

	double seg_hit_count(void) {
		double total = 0.0;
		for (int i = 0; i < size / 4; ++i) {
			total += gprof[i];
		}
		return total;
	}

	struct symbol *find_symbol_by_address(unsigned int addr, unsigned int *sym_addr, int *type_out) 
	{
		int lower = 0;
		int upper = symbol_count - 1;
		*type_out = type;

		// is symbol in this segment?
		if (symbol_count == 0) {
			return NULL;
		}
		if (addr < symbol_table[lower].st_value) {
			return NULL;
		}
		if (addr > symbol_table[upper].st_value + symbol_table[upper].st_size) {
			return NULL;
		}
		if (addr >= symbol_table[upper].st_value) {
			*sym_addr = symbol_table[upper].st_value;
			return &symbol_table[upper];
		}

		// binary search for symbol
		while (upper > lower + 1) {
			int mid = (upper + lower)/2;
			if (symbol_table[mid].st_value == addr) {
				*sym_addr = symbol_table[mid].st_value;
				return &symbol_table[mid];
			} else if (symbol_table[mid].st_value > addr) {
				upper = mid;
			} else {
				lower = mid;
			}
		}
		*sym_addr = symbol_table[lower].st_value;
		return &symbol_table[lower];
	}

	// how many hits between start and end-4, in this segment
	double hit_count(unsigned int start, unsigned int end, double &haz_count, double &iblock, double &dblock, bool &touch) 
	{
		double total = 0.0;
		unsigned int s, e;
		touch = false;
		haz_count = 0.0;
		iblock = 0.0;
		dblock = 0.0;
		if (start == end) {
			return 0.0;
		}
		if (end < start_address || start >= start_address + size) {
			return 0.0;
		}
		if (end > start_address + size) {
			return 0.0;
		} 
		for (unsigned int adr = start; adr < end; adr += 4) {
			total += gprof[(adr - start_address) / 4];
			haz_count += hazclocks[(adr - start_address) / 4];
			iblock += iblocked[(adr - start_address) / 4];
			dblock += dblocked[(adr - start_address) / 4];
			if (touched[(adr - start_address) / 4]) {
				touch = true;
			}
		}
		return total;
	}
	// how many hits between start and end-4
	double th_hit_count(unsigned int start, unsigned int end, int thread) 
	{
		double total = 0;
		unsigned int s, e;
		if (end < start_address || start >= start_address + size) {
			return 0;
		}
		if (end > start_address + size) {
			return 0;;
		} 
		for (unsigned int adr = start; adr < end; adr += 4) {
			total += thprof[thread][(adr - start_address) / 4];
		}
		return total;
	}

	memblock *next;
	unsigned int init_start_address;	// in bytes, unrelocated address from elf file
	unsigned int start_address;		// in bytes, memory address
	unsigned int size;			// in bytes
	unsigned int start_size;
	int type;				// type of segment, ultra, linux kernel, application, DLL, kernel loadable module
	unsigned int *pmem;			// contents of this block	
	double *gprof;				// hit counters per instruction (fraction per hit due to multithreading)
	double *thprof[PROFILE_MAX_THREADS];	// hit counters per instruction per thread
	double *iblocked;			// hit counters per instruction for time blocked due to instruction cache
	double *dblocked;			// hit counters per instruction for time blocked due to data cache
	double *hazards;			// how many times hazards detected at this instruction (not time in hazards!)
	double *mispredicts;			// jmp mispredicts here
	double *taken;				// taken jmp count here
	double *targetcount;			// how many times is this instruction a jmp/call/ret target or other hazard stalled
	double *targethazard;			// total hazards at this target
	double *hazclocks;			// clocks of hazards at this instructions (for reporting, at reporting instruction, not stalled instrcution)
	int *haztype;				// type of hazard at this instruction
	short *touched;				// was this instruction executed by thread 1<<threadid
	bool *target;				// is this instruction a branch target?
	char *seg_name;				// name of this text segment
	int *inst_type;				// instruction type for each instrcution

	// symbol table for this block
	int symbol_count;
	char *strings;				// all the strings for the symbol table
	struct symbol *symbol_table;		// symbols for the symbol table (addresses relative to init_start_address)
};

// a virtual text segment, but may be in unmapped space
// in unmapped space, the start_address and end_address are absolute, unmapped addresses, and match the start and end addresses in the elf file text segments
//   and offset must be zero
// in mapped space, the start_address and end_address are virtual addresses, and offset is subtracted from pc to get the address in the elf file
class map_segment {
public:
	map_segment(unsigned int start, unsigned int end, unsigned int ostart, unsigned int oend, const char *name) {
		seg = 0;
		gprof = 0.0;
		next = NULL;
		start_address = start;
		end_address = end;
		ocm_start = ostart;
		ocm_end = oend;
		strcpy(path, name);
		elf_start_address = 0;
	}
	~map_segment() {
		seg = NULL;
		if (next) {
			delete next;
			next = 0;
		}
	}
	char *get_name();			// get the name from the path

	char path[MAX_LINE];
	double gprof;				// total time in this memory segment
	map_segment *next;
	memblock *seg;
	unsigned int start_address;		// pc value start address of this segment
	unsigned int end_address;		// pc value end + 4 address of this segment
	unsigned int ocm_start;
	unsigned int ocm_end;
	unsigned int elf_start_address;		// pc start value corresponding to the start of the elf file (for relocating symbols)
};

/*
 * each pid (and pid zero for unmapped space) has a memory map, giving the start and end of each segment
 */
class memory_map {
public:
	memory_map(int c_pid, char *com, int time);
	~memory_map();
	map_segment *add_segment(unsigned int start, unsigned int end, unsigned int ostart, unsigned int oend, const char *name)
	{
		map_segment *ms = new map_segment(start, end, ostart, oend, name);
		ms->next = first_seg;
		first_seg = ms;
		return ms;
	}
	bool has_map() { return mapped; }

	memory_map *next;
	map_segment *first_seg;
	char command[MAX_NAME];
	int pid;
	bool mapped;
	double gprof;				// total time in this pid
	unsigned int data_size;
	unsigned int stack_size;
	unsigned int other_size;
	unsigned int code_size;
	unsigned int last_sample_time;		// when did last sample arrive for this PID?
};

// all address or pc parameters are the full 32 bit memory address

//  when add this many maps, clear out the old ones 
#define MAX_MAPS 50
// keep at least this many maps in the front of the list (recent and popular)
#define MIN_MAPS 10
// how old before a map can be cleaned up (in seconds)
#define MAX_AGE 10
// fraction of total time to preserve the PID
#define MIN_RATE 0.001


// all of text memory, a list of memory blocks, and memory maps for all PIDs
class memory {
public:
	memory();
	~memory();
	void init();

	/* add a new empty memory map for pid, return NULL if this pid already has a map */
	memory_map *add_memory_map(int pid, char *command, int time, double total_rate);

	/* parse input for a memory map for a pid */
	bool parse_map(char *input, int *pid, int time, double total_rate);

	/* get a pointer to the memory map for a PID, or NULL */
	memory_map *get_memory_map(int pid);

	/* clean up old memory maps */
	void clean_maps(int time, double total_rate);

	// add an existing memory block (text segment) to a memory map
	void add_memory_seg(int pid, memblock *mb, unsigned int start, unsigned int end, char *name) {
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *ms = pmap->first_seg; ms; ms = ms->next) {
					if (ms->seg == mb)
						return;
				}
				map_segment *ms = new map_segment(start, end, 0, 0, name);
				ms->seg = mb;
				ms->next = pmap->first_seg;
				pmap->first_seg = ms;
			}
		}
	}

	struct symbol *find_symbol_by_name(const char *name);
	// return pointer to symbol and the relocated symbol start address
	struct symbol *find_symbol_by_address(unsigned int addr, unsigned int pid, unsigned int *sym_addr, int *type);
	// return an elf file absolute address in memblock
	unsigned int translate(unsigned int pid, unsigned int addr);
	// add a new code segment of seg_size bytes.  data must be from malloc and will be freed in memory destructor
	memblock *add_memblock(unsigned int start, unsigned int seg_size, unsigned int *data, char *name, int type) 
	{
		memblock *p = new memblock(start, seg_size, data, name, type);
		if (!p) {
			free(data);
			return p;
		}
		if (first_memblock == 0) {
			first_memblock = p;
		} else {
			memblock *n = first_memblock;
			while (n->next != 0) {
				n = n->next;
			}
			n->next = p;
		}
		return p;
	}

	// add a dummy memblock with no code, to hold counts for unknown functions
	memblock *add_dummy_memblock(unsigned int start, char *name)
	{
		// combine duplicates from different PIDs
		for (memblock *p = first_memblock; p; p = p->next) {
			if (strcmp(name, p->seg_name) == 0)
				return p;
		}
		unsigned int seg_size = 8;	// need two dummy isntructions since some code looks at next instruction
		unsigned int *data = (unsigned int *)malloc(seg_size);
		if (data) {
			for (int i = 0; i < seg_size / 4; ++i) {
				data[i] = 0xc8000000;	// NOP instruction
			}
		}
		return add_memblock(start, seg_size, data, name, MEM_TYPE_DUMMY);
	}

	char *addr_to_section_name(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						return pseg->path;
					}
				}
			}
		}
		return MEM_ADDR_ERROR;
	}

	char *pid_to_command(unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				return pmap->command;
			}
		}
		return MEM_ADDR_ERROR;
	}

	void add_gprof(unsigned int pc, unsigned int pid, double rate)
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				pmap->gprof += rate;
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						pseg->gprof += rate;
						break;
					}
				}
			}
		}
	}

	// a valid mapped or unmapped address
	bool address_valid(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						return true;
					}
				}
			}
		}
		return false;
	}

	// a valid mapped or unmapped address, and we have an associated text segment for this address
	bool address_valid_text(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address && pseg->seg) {
						return true;
					}
				}
			}
		}
		return false;
	}


	int get_type(unsigned int pc, unsigned int pid)
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address && pseg->seg) {
						return pseg->seg->type;
					}
				}
			}
		}
		return MEM_TYPE_UNKNOWN;
	}

	unsigned int &pmem(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->pmem[0];
						else 
							return pseg->seg->pmem[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return uint_error;
	}

	int &inst_type(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->inst_type[0];
						else 
							return pseg->seg->inst_type[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return int_error;
	}


	double &gprof(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->gprof[0];
						else
							return pseg->seg->gprof[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	double &iblocked(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->iblocked[0];
						else
							return pseg->seg->iblocked[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	double &dblocked(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->dblocked[0];
						else
							return pseg->seg->dblocked[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	memblock *get_seg_by_name(char *name)
	{
		for (memblock *seg = first_memblock; seg != 0; seg = seg->next) {
			if (strcmp(seg->seg_name, name) == 0) {
				return seg;
			}
		}
		return NULL;
	}

	double &thprof(unsigned int pc, unsigned int pid, int thread) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->thprof[thread][0];
						else
							return pseg->seg->thprof[thread][(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	double &mispredicts(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->mispredicts[0];
						else
							return pseg->seg->mispredicts[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	double &taken(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->taken[0];
						else
							return pseg->seg->taken[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	// number of samples with a hazard
	double &hazards(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->hazards[0];
						else
							return pseg->seg->hazards[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	int &haztype(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->haztype[0];
						else
							return pseg->seg->haztype[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return haztype_error;
	}

	// number of clocks wasted in hazards
	double &hazclocks(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->hazclocks[0];
						else
							return pseg->seg->hazclocks[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	double &targetcount(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->targetcount[0];
						else
							return pseg->seg->targetcount[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}

	double &targethazard(unsigned int pc, unsigned int pid) 
	{
		for (memory_map *pmap = first_map; pmap; pmap = pmap->next) {
			if (pmap->pid == pid) {
				for (map_segment *pseg = pmap->first_seg; pseg; pseg = pseg->next) {
					if (pc >= pseg->start_address && pc < pseg->end_address) {
						if (pseg->seg->type == MEM_TYPE_DUMMY)
							return pseg->seg->targethazard[0];
						else
							return pseg->seg->targethazard[(pc - pseg->start_address) / 4];
					}
				}
			}
		}
		return double_error;
	}


	void write_histograms(FILE *f, double sps);

	unsigned int total_size()
	{	// return total size of all code segments in bytes
		unsigned int size = 0;
		for (memblock *p = first_memblock; p != 0; p = p->next) {
			size += p->size;
		}
		return size;
	}

	// get text segments
	memblock *get_first_memblock() {
		return first_memblock;
	}

	// the memory maps for unmapped space (7K and 8K) and user applications (8K)
	memory_map *first_memory_map() {
		return first_map;
	}

	// sort the maps to some extent
	void sort_maps(void) {
		memory_map **mm = &first_map;
		while ((*mm)->next) {
			if ((*mm)->next->gprof > (*mm)->gprof) {
				memory_map *tmp = (*mm)->next;
				(*mm)->next = tmp->next;
				tmp->next = (*mm);
				(*mm) = tmp;
			}
			mm = &(*mm)->next;
		}
	}
private:
	/* connect the memory map to segments with the same name */
	void update_memory_map(memory_map *mm);

	memblock *first_memblock;		// all loaded text segments
	memblock *dummy_memblock;		// segment for unrecognized addresses
	memory_map *first_map;			// all maps from pc, map each pid to its text segments, PID 0 is for unmapped space (everything on IP7K)
	unsigned int uint_error;
	int int_error;
	double double_error;
	short touched_error;
	bool target_error;
	int haztype_error;
	int num_maps;				// number of memory maps being tracked
	char line[MAX_LINE];			// hold one line of input while parsing
	char *end;				// end of line
};

#endif
