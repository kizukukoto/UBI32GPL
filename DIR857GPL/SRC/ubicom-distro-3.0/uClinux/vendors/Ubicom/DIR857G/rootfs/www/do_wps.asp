<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
var submit_button_flag = 0;
	function check_pin(){
		var accum = 0;
		var PIN = get_by_id("wps_sta_enrollee_pin").value;

		accum += 3 * Math.floor((PIN / 10000000) % 10);
		accum += 1 * Math.floor((PIN / 1000000) % 10);
		accum += 3 * Math.floor((PIN / 100000) % 10);
		accum += 1 * Math.floor((PIN / 10000) % 10);
		accum += 3 * Math.floor((PIN / 1000) % 10);
		accum += 1 * Math.floor((PIN / 100) % 10);
		accum += 3 * Math.floor((PIN / 10) % 10);
		accum += 1 * Math.floor((PIN / 1) % 10);

		return (0 == (accum % 10));
	}

	function CheckEnterKey(e) {

		var key = e.keyCode || e.which; //for IE and Firefox

		if (key == 13)
		{
			if (get_by_id("wps_sta_enrollee_pin").value == "")
			{
				if (e && e.preventDefault) {
					e.preventDefault(); //for Firefox
				} else
					window.event.returnValue = false; // for IE
					//e.stopPropagation();
				return false;
			}
			return true;
		}
	}

	function send_request(){
		if(!get_by_name("wps_pin_radio")[0].checked && !get_by_name("wps_pin_radio")[1].checked){
			alert(TEXT012);
			return false;
		}
		if(get_by_name("wps_pin_radio")[0].checked){
			if((get_by_id("wps_sta_enrollee_pin").value =="") || !_isNumeric(get_by_id("wps_sta_enrollee_pin").value) || (get_by_id("wps_sta_enrollee_pin").value.length != 8) || !check_pin()){
				alert(KR22_ww);
				return false;
			}
		}
		if(get_by_name("wps_pin_radio")[1].checked){
			get_by_id("html_response_page").value = "wps_back.asp";
			get_by_id("form1").action = "virtual_push_button.cgi";
		}
		if(submit_button_flag == 0){
			submit_button_flag = 1;
			get_by_id("form1").submit();
		}
	}
	
	function onPageLoad()
    {
            if (("<% CmoGetCfg("disable_wps_pin","none"); %>" === "1") && ("<% CmoGetCfg("wps_configured_mode","none"); %>" === "5")){
                    get_by_name("wps_pin_radio")[0].disabled = true;
                    get_by_id("wps_sta_enrollee_pin").disabled = true;
            }
    }
</script>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
<table border=0 cellspacing=0 cellpadding=0 align=center width=838>

<tr>
<td>

  <table width=836 border=0 cellspacing=0 cellpadding=0 align=center height=100>
<tr>
  <td bgcolor="#FFFFFF">    <div align="center">
    <table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
      <tr>
        <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
        <td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
		<td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
		<td>&nbsp;</td>
      </tr>
    </table>
    <img src="wlan_masthead.gif" width="836" height="92" align="middle">
      <br>
  </div>
    <table width="650" border="0" align="center"><br>
    <tr>

      <td><div class=box>

        <h2 align="left"><script>show_words(wps_LW13)</script></h2>

        <div align="left">

          <p><script>show_words(wps_p3_1)</script> </p>

          <P>-<script>show_words(wps_p3_2)</script> </P>

          <P>-<script>show_words(wps_p3_3)</script> </P>

          <form id="form1" name="form1" method="post" action="set_sta_enrollee_pin.cgi">
          	<input type="hidden" id="html_response_page" name="html_response_page" value="do_wps_save.asp">
			<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="do_wps.asp">
			<input type="hidden" id="reboot_type" name="reboot_type" value="none">
          <table align="center" class="formarea">
            <tr>
              <td width="50%" align="right"><input id="wps_pin_radio" name="wps_pin_radio" value="0" type="radio" checked="checked"></td>
              <td width="50%">
              	<b>PIN :&nbsp;</b>
              	<input id="wps_sta_enrollee_pin" name="wps_sta_enrollee_pin" maxlength="8" type="text" onKeyPress="CheckEnterKey(event)">
              </td>
            </tr>
            <tr>
            	<td colspan="2">

                <p><script>show_words(wps_p3_4)</script></p>

                <p></p>

            </td></tr>
            <tr>
              <td align="right"><INPUT type="radio" id="wps_pin_radio" name="wps_pin_radio" value="1"></td>
              <td><b>PBC&nbsp;</b></td>
            </tr>
            <tr>
              <td colspan="2">

              	<p><script>show_words(wps_p3_5)</script>

                              </p>

              	<p></p>

              </td>
            </tr>
            <tr align="center">
              <td colspan="3">

                             <input name="btn_prev" id="btn_prev" type="button" class=button_submit value="" onClick="window.location.href='wps_wifi_setup.asp'">

                             <input name="btn_connect" id="btn_connect" type="button" class=button_submit value="" onClick="send_request()">

                              <script>get_by_id("btn_prev").value = _prev;</script>

							  <script>get_by_id("btn_connect").value = _connect;</script>

              </td>
            </tr>
          </table>
      </form>
        </div>
      </div></td>
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
</td>
</tr>
<tr>
<td bgcolor="#FFFFFF">

  </td>
</tr>
</table>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
</body>
<script>
onPageLoad();
</script>
</html>
