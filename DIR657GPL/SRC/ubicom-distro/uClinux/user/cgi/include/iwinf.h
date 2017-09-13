#ifndef _IWINF_H
#define _IWINF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <net/if.h>
//#include <linux/sockios.h>
//#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/wireless.h>


/*
 * Libs: get wireless interface ssid
 * @iwname - Name of wireless interface
 * @ssid - Buffer, ouput the string of ssid
 * Return:
 *	Point to @ssid if success. or NULL returned on error.
 */
extern char *get_iw_ssid(const char* iwname, char *ssid);

/*
 * Libs: Get wireless interface frequency
 * @iwname - Name of wireless interface
 * @freq - Output converted to string
 * Return:
 *	Return frequency if success, or 0 as failure.
 */
extern double get_iw_freq(const char *iwname, char *freq);

#endif //_IWINF_H
