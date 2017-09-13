/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file pkey.c
 *
 */
#include <stdlib.h>

#include "pkey.h"
#include "atomic.h"
#include "tr_lib.h"

int set_parameter_key(const char *value)
{
	node_t node;
	int res;

	res = lib_resolve_node(PARAMETERKEY, &node);
	if(res == 0) {
		char *ov = NULL;

		res = lib_get_value(node, &ov);
		if(res == 0) {
			//ParameterKey just a special parameter, MUST use the spv function
			//to handler it and MUST be operated atomically
			res = spv_journal(PARAMETERKEY, ov);
			if(res == 0)
				res = lib_set_value(node, value);
		}

		if(ov)
			lib_destroy_value(ov);
	}

	return res == 0 ? 0 : -1;
}
