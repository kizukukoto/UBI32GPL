// profile.cpp: implementation of the profile class.
//
//////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// for ntohl
#if defined(WIN32)
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#define LOGNAME "profile_log.txt"
#define LOGSTATSNAME "profile_stats_log.txt"

#include "profile.h"

#ifdef NEVER
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif

#define NETPAGE_SIZE 256

char *pagenames[MAXPAGES] = {
	"Summary",
	"Threads",
	"Applications",
	"Functions",
	"Hazards",
	"Hazard Details",
	"Memory",
	"Fragmentation",
	"Locks",
	"Loops",
	"Graphs",
	"Statistics",
	"Instructions",
	"HRT Threads",
};

bool profile::inpram(unsigned int addr)
{
	if (chip_type == CHIP_7K) {
		return addr >= 0x3ffc0000 && addr < 0x40000000; 
	}
	return addr >= 0xbffc0000 && addr < 0xc0000000; 
}

int countbits(u16_t in)
{
	int count = 0;
	for (int i = 0; i < NUMTHREADS; ++i){
		if ((in>>i) & 1) {
			count++;
		}
	}
	return count;
}

int offset21(unsigned int inst)	// for jmp
{
	int offset = inst & 0x1fffff;
	if (offset & 0x100000) {
		offset = offset | 0xfff00000;	// sign extend
	}
	return offset << 2;
}

int offset24(unsigned int inst)	// for call
{
	int offset = inst & 0x1fffff;
	offset |= (inst & 0x07000000)>>3;
	if (offset & 0x800000) {
		offset = offset | 0xff000000;	// sign extend
	}
	return offset << 2;
}

int decode_instruction(unsigned int instruction)
{
	unsigned int opcode = (instruction >> 27) & 0x1f;
	unsigned int subop = 0;
	int type;
	if (opcode == 0) {		// 2 operand
		subop = (instruction >> 11) & 0x1f;
		if ((subop == 0xc || subop == 0xd || subop == 0xf) &&
			((instruction >> 24) & 0x7) == 0) {
			type = INST_NOP;	// move to immediate is a NOP
		} else {
			type = 32 + subop;
		}
	} else if (opcode == 2) {		// shift instruction
		subop = (instruction >> 21) & 0x1f;
		type = 64 + subop;
	} else if (opcode == 3) {		// shift instruction
		subop = (instruction >> 21) & 0x1f;
		type = 164 + subop;
	} else if (opcode == 6) {		// DSP instruction
		subop = (instruction >> 21) & 0x1f;
		type = 100 + subop;
	} else if (opcode == 0x19 &&
		((instruction >> 24) & 0x7) == 0) {
			type = INST_NOP;	// movei to immediate is a NOP
	} else if ((instruction & 0x8000) && opcode >= 8 && opcode <= 0x17) {
		type = 132 + opcode;		// general ops with bit above Dn set
	} else {
		type = opcode;
		if (type == INST_JMP) {	// jmp
			int cond = (instruction >> 23) & 0xf;
			if (cond == 13) {	//jmpt
				int offs = instruction & 0x1fffff;
				if (offs == 1) {
					if ((instruction >> 22) & 1) {
						type = INST_NOPJMPTT;
					} else {
						type = INST_NOPJMPTF;
					}
				} else {
					type = INST_JMPT;
				}
			}
			if (cond == 0) {
				type = INST_NOPJMPTF;
			}
		}
	}
	return type;
}

int decode_condition(unsigned int inst, unsigned char cond) 
{
	if ((inst >> 21) & 1) {
		cond = cond >> 4;
	}
	int N = (cond & 8) >> 3;
	int Z = (cond & 4) >> 2;
	int V = (cond & 2) >> 1;
	int C = (cond & 1);
	switch ((inst >> 23) & 0xf) {
	case 0: return false;
	case 1: return !C;
	case 2: return C;
	case 3: return Z;
	case 4: return (N&V)|(!N&!V);
	case 5: return (N&V&!Z)|(!N&!V&!Z);
	case 6: return C&!Z;
	case 7: return Z|(N&!Z)|(!N&V);
	case 8: return !C|Z;
	case 9: return (N&!V)|(!N&V);
	case 10: return N;
	case 11: return !Z;
	case 12: return !N;
	case 13: return true;
	case 14: return !V;
	case 15: return V;
	}
	return 0;
}

int jmphazards[CHIP_ID_COUNT][4][PROFILE_MAX_THREADS + 2] = {	// by taken, predicted, and multithreading
	{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0},	// not taken, pred not taken
	{ 7, 7, 3, 2, 1, 1, 1, 1, 0},	// not taken, pred taken
	{ 7, 7, 3, 2, 1, 1, 1, 1, 0},	// taken, pred not taken
	{ 3, 3, 1, 1, 0, 0, 0, 0, 0},	// taken, pred taken
	},
	{
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0},	// not taken, pred not taken
	{ 8, 8, 4, 2, 2, 1, 1, 1, 1},	// not taken, pred taken
	{ 8, 8, 4, 2, 2, 1, 1, 1, 1},	// taken, pred not taken
	{ 3, 3, 1, 1, 0, 0, 0, 0, 0},	// taken, pred taken
	},
};

int callhazards[PROFILE_MAX_THREADS + 2] = {
	3, 3, 1, 1, 0, 0, 0, 0, 0
};

int callihazards[PROFILE_MAX_THREADS + 2] = {
	4, 4, 2, 1, 1, 0, 0, 0, 0
};

int rethazards[CHIP_ID_COUNT][PROFILE_MAX_THREADS + 2] = {
	{
	7, 7, 3, 2, 1, 1, 1, 1, 0
	},
	{
	8, 8, 4, 2, 2, 1, 1, 1, 1
	},
};

int macwindow[CHIP_ID_COUNT][PROFILE_MAX_THREADS + 2] = {
	{
	3, 3, 1, 1, 0, 0, 0, 0, 0
	},
	{
	2, 2, 1, 0, 0, 0, 0, 0, 0
	},
};

int machazards[PROFILE_MAX_THREADS + 2] = {
	6, 6, 3, 2, 0, 0, 0, 0, 0
};

int anwindow[CHIP_ID_COUNT][PROFILE_MAX_THREADS + 2] = {
	{
	4, 4, 2, 1, 1, 0, 0, 0, 0
	},
	{
	5, 5, 2, 1, 1, 0, 0, 0, 0
	},
};

int areghazards[PROFILE_MAX_THREADS + 2] = {
	5, 5, 3, 2, 2, 0, 0, 0, 0
};

/* 
 * address compare function for qsort 
 */
int compare( const void *arg1, const void *arg2 )
{
	symbol *s1, *s2;
	s1 = (symbol *)arg1;
	s2 = (symbol *)arg2;

	if (s1->st_value < s2->st_value) {
		return -1;
	}
	if (s1->st_value > s2->st_value){
		return 1;
	}
	return 0;
}

