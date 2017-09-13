/*
 * arch/ubicom32/kernel/irq.c
 *   Ubicom32 architecture IRQ support.
 *
 * (C) Copyright 2009, Ubicom, Inc.
 * (C) Copyright 2007, Greg Ungerer <gerg@snapgear.com>
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

#include <linux/types.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#include <asm/ubicom32.h>
#include <asm/system.h>
#include <asm/traps.h>
#include <asm/pic.h>
#include <asm/machdep.h>
#include <asm/asm-offsets.h>
#include <asm/thread.h>
#include <asm/devtree.h>

#if defined(CONFIG_MMU)
#include <asm/tlb.h>
#endif

unsigned int irq_soft_avail;
static struct irqaction ubicom32_reserve_action[INTERRUPT_COUNT];

#if defined(CONFIG_UBICOM32_STOPWATCH_IRQ)
#define __STOPWATCH_USE__ 1
#endif
#include <asm/stopwatch.h>
#if defined(CONFIG_UBICOM32_STOPWATCH_IRQ)
/*
 *  Add 1 in for softirq (irq_exit());
 */
DEFINE_STATIC_STOPWATCH_ARRAY(irq_watches, INTERRUPT_COUNT + 1);
#endif

/*
 * irq_kernel_stack_check()
 *	See if the kernel stack is within STACK_WARN of the end.
 */
static void irq_kernel_stack_check(int irq, struct pt_regs *regs)
{
#ifdef CONFIG_DEBUG_STACKOVERFLOW
	unsigned long sp;

	/*
	 * Make sure that we are not close to the top of the stack and thus
	 * can not really service this interrupt.
	 */
	asm volatile (
	"	and.4		%0, SP, %1\n\t"
		: "=r" (sp)
		: "d" (THREAD_SIZE - 1)
		: "cc"
	);

	if (sp < (sizeof(struct thread_info) + STACK_WARN)) {
		printk(KERN_WARNING
			"cpu[%d]: possible overflow detected sp remain: %p, "
		       "irq: %d, regs: %p\n",
			thread_get_self(), (void *)sp, irq, regs);
		dump_stack();
	}

	if (sp < (sizeof(struct thread_info) + 16)) {
		THREAD_STALL;
	}
#endif
}

/*
 * ubicom32_reserve_handler()
 *	Bogus handler associated with pre-reserved IRQ(s).
 */
static irqreturn_t ubicom32_reserve_handler(int irq, void *dev_id)
{
	BUG();
	return IRQ_HANDLED;
}

/*
 * __irq_disable_vector()
 *	Disable the interrupt by clearing the appropriate bit in the
 *	PIC Mask Register.
 */
static void __irq_disable_vector(unsigned int irq)
{
	pic_disable_vector(irq);
}

/*
 * __irq_ack_vector()
 *	Acknowledge the specific interrupt by clearing the associate bit in
 *	hardware
 */
static void __irq_ack_vector(unsigned int irq)
{
	if (irq < 32) {
		asm volatile (
		"	move.4	INT_CLR0, %0\n\t"
			:
			: "d" (1 << irq));
	} else {
		asm volatile (
		"	move.4	 INT_CLR1, %0\n\t"
			:
			: "d" (1 << (irq - 32)));
	}
}

/*
 * __irq_enable_vector()
 *	Clean and then enable the interrupt by setting the appropriate bit in
 *	the PIC Mask Register.
 */
static void __irq_enable_vector(unsigned int irq)
{
	/*
	 * Acknowledge, really clear the vector.
	 */
	__irq_ack_vector(irq);
	pic_enable_vector(irq);
}

/*
 * __irq_mask_vector()
 */
static void __irq_mask_vector(unsigned int irq)
{
	pic_mask_vector(irq);
}

/*
 * __irq_unmask_vector()
 */
static void __irq_unmask_vector(unsigned int irq)
{
	pic_unmask_vector(irq);
}

/*
 * __irq_end_vector()
 *	Called once an interrupt is completed (reset the PIC mask).
 */
static void __irq_end_vector(unsigned int irq)
{
	pic_unmask_vector(irq);
}

#if defined(CONFIG_SMP)
/*
 * __irq_set_affinity()
 *	Set the cpu affinity for this interrupt.
 */
