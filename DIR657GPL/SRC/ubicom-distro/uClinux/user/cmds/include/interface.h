#ifndef __INTERFACE_H
#define __INTERFACE_H
#define NVRAM_MAX_IF	20

/* 
 * struct info_if only care about the "interface"
 * such as: NIC name, aliase, which protocol (static|dhcpc ...)
 * 	activated by. others info (IP address, mtu ...) was recorded
 * 	in struct proto_XXXXX
 * 
 * */
struct info_if {
	int index;	/* index the suffix of if_XXXX	*/
	char *dev;	/* if_devX			*/
	char *phy;	/* if_phyX			*/
	char *alias;	/* if_aliasX			*/
	char *proto;	/* if_protoX			*/
	char *proto_alias;
	char *mode;	/* mode avalid: route|nat	*/
	char *trust;
	char *services;	/* startup services list after if-up */
	char *route;	/* static route list		*/
	char *icmp;	/* icmp 1|0			*/
};

#endif // __INTERFACE_H
