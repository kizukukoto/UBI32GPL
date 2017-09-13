
/*
 * L2TP utility functions
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvl2tp.c,v 1.1 2009/03/11 04:06:28 peter_pan Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typedefs.h>
/*
#include <bcmnvram.h>
#include <nvl2tp.h>
*/
#include "nvram.h"
#include "nvl2tp.h"

/*
*   get the connection info for l2tpd
*/
int
get_nvl2tpd( l2tpd_t *l2tpd_data )
{
    char * nvram_ptr = NULL;
    int nvram_value_size = 0;

    // clear memory
    memset( l2tpd_data, 0x00, sizeof(l2tpd_t) );

    /* Get l2tpd enable/disable */
    nvram_ptr=nvram_get( L2TPD_ENABLE );
    if ( nvram_ptr != NULL )
        l2tpd_data->enable=atoi( nvram_ptr );

    /* Get l2tpd servername */
    nvram_ptr=nvram_get( L2TPD_SERVERNAME );
    if ( nvram_ptr != NULL ) {
        nvram_value_size = strlen( nvram_ptr )+1;
        if ( nvram_value_size > NVRAM_L2TPD_VALUE_SIZE )
            nvram_value_size = NVRAM_L2TPD_VALUE_SIZE;
        strncpy( l2tpd_data->servername, nvram_ptr, nvram_value_size );
    }

    /* Get local ip range info */
    nvram_ptr=nvram_get( L2TPD_LOCAL_IP );
    if ( nvram_ptr != NULL )
    {
        nvram_value_size = strlen( nvram_ptr )+1;
        if ( nvram_value_size > NVRAM_L2TPD_VALUE_SIZE )
            nvram_value_size = NVRAM_L2TPD_VALUE_SIZE;
        strncpy( l2tpd_data->localip, nvram_ptr, nvram_value_size );
    }

    /* Get remote ip range info */
    nvram_ptr=nvram_get( L2TPD_REMOTE_IPS );
    if ( nvram_ptr != NULL )
    {
        nvram_value_size = strlen( nvram_ptr )+1;
        if ( nvram_value_size > NVRAM_L2TPD_VALUE_SIZE )
            nvram_value_size = NVRAM_L2TPD_VALUE_SIZE;
        strncpy( l2tpd_data->remoteips, nvram_ptr, nvram_value_size );
    }
    return 0;
}


int
set_nvl2tpd( l2tpd_t *l2tpd_data )
{
    char tmpbuff[NVRAM_L2TPD_VALUE_SIZE+1];

    // set local ip address
    memset( tmpbuff, 0x00, sizeof(tmpbuff) );
    strncpy( tmpbuff, l2tpd_data->localip, NVRAM_L2TPD_VALUE_SIZE );
    nvram_set( L2TPD_LOCAL_IP, tmpbuff );
    // set remote ip address range
    memset( tmpbuff, 0x00, sizeof(tmpbuff) );
    strncpy( tmpbuff, l2tpd_data->remoteips, NVRAM_L2TPD_VALUE_SIZE );
    nvram_set( L2TPD_REMOTE_IPS, tmpbuff );
    // set l2tpd enable
    snprintf( tmpbuff, 16, "%d", l2tpd_data->enable);
    nvram_set( L2TPD_ENABLE, tmpbuff );

    return 0;

}
