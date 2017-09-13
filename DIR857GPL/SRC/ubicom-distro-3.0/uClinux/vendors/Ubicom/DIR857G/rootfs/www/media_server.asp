<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>

<script language="JavaScript">
    var submit_button_flag = 0;
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

	function onPageLoad()
	{
		set_checked("<% CmoGetCfg("media_server_enable","none"); %>", get_by_id("storage_media_enable"));

		if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
			DisableEnableForm(document.form1,true);
		}

		set_form_default_values("form1");
	}

	function send_request()
	{
		var storage_media_enable = get_checked_value(get_by_id("storage_media_enable"));
		var media_server_name = get_by_id("media_server_name").value;

		if ((storage_media_enable == get_by_id("media_server_enable").value) && (media_server_name == "<% CmoGetCfg("media_server_name","none"); %>") && !confirm(_ask_nochange)){
			return false;
		}

		get_by_id("media_server_enable").value = get_checked_value(get_by_id("storage_media_enable"));

		//Check Server Name cannot entry  ~!@#$%^&*()_+}{[]\|"?></
		var re = /[^A-Za-z0-9_.:\-]/;
		if(re.test(media_server_name)){
			alert(GW_MEDIA_SERVER_NAME_INVALID);
			return false;
		}

		if(submit_button_flag == 0){
			submit_button_flag = 1;
			send_submit("form1");
		}
	}
</script>

<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=iso-8859-1">
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
										show_side_bar(4);
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
			<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="media_server.asp">
			<input type="hidden" id="reboot_type" name="reboot_type" value="shutdown">

			<td valign="top" id="maincontent_container">
				<div id=maincontent>
                  <div id=box_header>
                    <h1><script>show_words(_media_server)</script></h1>
                    <script>show_words(_media_server_description_1)</script><br><br>
				    				<script>show_words(_media_server_description_2)</script><br><br>
                    <input name="button" id="button" class="button_submit" type="button" value="" onClick="send_request()">
                    <input name="button2" id="button2" class="button_submit" type="button" value="" onclick="page_cancel_1('form1', 'media_server.asp');">
                    <script>check_reboot();</script>
                    <script>get_by_id("button2").value = _dontsavesettings;</script>
                    <script>get_by_id("button").value = _savesettings;</script>
				  </div>
				  <br>
			<div class=box>
				<h2><script>show_words(_media_server)</script></h2>

			<table cellSpacing=1 cellPadding=1 width=525 border=0>
				<tr>
					<td width=155 align=right class="duple"><script>show_words(_enable_media_server)</script>&nbsp;:</td>
					<td width="360">&nbsp;
					<input type=checkbox id="storage_media_enable" name="storage_media_enable" value="1">
					<input type="hidden" id="media_server_enable" name="media_server_enable" value="<% CmoGetCfg("media_server_enable","none"); %>">

					</td>
				</tr>
				<tr>
                	<td width=155 align=right class="duple"><script>show_words(_media_server_name)</script>&nbsp;:</td>
                	<td width="360">&nbsp;
                	<input type="text" id="media_server_name" name="media_server_name"  size="40" maxlength="30" value="<% CmoGetCfg("media_server_name","none"); %>">
                	</td>
				</tr>
			</table>

			</div>

			</form>
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
                    <tr>
                      <td id=help_text><strong><script>show_words(_hints)</script>&hellip;</strong>
                          <!--<p><script>show_words(usb_network_support_help)</script> </p>
                          <p><script>show_words(usb_3g_help_support_help)</script></p>-->
						  <p class="more"><a href="support_internet.asp#USB"><script>show_words(_more)</script>&hellip;</a></p>
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
