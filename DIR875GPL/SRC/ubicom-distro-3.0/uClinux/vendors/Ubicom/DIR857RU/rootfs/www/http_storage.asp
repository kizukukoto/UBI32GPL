<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<title>D-LINK CORPORATION, INC | WIRELESS ROUTER | HOME</title>
<meta http-equiv=Content-Type content="text/html; charset=UTF-8">
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script type="text/JavaScript" src="/jquery-1.6.1.min.js"></script>
<script type="text/Javascript" src="/jquery-DOMWindow.js"></script>
<script>
	var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";
</script>
<style>
.treeTable {
  margin: 0;
  padding: 0;
  border: 0px;
  vertical-align: top;
  z-index: 1;
}

#ltable{
  width: 100%;
  background-color: #FFFFFF;
  border: 0px solid #2F4F4F;
  padding: 0 0 0 0;
  margin: 0 0 0 0;
}

.selected {
  background-color: #3875d7;
  color: #fff;
}

img {
        display: inline;
        border-width: 0;
        border-style: none;
}

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
			<td id="sidenav_container" valign="top" width="125">
				<table border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td id="sidenav_container">
							<div id="sidenav">
								<ul>
								  <script>
								  	show_side_bar(5);
								  </script>
								</ul>
							</div>
						</td>			
					</tr>
				</table>			
			</td>
			<form id="form1" name="form1" method="post" action="apply.cgi">
			<input type="hidden" id="html_response_page" name="html_response_page" value="count_down.asp">
			<input type="hidden" id="html_response_message" name="html_response_message" value="">
			<script>get_by_id("html_response_message").value = sc_intro_sv;</script>
			<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="http_storage.asp">
			<input type="hidden" id="storage_account" name="storage_account" value="<% CmoGetCfg("storage_account","none"); %>"> 
			<input type="hidden" id="storage_access" name="storage_access" value="<% CmoGetCfg("storage_access","none"); %>"> 
			<input type="hidden" id="ddns_en" name="ddns_en" value="<% CmoGetCfg("ddns_enable","none"); %>"> 
			<input type="hidden" id="ddns_host" name="ddns_host" value="<% CmoGetCfg("ddns_hostname","none"); %>"> 
			<input type="hidden" id="action" name="action" value="http_storage">
			<td valign="top" id="maincontent_container">
				<div id=maincontent>				 
                  <div id=box_header>
                    <h1><script>show_words(STORAGE_title);</script></h1>
                  <p><script>show_words(STORAGE_intro);</script></p>
		         
                  <input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_request();">
                  <input name="button2" id="button2" type="reset" class=button_submit value="" onClick="return page_cancel_1('form1', 'http_storage.asp');">
		  <script>check_reboot();</script>
		  <script>get_by_id("button").value = _savesettings;</script>
		  <script>get_by_id("button2").value = _dontsavesettings;</script>
                                                     
		  </div>
                  <div class=box>
                    <h2><script>show_words(STORAGE_http_storage);</script></h2>
		    <table cellSpacing=1 cellPadding=1 width=525 border=0>
		      <tr>
		        <td class="duple"><script>show_words(STORAGE_en_web);</script> :</td>
		        <td width="340">&nbsp;
		        <input name="EnableWeb" type=checkbox id="EnableWeb" onClick="enable_file_access_chk();">
		        <input name="file_access_enable" type=hidden id="file_access_enable" value="<% CmoGetCfg("file_access_enable","none"); %>">
		        </td>
		      </tr>
		      <tr>
		        <td class="duple"><script>show_words(STORAGE_en_http_remote);</script> :</td>
		        <td width="340">&nbsp;
		          <input name="EnableRemote" type=checkbox id="EnableRemote" onClick="enable_remote_chk();">
		          <input name="file_access_remote" type=hidden id="file_access_remote" value="<% CmoGetCfg("file_access_remote","none"); %>">
		        </td>
		      </tr>
		      <tr>
		        <td class="duple"><script>show_words(STORAGE_http_port);</script> :</td>
		        <td width="340">
			  &nbsp;&nbsp;&nbsp;<input name="file_access_remote_port" type="text" id="file_access_remote_port" size="20" maxlength="15" value="<% CmoGetCfg("file_access_remote_port","none"); %>" onchange="show_link();">
			</td>
		      </tr>
		      <tr>
		        <td class="duple"><script>show_words(STORAGE_en_https_remote);</script> :</td>
		        <td width="340">&nbsp;
		          <input name="EnableHttps" type=checkbox id="EnableHttps" onClick="enable_https_chk();">
		          <input name="file_access_ssl" type=hidden id="file_access_ssl" value="<% CmoGetCfg("file_access_ssl","none"); %>">
		        </td>
		      </tr>
		      <tr>
		        <td class="duple"><script>show_words(STORAGE_https_port);</script> :</td>
		        <td width="340">
			  &nbsp;&nbsp;&nbsp;<input name="file_access_ssl_port" type="text" id="file_access_ssl_port" size="20" maxlength="15" value="<% CmoGetCfg("file_access_ssl_port","none"); %>" onchange="show_link();">
			</td>
		      </tr>
		    </table>
                  </div>
		  <div class=box>
		    <h2>10 -- <script>show_words(STORAGE_create_user);</script></h2>
		    <table cellSpacing=1 cellPadding=1 width=525 border=0>
		      <tr>
		        <td class="duple"><script>show_words(_username);</script> :</td>
		        <td width="340">
			  &nbsp;&nbsp;&nbsp;<input name="username" type="text" id="username" size="20" maxlength="15" value="">
			  &nbsp;&lt;&lt;&nbsp;<span id="show_user"></span>
			</td>
		      </tr>
		      <tr>
		        <td class="duple"><script>show_words(_password);</script> :</td>
		        <td width="340">
			  &nbsp;&nbsp;&nbsp;<input name="user_pw" type="password" id="user_pw" size="20" maxlength="15" value="">
			</td>
		      </tr>
		      <tr>
		        <td class="duple"><script>show_words(_verifypw);</script> :</td>
		        <td width="340">
			  &nbsp;&nbsp;&nbsp;<input name="verify_pw" type="password" id="verify_pw" size="20" maxlength="15" value="">
			  &nbsp;&nbsp;&nbsp;<input name="add_user" id="add_user" type="button" class=button_submit value="" onClick="set_user_list();">
			  <input name="del_user" id="del_user" type="button" class=button_submit value="" onClick="del_user_list();">
			  <script>get_by_id("add_user").value = _add_edit;</script>
			  <script>get_by_id("del_user").value = _delete;</script>
			</td>
		      </tr>
	 	    </table>
		  </div>
		  <div class=box>
		    <h2><script>show_words(_user_list);</script></h2>
		    <div style="float:right">
		        <img src="edit.jpg" alt="Modify">
			 : <script>show_words(_modify);</script>
		        <img src="delete.jpg" alt="Delete">
			 : <script>show_words(_delete);</script>
		    </div><br><br>
		    <span id="show_user_path"></span>
		  </div>
		  <div class=box>
		    <h2><script>show_words(_num_of_device);</script> : <% CmoGetStatus("storage_get_number_of_dev"); %></h2>
		    <table width="525" id="device_list" border=1 bgcolor="#DFDFDF" bordercolor="#FFFFFF">
		      <tr align="center" > 
		        <td width="300"><strong><script>show_words(STORAGE_device);</script></strong></td> 
		        <td><strong><script>show_words(STORAGE_totalspace);</script></strong></td> 
		        <td><strong><script>show_words(STORAGE_freespace);</script></strong></td> 
		      </tr>
		      <% CmoGetStatus("storage_get_space"); %>
		    </table>
		  </div>
		  <div class=box>
		    <h2><script>show_words(STORAGE_dev_link_a);</script></h2>
		        <p><script>show_words(STORAGE_dev_link_msg);</script></p>
			<center><span id="show_device_link"></span></center>
			<!--<center><a href="http://192.168.2.5/">http://192.168.2.5/</a></center>-->
		  </div>
		</td>
	      </form>
	      <td valign="top" width="150" id="sidehelp_container" align="left">
		<table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
		  <tr>
		    <td id=help_text><strong><script>show_words(_hints);</script>&hellip;</strong>
		      <p><script>show_words(STORAGE_support_help)</script> </p>
		      <!--  <p class="more"><a href="support_internet.asp#USB"><script>show_words(_more)</script>;</a></p> -->
		    </td>
		  </tr>
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
         <div id="copyright"><script>show_words(_copyright);</script></div>
         <br>

