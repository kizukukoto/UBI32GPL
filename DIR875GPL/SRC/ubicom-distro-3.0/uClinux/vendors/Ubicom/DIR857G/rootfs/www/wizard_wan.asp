<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<style type="text/css">
/*
 * Styles used only on this page.
 * WAN mode radio buttons
 */
#wan_modes p {
    margin-bottom: 1px;
}
#wan_modes input {
    float: left;
    margin-right: 1em;
}
#wan_modes label.duple {
    float: none;
    width: auto;
    text-align: left;
}
#wan_modes .itemhelp {
    margin: 0 0 1em 2em;
}

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

    /*
     * Verification functions, if needed, for each page.
     * This array holds references to functions, one for each page of the wizard, to be called when
     * the user navigates away from the page.
     */
    var wz_verify = [];

    /*
     * Wizard pages.
     */
    var wz_pg_min = 1;              /* First wizard page */
    var wz_pg_max = 6;              /* Last wizard page */
    var wz_pg_wan_setup = 1;        /* First page of WAN setup within wizard process */
    var wz_pg_prev = 1;             /* Last wizard page (only useful when we identify verify failures) */
    var wz_pg_cur = 1;              /* Current wizard page */

    /*
     * wz_showhide()
     * Show/Hide wizard pages and buttons.
     */
    function wz_showhide()
    {
        for (var i = 1; i <= wz_pg_max; i++) {
            document.getElementById("wz_page_" + i).style.display = wz_pg_cur == i ? "block" : "none";
        }

        document.getElementById("wz_prev_b").disabled = (wz_pg_cur == wz_pg_min) ? true : false;
        document.getElementById("wz_next_b").disabled = (wz_pg_cur == wz_pg_max) ? true : false;
        document.getElementById("wz_save_b").disabled = (wz_pg_cur == wz_pg_max) ? false : true;

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

        /*
         * Remember the prev page.
         */
        wz_pg_prev = wz_pg_cur;

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
        /*
         * Remember the prev page.
         */
        wz_pg_prev = wz_pg_cur;

        wz_pg_cur--;
        wz_showhide();
    }

    /*
     * wz_verify_1()
     */
    function wz_verify_1()
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

        return true;
    }

    /*
     * wz_verify_4()
     */
    function wz_verify_4()
    {
        var wan_mode = get_by_id("wan_proto").value;
        var check_ip_mask_gw = 0, wan_msg;
        var ip, mask, gateway;

        var pppoe_type = get_by_name("wan_pppoe_dynamic_00");
        var pptp_type = get_by_name("wan_pptp_dynamic");
        var l2tp_type = get_by_name("wan_l2tp_dynamic");

        /*
         * WAN settings page.
         */
        if (wan_mode == "dhcpc") {
            /*
             * Validate MAC and activate cloning if necessary
             */
            var clonemac = get_by_id("wan_mac").value;
            if (!check_mac(clonemac)){
                alert(KR3);
                return false;
            }
            
            var wan_mac_tmp = "<% CmoGetCfg("wan_mac","none"); %>";
            if(clonemac != wan_mac_tmp)
            {
                get_by_id("reboot_type").value = "shutdown";
                get_by_id("html_response_page").value = "reboot.asp";
       	    }

            var mac = trim_string(get_by_id("wan_mac").value);
            if (!is_mac_valid(mac, true)) {
			    alert(KR3 + ":" + mac + ".");
			    return false;
            }
            else {
			    if (mac == "00:00:00:00:00:00")
				    get_by_id("wan_mac").value = get_by_id("old_wan_mac").value;
			    else
			        get_by_id("wan_mac").value = mac;
            }

            var host_name = get_by_id("hostname").value
            if (Find_word(host_name, "'") || Find_word(host_name, '"') || Find_word(host_name, "/")) {
                alert(TEXT004);
                return false
            }
        }
        else if (wan_mode == "pppoe") {
            if (pppoe_type[1].checked) {
                if (!is_ipv4_valid(get_by_id("wan_pppoe_ipaddr_00").value)) {
                    alert(LS46);
                    return false;
                }
            }

            get_by_id("wan_pppoe_username_00").value = trim_string(get_by_id("wan_pppoe_username_00").value);
            if (get_by_id("wan_pppoe_username_00").value == "") {
                alert(GW_WAN_PPPOE_USERNAME_INVALID);
                return false;
            }

	    if (get_by_id("wan_pppoe_password_00").value == "" || get_by_id("pppoe_pwd2").value == "") {
		alert(GW_WAN_PPPOE_PASSWORD_INVALID);
		return false;
	    }

            if (get_by_id("wan_pppoe_password_00").value != get_by_id("pppoe_pwd2").value) {
                alert(_pwsame);
                form1.wan_pppoe_password_00.select();
                form1.wan_pppoe_password_00.focus();
                return false;
            }
        }
        else if (wan_mode == "pptp") {
            if (!is_ipv4_valid(get_by_id("wan_pptp_ipaddr").value)) {
                alert(LS46);
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

	    if (get_by_id("wan_pptp_password").value == "" || get_by_id("pptp_password2").value == "") {
		alert(GW_WAN_PPTP_PASSWORD_INVALID);
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
                alert(LS46);
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

	    if (get_by_id("wan_l2tp_password").value == "" || get_by_id("l2tp_password2").value == "") {
		alert(GW_WAN_L2TP_PASSWORD_INVALID);
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
                alert(LS46);
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
                alert("Invalid Gateway Address");
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
            get_by_id("wan_primary_dns").value= "0.0.0.0";
        }

        get_by_id("wan_secondary_dns").value = trim_string(get_by_id("wan_secondary_dns").value);
        if (get_by_id("wan_secondary_dns").value == "") {
            get_by_id("wan_secondary_dns").value = "0.0.0.0";
        }

        if (!is_ipv4_valid(get_by_id("wan_primary_dns").value)) {
            alert("Invalid Primary DNS Address");
            form1.wan_primary_dns.select();
            form1.wan_primary_dns.focus();
            return false;
        }

        if (!is_ipv4_valid(get_by_id("wan_secondary_dns").value)) {
            alert("Invalid Secondary DNS Address");
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

        if ((wan_mode == "static") ||
            ((wan_mode == "pppoe") && pppoe_type[1].checked) ||
            ((wan_mode == "pptp") && pptp_type[1].checked) ||
            ((wan_mode == "l2tp") && l2tp_type[1].checked))
        {
            if (get_by_id("wan_primary_dns").value == "0.0.0.0") {
                if (get_by_id("wan_secondary_dns").value == "0.0.0.0")
                    alert(GW_WAN_DNS_SERVERS_INVALID);
                else
                    alert(GW_WAN_DNS_SERVER_SECONDARY_WITHOUT_PRIMARY_INVALID);

                return false;
            }
        }

        return true;
    }

    function wz_cancel()
    {
        if (is_form_modified("form1")) {
            if (confirm(_wizquit)) {
                window.location.href = "index.asp";
            }
        }
        else {
            window.location.href = "index.asp";
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

        for (var i = 0; i <= 4; i++) {
            document.getElementById("wan_ip_mode_box_" + i).style.display = (wan_type == i)? "block" : "none";
        }
    }

    function clone_mac_action()
    {
        get_by_id("wan_mac").value = get_by_id("mac_clone_addr").value;
        get_by_id("reboot_type").value = "shutdown";
        get_by_id("html_response_page").value = "reboot.asp";
    }

    function pppoe_mode_selector(address_mode)
    {
        var pppoe_mode = get_by_name("wan_pppoe_dynamic_00");

        if (address_mode == "1") {
            pppoe_mode[0].checked = true;
        }
        else {
            pppoe_mode[1].checked = true;
        }

        pppoe_mode.value = address_mode;
        get_by_id("wan_pppoe_ipaddr_00").disabled = pppoe_mode[0].checked;
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
        wz_verify[1] = wz_verify_1;
        wz_verify[2] = null;
        wz_verify[3] = null;
        wz_verify[4] = wz_verify_4;
        wz_verify[5] = null;

        /*
         * Show the wizard screen as appropriate.
         */
        wz_showhide();

        wan_mode_selector(get_by_id("wan_proto").value);
        pppoe_mode_selector("<% CmoGetCfg("wan_pppoe_dynamic_00","none"); %>");
        pptp_mode_selector("<% CmoGetCfg("wan_pptp_dynamic","none"); %>");
        l2tp_mode_selector("<% CmoGetCfg("wan_l2tp_dynamic","none"); %>");
        get_by_id("time_zone").selectedIndex = get_by_id("time_zone_area").value;

        set_form_default_values("form1");

        if ("<% CmoGetStatus("get_current_user"); %>" == "user"){
            DisableEnableForm(document.form1, true);
        }
    }

    function send_request()
    {
        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            window.location.href = "user_page.asp";
            return false;
        }

        if (submit_button_flag == 0) {
            submit_button_flag = 1;
            return true;
        }
        else {
            return false;
        }
    }
</script>

<title></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
</head>
<body onload="onPageLoad();" topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
<table border=0 cellspacing=0 cellpadding=0 align=center width=838>
<tr>
<td bgcolor="#FFFFFF">
<table width=838 border=0 cellspacing=0 cellpadding=0 align=center height=100>
<tr>
<td bgcolor="#FFFFFF">
<div align=center>
  <table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
    <tr>
      <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
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
        	<input type="hidden" id="old_wan_mac" name="old_wan_mac" value="<% CmoGetCfg("wan_mac","none"); %>">

            <form id="form1" name="form1" method="post" action="apply.cgi">
            <input type="hidden" id="time_zone_area" name="time_zone_area" value="<% CmoGetCfg("time_zone_area","none"); %>">
            <input type="hidden" id="mac_clone_addr" name="mac_clone_addr" value="<% CmoGetStatus("mac_clone_addr"); %>">
            <input type="hidden" id="wan_proto" name="wan_proto" value="<% CmoGetCfg("wan_proto","none"); %>">
            <input type="hidden" id="wan_specify_dns" name="wan_specify_dns" value="<% CmoGetCfg("wan_specify_dns","none"); %>">

            <div id="wz_page_1" style="display:none">
                <input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
                <input type="hidden" id="html_response_message" name="html_response_message" value="">
                <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
                <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="login.asp">
                <input type="hidden" id="reboot_type" name="reboot_type" value="shutdown">

                <h2 align="left"><script>show_words(wwa_title_wel)</script></h2>
                <p class="box_msg"><script>show_words(wwa_intro_wel)</script>.</p>
                <table class=formarea>
                    <tr>
                      <td width="334" height="81">
                        <UL>
                        <LI><script>show_words(wwa_title_s1)</script>
                        <LI><script>show_words(wwa_title_s2)</script>
                        <LI><script>show_words(wwa_title_s3)</script>
                        <LI><script>show_words(wwa_title_s4)</script>

                        </LI>
                        </UL>
                      </TD>
                    </tr>
                </table>
            </div><!-- wz_page_1 -->

            <div id="wz_page_2" style="display:none">
                <h2 align="left"><script>show_words(wwa_title_s1)</script></h2>
                <p class="box_msg"><script>show_words(wwa_intro_s1)</script>:</p>
                <table class=formarea>
                    <tr>
                      <td align=right class="duple"><script>show_words(_password)</script>:</td>
                      <td>&nbsp;<input type="password" id="admin_password" name="admin_password" size=20 maxlength=15 value="<% CmoGetCfg("admin_password","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_verifypw)</script> :</td>
                      <td>&nbsp;<input type="password" id="admin_password2" name="admin_password2" size=20 maxlength=15 value="<% CmoGetCfg("admin_password","none"); %>"></td>
                    </tr>
                </table>
                <br>
            </div><!-- wz_page_2 -->

            <div id="wz_page_3" style="display:none">
                <h2 align="left"><script>show_words(wwa_title_s2)</script></h2>
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
            </div><!-- wz_page_3 -->

            <div id="wz_page_4" style="display:none">
                <h2><script>show_words(wwa_title_s3)</script></h2>
                <!--p class="box_msg"><script>show_words(wwa_intro_s3)</script></p>
                <select id="select_isp" name="select_isp" onChange="wan_mode_selector(this.value)">
                  <option value="dhcpc"><script>show_words(wwa_selectisp_not)</script></option>
                  <option value="dhcpc">Adelphia Power Link</option>
                  <option value="pppoe">ALLTEL DSL</option>
                  <option value="pppoe">ATAT DSL Service</option>
                  <option value="pppoe">Bell Sympatico</option>
                  <option value="pppoe">Bellsouth</option>
                  <option value="dhcpc">Charter High-Speed</option>
                  <option value="dhcpc">Comcast</option>
                  <option value="pppoe">Covad</option>
                  <option value="dhcpc">Cox Communications</option>
                  <option value="dhcpc">Earthlink Cable</option>
                  <option value="pppoe">Earthlink DSL</option>
                  <option value="pppoe">FrontierNet</option>
                  <option value="dhcpc">Optimum Online</option>
                  <option value="dhcpc">RCN</option>
                  <option value="dhcpc">Road Runner</option>
                  <option value="dhcpc">Rogers Yahoo!</option>
                  <option value="pppoe">SBC Yahoo! DSL</option>
                  <option value="dhcpc">Shaw</option>
                  <option value="pppoe">Speakeasy</option>
                  <option value="pppoe">Sprint FastConnect</option>
                  <option value="pppoe">Telus</option>
                  <option value="dhcpc">Time Warner Cable</option>
                  <option value="pppoe">US West / Qwest</option>
                  <option value="pppoe">Verizon Online DSL</option>
                  <option value="pppoe">XO Communications</option>
                </select-->

                <P align="left" class=box_msg><script>show_words(wwa_msg_ispnot)</script></P>
                <table class=formarea >
                    <tr>
                      <td class=form_label>&nbsp;</td>
                      <td><input name="wan_mode" type="radio" value="dhcpc" checked="true" onchange="wan_mode_selector(this.value);">
                          <STRONG><script>show_words(_dhcpconn)</script></STRONG>
                          <div class=itemhelp><script>show_words(wwa_msg_dhcp)</script></div></td>
                    </tr>
                    <tr>
                      <td class=form_label>&nbsp;</td>
                      <td><input name="wan_mode" type="radio" value="pppoe" onchange="wan_mode_selector(this.value);">
                          <STRONG><script>show_words(wwa_wanmode_pppoe)</script></STRONG>
                          <div class=itemhelp><script>show_words(wwa_msg_pppoe)</script></div></td>
                    </tr>
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
            </div><!-- wz_page_4 -->

            <div id="wz_page_5" style="display:none">
                <div id="wan_ip_mode_box_0" style="display:none">
                <h2 align="left"><script>show_words(_dhcpconn)</script></h2>
                <p class="box_msg"><script>show_words(wwa_msg_set_dhcp)</script></p>
                <table align="center" class=formarea>
                    <tr>
                      <td width="170" align=right class="duple"><script>show_words(_macaddr)</script>:</td>
                      <td width="430">&nbsp;<input type="text" id="wan_mac" name="wan_mac" maxlength="17" size="20" value="<% CmoGetCfg("wan_mac","none"); %>"> (<script>show_words(LT124)</script>)</td>
                    </tr>
                    <tr>
                      <td>&nbsp;</td>
                      <td>&nbsp;<input name="clone" type="button" class="button_submit" id="clone" value="" onClick="clone_mac_action()"></td>
                      <script>get_by_id("clone").value = _clone;</script>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_hostname)</script>:</td>
                      <td>&nbsp;<input type=text id="hostname" name="hostname" size="25" maxlength="39" value="<% CmoGetCfg("hostname","none"); %>"></td>
                    </tr>
                </table>
                <div class="itemhelp"><script>show_words(wwa_note_hostname)</script></div>
                </div><!-- wan_ip_mode_box_0 -->

                <div id="wan_ip_mode_box_1" style="display:none">
                <h2 align="left"><script>show_words(wwa_title_set_pppoe)</script></h2>
                <p class="box_msg"><script>show_words(wwa_msg_set_pppoe)</script></p>
                <table align="center" class=formarea>
                    <tr>
                      <td width="170" align=right class="duple"><script>show_words(bwn_AM)</script> :</td>
                      <td width="430">&nbsp;
                        <input name="wan_pppoe_dynamic_00" type="radio" value="1" onClick="pppoe_mode_selector(this.value)">
                        <script>show_words(carriertype_ct_0)</script>&nbsp;&nbsp;
                        <input name="wan_pppoe_dynamic_00" type="radio" value="0" onClick="pppoe_mode_selector(this.value)">
                        <script>show_words(_sdi_staticip)</script></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(help256)</script> :</td>
                      <td>&nbsp;<input type="text" name="wan_pppoe_ipaddr_00" id="wan_pppoe_ipaddr_00" size="20" maxlength="15" value="<% CmoGetCfg("wan_pppoe_ipaddr_00","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_username)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pppoe_username_00" name="wan_pppoe_username_00" size="20" maxlength="63" value="<% CmoGetCfg("wan_pppoe_username_00","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_password)</script> :</td>
                      <td>&nbsp;<input type=password id="wan_pppoe_password_00" name="wan_pppoe_password_00" size="20" maxlength="63" value="<% CmoGetCfg("wan_pppoe_password_00","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(_verifypw)</script> :</td>
                      <td>&nbsp;<input type=password id="pppoe_pwd2" name="pppoe_pwd2" size="20" maxlength="63" value="<% CmoGetCfg("wan_pppoe_password_00","none"); %>"></td>
                    </tr>
                    <tr>
                      <td align=right class="duple"><script>show_words(help266)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_pppoe_service_00" name="wan_pppoe_service_00" size="20" maxlength="39" value="<% CmoGetCfg("wan_pppoe_service_00","none"); %>"> (<script>show_words(LT124)</script>)</td>
                    </tr>
                    <tr>
                      <td colspan="2" class="itemhelp"> <script>show_words(wwa_note_svcn)</script></td>
                    </tr>
                </table>
                </div><!-- wan_ip_mode_box_1 -->

                <div id="wan_ip_mode_box_2" style="display:none">
                <h2 align="left"><script>show_words(wwa_title_set_pptp)</script></h2>
                <p class="box_msg"><script>show_words(wwa_msg_set_pptp)</script></p>
                <table align="center" class=formarea >
                    <tr>
                      <td width="170" align=right class="duple"><script>show_words(bwn_AM)</script> :</td>
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
                <table align="center" class=formarea >
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
                <div id="wan_dns_settings_box">
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
            </div><!-- wz_page_5 -->

            <div id="wz_page_6" style="display:none">
                <h2 align="left"><script>show_words(_setupdone)</script></h2>
                <P align="left" class=box_msg><script>show_words(wwa_intro_s4)</script></P>
                <br>
            </div><!-- wz_page_6 -->

            <table align="center" class="formarea">
              <tr>
                <td>
                <fieldset id="wz_buttons">
                  <input type="button" class="button_submit" id="wz_prev_b" name="wz_prev_b" value="" onclick="wz_prev();">
                  <input type="button" class="button_submit" id="wz_next_b" name="wz_next_b" value="" onclick="wz_next();">
                  <input type="button" class="button_submit" id="wz_cancel_b" name="wz_cancel_b" value="" onclick="wz_cancel();">
                  <input type="submit" class="button_submit" id="wz_save_b" name="wz_save_b" value="" onclick="return send_request();">
                  <script>get_by_id("wz_prev_b").value = _prev;</script>
                  <script>get_by_id("wz_next_b").value = _next;</script>
                  <script>get_by_id("wz_cancel_b").value = _cancel;</script>
                  <script>get_by_id("wz_save_b").value = _connect;</script>
                </fieldset>
                </td>
              </tr>
            </table>
            </form>
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
</html>
