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

#include "ubicom32vfb.h"

D_DEBUG_DOMAIN( UBICOM32VFB_BLT, "Ubicom32VFB/BLT", "Ubicom32 VFB Accelerator");

#define UBICOM32VFB_SUPPORTED_DRAWINGFUNCTIONS	(DFXL_NONE)	//FILLRECTANGLE | DFXL_DRAWRECTANGLE | DFXL_DRAWLINE)
#define UBICOM32VFB_SUPPORTED_DRAWINGFLAGS	(DSDRAW_NOFX)
#define UBICOM32VFB_SUPPORTED_BLITTINGFUNCTIONS	(DFXL_BLIT)
#define UBICOM32VFB_SUPPORTED_BLITTINGFLAGS	(DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_COLORIZE)

static bool ubicom32vfbBlit16(void *drv, void *dev, DFBRectangle *rect, int x, int y );
static bool ubicom32vfbBlit32(void *drv, void *dev, DFBRectangle *rect, int x, int y );
static bool ubicom32vfbBlitARGB16(void *drv, void *dev, DFBRectangle *rect, int x, int y );

void
ubicom32vfbCheckState(void                *drv,
		      void                *dev,
		      CardState           *state,
		      DFBAccelerationMask  accel )
{
	D_DEBUG_AT(UBICOM32VFB_BLT, "%s( %p, 0x%08x )\n", __FUNCTION__, state, accel );
	D_DEBUG_AT(UBICOM32VFB_BLT, "dis=%u accel=%x dst.format=%x\n", state->disabled, accel, state->destination->config.format);
	//D_DEBUG_AT(UBICOM32VFB_BLT, "dst->policy = %u\n", state->dst.buffer->policy);

	/* 
	 * Return if the desired function is not supported at all.
	 */
	if (accel & ~(UBICOM32VFB_SUPPORTED_DRAWINGFUNCTIONS | UBICOM32VFB_SUPPORTED_BLITTINGFUNCTIONS)) {
		return;
	}

	/* 
	 * Return if the destination format is not supported.
	 */
	switch (state->destination->config.format) {
	case DSPF_RGB16:
	case DSPF_ARGB:
	case DSPF_RGB32:
		break;
	default:
		D_DEBUG_AT(UBICOM32VFB_BLT, "dst unsupported\n");
		return;
	}

	/* 
	 * Check if drawing or blitting is requested.
	 */
	if (DFB_DRAWING_FUNCTION( accel )) {
		/* 
		 * Return if unsupported drawing flags are set.
		 */
		if (state->drawingflags & ~UBICOM32VFB_SUPPORTED_DRAWINGFLAGS) {
			D_DEBUG_AT(UBICOM32VFB_BLT, "no draw fl=%x\n", state->drawingflags);
			return;
		}

#if 0
		/* 
		 * Return if blending with unsupported blend functions is requested.
		 */
		if (state->drawingflags & DSDRAW_BLEND) {
			/* Check blend functions. */
			if (!check_blend_functions( state )) {
				return;
			}

			/* XOR only without blending. */
			if (state->drawingflags & DSDRAW_XOR) {
				return;
			}
		}
#endif
		/* 
		 * Enable acceleration of drawing functions.
		 */
		state->accel |= UBICOM32VFB_SUPPORTED_DRAWINGFUNCTIONS;
		D_DEBUG_AT(UBICOM32VFB_BLT, "draw function\n");
		return;
	}

	DFBSurfaceBlittingFlags flags = state->blittingflags;

	D_DEBUG_AT(UBICOM32VFB_BLT, "blit src.format=%x fl=%x\n", state->source->config.format, flags);

	/*
	 * Return if unsupported blitting flags are set. 
	 */
	if (flags & ~UBICOM32VFB_SUPPORTED_BLITTINGFLAGS) {
		D_DEBUG_AT(UBICOM32VFB_BLT, "no blit\n");
		return;
	}

	/* 
	 * Return if the source format is not supported. 
	 */
	switch (state->source->config.format) {
	case DSPF_RGB32:
	case DSPF_ARGB:
	case DSPF_RGB16:
		break;

	default:
		D_DEBUG_AT(UBICOM32VFB_BLT, "blit unsupported\n");
		return;
	}

	state->accel |= UBICOM32VFB_SUPPORTED_BLITTINGFUNCTIONS;
}

static inline void
ubicom32vfbCheckSource(struct ubicom32vfb_device_data *udev, CardState *state)
{
	//CoreSurface       *surface = state->source;
	CoreSurfaceBuffer *buffer  = state->src.buffer;

	udev->src_addr = state->src.addr;
	udev->src_pitch = state->src.pitch;
	udev->src_bpp = DFB_BYTES_PER_PIXEL( buffer->format );
	udev->src_format = state->src.buffer->format;

	D_DEBUG_AT( UBICOM32VFB_BLT, "%s src_addr=%p pitch=%u bpp=%u\n", __FUNCTION__, state->src.addr, state->src.pitch, udev->src_bpp);
}

static inline void
ubicom32vfbCheckDest(struct ubicom32vfb_device_data *udev, CardState *state)
{
	CoreSurfaceBuffer *buffer  = state->src.buffer;

	udev->dst_addr  = state->dst.addr;
	udev->dst_pitch = state->dst.pitch;
	udev->dst_bpp   = DFB_BYTES_PER_PIXEL( buffer->format );
	udev->dst_format = state->dst.buffer->format;

	D_DEBUG_AT( UBICOM32VFB_BLT, "%s dst_addr=%p pitch=%u bpp=%u\n", __FUNCTION__, state->dst.addr, state->dst.pitch, udev->dst_bpp);
}

