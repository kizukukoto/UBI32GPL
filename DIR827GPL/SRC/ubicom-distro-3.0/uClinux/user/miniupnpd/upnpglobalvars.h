/* $Id: upnpglobalvars.h,v 1.4 2009/07/08 11:18:33 jimmy_huang Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __UPNPGLOBALVARS_H__
#define __UPNPGLOBALVARS_H__

#include <time.h>
#include "upnppermissions.h"
#include "miniupnpdtypes.h"
#include "config.h"

/* name of the network interface used to acces internet */
extern const char * ext_if_name;

/*	Date:	2009-07-08
*	Name:	jimmy huang
*	Reason:	To add rules with "-o br0"
*/
/* name of the local network interface */
extern const char * int_if_name;

/* file to store all leases */
#ifdef ENABLE_LEASEFILE
extern const char * lease_file;
#endif

/* forced ip address to use for this interface
 * when NULL, getifaddr() is used */
extern const char * use_ext_ip_addr;

/* parameters to return to upnp client when asked */
extern unsigned long downstream_bitrate;
extern unsigned long upstream_bitrate;

/* statup time */
extern time_t startup_time;

/* runtime boolean flags */
extern int runtime_flags;
#define LOGPACKETSMASK		0x0001
#define SYSUPTIMEMASK		0x0002
#ifdef ENABLE_NATPMP
#define ENABLENATPMPMASK	0x0004
#endif
#define CHECKCLIENTIPMASK	0x0008
#define SECUREMODEMASK		0x0010

#define ENABLEUPNPMASK		0x0020

#ifdef PF_ENABLE_FILTER_RULES
#define PFNOQUICKRULESMASK	0x0040
#endif

#define SETFLAG(mask)	runtime_flags |= mask
#define GETFLAG(mask)	runtime_flags & mask
#define CLEARFLAG(mask)	runtime_flags &= ~mask

extern const char * pidfilename;

extern char uuidvalue[];

/* Chun mofigy for WPS-COMPATIBLE */
extern char uuidvalue_1[256];//InternetGatewayDevice
extern char uuidvalue_2[256];//WANDevice
extern char uuidvalue_3[256];//WANConnectionDevice
extern char uuidvalue_4[256];//WFADevice

#define SERIALNUMBER_MAX_LEN (10)
extern char serialnumber[];

#define MODELNUMBER_MAX_LEN (48)
extern char modelnumber[];

//jimmy added
#define MODELNAME_MAX_LEN (64)
extern char modelname[];

#define MODELURL_MAX_LEN (64)
extern char modelurl[];

#define MANUFACTURERNAME_MAX_LEN (64)
extern char manufacturername[];

#define MANUFACTURERURL_MAX_LEN (64)
extern char manufacturerurl[];

#define FRIENDLYNAME_MAX_LEN (64)
extern char friendlyname[];
//----------------

#define PRESENTATIONURL_MAX_LEN (64)
extern char presentationurl[];

/* Chun add for WFA-XML-IP */
#define WPSENABLE_MAX_LEN (2)
extern char wpsenable[];

extern char wfadev_path[];
extern char wfadev_control[];
extern char wfadev_eventual[];
/* Chun end */

/* UPnP permission rules : */
extern struct upnpperm * upnppermlist;
extern unsigned int num_upnpperm;

#ifdef ENABLE_NATPMP
/* NAT-PMP */
extern unsigned int nextnatpmptoclean_timestamp;
extern unsigned short nextnatpmptoclean_eport;
extern unsigned short nextnatpmptoclean_proto;
#endif

#ifdef USE_PF
/* queue and tag for PF rules */
extern const char * queue;
extern const char * tag;
#endif

#ifdef USE_NETFILTER
extern const char * miniupnpd_nat_chain;
extern const char * miniupnpd_forward_chain;
#ifdef HAIRPIN_SUPPORT
extern const char * miniupnpd_nat_postrotue_chain;
#endif
#endif

/* lan addresses */
/* MAX_LAN_ADDR : maximum number of interfaces
 * to listen to SSDP traffic */
#define MAX_LAN_ADDR (4)
extern int n_lan_addr;
extern struct lan_addr_s lan_addr[];

extern const char * minissdpdsocketpath;

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
extern int wsccmd_ready_flag;
extern int last_WanConnectionStatus;
//0:disconnect
//1:connection

extern char last_ext_ip_addr[INET_ADDRSTRLEN];
extern char default_NewPortMappingDescription[64];
extern struct rdr_desc * rdr_desc_list; // in netfilter/iptcrde.c

//code in netfilter/iptcrdr.c, used within upnp_redirect(), upnpredirect.c
extern void update_redirect_desc(unsigned short eport, int proto, const char * desc
		, const char *RemoteHost, unsigned short InternalPort, const char *InternalClient
		, int Enabled, long int LeaseDuration, long int *out_expire_time);
//--------

extern int get_element(char *input, char *token, char *fmt, ...);

// configuration file path
#define MINIUPNPD_CONF "/var/etc/miniupnpd.conf"
//#define MINIUPNPD_CONF "/etc/miniupnpd/miniupnpd.conf"

//record addportmapping rules
/*
	This defintion for port mapping files must be identical with 
	httpd/cmobasicapi.c
	httpd/ajax.c
*/
#define MINIUPNPD_LEASE_FILE "/tmp/upnp_portmapping_rules"

//#define UUID_GEN_FILE "/var/tmp/uuid_file"
#define UUID_GEN_FILE "/etc/miniupnpd/uuid"

/* Chun add for INFO-FROM-NVRAM */
#define MANU_URL	"http://www.D-Link.com/"
#define MODEL_URL	"http://www.D-Link.com/"
#define MODEL_NUMBER	"1234"
#define SERIAL_NUMBER	"5678"
#define UPC				"0001"
/* Chun end */

/*	Date:	2009-05-07
*	Name:	jimmy huang
*	Reason:	UPnP-arch-DeviceArchitecture-v1.1-20081015.pdf, page 40
*			for multicast M-SEARCH request, 
*			if specifies MX value greater than 5, device should assume that it contained
*			the value 5 or less
*/
#define MX_MAXVALUE 5

#if defined(ENABLE_L3F_SERVICE)
#define WFA_DEV_KNOWN_SERVICE_INDEX 7
#define WFA_SRV_KNOWN_SERVICE_INDEX 8
#else
#define WFA_DEV_KNOWN_SERVICE_INDEX 6
#define WFA_SRV_KNOWN_SERVICE_INDEX 7
#endif // ENABLE_L3F_SERVICE

// jimmy test, for later improvement used
// typedef struct PortMapping_Table{
// 	unsigned short eport;
// }
// ----------

#endif



