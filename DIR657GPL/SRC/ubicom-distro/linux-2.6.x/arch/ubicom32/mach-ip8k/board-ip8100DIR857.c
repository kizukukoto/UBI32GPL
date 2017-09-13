/*
 * arch/ubicom32/mach-ip8k/board-ip8100DIR857.c
 *   Support for the IP8100 RouterGateway evaluation board.
 *
 * This file supports the following boards:
 *	8008-0110 Rev 1.0	IP8K RGW Board
 *
 *	8008-0111 Rev 1.1	IP8K RGW Board
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
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <linux/input.h>

#include <linux/spi/spi.h>
#include <asm/ubicom32-spi-gpio.h>
#include <asm/switch-dev.h>

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/pca953x.h>

#include <asm/board.h>
#include <asm/machdep.h>
#include <asm/ubicom32input.h>

#include <asm/flash.h>
#include <asm/ubi32-fc.h>
#include <asm/ubi32-nand-spi-er.h>

#include <asm/ubicom32sd.h>
#include <asm/sd_tio.h>

#ifdef CONFIG_SERIAL_UBI32_SERDES
#include <asm/ubicom32suart.h>
#endif



#ifdef CONFIG_SERIAL_UBI32_SERDES
static struct resource ip8100DIR857_ubicom32_suart_resources[] = {
	{
		.start	= RO,
		.end	= RO,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= RO_SERDES_RX_INT,
		.end	= RO_SERDES_INT),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= 250000000,
		.end	= 250000000,
		.flags	= UBICOM32_SUART_IORESOURCE_CLOCK,
	},
};

static struct platform_device ip8100DIR857_ubicom32_suart_device = {
	.name		= "ubicom32suart",
	.id		= -1,
	.num_resources  = ARRAY_SIZE(ip8100DIR857_ubicom32_suart_resources),
	.resource	= ip8100DIR857_ubicom32_suart_resources,
};
#endif

/*
 * Added Atheros Switch Driver
 */

static struct platform_device ip8100DIR857_ar8327_device = {
	.name		= "ar8327-smi",
	.id		= -1,
};

static struct platform_device *ip8100DIR857_devices[] __initdata = {

	&ip8100DIR857_ar8327_device,
};


/******************************************************************************
 * SD/IO Port D (Slot 0) platform data
 */
static struct resource ip8100DIR857_portd_sd_resources[] = {
	/*
	 * Send IRQ
	 */
	[0] = {
		/* 
		 * The init routine will query the devtree and fill this in
		 */
		.flags	= IORESOURCE_IRQ,
	},

	/*
	 * Receive IRQ
	 */
	[1] = {
		/* 
		 * The init routine will query the devtree and fill this in
		 */
		.flags	= IORESOURCE_IRQ,
	},

	/*
	 * Memory Mapped Registers
	 */
	[2] = {
		/* 
		 * The init routine will query the devtree and fill this in
		 */
		.flags	= IORESOURCE_MEM,
	},
};

static struct ubicom32sd_card ip8100DIR857_portd_sd_cards[] = {
	[0] = {
		.pin_wp		= GPIO_PG4_26,
		.wp_polarity	= 1,
		.pin_pwr	= GPIO_PG4_12,
		.pin_cd		= GPIO_PG4_27,
	},
};

static struct ubicom32sd_platform_data ip8100DIR857_portd_sd_platform_data = {
	.ncards		= 1,
	.cards		= ip8100DIR857_portd_sd_cards, 
};

static struct platform_device ip8100DIR857_portd_sd_device = {
	.name		= "ubicom32sd",
	.id		= 0,
	.resource	= ip8100DIR857_portd_sd_resources,
	.num_resources	= ARRAY_SIZE(ip8100DIR857_portd_sd_resources),
	.dev		= {
			.platform_data = &ip8100DIR857_portd_sd_platform_data,
	},

};
/*
 * ip8100DIR857_portd_sd_init
 */
static void ip8100DIR857_portd_sd_init(void)
{
	/*
	 * Check the device tree for the sd_tio
	 */
	struct sd_tio_node *sd_node = (struct sd_tio_node *)devtree_find_node("portd_sd");
	if (!sd_node) {
		printk(KERN_INFO "PortD SDTIO not found\n");
		return;
	}

	/*
	 * Fill in the resources and platform data from devtree information
	 */
	ip8100DIR857_portd_sd_resources[0].start = sd_node->dn.sendirq;
	ip8100DIR857_portd_sd_resources[1].start = sd_node->dn.recvirq;
	ip8100DIR857_portd_sd_resources[2].start = (u32_t)&(sd_node->regs);
	ip8100DIR857_portd_sd_resources[2].end = (u32_t)&(sd_node->regs) + sizeof(sd_node->regs);
	platform_device_register(&ip8100DIR857_portd_sd_device);
}

/*
 * ip8100DIR857_init
 *	Called to add the devices which we have on this board
 */
static int __init ip8100DIR857_init(void)
{
	if (ubicom32_flash_single_init()) {
		printk(KERN_ERR "%s: could not initialize flash controller\n", __FUNCTION__);
	}

	board_init();

	ubi_gpio_init();


	ip8100DIR857_portd_sd_init();

	printk(KERN_INFO "%s: registering device resources\n", __FUNCTION__);
	platform_add_devices(ip8100DIR857_devices, ARRAY_SIZE(ip8100DIR857_devices));

	printk(KERN_INFO "IP8100 \n");
	return 0;
}
arch_initcall(ip8100DIR857_init);

#ifdef CONFIG_PCI
/*
 * ubi32_pcie_reset_endpoint
 *      Do board specific GPIO operation to bring up a PCIe endpoint
 */
extern void __init __ubi32_pcie_reset_endpoint(int port, u32 reset_pg, u32 reset_pin, u32 clk_pg, u32 clk_pin);
void __init ubi32_pcie_reset_endpoint(int port)
{
	/*
	 * Current RGW board has only one reset GPIO pin assigned for both ports.
	 * This will be changed in next board design
	 */
	int reset_pg = PG4;
	int reset_pin = 9;
	int clk_pg = PG6;
	int clk_pin = port ? 1 : 0;

	/*
	 * Rev 1.1 boards have dedicated PCIe reset pin per slot
	 * slot 0: port PG3 and pin 20
	 * slot 1: port PG4 and pin 20
	 */
//	if ((strcmp(board_get_revision(), "1.1") == 0) && (port == 0)) {
//		reset_pg = PG3;
//		reset_pin = 20;
//	}
        printk(KERN_INFO "PCIe Reset \n");
        __ubi32_pcie_reset_endpoint(port, reset_pg, reset_pin, clk_pg, clk_pin);
}
#endif
