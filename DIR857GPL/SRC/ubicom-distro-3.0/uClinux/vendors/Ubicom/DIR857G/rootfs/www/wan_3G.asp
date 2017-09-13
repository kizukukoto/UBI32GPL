<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript" src="isplist.js"></script>
<script>
	var submit_button_flag = 0;
	var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

	function onPageLoad()
	{
		set_checked("<% CmoGetCfg("usb3g_reconnect_mode","none"); %>", get_by_name("usb3g_reconnect_mode"));
		set_selectIndex("<% CmoGetCfg("usb3g_country","none"); %>", get_by_id("country"));
		set_selectIndex("<% CmoGetCfg("usb3g_auth","none"); %>", get_by_id("temp_usb3g_auth"));	
		display_isplist();	
		check_connectmode();
		set_form_default_values("form1");
		var login_who= "<% CmoGetStatus("get_current_user"); %>";
		if(login_who== "user"){
			DisableEnableForm(document.form1,true);	
		} 
	}
	
	function check_usb_type(){
		var wanmode = get_by_id("asp_temp_52").value;
		var usb_mode = get_by_id("from_usb3g").value;
		var usb_type = get_by_id("usb_type").value;
		
		var pop_1 = usb_config1;
        var pop_2 = usb_config2;

		if(usb_mode == "0" && usb_type == "2" && wanmode == "usb3g" || usb_mode == "2" && usb_type == "2" && wanmode == "usb3g"){  
		        alert(pop_2); // SharePort
        }else if(usb_mode == "1" && wanmode == "usb3g") { 
                alert(pop_2); // WCN
        }else if (usb_mode == "2" && usb_type == "0" && wanmode != "usb3g" || usb_mode == "0" && usb_type == "0" && wanmode != "usb3g") { 
                alert(pop_1); //3G USB
        }
     }
	
	function check_connectmode(){
		var conn_mode = get_by_name("usb3g_reconnect_mode");
		var idle_time = get_by_id("usb3g_max_idle_time");
		idle_time.disabled = !conn_mode[1].checked;
	}
	
	function send_3g_request(){
		get_by_id("asp_temp_52").value = get_by_id("wan_proto").value;
		if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
			return false;
		}
		
		var idle = get_by_id("usb3g_max_idle_time").value;	    	
		var mtu = get_by_id("usb3g_wan_mtu").value;
		var dns1 = get_by_id("wan_primary_dns").value;
	    var dns2 = get_by_id("wan_secondary_dns").value;
		
		var max_idle_msg = replace_msg(check_num_msg, bwn_MIT, 0, 9999);
        var mtu_msg = replace_msg(check_num_msg, bwn_MTU, 128, 1492);
        var dns1_addr_msg = replace_msg(all_ip_addr_msg,wwa_pdns);
		var dns2_addr_msg = replace_msg(all_ip_addr_msg,wwa_sdns);
		
		var temp_idle = new varible_obj(idle, max_idle_msg, 0, 9999, false);
        var temp_mtu = new varible_obj(mtu, mtu_msg, 128, 1492, false);
        var temp_dns1_obj = new addr_obj(dns1.split("."), dns1_addr_msg, false, false);
		var temp_dns2_obj = new addr_obj(dns2.split("."), dns2_addr_msg, true, false);
    	
    	var wanmode = get_by_id("wan_proto").selectedIndex;
    	var usb_mode = get_by_id("from_usb3g").value;
    	var usb_type = get_by_id("usb_type").value;
		var warn_1 = usb_config3; 		 
    	var warn_2 = usb_config4;
    	var pop_4 = usb_config5;

    	if(usb_mode == "0" && usb_type == "0" && wanmode =="5" ){ 
                get_by_id("usb_type").value = "2"; // force to save usb_type
                get_by_id("qos_enable").value = "0"; // Disable Qos		         	
                get_by_id("traffic_shaping").value = "0"; // Disable traffic_shaping	         	
        }else if(usb_mode == "0" && usb_type == "2" && wanmode =="5" ){ //3G USB
         		//alert(warn_2); //for fixed tsd bug#41 WWAN issue
                //return false;
        }else if(usb_type != "2" && wanmode !="5" ){ 
                get_by_id("usb_type").value = "2"; // force to save usb_type	
        }else if(usb_mode == "2" && wanmode =="5" ){ //3G USB
                get_by_id("usb_type").value = usb_mode; // force to save usb_type
                get_by_id("qos_enable").value = "0"; // Disable Qos		
                get_by_id("traffic_shaping").value = "0"; // Disable traffic_shaping		
        }        
          	     
       	if (!check_varible(temp_idle)){
    		return false;
    	}
    	
    	if (dns1 != "" && dns1 != "0.0.0.0"){
    		if (!check_address(temp_dns1_obj)){
    			return false;
    		}
    	}
    	
    	if (dns2 != "" && dns2 != "0.0.0.0"){
    		if (!check_address(temp_dns2_obj)){
    			return false;
    		}
    	}    	
    	if (!check_varible(temp_mtu)){
    		return false;
    	}		
		
		if((get_by_id("wan_primary_dns").value =="" || get_by_id("wan_primary_dns").value =="0.0.0.0")&& ( get_by_id("wan_secondary_dns").value =="" || get_by_id("wan_secondary_dns").value =="0.0.0.0")){
			get_by_id("wan_specify_dns").value = 0;
		}else{
			get_by_id("wan_specify_dns").value = 1;
		}

		if(submit_button_flag == 0){
			if(get_by_id("password").value !="WDB8WvbXdH"){
				get_by_id("usb3g_password").value = get_by_id("password").value;
			}
			if (!is_form_modified("form1")) {
				get_by_id("usb3g_isp").value = <% CmoGetCfg("usb3g_isp","none"); %>;
			}else{
				get_by_id("usb3g_isp").value = get_by_id("ispList").selectedIndex;
			}
			get_by_id("usb3g_country").value = get_by_id("country").value;
			get_by_id("usb3g_auth").value = get_by_id("temp_usb3g_auth").value;
			
			submit_button_flag = 1;
			return true;
		}else{
			return false;
		}
    }
    
    
