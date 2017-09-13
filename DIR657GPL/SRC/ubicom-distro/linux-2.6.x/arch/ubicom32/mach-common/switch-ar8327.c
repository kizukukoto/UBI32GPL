/*
 * arch/ubicom32/mach-common/switch-ar8327.c
 *   Atheros switch driver, MDC/MDIO mode
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

#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/mii.h>

#include <asm/switch-dev.h>
#include "switch-core.h"
#include "AR8327/athrs17_phy.h"
#include "AR8327/mdio.h"

#define DRIVER_NAME "ar8327-smi"
#define DRIVER_VERSION "1.0"

#undef AR8327_DEBUG
#define AR8327_MAX_PORT_ID 6
#define AR8327_PORT_MASK ((1 << AR8327_MAX_PORT_ID) - 1)
#define AR8327_CPU_PORT_ID 0
#define AR8327_PORT_TO_PHY(p) (p-1)	// PHY ID = PORT ID -1

struct ar8327_data {
	struct switch_device			*switch_dev;

	/*
	 * Our private data
	 */
	u32_t					reg_addr;

	/*
	 * AR8327 Device ID
	 */
	u32_t					device_id;
};

/*
 * ar8327_handle_reset
 */
static int ar8327_handle_reset(struct switch_device *dev, char *buf, int inst)
{
	/*
	 * SW reset.
	 */
	printk("ar8327 switch reset\n");
	athrs17_reg_write(0x0000, athrs17_reg_read(0x0000) | 0x80000000);
	udelay(1000);
	BUG_ON(athrs17_reg_read(0x0000) & 0x80000000);

	return 0;
}

/*
 * ar8327_handle_vlan_ports_read
 */
static int ar8327_handle_vlan_ports_read(struct switch_device *dev,
					  char *buf, int inst)
{
	//struct ar8327_data *bd =
	//	(struct ar8327_data *)switch_get_drvdata(dev);

	printk(KERN_WARNING "Not Implemented - ar8327_handle_vlan_ports_read\n");

	return 0;
}

/*
 * ar8327_handle_vlan_ports_write
 */
static int ar8327_handle_vlan_ports_write(struct switch_device *dev,
					   char *buf, int inst)
{
	//struct ar8327_data *bd =
	//	(struct ar8327_data *)switch_get_drvdata(dev);
	u32_t untag;
	u32_t ports;
	u32_t def;

	switch_parse_vlan_ports(dev, buf, &untag, &ports, &def);

	printk(KERN_WARNING "Not Implemented - ar8327_handle_vlan_ports_write\n");
	printk(KERN_DEBUG "'%s' inst=%d untag=%08x ports=%08x def=%08x\n",
		buf, inst, untag, ports, def);
	
	return 0;
}

/*
 * Handlers for <this_driver>/vlan/<vlan_id>
 */
static const struct switch_handler ar8327_switch_handlers_vlan_dir[] = {
	{
		.name	= "ports",
		.read	= ar8327_handle_vlan_ports_read,
		.write	= ar8327_handle_vlan_ports_write,
	},
	{
	},
};

/*
 * ar8327_handle_vlan_delete_write
 */
static int ar8327_handle_vlan_delete_write(struct switch_device *dev,
					    char *buf, int inst)
{
	//struct ar8327_data *bd =
	//	(struct ar8327_data *)switch_get_drvdata(dev);
	int vid;

	vid = simple_strtoul(buf, NULL, 0);
	if (!vid) {
		return -EINVAL;
	}

	printk(KERN_WARNING "Not Implemented - ar8327_handle_vlan_delete_write %d\n", vid);

	return switch_remove_vlan_dir(dev, vid);
}

/*
 * ar8327_handle_vlan_create_write
 */
static int ar8327_handle_vlan_create_write(struct switch_device *dev,
					    char *buf, int inst)
{
	int vid;

	vid = simple_strtoul(buf, NULL, 0);
	if (!vid) {
		return -EINVAL;
	}

	return switch_create_vlan_dir(dev, vid,
				      ar8327_switch_handlers_vlan_dir);
}

/*
 * ar8327_handle_enable_read
 */
static int ar8327_handle_enable_read(struct switch_device *dev,
				      char *buf, int inst)
{
	//struct ar8327_data *bd =
	//	(struct ar8327_data *)switch_get_drvdata(dev);

	printk(KERN_WARNING "Not Implemented - ar8327_handle_enable_read\n");

	return sprintf(buf, "%d\n", 1);
}

