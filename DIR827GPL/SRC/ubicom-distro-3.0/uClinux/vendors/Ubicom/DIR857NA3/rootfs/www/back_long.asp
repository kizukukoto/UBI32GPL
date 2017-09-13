<html lang=en-US xml:lang="en-US" xmlns="http://www.w3.org/1999/xhtml">
<head><title><% CmoGetStatus("title"); %></title>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>	
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
	var count = 70;

	function onPageLoad()
	{
        var temp_count = "<% CmoGetCfg("countdown_time","none"); %>";
        if (temp_count != "") {
            count = parseInt(temp_count);
        }
		
	    do_count_down();
		get_by_id("html_response_page").value = "please_wait.asp";
	}

	function do_count_down()
	{
		get_by_id("button").disabled = true;
		get_by_id("show_sec").innerHTML = count;

		if (count == 0) {
	        get_by_id("button").disabled = false;
	        return;
	    }

		if (count > 0) {
	        count--;
	        setTimeout('do_count_down()', 1000);
	    }
	}

	function back()
	{
		if ("<% CmoGetStatus("get_current_user"); %>" == "user")
			window.location.href ="index.asp";
		else
			window.location.href = get_by_id("html_response_page").value;
	}
</script>

<link rel="STYLESHEET" type="text/css" href="css_router.css">
<style type="text/css">
<!--
.style1 {color: #FF6600}
-->
</style>
<title></title>
<meta http-equiv=Content-Type content="text/html; charset=iso-8859-1">
</head>
<body onload="onPageLoad();" topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
<div >
<form id="form1" name="form1" method="post">
<input type="hidden" id="html_response_page" name="html_response_page" value="index.asp">
<input type="hidden" id="html_response_message" name="html_response_message" value="">
<script>get_by_id("html_response_message").value = sc_intro_sv;</script>
<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="<% CmoGetCfg("html_response_return_page","none"); %>">
<input type="hidden" name="reboot_type" value="none">
  <table width="838" height="100" border=0 align="center" cellPadding=0 cellSpacing=0 id=table_shell>
  <tbody>

  <tr>
    <td bgcolor="#FFFFFF">
      <div align="center">
        <table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
          <tr>
            <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
            <td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
			<td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
			<td>&nbsp;</td>
          </tr>
        </table>
        <img src="wlan_masthead.gif" width="836" height="92"></div></td>
    </tr>
  <tr>
    <td>
      <table width="838" border=0 align="center" cellPadding=0 cellSpacing=0 >
        <tbody>
        <tr>
          <td bgcolor="#FFFFFF"></td></tr>
        <tr>
          <td bgcolor="#FFFFFF"></td>
        </tr>
        <tr>
          <td bgcolor="#FFFFFF"><p>&nbsp;</p>            <table width="650" border="0" align="center">
            <tr>
              <td height="15"><div id=box_header>
                  <H1 align="left"><span class="style1">&nbsp;</span>
                      <!-- insert name=title -->
                  </H1>
                  <div align="left">
                    <p align="center"><script>
					var login_who="<% CmoGetStatus("get_current_user"); %>";
					if(login_who== "user"){
						//document.write("Only admin account can change the settings.");
						show_words(ca_intro)
					}else{
						document.write(get_by_id("html_response_message").value+"&nbsp;");
						//show_words(rb_change)
					}

						document.write('&nbsp;Please wait&nbsp;<span id="show_sec"></span>&nbsp; seconds.');

					</script>
                    </p>
                    <p align="center">&nbsp;</p>
                    <p align="center">
                      <input name="button" id="button" type=button class=button_submit value="" onClick="back()">
                      <script>get_by_id("button").value = _continue;</script>
                    </p>
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
        </tbody></table>     </td>
    </tr>
  </tbody></table></form>
  <div id="copyright"><% CmoGetStatus("copyright"); %></div>
</div>
</BODY>
</html>
