/*
 * arch/ubicom32/mach-common/pcie.c
 *	PCI Express interface management.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#include <asm/devtree.h>
#include <asm/ubicom32.h>
#include <asm/cachectl.h>

#include "pcie.h"

struct pcie_port pcie_ports[PCIE_PORT_MAX];
static int num_pcie_ports;
static const char *port_names[PCIE_PORT_MAX] = {"pciej", "pciek"};

static size_t size_to_bits(size_t sz)
{
	int i = 0;
	for (; i < 32; i++) {
		if (sz & (1 << i)) {
			break;
		}
	}
	BUG_ON(i == 32);

	/* Size must be power of 2 */
	BUG_ON(sz & ~(1 << i));
	return i;
}

/*
 * pcie_get_config_addr
 *	Combine the bus, dev, fn, and register offset into the address format expected by the controller.
 */
static inline void pcie_get_config_addr(unsigned int bus, unsigned int devfn, int where,
				u32 *cfg_addr)
{
	/*
	 * For configuration transfers address register is used to identify targeted
	 * device and register as shown below:
	 *    Bits [63:32]: not used, must be zero
	 *    Bits [31:24]: targeted bus number (use 80h to access bridge configuration registers)
	 *    Bits [23:19]: targeted device number
	 *    Bits [18:16]: targeted function number
	 *    Bits [15:12]: not used, must be zero
	 *    Bits [11:0]: register or extended register number (bits [1:0] of must be set to “00”)
	 *
	 * We only return the lower DW
	 */
	*cfg_addr = ((bus << 24) | (devfn << 16) | where);
}

/*
 * Dump most of the internel registers of the pcie controller
 *	Can be used even when debug is off
 */
void pcie_dump_status_regs(struct pcie_port *pp)
{
	printk(KERN_INFO "pcie regdump port[%d]:\nphy->status0=%x, ctl->status(%x, %x, %x)\n", pp->port,
			pcie_nbr_readl(&pp->phy->status0),
			pcie_nbr_readl(&pp->ctl->status0),
			pcie_nbr_readl(&pp->ctl->status1),
			pcie_nbr_readl(&pp->ctl->status2));
	printk(KERN_INFO " core(info=%x imask=%x istatus=%x imsistatus=%x imsiaddr=%x wdma(pci:%x ahb:%x ctl:%x))\n",
			pcie_core_reg_readl(pp, &pp->core->info),
			pcie_core_reg_readl(pp, &pp->core->imask),
			pcie_core_reg_readl(pp, &pp->core->istatus),
			pcie_core_reg_readl(pp, &pp->core->imsistatus),
			pcie_core_reg_readl(pp, &pp->core->imsiaddr),
			pcie_core_reg_readl(pp, &pp->core->wdma_pci_addr_0),
			pcie_core_reg_readl(pp, &pp->core->wdma_ahb_addr),
			pcie_core_reg_readl(pp, &pp->core->wdma_cntl));
	printk(KERN_INFO " core(rdma(pci:%x ahb:%x ctl:%x) gen2=%x aspm=%x discard=%x pmstatus=%x)\n",
			pcie_core_reg_readl(pp, &pp->core->rdma_pci_addr_0),
			pcie_core_reg_readl(pp, &pp->core->rdma_ahb_addr),
			pcie_core_reg_readl(pp, &pp->core->rdma_cntl),
			pcie_core_reg_readl(pp, &pp->core->gen2_conf),
			pcie_core_reg_readl(pp, &pp->core->aspmconf),
			pcie_core_reg_readl(pp, &pp->core->ahbpci_timer),
			pcie_core_reg_readl(pp, &pp->core->pmstatus));
	printk(KERN_INFO " core(cap=%x dev=%x sub=%x class=%x csr(slot:%x prm:%x dev:%x link:%x root:%x))\n",
			pcie_core_reg_readl(pp, &pp->core->slot_cap_reg),
			pcie_core_reg_readl(pp, &pp->core->deviceid),
			pcie_core_reg_readl(pp, &pp->core->sub_id),
			pcie_core_reg_readl(pp, &pp->core->class_code),
			pcie_core_reg_readl(pp, &pp->core->slotcsr),
			pcie_core_reg_readl(pp, &pp->core->prm_csr),
			pcie_core_reg_readl(pp, &pp->core->dev_csr),
			pcie_core_reg_readl(pp, &pp->core->link_csr),
			pcie_core_reg_readl(pp, &pp->core->root_csr));

	printk(KERN_INFO " pciahb0_base=%08x pciahb1_base=%08x ahbpci0(%08x %08x), ahbpci1(%08x %08x)\n",
			pcie_core_reg_readl(pp, &pp->core->pciahb0_ahbbase),
			pcie_core_reg_readl(pp, &pp->core->pciahb1_ahbbase),
			pcie_core_reg_readl(pp, &pp->core->ahbpcihi0_ahbbase),
			pcie_core_reg_readl(pp, &pp->core->ahbpcilo0_ahbbase),
			pcie_core_reg_readl(pp, &pp->core->ahbpcihi1_ahbbase),
			pcie_core_reg_readl(pp, &pp->core->ahbpcilo1_ahbbase));
}

