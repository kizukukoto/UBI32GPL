<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var submit_button_flag = 0;
	var my_opendns_mode = 1 * "<% CmoGetCfg("opendns_mode","none"); %>";
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

    function onPageLoad()
    {
		// get device id
		var myurl =get_by_id("opendns_url").href;

		var lan_ip = "<% CmoGetCfg("lan_ipaddr","none"); %>"
		var lan_mac = "<% CmoGetCfg("lan_mac","none"); %>"
		lan_mac = lan_mac.replace(/:/g,"");


		var par_url;

		par_url = 'http://' + lan_ip + '/parentalcontrols';

		par_url += '/<% CmoGetStatus("opendns_token"); %>'

		myurl = 'https://www.opendns.com/device/register?vendor=dlink';
		myurl += '&url=' + encodeURIComponent(par_url);
		myurl += '&desc=' + encodeURIComponent("<% CmoGetCfg("pure_model_description","none"); %>"); //DIR-655%20Xtreme%20N%E2%84%A2%20Gigabit%20Router';
		myurl += '&mac=' + lan_mac;
		myurl += '&model=' + encodeURIComponent("<% CmoGetCfg("model_number","none"); %>");

// style ?
		myurl += '&style=WebUI_1_0';

		get_by_id("opendns_url").href = myurl;

		var deviceid = "<% CmoGetCfg("opendns_deviceid","none"); %>";
		var dashboard = get_by_id("opendns_dashboard").href;

		dashboard = "https://www.opendns.com/device/settings?";

		dashboard += "deviceid=" + deviceid;
//		dashboard += '&url=' + encodeURIComponent(par_url);

		get_by_id("opendns_dashboard").href = dashboard;
		get_by_id("opendns_dashboard2").href = dashboard;


		if( deviceid=="") {

//        	document.getElementById("register_device").style.display = "block";
        	document.getElementById("dashboard_option").style.display = "none";
		}
		else {

        	document.getElementById("register_device").style.display = "none";
//        	document.getElementById("dashboard_option").style.display = "block";
		}




		set_checked("<% CmoGetCfg("opendns_mode","none"); %>", get_by_name("opendns_mode"));
		//opendns_mode_selector("<% CmoGetCfg("opendns_mode","none"); %>");


/*
        var show_selected = get_by_id("usb_type").value;
        set_selectIndex(show_selected, get_by_id("usb"));
        set_checked("<% CmoGetCfg("netusb_guest_zone","none"); %>", get_by_id("shareport"));

        if (get_by_id("usb").value == "0")
            get_by_id("box_shareport_guest_field").style.display = "";
        else
            get_by_id("box_shareport_guest_field").style.display = "none";

        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1, true);
        }
*/
        set_form_default_values("form1");
    }


    function opendns_mode_selector(value)
    {
    	value = value * 1;
        if (value == 4) {
            get_by_id("wan_specify_dns").value ="1";
            get_by_id("wan_primary_dns").value ="204.194.232.200";
            get_by_id("wan_secondary_dns").value="204.194.234.200";
            get_by_id("wan_primary_dns").disabled = true;
            get_by_id("wan_secondary_dns").disabled = true;
        }
        else if (value == 3) {
            get_by_id("wan_specify_dns").value ="1";
            get_by_id("wan_primary_dns").value ="208.67.222.123";
            get_by_id("wan_secondary_dns").value="208.67.220.123";
            get_by_id("wan_primary_dns").disabled = true;
            get_by_id("wan_secondary_dns").disabled = true;
        }
        else if (value == 2) {
        	// Parental Controls
            get_by_id("wan_specify_dns").value ="1";
            get_by_id("wan_primary_dns").value ="208.67.222.222";
            get_by_id("wan_secondary_dns").value="208.67.220.220";
            get_by_id("wan_primary_dns").disabled = true;
            get_by_id("wan_secondary_dns").disabled = true;
        }
        else if (value == 1) {
            get_by_id("wan_specify_dns").value ="0";
            get_by_id("wan_primary_dns").disabled = false;
            get_by_id("wan_secondary_dns").disabled = false;
            get_by_id("wan_primary_dns").value = "0.0.0.0";
            get_by_id("wan_secondary_dns").value =  "0.0.0.0";

// original setting
//            get_by_id("wan_specify_dns").value   = "<% CmoGetCfg("wan_specify_dns","none"); %>";
//            get_by_id("wan_primary_dns").value   = "<% CmoGetCfg("wan_primary_dns","none"); %>";
//            get_by_id("wan_secondary_dns").value = "<% CmoGetCfg("wan_secondary_dns","none"); %>";

        }
        else {
            get_by_id("wan_specify_dns").value ="0";
            get_by_id("wan_primary_dns").disabled = false;
            get_by_id("wan_secondary_dns").disabled = false;
//           get_by_id("wan_primary_dns").value = "0.0.0.0";
//           get_by_id("wan_secondary_dns").value =  "0.0.0.0";

// original setting
//            get_by_id("wan_specify_dns").value   = "<% CmoGetCfg("wan_specify_dns","none"); %>";
//            get_by_id("wan_primary_dns").value   = "<% CmoGetCfg("wan_primary_dns","none"); %>";
//            get_by_id("wan_secondary_dns").value = "<% CmoGetCfg("wan_secondary_dns","none"); %>";

        }

    	// enable opendns_enable
    	if( value >= 2) {
	    	get_by_id("opendns_enable").value = 1;
		}
		else {
	    	get_by_id("opendns_enable").value = 0;
		}
        set_checked(get_by_id("opendns_enable").value, get_by_id("opendns_enable_sel"));

		my_opendns_mode = value * 1;
    }




    function send_request()
    {

// not disable, disable will not send value to server
        get_by_id("wan_primary_dns").disabled = false;
        get_by_id("wan_secondary_dns").disabled = false;

//    	alert(get_by_id("wan_primary_dns").value);




		var deviceid = "<% CmoGetCfg("opendns_deviceid","none"); %>";

		if( (deviceid=="") && (my_opendns_mode == 2) ) {
			alert(Tag05776);

			window.open(get_by_id("opendns_url").href);

			//return false;
		}

		if( (deviceid!="") && (my_opendns_mode == 2) ) {
			// update dns1/dns2
            get_by_id("wan_primary_dns").value = "<% CmoGetCfg("opendns_dns1","none"); %>";
            get_by_id("wan_secondary_dns").value= "<% CmoGetCfg("opendns_dns2","none"); %>";

		}


//		return false;

		send_submit("form1");
		return true;


    	return false;

/*
        var mode = get_by_id("wanmode").value;
        var usb_type = get_by_id("usb").value;
        var share_value = get_checked_value(get_by_id("shareport"));

        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }

        if ((usb_type == get_by_id("usb_type").value) && (share_value == get_by_id("netusb_guest_zone").value)) {
            return false;
        }

        if (mode == "usb3g" && usb_type == "0") {
            get_by_id("asp_temp_72").value = "0"; //usb_type = Network USB
            send_submit("form2");
            return false;
        }
        else if (usb_type == "2") {
            get_by_id("asp_temp_72").value = "2"; //usb_type = 3G USB
            send_submit("form2");
            return false;
        }
        else if (mode == "usb3g" && usb_type == "1") {
            get_by_id("asp_temp_72").value = "1"; //usb_type = 3G USB
            send_submit("form2");
            return false;
        }
        else {
            get_by_id("asp_temp_72").value= get_by_id("usb").value;
            send_submit("form2");
        }
        get_by_id("usb_type").value = get_by_id("usb").value;
        get_by_id("netusb_guest_zone").value = get_checked_value(get_by_id("shareport"));

        if (submit_button_flag == 0) {
            submit_button_flag = 1;
            return true;
        }
        return false;

*/
		send_submit("form1");
		return false;
    }

    function usb_change_type()
    {
        var usb_type = get_by_id("usb").value;
        if (usb_type == "0")
            get_by_id("box_shareport_guest_field").style.display = "";
        else
            get_by_id("box_shareport_guest_field").style.display = "none";
    }
