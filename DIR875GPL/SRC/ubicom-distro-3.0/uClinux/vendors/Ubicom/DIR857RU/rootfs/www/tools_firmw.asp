<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var submit_button_flag = 0;
    var uf;
    function onPageLoad()
    {
    	  uf = document.forms.form1;
        if ("<% CmoGetStatus("get_current_user"); %>" == "user"){
            DisableEnableForm(document.form1, true);
            DisableEnableForm(document.form2, true);
            DisableEnableForm(document.formclearlp, true);
            get_by_id("check_now_b").disabled = true;
        }
    }
/*IFDEF CONFIG_LP*/
var lp_version="<% CmoGetStatus("lp_version"); %>";

function show_LP_status(lp_ver){
	get_by_id("na_lp_ver_field").style.display = (lp_ver=="NA")? "":"none";
	get_by_id("lp_ver_field").style.display = (lp_ver=="NA")?"none":"";
	get_by_id("lp_clear_field").style.display =(lp_ver=="NA")?"none":"";
}

	function send_reqLP(aform){
		if (get_by_id("fileLP").value === "") {
			alert(tf_FWF1);
			return false;
		}
		send_file(aform);
	}
function confirm_clearlp(){
	if(confirm("Remove Language Pack"+"?"))
	send_submit("formclearlp");
}
/*ENDIF CONFIG_LP*/
	function send_file(aform){
		try {
			get_by_id("msg_upload").style.display = "";
			get_by_id("upgrade_button").disabled = true;
/*DEF CONFIG_LP*/			get_by_id("btn_LPupgrade").disabled = true;
			send_submit(aform);
		}catch (e){
			alert("%[.error:Error]%: " + e.message);
			get_by_id("msg_upload").style.display = "none";
			get_by_id("upgrade_button").disabled = false;
/*DEF CONFIG_LP*/			get_by_id("btn_LPupgrade").disabled = false;
        }
	}

	function send_request(){
		if (get_by_id("file").value === "") {
			alert(tf_FWF1);
			return false;
		}
		if (!confirm(tf_USSW)) {
			return false;
		}
		if (!confirm(tf_really_FWF+" \""+ get_by_id("file").value + " \"?" )) {
				return false;
		}

        try {
            get_by_id("msg_upload").style.display = "";
            get_by_id("upgrade_button").disabled = true;
            if (submit_button_flag == 0) {
                submit_button_flag = 1;
				send_submit("form1");
            }
        } catch (e) {
            alert("%[.error:Error]%: " + e.message);
            get_by_id("msg_upload").style.display = "none";
            get_by_id("upgrade_button").disabled = false;
        }
	}

	function New_update_button(){
        get_by_id("iframe_scan").src = "tools_firmw_chk.asp";
		//get_by_id("update_new_result").style.display ="";
        get_by_id("check_now_b").disabled = true;
        document.getElementById("check_result").innerHTML = "";
    }

	function download_latest_fm(type){
		for (var i = 0; i < get_by_id( type +"sel_site").length; i++){
			if(get_by_id(type +"sel_site").options[i].selected == true){
				var html = get_by_id( type +"sel_site").options[i].value;
            }
        }
        window.open(""+html+"");
    }
</script>

