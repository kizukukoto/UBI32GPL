#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/types.h>

static void help(char *argv0)
{
	printf("usage %s [physical memory address] [length]\n", argv0);
}
/* Test code */
int reg_read_main(int argc, char *argv[])
{
	__u32 regs[1024], addr, len, i;
	char *t;
	
	if (argc < 3) {
		help(argv[0]);
		return 0;
	}
       	addr = strtoul(argv[1], &t, 0);
	len = strtoul(argv[2], &t, 0);

	reg_read(addr, len, regs);
	
	printf("MAP %.8X(%d)\n", addr, len);
	for (i = 0; i < len; i += 4) {
		printf("%.4X: %.8X\n", i, regs[i / sizeof(*regs)]);
	}
}

