<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var submit_button_flag = 0;
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

    function onPageLoad()
    {
        set_selectIndex("<% CmoGetCfg("wlan0_txpower","none"); %>", get_by_id("wlan0_txpower"));
        set_checked("<% CmoGetCfg("wlan0_partition","none"); %>", get_by_id("wlan0_partition_sel"));
        set_checked("<% CmoGetCfg("wlan0_wmm_enable","none"); %>", get_by_id("wlan0_wmm_enable_sel"));
        set_checked("<% CmoGetCfg("wlan0_short_gi","none"); %>", get_by_id("wlan0_short_gi_sel"));

        set_selectIndex("<% CmoGetCfg("wlan1_txpower","none"); %>", get_by_id("wlan1_txpower"));
        set_checked("<% CmoGetCfg("wlan1_partition","none"); %>", get_by_id("wlan1_partition_sel"));
        set_checked("<% CmoGetCfg("wlan1_wmm_enable","none"); %>", get_by_id("wlan1_wmm_enable_sel"));
        set_checked("<% CmoGetCfg("wlan1_short_gi","none"); %>", get_by_id("wlan1_short_gi_sel"));

		if ("<% CmoGetCfg("wlan0_enable", "none"); %>" == "0") {
			get_by_id("wlan0_txpower").disabled = true;
			get_by_id("wlan0_beacon_interval").disabled = true;
			get_by_id("wlan0_rts_threshold").disabled = true;
			get_by_id("wlan0_fragmentation").disabled = true;
			get_by_id("wlan0_dtim").disabled = true;
			get_by_id("wlan0_partition_sel").disabled = true;
			get_by_id("wlan0_wmm_enable_sel").disabled = true;
			get_by_id("wlan0_short_gi_sel").disabled = true;
			get_by_name("11n_protection_coexist")[0].disabled = true;
			get_by_name("11n_protection_coexist")[1].disabled = true;			
		}

		if ("<% CmoGetCfg("wlan1_enable", "none"); %>" == "0") {
			get_by_id("wlan1_txpower").disabled = true;
			get_by_id("wlan1_beacon_interval").disabled = true;
			get_by_id("wlan1_rts_threshold").disabled = true;
			get_by_id("wlan1_fragmentation").disabled = true;
			get_by_id("wlan1_dtim").disabled = true;
			get_by_id("wlan1_partition_sel").disabled = true;
			get_by_id("wlan1_wmm_enable_sel").disabled = true;
			get_by_id("wlan1_short_gi_sel").disabled = true;
		}

		set_channel_width();

        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1, true);
        }

        set_form_default_values("form1");
    }

    function check_perform(which_one)
    {
        var obj;
        var temp_obj;

        switch (which_one) {
            case 0 :
                temp_obj = get_by_id("wlan0_beacon_interval");
                var beacon_msg = replace_msg(check_num_msg, "Beacon interval", 20, 1000);
                obj = new varible_obj(temp_obj.value, beacon_msg, 20, 1000, false);
                break;

            case 1 :
                temp_obj = get_by_id("wlan0_rts_threshold");
                var rts_msg = replace_msg(check_num_msg, "RTS Threshold", 0, 2347);
                obj = new varible_obj(temp_obj.value, rts_msg, 0, 2347, false);
                break;

            case 2 :
                temp_obj = get_by_id("wlan0_fragmentation");
                var dtim_msg = replace_msg(check_num_msg, "Fragmentation",256 ,2346);
                obj = new varible_obj(temp_obj.value, dtim_msg, 256, 2346, true);
                break;

            case 3 :
                temp_obj = get_by_id("wlan0_dtim");
                var dtim_msg = replace_msg(check_num_msg, "DTIM Interval",1 ,255);
                obj = new varible_obj(temp_obj.value, dtim_msg, 1, 255, false);
                break;
        }

        if (check_varible(obj)) {
            return true;
        }
        else {
            temp_obj.value = temp_obj.defaultValue;
            return false;
        }
    }

    function check_perform_5g(which_one)
    {
        var obj;
        var temp_obj;

        switch (which_one) {
            case 0 :
                temp_obj = get_by_id("wlan1_beacon_interval");
                var beacon_msg = replace_msg(check_num_msg, "Beacon interval", 20, 1000);
                obj = new varible_obj(temp_obj.value, beacon_msg, 20, 1000, false);
                break;

            case 1 :
                temp_obj = get_by_id("wlan1_rts_threshold");
                var rts_msg = replace_msg(check_num_msg, "RTS Threshold", 0, 2347);
                obj = new varible_obj(temp_obj.value, rts_msg, 0, 2347, false);
                break;

            case 2 :
                temp_obj = get_by_id("wlan1_fragmentation");
                var dtim_msg = replace_msg(check_num_msg, "Fragmentation",256 ,2346);
                obj = new varible_obj(temp_obj.value, dtim_msg, 256, 2346, true);
                break;

            case 3 :
                temp_obj = get_by_id("wlan1_dtim");
                var dtim_msg = replace_msg(check_num_msg, "DTIM Interval",1 ,255);
                obj = new varible_obj(temp_obj.value, dtim_msg, 1, 255, false);
                break;
        }

        if (check_varible(obj)) {
            return true;
        }
        else {
            temp_obj.value = temp_obj.defaultValue;
            return false;
        }
    }

	function set_channel_width()
	{
		if (get_by_id("wlan0_11n_protection").value == "20") {
			get_by_name("11n_protection_coexist")[0].disabled = true;
        	get_by_name("11n_protection_coexist")[1].disabled = true;
		}
		else {
			if (get_by_id("wlan0_11n_protection").value == "40")
				get_by_name("11n_protection_coexist")[1].checked = true;
			else
				get_by_name("11n_protection_coexist")[0].checked = true;
		}
	}

    function send_request()
    {
        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }

        var is_submit = false;
        for (var i = 0; i < 4; i++) {
            if ((check_perform(i) == false) || (check_perform_5g(i) == false)) {
                return false;
            }
        }

        get_by_id("wlan0_partition").value = get_checked_value(get_by_id("wlan0_partition_sel"));
        get_by_id("wlan0_wmm_enable").value = get_checked_value(get_by_id("wlan0_wmm_enable_sel"));
        get_by_id("wlan0_short_gi").value = get_checked_value(get_by_id("wlan0_short_gi_sel"));

        get_by_id("wlan1_partition").value = get_checked_value(get_by_id("wlan1_partition_sel"));
        get_by_id("wlan1_wmm_enable").value = get_checked_value(get_by_id("wlan1_wmm_enable_sel"));
        get_by_id("wlan1_short_gi").value = get_checked_value(get_by_id("wlan1_short_gi_sel"));

        if (get_by_id("wlan0_11n_protection").value != "20") {
            if (get_by_name("11n_protection_coexist")[0].checked)
                get_by_id("wlan0_11n_protection").value = "auto";
            else
            	get_by_id("wlan0_11n_protection").value = "40";
        }

        if (submit_button_flag == 0) {
            submit_button_flag = 1;
            return true;
        }

        return false;
    }
