/*
 * arch/ubicom32/mach-common/pcie_io.c
 *      PCI Express interface management.
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
 *
 * Ubicom32 implementation derived from (with many thanks):
 *   arch/arm
 *   arch/mips
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/io.h>
#include <asm/ubicom32.h>
#include <asm/cachectl.h>

#include "pcie.h"

//#define USE_PCIE_AHBPCI_WIN0
//#define USE_PCIE_BE 1

#define ADDR_ALIGN(addr) ((void *)((u32)addr & ~0x03))
#define ADDR_SHIFT(addr) (((u32)addr & 0x03) << 3)

/*
 * The controller has the capability of read/write byte, word.
 * For bringup, let's just read DW through PCI. Improve later.
 */

/*
 * Helper function to find the corresponding pcie port of a pci memory mapped I/O address
 */
static inline struct pcie_port *addr_to_port(const volatile void __iomem *addr)
{
	PCIE_ASSERT(((pcie_ports[0].mem_mask & (u32)addr) == pcie_ports[0].mem_base) ||
		    ((pcie_ports[1].mem_mask & (u32)addr) == pcie_ports[1].mem_base),
		    "bad PCIe address\n");
	return ((pcie_ports[0].mem_mask & (u32)addr) == pcie_ports[0].mem_base) ?
		&pcie_ports[0] : &pcie_ports[1];
}

/*
 * Helper function to find the corresponding pcie port of a pci I/O port address
 */
static inline struct pcie_port *ioport_to_port(unsigned long addr)
{
#if 0
	PCIE_ASSERT(((pcie_ports[0].io_mask & addr) == pcie_ports[0].io_base) ||
		    ((pcie_ports[1].io_mask & addr) == pcie_ports[1].io_base),
		    "bad PCIe IO address\n");
	return ((pcie_ports[0].io_mask & addr) == pcie_ports[0].io_base) ?
		&pcie_ports[0] : &pcie_ports[1];
#endif
	return addr_to_port((const volatile void __iomem *)addr);
}

/*
 * DMA / interrupt synchronization helper
 * Assumes spinlock of this port is held already by the caller.
 */
static inline void __pcie_ctl_dma_ready(struct ubicom32_io_port *ctl)
{
	/* polls dma_wcmdq_empty bit of STATUS0 */
	if (likely(ctl->status0 & WCMDQ_EMPTY_BIT)) {
		return;
	}

	PCIE_TRACE("dma is not ready, set stall bit and wait\n");
	/* tell the device stop feeding till the current written data is ready */
	ctl->ctl3 |= WSTALL_BIT;

	/* keep polling till it is ready */
	while (!(ctl->status0 & WCMDQ_EMPTY_BIT));

	/* clear the stall bit */
	ctl->ctl3 &= ~WSTALL_BIT;
}

/*
 * Ask the internal DMA engine of the controller to make a 32 bit read (configure, mem, or IO)
 *
 *	core: 		the starting address of the controller registers
 *	addr_lo:	low 32 bit of a possible 64 address
 *	val:		the place to keep the read value
 *	dma_type:	configure space, memory, IO, value defined by the datasheet
 *	be:		byte enable		// FIXME: not working due to the endian issue in the first tape-out
 *	swap:		do endian swap		// FIXME: may not be unnecessary to have this flexibility
 */