/*
 * Write value to a configuration register of the rootport core
 * Assumes spinlock of this port is held already by the caller.
 */
static inline void __pcie_core_cfg_writel(struct pcie_port *pp, int reg_offset, u32 val)
{
	u32 cfg_addr;
	u32 istatus;

	/* The rootport itself is using bus 0, dev 0 and func 0 */
	pcie_get_config_addr(0, 0, reg_offset, &cfg_addr);
	istatus = __pcie_cfg_writel(pp, cfg_addr, val);
	PCIE_INFO("write value %x to reg %x\n", val, reg_offset);
	BUG_ON(istatus);
}

/*
 * Read a configuration register from the rootport core
 * Assumes spinlock of this port is held already by the caller.
 */
static inline u32 __pcie_core_cfg_readl(struct pcie_port *pp, int reg_offset)
{
	u32 cfg_addr;
	u32 val, istatus;

	/* The rootport itself is using bus 0, dev 0 and func 0 */
	pcie_get_config_addr(0, 0, reg_offset, &cfg_addr);
	istatus = __pcie_cfg_readl(pp, cfg_addr, &val);
	BUG_ON(istatus);

	PCIE_INFO("\t reg (%x) val=%x\n", reg_offset, val);

	return val;
}

/*
 * ubi32_pcie_interrupt
 *
 * 	The interrupt handler for the pcie controller.
 *
 * All pcie related interrupts comes from the istatus register. We mask off DMA related
 * interrupts because we want to poll for the result, however, they may still show up in
 * the status register.
 *
 * We can not hold the spinlock while servicing the MSI interrupts which may do IO which
 * requires the spinlock again.
 */
static irqreturn_t ubi32_pcie_interrupt(int irq, void *arg)
{
	struct pcie_port *pp = (struct pcie_port *)arg;
	struct pcie_core_regs *core = pp->core;
	u32 istatus = 0, imsistatus = 0;
	int inta = 0;

	PCIE_ASSERT(pp->ctl_irq == irq, "bad PCIe interrupt\n");

	spin_lock(&pp->conf_lock);

	/* read istatus to find out the top level interrupts */
	istatus = pcie_core_reg_readl(pp, &core->istatus);

	/*
	 * Clear all but the DMA channel bits which are handled differently
	 * We also ignore some bits like doorbel, etc
	 */
	pcie_core_reg_writel(pp, &core->istatus, istatus & PCIE_INTR_CTL_MSK);

	/* Ignore interrupt bits which are not supposed to be handled here */
	istatus &= PCIE_INTR_CTL_MSK;

	/*
	 * It does not hurt much to check dma complete, doing it here avoids polluting
	 * all pci drivers
	 */
	if (likely(istatus & (PCIE_INTR_MSI | PCIE_INTR_INTA))) {
		/*
		 * Any DMA action involving the DMA engine writing something to DDR needs such
		 * a synchronization operation. This is to make sure the last DDR write, if exists,
		 * shows up properly in DDR before moving on.
		__pcie_ctl_dma_ready(PCIE_CORE_TO_CTL(core));
		 */

		if (istatus & PCIE_INTR_MSI) {
			/*
			 * Read the MSI interrupt status out and clear them
			 */
			imsistatus = pcie_core_reg_readl(pp, &core->imsistatus);
			pcie_core_reg_writel(pp, &core->imsistatus, imsistatus);
			imsistatus &= (1 << PCIE_PORT_MSI_MAX) - 1;
		}

		inta = (istatus & PCIE_INTR_INTA);
	} else {
		/*
		 * Print a simple warning if there is an interrupt other than MSI
		 */
		printk(KERN_WARNING "pcie unexpected int, istatus=%x, imsistatus=%x\n",
				istatus, imsistatus);
		return IRQ_HANDLED;
	}

	spin_unlock(&pp->conf_lock);

	/* Dispatch all MSI interrupts */
	if (imsistatus) {
		int i = 0;
		for (; imsistatus && (i < 32); i++, imsistatus >>= 1) {
			if (imsistatus & 1) {
				PCIE_TRACE("PCIe irq relay -> msi:%d\n", pp->msi_irq_base + i);
				generic_handle_irq(pp->msi_irq_base + i);
			}
		}
	}

	if (inta) {
		PCIE_TRACE("PCIe irq relay -> inta:%d\n", pp->inta_irq);
		generic_handle_irq(pp->inta_irq);

		/*
		 * The pcie core has a hardware defect which can not really clear it until the device's
		 * interrupt is cleared, so clear it after the isr is finished. RACE CONDITION NOT FIXED!
		pcie_core_reg_writel(pp, &core->istatus, PCIE_INTR_INTA);
		 */
	}

	return IRQ_HANDLED;
}

