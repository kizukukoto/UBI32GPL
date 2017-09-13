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

        var ip = get_by_id("wan_static_ipaddr").value;
        var mask = get_by_id("wan_static_netmask").value;
        var gateway = get_by_id("wan_static_gateway").value;
        var dns1 = get_by_id("wan_primary_dns").value;
        var dns2 = get_by_id("wan_secondary_dns").value;
        var mtu = get_by_id("wan_mtu").value;
        var lanip = get_by_id("lan_ipaddr").value;
        var lanmask = get_by_id("lan_netmask").value;
		var ip_addr_msg = replace_msg(all_ip_addr_msg,_ipaddr);
		var gateway_msg = replace_msg(all_ip_addr_msg,wwa_gw);
		var dns1_addr_msg = replace_msg(all_ip_addr_msg,wwa_pdns);
		var dns2_addr_msg = replace_msg(all_ip_addr_msg,wwa_sdns);
        var mtu_msg = replace_msg(check_num_msg, bwn_MTU, 1300, 1500);

        var temp_ip_obj = new addr_obj(ip.split("."), ip_addr_msg, false, false);
        var temp_mask_obj = new addr_obj(mask.split("."), subnet_mask_msg, false, false);
        var temp_lanip_obj = new addr_obj(lanip.split("."), ip_addr_msg, false, false);
        var temp_lanmask_obj = new addr_obj(lanmask.split("."), subnet_mask_msg, false, false);
        var temp_gateway_obj = new addr_obj(gateway.split("."), gateway_msg, false, false);
        var temp_dns1_obj = new addr_obj(dns1.split("."), dns1_addr_msg, false, false);
        var temp_dns2_obj = new addr_obj(dns2.split("."), dns2_addr_msg, true, false);
        var temp_mtu = new varible_obj(mtu, mtu_msg, 1300, 1500, false);

        if (!check_lan_setting(temp_ip_obj, temp_mask_obj, temp_gateway_obj, "WAN")) {
            return false;
        }

        if (!check_address(temp_dns1_obj)) {
            return false;
        }

        if (dns2 != "" && dns2 != "0.0.0.0") {
            if (!check_address(temp_dns2_obj)) {
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

    	if(usb_mode == "2" && usb_type == "2"  && real_wanmode =="static" ||usb_mode == "0" && usb_type == "2" && real_wanmode =="static"){ 
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

        //check lan and wan is same subnet
        if (temp_lanip_obj && temp_lanmask_obj) {
            var ipaddr = temp_ip_obj.addr;
            var maskaddr = temp_mask_obj.addr;
            var lanipaddr = temp_lanip_obj.addr;
            var lanmaskaddr = temp_lanmask_obj.addr;
            var count = 0;

            for (var i = 0; i < ipaddr.length; i++) {
                if ((ipaddr[i] & lanmaskaddr[i]) == (lanipaddr[i] & lanmaskaddr[i]))
                    count++;
            }

            if (count == ipaddr.length) {
        		alert(GW_WAN_LAN_SUBNET_CONFLICT_INVALID);
                return false;
            }
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

        if ((get_by_id("wan_primary_dns").value == "" || get_by_id("wan_primary_dns").value =="0.0.0.0") && (get_by_id("wan_secondary_dns").value =="" || get_by_id("wan_secondary_dns").value =="0.0.0.0"))
            get_by_id("wan_specify_dns").value = 0;
        else
            get_by_id("wan_specify_dns").value = 1;

        if (submit_button_flag == 0) {
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
            <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="wan_static.asp">
            <input type="hidden" id="usb_type" name="usb_type" value="<% CmoGetCfg("usb_type","none"); %>">
            <input type="hidden" id="from_usb3g" name="from_usb3g" value="<% CmoGetCfg("asp_temp_72","none"); %>"> 
            <input type="hidden" id="reboot_type" name="reboot_type" value="shutdown">

            <input type="hidden" id="mac_clone_addr" name="mac_clone_addr" value="<% CmoGetStatus("mac_clone_addr"); %>">
            <input type="hidden" id="wan_specify_dns" name="wan_specify_dns" value="<% CmoGetCfg("wan_specify_dns","none"); %>">
            <input type="hidden" id="asp_temp_51" name="asp_temp_51" value="<% CmoGetCfg("asp_temp_51","none"); %>">
            <input type="hidden" id="asp_temp_52" name="asp_temp_52" value="<% CmoGetCfg("wan_proto","none"); %>">
            <input type="hidden" id="lan_ipaddr" name="lan_ipaddr" value="<% CmoGetCfg("lan_ipaddr","none"); %>">
            <input type="hidden" id="lan_netmask" name="lan_netmask" value="<% CmoGetCfg("lan_netmask","none"); %>">

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

              </b>:&nbsp;</strong>

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

                        <option value="static" selected><script>show_words(_sdi_staticip)</script></option>

                        <option value="dhcpc"><script>show_words(bwn_Mode_DHCP)</script></option>

                        <option value="pppoe"><script>show_words(bwn_Mode_PPPoE)</script></option>

                        <option value="pptp"><script>show_words(bwn_Mode_PPTP)</script></option>

                        <option value="l2tp"><script>show_words(bwn_Mode_L2TP)</script></option>

                        <!--option value="usb3g"><script>show_words(usb_3g)</script></option-->

                         <!--option value="bigpond">BigPond (Australia)</option-->
			<option value="dslite">DS-Lite</option>                      
			</select></td>
                    </tr>
                </table>
              </div>

              <div class=box id=show_static>
                <h2><script>show_words(bwn_SIAICT)</script></h2>
                <p class="box_msg"><script>show_words(bwn_msg_SWM)</script></p>

                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                  <tr>

                <td width="185" class="duple">
                  <script>show_words(_ipaddr)</script>
                  : </td>
                    <td width="331">&nbsp;
                    <input type=text id="wan_static_ipaddr" name="wan_static_ipaddr" size="20" maxlength="15" value="<% CmoGetCfg("wan_static_ipaddr","none"); %>">
                    </td>
                  </tr>
                  <tr>

                <td width=150 class="duple">
                  <script>show_words(_subnet)</script>
                  : </td>
                    <td>&nbsp;
                    <input type=text id="wan_static_netmask" name="wan_static_netmask" size="20" maxlength="15" value="<% CmoGetCfg("wan_static_netmask","none"); %>">
                    </td>
                  </tr>
                  <tr>

                <td width=150 class="duple">
                  <script>show_words(_defgw)</script>
                  : </td>
                    <td>&nbsp;
                    <input type=text id="wan_static_gateway" name="wan_static_gateway" size="20" maxlength="15" value="<% CmoGetCfg("wan_static_gateway","none"); %>">
                    </td>
                  </tr>
                  <tr>

                <td width=150 class="duple">
                  <script>show_words(_dns1)</script>
                  : </td>
                    <td>&nbsp;
                    <input type=text id="wan_primary_dns" name="wan_primary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_primary_dns","none"); %>">
                    </td>
                  </tr>
                  <tr>

                <td width=150 class="duple">
                  <script>show_words(_dns2)</script>
                  : </td>
                    <td>&nbsp;
                    <input type=text id="wan_secondary_dns" name="wan_secondary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_secondary_dns","none"); %>">
                    </td>
                  </tr>
                  <tr>

                <td width=150 class="duple">
                  <script>show_words(bwn_MTU)</script>
                  : </td>
                    <td>&nbsp;
                    <input type=text id="wan_mtu" name="wan_mtu" size="10" maxlength="5" value="<% CmoGetCfg("wan_mtu","none"); %>">

                  <script>show_words(bwn_bytes)</script>

                  <script>show_words(_308)</script>

                  1500</td>

                  </tr>
                  <tr>

                <td width=150 valign=top class="duple">

                  <script>show_words(_macaddr)</script>

                  : </td>

                    <td>&nbsp;
                        <input type="text" id="wan_mac" name="wan_mac" size="20" maxlength="17" value="<% CmoGetCfg("wan_mac","none"); %>"><br>
                        &nbsp;<input name="clone" id="clone" type=button class=button_submit value="" onClick="clone_mac_action()">
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