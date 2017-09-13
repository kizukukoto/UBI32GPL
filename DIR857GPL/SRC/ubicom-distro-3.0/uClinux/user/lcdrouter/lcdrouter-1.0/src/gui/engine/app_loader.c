/*
 * Ubicom Network Player
 *
 * (C) Copyright 2009, Ubicom, Inc.
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

#include <gui/engine/app_loader.h>

struct app_loader_instance *global_app_list = NULL;
struct app_loader_instance *global_current_app_p = NULL;

struct app_loader_instance {
	void *app_p;

	void *handle;
	char *lib_path;

	char *type;
	char *weight;
	char *info;
	char *parent;
	IDirectFBSurface *icon;

	void* (*create_fp)(DFBRectangle *rect, void *app_data);
	void* (*start_fp)(void *app_p);
	void* (*pause_fp)(void *app_p);

	void* (*get_shared_data_fp)(void *app_p);
	int (*set_shared_data_fp)(void *app_p, void *data);

	int refs;

	struct app_loader_instance *prev;	
	struct app_loader_instance *next;
#ifdef DEBUG
	int	magic;
#endif
};

void* (*query_fp)(void *app_p);

#if defined(DEBUG)
#define APP_LOADER_MAGIC 0xBAFF
#define APP_LOADER_VERIFY_MAGIC(ali) app_loader_verify_magic(ali)

void app_loader_verify_magic(struct app_loader_instance  *ali)
{
	DEBUG_ASSERT(ali->magic == APP_LOADER_MAGIC, "%p: bad magic", ali);
}
#else
#define APP_LOADER_VERIFY_MAGIC(app)
#endif

static
void* app_loader_instance_open_lib(char *lib)
{
	void *handle;
	handle = dlopen( lib, RTLD_LAZY );
	if (!handle) {
		DEBUG_ERROR("Lib %s open failed... (%s)", lib, dlerror());
		return NULL;
	}
	return handle;
}

static
void* app_loader_instance_get_method( void *handle, char *method)
{
	char *error;
	void *method_p;

	method_p = dlsym( handle, method );
	error = dlerror();
	if (error != NULL) {
		DEBUG_ERROR("Can not find %s method", method);
		return NULL;
	}
	return method_p;
}

static
void app_loader_instance_close_lib(void *handle)
{
	dlclose(handle);
}

int app_loader_instance_start(struct app_loader_instance *ali)
{
	APP_LOADER_VERIFY_MAGIC(ali);

	if( !ali->start_fp(ali->app_p) ) {
		DEBUG_ERROR("app start failed...");
		return 0;	
	}
	global_current_app_p = ali;
	return 1;
}

int app_loader_instance_pause(struct app_loader_instance *ali)
{
	APP_LOADER_VERIFY_MAGIC(ali);

	if( !ali->pause_fp(ali->app_p) ) {
		DEBUG_ERROR("app start failed...");
		return 0;	
	}
	return 1;
}

void* app_loader_instance_get_shared_data(struct app_loader_instance *ali)
{
	APP_LOADER_VERIFY_MAGIC(ali);
	
	if (!ali->get_shared_data_fp) {
		DEBUG_ERROR("no get_shared_data api included in app... %s", ali->info);
		return NULL;
	}

	return  ali->get_shared_data_fp(ali->app_p);	
}

int app_loader_instance_set_shared_data(struct app_loader_instance *ali, void *data)
{
	APP_LOADER_VERIFY_MAGIC(ali);
	
	if (!ali->set_shared_data_fp) {
		DEBUG_ERROR("no set_shared_data api included in app... %s", ali->info);
		return -1;
	}

	return  ali->set_shared_data_fp(ali->app_p, data);
}

void* app_loader_instance_get_app_p(struct app_loader_instance *ali)
{
	APP_LOADER_VERIFY_MAGIC(ali);

	return ali->app_p;
}

void* app_loader_instance_get_prev(struct app_loader_instance *ali)
{
	APP_LOADER_VERIFY_MAGIC(ali);

	return ali->prev;
}


struct app_loader_instance* app_loader_instance_find_app_by_name(struct app_loader_instance *ali, char *app_name)
{
	APP_LOADER_VERIFY_MAGIC(ali);

	struct app_loader_instance *tmp = ali;

	while(1) {
		if ( tmp == NULL ) {
			DEBUG_ERROR("global_app_list empty or item not found...");
			return NULL;
		}
		DEBUG_ERROR("check app %s with %s", tmp->info, app_name);
		if ( !strcmp(tmp->info, app_name) ||  !strcmp(tmp->lib_path, app_name) ) {
			DEBUG_ERROR("App found %s", app_name);
			return tmp;
		}		
		tmp = tmp->prev;
	}
}

int app_loader_instance_switch_to_app_by_name(struct app_loader_instance *ali, char *app_name)
{
	APP_LOADER_VERIFY_MAGIC(ali);

	struct app_loader_instance *tmp = app_loader_instance_find_app_by_name(ali, app_name);

	if ( tmp == NULL ) {
		DEBUG_ERROR("global_app_list empty or item not found...");
		return 0;
	}

	if (global_current_app_p->pause_fp) {
		if( !global_current_app_p->pause_fp(global_current_app_p->app_p) ) {
			DEBUG_ERROR("global_current_app_p pause failed...");
			return 0;	
		}
	}
	
	global_current_app_p = tmp;

	if( !global_current_app_p->start_fp(global_current_app_p->app_p) ) {
		DEBUG_ERROR("app start failed...");
		return 0;	
	}
	return 1;
}

static
int app_loader_instance_print_app_list(struct app_loader_instance *ali)
{
	APP_LOADER_VERIFY_MAGIC(ali);

	struct app_loader_instance *tmp = global_app_list;

	while(1) {
		if ( tmp == NULL ) {
			DEBUG_ERROR("global_app_list empty or item not found...");
			return 0;
		}
		DEBUG_ERROR("app = %s", tmp->info);		
		tmp = tmp->prev;
	}
}

struct app_loader_instance* app_loader_instance_create(char *lib_path, DFBRectangle *rect, void *app_data)
{
	struct app_loader_instance *ali = calloc(1, sizeof(struct app_loader_instance));

	if ( ali == NULL ) {
		DEBUG_ERROR("app loader instance failed");
		return NULL;
	}
	ali->refs = 1;
#ifdef DEBUG
	ali->magic = APP_LOADER_MAGIC;
#endif
	query_fp = NULL;

	ali->handle = app_loader_instance_open_lib( lib_path );
	
	if (ali->handle == NULL) {
		goto error;
	}
	
	ali->create_fp = app_loader_instance_get_method( ali->handle, "app_instance_create");
	
	if (ali->create_fp == NULL) {
		goto error;
	}

	DEBUG_ERROR("app created function-p = %p", ali->create_fp);
	
	ali->app_p = ali->create_fp(rect, app_data);

	DEBUG_ERROR("app pointer = %p", ali->app_p);
			
	if( ali->app_p == NULL ) {
		DEBUG_ERROR("app create failed...");
		goto error;
	} 

	ali->start_fp = app_loader_instance_get_method( ali->handle, "app_instance_start");

	DEBUG_ERROR("app start function-p = %p", ali->start_fp);
	
	if (ali->start_fp == NULL) {
		goto error;
	}

	ali->pause_fp = app_loader_instance_get_method( ali->handle, "app_instance_pause");

	DEBUG_ERROR("app pause function-p = %p", ali->pause_fp);
	
	if (ali->pause_fp == NULL) {
		goto error;
	}

	ali->get_shared_data_fp = app_loader_instance_get_method( ali->handle, "app_instance_get_shared_data");

	if (ali->get_shared_data_fp == NULL) {
		DEBUG_ERROR("app get_shared_data function-p = %p", ali->get_shared_data_fp);
	}

	ali->set_shared_data_fp = app_loader_instance_get_method( ali->handle, "app_instance_set_shared_data");

	if (ali->set_shared_data_fp == NULL) {
		DEBUG_ERROR("app set_shared_data function-p = %p", ali->set_shared_data_fp);
	}

	/* get info string from app*/
	query_fp = app_loader_instance_get_method( ali->handle, "app_instance_get_info");
	
	if (query_fp == NULL) {
		goto error;
	}
	ali->info = strdup((char*) query_fp(ali->app_p));
	DEBUG_ERROR("app name = %s . %p", ali->info, query_fp);

	/* get type string from app*/
	query_fp = app_loader_instance_get_method( ali->handle, "app_instance_get_type");
	
	if (query_fp == NULL) {
		goto error;
	}
	ali->type = (char*) query_fp(ali->app_p);

	/* get parent string from app*/
	query_fp = app_loader_instance_get_method( ali->handle, "app_instance_get_parent");
	
	if (query_fp == NULL) {
		goto error;
	}
	ali->parent = (char*) query_fp(ali->app_p);

	/* get weight string from app*/
	query_fp = app_loader_instance_get_method( ali->handle, "app_instance_get_weight");
	
	if (query_fp == NULL) {
		goto error;
	}
	ali->weight = (char*) query_fp(ali->app_p);

	/* get icon string from app*/	
	query_fp = app_loader_instance_get_method( ali->handle, "app_instance_get_icon");
	
	if (query_fp == NULL) {
		goto error;
	}
	ali->icon = (IDirectFBSurface*) query_fp(ali->app_p);
	
	ali->lib_path = strdup(lib_path);

	/* add app to global list*/
	ali->next = NULL;

	if(global_app_list == NULL) {
		global_app_list = ali;
		ali->prev = NULL;
	}
	else {
		ali->prev = global_app_list;
		global_app_list->next = ali;
		global_app_list = ali ;		
	}
	
	app_loader_instance_print_app_list(NULL);

	return ali;
error:
	if(ali->handle != NULL) {
		app_loader_instance_close_lib(ali->handle);
	}
	return NULL;
}

struct app_loader_instance* app_loader_instance_create_start(char *lib_path, DFBRectangle *rect, void *app_data) 
{

	struct app_loader_instance *ali = app_loader_instance_create(lib_path, rect, app_data);
	if( !ali ) {
		return ali;
	}
	if ( app_loader_instance_start(ali) ) {
		return ali;
	}
	return NULL;	
}

