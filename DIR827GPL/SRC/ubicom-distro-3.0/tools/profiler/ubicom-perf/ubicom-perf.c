#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <signal.h>

int add (int i) __attribute__ ((noinline));
int add (int i)
{
	return i << 3;
}

int add_ret(int i) __attribute__ ((noinline));
int add_ret(int i)
{
	return add(i);
}

void hazard_adr_loop(void)
{
	int i, sum = 0, tmp;
	int a3 = 0, a4 = 0, d0 = 0, d1 = 0;
	while (1) {
		for (i = 0; i < 100000000; ++i) {
			sum += add_ret(i);  // call, ret, calli hazards
			asm volatile (
			"   movei %0, #1	\n"
			"   movei %1, #0	\n"
			"   movei %2, #0	\n"
			"   movei %4, #0	\n"

			// check for An or Dn defined late in pipe and used as source address
			"   move.4 %4, %3	\n"	// An source use too soon
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"


			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"
			"   move.4 %2, (%4, %1)	\n"	// don't show second hazard


			"   move.4 %1, %1	\n"	// Dn source used too soon
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"
			"   nop			\n"
			"   move.4 %2, (%4, %1)	\n"	// don't show second hazard

			// check for An or Dn defined late in pipe and used as source address in lea
			"   move.4 %4, %3	\n"	// An source use too soon
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %4, %3	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"


			"   move.4 %1, %1	\n"	// Dn source used too soon
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   lea.4 %2, (%4, %1)	\n"


			// check for Dn used too soon for movea
			"   move.4 %1, %1	\n"	// Dn source used too soon
			"   movea %4, %1	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   movea %4, %1	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   movea %4, %1	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   movea %4, %1	\n"

			"   move.4 %1, %1	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   movea %4, %1	\n"

			// test that jmp.t avoids an an or dn hazard
			"  move.4 %1, %1	\n"
			"  sub.4  #0, %1, %1	\n"
			"  jmpmi.t  .+4		\n"
			"  lea.2  #0, (%4, %1)	\n"

			// test call, calli, and An to calli hazard
			"   call  %4, .+4		\n"
			"   movei  %1, #12		\n"
			"   add.4  %4, %4, %1		\n"
			"   calli  %4, 0(%4)		\n"
			"   nop				\n"

			// test mac hazards
			"   mulu  acc0, %1, %1		\n"
			"   move.4 %0, acc0_lo		\n"

			: "=d" (tmp)
			: "d" (d0), "d" (d1), "a" (&a3), "a" (a4)
			: "cc"
			);
			sum += tmp;
		}
		printf("value is %d\n", sum);
	}
}

void hazard_jmp_loop(void)
{
	int i, sum = 0, tmp;
	int a3 = 0, a4 = 0, d0 = 0, d1 = 0;
	while (1) {
		for (i = 0; i < 100000000; ++i) {
			sum += add_ret(i);  // call, ret, calli hazards
			asm volatile (
			"   movei %0, #1	\n"
			"   movei %1, #0	\n"
			"   movei %2, #0	\n"
			"   movei %4, #0	\n"

			"   xor.4 d0, d0, d0 \n"	// set condition codes
			"   jmpf.f 1f	\n"
			"   nop		\n"
			"   jmpf.t 1f	\n"
			"1: jmpt.t 2f	\n"		// jumps in jump targets
			"   nop		\n"
			"2: jmpt.f 3f	\n"
			"   nop		\n"
			"3: jmpeq.t 4f	\n"
			"   nop		\n"
			"4: jmpeq.f 5f	\n"
			"   nop		\n"
			"5: nop		\n"

			"   jmpt.t 2f	\n"		// jumps not in jump targets
			"   nop		\n"
			"2: nop		\n"
			"   jmpt.f 3f	\n"
			"   nop		\n"
			"3: nop		\n"
			"   jmpf.t 4f	\n"
			"   nop		\n"
			"4: nop		\n"
			"   xor.4 d0, d0, d0 \n"
			"   jmpeq.t 5f	\n"
			"   nop		\n"
			"5: nop		\n"
			"   jmpeq.f 6f	\n"
			"   nop		\n"
			"6: nop		\n"
			"   xor.1  #0, d0, d0	\n"	// test instruction frequencies
			"   and.1  #0, d0, d0   \n"
			"   or.1  #0, d0, d0	\n"
			"   sub.1 #0, d0, d0	\n"
			"   shmrg.1 d0, d0, d0	\n"
			"   xor.1 #0, d0, d0	\n"
			"   add.1 #0, d0, d0	\n"
			"   jmpt.t 7f		\n"
			"7: sub.1 #0, d0, d0	\n"


			: "=d" (tmp)
			: "d" (d0), "d" (d1), "a" (&a3), "a" (a4)
			: "cc"
			);
			sum += tmp;
		}
		printf("value is %d\n", sum);
	}
}