/*
 * One of the hw_pci methods.
 * Allocate necessary memory and IO resources for all pcie controllers.
 */
static void __init ubi32_pcie_preinit(void)
{
	int i;
	int ret;

	for (i = 0; i < num_pcie_ports; i++) {
		struct pcie_port *pp = &pcie_ports[i];

		/* Skip down links */
		if (!pp->is_up) {
			continue;
		}

		PCIE_INFO("Preinit for pcie port %d\n", pp->port);

		/* reserve IO and memory resources */
		ret = request_resource(&ioport_resource, &pp->res[0]);
		//BUG_ON(ret);
		ret = request_resource(&iomem_resource, &pp->res[1]);
		BUG_ON(ret);

	}
}

/*
 * pci_ops method to read a config register of size 1, 2, 4 bytes
 */
static int pcie_read_config(struct pci_bus *bus, unsigned int devfn,
				 int where, int size, u32 *value)
{
	struct pcie_port *pp = ((struct pci_sys_data *)bus->sysdata)->pport;
	unsigned long flags;
	u32 cfg_addr;
	u32 istatus;

	PCIE_TRACE("pcie_read_config for bus(%x) devfn(%x) where(%d) sz(%d)\n", bus->number,
			devfn, where, size);

	if (pcie_valid_config(pp, bus->number, PCI_SLOT(devfn)) == 0) {
		*value = 0xffffffff;
		PCIE_WARNING("\tdevice not found\n");
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	pcie_get_config_addr(bus->number, devfn, where & ~0x03, &cfg_addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
	istatus = __pcie_cfg_readl(pp, cfg_addr, value);
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	if (istatus) {
		printk(KERN_WARNING "Failed conf rd for reg %d of %d:%d:%d, istatus=%x\n",
			where, 	bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), istatus);
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	if (size != 4) {
		int shift = (where & 0x3) << 3;
		PCIE_TRACE("Config register (4b) at %x is %08x, size=%d, shift=%d\n", where & ~0x03, *value, size, shift);
		*value = *value >> shift;
		*value = (size == 2) ? (*value & 0xffff) : (*value & 0xff);
	}

	PCIE_TRACE("\tvalue=0x%08x\n", *value);
	return PCIBIOS_SUCCESSFUL;
}

/*
 * pci_ops method to write a config register of size 1, 2, 4 bytes
 */
static int pcie_write_config(struct pci_bus *bus, unsigned int devfn,
				  int where, int size, u32 value)
{
	struct pcie_port *pp = ((struct pci_sys_data *)bus->sysdata)->pport;
	unsigned long flags;
	u32 cfg_addr, val = value;
	u32 istatus;

	PCIE_TRACE("pcie_write_config for bus(%x) devfn(%x) where(%d) sz(%d), val=%x\n", bus->number,
			devfn, where, size, value);

	if (pcie_valid_config(pp, bus->number, PCI_SLOT(devfn)) == 0) {
		PCIE_WARNING("\tdevice not found\n");
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	pcie_get_config_addr(bus->number, devfn, where & ~0x3, &cfg_addr);

	spin_lock_irqsave(&pp->conf_lock, flags);
	if (size != 4) {
		int shift = (where & 0x3) << 3;
		istatus = __pcie_cfg_readl(pp, cfg_addr, &val);
		if (istatus) {
			printk(KERN_WARNING "Failed conf wr first step (rd) for reg %d of %d:%d:%d, istatus=%x\n",
				where,  bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), istatus);
			goto exit1;
		}

		if (size == 1) {
			val &= ~(0xff << shift);
		} else {
			val &= ~(0xffff << shift);
		}
		val |= (value << shift);
	}

	PCIE_TRACE("\t write value 0x%08x to cfg reg %x (addr=%x), size=%d\n", val, where, cfg_addr, size);
	istatus = __pcie_cfg_writel(pp, cfg_addr, val);

exit1:
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	if (istatus) {
		printk(KERN_WARNING "Failed conf wr for reg %d of %d:%d:%d, istatus=%x\n",
			where,  bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), istatus);
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	return PCIBIOS_SUCCESSFUL;
}

/*
 * pcie config space read/write operation methods.
 */
static struct pci_ops ubi32_pcie_ops = {
	.read   = pcie_read_config,
	.write  = pcie_write_config,
};

/*
 * Prevent enumeration of root complex.
 */
static void __devinit rc_pci_fixup(struct pci_dev *dev)
{
	PCIE_INFO("root complex pci fixup is called\n");
	if (dev->bus->parent == NULL && dev->devfn == 0) {
		int i;
		for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
			dev->resource[i].start = 0;
			dev->resource[i].end   = 0;
			dev->resource[i].flags = 0;
		}
	}
}
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_UBICOM, PCI_ANY_ID, rc_pci_fixup);

