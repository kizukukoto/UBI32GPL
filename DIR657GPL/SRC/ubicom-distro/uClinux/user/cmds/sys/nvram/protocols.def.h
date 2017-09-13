struct items_range protocols[] = { 
        { "static_iplist", "", NV_RESET, 0, 2}, /* smtp@192.168.3.2,dns@192.168.3.3 */
        //{ "static_iplist1", "", NV_RESET }, /* smtp@192.168.3.2,dns@192.168.3.3 */
        //{ "static_iplist2", "", NV_RESET }, /* smtp@192.168.3.2,dns@192.168.3.3 */

	{ "static_alias0", "LAN_static", NV_RESET},
	{ "static_hostname0", "DIR-730", NV_RESET},
	{ "static_addr0", "192.168.0.1", NV_RESET},
	{ "static_netmask0", "255.255.255.0", NV_RESET},
	{ "static_gateway0", "", NV_RESET},
	{ "static_primary_dns0", "", NV_RESET},
	{ "static_secondary_dns0", "", NV_RESET},
	{ "static_mac0", "", NV_RESET},
	{ "static_mtu0", "1500", NV_RESET},
	{ "static_alias1", "DMZ_static", NV_RESET},
	{ "static_hostname1", "DIR-730", NV_RESET},
	{ "static_addr1", "172.17.1.1", NV_RESET},
	{ "static_netmask1", "255.255.255.0", NV_RESET},
	{ "static_gateway1", "", NV_RESET},
	{ "static_primary_dns1", "", NV_RESET},
	{ "static_secondary_dns1", "", NV_RESET},
	{ "static_mac1", "", NV_RESET},
	{ "static_mtu1", "1500", NV_RESET},
	{ "static_alias2", "WAN_static", NV_RESET},
	{ "static_hostname2", "DIR-730", NV_RESET},
	{ "static_addr2", "", NV_RESET},
	{ "static_netmask2", "", NV_RESET},
	{ "static_gateway2", "", NV_RESET},
	{ "static_primary_dns2", "", NV_RESET},
	{ "static_secondary_dns2", "", NV_RESET},
	{ "static_mac2", "", NV_RESET},
	{ "static_mtu2", "1500", NV_RESET},
	{ "static_alias3", "WAN_static_rupppoe", NV_RESET},
	{ "static_hostname3", "DIR-730", NV_RESET},
	{ "static_addr3", "192.168.3.1", NV_RESET},
	{ "static_netmask3", "255.255.255.0", NV_RESET},
	{ "static_gateway3", "192.168.3.254", NV_RESET},
	{ "static_primary_dns3", "192.168.3.254", NV_RESET},
	{ "static_secondary_dns3", "", NV_RESET},
	{ "static_mac3", "", NV_RESET},
	{ "static_mtu3", "", NV_RESET},
	{ "static_alias4", "WAN_static_rupptp", NV_RESET},
	{ "static_hostname4", "DIR-730", NV_RESET},
	{ "static_addr4", "192.168.3.1", NV_RESET},
	{ "static_netmask4", "255.255.255.0", NV_RESET},
	{ "static_gateway4", "192.168.3.254", NV_RESET},
	{ "static_primary_dns4", "192.168.3.254", NV_RESET},
	{ "static_secondary_dns4", "", NV_RESET},
	{ "static_mac4", "", NV_RESET},
	{ "static_mtu4", "", NV_RESET},

/* dhcpc */

        { "dhcpd_alias0", "LAN_dhcpd", NV_RESET },      /* DHCP server assign addr*/
        { "dhcpd_start0", "192.168.0.100", NV_RESET },  /* DHCP server assign addr*/
	{ "dhcpd_end0", "192.168.0.200", NV_RESET },  /* DHCP server assign addr*/
        { "dhcpd_lease0", "86400", NV_RESET },          /* lease time in seconds */
        { "dhcpd_enable0", "1", NV_RESET },     /* "1" enable DHCP Server, "0" disable DHCP server , "2" dhcp relay */
        { "dhcpd_start1", "172.17.1.100", NV_RESET },   /* DHCP server assign addr*/
	{ "dhcpd_end1", "172.17.1.200", NV_RESET },  /* DHCP server assign addr*/
        { "dhcpd_lease1", "86400", NV_RESET },          /* lease time in seconds */
        { "dhcpd_enable1", "1", NV_RESET },     /* "1" enable DHCP Server, "0" disable DHCP server */
        { "dhcpd_alias4", "WLAN_dhcpd", NV_RESET },     /* DHCP server assign addr*/
        { "dhcpd_start4", "192.168.10.100", NV_RESET }, /* DHCP server assign addr*/
	{ "dhcpd_end4", "192.168.10.200", NV_RESET },  /* DHCP server assign addr*/

        { "dhcpd_lease4", "86400", NV_RESET },          /* lease time in seconds */
        { "dhcpd_enable4", "1", NV_RESET },     /* "1" enable DHCP Server, "0" disable DHCP server */

	
	{ "dhcpc_alias0", "LAN_dhcpc", NV_RESET},
	{ "dhcpc_hostname0", "DIR-730", NV_RESET},
	{ "dhcpc_addr0", "", NV_RESET},
	{ "dhcpc_netmask0", "", NV_RESET},
	{ "dhcpc_gateway0", "", NV_RESET},
	{ "dhcpc_primary_dns0", "", NV_RESET},
	{ "dhcpc_secondary_dns0", "", NV_RESET},
	{ "dhcpc_mac0", "", NV_RESET},
	{ "dhcpc_mtu0", "1500", NV_RESET},
	{ "dhcpc_alias1", "DMZ_dhcpc", NV_RESET},
	{ "dhcpc_hostname1", "DIR-730", NV_RESET},
	{ "dhcpc_addr1", "", NV_RESET},
	{ "dhcpc_netmask1", "", NV_RESET},
	{ "dhcpc_gateway1", "", NV_RESET},
	{ "dhcpc_primary_dns1", "", NV_RESET},
	{ "dhcpc_secondary_dns1", "", NV_RESET},
	{ "dhcpc_mac1", "", NV_RESET},
	{ "dhcpc_mtu1", "1500", NV_RESET},
	{ "dhcpc_alias2", "WAN_dhcpc", NV_RESET},
	{ "dhcpc_hostname2", "", NV_RESET},
	{ "dhcpc_addr2", "", NV_RESET},
	{ "dhcpc_netmask2", "", NV_RESET},
	{ "dhcpc_gateway2", "", NV_RESET},
	{ "dhcpc_mac2", "", NV_RESET},
	{ "dhcpc_primary_dns2", "", NV_RESET},
	{ "dhcpc_secondary_dns2", "", NV_RESET},
	{ "dhcpc_mtu2", "1500", NV_RESET},
	{ "dhcpc_alias3", "WAN_dhcpc_rupppoe", NV_RESET},
	{ "dhcpc_hostname3", "DIR-730", NV_RESET},
	{ "dhcpc_addr3", "", NV_RESET},
	{ "dhcpc_netmask3", "", NV_RESET},
	{ "dhcpc_gateway3", "", NV_RESET},
	{ "dhcpc_mac3", "", NV_RESET},
	{ "dhcpc_primary_dns3", "", NV_RESET},
	{ "dhcpc_secondary_dns3", "", NV_RESET},
	{ "dhcpc_mtu3", "", NV_RESET},
	{ "dhcpc_alias4", "WAN_dhcpc_rupptp", NV_RESET},
	{ "dhcpc_hostname4", "DIR-730", NV_RESET},
	{ "dhcpc_addr4", "", NV_RESET},
	{ "dhcpc_netmask4", "", NV_RESET},
	{ "dhcpc_gateway4", "", NV_RESET},
	{ "dhcpc_mac4", "", NV_RESET},
	{ "dhcpc_primary_dns4", "", NV_RESET},
	{ "dhcpc_secondary_dns4", "", NV_RESET},
	{ "dhcpc_mtu4", "", NV_RESET},
	
/*  pppoe */
	{ "pppoe_pass0", "1234", NV_RESET },            /* password */
        { "pppoe_idle0", "300", NV_RESET },     /* idle time by sec */
        { "pppoe_unit0", "0", NV_RESET },       /* the index of pppX, ex ppp3 */
        { "pppoe_dial0", "1", NV_RESET },       /* "0" manual, "1" always, "2" demand, "3" */
        { "pppoe_mode0", "1", NV_RESET },       /* "1" dynamic, "0" static*/
        { "pppoe_pass1", "1234", NV_RESET },            /* password */
        { "pppoe_idle1", "300", NV_RESET },     /* idle time by sec */
        { "pppoe_unit1", "0", NV_RESET },       /* the index of pppX, ex ppp3 */
        { "pppoe_dial1", "1", NV_RESET },       /* "0" manual, "1" always, "2" demand, "3" */
        { "pppoe_mode1", "1", NV_RESET },       /* "1" dynamic, "0" static*/
        { "pppoe_pass2", "", NV_RESET },                /* password */
        { "pppoe_idle2", "300", NV_RESET },     /* idle time by sec */
        { "pppoe_unit2", "0", NV_RESET },       /* the index of pppX, ex ppp3 */
        { "pppoe_dial2", "1", NV_RESET },       /* "0" manual, "1" always, "2" demand, "3" */
        { "pppoe_mode2", "1", NV_RESET },       /* "1" dynamic, "0" static*/
        { "pppoe_pass3", "1234", NV_RESET },            /* password */
        { "pppoe_idle3", "300", NV_RESET },     /* idle time by sec */
        { "pppoe_unit3", "0", NV_RESET },       /* the index of pppX, ex ppp3 */
        { "pppoe_dial3", "1", NV_RESET },       /* "0" manual, "1" always, "2" demand, "3" */
        { "pppoe_mode3", "1", NV_RESET },       /* "1" dynamic, "0" static*/
        { "pppoe_pass4", "1234", NV_RESET },            /* password */
        { "pppoe_idle4", "300", NV_RESET },     /* idle time by sec */
        { "pppoe_unit4", "0", NV_RESET },       /* the index of pppX, ex ppp3 */
        { "pppoe_dial4", "1", NV_RESET },       /* "0" manual, "1" always, "2" demand, "3" */
        { "pppoe_mode4", "1", NV_RESET },       /* "1" dynamic, "0" static*/
        { "pppoe_unnum_enable4", "", NV_RESET },        /* "1" Enable, "0" disable */
        { "pppoe_unnum_ip4", "", NV_RESET },    /* for unnumber ip */
        { "pppoe_pass5", "1234", NV_RESET },            /* password */
        { "pppoe_idle5", "300", NV_RESET },     /* idle time by sec */
        { "pppoe_unit5", "0", NV_RESET },       /* the index of pppX, ex ppp3 */
        { "pppoe_dial5", "1", NV_RESET },       /* "0" manual, "1" always, "2" demand, "3" */
        { "pppoe_mode5", "1", NV_RESET },       /* "1" dynamic, "0" static*/
        { "pppoe_ntt5", "1492", NV_RESET },     /* "0" East, "1" West */
        { "pppoe_policy_type5", "", NV_RESET }, /* "0" ip/domain, "1" tcp/udp protocol */
/* */
	{ "pppoe_alias0", "LAN_pppoe", NV_RESET},
	{ "pppoe_hostname0", "wan_pppoe", NV_RESET},
	{ "pppoe_servicename0", "wan_pppoe", NV_RESET},
	{ "pppoe_addr0", "", NV_RESET},
	{ "pppoe_netmask0", "", NV_RESET},
	{ "pppoe_primary_dns0", "", NV_RESET},
	{ "pppoe_secondary_dns0", "", NV_RESET},
	{ "pppoe_mtu0", "1492", NV_RESET},
	{ "pppoe_user0", "", NV_RESET},
	{ "pppoe_mru0", "1496", NV_RESET},
	{ "pppoe_mac0", "", NV_RESET},
	{ "pppoe_alias1", "DMZ_pppoe", NV_RESET},
	{ "pppoe_hostname1", "wan_pppoe", NV_RESET},
	{ "pppoe_servicename1", "wan_pppoe", NV_RESET},
	{ "pppoe_addr1", "", NV_RESET},
	{ "pppoe_netmask1", "", NV_RESET},
	{ "pppoe_primary_dns1", "", NV_RESET},
	{ "pppoe_secondary_dns1", "", NV_RESET},
	{ "pppoe_mtu1", "1492", NV_RESET},
	{ "pppoe_user1", "", NV_RESET},
	{ "pppoe_mru1", "", NV_RESET},
	{ "pppoe_mac1", "", NV_RESET},
	{ "pppoe_alias2", "WAN_pppoe", NV_RESET},
	{ "pppoe_hostname2", "wan_pppoe", NV_RESET},
	{ "pppoe_servicename2", "", NV_RESET},
	{ "pppoe_addr2", "", NV_RESET},
	{ "pppoe_netmask2", "", NV_RESET},
	{ "pppoe_primary_dns2", "", NV_RESET},
	{ "pppoe_secondary_dns2", "", NV_RESET},
	{ "pppoe_mtu2", "1492", NV_RESET},
	{ "pppoe_user2", "", NV_RESET},
	{ "pppoe_mru2", "1496", NV_RESET},
	{ "pppoe_mac2", "", NV_RESET},
	{ "pppoe_alias3", "WAN_pppoe_ru", NV_RESET},
	{ "pppoe_hostname3", "wan_pppoe_ru", NV_RESET},
	{ "pppoe_servicename3", "wan_pppoe_ru", NV_RESET},
	{ "pppoe_addr3", "", NV_RESET},
	{ "pppoe_netmask3", "", NV_RESET},
	{ "pppoe_primary_dns3", "", NV_RESET},
	{ "pppoe_secondary_dns3", "", NV_RESET},
	{ "pppoe_mtu3", "1492", NV_RESET},
	{ "pppoe_user3", "", NV_RESET},
	{ "pppoe_mru3", "1496", NV_RESET},
	{ "pppoe_mac3", "", NV_RESET},
	{ "pppoe_alias4", "multi_pppoe_primary", NV_RESET},
	{ "pppoe_hostname4", "multi_pppoe_primary", NV_RESET},
	{ "pppoe_servicename4", "multi_pppoe_primary", NV_RESET},
	{ "pppoe_addr4", "", NV_RESET},
	{ "pppoe_netmask4", "", NV_RESET},
	{ "pppoe_primary_dns4", "", NV_RESET},
	{ "pppoe_secondary_dns4", "", NV_RESET},
	{ "pppoe_mtu4", "1492", NV_RESET},
	{ "pppoe_user4", "", NV_RESET},
	{ "pppoe_mru4", "1496", NV_RESET},
	{ "pppoe_mac4", "", NV_RESET},
	{ "pppoe_unnum_netmask4", "", NV_RESET},
	{ "pppoe_alias5", "multi_pppoe_secondary", NV_RESET},
	{ "pppoe_hostname5", "multi_pppoe_secondary", NV_RESET},
	{ "pppoe_servicename5", "multi_pppoe_secondary", NV_RESET},
	{ "pppoe_addr5", "", NV_RESET},
	{ "pppoe_netmask5", "", NV_RESET},
	{ "pppoe_primary_dns5", "", NV_RESET},
	{ "pppoe_secondary_dns5", "", NV_RESET},
	{ "pppoe_mtu5", "1492", NV_RESET},
	{ "pppoe_user5", "", NV_RESET},
	{ "pppoe_mru5", "1496", NV_RESET},
	{ "pppoe_mac5", "", NV_RESET},
	
/* pptp */
        { "pptp_pass0", "1234", NV_RESET },             /* password */
        { "pptp_idle0", "300", NV_RESET },      /* idle time by sec */
        { "pptp_unit0", "0", NV_RESET },        /* the index of pppX, ex ppp3 */
        { "pptp_dial0", "1", NV_RESET },        /* "0" manual, "1" always, "2" demand, "3" */
        { "pptp_mode0", "1", NV_RESET },        /* "1" dynamic, "0" static*/
        { "pptp_mppe0", "nomppe", NV_RESET },   /* require-mppe|nomppe */
        { "pptp_pass1", "1234", NV_RESET },             /* password */
        { "pptp_idle1", "300", NV_RESET },      /* idle time by sec */
        { "pptp_unit1", "0", NV_RESET },        /* the index of pppX, ex ppp3 */
        { "pptp_dial1", "1", NV_RESET },        /* "0" manual, "1" always, "2" demand, "3" */
        { "pptp_mode1", "1", NV_RESET },        /* "1" dynamic, "0" static*/
        { "pptp_mppe1", "nomppe", NV_RESET },   /* require-mppe|nomppe */
        { "pptp_pass2", "1234", NV_RESET },             /* password */
        { "pptp_idle2", "300", NV_RESET },      /* idle time by sec */
        { "pptp_unit2", "0", NV_RESET },        /* the index of pppX, ex ppp3 */
        { "pptp_dial2", "1", NV_RESET },        /* "0" manual, "1" always, "2" demand, "3" */
        { "pptp_mode2", "1", NV_RESET },        /* "1" dynamic, "0" static*/
        { "pptp_mppe2", "nomppe", NV_RESET },   /* require-mppe|nomppe */
        { "pptp_pass3", "1234", NV_RESET },             /* password */
        { "pptp_idle3", "300", NV_RESET },      /* idle time by sec */
        { "pptp_unit3", "0", NV_RESET },        /* the index of pppX, ex ppp3 */
        { "pptp_dial3", "1", NV_RESET },        /* "0" manual, "1" always, "2" demand, "3" */
        { "pptp_mode3", "1", NV_RESET },        /* "1" dynamic, "0" static*/
        { "pptp_mppe3", "nomppe", NV_RESET },   /* require-mppe|nomppe */


	{ "pptp_alias0", "LAN_pptp", NV_RESET},
	{ "pptp_hostname0", "lan_pptp", NV_RESET},
	{ "pptp_addr0", "", NV_RESET},
	{ "pptp_netmask0", "", NV_RESET},
	{ "pptp_gateway0", "", NV_RESET},
	{ "pptp_dns0", "", NV_RESET},
	{ "pptp_mtu0", "1450", NV_RESET},
	{ "pptp_serverip0", "", NV_RESET},
	{ "pptp_user0", "vincent", NV_RESET},
	{ "pptp_mru0", "1496", NV_RESET},
	{ "pptp_remote0", "10.0.0.1", NV_RESET},
	{ "pptp_alias1", "DMZ_pptp", NV_RESET},
	{ "pptp_hostname1", "dmz_pptp", NV_RESET},
	{ "pptp_addr1", "", NV_RESET},
	{ "pptp_netmask1", "", NV_RESET},
	{ "pptp_gateway1", "", NV_RESET},
	{ "pptp_dns1", "", NV_RESET},
	{ "pptp_mtu1", "1450", NV_RESET},
	{ "pptp_serverip1", "", NV_RESET},
	{ "pptp_user1", "vincent", NV_RESET},
	{ "pptp_mru1", "1496", NV_RESET},
	{ "pptp_remote1", "10.0.0.1", NV_RESET},
	{ "pptp_alias2", "WAN_pptp", NV_RESET},
	{ "pptp_hostname2", "wan_pptp", NV_RESET},
	{ "pptp_addr2", "", NV_RESET},
	{ "pptp_netmask2", "", NV_RESET},
	{ "pptp_gateway2", "", NV_RESET},
	{ "pptp_dns2", "", NV_RESET},
	{ "pptp_mtu2", "1450", NV_RESET},
	{ "pptp_serverip2", "", NV_RESET},
	{ "pptp_user2", "", NV_RESET},
	{ "pptp_mru2", "1496", NV_RESET},
	{ "pptp_remote2", "10.0.0.1", NV_RESET},
	{ "pptp_alias3", "WAN_pptp_ru", NV_RESET},
	{ "pptp_hostname3", "wan_pptp_ru", NV_RESET},
	{ "pptp_addr3", "", NV_RESET},
	{ "pptp_netmask3", "", NV_RESET},
	{ "pptp_gateway3", "", NV_RESET},
	{ "pptp_dns3", "", NV_RESET},
	{ "pptp_mtu3", "1450", NV_RESET},
	{ "pptp_serverip3", "", NV_RESET},
	{ "pptp_user3", "", NV_RESET},
	{ "pptp_mru3", "1496", NV_RESET},
	{ "pptp_remote3", "10.0.0.1", NV_RESET},
	{ "pptp_mac3", "", NV_RESET},
	
/* L2tp */

        { "l2tp_pass2", "1234", NV_RESET },             /* password */
        { "l2tp_idle2", "300", NV_RESET },      /* idle time by sec */
        { "l2tp_unit2", "0", NV_RESET },        /* the index of pppX, ex ppp3 */
        { "l2tp_dial2", "1", NV_RESET },        /* "0" manual, "1" always, "2" demand, "3" */
        { "l2tp_mode2", "1", NV_RESET },        /* "1" dynamic, "0" static*/
        { "l2tp_mppe2", "nomppe", NV_RESET },   /* require-mppe|nomppe */

	{ "l2tp_alias2", "WAN_l2tp", NV_RESET},
	{ "l2tp_hostname2", "WAN_l2tp", NV_RESET},
	{ "l2tp_addr2", "", NV_RESET},
	{ "l2tp_netmask2", "", NV_RESET},
	{ "l2tp_gateway2", "", NV_RESET},
	{ "l2tp_dns2", "", NV_RESET},
	{ "l2tp_mtu2", "1450", NV_RESET},
	{ "l2tp_serverip2", "", NV_RESET},
	{ "l2tp_user2", "", NV_RESET},
	{ "l2tp_mru2", "1496", NV_RESET},
	
	/* USB3G PROTO BASE */
	{ "usb3g_alias2", "WAN_usb3g", NV_RESET},
	{ "usb3g_hostname2", "WAN_l2tp", NV_RESET},
	{ "usb3g_addr2", "", NV_RESET},
	{ "usb3g_netmask2", "", NV_RESET},
	{ "usb3g_gateway2", "", NV_RESET},
	{ "usb3g_dns2", "", NV_RESET},
	{ "usb3g_mtu2", "1492", NV_RESET},
	{ "usb3g_mru2", "1496", NV_RESET},
	/* USB3G PRPTO PPPXXX */
	{ "usb3g_user2", "", NV_RESET},
	{ "usb3g_pass2", "", NV_RESET},
	{ "usb3g_unit2", "0", NV_RESET},
	{ "usb3g_dial2", "1", NV_RESET},	/* "0" manual, "1" always, "2" demand, "3" */
	{ "usb3g_mode2", "1", NV_RESET},	/* "1" dynamic, "0" static*/
	{ "usb3g_mac2", "", NV_RESET},
	{ "usb3g_idle", "5", NV_RESET},
	{ "usb3g_mppe2", "nomppe", NV_RESET},	/* require-mppe|nomppe */
	/* USB3G EXTRA PARAMS */
	{ "usb3g_country2", "", NV_RESET},
	{ "usb3g_isp2", "", NV_RESET},
	{ "usb3g_dial_num2", "", NV_RESET},
	{ "usb3g_apn_name2", "", NV_RESET},
	{ "usb3g_auth", "", NV_RESET},
	{NULL, NULL}
};