</script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style1 {font-size: 11px}
-->
</style>
</head>

<body onload="onPageLoad();" topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
	<table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
      <tr>
        <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/" onclick="return jump_if();"><% CmoGetCfg("model_number","none"); %></a></td>
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
			<td id="topnavon"><a href="index.asp" onclick="return jump_3g_if();"><script>show_words(_setup)</script></a></td>
			<td id="topnavoff"><a href="adv_virtual.asp" onclick="return jump_3g_if();"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp" onclick="return jump_3g_if();"><script>show_words(_tools)</script></a></td>
			<td id="topnavoff"><a href="st_device.asp" onclick="return jump_3g_if();"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp" onclick="return jump_3g_if();"><script>show_words(_support)</script></a></td>
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
									<script>
										show_side_bar(0);
									</script>
								</ul>
							</div>
						</td>			
					</tr>
				</table>			
			</td>
			<td valign="top" id="maincontent_container">			
			<form id="form1" name="form1" method="post" action="apply.cgi">
				<input type="hidden" id="html_response_page" name="html_response_page" value="reboot.asp">
				<input type="hidden" id="html_response_message" name="html_response_message" value="">
				<script>get_by_id("html_response_message").value = sc_intro_sv;</script>
				<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="wan_3G.asp">
				<input type="hidden" id="usb3g_password" name="usb3g_password" value="WDB8WvbXdHtZyM8Ms2RENgHlacJghQyG">
				<input type="hidden" id="asp_temp_51" name="asp_temp_51" value="<% CmoGetCfg("asp_temp_51","none"); %>">
				<input type="hidden" id="asp_temp_52" name="asp_temp_52" value="<% CmoGetCfg("wan_proto","none"); %>">
				<input type="hidden" id="reboot_type" name="reboot_type" value="shutdown">
				<input type="hidden" id="usb3g_isp" name="usb3g_isp" value="<% CmoGetCfg("usb3g_isp","none"); %>">
				<input type="hidden" id="usb3g_country" name="usb3g_country" value="<% CmoGetCfg("usb3g_country","none"); %>">
			    <input type="hidden" id="usb3g_auth" name="usb3g_auth" value="<% CmoGetCfg("usb3g_auth","none"); %>"> 
			    <input type="hidden" id="usb_type" name="usb_type" value="<% CmoGetCfg("usb_type","none"); %>">
			    <input type="hidden" id="from_usb3g" name="from_usb3g" value="<% CmoGetCfg("asp_temp_72","none"); %>"> 
			    <input type="hidden" id="wan_specify_dns" name="wan_specify_dns" value="<% CmoGetCfg("wan_specify_dns","none"); %>">  
				<input type="hidden" id="qos_enable" name="qos_enable" value="<% CmoGetCfg("qos_enable","none"); %>">
				<input type="hidden" id="traffic_shaping" name="traffic_shaping" value="<% CmoGetCfg("traffic_shaping","none"); %>">
			<div id=maincontent>              
              <div id=box_header>
            <h1><script>show_words(_WAN)</script></h1>
                
            <p>
              <script>show_words(bwn_intro_ICS)</script>
            </p>
                
            <p><strong><b> 
              <script>show_words(_note)</script>
              </b>:&nbsp;</strong>
              <script>show_words(bwn_note_clientSW)</script>
            </p>
            <input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_3g_request()">
            <input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel('form1', 'sel_wan.asp');">
            <script>check_reboot();</script>
            <script>get_by_id("button").value = _savesettings;</script>
			<script>get_by_id("button2").value = _dontsavesettings;</script>
              </div>
              <div class=box>
                <h2><script>show_words(bwn_ict)</script></h2>
                <p class="box_msg"><script>show_words(bwn_msg_Modes)</script></p>
                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                    <tr>
                      <td align=right width="185" class="duple"><script>show_words(bwn_mici)</script> :</td>
                      <td width="331">&nbsp; 
                      <select name="wan_proto" id="wan_proto" onChange="change_wan()">
                        <option value="static"><script>show_words(_sdi_staticip)</script></option>
                        <option value="dhcpc"><script>show_words(bwn_Mode_DHCP)</script></option>
                        <option value="pppoe"><script>show_words(bwn_Mode_PPPoE)</script></option>
                        <option value="pptp"><script>show_words(bwn_Mode_PPTP)</script></option>
                        <option value="l2tp"><script>show_words(bwn_Mode_L2TP)</script></option>
						<option value="usb3g" selected><script>show_words(usb_3g)</script></option>
                         <!--option value="bigpond">BigPond (Australia)</option-->
                      </select></td>
                    </tr>
                </table>
              </div>

              <div class=box id=show_wwanICT >
                <h2><script>show_words(bwn_wwanICT)</script></h2>
				<p class="box_msg"><script>show_words(_ispinfo)</script></p>
                
            <table cellSpacing=1 cellPadding=1 width=525 border=0>
                 <input type=hidden id=get_data name=get_data>
                 <input type=hidden id=temp_country name=temp_country>
              <tr> 
                <td width="185" align=right class="duple"> <script>show_words(_country)</script> :</td>
                <td width="331">&nbsp; 
                   <select name=country id=country size=1 onChange="CountryList()">
	        		<option value="0">-- <script>show_words(_select_country)</script> --</option>
					<option value="999999"> -- None -- </option>
					<option value="61">Australia</option>
					<option value="55">Brasil</option> 
					<option value="2">Canada</option> 
					<option value="57">Colombia</option>
					<option value="45">Denmark</option>
					<option value="370">Dominican Republic</option>
					<option value="20">Egypt</option> 
					<option value="706">El Salvador</option> 					 					 					 	
					<option value="358">Finland</option>
					<option value="208">France</option>
					<option value="49">Germany</option>
					<option value="852">HongKong</option>
					<option value="62">Indonesia</option> 					 															
					<option value="972">Israel</option>
					<option value="39">Italy</option> 						
					<option value="639">Kenya</option>
					<option value="60">Malaysia</option> 						
					<option value="617">Mauritius</option>	
					<option value="52">Mexico</option>
					<option value="47">Norway</option>
					<option value="744">Paraguay</option>
					<option value="63">Philippine</option> 					
					<option value="48">Poland</option>
					<option value="351">Portugal</option>
					<option value="65">Singapore</option>
					<option value="27">South Africa</option> 					 										
					<option value="46">Sweden</option>
					<option value="886">Taiwan</option> 
					<option value="66">Thailand</option>																														
					<option value="44">UK</option> 
					<option value="1">USA</option> 

	    		</select>
                  </td>
              </tr>
              <tr> 
                <td align=right class="duple"> <script>show_words(_ipaddr)</script>
                  ISP :</td>
                <td>&nbsp; 
				<span id=subISP></span>
                  </td>
              </tr>
              <tr> 
                <td align=right class="duple"> <script>show_words(bwn_UN)</script>
                  :</td>
                <td>&nbsp; <input type=text id="usb3g_username" name="usb3g_username" size="20" maxlength="63" value="<% CmoGetCfg("usb3g_username","none"); %>"> 
                  <script>show_words(_optional)</script> 
                </td>
              </tr>
              <tr> 
                <td align=right class="duple"> <script>show_words(_password)</script>
                  :</td>
                <td>&nbsp; <input type=password id="password" name="password" size="20" maxlength="63" value="WDB8WvbXdH"> 
                  <script>show_words(_optional)</script> 
                </td>
              </tr>
              <!--tr> 
                <td align=right class="duple"> <script>show_words(_verifypw)</script>
                  : </td>
                <td>&nbsp; <input type=password id="confirm_usb3g_password" name="confirm_usb3g_password" size="20" maxlength="63" value="WDB8WvbXdH"> 
                  <script>show_words(_optional)</script> 
                </td>
              </tr-->
              <tr> 
                <td align=right class="duple"> <script>show_words(usb3g_dial_num)</script>
                  :</td>
                <td>&nbsp; <input type=text id="usb3g_dial_num" name="usb3g_dial_num" size="20" maxlength="63" value="<% CmoGetCfg("usb3g_dial_num","none"); %>"> 
                </td>
              </tr>
              <tr> 
                <td align=right class="duple"> 
                	<script>show_words(wwan_auth_label)</script> :</td>
                <td>&nbsp; <select id="temp_usb3g_auth">
                    <option value="0"><script>show_words(wwan_auth_auto)</script></option>
                    <option value="1"><script>show_words(wwan_auth_pap)</script></option>
                    <option value="2"><script>show_words(wwan_auth_chap)</script></option>
                    </select> </td>
              </tr>
              <tr> 
                <td align=right class="duple"> APN :</td>
                <td>&nbsp; <input id="usb3g_apn_name" size="20" maxlength="63"  name="usb3g_apn_name" type="text" value="<% CmoGetCfg("usb3g_apn_name","none"); %>"> 
                  <script>show_words(_optional)</script> 
                </td>
              </tr>
              <tr> 
                <td align=right class="duple"> <script>show_words(bwn_RM)</script>
                  :</td>
                <td>&nbsp; 
                <input type=radio name="usb3g_reconnect_mode" value="always_on" onClick="check_connectmode()">
                  <script>show_words(bwn_RM_0)</script>
					  <input type=radio name="usb3g_reconnect_mode" value="on_demand" onClick="check_connectmode()">
                  <script>show_words(bwn_RM_1)</script>
					  <input type=radio name="usb3g_reconnect_mode" value="manual" onClick="check_connectmode()">
                  <script>show_words(bwn_RM_2)</script>
                
                </td>
              </tr>
              <tr> 
                <td align=right class="duple"> <script>show_words(bwn_MIT)</script>
                  :</td>
                <td>&nbsp; <input type="text" name="usb3g_max_idle_time" id="usb3g_max_idle_time" size="10" maxlength="5" value="<% CmoGetCfg("usb3g_max_idle_time","none"); %>"> 
                  <script>show_words(bwn_min)</script> 
                </td>
              </tr>

              <tr>                      
                <td align=right class="duple">
                  <script>show_words(_dns1)</script>
                  :</td>
                      <td>&nbsp;
                      	<input type=text id="wan_primary_dns" name="wan_primary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_primary_dns","none"); %>">
                      </td>
                    </tr>
                    <tr>
                      
                <td align=right class="duple">
                  <script>show_words(_dns2)</script>
                  :</td>
                      <td>&nbsp;
                        <input type=text id="wan_secondary_dns" name="wan_secondary_dns" size="20" maxlength="15" value="<% CmoGetCfg("wan_secondary_dns","none"); %>">
    				  </td>
                    </tr>

              <tr> 
                <td align=right class="duple"> <script>show_words(bwn_MTU)</script>
                  :</td>
                <td>&nbsp; <input type="text" name="usb3g_wan_mtu" id="usb3g_wan_mtu" size="10" maxlength="5" value="<% CmoGetCfg("usb3g_wan_mtu","none"); %>" > 
                  <script>show_words(bwn_bytes)</script>
                  (128~1492)</td>
              </tr>
            </table>
        </div>
		</div>
	</form>
   </td>
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
                    <tr>
                      <td id=help_text><strong><script>show_words(_hints)</script>
            &hellip;</strong> <p>
              <script>show_words(LW35)</script>
            </p>
            <p><script>show_words(LW36)</script></p>
                          <p class="more"><a href="support_internet.asp#WAN" onclick="return jump_3g_if();"><script>show_words(_more)</script>&hellip;</a></p>
					  </td>
                    </tr>
			  </table></td>
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
	reboot_form();
	onPageLoad();
</script>
<!-- insert name=restore_wan_type -->
</html>