<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
function onPageLoad(){
	var wlan0_security= "<% CmoGetCfg("wlan0_security","none"); %>";
	var wlan1_security= "<% CmoGetCfg("wlan1_security","none"); %>";
	var wlan0_vap1_security= "<% CmoGetCfg("wlan0_vap1_security","none"); %>";
	var wlan1_vap1_security= "<% CmoGetCfg("wlan1_vap1_security","none"); %>";	
	var security0 = wlan0_security.split("_");
	var security1 = wlan1_security.split("_");
	var vap1_security0 = wlan0_vap1_security.split("_");
	var vap1_security1 = wlan1_vap1_security.split("_");
	//2.4G wireless	
		if(wlan0_security == "disable"){					//Disabled
			get_by_id("secu_mode_0").innerHTML = "none";
			get_by_id("wpa_0").style.display = "none";
			get_by_id("wep_0").style.display = "none";
		}else if(security0[0] == "wep"){						//WEP
			get_by_id("wep_0").style.display = "";
			get_by_id("wpa_0").style.display = "none";
			var wep_key64 = "<% CmoGetCfg("wlan0_wep64_key_1","none"); %>";
			var wep_key128 = "<% CmoGetCfg("wlan0_wep128_key_1","none"); %>";
			var key;
			var type = "<% CmoGetCfg("wlan0_wep_display","none"); %>";
			var default_key = "<% CmoGetCfg("wlan0_wep_default_key","none"); %>";
			/*
			if(security0[2] == "64")
				var keyx = "<% CmoGetCfg("wlan0_wep64_key_1","ascii"); %>";
				var keyy = "<% CmoGetCfg("wlan0_wep64_key_1","hex"); %>";	
				key = "<% CmoGetCfg("wlan0_wep64_key_1",\"type\"); %>";
			if(security0[2] == "128")
				key = "<% CmoGetCfg("wlan0_wep128_key_1",type); %>";
			*/
				
			if(security0[2] == "64"){	
			key = (type == "ascii" ? hex_to_a(wep_key64) : wep_key64);
			} else {
				key = (type == "ascii" ? hex_to_a(wep_key128) : wep_key128);
			}		
	
			get_by_id("show_wep_key_0").innerHTML = "WEP KEY " + default_key + " : <strong>" + key + "</strong>";
			if(security0[1] == "auto"){
			get_by_id("secu_mode_0").innerHTML = "WEP - Both";
			}else if(security0[1] == "share"){
			get_by_id("secu_mode_0").innerHTML = "WEP - Shared Key";
			}
		}else if(security0[0] == "wpa" || security0[0] == "wpa2" || security0[0] == "wpa2auto"){	//WPA
			get_by_id("wpa_0").style.display = "";
			get_by_id("wep_0").style.display = "none";	
			if(security0[0] == "wpa" && security0[1] == "psk"){		
				get_by_id("secu_mode_0").innerHTML = "WPA - PSK";
			}else if(security0[0] == "wpa" && security0[1] == "eap"){
				get_by_id("secu_mode_0").innerHTML = "WPA - EAP";
			}else if(security0[0] == "wpa2" && security0[1] == "psk"){
				get_by_id("secu_mode_0").innerHTML = "WPA2 - PSK";
			}else if(security0[0] == "wpa2" && security0[1] == "eap"){
				get_by_id("secu_mode_0").innerHTML = "WPA2 - EAP";
			}else if(security0[0] == "wpa2auto" && security0[1] == "psk"){
				get_by_id("secu_mode_0").innerHTML = "Auto (WPA or WPA2) - PSK";
			}else if(security0[0] == "wpa2auto" && security0[1] == "eap"){
				get_by_id("secu_mode_0").innerHTML = "Auto (WPA or WPA2) - EAP";
			}
		}
	//5G wireless	
		if(wlan1_security == "disable"){					//Disabled
			get_by_id("secu_mode_1").innerHTML = "none";
			get_by_id("wpa_1").style.display = "none";
			get_by_id("wep_1").style.display = "none";
		}else if(security1[0] == "wep"){						//WEP
			get_by_id("wep_1").style.display = "";
			get_by_id("wpa_1").style.display = "none";
			var default_key = "<% CmoGetCfg("wlan1_wep_default_key","none"); %>";
			var key;
			if(security1[2] == "64")
				key = "<% CmoGetCfg("wlan1_wep64_key_1","none"); %>";
			if(security1[2] == "128")	
				key = "<% CmoGetCfg("wlan1_wep128_key_1","none"); %>";		
			get_by_id("show_wep_key_1").innerHTML = "WEP KEY " + default_key + " : <strong>" + key + "</strong>";
			if(security1[1] == "auto"){
			get_by_id("secu_mode_1").innerHTML = "WEP - Both";
			}else if(security1[1] == "share"){
			get_by_id("secu_mode_1").innerHTML = "WEP - Shared Key";
			}
		}else if(security1[0] == "wpa" || security1[0] == "wpa2" || security1[0] == "wpa2auto"){	//WPA
			get_by_id("wpa_1").style.display = "";
			get_by_id("wep_1").style.display = "none";	
			if(security1[0] == "wpa" && security1[1] == "psk"){		
				get_by_id("secu_mode_1").innerHTML = "WPA - PSK";
			}else if(security1[0] == "wpa" && security1[1] == "eap"){
				get_by_id("secu_mode_1").innerHTML = "WPA - EAP";
			}else if(security1[0] == "wpa2" && security1[1] == "psk"){
				get_by_id("secu_mode_1").innerHTML = "WPA2 - PSK";
			}else if(security1[0] == "wpa2" && security1[1] == "eap"){
				get_by_id("secu_mode_1").innerHTML = "WPA2 - EAP";
			}else if(security1[0] == "wpa2auto" && security1[1] == "psk"){
				get_by_id("secu_mode_1").innerHTML = "Auto (WPA or WPA2) - PSK";
			}else if(security1[0] == "wpa2auto" && security1[1] == "eap"){
				get_by_id("secu_mode_1").innerHTML = "Auto (WPA or WPA2) - EAP";
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
  <td bgcolor="#FFFFFF"><p>&nbsp;</p>
  	<table width="650" border="0" align="center">
    <tr>
      <td>
      	<div id=box_header>
        	<h1 align="left"><script>show_words(wps_LW13)</script></h1>
        	<div align="left">
          		<br><script>show_words(TEXT048)</script></br>
          		<table align="center" class="formarea">
	            <tr>
	              <td>
	              	<br>
		              	<script>show_words(help699)</script>: <strong><% CmoGetCfg("wlan0_ssid","none"); %></strong>
				        <br>
						<script>show_words(bws_SM)</script>: <strong><span id="secu_mode_0"></span></strong>
						<br>
						<div id="wpa_0" style="display:none">
						<script>show_words(bws_CT)</script>: <strong><font style="text-transform:uppercase;"><% CmoGetCfg("wlan0_psk_cipher_type","none"); %></font></strong>
					    <br>
				        <script>show_words(LW25)</script>: <strong><% CmoGetCfg("wlan0_psk_pass_phrase","none"); %></strong>
						<br>
						</div>
						<div id="wep_0" style="display:none">
						<span id="show_wep_key_0"></span>
						</div>
						<br>
						<br>
		              	<script>show_words(help699)</script>: <strong><% CmoGetCfg("wlan1_ssid","none"); %></strong>
				        <br>
						<script>show_words(bws_SM)</script>: <strong><span id="secu_mode_1"></span></strong>
						<br>
						<div id="wpa_1" style="display:none">
						<script>show_words(bws_CT)</script>: <strong><font style="text-transform:uppercase;"><% CmoGetCfg("wlan1_psk_cipher_type","none"); %></font></strong>
					    <br>
				        <script>show_words(LW25)</script>: <strong><% CmoGetCfg("wlan1_psk_pass_phrase","none"); %></strong>
						<br>
						</div>
						<div id="wep_1" style="display:none">
						<span id="show_wep_key_1"></span>
						</div>
						<br>
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
