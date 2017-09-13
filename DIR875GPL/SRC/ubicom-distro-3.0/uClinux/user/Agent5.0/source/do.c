/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file do.c
 *
 * \brief Implement the DeleteObject RPC method
 */
#include <stdlib.h>
#include <string.h>

#include "tr_lib.h"
#include "session.h"
#include "xml.h"
#include "tr_strings.h"
#include "log.h"
#include "atomic.h"
#include "do.h"
#include "pkey.h"
#include "method.h"
#include "atomic.h"
#include "war_string.h"

static int node_is_instance(node_t node);
static int node_is_object(node_t node);
static int backup_object(node_t node);
static int backup_parameter(node_t node);
#ifndef TR196
static int backup_subtree(node_t node);
#endif

/*!
 * \brief Check if or not a node is an instance
 *
 * \param node The node
 *
 * \return 1 if yes, or return 0
 * \remark As TR-069 protocol define, an writable interior object whose name just contains digit
 * is an instance which created by AddObject RPC method.
 */
static int node_is_instance(node_t node)
{
	char name[PROPERTY_LENGTH];
	int res = 0;

	if(node_is_writable(node) && node_is_object(node)
			&& lib_get_property(node, "name", name) == 0 && string_is_digits(name))
		res = 1;

	return res;
}

/*!
 * \brief Check if or not a node is an interior node
 *
 * \param node The node
 *
 * \return 1 if the node is an interior, or less return 0
 */
static int node_is_object(node_t node)
{
	char type[PROPERTY_LENGTH];
	int res = 0;

	if(lib_get_property(node, "type", type) == 0 && war_strcasecmp(type, "node") == 0)
		res = 1;

	return res;
}

/*!
 * \brief Back up a sub tree from the instance node
 * An instance can be deleted by DeleteObject RPC method. When ACS call this method to delete an
 * instance from the device's MOT, it will delete the whole subtree of the instance. We MUST
 * backup(write journal) the whole subtree to make sure the operation to be executed atomically.
 *
 * \param node The instance node
 *
 * \return 0 when success, -1 when any error
 */
static int backup_object(node_t node)
{
	node_t *children = NULL;
	int count;
	char *path;
	int len;

	count = lib_get_children(node, &children);
	while(count > 0) {
		count--;
		if(backup_subtree(children[count]) != 0) {
			lib_destroy_children(children);
			return -1;
		}
	}
	if(children)
		lib_destroy_children(children);
	path = lib_node2path(node);
	len = strlen(path);
	if(len > 0)
		path[len - 1] = '\0'; //Delete the end '.'
	

	if(node_is_writable(node)) {
		char nin[PROPERTY_LENGTH];
		if(lib_get_property(node, "nin", nin) != 0)
			return -1;
		else if(spa_nin_journal(path, nin) != 0) {
			return -1;
		}
	}

	if(node_is_instance(node)) {
		if(do_journal(path) != 0)
			return -1;
	}

	return 0;
}

/*!
 * \brief Backup a parameter
 * Because after an instance was added into the MOT, the ACS can call SetParameterValues and
 * SetParameterAttributes to change the parameter's default value and default attributes. So,
 * before we delete it from the MOT, we MUST backup the current value.
 *
 * \param node The parameter node
 * \return 0 when success, -1 when any error
 */
static int backup_parameter(node_t node)
{

	char *path;
	path = lib_node2path(node);
	if(node_is_writable(node)) {
		//Only a writable parameter's value can be change, so we just backup those
		//parameters'value
		char *value = NULL;
		if(lib_get_value(node, &value) != 0 || spv_journal(path, value) != 0) {
			if(value)
				lib_destroy_value(value);
			return -1;
		}
		if(value)
			lib_destroy_value(value);
	}
	{
		//Backup the NOC attribute
		char noc[PROPERTY_LENGTH];
		if(lib_get_property(node, "noc", noc) != 0 || spa_noc_journal(path, noc) != 0)
			return -1;
	}
	{
		//Backup the ACL attribute
		char acl[PROPERTY_LENGTH];
		//if(lib_get_property(node, "acl", acl) != 0 || spa_acl_journal(path, acl) != 0)
		if(lib_get_property(node, "acl", acl) != 0) {
			return -1;
		}
		if(spa_acl_journal(path, acl) != 0)
			return -1;
	}

	return 0;
}


