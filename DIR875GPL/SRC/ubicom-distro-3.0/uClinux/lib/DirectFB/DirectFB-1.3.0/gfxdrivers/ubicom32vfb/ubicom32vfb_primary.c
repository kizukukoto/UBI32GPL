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

#include <config.h>

#include <string.h>
#include <sys/ioctl.h>

#include <dfb_types.h>

#include <fbdev/fb.h>

#include <directfb.h>
#include <directfb_util.h>

#include <direct/debug.h>

#include <core/coredefs.h>
#include <core/coretypes.h>
#include <core/layers.h>
#include <core/screens.h>

#include <fbdev/fbdev.h>

#include "ubicom32vfb.h"

/*
 * ubicom32vfbUpdateRegion
 *	Called by the system to update the region to the panel
 */
static DFBResult
ubicom32vfbUpdateRegion( CoreLayer             *layer,
			 void                  *driver_data,
			 void                  *layer_data,
			 void                  *region_data,
			 CoreSurface           *surface,
			 const DFBRegion       *update,
			 CoreSurfaceBufferLock *lock )
{
	struct ubicom32vfb_driver_data *data = driver_data;
	DFBRectangle rect;

	D_DEBUG_AT(ubicom32vfb, "%s()\n", __FUNCTION__);

	dfb_rectangle_from_region(&rect, update);

	/*
	 * Check for full screen (at some point, it is better to update the whole
	 * screen, but for now we will just simply check the w/h against the xres/yres)
	 */
	if ((rect.w == data->panel->xres) && (rect.h == data->panel->yres)) {
		ubicom32vfb_lcd_update(data->panel, lock->addr);
	} else {
		ubicom32vfb_lcd_update_region(data->panel, lock->addr, rect.x, rect.y, rect.w, rect.h);
	}

	return DFB_OK;
}

DisplayLayerFuncs ubicom32vfbPrimaryLayerFuncs = {
	UpdateRegion:       ubicom32vfbUpdateRegion,
};