<div id="append" style=" display:none;" >
<h2><span id="j_show_append_new"></span></h2>
<table cellSpacing=1 cellPadding=1 width=525 border=0>
  <tr>
    <td class="duple"><span id="j_show_username"></span> :</td>
    <td width="340">
    &nbsp;&nbsp;&nbsp;<input name="append_username" type="text" id="append_username" size="20" maxlength="15" value="" disabled="true">
    </td>
  </tr>
  <tr>
    <td class="duple"><span id="j_show_devlink_t"></span> :</td>
    <td width="340">
    &nbsp;&nbsp;&nbsp;<span id="j_show_devlink"></span>
    </td>
  </tr>
  <tr>
    <td class="duple"><span id="j_show_folder"></span> :</td>
    <td width="340">
    &nbsp;&nbsp;&nbsp;<input name="append_folder" type="text" id="append_folder" size="30" value="" readonly="true">
    &nbsp;&nbsp;&nbsp;<span id="show_browse"></span>
    </td>
  </tr>
  <tr>
    <td class="duple"><span id="j_show_permission"></span> :</td>
    <td width="340">&nbsp;&nbsp;
    <span id="show_permission_sel"></span>
    </td>
  </tr>
</table>
<p align=center>
  <span id="show_append_btn"></span>
</p>
<center>
  <span id="show_ok_btn"></span>
  &nbsp;<input type="button" id="Cancel" name="Cancel" value="Cancel" class="closeDOMWindow">
</center>
</div>

<div id="tree" style="display:none;">
<span id="j_show_select_folder"></span>
<div class="treeTable">
<table id="ltable" cellspacing="0" width="100%">
  <tbody>
  </tbody>
</table>
</div>
</div>

