/*
 * arch/ubicom32/mach-ip8k/board-ip8100rgw.c
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

#ifdef CONFIG_SERIAL_UBI32_SERDES
#include <asm/ubicom32suart.h>
#endif

/*****************************************************************************
 * SPI bus over GPIO for Gigabit Ethernet Switch
 *	U58:
 *		MOSI	PG3_12
 *		MISO	PG3_15
 *		CLK	PG3_23
 *		CS	PG3_31
 *		RESET	PG3_28
 */
static struct ubicom32_spi_gpio_platform_data ip8100rgw_spi_gpio_data = {
	.pin_mosi       = GPIO_PG3_12,
	.pin_miso       = GPIO_PG3_15,
	.pin_clk        = GPIO_PG3_23,
	.bus_num        = 0,            // We'll call this SPI bus 0
	.num_chipselect = 1,            // only one device on this SPI bus
	.clk_default    = 1,
};

static struct platform_device ip8100rgw_spi_gpio_device = {
	.name   = "ubicom32-spi-gpio",
	.id     = 0,
	.dev    = {
		.platform_data = &ip8100rgw_spi_gpio_data,
	},
};

static struct ubicom32_spi_gpio_controller_data ip8100rgw_bcm539x_controller_data = {
	.pin_cs = GPIO_PG3_31,
};

static struct switch_core_platform_data ip8100rgw_bcm539x_platform_data = {
	.flags          = SWITCH_DEV_FLAG_HW_RESET,
	.pin_reset      = GPIO_PG3_28,
	.name           = "bcm539x",
};

static struct spi_board_info ip8100rgw_spi_board_info[] = {
	{
		.modalias               = "bcm539x-spi",
		.bus_num                = 0,
		.chip_select            = 0,
		.max_speed_hz           = 2000000,
		.platform_data          = &ip8100rgw_bcm539x_platform_data,
		.controller_data        = &ip8100rgw_bcm539x_controller_data,
		.mode                   = SPI_MODE_3,
	}
};

/*****************************************************************************
 * Use ubicom32input driver to monitor the various pushbuttons on this board.
 */
static struct ubicom32input_button ip8100rgw_ubicom32input_buttons[] = {
	{
		.type           = EV_KEY,
		.code           = KEY_FN_F1,
		.gpio           = GPIO_PG5_1,
		.desc           = "WPS",
		.active_low     = 1,
	},
};

static struct ubicom32input_platform_data ip8100rgw_ubicom32input_data = {
	.buttons	= ip8100rgw_ubicom32input_buttons,
	.nbuttons	= ARRAY_SIZE(ip8100rgw_ubicom32input_buttons),
};

static struct platform_device ip8100rgw_ubicom32input_device = {
	.name	= "ubicom32input",
	.id	= -1,
	.dev	= {
		.platform_data = &ip8100rgw_ubicom32input_data,
	},
};