static void __irq_set_affinity(unsigned int irq, cpumask_t dest)
{
	smp_set_affinity(irq, dest);
	irq_desc[irq].affinity = dest;
}
#endif

/*
 * On-Chip Generic Interrupt function handling.
 */
static struct irq_chip ubicom32_irq_chip = {
	.name		= "Ubicom32",
	.startup	= NULL,
	.shutdown	= NULL,
	.enable		= __irq_enable_vector,
	.disable	= __irq_disable_vector,
	.ack		= __irq_ack_vector,
	.mask		= __irq_mask_vector,
	.unmask		= __irq_unmask_vector,
	.end		= __irq_end_vector,
#if defined(CONFIG_SMP)
	.set_affinity	= __irq_set_affinity,
#endif
};

/*
 * do_IRQ()
 *	Primary interface for handling IRQ() requests.
 *
 * This function is force executed when the PIC switches a thread to it.
 */
asmlinkage void do_IRQ(struct pt_regs *regs, unsigned int irq)
{
	struct pt_regs *oldregs;
	struct thread_info *ti = current_thread_info();

	STOPWATCH_DECLARE;

	/*
	 * Mark that we are inside of an interrupt and 
	 * that interrupts are disabled.
	 */
	oldregs = set_irq_regs(regs);
	ti->interrupt_nesting++;
	trace_hardirqs_off();
	irq_kernel_stack_check(irq, regs);

	/*
	 * Start the interrupt sequence
	 */
	irq_enter();

	/*
	 * Execute the IRQ handler and any pending SoftIRQ requests.
	 */
	BUG_ON(!irqs_disabled());
	STOPWATCH_START();
	__do_IRQ(irq);
	STOPWATCH_END(&irq_watches[irq]);
	BUG_ON(!irqs_disabled());

	/*
	 * TODO: Since IRQ's are disabled when calling irq_exit()
	 * modify Kconfig to set __ARCH_IRQ_EXIT_IRQS_DISABLED flag.
	 * This will slightly improve performance by enabling
	 * softirq handling to avoid disabling/disabled interrupts.
	 */
	STOPWATCH_START();
	irq_exit();
	STOPWATCH_END(&irq_watches[INTERRUPT_COUNT]);
	BUG_ON(!irqs_disabled());

	/*
	 * Outside of an interrupt (or nested exit).
	 */
	set_irq_regs(oldregs);
	trace_hardirqs_on();
	ti->interrupt_nesting--;
}

/*
 * irq_soft_alloc()
 *	Allocate a soft IRQ.
 */
int irq_soft_alloc(unsigned int *soft)
{
	if (irq_soft_avail == 0) {
		printk(KERN_NOTICE "no soft irqs to allocate\n");
		return -EFAULT;
	}

	*soft = (ffs(irq_soft_avail) - 1);
	irq_soft_avail &= ~(1 << (*soft));
	return 0;
}

/*
 * ack_bad_irq()
 *	Called to handle an bad irq request.
 */
void ack_bad_irq(unsigned int irq)
{
	printk(KERN_ERR "IRQ: unexpected irq=%d\n", irq);
	__irq_end_vector(irq);
}

/*
 * show_interrupts()
 *	Return a string that displays the state of each of the interrupts.
 */
int show_interrupts(struct seq_file *p, void *v)
{
	struct irqaction *ap;
	int irq = *((loff_t *) v);
	int j;

	if (irq >= NR_IRQS) {
		return 0;
	}

	if (irq == 0) {
		seq_puts(p, "           ");
		for_each_online_cpu(j) {
			seq_printf(p, "CPU%d       ", j);
		}
		seq_putc(p, '\n');
	}

	ap = irq_desc[irq].action;
	if (ap) {
		seq_printf(p, "%3d: ", irq);
		for_each_online_cpu(j) {
			seq_printf(p, "%10u ", kstat_irqs_cpu(irq, j));
		}
		seq_printf(p, "%14s  ", irq_desc[irq].chip->name);
		seq_printf(p, "%s", ap->name);
		for (ap = ap->next; ap; ap = ap->next) {
			seq_printf(p, ", %s", ap->name);
		}
		seq_putc(p, '\n');
	}
	return 0;
}

