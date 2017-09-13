/*
 * arch/ubicom32/mach-ip7k/board-ip7160855MrgwAtheros.c
 *   Platform initialization for ip7160855Mrgw board.
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
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/delay.h>

#include <asm/board.h>
#include <asm/flash.h>

#include <linux/input.h>
#include <asm/ubicom32input.h>

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/pca953x.h>

#include <asm/ubicom32sd.h>
#include <asm/sd_tio.h>


#ifdef CONFIG_SERIAL_UBI32_SERDES
#include <asm/ubicom32suart.h>
#endif

#include <asm/switch-dev.h>

/*
 * LEDs
 *
 * WLAN1		PD0	(PWM capable)
 * WLAN2		PD1
 * USB2.0		PD2
 * Status		PD3
 * WPS			PD4
 *
 * TODO: check triggers, are they generic?
 */

static struct gpio_led ip7160rgw_gpio_leds[] = {
	{
		.name			= "d53:green:WLAN1",
		.default_trigger	= "WLAN1",
		.gpio			= GPIO_RD_0,
		.active_low		= 1,
	},
	{
		.name			= "d54:green:WLAN2",
		.default_trigger	= "WLAN2",
		.gpio			= GPIO_RD_1,
		.active_low		= 1,
	},
	{
		.name			= "d55:green:USB",
		.default_trigger	= "USB",
		.gpio			= GPIO_RD_2,
		.active_low		= 1,
	},
	{
		.name			= "d56:green:Status",
		.default_trigger	= "Status",
		.gpio			= GPIO_RD_3,
		.active_low		= 1,
	},
	{
		.name			= "d57:green:WPS",
		.default_trigger	= "WPS",
		.gpio			= GPIO_RD_4,
		.active_low		= 1,
	},
};

static struct gpio_led_platform_data ip7160rgw_gpio_led_platform_data = {
	.num_leds	= 5,
	.leds		= ip7160rgw_gpio_leds,
};

static struct platform_device ip7160rgw_gpio_leds_device = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev = {
		.platform_data = &ip7160rgw_gpio_led_platform_data,
	},
};

/*
 * Use ubicom32input driver to monitor the various pushbuttons on this board.
 *
 * WPS			PD5
 * FACT_DEFAULT		PD6
 *
 * TODO: pick some ubicom understood EV_xxx define for WPS and Fact Default
 */

static struct ubicom32input_button ip7160rgw_ubicom32input_buttons[] = {
	{
		.type		= EV_KEY,
		.code		= KEY_FN_F1,
		.gpio		= GPIO_RD_6,
		.desc		= "WPS",
		.active_low	= 1,
	},
//	{
//		.type		= EV_KEY,
//		.code		= KEY_FN_F2,
//		.gpio		= GPIO_RD_6,
//		.desc		= "Factory Default",
//		.active_low	= 1,
//	},
};

static struct ubicom32input_platform_data ip7160rgw_ubicom32input_data = {
	.buttons	= ip7160rgw_ubicom32input_buttons,
	.nbuttons	= ARRAY_SIZE(ip7160rgw_ubicom32input_buttons),
};

static struct platform_device ip7160rgw_ubicom32input_device = {
	.name	= "ubicom32input",
	.id	= -1,
	.dev	= {
		.platform_data = &ip7160rgw_ubicom32input_data,
	},
};

#ifdef CONFIG_SERIAL_UBI32_SERDES
static struct resource ip7160rgw_ubicom32_suart_resources[] = {
	{
		.start	= RE,
		.end	= RE,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= PORT_OTHER_INT(RE),
		.end	= PORT_OTHER_INT(RE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= 250000000,
		.end	= 250000000,
		.flags	= UBICOM32_SUART_IORESOURCE_CLOCK,
	},
};

static struct platform_device ip7160rgw_ubicom32_suart_device = {
	.name		= "ubicom32suart",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ip7160rgw_ubicom32_suart_resources),
	.resource	= ip7160rgw_ubicom32_suart_resources,
};
#endif

/*
 * Add RTL Switch
 */
static struct platform_device ip7160rgw_rtl8366_device = {
	.name		= "ar8316-smi",
	.id		= -1,
};

/*
 * List of all devices in our system
 */

static struct platform_device *ip7160rgw_devices[] __initdata = {
#ifdef CONFIG_SERIAL_UBI32_SERDES
	&ip7160rgw_ubicom32_suart_device,
#endif
	&ip7160rgw_ubicom32input_device,
//	&ip7160rgw_gpio_leds_device,
	&ip7160rgw_rtl8366_device,
};

/******************************************************************************
 * SD/IO Port F (Slot 1) platform data
 */
static struct resource ip7160rgw_portf_sd_resources[] = {
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

static struct ubicom32sd_card ip7160rgw_portf_sd_cards[] = {
	[0] = {
		.pin_wp		= GPIO_RF_9,
		.wp_polarity	= 1,
		.pin_pwr	= GPIO_RF_11,
		.pin_cd		= GPIO_RF_8,
	},
};

static struct ubicom32sd_platform_data ip7160rgw_portf_sd_platform_data = {
	.ncards		= 1,
	.cards		= ip7160rgw_portf_sd_cards, 
};

static struct platform_device ip7160rgw_portf_sd_device = {
	.name		= "ubicom32sd",
	.id		= 0,
	.resource	= ip7160rgw_portf_sd_resources,
	.num_resources	= ARRAY_SIZE(ip7160rgw_portf_sd_resources),
	.dev		= {
			.platform_data = &ip7160rgw_portf_sd_platform_data,
	},

};

/*
 * ip7160rgw_portf_sd_init
 */
static void ip7160rgw_portf_sd_init(void)
{
	/*
	 * Check the device tree for the sd_tio
	 */
	struct sd_tio_node *sd_node = (struct sd_tio_node *)devtree_find_node("portf_sd");
	if (!sd_node) {
		printk(KERN_INFO "PortF SDTIO not found\n");
		return;
	}

	/*
	 * For ip7160rgw, if we have Port F SDTIO, we need to reclaim RF_4 which
	 * is otherwise used as ethernet phy reset.
	 */
	UBICOM32_GPIO_DISABLE(GPIO_RF_4);

	/*
	 * Fill in the resources and platform data from devtree information
	 */
	ip7160rgw_portf_sd_resources[0].start = sd_node->dn.sendirq;
	ip7160rgw_portf_sd_resources[1].start = sd_node->dn.recvirq;
	ip7160rgw_portf_sd_resources[2].start = (u32_t)&(sd_node->regs);
	ip7160rgw_portf_sd_resources[2].end = (u32_t)&(sd_node->regs) + sizeof(sd_node->regs);

	platform_device_register(&ip7160rgw_portf_sd_device);
}


/*
 * ip7160rgw_init
 *	Called to add the devices which we have on this board
 */
static int __init ip7160rgw_init(void)
{
	if (ubicom32_flash_single_init()) {
		printk(KERN_ERR "%s: could not initialize flash controller\n", __FUNCTION__);
	}

	board_init();

	ubi_gpio_init();

	ip7160rgw_portf_sd_init();

	printk(KERN_INFO "%s: registering device resources\n", __FUNCTION__);
	platform_add_devices(ip7160rgw_devices, ARRAY_SIZE(ip7160rgw_devices));

	printk(KERN_INFO "%s: registering SPI resources\n", __FUNCTION__);

	return 0;
}

arch_initcall(ip7160rgw_init);
