/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file cli.h
 */
#ifndef __HEADER_CLI__
#define __HEADER_CLI__

#include "session.h"


/*!
 * \brief Eat the CLI configs
 *
 * \param name The config item name
 * \param value The config item value
 *
 * \return Always return 0
 */
int set_cli_config(const char *name, const char *value);
/*!
 * \brief Notify a parameter's value changed
 *
 * \param path The parameter's path
 * \param new The new value
 *
 * \return N/A
 */
void value_change(const char *path, const char *new);

/*!
 * \brief Launch the CLI listener to process the incoming CLI command
 *
 * \return 0 when success, -1 when any error
 */
int launch_cli_listener(void);

/*!
 * \brief Register a value change trigger
 *
 * \param path The parameter's path
 * \param trigger The callback function to be called when the parameter's value changed
 *
 * \return 0 when success, -1 when any error
 */
int register_vct(const char *path, void (*trigger)(const char *new_value));
#endif