</body>
<script language="JavaScript">
var submit_button_flag = 0;
var device_link = "";
var totalID=0;
var user_list = new Array();
var access_list = new Array();
var treeTable = new Array();
var access_pass = 0; 
	
	function enable_file_access_chk(){
		get_by_id("EnableRemote").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("file_access_remote_port").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("EnableHttps").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("file_access_ssl_port").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("username").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("username_sel").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("user_pw").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("verify_pw").disabled = !get_by_id("EnableWeb").checked;
		get_by_id("add_user").disabled = !get_by_id("EnableWeb").checked;

		if(get_by_id("EnableWeb").checked){
			enable_remote_chk();
			enable_https_chk();
		}
		show_link();
	}

	function enable_remote_chk(){
		get_by_id("file_access_remote_port").disabled = !get_by_id("EnableRemote").checked;
		show_link();
	}

	function enable_https_chk(){
		get_by_id("file_access_ssl_port").disabled = !get_by_id("EnableHttps").checked;
		show_link();
	}

	function get_user_list(){
		if (get_by_id("storage_account").value == "") {
			user_list = new Array();
			return;
		}
		user_list = get_by_id("storage_account").value.split("|");
		for(var i = 0; i < user_list.length; i++)
			user_list[i] = user_list[i].split("/");
	}

	function show_account(){
		var account_list = "<select id=\"username_sel\" onchange=\"show_account_info(this);\">";
		account_list += "<option value=\"0\" selected=\"selected\">" + _username + "</option>";
		for(var i = 0; i < user_list.length; i++)
			account_list += "<option value=\"" + (i + 1) + "\">" + user_list[i][0] + "</option>";
		account_list += "</select>";
		get_by_id("show_user").innerHTML = account_list;
		get_by_id("username").value = "";
		get_by_id("user_pw").value = "";
		get_by_id("verify_pw").value = "";
		get_by_id("del_user").disabled = true;
		
	}

	function show_account_info(obj){
		if (obj.selectedIndex == 0) {
			get_by_id("username").value = "";
			get_by_id("user_pw").value = "";
			get_by_id("verify_pw").value = "";
			get_by_id("del_user").disabled = true;
		} else {
			get_by_id("username").value = user_list[obj.selectedIndex - 1][0];
			get_by_id("user_pw").value = user_list[obj.selectedIndex - 1][1];
			get_by_id("verify_pw").value = user_list[obj.selectedIndex - 1][1];
			get_by_id("del_user").disabled = false;
		}
	}

	function merge_user_list(){
		var list_tmp = "";
		for (var i = 0; i < user_list.length; i++){
			if(user_list[i] == "")
				continue;
			else if(i > 0 && list_tmp != "")
				list_tmp += "|";

			list_tmp += user_list[i][0] + "/" + user_list[i][1];
		}
		get_by_id("storage_account").value = list_tmp;
	}

	function del_user_list(){
		var i = get_by_id("username_sel").value;
		user_list[i - 1] = "";
		access_list[i] = "";

		merge_user_list();
		merge_access_list();
		get_user_list();
		show_account();
		get_access_list();
		show_access();
	}

	function set_user_list(){
		var i = get_by_id("username_sel").value;
		var username = get_by_id("username").value;
		var password = get_by_id("user_pw").value;

		if(i == 0 && user_list.length >= 8){
			alert(STORAGE_max_account);
			return false;
		}

		if (username == ""){
			alert(FMT_INVALID_USERNAME);
			return false;
		} else if (check_legal_name(username) || Find_word(username,"\\") || Find_word(username,"|")){
			alert(_username + vpn_ipsec_illegal_name + " \\ |");
			return false
		}
		
		var list_len = user_list.length;
		for (var j = 0; j < list_len; j++ ) {
			if(i>0){
				if (username == "admin" || username == "guest") {
					alert(_duplicate_user);
					return false
				}
			}
			else{
				if (user_list[j][0] == username || username == "admin" || username == "guest") {
					alert(_duplicate_user);
					return false
				}
			}
		}

		if (password == "" ||  get_by_id("verify_pw").value == ""){
			alert(INVALID_PASSWORD);
			return false;
		} else if (password != get_by_id("verify_pw").value){
			alert(_pwsame);
			return false;
		} else if (check_legal_name(password) || Find_word(password,"\\") || Find_word(password,"|")){
			alert(_password + vpn_ipsec_illegal_name + " \\ |");
			return false
		}


		var i = get_by_id("username_sel").value;
		if(i == 0) {
			var list_tmp = [username, password];
			user_list.push(list_tmp);
			var access_tmp = new Array();
			access_tmp[0] = "none";
			access_list.push(access_tmp);
			merge_access_list();
			get_access_list();
			show_access();
		} else {
			user_list[i - 1][0] = username;
			user_list[i - 1][1] = password;
		}

		show_account();
		merge_user_list();
		show_access();
	}

	function get_access_list(){
		access_list = get_by_id("storage_access").value.split("|");
		for (var i = 0; i < access_list.length; i++) {
			access_list[i] = access_list[i].split("/");			
			for (var j = 0; j < access_list[i].length; j++) {
				var access_tmp = access_list[i][j].split("\\");
				access_list[i][j] = access_tmp;
			}
		}
	}

	function show_access(){
		var access_tmp = "<table width=\"525\" id=\"user_list\"><tr bgcolor=\"#7b9ac6\">";
		access_tmp += "<td width=\"30\" align=\"center\"><strong>" + _number + "</strong></td>";
		access_tmp += "<td width=\"80\"><strong>" + _username + "</strong></td>";
		access_tmp += "<td ><strong>" + STORAGE_access_path + "</strong></td>";
		access_tmp += "<td width=\"70\" colspan=3><strong>" + STORAGE_permission + "</strong></td>";
		access_tmp += "</tr><tr><td align=\"center\">1</td><td>admin</td><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;/</td>";
		access_tmp += "<td width=\"70\">" + STORAGE_rw + "</td><td width=\"20\">&nbsp;</td>";
		access_tmp += "<td width=\"20\">&nbsp;</td></tr>";

		for (var i = 0; i < access_list.length; i++) {
			var num = 2 + i;
			for (var j = 0; j < access_list[i].length; j++) {
				var path_num = 1 + j;
				access_tmp += "<tr><td align=\"center\">";
				if (path_num == 1)
					access_tmp += num;
				access_tmp += "</td><td>";
				if (i == 0 && j == 0)
					access_tmp += "guest";
				else if (j == 0)
					access_tmp += user_list[(i - 1)][0];
				access_tmp += "</td><td>";
				if (access_list[i][j] == "none")
					access_tmp += "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;" + _none;
				else 
					access_tmp += "(" + path_num + ")&nbsp;" + decodeURIComponent(access_list[i][j][0]);
				access_tmp += "</td><td>";
				if (i == 0 || access_list[i][j][1] != "rw")
					access_tmp += STORAGE_ro;
				else
					access_tmp += STORAGE_rw;
				access_tmp += "</td><td><img src=\"edit.jpg\" alt=\"Modify\"";
				access_tmp += " onClick=\"edit_append("+i+","+j+");\"></td><td>";
				if (access_list[i][j][0] != "none")
					access_tmp += "<img src=\"delete.jpg\" alt=\"Delete\" onClick=\"del_append("+i+","+j+");\">";
				access_tmp += "</td></tr>";
			}
			access_tmp += "<tr bgcolor=\"#DFDFDF\"><td height=1 colspan=6></td></tr>";
		}
		access_tmp += "</table>";
		get_by_id("show_user_path").innerHTML = access_tmp;
	}

	function del_append(i, j){
		if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            return false;
		}
		if (i == null)
			return false;
		if (get_by_id("EnableWeb").checked != true)
			return false;

		if (access_list[i].length == 1){
			access_list[i][j][0] = "none";
			access_list[i][j].pop();
		} else
			access_list[i].splice(j, 1);
		
		show_access();
		merge_access_list();
	}

	function edit_append(i, j){
		if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            return false;
		}        
		if (i == null)
			return false;
		if (get_by_id("EnableWeb").checked != true)
			return false;

		$.openDOMWindow({ 
			modal:1,
			overlay:1,
			height:220, 
			width:535, 	
			windowSourceID:'#append'
		}); 

		get_by_id("j_show_append_new").innerHTML = STORAGE_append_folder;
		get_by_id("j_show_username").innerHTML = _username;
		if (i == 0)
			get_by_id("append_username").value = "guest";
		else
			get_by_id("append_username").value = user_list[i - 1][0];
		get_by_id("j_show_devlink_t").innerHTML = STORAGE_dev_link_b;
		var devlink_tmp = "<input name=\"append_devlink\" type=\"text\" id=\"append_devlink\" size=\"40\"";
		devlink_tmp += " maxlength=\"15\" value=\"" + device_link + "\" readonly=\"true\">";
		get_by_id("j_show_devlink").innerHTML = devlink_tmp;
		get_by_id("j_show_folder").innerHTML = _folder;
		var browse_tmp = "<input type=\"button\" id=\"Browse\" name=\"Browse\" value=\"Browse\"";
		browse_tmp += " onClick=\"call_show_tree("+i+", "+j+");\">";
		get_by_id("show_browse").innerHTML = browse_tmp;
		if (j == "9")
			get_by_id("append_folder").value = "/";
		else if (access_list[i][j] == "none")
			get_by_id("append_folder").value = _none;
		else
			get_by_id("append_folder").value = decodeURIComponent(access_list[i][j][0]);
		get_by_id("j_show_permission").innerHTML = STORAGE_permission;
		var select_tmp = "<select id=\"permission_sel\"><option value=\"0\">" + STORAGE_ro + "</option>";
		select_tmp += "<option value=\"1\">" + STORAGE_rw + "</option>";
		get_by_id("show_permission_sel").innerHTML = select_tmp;
		if (i == 0)
			get_by_id("permission_sel").disabled = true;
		if (i == 0 || j == "9" || access_list[i][j] == "none")
			get_by_id("permission_sel").value = 0;
		else
			get_by_id("permission_sel").value = access_list[i][j][1] == "ro" ? 0 : 1;
		var append_tmp = "<input type=\"button\" id=\"new_append\" name=\"new_append\" value=\"" + STORAGE_append + "\"";
		append_tmp += " onClick=\"save_and_append_new("+i+", "+j+");\" ";
		if(get_by_id("append_folder").value == _none)
			append_tmp += "disabled=\"true\"";
		append_tmp += ">";
		get_by_id("show_append_btn").innerHTML = append_tmp;
		var ok_tmp = "<input type=\"submit\" id=\"OK\" name=\"OK\" value=\"&nbsp;" + _ok + "&nbsp;\"";
		ok_tmp += " onClick=\"save_append("+i+", "+j+");\">";
		get_by_id("show_ok_btn").innerHTML = ok_tmp;
	}

	function call_show_tree(i, j){
		var title = '<div style=float:right><input type="button" id="new_append" name="new_append" value="' + _cancel + '"';
		title += ' onClick=call_show_append("");></div>';
		title += '<h2>' + STORAGE_select_folder + '</h2>';
		get_by_id("j_show_select_folder").innerHTML = title;
		treeTable = new Array();
		$.ajax({
			type: "GET",
			url: "device.xml=file_access?left=:&0=0",
			dataType: "script",
			cache: false,
			complete: function(xml) {
				var tmp = eval(xml.responseText);
				if (typeof(tmp) === "undefined") {
					return;
				}
				var tbody = "";
				tbody += '<tr id="0">';
				tbody += '<td><span class="collapse" style="padding-left:19px;margin-left:-19px;">';
				tbody += '<img src="expand.png" /></span>';
				tbody += '<a onClick=call_show_append("/");><img src="directory.png"/>&nbsp;/&nbsp;</a>';
				tbody += '</td></tr>';
				var len = tmp.length;
				if (len < 2) {
					tbody += '<tr id="1">';
					tbody += '<td>';
					tbody += '<span class="expand" style="padding-left:39px;margin-left:-19px;">';
					tbody += '<img src="warning.png" /></span></a>';
					tbody += '<span>None USB Storage Insert.</span></a>';
					tbody += '</td></tr>';
					$("#ltable>tbody").append(tbody);
				}

				/* Add tbody content */
				for (var i = 0; i < len-1; i++) {
					var element = tmp[i]['id'] + "/" + tmp[i]['path'] + "/" + tmp[i]['name']+ "/" + tmp[i]['child'];
					var cnode = tmp[i]['id'].split("-");
					var c = cnode[1], f = cnode[0];
					tbody += '<tr id="' + f + '">';
					tbody += '<td>';
					if(tmp[i]['child'] == "1") {
						tbody += '<a onClick="expander('+f+');" >';
						tbody += '<span class="expand" style="padding-left:39px;margin-left:-19px">';
						tbody += '<img src="collapse.png" /></span></a>';
					}else {
						tbody += '<a onClick="expander('+c+');" style="text-decoration:none;">';
						tbody += '<span style="padding-left:51px;margin-left:-19px;">&nbsp;';
						tbody += '</span></a>';
					}

					tbody += '<a onClick=call_show_append("'+tmp[i]['path']+'");>';
					tbody += '<img src="directory.png" /><span>' + tmp[i]['name'] + '</span></a>';
					tbody += '</td></tr>';
					treeTable.push(element);
					totalID++;
				}
				$("#ltable>tbody").empty();
				$("#ltable>tbody").append(tbody);
				$.closeDOMWindow({ 
					windowSourceID: '#append'
				}); 
				setTimeout("show_tree("+i+","+j+")", 250);
			}	
		});
	}

