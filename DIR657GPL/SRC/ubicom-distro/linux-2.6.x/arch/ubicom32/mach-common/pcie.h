/*
 * arch/ubicom32/mach-common/pcie.h
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

#ifndef __UBICOM32_PCIE_H__
#define __UBICOM32_PCIE_H__

#include <linux/kernel.h>
#include <asm/ubicom32.h>
#include <asm/delay.h>

//#define PCIE_DEBUG
#define PCIE_AHB_BIGENDIAN

/*
 * FIXME: For now, we need to swap endianess when we set the core to work
 * with big endian, kind of weird.
 */
#ifdef PCIE_AHB_BIGENDIAN
#define PCIE_SWAP_ENDIAN
#endif

/* Debug level macros */
#ifdef PCIE_DEBUG

#define PCIE_TRACE(fmt, ...)			//printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define PCIE_INFO(fmt, ...)			printk(KERN_INFO fmt, ##__VA_ARGS__)
#define PCIE_WARNING(fmt, ...)			printk(KERN_WARNING fmt, ##__VA_ARGS__)

#define PCIE_ASSERT(x, fmt, ...) \
{ \
	if (!(x)) { \
		printk(KERN_ERR "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
		BUG(); \
	} \
}

#else // PCIE_DEBUG

#define PCIE_TRACE(fmt, ...)
#define PCIE_INFO(fmt, ...)
#define PCIE_WARNING(fmt, ...)
#define PCIE_ASSERT(x, fmt, ...)		BUG_ON(!(x))

#endif // PCIE_DEBUG

/* Up to two PCI instances */
#define PCIE_PORT_MAX 2
#define PCIE_PORT_MSI_MAX 2

#define PCIE_PHY0_BASE				RL
#define PCIE_PHY1_BASE				RM
#define PCIE_PHY_FUNC_BASE(inst)		((inst) ? PCIE_PHY1_BASE : PCIE_PHY0_BASE)
#define PCIE_PHY_BR_BASE(inst)			(PCIE_PHY_FUNC_BASE(inst) + IO_PORT_BR_OFFSET)

#define PCIE_CTL0_BASE				RJ
#define PCIE_CTL1_BASE			  	RK
#define PCIE_CTL_FUNC_BASE(inst)		((inst) ? PCIE_CTL1_BASE : PCIE_CTL0_BASE)
#define PCIE_CTL_BR_BASE(inst)			(PCIE_CTL_FUNC_BASE(inst) + IO_PORT_BR_OFFSET)

#define PCIE_CORE_TO_CTL(core)			(struct ubicom32_io_port *)((u32)core - IO_PORT_BR_OFFSET)

/* bit of wcmdq_empty in status 0*/
#define WCMDQ_EMPTY_BIT				(1 << 0)

/* bit of wstall in ctl3 */
#define WSTALL_BIT				(1 << 17)

/*
 * Bitmap definition for imask and istatus
 */
#define PCIE_INTR_WR_DMA_END 			(1 << 0)
#define PCIE_INTR_WR_PCI_ERR 			(1 << 1)
#define PCIE_INTR_WR_AHB_ERR 			(1 << 2)
#define PCIE_INTR_WR_DMA_MSK 			(PCIE_INTR_WR_DMA_END | PCIE_INTR_WR_PCI_ERR | PCIE_INTR_WR_AHB_ERR)

#define PCIE_INTR_RD_DMA_END 			(1 << 4)
#define PCIE_INTR_RD_PCI_ERR 			(1 << 5)
#define PCIE_INTR_RD_AHB_ERR 			(1 << 6)
#define PCIE_INTR_RD_DMA_MSK 			(PCIE_INTR_RD_DMA_END | PCIE_INTR_RD_PCI_ERR | PCIE_INTR_RD_AHB_ERR)

#define PCIE_INTR_WIN_FETCH			(1 << 13)
#define PCIE_INTR_WIN_DISCARD 			(1 << 14)
#define PCIE_INTR_WIN_ERR_MSK			(PCIE_INTR_WIN_FETCH | PCIE_INTR_WIN_DISCARD)

#define PCIE_INTR_INTA				(1 << 16)
#define PCIE_INTR_INTB				(1 << 17)
#define PCIE_INTR_INTC				(1 << 18)
#define PCIE_INTR_INTD				(1 << 19)
#define PCIE_INTR_INTX				(0xf << 16)

#define PCIE_INTR_SYSERR			(1 << 20)
#define PCIE_INTR_PMHOTPLUG			(1 << 21)
#define PCIE_INTR_AER				(1 << 22)
#define PCIE_INTR_MSI				(1 << 23)
#define PCIE_INTR_LEGACY_PWR			(1 << 29)
#define PCIE_INTR_CTL_MSK			(PCIE_INTR_INTX | PCIE_INTR_SYSERR | PCIE_INTR_PMHOTPLUG | PCIE_INTR_AER | PCIE_INTR_MSI | PCIE_INTR_LEGACY_PWR)

/*
 * Macros related to DMA control (P25 of the PLDA ref manual)
 * (size, byte_enable, command, stop, start)
 */
#define PCIE_DMA_RD_IO				0
#define PCIE_DMA_WR_IO				1
#define PCIE_DMA_RD_MEM_DW			2
#define PCIE_DMA_WR_MEM_DW			3
#define PCIE_DMA_RD_CFG_REG			4
#define PCIE_DMA_WR_CFG_REG			5
#define PCIE_DMA_RD_MEM_BRST			6
#define PCIE_DMA_WR_MEM_BRST			7

#define PCIE_DMA_CTL(sz, be, cmd, stp, strt)	(((sz) << 12) | ((be) << 8) | ((cmd) << 4) | ((stp) << 1) | (strt))
#define PCIE_DMA_CTL_CFG_RD()			PCIE_DMA_CTL(4, 0xf, PCIE_DMA_RD_CFG_REG, 0, 1)
#define PCIE_DMA_CTL_CFG_WR()			PCIE_DMA_CTL(4, 0xf, PCIE_DMA_WR_CFG_REG, 0, 1)
#define PCIE_DMA_CTL_DW_RD(be)			PCIE_DMA_CTL(4, be, PCIE_DMA_RD_MEM_DW, 0, 1)
#define PCIE_DMA_CTL_DW_WR(be)			PCIE_DMA_CTL(4, be, PCIE_DMA_WR_MEM_DW, 0, 1)

/*
 * Some PCI config space register offset
 */
#define PCIE_CFG_OFFSET_BAR0			0x10
#define PCIE_CFG_OFFSET_BAR1			0x14

#define PCIE_STATUS2_RESET_COMPLETE		(1 << 5)

#define PCIE_BRIDGE_CTL_AER			(1 << 2)
#define PCIE_BRIDGE_CTL_ENDPOINT		(1 << 3)
#define PCIE_BRIDGE_ROOTPORT			(2 << 3)
#define PCIE_BRIDGE_BIG_ENDIAN			(1 << 7)

#ifdef PCIE_AHB_BIGENDIAN
#define PCIE_BRIDGE_ENDIAN_RP_AER		(PCIE_BRIDGE_BIG_ENDIAN | PCIE_BRIDGE_ROOTPORT | PCIE_BRIDGE_CTL_AER)
#else
#define PCIE_BRIDGE_ENDIAN_RP_AER		(PCIE_BRIDGE_ROOTPORT | PCIE_BRIDGE_CTL_AER)
#endif

/* Just copy whatever is in the verification code, improve later */
#ifdef CONFIG_PCIE_GEN2
#define PCIE_GEN2_CONF_DATA			0x10082020
//#define PCIE_ASPM_CONF_DATA			0x32492020
#define PCIE_ASPM_CONF_DATA			0x32493f3f
#define PCIE_AHBPCIE_TIMER_DATA			0x00000100
#define PCIE_GEN2_LINK_SPEED_5G			(1 << 28)
#define PCIE_GEN2_FTS_SEP_CLOCK			5
#else
#define PCIE_GEN2_CONF_DATA			0x00000000
//#define PCIE_GEN2_CONF_DATA			0x10003f3f
#define PCIE_ASPM_CONF_DATA			0x32490501
#define PCIE_AHBPCIE_TIMER_DATA			0x00000200
#endif

/* PCIe capability register offset */
#define PCIE_CFG_LINK_CTL_REG			0x90
#define PCIE_CFG_LINK_CTL_DATA			0x00000020

#define PCIE_PCIAHB_CTL(ahb_base, enable, no_prefetch, size) \
	((ahb_base & ~((1 << 12) - 1)) | (enable << 7) | (no_prefetch << 6) | size)
#define PCIE_AHBPCI_CTL_LOW(pci_base, no_prefetch, size) \
	((pci_base & ~((1 << 7) - 1)) | (no_prefetch << 6) | size)

#define PCIE_BR_ACCESS_NONE			0
#define PCIE_BR_ACCESS_CORE_REG			1
#define PCIE_BR_ACCESS_AHBPCI_WIN0		2
#define PCIE_BR_ACCESS_AHBPCI_WIN1		3

/*
 * Configuration space read / write may fail during enumeration phase. We are not sure
 * if the controller will set PCI or AHB error in this case now, so set a limit for now.
 */
#define PCIE_DMA_MAX_LOOP 4000

#define PCI_VENDOR_ID_UBICOM  0x6563
#define PCIE_RC_UBICOM_DEV_ID 0x01

/*
 * pcie_nbr_readl
 *	Read a 32 bit value from the non-blocking-region
 */
static inline u32 pcie_nbr_readl(volatile u32 * addr)
{
	volatile u32 val = *addr;
	//PCIE_TRACE("read from NBR reg %p, val=%x\n", addr, val);
	return val;
}

/*
 * pcie_nbr_writel
 *	Write a 32 bit value into non-blocking-region
 */
static inline void pcie_nbr_writel(volatile u32 * addr, u32 val)
{
	//PCIE_TRACE("write %x to NBR reg %p\n", val, addr);
	*addr = val;
}

/*
 * The first tape-out has a bug in blocking region access when doing blocking
 * region access directly through DRAM. The workaround is to use a register or
 * OCM and wait for 5 cycles. The limitation will be gone in the production
 * release.
 */

/*
 * BR read needs dst to be a non-DDR address
 *
 * NOTE: this function should not be called directly without selecting the BR region
 * using ctl->ctl1
 */
static inline u32 pcie_br_readl(struct pcie_port *pp, volatile u32 * addr)
{
	u32 ret;

#ifdef CONFIG_SMP
	PCIE_ASSERT(((in_irq() || irqs_disabled()) && spin_is_locked(&pp->conf_lock)),
		"PCIe BR access unlocked\n");
#endif

	asm volatile (
		" cycles	8			\n"
#ifdef PCIE_SWAP_ENDIAN
		" swapb.4	%[ret], 0(%[addr])	\n"
#else
		" move.4	%[ret], 0(%[addr])	\n"
#endif
		" cycles	8			\n"
		: [ret] "=d" (ret)
		: [addr] "a" (addr)
	);
	PCIE_TRACE("read from BR reg %p, val=%x\n", addr, ret);
	return ret;
}

/*
 * BR write needs dst to be a non-DDR address
 *
 * NOTE: this function should not be called directly without selecting the BR region
 * using ctl->ctl1
 */
static inline void pcie_br_writel(struct pcie_port *pp, volatile u32 * addr, u32 val)
{
#ifdef CONFIG_SMP
	PCIE_ASSERT(((in_irq() || irqs_disabled()) && spin_is_locked(&pp->conf_lock)),
		"PCIe BR access unlocked\n");
#endif

	PCIE_TRACE("write %x to BR reg %p\n", val, addr);
	asm volatile (
		" cycles	8			\n"
#ifdef PCIE_SWAP_ENDIAN
		" swapb.4	0(%[addr]), %[val]	\n"
#else
		" move.4	0(%[addr]), %[val]	\n"
#endif
		" cycles	8			\n"
		:
		: [addr] "a" (addr), [val] "d" (val)
	);
	//PCIE_TRACE("read BR reg %p back, value=%x\n", addr, pcie_br_readl(pp, addr));
}

/*
 * Make sure config space is valid
 */
static inline int pcie_valid_config(struct pcie_port *pp, int bus, int dev)
{
	/*
	 * Don't go out when trying to access nonexisting devices, PCIE is point to point
	 * and can only have one device on a bus
	 */
	if (dev > 0)
		return 0;

	return 1;
}

/*
 * Write value to a rootport control register through BR
 */
static inline void pcie_core_reg_writel(struct pcie_port *pp, volatile u32 * addr, u32 val)
{
	/* Change access path to select BR access to core registers */
	struct ubicom32_io_port *ctl = pp->ctl;
	ctl->ctl1 = PCIE_BR_ACCESS_CORE_REG;

	pcie_br_writel(pp, addr, val);
}

/*
 * Read a rootport control register through BR
 */
static inline u32 pcie_core_reg_readl(struct pcie_port *pp, volatile u32 * addr)
{
	/* Change access path to select BR access to core registers */
	struct ubicom32_io_port *ctl = pp->ctl;
	ctl->ctl1 = PCIE_BR_ACCESS_CORE_REG;

	return pcie_br_readl(pp, addr);
}

extern struct pcie_port pcie_ports[PCIE_PORT_MAX];
extern void __init pcie_phy_config(struct ubicom32_io_port *phy);

/*
 * A board specific function must be provided by every PCIe capable board
 */
extern void __init ubi32_pcie_reset_endpoint(int port);

extern void pcie_dump_status_regs(struct pcie_port *pp);
extern u32 __pcie_cfg_writel(struct pcie_port *pp, u32 cfg_addr, u32 val);
extern u32 __pcie_cfg_readl(struct pcie_port *pp, u32 cfg_addr, u32 *val);

#define PCIE_HERE() printk(KERN_INFO "PCIE here %s: %d, %s\n", __FILE__, __LINE__, __FUNCTION__)

#endif

