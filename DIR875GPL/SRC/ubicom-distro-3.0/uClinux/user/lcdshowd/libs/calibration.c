/*
 * Touch Screen Calibration
 *
 * Copyright © 2009 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>

#include <linux/input.h>
#include <directfb.h>
#include <lite/lite.h>
#include <tslib.h>

#include "global.h"
#include "image.h"
#include "mouse.h"
#include "calibrate.h"

#define GET_DIRECTFB_IF() lite_get_dfb_interface()

#define DEBUG 1
#define TRACE 1

#if DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#if TRACE
#define DEBUG_TRACE(...) DEBUG_PRINTF(__VA_ARGS__)
#else
#define DEBUG_TRACE(...)
#endif

typedef void CoreInputDevice; //just a hack
CoreInputDevice *dfb_input_device_at(DFBInputDeviceID id);
DFBResult dfb_input_device_reload_keymap(CoreInputDevice *device);

extern int MarEnabled; //mouse1.c

/*
 * Touch screen initialization
 */
static struct tsdev *ts_init(void)
{
	char *tsn = getenv("TSLIB_TSDEVICE");
	if (!tsn) {
		tsn = "/dev/input/event1";
	}

	struct tsdev *ts = ts_open(tsn, 0);
	if (!ts) {
		DEBUG_PRINTF("ts_open %s error\n", tsn);
		return NULL;
	}

	if (ts_config(ts)) {
		DEBUG_PRINTF("ts_config error\n");
		ts_close(ts);
		return NULL;
	}

	return ts;
}

/*
 * Touch screen calibration datat reload
 */
static int ts_reload(MATRIX *m)
{
	IDirectFB *dfb = GET_DIRECTFB_IF();

	char *cbn = getenv("TSLIB_CALIBFILE");
	if (!cbn) {
		cbn = "/etc/ts.calib";
	}

	int fd = open(cbn, O_CREAT | O_RDWR);
	if (fd < 0) {
		DEBUG_PRINTF("open %s error\n", cbn);
		return -1;
	}

	char buf[12*7+1];
	snprintf(buf, sizeof(buf), "%ld %ld %ld %ld %ld %ld %ld\n", m->An, m->Bn, m->Cn, m->Dn, m->En, m->Fn, m->Divider);

	int i = strlen(buf);
	if (write(fd, buf, i) != i) {
		DEBUG_PRINTF("write %s error\n", cbn);
		close(fd);
		return -1;
	}
	close(fd);

	IDirectFBInputDevice *device;
	DFBInputDeviceDescription desc;

	//limit to 128 to prevent endless loop
	for(i = 0; i < 128; i++) {
		if (dfb->GetInputDevice(dfb, i, &device) == DFB_OK) {
			device->GetDescription(device, &desc);
			if (strcmp("tslib", desc.vendor) == 0) {
				if (dfb_input_device_reload_keymap(dfb_input_device_at (i)) == DFB_OK) {
					DEBUG_PRINTF("calib data reloaded\n");
					return 0;
				}
				break;
			}
		}
	}
	DEBUG_PRINTF("calib reload error\n");

	return -1;
}

/*
 * Draw a pixel
 */
static void cb_draw_pixel(IDirectFBSurface *surface, int x, int y)
{
	surface->DrawLine(surface, x, y, x, y);
}

/*
 * Draw four circle mirror pixels
 */
static void cb_draw_circle_pixels(IDirectFBSurface *surface, int cx, int cy, int x, int y)
{
	cb_draw_pixel(surface, cx + x, cy + y);
	cb_draw_pixel(surface, cx + x, cy - y);
	cb_draw_pixel(surface, cx - x, cy + y);
	cb_draw_pixel(surface, cx - x, cy - y);
}

/*
 * Draw a circle
 */
