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

#include "ubicom32vfb.h"
#include <core/screens.h>

DFB_GRAPHICS_DRIVER( ubicom32vfb )

/* exported symbols */

static int
driver_probe( CoreGraphicsDevice *device )
{
	return dfb_gfxcard_get_accelerator(device) == FB_ACCEL_UBICOM32_VFB;
}

static void
driver_get_info( CoreGraphicsDevice *device,
                 GraphicsDriverInfo *driver_info )
{
	driver_info->version.major = 0;
	driver_info->version.minor = 1;

	direct_snputs( driver_info->name,
		"Ubicom32VFB Driver", DFB_GRAPHICS_DRIVER_INFO_NAME_LENGTH );
	direct_snputs( driver_info->vendor,
		"Ubicom, Inc.", DFB_GRAPHICS_DRIVER_INFO_VENDOR_LENGTH );
	direct_snputs( driver_info->url,
		"http://www.ubicom.com", DFB_GRAPHICS_DRIVER_INFO_URL_LENGTH );
	direct_snputs( driver_info->license,
		"LGPL", DFB_GRAPHICS_DRIVER_INFO_LICENSE_LENGTH );

	driver_info->driver_data_size = sizeof(struct ubicom32vfb_driver_data);
	driver_info->device_data_size = sizeof(struct ubicom32vfb_device_data);
}

static DFBResult
driver_init_driver( CoreGraphicsDevice  *device,
                    GraphicsDeviceFuncs *funcs,
                    void                *driver_data,
                    void                *device_data,
                    CoreDFB             *core )
{
	struct ubicom32vfb_driver_data *data = driver_data;

	D_DEBUG_AT(ubicom32vfb, "%s()\n", __FUNCTION__);

#ifdef UBICOM32_ACCEL
	funcs->CheckState = ubicom32vfbCheckState;
	funcs->SetState = ubicom32vfbSetState;
#endif

	dfb_layers_hook_primary(device, driver_data, &ubicom32vfbPrimaryLayerFuncs, NULL, NULL );

	return DFB_OK;
}

static DFBResult
driver_init_device( CoreGraphicsDevice *device,
                    GraphicsDeviceInfo *device_info,
                    void               *driver_data,
                    void               *device_data )
{
	struct ubicom32vfb_driver_data *data = driver_data;

	D_DEBUG_AT( ubicom32vfb, "%s()\n", __FUNCTION__);

	device_info->caps.flags = CCF_READSYSMEM;

	direct_snputs( device_info->name, "ubicom32vfb", DFB_GRAPHICS_DEVICE_INFO_NAME_LENGTH );
	direct_snputs( device_info->vendor, "Ubicom", DFB_GRAPHICS_DEVICE_INFO_VENDOR_LENGTH );

	/*
	 * Attempt to guess the orientation of the panel corect
	 */
	VideoMode *mode = dfb_system_current_mode();
	if (ubicom32vfb_lcd_init(&data->panel, mode ? mode->xres : 0, mode ? mode->yres : 0)) {
		return DFB_INIT;
	}

	return DFB_OK;
}

static void
driver_close_device( CoreGraphicsDevice *device,
                     void               *driver_data,
                     void               *device_data )
{
}

static void
driver_close_driver( CoreGraphicsDevice *device,
                     void               *driver_data )
{
}
