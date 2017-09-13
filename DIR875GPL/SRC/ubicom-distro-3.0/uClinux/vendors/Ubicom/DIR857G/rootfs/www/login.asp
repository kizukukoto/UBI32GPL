<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=utf8">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
var submit_button_flag = true;
function encode_base64(psstr) {
   		return encode(psstr,psstr.length);
}

function encode (psstrs, iLen) {
	 var map1="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   var oDataLen = (iLen*4+2)/3;
   var oLen = ((iLen+2)/3)*4;
   var out='';
   var ip = 0;
   var op = 0;
   while (ip < iLen) {
      var xx = psstrs.charCodeAt(ip++);
      var yy = ip < iLen ? psstrs.charCodeAt(ip++) : 0;
      var zz = ip < iLen ? psstrs.charCodeAt(ip++) : 0;
      var aa = xx >>> 2;
      var bb = ((xx &   3) << 4) | (yy >>> 4);
      var cc = ((yy & 0xf) << 2) | (zz >>> 6);
      var dd = zz & 0x3F;
      out += map1.charAt(aa);
      op++;
      out += map1.charAt(bb);
      op++;
      out += op < oDataLen ? map1.charAt(cc) : '=';
      op++;
      out += op < oDataLen ? map1.charAt(dd) : '=';
      op++;
   }
   return out;
}

function check()
{
	var pwd=get_by_id("log_pass").value;

	if(submit_button_flag){
	 get_by_id("login_name").value=encode_base64(get_by_id("login_n").value);    // save to nvram
 	 get_by_id("login_pass").value=encode_base64(pwd);   // save to nvram
	 get_by_id("login_n").value=get_by_id("login_name").value;   //set admin field value encode too..
     get_by_id("log_pass").value=get_by_id("login_pass").value;   //set password field value encode too..

     	var auth = "<% CmoGetCfg("graph_auth_enable","none"); %>";
		if (auth == 1){
		get_by_id("graph_id").value=encode_base64(get_by_id("graph_id").value);
		get_by_id("graph_code").value=encode_base64(get_by_id("graph_code").value);
	}

		submit_button_flag = false;
	}
	return true;
}

function chk_KeyValue(e){
	var salt = "<% CmoGetStatus("login_salt"); %>"
	if(browserName == "Netscape") {
		var pKey=e.which;
	}
	if(browserName=="Microsoft Internet Explorer") {
		var pKey=event.keyCode;
	}
	if(pKey==13){
		if(check()){
			send_submit("form1");
		}
	}
}

function AuthShow(){
		get_by_id("show_graph").style.display = "none";
		get_by_id("show_graph2").style.display = "none";
		var auth = "<% CmoGetCfg("graph_auth_enable","none"); %>";
		if (auth == 1){
			get_by_id("show_graph").style.display = "";
			get_by_id("show_graph2").style.display = "";
		}
}
function OnPageLoad(){
	var check_login = get_by_id("alert_id").value;
		if(check_login == 1){
			alert(li_alert_3);
		}else if(check_login == 2){
			alert(li_alert_4);
		}else{

	}

	}

var browserName = navigator.appName;
document.onkeyup=chk_KeyValue;
</script>
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575" onLoad="document.form1.log_pass.focus();">
<table border=0 cellspacing=0 cellpadding=0 align=center width=30>
<tr>
<td></td>
</tr>
<tr>
<td>
<div align=left>
<table width=836 border=0 cellspacing=0 cellpadding=0 align=center>
<tr>
  <td align="center" valign="baseline" bgcolor="#FFFFFF">
  	<table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
    <tr>
      <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
        <td width="60%">&nbsp;</td>
        <td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
      <td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
      <td>&nbsp;</td>
      <td>&nbsp;</td>
    </tr>
  </table></td>
</tr>
<tr>
<td align="center" valign="baseline" bgcolor="#FFFFFF">
<div align=center>
  <table id="topnav_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
    <tr>
      <td align="center" valign="middle"><img src="wlan_masthead.gif" width="836" height="92"></td>
    </tr>
  </table>
  <br><br>
  <table width="650" border="0">
    <tr>
      <td height="10">
		<div id=box_header>
        <H1 align="left"><script>show_words(li_Login)</script></H1>
          <script>show_words(li_intro)</script><p>
			<form name="form1" id="form1" action="login.cgi" method="post" onSubmit="return check();">
				<input type="hidden" id="html_response_page" name="html_response_page" value="login.asp">
				<input type="hidden" id="login_name" name="login_name">
				<input type="hidden" id="login_pass" name="login_pass">
				<input type="hidden" id="graph_id" name="graph_id" value="<% CmoGetStatus("graph_auth_id"); %>">
				<input type="hidden" id="alert_id" name="alert_id" value="<% CmoGetStatus("gui_login_alert"); %>">
          <table width="95%" border="0" align="center" cellpadding="0" cellspacing="0">
            <tr height="24">
              <td width="30%"></td>
                <td colspan="2"><b><script>show_words(_username)</script> :&nbsp;</b>
              	<select id="login_n" name="login_n">
					<option value="admin"><script>show_words(_admin)</script></option>
					<option value="user"><script>show_words(_user)</script></option>
				</select></td>
            </tr>
            <tr height="24">
                <td width="30%"></td>
                <td colspan="2"><b><script>show_words(_password)</script> :&nbsp;</b>
                    <input type="password" id="log_pass" name="log_pass" value="" tabindex="120" onfocus="select();"/></td>
            </tr>
            <tr height="54" id="show_graph" style="display:yes">
                <td width="30%"></td>
                <td colspan="2" width="280"><b><script>show_words(_authword)</script>&nbsp;</b>
                    <input type="password" id="graph_code" name="graph_code" value="" maxlength="8" onfocus="select();"/></td>
              </tr>
             <tr id="show_graph2" style="display:yes">
                <td width="30%"></td>
                <td width="90" height="40"><img src="auth.bmp"></td>
                <td><input class="button_submit_padleft" type=button name=Refresh id=Refresh value="" onClick="window.location.reload( true );" style="width:120 ">
				<script>get_by_id("Refresh").value = regenerate;</script></td>
              </tr>
             </table>
             <table width="95%" border="0" align="center" cellpadding="0" cellspacing="0">
               <tr>
                 <td width="25%"></td>
                 <td width="90"></td>
                 <td><br>
                    <input class="button_submit_padleft" type="submit" name="login" id="login" value="" style="width:120 ">
			  <script>get_by_id("login").value = li_Log_In;</script>
                </td>
              </tr>
            </table>
            </form>
        </div>
      </td>
      </tr>
  </table>
  <p>&nbsp;</p>
  </div></td>
</tr>
</table>
</div>
</td>
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
</table><br>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
</body>
<script>
AuthShow();
OnPageLoad();
</script>
</html>
