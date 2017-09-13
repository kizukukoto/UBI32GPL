<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_ipv6.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
	var submit_button_flag = 0;
	var wan_ip;
 	var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";
function onPageLoad(){
	var link_local ;
	link_local = document.getElementById("link_local_ip_l").value;

	wan_ip = "<% CmoGetStatus("wan_current_ipaddr_00"); %>".split("/")[0];
 	get_by_id("wan_ip").innerHTML = replace_null_to_none(wan_ip);

	ipv6_6rd_link_local();
  	document.getElementById("lan_link_local_ip").innerHTML= link_local.toUpperCase();
	set_checked(get_by_id("ipv6_autoconfig").value, get_by_id("ipv6_autoconfig_sel"));
	set_checked(get_by_id("ipv6_6rd_hub").value, get_by_id("ipv6_6rd_hub_enable"));
	set_selectIndex("<% CmoGetCfg("ipv6_autoconfig_type","none"); %>", get_by_id("ipv6_autoconfig_type"));
	disable_autoconfig();
	set_ipv6_autoconfiguration_type();
	set_checked("1", get_by_name("ipv6_dhcp_auto_dns"));
	set_checked("<% CmoGetCfg("ipv6_6rd_dhcp","0"); %>", get_by_name("ipv6_6rd_dhcp"));
	dhcp_option();
	ipv6_6rd_prefix_onchange();
	set_ipv6_stateful_range();
	get_by_id("ipv6_dhcpd_lifetime").disabled = true;
	get_by_id("ipv6_6rd_adver_lifetime").disabled = true;
	var ipv6_advert_lifetime = <% CmoGetStatus("ipv6_tunnel_lifetime"); %>;
	get_by_id("ipv6_6rd_adver_lifetime").value = Math.round(ipv6_advert_lifetime/60*100)/100;
	get_by_id("ipv6_dhcpd_lifetime").value = get_by_id("ipv6_6rd_adver_lifetime").value;
	var login_who= "<% CmoGetStatus("get_current_user"); %>";
	if(login_who== "user"){
		DisableEnableForm(document.form1,true);	
	} 
	set_form_default_values("form1");
}

function replace_null_to_none(item){
                if(item=="(null)"|| item == "null" || item == "NULL")
                        item="None";
                return item;
}

