/*
 * arch/ubicom32/mm/context.c
 *   Task context initialization
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
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pagemap.h>

#include <asm/mmu_context.h>
#include <asm/tlbflush.h>
#include <asm/ubicom32.h>

/*
 * An ASID value of 0 is invalid.
 */
#define ROLLOVER_VALUE_ILLEGAL 0
#define ASID_BITS 10
#define ASID_MASK ((1 << ASID_BITS) - 1)

static DEFINE_SPINLOCK(asid_lock);
static unsigned int asid_rotator = UBICOM32_ASID_START;
static long rollover_rotator = 1;

/*
 * asid_increment()
 *	Increment the ASID counter based on size of the ASID.
 */
static unsigned int asid_increment(unsigned int asid)
{
	return ((asid + 1) & ASID_MASK);
}

/*
 * context_assign_kernel()
 *	Enable use of the Kernel ASID before a full process context is established.
 */
void context_assign_kernel(void)
{
	unsigned int val;
	thread_t tid = thread_get_self();

#if 0
	printk("[%d]: switching to kernel only mode\n", tid);
#endif

	ubicom32_write_reg((volatile unsigned int *)MMU_TNUM_PGD(tid), (u32_t)init_mm.pgd);
	ubicom32_write_reg((volatile unsigned int *)MMU_TNUM_ASIDS(tid), MMU_TNUM_ASID0_PUT(UBICOM32_ASID_KERNEL));
	val = MMU_TNUM_CMP_EN0_PUT(1) | MMU_TNUM_ASID_CMP0_PUT(0xa);
	ubicom32_write_reg((volatile unsigned int *)MMU_TNUM_ASID_CMP(tid), val);
}

/*
 * context_assign()
 *	Assign an ASID for this mm context on each use.
 *
 * The MMU for Ubicom32 has both a hardware TLB and a larger
 * On-Chip-Memory (OCM) based page table cache (PTEC).  
 *
 * In a classic TLB implementation, the TLB must be purged on context
 * switch.  This can result is significant performance overhead.
 * 
 * To avoid the expense of TLB purging (which for Ubicom32 requires both
 * a hardware purge and a PTEC purge), the TLB extends the address space
 * with an Address Space ID (ASID).  The ASID means that 2^n address
 * spaces can be uniquely identified.  For Ubicom32 the ASID is 10 bits,
 * giving us 1024 unique address spaces.
 *
 * Once the system uses/needs more than 2^10 unique address spaces, the
 * MMU will need to be purged (TLB/PTEC).  After the purge, some
 * mechanism must be used to ensure that previous ASID values are not
 * still "valid" in the system.  Since the PTEC can be large, chances
 * are that TLB mappings can remain in the PTEC across several context
 * switches.
 *
 * Several approaches can be used to prevent ASID re-use.  One could
 * search the Linux data structures (mm), one could maintain a bitmap of
 * in use ASIDs, or one could treat each new "set" of ASIDs as a unique
 * set using a rollover id to ensure that old ASIDs are reassigned
 * before an older tasks goes to use the ASID.  This later approach is
 * the simplest to implement and highest performance.
 *
 * The rollover counter is a 64 bit counter that is incremented each
 * time the ASID counter "rolls over".  If the "roll over" values in
 * the mm and the system do not match, the mm will be assigned a new
 * ASID on next use.
 *
 * If the ASID counter rolls over, the rollover counter is incremented
 * and the TLB and PTEC are purged.  This is necessary to avoid stale
 * re-use of ASID values.
 *
 * TODO: Do we want to enhance the algorithm to provide a unique asid
 * per shared vma.
 */
void context_assign(struct mm_struct *mm)
{	
	unsigned int val;
	int need_purge = 0;
	thread_t tid = thread_get_self();
	unsigned long flags;

	/*
	 * Track that the VM is run on this logical CPU.
	 * TODO: Are we really going to us this?
	 */
	mm->cpu_vm_mask = cpumask_of_cpu(smp_processor_id());

	/*
	 * We try to keep using the same ASID increasing the value of
	 * the PTEC.
	 */
	spin_lock_irqsave(&asid_lock, flags);
	if (mm->context.rollover == rollover_rotator) {
		spin_unlock_irqrestore(&asid_lock, flags);
		goto mmu_update;
	}

	mm->context.rollover = rollover_rotator;
	mm->context.asid = asid_rotator;
	asid_rotator = asid_increment(asid_rotator);
	if (asid_rotator == UBICOM32_ASID_ILLEGAL) {
		/*
		 * The asid_rotator has rolled over, change
		 * the rollover_rotator.
		 *
		 * TODO: Are we sure that this rollover is rare enough?
		 */
		if (++rollover_rotator == 0) {
			spin_unlock_irqrestore(&asid_lock, flags);
			panic("Maximum ASID assignments exceeded\n");
			return;
		}
		need_purge = 1;
		asid_rotator = UBICOM32_ASID_START;
		mm->context.rollover = rollover_rotator;
		mm->context.asid = asid_rotator;
		asid_rotator = asid_increment(asid_rotator);
	}
	spin_unlock_irqrestore(&asid_lock, flags);

	/*
	 * Purge the TLB and PTEC if needed.
	 */
	if (need_purge) {
		flush_tlb_all();
	}

mmu_update:
#if 0
	printk("[%d]: switching to mm[%p], asid[%lx]\n",
	       tid, mm, mm->context.asid);
#endif
	/*
	 * Setup the hardware ASID registers.  Use 0 for kernel space,
	 * and 1 and 2 for user space.
	 *
	 * ASID0 goes from 0xa0000000 (VM_ALLOC_START) to 0xafffffff (VM_ALLOC_END).
	 * ASID1 goes from 0 to 0x7fffffff.
	 * ASID2 goes from 0x80000000 to 0x9fffffff
	 */
	ubicom32_write_reg((volatile unsigned int *)MMU_TNUM_PGD(tid), (u32_t)mm->pgd);

	val = MMU_TNUM_ASID0_PUT(UBICOM32_ASID_KERNEL) |
		MMU_TNUM_ASID1_PUT(mm->context.asid) |
		MMU_TNUM_ASID2_PUT(mm->context.asid);
	ubicom32_write_reg((volatile unsigned int *)MMU_TNUM_ASIDS(tid), val);

	val = MMU_TNUM_ASID_MASK0_PUT(0xf) |
		MMU_TNUM_ASID_MASK1_PUT(0x8) |
		MMU_TNUM_ASID_MASK2_PUT(0xe);
	ubicom32_write_reg((volatile unsigned int *)MMU_TNUM_ASID_MASK(tid), val);

	val = MMU_TNUM_CMP_EN0_PUT(1) | MMU_TNUM_ASID_CMP0_PUT(0xa) |
		MMU_TNUM_CMP_EN1_PUT(1) | MMU_TNUM_ASID_CMP1_PUT(0x0) |
		MMU_TNUM_CMP_EN2_PUT(1) | MMU_TNUM_ASID_CMP2_PUT(0x8);
	ubicom32_write_reg((volatile unsigned int *)MMU_TNUM_ASID_CMP(tid), val);
}

/*
 * init_new_context()
 *	Create a new context.
 * 
 * For Ubicom32, we have both the TLB hardware and the larger PTEC
 * that must be managed.  We start by assigning an illegal value
 * to the context.  The running value will be picked up on context
 * switch.
 *
 * Called from Linux fork() code.
 */
int init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
#if 0
	printk("initing new context: %p\n", mm);
#endif
	mm->context.asid = UBICOM32_ASID_ILLEGAL;
	mm->context.rollover = ROLLOVER_VALUE_ILLEGAL;
	return 0;
}