/*
 * ar8327_handle_enable_write
 */
static int ar8327_handle_enable_write(struct switch_device *dev,
				       char *buf, int inst)
{
	//struct ar8327_data *bd =
	//	(struct ar8327_data *)switch_get_drvdata(dev);

	printk(KERN_WARNING "Not Implemented - ar8327_handle_enable_write\n");
	return 0;
}

/*
 * ar8327_handle_reg_addr_write
 */
static int ar8327_handle_reg_addr_write(struct switch_device *dev,
					char *buf, int inst)
{
	struct ar8327_data *bd = (struct ar8327_data *)switch_get_drvdata(dev);

	bd->reg_addr = simple_strtoul(buf, NULL, 0);
	return 0;
}

/*
 * ar8327_handle_reg_addr_read
 */
static int ar8327_handle_reg_addr_read(struct switch_device *dev,
					char *buf, int inst)
{
	struct ar8327_data *bd = (struct ar8327_data *)switch_get_drvdata(dev);

	return sprintf(buf, "0x%08x\n", bd->reg_addr);
}

/*
 * ar8327_handle_reg_write
 */
static int ar8327_handle_reg_write(struct switch_device *dev,
					char *buf, int inst)
{
	struct ar8327_data *bd = (struct ar8327_data *)switch_get_drvdata(dev);

        athrs17_reg_write(bd->reg_addr, simple_strtoul(buf, NULL, 0));

	return 0;
}

/*
 * ar8327_handle_reg_read
 */
static int ar8327_handle_reg_read(struct switch_device *dev,
					char *buf, int inst)
{
	struct ar8327_data *bd = (struct ar8327_data *)switch_get_drvdata(dev);

	return sprintf(buf, "0x%08x\n", athrs17_reg_read(bd->reg_addr));
}

/*
 * ar8327_handle_enable_vlan_read
 */
static int ar8327_handle_enable_vlan_read(struct switch_device *dev,
					   char *buf, int inst)
{
	//struct ar8327_data *bd =
	//	(struct ar8327_data *)switch_get_drvdata(dev);

	printk(KERN_WARNING "Not Implemented - ar8327_handle_enable_vlan_read\n");
	return sprintf(buf, "%d\n", 1);
}

/*
 * ar8327_handle_enable_vlan_write
 */
static int ar8327_handle_enable_vlan_write(struct switch_device *dev,
					    char *buf, int inst)
{
	//struct ar8327_data *bd =
	//	(struct ar8327_data *)switch_get_drvdata(dev);

	printk(KERN_WARNING "Not Implemented - ar8327_handle_enable_vlan_read\n");

	return 0;
}

/*
 * ar8327_handle_port_enable_read
 */
static int ar8327_handle_port_enable_read(struct switch_device *dev,
					   char *buf, int inst)
{
	return sprintf(buf, "%d\n", 1);
}

/*
 * ar8327_handle_port_enable_write
 */
static int ar8327_handle_port_enable_write(struct switch_device *dev,
					    char *buf, int inst)
{
	/*
	 * validate port
	 */
	if (!(dev->port_mask[0] & (1 << inst))) {
		return -EIO;
	}

	if (buf[0] != '1') {
		printk(KERN_WARNING "switch port[%d] disabling is not supported\n", inst);
	}
	return 0;
}

/*
 * ar8327_handle_port_state_read
 */
static int ar8327_handle_port_state_read(struct switch_device *dev,
					   char *buf, int inst)
{
	u16_t link;

	/*
	 * validate port
	 */
	if (!(dev->port_mask[0] & (1 << inst))) {
		return -EIO;
	}

	/*
	 * check PHY link state - CPU port (port 8) is always up
	 */
	if (inst == AR8327_CPU_PORT_ID) {
		return sprintf(buf, "%d\n", 1);
	}
	link = phy_reg_read(0, AR8327_PORT_TO_PHY(inst), 0x11);

	return sprintf(buf, "%d\n", ((link & (3 << 10)) == (3 << 10)) ? 1 : 0);
}

/*
 * ar8327_handle_port_media_read
 */
