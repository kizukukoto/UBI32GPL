<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var submit_button_flag = 0;
    var rule_max_num = 40;
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

    function onPageLoad()
    {
        set_selectIndex("<% CmoGetCfg("url_domain_filter_type","none"); %>", get_by_id("url_domain_filter_type"));
        set_virtual_server();

        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1, true);
        }

        set_form_default_values("form1");
    }

    function set_virtual_server()
    {
        for (var i = 0; i < rule_max_num; i++) {
            var temp_mf;

            if (i > 9)
                temp_mf = get_by_id("url_domain_filter_" + i).value;
            else
                temp_mf = get_by_id("url_domain_filter_0" + i).value;

            if (temp_mf.length > 1) {
                get_by_id("url_" + i).value = temp_mf;
            }
        }
    }

    function send_request()
    {
        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }
        for (var i = 0; i < rule_max_num; i++) {
        	get_by_id("url_" + i).value = get_by_id("url_" + i).value.toLowerCase();
            var temp_url = get_by_id("url_" + i).value;
            if (temp_url != "") {
                if(temp_url.indexOf("http://") != -1){
                    temp_url = temp_url.substring("http://".length);
                }
                if(temp_url.lastIndexOf("/")  != -1){
                    temp_url = temp_url.substring(0, temp_url.lastIndexOf("/"));
                }
                //alert(temp_url);
                for (var j = i+1; j < rule_max_num; j++) {
                    hid_temp_url = get_by_id("url_" + j).value;
                    if (hid_temp_url != ""){
                        if(hid_temp_url.indexOf("http://") != -1){
                            hid_temp_url = hid_temp_url.substring("http://".length);
                        }
                        if(hid_temp_url.lastIndexOf("/")  != -1){
                            hid_temp_url = hid_temp_url.substring(0, hid_temp_url.lastIndexOf("/"));
                        }
                    }
                    if (temp_url == hid_temp_url){
                        alert("The web site address" +" "+ get_by_id("url_" + j).value +" "+ "already used.");
                        return false;
                    }
					
                }
        }
		
		}		

        var count = 0;
        for (var i = 0; i < rule_max_num; i++) {
            if (i > 9)
                get_by_id("url_domain_filter_" + i).value = "";
            else
                get_by_id("url_domain_filter_0" + i).value = "";

            if (get_by_id("url_" + i).value != "") {
                /* check http:// ,https:// and / */
                var tmp_url = get_by_id("url_" + i).value;

                //Invalid url
                if (tmp_url.length == 1) {
                    var re = /^\d$/;
                    if (re.test(tmp_url)) {
                        alert("The web site address '" + tmp_url +"' is an invalid.");
                        return false;
                    }
                }

                var httppos = tmp_url.indexOf("http");
                var httpspos = tmp_url.indexOf("https");
                var httpcpos = tmp_url.indexOf("http://");
                var httpscpos = tmp_url.indexOf("https://");
                var lpos = tmp_url.lastIndexOf("/");
                var strGet_url;

                // Invalid https
                if ((httpspos != -1) && (httpscpos == -1)) {
                    alert("The web site address '" + tmp_url +"' is an invalid.");
                    return false;
                }

                // Invalid http
                if ((httppos != -1) && (httpcpos == -1)) {
                    if (httpspos == -1) {
                        alert("The web site address '" + tmp_url +"' is an invalid.");
                        return false;
                    }
                }

                if (httpcpos != -1) {
                    if (lpos < 7)
                        strGet_url = tmp_url.substring(httpcpos + 7);
                    else
                        strGet_url = tmp_url.substring(httpcpos + 7, lpos);
                }
                else if (httpscpos != -1) {
                    if (lpos < 7)
                        strGet_url = tmp_url.substring(httpscpos + 7);
                    else
                        strGet_url = tmp_url.substring(httpscpos + 7, lpos);
                }
                else {
                    if (lpos != -1)
                        strGet_url = tmp_url.substring(0, lpos);
                    else
                        strGet_url = tmp_url;
                }

                var kk = i;
                if (count < 10) {
                    kk = "0"+ count;
                }

                for (var j = 0; j < i; j++) {
                    if (strGet_url == get_by_id("url_" + j).value) {
                        alert(msg[DUPLICATE_URL_ERROR]);
                        return false;
                    }
                }
                get_by_id("url_domain_filter_" + kk).value = strGet_url;
                count++;
            }
        }

        if (submit_button_flag == 0) {
            submit_button_flag = 1;
            return true;
        }

        return false;
    }

    function check_date()
    {
        var is_change = false;
        var check_domain_type = "<% CmoGetCfg("url_domain_filter_type","none"); %>";

        if (get_by_id("url_domain_filter_type").value != check_domain_type) {
            is_change = true;
        }
        else if (!is_change) {
            for (i=0;i<rule_max_num;i++) {
                var kk = i;
                if (i < 10) {
                    kk = "0"+i;
                }

                if (get_by_id("url_domain_filter_"+ kk).value != get_by_id("url_"+ i).value) {
                    is_change = true;
                    break;
                }
            }
        }

        if (is_change) {
            if (!confirm(msg[IS_CHANGE_DATA])) {
                return false;
            }
        }

        location.href = "tools_schedules.asp";
    }

    function clear_list_URL()
    {
        for (var i = 0; i < rule_max_num; i++) {
            get_by_id("url_"+ i).value = "";
        }
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
        <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"onclick="return jump_if();"><% CmoGetCfg("model_number","none"); %></a></td>
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
                            <ul>
                            <LI><div><a href="adv_virtual.asp" onClick="return jump_if();"><script>show_words(_virtserv)</script></a></div></LI>
                            <LI><div><a href="adv_portforward.asp" onclick="return jump_if();"><script>show_words(_pf)</script></a></div></LI>
                            <LI><div><a href="adv_appl.asp" onclick="return jump_if();"><script>show_words(_specappsr)</script></a></div></LI>
                            <LI><div><a href="adv_qos.asp" onclick="return jump_if();"><script>show_words(YM48)</script></a></div></LI>
                            <LI><div><a href="adv_filters_mac.asp" onclick="return jump_if();"><script>show_words(_netfilt)</script></a></div></LI>
                            <LI><div><a href="adv_access_control.asp" onclick="return jump_if();"><script>show_words(_acccon)</script></a></div></LI>
                            <LI><div id=sidenavoff><script>show_words(_websfilter)</script></div></LI>
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

                </ul>
                        </div>
                    </td>
                </tr>
                </table></td>

                <form id="form1" name="form1" method="post" action="apply.cgi">
                <input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
                <input type="hidden" id="html_response_message" name="html_response_message" value="">
                <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
                <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="adv_filters_url.asp">
                <input type="hidden" id="reboot_type" name="reboot_type" value="filter">

                <input type="hidden" id="url_domain_filter_00" name="url_domain_filter_00" value="<% CmoGetCfg("url_domain_filter_00","none"); %>">
                <input type="hidden" id="url_domain_filter_01" name="url_domain_filter_01" value="<% CmoGetCfg("url_domain_filter_01","none"); %>">
                <input type="hidden" id="url_domain_filter_02" name="url_domain_filter_02" value="<% CmoGetCfg("url_domain_filter_02","none"); %>">
                <input type="hidden" id="url_domain_filter_03" name="url_domain_filter_03" value="<% CmoGetCfg("url_domain_filter_03","none"); %>">
                <input type="hidden" id="url_domain_filter_04" name="url_domain_filter_04" value="<% CmoGetCfg("url_domain_filter_04","none"); %>">
                <input type="hidden" id="url_domain_filter_05" name="url_domain_filter_05" value="<% CmoGetCfg("url_domain_filter_05","none"); %>">
                <input type="hidden" id="url_domain_filter_06" name="url_domain_filter_06" value="<% CmoGetCfg("url_domain_filter_06","none"); %>">
                <input type="hidden" id="url_domain_filter_07" name="url_domain_filter_07" value="<% CmoGetCfg("url_domain_filter_07","none"); %>">
                <input type="hidden" id="url_domain_filter_08" name="url_domain_filter_08" value="<% CmoGetCfg("url_domain_filter_08","none"); %>">
                <input type="hidden" id="url_domain_filter_09" name="url_domain_filter_09" value="<% CmoGetCfg("url_domain_filter_09","none"); %>">
                <input type="hidden" id="url_domain_filter_10" name="url_domain_filter_10" value="<% CmoGetCfg("url_domain_filter_10","none"); %>">
                <input type="hidden" id="url_domain_filter_11" name="url_domain_filter_11" value="<% CmoGetCfg("url_domain_filter_11","none"); %>">
                <input type="hidden" id="url_domain_filter_12" name="url_domain_filter_12" value="<% CmoGetCfg("url_domain_filter_12","none"); %>">
                <input type="hidden" id="url_domain_filter_13" name="url_domain_filter_13" value="<% CmoGetCfg("url_domain_filter_13","none"); %>">
                <input type="hidden" id="url_domain_filter_14" name="url_domain_filter_14" value="<% CmoGetCfg("url_domain_filter_14","none"); %>">
                <input type="hidden" id="url_domain_filter_15" name="url_domain_filter_15" value="<% CmoGetCfg("url_domain_filter_15","none"); %>">
                <input type="hidden" id="url_domain_filter_16" name="url_domain_filter_16" value="<% CmoGetCfg("url_domain_filter_16","none"); %>">
                <input type="hidden" id="url_domain_filter_17" name="url_domain_filter_17" value="<% CmoGetCfg("url_domain_filter_17","none"); %>">
                <input type="hidden" id="url_domain_filter_18" name="url_domain_filter_18" value="<% CmoGetCfg("url_domain_filter_18","none"); %>">
                <input type="hidden" id="url_domain_filter_19" name="url_domain_filter_19" value="<% CmoGetCfg("url_domain_filter_19","none"); %>">
                <input type="hidden" id="url_domain_filter_20" name="url_domain_filter_20" value="<% CmoGetCfg("url_domain_filter_20","none"); %>">
                <input type="hidden" id="url_domain_filter_21" name="url_domain_filter_21" value="<% CmoGetCfg("url_domain_filter_21","none"); %>">
                <input type="hidden" id="url_domain_filter_22" name="url_domain_filter_22" value="<% CmoGetCfg("url_domain_filter_22","none"); %>">
                <input type="hidden" id="url_domain_filter_23" name="url_domain_filter_23" value="<% CmoGetCfg("url_domain_filter_23","none"); %>">
                <input type="hidden" id="url_domain_filter_24" name="url_domain_filter_24" value="<% CmoGetCfg("url_domain_filter_24","none"); %>">
                <input type="hidden" id="url_domain_filter_25" name="url_domain_filter_25" value="<% CmoGetCfg("url_domain_filter_25","none"); %>">
                <input type="hidden" id="url_domain_filter_26" name="url_domain_filter_26" value="<% CmoGetCfg("url_domain_filter_26","none"); %>">
                <input type="hidden" id="url_domain_filter_27" name="url_domain_filter_27" value="<% CmoGetCfg("url_domain_filter_27","none"); %>">
                <input type="hidden" id="url_domain_filter_28" name="url_domain_filter_28" value="<% CmoGetCfg("url_domain_filter_28","none"); %>">
                <input type="hidden" id="url_domain_filter_29" name="url_domain_filter_29" value="<% CmoGetCfg("url_domain_filter_29","none"); %>">
                <input type="hidden" id="url_domain_filter_30" name="url_domain_filter_30" value="<% CmoGetCfg("url_domain_filter_30","none"); %>">
                <input type="hidden" id="url_domain_filter_31" name="url_domain_filter_31" value="<% CmoGetCfg("url_domain_filter_31","none"); %>">
                <input type="hidden" id="url_domain_filter_32" name="url_domain_filter_32" value="<% CmoGetCfg("url_domain_filter_32","none"); %>">
                <input type="hidden" id="url_domain_filter_33" name="url_domain_filter_33" value="<% CmoGetCfg("url_domain_filter_33","none"); %>">
                <input type="hidden" id="url_domain_filter_34" name="url_domain_filter_34" value="<% CmoGetCfg("url_domain_filter_34","none"); %>">
                <input type="hidden" id="url_domain_filter_35" name="url_domain_filter_35" value="<% CmoGetCfg("url_domain_filter_35","none"); %>">
                <input type="hidden" id="url_domain_filter_36" name="url_domain_filter_36" value="<% CmoGetCfg("url_domain_filter_36","none"); %>">
                <input type="hidden" id="url_domain_filter_37" name="url_domain_filter_37" value="<% CmoGetCfg("url_domain_filter_37","none"); %>">
                <input type="hidden" id="url_domain_filter_38" name="url_domain_filter_38" value="<% CmoGetCfg("url_domain_filter_38","none"); %>">
                <input type="hidden" id="url_domain_filter_39" name="url_domain_filter_39" value="<% CmoGetCfg("url_domain_filter_39","none"); %>">

            <td valign="top" id="maincontent_container">
                <div id="maincontent">
                  <div id=box_header>
                <h1><script>show_words(_websfilter)</script></h1>
                <script>show_words(dlink_awf_intro_WF)</script>
                <br><br>
				<input name="apply" id="apply" type="submit" class=button_submit value="" onClick="return send_request()">
				<input name="cancel" id="cancel" type=button class=button_submit value="" onclick="page_cancel('form1', 'adv_filters_url.asp');">
				<script>check_reboot();</script>
				<script>get_by_id("apply").value = _savesettings;</script>
				<script>get_by_id("cancel").value = _dontsavesettings;</script>

                  </div>
                  <div class=box>
            <h2>40 &ndash;
              <script>show_words(awf_title_WSFR)</script>
            </h2>
                    <table cellSpacing=1 cellPadding=2 width=500 border=0>
                        <tr>

                <td>
                  <script>show_words(dlink_wf_intro)</script>
                </td>
                        </tr>
                        <tr>
                          <td>
                          <select id="url_domain_filter_type" name="url_domain_filter_type">
                              <!--option value="disable">Turn Parental Control OFF</option-->
                              <option value="list_deny"><script>show_words(dlink_wf_op_0)</script></option>
                              <option value="list_allow"><script>show_words(dlink_wf_op_1)</script></option>
                            </select>
                          </td>
                        </tr>
                        <tr>
                          <td>
                            <br>
                            <script>document.write('<input type="button" class=button_submit value="'+awf_clearlist+'" onclick="clear_list_URL()">')</script> 
                          </td>
                        </tr>
                    </table>
                    <br>
                    <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=0>
                        <tr align=center>
                          <td width="100%" colspan="2"><b><script>show_words(aa_WebSite_Domain)</script></b></td>
                        </tr>
                        <tr align=center>
                          <td colspan="2"></td>
                        </tr>
                        <script>
                        for(var i=0 ; i<rule_max_num ; i++){
                            document.write("<tr align=center>")
                            document.write("<td><input id=\"url_" + i + "\" name=\"url_" + i + "\" maxlength=40 size=41></td>")
                            document.write("<td><input id=\"url_" + ++i + "\" name=\"url_" + i + "\" maxlength=40 size=41></td>")
                            document.write("</tr>")
                        }
                      </script>
                    </table>
                  </div>
              </div>
            </td>
        </form>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table width="125" border=0 cellPadding=2 cellSpacing=0 borderColor=#ffffff borderColorLight=#ffffff borderColorDark=#ffffff bgColor=#ffffff>
                    <tr>
          <td id=help_text><strong>
           <script>show_words(_hints)</script>&hellip;</strong>
         <p><script>show_words(dlink_hhwf_intro)</script></p>
         <p><script>show_words(hhwf_xref)</script></p>
         <p class="more"><a href="support_adv.asp#Web_Filter" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
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