/*
 * Make sure that the hardware is programmed for execution of 'accel' according to the 'state'.
 */
void
ubicom32vfbSetState(	void                *drv,
			void                *dev,
			GraphicsDeviceFuncs *funcs,
			CardState           *state,
			DFBAccelerationMask  accel )
{
	struct ubicom32vfb_driver_data *udrv = drv;
	struct ubicom32vfb_device_data *udev = dev;
	StateModificationFlags  modified = state->mod_hw;

	udev->draw_flags = state->drawingflags;
	udev->blit_flags = state->blittingflags;

	D_DEBUG_AT( UBICOM32VFB_BLT, "%s( %p, 0x%08x ) <- modified 0x%08x\n", __FUNCTION__, state, accel, modified );

	D_DEBUG_AT( UBICOM32VFB_BLT, "%s( 0x%p [%d] - %4d,%4d-%4dx%4d )\n", __FUNCTION__,
                 state->dst.addr, state->dst.pitch, DFB_RECTANGLE_VALS_FROM_REGION( &state->clip ) );

	ubicom32vfbCheckDest(udev, state);

	switch (accel) {
	case DFXL_FILLRECTANGLE:
		state->set = UBICOM32VFB_SUPPORTED_DRAWINGFUNCTIONS;
		break;

	case DFXL_BLIT:
		ubicom32vfbCheckSource(udev, state);
		state->set = UBICOM32VFB_SUPPORTED_BLITTINGFUNCTIONS;
		if ((udev->src_bpp == udev->dst_bpp) && (udev->src_bpp == 4)) {
			funcs->Blit = ubicom32vfbBlit32;
			break;
		}
		if ((udev->src_bpp == udev->dst_bpp) && (udev->src_bpp == 2)) {
			funcs->Blit = ubicom32vfbBlit16;
			break;
		}
		if ((udev->src_format == DSPF_ARGB) && (udev->dst_format == DSPF_RGB16)) {
			funcs->Blit = ubicom32vfbBlitARGB16;
			break;
		}
		break;

	default:
		break;
	}

	/*
	 * 4) Clear modification flags
	 *
	 * All flags have been evaluated in 1) and remembered for further validation.
	 * If the hw independent state is not modified, this function won't get called
	 * for subsequent rendering functions, unless they aren't defined by 3).
	 */
	state->mod_hw = 0;
}

/*
 * Blit a rectangle using the current hardware state.
 */
static bool ubicom32vfbBlit32(void *drv, void *dev, DFBRectangle *rect, int dx, int dy)
{
	struct ubicom32vfb_driver_data *udrv = drv;
	struct ubicom32vfb_device_data *udev = dev;
	int w = rect->w;
	int h = rect->h;

	D_DEBUG_AT( UBICOM32VFB_BLT, "%s( %d, %d - %dx%d  ->  %d, %d - %dx%d )\n", __FUNCTION__, DFB_RECTANGLE_VALS( rect ), dx, dy, w, h );

	void *dst = udev->dst_addr + (dy * udev->dst_pitch) + DFB_BYTES_PER_LINE(udev->dst_format, dx);
	void *src = udev->src_addr + (rect->y * udev->src_pitch) + DFB_BYTES_PER_LINE(udev->src_format, rect->x);

	w *= udev->dst_bpp;

	while (h--) {
		direct_memcpy(dst, src, w);
		src += udev->src_pitch;
		dst += udev->dst_pitch;
	}

	return true;
}

/*
 * Blit a rectangle using the current hardware state.
 */
static bool ubicom32vfbBlit16(void *drv, void *dev, DFBRectangle *rect, int dx, int dy)
{
	struct ubicom32vfb_driver_data *udrv = drv;
	struct ubicom32vfb_device_data *udev = dev;
	int w = rect->w;
	int h = rect->h;

	D_DEBUG_AT( UBICOM32VFB_BLT, "%s( %d, %d - %dx%d  ->  %d, %d - %dx%d )\n", __FUNCTION__, DFB_RECTANGLE_VALS( rect ), dx, dy, w, h );

	void *dst = udev->dst_addr + (dy * udev->dst_pitch) + DFB_BYTES_PER_LINE(udev->dst_format, dx);
	void *src = udev->src_addr + (rect->y * udev->src_pitch) + DFB_BYTES_PER_LINE(udev->src_format, rect->x);

	w *= udev->dst_bpp;

	while (h--) {
		direct_memcpy(dst, src, w);
		src += udev->src_pitch;
		dst += udev->dst_pitch;
	}

	return true;
}

/*
 * Blit a rectangle using the current hardware state.
 */
static bool ubicom32vfbBlitARGB16(void *drv, void *dev, DFBRectangle *rect, int dx, int dy)
{
	struct ubicom32vfb_driver_data *udrv = drv;
	struct ubicom32vfb_device_data *udev = dev;
	int w = rect->w;
	int h = rect->h;

	D_DEBUG_AT( UBICOM32VFB_BLT, "%s( %d, %d - %dx%d  ->  %d, %d - %dx%d )\n", __FUNCTION__, DFB_RECTANGLE_VALS( rect ), dx, dy, w, h );

	u16 *dst = udev->dst_addr + (dy * udev->dst_pitch) + DFB_BYTES_PER_LINE(udev->dst_format, dx);
	u32 *src = udev->src_addr + (rect->y * udev->src_pitch) + DFB_BYTES_PER_LINE(udev->src_format, rect->x);

	while (h--) {
		int i;
		u16 *tdst = dst;
		u32 *tsrc = src;
		for (i = 0; i < w; i++) {
			*tdst++ = *tsrc++;
		}
		src += udev->src_pitch;
		dst += udev->dst_pitch;
	}

	return true;
}

