<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<title>D-LINK CORPORATION, INC | WIRELESS ROUTER | HOME</title>
<meta http-equiv=Content-Type content="text/html; charset=UTF-8">
<style type="text/css">
<!--
.style1 {color: #FF6600}
-->
</style>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    function onPageLoad()
    {
        get_by_id("html_response_page").value = get_by_id("html_response_return_page").value;
    }

    function back()
    {
        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            window.location.href = "index.asp";
        }
        else {
            window.location.href = get_by_id("html_response_page").value;
        }
    }

    function reboot_now()
    {
        send_submit("form2");
    }
</script>
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
<div >
<form id="form1" name="form1" method="post">
<input type="hidden" id="html_response_page" name="html_response_page" value="">
<input type="hidden" id="html_response_message" name="html_response_message" value="<% CmoGetCfg("html_response_message","none"); %>">
<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="<% CmoGetCfg("html_response_return_page","none"); %>">
<input type="hidden" name="reboot_type" value="none">
  <table width="838" height="100" border=0 align="center" cellPadding=0 cellSpacing=0 id=table_shell>
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
        <img src="/wlan_masthead.gif" width="836" height="92"></div></td>
    </tr>
  <tr>
    <td>
      <table width="838" border=0 align="center" cellPadding=0 cellSpacing=0 >
        <tr>
          <td bgcolor="#FFFFFF"></td></tr>
        <tr>
          <td bgcolor="#FFFFFF"></td>
        </tr>
        <tr>
          <td bgcolor="#FFFFFF"><p>&nbsp;</p>
           <table width="650" border="0" align="center">
            <tr>
              <td height="15"><div id=box_header>
                  <h1><script>show_words(YM1);</script>&hellip;</h1>
                  <p class=""><script>show_words(YM2);</script></p>
                  <input name="reboot" id="reboot" type="button" class=button_submit onClick="reboot_now()">
                  <input name="later" id="later" type="button" class=button_submit onclick="back()">
                  <script>get_by_id("reboot").value = YM3</script>
                  <script>get_by_id("later").value = YM4</script>
                  <br>
              </div></td>
            </tr>
          </table>
          <p>&nbsp;</p>
          </td>
        </tr>
        <tr>
          <td bgcolor="#FFFFFF"><table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="836" align="center">
            <tr>
              <td width="125" align="center">&nbsp;&nbsp;<img src="/wireless_tail.gif" width="114" height="35"></td>
              <td width="10">&nbsp;</td>
              <td>&nbsp;</td>
              <td>&nbsp;</td>
            </tr>
          </table></td>
        </tr>
        </table>     </td>
    </tr>
  </table></form>
  <form id="form2" name="form2" method="post" action="reboot.cgi">
     <input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
  </form>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
</div>
</BODY>
<script>
    onPageLoad();
</script>
</html>