function dhcp_option()
{
       var ipv6_6rd_dhcp = get_by_name("ipv6_6rd_dhcp");
       if(ipv6_6rd_dhcp[0].checked){
               get_by_id("ipv6_6rd_prefix").disabled = true;
               get_by_id("ipv6_6rd_prefix_length").disabled = true;
               get_by_id("ipv6_6rd_ipv4_mask_length").disabled = true;
               get_by_id("ipv6_6rd_relay").disabled = true;
       }else{
               get_by_id("ipv6_6rd_prefix").disabled = false;
               get_by_id("ipv6_6rd_prefix_length").disabled = false;
               var pf_len = get_by_id("ipv6_6rd_prefix_length").value;
               get_by_id("ipv6_6rd_ipv4_mask_length").disabled = (pf_len <= 32);
               get_by_id("ipv6_6rd_relay").disabled = false;
       }
}

		
function disable_autoconfig(){ 
	var disable = get_by_id("ipv6_autoconfig_sel").checked; 
	get_by_id("ipv6_autoconfig").value = get_checked_value(get_by_id("ipv6_autoconfig_sel")); 
	get_by_id("ipv6_autoconfig_type").disabled = !disable; 
	get_by_id("ipv6_addr_range_start_suffix").disabled = !disable; 
	get_by_id("ipv6_addr_range_end_suffix").disabled = !disable; 
} 

	function generate_eui64(mac) {
		var ary_mac = mac.split(":");
		var u8_mac = new Array();
		var eui64 = new Array();
		for (i=0; i<6; i++) {
			u8_mac[i] = parseInt(ary_mac[i],16);
		}
		eui64[0] = u8_mac[0] ^ 0x02;
		eui64[1] = u8_mac[1]
		eui64[2] = u8_mac[2]
		eui64[3] = 0xff;
		eui64[4] = 0xfe;
		eui64[5] = u8_mac[3];
		eui64[6] = u8_mac[4];
		eui64[7] = u8_mac[5];
		return  parseInt(eui64[0].toString(16) + eui64[1].toString(16).lpad("0",2), 16).toString(16) + ":" +
			parseInt(eui64[2].toString(16) + eui64[3].toString(16).lpad("0",2), 16).toString(16) + ":" +
			parseInt(eui64[4].toString(16) + eui64[5].toString(16).lpad("0",2), 16).toString(16) + ":" +
			parseInt(eui64[6].toString(16) + eui64[7].toString(16).lpad("0",2), 16).toString(16) ;
	}

	function ipv6_6rd_link_local()
{
	var u32_pf;
	var ary_ip6rd_pf = [0,0];
	var pf = get_by_id("ipv6_6rd_prefix").value;
	var ary_ip4 = wan_ip.split(".");
	var u32_ip4 = (ary_ip4[0]*Math.pow(2,24)) + (ary_ip4[1]*Math.pow(2,16)) + (ary_ip4[2]*Math.pow(2,8)) + parseInt(ary_ip4[3]);
	if(get_by_id("wan_ip").innerHTML == "None"){
		get_by_id("ipv6_6rd_addr").innerHTML = "None";
		return;
	}

	u32_pf = parseInt(u32_ip4);

	str_tmp = u32_pf.toString(16).lpad("0",8);
	ary_ip6rd_pf[0] = str_tmp.substr(0,4);
	ary_ip6rd_pf[1] = str_tmp.substr(4,4);
	get_by_id("ipv6_6rd_addr").innerHTML = 
		("FE80::"+ary_ip6rd_pf[0]+":"+ary_ip6rd_pf[1]+"/64").toUpperCase();
}


	function ipv6_6rd_prefix_onchange() {
		var pf = get_by_id("ipv6_6rd_prefix").value;
		var pf_len = get_by_id("ipv6_6rd_prefix_length").value;
		var v4_masklen = get_by_id("ipv6_6rd_ipv4_mask_length").value;
		var v4_uselen; 
		var wan_mac = "<% CmoGetCfg("wan_mac","none"); %>";
		var lan_mac = "<% CmoGetCfg("lan_mac","none"); %>";
		var eui64;
		var ipv6_6rd_dhcp = get_by_name("ipv6_6rd_dhcp");

		var ary_ip4 = wan_ip.split(".");
		var u32_ip4 = (ary_ip4[0]*Math.pow(2,24)) + (ary_ip4[1]*Math.pow(2,16)) + (ary_ip4[2]*Math.pow(2,8)) + parseInt(ary_ip4[3]);
		var ary_pf = get_stateful_ipv6(pf).split(":");
		var u32_pf = [0,0];
		var mask_len;

		var ary_ip6rd_pf = [0,0,0,0];
		var str_tmp;

		var IsValid = true;

		/*check 6rd prefix length*/
		if(get_by_id("wan_ip").innerHTML == "None")
                        IsValid = false;

                if (IsValid) {
                        if ( (pf_len-0)==pf_len && pf_len.length>0 ) {
                                pf_len = parseInt(pf_len);
                                if (pf_len<1 || pf_len>63)
                                        IsValid = false;
                        } else
                                IsValid = false;
                }
                 if (pf_len <= 32) { 
                 			get_by_id("ipv6_6rd_ipv4_mask_length").disabled = true;
                 			get_by_id("ipv6_6rd_ipv4_mask_length").value = "0"; 
		} else {
               				get_by_id("ipv6_6rd_ipv4_mask_length").disabled = ipv6_6rd_dhcp[0].checked;
                }
                /* check ipv4 mask length*/
                if (IsValid && pf_len >32) {
                        if ( (v4_masklen-0)==v4_masklen && v4_masklen.length>0 ) {
                                v4_masklen = parseInt(v4_masklen);
                                v4_uselen = 32 - v4_masklen;
                                if (v4_masklen<1 || v4_masklen>31 || v4_uselen+pf_len > 64) 
                                        IsValid = false;
                        } else
                                IsValid = false;
                }

                /*check prefix*/
                var c;
                var tmp_ary_pf = pf.split("::");
                if (IsValid && pf=="")
			IsValid = false;

                if (IsValid && 
                        ((tmp_ary_pf.length>1 && tmp_ary_pf[1]==":") || tmp_ary_pf.length>2))
                        IsValid = false;

		if (IsValid) {
			for (var idx=0; idx < ary_pf.length; idx++) {
                                if (idx>0 && ary_pf[idx]=="")
                                        ary_pf[idx] = "0";
				if (ary_pf[idx].length>=1 && ary_pf[idx].length<=4) {
                                        for(var pos=0; pos < ary_pf[idx].length; pos++) {
						if( !check_hex(ary_pf[idx].charAt(pos)) )
                                                        IsValid = false;
                                        }
				} else {
					IsValid = false;
					break;
				}
			}
                        if (ary_pf.length<2) ary_pf[1] = "0";
                        if (ary_pf.length<3) ary_pf[2] = "0";
                        if (ary_pf.length<4) ary_pf[3] = "0";
		}

		if (!IsValid) {
                        get_by_id("ipv6_6rd_assigned_prefix").innerHTML = "None";
			get_by_id("lan_ipv6_ip_lan_ip").innerHTML = "None";
			return;
		}

		if (ary_pf.length >=1 ) {
			u32_pf[0] = parseInt( ary_pf[0].lpad("0",4) + ary_pf[1].lpad("0",4), 16);

                        if (pf_len == 32) {
                                u32_pf[1] = parseInt(u32_ip4);
                        } else if (pf_len < 32) {
				mask_len = (32-pf_len);
				u32_pf[0] = (u32_pf[0] >>> mask_len) * Math.pow(2,mask_len);
				u32_pf[0] = parseInt(u32_pf[0]) + (u32_ip4 >>> pf_len);
				u32_pf[1] = (u32_ip4 - ((u32_ip4 >>> pf_len) * Math.pow(2,pf_len))) * Math.pow(2,mask_len);
			} else {
                                u32_pf[1] = parseInt( ary_pf[2].lpad("0",4) + ary_pf[3].lpad("0",4), 16);
                                mask_len = (64-pf_len);
                                u32_pf[1] = (u32_pf[1] >>> mask_len) * Math.pow(2,mask_len);
				u32_ip4 = u32_ip4 % (1*Math.pow(2,v4_uselen));
				u32_pf[1] = parseInt(u32_pf[1]) + (u32_ip4*Math.pow(2,64-pf_len-v4_uselen));
			}
		}

		if (pf_len <= 32) {
			eui64 = generate_eui64(lan_mac);
			str_tmp = u32_pf[0].toString(16).lpad("0",8);
			ary_ip6rd_pf[0] = str_tmp.substr(0,4);
			ary_ip6rd_pf[1] = str_tmp.substr(4,4);
			str_tmp = u32_pf[1].toString(16).lpad("0",8);
			ary_ip6rd_pf[2] = str_tmp.substr(0,4);
			ary_ip6rd_pf[3] = str_tmp.substr(4,4);
                        get_by_id("ipv6_6rd_assigned_prefix").innerHTML =
                                (ary_ip6rd_pf[0]+":"+ary_ip6rd_pf[1]+":"+ary_ip6rd_pf[2]+":"+ary_ip6rd_pf[3]).toUpperCase()+"::/"+(pf_len + 32);
			str_tmp = ary_ip6rd_pf[0]+":"+ary_ip6rd_pf[1]+":"+ary_ip6rd_pf[2]+":"+ary_ip6rd_pf[3]+":"+eui64;
			get_by_id("lan_ipv6_ip_lan_ip").innerHTML = str_tmp.toUpperCase() + "/64";
                } else {
                        eui64 = generate_eui64(lan_mac);
                        str_tmp = u32_pf[0].toString(16).lpad("0",8);
                        ary_ip6rd_pf[0] = str_tmp.substr(0,4);
                        ary_ip6rd_pf[1] = str_tmp.substr(4,4);
                        str_tmp = u32_pf[1].toString(16).lpad("0",8);
                        ary_ip6rd_pf[2] = str_tmp.substr(0,4);
                        ary_ip6rd_pf[3] = str_tmp.substr(4,4);
                        get_by_id("ipv6_6rd_assigned_prefix").innerHTML =
				(ary_ip6rd_pf[0]+":"+ary_ip6rd_pf[1]+":"+ary_ip6rd_pf[2]+":"+ary_ip6rd_pf[3]).toUpperCase()+"::/"+(pf_len + v4_uselen);
                        str_tmp = ary_ip6rd_pf[0]+":"+ary_ip6rd_pf[1]+":"+ary_ip6rd_pf[2]+":"+ary_ip6rd_pf[3]+":"+eui64;
                        get_by_id("lan_ipv6_ip_lan_ip").innerHTML = str_tmp.toUpperCase() + "/64";
		}
		set_ipv6_autoconf_range();
	}


	function set_ipv6_autoconf_range(){
			var ipv6_6rd_lan_prefix = get_stateful_prefix(get_by_id("lan_ipv6_ip_lan_ip").innerHTML, 64);
			if(ipv6_6rd_lan_prefix != ""){
					get_by_id("ipv6_addr_range_start_prefix").value  = ipv6_6rd_lan_prefix.toUpperCase();
          get_by_id("ipv6_addr_range_end_prefix").value  = ipv6_6rd_lan_prefix.toUpperCase();
			}
	}

	function set_ipv6_stateful_range()
	{
			var prefix_length = 64;
			var ipv6_lan_ip = get_by_id("lan_ipv6_ip_lan_ip").innerHTML;
			var ipv6_dhcpd_start_range = get_by_id("ipv6_dhcpd_start").value;
			var ipv6_dhcpd_end_range = get_by_id("ipv6_dhcpd_end").value;
			var correct_ipv6="";
			if(ipv6_lan_ip != ""){
					correct_ipv6 = get_stateful_ipv6(ipv6_lan_ip);
					get_by_id("ipv6_addr_range_start_prefix").value  = get_stateful_prefix(correct_ipv6,prefix_length);
		    		get_by_id("ipv6_addr_range_end_prefix").value  = get_stateful_prefix(correct_ipv6,prefix_length);
		    }
				get_by_id("ipv6_addr_range_start_suffix").value  = get_stateful_suffix(ipv6_dhcpd_start_range);
				get_by_id("ipv6_addr_range_end_suffix").value  = get_stateful_suffix(ipv6_dhcpd_end_range);
	}

	function set_ipv6_autoconfiguration_type(){
 		var mode = get_by_id("ipv6_autoconfig_type").selectedIndex;
		switch(mode){
		case 0:
			get_by_id("show_ipv6_addr_range_start").style.display = "none";
			get_by_id("show_ipv6_addr_range_end").style.display = "none";
			get_by_id("show_ipv6_addr_lifetime").style.display ="none";
			get_by_id("show_router_advert_lifetime").style.display = "";
			break;
		case 1:
		 	set_ipv6_autoconf_range();
			get_by_id("ipv6_addr_range_start_prefix").disabled = true;
			get_by_id("ipv6_addr_range_end_prefix").disabled = true;
			get_by_id("show_ipv6_addr_range_start").style.display = "";
			get_by_id("show_ipv6_addr_range_end").style.display = "";
			get_by_id("show_ipv6_addr_lifetime").style.display ="";
			get_by_id("show_router_advert_lifetime").style.display = "none";
			break;

		default:
			get_by_id("show_ipv6_addr_range_start").style.display = "none";
			get_by_id("show_ipv6_addr_range_end").style.display = "none";
			get_by_id("show_ipv6_addr_lifetime").style.display ="none";
			get_by_id("show_router_advert_lifetime").style.display = "";
			break;
		}
       }

