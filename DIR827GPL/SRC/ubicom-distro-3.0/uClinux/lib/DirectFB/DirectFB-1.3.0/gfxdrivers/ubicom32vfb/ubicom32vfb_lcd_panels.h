/*
 * ubicom32vfb_lcd_panels.h
 *	Ubicom32 LCD panel code
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

#ifndef UBICOM32VFB_LCD_PANELS_H
#define UBICOM32VFB_LCD_PANELS_H

#include "ubicom32vfb_lcd.h"

extern const struct ubicom32vfb_lcd_panel *ubicom32vfb_lcd_panels[];
extern const struct ubicom32vfb_lcd_panel cfaf240320ktts_0;
extern const struct ubicom32vfb_lcd_panel cfaf240400d;
extern const struct ubicom32vfb_lcd_panel tft2n0369e_land;
extern const struct ubicom32vfb_lcd_panel tft2n0369e_port;
extern const struct ubicom32vfb_lcd_panel tft2n0369e_land_8bit;
extern const struct ubicom32vfb_lcd_panel tft2n0369e_port_8bit;
extern const struct ubicom32vfb_lcd_panel cfaf320240f;

#endif
