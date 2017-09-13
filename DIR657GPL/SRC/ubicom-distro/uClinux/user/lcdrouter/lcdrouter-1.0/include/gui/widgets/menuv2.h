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
#define MENU_H
#ifdef MENU_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>

#include <lite/lite.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/button.h>
#include <leck/textbutton.h>
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

#define MENU_C2C_SPACE 5// pixels between two components
#define MENU_C2B_SPACE 5// pixels between component and box edge

#define ITEM_IMAGE_WIDTH    55
#define ITEM_IMAGE_HEIGHT   55

#define ITEM_HEIGHT_FOR_LIST_WITH_IMAGES       55
#define ITEM_PER_PAGE_FOR_LIST_WITH_IMAGES       3

struct menu_instance;

typedef void (*menu_instance_on_item_select) (struct menu_instance *mi, void *app_data);
typedef void (*menu_instance_on_prev_menu) (struct menu_instance *mi, void *app_data);

DFBResult menu_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct menu_instance **ret_mi);
void menu_instance_set_menu_data(struct menu_instance *mi, struct menu_data_instance *menu_data);
struct menu_data_instance* menu_instance_get_menu_data(struct menu_instance *mi);

void menu_instance_set_on_item_select(struct menu_instance *mi, menu_instance_on_item_select func, void *app_data);
void menu_instance_set_on_prev_menu(struct menu_instance *mi, menu_instance_on_prev_menu func, void *app_data);
void menu_instance_attach_scrollbar(struct menu_instance *mi, struct scroll_instance *si);

void menu_instance_set_current_item(struct menu_instance *mi, int id);
int menu_instance_get_current_item(struct menu_instance *mi);

void menu_instance_set_header_instance(struct menu_instance *mi, struct header_instance *hi);

int menu_instance_next_page(struct menu_instance *mi);
int menu_instance_prev_page(struct menu_instance *mi);

int  menu_instance_ref(struct menu_instance *mi);
int  menu_instance_deref(struct menu_instance *mi);

#endif