static void cb_draw_circle(IDirectFBSurface *surface, int cx, int cy, int r)
{
	int x, y = r, d = -r, x2m1= -1;

	cb_draw_circle_pixels(surface, cx, cy, 0, r);

	for (x = 1; x < y; x++) {

		if ((d += (x2m1 += 2)) > 0) {
			d -= (--y << 1);
		}

		cb_draw_circle_pixels(surface, cx, cy, x, y);
		cb_draw_circle_pixels(surface, cx, cy, y, x);
	}

	cb_draw_circle_pixels(surface, cx, cy, x, y);
}

/*
 * Draw calibration crosshair
 */
#define FIN_LEN 12

static void cb_draw_cross(IDirectFBSurface *surface, int x, int y, int flip)
{
	surface->DrawLine(surface, x - FIN_LEN, y - 1, x + FIN_LEN, y - 1);
	surface->DrawLine(surface, x - FIN_LEN, y + 1, x + FIN_LEN, y + 1);
	surface->DrawLine(surface, x - 1, y - FIN_LEN, x - 1, y + FIN_LEN);
	surface->DrawLine(surface, x + 1, y - FIN_LEN, x + 1, y + FIN_LEN);

	if (flip) surface->Flip(surface, NULL, 0);
}

/*
 * Draw calibration background
 */
static void cb_draw_background(IDirectFBSurface *surface)
{
	#define BACKGROUND_COLOR 1 //use background color or image

#if BACKGROUND_COLOR
	int w, h;

	surface->GetSize(surface, &w, &h);
	surface->SetColor(surface, 0x18, 0x18, 0x18, 0);
	surface->FillRectangle(surface, 0, 0, w, h);
#else
	/*
	 * MAY BE REPLACED BY LOCAL UTILITY "image_surface_create()"
	 * IDirectFBSurface *background = image_surface_create("background.png");
	 * background->Release(background);
	 */
	IDirectFB *dfb = GET_DIRECTFB_IF();
	IDirectFBImageProvider *provider;
	DFBSurfaceDescription dsc;

	dfb->CreateImageProvider(dfb, IMAGE_DIR "background.png", &provider);
	provider->GetSurfaceDescription(provider, &dsc);

	dsc.flags |= DSDESC_CAPS;
	dsc.caps = DSCAPS_SHARED;

	provider->RenderTo(provider, surface, NULL);
	provider->Release(provider);
#endif

	surface->Flip(surface, NULL, 0);

	DEBUG_PRINTF("background loaded\n");
}

/*
 * Draw calibration hint text
 */
static void cb_draw_text(IDirectFBSurface *surface, DFBPoint *pt)
{
	#define TRUETYPE_DIR "/share/fonts/truetype/"

	IDirectFB *dfb = lite_get_dfb_interface();

	/*
	 * MAY BE REPLACED BY LiTE LABEL FUNCTiONS
	 */
	IDirectFBFont *font;
	DFBFontDescription font_dsc;

	font_dsc.flags = DFDESC_HEIGHT;
	surface->SetColor(surface, 0xff, 0xff, 0xff, 0);

	font_dsc.height = 15;
	dfb->CreateFont(dfb, TRUETYPE_DIR "vera.ttf", &font_dsc, &font);
	surface->SetFont(surface, font);
	surface->DrawString(surface, "TOUCH SCREEN CALiBRATiON", -1, pt->x/2, pt->y/2 - FIN_LEN, DSTF_BOTTOMCENTER);
	font->Release(font);

	font_dsc.height = 14;
	dfb->CreateFont(dfb, TRUETYPE_DIR "vera.ttf", &font_dsc, &font);
	surface->SetFont(surface, font);
	surface->DrawString(surface, "please tap crosshairs one by one", -1, pt->x/2, pt->y/2 + FIN_LEN, DSTF_TOPCENTER);
	font->Release(font);

	surface->Flip(surface, NULL, 0);

	DEBUG_PRINTF("text printed\n");
}

/*
 * Touch screen read samples
 */