/*
 * A hw_pci method to scan all the buses of a specific root complex (pcie controller)
 */
static struct pci_bus __init *ubi32_pcie_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pcie_port *pp;
	PCIE_INFO("ubi32_pcie_scan_bus is called for nr=%d\n", nr);

	BUG_ON(nr >= num_pcie_ports);

	pp = sys->pport;
	BUG_ON(nr != pp->port);

	if (!pp->is_up) {
		return NULL;
	}

	return pci_scan_bus(sys->busnr, &ubi32_pcie_ops, sys);
}

/*
 * Set up the pciahb and ahbpci windows of a pcie controller.
 */
static void __init pcie_port_setup_wins(struct pcie_port *pp)
{
	struct pcie_core_regs *core = pp->core;
	size_t bits;

	/* Disable window1 of both pciahb and ahbpci windows */
	pcie_core_reg_writel(pp, &core->pciahb1_ahbbase, 0);
	pcie_core_reg_writel(pp, &core->ahbpcihi1_ahbbase, 0);
	pcie_core_reg_writel(pp, &core->ahbpcilo1_ahbbase, 0);

	/*
	 * Map PCI address 1:1 into physical memory with offset == 0, to allow PCI to access
	 * the full DDR range.
	 *
	 * Note: if we can assume the max DDR size is 512M, maybe we can reserve the 2nd half
	 * of DDR memory space to allocate to the devices connected to the root port, starting
	 * from 0xe0000000 to 0xfffffff -- another choice
	 *
	 * FIXME: find a method to get the real DDR size
	 */
	bits = size_to_bits(0xffffffff - SDRAMSTART + 1);
	pcie_core_reg_writel(pp, &core->pciahb0_ahbbase, PCIE_PCIAHB_CTL(SDRAMSTART, 1, 0, bits));

	/* setup BAR0 to start of DRAM, and enable prefetch */
	__pcie_core_cfg_writel(pp, PCIE_CFG_OFFSET_BAR0, SDRAMSTART | 0x08);

	/*
	 * Program the AHB-PCI window0 based on resource we got, which should be different
	 * for each rootport instance. We assume res[1] is mem mapped space and there is
	 * only one device attached to the root complex.
	 * FIXME: if the above assumption does not stand
	 */
	BUG_ON(pp->res[1].flags != IORESOURCE_MEM);
	bits = size_to_bits(pp->res[1].end  - pp->res[1].start + 1);
	pcie_core_reg_writel(pp, &core->ahbpcihi0_ahbbase, 0);
	pcie_core_reg_writel(pp, &core->ahbpcilo0_ahbbase, PCIE_AHBPCI_CTL_LOW(pp->res[1].start, 0, bits));
}

/*
 * Set up some ID related config space registers.
 */
static void __init pcie_port_setup_ids(struct pcie_port *pp)
{
	/* FIXME: Update vendor ID and device ID field. */
	u32 id = PCI_VENDOR_ID_UBICOM | (PCIE_RC_UBICOM_DEV_ID << 16);
	__pcie_core_cfg_writel(pp, PCI_VENDOR_ID, id);

	/* FIXME: class_code << 16 | revision_id, hardcode now. */
	id = PCI_CLASS_BRIDGE_PCI << 16 | 02;
	__pcie_core_cfg_writel(pp, PCI_CLASS_REVISION, id);

	/* head_type << 16 | cache_line == 32 */
	id = (PCI_HEADER_TYPE_BRIDGE << 16) | (L1_CACHE_BYTES >> 2);
	__pcie_core_cfg_writel(pp, PCI_CACHE_LINE_SIZE, id);
}

/*
 * Set up the controller and enable it.
 */
