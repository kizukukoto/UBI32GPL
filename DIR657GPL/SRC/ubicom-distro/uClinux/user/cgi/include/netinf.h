#ifndef _NETINF_H
#define _NETINF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

/*
 * Libs: Get ip address, mac address, and netmask
 * @ifname - Name of interface, such as "eth0" ...
 * @buf - Buffer for geting data
 * Return:
 *	Point to @buf if success. Otherwise, NULL returned.
 *	And which were converted to string format to @freq.
 */
extern int get_ip(const char *ifname, char *buf);

extern int get_mac(const char *ifname, char *buf);

extern int get_netmask(const char *ifname, char *buf);

/*
 * Get default gateway from @PATH_PROC_ROUTE
 */
extern void get_def_gw(char *out);

/*
 * Get name server ip from @PATH_RESOLVECONF
 */
extern void get_dns_srv(char *out);

/* Get wan interface name from item 'connType' in MIB */
#endif //_NETINF_H
