/* $Id: upnpglobalvars.c,v 1.2 2009/07/08 11:18:33 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <sys/types.h>
#include <netinet/in.h>

#include "config.h"
#include "upnpglobalvars.h"

/* network interface for internet */
const char * ext_if_name = 0;

/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To add rules with "-o br0"
*/
/* local network interface */
const char * int_if_name = 0;

/* file to store leases */
#ifdef ENABLE_LEASEFILE
const char* lease_file = 0;
#endif

/* forced ip address to use for this interface
 * when NULL, getifaddr() is used */
const char * use_ext_ip_addr = 0;

/* LAN address */
/*const char * listen_addr = 0;*/

unsigned long downstream_bitrate = 0;
unsigned long upstream_bitrate = 0;

/* startup time */
time_t startup_time = 0;

#if 0
/* use system uptime */
int sysuptime = 0;

/* log packets flag */
int logpackets = 0;

#ifdef ENABLE_NATPMP
int enablenatpmp = 0;
#endif
#endif

int runtime_flags = 0;

const char * pidfilename = "/var/run/miniupnpd.pid";

char uuidvalue[] = "uuid:00000000-0000-0000-0000-000000000000";
char serialnumber[SERIALNUMBER_MAX_LEN] = "none";

/* Chun mofigy for WPS-COMPATIBLE */
char uuidvalue_1[256] = "uuid:11111111-1111-1111-1111-111111111111\0";//InternetGatewayDevice
char uuidvalue_2[256] = "uuid:22222222-2222-2222-2222-222222222222\0";//WANDevice
char uuidvalue_3[256] = "uuid:33333333-3333-3333-3333-333333333333\0";//WANConnectionDevice
char uuidvalue_4[256] = "uuid:565aa949-67c1-4c0e-aa8f-f349e6f59311\0";//WFADevice

/* chun add for XBOX-TEST */

char modelnumber[MODELNUMBER_MAX_LEN] = "1";

char wpsenable[WPSENABLE_MAX_LEN] = "0";

/* presentation url :
 * http://nnn.nnn.nnn.nnn:ppppp/  => max 30 bytes including terminating 0 */
char presentationurl[PRESENTATIONURL_MAX_LEN];

/* UPnP permission rules : */
struct upnpperm * upnppermlist = 0;
unsigned int num_upnpperm = 0;

#ifdef ENABLE_NATPMP
/* NAT-PMP */
unsigned int nextnatpmptoclean_timestamp = 0;
unsigned short nextnatpmptoclean_eport = 0;
unsigned short nextnatpmptoclean_proto = 0;
#endif

#ifdef USE_PF
const char * queue = 0;
const char * tag = 0;
#endif

#ifdef USE_NETFILTER
/* chain name to use, both in the nat table
 * and the filter table */
const char * miniupnpd_nat_chain = "MINIUPNPD";
const char * miniupnpd_forward_chain = "MINIUPNPD";
#ifdef HAIRPIN_SUPPORT
const char * miniupnpd_nat_postrotue_chain = "POSTROUTING";
#endif
#endif

int n_lan_addr = 0;
struct lan_addr_s lan_addr[MAX_LAN_ADDR];

/* Path of the Unix socket used to communicate with MiniSSDPd */
const char * minissdpdsocketpath = "/var/run/minissdpd.sock";

/*	Date:	2009-02-27
*	Name:	jimmy huang
*	Reason: for Vista Logo, win 7, dtm 1.3
*			miniupnpd must tell each upnp client that we have WFA Device service
*			only when the daemon (wsccmd) who take care of WFA Device service
*			is activated on the serive port, so that upnp client will show up
*			WFA Device icon
*			This variable represent that if wsccmd if activated or not
*	Note:	Added the variable below
*/
int wsccmd_ready_flag = 0;

int last_WanConnectionStatus = 0;
//0:disconnect
//1:connection

char last_ext_ip_addr[INET_ADDRSTRLEN] = {'\0'};
char default_NewPortMappingDescription[64]="miniupnpd\0";
//--------



