/*
 *  Calibration System-Wide Functions
 *
 * Copyright © 2009 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 *
 */

/*
 * TODO:
 * - re-write calibration_on_touch_callback. Funcstions per point_id should be specified.
 *   Regardless of point_id condition. Like showing/hiding icon.
 * - Modify the algorithm of getting user input. Sample number should be increased, 
 *   Better algorithm needs to be written irrevelant user input.
 * - Popup menu should be included about saving the new calibration data.
 * - Rewrite assignment of point_coordinates.
 * - in calc_average() check that enough samples (SAMPLE_NUMBER_PER_POINT) is collected.
 */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include <directfb.h>
#include <debug.h>
#include <lite/window.h>
#include <gui/widgets/grid.h>
#include <gui/widgets/header.h>
#include <gui/widgets/statusbar.h>
#include <gui/widgets/scroll.h>
#include <gui/mm/menu_data.h>
#include <gui/engine/app_loader.h>
#include <gui/widgets/calibration_window.h>

#include "calibrate.h"

#define DEBUG
#define NUMBER_OF_POINTS 3
struct calibration_container {
	char *dev_file;
	char *calib_file;
	POINT screen_sample[NUMBER_OF_POINTS];
	POINT display_sample[NUMBER_OF_POINTS];
	MATRIX matrix;
	int point_id;
	struct calibration_window_instance *ci;
};

static int calibration_get_samples(const char *dev, POINT *point);
static INT32 calc_average(const INT32 *sample, int n_sample);
static DFBResult calibration_on_touch_callback(DFBWindowEvent *ev, void *data);
static int calibration_window_populate_display_sample(POINT *display_sample, int xmax, int ymax);
static void calibration_finalize_calibration(struct calibration_container *cc);
static int calibration_process_samples(MATRIX *matrix, POINT *screen_samples, POINT *display_samples);
static int calibration_save_calibrationdata(const char *calib_file, MATRIX matrix);
static void calibration_destroy(struct calibration_container *cc);
static int calibration_reload_keymap(void);

void calibration_set_callback(struct menu_data_instance *menu_data, int id, void *data)
{
	DFBResult dfbres;
	int res;
	int xmax, ymax;

	struct calibration_container *cc = calloc(1, sizeof(struct calibration_container));
	if(!cc) {
	  DEBUG_ERROR("Cannot allocate calibration_container");
	  return;
	}

	cc->dev_file = getenv("TSLIB_TSDEVICE");
	if(!cc->dev_file || !strlen(cc->dev_file)) {
		DEBUG_ERROR("TSLIB_TSDEVICE is not set");
		goto error;
	}

	cc->calib_file = getenv("TSLIB_CALIBFILE");
	if(!cc->calib_file || !strlen(cc->calib_file)) {
		DEBUG_ERROR("TSLIB_CALIBFILE is not defined");
		goto error;
	}

	dfbres = calibration_window_instance_new_window(&cc->ci);
	if(dfbres != DFB_OK) {
		DEBUG_ERROR("calibration_window_instance_new_window failed");
		goto error;
	}

	calibration_window_instance_get_coordinates(cc->ci, &xmax, &ymax);

	res = calibration_window_populate_display_sample(cc->display_sample, xmax, ymax);
	if(!res) {
		DEBUG_ERROR("calibration_window_populate_display_sample");
		goto error;
	}

	/* TODO:
	 * This looks ugly, fix here 
	 */
	DFBRectangle point_coordinates[NUMBER_OF_POINTS] = {
		/* 1st point: 10% of X - (width of image)/2, 10% of Y - (height of image)/2 */
		{(cc->display_sample[0].x - 15), (cc->display_sample[0].y - 15), 30, 30},

		/* 2nd point: 50% of X - (width of image)/2, 90% of Y - (height of image)/2 */
		{(cc->display_sample[1].x - 15), (cc->display_sample[1].y - 15), 30, 30},

		/* 3rd point: (90% of X - (width of image)/2, 50% of Y - (height of image)/2 */
		{(cc->display_sample[2].x - 15), (cc->display_sample[2].y - 15), 30, 30},
	};

	dfbres = calibration_window_instance_place_icons(cc->ci, point_coordinates, NUMBER_OF_POINTS, "/share/LiTE/lcdrouter/theme/sky/point.png");
	if(dfbres != DFB_OK) {
		DEBUG_ERROR("calibration_window_instance_place_icons failed");
		goto error;
	}

	dfbres = calibration_window_instance_on_raw_window_mouse(cc->ci, calibration_on_touch_callback, cc);
	if(dfbres != DFB_OK) {
		DEBUG_ERROR("calibration_window_on_raw_window_mouse failed");
		goto error;
	}

	/* lets start show by showing first point, remember we already 0'ed point_id */
	dfbres = calibration_window_instance_show_icon_by_id(cc->ci, cc->point_id);
	if(dfbres != DFB_OK) {
		DEBUG_ERROR("calibration_window_show_icon_by_id failed");
		goto error;
	}

	dfbres = calibration_window_instance_set_text(cc->ci, "Touch the points");
	if(dfbres != DFB_OK) {
		DEBUG_ERROR("calibration_window_set_text failed");
		goto error;
	}

	return;

  error:
	calibration_destroy(cc);
}

