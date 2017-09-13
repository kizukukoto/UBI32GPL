/*
 * arch/ubicom32/include/asm/tlb.h
 *   Ubicom32 architecture TLB operations.
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
#ifndef _ASM_UBICOM32_TLB_H
#define _ASM_UBICOM32_TLB_H

#include <linux/pagemap.h>

#include <asm/pgalloc.h>
#include <asm/mmu_hardware.h>
#include <asm/mmu_ptec.h>

struct tlb_miss_per_thread {
	unsigned int missqw0;
	unsigned int missqw1;
}; 

extern volatile struct tlb_miss_per_thread tlb_missq[];
extern volatile unsigned int tlb_faulted_thread_mask;

extern void tlb_purge_all(void);
extern void tlb_purge_range(unsigned int asid, unsigned long start, unsigned long end);
extern void tlb_purge_asid(unsigned int asid);
extern void tlb_purge_page(unsigned int asid, unsigned long address);
extern void tlb_update_entry(int target, unsigned int asid, unsigned long addr, pte_t pte);

/*
 * tlb_threads_blocked_not_interruptable()
 *	Decide if a thread can handle an interrupt.
 *
 * Thread that are blocked by the MMU can not be interrupted.
 *
 * Threads that are blocked by I/O can not be interrupted.
 *
 * The hardware does not differentiate between I/O blocked and OCM
 * blocked so sometimes we will avoid interrupting a thread that is
 * OCM blocked instead of MMU or I/O blocked.
 */
static inline unsigned long tlb_threads_blocked_not_interruptable(void)
{

	/*
	 * Bus_state0 is blocked or unblocked. Bus_state3 is blocked
	 * on the cache.  If we are not blocked on the cache, we are
	 * blocked on MMU, OCM or I/O.
	 */
	unsigned int bus_state0 = ubicom32_read_reg((volatile int *)MMU_BUS_ST0);
	unsigned int bus_state1 = ubicom32_read_reg((volatile int *)MMU_BUS_ST1);
	unsigned int bus_state2 = ubicom32_read_reg((volatile int *)MMU_BUS_ST2);
	unsigned int bus_state3 = ubicom32_read_reg((volatile int *)MMU_BUS_ST3);
	return bus_state0 & ((bus_state1 & ~bus_state3) | (~bus_state1 & bus_state2));
}

/*
 * tlb_faulted_threads()
 *	Returns the threads that are currently held for faults.
 */
static inline unsigned int tlb_faulted_threads(void)
{
	return ubicom32_read_reg(&tlb_faulted_thread_mask);
}

/*
 * tlb_faulted_clear()
 *	Remove the thread from the list of faulted threads.
 *
 * Use assembly to ensure that the inst is atomic.
 */
static inline void tlb_faulted_clear(thread_t tid)
{
	asm volatile (
	"	and.4	(%[faulted]), (%[faulted]), %[tmask]	\n\t"
		:
		: [faulted]"a" (&tlb_faulted_thread_mask), [tmask]"r" (~(1 << tid))
		: "cc", "memory"
		);
}

/*
 * tlb_faulted_set()
 *	Set the faulted thread bit.
 *
 * Use assembly to ensure that the inst is atomic.
 */
static inline void tlb_faulted_set(thread_t tid)
{
	asm volatile (
	"	or.4	(%[faulted]), (%[faulted]), %[tmask]	\n\t"
		:
		: [faulted]"a" (&tlb_faulted_thread_mask), [tmask]"r" (1 << tid)
		: "cc", "memory"
	);
}

/*
 * tlb_faulted_missq()
 *	Obtain the missq information for the faulted thread.
 */
static inline void tlb_faulted_missq(thread_t tid, unsigned int *missqw0, unsigned int *missqw1)
{
	/*
	 * The entry can only be called from the thread that owns the
	 * entry and the entry will not be written again by any thread
	 * until the faulted bit is clear.  Thus no locking is
	 * necessary.
	 */
	*missqw0 = tlb_missq[tid].missqw0;
	*missqw1 = tlb_missq[tid].missqw1;
}

#define __tlb_remove_tlb_entry(tlb, ptep, address) \
	tlb_purge_page((tlb)->mm->context.asid, (address))

#define tlb_flush(tlb) tlb_purge_asid((tlb)->mm->context.asid)

/*
 * For now, we don't do anything special for vma or pte handling.
 */
#define tlb_start_vma(tlb, vma)	do { } while (0)
#define tlb_end_vma(tlb, vma)	do { } while (0)

#include <asm-generic/tlb.h>

#define __pte_free_tlb(tlb, x)	pte_free((tlb)->mm, (x));

DECLARE_PER_CPU(struct mmu_gather, mmu_gathers);

extern void tlb_init(void);

#endif /* _ASM_UBICOM32_TLB_H */
