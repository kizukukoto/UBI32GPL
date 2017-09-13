/*
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
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/ubicom32input.h>
#include "mdio.h"

#define MDIO_READ 1
#define MDIO_WRITE 0
#define MDIO_SETUP_TIME 10
#define MDIO_HOLD_TIME 10
#define MDIO_DELAY 250
#define MDIO_READ_DELAY 350

/*
* GPIO PINS
*/

#define MDIO 		GPIO_PG3_7
#define MDC 		GPIO_PG3_4
#define MDIO_RESET 	GPIO_PG4_14

/*
 * GPIO Bit Level Functions
 */

/* MDIO must already be configured as output. */
static void mdiobb_send_bit(int val)
{
	gpio_set_value(MDIO, val);
	ndelay(MDIO_DELAY);
	gpio_set_value(MDC, 1);
	ndelay(MDIO_DELAY);
	gpio_set_value(MDC, 0);
}

/* MDIO must already be configured as input. */
static int mdiobb_get_bit(void)
{
	int bit;

	ndelay(MDIO_DELAY);
	bit = gpio_get_value(MDIO);
	gpio_set_value(MDC, 1);
	ndelay(MDIO_READ_DELAY);
	gpio_set_value(MDC, 0);

	return bit;
}

/* MDIO must already be configured as output. */
static void mdiobb_send_num(u16 val, int bits)
{
	int i;

	for (i = bits - 1; i >= 0; i--)
		mdiobb_send_bit((val >> i) & 1);
}

/* MDIO must already be configured as input. */
static u16 mdiobb_get_num(int bits)
{
	int i;
	u16 ret = 0;

	for (i = bits - 1; i >= 0; i--) {
		ret <<= 1;
		ret |= mdiobb_get_bit();
	}

	return ret;
}

/* Utility to send the preamble, address, and
 * register (common to read and write).
 */
static void mdiobb_cmd(int read, u8 phy, u8 reg)
{
	int i;

	gpio_direction_output(MDIO, 1);

	/*
	 * Send a 32 bit preamble ('1's) with an extra '1' bit for good
	 * measure.  The IEEE spec says this is a PHY optional
	 * requirement.  The AMD 79C874 requires one after power up and
	 * one after a MII communications error.  This means that we are
	 * doing more preambles than we need, but it is safer and will be
	 * much more robust.
	 */

	for (i = 0; i < 32; i++)
		mdiobb_send_bit(1);

	/* send the start bit (01) and the read opcode (10) or write (10) */
	mdiobb_send_bit(0);
	mdiobb_send_bit(1);
	mdiobb_send_bit(read);
	mdiobb_send_bit(!read);

	mdiobb_send_num(phy, 5);
	mdiobb_send_num(reg, 5);
}


u32 mdiobb_read(int phy_base, int phy, int reg)
{
	u32 ret;

	/*
	* Get Right Phy Address
	*/

	phy = phy + phy_base;

	mdiobb_cmd(MDIO_READ, phy, reg);

	gpio_direction_input(MDIO);
	mdiobb_get_bit();

	/* check the turnaround bit: the PHY should be driving it to zero */
	if (mdiobb_get_bit() != 0) {
		//printk(KERN_WARNING "MDIO BAD READ FFFF from PHY(0x%x)/REG(0x%x)\n", phy, reg);
	}

	ret = mdiobb_get_num(16);

	return ret;
}

int mdiobb_write(int phy_base, int phy, int reg, u16 val)
{
	/*
	* Get Right Phy Address
	*/

	phy = phy + phy_base;

	mdiobb_cmd(MDIO_WRITE, phy, reg);

	/* send the turnaround (10) */
	mdiobb_send_bit(1);
	mdiobb_send_bit(0);

	mdiobb_send_num(val, 16);

	gpio_direction_input(MDIO);

	return 0;
}

int mdio_reset(void)
{
#ifdef MDIO_RESET
	int ret;

	/*
	* Request the Reset GPIO
	*/
	printk(KERN_WARNING "MDIO/MDC Reset\n");

	ret = gpio_request(MDIO_RESET, "switch-AR8316-RST");

	if (ret) {
		printk(KERN_WARNING "Could not request RST\n");
		//goto fail;
	}

	gpio_set_value(MDIO_RESET, 1);
	gpio_direction_output(MDIO_RESET,1);
	mdelay(20);
	gpio_set_value(MDIO_RESET, 0);
	mdelay(10);
	gpio_set_value(MDIO_RESET, 1);
	mdelay(100);

	/* keep GPIO pin held high throughout operation */
#endif
	return SUCCESS;
fail:
	return FAILED;
}


int mdio_init(void)
{
	int ret;

	/*
	 * Request the GPIO's and set initial direction and value
	 *	MDC: set to output low when idle.
	 *	MDIO: set to input when idle.
	 */
	ret = gpio_request(MDC, "switch-AR8316-MDC");
	if (ret) {
		printk(KERN_WARNING "Could not request MDC\n");
		//goto fail;
	}
	gpio_direction_output(MDC, 0);

	ret = gpio_request(MDIO, "switch-AR8316-MDIO");
	if (ret) {
		printk(KERN_WARNING "Could not request MDIO\n");
		//goto fail;
	}
	gpio_direction_input(MDIO);

	return SUCCESS;
fail:
	return FAILED;
}