/*
 * calibration_on_touch_callback()
 * This is the function to be called every touch on the calibration screen.
 */
static DFBResult calibration_on_touch_callback(DFBWindowEvent *ev, void *data)
{
	struct calibration_container *cc = (struct calibration_container *)data;
	DFBResult dfbres;
	int res;

	res = calibration_get_samples(cc->dev_file, &cc->screen_sample[cc->point_id]);
	if(!res) {
		DEBUG_ERROR("calibration_Get_samples() error! halting\n");
		return DFB_FAILURE;
	}

	dfbres = calibration_window_instance_hide_icon_by_id(cc->ci, cc->point_id);
	if(dfbres != DFB_OK) {
		DEBUG_ERROR("calibration_window_hide_icon_by_id failed (returned : %x)", dfbres);
		return dfbres;
	}

	cc->point_id++;

	if(!(cc->point_id < NUMBER_OF_POINTS)) {
		calibration_finalize_calibration(cc);

		/* should not fall here */
		return DFB_FAILURE;
	}
		
	calibration_window_instance_show_icon_by_id(cc->ci, cc->point_id);

	return DFB_OK;
}

/*
 *calibration_get_samples()
 * This function takes actual data from device file, note that this function is blocking
 */
static int calibration_get_samples(const char *dev, POINT *point)
{
#define SAMPLE_NUMBER_PER_POINT 5
	fd_set readfs;

	int ret;
	INT32 xsamples[SAMPLE_NUMBER_PER_POINT], ysamples[SAMPLE_NUMBER_PER_POINT], xaverage, yaverage;
	int n_xsamples, n_ysamples;

	struct input_event ev;
	int n_ev = sizeof(struct input_event);

	struct timeval timeout = {
		/* 4 secs listening should be enough */
		.tv_sec = 4,
		.tv_usec = 0,
	};

	int fd = open(dev, O_RDONLY);
	if(fd < 0) {
		DEBUG_ERROR("open file : %s, errno : %s", dev, errno);
		return fd;
	}

	n_xsamples = n_ysamples = 0;

	FD_ZERO(&readfs);
	FD_SET(fd, &readfs);

	while((ret = select(fd+1, &readfs, NULL, NULL, &timeout))) {
		if(ret == 0){
			/* timeout occured, which means user took of his finger from screen, 
			 *process the samples just gotten
			 */
			break;		
		}
		if(!FD_ISSET(fd, &readfs)) {
			/* what else can be set? */
			continue;
		}

		if(read(fd, &ev, n_ev) != n_ev) {
			DEBUG_ERROR("Read error from device : %s", dev);
			close(fd);
			return -1;
		}

		switch(ev.type) {
		case EV_ABS:
			switch(ev.code) {
			case ABS_X:
				n_xsamples %= SAMPLE_NUMBER_PER_POINT;
				xsamples[n_xsamples++] = ev.value;
				break;
			case ABS_Y:
				n_ysamples %= SAMPLE_NUMBER_PER_POINT;
				ysamples[n_ysamples++] = ev.value;
				break;
			default:
				break;
			}
		default:
			break;
		}
	}
	close(fd);

	/* Get average of samples we got */
	xaverage = calc_average(xsamples, SAMPLE_NUMBER_PER_POINT);
	yaverage = calc_average(ysamples, SAMPLE_NUMBER_PER_POINT);
	if(xaverage == 0 || yaverage == 0) {
		return -1;
	}

	/* We divide here by 4, to scale down 0 - 4096 values to 0 - 1024 */
	point->x = xaverage >> 2;
	point->y = yaverage >> 2;

	return 1;
}

/*
 * calc_average()
 * calculate the average of collected samples
 */
static INT32 calc_average(const INT32 *sample, int n_sample)
{
	int i;
	INT32 sum = 0;

	if(!n_sample) {
		return 0;
	}

	for(i = 0; i < n_sample; i++) {
		sum += sample[i];
	}

	return (sum/n_sample);
}

/*
 * calibration_window_populate_display_sample()
 * The coordinates of points, calculated. If more points are needed
 * dont forget to add coordinates for them in DisplayCoefficients
 */