int parse_chip_type(int cpu_id)
{
	if ((cpu_id >> 16) == 8)
		return CHIP_8K;
	return CHIP_7K;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

profile::profile()
{
	console_state = CONSOLE_IDLE;
	clear_elf_files();
	last_packet_time = time(NULL);
	update = true;
	clock_rate = 250.0;		// default value
	ddr_freq = 200;
	cpu_id = (7 << 16) + 1;
	chip_type = parse_chip_type(cpu_id);
	data_thread = -1;		// show all threads
	start_time = 0;
	log_data = false;
	log_seconds = 6;
	for (int i = 0; i < MAX_STAT_DISPLAY; ++i) {
		stat_data[i].enabled = 0;
		stat_data[i].val_count = 0;
		stat_data[i].sum_val = 0;
	}
	stat_data[0].enabled = stat_data[1].enabled = stat_data[2].enabled = 1;
	strcpy(elf_file_error_name, "No file name");
}

profile::~profile()
{
}

void profile::clear_elf_files()
{
	elf_file_count = 0;
	lib_path_count = 0;
}

/*
 * elf file names must be set up first
 */
void profile::init_profiler(bool initial_update, bool (*send)(const char *msg))
{
	console_send = send;
	update = initial_update;
	start_time = 0;			// wait for next update to really start
	init_all_data();
}

bool profile::init_all_data()
{
	pid_pending = -1;
	d_access_per_sec = 0;
	bad_sample_count = 0;
	d_miss_per_sec = 0;
	i_miss_per_sec = 0;
	d_tlb_miss_per_sec = 0;
	i_tlb_miss_per_sec = 0;
	ptec_miss_per_sec = 0;
	max_d_access_per_sec = 0;
	max_d_miss_per_sec = 0;
	ddr_freq = 0;
	last_total_rate = 0;
	current_mips = 0;
	avecnt = totheap = totnetpage = totsdheap = totsdnetpage = 0;
	stats_count = 0;
	stat_packet_count = 0;
	hrt_samples = false;
	elapsed_time = 0;
	mem.init();

	heapmin = 256 * 1024;	// bytes (all of OCM)
	netpagemin = 1024;	// pages (all of OCM)
	nrt_active_total = 0;
	nrt_active_count = 0;
	error = ERROR_NONE;
	illegal_inst = false;
	packet_count = 0;
	log_packet_count = 0;

	last_seq = 0;
	dropped_packets = 0;
	src_count = dst_count = both_count = stack_count = 0.0;
	src_move_count = dst_move_count = both_move_count = 0.0;
	total_rate = 0.0;
	init_count = 0;
	total_clocks = 0;
	last_total_clocks = 0;
	any_active_count = 0.0;
	any_i_blocked = 0;
	any_d_blocked = 0;
	current_clock = 0;
	profile_count = 0;
	next_cg = 0;
	recent_idle = 0;
	num_pm = 0;
	getting_map = false;
	page_shift = 12;	/* default 4 KB pages */
	ultra_start = 0x40000000;
	sdram_begin = 0x40000000;
	ocm_heap_run_begin = sdram_begin;
	ocm_heap_run_end = sdram_begin;
	heap_run_begin = sdram_begin;
	heap_run_end = sdram_begin;
	ultra_size = 0;
	linux_start = 0;
	linux_end = linux_start;
	jmpdirection[0] = jmpdirection[1] = jmpdirection[2] = jmpdirection[3] = 0;
	int i;
	for (i = 0; i < NUMTHREADS; ++i) {
		sample_count[i] = 0;
		type[i] = T_DISABLED;
		sw_type[i] = SWT_DISABLED;
		active_count[i] = 0;
		active_unblocked_count[i] = 0;
		last_active_count[i] = 0;
		flash_count[i] = 0;
		last_flash_count[i] = 0;
		instruction_count[i] = 0;
		current_inst_count[i] = 0;
		min_sp_val[i] = 0xffffffff;
		hazard_count[i] = 0.0;
		inst_counted[i] = 0.0;
		recent_mips[i] = 0;
		thread_rate_sum[i] = 0.0;
		thread_blocked_i_sum[i] = 0.0;
		thread_blocked_d_sum[i] = 0.0;
		thread_rate[i] = 0.0;
		hrt_rate[i] = 0.0;
		i_blocked[i] = d_blocked[i] = 0;
		src_count_thread[i] = 0;
		dst_count_thread[i] = 0;
		both_count_thread[i] = 0;
		src_move_count_thread[i] = 0;
		dst_move_count_thread[i] = 0;
		both_move_count_thread[i] = 0;
		stack_count_thread[i] = 0;
		jmp_count_thread[i] = 0;
		call_count_thread[i] = 0;
		ret_count_thread[i] = 0;
		calli_count_thread[i] = 0;
		dsp_count_thread[i] = 0;
	}
	for (i = 0; i < 5; ++i) {
		adrhazcounts[i] = 0;
	}
	for (i = 0; i < NUMTHREADS+1; ++i) {
		mtcount[i] = 0;
		mtactivecount[i] = 0;
	}
	for (i = 0; i < INST_COUNT; ++i) {
		icount[i] = 0.0;
	}
	mac4count = 0.;
	for (i = 0; i < 4; ++i) {
		jmpcounts[i]= 0.0;
		jmpfwdcounts[i] = 0.0;
	}
	for (i = 0; i < PROFILE_COUNTERS; ++i) {
		perf_counters[i] = 0;
		current_perf_counters[i] = 0;
		last_perf_counters[i] = 0;
	}
	for (i = 0; i < NUMHAZARD; ++i) {
		haztypecounts[i] = hazcounts[i] = 0.;
	}
	moveihazcount = 0.0;
	itotal_nrt = 0.0;
	itotal_all = 0.0;
	thread = 0;
	if (update) {
		for (i = 0; i < MAXPAGES; ++i) {
			lastline[i] = 0;
			for (int j = 0; j < MAXLINES; ++j)
				bars[i][j] = 0;
		}
		for (i = 0; i < MAX_STAT_DISPLAY; ++i) {
			stat_data[i].range = 100.;
			stat_data[i].found = 0;
			stat_data[i].max_val = 0.0;
			stat_data[i].min_val = MIN_INIT_VAL;
			stat_data[i].sum_val = 0.0;
			stat_data[i].val_count = 0;
			stat_data[i].range_min = 1.0;
			stat_data[i].range_max = 1000000000.;
			gvals[i] = 0.;
			for (int j = 0; j < MAXSECS; j++) {
				gval_hist[i][j] = 0.0;
			}
		}
		gnow = 0;
		for (i = 0; i < MAX_STATS; ++i) {
			stats_last_val[i] = 0;
		}
	}
	if (elf_file_count == 0) {
		return false;
	}
	char buf[500];
	if (read_elf_file(elf_files[0]) != ELF_OK) {
		sprintf(buf, "Please restart.  Can't open or parse the Ultra elf file %s", elf_files[0]);
		fatal_error(buf);
		error = ERROR_BAD_ELF;
		strcpy(elf_file_error_name, elf_files[0]);
		return false;
	}
	for (int i = 1; i < elf_file_count; ++i) {
		int err = read_posix_elf_file(elf_files[i]);
		if (err != ELF_OK) {
			if (err == ELF_ERROR_OPEN) {
				sprintf(buf, "Can't open %s", elf_files[i]);
			} else if (err == ELF_ERROR_BAD_FORMAT) {
				sprintf(buf, "Not ELF format: %s", elf_files[i]);
			} else if (err == ELF_ERROR_STRIPPED) {
				sprintf(buf, "Stripped file ignored: %s", elf_files[i]);
			} else {
				sprintf(buf, "Elf file error %s", elf_files[i]);
			}
			fatal_error(buf);
			error = ERROR_BAD_ELF;
			strcpy(elf_file_error_name, elf_files[i]);
		}
	}
	return true;
}

void profile::add_call(unsigned int p_adr, unsigned int s_adr, double freq)
{
	for (unsigned int i = 0; i < next_cg; ++i) {
		if (cg[i].parent == p_adr && cg[i].self == s_adr) {
			cg[i].count += freq;
			return;
		}
	}
	cg[next_cg].parent = p_adr;
	cg[next_cg].self = s_adr;
	cg[next_cg].count = freq;
	if (next_cg < MAX_CALL_ARCS-1) {
		next_cg++;
	}
}

#define CALL_INVALID 0
#define CALL_VALID 1
#define CALL_POSSIBLE 2

int profile::call_valid(unsigned int ret_addr, unsigned int pc, unsigned int pid, unsigned int func_start, int type) 
{
	if (!mem.address_valid_text(ret_addr, pid)) {
		return CALL_INVALID;		// not pointing at a text segment
	}

	/* find possible calling instruction from sample.parent */
	unsigned int parent = ret_addr - 4;
	
	unsigned int inst_parent = mem.pmem(parent, pid);
	int opcode = (inst_parent >> 27) & 0x1f;
	
	unsigned int parent_func_start;
	int parent_type;
	struct symbol *sym = mem.find_symbol_by_address(parent, &parent_func_start, &parent_type);
	if (sym == NULL) {
		return CALL_INVALID;
	}

	/* check if it is a call or calli */
	if (opcode == 0x1b) {		/* call */
		if (type == MEM_TYPE_KLM && parent_type == MEM_TYPE_KLM) {
			return TRUE;	/* klm calls not relocated so call address is worthless for verification */
		}
		unsigned int target = parent + offset24(mem.pmem(parent, pid));
		if (!mem.address_valid_text(target, pid)) {
			return CALL_INVALID;
		}
		unsigned int inst_parent = mem.pmem(target, pid);
		if ((inst_parent & 0xf8000000) == 0 && (inst_parent & 0xf800) == 0xf000) {
			return CALL_VALID;	/* call target is a pdec, so it's in the PLT TODO: long PLTs and validate call address somehow */
		}
		return target == func_start ? CALL_VALID : CALL_INVALID;
	}

	if (opcode == 0x1e) {		/* calli  TODO: must handle relocation of moveai constant*/
		unsigned int prev = mem.pmem(parent - 4, pid);
		int prev_opcode = (prev >> 27) & 0x1f;
		if (inst_parent == 0xf0a000a0 && prev_opcode != 0x1c) {	/* calli a5 0(a5) used as a return (no moveai prior) */
			return CALL_INVALID;
		}
		if (prev_opcode != 0x1c) {
			return CALL_POSSIBLE;		/* indirect call - could be to anywhere */
		}
		unsigned int target = ((prev & 0x07000000) << 4) | ((prev & 0x1fffff) << 7);
		if (target == 0) {	// unrelocated KLM
			return CALL_POSSIBLE;
		}
		target |= ((inst_parent & 0x300) >> 1) | ((inst_parent & 0x1f) << 2); 
//		target += mem.relocation(pc);		TODO, need some kind of reloaction for kernel modules
		return target == func_start ? CALL_VALID : CALL_INVALID;	
	}

	return CALL_INVALID;
}

void profile::call_graph(struct profile_sample sample, double freq)
{
	unsigned int func_start;
	int type;
	struct symbol * sym = mem.find_symbol_by_address(sample.pc, &func_start, &type);
	if (sym == NULL) {	
		return;		// can't find the address - KLM is not reloacated in elf file so no way to validate the address
	}

	if (call_valid(sample.a5, sample.pc, sample.pid, func_start, type)) {
		add_call(sample.a5 - 4, func_start, freq);
	} else if (call_valid(sample.parent, sample.pc, sample.pid, func_start, type)) {
		add_call(sample.parent - 4, func_start, freq);
	} else {
		add_call(0, func_start, freq);
	}
}

void profile::max_hazard(int hazard, char *buf, double inst)
{
	double max_val = 0;
	int max_pc = 0;
	char buf2[1000];
	memblock *max_mb;
	for (memblock *p = mem.first_segment(); p; p = p->next) {
		for (unsigned int i = 0; i < p->size / 4; i++) {
			if (p->hazclocks[i] > max_val && p->haztype[i] == hazard) {
				max_val = p->hazclocks[i];
				max_pc = i;
				max_mb = p;
			}
		}
	}
	if (max_val == 0) {
		strcpy(buf, "None");
	} else {
		symbol_address_to_name(max_pc, buf2);	// TODO!
		sprintf(buf, "%40s: %6.2f MIPS", buf2, 
			clock_rate * max_mb->hazclocks[max_pc] / itotal_nrt * inst / total_clocks);
	}
}


// take pc value as a byte address
// return a string corresponding to a code address, formatted
// like symbol+0xOFFSET
bool profile::symbol_address_to_name(unsigned int pc, char *buf)
{
	unsigned int func_start;
	int type;
	struct symbol *sym = mem.find_symbol_by_address(pc, &func_start, &type);
	if (sym == NULL) {
		strcpy(buf, "Symbol_not_found");
		return false;
	}
	sprintf(buf, "%s+%02x", sym->st_name, pc - func_start);
	return true;
}

bool profile::symbol_address_to_name(memblock *seg, int inst, char *buf)
{
	unsigned int func_start;
	int type;
	struct symbol *sym = seg->find_symbol_by_address(seg->start_address + inst * 4, &func_start, &type);
	if (sym == NULL) {
		strcpy(buf, "Symbol_not_found");
		return false;
	}
	sprintf(buf, "%s+%02x", sym->st_name, seg->start_address + inst * 4 - func_start);
	return true;
}

// a symbol index to its name  
char *profile::symbol_name(symbol *sym)
{
	if (*sym->st_name == '_') {
		return sym->st_name + 1;	
	} else {
		return sym->st_name;	
	}
}

// return true if next instruction is unconditional
// not: jmp (not jmpt), calli as return, or ret
// and update next value for unconditional next pc or conditional jmp target
// next can point to pc

bool profile::next_pc(unsigned int inst, unsigned int pc, memblock *seg, int type, unsigned int *next) 
{
	if (type == INST_JMP || type == INST_RET) {	// conditional jmp or ret
		*next = pc + offset21(inst);
		return false;
	}
	if (type == INST_CALLI) {		// is it a call?
		unsigned int prev = seg->pmem[(pc / 4) - 1];
		if (((prev >> 27) & 0x1f) != 0x1c) {
			return false;	// prev inst must be moveai
		}
		if (((prev >> 21) & 0x7) != 5) {
			return false;	// prev must be moveai to a5
		}
		unsigned int inst = seg->pmem[pc / 4];
		if (((inst >> 5) & 0x7) != 5) {
			return false;	// inst must be calli  a5, xx(a5)
		}
		if (((inst >> 21) & 0x7) != 5) {
			return false;
		}
		*next = ((prev << 4) & 0x70000000) | ((prev << 7) & 0x0fffff80) | ((inst << 2) & 0x7f);
//		*next += seg->relocation;	TODO - need relocation for kernel modules
		return true;
	} else if (type == INST_JMPT || type == INST_NOPJMPTF || type == INST_NOPJMPTT) {	// unconditional jmp
		*next = pc + offset21(inst);
		return true;
	} else if (type == INST_CALL) {
		*next = pc + offset24(inst);
		return true;
	}
	*next = pc + 4;
	return true;
}

#if 0

// mark this instruction touched, and all subsequent ones, 
// stop at conditional jmp or instruction already marked

void profile::touch_forward(unsigned int pc, int thread)
{
	if (pc == 0)	// ignore errors
		return;
	do {
		if (mem.touched(pc)) {
			return;
		}
		mem.touched(pc) |= 1 << thread;
		touch_count++;
		if (mem.initcode(pc)) {
			init_count--;
		}

		int type = decode_instruction(mem.pmem(pc, pid));
		unsigned int next;
		int valid = next_pc(mem.pmem(pc, pid), pc, pid, type, &next);
		if (type == INST_CALL && valid) {
			touch_forward(next, thread);	// also do the call target
			pc = pc + 4;
		} else if (type == INST_CALLI) {
			if (valid) {
				touch_forward(next, thread);		// calli calls also always return
				pc = pc + 4;
			} else {
				return;
			}
		} else if (type == INST_RET || type == INST_JMP) {
			return;
		} else if (valid) {
			pc = next;
		} else {
			return;
		}
	} while(1);
}


// touch backwards until a call or calli call or conditional jmp target
void profile::touch_reverse(unsigned int pc, int thread)
{
	while (pc >= 1) {
		if (mem.target(pc)) {
			return;
		}
		pc -= 4;
		if (mem.touched(pc)) {
			return;
		}
		mem.touched(pc) |= 1 << thread;
		touch_count++;
		if (mem.initcode(pc)) {
			init_count--;
		}

		int type = decode_instruction(mem.pmem(pc, pid));
		unsigned int next;
		int valid = next_pc(mem.pmem(pc, pid), pc, pid, type, &next);
		if (type == INST_CALL && valid) {
			touch_forward(next, thread);	// also do the call target
		} else if (type == INST_CALLI) {
			if (valid) {
				touch_forward(next, thread);		// calli calls also always return
			}
		}
	}
}

#endif

bool is_mapped(u32_t pc)
{
	return pc < 0xb0000000;
}


// process a sample from a thread (should never be from GDB thread)
void profile::on_sample(struct profile_sample &sample) 
{
	int thread = sample.thread & 0xf;		/* pull out the thread number */

	if (chip_type == CHIP_8K && is_mapped(sample.pc)) {
		if (mem.get_memory_map(sample.pid) == NULL)
			return;		/* ignore samples until the memory map is available */
	}
	if (!mem.address_valid(sample.pc, sample.pid)) {
		bad_pc_value = sample.pc;
		sample.pc = 0;				// error, map all to address 0
		if (sample.active & (1 << thread)) {
			bad_sample_count++;		// unknown addresses in active threads
		}
	}
	unsigned int pid = sample.pid;

	sample_count[thread]++;  /* number of profiler samples taken, whether active or not */

	if (type[thread] == T_HRT) {
		hrt_samples = true;
		sw_type[thread] = SWT_HRT;
	} else if (type[thread] != T_GDB) {
		int t = mem.get_type(sample.pc, sample.pid);
		if (t == MEM_TYPE_ULTRA) {
			sw_type[thread] = SWT_ULTRA;
		} else if (t == MEM_TYPE_LINUX) {
			sw_type[thread] = SWT_LINUX;
		}
	}
	int ccodes = sample.cond_codes;

	int nrt_hi_unblocked = 0;
	int nrt_lo_unblocked = 0;
	double hrt_unblocked = 0.0;	// total active unblocked hrt fraction (including NRT with HRT).  NRT threads split the rest

	int nrt_hi_blocked = 0;
	int nrt_lo_blocked = 0;

	int nrt_hi_active = 0;
	int nrt_lo_active = 0;
	double hrt_active = 0.0;	// sum of active thread rates for HRT threads
	int blocked = sample.i_blocked | sample.d_blocked;
	for (int t = 0; t < NUMTHREADS; ++t) {
		if (type[t] == T_GDB) {
			continue;	// don't count the profiling thread - always active when a sample is taken
		}
		if (sample.active & (1 << t)) {
			if (type[t] == T_HIGH) {
				nrt_hi_active++;
				if (!(blocked & (1 << t))) {
					nrt_hi_unblocked++;
				} else {
					nrt_hi_blocked++;
				}
			}
			else if (type[t] == T_NRT) {
				nrt_lo_active++;
				if (!(blocked & (1 << t))) {
					nrt_lo_unblocked++;
				} else {
					nrt_lo_blocked++;
				}
			} else if (type[t] == T_HRT) {
				hrt_active += hrt_rate[t];
			}
			if (!(blocked & (1 << t))) {
				hrt_unblocked += hrt_rate[t];
			}
		}
	}

	double nrt_active = nrt_hi_active;	// number of NRT active and unblocked (not counting GDB) - determines hazards
	int nrt_unblocked = nrt_hi_unblocked;
	if (nrt_active == 0) {
		nrt_active = nrt_lo_active;
		nrt_unblocked = nrt_lo_unblocked;
	}

	// for the NRT multithreading rate - how many threads are active when any NRT threads are running
	if (nrt_active != 0) {
		nrt_active_total += nrt_active + hrt_active;
		nrt_active_count++;
	}

	if (thread == 0) {
		int count = countbits(sample.active & (~blocked)) - 1;	/* active unblocked NRT or HRT threads.  Take away one for the profiling thread */
		if (count < 0) {
			count = 0;
		}
		if (sample.d_blocked) {
			any_d_blocked++;
		}
		if (sample.i_blocked) {
			any_i_blocked++;
		}
		if (nrt_active) {
			any_active_count++;		// not idle becuause an NRT is active
		} else {
			any_active_count += hrt_active;	// not idle for HRT fraction of time
		}
		if (nrt_hi_active) {
			// don't count active nrt low priority threads when high priority nrt is active, since they are not schedulable
			for (int t = 0; t < NUMTHREADS; ++t) {
				if (type[t] == T_NRT && (sample.active & (1 << t)) && !(blocked & (1 << t))) {
					count--;
				}
			}
		}
		mtcount[count]++;	// counts full value for HRT for display, not for hazard calc.
		count = countbits(sample.active) - 1;
		if (count < 0) {
			count = 0;
		}
		mtactivecount[count]++;
	}

	if (data_thread != -1 && data_thread != thread) {
		return;		// ignore samples not to be processed
	}

	int thread_active = sample.active & (1 << thread);	// this thread is active
	int thread_blocked = blocked & (1 << thread);	// this thread is blocked

	// collect data for each thread, when it is active (even if it is blocked)
	if (thread_active) { 
		active_count[thread]++;				// active, may be blocked
		if (!thread_blocked) {
			active_unblocked_count[thread]++;		// active and not blocked
		}

		// rate of execution of this thread, between 0 and 1, depending on multithreading, sum for all threads at one sample time is 1
		// rate is zero if this thread is blocked and others are not
		double rate = 0.0;	// rate of execution, if ANY nrt ACTIVE, MUST ADD TO ONE, ELSE ADD TO hrt_rate
		if (type[thread] == T_HRT) {
			if (!thread_blocked) {
				rate = thread_rate[thread];	// running (active and not blocked)
			} else if (!nrt_hi_unblocked && (nrt_hi_active != 0 || !nrt_lo_unblocked)) {
				rate = thread_rate[thread];	// blocked and no NRT threads to help
			}
		} else if (type[thread] == T_HIGH) {
			if (!thread_blocked) {
				rate = hrt_rate[thread] + (1 - hrt_unblocked) / nrt_hi_unblocked;
			} else if (!nrt_hi_unblocked) {		// all nrt threads are blocked, allocate time evenly among them
				rate = hrt_rate[thread] + (1 - hrt_unblocked) / nrt_hi_blocked;
			}
			// if this thread is blocked and other threads are unblocked, count nothing
		} else if (type[thread] == T_NRT && nrt_hi_active == 0) {
			if (!thread_blocked) {
				rate = hrt_rate[thread] + (1 - hrt_unblocked) / nrt_lo_unblocked;
			} else if (!nrt_lo_unblocked) {		// all nrt threads are blocked, allocate time evenly among them
				rate = hrt_rate[thread] + (1 - hrt_unblocked) / nrt_lo_blocked;
			}
			// if this thread is blocked and other threads are unblocked, count nothing
		}

		// time sampled at this instruction - mips + hazards + blocked - includes execution and hazard time
		mem.add_gprof(sample.pc, sample.pid, rate);
		if (!mem.address_valid_text(sample.pc, sample.pid)) {
			sample.pc = 0;
		}

		mem.gprof(sample.pc, sample.pid) += rate;
		mem.thprof(sample.pc, sample.pid, thread) += rate;

		total_rate += rate;
		thread_rate_sum[thread] += rate;	// rate when active

		double freq = rate;	// frequency at this point, derated for jmp/call/ret target and an/dn hazard overcounting by sampler

		int inst_type = decode_instruction(mem.pmem(sample.pc, pid));
		int src = idata[inst_type].src && ((mem.pmem(sample.pc, pid) & 0x400) || (mem.pmem(sample.pc, pid) & 0x200));
		int dst = idata[inst_type].dst && ((mem.pmem(sample.pc, pid) & 0x04000000) || (mem.pmem(sample.pc, pid) & 0x2000000));
		bool iblocked = sample.thread & PROFILE_I_BLOCKED;
		bool dblocked = sample.thread & PROFILE_D_BLOCKED;
		if (thread_blocked) {
			if (freq == 0)
				freq = 1;	// no other threads active so count full penalty
			// if cache miss is actually on an earlier instruction, just back up one to try to improve accuracy
			if (!src && !dst) {
				sample.pc -= 4;
			}
			freq = freq / 60;	// derate frequency by the typical cache miss latency
			mem.hazards(sample.pc, pid) += freq;

			if (iblocked) {
				i_blocked[thread]++;		// time blocked 
				// a real I miss cancels 3 clocks at the miss time, of about 60 clocks blocked
				// a false I cache miss cancels no clocks at the miss time (because they are already cancelled by the hazard)
				// when data is returned from a miss there can be 3 cancelled instructions due to an I fetch
				mem.iblocked(sample.pc, sample.pid) += rate;
				thread_blocked_i_sum[thread] += rate;

				double hazard = 3 * freq;
				mem.hazclocks(sample.pc, sample.pid) += hazard;
				hazard_count[thread] += hazard;
				haztypecounts[HAZ_IMISS] += hazard;
				hazcounts[HAZ_IMISS] += freq;
				mem.haztype(sample.pc, sample.pid) = HAZ_IMISS;
			}
			if (dblocked) {
				d_blocked[thread]++;
				// a d miss cancels 7 clocks at the miss time, of about 60 clocks blocked
				// when data is returned there can be an additional 5 clocks due to another D fetch
				mem.dblocked(sample.pc, sample.pid) += rate;
				thread_blocked_d_sum[thread] += rate;

				double hazard = rethazards[chip_type][1] * freq;
				mem.hazclocks(sample.pc, sample.pid) += hazard;
				hazard_count[thread] += hazard;
				haztypecounts[HAZ_DMISS] += hazard;
				hazcounts[HAZ_DMISS] += freq;
				mem.haztype(sample.pc, sample.pid) = HAZ_DMISS;
			}
			if (nrt_unblocked) {
				if (type[thread] == T_HRT) {
					if (iblocked)
						mem.iblocked(sample.pc, sample.pid) += thread_rate[thread];
					if (dblocked)
						mem.dblocked(sample.pc, sample.pid) += thread_rate[thread];
				}
				return;	// using no clocks here - blocked and another thread can execute.  no more stats
			}
		}

		if (!inpram(sample.pc)) {
			flash_count[thread]++;
		}

		// total of gprof / sample_count[0] is MIPS+hazards fraction of total time

		if (type[thread] != T_HRT) {
			call_graph(sample, rate);	// figure out callers
		}

		if (thread_blocked) {
			return;			// don't count inst or hazards while blocked
		}

		// remove extra counts of this instruction due to hazards counted at this instruction, so all instruction counts have equal weight
		if (mem.targetcount(sample.pc, sample.pid) > 0) {
			double count = mem.targetcount(sample.pc, sample.pid);
			double hazard = mem.targethazard(sample.pc, sample.pid);
			double factor = count / (count + hazard);
			if (factor < 1/8.)
				factor = 1/8.;
			freq *= factor;
		}
		// instruction frequencies, hazards, etc

		// instruction frequencies for NRT threads
		if (idata[inst_type].illegal) {
			illegal_inst = true;
			illegal_address = sample.pc;
			illegal_value = mem.pmem(sample.pc, pid);
			illegal_pid = pid;
		}
		icount[inst_type] += freq;
		if (inst_type == INST_JMP && type[thread] != T_HRT) {	//no condition code for HRT threads
			int taken = decode_condition(mem.pmem(sample.pc, pid), sample.cond_codes);
			int pbit = (mem.pmem(sample.pc, pid) >> 22) & 1;
			jmpcounts[taken * 2 + pbit] += freq;
			int dist = mem.pmem(sample.pc, pid) & 0x1fffff;
			if (dist <= 3) {
				jmpfwdcounts[dist] += freq;
			}
		}
		if (inst_type == INST_MULU4 || inst_type == INST_MULS4) {
			unsigned int next_inst = mem.pmem(sample.pc + 4, pid);
			int accumulator = (mem.pmem(sample.pc, pid) >> 16) & 1;
			int next_type = decode_instruction(next_inst);
			if (next_type == INST_ADD4 && (!accumulator && (next_inst & 0x7fe) == 0x128 || accumulator && (next_inst & 0x7fe) == 0x136)) {
				mac4count += freq;
			}
		}
		int srcan = (mem.pmem(sample.pc, pid) >> 5) & 0x7;
		int dstan = (mem.pmem(sample.pc, pid) >> 21) & 0x7;
		if (src && dst) {
			if (inst_type == INST_MOVE4 || inst_type == INST_MOVE2 || inst_type == INST_MOVE1 || inst_type == INST_MOVEA ||
				inst_type == INST_EXT1 || inst_type == INST_EXT2) {
				both_move_count_thread[thread] += freq;
				both_move_count += freq;
			}
			both_count += freq;
			both_count_thread[thread] += freq;
			if (srcan == 7) {
				stack_count += freq;
				stack_count_thread[thread] += freq;
			}
			if (dstan == 7) {
				stack_count += freq;
				stack_count_thread[thread] += freq;
			}
		} else if (src) {
			if (inst_type == INST_MOVE4 || inst_type == INST_MOVE2 || inst_type == INST_MOVE1 || inst_type == INST_MOVEA ||
				inst_type == INST_EXT1 || inst_type == INST_EXT2) {
				src_move_count_thread[thread] += freq;
				src_move_count += freq;
			}
			src_count += freq;
			src_count_thread[thread] += freq;
			if (srcan == 7) {
				stack_count += freq;
				stack_count_thread[thread] += freq;
			}
		} else if (dst) {
			if (inst_type == INST_MOVE4 || inst_type == INST_MOVE2 || inst_type == INST_MOVE1 || inst_type == INST_MOVEA ||
				inst_type == INST_EXT1 || inst_type == INST_EXT2) {
				dst_move_count_thread[thread] += freq;
				dst_move_count += freq;
			}
			dst_count += freq;
			dst_count_thread[thread] += freq;
			if (dstan == 7) {
				stack_count += freq;
				stack_count_thread[thread] += freq;
			}
		}
		if (inst_type == INST_JMP) {
			jmp_count_thread[thread] += freq;
		} else if (inst_type == INST_CALL) {
			call_count_thread[thread] += freq;
		} else if (inst_type == INST_RET) {
			ret_count_thread[thread] += freq;
		} else if (inst_type == INST_CALLI) {
			calli_count_thread[thread] += freq;
		} else if (inst_type == INST_CALLI) {
			calli_count_thread[thread] += freq;
		} else if (idata[inst_type].acc) {
			dsp_count_thread[thread] += freq;
		}
		itotal_all += freq;
		inst_counted[thread] += freq;

		if (type[thread] != T_GDB && type[thread] != T_HRT) {	// no hazards in flash code, hrt, or profiler
			do_hazards(thread, sample, inst_type, freq);
		}
	}
}

void get_src_dst(unsigned int inst, int inst_type, int &ansrc, int &andst, int &dnsrc, int &dndst)
{
	if (idata[inst_type].dst && ((inst >> 24) & 0x7) > 1) {	// memory destination
		andst = 32 + ((inst >> 21) & 0x7);
		if (((inst >> 24) & 0x7) == 3) {
			dndst = (inst >> 16) & 0xf;
		}
	}

	if ((idata[inst_type].src || inst_type == INST_LEA1 || inst_type == INST_LEA2 || inst_type == INST_LEA4 || inst_type == INST_PDEC) && 
		((inst >> 8) & 0x7) > 1) {	// memory source
		ansrc = 32 + ((inst >> 5) & 0x7);
		if (((inst >> 8) & 0x7) == 3) {
			dnsrc = inst & 0xf;
		}
	}

	if (inst_type == INST_MOVEA && 
		((inst >> 24) & 7) == 1 && ((inst >> 16) & 0xf8) == 0x20 &&	// destination An
		((inst >> 8) & 7) == 1 && (inst & 0xff) < 16) {			// source Dn
			dnsrc = inst & 0xff;
	}
	
	if (inst_type == INST_CALLI) {	// calli need an An early
		ansrc = 32 + ((inst >> 5) & 7);
	}
}

/*
 * calculate hazard penalties based on the instruction executing
 * adjust penalty for the actual number of active threads.
 * freq is adjusted for oversampling of some instructions due to hazards, giving the actual frequency of this instruction.
 * (so sum of all freq is less that total time, since doesn;t include hazard time at all)
 * jump and call targets are oversampled.  An hazard oversamples the instruction taking the hazard
 * remember that the profile thread affects the oversampling
 */
void profile::do_hazards(int thread, struct profile_sample sample, const int inst_type, const double freq)
{
	unsigned int pid = sample.pid;
	double acount = 0.0;	// how many threads are active and not blocked, partial counting hrt threads
	itotal_nrt += freq;
	for (int t = 0; t < NUMTHREADS; ++t) {
		if (type[t] != T_GDB && (sample.active & (1 << t)) && !(sample.i_blocked & (1 << t)) && !(sample.d_blocked & (1 << t))) {
			acount += thread_rate[t];
		}
	}

	int bcount = (int)acount;		// base threads for hazards
	double rcount = acount - bcount;	// remaining fraction at next higher value
	// jmp hazards
	if (inst_type == INST_JMPT || inst_type == INST_NOPJMPTF || inst_type == INST_NOPJMPTT && ((mem.pmem(sample.pc, pid) >> 22) & 1)) {	// unconditional jmp hazards
		int pbit = inst_type == INST_JMPT | inst_type == INST_NOPJMPTT;
		int haztype = HAZ_JMP;
		if (inst_type == INST_NOPJMPTT)
			haztype = HAZ_JMPTT;
		if (inst_type == INST_NOPJMPTF)
			haztype = HAZ_JMPTF;
		mem.hazards(sample.pc, sample.pid) += freq;
		double hazard = 
			jmphazards[chip_type][2 + pbit][bcount] * (1 - rcount) +
			jmphazards[chip_type][2 + pbit][bcount + 1] * rcount;
		hazard *= freq;
		// hazard overcounting for target is less than actual hazard since profiler thread runs during sampling
		double targ_hazard = 
			jmphazards[chip_type][2 + pbit][bcount + 1] * (1 - rcount) +
			jmphazards[chip_type][2 + pbit][bcount + 2] * rcount;
		targ_hazard *= freq;

		mem.hazclocks(sample.pc, sample.pid) += hazard;
		mem.haztype(sample.pc, sample.pid) = haztype;
		hazard_count[thread] += hazard;
		if (!pbit) {
			mem.mispredicts(sample.pc, sample.pid) += freq;
		}
		mem.taken(sample.pc, sample.pid) += freq;
		haztypecounts[haztype] += hazard;
		hazcounts[haztype] += freq;
		unsigned int target = sample.pc + offset21(mem.pmem(sample.pc, pid));
		mem.targetcount(target, sample.pid) += freq;
		mem.targethazard(target, sample.pid) += targ_hazard;
	} else if (inst_type == INST_JMP) {		// conditional jmp
		int taken = decode_condition(mem.pmem(sample.pc, pid), sample.cond_codes);
		if (mem.pmem(sample.pc, pid) & 0x100000) {
			jmpdirection[2 + taken] += freq;	// backward
		} else {
			jmpdirection[taken] += freq;	// forward
		}
		int pbit = (mem.pmem(sample.pc, pid) >> 22) & 1;
		mem.hazards(sample.pc, sample.pid) += freq;
		double hazard = 
			jmphazards[chip_type][taken * 2 + pbit][bcount] * (1 - rcount) +
			jmphazards[chip_type][taken * 2 + pbit][bcount + 1] * rcount;
		hazard *= freq;
		// hazard overcounting for target is less than actual hazard since profiler thread runs during sampling
		double targ_hazard = 
			jmphazards[chip_type][taken * 2 + pbit][bcount + 1] * (1 - rcount) +
			jmphazards[chip_type][taken * 2 + pbit][bcount + 2] * rcount;
		targ_hazard *= freq;
		mem.hazclocks(sample.pc, sample.pid) += hazard;
		mem.haztype(sample.pc, sample.pid) = HAZ_MISSPREDJMP;
		hazard_count[thread] += hazard;

		double p[40], t[40], h[40];
		for (int i = 0; i < 40; ++i) {
			unsigned int pc = sample.pc - 38 * 4 + 4 * i;
			p[i] = mem.gprof(pc, sample.pid);
			t[i] = mem.targetcount(pc, sample.pid);
			h[i] = mem.targethazard(pc, sample.pid);
		}

		if (pbit && !taken || !pbit && taken) {	// mispredicted
			mem.mispredicts(sample.pc, sample.pid) += freq;
		}
		if (taken) {
			mem.taken(sample.pc, sample.pid) += freq;
			unsigned int target = sample.pc + offset21(mem.pmem(sample.pc, pid));
			mem.targetcount(target, sample.pid) += freq;
			mem.targethazard(target, sample.pid) += targ_hazard;
		} else if (!pbit) {
			mem.targetcount(sample.pc + 4, sample.pid) += freq;
			mem.targethazard(sample.pc + 4, sample.pid) += targ_hazard;
		}
		haztypecounts[HAZ_MISSPREDJMP] += hazard;
		hazcounts[HAZ_MISSPREDJMP] += freq;
	} else if (inst_type == INST_CALL) {
		mem.hazards(sample.pc, sample.pid) += freq;
		double hazard = callhazards[bcount] * (1 - rcount) +
			callhazards[bcount + 1] * rcount;
		hazard *= freq;
		double targ_hazard = callhazards[bcount + 1] * (1 - rcount) +
			callhazards[bcount + 2] * rcount;
		targ_hazard *= freq;
		mem.hazclocks(sample.pc, sample.pid) += hazard;
		mem.haztype(sample.pc, sample.pid) = HAZ_CALL;
		hazcounts[HAZ_CALL] += freq;
		hazard_count[thread] += hazard;
		haztypecounts[HAZ_CALL] += hazard;
		// put in target counts for both the call target and the ret target (use calli, not ret)
		unsigned int target = sample.pc + offset24(mem.pmem(sample.pc, pid));
		mem.targetcount(target, sample.pid) += freq;
		mem.targethazard(target, sample.pid) += targ_hazard;
		mem.targetcount(sample.pc + 4, sample.pid) += freq;
		mem.targethazard(sample.pc + 4, sample.pid) += callihazards[bcount + 1] * freq;
	} else if (inst_type == INST_CALLI) {
		mem.hazards(sample.pc, sample.pid) += freq;
		double hazard = callihazards[bcount] * (1 - rcount) +
			callihazards[bcount + 1] * rcount;
		hazard *= freq;
		mem.hazclocks(sample.pc, sample.pid) += hazard;
		mem.haztype(sample.pc, sample.pid) = HAZ_CALLI;
		hazcounts[HAZ_CALLI] += freq;
		hazard_count[thread] += hazard;
		haztypecounts[HAZ_CALLI] += hazard;
	} else if (inst_type == INST_RET) {
		mem.hazards(sample.pc, sample.pid) += freq;
		double hazard = rethazards[chip_type][bcount] * (1 - rcount) +
			rethazards[chip_type][bcount + 1] * rcount;
		hazard *= freq;
		mem.hazclocks(sample.pc, sample.pid) += hazard;
		mem.haztype(sample.pc, sample.pid) = HAZ_RET;
		hazcounts[HAZ_RET] += freq;
		hazard_count[thread] += hazard;
		haztypecounts[HAZ_RET] += hazard;
	}
	// possible mac register hazard.  this inst targets an accumulator
	if (idata[inst_type].acc) {
		unsigned int pc = sample.pc;
		int accumulator;
		if (inst_type == INST_CRCGEN) {
			accumulator = 0;
		} else {
			accumulator = (mem.pmem(pc, pid) >> 16) & 1;
		}
		// TODO: bcount not right here
		for (int window = 0; window < macwindow[chip_type][bcount]; window++) {
			pc += 4;	
			int ntype = decode_instruction(mem.pmem(pc, pid));
			if (ntype == INST_CALL || ntype == INST_RET ||
				ntype == INST_CALLI || ntype == INST_JMPT ||
				ntype == INST_NOPJMPTF || ntype == INST_NOPJMPTT) {
				break;
			}
			if (ntype == INST_JMP && ((mem.pmem(pc, pid) >> 22) & 1)) {
				break;  // predicted taken jmp
			}
			if (mem.haztype(pc, sample.pid) == HAZ_AREG) {
				break;	// prior or simultaneous areg hazard
			}
			if (!idata[ntype].src) {
				continue;
			}
			if (accumulator == 0 && ((mem.pmem(pc, pid) & 0x7fe) == 0x128 ||
				(mem.pmem(pc, pid) & 0x7ff) == 0x12a) ||		// MAC_RC16
				accumulator == 1 && (mem.pmem(pc, pid) & 0x7fe) == 0x136) {	// hazard
				mem.hazards(pc, sample.pid) += freq;
				double hazard = machazards[bcount] * (1 - rcount) + machazards[bcount + 1] * rcount;
				hazard *= freq;
				double targ_hazard = machazards[bcount + 1] * (1 - rcount) + machazards[bcount + 2] * rcount;
				targ_hazard *= freq;
				mem.hazclocks(pc, sample.pid) += hazard;  // count hazard at the use
				mem.haztype(pc, sample.pid) = HAZ_MAC;
				hazcounts[HAZ_MAC] += freq;  
				hazard_count[thread] += hazard;
				haztypecounts[HAZ_MAC] += hazard;  
				mem.targetcount(pc, sample.pid) += freq;
				mem.targethazard(pc, sample.pid) += targ_hazard;
				break;
			}
		}
	}

	// find the (up to four) An and Dn address sources in the instruction
	int ansrc = -1;
	int andst = -1;
	int dnsrc = -1;
	int dndst = -1;
	unsigned int pc = sample.pc;
	get_src_dst(mem.pmem(pc, pid), inst_type, ansrc, andst, dnsrc, dndst);

	// search for an instruction that can cause a hazard
	if (ansrc + dnsrc + andst + dndst != -4) {
	    for (int window = 0; window < anwindow[chip_type][bcount]; window++) {
		pc -= 4;
		int ntype = decode_instruction(mem.pmem(pc, pid));
		if (ntype == INST_CALL || ntype == INST_RET || ntype == INST_CALLI ||
			ntype == INST_JMPT ||
			ntype == INST_NOPJMPTF || ntype == INST_NOPJMPTT) {
			break;		// control flow break
		}
		if (ntype == INST_JMP && ((mem.pmem(pc, pid) >> 22) & 1)) {
			break;		// predicted taken jmp, too slow for hazard if not taken
		}
		// previous address use of same source will take the hazard, so remove that cause from hazard search
		int pansrc;
		int pandst;
		int pdnsrc;
		int pdndst;
		get_src_dst(mem.pmem(pc, pid), ntype, pansrc, pandst, pdnsrc, pdndst);
		if (pansrc == ansrc)
			ansrc = -1;
		if (pdnsrc == dnsrc)
			dnsrc = -1;
		if (pandst == andst)
			andst = -1;
		if (pdndst == dndst)
			dndst = -1;
		if (ansrc + dnsrc + andst + dndst == -4)
			break;		// no possible causes of hazard left, done

		if ((ntype == INST_PDEC || ntype == INST_LEA1 ||	// fast path to An can't cause hazard 
			ntype == INST_LEA2 || ntype == INST_LEA4 ||
			ntype == INST_MOVEA) &&
			((mem.pmem(pc, pid) >> 16) & 0x7f8) == 0x120) {	// An target
				continue;
		}
		if (((mem.pmem(pc, pid) >> 27) & 0x1f) == 2 && ((mem.pmem(pc, pid) >> 21) & 0x1f) != 6) {	// instructions that always target a Dn
			int target = (mem.pmem(pc, pid) >> 16) & 0xf;
			if (target != dnsrc && target != dndst)
				continue;
		} else if (idata[ntype].dst) {	// general destination
			if (((mem.pmem(pc, pid) >> 24) & 7) != 1)
				continue;		// destination not a register
			int target = (mem.pmem(pc, pid) >> 16) & 0xff;
			if (target != ansrc && target != dnsrc && target != andst && target != dndst)
				continue;
		} else {
			continue;			// instructions without any destination
		}

		// found a hazard
		double hazard = areghazards[bcount] * (1 - rcount) + areghazards[bcount + 1] * rcount;
		hazard *= freq;
		double targ_hazard = areghazards[bcount + 1] * (1 - rcount) + areghazards[bcount + 2] * rcount;
		targ_hazard *= freq;
		if (window >= anwindow[chip_type][bcount + 1]) {
			targ_hazard = 0;
		}
		if (inst_type != INST_CALLI) {		// don't double count the instruction.  calli always has a hazard already
			mem.hazards(sample.pc, sample.pid) += freq;
			mem.haztype(sample.pc, sample.pid) = HAZ_AREG;
		}
		mem.hazclocks(sample.pc, sample.pid) += hazard;
		hazcounts[HAZ_AREG] += freq;
		if (ntype == INST_MOVEI) {
			moveihazcount += freq;
		}
		hazard_count[thread] += hazard;
		haztypecounts[HAZ_AREG] += hazard;
		mem.targetcount(sample.pc, sample.pid) += freq;
		mem.targethazard(sample.pc, sample.pid) += targ_hazard;
		adrhazcounts[window] += freq;

		break;
	    }
	}
}

void profile::parse_maps(char *buf, int size, int last_packet)
{
	struct profile_header_maps hdr = *(struct profile_header_maps *)buf;
	buf += sizeof(struct profile_header_maps);
	hdr.count = ntohs(hdr.count);
	page_shift = ntohl(hdr.page_shift);
	for (int i = 0; i < hdr.count && num_pm < PROFILE_NUM_MAPS; ++i) {
		pm[num_pm] = *(struct profile_map *)buf;
		pm[num_pm].start = ntohs(pm[num_pm].start);
		pm[num_pm].type_size = ntohs(pm[num_pm].type_size);
		buf += sizeof(struct profile_map);
		num_pm++;
	}
	if (last_packet) {
		getting_map = false;
	}
}

void profile::parse_counters(char *buf, int size)
{
	struct profile_header_counters hdr = *(struct profile_header_counters *)buf;
	buf += sizeof(struct profile_header_counters);
	stat_packet_count++;
	int ultra_count = ntohs(hdr.ultra_count);
	stats_count = ultra_count + ntohl(hdr.linux_count);
	if (stats_count > PROFILE_MAX_COUNTERS) {
		error = ERROR_BAD_COUNT;
		stats_count = 0;
		return;
	}
	struct profile_counter_pkt *counter = (struct profile_counter_pkt *)buf;
	for (int i = 0; i < stats_count; ++i) {
		stats_val[i] = ntohl(counter->value);
		strcpy(stats_string[i], counter->name);
		if (i < ultra_count)
			stats_time[i] = ntohl(hdr.ultra_sample_time);
		else 
			stats_time[i] = ntohl(hdr.linux_sample_time);
		buf += sizeof(struct profile_counter_pkt);
		counter++;
	}
}

void profile::on_connect(void)
{
	/* 
	 * tell board to send the memory map on the control connection
	 */
	if (console_send("map\n")) {
		console_state = CONSOLE_MAPPING;
	}
}

/*
 * got a packet on the control connection
 */
void profile::on_receive(const char *buf, const char *ip_address)
{
	char sending[100];
	if (console_state == CONSOLE_MAPPING) {
		/*
		 * parse map data until "done" or an error
		 */
		if (!mem.parse_map((char *)buf, &pid_pending)) {
			/*
			 * send my IP address so the board can send data on the data connection 
			 */
			sprintf(sending, "ip %s\n", ip_address);
			console_send(sending);
			/*
			 * tell board to start sending packets on data connection 
			 */
			console_send("start\n");
			console_state = CONSOLE_IDLE;
			pid_pending = -1;
		}
	} else {
		console_state = CONSOLE_PID;
		if (!mem.parse_map((char *)buf, &pid_pending)) {
			// update map links to segments
			console_state = CONSOLE_IDLE;
			pid_pending = -1;
		}
	}
}

/*
 * got a data packet from ip5k/7k/8k
 * these are UDP packets on the data connection, with binary data
 * defined in profpkt.h
 * validate it and convert byte order
 */
void profile::on_packet(char *buf, unsigned int size)
{
	if (start_time == 0) {
		return;		// ignore packets before first interval starts
	}
	if (elf_file_count == 0) {
		return;		// ignore if we don't have an elf file
	}
	last_packet_time = time(NULL);
	// check the magic number and packet version
	// parse the packet for each sample
	struct profile_header header = *(struct profile_header *)buf;
	header.magic = ntohs(header.magic);
	int magic = header.magic & 0xffe0;
	receive_version = header.magic & 0x1f;

	if (magic == PROF_MAGIC_MAPS) {
		parse_maps(buf, size, receive_version);
		return;
	}

	if (magic == PROF_MAGIC_COUNTERS) {
		parse_counters(buf, size);
		return;
	}

	if (magic != PROF_MAGIC) {
		error = ERROR_BAD_MAGIC;
		return;
	}

	if (receive_version != PROFILE_VERSION) {
		error = ERROR_BAD_VERSION;
		if (receive_version < 17) {
			return;		// version 4-12 are forward compatible with each other and 13 and above are forward compatible, ver 16 breaks compatibility due to thread count change.  ver 17 breaks compatibility on sample format.
		}
	} 

	if (header.sample_count < 1 || header.sample_count * sizeof(struct profile_sample) + header.header_size > size) {
		error = ERROR_BAD_COUNT;
		return;
	}

	unsigned int seq = ntohl(header.seq_num);
	if (last_seq != 0 && receive_version >= 7 && packet_count > 20) {
		dropped_packets += seq - last_seq - 1;
	}
	last_seq = seq;

	header.clocks = ntohl(header.clocks);
	if (current_clock == 0) {
		current_clock = header.clocks;
		for (int i = 0; i < NUMTHREADS; ++i) {
			current_inst_count[i] = ntohl(header.instruction_count[i]);
		}
		for (int i = 0; i < 4; ++i) {
			current_perf_counters[i] = ntohl(header.perf_counters[i]);
		}
		return;		// wait until next sample for consistent data
	}

	packet_count++;
	log_packet_count++;	
	
	total_clocks += (header.clocks - current_clock);
	unsigned int curr_clocks = (header.clocks - current_clock);
	current_clock = header.clocks;

	cpu_id = ntohl(header.cpu_id);
	chip_type = parse_chip_type(cpu_id);
	profile_count += ntohl(header.profile_instructions);

	unsigned int curr_inst = 0;
	for (int i = 0; i < NUMTHREADS; ++i) {
		header.instruction_count[i] = ntohl(header.instruction_count[i]);
		instruction_count[i] += (header.instruction_count[i] - current_inst_count[i]);
		curr_inst += (header.instruction_count[i] - current_inst_count[i]);
		current_inst_count[i] = header.instruction_count[i];
		header.min_sp[i] = ntohl(header.min_sp[i]);
		if (header.min_sp[i] < min_sp_val[i]) {
			min_sp_val[i] = header.min_sp[i];
		}
	}

	header.enabled = ntohs(header.enabled);
	header.hrt = ntohs(header.hrt);
	header.high = ntohs(header.high);
	header.profiler_thread = ntohs(header.profiler_thread);

	for (int t = 0; t < NUMTHREADS; ++t) {
		if (!(header.enabled & (1 << t))) {
			continue;
		}
		// type of thread
		if (type[t] != T_HRT && (header.hrt & (1 << t))) {
			type[t] = T_HRT;
		}
		if (type[t] < T_HIGH && (header.high & (1 << t))) {
			type[t] = T_HIGH;
			thread_rate[t] = 1.0;
		}
		if (type[t] < T_NRT) {
			type[t] = T_NRT;
			thread_rate[t] = 1.0;
		}
		if (t == header.profiler_thread) {
			type[t] = T_GDB;
			sw_type[t] = SWT_GDB;
		}
	}

	avecnt++;

	header.clock_freq = ntohl(header.clock_freq);
	clock_rate = header.clock_freq / 1000000.;

	for (int i = 0; i < PROFILE_COUNTERS; ++i) {
		header.perf_counters[i] = ntohl(header.perf_counters[i]);
	}

	if (curr_clocks != 0) {
		current_mips = (clock_rate * curr_inst / curr_clocks);
		d_access_per_sec = clock_rate * (header.perf_counters[2] - current_perf_counters[2]) / curr_clocks;
		d_miss_per_sec = clock_rate * (header.perf_counters[3] - current_perf_counters[3]) / curr_clocks;
		i_miss_per_sec = clock_rate * (header.perf_counters[1] - current_perf_counters[1]) / curr_clocks;
		d_tlb_miss_per_sec = clock_rate * (header.perf_counters[5] - current_perf_counters[5]) / curr_clocks;	// total misses
		i_tlb_miss_per_sec = clock_rate * (header.perf_counters[4] - current_perf_counters[4]) / curr_clocks;
		d_tlb_miss_per_sec -= i_tlb_miss_per_sec;
		ptec_miss_per_sec = clock_rate * (header.perf_counters[6] - current_perf_counters[6]) / curr_clocks;
	}

	if (d_access_per_sec > max_d_access_per_sec) {
		max_d_access_per_sec = d_access_per_sec;
	}
	if (d_miss_per_sec > max_d_miss_per_sec) {
		max_d_miss_per_sec = d_miss_per_sec;
	}
	for (int i = 0; i < PROFILE_COUNTERS; ++i) {
		perf_counters[i] += header.perf_counters[i] - current_perf_counters[i];
		current_perf_counters[i] = header.perf_counters[i];
	}
	ddr_freq = ntohl(header.ddr_freq);

	struct profile_sample sample;
	for (int i = 0; i < header.sample_count; ++i) {
		sample = *(struct profile_sample *)(buf+header.header_size+i*sizeof(struct profile_sample));
		sample.a5 = ntohl(sample.a5);
		sample.pc = ntohl(sample.pc);
		sample.pid = ntohl(sample.pid);
		sample.parent = ntohl(sample.parent);
		sample.active = ntohs(sample.active);
		sample.d_blocked = ntohs(sample.d_blocked);
		sample.i_blocked = ntohs(sample.i_blocked);
		on_sample(sample);
	}
}

bool profile::save_gmon_out()
{
	FILE *f = fopen("gmon.out", "wb");
	if (f == NULL) {
		return false;
	}

	/* write the header */
	fwrite("gmon", 1, 4, f);
	char v[16];
	for (unsigned int i = 0; i < 16; ++i) {
		v[i] = 0;
	}
	v[3] = 1;
	fwrite(v, 1, 16, f);

	/* write pc sample data */
	mem.write_histograms(f, samples_per_second);

	/* write the call graph data */
	unsigned char tag = 1;
	struct call_arc cgout;
	for (unsigned int i = 0; i < next_cg; ++i) {
		fwrite(&tag, 1, 1, f);	/* histogram record next */
		cgout.parent = ntohl(cg[i].parent);
		cgout.self = ntohl(cg[i].self);
		cgout.count = ntohl((int)cg[i].count);
		fwrite(&cgout, sizeof(struct call_arc), 1, f);
	}
	fclose(f);
	return true;
}

#define NM_SIZE 100

/*
 * read_posix_elf_file
 *	read text sections and symbols for vmlinux, an application, or a kernel loadable module
 */
int profile::read_posix_elf_file(const char *lpszPathName)
{
	struct elf_file_instance *elf_file;
	char error[500];
	char app_name[500];
	int good_section[MAX_SECTIONS];
	FILE *f;
	unsigned int section_offset = 0;	// for ddr sections in KLM, since all have same segment start address */

	if ((f = fopen(lpszPathName, "rb")) == NULL) {
		return ELF_ERROR_OPEN;
	}
	struct TELF32_HDR elf_header;
	fread(&elf_header, 1, sizeof(elf_header), f);
	if ((elf_header.e_ident[0] != 0x7F) ||
		(elf_header.e_ident[1] != 'E') ||
		(elf_header.e_ident[2] != 'L') ||
		(elf_header.e_ident[3] != 'F')) {
			return ELF_ERROR_BAD_FORMAT;
	}
	fclose(f);
	elf_header.e_type = ntohs(elf_header.e_type);
	if (!(elf_file = elf_open_file(lpszPathName))) {
		return ELF_ERROR_STRIPPED;
	}

	/* get the section header */
	TELF32_SHDR section;
	if (elf_find_section(elf_file, ".text", &section) == -1) {
		elf_close_file(elf_file);
		return ELF_ERROR_SECTION;
	}

	// read the text section(s)
	int count = elf_get_section_count(elf_file);
	if (count <= 0) {
		elf_close_file(elf_file);
		return ELF_ERROR_SECTION;
	}
	if (count > MAX_SECTIONS) {
		sprintf(error, "Too many sections.  Is this a valid file? %s", lpszPathName);
		fatal_error(error);
		elf_close_file(elf_file);
		return ELF_ERROR_SECTION;
	}

	// find the name of the application (between the last \ and next .)
	const char *start = strrchr(lpszPathName, '\\');
	if (start == NULL) {
		start = lpszPathName;
	} else {
		start++;
	}
	strcpy(app_name, start);
	strcat(app_name, ":");
	for (int snum = 0; snum < count && snum < MAX_SECTIONS; ++snum) {
		char name[NM_SIZE];
		good_section[snum] = FALSE;
		if (elf_get_section(elf_file, snum, &section, name, NM_SIZE) == -1) {
			break;
		}

		// find the code sections
		if (!(section.sh_flags & ELF_SHF_EXEC)) {
			continue;
		}
		// ignore the linux kernel or KLM init section since this code is thrown away after boot
		if (strncmp(name, ".init", 5) == 0) {
			continue;
		}
		// kernel should be configured to never unload modules  TODO: look at actual value of CONFIG_MODULE_UNLOAD
		if (strncmp(name, ".exit", 5) == 0) {
			continue;
		}
		// empty section
		if (section.sh_size == 0) {
			continue;
		}
		unsigned int *data = (unsigned int *)malloc(section.sh_size);
		if (data == 0) {
			fatal_error("Can't allocate data");
			elf_close_file(elf_file);
			return ELF_ERROR_MEMORY;
		}
		unsigned long length = section.sh_size;
		memset(data, 0, section.sh_size);
		if (elf_header.e_type == ELF_TYPE_EXEC || elf_header.e_type == ELF_TYPE_DYN) {
			elf_get_data_by_run_address(elf_file, section.sh_addr, &length, ((unsigned char *)data));
		} else {
			elf_get_data_from_section(elf_file, snum, &length, ((unsigned char *)data));
		}

		if (length != section.sh_size) {
			free(data);
			elf_close_file(elf_file);
			sprintf(error, "Elf section %s size mismatch %d != %d", name, length, section.sh_size);
			fatal_error(error);
			return ELF_ERROR_SECTION;
		}

		for (int i = 0; i < section.sh_size/4; ++i) {
			data[i] = ntohl(data[i]);	/* fix the byte order */
		}
		int type = MEM_TYPE_LINUX;
		if (elf_header.e_type == ELF_TYPE_DYN) {
			type = MEM_TYPE_DLL;
		} else if (elf_header.e_type == ELF_TYPE_REL) {
			type = MEM_TYPE_KLM;	
		} else if (section.sh_addr < 0x3fff0000) {
			type = MEM_TYPE_EXEC;
		}
		char seg_name[1000];
		sprintf(seg_name, "%s%s", app_name, name);
		memblock *m = 0;
		if (elf_header.e_type == ELF_TYPE_REL && !strstr(name, "ocm_text")) {
			m = mem.add_seg(section.sh_addr + section_offset, section.sh_size, data, seg_name, type);
			section_offset += length;
		} else {
			m = mem.add_seg(section.sh_addr, section.sh_size, data, seg_name, type);
		}
		if (type == MEM_TYPE_LINUX) {
			mem.add_memory_seg(0, m, section.sh_addr, section.sh_addr + section.sh_size, name);	// add an unmapped segment
		}
		if (strcmp(name, ".text") == 0 || strcmp(name, ".ocm_text") == 0) {
			if (m && !m->add_symbol_table(elf_file, app_name, snum)) {
				return ELF_ERROR_MEMORY;
			}
		}
		good_section[snum] = TRUE;
		if (strcmp(name, ".text") == 0 && section.sh_addr >= 0x40000000) {	/* first unmapped .text segment should be vmlinux */
			if (linux_start == 0 || section.sh_addr < linux_start) {
				linux_start = section.sh_addr;
			}
			if (linux_end == 0 || linux_end < section.sh_addr + section.sh_size) {
				linux_end = section.sh_addr + section.sh_size;
			}
		}
	}

	elf_close_file(elf_file);
	return ELF_OK;
}

/*
 * read_elf_file
 *	read the Ultra elf file
 */
int profile::read_elf_file(const char *lpszPathName)
{
	struct elf_file_instance *elf_file;
	char error[500];

	if (!(elf_file = elf_open_file(lpszPathName))) {
		sprintf(error, "Can't open elf file: %s", lpszPathName);
		fatal_error(error);
		return ELF_ERROR_OPEN;
	}

	TELF32_SHDR section;
	elf_get_symbol_value(elf_file, "__ocm_begin", &ocm_begin);
	elf_get_symbol_value(elf_file, "__ocm_text_run_end", &ocm_text_run_end);
	elf_get_symbol_value(elf_file, "__ocm_stack_run_begin", &ocm_stack_run_begin);
	elf_get_symbol_value(elf_file, "__ocm_heap_run_begin", &ocm_heap_run_begin);
	elf_get_symbol_value(elf_file, "__ocm_heap_run_end", &ocm_heap_run_end);
	elf_get_symbol_value(elf_file, "__heap_run_begin", &heap_run_begin);
	elf_get_symbol_value(elf_file, "__heap_run_end", &heap_run_end);
	elf_get_symbol_value(elf_file, "__text_run_begin", &text_run_begin);
	elf_get_symbol_value(elf_file, "__text_run_end", &text_run_end);
	elf_get_symbol_value(elf_file, "__os_begin", &os_begin);
	elf_get_symbol_value(elf_file, "__sdram_coredump_limit", &sdram_coredump_limit);

	if (ocm_begin < 0x40000000)
		chip_type = CHIP_7K;
	else
		chip_type = CHIP_8K;

	unsigned long ocm_netpage_run_begin = 0, ocm_netpage_run_end = 0, netpage_run_begin = 0, netpage_run_end = 0;
	elf_get_symbol_value(elf_file, "__ocm_netpage_run_begin", &ocm_netpage_run_begin);
	elf_get_symbol_value(elf_file, "__ocm_netpage_run_end", &ocm_netpage_run_end);
	elf_get_symbol_value(elf_file, "__netpage_run_begin", &netpage_run_begin);
	elf_get_symbol_value(elf_file, "__netpage_run_end", &netpage_run_end);

	elf_get_symbol_value(elf_file, "__sdram_begin", &sdram_begin);
	elf_get_symbol_value(elf_file, "__sdram_end", &sdram_end);
	elf_get_symbol_value(elf_file, "__flash_end", &flash_end);

	unsigned long start_vector;
	if (elf_get_symbol_value(elf_file, "__start_vector", &start_vector) == -1) {
		elf_close_file(elf_file);
		fatal_error("Can't find symbol __start_vector");
		return ELF_ERROR_SYMBOL;
	}

	unsigned long sdram_limit;
	if (elf_get_symbol_value(elf_file, "__sdram_limit", &sdram_limit) == -1) {
		elf_close_file(elf_file);
		fatal_error("Can't find symbol __sdram_limit");
		return ELF_ERROR_SYMBOL;
	}

	unsigned long hrt_table_adr;
	if (elf_get_symbol_value(elf_file, "_system_hrt_table", &hrt_table_adr) == -1) {
		elf_close_file(elf_file);
		fatal_error("Can't find symbol __system_hrt_table");
		return ELF_ERROR_SYMBOL;
	}

	unsigned long hrt_length = 64;
	elf_get_data_by_run_address(elf_file, hrt_table_adr, &hrt_length, ((unsigned char *)hrt_table));
	if (hrt_length != 64) {
		elf_close_file(elf_file);
		fatal_error("HRT table length is not 64.");
		return ELF_ERROR_SYMBOL;
	}

	for (unsigned int i = 0; i < NUMTHREADS; ++i) {
		hrt_count[i] = 0;
	}
	hrt_size = 1;
	for (int i = 0; i < 64; ++i) {
		if (!(hrt_table[i]&0x40)) {
			hrt_count[hrt_table[i]&0x1f]++;
		}
		if (hrt_table[i]&0x80) {
			hrt_size = i+1;
			break;
		}
	}
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (hrt_count[i] == 0) {
			thread_rate[i] = 1.0;
		} else {
			thread_rate[i] = (double)hrt_count[i]/hrt_size;
		}
		hrt_rate[i] = (double)hrt_count[i]/hrt_size;
	}

	// create the memory map for the unmapped region
	mem.add_memory_map(0, "Unmapped memory (Ultra, Kernel, etc)");

	// read all the code sections
	int count = elf_get_section_count(elf_file);
	if (count <= 0) {
		fatal_error("Elf file has no sections");
		return ELF_ERROR_SECTION;
	}
	int text_count = 0;
	for (int snum = 0; snum < count; ++snum) {
		char name[NM_SIZE];
		if (elf_get_section(elf_file, snum, &section, name, NM_SIZE) == -1) {
			break;
		}
		// find the interesting code sections
		if (!(section.sh_flags & ELF_SHF_EXEC)) {
			continue;
		}
		if (strncmp(name, ".upgrade.", 9) == 0) {
			continue;
		}
		if (strncmp(name, ".debug", 6) == 0) {
			continue;
		}
		if (strcmp(name, ".protect") == 0) {
			continue;
		}
		if (strcmp(name, ".flash_entry") == 0) {
			continue;
		}
		if (strcmp(name, ".codeloader") == 0) {
			continue;
		}
		if (strcmp(name, ".downloader") == 0) {
			continue;
		}
		unsigned int *data = (unsigned int *)malloc(section.sh_size);
		if (data == 0) {
			fatal_error("Can't allocate data");
			break;
		}
		unsigned long length = section.sh_size;
		memset(data, 0, section.sh_size);
		elf_get_data_by_run_address(elf_file, section.sh_addr, &length, ((unsigned char *)data));

		if (length != section.sh_size) {
			elf_close_file(elf_file);
			sprintf(error, "Elf section %s size mismatch %d != %d", name, length, section.sh_size);
			fatal_error(error);
			return ELF_ERROR_SECTION;
		}

		for (int i = 0; i < section.sh_size / 4; ++i) {
			data[i] = ntohl(data[i]);	/* fix the byte order */
		}
		char seg_name[1000];
		sprintf(seg_name, "%s:%s", "ultra", name);
		memblock *m = mem.add_seg(section.sh_addr, section.sh_size, data, seg_name, MEM_TYPE_ULTRA);
		mem.add_memory_seg(0, m, section.sh_addr, section.sh_addr + section.sh_size, name);
		if (strcmp(name, ".text") == 0 || strcmp(name, ".ocm_text") == 0) {
			if (m && !m->add_symbol_table(elf_file, "ultra:", snum)) {
				return ELF_ERROR_MEMORY;
			}
		}
		if (strcmp(name, ".text") == 0) {
			ultra_start = section.sh_addr;
			ultra_size = section.sh_size;
		}
		text_count++;
	}
	if (text_count == 0) {
		elf_close_file(elf_file);
		fatal_error("Elf file has no text sections");
		return ELF_ERROR_SECTION;
	}

	elf_close_file(elf_file);

	return ELF_OK;
}