<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
</head>
<body onload="onPageLoad();" topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
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
			<td id="topnavoff"><a href="index.asp" onclick="return jump_if();"><script>show_words(_setup)</script></a></td>
			<td id="topnavoff"><a href="adv_virtual.asp" onclick="return jump_if();"><script>show_words(_advanced)</script></a></td>
			<td id="topnavon"><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_tools)</script></a></td>
			<td id="topnavoff"><a href="st_device.asp" onclick="return jump_if();"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp" onclick="return jump_if();"><script>show_words(_support)</script></a></td>
        </tr>
    </table>
	<table border="1" cellpadding="2" cellspacing="0" width="838" height="100%" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
        <tr>
            <td id="sidenav_container" valign="top" width="125" align="right">
                <table border="0" cellpadding="0" cellspacing="0">
                    <tr>
                        <td id="sidenav_container">
                            <div id="sidenav">
                                <ul>
									<li><div><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_admin)</script></a></div></li>
									<li><div><a href="tools_time.asp" onclick="return jump_if();"><script>show_words(_time)</script></a></div></li>
									<li><div><a href="tools_syslog.asp" onclick="return jump_if();"><script>show_words(_syslog)</script></a></div></li>
									<li><div><a href="tools_email.asp" onclick="return jump_if();"><script>show_words(_email)</script></a></div></li>
									<li><div><a href="tools_system.asp" onclick="return jump_if();"><script>show_words(_system)</script></a></div></li>
									<li><div id="sidenavoff"><script>show_words(_firmware)</script></div></li>
									<li><div><a href="tools_ddns.asp" onclick="return jump_if();"><script>show_words(_dyndns)</script></a></div></li>
                                    <li><div><a href="tools_vct.asp" onclick="return jump_if();"><script>show_words(_syscheck)</script></a></div></li>
									<li><div><a href="tools_schedules.asp" onclick="return jump_if();"><script>show_words(_scheds)</script></a></div></li>
                                </ul>
                            </div>
                        </td>
                    </tr>
                </table>
            </td>
            <td valign="top" id="maincontent_container">
                <div id="maincontent">

                    <div id="box_header">
          <h1>
            <script>show_words(_firmware)</script>
          </h1>

          <p>
            <script>show_words(tf_intro_FWU1)</script>
            <% CmoGetCfg("model_number","none"); %>
            <script>show_words(tf_intro_FWU2)</script>
          </p>
          <p>
            <script>show_words(tf_intro_FWChB)</script>
          </p>
                    </div>
                    <div class="box">
          <h2>
            <script>show_words(tf_FWInf)</script>
          </h2>
                            <table width=525 border=0 cellpadding=2 cellspacing="2">

                              <tr>
								<td width="40%" align=right class="duple"><script>show_words(tf_CFWV)</script>&nbsp;:</td>
								<td width="15%" class="output"><% CmoGetStatus("version"); %></td>
								<td width="10%" align=right style="FONT-WEIGHT: bold" class="output"><script>show_words(_date)</script>&nbsp;:</td>
								<td width="35%" class="output"><% CmoGetStatus("version_date"); %></td>
							  </tr>
							  <tr id="lp_ver_field"  style="DISPLAY:none">
								<td width="40%" align=right class="duple" nowrap><script>show_words(tf_CLPV)</script>&nbsp;:</td>
								<td width="15%" class="output"><script>document.write(lp_version);</script></td>
								<td width="10%" align=right style="FONT-WEIGHT: bold" class="output"><script>show_words(_date)</script>&nbsp;:</td>
								<td width="35%" class="output"><% CmoGetStatus("lp_date"); %></td>
							  </tr>
							  <tr id="na_lp_ver_field" style="DISPLAY:none">
								<td class="duple" width="40%" align=right nowrap>
								<script>show_words(tf_CLPV)</script>&nbsp;:</td>
								<td class=output width="60%" colspan=3>
									<script>show_words(_nalp)</script></td>
							  </tr>

<form id="formclearlp" name="formclearlp" method="post" action="lp_clear.cgi">
<input type="hidden" name="html_response_page" value="back.asp">
<input type="hidden" name="html_response_return_page" value="tools_firmw.asp">
                              <tr id="lp_clear_field" sytle="Display:none">
                              	<td align=right class="duple"><script>show_words(_clearlp)</script>:</td>
                                <td width="60%" colspan=3 class="output">
									<input type="button" id="lpclrBtn" name="lpclrBtn" onclick="confirm_clearlp()">
<script>
get_by_id("lpclrBtn").value=_remove;
get_by_id("lpclrBtn").disabled = (lp_version == "") ?true:false;
</script>
              </td>
                              </tr>