static int calibration_window_populate_display_sample(POINT *display_sample, int xmax, int ymax)
{
	POINT *ds = display_sample;
	if(!ds) {
		return -1;
	}

	int i;
	struct Coefficients {
		/* Percentage on X Coordinate */
		int Px;

		/* percentage on Y Coordinate */
		int Py;

		int K;
	};

	/* If you are going to add more points,
	 *add coordinates here first
	 */
	struct Coefficients DisplayCoefficients[] = {
		/* 1st point: 10% of X, 10% of Y */
		{10, 10, 100},

		/* 2nd point: 50% of X, 90% of Y */
		{50, 90, 100},
		
		/* 3rd point: 90% of X, 50% of Y */
		{90, 50, 100},
	};

	if(sizeof(DisplayCoefficients)/sizeof(struct Coefficients) > NUMBER_OF_POINTS) {
		DEBUG_ERROR("If you want to increase point number, dont forget to add coordinates for them");
		return -1;
	}

	for(i = 0; i < NUMBER_OF_POINTS; i++) {
		ds[i].x = (INT32) (xmax * DisplayCoefficients[i].Px / DisplayCoefficients[i].K);
		ds[i].y = (INT32) (ymax * DisplayCoefficients[i].Py / DisplayCoefficients[i].K);
	}
	return 1;
}

static void calibration_finalize_calibration(struct calibration_container *cc)
{
	int res;

	res = calibration_process_samples(&cc->matrix, cc->screen_sample, cc->display_sample);
	if(!res) {
		DEBUG_ERROR("Error occured while processing samples");
		goto exit;
	}

	res = calibration_save_calibrationdata(cc->calib_file, cc->matrix);
	if(!res) {
		DEBUG_ERROR("Cannot save to a file : %s", cc->calib_file);
		goto exit;
	}

	res = calibration_reload_keymap();
	if(!res) {
	    DEBUG_ERROR("Cannot reload keymap");
	    goto exit;
	}

  exit:
	calibration_destroy(cc);
}

/*
 * Used coding convention of calibrate library in this function
 */
static int calibration_process_samples(MATRIX *matrix, POINT *screen_samples, POINT *display_samples)
{
	POINT *ss = screen_samples;
	POINT *ds = display_samples;
	MATRIX *mx = matrix;
	int res;

	POINT perfectScreenSample[3] = {
		{ 100, 100 },
		{ 900, 500 },
		{ 500, 900 }
	};
	POINT perfectDisplaySample[3] = {
		{ 100, 100 },
		{ 900, 500 },
		{ 500, 900 }
	};

	res = setCalibrationMatrix( &perfectDisplaySample[0], 
						  &perfectScreenSample[0], 
						  mx );
	if(res != OK) {
		return -1;
	}

	res = setCalibrationMatrix(ds, ss, mx);
	if(res != OK) {
		return -1;
	}

    DEBUG_TRACE("\n\nThis is the actual calibration matrix that we will use\n"
           "for all points (until we calibrate again):\n\n"
           "matrix.An = % 8d  matrix.Bn = % 8d  matrix.Cn = % 8d\n"
           "matrix.Dn = % 8d  matrix.En = % 8d  matrix.Fn = % 8d\n"
           "matrix.Divider = % 8d\n",
           mx->An,mx->Bn,mx->Cn,
           mx->Dn,mx->En,mx->Fn,
           mx->Divider ) ;

	return 1;
}

static int calibration_save_calibrationdata(const char *calib_file, MATRIX matrix)
{
	/* 
	 * max signed 32 bit value * 7 calibration values + 6 spaces = 83
	 * So 96 should enogh than
	 */
	int nbuff = 96, nsize;
	char buff[nbuff];

	int fd = open(calib_file, O_WRONLY | O_TRUNC);
	if(fd < 0) {
		DEBUG_ERROR("Cannot open calibration file : %s", calib_file);
		return -1;
	}

	snprintf(buff, nbuff, "%ld %ld %ld %ld %ld %ld %ld\n",
			 matrix.An, matrix.Bn, matrix.Cn,
			 matrix.Dn, matrix.En, matrix.Fn,
			 matrix.Divider);

	nsize = strlen(buff);
	if(write(fd, buff, nsize) != nsize) {
		DEBUG_ERROR("write() error on file : %s", calib_file);
		close(fd);
		return -1;
	}
	fsync(fd);
	close(fd);
	return 1;
}

static int calibration_reload_keymap(void)
{
	IDirectFB *dfb = lite_get_dfb_interface();
	IDirectFBInputDevice *device;
	DFBInputDeviceDescription desc;
	DFBResult dfbret;
	int i;

	for(i = 0; ; i++) {
	    dfbret = dfb->GetInputDevice(dfb, i, &device);
	    if (dfbret == DFB_OK) {
	 	dfbret = device->GetDescription(device, &desc);
		if(strcmp("tslib", desc.vendor) == 0) {
		
		    /* reload the keymap*/
		    dfbret = dfb_input_device_reload_keymap(dfb_input_device_at(i));
		    if(dfbret == DFB_OK) {
			break;
		    }

		    return -1;
		}
	    }
	}
	return 1;
}

static void calibration_destroy(struct calibration_container *cc)
{
	if(!cc) {
		return;
	}

	if(cc->ci) {
		calibration_window_instance_deref(cc->ci);
	}
	free(cc);
	return;
}

