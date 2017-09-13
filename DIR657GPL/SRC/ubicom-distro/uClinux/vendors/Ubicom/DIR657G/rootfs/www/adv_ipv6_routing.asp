<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript" src="public_ipv6.js"></script>
<script language="JavaScript">

var submit_button_flag = 0;
var rule_max_num = 10;
var resert_rule = rule_max_num;
var DataArray = new Array();
var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

function onPageLoad()
{
	if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
		DisableEnableForm(document.form1,true);
	}
	
	for(var i = 0; i < rule_max_num; i++)
		click_null(i);

	set_form_default_values("form1");
}


function click_null(idx)
{
	var interface = get_by_id("interface" + idx).selectedIndex;
	if (interface == 0 || interface == 3) 
		get_by_id("Sub_gateway" + idx).disabled = true;
	if (interface == 2 || interface == 1) 
		get_by_id("Sub_gateway" + idx).disabled = false;
	if (interface == 3)
		get_by_id("metric" + idx).disabled = true;
	else
		get_by_id("metric" + idx).disabled = false;
}

function Data(enable, name, ip_addr, net_mask, gateway, interface, metric, onList)
{
	this.Enable = enable;
	this.Name = name;
	this.Ip_addr = ip_addr;
	this.Net_mask = net_mask;
	this.Gateway = gateway;
	this.Interface = interface;
	this.Metric = metric;
	this.OnList = onList ;
}

function set_routes()
{
	var index = 0;
	for (var i = 0; i < rule_max_num; i++) {
		var temp_st;
		var k = i;
		if (parseInt(i, 10) < 10) {
			k = "0" + i;
		}

		temp_st = (get_by_id("static_routing_ipv6_" + k).value).split("/");
		if (temp_st.length > 1) {
			if (temp_st[1] != "name" && temp_st[0] != "") {
				DataArray[DataArray.length++] = new Data(temp_st[0], temp_st[1],
					temp_st[2], temp_st[3], temp_st[4], temp_st[5], temp_st[6],
					temp_st[7], index);
				index++;
			}
		}
	}
}

