<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<style type="text/css">

/*
 * Wizard buttons at bottom wizard "page".
 */
#wz_buttons {
    margin-top: 1em;
    border: none;
}
</style>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="Javascript">

    var submit_button_flag = 0;
    var wan_detect_count = 0;
    var wan_detect_id = 0;
    var wan_detect_type;
    var cable_detect_count = 0;
    var cable_detect_id = 0;
    var cable_detect_status = "";
    var support_5g = 0;


    /*
     * Verification functions, if needed, for each page.
     * This array holds references to functions, one for each page of the wizard, to be called when
     * the user navigates away from the page.
     */
    var wz_verify = [];

    var wz_pg_welcome = 1;
    var wz_pg_wan_detect = 2;
    var wz_pg_fail_detect = 3;
    var wz_pg_unable_detect = 4;
    var wz_pg_wan_select = 5;
    var wz_pg_wan_type = 6;
    var wz_pg_wifi = 7;
    var wz_pg_password = 8;
    var wz_pg_time = 9;
    var wz_pg_summary = 10;

    /*
     * Wizard pages.
     */
    var wz_pg_cur = wz_pg_welcome;              /* Current wizard page */

    /*
     * wz_showhide()
     * Show/Hide wizard pages and buttons.
     */
    function wz_showhide()
    {
        for (var i=1 ;i <= wz_pg_summary; i++) {
            document.getElementById("wz_page_" + i).style.display = wz_pg_cur == i ? "" : "none";
        }

        if ((wz_pg_cur == wz_pg_welcome) || (wz_pg_cur == wz_pg_unable_detect)) {
            get_by_id("wz_prev_b").style.display = "none";
        }
        else {
            get_by_id("wz_prev_b").style.display = "";
        }

        if (wz_pg_cur == wz_pg_summary) {
            get_by_id("wz_save_b").style.display = "";
        }
        else {
            get_by_id("wz_save_b").style.display = "none";
        }

        if (wz_pg_cur == wz_pg_fail_detect) {
            get_by_id("wz_try_a").style.display = "";
        }
        else {
            get_by_id("wz_try_a").style.display = "none";
        }

        if (wz_pg_cur == wz_pg_unable_detect) {
            get_by_id("wz_try_b").style.display = "";
            get_by_id("wz_guide_b").style.display = "";
        }
        else {
            get_by_id("wz_try_b").style.display = "none";
            get_by_id("wz_guide_b").style.display = "none";
        }

        if ((wz_pg_cur == wz_pg_fail_detect) ||
            (wz_pg_cur == wz_pg_unable_detect) ||
            (wz_pg_cur == wz_pg_summary)) {
            get_by_id("wz_next_b").style.display = "none";
        }
        else {
            get_by_id("wz_next_b").style.display = "";
        }

        if (wz_pg_cur == wz_pg_wan_select) {
            wan_mode_selector(get_by_id("wan_proto").value);
        }

        scroll(0, 0);
    }

    /*
     * wz_next()
     * Validate current page and then submit the page form.
     * NOTE: It is assumed that the user is happy with the configuration up to this point
     * and so the configuration is submitted properly - and according to the oplock action.
     */
    function wz_next()
    {
        if (typeof(wz_verify[wz_pg_cur - 1]) == "function" && !wz_verify[wz_pg_cur - 1]()) {
            return;
        }

        if (wz_pg_cur == wz_pg_welcome) {
            send_cable_detect();
            return;
        }

        if ((wz_pg_cur == wz_pg_wan_select) && (get_by_id("wan_proto").value == "dhcpc")) {
            wz_pg_cur++;  // skip wan setting page
        }

        wz_pg_cur++;
        wz_showhide();
    }

    /*
     * wz_prev()
     * Show previous page.
     * NOTE: When going back to a prior page it is assumed that the user
     * has made a mistake on a prior page (which could affect the conditions of the current page).
     * To that effect, configuration data entered onto the current page is submitted WITHOUT validation
     * when going to the 'prev' page.
     */
    function wz_prev()
    {
        clearTimeout(wan_detect_id);
        clearTimeout(cable_detect_id);

        if (wz_pg_cur <= wz_pg_wan_select) {
            wz_pg_cur = wz_pg_welcome;
        }
        else if ((wz_pg_cur == wz_pg_wan_type) ||
                 ((wz_pg_cur == wz_pg_wifi) && (get_by_id("wan_proto").value == "dhcpc"))) {
            if ((wan_detect_type == "dhcpc") || (wan_detect_type == "pppoe")) {
                wz_pg_cur = wz_pg_welcome;
            }
            else {
                wz_pg_cur = wz_pg_wan_select;
            }
        }
        else {
            wz_pg_cur--;
        }

        wz_showhide();
    }

    /*
     * wz_verify_pwd()
     */
    function wz_verify_pwd()
    {
        /*
         * Password page.
         */
        if (get_by_id("admin_password").value != get_by_id("admin_password2").value) {
            alert(_pwsame);
            return false;
        }

        if (!is_ascii(get_by_id("admin_password").value)) {
            alert(S493);
            return false;
        }

        if (get_by_id("graphical_enable").checked) {
            get_by_id("graph_auth_enable").value = 1;
        }
        else {
            get_by_id("graph_auth_enable").value = 0;
        }

        return true;
    }

    /*
     * wz_verify_wan()
     */
    function wz_verify_wan()
    {
        var wan_mode = get_by_id("wan_proto").value;
        var check_ip_mask_gw = 0, wan_msg;
        var ip, mask, gateway;

        var pptp_type = get_by_name("wan_pptp_dynamic");
        var l2tp_type = get_by_name("wan_l2tp_dynamic");

        /*
         * WAN settings page.
         */
        if (wan_mode == "pppoe") {
            get_by_id("wan_pppoe_username_00").value = trim_string(get_by_id("wan_pppoe_username_00").value);
            if (get_by_id("wan_pppoe_username_00").value == "") {
                alert(GW_WAN_PPPOE_USERNAME_INVALID);
                return false;
            }
        }
        else if (wan_mode == "pptp") {
            if (!is_ipv4_valid(get_by_id("wan_pptp_ipaddr").value)) {
                alert(KR2);
                form1.wan_pptp_ipaddr.select();
                form1.wan_pptp_ipaddr.focus();
                return false;
            }
            if (!is_ipv4_valid(get_by_id("wan_pptp_netmask").value)) {
                alert(LS202);
                form1.wan_pptp_netmask.select();
                form1.wan_pptp_netmask.focus();
                return false;
            }
            if (!is_ipv4_valid(get_by_id("wan_pptp_gateway").value)) {
                alert(LS204);
                form1.wan_pptp_gateway.select();
                form1.wan_pptp_gateway.focus();
                return false;
            }

            var server_ip = get_by_id("wan_pptp_server_ip").value;
            var server_ip_msg = replace_msg(all_ip_addr_msg, bwn_PPTPSIPA);
            var temp_sip_obj = new addr_obj(server_ip.split("."), server_ip_msg, false, false);

            if (server_ip == "") {
                alert(YM108);
                return false;
            }

            /* In order to enter domain name */
            if (!check_servername(server_ip)) {
                if (!check_address(temp_sip_obj))
                return false;
            }

            get_by_id("wan_pptp_username").value = trim_string(get_by_id("wan_pptp_username").value);
            if (get_by_id("wan_pptp_username").value == "") {
                alert(GW_WAN_PPTP_USERNAME_INVALID);
                return false;
            }

            get_by_id("wan_pptp_password").value = trim_string(get_by_id("wan_pptp_password").value);
            if (get_by_id("wan_pptp_password").value != get_by_id("pptp_password2").value) {
                alert(_pwsame);
                form1.wan_pptp_password.select();
                form1.wan_pptp_password.focus();
                return false;
            }

            if (pptp_type[1].checked) {
                check_ip_mask_gw = 1;
                wan_msg = "PPTP";
                ip = get_by_id("wan_pptp_ipaddr").value;
                mask = get_by_id("wan_pptp_netmask").value;
                gateway = get_by_id("wan_pptp_gateway").value;
            }
        }
        else if (wan_mode == "l2tp") {
            if (!is_ipv4_valid(get_by_id("wan_l2tp_ipaddr").value)) {
                alert(KR2);
                form1.wan_l2tp_ipaddr.select();
                form1.wan_l2tp_ipaddr.focus();
                return false;
            }
            if (!is_ipv4_valid(get_by_id("wan_l2tp_netmask").value)) {
                alert(LS202);
                form1.wan_l2tp_netmask.select();
                form1.wan_l2tp_netmask.focus();
                return false;
            }
            if (!is_ipv4_valid(get_by_id("wan_l2tp_gateway").value)) {
                alert(LS204);
                form1.wan_l2tp_gateway.select();
                form1.wan_l2tp_gateway.focus();
                return false;
            }

            var server_ip = get_by_id("wan_l2tp_server_ip").value;
            var server_ip_msg = replace_msg(all_ip_addr_msg, bwn_L2TPSIPA);
            var temp_sip_obj = new addr_obj(server_ip.split("."), server_ip_msg, false, false);

            if (server_ip == "") {
                alert(bwn_alert_17);
                return false;
            }

            /* In order to enter domain name */
            if (!check_servername(server_ip)) {
                if (!check_address(temp_sip_obj))
                return false;
            }

            get_by_id("wan_l2tp_username").value = trim_string(get_by_id("wan_l2tp_username").value);
            if (get_by_id("wan_l2tp_username").value == "") {
                alert(GW_WAN_L2TP_USERNAME_INVALID);
                return false;
            }

            get_by_id("wan_l2tp_password").value = trim_string(get_by_id("wan_l2tp_password").value);
            if (get_by_id("wan_l2tp_password").value != get_by_id("l2tp_password2").value) {
                alert(_pwsame);
                form1.wan_l2tp_password.select();
                form1.wan_l2tp_password.focus();
                return false;
            }

            if (l2tp_type[1].checked) {
                check_ip_mask_gw = 1;
                wan_msg = "L2TP";
                ip = get_by_id("wan_l2tp_ipaddr").value;
                mask = get_by_id("wan_l2tp_netmask").value;
                gateway = get_by_id("wan_l2tp_gateway").value;
            }
        }
        else if (wan_mode == "static") {
            if (!is_ipv4_valid(get_by_id("wan_static_ipaddr").value)) {
                alert(KR2);
                form1.wan_static_ipaddr.select();
                form1.wan_static_ipaddr.focus();
                return false;
            }
            if (!is_ipv4_valid(get_by_id("wan_static_netmask").value)) {
                alert(LS202);
                form1.wan_static_netmask.select();
                form1.wan_static_netmask.focus();
                return false;
            }
            if (!is_ipv4_valid(get_by_id("wan_static_gateway").value)) {
                alert(LS204);
                form1.wan_static_gateway.select();
                form1.wan_static_gateway.focus();
                return false;
            }

            check_ip_mask_gw = 1;
            wan_msg = "WAN";
            ip = get_by_id("wan_static_ipaddr").value;
            mask = get_by_id("wan_static_netmask").value;
            gateway = get_by_id("wan_static_gateway").value;
        }

        if (check_ip_mask_gw == 1) {
            var ip_addr_msg = replace_msg(all_ip_addr_msg, "IP address");
            var subnet_mask_msg = replace_msg(all_ip_addr_msg, "Subnet Mask");
            var gateway_msg = replace_msg(all_ip_addr_msg, "Gateway address");

            var temp_ip_obj = new addr_obj(ip.split("."), ip_addr_msg, false, false);
            var temp_mask_obj = new addr_obj(mask.split("."), subnet_mask_msg, false, false);
            var temp_gateway_obj = new addr_obj(gateway.split("."), gateway_msg, false, false);

            if (!check_lan_setting(temp_ip_obj, temp_mask_obj, temp_gateway_obj, wan_msg)) {
                return false;
            }
        }

        /*
         * Allow blank as well as 0.0.0.0 for DNS servers.
         */
        get_by_id("wan_primary_dns").value = trim_string(get_by_id("wan_primary_dns").value);
        if (get_by_id("wan_primary_dns").value == "") {
            get_by_id("wan_primary_dns").value = "0.0.0.0";
        }

        get_by_id("wan_secondary_dns").value = trim_string(get_by_id("wan_secondary_dns").value);
        if (get_by_id("wan_secondary_dns").value == "") {
            get_by_id("wan_secondary_dns").value = "0.0.0.0";
        }

        if (!is_ipv4_valid(get_by_id("wan_primary_dns").value)) {
            alert(YM128);
            form1.wan_primary_dns.select();
            form1.wan_primary_dns.focus();
            return false;
        }

        if (!is_ipv4_valid(get_by_id("wan_secondary_dns").value)) {
            alert(YM129);
            form1.wan_secondary_dns.select();
            form1.wan_secondary_dns.focus();
            return false;
        }

        if ((get_by_id("wan_primary_dns").value == "0.0.0.0") && (get_by_id("wan_secondary_dns").value == "0.0.0.0")) {
            get_by_id("wan_specify_dns").value = 0;
        }
        else {
            get_by_id("wan_specify_dns").value = 1;
        }

        return true;
    }

    /*
     * wz_verify_wls()
     */
    function wz_verify_wls()
    {
        var psk_value;

        if (check_ssid("wlan0_ssid") == false)
            return false;

        get_by_id("show_ssid").innerHTML = get_by_id("wlan0_ssid").value;

        psk_value = get_by_id("wlan0_psk_pass_phrase").value;
        if (psk_value.length < 8) {
            alert(msg[PSK_LENGTH_ERROR]);
            return false;
        }
        else if (psk_value.length > 63) {
            if (!isHex(psk_value)) {
                alert(msg[THE_PSK_IS_HEX]);
                return false;
            }
        }
        get_by_id("show_key").innerHTML = get_by_id("wlan0_psk_pass_phrase").value;

        if (support_5g == 0) {
            return true;
        }

        // 5G
        if (check_ssid("wlan1_ssid") == false)
            return false;

        get_by_id("show_ssid_5g").innerHTML = get_by_id("wlan1_ssid").value;

        psk_value = get_by_id("wlan1_psk_pass_phrase").value;
        if (psk_value.length < 8) {
            alert(msg[PSK_LENGTH_ERROR]);
            return false;
        }
        else if (psk_value.length > 63) {
            if (!isHex(psk_value)) {
                alert(msg[THE_PSK_IS_HEX]);
                return false;
            }
        }

        get_by_id("show_key_5g").innerHTML = get_by_id("wlan1_psk_pass_phrase").value;
        return true;
    }

    function wz_cancel()
    {
        clearTimeout(wan_detect_id);
        clearTimeout(cable_detect_id);

        if (confirm(_ws_ns_00)) {
            if (submit_button_flag == 0) {
                submit_button_flag = 1;
                send_submit("form2");
            }
        }
    }

    /*
     * wan_mode_selector()
     */
    function wan_mode_selector(wan_cfg)
    {
        var wan_type = 0;   // dhcpc
        var wan_mode = get_by_name("wan_mode");

        if (wan_cfg == "pppoe")
            wan_type = 1;
        else if (wan_cfg == "pptp")
            wan_type = 2;
        else if (wan_cfg == "l2tp")
            wan_type = 3;
        else if (wan_cfg == "static")
            wan_type = 4;

        wan_mode[wan_type].checked = true;
        get_by_id("wan_proto").value = wan_cfg;

        if (wan_type <= 1) {
            get_by_id("wan_dns_settings_box").style.display = "none";
        }
        else {
            get_by_id("wan_dns_settings_box").style.display = "";
        }

        for (var i=1 ;i <= 4; i++) {
            document.getElementById("wan_ip_mode_box_" + i).style.display = (wan_type == i)? "block" : "none";
        }
    }

    function pptp_mode_selector(address_mode)
    {
        var pptp_mode = get_by_name("wan_pptp_dynamic");

        if (address_mode == "1") {
            pptp_mode[0].checked = true;
        }
        else {
            pptp_mode[1].checked = true;
        }

        pptp_mode.value = address_mode;
        get_by_id("wan_pptp_ipaddr").disabled = pptp_mode[0].checked;
        get_by_id("wan_pptp_netmask").disabled = pptp_mode[0].checked;
        get_by_id("wan_pptp_gateway").disabled = pptp_mode[0].checked;
    }

    function l2tp_mode_selector(address_mode)
    {
        var l2tp_mode = get_by_name("wan_l2tp_dynamic");

        if (address_mode == "1") {
            l2tp_mode[0].checked = true;
        }
        else {
            l2tp_mode[1].checked = true;
        }

        l2tp_mode.value = address_mode;
        get_by_id("wan_l2tp_ipaddr").disabled = l2tp_mode[0].checked;
        get_by_id("wan_l2tp_netmask").disabled = l2tp_mode[0].checked;
        get_by_id("wan_l2tp_gateway").disabled = l2tp_mode[0].checked;
    }

    function time_zone_selector()
    {
        get_by_id("time_zone_area").value = get_by_id("time_zone").selectedIndex;
    }

    function onPageLoad()
    {
        /*
         * Initialise the per-wizard page well-formedness checkers.
         * Some wizard pages may not have anything to be checked.
         */
        wz_verify[0] = null;
        wz_verify[1] = null;
        wz_verify[2] = null;
        wz_verify[3] = null;
        wz_verify[4] = null;
        wz_verify[5] = wz_verify_wan;
        wz_verify[6] = wz_verify_wls;
        wz_verify[7] = wz_verify_pwd;
        wz_verify[8] = null;
        wz_verify[9] = null;
        wz_verify[10] = null;
        wz_verify[11] = null;
        wz_verify[12] = null;

        /*
         * Show the wizard screen as appropriate.
         */
        wz_showhide();

        pptp_mode_selector("1");
        l2tp_mode_selector("1");
        get_by_id("time_zone").selectedIndex = get_by_id("time_zone_area").value;

        if ("<% CmoGetStatus("wlan1_mac_addr","none"); %>" != "") {
            get_by_id("box_5g").style.display = "";
            get_by_id("ssid_24g").style.display = "none";
            get_by_id("pass_24g").style.display = "none";
            get_by_id("summary_5g").style.display = "";
            get_by_id("band_single").style.display = "none";
            support_5g = 1;
        }
        else {
            get_by_id("ssid_pass").style.display = "none";
            get_by_id("band_dual").style.display = "none";

            get_by_id("wlan1_ssid").disabled = true;
            get_by_id("wlan1_psk_pass_phrase").disabled = true;
            get_by_id("wlan1_security").disabled = true;
            get_by_id("wlan1_psk_cipher_type").disabled = true;
        }

        var d = new Date();
        var gmtH = -d.getTimezoneOffset() / 60;
        var objtz = get_by_id("time_zone");
        var i;
        for (i = 0; i < objtz.options.length; i++) {
            if (objtz.options[i].value == (gmtH * 16))
                break;
        }

        if (i < objtz.options.length) {
            objtz.selectedIndex = i;
            get_by_id("time_zone_area").value = i;
        }
        else
            objtz.selectedIndex = get_by_id("time_zone_area").value;
    }

    function support_bookmark()
    {
        var isMSIE = (-[1,]) ? false : true;
        var is_support = 0;

        if (window.sidebar && window.sidebar.addPanel) { //Firefox
            is_support = 1;
        }
        else if (isMSIE && window.external) {  //IE favorite
            is_support = 2;
        }

        return is_support;
    }

    function save_connect()
    {
        var bookmark_ret = support_bookmark();

        if (bookmark_ret > 0) {
            if (confirm(_ws_ns_01)) {
                var title = _ws_ns_02;
                var lan_ip = "<% CmoGetCfg("lan_ipaddr","none"); %>";
                var web_url;
                var temp_cURL = document.URL.split("/");
                var mURL = temp_cURL[2];
                var hURL = temp_cURL[0];

                if (mURL == lan_ip) {
                    if (hURL == "https:")
                        web_url="https://" + lan_ip;
                    else
                        web_url="http://" + lan_ip;
                }
                else {
                    if (hURL == "https:")
                        web_url="https://" + mURL;
                    else
                        web_url="http://" + mURL;
                }

                if (bookmark_ret == 1) { // Mozilla Firefox Bookmark
                    window.sidebar.addPanel(title, web_url, "");
                }
                else if (bookmark_ret == 2) { // IE Favorite
                    window.external.AddFavorite(web_url, title);
                }
            }
        }

        if (submit_button_flag == 0) {
            submit_button_flag = 1;
            send_submit("form1");
        }
    }

    function createRequest()
    {
        var XMLhttpObject = null;
        try {
            XMLhttpObject = new XMLHttpRequest();
        } catch (e) {
            try {
                XMLhttpObject = new ActiveXObject("Msxml2.XMLHTTP");
            } catch (e) {
                try {
                    XMLhttpObject = new ActiveXObject("Microsoft.XMLHTTP");
                } catch (e) {
                    return null;
                }
            }
        }

        return XMLhttpObject;
    }

    function get_wan_type()
    {
        xmlhttp = new createRequest();
        if (xmlhttp) {
            var url = "";
            var temp_cURL = document.URL.split("/");

            for (var i=0 ;i < temp_cURL.length-1; i++) {
                if (i == 1) continue;

                if (i == 0)
                    url += temp_cURL[i] + "\x2F\x2F";
                else
                    url += temp_cURL[i] + "/";
            }

            if (wan_detect_count == 0) {
                wan_detect_count = 1;
                url += "device.xml=wan_detect_status_init";
            }
            else {
                url += "device.xml=wan_detect_status";
            }

            xmlhttp.onreadystatechange = wan_detect_xmldoc;
            xmlhttp.open("GET", url, true);
            xmlhttp.send(null);
            wan_detect_type = "";
        }

        wan_detect_id = setTimeout("get_wan_type()", 1000);
    }

    function wan_detect_xmldoc()
    {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            if (++wan_detect_count >= 3) {
                clearTimeout(wan_detect_id);
                wan_detect_type = xmlhttp.responseXML.getElementsByTagName("wan_detect_type")[0].firstChild.nodeValue;

                if (wan_detect_type == "dhcpc") {
                    wan_mode_selector(wan_detect_type);
                    wz_pg_cur = wz_pg_wifi;
                }
                else if (wan_detect_type == "pppoe") {
                    wan_mode_selector(wan_detect_type);
                    wz_pg_cur = wz_pg_wan_type;
                }
                else {
                    wz_pg_cur = wz_pg_unable_detect;
                }

                wz_showhide();
            }
        }
    }

    function get_cable_info()
    {
        xmlhttp = createRequest();
        if (xmlhttp) {
            var web_url;
            var lan_ip = "<% CmoGetCfg("lan_ipaddr","none"); %>";
            var temp_cURL = document.URL.split("/");
            var mURL = temp_cURL[2];
            var hURL = temp_cURL[0];

            if (mURL == lan_ip) {
                if (hURL == "https:")
                    web_url = "https://" + lan_ip + "/device.xml=device_status";
                else
                    web_url = "http://" + lan_ip + "/device.xml=device_status";
            }
            else {
                if (hURL == "https:")
                    web_url = "https://" + mURL + "/device.xml=device_status";
                else
                    web_url = "http://" + mURL + "/device.xml=device_status";
            }

            xmlhttp.onreadystatechange = cable_info_xmldoc;
            xmlhttp.open("GET", web_url, true);
            xmlhttp.send(null);
        }

        cable_detect_id = setTimeout("get_cable_info()", 1000);
    }

    function cable_info_xmldoc()
    {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            cable_detect_status = xmlhttp.responseXML.getElementsByTagName("wan_cable_status")[0].firstChild.nodeValue;

            if (cable_detect_status == "connect") {
                clearTimeout(cable_detect_id);
                send_wan_type_detect();
            }
            else if (++cable_detect_count >= 2) {
                clearTimeout(cable_detect_id);
                wz_pg_cur = wz_pg_fail_detect;
                wz_showhide();
            }
        }
    }

    function send_cable_detect()
    {
        get_by_id("loading").src = "loading.gif";
        wz_pg_cur = wz_pg_wan_detect;
        wz_showhide();

        cable_detect_count = 0;
        get_cable_info();
    }

    function send_wan_type_detect()
    {
        wan_detect_count = 0;
        get_wan_type();
    }

    function guide_me()
    {
        wan_mode_selector("dhcpc");
        wz_next();
    }
