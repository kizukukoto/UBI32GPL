#ifndef __NET_SERVICES_H
#define __NET_SERVICES_H
/*
 * Define servies the order of services
 * */
#define LAN_SERVICES						\
	"httpd,httpd_LAN,@if_dev,@Z_LOCALIP "			\
	"dhcpd,LAN_dhcpd,@if_dev,@Z_LOCALIP,@Z_LOCALMASK "	\
	"upnp,@if_dev,@Z_LOCALIP route,@if_dev,@if_route "	\
	"webfilter,@if_dev,@Z_LOCALIP,@Z_LOCALMASK wps"

#define DMZ_SERVICES						\
	"dhcpd,DMZ_dhcpd,@if_dev,@Z_LOCALIP,@Z_LOCALMASK "	\
	"route,@if_dev,@if_route"

#define PROTO_SINGLE_SERVICES					\
	"ntp igmp ddns https,https_REMOTE,@if_dev,@Z_LOCALIP "	\
	"route,@if_dev,@if_route policy_route "			\
	"xl2tpd pptpd ipsec qos ips,@if_dev"			\
	
#define PROTO_DUAL0_SERVICES					\
	"https,https_REMOTE,@if_dev,@Z_LOCALIP "		\
	"netdevice,WAN_slave"

#define PROTO_DUAL1_SERVICES	PROTO_SINGLE_SERVICES

//#define PROTO_RU0_SERVICES	"netdevice,WAN_slave"

//#define PROTO_RU1_SERVICES	PROTO_SINGLE_SERVICES
#endif //__NET_SERVICES_H