static u32 __pcie_dma_readl(struct pcie_port *pp, u32 addr_lo, u32 *val, u32 dma_type, u32 be)
{
	struct pcie_core_regs *core = pp->core;
	u32 istatus;
	int cnt = 0;

	asm volatile (
	"	flush		(%0)		\n\t"
		:
		: "a" (&pp->buf)
	);

	/*
	 * Clear possible stale istatus bits. There might take effect immediately,
	 * but should be faster than the following writes.
	 */
	pcie_core_reg_writel(pp, &core->istatus, PCIE_INTR_RD_DMA_MSK);

	/* Set up DMA descriptors */
	pcie_core_reg_writel(pp, &core->rdma_pci_addr_0, addr_lo);
	pcie_core_reg_writel(pp, &core->rdma_pci_addr_1, 0);
	pcie_core_reg_writel(pp, &core->rdma_ahb_addr, (u32)(&pp->buf));

	/* Kick off the DMA action */
	pcie_core_reg_writel(pp, &core->rdma_cntl, PCIE_DMA_CTL(4, be, dma_type, 0, 1));

	/* Poll the the interrupt status register till it is complete */
	do {
		istatus = pcie_core_reg_readl(pp, &core->istatus);
		if (cnt++ == PCIE_DMA_MAX_LOOP) {
			PCIE_WARNING("read not complete\n");
			pcie_dump_status_regs(pp);
			/* Stop the action */
			pcie_core_reg_writel(pp, &core->rdma_cntl, PCIE_DMA_CTL(4, be, dma_type, 1, 0));
			break;
		}
		PCIE_TRACE("istatus=0x%x, MSK=0x%x\n", istatus, PCIE_INTR_RD_DMA_MSK);
	} while (!(istatus & PCIE_INTR_RD_DMA_MSK));

	/*
	 * Any DMA action involving the DMA engine writing something to DDR needs such
	 * a synchronization operation.
	 */
	istatus &= (PCIE_INTR_RD_PCI_ERR | PCIE_INTR_RD_AHB_ERR);
	if (istatus) {
		PCIE_ASSERT(0, "read DMA error: istatus = %x\n", istatus);
		*val = 0;
		return istatus;
	}

	/*
	 * make sure that DMA is completed
	 */
	__pcie_ctl_dma_ready(PCIE_CORE_TO_CTL(core));

	*val = pp->buf;
#ifdef PCIE_SWAP_ENDIAN
	*val = le32_to_cpu(*val);
#endif

	PCIE_TRACE("read DMA completed: REG[%x] => %x\n", addr_lo, *val);

	return istatus;
}

/*
 * Ask the internal DMA engine of the controller to make a 32 bit write (configure, mem, or IO)
 *
 *	core: 		the starting address of the controller registers
 *	addr_lo:	low 32 bit of a possible 64 address
 *	val:		the value to write
 *	dma_type:	configure space, memory, IO, value defined by the datasheet
 *	be:		byte enable		// FIXME: not working due to the endian issue in the first tape-out
 *	swap:		do endian swap		// FIXME: may not be unnecessary to have this flexibility
 */
static u32 __pcie_dma_writel(struct pcie_port *pp, u32 addr_lo, u32 val, u32 dma_type, u32 be)
{
	struct pcie_core_regs *core = pp->core;
	u32 istatus;
	int cnt = 0;

	PCIE_TRACE("write val = %x to addr_lo 0x%x\n", val, addr_lo);

	/* Make sure the right value to be DMAed is in memory */
#ifdef PCIE_SWAP_ENDIAN
	val = cpu_to_le32(val);
#endif

	asm volatile (
	"	move.4		(%0), %1	\n\t"
	"	flush		(%0)		\n\t"
	"	pipe_flush	0		\n\t"
	"	sync		(%0)		\n\t"
	"	flush		(%0)		\n\t"
		:
		: "a" (&pp->buf), "r" (val)
	);

	/*
	 * Clear possible stale istatus bits. There might take effect immediately,
	 * but should be faster than the following writes.
	 */
	pcie_core_reg_writel(pp, &core->istatus, PCIE_INTR_WR_DMA_MSK);

	/* Set up DMA descriptors */
	pcie_core_reg_writel(pp, &core->wdma_pci_addr_0, addr_lo);
	pcie_core_reg_writel(pp, &core->wdma_pci_addr_1, 0);
	pcie_core_reg_writel(pp, &core->wdma_ahb_addr, (u32)(&pp->buf));

	/* Kick off the DMA action */
	pcie_core_reg_writel(pp, &core->wdma_cntl, PCIE_DMA_CTL(4, be, dma_type, 0, 1));

	/* Poll the the interrupt status register till it is complete */
	do {
		istatus = pcie_core_reg_readl(pp, &core->istatus);
		if (cnt++ == PCIE_DMA_MAX_LOOP) {
			PCIE_WARNING("write not complete\n");
			pcie_dump_status_regs(pp);
			/* Stop the action */
			pcie_core_reg_writel(pp, &core->wdma_cntl, PCIE_DMA_CTL(4, be, dma_type, 1, 0));
			break;
		}
	} while (!(istatus & PCIE_INTR_WR_DMA_MSK));

	istatus &= (PCIE_INTR_WR_PCI_ERR | PCIE_INTR_WR_AHB_ERR);
	PCIE_ASSERT(!istatus, "write DMA error: istatus=%x\n", istatus);
	PCIE_TRACE("write DMA completed: REG[%x] <= %x\n", addr_lo, val);

	return istatus;
}

