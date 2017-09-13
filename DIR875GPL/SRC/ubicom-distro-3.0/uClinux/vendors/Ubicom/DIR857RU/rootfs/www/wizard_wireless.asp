<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<style type="text/css">
<!--
.style1 {
	color: #FF0000;
	font-weight: bold;
}
.style2 {font-size: 11px}
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
										show_side_bar(1);
									</script>
								</ul>
							</div>
						</td>
					</tr>
				</table>
			</td>
			<td valign="top" id="maincontent_container">
				<div id=maincontent>
                  <div id=box_header>
          <h1>

            <script>show_words(LW38)</script>

          </h1>



          <p>

            <script>show_words(LW39)</script>

          </p>



          <p>

            <script>show_words(LW39c)</script>

          </p>

                  </div>



                  <div class=box>

          <h2>

            <script>show_words(wwl_wl_wiz)</script>

          </h2>

          <script>show_words(LW41)</script>

                    <br>

                  <br>

          <center>

		  <input type="button" class="button_submit" id="wireless_wizard" name="wireless_wizard" value="" onClick=window.location.href="wizard_wlan.asp">

          <script>get_by_id("wireless_wizard").value = wwl_wl_wiz;</script>

		  </center>

                    <br><br>

          <strong>

          <script>show_words(_note)</script>

          :</strong>

          <script>show_words(bwz_note_WlsWz)</script>

                  </div>



                  <div class=box>

          <h2>

            <script>show_words(wps_LW13)</script>

          </h2>

          <script>show_words(LW40)</script>

                  <br>

                  <br>

          <center>

		  <input type="button" class="button_submit" id="wps_wizard" name="wps_wizard" value="" onClick=window.location.href="wps_wifi_setup.asp">

          <script>get_by_id("wps_wizard").value = LW13;</script>

		  </center>

                  </div>



				  <div class=box>

          <h2>

            <script>show_words(LW42)</script>

          </h2>

          <script>show_words(LW43)</script>

          <script>show_words(LW44)</script>

          <br>

          <br>

          <center>

		  <input type="button" class="button_submit" id="wizard" name="wizard" value="" onClick=window.location.href="wireless.asp">

          <script>get_by_id("wizard").value = LW42;</script>

		  </center>

                    <br>
                  </div>
			  </div></td>
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
                  <tbody>
                    <tr>



            <td id=help_text><strong>

              <script>show_words(_hints)</script>

              &hellip;</strong>

                          <!--p>If you already have a wireless network setup with Wi-Fi Protected Setup, click on <strong>Add Wireless Device Wizard</strong> to add new device to your wireless network.</p-->

              <p>

                <script>show_words(LW46)</script>

              </p>



              <p>

                <script>show_words(LW47)</script>

              </p>

						  <p><a href="support_internet.asp#Wireless"><script>show_words(_more)</script>&hellip;</a></p>

                      </td>
                    </tr>
                  </tbody>
				</table>
			  </td>
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
if ("<% CmoGetStatus("get_current_user"); %>" == "user" || 
    (("<% CmoGetCfg("wlan0_enable", "none"); %>" == "0") && 
     ("<% CmoGetCfg("wlan1_enable", "none"); %>" == "0"))) {
	get_by_id("wps_wizard").disabled = true;
    get_by_id("wireless_wizard").disabled = true;
}
</script>
</html>