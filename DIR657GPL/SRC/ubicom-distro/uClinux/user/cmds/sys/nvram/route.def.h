
#include "net_services.h"

#define NV_RESET 1

struct items_range route[] = {
	/* 0:WAN, 1:LAN, 2:DMZ*/
	{ "rip_enable", "0", NV_RESET, 0, 2},
	{ "rip_tx_version", "", NV_RESET, 0, 2},
	{ "rip_rx_version", "", NV_RESET, 0, 2},
	{ "static_route", "", NV_RESET},
	{ "ospf_if_area", "0", NV_RESET, 0, 2},
	{ "ospf_if_priority", "1", NV_RESET, 0, 1},
	{ "policy_route", "", NV_RESET, 0, 24},
	{NULL, NULL}
};
