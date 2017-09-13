/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file ao.c
 *
 * \brief Implementation of the AddObject RPC method
 */
#include <stdlib.h>
#include <string.h>

#include "tr.h"
#include "tr_lib.h"
#include "session.h"
#include "xml.h"
#include "tr_strings.h"
#include "log.h"
#include "atomic.h"
#include "ao.h"
#include "do.h"
#include "pkey.h"
#include "method.h"
#include "atomic.h"

#include "war_string.h"
/*!
 * \struct ao
 * The AddObject command
 */
static struct ao {
	int status; /*!<Indicates if or not the added instance is applied*/
	int fault_code; /*!<The fault code if any error occur*/
	uint32_t in; /*!<The instance number of the new added instance*/
} ao;

static int multi_instance_capable(node_t node);
static int instance_limit_reached(node_t node);

/*!
 * \brief Check if or not the node is multi-instances capable
 *
 * \param node The node to be checked
 * \return 1 when the node is multi instance capable, or return 0
 */
static int multi_instance_capable(node_t node)
{
	char il[PROPERTY_LENGTH];
	int res = 0;
	//Make use the "il" property to indicate if or not the node is multi instance capable
	if(lib_get_property(node, "il", il) == 0) {
		if(atoi(il) > 0 && node_is_writable(node) == 1)
			res = 1;
	}

	return res;
}

/*!
 * \brief Check if or not reach the instance number limitation
 * \param node The node to be checked
 * \return 1 if the limitation reached, or less return 0
 */
static int instance_limit_reached(node_t node)
{
	node_t *children = NULL;
	int count;
	int il;
	int number = 0;

	{
		char c_il[PROPERTY_LENGTH];
		if(lib_get_property(node, "il", c_il) < 0)
			return 0;
		il = atoi(c_il);
	}

	count = lib_get_children(node, &children);
	while(count > 0) {
		char name[PROPERTY_LENGTH];
		count--;
		if(lib_get_property(children[count], "name", name) == 0) {
			if(string_is_digits(name) == 1)
				number++;
		} else {
			break;
		}
	}

	if(children)
		lib_destroy_children(children);

	return number >= il;
}

/*!
 * \brief Add an object instance
 *
 * \param path The path to add the instance
 * \param path_len The length of the \a path
 *
 * \return  9002 if occurs any internal error, 9003 if the path is not ended with 
 * dot(.), 9004 if reach the instance limitation, or less return 9005.
 */
static int __ao__(char *path, int path_len)
{
	char npath[PROPERTY_LENGTH] = "";

	if(path_len <= 0 || path[path_len - 1] != '.' || path[0] == '.') {
		return 9005;
	} else {
		int res;
		node_t node;

		path[path_len - 1] = '\0';
		res = lib_resolve_node(path, &node);
		if(res < 0) {
			return 9002;
		} else if(res > 0 || multi_instance_capable(node) != 1) {
			return 9005;
		} else if(instance_limit_reached(node) == 1) {
			return 9004;
		} else {
			char nin[64] = "";
			res = lib_get_property(node, "nin", nin);
			if(res != 0) {
				return 9002;
			} else {
				ao.in = atoi(nin);
				//war_snprintf(path, sizeof(path) - path_len, ".%s", nin);//BUG changes
				war_snprintf(npath, PROPERTY_LENGTH, "%s.%s", path, nin);
				if(ao_journal(npath) != 0) { //Record journal, do not include the end '.' in the path
					return 9002;
				} else {
					int n;

					n = atoi(nin);
					ao.status = lib_ao(node, n);
					if(ao.status < 0) {
						return 9002;
					} else { //Update the nin property
						war_snprintf(nin, sizeof(nin), "%d", n + 1);
						lib_set_property(node, "nin", nin);
					}
				}
			}
		}
	}

	path[path_len - 1] = '.'; //If add OK, keep path unchanged
	return 0;
}

int add_object(char *path, int path_len)
{
	int ret;
	if(start_transaction() != 0) {
		tr_log(LOG_ERROR, "Start transaction failed!");
		return 9002;
	}
	ret = __ao__(path, path_len);
	//ret = 9001;
	if(ret == 0) {
		tr_log(LOG_DEBUG, "Add Object instance number is %d", ao.in);
		commit_transaction();
		return ao.in;
	} else {
		tr_log(LOG_WARNING, "__ao__ return %d and call rollback transaction\n", ret);
		rollback_transaction();
		return ret;
	}
}

int ao_process(struct session *ss, char **msg)
{
	struct xml tag;
	char pkey[33] = "";
	int found_pkey = 0;
	char path[256] = "";
	int found_path = 0;
	int len = 0;

	ao.fault_code = 0;

	if(start_transaction() != 0) {
		tr_log(LOG_ERROR, "Start transaction failed!");
		ao.fault_code = 9002;
		return METHOD_FAILED;
	}

	while(xml_next_tag(msg, &tag) == XML_OK) {
		if(war_strcasecmp(tag.name, "ObjectName") == 0) {
			len = war_snprintf(path, sizeof(path), "%s", tag.value ? tag.value : "");
			found_path = 1;
		} else if(war_strcasecmp(tag.name, "ParameterKey") == 0) {
			found_pkey = 1;
			war_snprintf(pkey, sizeof(pkey), "%s", tag.value ? tag.value : "");
		} else if(war_strcasecmp(tag.name, "/AddObject") == 0) {
			break;
		}
	}

	if(found_pkey && found_path) {
		ao.fault_code = __ao__(path, len);
	} else {
		ao.fault_code = 9003;
	}

	if(ao.fault_code == 0) {
		if(set_parameter_key(pkey) != 0) {
			ao.fault_code = 9002;
		}
	}


	if(ao.fault_code == 0) {
		commit_transaction();
		return METHOD_SUCCESSED;
	} else {
		rollback_transaction();
		return METHOD_FAILED;
	}
}


int ao_body(struct session *ss)
{
	struct buffer buf;

	init_buffer(&buf);
	if(ao.fault_code != 0) {
		push_buffer(&buf,
				"<FaultCode>%d</FaultCode>\n"
				"<FaultString>%s</FaultString>\n", ao.fault_code, fault_code2string(ao.fault_code));
	} else {
		push_buffer(&buf,
				"<InstanceNumber>%u</InstanceNumber>\n"
				"<Status>%d</Status>\n", ao.in, ao.status);
	}

	if(buf.data_len)
		push_buffer(&(ss->outbuf), "%x\r\n%s\r\n", buf.data_len, buf.data);

// 2011.04.14
	if(buf.data_len)
		push_buffer(&(ss->outbuf2_data), "%s", buf.data);

	destroy_buffer(&buf);

	return METHOD_COMPLETE;
}
