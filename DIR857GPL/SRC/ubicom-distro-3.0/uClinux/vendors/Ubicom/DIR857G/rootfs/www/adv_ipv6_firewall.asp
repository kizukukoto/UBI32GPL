<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript" src="public_ipv6.js"></script>
<script language="JavaScript">
    var submit_button_flag = 0;
    var rule_max_num = 20;
    var DHCPList_word = "";
    var is_chk_public_port = 0;
    var is_chk_private_port = 0;
    var inbound_used = 1;
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";
    
	function onPageLoad()
	{	set_checked(get_by_id("ipv6_simple_security").value, get_by_id("ipv6_simple_security_chk"));
		set_selectIndex("<% CmoGetCfg("ipv6_filter_type","none"); %>", get_by_id("ipv6_filter_type"));

		for(var i = 0; i < rule_max_num; i++)
		{
			var temp_firewall

			if(i > 9)
				temp_firewall = get_by_id("firewall_rule_ipv6_" + i).value;
			else
				temp_firewall = get_by_id("firewall_rule_ipv6_0" + i).value;


			if(temp_firewall=="") {
				if (i > 9)
					get_by_id("firewall_rule_ipv6_" + i).value = "0//Always/-1//-1//TCP/1/65535";
				else  
					get_by_id("firewall_rule_ipv6_0" + i).value = "0//Always/-1//-1//TCP/1/65535";
			}
			click_protocol(i);
		}

		set_ipv6_firewall_rule();
		disable_ipv6_filter();
		set_form_default_values("form1");

		if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
			DisableEnableForm(document.form1,true);
		}
	}		
	
	
	    function set_ipv6_firewall_rule()
			{
					var idx =1 ;
					var ipv6_filter_type = get_by_id("ipv6_filter_type").selectedIndex;
					for (var i = 0; i < rule_max_num; i++)
					 {
						 var temp_fr;
						if (i > 9)
							temp_fr = (get_by_id("firewall_rule_ipv6_" + i).value).split("/");
						else
							temp_fr = (get_by_id("firewall_rule_ipv6_0" + i).value).split("/");
					// enable/name/Action/Schedule/Source/::/Dest/::/port/0/0
					//       0//1/Always/0/::/0/::/Any/0/65535
					//      0/1/2/     3/4/5 /6/7 /8/9/10/11
					//      0//Always/-1//-1//TCP/0/65535

							if (temp_fr.length > 1) 
							  {
									set_checked(temp_fr[0], get_by_id("enable" + i));
									get_by_id("name" + i).value = temp_fr[1];
									//set_checked(temp_fr[2], get_by_name("action_"+i));
										if((temp_fr[2] == "Always") || (temp_fr[2] == "Never") || (temp_fr[2] == "")){
										set_selectIndex(temp_fr[2], get_by_id("schedule" + i));
										}else{
										var temp_index = get_schedule_index(temp_fr[2]);
										get_by_id("schedule" + i).selectedIndex=temp_index+2;
										}			
									set_selectIndex(temp_fr[3], get_by_id("interface_source_" + i));
									get_by_id("start_ip" + i).value = temp_fr[4];
									set_selectIndex(temp_fr[5], get_by_id("interface_dest_" + i));
									get_by_id("end_ip" + i).value = temp_fr[6];
									set_selectIndex(temp_fr[7], get_by_id("protocol" + i));
									get_by_id("start_port" + i).value = temp_fr[8];
									get_by_id("end_port" + i).value = temp_fr[9];
									
                if (ipv6_filter_type !=0) {
                     detect_protocol_change_port(get_by_id("protocol"+i).selectedIndex,i);
                }
									
					//alert("temp_fr="+ temp_fr);
					//alert("start_ip="+ get_by_id("start_ip" + i).value);
					//alert("start_ip="+ get_by_id("start_ip" + i).value);
				//alert("protocol=" + get_by_id("protocol" + i).value);
 
						  	   //click_protocol(idx);
                	//idx++;
						  	}

					    }
				}
				
			function click_protocol(idx)
					{
						var protocol = get_by_id("protocol" + idx).selectedIndex;
						
					  if (protocol == 0 || protocol == 1) 
							{
							get_by_id("start_port" + idx).value = 1;
							get_by_id("end_port" + idx).value = 65535;
							get_by_id("start_port" + idx).disabled = true;
							get_by_id("end_port" + idx).disabled = true;
							}
					if (protocol == 2 || protocol == 3) {
							get_by_id("start_port" + idx).disabled = false;
							get_by_id("end_port" + idx).disabled = false;
							
							}
							//disable_ipv6_filter();
							//detect_protocol_change_port(get_by_id("protocol"+idx).selectedIndex,idx);
						//	alert("1_disable_ipv6_filter_" +idx+"="+ get_by_id("start_port" + idx).disabled);
					 }		
					 
					 
					 
    function detect_protocol_change_port(proto,i)
    {
        if ((proto == 0)||(proto == 1)) {
            get_by_id("start_port"+i).disabled = true;
            get_by_id("end_port"+i).disabled = true;
			}
       if (proto == 2 || proto == 3) {
            get_by_id("start_port"+i).disabled =false ;
            get_by_id("end_port"+i).disabled = false;
        }
	  }
	  		
	 function disable_ipv6_filter()
    {
 				var ipv6_filter_type = get_by_id("ipv6_filter_type").selectedIndex;
		
		
        for (var i = 0; i < rule_max_num; i++) {
					var protocol = get_by_id("protocol" + i).selectedIndex;
        	get_by_id("enable" + i).disabled = !(ipv6_filter_type != 0);
            get_by_id("name" + i).disabled = !(ipv6_filter_type != 0);
            get_by_id("schedule" + i).disabled = !(ipv6_filter_type != 0);
            get_by_id("interface_source_" + i).disabled = !(ipv6_filter_type != 0);
            get_by_id("start_ip" + i).disabled = !(ipv6_filter_type != 0);
            get_by_id("interface_dest_" + i).disabled = !(ipv6_filter_type != 0);
					get_by_id("end_ip" + i).disabled = !(ipv6_filter_type != 0);
					get_by_id("protocol" + i).disabled = !(ipv6_filter_type != 0);
				if(ipv6_filter_type ==0){	
					get_by_id("start_port" + i).disabled = true;
					get_by_id("end_port" + i).disabled = true;
					}else{
					click_protocol(i);
					};

        }
    }
		
		
    function save_info()
    {
        var idx, enable, action_radio,firewall_rull = "";
        for (var i = 0; i <= rule_max_num; i++) {
            idx = i + 3;
            if ( get_by_id("enable" + i).checked|| get_by_id("name" + i).value != "") {
                if (get_by_id("enable" + i).checked)
                    enable = "1";
                else
                    enable = "0";
					
			//if( get_by_id("action_" + i).value==1)
			//			action_radio=1;
			//else
			//			action_radio=0;

			action_radio=get_checked_value(get_by_name("action_"+i))
					// enable/name/Action/Schedule/Source/::/Dest/::/port/0/0
					//       0//1/Always/0/::/0/::/Any/0/65535
					//      0/1/2/     3/4/5 /6/7 /8/9/10/11
					//      0//Always/-1//-1//TCP/0/65535
                firewall_rull = enable + "/" + get_by_id("name" + i).value + "/"+ get_by_id("schedule" + i).value + "/" +get_by_id("interface_source_" + i).value + "/"
                              + get_by_id("start_ip" + i).value + "/" + get_by_id("interface_dest_" + i).value + "/" + get_by_id("end_ip" + i).value + "/"
                              +  get_by_id("protocol" + i).value +"/" + get_by_id("start_port" + i).value + "/" + get_by_id("end_port" + i).value 
							  
				//alert("firewall_rull="+firewall_rull);
                if (parseInt(idx) < 10)
                    get_by_id("firewall_rule_ipv6_0" + idx).value = firewall_rull;
                else
                    get_by_id("firewall_rule_ipv6_" + idx).value = firewall_rull;
            }
        }
    }
		
  function send_request()
 {
        if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
            return false;
        }
        
        get_by_id("ipv6_simple_security").value = get_checked_value(get_by_id("ipv6_simple_security_chk"));
        
	var enable, action_radio,firewall_rull ;
	var ipv6_static_msg = replace_msg(all_ipv6_addr_msg, IPV6_TEXT0);
		
        for (var i = 0; i < rule_max_num; i++) {
            var temp_port_name = get_by_id("name" + i).value;
            if (temp_port_name != "") {
		
		if (Find_word(temp_port_name,'"') || Find_word(temp_port_name,"/")) {
                                alert(IPV6_firewall_name + " " + (i+1) + " " + illegal_characters + " " + temp_port_name);
                                return false;	
		}

                for (var j = i+1; j < rule_max_num; j++) {
                    if (temp_port_name == get_by_id("name" + j).value) {
                        alert(TEXT077);
                        return false;
                    }
                }
            }else{
                if (get_by_id("enable" + i).checked == true) {
                    alert(YM49+" "+(i+1)+".");
                    return false;
                }
            }
        }
            for (var i = 0; i < rule_max_num; i++) 
            {
            	if (get_by_id("enable" + i).checked == true)
            		{
			  if ((get_by_id("interface_source_" + i).selectedIndex ==1)&&(get_by_id("interface_dest_" + i).selectedIndex ==1))
    				{
					alert(IPV6_firewall_interface+(i+1));
					   	return false;
   				 }
			  if ((get_by_id("interface_source_" + i).selectedIndex ==2)&&(get_by_id("interface_dest_" + i).selectedIndex ==2))
		  		{
					alert(IPV6_firewall_interface+(i+1)+".");
					   	return false;
				  }
		         }
            }      
		var idx =0;
	  for (var i = 0; i < rule_max_num; i++) {
		         if (parseInt(i, 10) > 9)
                get_by_id("firewall_rule_ipv6_" + i).value = "";
            else
                get_by_id("firewall_rule_ipv6_0" + i).value = "";

            if (get_by_id("name" + i).value != "") {
                check_name = get_by_id("name" + i).value;
                if (Find_word(check_name,'"') || Find_word(check_name,"/")) {
                    alert(IPV6_firewall_rule_name);
                    return false;
                  }
               } 
              enable = get_checked_value(get_by_id("enable" + i));
	    
	var start_ip = get_by_id("start_ip"+i).value;
	var end_ip = get_by_id("end_ip"+i).value;
	var temp_start_ip = new ipv6_addr_obj(start_ip.split(":"), ipv6_static_msg, false, false);
	var temp_end_ip = new ipv6_addr_obj(end_ip.split(":"), ipv6_static_msg, false, false);
	var tmp_startip=start_ip.split("-");
	var tmp_endip=end_ip.split("-");
	var temp_ipv6_e ;
	var temp_ipv6_s ;
if(enable ==1){
		//check the ipv6 address for start_ip   
   if(Find_word(start_ip,"-")){

	  if(check_ipv6_symbol(tmp_startip[0],"::")==2){ // find two '::' symbol
            return false;
        }else if(check_ipv6_symbol(tmp_startip[0],"::")==1){    // find one '::' symbol
            temp_ipv6_s = new ipv6_addr_obj(tmp_startip[0].split("::"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_s ,"::")){
                return false;
            }
        }else{  //not find '::' symbol
            temp_ipv6_s  = new ipv6_addr_obj(tmp_startip[0].split(":"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_s,":")){
                return false;
            }
        }
    if(check_ipv6_symbol(tmp_startip[1],"::")==2){ // find two '::' symbol
            return false;
        }else if(check_ipv6_symbol(tmp_startip[1],"::")==1){    // find one '::' symbol
            temp_ipv6_s = new ipv6_addr_obj(tmp_startip[1].split("::"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_s ,"::")){
                return false;
            }
        }else{  //not find '::' symbol
            temp_ipv6_s  = new ipv6_addr_obj(tmp_startip[1].split(":"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_s,":")){
                return false;
            }
        }   
        

	}else if (start_ip != "::"){
    if(check_ipv6_symbol(start_ip,"::")==2){ // find two '::' symbol
			return false;
	}else if(check_ipv6_symbol(start_ip,"::")==1){    // find one '::' symbol
			temp_start_ip = new ipv6_addr_obj(start_ip.split("::"), ipv6_static_msg, false, false);
			if (!check_ipv6_address(temp_start_ip,"::")){
				return false;
			}
	}else{	//not find '::' symbol
			temp_start_ip = new ipv6_addr_obj(start_ip.split(":"), ipv6_static_msg, false, false);
			if (!check_ipv6_address(temp_start_ip,":")){
				return false;
			}
	}
 }
 
   // check the ipv6 address for end_ip
     if(Find_word(end_ip,"-"))
	  {


	  if(check_ipv6_symbol(tmp_endip[0],"::")==2){ // find two '::' symbol
            return false;
        }else if(check_ipv6_symbol(tmp_endip[0],"::")==1){    // find one '::' symbol
            temp_ipv6_e = new ipv6_addr_obj(tmp_endip[0].split("::"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_e ,"::")){
                return false;
            }
        }else{  //not find '::' symbol
            temp_ipv6_e  = new ipv6_addr_obj(tmp_endip[0].split(":"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_e,":")){
                return false;
            }
        }
	  if(check_ipv6_symbol(tmp_endip[1],"::")==2){ // find two '::' symbol
            return false;
        }else if(check_ipv6_symbol(tmp_endip[1],"::")==1){    // find one '::' symbol
            temp_ipv6_e = new ipv6_addr_obj(tmp_endip[1].split("::"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_e ,"::")){
                return false;
            }
        }else{  //not find '::' symbol
            temp_ipv6_e  = new ipv6_addr_obj(tmp_endip[1].split(":"), ipv6_static_msg, false, false);
            if (!check_ipv6_address(temp_ipv6_e,":")){
                return false;
            }
        }
	} else if (end_ip != "::"){
	if(check_ipv6_symbol(end_ip,"::")==2){ // find two '::' symbol
			return false;

	}else if(check_ipv6_symbol(end_ip,"::")==1){    // find one '::' symbol

			temp_end_ip = new ipv6_addr_obj(end_ip.split("::"), ipv6_static_msg, false, false);

			if (!check_ipv6_address(temp_end_ip,"::")){

				return false;

			}

	}else{	//not find '::' symbol

			temp_end_ip = new ipv6_addr_obj(end_ip.split(":"), ipv6_static_msg, false, false);

			if (!check_ipv6_address(temp_end_ip,":")){

				return false;

			  }
	    }
	
    }
	var port_msg = replace_msg(check_num_msg, GW_NAT_PORT_FORWARD_PORT_RANGE_INVALIDa, 1, 65535);
var start_port_obj = new varible_obj(get_by_id("start_port" + i).value, port_msg , 1, 65535, false);
var end_port_obj = new varible_obj(get_by_id("end_port" + i).value, port_msg , 1, 65535, false);
var ipv6_start_port = parseInt(get_by_id("start_port" + i).value);
var ipv6_end_port = parseInt(get_by_id("end_port" + i).value);

		if (!check_varible(start_port_obj)){
			return false;
		}
		if (!check_varible(end_port_obj)){
			return false;
}
 
			//if ( get_by_id("start_port" + i).value > get_by_id("end_port" + i).value){
			if ( ipv6_start_port > ipv6_end_port){
			alert(IPV6_firewall_port1);
			return false;
}

}
					// action_radio = get_checked_value(get_by_name("action_"+i));
					//alert("action_radio="+action_radio);
		    if (idx >9){
                   firewall_rull=  get_by_id("firewall_rule_ipv6_" + idx);
                }else{
                   firewall_rull = get_by_id("firewall_rule_ipv6_0" + idx);				
						   }
   
					// enable/name/Action/Schedule/Source/::/Dest/::/port/0/0
					//       0//1/Always/0/::/0/::/Any/0/65535
					//      0/1/2/     3/4/5 /6/7 /8/9/10/11
                firewall_rull.value= enable + "/" + get_by_id("name" + i).value  + "/" + get_by_id("schedule" + i).value + "/" + get_by_id("interface_source_" + i).value + "/"
                              + get_by_id("start_ip" + i).value + "/" + get_by_id("interface_dest_" + i).value + "/" + get_by_id("end_ip" + i).value + "/"
                              +  get_by_id("protocol" + i).value +"/" + get_by_id("start_port" + i).value + "/" + get_by_id("end_port" + i).value 
						  
						     //alert("firewall_rull="+ firewall_rull);
      				   //alert("firewall_rull="+firewall_rull.value);
      				   idx ++;
            }
        if ((!is_form_modified("form1")) && (get_by_id("ipv6_filter_type").selectedIndex == 1)) {
            alert(IPV6_firewall_filter_setting);
            return false;
        }
			if (submit_button_flag == 0) {
              submit_button_flag = 1;
              return true;
            }
            return false;
    }