function getFolder(id)
{
        var path = "";
        for (var i = 0; i < treeTable.length; i++)
        {
                var tmp = treeTable[i].split("/");
                var t = tmp[0].split("-");
                if (t[t.length-1] == id) {
                        path = tmp[1];
                        break;
                }
        }

        $.ajax({
                type: "GET",
                url: "device.xml=file_access?left="+path+"&"+id+"="+totalID,
                dataType: "script",
                cache: false,
                complete: function(xml) {
                        var tmp = eval(xml.responseText);
                        if (typeof tmp === "undefined")
                                return;
                        var len = tmp.length;
                        if (len < 2) 
                                return;

                        var space = ($("#"+id).find('span').eq(0).attr('style')).split(";");
                        var padding = "", margin = "", new_space = "";

                        if (space.length >= 2) {
                                padding = (space[0].split(":"))[1].split("px");
                                if (space[1] !== "")
                                        margin = (space[1].split(":"))[1].split("px");
                        }else {
                                padding = (space[0].split(":"))[1].split("px");
                        }

                        if (margin[0] != "") {
                                new_space = parseInt(padding[0]) + parseInt(margin[0]) + 30;
                        } else {
                                new_space = parseInt(padding[0]) + 30;
                        }
                        
                        /* Remove nodes that already exists and rebuild the tree . */
                        var pattern = "child-of-"+id;
                        var newbody = "";
                        $('#ltable>tbody tr').each(function(i){
                                var t = $(this).html();
                                var patt = new RegExp(pattern);
                                var nid = $(this).attr('id');
                                if (!patt.test(t)){
                                        newbody += '<tr id="'+ nid + '">' + $(this).html() + '</tr>';
                                }
                        });
                        $("#ltable>tbody").empty();
                        $("#ltable>tbody").append(newbody);

                        /* Add tbody content */
                        var tbody = "";
                        for (var i = 0; i < len-1; i++) {
                                var cnode = tmp[i]['id'].split("-");
                                var c = cnode[1], f = cnode[0];
                                tbody += '<tr id="' + c + '">';
                                tbody += '<td class="child-of-'+ f +'">';
                                if (tmp[i]['child'] == "1") {
                                        tbody += '<a onClick="expander('+c+');" style="text-decoration:none;">';
                                        tbody += '<span class="expand" style="padding-left:'+new_space+'px;margin-left:-19px;">&nbsp;';
                                        tbody += '<img src="collapse.png"/></span></a>';
                                }else {
                                        tbody += '<a onClick="expander('+c+');" style="text-decoration:none;">';
                                        tbody += '<span style="padding-left:'+ ( new_space+ 16 )+'px;margin-left:-19px;">&nbsp;';
                                        tbody += '</span></a>';
                                }
                                tbody += '<a onClick=call_show_append("'+tmp[i]['path']+'");>';
                                tbody += '<img src="directory.png"/><span>' + tmp[i]['name'] + '</span></a>';
                                tbody += '</td></tr>';
                                totalID++;
                                var flag = 1, index = 0;
                                for (var  j = 0; j < treeTable.length; j++) {
                                        var odata = treeTable[j];
                                        var opath = (odata.split("/"))[1];
                                        if (opath == tmp[i]['path']) {
                                                flag = 0; index = j;
                                        }
                                }
                                var element = tmp[i]['id'] + "/" + tmp[i]['path'] + "/" + tmp[i]['name']+ "/" + tmp[i]['child'];
                                if (flag)
                                        treeTable.push(element);
                                else {
                                        if (index != 0)
                                                treeTable[index] = element;
                                }
                        }
                        $(tbody).insertAfter($("#"+id));
                        $("#ltable>tbody>tr").each(function(i){
                                if($(this).find('span').eq(1).hasClass("selected")){
                                        $(this).find('span').eq(1).removeClass("selected");
                                }
                        });
                        $("#"+id).find('span').eq(1).addClass("selected");
                }
        });
}

