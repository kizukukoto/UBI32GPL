<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script>
    var submit_button_flag = 0;
    function opendns_enable_selector(value)
    {
        if (value == true) {
            get_by_id("wan_specify_dns").value = "1";
            get_by_id("wan_primary_dns").value = "204.194.232.200";
            get_by_id("wan_secondary_dns").value = "204.194.234.200";
            get_by_id("wan_primary_dns").disabled = true;
            get_by_id("wan_secondary_dns").disabled = true;
        }
        else {
            get_by_id("wan_specify_dns").value = "0";
            get_by_id("wan_primary_dns").disabled = false;
            get_by_id("wan_secondary_dns").disabled = false;
            get_by_id("wan_primary_dns").value = "0.0.0.0";
            get_by_id("wan_secondary_dns").value =  "0.0.0.0";
        }
    }

    function onPageLoad()
    {
        set_checked(get_by_id("opendns_enable").value, get_by_id("opendns_enable_sel"));
        if (get_by_id("opendns_enable_sel").checked)
            opendns_enable_selector(get_by_id("opendns_enable_sel").checked);
        set_checked("<% CmoGetCfg("wan_pptp_dynamic","none"); %>", get_by_name("wan_pptp_dynamic"));
        set_checked("<% CmoGetCfg("wan_pptp_connect_mode","none"); %>", get_by_name("wan_pptp_connect_mode"));
        get_by_id("pptppwd1").value = "<% CmoGetCfg("wan_pptp_password","none"); %>";
        get_by_id("pptppwd2").value = get_by_id("pptppwd1").value;
        check_connectmode();
        clickPPTP();
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
        var conn_mode = get_by_name("wan_pptp_connect_mode");
        get_by_id("wan_pptp_max_idle_time").disabled = !conn_mode[1].checked;
    }

    function clickPPTP()
    {
        var fixIP = get_by_name("wan_pptp_dynamic");
        var is_disabled = false;
        if (fixIP[0].checked)
            is_disabled = true;
        get_by_id("wan_pptp_ipaddr").disabled = is_disabled;
        get_by_id("wan_pptp_netmask").disabled = is_disabled;
        get_by_id("wan_pptp_gateway").disabled = is_disabled;
    }
    function send_request()
    {
    	    var ipv6_pppoe_share ="<% CmoGetStatus("ipv6_pppoe_share"); %>"; 
	        var ipv6_wan_proto = "<% CmoGetStatus("ipv6_wan_proto"); %>";
        	if (ipv6_wan_proto == "ipv6_pppoe" && ipv6_pppoe_share == "1"){
                	alert(IPV6_TEXT1);
	                return false;
        	}    	
        get_by_id("asp_temp_52").value = get_by_id("wan_proto").value;

        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }
        var fix_ip = get_by_name("wan_pptp_dynamic");
        var ip = get_by_id("wan_pptp_ipaddr").value;
        var mask = get_by_id("wan_pptp_netmask").value;
        var gateway = get_by_id("wan_pptp_gateway").value;
        var dns1 = get_by_id("wan_primary_dns").value;
        var dns2 = get_by_id("wan_secondary_dns").value;
        var server_ip = get_by_id("wan_pptp_server_ip").value;
        var user_name = get_by_id("wan_pptp_username").value;
        var idle = get_by_id("wan_pptp_max_idle_time").value;
        var mtu = get_by_id("wan_pptp_mtu").value;

        var ip_addr_msg = replace_msg(all_ip_addr_msg,_ipaddr);
        var gateway_msg = replace_msg(all_ip_addr_msg,wwa_gw);
        var primary_dns_msg = replace_msg(all_ip_addr_msg,wwa_pdns);
        var second_dns_msg = replace_msg(all_ip_addr_msg,wwa_sdns);
        var server_ip_msg = replace_msg(all_ip_addr_msg,bwn_PPTPSIPA);

        var max_idle_msg = replace_msg(check_num_msg, bwn_MIT, 0, 9999);
        var mtu_msg = replace_msg(check_num_msg, bwn_MTU, 1300, 1400);

        var temp_ip_obj = new addr_obj(ip.split("."), ip_addr_msg, false, false);
        var temp_mask_obj = new addr_obj(mask.split("."), subnet_mask_msg, false, false);
        var temp_gateway_obj = new addr_obj(gateway.split("."), gateway_msg, false, false);
        var temp_dns1 = new addr_obj(dns1.split("."), primary_dns_msg, true, false);
        var temp_dns2 = new addr_obj(dns2.split("."), second_dns_msg, true, false);
        var temp_sip_obj = new addr_obj(server_ip.split("."), server_ip_msg, false, false);
        var temp_idle = new varible_obj(idle, max_idle_msg, 0, 9999, false);
        var temp_mtu = new varible_obj(mtu, mtu_msg, 1300, 1400, false);

        if (fix_ip[1].checked) {
            if (!check_lan_setting(temp_ip_obj, temp_mask_obj, temp_gateway_obj, "PPTP")) {
                return false;
            }
        }

    	if (server_ip == "") {
    		alert(YM108);
    		return false;
	    }

	if(!check_servername(server_ip)) {
        	if (!check_address(temp_sip_obj)) 
            return false;
        }
        if (user_name == "") {
    		alert(GW_WAN_PPTP_USERNAME_INVALID);
            return false;
        }

	     if (get_by_id("pptppwd1").value == "" || get_by_id("pptppwd2").value == ""){
		 	alert(GW_WAN_PPTP_PASSWORD_INVALID);
			return false;
		}
        if (!check_pwd("pptppwd1", "pptppwd2")) {
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

    	if(usb_mode == "2" && usb_type == "2"  && real_wanmode =="pptp" ||usb_mode == "0" && usb_type == "2" && real_wanmode =="pptp"){ 
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
				get_by_id("wan_mac").value = get_by_id("old_wan_mac").value;
			else
			get_by_id("wan_mac").value = mac;
		}

        if (dns1 != "" && dns1 != "0.0.0.0") {
            if (!check_address(temp_dns1)) {
                return false;
            }
        }

        if ((get_by_id("wan_primary_dns").value == "" || get_by_id("wan_primary_dns").value == "0.0.0.0") && (get_by_id("wan_secondary_dns").value == "" || get_by_id("wan_secondary_dns").value == "0.0.0.0"))
            get_by_id("wan_specify_dns").value = 0;
        else
            get_by_id("wan_specify_dns").value = 1;
get_by_id("wan_pptp_russia_enable").value = "1";
        get_by_id("opendns_enable").value = get_checked_value(get_by_id("opendns_enable_sel"));
        if (get_by_id("opendns_enable").value == "1") {
            get_by_id("dns_relay").value = "1";
            get_by_id("wan_primary_dns").disabled = false;
            get_by_id("wan_secondary_dns").disabled = false;
        }

        if (submit_button_flag == 0) {
            get_by_id("wan_pptp_password").value = get_by_id("pptppwd1").value;
            submit_button_flag = 1;
            return true;
        }

        return false;
    }