function send_request(){
	var primary_dns = get_by_id("ipv6_6rd_primary_dns").value;
	var v6_primary_dns_msg = replace_msg(all_ipv6_addr_msg, _dns1);
	var secondary_dns = get_by_id("ipv6_6rd_secondary_dns").value;
	var v6_secondary_dns_msg = replace_msg(all_ipv6_addr_msg, _dns2);
	var check_mode = get_by_id("ipv6_autoconfig_type").selectedIndex;
	get_by_id("ipv6_autoconfig").value = get_checked_value(get_by_id("ipv6_autoconfig_sel"));
	var enable_autoconfig = get_by_id("ipv6_autoconfig").value;
	var ipv6_addr_s_suffix = get_by_id("ipv6_addr_range_start_suffix").value;
	var ipv6_addr_e_suffix = get_by_id("ipv6_addr_range_end_suffix").value;
	var v6_6rd_relay = get_by_id("ipv6_6rd_relay").value;
	var ipv6_addr_msg = replace_msg(all_ip_addr_msg,"IP Address");
	var v6_6rd_relay_obj = new addr_obj(v6_6rd_relay.split("."), ipv6_addr_msg, false, false);
	var ipv6_prefix_msg = replace_msg(all_ipv6_addr_msg,"IPv6 Prefix");
	var ipv6_6rd_prefix = get_by_id("ipv6_6rd_prefix").value;
	var temp_ipv6_6rd_prefix = new ipv6_addr_obj(ipv6_6rd_prefix.split(":"), ipv6_prefix_msg, false, false);
	var ipv6_6rd_dhcp = get_by_name("ipv6_6rd_dhcp");
	get_by_id("ipv6_wan_proto").value = get_by_id("ipv6_w_proto").value;
	get_by_id("ipv6_6rd_hub").value = get_checked_value(get_by_id("ipv6_6rd_hub_enable"));

        var is_modify = is_form_modified("form1");
        if (!is_modify && !confirm(up_jt_1+"\n"+up_jt_2+"\n"+up_jt_3)) {
			return false;
        }

	if (ipv6_6rd_dhcp[0].checked){
		var ipv4_wan_proto = "<% CmoGetCfg("wan_proto","none"); %>"
		if(ipv4_wan_proto != "dhcpc"){
                        alert(IPV6_6rd_v4wan);
			return false;
		}
	}else{

	if (!check_ipv6_relay_address(v6_6rd_relay_obj)){
			alert(LS46);			
			return false;
		}

		if(Find_word(v6_6rd_relay,":") || (v6_6rd_relay == "")){
			alert(LS46);
			return false;
		}

		if(check_ipv6_symbol(ipv6_6rd_prefix,"::") == 2){ // find two '::' symbol
		return false;
		}else if(check_ipv6_symbol(ipv6_6rd_prefix,"::") == 1){    // find one '::' symbol
			temp_ipv6_6rd_prefix = new ipv6_addr_obj(ipv6_6rd_prefix.split("::"), ipv6_prefix_msg, false, false);
			if(temp_ipv6_6rd_prefix.addr[temp_ipv6_6rd_prefix.addr.length-1].length == 0)
				temp_ipv6_6rd_prefix.addr[temp_ipv6_6rd_prefix.addr.length-1] = "1111";
			if (!check_ipv6_address(temp_ipv6_6rd_prefix,"::")){
		return false;
        }
		}else{  //not find '::' symbol
			temp_ipv6_6rd_prefix = new ipv6_addr_obj(ipv6_6rd_prefix.split(":"), ipv6_prefix_msg, false, false);
			if (!check_ipv6_address(temp_ipv6_6rd_prefix,":")){
		return false;
    }
        }					 		
	}

	//check DNS Address
	if (primary_dns != ""){
	if(check_ipv6_symbol(primary_dns,"::")==2){ // find two '::' symbol 
		return false;
	}else if(check_ipv6_symbol(primary_dns,"::")==1){    // find one '::' symbol	
		temp_ipv6_primary_dns = new ipv6_addr_obj(primary_dns.split("::"), v6_primary_dns_msg, false, false);
		if (!check_ipv6_address(temp_ipv6_primary_dns ,"::")){
			return false;
		}
	}else{	//not find '::' symbol
			temp_ipv6_primary_dns  = new ipv6_addr_obj(primary_dns.split(":"), v6_primary_dns_msg, false, false);
			if (!check_ipv6_address(temp_ipv6_primary_dns,":")){
				return false;
			}
	}
}
	if (secondary_dns != ""){
		if(check_ipv6_symbol(secondary_dns,"::")==2){ // find two '::' symbol
			return false;
		}else if(check_ipv6_symbol(secondary_dns,"::")==1){    // find one '::' symbol
			temp_ipv6_secondary_dns = new ipv6_addr_obj(secondary_dns.split("::"), v6_secondary_dns_msg, false, false);
			if (!check_ipv6_address(temp_ipv6_secondary_dns ,"::")){
					return false;
			}
		}else{	//not find '::' symbol
			temp_ipv6_secondary_dns  = new ipv6_addr_obj(secondary_dns.split(":"), v6_secondary_dns_msg, false, false);
			if (!check_ipv6_address(temp_ipv6_secondary_dns,":")){
				return false;
			}
		}
	}
	if(check_mode == 1 && enable_autoconfig == 1){
    		//check the suffix of Address Range(Start)
		if(!check_ipv6_address_suffix(ipv6_addr_s_suffix,IPV6_TEXT70)){
			return false;
		}
		//check the suffix of Address Range(End)
		if(!check_ipv6_address_suffix(ipv6_addr_e_suffix,IPV6_TEXT71)){
			return false;
		}
		//compare the suffix of start and the suffix of end
		if(!compare_suffix(ipv6_addr_s_suffix,ipv6_addr_e_suffix)){
    			return false;
    		}
    		//set autoconfiguration range value
		get_by_id("ipv6_dhcpd_start").value = get_by_id("ipv6_addr_range_start_prefix").value + "::" + get_by_id("ipv6_addr_range_start_suffix").value;
		get_by_id("ipv6_dhcpd_end").value = get_by_id("ipv6_addr_range_end_prefix").value + "::" + get_by_id("ipv6_addr_range_end_suffix").value;
	}
	if(submit_button_flag == 0){
		submit_button_flag = 1;
		get_by_id("form1").submit();
		return true;
	}else{
		return false;
	}
}