#if defined(CONFIG_UBICOM32_STOPWATCH_IRQ)
/*
 * irq_watch_show()
 *	Print out the min, avg, max values for each IRQ
 */
static int irq_watch_show(struct seq_file *p, void *v)
{
	struct irqaction *ap;
	int irq = *((loff_t *) v);

	if (irq == 0) {
		seq_puts(p, "\tmin\tavg\tmax\t(micro-seconds)\n");
	}

	if (irq == INTERRUPT_COUNT) {
		stopwatch_show(&irq_watches[irq], p, "softirq",STOPWATCH_SHOW_MICRO);
		return 0;
	}

	ap = irq_desc[irq].action;
	if ((ap) && (ap != &ubicom32_reserve_action[irq])) {
		stopwatch_show(&irq_watches[irq], p, ap->name, STOPWATCH_SHOW_MICRO);
	}
	return 0;
}

static int __init irq_watch_init(void)
{
	return stopwatch_register("irq", INTERRUPT_COUNT + 1, irq_watch_show);
}
module_init(irq_watch_init);
#endif

/*
 * init_soft_interrupts()
 *	Init the soft interrupts mask.
 */
void __init init_soft_interrupts(void)
{
	/*
	 * Pull out the list of software interrupts that are avialable to
	 * Linux and provide an allocation function for them.  The first
	 * 24 interrupts of INT0 are software interrupts.
	 */
	irq_soft_avail = 0;
	if (processor_interrupts(&irq_soft_avail, NULL) < 0) {
		printk(KERN_WARNING "No Soft IRQ(s) available\n");
	}
	irq_soft_avail &= ((1 << 24) - 1);
}

/*
 * init_IRQ(void)
 *	Initialize the on-chip IRQ subsystem.
 */
void __init init_IRQ(void)
{
	int irq;
	struct devtree_node *p = NULL;
	struct devtree_node *iter = NULL;
	unsigned int mask = 0;
	unsigned int reserved = 0;

	/*
	 * Initialize all of the on-chip interrupt handling
	 * to use a common set of interrupt functions.
	 */
	for (irq = 0; irq < INTERRUPT_COUNT; irq++) {
		irq_desc[irq].status = IRQ_DISABLED;
		irq_desc[irq].action = NULL;
		irq_desc[irq].depth = 1;
		set_irq_chip(irq, &ubicom32_irq_chip);
	}

	/*
	 * The sendirq of a devnode is not registered within Linux but instead
	 * is used by the software I/O thread.  These interrupts are reserved.
	 * The recvirq is used by Linux and registered by a device driver, these
	 * are not reserved.
	 *
	 * recvirq(s) that are in the software interrupt range are not supposed
	 * to be marked as reserved.  We track this while we scan the device
	 * nodes.
	 */
	p = devtree_find_next(&iter);
	while (p) {
		unsigned char sendirq, recvirq;
		devtree_irq(p, &sendirq, &recvirq);

		/*
		 * If the sendirq is valid, mark that irq as taken by the
		 * devtree node.
		 */
		if (sendirq < INTERRUPT_COUNT) {
			ubicom32_reserve_action[sendirq].handler =
				ubicom32_reserve_handler;
			ubicom32_reserve_action[sendirq].name = p->name;
			irq_desc[sendirq].action =
				&ubicom32_reserve_action[sendirq];
			mask |= (1 << sendirq);
		}

		/*
		 * Track the relevant recieve IRQ(s)
		 */
		if (recvirq < 24) {
			mask |= (1 << recvirq);
		}

		/*
		 * Move to the next node.
		 */
		p = devtree_find_next(&iter);
	}

	/*
	 * Remove these bits from the irq_soft_avail list and then use the
	 * result as the list of pre-reserved IRQ(s).
	 */
	reserved = ~irq_soft_avail & ~mask;
	for (irq = 0; irq < 24; irq++) {
		if ((reserved & (1 << irq))) {
			ubicom32_reserve_action[irq].handler =
				ubicom32_reserve_handler;
			ubicom32_reserve_action[irq].name = "reserved";
			irq_desc[irq].action = &ubicom32_reserve_action[irq];
		}
	}

	/*
	 * Initialize the PIC which is the Ubicom32 programmable
	 * interrupt controller.
	 */
	pic_init();
}
