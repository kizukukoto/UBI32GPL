<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">

	var get_email_detail = "";
	var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

    function onPageLoad()
    {
        //email notification
        set_checked(get_by_id("log_email_enable").value, get_by_id("email_enable"));
        set_checked(get_by_id("log_email_auth").value, get_by_id("email_auth"));

        //mail_schedule
        var mail_schedule = get_by_id("log_email_schedule").value.split("/");
        //on log full check
        set_checked(mail_schedule[0], get_by_id("email_sch_enable"));
        //on schedule check
        set_checked(mail_schedule[1], get_by_id("by_email_sch"));
        set_selectIndex(mail_schedule[3], get_by_id("email_schedule"));
        get_email_detail = mail_schedule[4];
        //key_word(get_by_id("email_schedule"), "email_detail");
        //get_by_id("email_detail").value = get_email_detail;
        get_detail();
        get_by_id("email_detail").readOnly = true;

        disable_schedule()
        disable_notification();

        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1,true);
        }

        set_form_default_values("form1");
    }

    function get_schedule_detail()
    {
    	if(get_by_id("by_email_sch").checked)
    	{
    			for (var i = 0; i < 32; i++){
						var temp_sch = get_by_id("schedule_rule_" + i).value;
						var temp_data = temp_sch.split("/");

					  if (temp_data.length > 1){
							if(get_by_id("email_schedule").value == temp_data[0]){
							get_email_detail = tr_week(temp_data[1])+" "+temp_data[2]+"-"+temp_data[3];
							return;
						  }
						}
						if(get_by_id("email_schedule").value == "Never"){
						 get_email_detail = "Never";
						 return;
						 }
					}
    	}
    }

    function tr_week(week)
    {
    	var s = new Array();
      for(var j = 0; j < 8; j++){
          if(week.charAt(j) == "1"){
              s[j] = "1";
          }else{
              s[j] = "0";
          }
      }

      var s_day = "", count = 0;
      for(var j = 0; j < 8; j++){
          if(s[j] == "1"){
              s_day = s_day + " " + Week[j];
              count++;
          }
      }
      return s_day;
    }

    function check_mail()
    {
        var tmp_smtp_port = get_by_id("log_email_server_port").value;
        var smtp_port_msg = replace_msg(check_num_msg, te_SMTPSv_Port, 1, 65535);
        var smtp_port_obj = new varible_obj(tmp_smtp_port, smtp_port_msg, 1, 65535, false);

        if (get_by_id("log_email_sender").value == "") {
			alert(_blankfromemailaddr);
            return false;
		}else if(get_by_id("log_email_recipien").value == ""){
			alert(_blanktomemailaddr);
            return false;
		}else if (get_by_id("log_email_server").value == ""){
			alert(_blanksmtpmailaddr);
            return false;
		}else if (!check_varible(smtp_port_obj)){
            return false;
		}else if (get_by_id("log_email_password").value != get_by_id("email_pw2").value){
			alert(YM102);
            return false;
        }
        return true;
    }

    function get_detail()
    {
        //key_word(get_by_id("email_schedule"), "email_detail");
        get_schedule_detail();
        get_by_id("email_detail").value = get_email_detail;
    }

    function disable_emai_auth()
    {
        if (get_by_id("email_enable").checked) {
            get_by_id("log_email_username").disabled = !get_by_id("email_auth").checked;
            get_by_id("log_email_password").disabled = !get_by_id("email_auth").checked;
            get_by_id("email_pw2").disabled = !get_by_id("email_auth").checked;
        }
    }

    function disable_notification()
    {
        get_by_id("log_email_sender").disabled = !get_by_id("email_enable").checked;
        get_by_id("log_email_recipien").disabled = !get_by_id("email_enable").checked;
        get_by_id("log_email_server").disabled = !get_by_id("email_enable").checked;
        get_by_id("log_email_server_port").disabled = !get_by_id("email_enable").checked;
        get_by_id("email_auth").disabled = !get_by_id("email_enable").checked;
        get_by_id("log_email_username").disabled = !get_by_id("email_enable").checked;
        get_by_id("log_email_password").disabled = !get_by_id("email_enable").checked;
        get_by_id("email_pw2").disabled = !get_by_id("email_enable").checked;
        get_by_id("email_sch_enable").disabled = !get_by_id("email_enable").checked;
        get_by_id("by_email_sch").disabled = !get_by_id("email_enable").checked;
        if (!get_by_id("email_enable").checked) {
            get_by_id("email_schedule").disabled = !get_by_id("email_enable").checked;
            get_by_id("email_detail").disabled = !get_by_id("email_enable").checked;
        }
        else {
            get_by_id("email_schedule").disabled = !get_by_id("by_email_sch").checked;
            get_by_id("email_detail").disabled = !get_by_id("by_email_sch").checked;
        }
        disable_emai_auth();
    }

    function disable_schedule()
    {
        get_by_id("email_schedule").disabled = !get_by_id("by_email_sch").checked;
        get_by_id("email_detail").disabled = !get_by_id("by_email_sch").checked;
    }

    function check_date()
    {
        if (is_form_modified("form1") && !confirm(msg[IS_CHANGE_DATA])) {
            return;
        }

        location.href="tools_schedules.asp";
    }

    function send_request()
    {
        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }

        if (get_by_id("email_enable").checked) {
            if (!check_mail()) {
                return false;
            }
        }
        get_by_id("log_email_enable").value = get_checked_value(get_by_id("email_enable"));
        get_by_id("log_email_auth").value = get_checked_value(get_by_id("email_auth"));
        //save mail_schedule
        get_schedule_detail();
        get_by_id("log_email_schedule").value = get_checked_value(get_by_id("email_sch_enable")) +"/"+get_checked_value(get_by_id("by_email_sch"))+"/0/"+ get_by_id("email_schedule").value+"/"+ get_email_detail;
        return true;
    }
