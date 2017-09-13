/*
 * Ubicom Network Player
 *
 * (C) Copyright 2009, Ubicom, Inc.
 * Celil URGAN curgan@ubicom.com
 * Ergun CELEBIOGLU ecelebioglu@ubicom.com
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
#ifndef MENU_DATA_H
#define MENU_DATA_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <debug.h>

struct list_item;//16 byte packed

struct menu_data_instance;

enum menu_data_types {
	HEADER_RIGHT_ICON = 0,
	HEADER_LEFT_ICON,
	HEADER_TEXT,

	LIST_RIGHT_ICON = 10,
	LIST_LEFT_ICON,
	LIST_TEXT1,
	LIST_TEXT2,
	LIST_TEXT3,
	LIST_TEXT4,
	LIST_TEXT5,
	LIST_TEXT6,
	LIST_URL,
	APP_URL,
};

typedef enum menu_data_types menu_data_types_t;

enum listitem_status {
	MAY_HAVE_SUBMENU = 0x00000001,
	IS_PLAYABLE      = 0x00000002,
	IS_NOW_PLAYING   = 0x00000003,
	IS_APP           = 0x00000004,
};

typedef enum listitem_status listitem_status_t;

typedef void (*menu_data_instance_callback_t) (struct menu_data_instance *mdi, int id, void *data);

struct menu_data_instance* menu_data_instance_create();
int  menu_data_instance_ref(struct menu_data_instance *mdi);
int  menu_data_instance_deref(struct menu_data_instance *mdi);



int menu_data_instance_get_list_item_count(struct menu_data_instance *mdi);

void menu_data_instance_set_prev_menu(struct menu_data_instance *mdi, struct menu_data_instance *prev);
struct menu_data_instance* menu_data_instance_get_prev_menu(struct menu_data_instance *mdi);

unsigned short menu_data_instance_get_focused_item(struct menu_data_instance *mdi);
void menu_data_instance_set_focused_item(struct menu_data_instance *mdi, unsigned short item_no);

void menu_data_instance_set_listitem_flag(struct list_item *li, int flag);
int menu_data_instance_get_listitem_flag(struct list_item *li);

void menu_data_instance_set_listitem_submenu(struct list_item *li, struct menu_data_instance *mdi);
struct menu_data_instance* menu_data_instance_get_listitem_submenu(struct list_item *li);

void  menu_data_instance_set_listitem_callback(struct list_item *li, void *callback);
void* menu_data_instance_get_listitem_callback(struct list_item *li);
void  menu_data_instance_set_listitem_callbackdata(struct list_item *li, void *callbackdata);
void* menu_data_instance_get_listitem_callbackdata(struct list_item *li);

struct list_item* menu_data_instance_list_item_open(struct menu_data_instance *mdi);
int menu_data_instance_add_metadata(struct menu_data_instance *mdi, struct list_item *li, menu_data_types_t type, int len, void *data);
int menu_data_instance_list_item_close(struct menu_data_instance *mdi, struct list_item *li);

struct list_item* menu_data_instance_get_list_item(struct menu_data_instance *mdi, int index);
char* menu_data_instance_get_metadata(struct list_item *li, menu_data_types_t type);

int menu_data_instance_update_metadata(struct list_item *li, menu_data_types_t type, char *data, int len);

void menu_data_instance_dump_metadata_chunk_memory(struct menu_data_instance *mdi);
void menu_data_instance_dump_listitem_chunk_memory(struct menu_data_instance *mdi);

#endif

