<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=iso-8859-1">
<style type="text/css">
<!--
#wps_enable {
	BORDER-RIGHT: #ff6f00 1px solid; PADDING-RIGHT: 10px; BORDER-TOP: #ff6f00 1px solid; PADDING-LEFT: 10px; PADDING-BOTTOM: 10px; BORDER-LEFT: #ff6f00 1px solid; PADDING-TOP: 0px; BORDER-BOTTOM: #ff6f00 1px solid; BACKGROUND-COLOR: #dfdfdf
}
#wps_disable {
	BORDER-RIGHT: #ff6f00 1px solid; PADDING-RIGHT: 10px; BORDER-TOP: #ff6f00 1px solid; PADDING-LEFT: 10px; PADDING-BOTTOM: 10px; BORDER-LEFT: #ff6f00 1px solid; PADDING-TOP: 0px; BORDER-BOTTOM: #ff6f00 1px solid; BACKGROUND-COLOR: #dfdfdf
}
-->
</style>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
	function onPageLoad(){
		var wps_status = "<% CmoGetStatus("wps_push_button_result"); %>";
    var next_value = "wps_back_fail.asp";
    if(wps_status != "1"){
    window.location.href = next_value;
    }
	}
</script>
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575" onLoad="onPageLoad();">
<table border=0 cellspacing=0 cellpadding=0 align=center width=838>
	<tr>
		<td>
		<form id="form1" name="form1" method="post">
		<input type="hidden" name="reboot_type" value="none">
			<table width=836 border=0 cellspacing=0 cellpadding=0 align=center height=100>
				<tr>
				  <td bgcolor="#FFFFFF">
				  <div align="center">
					<table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
					  <tr>
						<td width="100%">&nbsp;&nbsp;Product Page: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
						<td align="right" nowrap>Hardware Version: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
						<td align="right" nowrap>Firmware Version: <% CmoGetStatus("version"); %></td>
						<td>&nbsp;</td>
					  </tr>
					</table>
					<img src="wlan_masthead.gif" width="836" height="92" align="middle">
					  <br>
				  </div>
					<table width="650" border="0" align="center"><br>
						<tr>
						  <td>
						  <div id="wps_enable" class="box" >
							<h1 align="left">Step 2: Connect your Wireless Device</h1>
							<br>Adding wireless device: Succeeded. To add another device click on the Cancel button below or click on the Wireless Status button to check wireless status.</br>
							<P align="center">
								<input type="button" class="button_submit" name="btn_prev" value="Prev" disabled="disabled" >
                <input type="button" class="button_submit" name="btn_next" value="Next" disabled="disabled" onClick="send_request(); ">
								<input type="button" class="button_submit" name="btn_cancel" value="Cancel" onClick="window.location.href='wizard_wireless.asp'">
                <input type="button" class="button_submit" name="btn_wireless" value="Wireless Status" onClick="window.location.href='st_wireless.asp'">
								</P>
							</div>
						  </td>
						</tr>
					</table>
					 <p>&nbsp;</p>
				  </td>
				</tr>
				<tr>
				  <td bgcolor="#FFFFFF">
					  <table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="836" align="center">
						<tr>
						  <td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
						  <td width="10">&nbsp;</td>
						  <td>&nbsp;</td>
						  <td>&nbsp;</td>
						</tr>
					  </table>
				  </td>
				</tr>
			</table>
		</form>
		</td>
	</tr>
</table>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
</body>
</html>
