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

#ifndef __XHCI_UBI32_H__
#define __XHCI_UBI32_H__

#ifdef CONFIG_UBICOM32
#include <asm/ubicom32-io.h>

/* Flush the buf out of cache, data is on the way to DDR memory */
#define UBI_DMA_FLUSH(buf, size)	ubi32_flush(buf, size)

/* Make sure the previour written data is in DDR memory and ready for DMA devices to read */
#define UBI_DMA_SYNC()			ubi32_sync()

/* Invalidate the cache lines, data from DMA device is ready for the CPU to read */
#define UBI_DMA_INVALIDATE(buf, size)	ubi32_invalidate(buf, size)

/* Make sure the data written by the DMA device has reached DDR */
#define UBI_DMA_CHECK_READY(xhci)	ubi32_xhci_dma_ready(FUNCTION_BASE(xhci->cap_regs))

/* Flush the whole input ctx out, not very time consuming, not called very often */
static inline void ubi32_flush_xhci_in_ctx(struct xhci_hcd *xhci, dma_addr_t in_ctx_ptr,
		u32 slot_id)
{
	struct xhci_container_ctx *in_ctx = xhci->devs[slot_id]->in_ctx;
	ubi32_flush(in_ctx->bytes, in_ctx->size);
}
#define UBI_DMA_FLUSH_IN_CTX(xhci, in_ctx_ptr, slot_id)	ubi32_flush_xhci_in_ctx(xhci, in_ctx_ptr, slot_id)

static inline unsigned int xhci_readl(const struct xhci_hcd *xhci,
		__u32 __iomem *regs)
{
	return ubi32_xhci_readl(regs, FUNCTION_BASE(xhci->cap_regs));
}

static inline void xhci_writel(struct xhci_hcd *xhci,
		const unsigned int val, __u32 __iomem *regs)
{
	ubi32_xhci_writel(val, regs, FUNCTION_BASE(xhci->cap_regs));
}

/* Workaround for non-aligned trbs */
extern int xhci_init_4bytes_pool(struct xhci_hcd *xhci);
extern void xhci_free_4bytes_pool(struct xhci_hcd *xhci);
extern dma_addr_t xhci_align_map_4bytes(struct xhci_hcd *xhci, void *buf);
extern void xhci_align_unmap_4bytes(struct xhci_hcd *xhci, dma_addr_t addr);

#else
#define UBI_DMA_FLUSH(buf, size)
#define UBI_DMA_SYNC()
#define UBI_DMA_INVALIDATE(buf, size)
#define UBI_DMA_CHECK_READY(regs)
#define UBI_DMA_FLUSH_IN_CTX(xhci, in_ctx_ptr, slot_id)
#endif

#endif
