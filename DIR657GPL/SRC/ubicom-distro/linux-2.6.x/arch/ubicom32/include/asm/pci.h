/*
 * arch/ubicom32/include/asm/pci.h
 *   Definitions of PCI operations for Ubicom32 architecture.
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
 *   arch/arm
 */
#ifndef _ASM_UBICOM32_PCI_H
#define _ASM_UBICOM32_PCI_H

#include <asm/io.h>

/* The PCI address space does equal the physical memory
 * address space.  The networking and block device layers use
 * this boolean for bounce buffer decisions.
 */
#define PCI_DMA_BUS_IS_PHYS	(1)



/*
 * Perform a master read/write to the PCI bus.
 * These functions return a PCI_RESP_xxx code.
 */
extern u8 pci_read_u32(u8 pci_cmd, u32 address, u32 *data);
extern u8 pci_write_u32(u8 pci_cmd, u32 address, u32 data);
extern u8 pci_read_u16(u8 pci_cmd, u32 address, u16 *data);
extern u8 pci_write_u16(u8 pci_cmd, u32 address, u16 data);
extern u8 pci_read_u8(u8 pci_cmd, u32 address, u8 *data);
extern u8 pci_write_u8(u8 pci_cmd, u32 address, u8 data);

#define PCIBIOS_MIN_IO          0x100
#define PCIBIOS_MIN_MEM         0x10000000

#define pcibios_assign_all_busses()	0
#define pcibios_scan_all_fns(a, b)	0
extern void pcibios_resource_to_bus(struct pci_dev *dev, struct pci_bus_region *region,
	struct resource *res);

extern void pcibios_bus_to_resource(struct pci_dev *dev, struct resource *res,
	struct pci_bus_region *region);

#ifdef CONFIG_PCI
#if (UBICOM32_ARCH_VERSION > 4)
/*
 * Register definition of the PCIe IP core
 */
struct pcie_core_regs {
        volatile u32            wdma_pci_addr_0;
        volatile u32            wdma_pci_addr_1;
        volatile u32            wdma_ahb_addr;
        volatile u32            wdma_cntl;
        volatile u32            rsvd0[4];
        volatile u32            rdma_pci_addr_0;
        volatile u32            rdma_pci_addr_1;
        volatile u32            rdma_ahb_addr;
        volatile u32            rdma_cntl;
        volatile u32            rsvd1[4];
        volatile u32            imask;
        volatile u32            istatus;
        volatile u32            icmd_reg;
        volatile u32		info;
        volatile u32            imsistatus;
        volatile u32            imsiaddr;
        volatile u32            rsvd2;
        volatile u32            slot_cap_reg;
        volatile u32            deviceid;
        volatile u32            sub_id;
        volatile u32            class_code;
        volatile u32      	slotcsr;
        volatile u32      	prm_csr;
        volatile u32      	dev_csr;
        volatile u32      	link_csr;
        volatile u32      	root_csr;
        volatile u32            pciahb0_ahbbase;
        volatile u32            pciahb1_ahbbase;
        volatile u32            rsvd3[2];
        volatile u32            ahbpcilo0_ahbbase;
        volatile u32            ahbpcihi0_ahbbase;
        volatile u32            ahbpcilo1_ahbbase;
        volatile u32            ahbpcihi1_ahbbase;
        volatile u32            ahbpci_timer;
        volatile u32            rsvd4;
        volatile u32            gen2_conf;
        volatile u32            aspmconf;
        volatile u32		pmstatus;
        volatile u32            pmconf2;
        volatile u32            pmconf1;
        volatile u32            pmconf0;
};