</script>

<meta http-equiv=Content-Type content="text/html; charset=iso-8859-1">
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
<table border=0 cellspacing=0 cellpadding=0 align=center width=838>
<tr>
<td bgcolor="#FFFFFF">
<table width=838 border=0 cellspacing=0 cellpadding=0 align=center height=100>
<tr>
<td bgcolor="#FFFFFF">
<div align=center>
  <table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
    <tr>
      <td width="100%"><script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
      <td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
      <td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
      <td>&nbsp;</td>
    </tr>
  </table>
  <img src="wlan_masthead.gif" width="836" height="92" align="middle">
     </div>
   </div></td>
</tr>
</table>
</div></td>
</tr>
<tr>
<td bgcolor="#FFFFFF">
  <br>
  <table width="650" border="0" align="center">
    <tr>
      <td>
        <div class=box>
            <form id="form2" name="form2" method="post" action="http://<% CmoGetCfg("lan_ipaddr","none"); %>/wizard_cancel.cgi"></form>

            <form id="form1" name="form1" method="post" action="http://<% CmoGetCfg("lan_ipaddr","none"); %>/wizard_apply.cgi">
            <input type="hidden" id="nvram_default_value" name="nvram_default_value" value="0">
            <input type="hidden" id="time_zone_area" name="time_zone_area" value="<% CmoGetCfg("time_zone_area","none"); %>">
            <input type="hidden" id="wan_proto" name="wan_proto" value="<% CmoGetCfg("wan_proto","none"); %>">
            <input type="hidden" id="wan_specify_dns" name="wan_specify_dns">
            <input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
            <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="http://www.dlink.com">
            <input type="hidden" id="reboot_type" name="reboot_type" value="shutdown">

            <div id="wz_page_1" style="display:none">
                <h2 align="left"><script>show_words(_ws_ns_03)</script></h2>
                <p class="box_msg"><script>show_words(wwa_intro_wel)</script>.</p>
                <table class=formarea>
                    <tr>
                      <td width="334" height="81">
                        <UL>
                            <LI><script>show_words(Tag06056)</script>: <script>show_words(Tag06079)</script>
                            <LI><script>show_words(Tag06057)</script>: <script>show_words(Tag06106)</script>
                            <LI><script>show_words(Tag06058)</script>: <script>show_words(Tag06076)</script>
                            <LI><script>show_words(Tag06059)</script>: <script>show_words(Tag06077)</script>
                            <LI><script>show_words(Tag06060)</script>: <script>show_words(_savesettings)</script>
                        </UL>
                      </TD>
                    </tr>
                </table>
            </div><!-- wz_page_1 -->

            <div id="wz_page_2" style="display:none">
                <h2 align="left"><script>show_words(Tag06056)</script>: <script>show_words(Tag06079)</script></h2>
                <p class="box_msg"><script>show_words(_ws_ns_09)</script> ...</p>
                <table class=formarea>
                  <tr align="center">
                    <td width="10%">&nbsp;</td>
                    <td><img id="loading"></td>
                  </tr>
                </table>
            </div><!-- wz_page_2 -->

            <div id="wz_page_3" style="display:none">
                <h2 align="left"><script>show_words(Tag06056)</script>: <script>show_words(Tag06079)</script></h2>
                <p class="box_msg"><script>show_words(Tag06015)</script></p>
                <img src=network.jpg>
            </div><!-- wz_page_3 -->

            <div id="wz_page_4" style="display:none">
                <h2 align="left"><script>show_words(Tag06056)</script>: <script>show_words(Tag06079)</script></h2>
                <p class="box_msg"><script>show_words(_ws_ns_10)</script>.</p>
            </div><!-- wz_page_4 -->

            <div id="wz_page_5" style="display:none">
                <h2 align="left"><script>show_words(wwa_title_s3)</script></h2>
                <P align="left" class=box_msg><script>show_words(_ws_ns_11)</script>:</P>
                <table class=formarea >
                    <tr>
                      <td class=form_label>&nbsp;</td>
                      <td><input name="wan_mode" type="radio" value="dhcpc" checked="true" onchange="wan_mode_selector(this.value);">
                          <STRONG><script>show_words(_dhcpconn)</script></STRONG>
                          <div class=itemhelp><script>show_words(wwa_msg_dhcp)</script></div></td></tr>
                    <tr>
                      <td class=form_label>&nbsp;</td>
                      <td><input name="wan_mode" type="radio" value="pppoe" onchange="wan_mode_selector(this.value);">
                          <STRONG><script>show_words(wwa_wanmode_pppoe)</script></STRONG>
                          <div class=itemhelp><script>show_words(wwa_msg_pppoe)</script></div></td></tr>
                    <tr>
                      <td class=form_label>&nbsp;</td>
                      <td><input name="wan_mode" type="radio" value="pptp" onchange="wan_mode_selector(this.value);">
                          <STRONG><script>show_words(wwa_wanmode_pptp)</script></STRONG>
                          <div class=itemhelp><script>show_words(wwa_msg_pptp)</script></div></td>
                    </tr>
                    <tr>
                      <td class=form_label>&nbsp;</td>
                      <td><input name="wan_mode" type="radio" value="l2tp" onchange="wan_mode_selector(this.value);">
                          <STRONG><script>show_words(wwa_wanmode_l2tp)</script></STRONG>
                          <div class=itemhelp><script>show_words(wwa_msg_l2tp)</script></div></td>
                    </tr>
                    <tr>
                      <td class=form_label>&nbsp;</td>
                      <td><input name="wan_mode" type="radio" value="static" onchange="wan_mode_selector(this.value);">
                          <STRONG><script>show_words(wwa_wanmode_sipa)</script></STRONG>
                          <div class=itemhelp><script>show_words(wwa_msg_sipa)</script></div></td>
                    </tr>
                </table>
                <br>
            </div><!-- wz_page_5 -->

            <div id="wz_page_6" style="display:none">
                <div id="wan_ip_mode_box_1" style="display:none">
                <h2 align="left"><script>show_words(wwa_title_set_pppoe);</script></h2>
                <p class="box_msg"><script>show_words(wwa_msg_set_pppoe)</script></p>
                <table align="center" class=formarea>
                    <tr>
                      <td width="170" align=right class="duple"><script>show_words(_username)</script> :</td>
                      <td width="430">&nbsp;<input type=text id="wan_pppoe_username_00" name="wan_pppoe_username_00" size="20" maxlength="63" value="<% CmoGetCfg("wan_pppoe_username_00","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_password)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pppoe_password_00" name="wan_pppoe_password_00" size="20" maxlength="63" value="<% CmoGetCfg("wan_pppoe_password_00","none"); %>"></td>
                    </tr>
                </table>
                </div><!-- wan_ip_mode_box_1 -->

                <div id="wan_ip_mode_box_2" style="display:none">
                <h2 align="left"><script>show_words(wwa_title_set_pptp)</script></h2>
                <p class="box_msg"><script>show_words(wwa_msg_set_pptp)</script></p>
                <table align="center" class=formarea>
                    <tr>
                      <td width="170" align=right class="duple">Address Mode :</td>
                      <td width="430">&nbsp;
                        <input name="wan_pptp_dynamic" type="radio" value="1" onClick="pptp_mode_selector(this.value)">
                        <script>show_words(carriertype_ct_0)</script>&nbsp;&nbsp;
                        <input name="wan_pptp_dynamic" type="radio" value="0" onClick="pptp_mode_selector(this.value)">
                        <script>show_words(_sdi_staticip)</script>
                     </td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_PPTPip)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pptp_ipaddr" name="wan_pptp_ipaddr" size="20" maxlength="15" value="<% CmoGetCfg("wan_pptp_ipaddr","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_PPTPsubnet)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pptp_netmask" name="wan_pptp_netmask" size="20" maxlength="15" value="<% CmoGetCfg("wan_pptp_netmask","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_PPTPgw)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pptp_gateway" name="wan_pptp_gateway" size="20" maxlength="15" value="<% CmoGetCfg("wan_pptp_gateway","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(wwa_pptp_svraddr)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pptp_server_ip" name="wan_pptp_server_ip" size="20" value="<% CmoGetCfg("wan_pptp_server_ip","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_username)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pptp_username" name="wan_pptp_username" size="20" maxlength="63" value="<% CmoGetCfg("wan_pptp_username","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_password)</script> :</td>
                      <td>&nbsp;<input type=password id="wan_pptp_password" name="wan_pptp_password" size="20" maxlength="63" value="<% CmoGetCfg("wan_pptp_password","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_verifypw)</script> :</td>
                      <td>&nbsp;<input type=password id="pptp_password2" name="pptp_password2" size="20" maxlength="63" value="<% CmoGetCfg("wan_pptp_password","none"); %>"></td>
                    </tr>
                </table>
                </div><!-- wan_ip_mode_box_2 -->

                <div id="wan_ip_mode_box_3" style="display:none">
                <h2 align="left"><script>show_words(wwa_set_l2tp_title)</script></h2>
                <p class="box_msg"><script>show_words(wwa_set_l2tp_msg)</script></p>
                <table align="center" class=formarea>
                    <tr>
                      <td width="170" align=right class="duple"><script>show_words(bwn_AM)</script> :</td>
                      <td width="430">&nbsp;
                        <input name="wan_l2tp_dynamic" type="radio" value="1" onClick="l2tp_mode_selector(this.value)">
                        <script>show_words(carriertype_ct_0)</script>&nbsp;&nbsp;
                        <input name="wan_l2tp_dynamic" type="radio" value="0" onClick="l2tp_mode_selector(this.value)">
                        <script>show_words(_sdi_staticip)</script>
                     </td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_L2TPip)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_l2tp_ipaddr" name="wan_l2tp_ipaddr" size="20" maxlength="15" value="<% CmoGetCfg("wan_l2tp_ipaddr","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_L2TPsubnet)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_l2tp_netmask" name="wan_l2tp_netmask" size="20" maxlength="15" value="<% CmoGetCfg("wan_l2tp_netmask","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_L2TPgw)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_l2tp_gateway" name="wan_l2tp_gateway" size="20" maxlength="15" value="<% CmoGetCfg("wan_l2tp_gateway","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(wwa_l2tp_svra)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_l2tp_server_ip" name="wan_l2tp_server_ip" size="20" value="<% CmoGetCfg("wan_l2tp_server_ip","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_username)</script></td>
                      <td>&nbsp;<input type=text id="wan_l2tp_username" name="wan_l2tp_username" size="20" maxlength="63" value="<% CmoGetCfg("wan_l2tp_username","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_password)</script> :</td>
                      <td>&nbsp;<input type=password id="wan_l2tp_password" name="wan_l2tp_password" size="20" maxlength="63" value="<% CmoGetCfg("wan_l2tp_password","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_verifypw)</script> :</td>
                      <td>&nbsp;<input type=password id="l2tp_password2" name="l2tp_password2" size="20" maxlength="63" value="<% CmoGetCfg("wan_l2tp_password","none"); %>"></td>
                    </tr>
                </table>
                </div><!-- wan_ip_mode_box_3 -->

                <div id="wan_ip_mode_box_4" style="display:none">
                <h2 align="left"><script>show_words(wwa_set_sipa_title)</script></h2>
                <p class="box_msg"><script>show_words(wwa_set_sipa_msg)</script></p>
                <table align="center" class=formarea>
                  <tr>
                    <td width="170" align=right class="duple"><script>show_words(help256)</script>&nbsp;:</td>
                    <td width="430">&nbsp;<input type=text id="wan_static_ipaddr" name="wan_static_ipaddr" size="20" maxlength="15" value="<% CmoGetCfg("wan_static_ipaddr","none"); %>"></td>
                  </tr>
                  <tr>
                    <td align=right class="duple"><script>show_words(_subnet)</script>&nbsp;:</td>
                    <td>&nbsp;<input type=text id="wan_static_netmask" name="wan_static_netmask" size="20" maxlength="15" value="<% CmoGetCfg("wan_static_netmask","none"); %>"></td>
                  </tr>
                  <tr>
                    <td align=right class="duple"><script>show_words(wwa_gw)</script>&nbsp;:</td>
                    <td>&nbsp;<input type=text id="wan_static_gateway" name="wan_static_gateway" size="20" maxlength="15" value="<% CmoGetCfg("wan_static_gateway","none"); %>"></td>
                  </tr>
                </table>
                </div><!-- wan_ip_mode_box_4 -->

                <br>
                <div id="wan_dns_settings_box" style="display:none">
                  <h2 align="left"><script>show_words(wwa_dnsset)</script></h2>
                  <table align="center" class=formarea>
                    <tr>
                      <td width="170" align=right class="duple"><script>show_words(wwa_pdns)</script> :</td>
                      <td width="430">&nbsp;<input type=text id="wan_primary_dns" name="wan_primary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_primary_dns","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(wwa_sdns)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_secondary_dns" name="wan_secondary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_secondary_dns","none"); %>"></td>
                    </tr>
                  </table>
                  <br>
                </div><!-- wan_dns_settings_box -->
            </div><!-- wz_page_6 -->

            <div id="wz_page_7" style="display:none">
                <input type="hidden" id="wlan0_security" name="wlan0_security" value="wpa2auto_psk">
                <input type="hidden" id="wlan0_psk_cipher_type" name="wlan0_psk_cipher_type" value="both">
                <input type="hidden" id="wlan1_security" name="wlan1_security" value="wpa2auto_psk">
                <input type="hidden" id="wlan1_psk_cipher_type" name="wlan1_psk_cipher_type" value="both">
                <input type="hidden" id="wps_configured_mode" name="wps_configured_mode" value="5">
                <input type="hidden" id="disable_wps_pin" name="disable_wps_pin" value="1">
                <input type="hidden" id="wps_enable" name="wps_enable" value="1">

                <h2 align="left"><script>show_words(Tag06057)</script>: <script>show_words(Tag06106)</script></h2>
                <table class=formarea width="80%">
                    <tr>
                      <td width="10%">&nbsp;</td>
                      <td width="90%">
                        <br>
                        <P id="ssid_24g" class=box_msg><script>show_words(Tag06109)</script></P>
                        <P id="ssid_pass" class=box_msg><script>show_words(Tag06111)</script>
                        (<script>show_words(GW_WLAN_RADIO_0_NAME)</script>)</P>
                        <P><b>Wi-Fi <script>show_words(sd_NNSSID)</script> :</b></P>
                        <input type="text" id="wlan0_ssid" name="wlan0_ssid" size="40" maxlength="32" value="<% CmoGetCfg("wlan0_ssid","none"); %>">
                        (<script>show_words(Tag06113)</script>)
                        <br><br>
                        <P id="pass_24g" class=box_msg><script>show_words(Tag06110)</script></P>
                        <P><b><script>show_words(Tag06115)</script> :</b></P>
                        <input type="text" id="wlan0_psk_pass_phrase" name="wlan0_psk_pass_phrase" size="40" maxlength="63">
                        (<script>show_words(Tag06116)</script>)<br><br></td>
                    </tr>
                </table>

                <div id="box_5g" style="display:none">
                <table class=formarea width="80%">
                    <tr>
                      <td width="10%">&nbsp;</td>
                      <td width="90%">
                        <P class=box_msg><script>show_words(Tag06111)</script> (<script>show_words(GW_WLAN_RADIO_1_NAME)</script>)</P>
                        <P><b>Wi-Fi <script>show_words(sd_NNSSID)</script> :</b></P>
                        <input type="text" id="wlan1_ssid" name="wlan1_ssid" size="40" maxlength="32" value="<% CmoGetCfg("wlan1_ssid","none"); %>">
                        (<script>show_words(Tag06113)</script>)
                        <br><br>
                        <P><b><script>show_words(Tag06115)</script> :</b></P>
                        <input type="text" id="wlan1_psk_pass_phrase" name="wlan1_psk_pass_phrase" size="40" maxlength="63">
                        (<script>show_words(Tag06116)</script>)<br><br></td>
                    </tr>
                </table>
                </div>
            </div><!-- wz_page_7 -->

            <div id="wz_page_8" style="display:none">
                <h2 align="left"><script>show_words(Tag06058)</script>: <script>show_words(Tag06076)</script></h2>
                <p class="box_msg"><script>show_words(_ws_ns_06)</script></p>
                <table class=formarea>
                    <tr>
                      <td align=right class="duple"><script>show_words(_password)</script>:</td>
                      <td>&nbsp;<input type="password" id="admin_password" name="admin_password" size=20 maxlength=15 value="<% CmoGetCfg("admin_password","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_verifypw)</script> :</td>
                      <td>&nbsp;<input type="password" id="admin_password2" name="admin_password2" size=20 maxlength=15 value="<% CmoGetCfg("admin_password","none"); %>"></td>
                    </tr>
                    <tr>
                      <td class="duple"><script>show_words(_graph_auth)</script> :</td>
                      <td><input name="graphical_enable" type="checkbox" id="graphical_enable">
                          <input type="hidden" id="graph_auth_enable" name="graph_auth_enable"></td>
                    </tr>
                </table>
                <br>
            </div><!-- wz_page_8 -->

            <div id="wz_page_9" style="display:none">
                <h2 align="left"><script>show_words(Tag06059)</script>: <script>show_words(Tag06077)</script></h2>
                <p class="box_msg"><script>show_words(wwa_intro_s2)</script></p>
                <P align="center">
                <select size=1 id="time_zone" name="time_zone" onchange="time_zone_selector();">
			<option value="-192"><font face=Arial><script>show_words(up_tz_00)</script></font></option>
			<option value="-176"><font face=Arial><script>show_words(up_tz_01)</script></font></option>
			<option value="-160"><font face=Arial><script>show_words(up_tz_02)</script></font></option>
			<option value="-144"><font face=Arial><script>show_words(up_tz_03)</script></font></option>
			<option value="-128"><font face=Arial><script>show_words(up_tz_04)</script></font></option>
			<option value="-112"><font face=Arial><script>show_words(up_tz_05)</script></font></option>
			<option value="-112"><font face=Arial><script>show_words(up_tz_06)</script></font></option>
			<option value="-96"><font face=Arial><script>show_words(up_tz_07)</script></font></option>
			<option value="-96"><font face=Arial><script>show_words(up_tz_08)</script></font></option>
			<option value="-96"><font face=Arial><script>show_words(up_tz_09)</script></font></option>
			<option value="-96"><font face=Arial><script>show_words(up_tz_10)</script></font></option>
			<option value="-80"><font face=Arial><script>show_words(up_tz_11)</script></font></option>                  
			<option value="-80"><font face=Arial><script>show_words(up_tz_12)</script></font></option>
			<option value="-80"><font face=Arial><script>show_words(up_tz_13)</script></font></option>
			<option value="-64"><font face=Arial><script>show_words(up_tz_14)</script></font></option>
			<option value="-64"><font face=Arial><script>show_words(up_tz_15)</script></font></option>
			<option value="-64"><font face=Arial><script>show_words(up_tz_16)</script></font></option>
			<option value="-56"><font face=Arial><script>show_words(up_tz_17)</script></font></option>
			<option value="-48"><font face=Arial><script>show_words(up_tz_18)</script></font></option>
			<option value="-48"><font face=Arial><script>show_words(up_tz_19)</script></font></option>
			<option value="-48"><font face=Arial><script>show_words(up_tz_20)</script></font></option>
			<option value="-32"><font face=Arial><script>show_words(up_tz_21)</script></font></option>
			<option value="-16"><font face=Arial><script>show_words(up_tz_22)</script></font></option>
			<option value="-16"><font face=Arial><script>show_words(up_tz_23)</script></font></option>
			<option value="0"><font face=Arial><script>show_words(up_tz_24)</script></font></option>
			<option value="0"><font face=Arial><script>show_words(up_tz_25)</script></font></option>
			<option value="16"><font face=Arial><script>show_words(up_tz_26)</script></font></option>
			<option value="16"><font face=Arial><script>show_words(up_tz_27)</script></font></option>
			<option value="16"><font face=Arial><script>show_words(up_tz_28)</script></font></option>
			<option value="16"><font face=Arial><script>show_words(up_tz_29)</script></font></option>
			<option value="16"><font face=Arial><script>show_words(up_tz_29b)</script></font></option>
			<option value="16"><font face=Arial><script>show_words(up_tz_30)</script></font></option>
			<option value="32"><font face=Arial><script>show_words(up_tz_31)</script></font></option>
			<option value="32"><font face=Arial><script>show_words(up_tz_32)</script></font></option>
			<option value="32"><font face=Arial><script>show_words(up_tz_33)</script></font></option>
			<option value="32"><font face=Arial><script>show_words(up_tz_34)</script></font></option>
			<option value="32"><font face=Arial><script>show_words(up_tz_35)</script></font></option>
			<option value="32"><font face=Arial><script>show_words(up_tz_36)</script></font></option>
			<option value="48"><font face=Arial><script>show_words(up_tz_37)</script></font></option>
			<option value="48"><font face=Arial><script>show_words(up_tz_38)</script></font></option>
			<option value="48"><font face=Arial><script>show_words(up_tz_40)</script></font></option>
			<option value="56"><font face=Arial><script>show_words(up_tz_41)</script></font></option>
			<option value="64"><font face=Arial><script>show_words(up_tz_39)</script></font></option>
			<option value="64"><font face=Arial><script>show_words(up_tz_42)</script></font></option>
			<option value="64"><font face=Arial><script>show_words(up_tz_43)</script></font></option>
			<option value="72"><font face=Arial><script>show_words(up_tz_44)</script></font></option>
			<option value="80"><font face=Arial><script>show_words(up_tz_46)</script></font></option>
			<option value="88"><font face=Arial><script>show_words(up_tz_47)</script></font></option>
			<option value="92"><font face=Arial><script>show_words(up_tz_48)</script></font></option>
			<option value="96"><font face=Arial><script>show_words(up_tz_50)</script></font></option>
			<option value="96"><font face=Arial><script>show_words(up_tz_51)</script></font></option>
			<option value="96"><font face=Arial><script>show_words(up_tz_45)</script></font></option>
			<option value="104"><font face=Arial><script>show_words(up_tz_52)</script></font></option>
			<option value="112"><font face=Arial><script>show_words(up_tz_49)</script></font></option>
			<option value="112"><font face=Arial><script>show_words(up_tz_53)</script></font></option>
			<option value="128"><font face=Arial><script>show_words(up_tz_54)</script></font></option>
			<option value="128"><font face=Arial><script>show_words(up_tz_55)</script></font></option>
			<option value="128"><font face=Arial><script>show_words(up_tz_57)</script></font></option>
			<option value="128"><font face=Arial><script>show_words(up_tz_58)</script></font></option>
			<option value="128"><font face=Arial><script>show_words(up_tz_59)</script></font></option>
			<option value="144"><font face=Arial><script>show_words(up_tz_60)</script></font></option>
			<option value="144"><font face=Arial><script>show_words(up_tz_56)</script></font></option>
			<option value="144"><font face=Arial><script>show_words(up_tz_61)</script></font></option>
			<option value="152"><font face=Arial><script>show_words(up_tz_63)</script></font></option>
			<option value="152"><font face=Arial><script>show_words(up_tz_64)</script></font></option>
			<option value="160"><font face=Arial><script>show_words(up_tz_62)</script></font></option>
			<option value="160"><font face=Arial><script>show_words(up_tz_65)</script></font></option>
			<option value="160"><font face=Arial><script>show_words(up_tz_66)</script></font></option>
			<option value="160"><font face=Arial><script>show_words(up_tz_67)</script></font></option>
			<option value="160"><font face=Arial><script>show_words(up_tz_68)</script></font></option>
			<option value="172"><font face=Arial><script>show_words(up_tz_69)</script></font></option>
			<option value="192"><font face=Arial><script>show_words(up_tz_70)</script></font></option>
			<option value="192"><font face=Arial><script>show_words(up_tz_71)</script></font></option>
			<option value="192"><font face=Arial><script>show_words(up_tz_72)</script></font></option>
			<option value="208"><font face=Arial><script>show_words(up_tz_73)</script></font></option>
                </select>
                </p>
            </div><!-- wz_page_9 -->

            <div id="wz_page_10" style="display:none">
                <h2 align="left"><script>show_words(_setupdone)</script></h2>
                <P align="left" class=box_msg><script>show_words(Tag06124)</script></P><br>

                <table class=formarea width="80%">
                  <tr>
                    <td width="65%" align="right">
                      <div id="band_single"><b>Wi-Fi <script>show_words(sd_NNSSID)</script> :</b><br><br></div>
                      <div id="band_dual"><b>Wi-Fi <script>show_words(sd_NNSSID)</script> <script>show_words(GW_WLAN_RADIO_0_NAME)</script> :</b><br><br></div>
                      </td>
                    <td width="35%"><span id="show_ssid"></span><br><br></td>
                  </tr>
                  <tr>
                    <td align="right"><b><script>show_words(Tag06115)</script> :</b><br><br></td>
                    <td><span id="show_key"></span><br><br></td>
                  </tr>
                </table>

                <div id="summary_5g" style="display:none">
                <table class=formarea width="80%">
                  <tr>
                    <td width="65%" align="right">
                      <b>Wi-Fi <script>show_words(sd_NNSSID)</script>) <script>show_words(GW_WLAN_RADIO_1_NAME)</script> :</b><br><br></td>
                    <td width="35%"><span id="show_ssid_5g"></span><br><br></td>
                  </tr>
                  <tr>
                    <td align="right"><b><script>show_words(Tag06115)</script> :</b><br><br></td>
                    <td><span id="show_key_5g"></span><br><br></td>
                  </tr>
                </table>
                </div>

                <br>
                <P align="left" class=box_msg><script>show_words(Tag06123)</script></P>
            </div><!-- wz_page_10 -->
            </form>

            <table align="center" class="formarea">
              <tr>
                <td>
                <fieldset id="wz_buttons">
                  <input type="button" class="button_submit" style="display:none" id="wz_prev_b" name="wz_prev_b" onclick="wz_prev();">
                  <input type="button" class="button_submit" style="display:none" id="wz_next_b" name="wz_next_b" onclick="wz_next();">
                  <input type="button" class="button_submit" style="display:none" id="wz_try_a" name="wz_try_a" onclick="send_cable_detect();">
                  <input type="button" class="button_submit" style="display:none" id="wz_save_b" name="wz_save_b" onclick="save_connect();">
                  <input type="button" class="button_submit" id="wz_cancel_b" name="wz_cancel_b" onclick="wz_cancel();">
                  <input type="button" class="button_submit" style="display:none" id="wz_try_b" name="wz_try_b" onclick="send_cable_detect();">
                  <input type="button" class="button_submit" style="display:none" id="wz_guide_b" name="wz_guide_b" onclick="guide_me();">
                    <script>get_by_id("wz_prev_b").value = _prev;</script>
                    <script>get_by_id("wz_next_b").value = _next;</script>
                    <script>get_by_id("wz_try_a").value = _connect;</script>
                    <script>get_by_id("wz_save_b").value = _save;</script>
                    <script>get_by_id("wz_cancel_b").value = _cancel;</script>
                    <script>get_by_id("wz_try_b").value = IPv6_wizard_button_0;</script>
                    <script>get_by_id("wz_guide_b").value = _ws_ns_17;</script>
                </fieldset>
                </td>
              </tr>
            </table>
         </div>
      </td>
    </tr>
  </table>
  <p>&nbsp;</p>
  </td>
</tr>
<tr>
  <td bgcolor="#FFFFFF"><table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="836" align="center">
    <tr>
      <td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
      <td width="10">&nbsp;</td>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
    </tr>
  </table></td>
</tr>
</table>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
</body>
<script>
    onPageLoad();
</script>
</html>