// extract symbols form the symbol table that are relevant for this section and add them to the symbol table for this section
// for absolute sections, the symbol table has addresses in the section.
bool memblock::add_symbol_table(struct elf_file_instance *elf_file, char *name, int section)
{
	int stsize = elf_get_symbol_count(elf_file);
	int len = strlen(name);
	int count = elf_get_section_count(elf_file);

	free(symbol_table);

	// one extra symbol for the error functions
	symbol_table = (struct symbol *)malloc((stsize + 1) * sizeof(struct symbol));
	if (symbol_table == NULL) {
		fatal_error("Can't allocate symbol table");
		elf_close_file(elf_file);
		return false;
	}

	free(strings);
	strings = (char *)malloc((stsize + 1) * MAX_SYMBOL_LENGTH);
	if (strings == NULL) {
		fatal_error("Can't allocate strings");
		elf_close_file(elf_file);
		return false;
	}

	symbol_count = 0;	/* next good symbol entry */
	struct TELF32_SYM sym;
	for (int i = 0; i < (unsigned int)stsize; ++i) {
		// segment name
		strcpy(strings + MAX_SYMBOL_LENGTH * symbol_count, name);
		// append symbol name
		if (elf_get_symbol(elf_file, i, &sym, strings + MAX_SYMBOL_LENGTH * symbol_count + len, MAX_SYMBOL_LENGTH - len) == -1) {
			fatal_error("Not enough symbols in elf file");
			elf_close_file(elf_file);
			return false;
		}
		if (sym.st_shndx != section) {
			continue;
		}
		int type = sym.st_info & 0xf;
		if (type != 0 && type != 2) {
			continue;	/* must be no_type or function */
		}
		if (strncmp(&strings[MAX_SYMBOL_LENGTH * symbol_count + len], "_upgrade", 8) == 0) {
			continue;	// ignore upgrade symbols
		}
		if (strncmp(&strings[MAX_SYMBOL_LENGTH * symbol_count + len], "_binary_downloader", 18) == 0) {
			continue;	// ignore downloader symbols
		}
		if (strncmp(&strings[MAX_SYMBOL_LENGTH * symbol_count + len], "__ocm_text", 10) == 0) {
			continue;	// ignore ocm limit symbols
		}
		if (sym.st_value < init_start_address || sym.st_value > init_start_address + size) {
			continue;
		}
//		if (sym.st_shndx >= count) {
//			continue;	// not a text section symbol
//		}
		if (sym.st_value < init_start_address || sym.st_value > init_start_address + size - 4) {
			continue;	// symbol not in the segment
		}
		symbol_table[symbol_count].st_info = sym.st_info;
		symbol_table[symbol_count].st_other = sym.st_other;
		symbol_table[symbol_count].st_shndx = sym.st_shndx;
		symbol_table[symbol_count].st_value = sym.st_value;
		symbol_table[symbol_count].st_name = &strings[MAX_SYMBOL_LENGTH * symbol_count];	/* pointer to start of string */
		target[(symbol_table[symbol_count].st_value - init_start_address) / 4] = true;	// every text symbol is a potential jmp or call target
		symbol_count++;
		symbol_table[symbol_count].st_value = 0x7fffffff;	// final extra value for later subtacts for function sizes
	}

	// sort symbols in order by address
	qsort(symbol_table, symbol_count, sizeof(struct symbol), compare);

	// fix sizes
	for (int i = 0; i < symbol_count - 1; ++i) {
		symbol_table[i].st_size = symbol_table[i + 1].st_value - symbol_table[i].st_value;
	}
	symbol_table[symbol_count - 1].st_size = init_start_address + size - symbol_table[symbol_count - 1].st_value;
	return true;
}