</script>

<title><% CmoGetStatus("title"); %></title>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
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
																		show_side_bar(7);
																	</script>
                                </ul>
                            </div>
                        </td>
                    </tr>
                </table>
            </td>
            <form id="form2" name="form2" method="post" action="apply.cgi">
            <input type="hidden" id="html_response_page" name="html_response_page" value="dns_setting.asp">
            <input type="hidden" id="asp_temp_72" name="asp_temp_72" value="<% CmoGetCfg("asp_temp_72","none"); %>">
            <input type="hidden" id="reboot_type" name="reboot_type" value="none">
            </form>

            <form id="form1" name="form1" method="post" action="apply.cgi">
            <input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
            <input type="hidden" id="html_response_message" name="html_response_message" value="The setting is saved.">
            <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="dns_setting.asp">

<!-- new add -->
	            <input type="hidden" id="wan_specify_dns" name="wan_specify_dns" value="<% CmoGetCfg("wan_specify_dns","none"); %>">
                <input type="hidden" id="opendns_enable" name="opendns_enable" value="<% CmoGetCfg("opendns_enable","none"); %>">
                <input type="hidden" id="dns_relay" name="dns_relay" value="1">

                <input type="hidden" id="wan_primary_dns" name="wan_primary_dns" value="<% CmoGetCfg("wan_primary_dns","none"); %>">
                <input type="hidden" id="wan_secondary_dns" name="wan_secondary_dns" value="<% CmoGetCfg("wan_secondary_dns","none"); %>">

                <input type="hidden" id="reboot_type" name="reboot_type" value="wan">

            <td valign="top" id="maincontent_container">
                <div id="maincontent">
                  <div id=box_header>
                    <h1><script>show_words(Tag05775)</script></h1>
                    <script>show_words(Tag05777)</script>
                    <br><br>
                  <input name="button" id="button" class="button_submit" type="submit" onClick="return send_request()">
                  <input name="button2" id="button2" class="button_submit" type="button" onclick="page_cancel('form1', 'dns_setting.asp');">
                  <script>check_reboot();</script>
                  <script>get_by_id("button2").value = _dontsavesettings;</script>
                  <script>get_by_id("button").value = _savesettings;</script>
                  </div>
                  <br>




              <div class=box>
                <h2><script>show_words(Tag05778)</script></h2>
                <p style="display:none " class="box_msg">Advanced DNS is a free security option that provides Anti-Phishing to protect your Internet connection from fraud and navigation improvements such as auto-correction of common URL types.</p>
                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                <tr style="display:none ">
                <td width="185" align=right style="WIDTH: 190px;" class="duple">Enable Advanced DNS Service :</td>
                <td width="331">&nbsp;<input type="checkbox" id="opendns_enable_sel" name="opendns_enable_sel" value="1" onclick="opendns_enable_selector(this.checked);"></td>
                </tr>
