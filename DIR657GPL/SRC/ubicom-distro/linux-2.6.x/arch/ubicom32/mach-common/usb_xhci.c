/*
 * arch/ubicom32/mach-common/usb_xhci.c
 *   Ubicom32 architecture usb support for xHCI.
 *
 * (C) Copyright 2010, Ubicom, Inc.
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
 */
#include <linux/profile.h>
#include <asm/ubicom32-asm.h>
#include <asm/ubicom32-io.h>

#ifdef CONFIG_ATOMIC_INDIRECT_ACCESS
#define BR_INDIRECT_LOCK(flags) __atomic_lock_acquire()
#define BR_INDIRECT_UNLOCK(flags) __atomic_lock_release()
#else
#include <asm/processor.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
static spinlock_t br_indirect_lock = SPIN_LOCK_UNLOCKED;
#define BR_INDIRECT_LOCK(flags) spin_lock_irqsave(&br_indirect_lock, flags)
#define BR_INDIRECT_UNLOCK(flags) spin_unlock_irqrestore(&br_indirect_lock, flags)
#endif

/*
 * Read a register indirectly from a blocking region of xHC controller
 * 	addr: the register address
 * 	base: the PIO function base of the xHC core
 */
unsigned int ubi32_xhci_readl(const volatile void __iomem *addr, const volatile void __iomem *base)
{
	unsigned int val;
	volatile void __iomem *ctl0 = FUNCTION_CTL0(base);
	volatile void __iomem *br_base = FUNCTION_BR_BASE(base); 
	long offset = FUNCTION_BR_OFFSET(addr, br_base);
#ifndef CONFIG_ATOMIC_INDIRECT_ACCESS
	unsigned long flags;
#endif

	BR_INDIRECT_LOCK(flags);

	/* write BR offset into CTR0, wait for 5 cycles, then read the value */
	asm volatile (
		" move.4 %[ctl0], %[offset]			\n"
		" cycles 5					\n"
		" move.4 %[val], (%[br_base])			\n"
		: [val] "=&d" (val)
		: [ctl0] "a" (ctl0), [br_base] "a" (br_base), [offset] "d" (offset)
		: "cc"
		);

	BR_INDIRECT_UNLOCK(flags);

	return val;
}

/*
 * Write a new value indirectly to a blocking region register of xHC core 
 * 	addr: the register address
 * 	base: the PIO function base of the xHC core
 */
void ubi32_xhci_writel(unsigned int val, const volatile void __iomem *addr, const volatile void __iomem *base)
{
	volatile void __iomem *ctl0 = FUNCTION_CTL0(base);
	volatile void __iomem *br_base = FUNCTION_BR_BASE(base); 
	long offset = FUNCTION_BR_OFFSET(addr, br_base);
#ifndef CONFIG_ATOMIC_INDIRECT_ACCESS
	unsigned long flags;
#endif

	BR_INDIRECT_LOCK(flags);

	/* write BR offset into CTR0, wait for 5 cycles, then write the value */
	asm volatile (
		" move.4 %[ctl0], %[offset]			\n"
		" cycles 5					\n"
		" move.4 (%[br_base]), %[val]			\n"
		: 
		: [ctl0] "a" (ctl0), [br_base] "a" (br_base), [offset] "d" (offset), [val] "d" (val)
		: "cc"
		);

	BR_INDIRECT_UNLOCK(flags);
}

#define XHCI_CTL1_WSTALL_BIT (1 << 31)
#define XHCI_STATUS0_WCMDQ_EMPTY_BIT (1 << 31)

/* Helper macros handling registers */
#define XHCI_REG_TST(reg, val) ( *((volatile unsigned int *)(reg)) & val )
#define XHCI_REG_SET(reg, val) { volatile unsigned int *r = (unsigned int *)(reg); *r = (*r | (val)); }
#define XHCI_REG_CLR(reg, val) { volatile unsigned int *r = (unsigned int *)(reg); *r = (*r & ~(val)); }
#define XHCI_REG_WRITE(reg, val) { volatile unsigned int *r = (unsigned int *)(reg); *r = (val); }
#define XHCI_REG_READ(reg, val) { volatile unsigned int *r = (unsigned int *)(reg); val = *r; }

void ubi32_xhci_dma_ready(void __iomem *fbase)
{
	/* polls dma_wcmdq_empty bit of STATUS0 */
	if (likely(XHCI_REG_TST(fbase + IO_STATUS0, XHCI_STATUS0_WCMDQ_EMPTY_BIT)))
		return;

	/* tell device stop feeding till the current written data is ready */
	XHCI_REG_SET(fbase + IO_CTL1, XHCI_CTL1_WSTALL_BIT);

	/* keep polling till it is ready */
	while (!XHCI_REG_TST(fbase + IO_STATUS0, XHCI_STATUS0_WCMDQ_EMPTY_BIT)) {}

	/* clear the stall bit */
	XHCI_REG_CLR(fbase + IO_CTL1, XHCI_CTL1_WSTALL_BIT);
}