function send_request()
{
	if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
		return false;
	}
	var count = 0;
	for (var i = 0; i < rule_max_num; i++) {
		var temp_mac;
		var enable = get_checked_value(get_by_id("enable" + i));

		if (i > 9)
			get_by_id("static_routing_ipv6_" + i).value = "0///64//NULL/";
		else
			get_by_id("static_routing_ipv6_0" + i).value = "0///64//NULL/";


		if(enable == 1){
			var ipv6_static_msg = replace_msg(all_ipv6_addr_msg,"IPv6 address");
			var dst_ip = get_by_id("Destination"+i).value;
			var gw_ip = get_by_id("Sub_gateway"+i).value;
			var temp_dst_ip = new ipv6_addr_obj(dst_ip.split(":"), ipv6_static_msg, false, false);
			var temp_gw_ip = new ipv6_addr_obj(gw_ip.split(":"), ipv6_static_msg, false, false);
			var prefix_length_msg = replace_msg(check_num_msg, "Subnet Prefix Length", 0, 128);
			var prefix_length_obj = new varible_obj(get_by_id("Sub_Mask"+i).value, prefix_length_msg, 0, 128, false);
			var interface = get_by_id("interface" + i).selectedIndex;
			var metric = get_by_id("metric"+ i).value;
			var metric_msg = replace_msg(check_num_msg, "Metric", 1, 16);
			var temp_metric = new varible_obj(metric, metric_msg, 1, 16, false);

				if(interface != 3)
					if (!check_varible(temp_metric))
						return false;
				var temp_port_name = get_by_id("name" + i).value;
				if(temp_port_name != ""){
					if (Find_word(temp_port_name,'"') || Find_word(temp_port_name,"/")) {
                				alert(_name + " " + (i+1) + " " + illegal_characters + " " + temp_port_name);
		                        	return false;   
		                	}
				}else{
					alert(YM49+" "+(i+1)+".");
					return false;
				}

				for (var j = i+1; j < rule_max_num; j++) {
					if (temp_port_name == get_by_id("name" + j).value) {
						alert(TEXT084);
						return false;
					}
					if(interface == 3 && get_by_id("interface" + j).selectedIndex == 3){
						if(dst_ip == get_by_id("Destination" + j).value)
							if(get_by_id("Sub_Mask" + i).value == get_by_id("Sub_Mask" + j).value){
								alert(TEXT085);
						return false;
					}
						if(get_by_id("Sub_Mask" + i).value > 64){
							alert(TEXT086)
							return false
						}
					}
				}

		                if(check_ipv6_symbol(dst_ip,"::") == 2){ // find two '::' symbol
                		        return false;
		                }else if(check_ipv6_symbol(dst_ip,"::") == 1){    // find one '::' symbol
                		        temp_dst_ip = new ipv6_addr_obj(dst_ip.split("::"), ipv6_static_msg, false, false);
					if(temp_dst_ip.addr[temp_dst_ip.addr.length-1].length == 0)
						temp_dst_ip.addr[temp_dst_ip.addr.length-1] = "1111";
		                        if (!check_ipv6_address(temp_dst_ip,"::")){
		                                return false;
                	        	}
		                }else{  //not find '::' symbol
                        		temp_dst_ip = new ipv6_addr_obj(dst_ip.split(":"), ipv6_static_msg, false, false);
		                        if (!check_ipv6_address(temp_dst_ip,":")){
                		                return false;
	                       		}
	        	        }

				if(interface != 0 && interface !=3){
					if(check_ipv6_symbol(gw_ip,"::") == 2){ // find two '::' symbol
        	                        	return false;
			                }else if(check_ipv6_symbol(gw_ip,"::") == 1){    // find one '::' symbol
                	        	        temp_gw_ip = new ipv6_addr_obj(gw_ip.split("::"), ipv6_static_msg, false, false);
                        		        if (!check_ipv6_address(temp_gw_ip,"::")){
                	                	        return false;
		                                }
	        		        }else{  //not find '::' symbol
                        		        temp_gw_ip = new ipv6_addr_obj(gw_ip.split(":"), ipv6_static_msg, false, false);
                        	        	if (!check_ipv6_address(temp_gw_ip,":")){
                	                        	return false;
		                                }
	        		        }

	        		        if (!check_varible(prefix_length_obj)){
        		        	        return false;
			                }
				}
			}

			if (count > 9)
				temp_mac = get_by_id("static_routing_ipv6_" + count);
			else
				temp_mac = get_by_id("static_routing_ipv6_0" + count);

			if(interface == 3)
				get_by_id("metric" + i).value = "1";

			temp_mac.value = get_checked_value(get_by_id("enable"+i))+"/"+ get_by_id("name" + i).value +"/"
				+ get_by_id("Destination" + i).value + "/"+ get_by_id("Sub_Mask" + i).value +"/"
				+ get_by_id("Sub_gateway" + i).value +"/"+ get_by_id("interface" + i).value + "/"+get_by_id("metric" + i).value;
			count++;
		}

	if (submit_button_flag == 0) {
		submit_button_flag = 1;
		return true;
	}
		return false;
}
</script>    

<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
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
			<td id="topnavon"><a href="adv_virtual.asp" onclick="return jump_if();"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_tools)</script></a></td>
			<td id="topnavoff"><a href="st_device.asp" onclick="return jump_if();"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp" onclick="return jump_if();"><script>show_words(_support)</script></a></td>
		</tr>
