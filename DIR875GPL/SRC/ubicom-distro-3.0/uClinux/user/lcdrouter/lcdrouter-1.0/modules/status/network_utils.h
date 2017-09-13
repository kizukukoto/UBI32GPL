/*
 * This file is derived from rc/network.h and will be used temporarily
 * for demo purposes!
 */
#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

//char *get_macaddr(char *if_name);
char *get_ipaddr(char *if_name);
char *get_netmask(char *if_name);
char *get_dns(int index);

#endif
