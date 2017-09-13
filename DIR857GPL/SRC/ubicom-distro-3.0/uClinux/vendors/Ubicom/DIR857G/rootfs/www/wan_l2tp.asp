<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script>
    var submit_button_flag = 0;
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

    function onPageLoad()
    {
        set_checked("<% CmoGetCfg("wan_l2tp_dynamic","none"); %>", get_by_name("wan_l2tp_dynamic"));
        set_checked("<% CmoGetCfg("wan_l2tp_connect_mode","none"); %>", get_by_name("wan_l2tp_connect_mode"));
        get_by_id("l2tppwd1").value = "<% CmoGetCfg("wan_l2tp_password","none"); %>";
        get_by_id("l2tppwd2").value = get_by_id("l2tppwd1").value;

        check_connectmode();
        disable_l2tp_ip();

        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
           DisableEnableForm(document.form1,true);
        }

        set_form_default_values("form1");
    }

    function clone_mac_action()
    {
        get_by_id("wan_mac").value = get_by_id("mac_clone_addr").value;
        get_by_id("reboot_type").value = "shutdown";
        get_by_id("html_response_page").value = "reboot.asp";
    }

    function check_connectmode()
    {
        var conn_mode = get_by_name("wan_l2tp_connect_mode");
        var idle_time = get_by_id("wan_l2tp_max_idle_time");
        idle_time.disabled = !conn_mode[1].checked;
    }

    function disable_l2tp_ip()
    {
        var fixIP = get_by_name("wan_l2tp_dynamic");
        var is_disabled = false;

        if (fixIP[0].checked)
            is_disabled = true;

        get_by_id("wan_l2tp_ipaddr").disabled = is_disabled;
        get_by_id("wan_l2tp_netmask").disabled = is_disabled;
        get_by_id("wan_l2tp_gateway").disabled = is_disabled;
    }

    function send_request()
    {
		  var ipv6_pppoe_share = "<% CmoGetStatus("ipv6_pppoe_share"); %>";
    	var ipv6_wan_proto = "<% CmoGetStatus("ipv6_wan_proto"); %>";
      if (ipv6_wan_proto == "ipv6_pppoe" && ipv6_pppoe_share == "1"){
    	        alert(IPV6_TEXT1);
            	return false;
      }
	        
	        
        get_by_id("asp_temp_52").value = get_by_id("wan_proto").value;

        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }

        var fix_ip = get_by_name("wan_l2tp_dynamic");
        var ip = get_by_id("wan_l2tp_ipaddr").value;
        var mask = get_by_id("wan_l2tp_netmask").value;
        var gateway = get_by_id("wan_l2tp_gateway").value;
        var dns1 = get_by_id("wan_primary_dns").value;
        var dns2 = get_by_id("wan_secondary_dns").value;
        var server_ip = get_by_id("wan_l2tp_server_ip").value;
        var idle = get_by_id("wan_l2tp_max_idle_time").value;
        var mtu = get_by_id("wan_l2tp_mtu").value;
        var user_name = get_by_id("wan_l2tp_username").value;

        var ip_addr_msg = replace_msg(all_ip_addr_msg,_ipaddr);
        var gateway_msg = replace_msg(all_ip_addr_msg,wwa_gw);
        var primary_dns_msg = replace_msg(all_ip_addr_msg,wwa_pdns);
        var second_dns_msg = replace_msg(all_ip_addr_msg,wwa_sdns);
        var max_idle_msg = replace_msg(check_num_msg, bwn_MIT, 0, 9999);
        var mtu_msg = replace_msg(check_num_msg, bwn_MTU, 1300, 1400);
        var server_ip_msg = replace_msg(all_ip_addr_msg,bwn_L2TPSIPA);

        var temp_ip_obj = new addr_obj(ip.split("."), ip_addr_msg, false, false);
        var temp_mask_obj = new addr_obj(mask.split("."), subnet_mask_msg, false, false);
        var temp_gateway_obj = new addr_obj(gateway.split("."), gateway_msg, false, false);
        var temp_dns1 = new addr_obj(dns1.split("."), primary_dns_msg, true, false);
        var temp_dns2 = new addr_obj(dns2.split("."), second_dns_msg, true, false);
        var temp_sip_obj = new addr_obj(server_ip.split("."), server_ip_msg, false, false);

        var temp_idle = new varible_obj(idle, max_idle_msg, 0, 9999, false);
        var temp_mtu = new varible_obj(mtu, mtu_msg, 1300, 1400, false);

        if (fix_ip[1].checked) {
            if (!check_lan_setting(temp_ip_obj, temp_mask_obj, temp_gateway_obj, "L2TP")) {
                return false;
            }
        }

    	if (server_ip == "") {
    		alert(bwn_alert_17);
    		return false;
       	}

	if(!check_servername(server_ip)) {
        	if (!check_address(temp_sip_obj)) 
                return false;
        }

        if (server_ip == "") {
    		alert(bwn_alert_17);
            return false;
        }

        if (user_name == "") {
    		alert(MSG011);
            return false;
         }

	    if (get_by_id("l2tppwd1").value == "" || get_by_id("l2tppwd2").value == ""){
		 	alert(GW_WAN_L2TP_PASSWORD_INVALID);
			return false;
		}

        if (!check_pwd("l2tppwd1", "l2tppwd2")) {
            return false;
        }

        if (!check_varible(temp_idle)) {
            return false;
        }

        if (dns1 != "" && dns1 != "0.0.0.0") {
            if (!check_address(temp_dns1)) {
                return false;
            }
        }

        if (dns2 != "" && dns2 != "0.0.0.0") {
            if (!check_address(temp_dns2)) {
                return false;
            }
        }

        if (!check_varible(temp_mtu)) {
            return false;
        }

    	var wanmode = get_by_id("wan_proto").selectedIndex;
    	var real_wanmode = get_by_id("asp_temp_52").value;
    	var usb_mode = get_by_id("from_usb3g").value;
    	var usb_type = get_by_id("usb_type").value;
    	var pop_3 = usb_config6; 		 
    	var warn_1 = usb_config3; 		 
    	var warn_2 = usb_config4;

    	if(usb_mode == "2" && usb_type == "2"  && real_wanmode =="l2tp" ||usb_mode == "0" && usb_type == "2" && real_wanmode =="l2tp"){ 
			alert(pop_3);
			get_by_id("usb_type").value = "0"; // force to save usb_type	
			get_by_id("reboot_type").value = "all";
    	}else if(usb_mode == "2" && usb_type == "0" && real_wanmode =="usb3g" ){ //3G USB
			get_by_id("usb_type").value = "0"; // force to save usb_type	
			get_by_id("reboot_type").value = "all"; 
    	}else if(usb_mode == "2" && usb_type == "0" && wanmode !="5" ||usb_mode == "2" && usb_type == "1" && wanmode !="5" ){ //3G USB
           	alert(warn_1);
         	return false;
    	}else if(usb_mode == "0" && wanmode !="5" ){ //network USB
			get_by_id("usb_type").value = "0"; // force to save usb_type	
			get_by_id("reboot_type").value = "all";
    	}

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
				get_by_id("wan_mac").value = "<% CmoGetStatus("mac_default_addr"); %>";
			else
				get_by_id("wan_mac").value = mac;
		}

        if (dns1 != "" && dns1 != "0.0.0.0") {
            if (!check_address(temp_dns1)) {
                return false;
            }
        }

        if (get_by_id("wan_primary_dns").value == "" || get_by_id("wan_primary_dns").value == "0.0.0.0")
            get_by_id("wan_specify_dns").value = 0;
        else
            get_by_id("wan_specify_dns").value = 1;

        if (submit_button_flag == 0) {
            get_by_id("wan_l2tp_password").value = get_by_id("l2tppwd1").value;
            submit_button_flag = 1;
            return true;
        }

        return false;
    }
