// profile.h: interface for the profile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROFILE_H__A23A7B58_84FB_4610_8A43_B5D2D6019C38__INCLUDED_)
#define AFX_PROFILE_H__A23A7B58_84FB_4610_8A43_B5D2D6019C38__INCLUDED_
#include <time.h>
#include <assert.h>

typedef unsigned int u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;

#include "profpkt.h"
#include "profilesample.h"
#include "memory.h"
extern "C" {
#include "elf.h"
}

// console communication states
#define CONSOLE_IDLE 0
// waiting for data from a map command
#define CONSOLE_MAPPING 1
// waiting for response to a pid command
#define CONSOLE_PID 2

// function sorting
#define SORT_TIME 0
#define SORT_NAME 1

#define MAX_POSIX_APPS 1000
#define MAX_POSIX_ELF 100

#define NUMTHREADS PROFILE_MAX_THREADS

// maximum number of traces to keep
#define MAXTRACE 10
#define MAXTRACEDEPTH 100

extern void fatal_error(const char *);

// elf file errors
#define ELF_OK 0
#define ELF_ERROR_OPEN 1
#define ELF_ERROR_SECTION 2
#define ELF_ERROR_STRIPPED 3
#define ELF_ERROR_MEMORY 4
#define ELF_ERROR_SYMBOL 5
#define ELF_ERROR_BAD_FORMAT 6

#define ERROR_NONE 0
#define ERROR_BAD_VERSION 1
#define ERROR_BAD_MAGIC 2
#define ERROR_BAD_COUNT 3
#define ERROR_BAD_PC_VALUE 4
#define ERROR_BAD_SAMPLE 5
#define ERROR_ASSERT 6
#define ERROR_BAD_ELF 7

#define MAX_SYMBOL_LENGTH 128

#define INST_COUNT 196
#define INST_BSET 4
#define INST_ADD4 15
#define INST_MOVEI (0x19)
#define INST_JMP 26
#define INST_NOP 96
#define INST_JMPT 97
#define INST_NOPJMPTT 98
#define INST_NOPJMPTF 99
#define INST_CALL 27
#define INST_CALLI 30
#define INST_RET 36
#define INST_TBSET 51
#define INST_EXT1 55
#define INST_EXT2 53
#define INST_MOVE4 44
#define INST_MOVE2 45
#define INST_MOVE1 47
#define INST_MOVEA 46
#define INST_MAC 73
#define INST_MULF 69
#define INST_MULS 65
#define INST_MULU 67
#define INST_CRCGEN 72
#define INST_PDEC 62
#define INST_LEA1 63
#define INST_LEA2 61
#define INST_LEA4 60
#define INST_IREAD 38
#define INST_IWRITE 48
#define INST_MULU4 110
#define INST_MULS4 108

#define CHIP_ID_COUNT 2
#define CHIP_7K 0
#define CHIP_8K 1

struct inst_data {
	int src, dst;	// memory operand allowed and can do a real memory reference
	int src_haz;	// source address arithmetic for hazards (src or an lea instruction)
	int acc;	// accumulator target?
	int illegal;
	char *name;
};

extern inst_data idata[INST_COUNT];

// thread types
#define T_DISABLED 0
#define T_NRT 1
#define T_HIGH 2
#define T_HRT 3
#define T_GDB 4
// NRT with HRT table entries
#define T_NRT_HRT 5

// software thread types
#define SWT_DISABLED 0
#define SWT_GDB 1
#define SWT_HRT 2
#define SWT_ULTRA 3
#define SWT_LINUX 4


#define MAX_CALL_ARCS 10000

struct call_arc
{
	unsigned int parent;	// pc of call instruction
	unsigned int pid;	// pid of self
	unsigned int self;	// address of called routine
	bool reliable;		// is this a reliable call - validated call address?
	double count;		// number of hits
};

extern int offset21(unsigned int inst);	// for jmp

extern int offset24(unsigned int inst);	// for call


static char *hazname[NUMHAZARD] = {
	"JMPT   ",
	"JMPcc  ",
	"JMPTT  ",
	"JMPTF  ",
	"CALL   ",
	"CALLI  ",
	"RET    ",
	"Address",
	"MAC    ",
	"I miss ",
	"D miss "
};

#define MAXSECS (60*60 + 200)

