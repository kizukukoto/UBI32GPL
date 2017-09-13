#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <netinet/in.h>

/* Computing the IPv6 mask according to the mask lentgh into the in6_addr structure given */
int
getv6AddressMask(struct in6_addr * addr, int masklen);

/* Adding 0's when needed to have an 32 xdigit IPv6 address */
void
paddingAddr(char *str, int *index, int lim);

/* Re-Formating the IPv6 address accordingly to the useful presentation method */
void
add_char(char *str);

/* Formating the IPv6 address accordingly to the address stored in the /proc/net/if_inet6 file */
void
del_char(char *str);

/* Retrieve the index of an interface from its address */
int
getifindex(const char * addr, int * ifindex);