function expander(id, path)
{
        if(($("#"+id).find('span').hasClass('expand') == false && $("#"+id).find('span').hasClass("collapse") == false)) {
                return;
        }

        if ($("#"+id).find('span').hasClass('expand')) {
                $("#"+id).find('span').removeClass('expand').addClass('collapse');
                $("#"+id+">td>a>span>img").attr('src', 'expand.png');
                getFolder(id);
        }else{
                $("#"+id).find('span').removeClass('collapse').addClass('expand');
                $("#"+id+">td>a>span>img").attr('src', 'collapse.png');
                var pattern = "child-of-"+id;
                var newbody = "";
                var tmp = new Array();
                $('#ltable>tbody tr').each(function(i){
                        var t = $(this).html();
                        var patt = new RegExp(pattern);
                        var nid = $(this).attr('id');

                        /* Be sure to remove the nodes that has still child nodes */
                        if (tmp.length != 0) {
                                var cflag = 0;
                                for(var i = 0; i < tmp.length; i++) {
                                        var newpattern = 'child-of-'+tmp[i];
                                        var newpatt = new RegExp(newpattern);
                                        if (newpatt.test(t)) {
                                                cflag = 1;
                                                break;
                                        }
                                }
                                if (cflag) {
                                        tmp.push(nid);
                                        return;
                                }
                        }

                        if (!patt.test(t)){
                                newbody += '<tr id="'+ nid + '">' + $(this).html() + '</tr>';
                        }else {
                                tmp.push(nid);
                        }
                });
                $("#ltable>tbody").empty();
                $("#ltable>tbody").append(newbody);
                
                $("#ltable>tbody>tr").each(function(i){
                        if($(this).find('span').eq(1).hasClass("selected")){
                                $(this).find('span').eq(1).removeClass("selected");
                        }
                });
                $("#"+id).find('span').eq(1).addClass("selected");
        }
}

	function call_show_append(path){
		$.closeDOMWindow({ 
			windowSourceID: '#tree'
		}); 
		if (path != "")
			get_by_id("append_folder").value = decodeURIComponent(path);

		if(get_by_id("append_folder").value == _none)
			get_by_id("new_append").disabled = true;
		else
			get_by_id("new_append").disabled = false;
		setTimeout("show_append()", 250);
	}

	function show_tree(i, j){
		$.openDOMWindow({ 
			modal:1,
			height:500, 
			width:525, 	
			windowSourceID:'#tree' 
		}); 
	}

	function show_append(){
		$.openDOMWindow({ 
			modal:1,
			height:220, 
			width:535, 	
			windowSourceID:'#append' 
		}); 
	}

	function save_append(i, j){
		if (i == null)
			return false;

		$.closeDOMWindow({ 
			windowSourceID: '#append'
		}); 
		if(j == "9"){
			var access_tmp = new Array();
			access_tmp[0] = encodeURIComponent(get_by_id("append_folder").value);
			access_tmp[1] = (get_by_id("permission_sel").value == 0) ? "ro" : "rw";
			access_list[i].push(access_tmp);
		} else if (get_by_id("append_folder").value != "none"){
			access_list[i][j][0] = encodeURIComponent(get_by_id("append_folder").value);
			access_list[i][j][1] = (get_by_id("permission_sel").value == 0) ? "ro" : "rw";
		}
		
		show_access();
		merge_access_list();
	}

	function save_and_append_new(i, j){
		if (i == null)
			return false;

		if(j == "9"){
			var access_tmp = new Array();
			access_tmp[0] = encodeURIComponent(get_by_id("append_folder").value);
			access_tmp[1] = (get_by_id("permission_sel").value == 0) ? "ro" : "rw";
			access_list[i].push(access_tmp);
		} else {
			access_list[i][j][0] = encodeURIComponent(get_by_id("append_folder").value);
			access_list[i][j][1] = (get_by_id("permission_sel").value == 0) ? "ro" : "rw";
		}
		if(i != 0 && access_list[i].length == 5){
			alert(STORAGE_account_paths_max);
			$.closeDOMWindow({ 
				windowSourceID: '#append'
			}); 
			show_access();
		} else if (i == 0 && access_list[i].length == 2) {
			alert(STORAGE_guest_paths_max);
			$.closeDOMWindow({ 
				windowSourceID: '#append'
			}); 
			show_access();
		} else {
			$.closeDOMWindow({ 
				windowSourceID: '#append'
			}); 
			setTimeout("edit_append("+i+", 9)", 250);
		}

		show_access();
		merge_access_list();
	}

	function merge_access_list(){
		var access_tmp = "";
		for (var i = 0; i < access_list.length; i++){
			if(access_list[i] == "")
				continue;
			else if(i > 0 && access_tmp != "")
				access_tmp += "|";

			for (var j = 0; j < access_list[i].length; j++){
				if (access_list[i][j] == "none")
					access_tmp += "none";
				else 
					access_tmp += access_list[i][j][0] + "\\" + access_list[i][j][1];
				if ((access_list[i].length != 1) && (j != (access_list[i].length - 1)))
					access_tmp += "/";
			}
		}
		if(access_tmp.length > 240){
			access_pass = 1;
		}
		get_by_id("storage_access").value = access_tmp;
	}

    function createRequest()
    {
        var XMLhttpObject = null;
        try {
            XMLhttpObject = new XMLHttpRequest();
        } catch (e) {
            try {
                XMLhttpObject = new ActiveXObject("Msxml2.XMLHTTP");
            } catch (e) {
                try {
                    XMLhttpObject = new ActiveXObject("Microsoft.XMLHTTP");
                } catch (e) {
                    return null;
                }
            }
        }

        return XMLhttpObject;
    }

	var send_link_id;
	var send_link_count;

    function send_device_link()
    {
        xmlhttp = createRequest();
        if (xmlhttp) {
            var web_url;
            var lan_ip = "<% CmoGetCfg("lan_ipaddr","none"); %>";
            var temp_cURL = document.URL.split("/");
            var mURL = temp_cURL[2];
            var hURL = temp_cURL[0];

            if (mURL == lan_ip) {
                if (hURL == "https:")
                    web_url = "https://" + lan_ip + "/device.xml=send_device_link";
                else
                    web_url = "http://" + lan_ip + "/device.xml=send_device_link";
            }
            else {
                if (hURL == "https:")
                    web_url = "https://" + mURL + "/device.xml=send_device_link";
                else
                    web_url = "http://" + mURL + "/device.xml=send_device_link";
            }

            if (send_link_count == 0) {
                send_link_count = 1;
                web_url += "_init";
            }

            xmlhttp.onreadystatechange = cable_info_xmldoc;
            xmlhttp.open("GET", web_url, true);
            xmlhttp.send(null);
        }

        send_link_id = setTimeout("send_device_link()", 5000);
    }

    function cable_info_xmldoc()
    {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            var mail_status = xmlhttp.responseXML.getElementsByTagName("status")[0].firstChild.nodeValue;

            if (mail_status == "1") {
                get_by_id("mail_msg").innerHTML = _success;
                clearTimeout(send_link_id);
            }
            else if (mail_status == "-1") {
                get_by_id("mail_msg").innerHTML = _wifisc_addfail;
                clearTimeout(send_link_id);
            }
        }
    }

	function pre_send_mail(query){
		get_by_id("mail_msg").innerHTML = STORAGE_send_mail;
		get_by_id("mailnow").disabled = true;

		send_link_count = 0;
		send_device_link();
	}

	function show_link(){
		var ddns_enable = "<% CmoGetCfg("ddns_enable","none"); %>";
		var ddns_hostname = "<% CmoGetCfg("ddns_hostname","none"); %>";
		var access_enable = (get_by_id("EnableWeb").checked) ? 1 : 0;
		var remote_enable = (get_by_id("EnableRemote").checked) ? 1 : 0;
		var remote_port = get_by_id("file_access_remote_port").value;
		var ssl_enable = (get_by_id("EnableHttps").checked) ? 1 : 0;
		var ssl_port = get_by_id("file_access_ssl_port").value;
		var lan_ip = "<% CmoGetStatus("storage_wan_ip"); %>";
		var href = "";
		var link = "";
		var port = "";
		var dev_link = "";
		var link_query = "";
		var remote_port_msg = replace_msg(check_num_msg, STORAGE_http_port, 1, 65535);
		var ssl_port_msg = replace_msg(check_num_msg, STORAGE_https_port, 1, 65535);
		var remote_port_obj = new varible_obj(remote_port, remote_port_msg, 1, 65535, false);
		var ssl_port_obj = new varible_obj(ssl_port, ssl_port_msg, 1, 65535, false);

		if (access_enable != 1)
			;
		else {
			if (!check_varible(remote_port_obj)){
				return false;
			}
			if (!check_varible(ssl_port_obj)){
				return false;
			}

			if (ddns_enable == 1) {
				link = ddns_hostname;
			} else {
				link = lan_ip;
			}
			
			device_link = "";
			dev_link = "<table width=\"500\"><tr><td>";
			if (ssl_enable == 1 && ssl_port.length != 0) {
				href = "https";
				port = ssl_port;
				dev_link += "<p><a href=\"" + href + "://" + link + ":" + port + "/\" target=\"_blank\">";
				dev_link += href + "://" + link + ":" + port + "/</a></p>";
				device_link = "https://" + link + ":" + port + "/";
				link_query = "https=" + encodeURIComponent("https://" + link + ":" + port + "/");
			}
			if (remote_enable == 1 && remote_port.length != 0) {
				href = "http";
				port = remote_port;
				dev_link += "<p><a href=\"" + href + "://" + link + ":" + port + "/\" target=\"_blank\">";
				dev_link += href + "://" + link + ":" + port + "/</a></p>";
				if (device_link == "")
					device_link = "http://" + link + ":" + port + "/";
				if (link_query != "")
					link_query += "&";
				link_query += "http=" + encodeURIComponent("http://" + link + ":" + port + "/");
			}
			if (href != "" && link != "" && port != "") {
				dev_link += "</td><td><input name=\"mailnow\" id=\"mailnow\" type=\"button\"";
				dev_link += "value=\"Email Now\" onClick=pre_send_mail(\"" + link_query + "\");";
				if ("<% CmoGetCfg("log_email_enable","none"); %>" == "0")
					dev_link += " disabled=true";
				dev_link += ">&nbsp;<span id=\"mail_msg\"></span></td></tr></table>";
			} else
				dev_link = "";
		}
		get_by_id("show_device_link").innerHTML = dev_link;
	}

