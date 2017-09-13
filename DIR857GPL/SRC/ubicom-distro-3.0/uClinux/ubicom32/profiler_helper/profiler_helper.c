/*
 * profiler_helper.c
 *
 * (C) Copyright 2009, Ubicom, Inc.
 *
 * This is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * The profiler_helper is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * the profiler_helper.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/user.h>
#undef NULL
#define NULL 0


#define print_error printf

#ifdef __ubicom32__
#define PT_TEXT_ADDR	200
#define PT_TEXT_END_ADDR 204
#define PT_DATA_ADDR	208
#endif
/* Wait for process, returns status */

int ptrace_internal(int request, pid_t pid, long addr, long data)
{
	int rv = ptrace(request, pid, addr, data);
	if (rv < 0) {
		printf("PTRACE: Error with %d, %d 0x%08lx 0x%08lx\n", request, pid, addr, data);
		exit(1);
	}

	return rv;
}

char * app_name;
int mywait (const pid_t child)
{

	pid_t pid;
	int w;
//	pid = waitpid(child, &w, WCONTINUED);
	pid = wait(&w);
	if (pid != child) {
		print_error("\n%s: Wait Got %d Child %d", app_name, pid, child);
		return 0;
	}

	if (WIFEXITED (w))  {
		print_error("\n\t%s: Child exited with retcode = %d \n", app_name, WEXITSTATUS (w));
		return 0;
	}
	else if (!WIFSTOPPED (w)) {
		print_error("\n\t%s: Child terminated with signal = %d \n", app_name, WTERMSIG (w));
		return 0;
	}
	return w;
}

int main(int argc, char *argv[])
{
	app_name = strrchr(argv[0], '/');
	if (app_name) {
		app_name ++;
	} else {
		app_name = argv[0];
	}

	if (argc < 2) {
		fprintf(stderr, "ERROR: Not enough arguments\nUsage: %s [cmd]\n", app_name);
		exit(-1);
	}

	pid_t child;

	child = vfork();
	if(child == 0) {
		ptrace_internal(PTRACE_TRACEME, 0, NULL, NULL);
		execv(argv[1], argv + 1);
		fprintf(stderr, "ERROR: Child process unable to start '%s', errno=%d\nPerhaps you didn't use a fully qualified path\n", argv[1], errno);
		exit(-1);
	}
	if (child < 0) {
		fprintf(stderr, "ERROR: Could not fork %d\n", child);
		return -1;
	}

	if (mywait(child)) {

		int text_start = ptrace(PTRACE_PEEKUSER, child, PT_TEXT_ADDR);
		int text_end =  ptrace(PTRACE_PEEKUSER, child, PT_TEXT_END_ADDR);
		int data_addr = ptrace(PTRACE_PEEKUSER, child, PT_DATA_ADDR);

		printf("\nThe code has been loaded for %s:\n"
		       "\tCode [0x%08x] - [0x%08x]\n"
		       "\tData [0x%08x]\n", argv[1], text_start, text_end, data_addr);
		printf("\n\t<<<< Press Enter to Start >>>>\n");
		printf("\t-------------------------------\n");
		getc(stdin);
		ptrace_internal(PTRACE_DETACH, child, NULL, NULL);

		mywait(child);
	}
	return 0;
}
