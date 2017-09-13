/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file grm.c
 *
 * \brief The implementation of GetRPCMethods CPE RPC methods
 */
#include <stdio.h>
#include <string.h>

#include "method.h"
#include "buffer.h"
#include "grm.h"
#include "xml.h"
#include "log.h"
#include "request.h"

int get_num_from_attrs(struct xml *tag)
{
    int i;
    int num = 0;
    for(i=0; i<tag->attr_count; i++) {
        if (war_strcasecmp(tag->attributes[i].attr_name, "arrayType")==0) {
            char *t_flag = strchr(tag->attributes[i].attr_value, '[');
	    if (t_flag != NULL) {
		t_flag++;
		num = atoi(t_flag);
	    }
	    break;
	}
    }
    return num;
}

int cpe_grm_body(struct session *ss)
{
    int count;
    struct buffer buf;
    const struct cpe_method *cpe;

    init_buffer(&buf);
    count = cpe_methods_count();

    push_buffer(&buf, "<MethodList xsi:type='soap-enc:Array' soap-enc:arrayType='cwmp:string[%d]'>\n", count);

	for(; count > 0;) {
            count--;
	    cpe = get_cpe_method_by_index(count);
            push_buffer(&buf, "<string>%s</string>\n", cpe->name);
	}
        push_buffer(&buf, "</MethodList>\n");

        push_buffer(&(ss->outbuf), "%x\r\n%s\r\n", buf.data_len, buf.data);

// 2011.04.14
        push_buffer(&(ss->outbuf2_data), "%s", buf.data);

        destroy_buffer(&buf);

	return METHOD_COMPLETE;
}
