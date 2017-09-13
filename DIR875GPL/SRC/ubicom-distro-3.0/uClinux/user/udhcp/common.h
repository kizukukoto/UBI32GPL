/* common.h
 *
 * Russ Dill <Russ.Dill@asu.edu> September 2001
 * Rewritten by Vladimir Oleynik <dzo@simtreas.ru> (C) 2003
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _COMMON_H
#define _COMMON_H

#include "version.h"
#include "libbb_udhcp.h"


#ifndef UDHCP_SYSLOG
enum syslog_levels {
	LOG_EMERG = 0,
	LOG_ALERT,
	LOG_CRIT,
	LOG_WARNING,
	LOG_ERR,
	LOG_INFO,
	LOG_DEBUG
};
#else
#include <syslog.h>
#endif

long uptime(void);
void background(const char *pidfile);
void start_log_and_pid(const char *client_server, const char *pidfile);
void background(const char *pidfile);
void udhcp_logging(int level, const char *fmt, ...);
                                                            
#define LOG(level, str, args...) udhcp_logging(level, str, ## args)

#ifdef UDHCP_DEBUG
# define DEBUG(level, str, args...) LOG(level, str, ## args)
#else
# define DEBUG(level, str, args...) do {;} while(0)
#endif
#define MAX_HOSTNAME_LEN 64
/*	Date: 2009-12-17
*	Name: jimmy huang
*	Reason:	Modified option length from 308 to 408
*	Note:	
*		1.	TEW-634GRU RUssia has the case, dhcpd offer both option 121 and 249, each contains 15 rules
*			And the total dhcp OFFER packet size is 601, which will cause our udhcpc can not handle the packet
*			thus we increase to 408 to let udhcpc can hanlde these kinds of packet
*		2.	We defined new definition "DHCP_OPTION_SIZE" in common.h, all related variable
*			will be replaced with this definition
*/
#ifdef UDHCP_ENHANCE_OPTION_SIZE
        #define DHCP_OPTION_SIZE 408
#else
        #define DHCP_OPTION_SIZE 308
#endif

#endif