static int __init ubi32_pcie_setup(int nr, struct pci_sys_data *sys)
{
	struct pcie_port *pp;
	u32 cmd, bus;
	unsigned long flags;

	PCIE_INFO("ubi32_pcie_setup is called for nr=%d\n", nr);

	if (nr >= num_pcie_ports)
		return 0;

	pp = &pcie_ports[nr];
	sys->pport = pp;

	if (!pp->is_up) {
		return 0;
	}

	spin_lock_irqsave(&pp->conf_lock, flags);

	pp->root_bus_nr = sys->busnr;

	/* Set up PCIe unit decode windows to DRAM space */
	pcie_port_setup_wins(pp);

	/* Set up device, vendor, class, etc */
	pcie_port_setup_ids(pp);

	/*
	 * FIXME: Master + slave enable. Do we need IO?
	 */
	cmd = __pcie_core_cfg_readl(pp, PCI_COMMAND);
	cmd |= (PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	__pcie_core_cfg_writel(pp,  PCI_COMMAND, cmd);

	/*
	 * Config local bus numbes
	 * Set Primary Bus Number = nr, Sec. = nr + 1 and Sub. = 0xFF
	 * It is up to the pci core enumeration code to set subordinate bus properly
	 *
	 * FIXME: once primary bus number is assigned, should we still use BDF(0, 0, 0)
	 * to access the second controller?
	 */
	bus = (0xFF << 16) | ((sys->busnr + 1) << 8) | sys->busnr;
	__pcie_core_cfg_writel(pp, PCI_PRIMARY_BUS, bus);

	/* request irq for the controller itself */
	if (request_irq(pp->ctl_irq, ubi32_pcie_interrupt, 0, port_names[pp->port], pp)) {
		BUG();
	}

	/*
	 * Enable those interesting interrupts,  assumes only one device connected
	 * to this root bridge.
	 * FIXME: add more error INTR checking
	 */
	pcie_core_reg_writel(pp, &pp->core->imask, PCIE_INTR_CTL_MSK);

	/*
	 * FIXME: switch to the above solution after it is fixed in hardware.
	 * workaround for 1st tapeout: interrupts are not delivered if we only set CTL_MSK here
	 * endianess is not the cause.
	 */
	pp->ctl->int_mask = 0xffffffff;

	/* Update resources */
	sys->resource[0] = &pp->res[0];
	sys->resource[1] = &pp->res[1];
	sys->resource[2] = NULL;

	spin_unlock_irqrestore(&pp->conf_lock, flags);

	return 1;
}

/*
 * Map a PCIe device's interrupt pin number to system interrupt number
 */
static int ubi32_pcie_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	struct pcie_port *pp = ((struct pci_sys_data *)dev->bus->sysdata)->pport;

	PCIE_INFO("ubi32_pcie_map_irq called for bus(%d) slot (%d), pin(%d)\n",
			dev->bus->number, slot, pin);

	/*
	 * Force every device of a port to use the same irq. This is just a dummy
	 * place holder. Due to a hardware defect, the PLDA controller's INTx has some
	 * limitation, all devices are recommended to switch to MSI.
	 */
	return pp->inta_irq;
}

struct hw_pci ubi32_pci __initdata = {
	.nr_controllers	= 2,
	.preinit	= ubi32_pcie_preinit,
	.setup		= ubi32_pcie_setup,
	.scan		= ubi32_pcie_scan_bus,
	.map_irq	= ubi32_pcie_map_irq,
};

struct pcie_devnode {
	struct 	devtree_node dn;
	int	inta_irq;
	int	msi_irq_base;
	struct resource res[2];
};

/*
 * FIXME: Fake devtree entries for bringup only. Will improve devtree & ultra later
 */
struct pcie_devnode __initdata pcie_devtree[2] =
{
	{
		.dn = {.recvirq = 93},
		/* use the power management intr to deliver INTa or MSI */
		.inta_irq = 92,			//should never be used
		.msi_irq_base = (32+12),	// we need two MSI, one for rootport, one for the device
						// this is an ugly hack to steal other INTs
		.res[0] = {
			.name	= "PCIeRJ IO space",
			.start	= 0,
			.end	= 0,
			.flags	= IORESOURCE_IO,
		},
		.res[1] = {
			.name	= "PCIeRJ mem space",
			.start  = PCI_DEV_ADDR_BASE,
			.end	= PCI_DEV_ADDR_BASE + (PCI_DEV_ADDR_SIZE / 2) - 1,
			.flags	= IORESOURCE_MEM,
		},
	},
	{
		.dn = {.recvirq = 95},
		/* use the power management intr to deliver INTa or MSI */
		.inta_irq = 94,			//should never be used
		.msi_irq_base = (32+12+PCIE_PORT_MSI_MAX),
		.res[0] = {
			.name	= "PCIeRK IO space",
			.start	= 0,
			.end	= 0,
			.flags	= IORESOURCE_IO,
		},
		.res[1] = {
			.name	= "PCIeRK mem space",
			.start  = PCI_DEV_ADDR_BASE + (PCI_DEV_ADDR_SIZE / 2),
			.end	= PCI_DEV_ADDR_BASE + PCI_DEV_ADDR_SIZE - 1,
			.flags	= IORESOURCE_MEM,
		},
	},
};