#define MAX_FILE_NAME 1000
#define MAXLINESIZE 500
#define MAXLINES 500

#define MAXPAGES 13

#define P_SUMMARY 0
#define P_THREAD 1
#define P_APPS 2
#define P_FUNC 3
#define P_HAZARDS 4
#define P_HAZ_DETAIL 5
//#define P_MEMORY 6
#define P_MEM_FRAG 6
#define P_LOCKS 7
#define P_LOOP 8
#define P_GRAPH 9
#define P_STATS 10
#define P_INST 11
#define P_HRT 12


/* predefined display values */
/* change VERSION if number of display items changes */
#define FIXED_STAT_DISPLAY 5
#define MAX_STAT_DISPLAY (FIXED_STAT_DISPLAY + PROFILE_MAX_COUNTERS * 2)
#define MIN_INIT_VAL 10000000
#define MAX_LABEL 100
#define MAX_STATS PROFILE_MAX_COUNTERS

struct stat_d {
	int name;
	double range_min, range_max;
	char s[MAX_LABEL];
	int enabled;		// enabled for display (off, bold, dim)
	double range;		// actual range to use
	int found;		// received any data
	double min_val;		
	double max_val;		// for auto-range
	double sum_val;
	int val_count;		// for average
};
extern struct stat_d stat_data[MAX_STAT_DISPLAY];

#define MAXHEAPBLOCKS 20000
#define MAX_CALLERS 10

struct one_heap_block {
	u32_t	count;			/* how many blocks of this size and callers */
	u32_t	size;			/* size of the heap block (amount user allocated rounded up to allocation granularity) */
	u32_t	callers[MAX_CALLERS];		
};

#if 0

// trace tree - tree of called function timings
class ttree
{
public:
	ttree() 
	{
		child = next = NULL;
		count = delay = address = 0;
		expand = false;
	}
	~ttree() 
	{
		if (child) {
			delete child;
			child = NULL;
		}
		if (next) {
			delete next;
			next = NULL;
		}
	}
	ttree *child;
	ttree *next;
	int count;	// number of samples
	int delay;	// maximum delay through this node
	u32_t address;	// address of function
	bool expand;
};

struct trace
{
	int count;	// number of samples
	int delay;	// time of sample
	u32_t address[MAXTRACEDEPTH];	// addresses from trace, most recent at 0
	bool valid[MAXTRACEDEPTH];
};

#endif

extern bool view_all_latency();

#define MAX_SECTIONS 200
#define PROFILE_NUM_MAPS 5000

class profile  
{
public:
	profile();
	~profile();
	void init_profiler(bool initial_update, bool (*send)(const char *msg));		// initialize now, and wait for next timer to start collecting samples again
	void clear_elf_files(void);							// clear the elf file list and library paths

	void on_connect();								// successful connection to the board
	void on_receive(const char *data, const char *ip_address);			// stream data received from board on control connection, and my ip address
	bool on_packet(char *buf, unsigned int size);					// a packet has arrived from the board on the data connection
	void create_display(bool btb = false, int page = -1, int sortby = SORT_TIME);	// make the display output, for a page, or -1 for all pages

	bool save_graph(const char *name);	
	bool read_graph(const char *name);	 
	bool save_gmon_out(void);
	bool save_hazards(int count, const char *filename, int page);		// NULL filename for sending to display

	// tell the profiler to save a log
	bool get_log(void)
	{ 
		return log_data; 
	}
	void set_log(bool log, int seconds);

	bool add_lib_path(const char *path) {
		if (lib_path_count >= MAX_POSIX_ELF)
			return false;
		strcpy(lib_path[lib_path_count], path);
		lib_path_count++;
		return true;
	}
	bool add_elf_file(const char *path) {
		if (elf_file_count >= MAX_POSIX_ELF)
			return false;
		strcpy(elf_files[elf_file_count], path);
		elf_file_count++;
		return true;
	}
	int get_lib_path_count() {
		return lib_path_count;
	}
	const char *get_lib_path(int n) {
		return lib_path[n];
	}
	int get_elf_file_count() {
		return elf_file_count;
	}
	const char *get_elf_file(int n) {
		return elf_files[n];
	}

	int get_chip_type() {
		return chip_type;
	}

