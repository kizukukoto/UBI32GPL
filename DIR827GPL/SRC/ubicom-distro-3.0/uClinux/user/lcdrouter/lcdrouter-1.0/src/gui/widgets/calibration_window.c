/*
 *  Calibration Window GUI Component
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
#include <gui/widgets/calibration_window.h>

struct calibration_window_instance{
	LiteWindow *window;
	LiteLabel *text;
	LiteWindowMouseFunc mouse_func;
	LiteImage **spots;
	int refs;
#ifdef DEBUG
	int	magic;
#endif
};

#if defined(DEBUG)
#define CALIBRATION_WINDOW_MAGIC 0x3245
#define CALIBRATION_WINDOW_VERIFY_MAGIC(ci) calibration_window_verify_magic(ci)
static void calibration_window_verify_magic(struct calibration_window_instance *ci)
{
	DEBUG_ASSERT(ci->magic == CALIBRATION_WINDOW_MAGIC, "%p: bad magic", ci);
}
#else
#define CALIBRATION_WINDOW_VERIFY_MAGIC(ki)
#endif

static void calibration_window_instance_destroy(struct calibration_window_instance *ci)
{
	DEBUG_ASSERT(ci != NULL, " calibration_window destroy with NULL instance ");
#ifdef DEBUG
	ci->magic = 0;
#endif
	lite_destroy_window(LITE_WINDOW(ci->window));
	free(ci);
	ci = NULL;
}

int calibration_window_instance_ref(struct calibration_window_instance *ci)
{
	CALIBRATION_WINDOW_VERIFY_MAGIC(ci);
	return ++ci->refs;
}

int calibration_window_instance_deref(struct calibration_window_instance *ci)
{
	CALIBRATION_WINDOW_VERIFY_MAGIC(ci);
	DEBUG_ASSERT(ci->refs, "%p: calibration window has no references", ci);

	ci->refs--;
	DEBUG_TRACE("%p: deref - %d", ci, ci->refs);

	if (ci->refs) {
		return ci->refs;
	}
	calibration_window_instance_destroy(ci);
	return 0;
}

static DFBResult calibration_window_instance_box_destroy(LiteBox *box)
{
	struct calibration_window_instance *ci = (struct calibration_window_instance*)box;
	CALIBRATION_WINDOW_VERIFY_MAGIC(ci);
	DEBUG_TRACE("%p: calibration_window widget destroy called from GUI loop", ci);
	calibration_window_instance_deref(ci);
	return DFB_OK;
}

DFBResult calibration_window_instance_new_window(struct calibration_window_instance **ret_ci)
{
	DFBResult res = DFB_FAILURE;

	struct calibration_window_instance *ci = calloc(1, sizeof(struct calibration_window_instance));
	if (!ci) {
		DEBUG_ERROR("create new calibration window failed");
		return res;
	}

	ci->refs = 1;
#ifdef DEBUG
	ci->magic = CALIBRATION_WINDOW_MAGIC;
#endif

	DFBRectangle rect;
	
	rect.x =  0;
	rect.y =  0;
	rect.w = 320;
	rect.h = 240;
	res = lite_new_window( NULL, &rect, DWCAPS_ALPHACHANNEL, liteNoWindowTheme, "", &ci->window);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("%p: create new calibration window failed", ci);
		goto error;
	}

	lite_set_window_background_color(ci->window, 0x11, 0x11, 0x11, 0xbb);
	ci->window->window->SetOpaqueRegion(ci->window->window, 0, 0, 0, 0);
	lite_window_set_modal(ci->window, 1);
	lite_set_window_opacity(ci->window, liteFullWindowOpacity);
	
	rect.x = 5;
	rect.y = 70;
	rect.w = 315; 
	rect.h = 42;
	res = lite_new_label(LITE_BOX(ci->window), &rect, liteNoLabelTheme, 22, &ci->text );
	if ( res != DFB_OK ) {
		DEBUG_ERROR("%p: create popup window label failed", ci);
		goto error;
	}
	lite_set_label_alignment(ci->text, LITE_LABEL_CENTER);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("%p: set popup window label alignement failed", ci);
		goto error;
	}
	DFBColor text_color = { 0x00, 0xFF, 0xFF, 0xFF};
	lite_set_label_color(ci->text, &text_color);
	if ( res != DFB_OK ) {
		DEBUG_ERROR("%p: set popup window label color failed", ci);
		goto error;
	}
	*ret_ci = ci;
	return res;
error:
	calibration_window_instance_destroy(ci);
	return res;
}

DFBResult calibration_window_instance_set_text(struct calibration_window_instance *ci, char *text)
{
	CALIBRATION_WINDOW_VERIFY_MAGIC(ci);
	return lite_set_label_text(ci->text, text);
}

DFBResult calibration_window_instance_on_raw_window_mouse(struct calibration_window_instance *ci, LiteWindowMouseFunc func, void *data)
{
	CALIBRATION_WINDOW_VERIFY_MAGIC(ci);
	ci->mouse_func = func;
	return lite_on_raw_window_mouse(ci->window, ci->mouse_func, data);
}

DFBResult calibration_window_instance_place_icons(struct calibration_window_instance *ci, DFBRectangle rects[], int number, const char *path)
{
	CALIBRATION_WINDOW_VERIFY_MAGIC(ci);
	int i = 0;
	DFBRectangle *rect;
	DFBResult res;

	if(!path) {
		goto error;
	}

	if(!number) {
		goto error;
	}

	/* Last +1 for NUL character */
	ci->spots = calloc(1, sizeof(LiteImage *)*number + 1);
	if(!ci->spots) {
		DEBUG_ERROR("ci->spots allocation failed");
		goto error;
	}

	for(;i < number; i++) {
		rect = &rects[i];

		res = lite_new_image(LITE_BOX(ci->window), rect, liteNoImageTheme, &ci->spots[i]);
		if(res != DFB_OK) {
		    goto error;
		}

		res = lite_set_image_visible(ci->spots[i], 0);
		if(res != DFB_OK) {
			goto error;
		}

		res = lite_load_image(ci->spots[i], path);
		if(res != DFB_OK) {
			goto error;
		}
	}
	return DFB_OK;

  error:
	return DFB_FAILURE;
}

DFBResult calibration_window_instance_show_icon_by_id(struct calibration_window_instance *ci, int id)
{
	return lite_set_image_visible(ci->spots[id], 1);
}

DFBResult calibration_window_instance_hide_icon_by_id(struct calibration_window_instance *ci, int id)
{
	return lite_set_image_visible(ci->spots[id], 0);
}

void calibration_window_instance_get_coordinates(struct calibration_window_instance *ci, int *width, int *height)
{
	*width = ci->window->width;
	*height = ci->window->height;
}
 
