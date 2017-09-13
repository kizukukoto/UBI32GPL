/* dhcpc.h */
#ifndef _DHCPC_H
#define _DHCPC_H

#define DEFAULT_SCRIPT  "/usr/share/udhcpc/default.script"
#define RUSSIA_DHCPC_DNS_SCRIPT		"/usr/share/udhcpc/default.russia-bound-dns"
#define RUSSIA_DHCPC_NODNS_SCRIPT	"/usr/share/udhcpc/default.russia-bound-nodns"

/* allow libbb_udhcp.h to redefine DEFAULT_SCRIPT */
#include "libbb_udhcp.h"

#define INIT_SELECTING	0
#define REQUESTING	1
#define BOUND		2
#define RENEWING	3
#define REBINDING	4
#define INIT_REBOOT	5
#define RENEW_REQUESTED 6
#define RELEASED	7


struct client_config_t {
	char foreground;		/* Do not fork */
	char quit_after_lease;		/* Quit after obtaining lease */
	char abort_if_no_lease;		/* Abort if no lease */
	char background_if_no_lease;	/* Fork to background if no lease */
	char *interface;		/* The name of the interface to use */
	char *pidfile;			/* Optionally store the process ID */
	char *script;			/* User script to run at dhcp events */
	uint8_t *clientid;		/* Optional client id to use */
	uint8_t *vendorclass;		/* Optional vendor class-id to use */
	uint8_t *hostname;		/* Optional hostname to use */
	uint8_t *fqdn;			/* Optional fully qualified domain name to use */
	int ifindex;			/* Index number of the interface to use */
	uint8_t arp[6];			/* Our arp address */
	char *wan_proto;		/*NickChou add 07.5.28*/
#ifdef 	UDHCPD_NETBIOS
        int netbios_enable; /*NickChou add 07.7.31*/
#endif
#ifdef RPPPOE
	int russia_enable;		/* Chun add */
#endif
#ifdef CONFIG_BRIDGE_MODE
	int ap_mode;		/* Chun add */
/*	Date: 2009-04-10
	*	Name: Ken Chiang
	*	Reason:	Add support for enable auto mode select(router mode/ap mode).
	*	Note: 
*/		
#ifdef AUTO_MODE_SELECT
	int auto_mode_select;
#endif		
#endif
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	Add support for disable vendor class identifier
	*	Note: 
	*/
	int  vendor_class_identifier_disable;
	/*	Date: 2009-01-07
	*	Name: jimmy huang
	*	Reason:	Add support for enable classless static route (121/249)
	*	Note: 
	*/
#if defined(UDHCPC_MS_CLASSLESS_STATIC_ROUTE) || defined(UDHCPC_RFC_CLASSLESS_STATIC_ROUTE)
	int classless_static_route_enable;
#endif
// 2011.03.18
	int tr069_mode;
	uint8_t *tr069id;		/* Optional client id to use */

#ifdef IPV6_6RD
	int option_6rd_enable;
#endif
};

extern struct client_config_t client_config;


#endif