#ifdef USE_PCIE_AHBPCI_WIN0
/*
 * Internal function to read a 32 bit value from a memory mapped space
 */
static u32 __pcie_win0_readl(struct pcie_port *pp, const volatile void __iomem *pci_addr, u32 *val)
{
	struct ubicom32_io_port *ctl = pp->ctl;
	u32 istatus;

	/* Where to read the BR register */
	u32 *br_addr = (u32 *)((u32)pp->core + ((u32)pci_addr & 4095));

	/* Clear possible stale istatus bits. */
	pcie_core_reg_writel(pp, &pp->core->istatus, PCIE_INTR_WIN_ERR_MSK);

	/* Set high bits then read the value */
	pcie_nbr_writel(&ctl->ctl0, (u32)pci_addr);

	/* Select window0 */
	pcie_nbr_writel(&ctl->ctl1, PCIE_BR_ACCESS_AHBPCI_WIN0);

	*val = pcie_br_readl(pp, br_addr);

	istatus = pcie_core_reg_readl(pp, &pp->core->istatus)
		& PCIE_INTR_WIN_ERR_MSK;
	if (istatus) {
		PCIE_ASSERT(0, "read WIN0 error: istatus = %x\n", istatus);
		*val = 0;
	}

	pp->last_wr_addr = 0;
	PCIE_TRACE("read WIN0 completed: REG[%x] => %x\n", pci_addr, *val);

	return istatus;
}

/*
 * Internal function to write a 32 bit value into a memory mapped space
 */
static u32 __pcie_win0_writel(struct pcie_port *pp, u32 val, const volatile void __iomem *pci_addr)
{
	struct ubicom32_io_port *ctl = pp->ctl;
	u32 istatus;

	/* Where to write the BR register */
	u32 *br_addr = (u32 *)((u32)pp->core + ((u32)pci_addr & 4095));

	/* flush last write by performing a read */
	/*if (pp->last_wr_addr) {
		u32 vtmp;
		__pcie_win0_readl(pp, (void *)(((u32)pci_addr & ~0xffff) + 8), &vtmp);
	}*/

	/* Clear possible stale istatus bits. */
	pcie_core_reg_writel(pp, &pp->core->istatus, PCIE_INTR_WIN_ERR_MSK);

	/* Set high bits then read the value */
	pcie_nbr_writel(&ctl->ctl0, (u32)pci_addr);

	/* Select window0 */
	pcie_nbr_writel(&ctl->ctl1, PCIE_BR_ACCESS_AHBPCI_WIN0);

	pcie_br_writel(pp, br_addr, val);

	istatus = pcie_core_reg_readl(pp, &pp->core->istatus)
		& PCIE_INTR_WIN_FETCH;
	PCIE_ASSERT(!istatus, "write WIN0 error: istatus = %x\n", istatus);

	pp->last_wr_addr = (u32)pci_addr;
	PCIE_TRACE("write WIN0 completed: REG[%x] <= %x\n", pci_addr, val);

	return istatus;
}
#endif

/*
 * Public function to read a 32 bit value from a PCI memory address
 */
unsigned int ubi32_pci_read_u32(const volatile void __iomem *addr)
{
	unsigned int val;
	unsigned long flags;
	struct pcie_port *pp = addr_to_port(addr);

	PCIE_TRACE(" -- read u32 from pci_addr %p\n", addr);
	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_AHBPCI_WIN0
	__pcie_win0_readl(pp, addr, &val);
#else
	__pcie_dma_readl(pp, (u32)addr, &val, PCIE_DMA_RD_MEM_DW, 0xf);
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);
	PCIE_TRACE("ioread32 for addr %p, val=%x\n", addr, val);
	return val;
}