</script>

<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<style type="text/css">
<!--
.style1 {font-size: 11px}
-->
</style>
</head>

<body onload="onPageLoad();" topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
    <table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
      <tr>
        <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/" onclick="return jump_if();"><% CmoGetCfg("model_number","none"); %></a></td>
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

            <input type="hidden" id="old_wan_mac" name="old_wan_mac" value="<% CmoGetCfg("wan_mac","none"); %>">

            <form id="form1" name="form1" method="post" action="apply.cgi">
            <input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
            <input type="hidden" id="html_response_message" name="html_response_message" value="">
            <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
            <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="wan_rus_pptp.asp">
            <input type="hidden" id="mac_clone_addr" name="mac_clone_addr" value="<% CmoGetStatus("mac_clone_addr"); %>">
            <input type="hidden" id="wan_specify_dns" name="wan_specify_dns" value="<% CmoGetCfg("wan_specify_dns","none"); %>">
            <input type="hidden" id="wan_pptp_password"  name="wan_pptp_password" value="">
            <input type="hidden" id="asp_temp_51" name="asp_temp_51" value="<% CmoGetCfg("asp_temp_51","none"); %>">
            <input type="hidden" id="asp_temp_52" name="asp_temp_52" value="<% CmoGetCfg("wan_proto","none"); %>">
            <input type="hidden" id="usb_type" name="usb_type" value="<% CmoGetCfg("usb_type","none"); %>">
            <input type="hidden" id="from_usb3g" name="from_usb3g" value="<% CmoGetCfg("asp_temp_72","none"); %>"> 
		<input type="hidden" id="wan_pptp_russia_enable" name="wan_pptp_russia_enable" value="<% CmoGetCfg("wan_pptp_russia_enable","none"); %>">