</form>
                              <tr>
                                <td colspan=4>&nbsp;&nbsp;&nbsp;&nbsp;<strong><script>show_words(tf_COLFL)</script>&nbsp;:&nbsp;
                <input id="check_now_b" name="check_now_b" type="button" class=button_submit value="" onClick="New_update_button();">
                <script>get_by_id("check_now_b").value = tf_CKN;</script>
			    </strong> </td>
                              </tr>
				 &nbsp;&nbsp;&nbsp;&nbsp; &nbsp;<tr>
                                <td colspan=4>
                                    <strong><div id="check_result"></div></strong>
                                </td>
                              </tr>
                            </table>
                    </div>
				  	<div id="downloadsite_info" name="donwloadsite_info" class="box" style="display:none">
				  	<h2><script>show_words(tf_FWCheckInf)</script></h2>
						<div id="FWdownloadsite_info" style="display:none">
							<table width=76% height=57 border=0 cellpadding=2 cellspacing="2">
                              <tr>
                                <td width="40%"><div align="right"><script>show_words(tf_LFWV)</script>&nbsp;</div></td>
                                <td width="60%" height=10 colspan=2>
								<div align="left" id="FWlatest_version"></div>
								</td>
                              </tr>
                              <tr>
                              <td width="40%">
							  	<div align="right"><script>show_words(tf_LFWD);</script> &nbsp;</div>
								</td>
                                <td width="60%" height=10 colspan=2>
                                  <div align="left" id="FWlatest_date"></div></td>
                              </tr>
                              <td width="40%"><div align="right"><script>show_words(tf_ADS);</script> &nbsp;</div></td>
                                <td width="60%" height=10 colspan=2>
                                 <select id="FWsel_site" name="FWsel_site" onChange="">
								 </select>
								 </td>
                              </tr>
                              <tr>
                                <td colspan="3" height="21" align="center">
                                    <input type="button" name="FWdownload"  id="FWdownload" value="" onClick="download_latest_fm('FW');">
                                    <script>get_by_id("FWdownload").value=help501;</script>
                                </td>
                              </tr>
                            </table>
						</div>
						<div id="LPdownloadsite_info" style="display:none">
                            <table width=76% height=57 border=0 cellpadding=2 cellspacing="2">
                              <tr>
							    <td width="40%"><div align="right"><script>show_words(tf_LLPV);</script>&nbsp;</div></td>
                                <td width="60%" height=10 colspan=2>
							    <div align="left" id="LPlatest_version"></div>
                                </td>
                              </tr>
                              <tr>
                              <td width="40%">
							    <div align="right"><script>show_words(tf_LLPD);</script>; &nbsp;</div>
                                </td>
                                <td width="60%" height=10 colspan=2>
							      <div align="left" id="LPlatest_date"></div></td>
                              </tr>
							  <td width="40%"><div align="right"><script>show_words(tf_ADS);</script> &nbsp;</div></td>
                                <td width="60%" height=10 colspan=2>
							     <select id="LPsel_site" name="LPsel_site" onChange="">
                                 </select>
                                 </td>
                              </tr>
                              <tr>
                                <td colspan="3" height="21" align="center">
							        <input type="button" name="LPdownload"  id="LPdownload" value="" onClick="download_latest_fm('LP');">
							        <script>get_by_id("LPdownload").value=help501;</script>
                                </td>
                              </tr>
                            </table>
                    </div>
				  	</div>
                    <form id="form1" name="form1" method=POST action="firmware_upgrade.cgi" enctype=multipart/form-data>
                    <input type="hidden" id="html_response_page" name="html_response_page" value="ustatus.asp">
                    <input type="hidden" name="html_response_return_page" value="tools_firmw.asp">

                    <div class="box">
            <h2>
              <script>show_words(tf_FWUg)</script>
            </h2>
                        <table width=525 border=0 cellpadding=2 cellspacing="2">
                          <tr>

                <td colspan=3> <p class="box_alert"> <strong>
                    <script>show_words(_note)</script>
                    :</strong>
                    <script>show_words(tf_msg_FWUgReset)</script>
                                </p>
                                <p class="box_msg">
                    <script>show_words(tf_msg_wired)</script>
                                </p>
                            </td>
                          </tr>
                          <tr>
                            <td align=right class="duple"></td>
                            <td height="59" colspan=2>
                                <input type=file id=file name=file size="30"><br>
                                <input type="button" id="upgrade_button" name="upgrade_button" class=button_submit value="" onClick="send_request();">
                            	<script>get_by_id("upgrade_button").value = tf_Upload;</script>
                            </td>
                          </tr>
                          <tr id="msg_upload" style="display:none" class="msg_inprogress">
                            <td align=right class="duple"></td>
                            <td colspan=2><script>show_words(tf_msg_Upping);</script></td>
                          </tr>
                        </table>
				    </div>
				  </form>
				<form id="form2" name="form2" method=POST enctype=multipart/form-data action="lp_upgrade.cgi">
					<input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
					<input type="hidden" name="html_response_return_page" value="tools_firmw.asp">
                    <div class="box">
                        <h2><script>show_words("LANGUAGE PACK UPGRADE");</script></h2>
                        <table width=525 border=0 cellpadding=2 cellspacing="2">
                          <tr>
                            <td align=right class="duple"></td>
                            <td height="59" colspan=2>
                                <input type=file id=fileLP name=fileLP size="30"><br>
                                <input type=button id="btn_LPupgrade" name="btn_LPupgrade" class=button_submit onClick="send_reqLP('form2');">
                                <script>get_by_id("btn_LPupgrade").value=tf_Upload;
                                get_by_id("btn_LPupgrade").disabled=(<% CmoGetStatus("language_mtdblock_check"); %> == 0) ?true:false;
                                </script>
                            </td>
                          </tr>
                        </table>
                    </div>
                  </form>
                  <div id="update_new_result" name="update_new_result">
                    <table width=90% border=0 cellpadding=2 cellspacing="2">
                        <tr>
                          <td height=0 align="center">
                            <IFRAME id="iframe_scan" name="iframe_scan" align=middle border="0" frameborder="0" marginWidth=0 marginHeight=0 src="" width="100%" height=0 scrolling="no"></IFRAME>
                          </td>
                        </tr>
                    </table>
                  </div>
                </div>
            </td>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table cellpadding="2" cellspacing="0" border="0" bgcolor="#FFFFFF">
                    <tr>

          <td id=help_text><strong>
            <script>show_words(_hints)</script>
            &hellip;</strong>
            <p>
              <script>show_words(ZM17)</script>
            </p>
							<p class="more"><a href="support_tools.asp#Firmware" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
                      </td>
                    </tr>
                </table>
            </td>
        </tr>
    </table>
    <table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
        <tr>
            <td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
            <td width="10">&nbsp;</td>
            <td>&nbsp;</td>
        </tr>
    </table>
<br>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
<br>
</body>
<script>
/*DEF CONFIG_LP*/	show_LP_status(lp_version);
</script>
</html>
