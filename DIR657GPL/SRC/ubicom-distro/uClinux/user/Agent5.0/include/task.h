/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file task.h
 *
 */
#ifndef __HEADER_TASK__
#define __HEADER_TASK__

#include "session.h"

int add_task_config(const char *name, const char *value);

int load_task(void);
int gaqt_body(struct session *ss);
int gqt_body(struct session *ss);

int download_upload_process(struct session *ss, char **msg);
int download_upload_body(struct session *ss);

#ifdef TR143
void stop_dd(void);
void start_dd(void);
void stop_ud(void);
void start_ud(void);
#endif

#ifdef TR196
int gen_upload_pm_task(const char *path, const char *user, const char *pwd, const char *url);
#endif

int gen_transfer_task(const char* announce_url, char *data);
#endif