#ifdef CONFIG_SERIAL_UBI32_SERDES
static struct resource ip8100rgw_ubicom32_suart_resources[] = {
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

static struct platform_device ip8100rgw_ubicom32_suart_device = {
	.name		= "ubicom32suart",
	.id		= -1,
	.num_resources  = ARRAY_SIZE(ip8100rgw_ubicom32_suart_resources),
	.resource	= ip8100rgw_ubicom32_suart_resources,
};
#endif

/*****************************************************************************
 * Additional GPIO chips
 */
static struct pca953x_platform_data ip8100rgw_gpio_u16_platform_data = {
	.gpio_base = IP8100RGW_U16_BASE,
};

/******************************************************************************
 * Devices on the I2C bus
 *
 * BEWARE of changing the order of things in this array as we depend on
 * certain things to be in certain places.
 */
static struct i2c_board_info __initdata ip8100rgw_i2c_board_info[] = {
	/*
	 * U16, MAX7310 IO expander, 8 bits, address 0x19
	 *      IO0: D18       IO4: SW4
	 *      IO1: D19       IO5: SW5
	 *      IO2: D20       IO6: SW6
	 *      IO3: D21       IO7: SW7
	 * see gpio-ip8100rgw.h for more information
	 */
	{
	.type           = "max7310",
	.addr           = 0x19,
	.platform_data  = &ip8100rgw_gpio_u16_platform_data,
	},
};


/******************************************************************************
 * I2C bus on the board, SDA G4P17, SCL G4P16
 */
static struct i2c_gpio_platform_data ip8100rgw_i2c_data = {
	.sda_pin                = GPIO_PG4_17,
	.scl_pin                = GPIO_PG4_16,
	.sda_is_open_drain      = 0,
	.scl_is_open_drain      = 0,
	.udelay                 = 50,
};

static struct platform_device ip8100rgw_i2c_device = {
	.name   = "i2c-gpio",
	.id     = 0,
	.dev    = {
		.platform_data = &ip8100rgw_i2c_data,
	},
};

/*
 * List of all devices in our system
 */
static struct platform_device *ip8100rgw_devices[] __initdata = {
#ifdef CONFIG_SERIAL_UBI32_SERDES
	&ip8100rgw_ubicom32_suart_device,
#endif
	&ip8100rgw_ubicom32input_device,
	&ip8100rgw_spi_gpio_device,
	&ip8100rgw_i2c_device,
};


/*
 * ip8100rgw_nand_select
 */
static void ip8100rgw_nand_select(void *appdata)
{
	gpio_set_value(GPIO_PG5_2, 1);
}

/*
 * ip8100rgw_nor_select
 */
static void ip8100rgw_nor_select(void *appdata)
{
	gpio_set_value(GPIO_PG5_2, 0);
}

/*
 * LEDs
 *
 * WLAN1                IO0     
 * WLAN2                IO1
 * USB2.0               IO2
 * Status               IO3
 *
 */
static struct gpio_led ip8100rgw_gpio_leds[] = {
        {
                .name                   = "d18:green:WLAN1",
                .default_trigger        = "WLAN1",
                .gpio                   = IP8100RGW_IO0,
                .active_low             = 1,
        },
        {
                .name                   = "d19:green:WLAN2",
                .default_trigger        = "WLAN2",
                .gpio                   = IP8100RGW_IO1,
                .active_low             = 1,
        },
        {
                .name                   = "d20:green:USB",
                .default_trigger        = "USB",
                .gpio                   = IP8100RGW_IO2,
                .active_low             = 1,
        },
        {
                .name                   = "d21:green:Status",
                .default_trigger        = "Status",
                .gpio                   = IP8100RGW_IO3,
                .active_low             = 1,
        },
/*      {
                .name                   = "d57:green:WPS",
                .default_trigger        = "WPS",
                .gpio                   = GPIO_RD_4,
                .active_low             = 1,
        },
*/
};

static struct gpio_led_platform_data ip8100rgw_gpio_led_platform_data = {
        .num_leds       = 4,
        .leds           = ip8100rgw_gpio_leds,
};

static struct platform_device ip8100rgw_gpio_leds_device = {
        .name           = "leds-gpio",
        .id             = -1,
        .dev = {
                .platform_data = &ip8100rgw_gpio_led_platform_data,
        },
};


int __init ip8100rgw_late_init(void)
{
	return platform_device_register(&ip8100rgw_gpio_leds_device);
}
late_initcall(ip8100rgw_late_init);


/*
 * ip8100rgw_init
 *	Called to add the devices which we have on this board
 */
static int __init ip8100rgw_init(void)
{
	int ret;
	void *tmp;

	board_init();

	ubi_gpio_init();

	/*
	 * Flash storage devices
	 *      NAND selected when PG5[2] is HIGH
	 *      NOR selected when PG5[2] is LOW
	 */
	ubicom32_flash_init();
	ret = gpio_request(GPIO_PG5_2, "FLASH SEL");
	if (ret) {
		printk(KERN_ERR "%s: Could not request FLASH_SEL\n", __FUNCTION__);
	}
	gpio_direction_output(GPIO_PG5_2, 0);

	tmp = ubicom32_flash_alloc(ip8100rgw_nor_select, NULL, NULL);
	if (!tmp) {
		printk(KERN_ERR "%s: Could not alloc NOR flash\n", __FUNCTION__);
	} else {
		struct ubicom32fc_platform_data pd;
		memset(&pd, 0, sizeof(pd));
		pd.select = ubicom32_flash_select;
		pd.unselect = ubicom32_flash_unselect;
		pd.appdata = tmp;
		ret = ubicom32_flash_add("ubicom32fc", 0, &pd, sizeof(pd));
		if (ret) {
			printk(KERN_ERR "%s: Could not add NOR flash\n", __FUNCTION__);
		}
	}

	tmp = ubicom32_flash_alloc(ip8100rgw_nand_select, NULL, NULL);
	if (!tmp) {
		printk(KERN_ERR "%s: Could not alloc NAND flash\n", __FUNCTION__);
	} else {
		struct ubi32_nand_spi_er_platform_data pd;
		memset(&pd, 0, sizeof(pd));
		pd.select = ubicom32_flash_select;
		pd.unselect = ubicom32_flash_unselect;
		pd.appdata = tmp;
		ret = ubicom32_flash_add("ubi32-nand-spi-er", 0, &pd, sizeof(pd));
		if (ret) {
			printk(KERN_ERR "%s: Could not add NAND flash\n", __FUNCTION__);
		}
	}

	/*
	 * Reserve switch SPI CS on behalf on switch driver
	 */
	if (gpio_request(ip8100rgw_bcm539x_controller_data.pin_cs, "switch-bcm539x-cs")) {
		printk(KERN_WARNING "Could not request cs of switch SPI I/F\n");
		return -EIO;
	}
	gpio_direction_output(ip8100rgw_bcm539x_controller_data.pin_cs, 1);

	printk(KERN_INFO "%s: registering device resources\n", __FUNCTION__);
	platform_add_devices(ip8100rgw_devices, ARRAY_SIZE(ip8100rgw_devices));

	printk(KERN_INFO "%s: registering SPI resources\n", __FUNCTION__);
	spi_register_board_info(ip8100rgw_spi_board_info, ARRAY_SIZE(ip8100rgw_spi_board_info));

	printk(KERN_INFO "%s: registering i2c resources\n", __FUNCTION__);
	i2c_register_board_info(0, ip8100rgw_i2c_board_info, ARRAY_SIZE(ip8100rgw_i2c_board_info));


	printk(KERN_INFO "IP8100 RGW\n");
	return 0;
}
arch_initcall(ip8100rgw_init);

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
	int reset_pin = 20;
	int clk_pg = PG6;
	int clk_pin = port ? 1 : 0;

	/*
	 * Rev 1.1 boards have dedicated PCIe reset pin per slot
	 * slot 0: port PG3 and pin 20
	 * slot 1: port PG4 and pin 20
	 */
	if ((strcmp(board_get_revision(), "1.1") == 0) && (port == 0)) {
		reset_pg = PG3;
		reset_pin = 20;
	}

        __ubi32_pcie_reset_endpoint(port, reset_pg, reset_pin, clk_pg, clk_pin);
}
#endif
