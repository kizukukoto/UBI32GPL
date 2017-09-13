#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/types.h>

#define KMEM	"/dev/mem"


int reg_read(__u32 phyaddr, __u32 len, __u32 *out)
{
	int fd, i, e = 0;
	__u32 *reg;
	__u32 base, offset;
	char *t;


	offset = base = phyaddr;
	/* Force base is scale on page size */
	i = getpagesize();
	base = ((base - (i - 1)) / i) * i + i;
	offset = offset - base;
	
	fd = open(KMEM, O_RDWR);
	if (fd < -1) {
		perror("open mem failure: ");
		return -1;
	}

	reg = (__u32 *)mmap(NULL, len + offset, 
			PROT_READ, MAP_SHARED, 
			fd, base);
	
	if (reg == MAP_FAILED) {
		puts("mmap kmem failure");
		perror("");
		exit(EXIT_FAILURE);
	}
	/* To avoid of optimize by @memcpy, it will case a "Bus Error" such as:
	 *	Unhandled fault: external abort on non-linefetch (0x01a) at 0x4001702c
	 *	Bus error
	 * volatile ?
	 */
	for (i = offset; i < (len + offset); i += 4) {
		//printf("%.4X: %.8X\n", i, reg[i / sizeof(*reg)]);
		out[e++] = reg[i / sizeof(*reg)];
	}
	munmap(reg, len);
	close(fd);
	return 0;
}
#if 0
/* Test code */
int main(int argc, char *argv[])
{
	__u32 regs[1024], addr, len, i;
	char *t;
	
       	addr = strtoul(argv[1], &t, 0);
	len = strtoul(argv[2], &t, 0);
	//printf("%X, %X\n", addr, len);
	reg_read(addr, len, regs);
	printf("\nMAIN\n");
	for (i = 0; i < len; i += 4) {
		printf("%.4X: %.8X\n", i, regs[i / sizeof(*regs)]);
	}
}
#endif
#if 0
/* orignal source code */
char *program;

void print_usage(char *info, int exit_code)
{
	printf("%s%s START [LEN]\n"
			"\t dump kernel space memory\n", info, program);
	exit(exit_code);
}
int main(int argc, char *argv[])
{
	int fd, i;
	__u32 *reg, len = 0x100;
	__u32 base, offset;
	char *t;

	if (argc < 2)
		print_usage("", EXIT_FAILURE);
	else if (argc >= 3) {
		len = strtoul(argv[2], &t, 0);
		if (*t != '\0')
			print_usage("invaild length\n",
					EXIT_FAILURE);
	}

	offset = base = strtoul(argv[1], &t, 0);
	if (*t != '\0')
		print_usage("invaild base\n", EXIT_FAILURE);
	/* Force base is scale on page size */
	i = getpagesize();
	base = ((base - (i - 1)) / i) * i + i;
	offset = offset - base;

	fd = open(KMEM, O_RDWR);
	if (fd < -1) {
		puts("open kmem failure");
		exit(EXIT_FAILURE);
	}

	printf("MAP %.8X(%d)\n", base, len);
	reg = (__u32 *)mmap(NULL, len + offset, 
			PROT_READ, MAP_SHARED, 
			fd, base);
	if (reg == MAP_FAILED) {
		puts("mmap kmem failure");
		perror("");
		exit(EXIT_FAILURE);
	}

	for (i = offset; i <= len; i += 4) {
		printf("%.4X: %.8X\n", i, reg[i / sizeof(*reg)]);
	}

	munmap(reg, len);
	close(fd);
}
#endif