static int ar8327_handle_port_media_read(struct switch_device *dev,
					   char *buf, int inst)
{
	u16_t link, duplex;
	u32_t speed;

	/*
	 * validate port
	 */
	if (!(dev->port_mask[0] & (1 << inst))) {
		return -EIO;
	}

	/*
	 * check PHY link state first - CPU port (port 8) is always up
	 * (reading Atheros specific PHY status register)
	 */
	if (inst == AR8327_CPU_PORT_ID) {
		return sprintf(buf, "1000FD\n");
	}

	link = phy_reg_read(0, AR8327_PORT_TO_PHY(inst), 0x11);
	if ((link & (3 << 10)) != (3 << 10)) {
		return sprintf(buf, "UNKNOWN\n");
	}

	/*
	 * get link speeda dn duplex - CPU port (port 8) is 1000/full
	 */
	speed = (link >> 14) & 3;
	duplex = (link >> 13) & 1;

	return sprintf(buf, "%d%cD\n",
		(speed == 0) ? 10 : ((speed == 1) ? 100 : 1000),
		duplex ? 'F' : 'H');
}

/*
 * ar8327_handle_port_meida_write
 */
static int ar8327_handle_port_meida_write(struct switch_device *dev,
					    char *buf, int inst)
{
	u16_t ctrl_word, local_cap, local_giga_cap;

	/*
	 * validate port (not for CPU port)
	 */
	if (!(dev->port_mask[0] & (1 << inst) & ~(1 << 0))) {
		return -EIO;
	}

	/*
	 * Get the maximum capability from status
	 */
	local_cap = phy_reg_read(0, AR8327_PORT_TO_PHY(inst), MII_ADVERTISE);
	local_giga_cap = phy_reg_read(0, AR8327_PORT_TO_PHY(inst), MII_CTRL1000);

	/* Configure to the requested speed */
	if (strncmp(buf, "1000FD", 6) == 0) {
		/* speed */
		local_cap &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL);
		local_cap &= ~(ADVERTISE_100HALF | ADVERTISE_100FULL);
		local_giga_cap |= (ADVERTISE_1000HALF | ADVERTISE_1000FULL);
		/* duplex */
	} else if (strncmp(buf, "100FD", 5) == 0) {
		/* speed */
		local_cap &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL);
		local_cap |= (ADVERTISE_100HALF | ADVERTISE_100FULL);
		local_giga_cap &= ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
		/* duplex */
		local_cap &= ~(ADVERTISE_100HALF);
	} else if (strncmp(buf, "100HD", 5) == 0) {
		/* speed */
		local_cap &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL);
		local_cap |= (ADVERTISE_100HALF | ADVERTISE_100FULL);
		local_giga_cap &= ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
		/* duplex */
		local_cap &= ~(ADVERTISE_100FULL);
	} else if (strncmp(buf, "10FD", 4) == 0) {
		/* speed */
		local_cap |= (ADVERTISE_10HALF | ADVERTISE_10FULL);
		local_cap &= ~(ADVERTISE_100HALF | ADVERTISE_100FULL);
		local_giga_cap &= ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
		/* duplex */
		local_cap &= ~(ADVERTISE_10HALF);
	} else if (strncmp(buf, "10HD", 4) == 0) {
		/* speed */
		local_cap |= (ADVERTISE_10HALF | ADVERTISE_10FULL);
		local_cap &= ~(ADVERTISE_100HALF | ADVERTISE_100FULL);
		local_giga_cap &= ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
		/* duplex */
		local_cap &= ~(ADVERTISE_10FULL);
	} else if (strncmp(buf, "AUTO", 4) == 0) {
		/* speed */
		local_cap |= (ADVERTISE_10HALF | ADVERTISE_10FULL);
		local_cap |= (ADVERTISE_100HALF | ADVERTISE_100FULL);
		local_giga_cap |= (ADVERTISE_1000HALF | ADVERTISE_1000FULL);
	} else {
		return -EINVAL;
	}

	/* Active PHY with the requested speed for auto-negotiation */
	phy_reg_write(0, AR8327_PORT_TO_PHY(inst), MII_ADVERTISE, local_cap);
	phy_reg_write(0, AR8327_PORT_TO_PHY(inst), MII_CTRL1000, local_giga_cap);

	ctrl_word = phy_reg_read(0, AR8327_PORT_TO_PHY(inst), MII_BMCR);
	ctrl_word |= (BMCR_ANENABLE | BMCR_ANRESTART);
	phy_reg_write(0, AR8327_PORT_TO_PHY(inst), MII_BMCR, ctrl_word);

	return 0;
}

/*
 * proc_fs entries for this switch
 */
