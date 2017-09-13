/*
 * arch/ubicom32/include/asm/mmu_ptec.h
 *   Interface to the Page Table Cache that Hardware "walks".
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
#ifndef _ASM_UBICOM32_MMU_PTEC_H
#define _ASM_UBICOM32_MMU_PTEC_H

/*
 * Each PTEC_TAG has 6 bits of tag from the VPN
 * and 10 bits of asid value.  Use defines instead
 * of bit fields for both speed and because C
 * has trouble assigning computed values to structs.
 */
#define PTEC_TAG_ASID_SHIFT 0
#define PTEC_TAG_ASID_BITS 10
#define PTEC_TAG_TAG_SHIFT 10
#define PTEC_TAG_TAG_BITS 6

#if !defined(__ASSEMBLY__)
struct ptec_tag {
	unsigned short tag0;
	unsigned short tag1;
};
#endif

/*
 * A PTEC entry
 */
#define PTEC_ENTRY_PFN_SHIFT 12
#define PTEC_ENTRY_PFN_BITS 20
#define PTEC_ENTRY_RTAG_SHIFT 5
#define PTEC_ENTRY_RTAG_BITS 7
#define PTEC_ENTRY_READ_SHIFT 5
#define PTEC_ENTRY_READ_BITS 1
#define PTEC_ENTRY_WRITE_SHIFT 5
#define PTEC_ENTRY_WRITE_BITS 1
#define PTEC_ENTRY_EXEC_SHIFT 5
#define PTEC_ENTRY_EXEC_BITS 1
#define PTEC_ENTRY_USER_SHIFT 5
#define PTEC_ENTRY_USER_BITS 1
#define PTEC_ENTRY_PAD_SHIFT 5
#define PTEC_ENTRY_PAD_BITS 1

/*
 * Mask used to clear RTAG bits from a PTE the bits are likely used by
 * Linux and must be cleared before loading into TLB.  Create a 16 bit
 * value for use with movei.
 *
 * ~(((1 << PTEC_ENTRY_RTAG_BITS) - 1) << PTEC_ENTRY_RTAG_SHIFT)
 */
#define PTEC_ENTRY_RTAG_MASK 0x0fe0


#endif /* _ASM_UBICOM32_MMU_PTEC_H */