</table>
	<table border="1" cellpadding="2" cellspacing="0" width="838" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
		<tr>
			<td id="sidenav_container" valign="top" width="125" align="right">
				<table cellSpacing=0 cellPadding=0 border=0>
                  <tbody>
                    <tr>
                      <td id=sidenav_container>
                        <div id=sidenav>
                          <UL>
                          	<LI><div><a href="adv_virtual.asp" onClick="return jump_if();"><script>show_words(_virtserv)</script></a></div></LI>
                            <LI><div><a href="adv_portforward.asp" onclick="return jump_if();"><script>show_words(_pf)</script></a></div></LI>
                            <LI><div><a href="adv_appl.asp" onclick="return jump_if();"><script>show_words(_specappsr)</script></a></div></LI>
                            <LI><div><a href="adv_qos.asp" onclick="return jump_if();"><script>show_words(YM48)</script></a></div></LI>
                            <LI><div><a href="adv_filters_mac.asp" onclick="return jump_if();"><script>show_words(_netfilt)</script></a></div></LI>
                            <LI><div><a href="adv_access_control.asp" onclick="return jump_if();"><script>show_words(_acccon)</script></a></div></LI>
                            <LI><div><a href="adv_filters_url.asp" onclick="return jump_if();"><script>show_words(_websfilter)</script></a></div></LI>
                            <LI><div><a href="Inbound_Filter.asp" onclick="return jump_if();"><script>show_words(_inboundfilter)</script></a></div></LI>
                            <LI><div><a href="adv_dmz.asp" onclick="return jump_if();"><script>show_words(_firewalls)</script></a></div></LI>
                            <LI><div><a href="adv_routing.asp" onclick="return jump_if();"><script>show_words(_routing)</script></a></div></LI>
                            <LI><div><a href="adv_wlan_perform.asp" onclick="return jump_if();"><script>show_words(_adwwls)</script></a></div></LI>
                            <LI><div><a href="adv_wish.asp" onclick="return jump_if();">WISH</a></div></LI>
                            <LI><div><a href="adv_wps_setting.asp" onclick="return jump_if();"><script>show_words(LY2)</script></a></div></LI>
                            <LI><div><a href="adv_network.asp" onclick="return jump_if();"><script>show_words(_advnetwork)</script></a></div></LI>
                            <LI><div><a href="guest_zone.asp" onclick="return jump_if();"><script>show_words(_guestzone)</script></a></div></LI>
                            <LI><div><a href="adv_ipv6_firewall.asp" onclick="return jump_if();"><script>show_words(Tag05762)</script></div></LI>
                	    <LI><div id=sidenavoff><script>show_words(IPV6_routing)</script></div></LI>    
                          	
                          	
                          </UL>
                      	</div>
                      </td>
                    </tr>
                  </tbody>
                </table></td>

                <form id="form1" name="form1" method="post" action="apply.cgi">
                <input type="hidden" id="html_response_page" name="html_response_page" value="back_long.asp">
                <input type="hidden" id="html_response_message" name="html_response_message" value="The setting is saved.">
                <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="adv_ipv6_routing.asp">
				<input type="hidden" id="reboot_type" name="reboot_type" value="all">

                <!-- New Variable -->
				<input type="hidden" id="static_routing_ipv6_00" name="static_routing_ipv6_00" value="<% CmoGetCfg("static_routing_ipv6_00","none"); %>">
				<input type="hidden" id="static_routing_ipv6_01" name="static_routing_ipv6_01" value="<% CmoGetCfg("static_routing_ipv6_01","none"); %>">
				<input type="hidden" id="static_routing_ipv6_02" name="static_routing_ipv6_02" value="<% CmoGetCfg("static_routing_ipv6_02","none"); %>">
				<input type="hidden" id="static_routing_ipv6_03" name="static_routing_ipv6_03" value="<% CmoGetCfg("static_routing_ipv6_03","none"); %>">
				<input type="hidden" id="static_routing_ipv6_04" name="static_routing_ipv6_04" value="<% CmoGetCfg("static_routing_ipv6_04","none"); %>">
				<input type="hidden" id="static_routing_ipv6_05" name="static_routing_ipv6_05" value="<% CmoGetCfg("static_routing_ipv6_05","none"); %>">
				<input type="hidden" id="static_routing_ipv6_06" name="static_routing_ipv6_06" value="<% CmoGetCfg("static_routing_ipv6_06","none"); %>">
				<input type="hidden" id="static_routing_ipv6_07" name="static_routing_ipv6_07" value="<% CmoGetCfg("static_routing_ipv6_07","none"); %>">
				<input type="hidden" id="static_routing_ipv6_08" name="static_routing_ipv6_08" value="<% CmoGetCfg("static_routing_ipv6_08","none"); %>">
				<input type="hidden" id="static_routing_ipv6_09" name="static_routing_ipv6_09" value="<% CmoGetCfg("static_routing_ipv6_09","none"); %>">
				
			<td valign="top" id="maincontent_container">
				<div id="maincontent">
                  <div id=box_header>
                    <h1><script>show_words(_routing)</script> :</h1>
                    <script>show_words(av_intro_r)</script>
                    <br><br>
                    <input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_request()">
           	        <input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel('form1', 'adv_ipv6_routing.asp');">
           	        <script>check_reboot();</script>
                    <script>get_by_id("button").value = _savesettings;</script>
                    <script>get_by_id("button2").value = _dontsavesettings;</script>
				  </div>
                  <a name="show_list"></a>
                  <div class=box>
                    <h2><script>document.write(rule_max_num);</script> --<script>show_words(r_rlist)</script></h2>
                    <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1>
                      <tbody>
                        <script>
			set_routes();
			for(var i=0 ; i<rule_max_num ; i++){
			var is_checked = "";
			var obj_Name = "";
			var obj_IP = "";
			var obj_Mask = "";
			var obj_gateway = "";
			var obj_interface = "WAN";
			var obj_metric = "";
			if(i < DataArray.length){
				obj_Name = DataArray[i].Name;
				obj_IP = DataArray[i].Ip_addr;
				obj_Mask = DataArray[i].Net_mask;
				obj_gateway = DataArray[i].Gateway;
				obj_interface = DataArray[i].Interface;
				obj_metric = DataArray[i].Metric;
			}
		document.write("<tr>");
		document.write("<td align=center rowspan=2><input type=\"checkbox\" id=\"enable" + i + "\" name=\"enable" + i + "\" value=\"1\"></td>");
		document.write('<td colspan=2>'+_name+'<br><input type=text class=flatL id=name' + i + ' name=name'+ i +' size=20 maxlength=15 value='+ obj_Name +'></td>');
		document.write('<td >'+_destipv6+'<br><input type=text class=flatL id=Destination' + i + ' name=Destination' + i + ' size=43 maxlength=40 value='+ obj_IP +'>/<input type=text class=flatL id=Sub_Mask' + i + ' name=Sub_Mask' + i + ' size=5 maxlength=5 value='+ obj_Mask +'></td>');
		document.write("</tr>");

		document.write("<tr>");
		document.write('<td>'+ _metric+'<br><input type=text class=flatL id=metric' + i + ' name=metric' + i + ' size=5 maxlength=5 value='+ obj_metric +'></td>');
		document.write('<td>'+_interface+'<br><select style=width:90 id=interface' + i +' name=interface' + i +' onChange=click_null(' + i + ');>');
		document.write("<option value=\"NULL\">NULL</option>");
		document.write("<option value=\"LAN\">LAN</option>");
		document.write("<option value=\"WAN\">WAN</option>");
		document.write("<option value=\"DHCPPD\">LAN(DHCP-PD)</option>");
		document.write("</select></td>")
		document.write('<td>'+_gateway+'<br><input type=text class=flatL id=Sub_gateway' + i + ' name=Sub_gateway' + i + ' size=43 maxlength=42 value='+ obj_gateway +'></td>')
		document.write("</tr>")


		if(i < DataArray.length){
			set_checked(DataArray[i].Enable, get_by_id("enable"+i));
			set_selectIndex(DataArray[i].Interface, get_by_id("interface"+i));
		}
}
						  </script>
                      </tbody>
                    </table>
                  </div>
			  </div>

      </td>
			  </form>
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table borderColor=#ffffff cellSpacing=0 borderColorDark=#ffffff
      cellPadding=2 bgColor=#ffffff borderColorLight=#ffffff border=0>
                  <tbody>
                    <tr>
                      <td id=help_text><b><script>show_words(_hints)</script>&hellip;</b><br>

	                          <p><br><script>show_words(hhav_enable)</script></p>
				  <p><script>show_words(hhav_r_name)</script></p>
				  <p><script>show_words(hhav_r_dest_ip)</script></p>
				  <p><script>show_words(hhav_r_netmask)</script></p>
				  <p><script>show_words(hhav_r_gateway)</script></p>
				  <p><a href="support_adv.asp#Routing" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
                      </td>
                    </tr>
                  </tbody>
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
</html>
