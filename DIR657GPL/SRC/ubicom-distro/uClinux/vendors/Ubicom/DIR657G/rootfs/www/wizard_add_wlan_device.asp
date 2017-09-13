<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
function onPageLoad(){
	var wlan0_security= "<% CmoGetCfg("wlan0_security","none"); %>";
		var security = wlan0_security.split("_");

		if(wlan0_security == "disable"){					//Disabled
			get_by_id("secu_mode").innerHTML = "none";
			get_by_id("wpa").style.display = "none";
			get_by_id("wep").style.display = "none";
		}else if(security[0] == "wep"){						//WEP
			get_by_id("wep").style.display = "";
			get_by_id("wpa").style.display = "none";
			var default_key = "<% CmoGetCfg("wlan0_wep_default_key","none"); %>";
			var key;
			if(security[2] == "64")
				key = "<% CmoGetCfg("wlan0_wep64_key_1","none"); %>";
			if(security[2] == "128")	
				key = "<% CmoGetCfg("wlan0_wep128_key_1","none"); %>";	
			get_by_id("show_wep_key").innerHTML = "WEP KEY " + default_key + " : " + key;
			if(security[1] == "open"){
			get_by_id("secu_mode").innerHTML = "WEP - Open";
			}else if(security[1] == "share"){
			get_by_id("secu_mode").innerHTML = "WEP - Shared Key";
			}
		}else if(security[0] == "wpa" || security[0] == "wpa2" || security[0] == "wpa2auto"){	//WPA
			get_by_id("wpa").style.display = "";
			get_by_id("wep").style.display = "none";
			if(security[0] == "wpa" && security[1] == "psk"){
				get_by_id("secu_mode").innerHTML = "WPA - PSK";
			}else if(security[0] == "wpa" && security[1] == "eap"){
			get_by_id("secu_mode").innerHTML = "WPA - EAP";
			}else if(security[0] == "wpa2" && security[1] == "psk"){
			get_by_id("secu_mode").innerHTML = "WPA2 - PSK";
			}else if(security[0] == "wpa2" && security[1] == "eap"){
			get_by_id("secu_mode").innerHTML = "WPA2 - EAP";
			}else if(security[0] == "wpa2auto" && security[1] == "psk"){
			get_by_id("secu_mode").innerHTML = "Auto (WPA or WPA2) - PSK";
			}else if(security[0] == "wpa2auto" && security[1] == "eap"){
			get_by_id("secu_mode").innerHTML = "Auto (WPA or WPA2) - EAP";
			}
		}
}
</script>
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style4 {font-size: 10px}
-->
</style>
</head>
<body onload="onPageLoad();" topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
<table border=0 cellspacing=0 cellpadding=0 align=center width=838>
<tr>
<td></td>
</tr>
<tr>
<td>
<div align=left>
<table width=838 border=0 cellspacing=0 cellpadding=0 align=center height=100>
<tr>
<td bgcolor="#FFFFFF"><div align=center>
  <table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
    <tr>
      <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
      <td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
      <td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
      <td>&nbsp;</td>
    </tr>
  </table>
  <div align="center"><img src="wlan_masthead.gif" width="836" height="92" align="middle"></div></td>
</tr>
</table>
</div>
</td>
</tr>
<tr>
  <td bgcolor="#FFFFFF"><p>&nbsp;</p>    <table width="650" border="0" align="center">
    <tr>
      <td>
      	<div id=box_header>
        	<h1 align="left"><script>show_words(wps_LW13)</script></h1>
        	<div align="left">
          		<br><script>show_words(TEXT048)</script></br>
          		<table align="center" class="formarea">
	            <tr>
	              <td>
	              	<script>show_words(help699)</script>: <strong><% CmoGetCfg("wlan0_ssid","none"); %></strong>
			        <br></br>
					<script>show_words(bws_SM)</script>: <strong><span id="secu_mode"></span><strong>
				<br><br>

					<div id="wpa" style="display:none">
					<script>show_words(bws_CT)</script>: <strong><font style="text-transform:uppercase;"><% CmoGetCfg("wlan0_psk_cipher_type","none"); %></font><strong>
				  <br>
			        <script>show_words(LW25)</script>: <strong><% CmoGetCfg("wlan0_psk_pass_phrase","none"); %><strong>
					<br><br>
					</div>
					<div id="wep" style="display:none">
					<span id="show_wep_key"></span>
					</div>
	              </td>
	            </tr>
	            <tr align="center">
	              <td>
					<input type="button" class="button_submit" id="OK_b" name="OK_b" value="" onclick="window.location.href='wizard_wireless.asp'">&nbsp;&nbsp;
					<script>get_by_id("OK_b").value = _ok;</script>
	              </td>
	            </tr>
	          </table>
        	</div>
        </div>
	  </td>
    </tr>
  </table>
    <p>&nbsp;</p></td>
</tr>
<tr>
  <td bgcolor="#FFFFFF"><table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="836" align="center">
    <tr>
      <td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
      <td width="10">&nbsp;</td>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
    </tr>
  </table></td>
</tr>
</table>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
</body>
</html>
