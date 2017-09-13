struct items_range inbound_filter[] = { 
	{ "inbound_list_", "", NV_RESET, 1, 10},
	/* policy_rule */
	{ "policy_rule", "", NV_RESET, 0, 24},
	/* multi_ssid */
	{ "multi_ssid_list", "", NV_RESET, 0, 2},
	{ "multi_ssid_security", "", NV_RESET, 0, 2},
        { "multi_ssid_psk", "", NV_RESET, 0, 2},
        { "multi_ssid_secret", "", NV_RESET, 0, 2},
        { "wlan2_wep64_key_", "0000000000", NV_RESET, 1, 4},
        { "wlan2_wep128_key_", "00000000000000000000000000", NV_RESET, 1, 4},
        { "wlan3_wep64_key_", "0000000000", NV_RESET, 1, 4},
        { "wlan3_wep128_key_", "00000000000000000000000000", NV_RESET, 1, 4},
        { "wlan4_wep64_key_", "0000000000", NV_RESET, 1, 4},
        { "wlan4_wep128_key_", "00000000000000000000000000", NV_RESET, 1, 4},
	{NULL, NULL}
};