	// the text that appears on the display or for saving to a .txt file.  Call create_display to set it
	char lines[MAXPAGES][MAXLINES][MAXLINESIZE];
	int lastline[MAXPAGES];
	// bar graph values (0 means no bar)
	double bars[MAXPAGES][MAXLINES];
	// characters to indent the bar
	int bar_indent[MAXPAGES];
	double bar_range[MAXPAGES];	// 0 to 1

	// values for maps
	struct profile_map pm[PROFILE_NUM_MAPS];
	int num_pm;
	int page_shift;
	unsigned int ultra_start, ultra_size, linux_start, linux_end;
	unsigned long ocm_heap_run_begin, ocm_heap_run_end, heap_run_begin, heap_run_end, ocm_text_run_end, ocm_begin;
	unsigned long ocm_stack_run_begin, text_run_begin, text_run_end, os_begin, sdram_coredump_limit;
	bool getting_map;

	// values for the graphs
	float gvals[MAX_STAT_DISPLAY];
	float last_gvals[MAX_STAT_DISPLAY];
	float gval_hist[MAX_STAT_DISPLAY][MAXSECS];	// circular buffer for display values
	int gnow;				// which is the most recent entry

	// average values for graphs
	double avecnt;
	double totheap;
	double totsdheap;
	double totnetpage;
	double totsdnetpage;

	// min free values for graphs
	int heapmin;		// bytes
	int sdheapmin;
	int netpagemin;		// pages
	int sdnetpagemin;

	int type[NUMTHREADS];		// type of thread: disabled, hrt, hipri, lopri, GDB
	int sw_type[NUMTHREADS];	// software thread type
	int data_thread;		// thread num to take data, or -1 for all threads
	double itotal_nrt;		// total NRT instructions sampled (adjusted for target hazards and multithreading rate)
	int packet_count;		// how many sample packets received?
	bool update;			// update from incoming data
	unsigned long sdram_begin;
	unsigned long sdram_end;

private:
//	void update_memory_map(int pid);
	bool init_all_data(void);			// do the actual init
	void display_threads(double total_mips);
	void display_hazards(double hazard_total, __int64 total_instruction_count);
	void display_memory(void);
	void display_instructions(bool btb, double total_mips);
	void display_functions(int sortby, double hazard_total, __int64 total_instruction_count);
	double get_callers(unsigned int address, int *cgi, double *called_count);
	void caller_format(char *buf, int *cgi);
	void display_apps(void);
	void display_hrt(void);
	void display_stats(void);
	void display_locks(void);

	void get_pid_map(void);
	bool inpram(unsigned int addr);
	bool (*console_send)(const char *msg);		// console send callback
	int console_state;
	int pid_pending;				// pid we are receiving map for, -1 if waiting for first input line

	// first file must be the ultra elf file.  It gets special processing.
	char elf_files[MAX_POSIX_ELF][MAX_FILE_NAME];
	int elf_file_count;
	char lib_path[MAX_POSIX_ELF][MAX_FILE_NAME];
	int lib_path_count;

	bool findloop(int lp, int type);
	int call_valid(unsigned int ret_addr, unsigned int pc, unsigned int pid, unsigned int func, int type);

	int stack_words;	// number of stack words in each profile sample */
	int chip_type;			// CHIP_7K = 0, CHIP_8K = 1, etc, for Hazards
	char elf_file_error_name[MAX_FILE_NAME];
	double clock_rate;		// in MHz
	int dropped_packets;		// how many packes missed
	int log_packet_count;
	int stat_packet_count;		// how many stats packets received?
	double instcnt;			// total instructions sampled
	double itotal_all;		// total instructions sampled (adjusted for target hazards and multithreading rate)
	void parse_counters(char *buf, int size);
	void parse_maps(char *buf, int size, int last_packet);
	time_t last_packet_time;	// when did we get the last packet
	bool log_data;			// log the profile data
	int log_seconds;		// how often to log
	int elapsed_time;
	int next_log;			// when to write next log
	int read_elf_file(const char *lpszPathName);	// read the elf file - can re-read it
	int read_posix_elf_file(const char *lpszPathName);	// read a posix elf file - can re-read it
	bool next_pc(memblock *seg, unsigned int index, int type, unsigned int *next);
	void write_log();
	void initbars(int page, double range, int indent) 
	{
		bar_indent[page] = indent;
		bar_range[page] = range;
		for (int i = 0; i < MAXLINES; ++i)
			bars[page][i] = 0.;
	}
	void addbar(double val, int page)
	{
		bars[page][lastline[page]] = val;
	}
	void addline(char  *val, int page)
	{
		if (page >= MAXPAGES) {
			return;
		}
		strncpy(lines[page][lastline[page]], val, MAXLINESIZE-1);
		if (lastline[page] < MAXLINES-1){
			lastline[page]++;
		}
	}
	unsigned int cpu_id;
	unsigned int last_seq;		// last received sequence number
	bool symbol_address_to_name(unsigned int pc, unsigned int pid, char *buf);
	bool symbol_address_to_name(memblock *seg, int inst, char *buf);
	char *symbol_name(symbol *sym);
//	void max_hazard(int hazard, char *buf, double inst);
	double inst_counted[NUMTHREADS];	// number of instruction executions sampled, adjusted for hazard targets
	unsigned int bad_pc_value;
	unsigned int sec_start[MAX_SECTIONS];
	unsigned int sec_size[MAX_SECTIONS];
	unsigned long netpage_begin, netpage_end;
	unsigned long entry_addr;
	unsigned long flash_end;

