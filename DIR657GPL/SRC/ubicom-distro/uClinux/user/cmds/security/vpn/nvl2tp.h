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
 * $Id: nvl2tp.h,v 1.1 2009/03/11 04:06:28 peter_pan Exp $
 */

#ifndef _NVL2TP_H_
#define _NVL2TP_H_


#define NVRAM_L2TPD_VALUE_SIZE  32

#define L2TPD_ENABLE           "l2tpd_enable"
#define L2TPD_SERVERNAME       "l2tpd_servername"
#define L2TPD_REMOTE_IPS       "l2tpd_remote_ip"
#define L2TPD_LOCAL_IP         "l2tpd_local_ip"


typedef struct l2tpd_s {
    // l2tp server data struct
    int enable;                                 // enable l2tp server
    char servername[NVRAM_L2TPD_VALUE_SIZE];      // server name
    char localip[NVRAM_L2TPD_VALUE_SIZE];       // local ip address
    char remoteips[NVRAM_L2TPD_VALUE_SIZE];      // remote ip addresses
}l2tpd_t;


int get_nvl2tpd( l2tpd_t *l2tpd_data );
int set_nvl2tpd( l2tpd_t *l2tpd_data );
int write_l2tpd_lns(char *);
int write_l2tpd_lac(char * );

#endif /* _NVL2TP_H_ */