struct hsort {
	double value;
	memblock *seg;
	unsigned int inst;	// sorted hazard indexes
	unsigned int pad;
};


int haz_compare( const void *arg1, const void *arg2 )
{
	struct hsort *h1, *h2;
	h1 = (struct hsort *)arg1;
	h2 = (struct hsort *)arg2;
	if (h1->value > h2->value) {
		return -1;
	}
	if (h1->value < h2->value) {
		return 1;
	}
	return 0;
}

#define MINHAZCLOCKS 50

bool profile::save_hazards(int count, char *filename, int page)
{
	FILE *f;
	if (filename != NULL) {
		f = fopen(filename, "w");
		if (f == NULL) {
			return false;
		}
	}

	unsigned int size = mem.total_size() / 4;
	struct hsort *haz_sort = (struct hsort *)malloc(size * sizeof(hsort));
	if (haz_sort == 0) {
		fatal_error("Can't allocate haz_sort");
		return false;
	}
	int hazard_count = 0;
	double haz_total = 0.0;
	double haz_total_display = 0.0;
	for (memblock *p = mem.first_segment(); p; p = p->next) {
		for (unsigned int i = 0; i < p->size / 4; i++) {
			haz_sort[hazard_count].value = p->hazclocks[i];
			haz_total += haz_sort[hazard_count].value;
			if (haz_sort[hazard_count].value < MINHAZCLOCKS) {
				continue;
			}
			haz_total_display += haz_sort[hazard_count].value;
			haz_sort[hazard_count].seg = p;
			haz_sort[hazard_count].inst = i;
			hazard_count++;
		}
	}
	if (hazard_count == 0) {
		if (filename == NULL) {
			addline("Insufficient hazard information received.  Wait for more samples.", page);
		} else {
			fprintf(f, "No hazard information received", page);
			fclose(f);
		}
		free(haz_sort);
		return true;
	}
	qsort(haz_sort, hazard_count, sizeof(struct hsort), haz_compare);
	double inst = 0;
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] != T_DISABLED && type[i] != T_HRT && type[i] != T_GDB) {
			inst += instruction_count[i];
		}
	}
	char tmp[1000], out[1000];
	sprintf(out, "Hazard MIPS for top %d instructions with %5.1f hazards of %5.1f total hazards.  Cache hazards are partially on nearby instructions.", 
		hazard_count,
		clock_rate * haz_total_display / itotal_nrt * inst / total_clocks,
		clock_rate * haz_total / itotal_nrt * inst / total_clocks);
	if (filename == NULL) {
		addline("MIPS wasted by an instruction (clocks wasted each execution) hazard details, and the location of the instruction.", page);
		addline(" ", page);
		addline(out, page);
		addline(" ", page);
	} else {
		fprintf(f, "%s\n\n", out);
	}
	for (int i = 0; i < count && i < hazard_count; ++i) {
		if (haz_sort[i].value == 0){
			break;
		}
		memblock *seg = haz_sort[i].seg;
		symbol_address_to_name(seg, haz_sort[i].inst, tmp);
		double hazmips = (double)clock_rate * haz_sort[i].value / itotal_nrt * inst / total_clocks;
		if (hazmips < 0.05)
			break;
		if (seg->haztype[haz_sort[i].inst] == HAZ_MISSPREDJMP) {
			int backward = (haz_sort[i].seg->pmem[haz_sort[i].inst] >> 20) & 1;	// backward direction
			sprintf(out, "%5.2f MIPS (%3.1f clks) %s %6.0f samples %4.0f%% taken %4.0f%% mispredicted %s %s",
				hazmips,
				(double)haz_sort[i].value / seg->hazards[haz_sort[i].inst],
				hazname[seg->haztype[haz_sort[i].inst]],
				seg->gprof[haz_sort[i].inst],
				(100. * seg->taken[haz_sort[i].inst]) / seg->hazards[haz_sort[i].inst],
				(100. * seg->mispredicts[haz_sort[i].inst]) / seg->hazards[haz_sort[i].inst],
				backward?"backward ": "forward  ",
				tmp
				);
		} else {
			sprintf(out, "%5.2f MIPS (%3.1f clks) %s %6.0f samples   %s",
				hazmips,
				(double)haz_sort[i].value / seg->hazards[haz_sort[i].inst],
				hazname[seg->haztype[haz_sort[i].inst]],
				seg->gprof[haz_sort[i].inst],
				tmp
				);
		}
		if (filename == NULL) {
			addline(out, page);
		} else {
			fprintf(f, "%s\n", out);
		}
	}
	free(haz_sort);

	if (filename != NULL) {
		fclose(f);
	}
	return true;
}

struct inst_data idata[INST_COUNT] = {

	/* 32 opcodes - note repeated below*/
	0, 0, 0, 0, 1, "1-2 operand",	//0 - unused - see other table
	0, 0, 0, 0, 1, "Reserved 1",
	1, 0, 1, 0, 0, "shift/mul",
	0, 0, 0, 0, 1, "Reserved 3",
	1, 1, 1, 0, 0, "BSET",
	1, 1, 1, 0, 0, "BCLR",
	0, 0, 0, 0, 1, "DSP ops",
	0, 0, 0, 0, 1, "Reserved 7",
	1, 1, 1, 0, 0, "AND.2",
	1, 1, 1, 0, 0, "AND.4",
	1, 1, 1, 0, 0, "OR.2",			// 10
	1, 1, 1, 0, 0, "OR.4",
	1, 1, 1, 0, 0, "XOR.2",
	1, 1, 1, 0, 0, "XOR.4",
	1, 1, 1, 0, 0, "ADD.2",
	1, 1, 1, 0, 0, "ADD.4",
	1, 1, 1, 0, 0, "ADDC",
	1, 1, 1, 0, 0, "SUB.2",
	1, 1, 1, 0, 0, "SUB.4",
	1, 1, 1, 0, 0, "SUBC",
	1, 1, 1, 0, 0, "PXBLEND",		// 20
	1, 1, 1, 0, 0, "PXVI",
	1, 1, 1, 0, 0, "PXADDS",
	0, 0, 0, 0, 1, "Reserved 23",
	1, 0, 1, 0, 0, "CMPI",
	0, 1, 0, 0, 0, "MOVEI",
	0, 0, 0, 0, 0, "JMPcc",
	0, 0, 0, 0, 0, "CALL",
	0, 0, 0, 0, 0, "MOVEAI",
	0, 0, 0, 0, 0, "MOVEAIH",
	0, 0, 0, 0, 0, "CALLI",			// 30
	0, 0, 0, 0, 1, "Reserved 31",
	/* 32 opcode 0 */
	0, 0, 0, 0, 1, "Reserved 0:0",		
	0, 0, 0, 0, 0, "SUSPEND",
	0, 1, 0, 0, 0, "FLUSH",	// flush
	0, 1, 0, 0, 0, "SYNC",	// sync
	1, 0, 1, 0, 0, "RET",
	0, 1, 0, 0, 0, "PREFETCH",
	0, 0, 0, 0, 0, "IREAD",
	1, 0, 1, 0, 0, "BKPT",
	1, 1, 0, 0, 0, "SYSRET",		// 40
	0, 1, 0, 0, 0, "SYSCALL",
	1, 1, 1, 0, 0, "NOT.4",
	1, 1, 1, 0, 0, "NOT.2",
	1, 1, 1, 0, 0, "MOVE.4",
	1, 1, 1, 0, 0, "MOVE.2",
	1, 1, 1, 0, 0, "MOVEA",
	1, 1, 1, 0, 0, "MOVE.1",
	1, 0, 1, 0, 0, "IWRITE",	
	0, 0, 0, 0, 1, "Reserved 0:17",
	1, 0, 1, 0, 0, "SETCSR",			// 50
	1, 1, 0, 0, 0, "TBSET",
	1, 1, 0, 0, 0, "TBCLR",
	1, 1, 1, 0, 0, "EXT.2",
	0, 0, 0, 0, 1, "Reserved 0:22",
	1, 1, 1, 0, 0, "EXT.1",
	1, 1, 1, 0, 0, "SWAPB.2",
	1, 1, 1, 0, 0, "SWAPB.4",
	1, 1, 1, 0, 0, "PXCNV",
	1, 1, 1, 0, 0, "PXCNV.T",
	0, 1, 1, 0, 0, "LEA.4",			// 60
	0, 1, 1, 0, 0, "LEA.2",   // LEA does not have a source for counting memory operands, but does have a source for hazards.
	0, 1, 1, 0, 0, "PDEC",
	0, 1, 1, 0, 0, "LEA.1",
	/* 32 opcode 2 */
	1, 0, 1, 0, 0, "PXHI",
	1, 0, 1, 1, 0, "MULS",
	1, 0, 1, 0, 0, "PXHI.S",
	1, 0, 1, 1, 0, "MULU",
	0, 0, 0, 0, 1, "Reserved 2:4",
	1, 0, 1, 1, 0, "MULF",
	1, 0, 1, 0, 0, "BTST",			// 70
	0, 0, 0, 0, 1, "Reserved 2:7",
	1, 0, 1, 1, 0, "CRCGEN",
	1, 0, 1, 1, 0, "MAC",
	1, 0, 1, 0, 0, "LSL.1",
	1, 0, 1, 0, 0, "LSR.1",
	1, 0, 1, 0, 0, "ASR.1",
	0, 0, 0, 0, 1, "Reserved 2:13",
	0, 0, 0, 0, 1, "Reserved 2:14",
	0, 0, 0, 0, 1, "Reserved 2:15",
	1, 0, 1, 0, 0, "LSL.4",			//80
	1, 0, 1, 0, 0, "LSL.2",
	1, 0, 1, 0, 0, "LSR.4",
	1, 0, 1, 0, 0, "LSR.2",
	1, 0, 1, 0, 0, "ASR.4",
	1, 0, 1, 0, 0, "ASR.2",
	1, 0, 1, 0, 0, "BFEXTU",
	0, 0, 0, 0, 1, "Reserved 2:23",
	1, 0, 1, 0, 0, "BFRVRS",
	0, 0, 0, 0, 1, "Reserved 2:25",
	1, 0, 1, 0, 0, "SHFTD",			// 90
	0, 0, 0, 0, 1, "Reserved 2:27",
	1, 0, 1, 0, 0, "MERGE",
	0, 0, 0, 0, 1, "Reserved 2:29",
	1, 0, 1, 0, 0, "SHMRG.2",
	1, 0, 1, 0, 0, "SHMRG.1",
	/* nop */
	0, 0, 0, 0, 0, "NOP",
	0, 0, 0, 0, 0, "JMPT",
	0, 0, 0, 0, 0, "NOP JMPT.T",
	0, 0, 0, 0, 0, "NOP JMPT.F",
	/* 32 opcode 6 */
	1, 0, 1, 1, 0, "MULS",			// 100
	1, 0, 1, 1, 0, "MACS",			
	1, 0, 1, 1, 0, "MULU",			
	1, 0, 1, 1, 0, "MACU",			
	1, 0, 1, 1, 0, "MULF",			
	1, 0, 1, 1, 0, "MACF",			
	0, 0, 0, 0, 1, "Reserved 6:6",			
	1, 0, 1, 1, 0, "MACUS",			
	1, 0, 1, 1, 0, "MULS.4",			
	1, 0, 1, 1, 0, "MSUF",			
	1, 0, 1, 1, 0, "MULU.4",		// 110
	0, 0, 0, 0, 1, "Reserved 6:11",			
	0, 0, 0, 0, 1, "Reserved 6:12",			
	0, 0, 0, 0, 1, "Reserved 6:13",			
	0, 0, 0, 0, 1, "Reserved 6:14",			
	0, 0, 0, 0, 1, "Reserved 6:15",			
	1, 0, 1, 1, 0, "MADD.4",			
	1, 0, 1, 1, 0, "MADD.2",			
	1, 0, 1, 1, 0, "MSUB.4",			
	1, 0, 1, 1, 0, "MSUB.4",			
	0, 0, 0, 0, 1, "Reserved 6:20",		// 120			
	0, 0, 0, 0, 1, "Reserved 6:21",			
	0, 0, 0, 0, 1, "Reserved 6:22",			
	0, 0, 0, 0, 1, "Reserved 6:23",			
	0, 0, 0, 0, 1, "Reserved 6:24",			
	0, 0, 0, 0, 1, "Reserved 6:25",			
	0, 0, 0, 0, 1, "Reserved 6:26",			
	0, 0, 0, 0, 1, "Reserved 6:27",			
	0, 0, 0, 0, 1, "Reserved 6:28",			
	0, 0, 0, 0, 1, "Reserved 6:29",			
	0, 0, 0, 0, 1, "Reserved 6:30",		// 130			
	0, 0, 0, 0, 1, "Reserved 6:31",	
	/* 32 opcode with upper Dn bit set */
	0, 0, 0, 0, 0, "1-2 operand",
	0, 0, 0, 0, 1, "Reserved 1.d",
	1, 0, 1, 0, 0, "shift/mul",
	0, 0, 0, 0, 1, "Reserved 3.d",
	1, 1, 1, 0, 0, "BSET",
	1, 1, 1, 0, 0, "BCLR",
	0, 0, 0, 0, 1, "Reserved 6.d",
	0, 0, 0, 0, 1, "Reserved 7.d",
	1, 1, 1, 0, 0, "AND.1",		// 140
	1, 1, 1, 0, 0, "AND.4",
	1, 1, 1, 0, 0, "OR.1",		
	1, 1, 1, 0, 0, "OR.4",
	1, 1, 1, 0, 0, "XOR.1",
	1, 1, 1, 0, 0, "XOR.4",
	1, 1, 1, 0, 0, "ADD.1",
	1, 1, 1, 0, 0, "ADD.4",
	1, 1, 1, 0, 0, "ADDC",
	1, 1, 1, 0, 0, "SUB.1",
	1, 1, 1, 0, 0, "SUB.4",		// 150
	1, 1, 1, 0, 0, "SUBC",
	1, 1, 1, 0, 0, "PXBLEND.T",	
	1, 1, 1, 0, 0, "PXVI.S",
	1, 1, 1, 0, 0, "PXADDS.U",
	0, 0, 0, 0, 1, "Reserved 23.d",
	1, 0, 1, 0, 0, "CMPI",
	0, 1, 0, 0, 0, "MOVEI",
	0, 0, 0, 0, 0, "JMP",
	0, 0, 0, 0, 0, "CALL",
	0, 0, 0, 0, 0, "MOVEAI",		// 160
	0, 0, 0, 0, 1, "Reserved 29.d",
	0, 0, 0, 0, 0, "CALLI",	
	0, 0, 0, 0, 1, "Reserved 31.d",
	/* 32 opcode 3 FP instructions */
	1, 0, 1, 1, 0, "FADDS",
	1, 0, 1, 1, 0, "FSUBS",
	1, 0, 1, 1, 0, "FMULS",
	1, 0, 1, 1, 0, "FDIVS",
	1, 0, 1, 1, 0, "FI2D",
	1, 0, 1, 1, 0, "FS2D",
	1, 0, 1, 1, 0, "FS2L",		// 170
	1, 0, 1, 1, 0, "FSQRTS",
	1, 0, 1, 1, 0, "FNEGS",		
	1, 0, 1, 1, 0, "FABSS",
	1, 0, 1, 1, 0, "FI2S",		
	1, 0, 1, 1, 0, "FS2I",
	1, 0, 1, 1, 0, "FCMPS",
	0, 0, 0, 0, 1, "Reserved 3.d",
	0, 0, 0, 0, 1, "Reserved 3.e",
	0, 0, 0, 0, 1, "Reserved 3.f",
	1, 0, 1, 1, 0, "FADDD",		// 180
	1, 0, 1, 1, 0, "FSUBD",
	1, 0, 1, 1, 0, "FMULD",	
	1, 0, 1, 1, 0, "FDIVD",
	1, 0, 1, 1, 0, "FL2S",	
	1, 0, 1, 1, 0, "FD2S",
	1, 0, 1, 1, 0, "FD2I",
	1, 0, 1, 1, 0, "FSQRTD",
	1, 0, 1, 1, 0, "FNEGD",
	1, 0, 1, 1, 0, "FABSD",
	1, 0, 1, 1, 0, "FL2D",		// 190
	1, 0, 1, 1, 0, "FD2L",
	1, 0, 1, 1, 0, "FCMPD",	
	0, 0, 0, 0, 1, "Reserved 3.d",
	0, 0, 0, 0, 1, "Reserved 3.e",	
	0, 0, 0, 0, 1, "Reserved 3.f",
};

#define MAXJCOUNT 1000

struct iv {
	double count;
	int type;
} inst_view[INST_COUNT];

int inst_cmp(const void *in1, const void *in2)
{
	struct iv *iv1 = (struct iv *)in1;
	struct iv *iv2 = (struct iv *)in2;
	if (iv1->count < iv2->count) {
		return 1;
	}
	if (iv1->count > iv2->count) {
		return -1;
	}
	return 0;
}

int int_cmp(const void *in1, const void *in2)
{
	int *iv1 = (int *)in1;
	int *iv2 = (int *)in2;
	if (*iv1 < *iv2) {
		return 1;
	}
	if (*iv1 > *iv2) {
		return -1;
	}
	return 0;
}

int dbl_cmp(const void *in1, const void *in2)
{
	double *iv1 = (double *)in1;
	double *iv2 = (double *)in2;
	if (*iv1 < *iv2) {
		return 1;
	}
	if (*iv1 > *iv2) {
		return -1;
	}
	return 0;
}

#define FUNCT_NAME_SIZE 48
struct funct {
	unsigned int address;
	unsigned int size;
	double count;			// total count for this symbol
	double haz;			// hazard count for this symbol
	double iblock;			// inst blocked count for this symbol
	double dblock;			// data blocked count for this symbol
	double thcount[NUMTHREADS];	// hit count for symbol by thread
	double called;			// number of times found in call_graph record
	int pram;			// is finction in OCM?
	int type;
	char name[FUNCT_NAME_SIZE];	
};

int funcsortby = SORT_TIME;

int fcompare(const void *in1, const void *in2)
{
	struct funct *iv1 = (struct funct *)in1;
	struct funct *iv2 = (struct funct *)in2;
	if (funcsortby == SORT_NAME) {
		if (iv1->count == 0 && iv2->count != 0) {	// unsampled go to the bottom
			return 1;
		}
		if (iv1->count != 0 && iv2->count == 0) {
			return -1;
		}
		return strcmp(iv1->name, iv2->name);
	}

	if (iv1->count < iv2->count) {
		return 1;
	}
	if (iv1->count > iv2->count) {
		return -1;
	}

	if (iv1->count == 0 && iv2->count == 0) {
		if (iv1->pram && !iv2->pram) {
			return -1;
		}
		if (!iv1->pram && iv2->pram) {
			return 1;
		}
		int size1 = iv1->size;
		int size2 = iv2->size;
		if (size1 < size2) {
			return 1;
		}
		if (size1 > size2) {
			return -1;
		}
	}
	return 0;
}

