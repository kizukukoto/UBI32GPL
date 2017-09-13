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
#define LISTBOX_H
#ifdef LISTBOX_H

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

#define C2C_SPACE 5// pixels between two components
#define C2B_SPACE 5// pixels between component and box edge

#define ITEM_HEIGHT   25

#define ITEM_PER_PAGE 6

struct listbox_instance;

typedef void (*listbox_instance_on_item_select) (struct listbox_instance *li, void *app_data);
typedef void (*listbox_instance_on_go_back) (struct listbox_instance *li, void *app_data);

DFBResult listbox_instance_alloc(LiteBox *parent, DFBRectangle *rect, struct listbox_instance **ret_li);
void listbox_instance_set_menu_data(struct listbox_instance *li, struct menu_data_instance *menu_data);
struct menu_data_instance* listbox_instance_get_menu_data(struct listbox_instance *li);

void listbox_instance_set_on_item_select(struct listbox_instance *li, listbox_instance_on_item_select func, void *app_data);
void listbox_instance_set_on_go_back(struct listbox_instance *li, listbox_instance_on_go_back func, void *app_data);
void listbox_instance_attach_scrollbar(struct listbox_instance *li, struct scroll_instance *si);

int listbox_instance_get_current_item(struct listbox_instance *li);
void listbox_instance_set_current_item(struct listbox_instance *li, int id);

void listbox_instance_set_header_instance(struct listbox_instance *li, struct header_instance *hi);
int  listbox_instance_ref(struct listbox_instance *li);
int  listbox_instance_deref(struct listbox_instance *li);

#endif