/*!
 * \brief Backup a subtree
 *
 * \param node The instance node
 *
 * \return 0 when success, -1 when any error
 */

#ifdef TR196
int backup_subtree(node_t node)
#else
static int backup_subtree(node_t node)
#endif
{
	int res;
	if(node_is_object(node)) {
		res = backup_object(node);
	} else {
		res = backup_parameter(node);
	}

	return res;
}


int node_is_writable(node_t node)
{
	char rw[PROPERTY_LENGTH];
	int res = 0;

	if(lib_get_property(node, "rw", rw) == 0 && string2boolean(rw) == BOOLEAN_TRUE) {
		res = 1;
	}

	return res;
}


static int __do__(char *path, int path_len)
{

	if(path_len <= 0 || path[path_len - 1] != '.' || path[0] == '.') {
		return 9005;
	} else {
		int res;
		node_t node;

		path[path_len - 1] = '\0';
		res = lib_resolve_node(path, &node);
		if(res < 0) {
			return 9002;
		} else if(res > 0) {
			return 9005;
		} else {
			if(node_is_instance(node)) {
				if(backup_subtree(node) != 0 || lib_do(node) != 0)
					return 9002;
			} else {
				return 9005;
			}
		}
	}

	return 0;
}

int delete_object(char *path, int path_len)
{
	int ret = -1;
	if(start_transaction() != 0) {
		tr_log(LOG_ERROR, "Start transaction failed!");
		return 9002;
	}
	ret = __do__(path, path_len);
	//ret = 9001;
	if(ret == 0) {
		tr_log(LOG_DEBUG, "Delete Object OK");
		commit_transaction();
		return 0;
	} else {
		tr_log(LOG_WARNING, "__do__ return %d and call rollback transaction\n", ret);
		rollback_transaction();
		return ret;
	}
}

int do_process(struct session *ss, char **msg)
{
	struct xml tag;
	char pkey[33] = "";
	int found_pkey = 0;
	char path[256] = "";
	int found_path = 0;
	int len = 0;

	ss->cpe_pdata = (void *)0;

	if(start_transaction() != 0) {
		tr_log(LOG_ERROR, "Start transaction failed!");
		ss->cpe_pdata = (void *)9002;
		return METHOD_FAILED;
	}

	while(xml_next_tag(msg, &tag) == XML_OK) {
		if(war_strcasecmp(tag.name, "ObjectName") == 0) {
			len = war_snprintf(path, sizeof(path), "%s", tag.value ? tag.value : "");
			found_path = 1;
		} else if(war_strcasecmp(tag.name, "ParameterKey") == 0) {
			war_snprintf(pkey, sizeof(pkey), "%s", tag.value ? tag.value : "");
			found_pkey = 1;
		} else if(war_strcasecmp(tag.name, "/DeleteObject") == 0) {
			break;
		}
	}

	if(found_path && found_pkey) {
		ss->cpe_pdata = (void *)__do__(path, len);
	} else {
		ss->cpe_pdata = (void *)9005;
	}

	if(ss->cpe_pdata == (void *)0) {
		if(set_parameter_key(pkey) != 0) {
			ss->cpe_pdata = (void *)9002;
		}
	}

	if(ss->cpe_pdata == (void *)0) {
		commit_transaction();
		return METHOD_SUCCESSED;
	} else {
		rollback_transaction();
		return METHOD_FAILED;
	}
}


int do_body(struct session *ss)
{
	struct buffer buf;

	init_buffer(&buf);
	if(ss->cpe_pdata != (void *)0) {
		push_buffer(&buf,
				"<FaultCode>%d</FaultCode>\n"
				"<FaultString>%s</FaultString>\n", (int)(ss->cpe_pdata), fault_code2string((int)(ss->cpe_pdata)));
	} else {
		push_buffer(&buf, "<Status>1</Status>\n");
	}

	if(buf.data_len)
		push_buffer(&(ss->outbuf), "%x\r\n%s\r\n", buf.data_len, buf.data);

// 2011.04.14
	if(buf.data_len)
		push_buffer(&(ss->outbuf2_data), "%s", buf.data);


	destroy_buffer(&buf);

	return METHOD_COMPLETE;
}
