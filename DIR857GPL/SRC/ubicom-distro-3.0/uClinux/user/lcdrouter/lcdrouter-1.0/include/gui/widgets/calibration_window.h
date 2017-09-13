/*
 * Ubicom Network Player
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
#ifndef CALIBRATION_WINDOW_H
#define CALIBRATION_WINDOW_H
#include <gui/gui.h>
#include <debug.h>
#include <lite/lite.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/textbutton.h>
#include <leck/list.h>
#include <leck/image.h>
#include <leck/check.h>
#include <leck/label.h>
#include <leck/button.h>
#include <leck/textline.h>
#define DEBUG

struct calibration_window_instance;

extern DFBResult calibration_window_instance_new_window(struct calibration_window_instance **ret_ci);
extern int calibration_window_instance_ref(struct calibration_window_instance *ci);
extern int calibration_window_instance_deref(struct calibration_window_instance *ci);
extern DFBResult calibration_window_instance_set_text(struct calibration_window_instance *ci, char *text);
extern DFBResult calibration_window_instance_on_raw_window_mouse(struct calibration_window_instance *ci, LiteWindowMouseFunc func, void *data);
extern DFBResult calibration_window_instance_place_icons(struct calibration_window_instance *ci, DFBRectangle rects[], int number, const char *path);
extern DFBResult calibration_window_instance_show_icon_by_id(struct calibration_window_instance *ci, int id);
extern DFBResult calibration_window_instance_hide_icon_by_id(struct calibration_window_instance *ci, int id);
extern void calibration_window_instance_get_coordinates(struct calibration_window_instance *ci, int *width, int *height);

#endif