</script>

<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style1 {color: #000000}
-->
</style>
</head>

<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
<table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
      <tr>
        <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
		<td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
		<td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
        <td>&nbsp;</td>
      </tr>
    </table>
    <table id="topnav_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
        <tr>
            <td align="center" valign="middle"><img src="wlan_masthead.gif" width="836" height="92"></td>
        </tr>
    </table>
    <table border="0" cellpadding="2" cellspacing="1" width="838" align="center" bgcolor="#FFFFFF">
        <tr id="topnav_container">
            <td><img src="short_modnum.gif" width="125" height="25"></td>
			<td id="topnavoff"><a href="index.asp" onclick="return jump_if();"><script>show_words(_setup)</script></a></td>
			<td id="topnavon"><a href="adv_virtual.asp" onclick="return jump_if();"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_tools)</script></a></td>
			<td id="topnavoff"><a href="st_device.asp" onclick="return jump_if();"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp" onclick="return jump_if();"><script>show_words(_support)</script></a></td>
        </tr>
    </table>
    <table border="1" cellpadding="2" cellspacing="0" width="838" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
        <tr>
            <td id="sidenav_container" valign="top" width="125" align="right">
                <table cellSpacing=0 cellPadding=0 border=0>
                    <tr>
                      <td id=sidenav_container>
                        <div id=sidenav>
                          <UL>
						  <LI><div><a href="adv_virtual.asp" onClick="return jump_if();"><script>show_words(_virtserv)</script></a></div></LI>
                            <LI><div><a href="adv_portforward.asp" onclick="return jump_if();"><script>show_words(_pf)</script></a></div></LI>
                            <LI><div><a href="adv_appl.asp" onclick="return jump_if();"><script>show_words(_specappsr)</script></a></div></LI>
                            <LI><div><a href="adv_qos.asp" onclick="return jump_if();"><script>show_words(YM48)</script></a></div></LI>
                            <LI><div><a href="adv_filters_mac.asp" onclick="return jump_if();"><script>show_words(_netfilt)</script></a></div></LI>
                            <LI><div><a href="adv_access_control.asp" onclick="return jump_if();"><script>show_words(_acccon)</script></a></div></LI>
							<LI><div><a href="adv_filters_url.asp" onclick="return jump_if();"><script>show_words(_websfilter)</script></a></div></LI>
							<LI><div><a href="Inbound_Filter.asp" onclick="return jump_if();"><script>show_words(_inboundfilter)</script></a></div></LI>
							<LI><div><a href="adv_dmz.asp" onclick="return jump_if();"><script>show_words(_firewalls)</script></a></div></LI>
                            <LI><div><a href="adv_routing.asp" onclick="return jump_if();"><script>show_words(_routing)</script></a></div></LI>
							<LI><div id=sidenavoff><script>show_words(_adwwls)</script></div></LI>
                            <LI><div><a href="adv_wish.asp" onclick="return jump_if();">WISH</a></div></LI>
							<LI><div><a href="adv_wps_setting.asp" onclick="return jump_if();"><script>show_words(LY2)</script></a></div></LI>
                            <LI><div><a href="adv_network.asp" onclick="return jump_if();"><script>show_words(_advnetwork)</script></a></div></LI>
                            <LI><div><a href="guest_zone.asp" onclick="return jump_if();"><script>show_words(_guestzone)</script></a></div></LI>
                           <LI><div><a href="adv_ipv6_firewall.asp" onclick="return jump_if();"><script>show_words(Tag05762)</script></a></div></LI>
                            <LI><div><a href="adv_ipv6_routing.asp" onclick="return jump_if();"><script>show_words(IPV6_routing)</script></a></div></LI>
                          </UL>
                       </div>
                      </td>
                    </tr>
                </table>
            </td>

            <form id="form1" name="form1" method="post" action="apply.cgi">
            <input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
            <input type="hidden" id="html_response_message" name="html_response_message" value="">
            <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
            <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="adv_wlan_perform.asp">
            <input type="hidden" id="reboot_type" name="reboot_type" value="wireless">

            <input type="hidden" id="wlan0_wmm_enable" name="wlan0_wmm_enable" value="<% CmoGetCfg("wlan0_wmm_enable","none"); %>">
            <input type="hidden" id="wlan0_partition" name="wlan0_partition" value="<% CmoGetCfg("wlan0_partition","none"); %>">
            <input type="hidden" id="wlan0_short_gi" name="wlan0_short_gi" value="<% CmoGetCfg("wlan0_short_gi","none"); %>">

            <input type="hidden" id="wlan1_wmm_enable" name="wlan1_wmm_enable" value="<% CmoGetCfg("wlan1_wmm_enable","none"); %>">
            <input type="hidden" id="wlan1_partition" name="wlan1_partition" value="<% CmoGetCfg("wlan1_partition","none"); %>">
            <input type="hidden" id="wlan1_short_gi" name="wlan1_short_gi" value="<% CmoGetCfg("wlan1_short_gi","none"); %>">

            <td valign="top" id="maincontent_container">
                <div id="maincontent">
                  <div id=box_header>
          <h1>
            <script>show_words(_advwls)</script>
          </h1>

          <p>
            <script>show_words(aw_intro)</script>
          </p>
        <input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_request()">
		<input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel('form1', 'adv_wlan_perform.asp');">
		<script>check_reboot();</script>
		<script>get_by_id("button").value = _savesettings;</script>
		<script>get_by_id("button2").value = _dontsavesettings;</script>

                  </div>
                  <br>
                  <div class=box>
                    <h2><script>show_words(aw_title_2)</script></h2>

                  <TABLE width=525>
                        <tr> 
                          <td class="duple"><script>show_words(wwl_band)</script> :</td>
                          <td><strong><script>show_words(GW_WLAN_RADIO_0_NAME)</script></strong></td>
                        </tr>       	
                        <tr>
                            <td width="183" align=right class="duple"><script>show_words(aw_TP)</script> :</td>
                            <td width="330">
                                <select id="wlan0_txpower" name="wlan0_txpower" size="1" >
                                      <option value="19"><script>show_words(aw_TP_0)</script></option>
                                      <option value="15"><script>show_words(aw_TP_1)</script></option>
                                      <option value="3"><script>show_words(aw_TP_2)</script></option>
                                </select>
                            </td>
                        </tr>
                        <TR style="display:none">
              <TD align=right class="duple"><script>show_words(aw_BP)</script> :</TD>
              <TD> <input maxlength=4 id="wlan0_beacon_interval" name="wlan0_beacon_interval" size=6 value="<% CmoGetCfg("wlan0_beacon_interval","none"); %>">
                (20..1000) </TD>
                        </TR>
                        <tr style="display:none">
              <td align=right class="duple"><script>show_words(aw_RT)</script> :</td>
                           <td><input maxlength=4 id="wlan0_rts_threshold" name="wlan0_rts_threshold" size=6 value="<% CmoGetCfg("wlan0_rts_threshold","none"); %>">
                (0..2347) </td>
                        </tr>
                        <tr style="display:none">
              <td align=right class="duple"><script>show_words(aw_FT)</script> :</td>
                           <td><input maxlength=4 id="wlan0_fragmentation" name="wlan0_fragmentation" size=6 value="<% CmoGetCfg("wlan0_fragmentation","none"); %>">
                (256..2346) </td>
                        </tr>
                        <TR style="display:none">
              <TD align=right class="duple"><script>show_words(aw_DI)</script> :</TD>
                          <TD><input maxlength=3 id="wlan0_dtim" name="wlan0_dtim" size=6 value="<% CmoGetCfg("wlan0_dtim","none"); %>">
                            (1..255)
                          </TD>
                        </TR>
                        <TR>
                          <TD align=right class="duple"><script>show_words(KR4_ww)</script> :</TD>
                          <TD>
                              <INPUT type="checkbox" id="wlan0_partition_sel" name="wlan0_partition_sel" value="1">
                          </TD>
                        </TR>
                        <TR>
                          <TD align=right class="duple"><script>show_words(aw_WE)</script> :</TD>
                          <TD>
                              <INPUT name="wlan0_wmm_enable_sel" type="checkbox" id="wlan0_wmm_enable_sel" value="1" disabled>
                          </TD>
                        </TR>
                        <TR>
                          <TD align=right class="duple"><script>show_words(aw_sgi)</script> :</TD>
                          <TD>
                              <INPUT type="checkbox" id="wlan0_short_gi_sel" name="wlan0_short_gi_sel" value="1">
                          </TD>
                        </TR>
                        <TR>
                          <TD align=right class="duple">HT20/40 Coexistence :</td>
                          <TD>
                              <input type="radio" name="11n_protection_coexist" value="1">
                              <script>show_words(_enable)</script>
                              <input type="radio" name="11n_protection_coexist" value="0">
                              <script>show_words(_disabled)</script>
                              <input type="hidden" id="wlan0_11n_protection" name="wlan0_11n_protection" value="<% CmoGetCfg("wlan0_11n_protection","none"); %>">
                          </TD>
                        </TR>
                </TABLE>
            </DIV>

                  <div class=box>
                    <h2><script>show_words(aw_title_2)</script></h2>

                  <TABLE width=525>
                        <tr> 
                          <td class="duple"><script>show_words(wwl_band)</script> :</td>
                          <td><strong><script>show_words(GW_WLAN_RADIO_1_NAME)</script></strong></td>
                        </tr>                   	
                        <tr>
                            <td width="183" align=right class="duple"><script>show_words(aw_TP)</script> :</td>
                            <td width="330">
                                <select id="wlan1_txpower" name="wlan1_txpower" size="1" >
                                      <option value="19"><script>show_words(aw_TP_0)</script></option>
                                      <option value="15"><script>show_words(aw_TP_1)</script></option>
                                      <option value="3"><script>show_words(aw_TP_2)</script></option>
                                </select>
                            </td>
                        </tr>
                        <TR style="display:none">
              <TD align=right class="duple"><script>show_words(aw_BP)</script> :</TD>
              <TD> <input maxlength=4 id="wlan1_beacon_interval" name="wlan1_beacon_interval" size=6 value="<% CmoGetCfg("wlan1_beacon_interval","none"); %>">
                (20..1000) </TD>
                        </TR>
                        <tr style="display:none">
              <td align=right class="duple"><script>show_words(aw_RT)</script> :</td>
                           <td><input maxlength=4 id="wlan1_rts_threshold" name="wlan1_rts_threshold" size=6 value="<% CmoGetCfg("wlan1_rts_threshold","none"); %>">
                (0..2347) </td>
                        </tr>
                        <tr style="display:none">
              <td align=right class="duple"><script>show_words(aw_FT)</script> :</td>
                           <td><input maxlength=4 id="wlan1_fragmentation" name="wlan1_fragmentation" size=6 value="<% CmoGetCfg("wlan1_fragmentation","none"); %>">
                (256..2346) </td>
                        </tr>
                        <TR style="display:none">
              <TD align=right class="duple"><script>show_words(aw_DI)</script> :</TD>
                          <TD><input maxlength=3 id="wlan1_dtim" name="wlan1_dtim" size=6 value="<% CmoGetCfg("wlan1_dtim","none"); %>">
                            (1..255)
                          </TD>
                        </TR>
                        <TR>
                          <TD align=right class="duple"><script>show_words(KR4_ww)</script> :</TD>
                          <TD>
                              <INPUT type="checkbox" id="wlan1_partition_sel" name="wlan1_partition_sel" value="1">
                          </TD>
                        </TR>
                        <TR>
                          <TD align=right class="duple"><script>show_words(aw_WE)</script> :</TD>
                          <TD>
                              <INPUT name="wlan1_wmm_enable_sel" type="checkbox" id="wlan1_wmm_enable_sel" value="1" disabled>
                          </TD>
                        </TR>
                        <TR>
                          <TD align=right class="duple"><script>show_words(aw_sgi)</script> :</TD>
                          <TD>
                              <INPUT type="checkbox" id="wlan1_short_gi_sel" name="wlan1_short_gi_sel" value="1">
                          </TD>
                        </TR>
                </TABLE>
            </DIV>            
            </TD>
            </form>
<td valign="top" width="150" id="sidehelp_container" align="left">
                <table borderColor=#ffffff cellSpacing=0 borderColorDark=#ffffff
      cellPadding=2 bgColor=#ffffff borderColorLight=#ffffff border=0>
                    <tr>
	                   <td id=help_text><strong><b><strong><script>show_words(_hints)</script></strong></b>&hellip;</strong>
					    <p><script>show_words(hhaw_1)</script></p>
            			<!--<p><script>show_words(hhaw_11d)</script></p>-->
            			<p><script>show_words(hhaw_wmm)</script></p>
	                   <p class="more"><a href="support_adv.asp#Advanced_Wireless" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
                       </TD>
                    </tr>
                </table></td>
        </tr>
    </table>
    <table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
        <tr>
            <td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
            <td width="10">&nbsp;</td><td>&nbsp;</td>
        </tr>
    </table>
<br>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
<br>
</body>
<script> 
	reboot_form();
	onPageLoad();
</script>
</html>
