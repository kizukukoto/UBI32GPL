
#include "net_services.h"

#define NV_RESET 1


struct items_range dhcpd[] = { 
	{ "dhcpd_dns0", "192.168.0.1", NV_RESET},
	{ "dhcpd_reserve0", "", NV_RESET},
	{ "dhcpd_reserve_", "", NV_RESET, 0, 49},
	{ "dhcpd_dns_relay0", "1", NV_RESET},
	{ "dhcpd_alias1", "DMZ_dhcpd", NV_RESET},
	{ "dhcpd_dns1", "", NV_RESET},
	{ "dhcpd_reserve1", "", NV_RESET},
	{ "dhcpd_dns_relay1", "1", NV_RESET},
	{ "dhcpd_dns4", "192.168.10.1", NV_RESET},
	{ "dhcpd_reserve4", "", NV_RESET},
	{ "dhcpd_dns_relay4", "1", NV_RESET},
	{NULL, NULL}
};
