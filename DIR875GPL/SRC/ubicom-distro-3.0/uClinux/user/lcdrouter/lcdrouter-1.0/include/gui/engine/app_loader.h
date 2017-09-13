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
#ifndef APPLOADER_H
#define APPLOADER_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include <lite/lite.h>
#include <lite/box.h>
#include <lite/window.h>
#include <lite/util.h>

#include <string.h>

#include <directfb.h>

#include <debug.h>

struct app_loader_instance;

extern struct app_loader_instance *global_app_list;

int app_loader_instance_start(struct app_loader_instance *ali);
int app_loader_instance_pause(struct app_loader_instance *ali);
void* app_loader_instance_get_prev(struct app_loader_instance *ali);
struct app_loader_instance* app_loader_instance_create(char *lib_path, DFBRectangle *rect, void *app_data);
struct app_loader_instance* app_loader_instance_create_start(char *lib_path, DFBRectangle *rect, void *app_data);
void* app_loader_instance_get_app_p(struct app_loader_instance *ali);

int app_loader_instance_switch_to_app_by_name(struct app_loader_instance *ali, char *app_name);
struct app_loader_instance* app_loader_instance_find_app_by_name(struct app_loader_instance *ali, char *app_name);

void* app_loader_instance_get_shared_data(struct app_loader_instance *ali);
int app_loader_instance_set_shared_data(struct app_loader_instance *ali, void *data);

#endif