/*
 * Public function to read a 16 bit value from a PCI memory address
 */
unsigned short ubi32_pci_read_u16(const volatile void __iomem *addr)
{
	unsigned int val, shift;
	unsigned long flags;
	struct pcie_port *pp = addr_to_port(addr);

	/* Bit shift count, then find the 32bit aligned addr */
	shift = ADDR_SHIFT(addr);

	PCIE_TRACE(" -- read u16 from pci_addr %p\n", addr);
	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_AHBPCI_WIN0
	__pcie_win0_readl(pp, ADDR_ALIGN(addr), &val);
#else
	__pcie_dma_readl(pp, (u32)ADDR_ALIGN(addr), &val, PCIE_DMA_RD_MEM_DW, 0xf);
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	val = (val >> shift) & 0xffff;
	PCIE_TRACE("ioread16 for addr %p, val=%x\n", addr, val);
	return val;
}

/*
 * Public function to read a 8 bit value from a PCI memory address
 */
unsigned char ubi32_pci_read_u8	(const volatile void __iomem *addr)
{
	unsigned int val, shift;
	unsigned long flags;
	struct pcie_port *pp = addr_to_port(addr);

	/* Bit shift count, then find the 32bit aligned addr */
	shift = ADDR_SHIFT(addr);

	PCIE_TRACE(" -- read u8 from pci_addr %p\n", addr);
	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_AHBPCI_WIN0
	__pcie_win0_readl(pp, ADDR_ALIGN(addr), &val);
#else
	__pcie_dma_readl(pp, (u32)ADDR_ALIGN(addr), &val, PCIE_DMA_RD_MEM_DW, 0xf);
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	val = (val >> shift) & 0xff;
	PCIE_TRACE("ioread8 for addr %p, val=%x\n", addr, val);
	return val;
}

/*
 * Public function to write a 32 bit value to a PCI memory address
 */
void ubi32_pci_write_u32(unsigned int value, const volatile void __iomem *addr)
{
	unsigned long flags;
	struct pcie_port *pp = addr_to_port(addr);

	PCIE_TRACE("iowrite32 for addr %p, val=%x\n", addr, value);
	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_AHBPCI_WIN0
	__pcie_win0_writel(pp, value, addr);
#else
	__pcie_dma_writel(pp, (u32)addr, value, PCIE_DMA_WR_MEM_DW, 0xf);
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);
}

/*
 * Public function to write a 16 bit value to a PCI memory address
 */
void ubi32_pci_write_u16(unsigned short value, const volatile void __iomem *addr)
{
	unsigned int shift;
	unsigned long flags;
	struct pcie_port *pp = addr_to_port(addr);
	unsigned int val;

	PCIE_TRACE("iowrite16 for addr %p, val=%x\n", addr, value);
	shift = ADDR_SHIFT(addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_AHBPCI_WIN0
	__pcie_win0_readl(pp, ADDR_ALIGN(addr), &val);
	val &= ~(0xffff << shift);
	val |= value << shift;
	__pcie_win0_writel(pp, val, ADDR_ALIGN(addr));
#else
#ifdef USE_PCIE_BE
	val = value << shift;
	__pcie_dma_writel(pp, (u32)ADDR_ALIGN(addr), val, PCIE_DMA_WR_MEM_DW,
			(((u32)addr & 0x02) ? (0x3 << 2) : 0x3));
#else
	__pcie_dma_readl(pp, (u32)ADDR_ALIGN(addr), &val, PCIE_DMA_RD_MEM_DW, 0xf);
	val &= ~(0xffff << shift);
	val |= value << shift;
	__pcie_dma_writel(pp, (u32)ADDR_ALIGN(addr), val, PCIE_DMA_WR_MEM_DW, 0xf);
#endif
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);
}

/*
 * Public function to write a 8 bit value to a PCI memory address
 */
