#ifndef __CLIST_H
#define __CLIST_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include "linux_list.h"


extern void add_ips(u_int32_t);
/* NOTE: -1 as not found*/
extern u_int32_t search_ips(u_int32_t);
extern void add_domain2ip(const char *);

/*
 * Search ip address in list.
 * @addr : string of ip. ex: "192.168.0.1"
 * Return	1: As Sucess
 * 		-1: Error ip, like :"1.2.3.666" or "www.pchome.com.tw"
 *		0: Not found in list
 */
extern int search_addr(const char *addr);
#endif //__CLIST_H
