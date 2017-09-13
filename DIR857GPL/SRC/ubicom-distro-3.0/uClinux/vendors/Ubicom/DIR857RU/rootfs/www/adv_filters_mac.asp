<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var submit_button_flag = 0;
    var rule_max_num = 24;
    var resert_rule = rule_max_num;
    var DHCPL_DataArray = new Array();
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

    function onPageLoad()
    {
        set_selectIndex("<% CmoGetCfg("mac_filter_type","none"); %>", get_by_id("mac_filter_type"));
        disable_mac_filter();

        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1,true);
        }

        set_mac_filter();
        set_form_default_values("form1");
    }

    function DHCP_Data(name, ip, mac, Exp_time, onList)
    {
        this.Name = name;
        this.IP = ip;
        this.MAC = mac;
        this.EXP_T = Exp_time;
        this.OnList = onList;
    }

    function set_mac_filter()
    {
        for (var i = 0; i < rule_max_num; i++) {
            var temp_mf;

            if (i > 9)
                temp_mf = (get_by_id("mac_filter_" + i).value).split("/");
            else
                temp_mf = (get_by_id("mac_filter_0" + i).value).split("/");

            if (temp_mf.length > 1){
                if (temp_mf[2] != "00:00:00:00:00:00") {
                    get_by_id("mac" + i).value = temp_mf[2];
                }
                if (temp_mf[2].length > 0 && temp_mf[2] !="00:00:00:00:00:00") {
                    resert_rule--;
                }
            }
        }
    }

    function disable_mac_filter()
    {
        var mac_filter_type = get_by_id("mac_filter_type").selectedIndex;

        for (var i = 0; i < rule_max_num; i++) {
            get_by_id("mac" + i).disabled = !(mac_filter_type != 0);
            get_by_id("copy" + i).disabled = !(mac_filter_type != 0);
            get_by_id("dhcp_list" + i).disabled = !(mac_filter_type != 0);
            get_by_id("clear" + i).disabled = !(mac_filter_type != 0);
        }
    }

    function copy_mac(index)
    {
        if (get_by_id("dhcp_list" + index).selectedIndex > 0)
            get_by_id("mac" + index).value = get_by_id("dhcp_list" + index).options[get_by_id("dhcp_list" + index).selectedIndex].value;
        else
            alert(msg[SELECT_MACHINE_ERROR]);
    }

    function clear_mac(index)
    {
        get_by_id("mac" + index).value = "";
    }

    function check_dhcp_ip(index)
    {
        var index = 0;
        var mac = get_by_id("dhcp_list" + index).options[get_by_id("dhcp_list" + index).selectedIndex].value;
        var temp_dhcp_list = get_by_id("dhcp_list").value.split(",");

        for (var i = 0; i < temp_dhcp_list.length; i++) {
            var temp_data = temp_dhcp_list[i].split("/");
            if (temp_data.length > 1) {
                DHCPL_DataArray[DHCPL_DataArray.length++] = new DHCP_Data(temp_data[0], temp_data[1], temp_data[2], temp_data[3], index);

                //check selected mac equal to mac or not?
                index++;
                if (mac == temp_data[2]) {
                    var lan_ip = "<% CmoGetCfg("lan_ipaddr","none"); %>";
                    var lan_ip_addr_msg = replace_msg(all_ip_addr_msg,"IP address");
                    var temp_lan_ip_obj = new addr_obj(lan_ip.split("."), lan_ip_addr_msg, false, false);
                    var ip = temp_data[1];
                    var ip_addr_msg = replace_msg(all_ip_addr_msg,"IP address");
                    var temp_ip_obj = new addr_obj(ip.split("."), ip_addr_msg, false, false);

                    if (!check_LAN_ip(temp_lan_ip_obj.addr, temp_ip_obj.addr, "IP address")) {
                        return false;
                    }

                    return true;
                }
            }
        }
        return true;
    }

    function send_request()
    {
        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }

        for (var i = 0; i < rule_max_num; i++) {
            var mac = get_by_id("mac" + i).value;

            if (mac == "") {
                continue;
            }
            if (get_by_id("mac" + i).value != ""){			
				if (mac != "00:00:00:00:00:00" && mac !=""){
					if (!check_mac(get_by_id("mac" + i).value)){
						alert(LS47);
						return false;
					}									
				}			
            }

            if (mac == "00:00:00:00:00:00" || (!is_mac_valid(get_by_id("mac" + i).value))) {
                alert(LS47);
                return false;
            }

            if (!check_dhcp_ip()) {
                return false;
            }

            for (var j = i+1; j < rule_max_num; j++) {
                if (mac != "00:00:00:00:00:00" && mac !="" && mac.toUpperCase() == get_by_id("mac" + j).value.toUpperCase()) {
                    alert(Tag04105);
                    return false;
                }
            }
        }

        var count = 0;
        for (var i = 0; i < rule_max_num; i++) {
            if (i > 9)
                get_by_id("mac_filter_" + i).value = "0/Name/00:00:00:00:00:00/Always";
            else
                get_by_id("mac_filter_0" + i).value = "0/Name/00:00:00:00:00:00/Always";

            if (get_by_id("mac" + i).value != "" && get_by_id("mac" + i).value != "00:00:00:00:00:00") {
                var dat = "1/Name/"+ get_by_id("mac" + i).value + "/Always";
                if (count > 9)
                    get_by_id("mac_filter_" + count).value = dat;
                else
                    get_by_id("mac_filter_0" + count).value = dat;

                count++;
            }
        }

        if ((count == 0) && (get_by_id("mac_filter_type").selectedIndex == 1)) {
            alert(GW_MAC_FILTER_ALL_LOCKED_OUT_INVALID);
            return false;
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
                            <LI><div><a href="adv_virtual.asp" onclick="return jump_if();"><script>show_words(_virtserv)</script></a></div></LI>
                            <LI><div><a href="adv_portforward.asp" onclick="return jump_if();"><script>show_words(_pf)</script></a></div></LI>
                            <LI><div><a href="adv_appl.asp" onclick="return jump_if();"><script>show_words(_specappsr)</script></a></div></LI>
                            <LI><div><a href="adv_qos.asp" onclick="return jump_if();"><script>show_words(YM48)</script></a></div></LI>
                            <LI><div id=sidenavoff><script>show_words(_netfilt)</script></div></LI>
                            <LI><div><a href="adv_access_control.asp" onclick="return jump_if();"><script>show_words(_acccon)</script></a></div></LI>
                            <LI><div><a href="adv_filters_url.asp" onclick="return jump_if();"><script>show_words(_websfilter)</script></a></div></LI>
                            <LI><div><a href="Inbound_Filter.asp" onclick="return jump_if();"><script>show_words(_inboundfilter)</script></a></div></LI>
                            <LI><div><a href="adv_dmz.asp" onclick="return jump_if();"><script>show_words(_firewalls)</script></a></div></LI>
                            <LI><div><a href="adv_routing.asp" onclick="return jump_if();"><script>show_words(_routing)</script></a></div></LI>
                            <LI><div><a href="adv_wlan_perform.asp" onclick="return jump_if();"><script>show_words(_adwwls)</script></a></div></LI>
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
                </table></td>

                <input type="hidden" id="dhcp_list" name="dhcp_list" value="<% CmoGetList("dhcpd_leased_table"); %>">

                <form id="form1" name="form1" method="post" action="apply.cgi">
                <input type="hidden" id="html_response_page" name="html_response_page" value="back_long.asp">
                <input type="hidden" id="html_response_message" name="html_response_message" value="">
                <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
                <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="adv_filters_mac.asp">
                <input type="hidden" id="reboot_type" name="reboot_type" value="all">

                <input type="hidden" id="mac_filter_00" name="mac_filter_00" value="<% CmoGetCfg("mac_filter_00","none"); %>">
                <input type="hidden" id="mac_filter_01" name="mac_filter_01" value="<% CmoGetCfg("mac_filter_01","none"); %>">
                <input type="hidden" id="mac_filter_02" name="mac_filter_02" value="<% CmoGetCfg("mac_filter_02","none"); %>">
                <input type="hidden" id="mac_filter_03" name="mac_filter_03" value="<% CmoGetCfg("mac_filter_03","none"); %>">
                <input type="hidden" id="mac_filter_04" name="mac_filter_04" value="<% CmoGetCfg("mac_filter_04","none"); %>">
                <input type="hidden" id="mac_filter_05" name="mac_filter_05" value="<% CmoGetCfg("mac_filter_05","none"); %>">
                <input type="hidden" id="mac_filter_06" name="mac_filter_06" value="<% CmoGetCfg("mac_filter_06","none"); %>">
                <input type="hidden" id="mac_filter_07" name="mac_filter_07" value="<% CmoGetCfg("mac_filter_07","none"); %>">
                <input type="hidden" id="mac_filter_08" name="mac_filter_08" value="<% CmoGetCfg("mac_filter_08","none"); %>">
                <input type="hidden" id="mac_filter_09" name="mac_filter_09" value="<% CmoGetCfg("mac_filter_09","none"); %>">
                <input type="hidden" id="mac_filter_10" name="mac_filter_10" value="<% CmoGetCfg("mac_filter_10","none"); %>">
                <input type="hidden" id="mac_filter_11" name="mac_filter_11" value="<% CmoGetCfg("mac_filter_11","none"); %>">
                <input type="hidden" id="mac_filter_12" name="mac_filter_12" value="<% CmoGetCfg("mac_filter_12","none"); %>">
                <input type="hidden" id="mac_filter_13" name="mac_filter_13" value="<% CmoGetCfg("mac_filter_13","none"); %>">
                <input type="hidden" id="mac_filter_14" name="mac_filter_14" value="<% CmoGetCfg("mac_filter_14","none"); %>">
                <input type="hidden" id="mac_filter_15" name="mac_filter_15" value="<% CmoGetCfg("mac_filter_15","none"); %>">
                <input type="hidden" id="mac_filter_16" name="mac_filter_16" value="<% CmoGetCfg("mac_filter_16","none"); %>">
                <input type="hidden" id="mac_filter_17" name="mac_filter_17" value="<% CmoGetCfg("mac_filter_17","none"); %>">
                <input type="hidden" id="mac_filter_18" name="mac_filter_18" value="<% CmoGetCfg("mac_filter_18","none"); %>">
                <input type="hidden" id="mac_filter_19" name="mac_filter_19" value="<% CmoGetCfg("mac_filter_19","none"); %>">
                <input type="hidden" id="mac_filter_20" name="mac_filter_20" value="<% CmoGetCfg("mac_filter_20","none"); %>">
                <input type="hidden" id="mac_filter_21" name="mac_filter_21" value="<% CmoGetCfg("mac_filter_21","none"); %>">
                <input type="hidden" id="mac_filter_22" name="mac_filter_22" value="<% CmoGetCfg("mac_filter_22","none"); %>">
                <input type="hidden" id="mac_filter_23" name="mac_filter_23" value="<% CmoGetCfg("mac_filter_23","none"); %>">
                <input type="hidden" id="mac_filter_24" name="mac_filter_24" value="<% CmoGetCfg("mac_filter_24","none"); %>">

            <td valign="top" id="maincontent_container">
                <div id="maincontent">
                  <div id=box_header>
                    <h1><script>show_words(_macfilt)</script></h1>
                    <script>show_words(am_intro_1)</script>
                    <br><br>
                    <input name="button" type="submit" id="button" class=button_submit value="" onClick="return send_request()">
                    <input name="button2" type="button" id="button2" class=button_submit value="" onclick="page_cancel('form1', 'adv_filters_mac.asp');">
                    <script>check_reboot();</script>
                    <script>get_by_id("button2").value = _dontsavesettings;</script>
                    <script>get_by_id("button").value = _savesettings;</script>
                  </div>
                  <br>
                  <div class=box>
                    <h2><script>document.write(rule_max_num)</script> &ndash;&ndash; <script>show_words(am_MACFILT)</script></h2>
                    <table cellSpacing=1 cellPadding=2 width=525 border=0>
                      <tbody>
                        <tr>
                          <td><script>show_words(am_intro)</script>:</td>
                        </tr>
                        <tr>
                          <td>
                          <select id="mac_filter_type" name="mac_filter_type" onChange="disable_mac_filter();">
                              <option value="disable"><script>show_words(am_FM_2)</script></option>
                              <option value="list_allow"><script>show_words(am_FM_3)</script></option>
                              <option value="list_deny"><script>show_words(am_FM_4)</script></option>
                            </select>
                          </td>
                        </tr>
                      </tbody>
                    </table>
                    <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1>
                      <tbody>
                        <tr>
                          <td width="100" align=left><strong><script>show_words(_macaddr)</script></strong></td>
                          <td>&nbsp;</td>
                          <td width="250" align=left><strong><script>show_words(bd_DHCP)</script></strong></td>
                          <td>&nbsp;</td>
                        </tr>
                        <script>
                            //var Schedule_list = set_schedule_option(Schedule_list);
                            for(var i=0 ; i<rule_max_num ; i++){
                                document.write("<tr>")
                                //document.write("<td align=\"middle\"><INPUT type=\"checkbox\" id=\"entry_enable_"+ i +"\" id=\"entry_enable_"+ i +"\" value=\"1\"></td>")
                                document.write("<td><input type=text class=flatL id=mac" + i + " name=mac" + i + " size=20 maxlength=17><input type=hidden id=name" + i + " name=name" + i + " maxlength=31></td>")
                                document.write("<td><input type=button id=copy" + i + " name=copy" + i + " value=<< class=btnForCopy onClick='copy_mac(" + i + ")'></td>");

                                document.write("<td width=155> <select class=wordstyle width=140 id=dhcp_list" + i + " name=dhcp_list" + i + " modified=\"ignore\">")
                                document.write("<option value=-1>"+bd_CName+"</option>")
                                document.write(set_mac_list("mac"))
                                document.write("</select></td>")
                                document.write("<td align=center>");
                                document.write("<input type=button id=clear" + i + " name=clear" + i + " value="+_clear + " onClick='clear_mac(" + i + ")'>");
                                document.write("</td>");
                                document.write("</tr>")
                            }
                          </script>
                      </tbody>
                    </table>
                  </div>
              </div></td></form>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table borderColor=#ffffff cellSpacing=0 borderColorDark=#ffffff
      cellPadding=2 bgColor=#ffffff borderColorLight=#ffffff border=0>
                  <tbody>
                    <tr>
                      <td id=help_text><b><script>show_words(_hints)</script>&hellip;</b>
                        <p><script>show_words(hham_intro)</script></p>
                        <p><script>show_words(hham_add)</script></p>
                        <p><script>show_words(hham_del)</script></p>
						<p><a href="support_adv.asp#MAC_Address_Filter" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
                      </td>
                    </tr>
                  </tbody>
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