static char *types[5] = { "   DIS ", "   NRT ", "  HIGH ", "   HRT ", "   GDB " };
static char *swtypes[5] = { "   DIS ", "   GDB ", "   HRT ", " ULTRA ", " LINUX " };



void profile::set_log(bool log, int secs)
{
	log_data = log;
	log_seconds = secs;
	next_log = secs;
}

bool firstline = true;

// write one line per log interval
void profile::write_log()
{
	if (firstline) {
		firstline = false;
		FILE *log = fopen(LOGNAME, "w");
		if (log != NULL) {
			time_t tm = time(NULL);
			fprintf(log, "Profile log for %s, %d sec/sample, log starts at %s\n", 
				elf_files[0], log_seconds, ctime(&tm));
			fprintf(log, "Time,    T0 MIPS,T1 MIPS,T2 MIPS,T3 MIPS,T4 MIPS,T5 MIPS,T6 MIPS,T7 MIPS,Tot MIPS, Flsh%%, Util%%,   Heap,     Netpages.  Cov%%  Mqueue\n");
			fclose(log);
		}
		log = fopen(LOGSTATSNAME, "w");
		if (log != NULL) {
			time_t tm = time(NULL);
			fprintf(log, "Profile statistics log for %s, %d sec/sample, log starts at %s\n", 
				elf_files[0], log_seconds, ctime(&tm));
			for (int i = 0; i < MAX_STAT_DISPLAY; ++i) {
				fprintf(log, "%20s, ", stat_data[i].s);
			}
			fprintf(log, "\n");
			fclose(log);
		}
	}

	FILE *f = fopen(LOGNAME, "a");
	if (f == NULL) {
		return;
	}

	double total_mips = 0;
	double total_flash = 0;
	double total_active = 0;
	for (int i = 0; i < NUMTHREADS; ++i) {
		total_mips += recent_mips[i];
		if (type[i] != T_DISABLED && type[i] != T_HRT && type[i] != T_GDB) {
			total_flash += flash_count[i] - last_flash_count[i];
			total_active += active_count[i] - last_active_count[i];
		}
	}
	if (total_active == 0) {
		total_active = 1;
	}

	unsigned int pmem_size = mem.total_size()/4;	// in instructions
	fprintf(f, "%2d:%02d:%02d,%7.2f,%7.2f,%7.2f,%7.2f,%7.2f,%7.2f,%7.2f,%7.2f,%7.2f, %5.1f, %5.1f\n",
		elapsed_time/3600, (elapsed_time/60)%60, elapsed_time%60, 
		recent_mips[0], recent_mips[1], recent_mips[2], recent_mips[3], recent_mips[4], recent_mips[5], recent_mips[6], recent_mips[7], 
		total_mips, 100.*total_flash/total_active, 100. - recent_idle);

	fclose(f);

	f = fopen(LOGSTATSNAME, "a");
	if (f == NULL){
		return;
	}

	for (int i = 0; i < MAX_STAT_DISPLAY; ++i) {
		fprintf(f, "%20.1f, ", gvals[i]);
	}

	fprintf(f, "\n");
	fclose(f);
}

#define MAXLOOPSIZE 200
#define MAXLOOP 1000
struct oneloop {	// one per loop for BTB analysis
	double hazmips;
	memblock *seg;
	unsigned int pc;	// the backward brtanch closing the loop
	unsigned int numjmps;
	unsigned int numfwdtakenjmps;
	unsigned int numinst;
	double tripcnt;
	int calls;
	double callhazmips;
	double rethazmips;
	double fwdtakenmips;
	double untaken;
} loop[MAXLOOP];

#define MAXDEPTH 3
#define MAXTRIPS 1000000.
// need a minimum untaken to get statistically significant results
#define MINTAKEN 20.

// find simple loops
// return true if this is a valid loop
bool profile::findloop(int lp, int type) 
{
	double fall = 1.0;	// fraction of loops that fal to the bottom without early exit
	unsigned int pc;
	unsigned int stack[MAXDEPTH];
	memblock *seg = loop[lp].seg;
	int sp = 0;
	loop[lp].numjmps = 1;
	loop[lp].numinst = 1;
	loop[lp].tripcnt = 1;
	loop[lp].numfwdtakenjmps = 0;
	loop[lp].calls = 0;
	loop[lp].callhazmips = 0;
	loop[lp].rethazmips = 0;
	loop[lp].fwdtakenmips = 0;
	next_pc(seg->pmem[loop[lp].pc / 4], loop[lp].pc, seg, type, &pc);	// get target
	int count = 1;
	while (pc != loop[lp].pc) {
		loop[lp].numinst++;
		if (count++ > MAXLOOPSIZE) {
			return false;
		}
		unsigned int inst = seg->pmem[pc / 4];
		int t = decode_instruction(inst);
		if (t == INST_CALL) {
			if (sp == MAXDEPTH){
				return false;
			}
			loop[lp].callhazmips += (double)clock_rate * seg->hazclocks[pc / 4] / itotal_nrt * instcnt / total_clocks;
			unsigned int next;
			bool valid = next_pc(inst, pc, seg, t, &next);
			stack[sp++] = pc + 4;
			pc = next;
			loop[lp].calls++;
		} else if (t == INST_CALLI) {
			unsigned int next;
			bool valid = next_pc(inst, pc, seg, t, &next);
			if (!valid) {	// it's really a return
				if (sp == 0) {
					return false;
				}
				loop[lp].rethazmips += (double)clock_rate * seg->hazclocks[pc] / 4 / itotal_nrt * instcnt/total_clocks;
				pc = stack[--sp];
			} else {
				if (sp == MAXDEPTH){
					return false;
				}
				stack[sp++] = pc + 4;
				pc = next;
				loop[lp].calls++;
			}
		} else if (t == INST_RET) {
			if (sp == 0) {
				return false;
			}
			loop[lp].rethazmips += (double)clock_rate * seg->hazclocks[pc / 4] / itotal_nrt * instcnt / total_clocks;
			pc = stack[--sp];
		} else if (t == INST_JMPT || t == INST_JMP) {
			loop[lp].numjmps++;
			int backward = (inst >> 20) & 1;	// backward direction
			unsigned int next;
			bool valid = next_pc(inst, pc, seg, t, &next);
			if (t == INST_JMP) {		// don't follow conditional branches, nested loops
				double taken = 0;	// fraction of time jmp is taken
				if (seg->gprof[pc / 4] != 0) {
					taken = seg->taken[pc / 4] / seg->gprof[pc / 4];
				}
				if (sp == 0 && next >= loop[lp].pc + 4) {		// early exit
					fall *= (1 - taken);
					pc += 4;
				} else if (!backward && taken > 0.5) {
					loop[lp].fwdtakenmips += (double)clock_rate * seg->hazclocks[pc / 4] / itotal_nrt * instcnt / total_clocks;
					loop[lp].numfwdtakenjmps++;
					pc = next;
				} else  {
					pc += 4;
				}
			} else {	// unconditional jmp
				pc = next;
			}
		} else {
			pc += 4;
		}
	}
	if (seg->gprof[loop[lp].pc / 4] > 0.1) {
		double untaken = 1.0 - (seg->taken[loop[lp].pc / 4] / seg->gprof[loop[lp].pc / 4]) * fall;
		loop[lp].untaken = seg->gprof[loop[lp].pc / 4] - seg->taken[loop[lp].pc / 4];
		if (loop[lp].untaken < MINTAKEN) {
			loop[lp].tripcnt = MAXTRIPS;
		} else {
			loop[lp].tripcnt = 1.0 / untaken;
		}
	}
	return true;
}

#define MAX_DISPLAY 5000
void profile::display_functions(int sortby, double hazard_total, __int64 total_instruction_count)
{
	char buf[MAXLINESIZE];
	char buf2[1000];
	char buf3[1000];
	bool touched;
	double haz_cnt, iblock_cnt, dblock_cnt;

	int sym_cnt = 0;	// number of interesting symbols to sort, doe to .05 MIPS
	funct *f = (funct *)malloc(sizeof(funct) * MAX_DISPLAY);
	if (!f) {
		addline("Out of memory, malloc failed", P_FUNC);
		return;
	}
	for (memblock *m = mem.first_segment(); m != NULL; m = m->next) {
		for (int sym = 0; sym < m->symbol_count && sym_cnt < MAX_DISPLAY; ++sym) {
			f[sym_cnt].address = m->symbol_table[sym].st_value;
			f[sym_cnt].size = m->symbol_table[sym].st_size;
			f[sym_cnt].count = m->hit_count(f[sym_cnt].address, 
				f[sym_cnt].address + f[sym_cnt].size, haz_cnt, iblock_cnt, dblock_cnt, touched);
			f[sym_cnt].pram = inpram(f[sym_cnt].address);
			/* show 0.1 MIP or in OCM */
			if ((f[sym_cnt].count / total_rate < 0.0002 || f[sym_cnt].count < total_rate / MAX_DISPLAY) && !f[sym_cnt].pram) {
				continue;
			}
			f[sym_cnt].haz = haz_cnt;
			f[sym_cnt].iblock = iblock_cnt;
			f[sym_cnt].dblock = dblock_cnt;
			f[sym_cnt].called = 0.;
			f[sym_cnt].type = m->type;
			strncpy(f[sym_cnt].name, m->symbol_table[sym].st_name, FUNCT_NAME_SIZE - 1);
			f[sym_cnt].name[FUNCT_NAME_SIZE - 1] = 0;
			for (int i = 0; i < next_cg; ++i) {
				if (cg[i].self == f[sym_cnt].address) {
					f[sym_cnt].called += cg[i].count;
				}
			}
			for (int i = 0; i < NUMTHREADS; ++i) {
				f[sym_cnt].thcount[i] = m->th_hit_count(f[sym_cnt].address, 
						f[sym_cnt].address + f[sym_cnt].size, i);
			}
			sym_cnt++;
		}
	}
	funcsortby = sortby;
	qsort(f, sym_cnt, sizeof(funct), fcompare);
	double sum = 0.;
	int pram_size = 0;
	int unused_size = 0;
	int flash_size = 0;
	if (hrt_samples) {
		addline("Function timing and memory usage for all threads.", P_FUNC);
	} else {
		addline("Function timing and memory usage for non real-time threads.", P_FUNC);
	}
	addline("OC: On chip memory, DR: Off chip DRAM.  MIPS is instructions executed.  Haz is estimated hazards.", P_FUNC);
	addline("I-BLK and D_BLK are time wasted waiting for the instruction or data caches.", P_FUNC);
	addline("Unknown functions are caused by missing files on the command line, stripped files, or incorrect -L addresses.", P_FUNC);
	addline("Unknown functions have unknown hazard MIPS", P_FUNC);
	addline("Incorrect callers are caused by stripped libraries.", P_FUNC);
	addline(" ", P_FUNC);
	sprintf(buf, "Loc Time    Sum Total  MIPS   Haz I-BLK D-BLK ");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_HIGH || type[i] == T_NRT || type[i] == T_HRT && hrt_samples) {
			sprintf(buf2, "T%-2d ", i);
			strcat(buf, buf2);
		}
	}
	strcat(buf, "Size    SDRAM   OCM  ");
	strcat(buf, " Sampl  Function name (sampled called-from) ");
	addline(buf, P_FUNC);
	
	double totmips = 0.0;
	double totiblocked = 0.0;
	double totdblocked = 0.0;
	double tothazards = 0.0;
	double tot_thread_mips[NUMTHREADS];
	for (int i = 0; i < NUMTHREADS; ++i) {
		tot_thread_mips[i] = 0.0;
	}
	for (int sym = 0; sym < sym_cnt; ++sym) {
		if (f[sym].count == 0 && (sym == 0 || sym > 0 && f[sym-1].count > 0)) {
			addline(" ", P_FUNC);
			sprintf(buf, "Total MIPS:      %5.1f %5.1f %5.1f %5.1f %5.1f", 
				totmips + tothazards + totiblocked + totdblocked, totmips, tothazards, totiblocked, totdblocked);
			for (int i = 0; i < NUMTHREADS; ++i) {
				if (type[i] == T_HIGH || type[i] == T_NRT || type[i] == T_HRT && hrt_samples) {
					sprintf(buf2, "%3.0f ", tot_thread_mips[i]);
					strcat(buf, buf2);
				}
			}
			addline(buf, P_FUNC);
			addline(" ", P_FUNC);
			addline("Functions in On Chip Memory that were never sampled.", P_FUNC);
			addline(" ", P_FUNC);
			addline("  Size Total   Function Name", P_FUNC);
		}
		if (f[sym].count == 0 && !f[sym].pram) {
			continue;
		}
		int size = f[sym].size;
		sum += 100. * f[sym].count / total_rate;
		if (f[sym].pram) {
			pram_size += size;
		} else {
			flash_size += size;
		}
		double hazards = clock_rate * f[sym].haz * total_instruction_count / hazard_total / total_clocks;
		double iblocked = clock_rate * f[sym].iblock / sample_count[0];
		double dblocked = clock_rate * f[sym].dblock / sample_count[0];
		double mips = clock_rate * f[sym].count / sample_count[0] - hazards - iblocked - dblocked;
		/* hacks to prevent negative numbers when the estimates are wrong */
		if (mips < 0) {
			hazards += mips;
			mips = 0;
		}
		if (hazards < 0) {
			dblocked += mips;
			hazards = 0;
		}
		totmips += mips;
		totiblocked += iblocked;
		totdblocked += dblocked;
		tothazards += hazards;
		char *loc = f[sym].pram ? "OC" : "DR";
		buf[0] = 0;
		if (f[sym].count != 0) {
			if (f[sym].type == MEM_TYPE_UNKNOWN) {
				sprintf(buf, "%s %4.1f%% %5.1f%% %5.1f   ???   ??? %5.1f %5.1f ", 
					loc, 100. * f[sym].count / total_rate, 
					sum, mips + hazards + iblocked + dblocked, iblocked, dblocked);
			} else {
				sprintf(buf, "%s %4.1f%% %5.1f%% %5.1f %5.1f %5.1f %5.1f %5.1f ", 
					loc, 100. * f[sym].count / total_rate, 
					sum, mips + hazards + iblocked + dblocked, mips, hazards, iblocked, dblocked);
			}
			for (int i = 0; i < NUMTHREADS; ++i) {
				if (type[i] == T_HIGH || type[i] == T_NRT || type[i] == T_HRT && hrt_samples) {
					if (f[sym].thcount[i] == 0) {
						sprintf(buf2, "    ");
					} else {
						double tmips = clock_rate * f[sym].thcount[i] / sample_count[0];
							tot_thread_mips[i] += tmips;
						sprintf(buf2, "%3.0f ", tmips);
					}
					strcat(buf, buf2);
				}
			}

			sprintf(buf2, " %6d %6d %6d  ", 
				size, flash_size, pram_size);
			strcat(buf, buf2);
			sprintf(buf2, "%4.0f  ", f[sym].called);
			strcat(buf, buf2);
		} else {
			unused_size += size;
			sprintf(buf2, "%6d %6d  ", size, unused_size);
			strcat(buf, buf2);
		}
		strcat(buf, f[sym].name);
		if (f[sym].called != 0 && f[sym].type != MEM_TYPE_UNKNOWN) {
			double bestval = 0, secondval = 0, thirdval = 0;
			int best = -1, second = -1, third = -1;
			for (int i = 0; i < next_cg; ++i) {
				if (cg[i].self == f[sym].address && cg[i].count > bestval) {
					thirdval = secondval;
					third = second;
					secondval = bestval;
					second = best;
					bestval = cg[i].count;
					best = i;
				} else if (cg[i].self == f[sym].address && cg[i].count > secondval) {
					thirdval = secondval;
					third = second;
					secondval = cg[i].count;
					second = i;
				} else if (cg[i].self == f[sym].address && cg[i].count > thirdval) {
					thirdval = cg[i].count;
					third = i;
				}
			}
			if (best != -1) {
				symbol_address_to_name(cg[best].parent, buf3);
				sprintf(buf2, " (%1.0f %s)", cg[best].count, buf3);
				strcat(buf, buf2);
			} else { 
				strcat(buf, "                                    ");
			}
			if (second != -1) {
				symbol_address_to_name(cg[second].parent, buf3);
				sprintf(buf2, " (%1.0f %s)", cg[second].count, buf3);
				strcat(buf, buf2);
			} else { 
				strcat(buf, "                                    ");
			}
			if (third != -1) {
				symbol_address_to_name(cg[third].parent, buf3);
				sprintf(buf2, " (%1.0f %s)", cg[third].count, buf3);
				strcat(buf, buf2);
			} else { 
				strcat(buf, "                                    ");
			}
		}
		addline(buf, P_FUNC);
	}
	free(f);
}

// hazards tab output
void profile::display_hazards(double hazard_total, __int64 total_instruction_count) 
{
	char buf[MAXLINESIZE];
	addline("Estimated Hazards, MIPS, Cycles per instruction, and Hazards per instruction (NRT only).", P_HAZARDS);
	addline("    (Reduce with more active threads)", P_HAZARDS);

	if (hazard_total == 0) {
		addline(" ", P_HAZARDS);
		addline("No Hazard data received", P_HAZARDS);
		return;
	}
	addline(" ", P_HAZARDS);
	addline("    Hazard   MIPS    CPI    HPI", P_HAZARDS);
	double totalcpi = 0., totalhazmips = 0., totalhpi = 0.0;
	char tmp[1000];
	for (int i = 0; i < NUMHAZARD; ++i) {
		double cpi = haztypecounts[i] / hazard_total;
		totalcpi += cpi;
		double mips = (haztypecounts[i] / hazard_total) * 
			clock_rate * total_instruction_count / total_clocks;
		totalhazmips += mips;
		double hpi = hazcounts[i] / hazard_total;
		totalhpi += hpi;
		sprintf(buf, "%10s %6.1f, %5.3f, %5.3f   ",
			hazname[i], mips, cpi, hpi);
		addline(buf, P_HAZARDS);
	}
	sprintf(buf, "    Total: %6.1f, %5.3f, %5.3f", totalhazmips, totalcpi, totalhpi);
	addline(buf, P_HAZARDS);
	addline(" ", P_HAZARDS);
	double total = 0;
	for (int i = 0; i < 4; ++i) {
		total += adrhazcounts[i];
	}
	addline("   Address hazard distance to prior instruction causing hazard:", P_HAZARDS);
	for (int i = 0; i < 4 || chip_type == CHIP_8K && i < 5; ++i) {
		sprintf(buf, "        %d: %5.2f%%", i + 1, 100. * adrhazcounts[i] / total);
		addline(buf, P_HAZARDS);
	}
	addline(" ", P_HAZARDS);
	addline("Hazards are cancelled instructions caused by executed instructions.", P_HAZARDS);
	addline("A correctly predicted taken jump costs 3 clocks.  Unroll loops to fix.", P_HAZARDS);
	sprintf(buf, "A mispredicted jump costs %d clocks.  Use LIKELY/UNLIKELY macros to fix. ", jmphazards[chip_type][1][0]);
	addline(buf, P_HAZARDS);
	addline("Call costs 3 clocks.  Inline function to fix. ", P_HAZARDS);
	addline("Calli costs 4 clocks (or more is the target triggers an address hazard).  Inline function to fix.", P_HAZARDS);
	sprintf(buf, "Ret (function return) costs %d clocks.  Inline, or use calli to fix.", rethazards[chip_type][0]);
	addline(buf, P_HAZARDS);
	addline("Address penalty is triggered if a register is used to create an address", P_HAZARDS);
	sprintf(buf, "within %d clocks of the instruction that sets its value.", anwindow[chip_type][0]);
	addline(buf, P_HAZARDS);
	addline("The cost is 5 clocks.  Use LEA or MOVEA to set the value to avoid triggering the hazard.", P_HAZARDS);
	sprintf(buf, "MAC penalty is triggered if an accumulator is read within %d clocks", macwindow[chip_type][0]);
	addline(buf, P_HAZARDS);
	addline("of the math instruction that sets its value.  Use by a MAC is OK.", P_HAZARDS);
	addline("The cost is 6 clocks.  Use both accumulators to avoid.", P_HAZARDS);
	addline("I miss and D miss are instructions cancelled due to a cache miss.", P_HAZARDS);
	addline(" ", P_HAZARDS);
	addline("All hazards are reduced if multiple threads are active.", P_HAZARDS);
	addline("Hazard Details shows the actual cost in clocks for each hazard.", P_HAZARDS);
}

void profile::display_memory(void)
{
	char buf[400];
	addline("System memory usage", P_MEMORY);
	addline(" ", P_MEMORY);
	addline("On Chip Memory (240 KB total available)", P_MEMORY);
	sprintf(buf, "      Ultra code:      %d KB", (ocm_text_run_end - ocm_begin) / 1024);
	addline(buf, P_MEMORY);
	sprintf(buf, "      Ultra data:      %d KB", (ocm_heap_run_begin - ocm_stack_run_begin) / 1024);
	addline(buf, P_MEMORY);
	sprintf(buf, "      Ultra heap:      %d KB", (ocm_heap_run_end - ocm_heap_run_begin) / 1024);
	addline(buf, P_MEMORY);
	addline(" ", P_MEMORY);
	addline("DRAM Memory", P_MEMORY);
	sprintf(buf, "      Ultra code:      %d KB", (text_run_end - text_run_begin) / 1024);
	addline(buf, P_MEMORY);
	sprintf(buf, "      Ultra data:      %d KB", (os_begin - text_run_end) / 1024);
	addline(buf, P_MEMORY);
	sprintf(buf, "      Linux memory:    %d MB", (sdram_coredump_limit - os_begin) / (1024 * 1024));
	addline(buf, P_MEMORY);
}

#if 0

