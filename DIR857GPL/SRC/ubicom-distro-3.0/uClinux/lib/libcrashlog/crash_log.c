/*
 * crash_log.c
 * (C) Copyright 2009, Ubicom Inc.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License.  See the file COPYING.LIB in the main
 * directory of this archive for more details.
 *
 *
 * Usage Information:
 * ------------------
 *
 * When linked, crash log automatically installs signal handlers for
 * exception signals (SIGSEGV/SIGILL etc), in order to allow one obtain
 * more information about a particular crash.
 *
 * Once built you can use this library by just linking to it with.
 *
 * -Wl,--whole-archive -lcrashlog -Wl,--no-whole-archive
 * if linking statically you will also need to add '-ldl'
 *
 * If you are not using shared libraries then this library may not be
 * for you as it requires the use of libdl.so/libdl.a.
 *
 * At run-time the library checks the environment variable CRASHLOG_CONF
 * One or more of the following options are allowed to be present
 * (i.e. CRASHLOG_CONF=wait,debug)
 *
 *   wait	- upon a crash the tool will sleep for 10 minutes allowing
 *		one to attach to the process with gdb/gdbserver.
 *
 *   disable	- disable the library completely.
 *
 *   _exit	- calls _exit after an exception, default is exit()
 *
 *   raise	- raises the signal again after an exception
 *
 *   debug	- enables small amount of debug
 *
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>

#include <sys/ucontext.h>

#define dprintf( ...) fprintf(stderr, __VA_ARGS__)
#define PRINT_IF_TTY(str) if(isatty(fileno(stderr)) > 0) { fprintf(stderr, str); }

#define ANSI_RED()	PRINT_IF_TTY("\033[31m")
#define ANSI_GREEN()	PRINT_IF_TTY("\033[32m")
#define ANSI_RESET()	PRINT_IF_TTY("\033[0m")


extern void crash_log_start(void) __attribute__((constructor));

static volatile int crash_log_trap_count = 0; /* used for detecting recursive traps */

static const char * crash_log_env = NULL;
static int crash_log_debug = 0;
/*
 * get_signal_name()
 */
static char * get_signal_name(int signum)
{
	switch(signum) {
		case SIGSEGV:
			return "SIGSEGV";
		case SIGILL:
			return "SIGILL";
		case SIGABRT:
			return "SIGABRT";
		case SIGFPE:
			return "SIGFPE";
		case SIGBUS:
			return "SIGBUS";
	}
	return "";
}

/*
 * print_reg()
 */
static int print_reg(const char * name, int value)
{
	return dprintf(" %7s: %08x", name, value);
}

/*
 * print_reg_and_symbol()
 */
static int print_reg_and_symbol(const char * name, int value)
{
	print_reg(name, value);
	Dl_info dlinfo;
	if (dladdr((void*)value, &dlinfo) > 0) {
		return dprintf(" %s:%s @ %08x\n", dlinfo.dli_sname, dlinfo.dli_fname, dlinfo.dli_fbase);
	} else {
		return dprintf(" <unknown>\n");
	}
}

/*
 * print_ucontext()
 */
static void print_ucontext (const struct ucontext *uc)
{
	const greg_t *regs = uc->uc_mcontext.gregs;
	char str[10];
	int i;

#if defined(__UBICOM32__)
	/*
	 * Print the D regs
	 */
	for(i = 0; i < 16; i++) {
		if (!(i & 0x3)) {
			dprintf("\n");
		}
		snprintf(str, sizeof(str), "D%d", i);
		print_reg(str, regs[REG_D0+i]);
	}

	/*
	 * Print the A regs
	 */
	for(i = 0; i < 8; i++) {
		if (!(i & 0x3)) {
			dprintf("\n");
		}
		snprintf(str, sizeof(str), "A%d", i);
		print_reg(str, regs[REG_A0+i]);
	}

	/*
	 * Print the remaining
	 */
	dprintf("\n");
	print_reg("acc0 H", regs[REG_ACC0HI]); print_reg("acc0 L", regs[REG_ACC0LO]);
	print_reg("acc1 H", regs[REG_ACC1HI]); print_reg("acc1 L", regs[REG_ACC1LO]);
	dprintf("\n");
	print_reg("source3", regs[REG_SOURCE3]);
	print_reg("macrc16", regs[REG_MAC_RC16]);
	print_reg("instcnt", regs[REG_INST_CNT]);
	print_reg("csr", regs[REG_CSR]);
	dprintf("\n\n");

	print_reg_and_symbol("         pc", regs[REG_PC]);
	print_reg_and_symbol("previous_pc", regs[REG_PREVIOUS_PC]);
	print_reg_and_symbol("     rp(a5)", regs[REG_A5]);

#else
	/*
	 * Print the regs (we don't know the names..)
	 */
	for(i = 0; i < sizeof(uc->uc_mcontext.gregs)/sizeof(greg_t); i++) {
		if (!(i & 0x3)) {
			dprintf("\n");
		}
		sprintf(str,"reg%d", i);
		print_reg(str, regs[i]);
	}
	dprintf("\n");
#endif

}
/*
 * crash_log_trap_handler()
 */