/*
 * Select a function for an IOPIO port
 */
static void inline __init pcie_io_port_func_select(struct ubicom32_io_port *io, int func)
{
	u32 func_val = pcie_nbr_readl(&io->function) & ~0x7;

	/* select the function */
	func_val |=  IO_FUNC_SELECT(func);
	pcie_nbr_writel(&io->function, func_val);
}

/*
 * Reset a function of an IOPIO port
 */
static void inline __init pcie_io_port_func_reset_only(struct ubicom32_io_port *io, int func)
{
	u32 func_val = pcie_nbr_readl(&io->function);

	/* assert the reset */
	func_val |= IO_FUNC_FUNCTION_RESET(func);
	pcie_nbr_writel(&io->function, func_val);
}

/*
 * Select an IO function and reset it
 */
static void inline __init pcie_io_port_func_reset(struct ubicom32_io_port *io, int func)
{
	pcie_io_port_func_select(io, func);
	pcie_io_port_func_reset_only(io, func);
}

/*
 * Get an IO function out of reset
 */
static void inline __init pcie_io_port_func_remove_reset(struct ubicom32_io_port *io, int func)
{
	u32 func_val = pcie_nbr_readl(&io->function);
	func_val &= ~IO_FUNC_FUNCTION_RESET(func);
	func_val |= 0x100;
	pcie_nbr_writel(&io->function, func_val);
}

/*
 * A generic function to be called by board init function to use different GPIO settings if necessary
 */
void __init __ubi32_pcie_reset_endpoint(int port, u32 reset_pg, u32 reset_pin, u32 clk_pg, u32 clk_pin)
{
	u32 pgr_dir, pgr_out;
	u32 pgc_dir, pgc_fn1;

	printk("PCIe port[%d] reset: reset pin 0x%08x:%d clock pin 0x%08x:%d\n",
		port, reset_pg, reset_pin, clk_pg, clk_pin);

	/* Set reset_l bit of pg4 pin20 to 0 */
	pgr_out = pcie_nbr_readl((volatile u32 *)(reset_pg + IO_GPIO_OUT));
	pcie_nbr_writel((volatile u32 *)(reset_pg + IO_GPIO_OUT), pgr_out & ~(1 << reset_pin));

	/* Set reset signal to the external endpoint */
	pgr_dir = pcie_nbr_readl((volatile u32 *)(reset_pg + IO_GPIO_CTL));
	pcie_nbr_writel((volatile u32 *)(reset_pg + IO_GPIO_CTL), pgr_dir | (1 << reset_pin));

	mdelay(100);

	/* Supply 100Mhz reference clock for the endpoint */
	pgc_dir = pcie_nbr_readl((volatile u32 *)(clk_pg + IO_GPIO_CTL));
	pcie_nbr_writel((volatile u32 *)(clk_pg + IO_GPIO_CTL), pgc_dir | (1 << clk_pin));

	pgc_fn1 = pcie_nbr_readl((volatile u32 *)(clk_pg + IO_GPIO_FN1));
	pcie_nbr_writel((volatile u32 *)(clk_pg + IO_GPIO_FN1), pgc_fn1 | (1 << clk_pin));
	printk(KERN_INFO "\tPCIe port %d:  endpoint ref clock set to 100 Mhz\n", port);

	mdelay(100);

	/* Set reset_l bit to 1 */
	pgr_out = pcie_nbr_readl((volatile u32 *)(reset_pg + IO_GPIO_OUT));
	pcie_nbr_writel((volatile u32 *)(reset_pg + IO_GPIO_OUT), pgr_out | (1 << reset_pin));
}

/*
 * Reset the port's phy and controller. After this function
 * the controller core is ready to be programed.
 */