</script>

<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style1 {font-size: 11px}
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
            <td id="topnavon"><a href="index.asp" onclick="return jump_if();"><script>show_words(_setup)</script></a></td>
            <td id="topnavoff"><a href="adv_virtual.asp" onclick="return jump_if();"><script>show_words(_advanced)</script></a></td>
            <td id="topnavoff"><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_tools)</script></a></td>
            <td id="topnavoff"><a href="st_device.asp" onclick="return jump_if();"><script>show_words(_status)</script></a></td>
            <td id="topnavoff"><a href="support_men.asp" onclick="return jump_if();"><script>show_words(_support)</script></a></td>
        </tr>
    </table>
    <table border="1" cellpadding="2" cellspacing="0" width="838" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
        <tr>
            <td id="sidenav_container" valign="top" width="125" align="right">
                <table border="0" cellpadding="0" cellspacing="0">
                    <tr>
                        <td id="sidenav_container">
                            <div id="sidenav">
                                <ul>
																	<script>
																		show_side_bar(0);
																	</script>
                                </ul>
                            </div>
                        </td>
                    </tr>
                </table>
            </td>

            <form id="form1" name="form1" method="post" action="apply.cgi">
            <input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
            <input type="hidden" id="html_response_message" name="html_response_message" value="">
            <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
            <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="wan_l2tp.asp">
            <input type="hidden" id="mac_clone_addr" name="mac_clone_addr" value="<% CmoGetStatus("mac_clone_addr"); %>">
            <input type="hidden" id="wan_specify_dns" name="wan_specify_dns" value="<% CmoGetCfg("wan_specify_dns","none"); %>">
            <input type="hidden" id="wan_l2tp_password" name="wan_l2tp_password" value="">
            <input type="hidden" id="asp_temp_51" name="asp_temp_51" value="<% CmoGetCfg("asp_temp_51","none"); %>">
            <input type="hidden" id="asp_temp_52" name="asp_temp_52" value="<% CmoGetCfg("wan_proto","none"); %>">
            <input type="hidden" id="usb_type" name="usb_type" value="<% CmoGetCfg("usb_type","none"); %>">
            <input type="hidden" id="from_usb3g" name="from_usb3g" value="<% CmoGetCfg("asp_temp_72","none"); %>"> 
            <input type="hidden" id="reboot_type" name="reboot_type" value="shutdown">

            <td valign="top" id="maincontent_container">
                <div id="maincontent">

              <div id=box_header>

            <h1>
              <script>show_words(_WAN)</script>
            </h1>

            <p>
              <script>show_words(bwn_intro_ICS)</script>
            </p>

            <p><strong><b>
              <script>show_words(_note)</script>
              </b>:</strong>
              <script>show_words(bwn_note_clientSW)</script>
            </p>

            <input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_request()">
            <input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel('form1', 'sel_wan.asp');">
            <script>check_reboot();</script>
            <script>get_by_id("button").value = _savesettings;</script>
			<script>get_by_id("button2").value = _dontsavesettings;</script>

              </div>
              <br>
              <div class=box>

                <h2><script>show_words(bwn_ict)</script></h2>

                <p class="box_msg"><script>show_words(bwn_msg_Modes)</script></p>

                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                    <tr>

                      <td align=right width="185" class="duple"><script>show_words(bwn_mici)</script> :</td>

                      <td width="331">&nbsp; <select name="wan_proto" id="wan_proto" onChange="change_wan()">
                        <option value="static"><script>show_words(_sdi_staticip)</script></option>
                        <option value="dhcpc"><script>show_words(bwn_Mode_DHCP)</script></option>
                        <option value="pppoe"><script>show_words(bwn_Mode_PPPoE)</script></option>
                        <option value="pptp"><script>show_words(bwn_Mode_PPTP)</script></option>
                        <option value="l2tp" selected><script>show_words(bwn_Mode_L2TP)</script></option>
                        <!--option value="usb3g"><script>show_words(usb_3g)</script></option-->
                         <!--option value="bigpond">BigPond (Australia)</option-->
                      <option value="dslite">DS-Lite</option>
			</select></td>
                    </tr>
                </table>
              </div>

              <div class=box id=show_l2tp>

                <h2><script>show_words(bwn_L2TPICT)</script></h2>

				<p class="box_msg"><script>show_words(_ispinfo)</script> </p>



                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                    <tr>



                <td width="185" align=right class="duple">

                  <script>show_words(bwn_AM)</script>

                </td>

                      <td width="331">&nbsp;
                          <input type=radio value="1" name="wan_l2tp_dynamic" onClick=disable_l2tp_ip() checked>

                  <script>show_words(carriertype_ct_0)</script>

                          <input type=radio value="0" name="wan_l2tp_dynamic" onClick=disable_l2tp_ip()>

                  <script>show_words(_sdi_staticip)</script>

                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(_L2TPip)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=text id="wan_l2tp_ipaddr" name="wan_l2tp_ipaddr" size="20" maxlength="15" value="<% CmoGetCfg("wan_l2tp_ipaddr","none"); %>">
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(_L2TPsubnet)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=text id="wan_l2tp_netmask" name="wan_l2tp_netmask" size="20" maxlength="15" value="<% CmoGetCfg("wan_l2tp_netmask","none"); %>">
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(_L2TPgw)</script>

                  :</td>

                      <td>&nbsp;
                        <input name="wan_l2tp_gateway" type=text id="wan_l2tp_gateway" size="20" maxlength="15" value="<% CmoGetCfg("wan_l2tp_gateway","none"); %>">
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(bwn_L2TPSIPA)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=text id="wan_l2tp_server_ip" name="wan_l2tp_server_ip" size="20" value="<% CmoGetCfg("wan_l2tp_server_ip","none"); %>">
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(bwn_UN)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=text id="wan_l2tp_username" name="wan_l2tp_username" size="20" maxlength="63" value="<% CmoGetCfg("wan_l2tp_username","none"); %>">
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(_password)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=password id="l2tppwd1" name="l2tppwd1" size="20" maxlength="63" onfocus="select();" >
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(_verifypw)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=password id=l2tppwd2 name=l2tppwd2 size="20" maxlength="63" onfocus="select();" >
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(bwn_RM)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=radio name="wan_l2tp_connect_mode" value="always_on" onClick="check_connectmode()">

                  <script>show_words(bwn_RM_0)</script>

                        <input type=radio name="wan_l2tp_connect_mode" value="on_demand" onClick="check_connectmode()">

                  <script>show_words(bwn_RM_1)</script>

                        <input type=radio name="wan_l2tp_connect_mode" value="manual" onClick="check_connectmode()">

                  <script>show_words(bwn_RM_2)</script>

                       </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(bwn_MIT)</script>

                  :</td>

                      <td>&nbsp;
                          <input type=text id="wan_l2tp_max_idle_time" name="wan_l2tp_max_idle_time" maxlength="5" size="10" value="<% CmoGetCfg("wan_l2tp_max_idle_time","none"); %>">

                  <script>show_words(bwn_min)</script>

                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(_dns1)</script>

                  :</td>

                      <td>&nbsp;
                          <input type=text id="wan_primary_dns" name="wan_primary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_primary_dns","none"); %>">
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(_dns2)</script>

                  :</td>

                      <td>&nbsp;
                          <input type=text id="wan_secondary_dns" name="wan_secondary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_secondary_dns","none"); %>">
                      </td>
                    </tr>
                    <tr>



                <td align=right class="duple">

                  <script>show_words(bwn_MTU)</script>

                  :</td>

                      <td>&nbsp;
                        <input type=text id="wan_l2tp_mtu" name="wan_l2tp_mtu" size="10" maxlength="5" value="<% CmoGetCfg("wan_l2tp_mtu","none"); %>">

                  <script>show_words(bwn_bytes)</script>

                  <script>show_words(_308)</script>

                  1400</td>

                    </tr>
                     <tr>



                <td width=150 valign=top class="duple">

                  <script>show_words(_macaddr)</script>

                  :</td>

                      <td>&nbsp;
                        <input type="text" id="wan_mac" name="wan_mac" size="20" maxlength="17" value="<% CmoGetCfg("wan_mac","none"); %>"><br>
                        &nbsp;<input name="clone" id="clone" type="button" class=button_submit value="" onClick="clone_mac_action()">
                        <script>get_by_id("clone").value = _clone;</script></td>
                      </td>
                    </tr>
                </table>
              </div>
            </div>
</form>
           </td>


            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
                    <tr>

                      <td id=help_text><strong><script>show_words(_hints)</script>

            &hellip;</strong> <p>

              <script>show_words(LW35)</script>

            </p>

            <p><script>show_words(LW36)</script></p>

                          <p class="more"><a href="support_internet.asp#WAN" onclick="return jump_if();">More&hellip;</a></p>
                      </td>
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