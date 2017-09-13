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
		.start	= IO_PORT_RO,
		.end	= IO_PORT_RO,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= RO_SERDES_RX_INT,
		.end	= RO_SERDES_INT,
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
* Added WPS Button
*/
static struct ubicom32input_button ip8100DIR857_ubicom32input_buttons[] = {
       {
               .type           = EV_KEY,
               .code           = KEY_FN_F1,
               .gpio           = GPIO_PG4_30,
               .desc           = "WPS",
               .active_low     = 1,
       },
};

static struct ubicom32input_platform_data ip8100DIR857_ubicom32input_data = {
       .buttons       = ip8100DIR857_ubicom32input_buttons,
       .nbuttons      = ARRAY_SIZE(ip8100DIR857_ubicom32input_buttons),
};

static struct platform_device ip8100DIR857_ubicom32input_device = {
       .name   = "ubicom32input",
       .id     = -1,
       .dev    = {
       .platform_data = &ip8100DIR857_ubicom32input_data,
       },
};

/*
 * Added Atheros Switch Driver
 */

static struct platform_device ip8100DIR857_ar8327_device = {
	.name		= "ar8327-smi",
	.id		= -1,
};

static struct platform_device *ip8100DIR857_devices[] __initdata = {

	&ip8100DIR857_ar8327_device,
	&ip8100DIR857_ubicom32input_device,	
};


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

	printk(KERN_INFO "%s: registering device resources\n", __FUNCTION__);
	platform_add_devices(ip8100DIR857_devices, ARRAY_SIZE(ip8100DIR857_devices));

	printk(KERN_INFO "IP8100 \n");
	return 0;
}
arch_initcall(ip8100DIR857_init);
