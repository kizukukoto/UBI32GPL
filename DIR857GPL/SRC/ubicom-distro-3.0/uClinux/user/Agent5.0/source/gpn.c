/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file gpn.c
 * \brief The implementation of GetParameterNames RPC method
 */
#include <stdlib.h>
#include <string.h>

#include "tr_strings.h"
#include "tr_lib.h"
#include "session.h"
#include "xml.h"
#include "log.h"
#include "method.h"
#include "war_string.h"

struct gpn_node {
	node_t node;
	struct gpn_node *next;
};


struct gpn {
	int fault_code;
	int count;
	struct gpn_node *gns;
	struct gpn_node *next;
};

static int gpn_add_item(node_t node, struct gpn *gpn)
{
	struct gpn_node *gn;

	gn = calloc(1, sizeof(*gn));
	if(gn == NULL) {
		gpn->fault_code = 9002;
		return -1;
	}

	memcpy(&(gn->node), &node, sizeof(node));
	gn->next = gpn->gns;
	gpn->gns = gn;
	gpn->count++;

	return 0;
}

/*!
 * \brief Add a sub tree of the MOT into a response
 *
 * \param len The length of the Path argument of the method
 * \param next_level The NextLevel argument of the method
 * \param node The root node of the subtree
 * \param gpn The method's represent
 *
 * \return METHOD_SUCCESSED on success, METHOD_FAILED on failure
 */
static int gpn_add(int len, int next_level, node_t node, struct gpn *gpn)
{
	if((len == 0 || next_level == 0) && gpn_add_item(node, gpn) < 0) {
		return METHOD_FAILED;
	} else if(len > 0 || next_level == 0){
		int count;
		int res = METHOD_SUCCESSED;
		node_t *children = NULL;

		count = lib_get_children(node, &children);
		if(count > 0) {
			int i;
			for(i = 0; i < count && res == METHOD_SUCCESSED; i++) {
				if(next_level) {
					if(gpn_add_item(children[i], gpn) < 0)
						res =  METHOD_FAILED;
				} else {
					res = gpn_add(len, next_level, children[i], gpn);
				}
			}
		}

		if(children)
			lib_destroy_children(children);
		return res;
	}

	return METHOD_SUCCESSED;
}

int gpn_process(struct session *ss, char **msg)
{
	struct xml tag;
	char path[256] = "";
	int next_level = BOOLEAN_ERROR;
	int found_nl = 0;
	int end_with_dot = 0;
	int res;
	int len = -1;
	struct gpn *gpn;

	node_t node;

	while(xml_next_tag(msg, &tag) == XML_OK) {
		if(war_strcasecmp(tag.name, "ParameterPath") == 0) {
			if(tag.value)
				len = war_snprintf(path, sizeof(path), "%s", tag.value ? tag.value : "");
		} else if(war_strcasecmp(tag.name, "NextLevel") == 0) {
			if(tag.value)
				next_level = string2boolean(tag.value);
			found_nl = 1;
		} else if(war_strcasecmp(tag.name, "/GetParameterNames") == 0) {
			break;
		}
	}

	gpn = calloc(1, sizeof(*gpn));
	if(gpn == NULL) {
		tr_log(LOG_ERROR, "Out of memory!");
		return METHOD_FAILED;
	} else {
		ss->cpe_pdata = gpn;
	}
	/*judge full path or partial path*/
	if(len == 0 || path[len - 1] == '.') {
		end_with_dot = 1;
	}

	if(len < 0 || path[0] == '.') {
		gpn->fault_code = 9005;
		return METHOD_FAILED;
	/*Parameter Path and NextLevel is true, should return 9003*/
	} else if(found_nl == 0 || next_level == BOOLEAN_ERROR || (end_with_dot == 0 && next_level == BOOLEAN_TRUE)) {
		gpn->fault_code = 9003;
		return METHOD_FAILED;
	}
#if 0
	if(len == 0 || path[len - 1] == '.') {
		end_with_dot = 1;
	}
#endif
	if(len > 0 && end_with_dot) {
		path[len - 1] = '\0';
	}

	res = lib_resolve_node(path, &node);
	if(res == 0) {
		char type[PROPERTY_LENGTH];
		if(lib_get_property(node, "type", type) == 0) {
			int is_node;

			is_node = war_strcasecmp(type, "node") == 0;
			if((end_with_dot == 0 && is_node) || (end_with_dot && !is_node)) {
				gpn->fault_code = 9005;
				return METHOD_FAILED;
			} else {
				res = gpn_add(len, next_level, node, gpn); 
			}
		} else {
			gpn->fault_code = 9002;
			return METHOD_FAILED;
		}
	} else if(res == -1) {
		gpn->fault_code = 9002;
		return METHOD_FAILED;
	} else {
		gpn->fault_code = 9005;
		return METHOD_FAILED;
	}

	gpn->next = gpn->gns;
	return res;
}

int gpn_body(struct session *ss)
{
	int res = METHOD_COMPLETE;
	struct buffer buf;

	init_buffer(&buf);
	if(ss->cpe_pdata == NULL) {
		push_buffer(&buf,
				"<FaultCode>9002</FaultCode>\n"
				"<FaultString>Internal error</FaultString>\n");
	} else {
		struct gpn *gpn;
		gpn = (struct gpn *)(ss->cpe_pdata);
		if(gpn->fault_code) {
			push_buffer(&buf,
					"<FaultCode>%d</FaultCode>\n"
					"<FaultString>%s</FaultString>\n", gpn->fault_code, fault_code2string(gpn->fault_code));
		} else if(gpn->next) {
			if(gpn->next == gpn->gns)
				push_buffer(&buf, "<ParameterList xsi:type='soap-enc:Array' soap-enc:arrayType='cwmp:ParameterInfoStruct[%d]'>\n", gpn->count);

			if(gpn->next) {
				char rw[PROPERTY_LENGTH];

				lib_get_property(gpn->next->node, "rw", rw);
				push_buffer(&buf,
						"<ParameterInfoStruct>\n"
						"<Name>%s</Name>\n"
						"<Writable>%s</Writable>\n"
						"</ParameterInfoStruct>\n", lib_node2path(gpn->next->node), rw);

				gpn->next = gpn->next->next;
			}

			if(gpn->next == NULL) {
				push_buffer(&buf, "</ParameterList>\n");
				res = METHOD_COMPLETE;
			} else {
				res = METHOD_MORE_DATA;
			}
		}else{
			push_buffer(&buf, "<ParameterList xsi:type='soap-enc:Array' soap-enc:arrayType='cwmp:ParameterInfoStruct[0]'></ParameterList>\n");
		}
	}


	if(buf.data_len > 0)
		push_buffer(&(ss->outbuf), "%x\r\n%s\r\n", buf.data_len, buf.data);

// 2011.04.14
	if(buf.data_len > 0)
		push_buffer(&(ss->outbuf2_data), "%s", buf.data);


	destroy_buffer(&buf);



	return res;
}

void gpn_rewind(struct session *ss)
{
	if(ss->cpe_pdata) {
		struct gpn *gpn;
		gpn = (struct gpn *)(ss->cpe_pdata);

		gpn->next = gpn->gns;
	}
	return;
}

void gpn_destroy(struct session *ss)
{
	if(ss->cpe_pdata) {
		struct gpn *gpn;
		struct gpn_node *cur, *next;
		gpn = (struct gpn *)(ss->cpe_pdata);
		for(cur = gpn->gns; cur; cur = next) {
			next = cur->next;
			free(cur);
		}
		free(gpn);
		ss->cpe_pdata = NULL;
	}
	return;
}