void ubi32_pci_write_u8(unsigned char value, const volatile void __iomem *addr)
{
	unsigned int shift;
	unsigned long flags;
	struct pcie_port *pp = addr_to_port(addr);
	unsigned int val;

	PCIE_TRACE("iowrite8 for addr %p, val=%x\n", addr, value);
	shift = ADDR_SHIFT(addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_AHBPCI_WIN0
	__pcie_win0_readl(pp, ADDR_ALIGN(addr), &val);
	val &= ~(0xff << shift);
	val |= value << shift;
	__pcie_win0_writel(pp, val, ADDR_ALIGN(addr));
#else
#ifdef USE_PCIE_BE
	val = value << shift;
	__pcie_dma_writel(pp, (u32)ADDR_ALIGN(addr), val, PCIE_DMA_WR_MEM_DW,
			(1 << ((u32)addr & 0x03)));
#else
	__pcie_dma_readl(pp, (u32)ADDR_ALIGN(addr), &val, PCIE_DMA_RD_MEM_DW, 0xf);
	val &= ~(0xff << shift);
	val |= value << shift;
	__pcie_dma_writel(pp, (u32)ADDR_ALIGN(addr), val, PCIE_DMA_WR_MEM_DW, 0xf);
#endif
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);
}

EXPORT_SYMBOL(ubi32_pci_write_u32);
EXPORT_SYMBOL(ubi32_pci_write_u16);
EXPORT_SYMBOL(ubi32_pci_write_u8);

EXPORT_SYMBOL(ubi32_pci_read_u32);
EXPORT_SYMBOL(ubi32_pci_read_u16);
EXPORT_SYMBOL(ubi32_pci_read_u8);

/*
 * Public function to write a 8 bit value to a PCI memory address
 */
static inline u32 __pcie_port_inl(struct pcie_port *pp, const volatile void __iomem *port_addr, u32 *val)
{
	__pcie_dma_readl(pp, (u32)port_addr, val, PCIE_DMA_RD_IO, 0xf);
	return *val;
}

static inline void __pcie_port_outl(struct pcie_port *pp, u32 val, const volatile void __iomem *port_addr, u32 be)
{
	__pcie_dma_writel(pp, (u32)port_addr, val, PCIE_DMA_WR_IO, be);
}

/*
 * Public function to read a 8 bit value from a PCI I/O address
 */