//pads left

String.prototype.lpad = function(padString, length) {
	var str = this;
	while (str.length < length)
		str = padString + str;
	return str;
}

</script>
<title>D-LINK CORPORATION, INC | WIRELESS ROUTER | HOME</title>

<meta http-equiv=Content-Type content="text/html; charset=iso-8859-1">

<style type="text/css">
<!--
.style1 {font-size: 11px}
-->
</style>
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
	<table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
      <tr>
        <td width="100%">&nbsp;&nbsp;Product Page: <a href="http://support.dlink.com.tw/" onclick="return jump_if();"><% CmoGetCfg("model_number","none"); %></a></td>
        <td align="right" nowrap>Hardware Version: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
		<td align="right" nowrap>Firmware Version: <% CmoGetStatus("version"); %></td>
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
	<table border="1" cellpadding="2" cellspacing="0" width="838" height="100%" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
		<tr>
			<td id="sidenav_container" valign="top" width="125" align="right">
				<table cellSpacing=0 cellPadding=0 border=0>
                  <tbody>
                    <tr>
						<td id="sidenav_container">
							<div id="sidenav">
								<ul>
									<script>
										show_side_bar(6);
									</script>
								</ul>
                      	</div>
                      </td>
                    </tr>
                  </tbody>
                </table></td>
	       <form id="form1" name="form1" method="post" action="apply.cgi">
	       	    <input type="hidden" id="html_response_page" name="html_response_page" value="back_long.asp">
			<input type="hidden" id="html_response_message" name="html_response_message" value="">
			<script>get_by_id("html_response_message").value = sc_intro_sv;</script>

				<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="adv_ipv6_6rd.asp">
				<input type="hidden" id="ipv6_autoconfig" name="ipv6_autoconfig" value="<% CmoGetCfg("ipv6_autoconfig","none"); %>">
				<input type="hidden" id="ipv6_6rd_hub" name="ipv6_6rd_hub" value="<% CmoGetCfg("ipv6_6rd_hub","none"); %>"> 
				<input type="hidden" id="ipv6_dhcpd_start" name="ipv6_dhcpd_start" value="<% CmoGetCfg("ipv6_dhcpd_start","none"); %>">
				<input type="hidden" id="ipv6_dhcpd_end" name="ipv6_dhcpd_end" value="<% CmoGetCfg("ipv6_dhcpd_end","none"); %>">
				<input type="hidden" id="ipv6_wan_proto" name="ipv6_wan_proto" value="<% CmoGetCfg("ipv6_wan_proto","none"); %>">
				<input type="hidden" maxLength=80 size=80 name="ipv6_6rd_tunnel_address" id="ipv6_6rd_tunnel_address" value="<% CmoGetStatus("6rd_tunnel_address","none"); %>">
				<input type="hidden" maxLength=80 size=80 name="link_local_ip_l" id="link_local_ip_l" value="<% CmoGetStatus("link_local_ip_l","none"); %>">
				<input type="hidden" id="reboot_type" name="reboot_type" value="wan">
				<input type="hidden" id="ipv6_wan_specify_dns" name="ipv6_wan_specify_dns" value="1">
               <td valign="top" id="maincontent_container">
				<div id=maincontent>
		  <div id=box_header>
                    <h1>IPv6</h1>
                    <script>show_words(IPV6_6rd_info);</script><br>
                    <br>
                    <input name="button" id="button" type="button" class=button_submit value="" onClick="send_request()">
            		<input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel_1('form1', 'adv_ipv6_6rd.asp');">
            		<script>check_reboot();</script>
           			<script>get_by_id("button").value = _savesettings;</script>
					<script>get_by_id("button2").value = _dontsavesettings;</script>
                 </div>
                  <div class=box>
                    <h2 style=" text-transform:none"> 
                   <script>show_words(IPV6_TEXT29);</script></h2>
				   <p class="box_msg"><script>show_words(IPV6_TEXT30)</script></p>
                   <table cellSpacing=1 cellPadding=1 width=525 border=0>
                    <tr>
                      <td align=right width="187" class="duple"><script>show_words(IPV6_TEXT31);</script> :</td>
                      <td width="331">&nbsp; <select name="ipv6_w_proto" id="ipv6_w_proto" onChange=select_ipv6_wan_page(get_by_id("ipv6_w_proto").value);>
			<option value="ipv6_autodetect"><script>show_words(IPV6_TEXT32b);</script></option>
			<option value="ipv6_static"><script>show_words(IPV6_TEXT32);</script></option>
                        <option value="ipv6_autoconfig"><script>show_words(IPV6_TEXT32a);</script></option>
                        <option value="ipv6_pppoe"><script>show_words(IPV6_TEXT34);</script></option>
                        <option value="ipv6_6in4"><script>show_words(IPV6_TEXT35);</script></option>
                        <option value="ipv6_6to4"><script>show_words(IPV6_TEXT36);</script></option>
                        <option value="ipv6_6rd" selected><script>show_words(IPV6_TEXT36a);</script></option>
                        <option value="link_local"><script>show_words(IPV6_TEXT37a);</script></option>
                      </select></td>
                    </tr>
                   </table>
                  </div>
	         <div class=box id=wan_ipv6_settings>
	                <h2 style=" text-transform:none"><script>show_words(IPV6_6rd_setting);</script></h2>
					<p class="box_msg"><script>show_words(IPV6_TEXT61);</script> </p>
	                <table cellSpacing=1 cellPadding=1 width=525 border=0>
                            <tr>
                              <td width="187" align=right class="duple">Enable Hub and Spoke Mode :</td>
				<td width="331">&nbsp;<input type=checkbox id="ipv6_6rd_hub_enable" name="ipv6_6rd_hub_enable" value="1"></td>
                            </tr>	
