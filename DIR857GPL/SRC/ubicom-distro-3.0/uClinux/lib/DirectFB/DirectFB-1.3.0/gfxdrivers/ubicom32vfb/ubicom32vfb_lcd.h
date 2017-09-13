/*
 * ubicom32vfb_lcd.h
 *	Ubicom32 lcd panel drivers
 *
 * (C) Copyright 2009, Ubicom, Inc.
 *
 * This file is part of the Ubicom32 Linux Kernel Port.
 *
 * This Ubicom32 library is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Ubicom32 library is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Ubicom32 Linux Kernel Port.  If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef _UBICOM32VFB_LCD_H_
#define _UBICOM32VFB_LCD_H_

enum ubicom32vfb_lcd_op {
	/*
	 * Sleep for (data) ms
	 */
	LCD_STEP_SLEEP,

	/*
	 * Execute write of command
	 */
	LCD_STEP_CMD,

	/*
	 * Execute write of data
	 */
	LCD_STEP_DATA,

	/*
	 * Execute write of command/data
	 */
	LCD_STEP_CMD_DATA,

	/*
	 * Script done
	 */
	LCD_STEP_DONE,
};

struct ubicom32vfb_lcd_step {
	enum ubicom32vfb_lcd_op		op;
	u16				cmd;
	u16				data;
};

#define UBICOM32VFB_LCD_PANEL_FLAG_NO_PARTIAL		0x0001
#define UBICOM32VFB_LCD_PANEL_FLAG_8BIT			0x0002

/*
 * Panel is rotated, goto command will swap x and y
 */
#define UBICOM32VFB_LCD_PANEL_FLAG_ROTATED		0x0004

/*
 * Panel's X axis is flipped, compute goto by xofs - x instead of x + xofs
 */
#define UBICOM32VFB_LCD_PANEL_FLAG_FLIPX		0x0008

/*
 * Panel's Y axis is flipped, compute goto by yofs - y instead of y + yofs
 */
#define UBICOM32VFB_LCD_PANEL_FLAG_FLIPY		0x0010

struct ubicom32vfb_lcd_panel {
	const struct ubicom32vfb_lcd_step	*init_seq;
	const char				*desc;

	/*
	 * 0..15 for asserted delay, 16..31 for de-asserted delay
	 */
	u32					delay;

	u32					xres;
	u32					yres;
	u32					stride;

	/*
	 * X and Y offset to use for goto command (panels which are
	 * rotated will most likely use this)
	 */
	u32					xofs;
	u32					yofs;
	u32					flags;

	u16					id;
	u16					horz_reg;
	u16					vert_reg;
	u16					gram_reg;
};

typedef void (*ubicom32vfb_lcd_update_region_fn)(const struct ubicom32vfb_lcd_panel *, u16 *, int, int, int, int);
typedef void (*ubicom32vfb_lcd_update_fn)(const struct ubicom32vfb_lcd_panel *, u16 *);
typedef u16 (*ubicom32vfb_lcd_read_fn)(void);
typedef void (*ubicom32vfb_lcd_write_fn)(u16);
		 
#define UBICOM32VFB_LCD_BUS_8BIT		0x0001
#define UBICOM32VFB_LCD_BUS_16BIT		0x0002

struct ubicom32vfb_lcd_bus {
	const char				*name;

	ubicom32vfb_lcd_update_region_fn	update_region_fn;
	ubicom32vfb_lcd_update_fn		update_fn;
	ubicom32vfb_lcd_read_fn			read_fn;
	ubicom32vfb_lcd_write_fn		write_fn;
	u32					flags;

	u32					data_port;
	u32					data_mask;
	u32					rs_port;
	u32					rd_port;
	u32					wr_port;
	u32					cs_port;
	u32					reset_port;
	u8					rs_pin;
	u8					rd_pin;
	u8					wr_pin;
	u8					cs_pin;
	u8					reset_pin;
};

extern int ubicom32vfb_lcd_init(const struct ubicom32vfb_lcd_panel **panel, int w, int h);
extern void ubicom32vfb_lcd_update(const struct ubicom32vfb_lcd_panel *panel, u16 *data);
extern void ubicom32vfb_lcd_update_region(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h);

#endif
