<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
var submit_button_flag = 0;
var USBSTYL_DataArray = new Array();
var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

	function onPageLoad()
	{
		if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
			DisableEnableForm(document.form1,true);
		}

		set_checked("<% CmoGetCfg("usb_storage_http_enable","none"); %>", get_by_id("storage_http_enable"));
	}

	function usb_data_set(linkname, devicename, totalspace, freespace, onList)
	{
		this.LinkName = linkname;
		this.DeviceName = devicename;
		this.TotalSpace = totalspace;
		this.FreeSpace = freespace;
		this.OnList = onList;
	}

	function set_usb_storge_list(){
	var index = 0;
	var temp_usb_storage_list = get_by_id("usb_storage_list").value.split(",");
	var delim = "*5#x"
	for (var i = 0; i < temp_usb_storage_list.length; i++){
		var temp_data = temp_usb_storage_list[i].split(delim);
		if(temp_data.length > 1){
			USBSTYL_DataArray[USBSTYL_DataArray.length++] = new usb_data_set(temp_data[0], temp_data[1], temp_data[2], temp_data[3], index);
			index++;
		}
	}
	get_by_id("usb_storage_num").innerHTML = USBSTYL_DataArray.length;

}

	function send_request(){

		var storage_http_enable = get_checked_value(get_by_id("storage_http_enable"));


		if ((storage_http_enable == get_by_id("usb_storage_http_enable").value) && !confirm(_ask_nochange)){
			return false;
		}
		get_by_id("usb_storage_http_enable").value = get_checked_value(get_by_id("storage_http_enable"));

		if(submit_button_flag == 0){
			submit_button_flag = 1;
			send_submit("form1");
		}

	}
</script>

<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=iso-8859-1">
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
        <td width="100%">&nbsp;&nbsp;Product Page: <a href="http://support.dlink.com.tw/" onclick="return jump_if();"><% CmoGetCfg("model_number","none"); %></a></td>
        <td align="right" nowrap>Hardware Version: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
        <td align="right" nowrap>Firmware Version: <% CmoGetStatus("version"); %></td>
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
										show_side_bar(5);
									</script>
								</ul>
							</div>
						</td>
					</tr>
				</table>
			</td>
			<form id="form1" name="form1" method="post" action="apply.cgi">
			<input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
			<input type="hidden" id="html_response_message" name="html_response_message" value="">
			<script>get_by_id("html_response_message").value = sc_intro_sv;</script>
			<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="usb_storage.asp">
			<input type="hidden" id="usb_storage_list" name="usb_storage_list" value="<% CmoGetList("usb_storage_list"); %>">

			<td valign="top" id="maincontent_container">
				<div id=maincontent>
                  <div id=box_header>
                   <h1><script>show_words(usb_storage)</script></h1>
                   <script>show_words(bwn_intro_usb)</script><br><br>
				   <script>show_words(usb_3g_help_support_help)</script>
				   <br><br>
                   <input name="button" id="button" class="button_submit" type="button" value="" onClick="send_request()">
                   <input name="button2" id="button2" class="button_submit" type="button" value="" onclick="page_cancel_1('form1', 'usb_setting.asp');">
                   <script>check_reboot();</script>
                   <script>get_by_id("button2").value = _dontsavesettings;</script>
                   <script>get_by_id("button").value = _savesettings;</script>
				  </div>
			<div class=box style="display:none">
				<h2><script>show_words(usb_storage_ftp)</script></h2>
			<table cellSpacing=1 cellPadding=1 width=525 border=0>
				<tr>
					<td width=155 align=right class="duple"><script>show_words(enable_usb_storage_ftp)</script>&nbsp;:</td>
					<td width="360">&nbsp;
					<input type=checkbox id="storage_ftp_enable" name="storage_ftp_enable" value="1">
					<input type="hidden" id="usb_storage_ftp_enable" name="usb_storage_ftp_enable" value="<% CmoGetCfg("usb_storage_ftp_enable","none"); %>">
					 </td>
				</tr>
			</table>
			</div>

			<div class=box>
				<h2><script>show_words(usb_storage_http)</script></h2>
			<table cellSpacing=1 cellPadding=1 width=525 border=0>
				<tr>
					<td width=155 align=right class="duple"><script>show_words(enable_usb_storage_http)</script>&nbsp;:</td>
					<td width="360">&nbsp;
					<input type=checkbox id="storage_http_enable" name="storage_http_enable" value="1">
					<input type="hidden" id="usb_storage_http_enable" name="usb_storage_http_enable" value="<% CmoGetCfg("usb_storage_http_enable","none"); %>">
					 </td>
				</tr>
			</table>
			</div>

			<div class=box id="usb_link_list">
	                  <h2><script>show_words(bd_title_usblinks)</script> <span id="usb_storage_num"></span></h2>
	                  <table id="table1" width="525" border=1 cellPadding=1 cellSpacing=1 bgcolor="#DFDFDF" bordercolor="#FFFFFF">
	                      <tr>
	                        <td><center><script>show_words(usb_link)</script></center></td>
	                        <td><center><script>show_words(usb_devic)</script></center></td>
	                        <td><center><script>show_words(usb_total_space)</script></center></td>
	                        <td><center><script>show_words(usb_free_space)</script></center></td>
	                        <td></td>
	                      </tr>
	                    <script>
							set_usb_storge_list();
							for(i=0;i<USBSTYL_DataArray.length;i++){
								document.write("<tr><td><center><a href='"+ USBSTYL_DataArray[i].LinkName +"' target='_blank'>"+ USBSTYL_DataArray[i].LinkName +"</a></center></td><td><center>"+ USBSTYL_DataArray[i].DeviceName +"</center></td><td><center>"+ USBSTYL_DataArray[i].TotalSpace +"</center></td><td><center>"+ USBSTYL_DataArray[i].FreeSpace +"</center></td><td><center></center></td></tr>");
							}
						</script>
	                  </table>
			</div>

			</form>
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
                    <tr>
                      <td id=help_text><strong><script>show_words(_hints)</script>&hellip;</strong>
                          <p><script>show_words(usb_network_support_help)</script> </p>
                          <p><script>show_words(usb_3g_help_support_help)</script></p>
                          <p style="display:none"><script>show_words(hhan_ping)</script></p>
                          <p style="display:none"><script>show_words(hhan_mc)</script></p>

						  <!--<p class="more"><a href="support_internet.asp#USB"><script>show_words(_more)</script>&hellip;</a></p>-->
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