/*
 * Abstraction of a PCIe port
 *
 *	port		0/1, instance number
 *	root_bus_nr	assigned bus number
 *	conf_lock	spinlock protecting this port
 *
 * 	ctl_irq		irq number for the controller
 * 	msi_irq_base	the base irq number of all msi interrupts of the controller
 * 	msi_irq_in_use	which irqs are being allocated
 * 	msi_addr	MSI capture address
 *
 * 	mem_base	cache of dev BAR0 start
 * 	mem_mask	To speed up address match.
 *
 *	mem_space_name	name
 *	res		resource from os
 *	phy		addr of NBR registers of the phy
 *	ctl		addr of NBR registers of the controller
 *	core		addr of BR registers of the controller
 */
struct pcie_port {
	u8                      port;
	u8                      root_bus_nr;
	u8			is_up;
	spinlock_t              conf_lock;

	int			ctl_irq;
	int			inta_irq;
	int			msi_irq_base;
	u32			msi_irq_in_use;
	u32 			msi_addr;

	struct ubicom32_io_port	*phy;
	struct ubicom32_io_port	*ctl;
	struct pcie_core_regs	*core;

	/*
	 * Make sure the internal DMA does not mess up with others on the same cache line
	 */
	u32			last_wr_addr;
	u32			pad1[7];
	u32			buf;
	u32			pad2[7];

	u32			mem_base;
	u32			mem_mask;
	char                    io_space_name[16];
	char                    mem_space_name[16];
	struct resource         res[2];
};

#define pci_domain_nr(bus) (((struct pci_sys_data *)bus->sysdata)->pport->port)
#define pci_proc_domain(bus) (1)

/* implement the pci_ DMA API in terms of the generic device dma_ one */
#include <asm-generic/pci-dma-compat.h>

#else
#define HAVE_ARCH_PCI_SET_DMA_MAX_SEGMENT_SIZE 1
#define HAVE_ARCH_PCI_SET_DMA_SEGMENT_BOUNDARY 1

#include <asm/pci-dma-v34.h>

#endif

/*
 * Per-controller structure
 */
struct pci_sys_data {
        struct list_head node;
        int             busnr;          /* primary bus number                   */
        u64             mem_offset;     /* bus->cpu memory mapping offset       */
        unsigned long   io_offset;      /* bus->cpu IO mapping offset           */
        struct pci_bus  *bus;           /* PCI bus                              */
        struct resource *resource[3];   /* Primary PCI bus resources            */
                                        /* Bridge swizzling                     */
        u8              (*swizzle)(struct pci_dev *, u8 *);
                                        /* IRQ mapping                          */
        int             (*map_irq)(struct pci_dev *, u8, u8);
        struct hw_pci   *hw;
#if (UBICOM32_ARCH_VERSION > 4)
	struct pcie_port *pport;
#endif
};

struct hw_pci {
        struct list_head buses;
        int             nr_controllers;
        int             (*setup)(int nr, struct pci_sys_data *);
        struct pci_bus *(*scan)(int nr, struct pci_sys_data *);
        void            (*preinit)(void);
        void            (*postinit)(void);
        u8              (*swizzle)(struct pci_dev *dev, u8 *pin);
        int             (*map_irq)(struct pci_dev *dev, u8 slot, u8 pin);
};

/*
 * This is the standard PCI-PCI bridge swizzling algorithm.
 */
u8 pci_std_swizzle(struct pci_dev *dev, u8 *pinp);

/*
 * Call this with your hw_pci struct to initialise the PCI system.
 */
void pci_common_init(struct hw_pci *);

static  inline struct resource *
pcibios_select_root(struct pci_dev *pdev, struct resource *res)
{
        struct resource *root = NULL;

        if (res->flags & IORESOURCE_IO)
                root = &ioport_resource;
        if (res->flags & IORESOURCE_MEM)
                root = &iomem_resource;

        return root;
}

static inline void pcibios_set_master(struct pci_dev *dev)
{
        /* No special bus mastering setup handling */
}

extern void __iomem *pci_iomap(struct pci_dev *dev, int bar, unsigned long max);
extern void pci_iounmap(struct pci_dev *dev, void __iomem *);
#endif

#endif /* _ASM_UBICOM32_PCI_H */
