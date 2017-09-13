/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file spv.h
 *
 */
#ifndef __HEADER_SPV__
#define __HEADER_SPV__

#include "tr_lib.h"
#include "session.h"

int spv_process(struct session *ss, char **msg);
int spv_body(struct session *ss);
void spv_rewind(struct session *ss);
void spv_destroy(struct session *ss);

#ifdef TR196
int check_parameter_value(const char *value, const char *type, char *limitation);
#endif

#endif
