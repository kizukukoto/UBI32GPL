/*
   (c) Copyright 2009       Ubicom, Inc.
   (c) Copyright 2001-2008  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   Written by Patrick Tjin <pat.tjin@ubicom.com>.

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

#ifndef _UBICOM32VFB_BLT_H_
#define _UBICOM32VFB_BLT_H_

#include "ubicom32vfb.h"

extern void
ubicom32vfbCheckState(void                *drv,
		      void                *dev,
		      CardState           *state,
		      DFBAccelerationMask  accel );
extern void
ubicom32vfbSetState(	void                *drv,
			void                *dev,
			GraphicsDeviceFuncs *funcs,
			CardState           *state,
			DFBAccelerationMask  accel );

#endif
