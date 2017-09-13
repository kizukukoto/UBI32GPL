/*
 * Ubicom Network Player
 *
 * (C) Copyright 2009, Ubicom, Inc.
 * Celil URGAN curgan@ubicom.com
 *
 * The Network Player Reference Code is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Network Player Reference Code is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Network Audio Device Reference Code.  If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */
#define GRID_H
#ifdef GRID_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include <lite/lite.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/textbutton.h>
#include <leck/button.h>
#include <leck/list.h>
#include <leck/image.h>
#include <leck/check.h>
#include <leck/label.h>
#include <string.h>

#include <directfb.h>

#include <debug.h>

#define ANIMATION_ENABLED
#ifdef ANIMATION_ENABLED
#include <gui/engine/animation.h>
#endif

#include <gui/mm/menu_data.h>
#include <gui/widgets/scroll.h>
#include <gui/widgets/header.h>

#define DEBUG

#define GRID_C2C_SPACE 10// pixels between two components
#define GRID_C2B_SPACE 10// pixels between component and box edge

#define ITEM_HEIGHT_FOR_GRID                   85
#define ITEM_WIDTH_FOR_GRID                    110
#define ITEM_PER_PAGE_FOR_GRID                  4

struct grid_instance;

typedef void (*grid_instance_on_item_select) (struct grid_instance *gi, void *app_data);

DFBResult grid_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct grid_instance **ret_gi);
void grid_instance_set_menu_data(struct grid_instance *gi, struct menu_data_instance *menu_data);
struct menu_data_instance* grid_instance_get_menu_data(struct grid_instance *gi);

void grid_instance_set_on_item_select(struct grid_instance *gi, grid_instance_on_item_select func, void *app_data);
void grid_instance_attach_scrollbar(struct grid_instance *gi, struct scroll_instance *si);

void grid_instance_set_header_instance(struct grid_instance *gi, struct header_instance *hi);

int grid_instance_get_current_item(struct grid_instance *gi);

int grid_instance_next_page(struct grid_instance *gi);
int grid_instance_prev_page(struct grid_instance *gi);

int  grid_instance_ref(struct grid_instance *gi);
int  grid_instance_deref(struct grid_instance *gi);

#endif
