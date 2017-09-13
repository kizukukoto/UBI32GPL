/*
 * arch/ubicom32/include/asm/segment.h
 *   Memory segment definitions for Ubicom32 architecture.
 *
 * (C) Copyright 2009, Ubicom, Inc.
 *
 * This file is part of the Ubicom32 Linux Kernel Port.
 *
 * The Ubicom32 Linux Kernel Port is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Ubicom32 Linux Kernel Port is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Ubicom32 Linux Kernel Port.  If not, 
 * see <http://www.gnu.org/licenses/>.
 *
 * Ubicom32 implementation derived from (with many thanks):
 *   arch/m68knommu
 *   arch/blackfin
 *   arch/parisc
 */
#ifndef _ASM_UBICOM32_SEGMENT_H
#define _ASM_UBICOM32_SEGMENT_H

#ifndef __ASSEMBLY__

/*
 * For non-MMU systems, all of physical memory is in the same segment.
 * For MMU, systems, the mapped space is all user (today) and the
 * unmapped space is where kernel space goes.
 */
typedef struct {
	unsigned long seg;
} mm_segment_t;

#define MAKE_MM_SEG(s)	((mm_segment_t) { (s) })

#if !defined(CONFIG_MMU)
#define USER_DS		MAKE_MM_SEG(-1UL)
#define KERNEL_DS	MAKE_MM_SEG(-1UL)
#else
#define USER_DS		MAKE_MM_SEG(0xa0000000UL)
#define KERNEL_DS	MAKE_MM_SEG(-1UL)
#endif

#define get_ds()	(KERNEL_DS)
#define get_fs()	(current_thread_info()->addr_limit)
#define set_fs(x)	(current_thread_info()->addr_limit = (x))
#define segment_eq(a,b)	((a).seg == (b).seg)


/*
 * Ubicom32 shares a 32 bit address space between mapped and unmapped
 * space.  The use of the mapped region is entirely software driven.
 * The following segmentation defines the layout of memory.
 *
 * NOTE: If you change this layout, you might need to update
 * VMALLOC_START in pgtable.h and TASK_SIZE in processor.h
 *
 * 0x00000000		- PAGE_SIZE  	- Dead Page
 * PAGE_SIZE		- End Data	- Executable + Data
 * End Data		- MAPPED_START	- Break Area
 * MAPPED_START		- Stack		- Mapped Object Area
 * 0xa0000000		- Negative	- Stack
 * 0xa0000000		- 0xafffffff	- Kernel Virtual Space
 * 0xb0000000		- 0xffffffff	- Unmapped Space
 */
#define UBICOM32_FDPIC_TEXT_START	(PAGE_SIZE)
#define UBICOM32_FDPIC_MAPPED_START	(0x70000000UL)
#define UBICOM32_FDPIC_STACK_END	UBICOM32_FDPIC_KERNEL_START
#define UBICOM32_FDPIC_KERNEL_START	(0xa0000000UL)
#define UBICOM32_FDPIC_KERNEL_END	UBICOM32_FDPIC_UNMAPPED_START
#define UBICOM32_FDPIC_UNMAPPED_START	(0xb0000000UL)

/*
 * ASID values shared for kernel, unique for user.
 */
#define UBICOM32_ASID_ILLEGAL 0
#define UBICOM32_ASID_KERNEL 1
#define UBICOM32_ASID_START 2

#endif /* __ASSEMBLY__ */

#endif /* _ASM_UBICOM32_SEGMENT_H */
