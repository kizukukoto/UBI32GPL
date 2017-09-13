struct items_range interfaces[] = {
	{ "wan_port_speed", "auto", NV_RESET}, /* the name is not good. */
	{ "zone_if_proto2", "dhcpc", NV_RESET},    /* static|dhcpc|pppoe|pptp|l2tp */
	{ "if_proto0", "static", NV_RESET },    /* static|dhcpc|pppoe */
        { "if_proto_alias0", "LAN_static", NV_RESET },  /* alias of proto */
        { "if_mode0", "route", NV_RESET },      /* route|nat XXX: up on ifup */
        { "if_trust0", "", NV_RESET },          /* trust packet from interface alias */
        { "if_route0", "", NV_RESET },          /* fmt: NAME1/NET/MASK/GW/Metric,NAME2/NET..*/
        { "if_icmp0", "1", NV_RESET },          /* 1: enable. 0: disable*/
        { "if_proto1", "static", NV_RESET },    /* static|dhcpc|pppoe */
        { "if_proto_alias1", "DMZ_static", NV_RESET },  /* alias of proto */
        { "if_mode1", "route", NV_RESET },      /* route|nat */
        { "if_icmp2", "1", NV_RESET },          /* 1: enable. 0: disable*/
        { "if_proto2", "dhcpc", NV_RESET },     /* static|dhcpc|pppoe */
        { "if_proto_alias2", "WAN_dhcpc", NV_RESET },   /* alias of proto */
        { "if_mode2", "nat", NV_RESET },        /* route|nat */
        { "if_icmp2", "1", NV_RESET },          /* 1: enable. 0: disable*/
        { "if_proto3", "", NV_RESET },  /* pppoe|pptp|l2tp|rupppoe|rupptp|rul2tp */
        { "if_proto_alias3", "WAN_pppoe_ru", NV_RESET },        /* alias of proto */
        { "if_mode3", "nat", NV_RESET },        /* route|nat */
        { "if_trust3", "LAN", 0, NV_RESET},            /* 0|1 */
        { "if_icmp3", "1", NV_RESET },          /* 1: enable. 0: disable*/

	{ "if_alias0", "LAN", NV_RESET},
	{ "if_dev0", "br0", NV_RESET},
	{ "if_phy0", "eth0", NV_RESET},
	{ "if_services0", LAN_SERVICES, NV_RESET},
	{ "if_alias1", "DMZ", NV_RESET},
	{ "if_dev1", "eth2", NV_RESET},
	{ "if_phy1", "eth2", NV_RESET},
	{ "if_trust1", "LAN", NV_RESET},
	{ "if_services1", DMZ_SERVICES, NV_RESET},
	{ "if_route2", "", NV_RESET},
	{ "if_alias2", "WAN_primary", NV_RESET},
	{ "if_dev2", "eth1", NV_RESET},
	{ "if_phy2", "eth1", NV_RESET},
	{ "if_trust2", "LAN", NV_RESET},
	{ "if_services2", PROTO_SINGLE_SERVICES, NV_RESET},
	{ "if_route2", "", NV_RESET},
	{ "if_alias3", "WAN_slave", NV_RESET},
	{ "if_dev3", "ppp3", NV_RESET},
	{ "if_phy3", "eth1", NV_RESET},
	{ "if_trust3", "LAN", NV_RESET},
	{ "if_services3", PROTO_DUAL1_SERVICES, NV_RESET},
	{ "if_route3", "", NV_RESET},
	{ "if_dev4", "ra0", NV_RESET},	/* wireless device, used for get_trx (cgi/misc) */
	{ "vdev_0", "LAN:LAN", NV_RESET},
	{ "vdev_1", "DMZ:DMZ", NV_RESET},
	{ "vdev_2", "WAN:WAN_primary", NV_RESET}, /* Groupping of if_aliasXXX, used in firewall interfaces */
	{NULL, NULL}
};