static void __init pcie_port_reset(struct pcie_port *pp)
{
	struct ubicom32_io_port *phy = pp->phy;
	struct ubicom32_io_port *ctl = pp->ctl;

	/* select the ctl, func number 1 */
	pcie_io_port_func_select(ctl, 1);

	/* reset the phy, func number 1, and configure it */
	pcie_io_port_func_reset(phy, 1);
	pcie_io_port_func_remove_reset(phy, 1);
	pcie_phy_config(phy);

	/* wait for phy CMU ok signal */
	PCIE_INFO("\twaiting for CMU ok\n");
	while (!(pcie_nbr_readl(&phy->status0) & 1));

	/* remove ctl out of reset */
	pcie_io_port_func_reset_only(ctl, 1);
	pcie_io_port_func_remove_reset(ctl, 1);

	/* wait for phy to completely out of reset */
	PCIE_INFO("\twaiting for phy out of reset\n");
	while (pcie_nbr_readl(&phy->status0) & 2);
	PCIE_INFO("\tPhy out of reset, stat0=%x\n", pcie_nbr_readl(&phy->status0));

	/* wait for the ctl to completely out of reset */
	PCIE_INFO("\twaiting for the ctl out of reset\n");
	while ((pcie_nbr_readl(&ctl->status2) & 0x00000020) != 0x00000020);
	PCIE_INFO("\tCtl out of reset, stat2=%x\n", pcie_nbr_readl(&ctl->status2));

	ubi32_pcie_reset_endpoint(pp->port);


	pcie_io_port_func_reset_only(ctl, 1);
	pcie_io_port_func_remove_reset(ctl, 1);

	/* wait for the ctl to completely out of reset */
	PCIE_INFO("\twaiting for the ctl out of reset\n");
	while ((pcie_nbr_readl(&ctl->status2) & 0x00000020) != 0x00000020);
	PCIE_INFO("\tCtl out of reset, stat2=%x\n", pcie_nbr_readl(&ctl->status2));
}

/*
 * Initialize the pcie controller core registers
 */
static void __init pcie_port_init_core(struct pcie_port *pp)
{
	/* core registers starts with BR offset 0 */
	struct pcie_core_regs *core = pp->core;
	unsigned long flags;
	spin_lock_irqsave(&pp->conf_lock, flags);

	/* Mask off all interrupts initially */
	pcie_core_reg_writel(pp, &core->imask, 0);

	/* Write DEV_ID & Vendor ID (FIXME: we are doing it twice, again in cfg space setup_ids) */
	pcie_core_reg_writel(pp, &core->deviceid, (PCIE_RC_UBICOM_DEV_ID << 16) | PCI_VENDOR_ID_UBICOM);
	pcie_core_reg_writel(pp, &core->class_code, (0x06040002));

	/*
	 * ASPM must be configured, and timer is necessary to avoid blocking AHB bus
	 * Also set the proper gen2 mode
	 */
	pcie_core_reg_writel(pp, &core->ahbpci_timer, PCIE_AHBPCIE_TIMER_DATA);
	pcie_core_reg_writel(pp, &core->aspmconf, PCIE_ASPM_CONF_DATA);
	pcie_core_reg_writel(pp, &core->gen2_conf, PCIE_GEN2_CONF_DATA);

	/* Set the msi report addr */
	pcie_core_reg_writel(pp, &core->imsiaddr, (u32)pp->msi_addr);

	spin_unlock_irqrestore(&pp->conf_lock, flags);
	PCIE_INFO("\tcore init done\n");
}

/*
 * Keep polling link state till it is either L0 or too long
 */
static int __init pcie_port_link_state_L0(struct pcie_port *pp)
{
	u32 ltssm;
	u32 cnt = 0;

	printk(KERN_INFO "\tPolling LTSSM state ... takes up to 2 seconds\n");
	do {
		/* LTSSM is reported by the lower 4 bits of status2 */
		ltssm = pcie_nbr_readl(&pp->ctl->status2) & 0xF;
		PCIE_INFO("\t\tLTSSM state = %x at trial %d\n", ltssm, cnt);
		mdelay(20);
	} while ((ltssm != 0xF) && (cnt++ < 100));

	printk(KERN_INFO "\tLTSSM state of port %d is %x, %s\n", pp->port, ltssm, ltssm == 0xf? "up" : "down");
	return ltssm == 0xF;
}

/*
 * This function inits the phy and controller and brings up the link to L0 state
 * It assumes it is only called at init phase and no spinlock is needed.
 *
 * Return 0 means link is initialized properly and is up.
 */