u8 ubi32_pci_inb(unsigned long addr)
{
	unsigned int shift;
	unsigned long flags;
	struct pcie_port *pp = ioport_to_port(addr);
	unsigned int val;

	shift = ADDR_SHIFT(addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
	__pcie_port_inl(pp, ADDR_ALIGN(addr), &val);
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	val = (val >> shift) & 0xff;
	PCIE_INFO("inb for port %d, val=%x\n", (unsigned int)addr, val);

	return val;
}

/*
 * Public function to write a 8 bit value to a PCI I/O address
 */
void ubi32_pci_outb(u8 value, unsigned long addr)
{
	unsigned int shift;
	unsigned long flags;
	struct pcie_port *pp = ioport_to_port(addr);
	unsigned int val;

	shift = ADDR_SHIFT(addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_BE
	val = value << shift;
	__pcie_port_outl(pp, val, ADDR_ALIGN(addr), (1 << ((u32)addr & 0x03)));
#else
	__pcie_port_inl(pp, ADDR_ALIGN(addr), &val);
	val &= ~(0xff << shift);
	val |= value << shift;
	__pcie_port_outl(pp, val, ADDR_ALIGN(addr), 0xf);
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);
}

/*
 * Public function to read a 16 bit value from a PCI I/O address
 */
u16 ubi32_pci_inw(unsigned long addr)
{
	unsigned int shift;
	unsigned long flags;
	struct pcie_port *pp = ioport_to_port(addr);
	unsigned int val;

	shift = ADDR_SHIFT(addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
	__pcie_port_inl(pp, ADDR_ALIGN(addr), &val);
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	val = (val >> shift) & 0xffff;
	PCIE_INFO("inb for port %d, val=%x\n", (unsigned int)addr, val);

	return val;
}

/*
 * Public function to write a 16 bit value to a PCI I/O address
 */
void ubi32_pci_outw(u16 value, unsigned long addr)
{
	unsigned int shift;
	unsigned long flags;
	struct pcie_port *pp = ioport_to_port(addr);
	unsigned int val;

	shift = ADDR_SHIFT(addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
#ifdef USE_PCIE_BE
	val = value << shift;
	__pcie_port_outl(pp, val, ADDR_ALIGN(addr), (((u32)addr & 0x02) ? (0x3 << 2) : 0x3));
#else
	__pcie_port_inl(pp, ADDR_ALIGN(addr), &val);
	val &= ~(0xffff << shift);
	val |= value << shift;
	__pcie_port_outl(pp, val, ADDR_ALIGN(addr), 0xf);
#endif
	spin_unlock_irqrestore(&pp->conf_lock, flags);
}

/*
 * Public function to read a 32 bit value from a PCI I/O address
 */
u32 ubi32_pci_inl(unsigned long addr)
{
	unsigned long flags;
	struct pcie_port *pp = ioport_to_port(addr);
	u32 val;

	spin_lock_irqsave(&pp->conf_lock, flags);
	__pcie_port_inl(pp, (void __iomem *)addr, &val);
	spin_unlock_irqrestore(&pp->conf_lock, flags);
	return val;
}

/*
 * Public function to write a 32 bit value to a PCI I/O address
 */
void ubi32_pci_outl(u32 value, unsigned long addr)
{
	unsigned long flags;
	struct pcie_port *pp = ioport_to_port(addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
	__pcie_port_outl(pp, value, (void __iomem *)addr, 0xf);
	spin_unlock_irqrestore(&pp->conf_lock, flags);
}

EXPORT_SYMBOL(ubi32_pci_outl);
EXPORT_SYMBOL(ubi32_pci_outw);
EXPORT_SYMBOL(ubi32_pci_outb);

EXPORT_SYMBOL(ubi32_pci_inl);
EXPORT_SYMBOL(ubi32_pci_inw);
EXPORT_SYMBOL(ubi32_pci_inb);

/*
 * Write a 32bit value into a PCI device's config space register
 * Assumes spinlock of this port is held already by the caller.
 */
u32 __pcie_cfg_writel(struct pcie_port *pp, u32 cfg_addr, u32 val)
{
	return __pcie_dma_writel(pp, cfg_addr, val, PCIE_DMA_WR_CFG_REG, 0xf);
}

/*
 * Read a DW from a PCI config space register
 * Assumes spinlock of this port is held already by the caller.
 */
u32 __pcie_cfg_readl(struct pcie_port *pp, u32 cfg_addr, u32 *val)
{
	return __pcie_dma_readl(pp, cfg_addr, val, PCIE_DMA_RD_CFG_REG, 0xf);
}

/*
 * APIs to make sure DMA device is not updating DDR while host is accessing it.
 * User must be careful enough not issuing any PCI bus access (config space and MMIO space)
 * while holding the dma lock, otherwise there will be deadlock.
 */

/*
 * Find the corresponding pcie port given a PCIe memory mapped address
 */
int ubi32_pcie_addr_to_port(void __iomem *addr)
{
	struct pcie_port *pp = addr_to_port(addr);
	return pp->port;
}

/*
 * Lock the DMA engine from making more write transactions to the platform memory
 */
void ubi32_pcie_dma_lock(int port, unsigned long *flags)
{
	struct pcie_port *pp = &pcie_ports[port];
	struct ubicom32_io_port *ctl = pp->ctl;

	spin_lock_irqsave(&pp->conf_lock, *flags);
	ctl->ctl3 |= WSTALL_BIT;
	while (!(ctl->status0 & WCMDQ_EMPTY_BIT));
}

/*
 * Unlock the DMA engine to allow more write transactions to the platform memory
 */
void ubi32_pcie_dma_unlock(int port, unsigned long *flags)
{
	struct pcie_port *pp = &pcie_ports[port];
	struct ubicom32_io_port *ctl = pp->ctl;

	ctl->ctl3 &= ~WSTALL_BIT;
	spin_unlock_irqrestore(&pp->conf_lock, *flags);
}

EXPORT_SYMBOL(ubi32_pcie_addr_to_port);
EXPORT_SYMBOL(ubi32_pcie_dma_lock);
EXPORT_SYMBOL(ubi32_pcie_dma_unlock);

