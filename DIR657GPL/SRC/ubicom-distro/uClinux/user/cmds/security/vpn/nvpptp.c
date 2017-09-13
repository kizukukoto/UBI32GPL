
/*
 * PPTP utility functions
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvpptp.c,v 1.1 2009/03/11 04:06:28 peter_pan Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typedefs.h>
/*
#include <netconf.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <nvparse.h>

#include <nvipsec.h>
#include <nvpptp.h>
*/
#include "nvpptp.h"

#define STRSEP strsep_with_validation
static char * strsep_with_validation(char **ptr,char *sep,char * retchar)	
{ 
	char * strptr ;	
	char *cptr;

	if (*ptr == NULL)
		return retchar;	

	strptr  =strsep(ptr,sep);	
	if (strptr != NULL) {

		/* change eny last \n to NULL */	
		cptr = strchr(strptr,'\r');;
		if (cptr != NULL)
			*cptr =' '; 


		if (*strptr == 0x00)		
			return retchar;	
	
	}else if (strptr == NULL)
		return retchar;
	
	return	strptr;		
}


/*
*	get the connection info[&pptp_Conn] of conn_id
*	parse the nvram value of format
*	ip,user,passwd,encrypt_alg,extraparms
*/
int get_nvpptp_Conn(int conn_id, pptp_Conn_t *pptp_Conn, char * value_ptr, char*null)
{
	char * nvram_ptr = NULL;
	int nvram_value_size =0;
	char *value =NULL;
	char name[32];

	/* Get connection info */
	snprintf(name,sizeof(name),"pptpd_conn_%d",conn_id);

	nvram_ptr=nvram_get(name);
	if (nvram_ptr == NULL) 
		return -1;
	nvram_value_size =strlen(nvram_ptr)+1;

	if (nvram_value_size > MAX_NVRAM_PPTP_VALUE_SIZE)
		return -1;

	value=value_ptr;
	memset(value,0x00,nvram_value_size);
	strncpy(value,nvram_ptr,nvram_value_size);

	pptp_Conn->ip=STRSEP(&value,",",null);
	pptp_Conn->remote_subnet = STRSEP(&value,",",null);
	pptp_Conn->user=STRSEP(&value,",",null);
	pptp_Conn->passwd=STRSEP(&value,",",null);
	pptp_Conn->encrypt_alg=STRSEP(&value,",",null);
	pptp_Conn->extra_param=STRSEP(&value,"",null);	

	/* Get enable/disable */
	snprintf(name,sizeof(name),"pptpd_conn_%d_enable",conn_id);
	nvram_ptr=nvram_get(name);
	if (nvram_ptr == NULL) 
		pptp_Conn->enable=0;
	else
		pptp_Conn->enable=atoi(nvram_ptr);


	return 0;
}


int set_nvpptp_Conn(int conn_id, pptp_Conn_t *pptp_Conn, char * tmpbuffer, int tmpsize)
{
	char name[128];
	snprintf(name,sizeof(name),"pptpd_conn_%d",conn_id);
	snprintf(tmpbuffer,tmpsize, 
			"%s,%s,%s,%s,%s,%s",
			pptp_Conn->ip,
			pptp_Conn->remote_subnet,
			pptp_Conn->user,
			pptp_Conn->passwd,
			pptp_Conn->encrypt_alg,
			pptp_Conn->extra_param
		);
	nvram_set(name,tmpbuffer);
	//printf("%s: %s\n",name,tmpbuffer);
	
	snprintf(name,sizeof(name),"pptpd_conn_%d_enable",conn_id);
	snprintf(tmpbuffer,tmpsize, "%d",pptp_Conn->enable);
	nvram_set(name,tmpbuffer);
	//printf("%s: %s\n",name,tmpbuffer);

	return 0;

}


/*
*   get the connection info[&pptpd_Conn] for pptpd
*/
int
get_nvpptpd_Conn( pptpd_Conn_t *pptpd_Conn )
{
    char * nvram_ptr = NULL;
    int nvram_value_size = 0;

    // clear memory
    memset( pptpd_Conn, 0x00, sizeof(pptpd_Conn_t) );

    /* Get pptpd enable/disable */
    nvram_ptr=nvram_get( PPTPD_CONN_ENABLE );
    if ( nvram_ptr != NULL )
        pptpd_Conn->enable=atoi( nvram_ptr );

    /* Get local ip range info */
    nvram_ptr=nvram_get( PPTPD_CONN_LOCAL_IP );
    if ( nvram_ptr != NULL )
    {
        nvram_value_size = strlen( nvram_ptr )+1;
        if ( nvram_value_size > MAX_NVRAM_PPTPD_VALUE_SIZE )
            nvram_value_size = MAX_NVRAM_PPTPD_VALUE_SIZE;
        strncpy( pptpd_Conn->localip, nvram_ptr, nvram_value_size );
    }

    /* Get remote ip range info */
    nvram_ptr=nvram_get( PPTPD_CONN_REMOTE_IP );
    if ( nvram_ptr != NULL )
    {
        nvram_value_size = strlen( nvram_ptr )+1;
        if ( nvram_value_size > MAX_NVRAM_PPTPD_VALUE_SIZE )
            nvram_value_size = MAX_NVRAM_PPTPD_VALUE_SIZE;
        strncpy( pptpd_Conn->remoteip, nvram_ptr, nvram_value_size );
    }

    return 0;
}


int
set_nvpptpd_Conn( pptpd_Conn_t *pptpd_Conn )
{
    char tmpbuff[MAX_NVRAM_PPTPD_VALUE_SIZE+1];

    // set local ip address range
    memset( tmpbuff, 0x00, sizeof(tmpbuff) );
    strncpy( tmpbuff, pptpd_Conn->localip, MAX_NVRAM_PPTPD_VALUE_SIZE );
    nvram_set( PPTPD_CONN_LOCAL_IP, tmpbuff );
    // set remote ip address range
    memset( tmpbuff, 0x00, sizeof(tmpbuff) );
    strncpy( tmpbuff, pptpd_Conn->remoteip, MAX_NVRAM_PPTPD_VALUE_SIZE );
    nvram_set( PPTPD_CONN_REMOTE_IP, tmpbuff );
    // set pptpd enable
    snprintf( tmpbuff, 16, "%d", pptpd_Conn->enable);
    nvram_set( PPTPD_CONN_ENABLE, tmpbuff );

    return 0;

}