static int __init pcie_port_init(struct pcie_port *pp)
{
	/* Update the clock source from 125 Mhz to 250 Mhz */
	u32 clk_select = pcie_nbr_readl((volatile u32 *)(GENERAL_CFG_BASE + GEN_CLK_IO_SELECT));
	pcie_nbr_writel((volatile u32 *)(GENERAL_CFG_BASE + GEN_CLK_IO_SELECT), clk_select | (1 << pp->port));
	printk(KERN_INFO "\tPCIe port %d: core clock source set to 250 Mhz\n", pp->port);

	mdelay(1);

	/* Reset and init the phy, core, wait for them to be ready */
	pcie_port_reset(pp);

	mdelay(1);

	/* Set endianess, role (RP), and mode (AER) properly, also set slotclk_cfg to jupiter common ref clock */
//	pcie_nbr_writel(&pp->ctl->ctl3, PCIE_BRIDGE_ENDIAN_RP_AER | (1 << 16));
	pcie_nbr_writel(&pp->ctl->ctl3, PCIE_BRIDGE_ENDIAN_RP_AER);

	mdelay(1);

	/* Initialize the ctl core */
	pcie_port_init_core(pp);

	mdelay(1);

	/* Doing crst/prst for loading new values in core reg */
	pcie_nbr_writel(&pp->ctl->int_set, 0x3);
	mdelay(10);

	/* poll to see if the link state moves to L0 */
	if (!pcie_port_link_state_L0(pp)) {
		unsigned long flags;
		spin_lock_irqsave(&pp->conf_lock, flags);
		pcie_dump_status_regs(pp);
		spin_unlock_irqrestore(&pp->conf_lock, flags);
		return -1;
	}

	return 0;
}

/*
 * The main entrance to setup and enable a pcie controller port.
 * It does all the work before handling it to the generic pcibios code.
 */
static void __init add_pcie_port(int port, struct pcie_devnode *devnode)
{
	struct pcie_port *pp = &pcie_ports[num_pcie_ports++];

	printk(KERN_INFO "Trying add PCIe port %d: %s\n", port, devnode->dn.name);

	pp->port = port;
	/* FIXME: now we assume the device is already connected, need improve later after bringup */
	pp->is_up = 0;
	pp->root_bus_nr = -1;
	spin_lock_init(&pp->conf_lock);
	memset(pp->res, 0, sizeof(pp->res));

	pp->ctl_irq = devnode->dn.recvirq;
	pp->inta_irq = devnode->inta_irq;
	pp->msi_irq_base = devnode->msi_irq_base;
	pp->msi_irq_in_use = 0;

	pp->mem_base = devnode->res[1].start;
	pp->mem_mask = ~(devnode->res[1].end ^ devnode->res[1].start);

	/* Cache the starting address of port control registers */
	pp->phy = (struct ubicom32_io_port *)PCIE_PHY_FUNC_BASE(port);
	pp->ctl = (struct ubicom32_io_port *)PCIE_CTL_FUNC_BASE(port);
	pp->core = (struct pcie_core_regs *)PCIE_CTL_BR_BASE(port);

	/*
	 * Setup the MSI address register, this address must not fall in the PCI-AHB window
	 * range, also it has to be 64 bit aligned.
	 *
	 * So any address not overlapping with DDR should work, for now just re-use the last
	 * qword address of the NBR region.
	 */
	pp->msi_addr = (u32)pp->core - 8;

	pp->res[0] = devnode->res[0];
	pp->res[1] = devnode->res[1];

	/* Init phy, core and make sure link is up */
	if (pcie_port_init(pp)) {
		printk(KERN_WARNING "PCIe port %d: %s init failed\n", port, devnode->dn.name);
		return;
	}

	/* Every thing is ok, mark this port up */
	pp->is_up = 1;
	printk(KERN_INFO "PCIe port %d: %s link is up\n", port, devnode->dn.name);
}

/*
 * The pcie subsystem init entry point
 */
static int __init ubi32_pcie_init(void)
{
	struct pcie_devnode *devnode0, *devnode1;

	devnode0 = (struct pcie_devnode *)devtree_find_node(port_names[0]);
	if (devnode0 != NULL) {
		printk(KERN_INFO "PCI port RJ is added\n");
		add_pcie_port(0, &pcie_devtree[0]);
	}

	devnode1 = (struct pcie_devnode *)devtree_find_node(port_names[1]);
	if (devnode1 != NULL) {
		printk(KERN_INFO "PCI port RK is added\n");
		add_pcie_port(1, &pcie_devtree[1]);
	}

	if ((devnode0 == NULL) && (devnode1 == NULL)) {
		printk(KERN_WARNING "PCI port RJ/RK init failed\n");
		return -ENOSYS;
	}

	pci_common_init(&ubi32_pci);
	PCIE_INFO("End of %s\n", __FUNCTION__);

	return 0;
}

subsys_initcall(ubi32_pcie_init);

