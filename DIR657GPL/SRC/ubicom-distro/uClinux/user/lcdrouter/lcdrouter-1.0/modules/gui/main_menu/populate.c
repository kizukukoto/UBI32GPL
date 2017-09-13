#include "populate.h"

/*
 * create_and_populate_menu_data
 */
struct menu_data_instance* create_and_populate_menu_data() 
{
	struct menu_data_instance *menu_data = menu_data_instance_create();
	if (!menu_data) {
		DEBUG_WARN("md alloc failed");
		return NULL;
	}

	//populate header first
	char *tmp = strdup("Main Menu");
	struct list_item* list_item = menu_data_instance_list_item_open(menu_data);	
	menu_data_instance_add_metadata(menu_data, list_item, HEADER_TEXT, strlen(tmp)+1, tmp);
	menu_data_instance_list_item_close(menu_data, list_item);
	free(tmp);

	tmp = strdup("network");
        list_item = menu_data_instance_list_item_open(menu_data);
	menu_data_instance_add_metadata(menu_data, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
//	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_XXX(menu_data));
//	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_list_item_close(menu_data, list_item);
	free(tmp);

	tmp = strdup("wireless");
        list_item = menu_data_instance_list_item_open(menu_data);
	menu_data_instance_add_metadata(menu_data, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
//	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_YYY(menu_data));
//	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_add_metadata(menu_data, list_item, APP_URL , strlen(MODULES_PATH "/network_map.so")+1, MODULES_PATH "/network_map.so");
	menu_data_instance_set_listitem_flag(list_item, IS_APP);
	menu_data_instance_list_item_close(menu_data, list_item);
	free(tmp);

	tmp = strdup("status");
        list_item = menu_data_instance_list_item_open(menu_data);
	menu_data_instance_add_metadata(menu_data, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_status());
	menu_data_instance_add_metadata(menu_data, list_item, APP_URL , strlen(MODULES_PATH "/network_status.so")+1, MODULES_PATH "/network_status.so");
	menu_data_instance_set_listitem_flag(list_item, IS_APP);
	menu_data_instance_list_item_close(menu_data, list_item);
	free(tmp);

	tmp = strdup("alerts");
        list_item = menu_data_instance_list_item_open(menu_data);
	menu_data_instance_add_metadata(menu_data, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_firmware());
//	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_ZZZ(menu_data));
	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_list_item_close(menu_data, list_item);
	free(tmp);

	//menu_data_instance_dump_metadata_chunk_memory(menu_data);
	//menu_data_instance_dump_listitem_chunk_memory(menu_data);
	
	return menu_data;
}