static void ts_read_samples(IDirectFBSurface *surface, struct tsdev *ts, POINT *xs_samps)
{
	#define MOVE_STEPS 32
	#define MOVE_UDELAY 1500

	DFBWindowEvent evt;
	struct ts_sample samp;
	int i, j, x, y, dx, dy, t;

	surface->SetDrawingFlags(surface, DSDRAW_XOR);
	surface->SetColor(surface, 0xff, 0xff, 0xff, 0xff);

	t = MarEnabled;
	MarEnabled = 0; //disable margin check

	DEBUG_PRINTF("sampling...\n");

	for (i = 0; i < 3; i++) {

		if (i != 0) {

			cb_draw_circle(surface, x, y, FIN_LEN + 1);

			dx = (xs_samps[i].x - x) / MOVE_STEPS;
			dy = (xs_samps[i].y - y) / MOVE_STEPS;
		
			for (j = 0; j < MOVE_STEPS; j++) {

				cb_draw_cross(surface, x, y, 0);
				x += dx;
				y += dy;
				cb_draw_cross(surface, x, y, 1);
				usleep(MOVE_UDELAY);
			}

			cb_draw_cross(surface, x, y, 1);
			usleep(MOVE_UDELAY);
		}

		x = xs_samps[i].x;
		y = xs_samps[i].y;

		cb_draw_circle(surface, x, y, FIN_LEN + 1);
		cb_draw_cross(surface, x, y, 1);

		//read a sample
		while (1) {

			int ret = ts_read_raw(ts, &samp, 1);
			if (ret < 0) {
				DEBUG_PRINTF("ts_read_raw error\n");
				continue;
			}
			if (ret != 1) continue;
			if (samp.pressure > 1024) continue; //ignore sample whose pressure value is invalid

			evt.cx   = samp.x;
			evt.cy   = samp.y;
			evt.type = samp.pressure? DWET_MOTION : DWET_BUTTONUP;

			ms_advanced_handler(&evt);
			if (samp.pressure == 0 && evt.type == DWET_BUTTONUP) break;
		}

		xs_samps[i].x = evt.cx;
		xs_samps[i].y = evt.cy;
	}

	MarEnabled = t; //restore margin check flag
}

/*
 * Initialize calibration samples
 */
static void cb_init_samples(POINT *samps, int res_x, int res_y)
{
	#define PERCENT 15

	samps[0].x = res_x * PERCENT / 100;
	samps[0].y = res_y * PERCENT / 100;

	samps[1].x = res_x *  50 / 100;
	samps[1].y = res_y * (100 - PERCENT) / 100;

	samps[2].x = res_x * (100 - PERCENT) / 100;
	samps[2].y = res_y *  50 / 100;
}

/*
 * Touch screen calibration entrance
 */
int ts_calibration(IDirectFBSurface *surface)
{
	DFBPoint xs_res;

	surface->GetSize(surface, &xs_res.x, &xs_res.y);
	DEBUG_PRINTF("display %dx%d\n", xs_res.x, xs_res.y);

	//open device
	struct tsdev *ts = ts_init();
	if (!ts) return -1;

	//background and text
	cb_draw_background(surface);
	cb_draw_text(surface, &xs_res);

	MATRIX mat;
	POINT ds_samps[3], ts_samps[3];

	//sampling
	cb_init_samples(ts_samps, xs_res.x, xs_res.y);
	ts_read_samples(surface, ts, ts_samps);
	DEBUG_PRINTF("samps=%d,%d %d,%d %d,%d\n",
		ts_samps[0].x, ts_samps[0].y, ts_samps[1].x, ts_samps[1].y, ts_samps[2].x, ts_samps[2].y);

	ts_close(ts);

	//calibration
	cb_init_samples(ds_samps, xs_res.x, xs_res.y);
	setCalibrationMatrix(ds_samps, ts_samps, &mat);
	DEBUG_PRINTF("calib mat: An=%ld Bn=%ld Cn=%ld Dn=%ld En=%ld Fn=%ld Di=%ld\n",
		mat.An, mat.Bn, mat.Cn, mat.Dn, mat.En, mat.Fn, mat.Divider);

	return ts_reload(&mat);
}