function checksessionStorage()
{
	/* Because old browsers (it's likes IE5.5, IE6, ...etcs.) not support HTML5 function, we just do it with old arch. */
	if (typeof(sessionStorage) == "undefined") {
		return "<% CmoGetStatus("get_current_user"); %>";
	} else {
		return sessionStorage.getItem("account");
	}
}

	function onPageLoad(){
		var login_who= checksessionStorage();

		get_by_id("EnableWeb").checked = (get_by_id("file_access_enable").value == 1) ? true : false;
		get_by_id("EnableRemote").checked = (get_by_id("file_access_remote").value == 1) ? true : false;
		get_by_id("EnableHttps").checked = (get_by_id("file_access_ssl").value == 1) ? true : false;

		get_user_list();
		show_account();
		get_access_list();
		show_access();

		enable_file_access_chk();
		
		if(login_who== "user"){
			DisableEnableForm(document.form1,true);	
		}
		if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1, true);
            get_by_id("del_user").disabled = true;
            get_by_id("username_sel").disabled = true;
        }

		set_form_default_values("form1");
	}
	
	function send_request(){
		if (get_by_id("EnableWeb").checked == true) {
			if (get_by_id("EnableRemote").checked != true && get_by_id("EnableHttps").checked != true) {
				alert(STORAGE_en_least_one);
				return false;
			}
		}

		var remote_port = get_by_id("file_access_remote_port").value;
		var ssl_port = get_by_id("file_access_ssl_port").value;

		var remote_port_msg = replace_msg(check_num_msg, STORAGE_http_port, 1, 65535);
		var ssl_port_msg = replace_msg(check_num_msg, STORAGE_https_port, 1, 65535);
		var remote_port_obj = new varible_obj(remote_port, remote_port_msg, 1, 65535, false);
		var ssl_port_obj = new varible_obj(ssl_port, ssl_port_msg, 1, 65535, false);

		if (!check_varible(remote_port_obj)){
			return false;
		}
		if (!check_varible(ssl_port_obj)){
			return false;
		}

		//check  management port and storage port conflict problem
		var chk_port ="<% CmoGetCfg("remote_http_management_port", "none"); %>";
                if (remote_port == chk_port ) {
                        alert(STORAGE_portconflict_st +" "+ tool_admin_check);  
                        return false;
                }

                if (ssl_port == chk_port ) {
                        alert(STORAGE_portconflict_sl +" "+ tool_admin_check);  
                        return false;
                }

                if (ssl_port == remote_port ) {
                        alert(STORAGE_portconflict_stl +" "+ tool_admin_check); 
                        return false;
                }


		get_by_id("file_access_enable").value = (get_by_id("EnableWeb").checked == true) ? 1 : 0;
		get_by_id("file_access_remote").value = (get_by_id("EnableRemote").checked == true) ? 1 : 0;
		get_by_id("file_access_ssl").value = (get_by_id("EnableHttps").checked == true) ? 1 : 0;

		if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
			return false;
		}
		
		if(access_pass == 1)
			return false;
		
		if(submit_button_flag == 0){
			submit_button_flag = 1;			
			send_submit("form1");
		}
		
	}	

//reboot_needed(left["Setup"].link[5]);
reboot_form();
onPageLoad();
</script>
</html>