</table><table cellSpacing=1 cellPadding=1 width=525 border=0>
                <tr>
				<td width="30" >
			    <input type="radio" align=left id="opendns_mode" name="opendns_mode" value="4" onclick="opendns_mode_selector(this.value);">
				</td>
				<td width="480" >
				<b>
					<script>show_words(Tag05779)</script>
				</b>
<td><tr><td></td><td>
<script>show_words(Tag05780)</script>
            	</td>
                </tr>

                <tr>
				<td width="30" >
			    <input type="radio" align=left id="opendns_mode" name="opendns_mode" value="3" onclick="opendns_mode_selector(this.value);">
				</td>
				<td width="480" >
				<b>
				<script>show_words(Tag05781)</script>
				</b>
<td><tr><td></td><td>

<script>show_words(Tag05782)</script>

            	</td>
                </tr>
                <tr>
				<td width="30" >
			    <input type="radio" align=left id="opendns_mode" name="opendns_mode" value="2" onclick="opendns_mode_selector(this.value);">
				</td>
				<td width="480" >
				<b>
				<script>show_words(Tag05783)</script>
				</b>
<td><tr><td></td><td>
<script>show_words(Tag05784)</script>
				<br>
				<br>
				<a id="opendns_dashboard2" target="_blank" href=""><script>show_words(Tag05785)</script></a> <script>show_words(Tag05786)</script> <a target="_blank" href="http://www.opendns.com">www.opendns.com</a>
				<br>
				<br>
				</td>
                </tr>


                    <tr  style="display:none ">
                      <td></td>
                      <td>
                      Device Id:
                      &nbsp;
                          <input type=text id="opendns_deviceid" name="opendns_deviceid" size="16" maxlength="16" value="<% CmoGetCfg("opendns_deviceid","none"); %>">
                      </td>
                    </tr>

                    <tr style="display:none " id="register_device" name="register_device" >
                      <td></td>
                      <td>

                      &nbsp;
                      <a id="opendns_url" name="opendns_url"  target="_blank" href="" onclick="return jump_if();">Register your device</a>
                      </td>
                    </tr>


                    <tr style="display:none " id="dashboard_option" name="dashboard_option">
                      <td></td>
                      <td>

                      &nbsp;
                      <a id="opendns_dashboard" target="_blank" name="opendns_dashboard" href="" onclick="return jump_if();">Management your device </a>
                      </td>
                    </tr>


                <tr>
				<td width="30" >
			    <input type="radio" align=left id="opendns_mode" name="opendns_mode" value="1" onclick="opendns_mode_selector(this.value);">
				</td>
				<td width="480" >
				<b>

				<script>show_words(Tag05787)</script>
				</b>
<td><tr><td></td><td>
<script>show_words(Tag05788)</script>

            	</td>
                </tr>
                <tr style="display:none">
				<td width="30" >
			    <input type="radio" align=left id="opendns_mode" name="opendns_mode" value="0" onclick="opendns_mode_selector(this.value);">
				</td>
				<td width="480" >
				<b>

				Manually Configure DNS Servers
				</b>
<td><tr style="display:none"><td></td><td>
Users should be allowed to specify their own preferred DNS servers
            	</td>



                </tr>
                </table>
                </div>



              </div></td>
            </form>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
                    <tr>
                      <td id=help_text><strong style="display:none"><script>show_words(_hints)</script>&hellip;</strong>
                          <p style="display:none"><script>show_words(usb_network_support_help)</script> </p>
                          <p style="display:none"><script>show_words(usb_3g_help_support_help)</script></p>

                          <p><script>show_words(Tag05789)</script>
                          </p>
                          <p> <script>show_words(Tag05790)</script>
                          </p>



                          <p style="display:none"><script>show_words(hhan_ping)</script></p>
                          <p style="display:none"><script>show_words(hhan_mc)</script></p>

                          <p style="display:none" class="more"><a href="support_internet.asp#USB"><script>show_words(_more)</script>&hellip;</a></p>
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
