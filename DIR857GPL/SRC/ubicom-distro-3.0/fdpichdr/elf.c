/*
 * File:         elf.c
 * Author:       Mike Frysinger <michael.frysinger@analog.com>
 * Description:  Hide all the ELF logic here.
 * Modified:     Copyright 2006-2008 Analog Devices Inc.
 * Bugs:         Enter bugs at http://blackfin.uclinux.org/
 * Licensed under the GPL-2, see the file COPYING in this dir
 */

#include "headers.h"
#include "helpers.h"
#include "elf.h"

/* valid elf buffer check */
#define IS_ELF_BUFFER(buff) \
	(buff[EI_MAG0] == ELFMAG0 && \
	 buff[EI_MAG1] == ELFMAG1 && \
	 buff[EI_MAG2] == ELFMAG2 && \
	 buff[EI_MAG3] == ELFMAG3)
/* for now, only handle 32bit little endian as it
 * simplifies the input code greatly */
#define DO_WE_LIKE_ELF(buff) \
	((buff[EI_CLASS] == ELFCLASS32 /*|| buff[EI_CLASS] == ELFCLASS64*/) && \
	 (buff[EI_DATA] == ELFDATA2MSB /*|| buff[EI_DATA] == ELFDATA2MSB*/) && \
	 (buff[EI_VERSION] == EV_CURRENT))

char do_reverse_endian;

int set_stack(uint32_t stack, char *argv[])
{
	int fd;
	int i, set_stack_ret;
	uint16_t p;
	ssize_t ss_ret;
	uint32_t put_stack;

	set_stack_ret = 0;
	for (i = 0; argv[i]; ++i) {
		Elf32_Ehdr ehdr;

		if (verbose)
			printf("%s: setting stack size to %u (0x%x)\n", argv[i], stack, stack);

		fd = open(argv[i], O_RDWR);
		if (fd == -1) {
			if (!quiet)
				warnp("%s: unable to open", argv[i]);
			++set_stack_ret;
			continue;
		}

		/* Verify this is a usuable ELF */
		ss_ret = read(fd, &ehdr, sizeof(ehdr));
		if (ss_ret != sizeof(ehdr)) {
			if (!quiet)
				warnp("%s: file is too small to be an ELF", argv[i]);
			goto err_close_cont;
		}
		if (!IS_ELF_BUFFER(ehdr.e_ident)) {
			if (!quiet)
				warn("%s: file is not an ELF", argv[i]);
			goto err_close_cont;
		}
		if (!DO_WE_LIKE_ELF(ehdr.e_ident)) {
			if (!quiet)
				warn("%s: file is not an ELF we can work with", argv[i]);
			goto err_close_cont;
		}

		do_reverse_endian = (ELF_DATA != ehdr.e_ident[EI_DATA]);
		ESET(put_stack, stack);

		/* Grab the elf header */
		if (lseek(fd, EGET(ehdr.e_phoff), SEEK_SET) != EGET(ehdr.e_phoff)) {
			if (!quiet)
				warnp("%s: unable to seek to program header table", argv[i]);
			goto err_close_cont;
		}

		/* Scan the program headers and find GNU_STACK */
		for (p = 0; p < EGET(ehdr.e_phnum); ++p) {
			Elf32_Phdr phdr;
			ss_ret = read(fd, &phdr, sizeof(phdr));
			if (ss_ret != sizeof(phdr)) {
				if (!quiet)
					warnp("%s: unable to read program header %i", argv[i], p);
				goto err_close_cont;
			}

			if (EGET(phdr.p_type) == PT_GNU_STACK) {
				if (verbose)
					printf("\tFound PT_GNU_STACK; changing value 0x%X -> 0x%X\n", EGET(phdr.p_memsz), stack);
				lseek(fd, -sizeof(phdr) + offsetof(Elf32_Phdr, p_memsz), SEEK_CUR);
				write(fd, &put_stack, sizeof(put_stack));
			} else if (verbose)
				printf("\tskipping program header 0x%X\n", EGET(phdr.p_type));
		}

		close(fd);
		continue;

err_close_cont:
		++set_stack_ret;
		close(fd);
	}
	
	return set_stack_ret;
}