/* lots of hazards */
void call_loop(void)
{
	int i;
	int sum = 0;
	printf("call loop\n");
	while (1) {
		for (i = 0; i < 100000000; ++i) {
			sum += add(i);
		}
		printf("value is %d\n", sum);
	}
}


/* lots of cache read misses */
#define MAXLINES (65536)
static int mem[MAXLINES * 8];
void cache_read_loop(char *lines)
{
	int i, j;
	int sum = 0;
	int *ptr;
	int num_lines = MAXLINES;
	if (*lines == ':') {
		num_lines = atoi(&lines[1]);
		if (num_lines > MAXLINES)
			num_lines = MAXLINES;
		if (num_lines < 8)
			num_lines = 8;

	}

	ptr = mem + num_lines * 8 - 8;
	for (i = num_lines; i > 0; i--) {
		*ptr = i;
		ptr -= 8;
	}
	printf("Cache read test using %d cache lines\n", num_lines);
	while (1) {
		for (j = 0; j < 100000; ++j) {
			ptr = mem + num_lines * 8 - 8;
			for (i = num_lines; i > 0; i -= 8) {
				asm volatile (
				"	add.4 %1, (%0)-32++, %1	\n"
				"	add.4 %1, (%0)-32++, %1	\n"
				"	add.4 %1, (%0)-32++, %1	\n"
				"	add.4 %1, (%0)-32++, %1	\n"
				"	add.4 %1, (%0)-32++, %1	\n"
				"	add.4 %1, (%0)-32++, %1	\n"
				"	add.4 %1, (%0)-32++, %1	\n"
				"	add.4 %1, (%0)-32++, %1	\n"
				: "+a" (ptr), "+d" (sum)
				);
			}
		}
		printf("value is %d\n", sum);
	}
}

void loop_loop(void)
{
	int i;
	int sum = 0;
	printf("looping\n");
	while (1) {
		for (i = 0; i < 100000000; ++i) {
			sum += i << 3;
		}
		printf("value is %d\n", sum);
	}
}

/* lots of writes that hit the cache, to fill the write buffer */
void write_loop(void)
{
	static volatile int buf[8];
	while(1) {
		asm volatile (
			"move.4  (%0), (%0)	\n\t"
			"move.4  4(%0), (%0)	\n\t"
			"move.4  8(%0), (%0)	\n\t"
			"move.4  12(%0), (%0)	\n\t"
			"move.4  16(%0), (%0)	\n\t"
			"move.4  20(%0), (%0)	\n\t"
			"move.4  24(%0), (%0)	\n\t"
			"move.4  28(%0), (%0)	\n\t"
		:
		: "a" (buf)
		);
	}
}

void unaligned_loop(void)
{
	int j;
	volatile int sum[100] = {0}, *ptr;
	while(1) {
		for (j = 0; j < 100; ++j) {
			sum[j] += j;
		}
		ptr = &sum[0];
		ptr = (int *)((char *)ptr + 1);
		sum[0] += *ptr;	/* unaligned access */
	}
}

void imiss_loop(void)
{
	while (1) {
		asm volatile (
			"1:			\n\t"
			".rept 65000		\n\t"
			"nop			\n\t"
			".endr			\n\t"
			"jmpt.t 1b		\n\t"
		);
	}
}