</script>

<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=iso-8859-1">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style1 {font-size: 11px}
-->
</style>
</head>

<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
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
            <td id="topnavoff"><a href="index.asp" onclick="return jump_if();"><script>show_words(_setup)</script></a></td>
			<td id="topnavon"><a href="adv_virtual.asp" onclick="return jump_if();"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_tools)</script></a></td>
			<td id="topnavoff"><a href="st_device.asp" onclick="return jump_if();"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp" onclick="return jump_if();"><script>show_words(_support)</script></a></td>
        </tr>
    </table>
    <table border="1" cellpadding="2" cellspacing="0" width="838" height="100%" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
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
                                <LI><div id=sidenavoff><script>show_words(Tag05762)</script></div></LI>
                		<LI><div><a href="adv_ipv6_routing.asp" onclick="return jump_if();"><script>show_words(IPV6_routing)</script></a></div></LI>
                          </UL>
                        </div>
                      </td>
                    </tr>
                  </tbody>
                </table></td>
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
				    <input type="hidden" id="html_response_page" name="html_response_page" value="back_long.asp">
                    <input type="hidden" id="html_response_message" name="html_response_message" value="The setting is saved.">
                    <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="adv_ipv6_firewall.asp">
                    <input type="hidden" id="reboot_type" name="reboot_type" value="all">
                    
                    
                    <input type="hidden" id="firewall_rule_ipv6_00" name="firewall_rule_ipv6_00" value="<% CmoGetCfg("firewall_rule_ipv6_00","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_01" name="firewall_rule_ipv6_01" value="<% CmoGetCfg("firewall_rule_ipv6_01","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_02" name="firewall_rule_ipv6_02" value="<% CmoGetCfg("firewall_rule_ipv6_02","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_03" name="firewall_rule_ipv6_03" value="<% CmoGetCfg("firewall_rule_ipv6_03","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_04" name="firewall_rule_ipv6_04" value="<% CmoGetCfg("firewall_rule_ipv6_04","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_05" name="firewall_rule_ipv6_05" value="<% CmoGetCfg("firewall_rule_ipv6_05","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_06" name="firewall_rule_ipv6_06" value="<% CmoGetCfg("firewall_rule_ipv6_06","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_07" name="firewall_rule_ipv6_07" value="<% CmoGetCfg("firewall_rule_ipv6_07","none"); %>">
										
										<input type="hidden" id="firewall_rule_ipv6_08" name="firewall_rule_ipv6_08" value="<% CmoGetCfg("firewall_rule_ipv6_08","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_09" name="firewall_rule_ipv6_09" value="<% CmoGetCfg("firewall_rule_ipv6_09","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_10" name="firewall_rule_ipv6_10" value="<% CmoGetCfg("firewall_rule_ipv6_10","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_11" name="firewall_rule_ipv6_11" value="<% CmoGetCfg("firewall_rule_ipv6_11","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_12" name="firewall_rule_ipv6_12" value="<% CmoGetCfg("firewall_rule_ipv6_12","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_13" name="firewall_rule_ipv6_13" value="<% CmoGetCfg("firewall_rule_ipv6_13","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_14" name="firewall_rule_ipv6_14" value="<% CmoGetCfg("firewall_rule_ipv6_14","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_15" name="firewall_rule_ipv6_15" value="<% CmoGetCfg("firewall_rule_ipv6_15","none"); %>">
										
										<input type="hidden" id="firewall_rule_ipv6_16" name="firewall_rule_ipv6_16" value="<% CmoGetCfg("firewall_rule_ipv6_16","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_17" name="firewall_rule_ipv6_17" value="<% CmoGetCfg("firewall_rule_ipv6_17","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_18" name="firewall_rule_ipv6_18" value="<% CmoGetCfg("firewall_rule_ipv6_18","none"); %>">
										<input type="hidden" id="firewall_rule_ipv6_19" name="firewall_rule_ipv6_19" value="<% CmoGetCfg("firewall_rule_ipv6_19","none"); %>">
										<input type="hidden" id="ipv6_simple_security" name="ipv6_simple_security" value="<% CmoGetCfg("ipv6_simple_security","none"); %>">
            <td valign="top" id="maincontent_container">
                   <div id="maincontent" style="display:<% CmoGetStatus("reboot_no_needed"); %>">
                  <div id=box_header>
                    <h1><script>show_words(IPV6_firewall);</script></h1>
                    <script>show_words(IPV6_firewall_info);</script><br><br>
                    <input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_request()">
                    <input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel('form1', 'adv_ipv6_firewall.asp');">
                    <script>get_by_id("button").value = _savesettings;</script>
                    <script>get_by_id("button2").value = _dontsavesettings;</script>                    
                  </div>
                  <div class=box>
                    <h2><script>show_words(IPV6_simple_security);</script></h2>
                      <table cellSpacing=1 cellPadding=2 width=525 border=0>
                      <tbody>
                        <tr>
                          <td width=200><script>show_words(IPV6_simple_security_enable);</script>:</td>
                          <td><input type=checkbox id="ipv6_simple_security_chk" name="ipv6_simple_security_chk" value="1"></td>
                        </tr>
                      </tbody>
                    </table>
                  </div>
                  <div class=box>
                    <h2> <!--<script>document.write(rule_max_num)</script> &ndash;&ndash; --> <script>show_words(IPV6_firewall);</script></h2>
                                        <table cellSpacing=1 cellPadding=2 width=525 border=0>
                      <tbody>
                        <tr>
                          <td><script>show_words(IPV6_firewall_config);</script>:</td>
                        </tr>
                        <tr>
                          <td>
                          <select id="ipv6_filter_type" name="ipv6_filter_type" onChange="disable_ipv6_filter();">
                              <option value="disable"><script>show_words(IPV6_firewall_turn_off);</script></option>
                              <option value="list_allow"><script>show_words(IPV6_firewall_allow);</script></option>
                              <option value="list_deny"><script>show_words(IPV6_firewall_deny);</script></option>
                            </select>
                          </td>
                        </tr>
                      </tbody>
                    </table>
                    
                    <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1>

                      <tbody>
					    <script>show_words(IPV6_firewall_remaining);</script>:<script>rule_max_num</script>
                        <script>
                            for(var i=0 ; i<rule_max_num ; i++){
                                document.write("<tr>");
                                document.write("<td align=center valign=middle rowspan=3>" + (i+1) +".<br><input type=checkbox id=\"enable" + i + "\" name=\"enable" + i + "\" value=\"1\"></td>");
                                document.write("<td valign=bottom colspan=2>" + _name + "<br><input type=text class=flatL id=\"name" + i + "\" name=\"name" + i +"\" size=16 maxlength=31></td>");
                                document.write("<td valign=bottom align=left class=flatL>" + _sched + "<br>");
                                document.write("<select width=30 id=schedule" + i + " name=schedule" + i + " style='width:90'>");
                                document.write("<option value=\"Always\" selected>"+_always+"</option>");
                                document.write("<option value=\"Never\" selected>"+_never+"</option>");
																document.write(set_schedule_option());
                                document.write("</select>");
                                document.write("</td>");
                                document.write("<td valign=bottom width=\"150\">&nbsp;</td>");
                                document.write("</tr>");
                                document.write("<tr>");
                                document.write("<td valign=bottom>" + _source + "</td>");
                                document.write("<td align=left valign=bottom>" + _interface + "<br>");
                                document.write("<select style='width:60' id=interface_source_" + i +" name=interface_source_" + i +">");
                                document.write("<option value=-1>*</option>");
                                document.write("<option value=0>LAN</option>");
								document.write("<option value=1>WAN</option>");
                                document.write("</select></td>");
                                document.write("<td align=left valign=bottom>" + ip_addr_range);
                                document.write("<br>");
                                document.write("<input type=text class=flatL id=start_ip" + i + " name=start_ip" + i +" size=40>");             
                                document.write("</td>");
                                document.write("<td align=left>" + _protocol + "<br>");
                                document.write("<select width=80 id=protocol" + i + " name=protocol" + i + " onChange=\"click_protocol(" + i + ");detect_protocol_change_port(this.selectedIndex,'" + i + "');\">");
								document.write("<option value=ICMPv6>ICMP</option>");
								document.write("<option value=Any>Any</option>");
								document.write("<option value=TCP>TCP</option>");
								document.write("<option value=UDP>UDP</option>");
                                document.write("</select></td>");
                                document.write("</tr>");
                                document.write("<tr>");
                                document.write("<td valign=bottom>" + _dest + "</td>");
                                document.write("<td align=left valign=bottom>" + _interface + "<br>");
                                document.write("<select style='width:60' id=interface_dest_" + i +" name=interface_dest_" + i +">");
                                document.write("<option value=-1>*</option>");
								document.write("<option value=0>LAN</option>");
								document.write("<option value=1>WAN</option>");
                                document.write("<td align=left valign=bottom>" + ip_addr_range);
                                document.write("<br>");
                                document.write("<input type=text class=flatL id=end_ip" + i + " name=end_ip" + i +" size=40>");
                                document.write("</td>");
                                document.write("<td align=left valign=bottom>" + port_range + "<br>");
                                document.write("<input type=text class=flatL id=start_port" + i + " name=start_port" + i +" size=8 maxlength=5>~<input type=text class=flatL id=end_port" + i + " name=end_port" + i +" size=8 maxlength=5></td>");
                                document.write("</tr>");
							                	//click_protocol(i);
                            }
                        </script>
                      </tbody>
                    </table>
                  </div>
                </div>
             </td></form>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table borderColor=#ffffff cellSpacing=0 borderColorDark=#ffffff cellPadding=2 bgColor=#ffffff borderColorLight=#ffffff border=0>
                  <tbody>
                    <tr>
                      <td id=help_text><b><script>show_words(_hints);</script>&hellip;</b>
                        <p><script>show_words(IPV6_firewall_msg1);</script></p>
                        <p><script>show_words(IPV6_firewall_msg2);</script></p>
                        <p><a href="support_adv.asp#firewallv6" onclick="return jump_if();"><script>show_words(_more);</script>&hellip;</a></p>
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
<div id="copyright"><script>show_words(_copyright);</script></div>
<br>
</body>
<script>
	reboot_form();
	onPageLoad();
</script>
</html>
