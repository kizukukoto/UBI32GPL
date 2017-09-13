/*
 * Broadcom Home Gateway Reference Design
 * PPTP Web Page Configuration Support Routines
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id: nvpptp.h,v 1.1 2009/03/11 04:06:28 peter_pan Exp $
 */

#ifndef _NVPPTP_H_
#define _NVPPTP_H_


#define MAX_NVRAM_PPTP_VALUE_SIZE   1024
#define MAX_NVRAM_PPTPD_VALUE_SIZE  256

#define PPTPD_CONN_ENABLE           "pptpd_enable"
#define PPTPD_CONN_LOCAL_IP         "pptpd_local_ip"
#define PPTPD_CONN_REMOTE_IP        "pptpd_remote_ip"


typedef struct pptp_Conn_s {
	int enable;
	char * ip;
	char * remote_subnet;
	char * user;
	char * passwd;
	char * encrypt_alg;
	char * extra_param;
}pptp_Conn_t;

typedef struct pptpd_Conn_s {
    // pptp server data struct
    int enable;                                 // enable pptp server
    char localip[MAX_NVRAM_PPTPD_VALUE_SIZE];   // local ip addresses
    char remoteip[MAX_NVRAM_PPTPD_VALUE_SIZE];  // remote ip addresses
}pptpd_Conn_t;


int get_nvpptp_Conn(int conn_id, pptp_Conn_t *pptp_Conn, char * value_ptr, char*null);
int set_nvpptp_Conn(int conn_id, pptp_Conn_t *pptp_Conn, char * tmpbuffer, int tmpsize);
int get_nvpptpd_Conn( pptpd_Conn_t *pptpd_Conn );
int set_nvpptpd_Conn( pptpd_Conn_t *pptpd_Conn );


//#include <nvcommons5.h>

#endif /* _NVPPTP_H_ */