<input type="hidden" id="wan_l2tp_russia_enable" name="wan_l2tp_russia_enable" value="0">
                                <input type="hidden" id="wan_pppoe_russia_enable" name="wan_pppoe_russia_enable" value="0">
            <input type="hidden" id="reboot_type" name="reboot_type" value="shutdown">
            <td valign="top" id="maincontent_container">
                <div id="maincontent">
              <div id=box_header>
            <h1><script>show_words(_WAN)</script></h1>
            <p><script>show_words(bwn_intro_ICS)</script></p>
            <p><strong><b><script>show_words(_note)</script></b>:&nbsp;</strong>
              <script>show_words(bwn_note_clientSW)</script>
            </p>
            <input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_request()">
            <input name="button2" id="button2" type=reset class=button_submit value="" onclick="page_cancel('form1', 'sel_wan.asp');">
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
                        <option value="l2tp"><script>show_words(bwn_Mode_L2TP)</script></option>
                        <option value="usb3g"><script>show_words(usb_3g)</script></option>
                        <!--option value="bigpond"><script>show_words(bwn_Mode_BigPond)</script></option-->
                        <option value="dslite">DS-Lite</option>
<option value="pptp" selected>Russia PPTP (Dual Access)</option>
<option value="l2tp">Russia L2TP (Dual Access)</option>
                        <option value="pppoe">Russia PPPoE (Dual Access)</option>
                      </select></td>
                    </tr>
                </table>
              </div>
               <!--IFDEF    OPENDNS-->
                <input type="hidden" id="opendns_enable" name="opendns_enable" value="<% CmoGetCfg("opendns_enable","none"); %>">
                <input type="hidden" id="dns_relay" name="dns_relay" value="<% CmoGetCfg("dns_relay","none"); %>">
                <div class=box style="display:none">
				<h2><script>show_words(_title_AdvDns);</script></h2>
				<p class="box_msg"><script>show_words(_desc_AdvDns);</script></p>
                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                <tr>
				<td width="185" align=right style="WIDTH: 190px;" class="duple"><script>show_words(_en_AdvDns);</script> :</td>
                <td width="331">&nbsp;<input type="checkbox" id="opendns_enable_sel" name="opendns_enable_sel" value="1" onclick="opendns_enable_selector(this.checked);"></td>
                </tr>
                </table>
                </div>
                <!--ENDIF   OPENDNS-->
              <div class=box id=show_pptp>
                <h2>RUSSIA PPTP (DUAL ACCESS) <script>show_words(bwn_ict)</script> </h2>
                <p class="box_msg"><script>show_words(_ispinfo)</script> </p>

                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                    <tr>
                <td width="185" align=right class="duple"><script>show_words(bwn_AM)</script></td>
                      <td width="331">&nbsp;
                      <input type=radio value="1" name="wan_pptp_dynamic" onClick=clickPPTP() checked>
                  <script>show_words(carriertype_ct_0)</script>
                      <input type=radio value="0" name="wan_pptp_dynamic" onClick=clickPPTP()>
                  <script>show_words(_sdi_staticip)</script></td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(_PPTPip)</script> :</td>
                      <td>&nbsp;
                        <input type=text id="wan_pptp_ipaddr" name="wan_pptp_ipaddr" size="20" maxlength="15" value="<% CmoGetCfg("wan_pptp_ipaddr","none"); %>">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(_PPTPsubnet)</script> :</td>
                      <td>&nbsp;
                        <input type=text id="wan_pptp_netmask" name="wan_pptp_netmask" size="20" maxlength="15" value="<% CmoGetCfg("wan_pptp_netmask","none"); %>">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(_PPTPgw)</script> :</td>
                      <td>&nbsp;
                        <input name="wan_pptp_gateway" type=text id="wan_pptp_gateway" size="20" maxlength="15" value="<% CmoGetCfg("wan_pptp_gateway","none"); %>">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(bwn_PPTPSIPA)</script> :</td>
                      <td>&nbsp;
                        <input type=text id="wan_pptp_server_ip" name="wan_pptp_server_ip" size="20" value="<% CmoGetCfg("wan_pptp_server_ip","none"); %>">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(bwn_UN)</script> :</td>
                      <td>&nbsp;
                        <input type=text id="wan_pptp_username" name="wan_pptp_username" size="20" maxlength="63" value="<% CmoGetCfg("wan_pptp_username","none"); %>">
                      </td>
                    </tr>
                    <tr>
                <td  align=right class="duple"><script>show_words(_password)</script> :</td>
                      <td>&nbsp;
                        <input type=password id="pptppwd1" name="pptppwd1" size="20" maxlength="63" onfocus="select();" value="">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(_verifypw)</script> :</td>

                      <td>&nbsp;
                        <input type=password id=pptppwd2 name=pptppwd2 size="20" maxlength="63" onfocus="select();" value="">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(bwn_RM)</script> :</td>
                      <td>&nbsp;
                      <input type=radio name="wan_pptp_connect_mode" value="always_on" onClick="check_connectmode()">
                  <script>show_words(bwn_RM_0)</script>
                      <input type=radio name="wan_pptp_connect_mode" value="on_demand" onClick="check_connectmode()">
                  <script>show_words(bwn_RM_1)</script>
                      <input type=radio name="wan_pptp_connect_mode" value="manual" onClick="check_connectmode()">
                  <script>show_words(bwn_RM_2)</script></td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(bwn_MIT)</script> :</td>
                      <td>&nbsp;
                        <input type=text id="wan_pptp_max_idle_time" name="wan_pptp_max_idle_time" maxlength="5" size="10" value="<% CmoGetCfg("wan_pptp_max_idle_time","none"); %>">
                  <script>show_words(bwn_min)</script></td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(_dns1)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_primary_dns" name="wan_primary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_primary_dns","none"); %>">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(_dns2)</script> :</td>
                      <td>&nbsp;<input type=text id="wan_secondary_dns" name="wan_secondary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_secondary_dns","none"); %>">
                      </td>
                    </tr>
                    <tr>
                <td align=right class="duple"><script>show_words(bwn_MTU)</script> :</td>
                      <td>&nbsp;
                        <input type=text id="wan_pptp_mtu" name="wan_pptp_mtu" size="10" maxlength="5" value="<% CmoGetCfg("wan_pptp_mtu","none"); %>">
                  <script>show_words(bwn_bytes)</script>
                  <script>show_words(_308)</script>
                  1400</td>
                    </tr>
                    <tr>
                <td width=150 valign=top class="duple">
                  <script>show_words(_macaddr)</script> :</td>
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
                      <td id=help_text><strong><script>show_words(_hints)</script>&hellip;</strong> 
<p><script>show_words(LW35)</script></p>
           <p><script>show_words(LW36)</script></p>
            <!--IFDEF	OPENDNS-->
            <p><script>show_words(_sp_desc2_AdvDNS)</script></p>
			<p><script>show_words(_sp_desc3_AdvDNS)</script></p>
			<p><script>show_words(_sp_desc4_AdvDNS)</script></p>
			<!--END	OPENDNS-->
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
</html>
