/*
   (c) Copyright 2009       Ubicom, Inc.
   (c) Copyright 2001-2008  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   Written by Patrick Tjin <pat.tjin@ubicom.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef UBICOM32VFB_H
#define UBICOM32VFB_H

#include <config.h>

#include <sys/ioctl.h>

#include <dfb_types.h>

#include <fbdev/fb.h>

#include <directfb.h>

#include <direct/util.h>
#include <direct/memcpy.h>

#include <core/coredefs.h>
#include <core/coretypes.h>
#include <core/gfxcard.h>
#include <core/layers.h>

#include <fbdev/fbdev.h>

#include <core/graphics_driver.h>

#include <direct/debug.h>

#include "ubicom32vfb_blt.h"
#include "ubicom32vfb_lcd.h"
#include "ubicom32vfb_lcd_panels.h"

struct ubicom32vfb_driver_data {
	const struct ubicom32vfb_lcd_panel	*panel;
};

struct ubicom32vfb_device_data {
	DFBAccelerationMask	draw_flags;
	DFBAccelerationMask	blit_flags;

	/*
	 * source and destination parameters
	 */
	void			*src_addr;
	u32			src_phys;
	u32			src_pitch;
	DFBSurfacePixelFormat	src_format;
	u32			src_bpp;

	void			*dst_addr;
	u32			dst_phys;
	u32			dst_size;
	u32			dst_pitch;
	DFBSurfacePixelFormat	dst_format;
	u32			dst_bpp;
};

D_DEBUG_DOMAIN( ubicom32vfb, "Ubicom32VFB", "Ubicom32 VFB gfx driver" );

extern DisplayLayerFuncs ubicom32vfbPrimaryLayerFuncs;

#endif