static const struct switch_handler ar8327_switch_handlers[] = {
	{
		.name	= "enable",
		.read	= ar8327_handle_enable_read,
		.write	= ar8327_handle_enable_write,
	},
	{
		.name	= "enable_vlan",
		.read	= ar8327_handle_enable_vlan_read,
		.write	= ar8327_handle_enable_vlan_write,
	},
	{
		.name	= "reset",
		.write	= ar8327_handle_reset,
	},
	{
	},
};

/*
 * Handlers for <this_driver>/reg
 */
static const struct switch_handler ar8327_switch_handlers_reg[] = {
	{
		.name   = "addr",
		.read   = ar8327_handle_reg_addr_read,
		.write  = ar8327_handle_reg_addr_write,
	},
	{
		.name   = "val",
		.read   = ar8327_handle_reg_read,
		.write  = ar8327_handle_reg_write,
	},
	{
	}
};

/*
 * Handlers for <this_driver>/vlan
 */
static const struct switch_handler ar8327_switch_handlers_vlan[] = {
	{
		.name	= "delete",
		.write	= ar8327_handle_vlan_delete_write,
	},
	{
		.name	= "create",
		.write	= ar8327_handle_vlan_create_write,
	},
	{
	},
};

/*
 * Handlers for <this_driver>/port/<port number>
 */
static const struct switch_handler ar8327_switch_handlers_port[] = {
	{
		.name	= "enable",
		.read	= ar8327_handle_port_enable_read,
		.write	= ar8327_handle_port_enable_write,
	},
	{
		.name	= "state",
		.read	= ar8327_handle_port_state_read,
	},
	{
		.name	= "media",
		.read	= ar8327_handle_port_media_read,
		.write	= ar8327_handle_port_meida_write,
	},
	{
	},
};

/*
 * ar8327_probe
 */
static int __devinit ar8327_probe(struct platform_device *pdev) 
{
	struct ar8327_data *bd;
	struct switch_device *switch_dev = NULL;
	int ret;

	mdio_init();
	mdio_reset();
	athrs17_reg_init();

	/*
	 * Allocate our private data structure
	 */
	bd = kzalloc(sizeof(struct ar8327_data), GFP_KERNEL);
	if (!bd) {
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, bd);
	bd->device_id = (phy_reg_read(0, 0, MII_PHYSID1) << 16)
			| phy_reg_read(0, 0, MII_PHYSID2);

	/*
	 * Setup the switch driver structure
	 */
	switch_dev = switch_alloc();
	if (!switch_dev) {
		ret = -ENOMEM;
		goto fail;
	}
	switch_dev->name = pdev->name;

	switch_dev->ports = AR8327_MAX_PORT_ID;
	switch_dev->port_mask[0] = AR8327_PORT_MASK;
	switch_dev->driver_handlers = ar8327_switch_handlers;
	switch_dev->reg_handlers = ar8327_switch_handlers_reg;
	switch_dev->vlan_handlers = ar8327_switch_handlers_vlan;
	switch_dev->port_handlers = ar8327_switch_handlers_port;

	bd->switch_dev = switch_dev;
	switch_set_drvdata(switch_dev, (void *)bd);

	ret = switch_register(bd->switch_dev);
	if (ret < 0) {
		goto fail;
	}

	printk(KERN_INFO "ar8327 (ID=0x%08x) switch chip initialized\n", bd->device_id);

	return ret;

fail:
	if (switch_dev) {
		switch_release(switch_dev);
	}
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(bd);
	return ret;
}

static int ar8327_remove(struct platform_device *pdev)
{
	struct ar8327_data *bd;

	bd = dev_get_drvdata(&pdev->dev);

	if (bd->switch_dev) {
		switch_unregister(bd->switch_dev);
		switch_release(bd->switch_dev);
	}

	dev_set_drvdata(&pdev->dev, NULL);

	kfree(bd);

	return 0;
}

static struct platform_driver ar8327switchdriver = {
	.driver = {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
	},
	.probe		= ar8327_probe,
	.remove		= __devexit_p(ar8327_remove),
};

static int ar8327_init(void)
{
	return platform_driver_register(&ar8327switchdriver);
}

module_init(ar8327_init);

static void ar8327_exit(void)
{
	platform_driver_unregister(&ar8327switchdriver);
}
module_exit(ar8327_exit);

MODULE_AUTHOR("Thomas Wu");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ar8327 switch chip driver");
