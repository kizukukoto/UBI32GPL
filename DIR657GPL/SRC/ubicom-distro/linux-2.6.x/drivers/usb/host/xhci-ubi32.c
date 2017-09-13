/*
 * driver/usb/host/xhci-ubi32.c
 *   Ubicom32 architecture platform driver for xhci.
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
#include <linux/platform_device.h>
#include <asm/ubicom32.h>
#include <linux/dma-mapping.h>

/* Turn off Power Management first */
#undef CONFIG_PM

int xhci_init_4bytes_pool(struct xhci_hcd *xhci)
{
	struct device *dev = xhci_to_hcd(xhci)->self.controller;
	struct xhci_4bytes_pool *pool = kzalloc(sizeof(struct xhci_4bytes_pool), GFP_KERNEL);
	if (!pool)
		return -ENOMEM;

	pool->buf = (void *)dma_alloc_coherent(dev, XHCI_4BYTES_POOL_SIZE, &pool->dma, GFP_KERNEL);
	if (!pool->buf) {
		kfree(pool);
		return -ENOMEM;
	}

	xhci->align_pool = pool;
	return 0;
}

void xhci_free_4bytes_pool(struct xhci_hcd *xhci)
{
	struct device *dev = xhci_to_hcd(xhci)->self.controller;
	if (xhci->align_pool == NULL)
		return;

	dma_free_coherent(dev, XHCI_4BYTES_POOL_SIZE, xhci->align_pool->buf, xhci->align_pool->dma);
	kfree(xhci->align_pool);
	xhci->align_pool = NULL;
}

static char *xhci_alloc_4bytes(struct xhci_hcd *xhci, dma_addr_t *addr)
{
	u32 *bitmap = xhci->align_pool->bitmap;
	u32 offset = 0;
	int i = 0, j;

	/* Find a free slot */	
	for (; i < XHCI_4BYTES_MAP_CNT; i++, bitmap++) {
		if (*bitmap != 0xFFFFFFFF)
			break;
	}
	if (i == XHCI_4BYTES_MAP_CNT)
		return NULL;

	for (j = 0; j < 32; j++) {
		u32 mask = 0x1 << j;
		if ((*bitmap & mask) == 0) {
			/* Mark it used */
			*bitmap |= mask;
			break;
		}
	}
	
	offset = i * 32 * 4;
	offset += j * 4;

	*addr = xhci->align_pool->dma + offset;
	return (char *)(xhci->align_pool->buf) + offset;
}

dma_addr_t xhci_align_map_4bytes(struct xhci_hcd *xhci, void *buf)
{
	dma_addr_t addr;
	char *buf0 = xhci_alloc_4bytes(xhci, &addr);
	char *buf1 = (char *)buf;

	if (!buf0)
		return 0;

	/* Copy unaligned data to the aligned buffer */
	buf0[0] = buf1[0];	
	buf0[1] = buf1[1];	
	buf0[2] = buf1[2];
	buf0[3] = buf1[3];	/* unnecessary */
	
	UBI_DMA_FLUSH(buf0, 4);
	return addr;
}

void xhci_align_unmap_4bytes(struct xhci_hcd *xhci, dma_addr_t addr)
{
	u32 offset, i, j;

	if ((addr < xhci->align_pool->dma) || (addr >= xhci->align_pool->dma + XHCI_4BYTES_POOL_SIZE)) {
		printk(KERN_ERR "addr %ld out of range\n", addr);
		return;
	}

	offset = (addr - xhci->align_pool->dma) >> 2;
	j = offset & (32 - 1);
	i = (offset - j) >> 5;
	xhci->align_pool->bitmap[i] &= ~(1 << j);
}

static int ubi32_xhci_drv_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	int ret;

	if (usb_disabled())
		return -ENODEV;
#if 0

	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENOMEM;
	}
	// funny that we have two host controllers

	hcd = usb_create_hcd(&ubi32_xhc_driver, &pdev->dev, "ubi32");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_debug("request_mem_region failed");
		ret = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		pr_debug("ioremap failed");
		ret = -ENOMEM;
		goto err2;
	}

	ubi32_start_xhc();

	xhci = hcd_to_xhci(hcd);
	xhci->caps = hcd->regs;
	xhci->regs = hcd->regs + HC_LENGTH(readl(&ehci->caps->hc_capbase));
	/* cache this readonly data; minimize chip reads */
	xhci->hcs_params = readl(&ehci->caps->hcs_params);

	ret = usb_add_hcd(hcd, pdev->resource[1].start,
			  IRQF_DISABLED | IRQF_SHARED);
	if (ret == 0) {
		platform_set_drvdata(pdev, hcd);
		return ret;
	}

	ubi32_stop_xhc();
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return ret;
#endif
	return 0;
}

static int ubi32_xhci_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	//ubi32_stop_xhc();
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int ubi32_xhci_drv_suspend(struct platform_device *pdev,
					pm_message_t message)
{
	/* FIXME */
	return 0;
}

static int ubi32_xhci_drv_resume(struct platform_device *pdev)
{
	/* FIXME */
	return 0;
}
#else
#define ubi32_xhci_drv_suspend NULL
#define ubi32_xhci_drv_resume NULL
#endif

MODULE_ALIAS("platform:fsl-ehci");

static struct platform_driver ubi32_xhci_driver = {
	.probe = ubi32_xhci_drv_probe,
	.remove = ubi32_xhci_drv_remove,
	.shutdown = usb_hcd_platform_shutdown,
	.suspend	= ubi32_xhci_drv_suspend,
	.resume		= ubi32_xhci_drv_resume,
	.driver = {
		   .name = "ubi32-xhci",
	},
};