void profile::display_memory(void)
{
	char buf[MAXLINESIZE];
	double ocm = 0.;
	double ddr = 0.;
	double fsize = 0.;
	for (int sec = 0; sections[sec] != 0; ++sec) {
		if (sec_start[sec] < 0x40000000) {
			ocm += sec_size[sec];
		} else if (sec_start[sec] < 0x60000000) {
			ddr += sec_size[sec];
		} else if (sec_start[sec] < 0x70000000) {
			fsize += sec_size[sec];
		}
	}

	sprintf(buf, "OCM memory:             %8.1f KB of 192.0 KB (found %7.1f KB)", (ocm - heapmin - netpagemin*256.- sec_size[UNUSED_CODE])/1024., ocm/1024.);
	addline(buf, P_MEMORY);
	for (int sec = 0; sections[sec] != 0; ++sec) {
		if (sec_start[sec] < 0x40000000) {
			sprintf(buf, "%22s: %8.1f KB", sections[sec], sec_size[sec]/1024.);
			addline(buf, P_MEMORY);
			if (sec == UNUSED_CODE) {
				addline(" ", P_MEMORY);
			}
		}
	}
	addline(" ", P_MEMORY);
	sprintf(buf, "     Ave free netpages: %8.1f pages of %4.0f total pages", 
		avecnt ? (double)totnetpage/avecnt : sec_size[OCM_NETPAGE]/256., sec_size[OCM_NETPAGE]/256.);
	addline(buf, P_MEMORY);
	sprintf(buf, "     Min free netpages: %8.1f pages", (double)netpagemin);
	addline(buf, P_MEMORY);
	sprintf(buf, "         Ave free heap: %8.1f KB", 
		avecnt ? (double)totheap/avecnt/1024. : heapmin/1024.);
	addline(buf, P_MEMORY);
	sprintf(buf, "         Min free heap: %8.1f KB", heapmin/1024.);
	addline(buf, P_MEMORY);
	addline(" ", P_MEMORY);

	sprintf(buf, "ipOS DDR memory:        %8.1f KB of %7.1f KB", (ddr - sdheapmin) / 1024., (sdram_end - 0x40000000) / 1024.);
	addline(buf, P_MEMORY);
	for (int sec = 0; sections[sec] != 0; ++sec) {
		if (sec_start[sec] >= 0x40000000 && sec_start[sec] < 0x60000000) {
			sprintf(buf, "%22s: %8.1f KB", sections[sec], sec_size[sec]/1024.);
			addline(buf, P_MEMORY);
		}
	}
	addline(" ", P_MEMORY);
	sprintf(buf, "     Ave free netpages: %8.1f pages of %6.0f total pages", 
		avecnt ? (double)totsdnetpage/avecnt : sec_size[DDR_NETPAGE]/256., sec_size[DDR_NETPAGE]/256.);
	addline(buf, P_MEMORY);
	sprintf(buf, "     Min free netpages: %8.1f pages", (double)sdnetpagemin);
	addline(buf, P_MEMORY);
	sprintf(buf, "         Ave free heap: %8.1f KB", 
		avecnt ? (double)totsdheap / avecnt / 1024. : (double) sdheapmin / 1024.);
	addline(buf, P_MEMORY);
	sprintf(buf, "         Min free heap: %8.1f KB", (double) sdheapmin / 1024.);
	addline(buf, P_MEMORY);
	addline(" ", P_MEMORY);

	sprintf(buf, "POSIX DDR memory:       %6.0f KB", (ipos_begin - 0x40000000) / 1024.);
	addline(buf, P_MEMORY);
	sprintf(buf, "       POSIX ipOS heap: %6d KB  (Used for application stack and heap memory, and memory file system)", posix_heap_size);
	addline(buf, P_MEMORY);
	sprintf(buf, "             Free heap: %6d KB", posix_heap_free);
	addline(buf, P_MEMORY);
	sprintf(buf, "    Memory file system: %6d KB (%5d KB free)", posix_filemem_size, posix_filemem_free); 
	addline(buf, P_MEMORY);
	for (int i = 0; i < posix_app_count; ++i) {
		sprintf(buf, "             POSIX App: %6d KB (%5d text, %5d data, %5d bss, %5d heap, %5d stack): %s", 
			(posix_app[i].bss_size + posix_app[i].text_size + posix_app[i].stack_size + posix_app[i].data_size + posix_app[i].heap_size),
			posix_app[i].text_size, posix_app[i].data_size, posix_app[i].bss_size, posix_app[i].heap_size, posix_app[i].stack_size,
			posix_app[i].name);
	addline(buf, P_MEMORY);
	}
	addline(" ", P_MEMORY);

	sprintf(buf, "FLASH memory required:  %6.0f KB  (%7.0f KB used)", (flash_end - 0x60000000 + 511) / 1024., (fsize + 511) / 1024.);
	addline(buf, P_MEMORY);
	for (int sec = 0; sections[sec] != 0; ++sec) {
		if (sec_start[sec] >= 0x60000000 && sec_start[sec] < 0x70000000) {
			sprintf(buf, "%22s: %8.1f KB", sections[sec], sec_size[sec]/1024.);
			addline(buf, P_MEMORY);
		}
	}
}
#endif