	unsigned int ddr_freq;			// ddr pll frequency

	int error;			// any error encountered
	bool illegal_inst;
	unsigned int illegal_address;
	unsigned int illegal_value;
	unsigned int illegal_pid;
	int receive_version;

	char stats_string[MAX_STATS ][PROFILE_COUNTER_NAME_LENGTH];	// most recent stat number
	int stats_val[MAX_STATS];		// most recent stats value
	int stats_last_val[MAX_STATS];		// last value when displayed
	unsigned int stats_time[MAX_STATS];
	unsigned int stats_last_time[MAX_STATS];
	int stats_count;			// how many stats values were received

	double samples_per_second;
	int last_sample_count;
	time_t start_time;	// when started taking samples - 0 means ignore smaples, since initialization not complete
	double total_rate;	// sum of all rates. total_rate + idle_count must equal sample_count[0]
	double thread_rate_sum[NUMTHREADS];	// sum of rate for each thread
	double thread_blocked_i_sum[NUMTHREADS];	// sum of i cache blocked and no other threads running
	double thread_blocked_d_sum[NUMTHREADS];	// sum of d cache blocked and no other threads running
	int mtcount[NUMTHREADS + 1];			// how many threads simultaneously active and unblocked
	int mtactivecount[NUMTHREADS + 1];		// how many threads simultaneously active
	double last_recent_idle;
	double recent_idle;

	int current_time;			// how long has this session been running, in seconds
	unsigned int curr_clocks;	// number of clocks for data received since last display update
	__int64 total_clocks;		// how many total machine clocks sampled
	__int64 last_total_clocks;	// total clocks value at start of last interval
	__int64 profile_count;		// how many mainline instructions executed in the profiler
	__int64 last_profile_count;	// how many mainline instructions executed in the profiler
	__int64 bad_mapped_sample_count;
	__int64 bad_unmapped_sample_count;
	double any_active_count;	// is any thread active (hrt scaled to rate)
	__int64 any_i_blocked;		// thread 0 samples with any thread i blocked
	__int64 any_d_blocked;		// thread 0 samples with any thread d blocked
	double last_total_rate;

	double nrt_active_total;	// for average NRT multithreading rate
	__int64 nrt_active_count;
	bool hrt_samples;		// have samples from an hrt thread

	// performance
	__int64 perf_counters[PROFILE_COUNTERS];	// total of performance counts (I access, miss, D access, miss)
	__int64 last_perf_counters[PROFILE_COUNTERS];	// last values for stats updates
	unsigned int current_perf_counters[PROFILE_COUNTERS];	// counts received in last sample
	double d_access_per_sec;
	double d_miss_per_sec;
	double i_miss_per_sec;
	double d_tlb_miss_per_sec;
	double i_tlb_miss_per_sec;
	double ptec_miss_per_sec;
	double max_d_access_per_sec;
	double max_d_miss_per_sec;
	double d_buffer_per_sec;

