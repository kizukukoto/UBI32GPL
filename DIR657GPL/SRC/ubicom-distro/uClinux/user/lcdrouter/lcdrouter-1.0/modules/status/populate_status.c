#include "populate_status.h"

/*
 * populate_and_create_network_status
 *//*
struct menu_data_instance* populate_and_create_network_status(struct menu_data_instance *menu_data)
{
	struct menu_data_instance *network_status_md = menu_data_instance_create();
	if (!network_status_md) {
		DEBUG_WARN("md alloc failed");
		return NULL;
	}
	menu_data_instance_set_prev_menu(network_status_md, menu_data);

	//populate header first
	char *tmp = strdup("Network Status");
	struct list_item* list_item = menu_data_instance_list_item_open(network_status_md);
	menu_data_instance_add_metadata(network_status_md, list_item, HEADER_TEXT, strlen(tmp)+1,tmp);
	menu_data_instance_list_item_close(network_status_md, list_item);
	free(tmp);

	tmp = strdup("network_status");
    list_item = menu_data_instance_list_item_open(network_status_md);
	menu_data_instance_add_metadata(network_status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
	menu_data_instance_add_metadata(network_status_md, list_item, APP_URL , strlen(MODULES_PATH "/network_status.so")+1, MODULES_PATH "/network_status.so");
	menu_data_instance_set_listitem_flag(list_item, IS_APP);
	menu_data_instance_list_item_close(network_status_md, list_item);
	free(tmp);

	tmp = strdup("TBD");
	list_item = menu_data_instance_list_item_open(network_status_md);
	menu_data_instance_add_metadata(network_status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
//	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_XXX(netwok_status_md));
//	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_list_item_close(network_status_md, list_item);
	free(tmp);

	return network_status_md;
}
*/
/*
 * populate_and_create_device_status
 */
struct menu_data_instance* populate_and_create_device_status(struct menu_data_instance *menu_data)
{
	struct menu_data_instance *device_status_md = menu_data_instance_create();
	if (!device_status_md) {
		DEBUG_WARN("md alloc failed");
		return NULL;
	}
	menu_data_instance_set_prev_menu(device_status_md, menu_data);

	//populate header first
	char *tmp = strdup("Device Status");
	struct list_item* list_item = menu_data_instance_list_item_open(device_status_md);
	menu_data_instance_add_metadata(device_status_md, list_item, HEADER_TEXT, strlen(tmp)+1,tmp);
	menu_data_instance_list_item_close(device_status_md, list_item);
	free(tmp);

	tmp = strdup("TBD");
	list_item = menu_data_instance_list_item_open(device_status_md);
	menu_data_instance_add_metadata(device_status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
//	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_XXX(device_status_md));
//	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_list_item_close(device_status_md, list_item);
	free(tmp);

	tmp = strdup("Calibration");
	list_item = menu_data_instance_list_item_open(device_status_md);
	menu_data_instance_add_metadata(device_status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
	menu_data_instance_set_listitem_callback(list_item, calibration_set_callback);
	menu_data_instance_list_item_close(device_status_md, list_item);
	free(tmp);	

	return device_status_md;
}

/*
 * populate_and_create_status
 */
struct menu_data_instance* populate_and_create_status()
{
	struct menu_data_instance *status_md = menu_data_instance_create();
	if (!status_md) {
		DEBUG_WARN("md alloc failed");
		return NULL;
	}
	//populate header first
	char *tmp = strdup("Status");
	struct list_item *list_item = menu_data_instance_list_item_open(status_md);
	menu_data_instance_add_metadata(status_md, list_item, HEADER_TEXT, strlen(tmp)+1,tmp);
	menu_data_instance_list_item_close(status_md, list_item);
	free(tmp);

	tmp = strdup("Network Status");
    list_item = menu_data_instance_list_item_open(status_md);
	menu_data_instance_add_metadata(status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
	menu_data_instance_add_metadata(status_md, list_item, APP_URL , strlen(MODULES_PATH "/network_status.so")+1, MODULES_PATH "/network_status.so");
	menu_data_instance_set_listitem_flag(list_item, IS_APP);
	menu_data_instance_list_item_close(status_md, list_item);
	free(tmp);

	tmp = strdup("Device");
	list_item = menu_data_instance_list_item_open(status_md);
	menu_data_instance_add_metadata(status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
	menu_data_instance_set_listitem_submenu(list_item, populate_and_create_device_status(status_md));
	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_list_item_close(status_md, list_item);
	free(tmp);

	return status_md;
       
}



/*
 * populate_and_create_status
 */
struct menu_data_instance* populate_and_create_firmware()
{
	struct menu_data_instance *status_md = menu_data_instance_create();
	if (!status_md) {
		DEBUG_WARN("md alloc failed");
		return NULL;
	}
	//populate header first
	char *tmp = strdup("Firmware Upgrade");
	struct list_item *list_item = menu_data_instance_list_item_open(status_md);
	menu_data_instance_add_metadata(status_md, list_item, HEADER_TEXT, strlen(tmp)+1,tmp);
	menu_data_instance_list_item_close(status_md, list_item);
	free(tmp);

	tmp = strdup("Yes - FW Upgrade to Rev 1.1");
	list_item = menu_data_instance_list_item_open(status_md);
	menu_data_instance_add_metadata(status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
//	menu_data_instance_add_metadata(status_md, list_item, APP_URL , strlen(MODULES_PATH "/dummy_return.so")+1, MODULES_PATH "/dummy_return.so");
	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_list_item_close(status_md, list_item);
	free(tmp);

	tmp = strdup("No");
	list_item = menu_data_instance_list_item_open(status_md);
	menu_data_instance_add_metadata(status_md, list_item, LIST_TEXT1, strlen(tmp)+1, tmp);
//	menu_data_instance_add_metadata(status_md, list_item, APP_URL , strlen(MODULES_PATH "/dummy_return.so")+1, MODULES_PATH "/dummy_return.so");
	menu_data_instance_set_listitem_flag(list_item, MAY_HAVE_SUBMENU);
	menu_data_instance_list_item_close(status_md, list_item);
	free(tmp);

	return status_md;
       
}