void libc_loop(void)
{
	FILE *f;
	char buf[200];
	printf("libc loop\n");
	f = fopen("/dev/null", "w");
	while(1) {
		sprintf(buf, "%d, %s", 57, "hi");
		strcat(buf, "more text");
		fwrite(buf, strlen(buf), 1, f);
	}
	fclose(f);
}

char *mystrcpy(char *s1, char *s2) __attribute__((noinline));
char *mystrcpy(char *s1, char *s2)
{
	char *a1 = s1;
	char *a2 = s2;
	asm volatile (
	"1:	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpeq.s.f	2f		\n"
	"					\n"
	"	ext.1	(%0)1++, (%1)1++	\n"
	"	jmpne.s.t	1b		\n"
	"					\n"
	"2:					\n"
		: "+a" (a1), "+a" (a2)
		:
		: "cc", "memory"
	);
	return s1;
}

void strcpy_loop(void)
{
	char s1[31];
	printf("strcpy loop\n");
	while (1) {
		mystrcpy (s1, "DHRYSTONE PROGRAM, 2'ND STRING");
	}
}

void *pthread_function(void *ptr)
{
	int thread = (int)ptr;
	printf("thread %d starts as pid %d\n", thread, getpid());
	switch(thread) {
		case 1:
			strcpy_loop();
		case 2:
			call_loop();
		case 3:
			write_loop();
		case 4:
			hazard_adr_loop();
		case 5:
			hazard_jmp_loop();
		default:
			loop_loop();
	}
	printf("thread %d exits\n", thread);
	return NULL;
}

#define NUM_PTHREAD 8
void pthread_loop(void)
{
	int i, tid;
	pthread_t thread[NUM_PTHREAD];
	for (i = 0; i < NUM_PTHREAD; ++i) {
		tid = pthread_create(&(thread[i]), NULL, pthread_function, (void *)i);
		if (tid) {
			printf("thread %d failed to start\n", i);
		}
	}
	loop_loop();
}

/* 
 * attempt to cause many ptec conflict misses
 *    ptec is two way associative, 16 KB of memory mapped for each entry
 *    default is 256 entries per way, 512 entries total, covering 8 MB of virtual memory
 *    each entry is 6 bytes, so default is 3 KB of OCM
 */
#define PTEC_SIZE 512
#define PAGE_BYTES (16 * 1024)
void ptec_loop(void)
{
	int pages = PTEC_SIZE * 8;
	int size = PAGE_BYTES * pages;
	int i, loops = 0;
	int sum;
	volatile char *ptr;
	volatile char *memory = malloc(size);
	if (!memory) {
		printf("can't malloc %d bytes\n");
		exit(1);
	}
	sum = 0;
	while (1) {
		ptr = memory;
		for (i = 0; i < pages / 2; i += 8) {
			sum + *ptr;
			*(ptr + PAGE_BYTES) = *(ptr + PAGE_BYTES * 2);
			*(ptr + PAGE_BYTES * 3) = *(ptr + PAGE_BYTES * 3);
			*(ptr + PAGE_BYTES * 4) = *(ptr + PAGE_BYTES * 4 + PAGE_BYTES * PTEC_SIZE);
			ptr += PAGE_BYTES * 8;
		}
		loops++;
		if (loops > 1000000000) {
			printf ("sum = %d\n", sum);
			loops = 0;
		}
	}
}


int segv_count = 0;

void sig_handler(int signal)
{
//	printf("in segv handler\n");
	segv_count++;
	if (segv_count > 100000) {
		printf("Getting expected segv\n");
		segv_count = 0;
	}
}