void crash_log_trap_handler (int signum, struct siginfo *si,
			    struct ucontext *uc)
{
	static struct sigaction action;

	/*
	 * Set the default signal handler, just in case this code
	 * generates another trap.
	 */
	action.sa_handler = SIG_DFL;
	sigemptyset (&action.sa_mask);
	sigaction(signum, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGBUS, &action, NULL);

	ANSI_RED();
	dprintf("\n *** libcrashlog caught %s(%d) ***\n",
		  get_signal_name(signum), signum);

	/*
	 * Check for recursive signals
	 */
	if (crash_log_trap_count++ > 0) {
		goto done;
	}
#if defined __UCLIBC__
	dlinfo();
#else
	/*
	 * No solution for glibc just yet
	 */
	dprintf("dlinfo not implemented for glibc yet\n");
#endif

	print_ucontext(uc);

#if 0 /* Disabled because it crashes sometimes */
	int count = 0;
	extern void *__libc_stack_end;
	const int **stack_start = (const int**)regs[REG_SP];
	const int **ptr = stack_start;
	const int **stack_limit = (const int**)__libc_stack_end;

	/*
	 * Softlimit in case stack is wrong
	 */
	if ( stack_start + 512 > stack_limit) {
		stack_start = stack_limit + 512;
	}

	dprintf("Back Trace\n");
	while(ptr < stack_limit) {
		Dl_info dlinfo;
		const int *addr = *ptr++;

		memset(&dlinfo, 0, sizeof(dlinfo));
		if (addr > (int*)stack_start && addr < (int*)stack_limit) {
			continue;
		}

		if ((int)addr < 0x3FFC0000 || (int)addr > 0x41000000) {
			continue;
		}
		if (dladdr((void*)addr, &dlinfo) > 0) {
			if (dlinfo.dli_fname) {
				unsigned int instruction = *(addr-1);
				/*
				 * Is this a call or calli instruction?
				 */
				if ((instruction & 0xF8000000) == (unsigned int)(0x1B << 27) ||
				    (instruction & 0xF8000000) == (unsigned int)(0x1E << 27)) {
					dprintf(" %d: %s in %s base %x\n", count++,
						  dlinfo.dli_sname ?  dlinfo.dli_sname : "<unknown>",
						  dlinfo.dli_fname, dlinfo.dli_fbase);
				}
			}
		}
	}
	dprintf(" end of call/calli on stack\n");
#endif

done:
	ANSI_RESET();

	if (crash_log_env) {
		if (strstr(crash_log_env, "wait")) {
			dprintf("Will sleep(600) so you can attach the debugger PID=%d, if "
				"you do you should NOT, continue or set any breakpoints as doing "
				"can crash the entire platform.\n", getpid());
			sleep(600);
		}

		if (strstr(crash_log_env, "_exit")) {
			dprintf("process %d ,_exit(-1)\n", getpid());
			_exit(-1);
		}

		if (strstr(crash_log_env, "raise")) {
			dprintf("process %d, disabled signal handler, return to crash once more.\n", getpid());
			return;
		}
	}
	dprintf("process %d, exit(-1)\n", getpid());
	exit(-1);
}

/*
 * install_hook_list()
 */
static void install_hook_list(int *list, int length) {

	static struct sigaction action;
	int i;
	/* Set up the structure to specify the new action. */
	action.sa_handler = (__sighandler_t) crash_log_trap_handler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset (&action.sa_mask);

	for(i = 0; i < length; i++) {
		sigaction(list[i], &action, NULL);
	}

}

/*
 * crash_log_install_for_signal()
 *	This is not called automatically and can be called by the client
 *	if they so desire.
 */
void crash_log_install_for_signal(int signum)
{
	install_hook_list(&signum, 1);
}

/*
 * crash_log_start()
 *	As this is a constructor, it is called automatically before main().
 */
void crash_log_start(void)
{
	crash_log_env = getenv("CRASHLOG_CONF");

	if (crash_log_env) {
		if (strstr(crash_log_env, "disable") || strstr(crash_log_env, "off")) {
			return;
		}

		if (strstr(crash_log_env, "debug")) {
			crash_log_debug = 1;
		}
	}

	int list[4] = { SIGSEGV, SIGILL, SIGBUS, SIGABRT };

	install_hook_list(list, 4);

	if (crash_log_debug) {
		ANSI_GREEN();
		dprintf("Crash Log watch active on process %d\n", getpid());
		ANSI_RESET();
	}
}