void profile::display_instructions(bool btb, double total_mips)
{
	char buf[MAXLINESIZE];
	char tbuf[200];

	if (itotal_all == 0) {
		addline("No instruction data", P_INST);
		return;
	}

	sprintf(buf, "Memory operands:   Total");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		sprintf(tbuf, "     %2d", i);
		strcat(buf, tbuf);
	}
	addline(buf, P_INST);
	sprintf(buf, "    None            %5.1f%% ",
		100.*(itotal_all - both_count - src_count - dst_count)/itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100.*(inst_counted[i] - both_count_thread[i] - src_count_thread[i] - dst_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "    Source          %5.1f%% ",
		100. * src_count / itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100.*(src_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "    Destination     %5.1f%% ",
		100. * dst_count / itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100.*(dst_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "    Both            %5.1f%% ",
		100. * both_count / itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100.*(both_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "    Move src (load) %5.1f%% ",
		100. * src_move_count / itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100.*(src_move_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "    Move dst(store) %5.1f%% ",
		100. * dst_move_count / itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100.*(dst_move_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "    Move both       %5.1f%% ",
		100. * both_move_count / itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100.*(both_move_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);	sprintf(buf, "    Stack fraction  %5.1f%% ",
		100. * stack_count / (src_count + dst_count + 2. * both_count));
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%",	100.*(stack_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	addline(" ", P_INST);
	sprintf(buf, "    Per Instruction  %5.2f ",
		(src_count + dst_count + 2. * both_count) / itotal_all);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %6.2f% ", (src_count_thread[i] + dst_count_thread[i] + 2.0 * both_count_thread[i])/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	addline(" ", P_INST);
	sprintf(buf, "Mix of hazard causes    ");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		sprintf(tbuf, "     %2d", i);
		strcat(buf, tbuf);
	}
	addline(buf, P_INST);
	sprintf(buf, "                    JMP    ");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100. * jmp_count_thread[i]/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "                    CALL   ");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100. * call_count_thread[i]/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "                    RET    ");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100. * ret_count_thread[i]/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "                    CALLI  ");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100. * calli_count_thread[i]/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	sprintf(buf, "                    DSP    ");
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		if (inst_counted[i] == 0.0) {
			strcat(buf, "       ");
		} else {
			sprintf(tbuf, " %5.1f%%", 100. * dsp_count_thread[i]/inst_counted[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_INST);
	addline(" ", P_INST);
	sprintf(buf, "JMPcc frequencies (Predicted/Taken) - NRT threads only:");
	addline(buf, P_INST);
	sprintf(buf, "    No/No    %5.1f%%",
		100. * jmpcounts[0] / (jmpcounts[0] + jmpcounts[1] + jmpcounts[2] + jmpcounts[3]));
	addline(buf, P_INST);
	sprintf(buf, "    No/Yes   %5.1f%%",
		100. * jmpcounts[2] / (jmpcounts[0] + jmpcounts[1] + jmpcounts[2] + jmpcounts[3]));
	addline(buf, P_INST);
	sprintf(buf, "    Yes/No   %5.1f%%",
		100. * jmpcounts[1] / (jmpcounts[0] + jmpcounts[1] + jmpcounts[2] + jmpcounts[3]));
	addline(buf, P_INST);
	sprintf(buf, "    Yes/Yes  %5.1f%%",
		100. * jmpcounts[3] / (jmpcounts[0] + jmpcounts[1] + jmpcounts[2] + jmpcounts[3]));
	addline(buf, P_INST);
	sprintf(buf, "    Forward untaken  %5.1f%%",
		100. * jmpdirection[0] / (jmpdirection[0] + jmpdirection[1] + jmpdirection[2] + jmpdirection[3]));
	addline(buf, P_INST);
	sprintf(buf, "    Forward taken    %5.1f%%",
		100. * jmpdirection[1] / (jmpdirection[0] + jmpdirection[1] + jmpdirection[2] + jmpdirection[3]));
	addline(buf, P_INST);
	sprintf(buf, "    Backward untaken %5.1f%%",
		100. * jmpdirection[2] / (jmpdirection[0] + jmpdirection[1] + jmpdirection[2] + jmpdirection[3]));
	addline(buf, P_INST);
	sprintf(buf, "    Backward taken   %5.1f%%",
		100. * jmpdirection[3] / (jmpdirection[0] + jmpdirection[1] + jmpdirection[2] + jmpdirection[3]));
	addline(buf, P_INST);

	int jmpcount[2] = { 0, 0 };	// number of unique jmp instructions sampled
	int jmpttcount[2] = { 0, 0 };	// number of jmpt.t
	double totjump[2] = { 0, 0 };	// total number of jmp sampled/executed
	double totttjump[2] = { 0, 0 };	// total number of jmpt.t sampled/executed
	double jmphaz[2] = { 0, 0};
	double jmptthaz[2] = {0, 0 };
	int takerate[11][2];	// taken rate histogram
	int uniqueinst = 0;
	for (int i = 0; i < 11; ++i) {
		takerate[i][0] = takerate[i][1] = 0;
	}
	double jmpcnts[MAXJCOUNT];
	int nextjmp = 0;
	double totaltaken = 0;
	instcnt = 0;	// total NRT instructions sampled
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] != T_DISABLED && type[i] != T_HRT && type[i] != T_GDB) {
			instcnt += instruction_count[i];
		}
	}
	// find all the small loops for BTB statistics
	int nextloop = 0;
	for (memblock *p = mem.first_segment(); p; p = p->next) {
		for (unsigned int i = 0; i < p->size / 4; i++) {
			double cnt = p->gprof[i];
			if (cnt * 50000 < total_rate){
				continue;
			}
			uniqueinst++;
			int type = p->inst_type[i];	// must be an instruction actually sampled
			if (type != INST_JMP && type != INST_JMPT)
				continue;
			double hazmips = (double)clock_rate * p->hazclocks[i] / itotal_nrt * instcnt / total_clocks;
			unsigned int inst = p->pmem[i];
			int pred_taken = (inst >> 22) & 1;	// jmp predicted taken
			int backward = (inst >> 20) & 1;	// backward direction
			if (backward && nextloop < MAXLOOP) {
				loop[nextloop].pc = p->start_address + i * 4;
				loop[nextloop].hazmips = hazmips;
				loop[nextloop].seg = p;
				if (findloop(nextloop, type))
					nextloop++;
			}
			if (type == INST_JMPT) {
				jmpttcount[backward]++;
				totttjump[backward] += cnt;
				jmptthaz[backward] += hazmips;
				continue;
			}
			jmpcount[backward]++;
			totjump[backward] += cnt;
			jmphaz[backward] += hazmips;
			if (p->taken[i] > cnt + 0.001 || cnt == 0.0) {
				continue;
			}
			takerate[(int)(10 * p->taken[i] / cnt)][backward]++;
			if (backward) {
				if (p->taken[i] != 0.0 && nextjmp < MAXJCOUNT-1) {
					jmpcnts[nextjmp++] = hazmips;
				}
				totaltaken += hazmips;
			}
		}
	}
	qsort(loop, nextloop, sizeof(struct oneloop), dbl_cmp);
	addline("Loop statistics", P_LOOP);
	addline(" ", P_LOOP);
	if (btb) {
		addline("# Inst # jmps fwd-tkn calls  trips   loop-haz fwd-haz call-haz    jBTB  fBTB   crBTB  untaken where", P_LOOP);
	} else {
		addline("# Inst # jmps fwd-tkn calls  trips   loop-haz fwd-haz call-haz    untaken where", P_LOOP);
	}
	double tothaz = 0;
	double totcall = 0;
	double totjbtb = 0;
	double totfbtb = 0;
	double totcbtb = 0;
	for (int lp = 0; lp < 50 && lp < nextloop; ++lp) {
		double jsave = 0;
		double fsave = 0;
		double csave = 0;
		if (loop[lp].tripcnt > 2) {
			jsave = loop[lp].hazmips * (loop[lp].tripcnt-2)/loop[lp].tripcnt;
		}
		if (loop[lp].tripcnt > 1) {
			fsave = loop[lp].fwdtakenmips * (loop[lp].tripcnt-1)/loop[lp].tripcnt;
			csave = loop[lp].callhazmips * (loop[lp].tripcnt-1)/loop[lp].tripcnt + loop[lp].rethazmips;
		}
		if (symbol_address_to_name(loop[lp].pc, tbuf)) {
			char trips[20];
			if (loop[lp].tripcnt == MAXTRIPS) {
				strcpy(trips, " ??? ");
			} else {
				sprintf(trips, "%5.2f", loop[lp].tripcnt);
			}
			if (btb) {
				sprintf(buf, "%5d  %5d %5d  %5d   %5s     %5.2f   %5.2f    %5.2f     %5.2f  %5.2f  %5.2f   %5.0f   %s", 
					loop[lp].numinst, loop[lp].numjmps, loop[lp].numfwdtakenjmps, loop[lp].calls, 
					trips, loop[lp].hazmips, loop[lp].fwdtakenmips, loop[lp].callhazmips + loop[lp].rethazmips, 
					jsave, fsave, csave, loop[lp].untaken, tbuf);
			} else{
				sprintf(buf, "%5d  %5d %5d  %5d   %5s     %5.2f   %5.2f    %5.2f     %5.0f   %s", 
					loop[lp].numinst, loop[lp].numjmps, loop[lp].numfwdtakenjmps, loop[lp].calls, 
					trips, loop[lp].hazmips, loop[lp].fwdtakenmips, loop[lp].callhazmips + loop[lp].rethazmips, 
					loop[lp].untaken, tbuf);
			}
			addline(buf, P_LOOP);
			totjbtb += jsave;
			totfbtb += fsave;
			totcbtb += csave;
			tothaz += loop[lp].hazmips;
			totcall += loop[lp].callhazmips + loop[lp].rethazmips;
		}
	}
	addline(" ", P_LOOP);
	sprintf(buf, "Total loop hazards is   %5.2f MIPS.  Reduce by unrolling loops.", tothaz);
	addline(buf, P_LOOP);
	sprintf(buf, "Total call/ret hazards  %5.2f MIPS.  Reduce by inlining functions in loops.", totcall);
	addline(buf, P_LOOP);
	if (btb) {
		addline(" ", P_LOOP);
		sprintf(buf, "Loop BTB savings is     %5.2f MIPS", totjbtb);
		addline(buf, P_LOOP);
		sprintf(buf, "fwd jmp BTB savings is  %5.2f MIPS", totfbtb);
		addline(buf, P_LOOP);
		sprintf(buf, "call/ret BTB savings is %5.2f MIPS", totcbtb);
		addline(buf, P_LOOP);
		sprintf(buf, "Total BTB savings is    %5.2f MIPS (%5.1f%%)", totjbtb + totfbtb + totcbtb, 100.*(totjbtb + totfbtb + totcbtb) / total_mips);
		addline(buf, P_LOOP);
	}
	addline(" ", P_LOOP);
	qsort(jmpcnts, nextjmp, sizeof(double), dbl_cmp);
	sprintf(buf, "%d unique backward jmpt, sampled %3.0f times (%5.1f sample/jmp).   %5.1f hazard MIPS", jmpttcount[1], totttjump[1], (double)totttjump[1]/jmpttcount[1], jmptthaz[1]);
	addline(buf, P_INST);
	sprintf(buf, "%d unique forward jmpt, sampled %3.0f times (%5.1f sample/jmp).    %5.1f hazard MIPS", jmpttcount[0], totttjump[0], (double)totttjump[0]/jmpttcount[0], jmptthaz[0]);
	addline(buf, P_INST);
	sprintf(buf, "%d unique backward jmpcc sampled %3.0f times (%5.1f sample/jmp).     %5.1f hazard MIPS", jmpcount[1], totjump[1], (double)totjump[1]/jmpcount[1], jmphaz[1]);
	addline(buf, P_INST);
	addline(     "Taken %  0    10   20   30   40   50   60   70   80   90  100", P_INST);
	sprintf(buf, "        %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d", 
		takerate[0][1],
		takerate[1][1],
		takerate[2][1],
		takerate[3][1],
		takerate[4][1],
		takerate[5][1],
		takerate[6][1],
		takerate[7][1],
		takerate[8][1],
		takerate[9][1],
		takerate[10][1]);
	addline(buf, P_INST);
	sprintf(buf, "%d unique forward jmpcc sampled %3.0f times (%5.1f sample/jmp).      %5.1f hazard MIPS", jmpcount[0], totjump[0], (double)totjump[0]/jmpcount[0], jmphaz[0]);
	addline(buf, P_INST);
	addline(     "Taken %  0    10   20   30   40   50   60   70   80   90  100", P_INST);
	sprintf(buf, "        %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d  %3d", 
		takerate[0][0],
		takerate[1][0],
		takerate[2][0],
		takerate[3][0],
		takerate[4][0],
		takerate[5][0],
		takerate[6][0],
		takerate[7][0],
		takerate[8][0],
		takerate[9][0],
		takerate[10][0]);
	addline(buf, P_INST);
	double sum = 0;
	bool o25 = false, o50 = false, o75 = false;
	for (int i = 0; i < nextjmp; ++i) {
		sum += jmpcnts[i];
		if (sum > totaltaken*0.9) {
			sprintf(buf, "%d backward jmp instructions for 90%% of all taken backward hazard MIPS", i);
			addline(buf, P_INST);
			break;
		}
		if (sum > totaltaken*0.75 && !o75) {
			sprintf(buf, "%d  backward jmp instructions for 75%% of all taken backward hazard MIPS", i);
			addline(buf, P_INST);
			o75 = true;
		}
		if (sum > totaltaken*0.5 && !o50) {
			sprintf(buf, "%d backward jmp instructions for 50%% of all taken backward hazard MIPS", i);
			addline(buf, P_INST);
			o50 = true;
		}
		if (sum > totaltaken*0.25 && !o25) {
			sprintf(buf, "%d backward jmp instructions for 25%% of all taken backward hazard MIPS", i);
			addline(buf, P_INST);
			o25 = true;
		}
	}

	addline(" ", P_INST);
	sprintf(buf, "Instruction frequencies on %3.0f instruction samples (%d unique instructions):", itotal_all, uniqueinst);
	addline(buf, P_INST);
	addline(" ", P_INST);
	for (int i = 0; i < INST_COUNT; ++i) {
		inst_view[i].count = icount[i];
		inst_view[i].type = i;
	}
	qsort(inst_view, INST_COUNT, sizeof(inst_view[0]), inst_cmp);
	int col = 0;
	char oline[MAXLINESIZE];
	oline[0] = 0;
	for (int i = 0; i < INST_COUNT; ++i) {
		if (inst_view[i].count == 0) continue;
		double freq = (100. * inst_view[i].count) / itotal_all;
		sprintf(buf, "%-14s %5.2f%%     ", idata[inst_view[i].type].name, freq);
		strcat(oline, buf);
		col++;
		if (col == 4) {
			addline(oline, P_INST);
			oline[0] = 0;
			col = 0;
		}
	}
	if (col != 0) {
		addline(oline, P_INST);
	}
	addline(" ", P_INST);
	sprintf(buf, "MAC.4 frequency %5.2f%%", (100. * mac4count) / itotal_all);
	addline(buf, P_INST);
	sprintf(buf, "Small forward JMPcc skipping instructions.  1: %5.2f%%, 2: %5.2f%%, 3: %5.2f%%",
		(100. * jmpfwdcounts[1]) / itotal_all,
		(100. * jmpfwdcounts[2]) / itotal_all,
		(100. * jmpfwdcounts[3]) / itotal_all);
	addline(buf, P_INST);
	sprintf(buf, "%5.2f%% of address hazards are cause by MOVEI", 100. * moveihazcount / hazcounts[HAZ_AREG]);
	addline(buf, P_INST);
}

#if 0

void profile::display_working_set(void)
{
	char buf[MAXLINESIZE];
	addline("All values here are based on full functions", P_WORKING);
	addline(" ", P_WORKING);
	int total_size = 0, init_size = 0, ocm_untouched = 0, dsr_size = 0;
	int thread_size[NUMTHREADS][2];
	int multiple_size = 0;
	for (int i = 0; i < NUMTHREADS; ++i) {
		thread_size[i][0] = 0;
		thread_size[i][1] = 0;
	}
	bool istext;
	for (memblock *m = mem.first_segment(); m != NULL; m = m->next) {
		istext = strstr(m->seg_name, ".ocm_text") != NULL;
		for (int i = 0; i < m->symbol_count; ++i) {
			unsigned int size = m->symbol_table[i].st_size;
			if (istext) {
				total_size += size;
			}
			if (m->symbol_table[i].st_info & INFO_DSR) {
				dsr_size += size;
			}
			short who = mem.who_touched(m->symbol_table[i].st_value + m->relocation, 
				m->symbol_table[i].st_value + m->relocation + size);
			if (!who && istext) {
				ocm_untouched += size;
			} else {
				int count = 0;
				for (int j = 0; j < NUMTHREADS; ++j) {
					if (who & (1 << j)) {
						thread_size[j][!istext] += size;
						count++;
					}
				}
				if (count > 1) {	
					multiple_size += size;
				}
			}
		}
	}
	sprintf(buf, "Total on-chip code:          %5.1f KB", total_size/1024.);
	addline(buf, P_WORKING);
	addline(" ", P_WORKING);
	sprintf(buf, "on-chip never executed:      %5.1f KB", ocm_untouched/1024.);
	addline(buf, P_WORKING);
	addline(" ", P_WORKING);
	for (int i = 0; i < NUMTHREADS; ++i) {
		if (thread_size[i][0] != 0 || thread_size[i][1] != 0){
			sprintf(buf, "Thread %d executed: OCM: %5.1f KB   DDR: %5.1f KB", i, thread_size[i][0]/1024., thread_size[i][1]/1024.);
			addline(buf, P_WORKING);
		}
	}
	addline(" ", P_WORKING);
	if (multiple_size > 0) {
		sprintf(buf, "Multiple threads executed: %5.2f KB", multiple_size/1024.);
		addline(buf, P_WORKING);
	}
}


void profile::display_heap_data(void)
{
	char buf[MAXLINESIZE];
	char buf3[100];
	if (!display_heap->valid) {
		addline("To see heap data, enable heap profiling in ipProfile, enable debug in ipHeap, and enable Global Debug using the Config Tool.", P_HEAP);
	} else {
		int totsize = 0;
		for (int i = 0; i < display_heap->next_block; ++i) {
			totsize += display_heap->blocks[i].size * display_heap->blocks[i].count;
		}
		if (display_heap->entries == 0) {
			addline("No heap data received.  Enable ipHeap debug and Global Debug to get heap data", P_HEAP);
		}
		if (display_heap->num_callers == 0) {
			addline("No caller data.  Enable Record Block Allocation Callers in ipHeap to see callers", P_HEAP);
		}
		sprintf(buf, "%5d  on-chip heap entries, %5d unique entry types, %7d bytes, with %d recorded callers", 
			display_heap->entries, display_heap->next_block, totsize, display_heap->num_callers);
		addline(buf, P_HEAP);
		addline("Note: netbufs are reused, so the function that allocated a netbuf has no relation to the current user.", P_HEAP);
		addline(" ", P_HEAP);
		addline("Count  Size Total  Callers", P_HEAP);
		for (int i = 0; i < display_heap->next_block; ++i) {
			sprintf(buf, "%5d %5d %5d  ", display_heap->blocks[i].count, display_heap->blocks[i].size,
				display_heap->blocks[i].count * display_heap->blocks[i].size);
			for (int j = 0; j < display_heap->num_callers; ++j) {
				if (display_heap->blocks[i].callers[j] == 0) {
					strcat(buf, "No caller");
				} else {
					char buf2[1000];
					symbol_address_to_name(display_heap->blocks[i].callers[j], buf2);
					sprintf(buf3, "%-35s   ", buf2);
					strcat(buf, buf3);
				}
			}
			addline(buf, P_HEAP);
		}
	}
}

#endif

#define KB_PER_PAGE 8

static char *seg_type[] = {
	"Unknown",
	"Ultra  ",
	"Exec   ",
	"Exec   ",
	"DLL    ",
	"KLM    ",
};

void profile::display_apps(void)
{
	addline("Linux application and library code segment memory maps", P_APPS);
	addline(" ", P_APPS);

	unsigned int total_stack = 0;
	unsigned int total_data = 0;
	unsigned int total_code = 0;
	unsigned int total_other = 0;
	char tmp[500];

	for (memory_map *mm = mem.first_memory_map(); mm; mm = mm->next) {
		total_stack += mm->stack_size;
		total_data += mm->data_size;
		total_code += mm->code_size;
		total_other += mm->other_size;
	}
	addline(" ", P_APPS);
	sprintf(tmp, "Total system code memory used:  %5d KB", total_code / 1024);
	addline(tmp, P_APPS);
	sprintf(tmp, "Total system stack memory used: %5d KB", total_stack / 1024);
	addline(tmp, P_APPS);
	sprintf(tmp, "Total system data memory used:  %5d KB", total_data / 1024);
	addline(tmp, P_APPS);
	if (total_other != 0) {
		sprintf(tmp, "Total system other memory used: %5d KB", total_other / 1024);
		addline(tmp, P_APPS);
	}
	addline(" ", P_APPS);
	addline("Known text segments.  Unknown Functions indicates that the ELF file was not specified on the command line.", P_APPS);
	addline("* - This address range does not have a loaded elf file.  Functions page will show Unknown Functions for this time.", P_APPS);
	addline(" ", P_APPS);
	addline("Load address range  Size   Time    Type    Segment name", P_APPS);
	int max_name = 0;
	for (memblock *m = mem.first_segment(); m != NULL; m = m->next) {
		int len = strlen(m->seg_name);
		if (len > max_name)
			max_name = len;
	}
	initbars(P_APPS, 100., 50 + max_name);
	for (memblock *m = mem.first_segment(); m != NULL; m = m->next) {
		double hit_count = m->seg_hit_count();
		if (m->init_start_address < 0x10000000) {
			/* unknown */
			sprintf(tmp, "Mapped addresses  %4d KB %5.1f%%   %s %s",
				(m->size + 511) / 1024,
				total_rate == 0.? 0.: 100. * hit_count / total_rate,
				seg_type[m->type],
				m->seg_name);
		} else {
			sprintf(tmp, "%08x-%08x %4d KB %5.1f%%   %s %s",
				m->init_start_address, m->init_start_address + m->size, 
				(m->size + 511) / 1024,
				total_rate == 0.? 0.: 100. * hit_count / total_rate,
				seg_type[m->type],
				m->seg_name);
		}
		addbar(total_rate == 0.? 0.: 100. * hit_count / total_rate, P_APPS);
		addline(tmp, P_APPS);
	}
	addline(" ", P_APPS);
	addline(" ", P_APPS);
	addline("Unmapped text segments", P_APPS);
	addline("Load address range  Size   Time    Type    Segment name", P_APPS);
	memory_map *mm0 = mem.get_memory_map(0);
	if (!mm0)
		return;
	for (map_segment *ms = mm0->first_seg; ms; ms = ms->next) {
		sprintf(tmp, "%08x-%08x %4d KB %5.1f%%   %s %s",
			ms->start_address, ms->end_address, 
			(ms->end_address - ms->start_address + 511) / 1024,
			total_rate == 0.? 0.: 100. * ms->gprof / total_rate,
			ms->seg? seg_type[ms->seg->type] : "unknown",
			ms->seg? ms->seg->seg_name : "unknown");
		addbar(total_rate == 0.? 0.: 100. * ms->gprof / total_rate, P_APPS);
		addline(tmp, P_APPS);
	}

	if (chip_type == CHIP_7K)
		return;

	mem.sort_maps();

	for (memory_map *mm = mem.first_memory_map(); mm; mm = mm->next) {
		if (mm->pid == 0) {
			continue;
		}
		if (mm->gprof * 1000. < total_rate) {
			continue;
		}
		addline(" ", P_APPS);
		if (!mm->first_seg) {
			sprintf(tmp, "PID %d, %s                %5.1f%%   %s %s",
				mm->pid, mm->command,
				total_rate == 0.? 0.: 100. * mm->gprof / total_rate,
				seg_type[0],
				"unknown");
			addbar(total_rate == 0.? 0.: 100. * mm->gprof / total_rate, P_APPS);
			addline(tmp, P_APPS);
		} else {
			sprintf(tmp, "PID %d, %s", mm->pid, mm->command);
			addline(tmp, P_APPS);
		}
		for (map_segment *ms = mm->first_seg; ms; ms = ms->next) {
			if (ms->gprof == 0.0) {
				continue;
			}
			sprintf(tmp, "%08x-%08x%c %4d KB %5.1f%%    %s",
				ms->start_address, ms->end_address, 
				ms->seg? ' ' : '*',
				(ms->end_address - ms->start_address + 511) / 1024,
				total_rate == 0.? 0.: 100. * ms->gprof / total_rate,
				ms->path);
			addbar(total_rate == 0.? 0.: 100. * ms->gprof / total_rate, P_APPS);
			addline(tmp, P_APPS);
		}
	}
}

void profile::display_hrt(void)
{
	char buf[MAXLINESIZE];
	if (!hrt_samples) {
		addline("No HRT data has been received.  HRT profiling can be enabled in the ipProfile package using the config tool.", P_HRT);
	} else {

		addline("Detailed HRT timing by instruction address and sample count (includes cancelled instructions).", P_HRT);

		for ( int thread = 0; thread < NUMTHREADS; ++thread) {
			if (type[thread] != T_HRT) {
				continue;
			}
			addline(" ", P_HRT);
			sprintf(buf, "Thread %d:", thread);
			addline(buf, P_HRT);
			addline(" ", P_HRT);
			struct symbol *oldsym = 0;
			for (memblock *p = mem.first_segment(); p; p = p->next) {
				for (unsigned int i = 0; i < p->size / 4; i++) {
					int hits = (int)p->thprof[thread][i];
					unsigned int adr = p->start_address + i * 4;
					if (hits != 0) {
						unsigned int func_addr;
						int type;
						struct symbol *sym = p->find_symbol_by_address(adr, &func_addr, &type);
						if (sym != oldsym) {
							sprintf(buf, "%s:", sym->st_name);
							addline(buf, P_HRT);
							oldsym = sym;
						}
						sprintf(buf, "%x:%6d   %-20s", adr, hits, idata[p->inst_type[i]].name);
						if (p->hazclocks[i] > 0.01) {
							char buf2[100];
							sprintf(buf2, "%7.1f: %s", p->hazclocks[i], hazname[p->haztype[i]]);
							strcat(buf, buf2);
						}
						if (p->iblocked[i] > 0.0) {
							char buf2[100];
							sprintf(buf2, "%7.1f: I cache miss near here", p->iblocked[i]);
							strcat(buf, buf2);
						}
						if (p->dblocked[i] > 0.0) {
							char buf2[100];
							sprintf(buf2, "%7.1f: Data Blocked near here ( D cache miss or HRT read and write OCM or I/O blocking read)", mem.dblocked(adr, 0));
							strcat(buf, buf2);
						}
						addline(buf, P_HRT);
					}
				}
			}
		}
	}
}

#define STAT_VIRTUAL 0
#define STAT_UTIL 1
#define STAT_MIPS 2
#define STAT_AVE_LATENCY 3
#define STAT_DMISS 4
#define STAT_IMISS 5
#define STAT_DBUFFER 6



// define the order, default range, and text for output graph
struct stat_d stat_data[MAX_STAT_DISPLAY] = {
{	STAT_UTIL,				100,	100,	"CPU Utilization %",		},	// system stats
{	STAT_MIPS,				100,	100,	"Useful MIPS",		},
{	STAT_DMISS,				1,	10,	"M D miss/sec",		},
{	STAT_IMISS,				1,	10,	"M I miss/sec",		},
{	STAT_DBUFFER,				1,	100,	"D cache write buffer",	},
};


void profile::display_stats(void)
{
	char buf[MAXLINESIZE];
	if (stats_count == 0) {
		addline("No application statistics data has been received", P_STATS);
		addline(" ", P_STATS);
	}
	addline("              Name                   Value      Minimum       Maximum       Average", P_STATS);
	addline(" ", P_STATS);

	/* captured values for calculations */
	int agg_num = 0, agg_pkt = 0, agg_bytes = 0, tx_frames = 0, rx_frames = 0, tx_retries = 0, rx_retries = 0, rx_dups = 0;
	int rx_aggrs = 0, rx_aggr_err = 0, rx_bytes = 0, tx_bytes = 0;
	/* second radio */
	int agg_num2 = 0, agg_pkt2 = 0, agg_bytes2 = 0, tx_frames2 = 0, rx_frames2 = 0, tx_retries2 = 0, rx_retries2 = 0, rx_dups2 = 0;
	int rx_aggrs2 = 0, rx_aggr_err2 = 0, rx_bytes2 = 0, tx_bytes2 = 0;
	/* ethernet */
	int lan_rx_bytes = 0, lan_tx_bytes = 0, wan_rx_bytes = 0, wan_tx_bytes = 0;

	for (int j = 0; j < MAX_STAT_DISPLAY; ++j) {
		gvals[j] = 0;
	}

	// save values received in a stat packet
	for (int i = 0; i < stats_count; ++i) {
		int j = i * 2 + FIXED_STAT_DISPLAY;
		if (stats_last_val[i] != 0 && stats_time[i] != stats_last_time[i]) {
			gvals[j + 1] = clock_rate * 1000000. * (float)(stats_val[i] - stats_last_val[i]) / (stats_time[i] - stats_last_time[i]);
		}
		stats_last_val[i] = stats_val[i];
		stats_last_time[i] = stats_time[i];
		gvals[j] = (float)stats_val[i];
		if (gvals[j] != 0 && !stat_data[j].found) {
			strcpy(stat_data[j].s, stats_string[i]);
			strcpy(stat_data[j + 1].s, stats_string[i]);
			strcat(stat_data[j + 1].s, "/s");
		}
	}

	// do calculated values
	for (int j = 0; j < MAX_STAT_DISPLAY; ++j) {
		if (stat_data[j].name == STAT_MIPS){
			gvals[j] = current_mips;
		}
		if (stat_data[j].name == STAT_DMISS) {
			gvals[j] = d_miss_per_sec;
		}
		if (stat_data[j].name == STAT_DBUFFER && last_perf_counters[0] != perf_counters[0]) {
			if (last_perf_counters[0]) {
				gvals[j] = 100. * (double)(perf_counters[0] - last_perf_counters[0]) / (clock_rate * 1000000.);
			}
			last_perf_counters[0] = perf_counters[0];
		}
		if (stat_data[j].name == STAT_IMISS) {
			gvals[j] = i_miss_per_sec;
		}
		if (stat_data[j].name == STAT_UTIL) {
			gvals[j] = 100. - recent_idle;
		}

		if (gvals[j] != 0) {
			stat_data[j].found++;
			if (gvals[j] > stat_data[j].max_val) {
				stat_data[j].max_val = gvals[j];
			}
			if (gvals[j] < stat_data[j].min_val) {
				stat_data[j].min_val = gvals[j];
			}
			stat_data[j].val_count++;
			stat_data[j].sum_val += gvals[j];
		}
		if (stat_data[j].found) {
			sprintf(buf, "%30s: %10.1f   %10.1f    %10.1f    %10.1f", stat_data[j].s, gvals[j], stat_data[j].min_val, stat_data[j].max_val,
				stat_data[j].sum_val / stat_data[j].val_count);
			addline(buf, P_STATS);
		}
	}
	avecnt = 0;
	totheap = totsdheap = 0;
	totnetpage = totsdnetpage = 0;

	// autorange - one value max per iteration
	for (int j = 0; j < MAX_STAT_DISPLAY; ++j) {
		if (stat_data[j].found < 10) {
			continue;
		}
		double old_range = stat_data[j].range;
		double range_fac = 1.0;
		char buf[100];
		sprintf(buf, "%1.0f", old_range + .001);	// add a little in case of rounding error
		// reduce the range
		if (stat_data[j].max_val < old_range/3.5 && old_range > stat_data[j].range_min) {
			if (buf[0] == '3') {
				range_fac = 1./3.;
			} else {
				range_fac = (3./10.);
			}
		}
		// increase the range
		else if (stat_data[j].max_val > old_range && old_range < stat_data[j].range_max) {
			if (buf[0] == '3') {
				range_fac = (10. / 3.);
			} else { 
				range_fac = 3.;
			}
		}
		if (range_fac == 1.0) {
			continue;
		}
		stat_data[j].range = old_range * range_fac;
		for (int i = 0; i < MAXSECS; ++i) {
			gval_hist[j][i] /= range_fac;
		}
		break;
	}

	// save history
	gnow++;
	if (gnow >= MAXSECS) {
		gnow = 0;
	}
	for (int i = 0; i < MAX_STAT_DISPLAY; ++i) {
		gval_hist[i][gnow] = gvals[i] / stat_data[i].range;
	}
}

#define SCRATCHPAD1 0x161
#define NUMLOCKS 7
#define MT_BIT 31
#define ATOMIC_BIT 30
#define PIC_BIT 29
#define PCI_BIT 28
#define TLB_BIT 25

// spinlock and trylock for each lock type
char *locknames[NUMLOCKS] = {
	"Memory  ",
	"MT_EN   ",
	"Atomic  ",
	"PIC     ",
	"PCI     ",
	"TLB     ",
	"Other   ",
};

#define NUM_LOCK_DESC 1000
#define MIN_LOCK_MIPS 0.1
struct lock {
	int type;
	int try_lock;
	double mips;
	double samples;
	unsigned int pc;
};

int lock_compare(const void *in1, const void *in2)
{
	struct lock *iv1 = (struct lock *)in1;
	struct lock *iv2 = (struct lock *)in2;
	if (iv1->mips < iv2->mips) {
		return 1;
	} 
	if (iv1->mips > iv2->mips) {
		return -1;
	} 
	return 0;
}

void profile::display_locks(void)
{

	struct lock lock_desc[NUM_LOCK_DESC];
	int next_lock = 0;
	double samples = 0.;

	addline("Kernel Lock Statistics", P_LOCKS);
	addline(" ", P_LOCKS);

	double lock_mips[NUMLOCKS][2];
	for (int i = 0; i < NUMLOCKS; ++i) {
		lock_mips[i][0] = lock_mips[i][1] = 0.;
	}

	double mips_rate = clock_rate / sample_count[0];
	for (memblock *p = mem.first_segment(); p; p = p->next) {
		for (unsigned int i = 0; i < p->size / 4; i++) {
			double cnt = p->gprof[i];
			if (cnt < 0.0){
				continue;
			}

			// look for bset followed by jmpne backwards one instruction
			unsigned int inst = p->pmem[i];
			int inst_type = p->inst_type[i];
			if (inst_type != INST_BSET && inst_type != INST_TBSET) {
				continue;
			}
			unsigned int jmp_inst = p->pmem[i + 1];
			if ((jmp_inst >> 27) != 0x1a) {
				continue;
			}
			if (((jmp_inst >> 23) & 0xf) != 11) {
				continue;
			}
			if ((jmp_inst & 0x1fffff) != 0x1fffff) {
				continue;
			}
			int try_lock = 1;
			if ((jmp_inst & 0x1ffff) == 0x1ffff) {
				try_lock = 0;
			} else if ((jmp_inst & 0x1ffff) > 3) {
				continue;
			}

			// what type is it?
			int type = 0;
			if ((inst & 0x7ff) == SCRATCHPAD1) {
				int bit = (inst >> 11) & 0x1f;
				if (bit == MT_BIT)
					type = 1;
				else if (bit == ATOMIC_BIT)
					type = 2;
				else if (bit == PIC_BIT)
					type = 3;
				else if (bit == PCI_BIT)
					type = 4;
				else if (bit == TLB_BIT)
					type = 5;
				else 
					type = 6;
			}

			double mips = (cnt + p->gprof[i + 1]) * mips_rate;
			lock_mips[type][try_lock] += mips;
			samples += cnt;

			if (mips > MIN_LOCK_MIPS && cnt > 5 && next_lock < NUM_LOCK_DESC) {
				lock_desc[next_lock].type = type;
				lock_desc[next_lock].pc = p->start_address + i * 4;
				lock_desc[next_lock].try_lock = try_lock;
				lock_desc[next_lock].samples = cnt;
				lock_desc[next_lock].mips = mips;
				next_lock++;
			}
		}
	}

	addline("MIPS to acquire each lock type (used by the bset and jmpne, not including the bclr).", P_LOCKS);
	char buf[500], name[500];
	sprintf(buf, "%5.0f samples found a lock.  Run to 100 or more samples for accuracy", samples + 0.99);
	addline(buf, P_LOCKS);
	double total_lock_mips = 0.0;
	for (int i = 0; i < NUMLOCKS; ++i) {
		sprintf(buf, "%s: locks %5.3f, trylocks %5.3f", locknames[i], 
			lock_mips[i][0], lock_mips[i][1]);
		addline(buf, P_LOCKS);
		total_lock_mips += lock_mips[i][0] + lock_mips[i][1];
	}
	sprintf(buf, "Total MIPS: %5.3f", total_lock_mips);
	addline(buf, P_LOCKS);

	addline(" ", P_LOCKS);
	if (next_lock == 0) {
		sprintf(buf, "No single lock uses more than %3.1f MIPS. (or too few samples)", MIN_LOCK_MIPS);
		addline(buf, P_LOCKS);
		return;
	}

	addline("Top locks by time used (with enough samples seen)", P_LOCKS);
	qsort(lock_desc, next_lock, sizeof(lock_desc[0]), lock_compare);

	for (int i = 0; i < next_lock && i < 20; ++i) {
		symbol_address_to_name(lock_desc[i].pc, name);
		sprintf(buf, "%s: %s  %5.3f MIPS  %4.0f samples %s", locknames[lock_desc[i].type], lock_desc[i].try_lock ? "TRY " : "LOCK", lock_desc[i].mips, lock_desc[i].samples + 0.99, name);
		addline(buf, P_LOCKS);
	}
	for (int i = 0; i < 20; ++i) {
		addline("                                                                                  ", P_LOCKS);
	}
}

void profile::display_threads(double total_mips)
{
	int line = 0;
	double mclocks = total_clocks/1000000000.;
	char buf[MAXLINESIZE];
	char tbuf[500];
	int numthreads = NUMTHREADS;
	double bps[2] = {0.0,0.0 };	// I cache and D cache blocked per sample

	if (total_clocks - last_total_clocks > 0) {
		recent_mips[0] = clock_rate * (instruction_count[0] - last_instruction_count[0] - (profile_count - last_profile_count)) / (total_clocks-last_total_clocks);
		for (int i = 1; i < NUMTHREADS; ++i) {
			recent_mips[i] = clock_rate *(instruction_count[i] - last_instruction_count[i]) / (total_clocks - last_total_clocks);
		}
	} else {
		for (int i = 0; i < NUMTHREADS; ++i) {
			recent_mips[i] = 0;
		}
	}
	recent_idle = 100. * (1 - (total_rate - last_total_rate) / (sample_count[0] - last_sample_count));

	addline("Profiling functions contained in the following elf files.  Other code will show as Unknown Functions.", P_SUMMARY);
	addline(" ", P_SUMMARY);
	for (int i = 0; i < elf_file_count; ++i) {
		sprintf(tbuf, "%s", elf_files[i]);
		addline(tbuf, P_SUMMARY);
	}
	addline(" ", P_SUMMARY);

	ctime_s(tbuf, 30, &last_packet_time);
	tbuf[24] = 0;
	sprintf(buf, "%5d second profile through %s.  IP%xxxx rev %x running at %3.0f MHz, with %3d MHz DDR clock.", 
		(int)(total_clocks / clock_rate / 1000000.), tbuf, cpu_id >> 16, cpu_id & 0xffff, clock_rate, ddr_freq/1000000);
	addline(buf, P_SUMMARY);
	addline(" ", P_SUMMARY);

	samples_per_second = 0.0;
	if (last_packet_time > start_time) {
		samples_per_second = (double)(sample_count[0]) / (last_packet_time - start_time);
	}
	double dropped = 0.0;
	if (packet_count) {
		dropped = 100. * dropped_packets / (packet_count + dropped_packets);
	}
	sprintf(buf, "Received %d packets and %d samples (%.0f/s). %4.1f%% (%d) dropped packets.  Received %d stat packets", 
		packet_count, 
		sample_count[0], 
		samples_per_second, dropped, dropped_packets, stat_packet_count);
	addline(buf, P_SUMMARY);

	addline(" ", P_SUMMARY);
	if (error) {
		switch (error) {
		case ERROR_BAD_VERSION:
			sprintf(buf, "Warning: Version mismatch.  ipProfile: %d, ip3kprof: %d.", 
				receive_version, PROFILE_VERSION);
			break;
		case ERROR_BAD_MAGIC:
			sprintf(buf, "ERROR: Packet received with a bad magic number.  Please upgrade your profiler tool.");
			break;
		case ERROR_BAD_COUNT:
			sprintf(buf, "ERROR: Packet has bad sample count.");
			break;
		case ERROR_BAD_SAMPLE:
			sprintf(buf, "ERROR: Sample recieved with bad data.");
			break;
		case ERROR_ASSERT:
			sprintf(buf, "ERROR: Internal Profiler Error.");
			break;
		case ERROR_BAD_ELF:
			sprintf(buf, "ERROR: Can't read elf file %s.", elf_file_error_name);
			break;
		default:
			sprintf(buf, "ERROR: Unrecognized error number %d.", error);
		}
		addline(buf, P_SUMMARY);
	} 

	if (illegal_inst) {
		sprintf(tbuf, "ERROR: Sampled illegal instruction 0x%x at 0x%x in %s:%s.  The elf file doesn't match the code on the board.", 
			illegal_value, illegal_address, mem.pid_to_command(illegal_pid), mem.addr_to_section_name(illegal_address, illegal_pid));
		addline(tbuf, P_SUMMARY);
	}

	if (packet_count == 0) {
		addline("No profile data received.", P_THREAD);
		return;
	}

	__int64 total_active_count = 0;
	for (int i = 0; i < NUMTHREADS; ++i) {
		total_active_count += active_count[i];
	}

	double bad_rate = (double)bad_sample_count / total_active_count;
	if (bad_rate > 0.05) {
		sprintf(buf, "Warning: %4.1f%% of samples are not in a known text segment.  Check the elf file names on the command line.  Example: 0x%x",
			bad_rate * 100., bad_pc_value);
		addline(buf, P_SUMMARY);
	} else  if (bad_rate > 0.001) {
		sprintf(buf, "%4.1f%% of samples are not in a known text segment.  They are lumped together as unknown_functions on the functions tab.",
			bad_rate * 100.);
		addline(buf, P_SUMMARY);
	} else {
		addline(" ", P_SUMMARY);
	}

	if (dropped > 5.0) {
		addline("Warning: more than 5% dropped packets may cause misleading results.  Rerun with a lower sample rate and check results.", P_SUMMARY);
		addline(" ", P_SUMMARY);
	} else {
		addline(" ", P_SUMMARY);
	}

	/* type of each thread */
	sprintf(buf, "Thread:          ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		sprintf(tbuf, "%2d      ", i);
		strcat(buf, tbuf);
	}
	strcat(buf, "  Total");
	addline(buf, P_THREAD);
	sprintf(buf, "Type:        ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		}
		sprintf(tbuf, " %s", swtypes[sw_type[i]]);
		strcat(buf, tbuf);
	}
	addline(buf, P_THREAD);

	sprintf(buf, "Peak Rate:    ");
	double peak_total = 0;
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if (type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			sprintf(tbuf, "%6.1f%% ",
				100. * thread_rate[i]);
				strcat(buf, tbuf);
			peak_total += thread_rate[i];
		}
	}
	addline(buf, P_THREAD);

	/* active ratio for each thread */
	sprintf(buf, "Active:       ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			sprintf(tbuf, "%6.1f%% ",
			100. * active_count[i] / sample_count[i]);
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %6.1f%% (Thread is not suspended)", 100. * any_active_count / sample_count[0]);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	/* active and unblocked ratio for each thread */
	sprintf(buf, "Schedulable:  ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			sprintf(tbuf, "%6.1f%% ",
			100. * active_unblocked_count[i] / sample_count[i]);
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %6.1f%% (Thread can be scheduled - active and not waiting for cache)", 100.-100.*mtcount[0]/sample_count[0]);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);
	
	/* executing ratio for each thread */
	sprintf(buf, "Executed:     ");
	__int64 totinstcnt = 0;
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if (type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			sprintf(tbuf, "%6.1f%% ",
			100. * instruction_count[i] / total_clocks);
			strcat(buf, tbuf);
			totinstcnt += instruction_count[i];
		}
	}
	sprintf(tbuf, "   %6.1f%% (instructions executed, not cancelled)", 100.*totinstcnt/total_clocks);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	/* flash/sdram ratio for each thread */
	sprintf(buf, "Code in SDRAM:");
	__int64 tot_count = 0, tot_active = 0;	// for non-gdb thread
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			if (type[i] != T_GDB){
				tot_count += flash_count[i];
				tot_active += active_count[i];
			}
			double flash = 100. * flash_count[i] / sample_count[i];
			if (active_count[i] == 0.) {
				flash = 0.;
			}
			sprintf(tbuf, "%6.1f%% ", flash);
			strcat(buf, tbuf);
		}
	}
	double flash = 100. * tot_count / tot_active;
	if (tot_active == 0.) {
		flash = 0.;
	}
	sprintf(tbuf, "   %6.1f%%", flash);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	/* I and D cache blocked for each thread */
	sprintf(buf, "I Blocked     ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			double blocked = 100. * i_blocked[i] / sample_count[i];
			if (sample_count[i] == 0.) {
				blocked = 0.;
			}
			sprintf(tbuf, "%6.1f%% ", blocked);
			strcat(buf, tbuf);
			bps[0] += (double)i_blocked[i] / sample_count[i];
		}
	}
	addline(buf, P_THREAD);
	sprintf(buf, "D Blocked     ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			double blocked = 100. * d_blocked[i] / sample_count[i];
			if (sample_count[i] == 0.) {
				blocked = 0.;
			}
			sprintf(tbuf, "%6.1f%% ", blocked);
			strcat(buf, tbuf);
			bps[1] += (double)d_blocked[i] / sample_count[i];
		}
	}
	addline(buf, P_THREAD);

	/* multithreading rate */
	sprintf(buf, "Multithread:  ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB || type[i] == T_HRT || thread_rate_sum[i] == 0) {
			strcat(buf, "        ");
		} else {
			/* thread rate sum includes active-unblocked time and all blocked time */
			sprintf(tbuf, "%7.2f ", (active_unblocked_count[i] + thread_blocked_d_sum[i] + thread_blocked_i_sum[i]) / thread_rate_sum[i]);
			strcat(buf, tbuf);
		}
	}
	addline(buf, P_THREAD);

	/* MIPS for each thread */
	addline("", P_THREAD);
	__int64 total_count = 0;	// total instructions executed, not counting thread zero profiler or gdb threads
	__int64 nonhrt_count = 0;
	double total_time[NUMTHREADS];

	sprintf(buf, "Useful MIPS: ");
	for (int i = 0; i < numthreads; ++i) {
		total_time[i] = 0;
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			double m = clock_rate * instruction_count[i] / total_clocks; 
			total_time[i] += m;
			strcat(buf, "        ");
		} else if (i == 0) {
			double m = clock_rate * (instruction_count[i] - profile_count) / total_clocks;
			total_time[i] += m;
			sprintf(tbuf, "%7.1f ", m);
			strcat(buf, tbuf);
			total_count += instruction_count[i] - profile_count;
			nonhrt_count += instruction_count[i] - profile_count;
		} else {
			double m = clock_rate * instruction_count[i] / total_clocks; 
			total_time[i] += m;
			sprintf(tbuf, "%7.1f ", m);
			strcat(buf, tbuf);
			total_count += instruction_count[i];
			if (type[i] != T_HRT) {
				nonhrt_count += instruction_count[i];
			}
		}
	}
	double total_mips = clock_rate * total_count / total_clocks;
	sprintf(tbuf, "   %7.1f", total_mips);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	sprintf(buf, "Recent MIPS: ");
	double total_recent_mips = 0.;
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			sprintf(tbuf, "%7.1f ", recent_mips[i]);
			strcat(buf, tbuf);
			total_recent_mips += recent_mips[i];
		}
	}
	sprintf(tbuf, "   %7.1f", total_recent_mips);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	sprintf(buf, "Profiler MIPS:");
	__int64 prof_total_count = 0;
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if (i == 0) {
			double m = clock_rate * profile_count / total_clocks;
			total_time[i] += m;
			sprintf(tbuf,"%6.1f ", m);
			strcat(buf, tbuf);
			prof_total_count += profile_count;
		} else if (type[i] == T_GDB){
			sprintf(tbuf, "%7.1f ",
				clock_rate * instruction_count[i] / total_clocks);
			strcat(buf, tbuf);
			prof_total_count += instruction_count[i];
		} else {
			strcat(buf, "        ");
		}
	}
	double profiler_mips = clock_rate * prof_total_count / total_clocks;
	sprintf(tbuf, "   %7.1f", profiler_mips);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);
	
	double i_blocked_total_count = 0.;
	double d_blocked_total_count = 0.;
	sprintf(buf, "I Block MIPS:");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			double blk = clock_rate * (thread_blocked_i_sum[i] / sample_count[i]);
			i_blocked_total_count += blk;
			total_time[i] += blk;
			sprintf(tbuf, "%7.1f ", blk);
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %7.1f", i_blocked_total_count);
	strcat(buf, tbuf);
	strcat(buf," (All NRT threads suspended or waiting for I cache)");
	addline(buf, P_THREAD);

	sprintf(buf, "D Block MIPS:");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			double blk = clock_rate * (thread_blocked_d_sum[i] / sample_count[i]);
			d_blocked_total_count += blk;
			total_time[i] += blk;
			sprintf(tbuf, "%7.1f ", blk);
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %7.1f", d_blocked_total_count);
	strcat(buf, tbuf);
	strcat(buf," (All NRT threads suspended or waiting for D cache)");
	addline(buf, P_THREAD);
	// exact hazards is total active time, minus instruction count, minus blocked time
	double haz_total_count = 0.;
	double haz_mips[NUMTHREADS];
	sprintf(buf, "Hazard MIPS: ");
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			haz_mips[i] = 0.;
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
			haz_mips[i] = 0.;
		} else if (type[i] == T_HRT) {
			haz_mips[i] = clock_rate * thread_rate[i] * (active_unblocked_count[i] + thread_blocked_d_sum[i] + thread_blocked_i_sum[i]) / sample_count[i] -
				clock_rate * instruction_count[i] / total_clocks;
			if (haz_mips[i] < 0) {
				haz_mips[i] = 0;
			}
			haz_total_count += haz_mips[i];
			total_time[i] += haz_mips[i];
			sprintf(tbuf, "%7.1f ", haz_mips[i]);
			strcat(buf, tbuf);
		} else {
			haz_mips[i] = clock_rate * ((thread_rate_sum[i] - thread_blocked_d_sum[i] - thread_blocked_i_sum[i]) / sample_count[i] - 
				(double)instruction_count[i] / total_clocks);
			if (haz_mips[i] < 0) {
				haz_mips[i] = 0;
			}
			haz_total_count += haz_mips[i];
			total_time[i] += haz_mips[i];
			sprintf(tbuf, "%7.1f ", haz_mips[i]);
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %7.1f (Exact total hazards per thread)", haz_total_count);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	sprintf(buf, "Hazard  est: ");
	double haz_total_count_est = 0.;
	double hrt_haz = 0.0;
	double haz_est_mips[NUMTHREADS];
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
			haz_est_mips[i] = 0.;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
			haz_est_mips[i] = 0.;
		} else if (type[i] == T_HRT) {
			haz_est_mips[i] = clock_rate * thread_rate[i] * (active_unblocked_count[i] + thread_blocked_d_sum[i] + thread_blocked_i_sum[i])/sample_count[i] -
				clock_rate * instruction_count[i] / total_clocks;
			if (haz_est_mips[i] < 0) {
				haz_est_mips[i] = 0;
			}
			haz_total_count_est += haz_est_mips[i];
			hrt_haz += haz_est_mips[i];
			sprintf(tbuf, "%7.1f ", haz_est_mips[i]);
			strcat(buf, tbuf);
		} else {
			haz_est_mips[i] = (hazard_count[i] / inst_counted[i]) *		// hazard cycles per executed instruction
				clock_rate * instruction_count[i] / total_clocks;	// times actual mip rate
			if (inst_counted[i] == 0.0) {
				haz_est_mips[i] = 0.0;
			}
			haz_total_count_est += haz_est_mips[i];
			sprintf(tbuf, "%7.1f ", haz_est_mips[i]);
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %7.1f (sum of estimated hazards per thread)", haz_total_count_est);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	sprintf(buf, "Hzrd unknown:");
	double haz_cache_total = 0.;
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else if(type[i] == T_GDB) {
			strcat(buf, "        ");
		} else {
			double haz_c = haz_mips[i] - haz_est_mips[i];
			if (haz_c < 0.) {
				haz_c = 0.;
			}
			sprintf(tbuf, "%7.1f ", haz_c);
			haz_cache_total += haz_c;
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %7.1f (Instructions canceled due to unknown functions or estimation errors or Cache miss or Cache arbitration)", haz_cache_total);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	addline("", P_THREAD);
	sprintf(buf, "Total Time:  ");
	double total_total = 0.0;
	for (int i = 0; i < numthreads; ++i) {
		if (type[i] == T_DISABLED) {
			continue;
		} else {
			total_total += total_time[i];
			sprintf(tbuf, "%7.1f ", total_time[i]);
			strcat(buf, tbuf);
		}
	}
	sprintf(tbuf, "   %7.1f  M Clocks Per Second", total_total);
	strcat(buf, tbuf);
	addline(buf, P_THREAD);

	addline("", P_THREAD);
	double cpu_utilization = 100. * (total_rate / sample_count[0]);
	if (cpu_utilization > 100.0) {
		cpu_utilization = 100.0;
	}
	sprintf(buf, "CPU utilization:%6.1f%%  %6.1f MIPS.  (recent %6.1f%%)", 
		cpu_utilization, 
		(total_rate / sample_count[0]) * clock_rate, 
		100. - recent_idle);
	addline(buf, P_THREAD);
	addline("", P_THREAD);	
	/* fraction of time with threads simultaneously active */
	sprintf(buf, "                          ");
	for (int i = 0; i < numthreads; ++i) {
		sprintf(tbuf, "%2d    ", i);
		strcat(buf, tbuf);
	}
	addline(buf, P_THREAD);
	sprintf(buf, "Simul active:           ");
	for (int i = 0; i < numthreads; ++i) {
		sprintf(tbuf, " %4.1f%%", 100. * mtactivecount[i] / sample_count[0]);
		strcat(buf, tbuf);
	}
	addline(buf, P_THREAD);
	sprintf(buf, "Simul active/unblocked: ");
	for (int i = 0; i < numthreads; ++i) {
		sprintf(tbuf, " %4.1f%%", 100. * mtcount[i] / sample_count[0]);
		strcat(buf, tbuf);
	}
	addline(buf, P_THREAD);
	sprintf(buf, "NRT multithreading rate: %3.2f (of %3.2f peak). 4 or more hides most hazards.", 
		nrt_active_count == 0 ? 0 : nrt_active_total / nrt_active_count, peak_total);
	addline(buf, P_THREAD);

	// cache statistics
	addline(" ", P_THREAD);
	sprintf(buf, "DDR SDRAM clock frequency: %5.1f MHz", ddr_freq/1000000.);
	addline(buf, P_THREAD);
	double imisspersec = clock_rate * perf_counters[1] / total_clocks;
	double dmisspersec = clock_rate * perf_counters[3] / total_clocks;
	// misses count as two accesses, one that misses and one that hits
	// an I or D miss causes an instruction to be fetched again
	sprintf(buf, "Instruction cache:  %5.2f M miss/s, %5.1f any blocked per miss, %5.1f all blocked/miss, %5.1f%% utilized",
		imisspersec,
		bps[0] * total_clocks / perf_counters[1],
		i_blocked_total_count / imisspersec,
		100. * any_i_blocked / sample_count[0]
		);
	addline(buf, P_THREAD);
	double dnoackpersec = clock_rate * perf_counters[0] / total_clocks;
	sprintf(buf, "D cache cancels instruction (D miss or lose arbitration) %5.2f M / second.  Cost is approximately %5.2f MIPS", 
		dnoackpersec, dnoackpersec * 7. / (nrt_active_total / nrt_active_count));
	addline(buf, P_THREAD);

	sprintf(buf, "Data cache:         %5.2f access/inst, %6.2f%% miss rate, %5.2f M access/s, %5.2f M miss/s, %5.1f any blocked per miss, %5.1f all blocked/miss, %5.1f%% utilized",
		((double)perf_counters[2]- perf_counters[3]) / (double)total_count,
		100. * perf_counters[3] / (perf_counters[2] - perf_counters[3]),
		clock_rate * (perf_counters[2] - perf_counters[3]) / total_clocks,
		dmisspersec,
		bps[1] * total_clocks / perf_counters[3],
		d_blocked_total_count / dmisspersec,
		100. * any_d_blocked / sample_count[0]
		);
	addline(buf, P_THREAD);
	sprintf(buf, "Data cache:    Last %5.2f M access/sec, %5.2f M miss/sec.  Maximum %5.2f M access/sec, %5.2f M miss/sec",
		d_access_per_sec, d_miss_per_sec, max_d_access_per_sec, max_d_miss_per_sec
		);
	addline(buf, P_THREAD);
	if (chip_type == CHIP_8K) {
		sprintf(buf, "I-TLB %5.2f K misses/sec.  D-TLB %5.2f K misses/sec.  Page Table Cache %5.2f K misses/sec",
			i_tlb_miss_per_sec * 1000., d_tlb_miss_per_sec * 1000., ptec_miss_per_sec * 1000.);
		addline(buf, P_THREAD);
	}

	// remaining summary information
	addline("Where does the application spend its time?", P_SUMMARY);
	initbars(P_SUMMARY, 100., 75);
	sprintf(buf, "Executed instructions:  %5.1f%%", 100. * total_mips / clock_rate);
	addbar(100. * total_mips / clock_rate, P_SUMMARY);
	addline(buf, P_SUMMARY);
	sprintf(buf, "Cancelled instructions: %5.1f%% (hazards cause cancelled instructions)",
		100. * haz_total_count / clock_rate);
	addbar(100. * haz_total_count / clock_rate, P_SUMMARY);
	addline(buf, P_SUMMARY);
	sprintf(buf, "D cache wait:           %5.1f%% (no thread running due to D cache)", 100. * d_blocked_total_count / clock_rate);
	addbar(100. * d_blocked_total_count / clock_rate, P_SUMMARY);
	addline(buf, P_SUMMARY);
	sprintf(buf, "I cache wait:           %5.1f%% (no thread running due to I cache)", 100. * i_blocked_total_count /clock_rate);
	addbar(100. * i_blocked_total_count /clock_rate, P_SUMMARY);
	addline(buf, P_SUMMARY);
	sprintf(buf, "Idle time:              %5.1f%% (no thread wants to run)", 100. - cpu_utilization);
	addbar(100. - cpu_utilization, P_SUMMARY);
	addline(buf, P_SUMMARY);
	addline(" ", P_SUMMARY);
	sprintf(buf, "CPU utilization is %4.1f%%.", cpu_utilization);
	addline(buf, P_SUMMARY);
	sprintf(buf, "I cache misses: %5.2f M/sec, %5.1f%% utilization.", imisspersec, 100. * any_i_blocked / sample_count[0]);
	addline(buf, P_SUMMARY);
	sprintf(buf, "D cache misses: %5.2f M/sec, %5.1f%% utilization.", dmisspersec, 100. * any_d_blocked / sample_count[0]);
	addline(buf, P_SUMMARY);
	sprintf(buf, "Multithreading rate: %4.1f", nrt_active_count == 0 ? 0 : nrt_active_total / nrt_active_count);
	addline(buf, P_SUMMARY);
	addline(" ", P_SUMMARY);
	addline("Top Functions by percentage of total time used:", P_SUMMARY);

#define MAX_FUNCS 100
	funct *f = (funct *)malloc(sizeof(funct) * MAX_FUNCS);
	int next_hit = 0;
	double max_hits = total_rate * 0.03;
	for (memblock *m = mem.first_segment(); m != NULL; m = m->next) {
		for (int i = 0; i < m->symbol_count && next_hit < MAX_FUNCS; ++i) {
			unsigned int size = m->symbol_table[i].st_size;
			bool touched;
			double haz, iblock, dblock;
			f[next_hit].count = m->hit_count(m->symbol_table[i].st_value, 
				m->symbol_table[i].st_value + size, haz, iblock, dblock, touched);
			if (f[next_hit].count < max_hits || f[next_hit].count == 0) {
				continue;
			}
			strncpy(f[next_hit].name, m->symbol_table[i].st_name, FUNCT_NAME_SIZE - 1);
			f[next_hit].name[FUNCT_NAME_SIZE - 1] = 0;
			next_hit++;
		}
	}
	funcsortby = SORT_TIME;
	qsort(f, next_hit, sizeof(funct), fcompare);
	for (int i = 0; i < next_hit; ++i) {
		sprintf(buf, "%5.1f%% %s", 100. * f[i].count / total_rate, f[i].name);
		addline(buf, P_SUMMARY);
	}
	free(f);
}

// check to see if we need to ask for a memory map for a PID
void profile::get_pid_map()
{
	char buf[50];
	double max_hits = 0.;
	memory_map *best = 0;
	if (console_state != CONSOLE_IDLE) {
		return;
	}

	// find highest usage pid without a map
	for (memory_map *mm = mem.first_memory_map(); mm; mm = mm->next) {
		if (!mm->has_map() && mm->gprof >max_hits) {
			max_hits = mm->gprof;
			best = mm;
		}
	}
	if (!best) {
		return;
	}
	if (max_hits < sample_count[0] * 0.01) {
		return;
	}
	sprintf(buf, "pid %d\n", best->pid);
	console_send(buf);
	console_state = CONSOLE_PID;
}

// this should be called exactly once per second to create the string array to be displayed
// page is page to display, or -1 for all of them
void profile::create_display(bool btb, int page, int sortby)
{
	if (start_time == 0) {	// always start data on a one second boundary for maximum short term accuracy
		time(&start_time);
		return;
	}

	elapsed_time++;

	if (!update) {	// TODO - broken when page == -1
		return;
	}

	get_pid_map();

	__int64 total_instruction_count = 0;
	double hazard_total = 0.;
	double total_mips = (total_rate / sample_count[0]) * clock_rate;

	for (int i = 0; i < NUMTHREADS; ++i) {
		if (type[i] != T_DISABLED && type[i] != T_HRT && type[i] != T_GDB){
			total_instruction_count += instruction_count[i];
			hazard_total += inst_counted[i];
		}
	}

	for (int i = 0; i < MAXPAGES; ++i) {
		lastline[i] = 0;
	}

	/*
	 * always display threads for summary information
	 * always display stats so graph always shows history
	 */
	display_threads(total_mips);
	display_stats();

	if (page == P_MEM_FRAG) {
		if (num_pm == 0 || elapsed_time % 4 == 0) {
			if (!getting_map && console_state == CONSOLE_IDLE) {
				if (console_send("vma\n")) {
					num_pm = 0;
					getting_map = true;
				}
			}
		}
	}

	if (page == -1 || page == P_HAZARDS) {
		display_hazards(hazard_total, total_instruction_count);
	}

	if (page == -1 || page == P_MEMORY) {
		display_memory();
	}

	if (page == -1 || page == P_INST || page == P_LOOP) {
		display_instructions(btb, total_mips);
	}

	if (page == -1 || page == P_FUNC) {
		display_functions(sortby, hazard_total, total_instruction_count);
	}

	if (page == -1 || page == P_APPS) {
		display_apps();
	}
	
	if (page == -1 || page == P_HRT) {
		display_hrt();
	}

	if (page == -1 || page == P_HAZ_DETAIL) {
		save_hazards(200, NULL, P_HAZ_DETAIL);
	}

	if (page == -1 || page == P_LOCKS) {
		display_locks();
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// write the log file - last, after everything else
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	next_log--;
	if (next_log == 0) {	// update and output recent data
		if (log_data) {
			write_log();
		}
		next_log = log_seconds;
	}

	if (total_clocks != last_total_clocks) {	// if no samples received in last second, don't update values
		last_total_rate = total_rate;
		last_total_clocks = total_clocks;
		last_profile_count = profile_count;
		last_sample_count = sample_count[0];
		for (int i = 0; i < NUMTHREADS; ++i) {
			last_instruction_count[i] = instruction_count[i];
			last_flash_count[i] = flash_count[i];
			last_active_count[i] = active_count[i];
		}
	}
}

#define VERSION 2

bool profile::save_graph(const char *name)
{
	FILE *f = fopen(name, "wb");
	if (f == NULL) {
		return false;
	}
	int size = MAX_STAT_DISPLAY;
	int version = VERSION;
	fwrite(&version, sizeof(int), 1, f);
	fwrite(&size, sizeof(int), 1, f);
	fwrite(&gnow, sizeof(int), 1, f);
	fwrite(stat_data, sizeof(struct stat_d), size, f);
	for (int i = 0; i < size; ++i) {
		if (stat_data[i].found) {
			fwrite(gval_hist[i], sizeof(float), MAXSECS, f);
		}
	}
	fwrite(lastline, sizeof(int), MAXPAGES, f);
	for (int i = 0; i < MAXPAGES; ++i) {
		fwrite(lines[i], 1, lastline[i]*MAXLINESIZE, f); 
	}

	fclose(f);
	return true;
}

bool profile::read_graph(const char *name)
{
	FILE *f = fopen(name, "rb");
	if (f == NULL) {
		return false;
	}
	int size, version;
	fread(&version, sizeof(int), 1, f);
	if (version != VERSION) {
		fclose(f);
		return false;
	}
	fread(&size, sizeof(int), 1, f);
	fread(&gnow, sizeof(int), 1, f);
	fread(stat_data, sizeof(struct stat_d), size, f);
	for (int i = 0; i < size; ++i) {
		if (stat_data[i].found) {
			fread(gval_hist[i], sizeof(float), MAXSECS, f);
		}
	}
	fread(lastline, sizeof(int), MAXPAGES, f);
	for (int i = 0; i < MAXPAGES; ++i) {
		fread(lines[i], 1, lastline[i]*MAXLINESIZE, f); 
	}
	fclose(f);
	return true;
}

#if 0

// called whenever any memory map changes, so may be multiple map updates for the same app or library
void profile::update_memory_map(int pid)
{
	char *n, name[MAX_NAME];
	memory_map *mm = mem.get_memory_map(pid);
	if (!mm) {
		return;
	}
	for (map_segment *ms = mm->first_seg; ms; ms = ms->next) {
		n = ms->get_name();
		if (!*n) {
			continue;
		}
		strcpy(name, n);
		char buf[500], buf2[500];
		sprintf(buf, "%s.ko", name);
		sprintf(buf2, "%s_unstripped", name);
		bool found = false;
		for (memblock *m = mem.first_segment(); m != NULL; m = m->next) {
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
			if (strcmp(m->seg_name, name) == 0 || strcmp(segname, name) == 0 || strcmp(segname, buf) == 0 || strcmp(segname, buf2) == 0) {
				ms->seg = m;
				ms->start_address = ms->elf_start_address + ms->seg->start_address;
				ms->end_address = ms->start_address + ms->seg->size;
#if 0
				if (ms->ocm_start && strstr(m->seg_name, "ocm_text")) {
					m->set_reloc(linux_memory_map.linux_map[i].ocm_start);
				} else {
					m->set_reloc(linux_memory_map.linux_map[i].start);
				}
#endif
				found = true;
			}
		}

#if 0
		// try to load a module that is not already found
		if (!found && strstr(name, "lib")) {
			for (int j = 0; j < lib_path_count; ++j) {
				if (lib_path[j][0] == '/') {
					sprintf(buf, "%s/%s", lib_path[j], name);
				} else {
					sprintf(buf, "%s\\%s", lib_path[j], name);
				}
				FILE *f;
				if (f = fopen(buf, "r")) {
					fclose(f);
					if (read_posix_elf_file(buf) == ELF_OK) {
						found = true;
					}
				}
			}
		}
		if (!found) {
			// make dummy text section
			unsigned int size = linux_memory_map.linux_map[i].end - linux_memory_map.linux_map[i].start;
			unsigned int *data = (unsigned int *)malloc(size);
			memset(data, 0x50, size);	// fill memory with add  #0x50, reg, #0x50
			sprintf(buf, "%s:Unknown Functions", name);
			mem.add_seg(0, size, data, buf, MEM_TYPE_UNKNOWN);
			if (linux_memory_map.linux_map[i].ocm_start) {
				unsigned int size = linux_memory_map.linux_map[i].ocm_end - linux_memory_map.linux_map[i].ocm_start;
				unsigned int *data = (unsigned int *)malloc(size);
				memset(data, 0x50, size);	// fill memory with add  #0x50, reg, #0x50
				sprintf(buf, "%s:Unknown Functions", name);
				mem.add_seg(0, size, data, buf, MEM_TYPE_UNKNOWN);
			}
		}
#endif
	}
}

#endif