void segv_loop()
{
	struct sigaction act;
	char *memory, tmp;

	// set sig_segv handler
	act.sa_handler = sig_handler;
	sigemptyset (&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGSEGV, &act, NULL);	

	// set up private memory region
	memory = mmap(0, 16 * 1024, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	// read memory once to get the page allocated and into the TLB
	tmp = *memory;

	// cause segv infinite loop, since handler doesn't fix it
	*memory = 0;

	printf("Oops, failed to cause expected segv, tmp is %c\n", tmp);
	exit(1);
}

#if UBICOM32_ARCH_VERSION >= 5

volatile char flush_buf[96];
void flush_miss_loop(void)
{
	volatile char *ptr = &flush_buf[40];
	ptr = (volatile char *)((unsigned int)ptr & ~0x1f);		// align to 32 bytes
	while (1) {
		asm volatile(
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   flush (%0)		\n"
		: 
		: "a" (ptr)
		: "cc", "memory"
		);
	}
}

void flush_clean_loop(void)
{
	volatile char *ptr = &flush_buf[40];
	ptr = (volatile char *)((unsigned int)ptr & ~0x1f);		// align to 32 bytes
	while (1) {
		asm volatile(
			"   move.4   d0, (%0)	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   flush (%0)		\n"
		: 
		: "a" (ptr)
		: "cc", "memory", "d0"
		);
	}
}

void flush_dirty_loop(void)
{
	volatile char *ptr = &flush_buf[40];
	ptr = (volatile char *)((unsigned int)ptr & ~0x1f);		// align to 32 bytes
	while (1) {
		asm volatile(
			"   move.4   (%0), #1	\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   nop			\n"
			"   flush (%0)		\n"
		: 
		: "a" (ptr)
		: "cc", "memory"
		);
	}
}

#endif

void usage(void)
{
	printf("Usage: loop -option\n");
	printf("options are:\n");
	printf("  -help, --help, --h   print this message\n");
	printf("  -loop                jmp hazard in a loop\n");
	printf("  -call                call and jmp hazards in loop\n");
	printf("  -cache_read[:lines]  maximize read cache misses to lines cache lines (default and maximum 65536]\n");
	printf("  -write               maximize writes hitting cache\n");
	printf("  -unaligned           unaligned reads\n");
	printf("  -imiss               Instruction cache misses\n");
	printf("  -libc                library calls\n"); 
	printf("  -strcpy              string copy\n");
	printf("  -hazard_adr          address hazards\n");
	printf("  -hazard_jmp          jump hazards\n");
	printf("  -pthread             pthreads\n");
	printf("  -ptec                ptec misses\n");
	printf("  -segv                 copy on write\n");
#if UBICOM32_ARCH_VERSION >= 5
	printf("  -flush_miss          flush misses cache\n");
	printf("  -flush_clean         flush hits clean cache\n");
	printf("  -flush_dirty         flush hits dirty cache\n");
#endif
//	printf("Arch version is %d\n", UBICOM32_ARCH_VERSION);
}



int main(int argc, char **argv)
{
	if (argc <= 1) {
		usage();
		exit(0);
	}

	if (strcmp(argv[1], "-call") == 0) {
		call_loop();
	} else if (strcmp(argv[1], "-loop") == 0) {
		loop_loop();
	} else if (strncmp(argv[1], "-cache_read", 11) == 0) {
		cache_read_loop(&argv[1][11]);
	} else if (strcmp(argv[1], "-unaligned") == 0) {
		unaligned_loop();
	} else if (strcmp(argv[1], "-write") == 0) {
		write_loop();
	} else if (strcmp(argv[1], "-imiss") == 0) {
		imiss_loop();
	} else if (strcmp(argv[1], "-libc") == 0) {
		libc_loop();
	} else if (strcmp(argv[1], "-strcpy") == 0) {
		strcpy_loop();
	} else if (strcmp(argv[1], "-hazard_adr") == 0) {
		hazard_adr_loop();
	} else if (strcmp(argv[1], "-hazard_jmp") == 0) {
		hazard_jmp_loop();
	} else if (strcmp(argv[1], "-pthread") == 0) {
		pthread_loop();
	} else if (strcmp(argv[1], "-ptec") == 0) {
		ptec_loop();
	} else if (strcmp(argv[1], "-segv") == 0) {
		segv_loop();
#if UBICOM32_ARCH_VERSION >= 5
	} else if (strcmp(argv[1], "-flush_miss") == 0) {
		flush_miss_loop();
	} else if (strcmp(argv[1], "-flush_clean") == 0) {
		flush_clean_loop();
	} else if (strcmp(argv[1], "-flush_dirty") == 0) {
		flush_dirty_loop();
#endif
	} else if (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "--h") == 0) {
		usage();
	} else {
		printf("Unrecognized option\n");
		usage();
	}
	return 0;
}