	// hrt table
	unsigned char hrt_table[64];
	unsigned int hrt_size;			// used entries in hrt table
	unsigned int hrt_count[NUMTHREADS];	// number of times each thread appears in the HRT table
	double thread_rate[NUMTHREADS];		// 0 to 1.0, rate of execution of this thread when it is running (1.0 for NRT, less for HRT)
	double hrt_rate[NUMTHREADS];		// 0 to 1.0, minimum guaranteed rate of execution in HRT table.  MIght be nonzero for NRT threads if have HRT entries

	// for each thread, 
	int sample_count[NUMTHREADS];		// number of profiler samples taken - each can generate zero or one sample value per thread
	__int64 active_count[NUMTHREADS];	// number of samples when this thread was active (includes GDB thread)
	__int64 last_active_count[NUMTHREADS];	// number of samples when this thread was active
	__int64 active_unblocked_count[NUMTHREADS];	// number of samples when this thread was active and unblocked (includes GDB thread)
	__int64 instruction_count[NUMTHREADS];	// total instructions executed (from hw mips counter)
	__int64 last_instruction_count[NUMTHREADS];	// for logging to a file
	double hazard_count[NUMTHREADS];	// total hazard clocks per thread
	__int64 flash_count[NUMTHREADS];	// how many samples with thread active were executing in flash/sdram? (includes GDB thread)
	__int64 last_flash_count[NUMTHREADS];	// how many samples with thread active were executing in flash/sdram?
	double last_recent_mips[NUMTHREADS];
	double recent_mips[NUMTHREADS];
	__int64 i_blocked[NUMTHREADS];		// counts where each thread was blocked for I or D cache misses
	__int64 d_blocked[NUMTHREADS];

	// for target thread
	int thread;			// which thread

	// for instruction stats
	double icount[INST_COUNT];		// count of instructions by type
	double mac4count;			// how many mac4 opportunities
	double jmpcounts[4];			// jmp prediction counters
	double jmpfwdcounts[4];			// short forward jumps
	double jmpdirection[4];			// conditional jmp directions, forward=0/1, back=2/3, taken = 1/3
	double haztypecounts[NUMHAZARD];	// clock counts for each type of hazard
	double hazcounts[NUMHAZARD];	    // event counts for each type of hazard
	double moveihazcount;			// count of an hazards caused by movei
	double src_count, dst_count, both_count, stack_count;	// number of instructions with src, dest, or both in memory 
	double src_move_count, dst_move_count, both_move_count;	// number of instructions with src, dest, or both in memory 
	double src_count_thread[NUMTHREADS], dst_count_thread[NUMTHREADS], both_count_thread[NUMTHREADS], stack_count_thread[NUMTHREADS];	// number of instructions with src, dest, or both in memory 
	double both_move_count_thread[NUMTHREADS], src_move_count_thread[NUMTHREADS], dst_move_count_thread[NUMTHREADS];
	double am_index_count[NUMTHREADS], am_post_count[NUMTHREADS], am_pre_count[NUMTHREADS], am_offset_count[NUMTHREADS];
	double jmp_count_thread[NUMTHREADS];
	double call_count_thread[NUMTHREADS];
	double ret_count_thread[NUMTHREADS];
	double calli_count_thread[NUMTHREADS];
	double dsp_count_thread[NUMTHREADS];
	double adrhazcounts[6];				// for every address use, how far away was it generates (1 to 5 instructions, or earlier)

	void touch_forward(unsigned int pc, int thread);	// coverage touching for this instruction
	void touch_reverse(unsigned int pc, int thread);	// coverage touching for this instruction
	void on_sample(struct profile_sample &sample);
	void call_graph(struct profile_sample sample, double freq);
	void do_hazards(int thread, struct profile_sample sample, int type, double freq);
	void add_call(unsigned int p_adr, unsigned int s_adr, unsigned int s_pid, double freq, bool reliable);

	unsigned int current_clock;			// last received clock value
	unsigned int current_inst_count[NUMTHREADS];
	double current_mips;				// MIPS in last second

	memory mem;							// all code memory and per-instruction data

	struct call_arc cg[MAX_CALL_ARCS];
	unsigned int next_cg;				// number of used call graphs
	int init_count;					// how many instructions in the init code, not executed later
							// st_info is 1 for init code, | 2 for function (unreliable) | 4 for executed code | 8 for DSR code
	double total_hits;			// total hits for all symbols
};



#endif // !defined(AFX_PROFILE_H__A23A7B58_84FB_4610_8A43_B5D2D6019C38__INCLUDED_)