</script>

<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style4 {
    font-size: 11px;
    font-weight: bold;
}
.style5 {font-size: 11px}
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
                  <tbody>
                    <tr>
                        <td id="sidenav_container">
                            <div id="sidenav">
                            <!-- === BEGIN SIDENAV === -->
                                <ul>
									<li><div><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_admin)</script></a></div></li>
									<li><div><a href="tools_time.asp" onclick="return jump_if();"><script>show_words(_time)</script></a></div></li>
									<li><div><a href="tools_syslog.asp" onclick="return jump_if();"><script>show_words(_syslog)</script></a></div></li>
									<li><div id="sidenavoff"><script>show_words(_email)</script></div></li>
									<li><div><a href="tools_system.asp" onclick="return jump_if();"><script>show_words(_system)</script></a></div></li>
									<li><div><a href="tools_firmw.asp" onclick="return jump_if();"><script>show_words(_firmware)</script></a></div></li>
									<li><div><a href="tools_ddns.asp" onclick="return jump_if();"><script>show_words(_dyndns)</script></a></div></li>
                                    <li><div><a href="tools_vct.asp" onclick="return jump_if();"><script>show_words(_syscheck)</script></a></div></li>
									<li><div><a href="tools_schedules.asp" onclick="return jump_if();"><script>show_words(_scheds)</script></a></div></li>
                                </ul>
                                <!-- === END SIDENAV === -->
                            </div>
                        </td>
                    </tr>
                  </tbody>
                </table>
            </td>

            <input type="hidden" id="schedule_rule_0" name="schedule_rule_0" value="<% CmoGetCfg("schedule_rule_00","none"); %>">
            <input type="hidden" id="schedule_rule_1" name="schedule_rule_1" value="<% CmoGetCfg("schedule_rule_01","none"); %>">
            <input type="hidden" id="schedule_rule_2" name="schedule_rule_2" value="<% CmoGetCfg("schedule_rule_02","none"); %>">
            <input type="hidden" id="schedule_rule_3" name="schedule_rule_3" value="<% CmoGetCfg("schedule_rule_03","none"); %>">
            <input type="hidden" id="schedule_rule_4" name="schedule_rule_4" value="<% CmoGetCfg("schedule_rule_04","none"); %>">
            <input type="hidden" id="schedule_rule_5" name="schedule_rule_5" value="<% CmoGetCfg("schedule_rule_05","none"); %>">
            <input type="hidden" id="schedule_rule_6" name="schedule_rule_6" value="<% CmoGetCfg("schedule_rule_06","none"); %>">
            <input type="hidden" id="schedule_rule_7" name="schedule_rule_7" value="<% CmoGetCfg("schedule_rule_07","none"); %>">
            <input type="hidden" id="schedule_rule_8" name="schedule_rule_8" value="<% CmoGetCfg("schedule_rule_08","none"); %>">
            <input type="hidden" id="schedule_rule_9" name="schedule_rule_9" value="<% CmoGetCfg("schedule_rule_09","none"); %>">
            <input type="hidden" id="schedule_rule_10" name="schedule_rule_10" value="<% CmoGetCfg("schedule_rule_10","none"); %>">
            <input type="hidden" id="schedule_rule_11" name="schedule_rule_11" value="<% CmoGetCfg("schedule_rule_11","none"); %>">
            <input type="hidden" id="schedule_rule_12" name="schedule_rule_12" value="<% CmoGetCfg("schedule_rule_12","none"); %>">
            <input type="hidden" id="schedule_rule_13" name="schedule_rule_13" value="<% CmoGetCfg("schedule_rule_13","none"); %>">
            <input type="hidden" id="schedule_rule_14" name="schedule_rule_14" value="<% CmoGetCfg("schedule_rule_14","none"); %>">
            <input type="hidden" id="schedule_rule_15" name="schedule_rule_15" value="<% CmoGetCfg("schedule_rule_15","none"); %>">
            <input type="hidden" id="schedule_rule_16" name="schedule_rule_16" value="<% CmoGetCfg("schedule_rule_16","none"); %>">
            <input type="hidden" id="schedule_rule_17" name="schedule_rule_17" value="<% CmoGetCfg("schedule_rule_17","none"); %>">
            <input type="hidden" id="schedule_rule_18" name="schedule_rule_18" value="<% CmoGetCfg("schedule_rule_18","none"); %>">
            <input type="hidden" id="schedule_rule_19" name="schedule_rule_19" value="<% CmoGetCfg("schedule_rule_19","none"); %>">
            <input type="hidden" id="schedule_rule_20" name="schedule_rule_20" value="<% CmoGetCfg("schedule_rule_20","none"); %>">
            <input type="hidden" id="schedule_rule_21" name="schedule_rule_21" value="<% CmoGetCfg("schedule_rule_21","none"); %>">
            <input type="hidden" id="schedule_rule_22" name="schedule_rule_22" value="<% CmoGetCfg("schedule_rule_22","none"); %>">
            <input type="hidden" id="schedule_rule_23" name="schedule_rule_23" value="<% CmoGetCfg("schedule_rule_23","none"); %>">
            <input type="hidden" id="schedule_rule_24" name="schedule_rule_24" value="<% CmoGetCfg("schedule_rule_24","none"); %>">
            <input type="hidden" id="schedule_rule_25" name="schedule_rule_25" value="<% CmoGetCfg("schedule_rule_25","none"); %>">
            <input type="hidden" id="schedule_rule_26" name="schedule_rule_26" value="<% CmoGetCfg("schedule_rule_26","none"); %>">
            <input type="hidden" id="schedule_rule_27" name="schedule_rule_27" value="<% CmoGetCfg("schedule_rule_27","none"); %>">
            <input type="hidden" id="schedule_rule_28" name="schedule_rule_28" value="<% CmoGetCfg("schedule_rule_28","none"); %>">
            <input type="hidden" id="schedule_rule_29" name="schedule_rule_29" value="<% CmoGetCfg("schedule_rule_29","none"); %>">
            <input type="hidden" id="schedule_rule_30" name="schedule_rule_30" value="<% CmoGetCfg("schedule_rule_30","none"); %>">
            <input type="hidden" id="schedule_rule_31" name="schedule_rule_31" value="<% CmoGetCfg("schedule_rule_31","none"); %>">

            <form id="form1" name="form1" method="post" action="apply.cgi">
            <input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
            <input type="hidden" id="html_response_message" name="html_response_message" value="">
            <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
            <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="tools_email.asp">
            <input type="hidden" id="reboot_type" name="reboot_type" value="application">

            <input type="hidden" id="log_email_schedule" name="log_email_schedule" value="<% CmoGetCfg("log_email_schedule","none"); %>">
            <input type="hidden" id="log_email_enable" name="log_email_enable" value="<% CmoGetCfg("log_email_enable","none"); %>">

            <td valign="top" id="maincontent_container">
            	<div id="maincontent">
                    <!-- === BEGIN MAINCONTENT === -->
                <div id="box_header">
            <h1>
              <script>show_words(_email)</script>
            </h1>
            <script>show_words(te_intro_Em)</script>
            <br><br>
			<input name="button3" id="button3" type="submit" class=button_submit value="" onClick="return send_request()">
			<input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel('form1', 'tools_email.asp');">
			<script>check_reboot();</script>
			<script>get_by_id("button3").value = _savesettings;</script>
			<script>get_by_id("button2").value = _dontsavesettings;</script>
                </div>
                <br>
                <div class="box">
            <h2>
              <script>show_words(_enable)</script>
            </h2>
                    <table cellSpacing=1 cellPadding=2 width=525 border=0>
                      <tr>

                <td class="duple">
                  <script>show_words(te_EnEmN)</script>
                  :</td>
                        <td width="340">&nbsp;<input type="Checkbox" id="email_enable" name="email_enable" value="1" onClick="disable_notification();"></td>
                      </tr>
                    </table>
                </div>
                <div class="box">
            <h2>
              <script>show_words(te_EmSt)</script>
            </h2>

                    <table cellSpacing=1 cellPadding=2 width=525 border=0>
                      <tr>
                <td class="duple"> <script>show_words(te_FromEm)</script>
                  :</td>
                <td width="340">&nbsp; <input type=text id="log_email_sender" name="log_email_sender" size=30 maxlength=99 value="<% CmoGetCfg("log_email_sender","none"); %>">
                        </td>
                      </tr>
                      <tr>
                <td class="duple"> <script>show_words(te_ToEm)</script>
                  :</td>
                <td width="340">&nbsp; <input type=text id="log_email_recipien" name="log_email_recipien" size=30 maxlength=99 value="<% CmoGetCfg("log_email_recipien","none"); %>">
                        </td>
                      </tr>
                      <tr>
                <td class="duple"> <script>show_words(te_SMTPSv)</script>
                  :</td>
                <td width="340">&nbsp; <input type=text id="log_email_server" name="log_email_server" size=30 maxlength=99 value="<% CmoGetCfg("log_email_server","none"); %>">
                        </td>
                      </tr>
                      <tr>
                        <td class="duple"><script>show_words(te_SMTPSv_Port)</script> :</td>
                        <td width="340">&nbsp;
                        <input type=text id="log_email_server_port" name="log_email_server_port" size=30 maxlength=99 value="<% CmoGetCfg("log_email_server_port","none"); %>"></td>
                        </td>
                      </tr>
              <tr>
                <td class="duple"> <script>show_words(te_EnAuth)</script>
                  :</td>
                <td width="340">&nbsp;
                  <input type="Checkbox" id="email_auth" name="email_auth" value="1" onClick="disable_emai_auth();"></td>
                        <input type="hidden" id="log_email_auth" name="log_email_auth" value="<% CmoGetCfg("log_email_auth","none"); %>">
                      </tr>
                      <tr>
                <td class="duple"> <script>show_words(te_Acct)</script>
                  :</td>
                <td width="340">&nbsp; <input type=text id="log_email_username" name="log_email_username" size=30 maxlength=99 value="<% CmoGetCfg("log_email_username","none"); %>">
                        </td>
                      </tr>
                      <tr>
                <td class="duple"> <script>show_words(_password)</script>
                  :</td>
                <td width="340">&nbsp; <input type=password id="log_email_password" name="log_email_password" size=30 maxlength=99 value="<% CmoGetCfg("log_email_password","none"); %>">
                        </td>
                      </tr>
                      <tr>
                <td class="duple"> <script>show_words(_verifypw)</script>
                  :</td>
                <td width="340">&nbsp; <input type=password id="email_pw2" name="email_pw2" size=30 maxlength=99 value="<% CmoGetCfg("log_email_password","none"); %>">
                        </td>
                      </tr>
                    </table>
                </div>

                <div class="box">
            <h2>
              <script>show_words(te__title_EmLog)</script>
            </h2>
                    <table cellSpacing=1 cellPadding=2 width=525 border=0>
                      <tr>

                <td class="duple">
                  <script>show_words(te_OnFull)</script> :</td>
                        <td width="340">&nbsp;<input type="Checkbox" id="email_sch_enable" name="email_sch_enable" value="1"></td>
                      </tr>

                      <tr>

                <td class="duple">
                  <script>show_words(te_OnSch)</script>
                  :</td>
                        <td width="340">&nbsp;<input type="Checkbox" id="by_email_sch" name="by_email_sch" value="25" onClick="disable_schedule();"></td>
                      </tr>

                      <tr>
                        <td class="duple"><a href="javascript:check_date();"><script>show_words(_sched)</script></a> :</td>
                        <td width="340">&nbsp;
                            <select id="email_schedule" name="email_schedule" onClick="get_detail();">
	                        	<option value="Never"><script>show_words(_never)</script></option>
                                <script>
                                    document.write(set_schedule_option());
                                </script>
                            </select>
                        </td>
                      </tr>
                      <tr>

                <td class="duple">
                  <script>show_words(_aa_details)</script>
                  :</td>
                        <td width="340">&nbsp;&nbsp;<input type="text" id="email_detail" name="email_detail"></td>
                      </tr>
                    </table>
                </div>
            </div>
            </td>
        </form>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table cellpadding="2" cellspacing="0" border="0" bgcolor="#FFFFFF">
                  <tr>

          <td id="help_text"><strong>
            <script>show_words(_hints)</script>
            &hellip;</strong> <p>
              <script>show_words(hhte_intro)</script>
            </p>
					  <p><a href="support_tools.asp#EMail" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
                    </td>
                  </tr>
                </table></td>
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
	reboot_form();
	onPageLoad();
</script>
</html>