<tr>
<td width="185" align=right class="duple"><script>show_words(IPV6_6rd_config);</script> :</td>
<td width="331">&nbsp;
<input type="radio" name="ipv6_6rd_dhcp" value="1" onClick="dhcp_option()" checked>
<script>show_words(IPV6_6rd_dhcp);</script>
<input type="radio" name="ipv6_6rd_dhcp" value="0" onClick="dhcp_option()">
<script>show_words(IPV6_6rd_manual);</script>
</td>
</tr>
				<tr>
	                      <td width="187" align=right class="duple"><script>show_words(IPV6_6rd_prefix);</script> :</td>
				<td width="331">&nbsp;<b>
					<input type=text id="ipv6_6rd_prefix" name="ipv6_6rd_prefix" size="30" maxlength="39" value="<% CmoGetCfg("ipv6_6rd_prefix","none"); %>" onChange="ipv6_6rd_prefix_onchange()">/
					<input type=text id="ipv6_6rd_prefix_length" name="ipv6_6rd_prefix_length" size="5" maxlength="2" value="<% CmoGetCfg("ipv6_6rd_prefix_length",""); %>" onChange="ipv6_6rd_prefix_onchange()">

				</b></td>
	                    </tr>
	                    <tr>
                              <td width="187" align=right class="duple"><script>show_words(IPV6_TEXT01);</script> :</td>
                              <td width="331">&nbsp;<b><span id="wan_ip"></span></b>&nbsp;&nbsp;&nbsp;<b><script>show_words(IPV6_6rd_mask);</script> :</b><input type=text id="ipv6_6rd_ipv4_mask_length" name="ipv6_6rd_ipv4_mask_length" size="5" maxlength="2" value="<% CmoGetCfg("ipv6_6rd_ipv4_mask_length","none"); %>" onChange="ipv6_6rd_prefix_onchange()"></td>
                            </tr>
                            <tr>
                              <td width="187" align=right class="duple"><script>show_words(IPV6_6rd_assign);</script> :</td>
                              <td width="331">&nbsp;<b><span id="ipv6_6rd_assigned_prefix"></span></b></td>
                            </tr>
	                    <tr>
	                      <td width="187" align=right class="duple"><script>show_words(IPV6_6rd_tunnel);</script> :</td>
	                      <td width="331">&nbsp;<b><span id="ipv6_6rd_addr"></span></b></td>
	                    </tr>
	                    <tr>
	                      <td width="187" align=right class="duple"><script>show_words(IPV6_6rd_relay);</script> :</td>
	                      <td width="331">&nbsp;<input type=text id="ipv6_6rd_relay" name="ipv6_6rd_relay" size="30" maxlength="39" value="<% CmoGetCfg("ipv6_6rd_relay","none"); %>"></td>
	                    </tr>
	                    <tr>
	                      <td width="187" align=right class="duple"><script>show_words(IPV6_PDNS)</script> :</td>
	                      <td width="331">&nbsp;<input type=text id="ipv6_6rd_primary_dns" name="ipv6_6rd_primary_dns" size="30" maxlength="39" value="<% CmoGetCfg("ipv6_6rd_primary_dns","none"); %>"></td>
	                    </tr>
	                    <tr>
	                      <td width="187" align=right class="duple"><script>show_words(IPV6_SDNS)</script> :</td>
	                      <td width="331">&nbsp;<input type=text id="ipv6_6rd_secondary_dns" name="ipv6_6rd_secondary_dns" size="30" maxlength="39" value="<% CmoGetCfg("ipv6_6rd_secondary_dns","none"); %>"></td>
	                    </tr>
	                </table>
                 </div>
		 <div class=box id=lan_ipv6_settings>
	                <h2 style=" text-transform:none"><script>show_words(IPV6_TEXT44);</script></h2>
					<p align="justify" class="style1"><script>show_words(IPV6_TEXT45)</script> </p>
	                <table cellSpacing=1 cellPadding=1 width=525 border=0>
	                    <tr>
	                      <td width="187" align=right class="duple"><script>show_words(IPV6_TEXT46);</script> :</td>
	                      <td width="331">&nbsp;
							<b><span id="lan_ipv6_ip_lan_ip"></span></b>
                      	  </td>
			    </tr>
			    <tr>
			      <td width="187" align=right class="duple"><script>show_words(IPV6_TEXT47);</script> :</td>
	                      <td width="331">&nbsp;
	                      	<b><span id="lan_link_local_ip"></span></b>
	                      </td>
                      </tr>
			</table>
                 </div>
		 <div class="box" id=ipv6_autoconfiguration_settings>
			  <h2><script>show_words(IPV6_TEXT48);</script></h2>
			   <p align="justify" class="style1"><script>show_words(IPV6_TEXT49)</script></p>
                          <table width="525" border=0 cellPadding=1 cellSpacing=1 class=formarea summary="">
                                <tr>
                                  <td width="187" class="duple"><script>show_words(IPV6_TEXT50);</script> :</td>
                                  <td width="331">&nbsp;<input name="ipv6_autoconfig_sel" type=checkbox id="ipv6_autoconfig_sel" value="1" onClick="disable_autoconfig()"></td>
                                </tr>
                                <tr>
                                <td class="duple"><script>show_words(IPV6_TEXT51);</script> :</td>
                                  <td width="331">&nbsp;
				   <select id="ipv6_autoconfig_type" name="ipv6_autoconfig_type" onChange="set_ipv6_autoconfiguration_type()">
				      <option value="stateless"><script>show_words(IPV6_TEXT52);</script></option>
				      <option value="stateful"><script>show_words(IPV6_TEXT53);</script></option>
				      <option value="stateless_dhcp"><script>show_words(IPV6_TEXT53a);</script></option>
				   </select>
				  </td>
                                </tr>
                                <tr id="show_ipv6_addr_range_start" style="display:none">
				     <td class="duple"><script>show_words(IPV6_TEXT54);</script>:</td>
				     <td width="331">&nbsp;&nbsp;<input type=text id="ipv6_addr_range_start_prefix" name="ipv6_addr_range_start_prefix" size="20" maxlength="19">
				     ::<input type=text id="ipv6_addr_range_start_suffix" name="ipv6_addr_range_start_suffix" size="5" maxlength="4">
				     </td>
				</tr>
				<tr id="show_ipv6_addr_range_end" style="display:none">
				     <td class="duple"><script>show_words(IPV6_TEXT55);</script>:</td>
				     <td width="331">&nbsp;&nbsp;<input type=text id="ipv6_addr_range_end_prefix" name="ipv6_addr_range_end_prefix" size="20" maxlength="19">
				     ::<input type=text id="ipv6_addr_range_end_suffix" name="ipv6_addr_range_end_suffix" size="5" maxlength="4">
				     </td>
				</tr>
          </form>
				<tr id="show_ipv6_addr_lifetime" style="display:none">
				     <td class="duple"><script>show_words(IPV6_TEXT56);</script> :</td>
				     <td width="331">&nbsp;&nbsp;<input type=text id="ipv6_dhcpd_lifetime" name="ipv6_dhcpd_lifetime" size="20" maxlength="15" value="">
				     <script>show_words(_minutes);</script></td>
				</tr>
				<tr id="show_router_advert_lifetime">
				     <td class="duple"><script>show_words(IPV6_TEXT57);</script> :</td>
				     <td width="331">&nbsp;&nbsp;<input type=text id="ipv6_6rd_adver_lifetime" name="ipv6_6rd_adver_lifetime" size="20" maxlength="15" value="">
				     <script>show_words(_minutes);</script></td>
				</tr>
                          </table>
		 </div>
               </td>
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table borderColor=#ffffff cellSpacing=0 borderColorDark=#ffffff
      cellPadding=2 bgColor=#ffffff borderColorLight=#ffffff border=0>
                  <tbody>
                    <tr>
                      <td id=help_text><b><script>show_words(_hints)</script>&hellip;</b>
                      	<p><script>show_words(IPV6_TEXT58);</script></p>
              			<p><script>show_words(IPV6_TEXT59);</script></p>
						<p><a href="support_internet.asp#ipv6" onclick="return jump_if();"><script>show_words(_more);</script>&hellip;</a></p>
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
