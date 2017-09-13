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

#ifndef __MDIO_H__
#define __MDIO_H__
#define mdelay(n) udelay((n)*1000)

#define phy_reg_read mdiobb_read
#define phy_reg_write mdiobb_write

int mdiobb_write(int phy_base, int phy, int reg, u16 val);
u32 mdiobb_read(int phy_base, int phy, int reg);
int mdio_reset(void);
int mdio_init(void);

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILED
#define FAILED -1
#endif

#endif
