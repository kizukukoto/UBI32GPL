<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script type="text/javascript" src="jquery-1.6.1.min.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script>


	var connect;
	var wTimesec = "", wan_time = "";	

	function set_mac_info()
	{
		var temp_dhcp_list = get_by_id("dhcp_list").value.split("@");

		for (var i = 0; i < temp_dhcp_list.length; i++)
		{
			var temp_data = temp_dhcp_list[i].split("/");
			if(temp_data.length != 0)
			{
				if(temp_data.length > 1)
				{
					var temp_ipv6_address = temp_data[2].split(",");
					if(temp_ipv6_address.length > 1)
					{
						for(var j = 0; j < temp_ipv6_address.length;j++)
						{
							if(j > 0)
							{
								temp_data[1]="";
								temp_data[0]="";
							}
							document.write("<tr><td>" + temp_ipv6_address[j] + "</td><td>" + temp_data[1] + "</td></tr>")
						}
					}
					else
              document.write("<tr><td>" + temp_data[2] + "</td><td>" + temp_data[1] + "</td></tr>")
				}
			}
		}
	}

        var wan_connection_type = function(conn_type) 
        {
		get_by_id("disconnect").disabled = true;
		get_by_id("connect").disabled = true;
		var wan_url = conn_type+".asp";
		$.ajax({
			type : "GET",
			url : wan_url,
			cache : false,
			success : function(res) {
			}
			
		});
        };

function replace_null_to_none(item){
	if(item=="(null)"|| item == "null" || item == "NULL")
		item=_none;
	return item;
}

function xmldoc(){
	var ipv6_wan_type;
	var network_ipv6_status;
	var ipv6_network_status_flag;
	var tr_dhcp_pd_networkassined;
	var tr_network_ipv6_status;
	var tr_wan_ipv6_addr;
	var	tr_lan_ipv6_addr;
	var	tr_wan_ipv6_gw;	
	var	tr_primary_ipv6_dns;
	var	tr_secondary_ipv6_dns;
	var tr_wan_ipv6_addr_local;
	var tr_dhcp_pd;
	var tr_show_button;
	var ipv6_wan_ip;
	var ipv6_wan_ip_local;
	var ipv6_wan_ip_list;
	var ipv6_wan_gw;
	var ipv6_lan_ip;
	var ipv6_primary_dns;
	var ipv6_secondary_dns;
	var link_local;
	var dhcp_pd;
	var dhcp_pd_networkassined;
	var ipv6_sel_wan = "<% CmoGetCfg("ipv6_wan_proto","none"); %>";
	var ipv6_pppoe_share = "<% CmoGetCfg("ipv6_pppoe_share","none"); %>"; 
	var ipv6_dhcp_ip;
	var show_button;	
	if ( ! (xmlhttp.readyState == 4 && xmlhttp.status == 200) )
		return;

	ipv6_wan_type = get_by_id("connection_ipv6_type");
		network_ipv6_status = get_by_id("network_ipv6_status");
		tr_dhcp_pd_networkassined=get_by_id("tr_dhcp_pd_networkassined");
		tr_network_ipv6_status=get_by_id("tr_network_ipv6_status");
		tr_show_button=get_by_id("tr_show_button");
		tr_wan_ipv6_addr=get_by_id("tr_wan_ipv6_addr");
		tr_lan_ipv6_addr=get_by_id("tr_lan_ipv6_addr");
		tr_wan_ipv6_gw=get_by_id("tr_wan_ipv6_gw");
		tr_primary_ipv6_dns=get_by_id("tr_primary_ipv6_dns");
		tr_secondary_ipv6_dns=get_by_id("tr_secondary_ipv6_dns");
		tr_wan_ipv6_addr_local=get_by_id("tr_wan_ipv6_addr_local");
		tr_dhcp_pd=get_by_id("tr_dhcp_pd");
	ipv6_wan_ip = get_by_id("wan_ipv6_addr");
	ipv6_wan_ip_local = get_by_id("wan_ipv6_addr_local");
	ipv6_wan_gw = get_by_id("wan_ipv6_gw");
	ipv6_lan_ip = get_by_id("lan_ipv6_addr");
	ipv6_primary_dns = get_by_id("primary_ipv6_dns");
	ipv6_secondary_dns = get_by_id("secondary_ipv6_dns");
	link_local = get_by_id("lan_link_local_ip");
	dhcp_pd = get_by_id("dhcp_pd");
	dhcp_pd_networkassined = get_by_id("dhcp_pd_networkassined");
		show_button = get_by_id("show_button");

var xml = xmlhttp.responseXML;

		ipv6_sel_wan = replace_null_to_none(xml.getElementsByTagName("ipv6_wan_proto")[0].firstChild.nodeValue);
		ipv6_network_status_flag=xml.getElementsByTagName("ipv6_wan_network_status")[0].firstChild.nodeValue;
		ipv6_wan_ip_list =replace_null_to_none(xml.getElementsByTagName("ipv6_wan_ip")[0].firstChild.nodeValue).split(",");
		ipv6_primary_dns.innerHTML = replace_null_to_none(xml.getElementsByTagName("ipv6_wan_primary_dns")[0].firstChild.nodeValue);
		ipv6_secondary_dns.innerHTML = replace_null_to_none(xml.getElementsByTagName("ipv6_wan_secondary_dns")[0].firstChild.nodeValue);
	if (ipv6_network_status_flag == "connect") {
		network_ipv6_status.innerHTML = CONNECTED;
		connect = 1;
        }else{
			ipv6_wan_ip_list = "None".split(",");
		ipv6_primary_dns.innerHTML = _none;
		ipv6_secondary_dns.innerHTML = _none;
			network_ipv6_status.innerHTML = DISCONNECTED;
			connect = 0;
             }
		ipv6_wan_ip_local.innerHTML = replace_null_to_none(xml.getElementsByTagName("ipv6_wan_ip_local")[0].firstChild.nodeValue);
		ipv6_lan_ip.innerHTML = replace_null_to_none(xml.getElementsByTagName("ipv6_lan_ip_global")[0].firstChild.nodeValue);
		link_local.innerHTML = replace_null_to_none(xml.getElementsByTagName("ipv6_lan_ip_local")[0].firstChild.nodeValue);
		ipv6_wan_gw.innerHTML = replace_null_to_none(xml.getElementsByTagName("ipv6_wan_default_gateway")[0].firstChild.nodeValue);
		dhcp_pd_status = replace_null_to_none(xml.getElementsByTagName("ipv6_dhcp_pd_enable")[0].firstChild.nodeValue);
		dhcp_pd_networkassined_status = replace_null_to_none(xml.getElementsByTagName("ipv6_dhcp_pd")[0].firstChild.nodeValue);
	dhcp_pd.innerHTML = dhcp_pd_status;
	dhcp_pd_networkassined.innerHTML = dhcp_pd_networkassined_status;
		ipv6_dhcp_ip = replace_null_to_none(xml.getElementsByTagName("ipv6_dhcp_ip")[0].firstChild.nodeValue);
		wTimesec = xmlhttp.responseXML.getElementsByTagName("ipv6_wan_uptime")[0].firstChild.nodeValue;
	if(dhcp_pd_status =="Disabled"){
		tr_dhcp_pd_networkassined.style.display="none";
	}else{
		tr_dhcp_pd_networkassined.style.display="";
	}

	for (var i = 0; i < ipv6_wan_ip_list.length; i++){
		if(i==0){
			ipv6_wan_ip.innerHTML = ipv6_wan_ip_list[i];
		}else{
			ipv6_wan_ip.innerHTML +="<br>&nbsp;";
			ipv6_wan_ip.innerHTML += ipv6_wan_ip_list[i];
		}
	}

	tr_network_ipv6_status.style.display="";
	tr_wan_ipv6_addr.style.display="";
	tr_lan_ipv6_addr.style.display="";
	tr_wan_ipv6_gw.style.display="";	
	tr_primary_ipv6_dns.style.display="";
	tr_secondary_ipv6_dns.style.display="";
		tr_dhcp_pd_networkassined.style.display="";
		tr_show_button.style.display="";
	if(ipv6_sel_wan =="ipv6_static"){
		var use_link_local = "<% CmoGetCfg("ipv6_use_link_local","none"); %>";
		if ( use_link_local == 1)
				ipv6_wan_ip.innerHTML = replace_null_to_none(xml.getElementsByTagName("ipv6_wan_ip_local")[0].firstChild.nodeValue);
		      ipv6_wan_type.innerHTML = IPV6_TEXT32;
		      tr_wan_ipv6_addr_local.style.display="none";
          tr_dhcp_pd.style.display="none";
          tr_dhcp_pd_networkassined.style.display="none";
			tr_show_button.style.display="none";
			tr_show_uptime.style.display="none";
	
 	}else if(ipv6_sel_wan =="ipv6_autoconfig"){
		ipv6_wan_type.innerHTML = IPV6_TEXT32a;
		tr_wan_ipv6_addr_local.style.display="none";
/*
			if ("<% CmoGetCfg("ipv6_wan_proto","none"); %>" == "ipv6_autodetect")
				tr_show_button.style.display="none";
			else
*/
		 {
				show_button.innerHTML = "<input type=button id=connect name=connect value=\"" + LS312 + "\" onclick=ipv6_dhcp_renew(ipv6_dhcp_renew)>";
				show_button.innerHTML += "<input type=button id=disconnect name=disconnect value=\"" + LS313 + "\" onclick=ipv6_dhcp_release()>";
			
			//alert("ipv6_dhcp_ip=" + ipv6_dhcp_ip + "," + "dhcp_pd_networkassined_status=" + dhcp_pd_networkassined_status);
			if (ipv6_dhcp_ip != "None" || dhcp_pd_networkassined_status != "None") {
				//alert("1");
				get_by_id("disconnect").disabled = false;
				get_by_id("connect").disabled = false;
			} else {
				//alert("2");
				get_by_id("disconnect").disabled = true;
				get_by_id("connect").disabled = false;
			}
			}
	}else if(ipv6_sel_wan =="ipv6_autodetect"){ 
     			ipv6_wan_type.innerHTML = IPV6_TEXT32b ; 
			tr_wan_ipv6_addr_local.style.display="none";              
			tr_show_button.style.display="none";

	}else if(ipv6_sel_wan =="ipv6_pppoe"){
                ipv6_wan_type.innerHTML = IPV6_TEXT34;
		tr_wan_ipv6_addr_local.style.display="none";		  
/*		  
			if ("<% CmoGetCfg("ipv6_wan_proto","none"); %>" == "ipv6_autodetect")
				tr_show_button.style.display="none";
			else 
*/			
			
			if(ipv6_pppoe_share == "0"){
				//show_button.innerHTML = "<input type=button id=connect name=connect value=\"" + _connect + "\" onclick=ipv6_pppoe_connect()>";
				//show_button.innerHTML += "<input type=button id=disconnect name=disconnect value=\"" + _disonnect + "\" onclick=ipv6_pppoe_disconnect()>";
				show_button.innerHTML = '<input type=button id=connect name=connect value='+_connect+' onClick=ipv6_pppoe_connect()>&nbsp;<input type=button id=disconnect name=disconnect value='+sd_Disconnect+' onClick=ipv6_pppoe_disconnect()>';
				if (ipv6_network_status_flag == "connect") {
					get_by_id("disconnect").disabled = false;
					get_by_id("connect").disabled = true;
				} else {
					get_by_id("disconnect").disabled = true;
					get_by_id("connect").disabled = false;
				}
			}else if(ipv6_pppoe_share == "1"){
				//show_button.innerHTML = "<input type=button id=connect name=connect value=\"" + _connect + "\" onclick=wan_connection_type(\"pppoe_connect\")>";
				//show_button.innerHTML += "<input type=button id=disconnect name=disconnect value=\"" + _disonnect + "\" onclick=wan_connection_type(\"pppoe_disconnect\")>";
   				show_button.innerHTML = '<input type=button id=connect name=connect value='+_connect+' onClick=ipv6_pppoe_connect()>&nbsp;<input type=button id=disconnect name=disconnect value='+sd_Disconnect+' onClick=ipv6_pppoe_disconnect()>';
		                if (ipv6_network_status_flag == "connect") {
                	        get_by_id("disconnect").disabled = false;
                        	get_by_id("connect").disabled = true;
	                } else {
        	                get_by_id("disconnect").disabled = true;
                	        get_by_id("connect").disabled = false;
									}
               }
	}else if(ipv6_sel_wan =="ipv6_6in4"){
                  ipv6_wan_type.innerHTML = IPV6_TEXT35;
			tr_wan_ipv6_addr_local.style.display="none";		  
			tr_show_button.style.display="none";
			tr_show_uptime.style.display="none";

	}else if(ipv6_sel_wan =="ipv6_6to4"){
		  ipv6_wan_type.innerHTML = IPV6_TEXT36;
		  		tr_wan_ipv6_addr_local.style.display="none";		  
          tr_dhcp_pd.style.display="none";
          tr_dhcp_pd_networkassined.style.display="none";
			tr_show_button.style.display="none";
			tr_show_uptime.style.display="none";
          
	}else if(ipv6_sel_wan =="ipv6_6rd"){
		      ipv6_wan_type.innerHTML = IPV6_TEXT36a;
		  		tr_wan_ipv6_addr.style.display="none";		  
          tr_dhcp_pd.style.display="none";
          tr_dhcp_pd_networkassined.style.display="none";
			tr_show_button.style.display="none";
			tr_show_uptime.style.display="none";

	}else if(ipv6_sel_wan =="link_local"){
		  tr_network_ipv6_status.style.display="none";
		  tr_wan_ipv6_addr.style.display="none";
			tr_wan_ipv6_addr_local.style.display="none";		  
		  tr_lan_ipv6_addr.style.display="none";
		  tr_primary_ipv6_dns.style.display="none";
		  tr_secondary_ipv6_dns.style.display="none";
		tr_dhcp_pd.style.display="none";
		tr_wan_ipv6_gw.style.display="none";	
		tr_dhcp_pd_networkassined.style.display="none";
		tr_show_button.style.display="none";
		tr_show_uptime.style.display="none";
                  ipv6_wan_type.innerHTML = IPV6_TEXT37a;
}
               if(wTimesec == 0 || connect == 0){
                                              wan_time = _NA;
                                      }else{
                                              caculate_time();
                                      }
}

        function padout(number)
        {
                return (number < 10) ? '0' + number : number;
        }

        function caculate_time(){
                var wTime = wTimesec;
                var days = Math.floor(wTime / 86400);
                wTime %= 86400;
                var hours = Math.floor(wTime / 3600);
                wTime %= 3600;
                var mins = Math.floor(wTime / 60);
                wTime %= 60;

                wan_time = days + " " + 
                        ((days <= 1) ? "Day" : "Days")
                        + ", ";
                wan_time += hours + ":" + padout(mins) + ":" + padout(wTime);
        }
        
        function get_wan_time()
        {
                wTimesec = <% CmoGetStatus("get_ipv6_wantime"); %>;
                if(wTimesec == 0 || connect == 0){
                        wan_time = "N/A";
                        get_by_id("show_uptime").innerHTML = wan_time;
                }else{
                        caculate_time();
                }
        }
        
        function WTime()
        {       
                if(connect == 0) {
                        wan_time = "N/A";
                        wTimesec = 0;
                }
                
                get_by_id("show_uptime").innerHTML = wan_time;
                if(wTimesec != 0){
                        wTimesec++;
                        caculate_time();
                }
                setTimeout('WTime()', 1000);
        }

function get_File() {
        xmlhttp = new createRequest();
        if(xmlhttp){
                var url = "";
                var temp_cURL = document.URL.split("/");
                for (var i = 0; i < temp_cURL.length-1; i++) {
                        if (i == 1) continue;
                        if ( i == 0)
                                url += temp_cURL[i] + "//";
                        else
                                url += temp_cURL[i] + "/";
                }
                url += "device.xml=ipv6_status";
                xmlhttp.onreadystatechange = xmldoc; 
                xmlhttp.open("GET", url, true); 
                xmlhttp.send(null); 
        }
        setTimeout("get_File()", 3000);
}

function createRequest() {
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

function show_status(ipv6_wan_proto){
	
//	var ipv6_pppoe_share = "<% CmoGetCfg("ipv6_pppoe_share","none"); %>"; 
	
//	get_by_id("tr_ipv6_net_status").style.display = "none";

	if(ipv6_wan_proto != ""){

		if( (ipv6_wan_proto == "ipv6_pppoe")){
			//get_by_id("tr_ipv6_net_status").style.display = "";
			get_by_id("show_button").innerHTML = '<input type=button id=connect name=connect value='+_connect+' onClick=ipv6_pppoe_connect()>&nbsp;<input type=button id=disconnect name=disconnect value='+sd_Disconnect+' onClick=ipv6_pppoe_disconnect()>';
	    	}
	 				if( (ipv6_wan_proto == "ipv6_autoconfig")){
						//get_by_id("tr_ipv6_net_status").style.display = "";
						get_by_id("show_button").innerHTML = '<input type=button id=connect name=connect value='+LS312+' onClick=ipv6_dhcp_renew()>&nbsp;<input type=button id=disconnect name=disconnect value='+LS313+' onClick=ipv6_dhcp_release()>';
	    		}
	}
    }

    function ipv6_pppoe_connect(){
var ipv6_pppoe_share = "<% CmoGetCfg("ipv6_pppoe_share","none"); %>";
        get_by_id("connect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
        	if(ipv6_pppoe_share == "0")
            	send_submit("form3");
          else  
          		send_submit("form4");
        }
    }

    function ipv6_pppoe_disconnect(){
var ipv6_pppoe_share = "<% CmoGetCfg("ipv6_pppoe_share","none"); %>";
        get_by_id("disconnect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
        		if(ipv6_pppoe_share == "0")
            		send_submit("form2");
            else
            		send_submit("form5");
        }
    }

function ipv6_dhcp_renew(){
			var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
        
        send_submit("form6");
		}
}

function ipv6_dhcp_release(){
var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
        
        send_submit("form7");
		}
}
</script>

<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style3 {font-size: 12px}
.style4 {
	font-size: 11px;
	font-weight: bold;
}
.style5 {font-size: 11px}
-->
</style>
</head>
<input type="hidden" id="dhcp_list" name="dhcp_list" value="<% CmoGetList("ipv6_client_list"); %>">


<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
	<input type="hidden" id="ipv6_6to4_tunnel_address" name="ipv6_6to4_tunnel_address" maxLength=80 size=80 value="<% CmoGetStatus("6to4_tunnel_address","none"); %>">
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
			<td id="topnavoff"><a href="index.asp"><script>show_words(_setup)</script></a></td>
			<td id="topnavoff"><a href="adv_virtual.asp"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp"><script>show_words(_tools)</script></a></td>
			<td id="topnavon"><a href="st_device.asp"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp"><script>show_words(_support)</script></a></td>
		</tr>
	</table>
	<table border="1" cellpadding="2" cellspacing="0" width="838" height="100%" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
		<tr>
			<td id="sidenav_container" valign="top" width="125" align="right">
				<table border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td id="sidenav_container">
							<div id="sidenav">
							<!-- === BEGIN SIDENAV === -->
								<ul>
									<li><div><a href="st_device.asp"><script>show_words(_devinfo)</script></a></div></li>
									<li><div><a href="st_log.asp"><script>show_words(_logs)</script></a></div></li>
									<li><div><a href="st_stats.asp"><script>show_words(_stats)</script></a></div></li>
									<li><div><a href="internet_sessions.asp"><script>show_words(YM157)</script></a></div></li>
									<li><div><a href="st_routing.asp"><script>show_words(_routing)</script></a></div></li> 
									<li><div><a href="st_wireless.asp"><script>show_words(_wireless)</script></a></div></li>
         					<li><div id="sidenavoff">IPv6</a></div></li>
									<li><div><a href="st_routing6.asp"><script>show_words(IPV6_routing)</script></a></div></li>
								</ul>
								<!-- === END SIDENAV === -->
							</div>
						</td>
					</tr>
				</table>
			</td>
			<form id="form1" name="form1" method="post" action="st_device.cgi">
			<td valign="top" id="maincontent_container">
				<div id="maincontent">
					<!-- === BEGIN MAINCONTENT === -->
					<div id="box_header">
						<h1 style=" text-transform:none"><script>show_words(TEXT068);</script></h1>
						<script>show_words(TEXT069);</script>
						<br>
						<br>
				  	</div>
				  	<div class="box">
						<h2 style=" text-transform:none"><script>show_words(TEXT070);</script></h2>
						<table cellpadding="1" cellspacing="1" border="0" width="525">
							<tr>
								<td width="340" align=right class="duple"  nowrap><script>show_words(IPV6_TEXT29a);</script> :</td>
								<td width="340">&nbsp;
									<span id="connection_ipv6_type" nowrap></span>
								</td>
							</tr>
							<tr id="tr_network_ipv6_status">
								<td align=right class="duple"  nowrap><script>show_words(_networkstate);</script> :</td>
								<td width="340">&nbsp;
								<span id="network_ipv6_status" nowrap></span>
								</td>
							</tr>
							<tr id="tr_show_uptime">
								<td class="duple"><script>show_words(_conuptime)</script> :</td>
								<td width="340">&nbsp;
								<span id="show_uptime" nowrap></span>
								</td>
							</tr>
							<tr id="tr_show_button">
								<td align=right class="duple"  nowrap></td>
								<td width="340">&nbsp;
								<span id="show_button" nowrap></span>
								</td>
                                			</tr>							
<tr id="tr_wan_ipv6_addr">
								<td align=right class="duple"  nowrap><script>show_words(TEXT071);</script> :</td>
								<td width="340">&nbsp;
								<span id="wan_ipv6_addr" nowrap></span>
								</td>
							</tr>
              <tr id="tr_wan_ipv6_addr_local">
                 <td align=right class="duple"  nowrap>Tunnel Link-Local Address :</td>
                 <td width="340">&nbsp;
                 <span id="wan_ipv6_addr_local" nowrap></span>
                 </td>
              </tr>
							<tr id="tr_wan_ipv6_gw">
								<td align=right class="duple"  nowrap>IPv6 <script>show_words(IPV6_TEXT75)</script> :</td>
								<td width="340">&nbsp;
								<span id="wan_ipv6_gw" nowrap></span>
								</td>
							</tr>
							<tr id="tr_lan_ipv6_addr">
								<td align=right class="duple"  nowrap><script>show_words(IPV6_TEXT46);</script> :</td>
								<td width="340">&nbsp;
								<span id="lan_ipv6_addr" nowrap></span>
								</td>
							</tr>
							<tr id="tr_lan_link_local_ip">
								<td align=right class="duple" nowrap><script>show_words(IPV6_TEXT47);</script> :</td>
								<td width="340">&nbsp;
								<span id="lan_link_local_ip" nowrap></span>
								</td>
							</tr>
							<tr id="tr_primary_ipv6_dns">
								<td align=right class="duple"  nowrap><script>show_words(wwa_pdns);</script> :</td>
								<td width="340">&nbsp;
								<span id="primary_ipv6_dns" nowrap></span>
								</td>
							</tr>
							<tr id="tr_secondary_ipv6_dns">
								<td align=right class="duple"  nowrap><script>show_words(wwa_sdns);</script> :</td>
								<td width="340">&nbsp;
								<span id="secondary_ipv6_dns" nowrap></span>
								</td>
							</tr>
							<tr id="tr_dhcp_pd">
								<td align=right class="duple"  nowrap>DHCP-PD :</td>
								<td width="337">&nbsp;
								<span id="dhcp_pd" nowrap></span>
								</td>
							</tr>
							<tr id="tr_dhcp_pd_networkassined" style="display:none ">
								<td align=right class="duple"  nowrap>IPv6 network assigned <br>by DHCP-PD:</td>
								<td width="337">&nbsp;
								<span id="dhcp_pd_networkassined" nowrap></span>
								</td>
							</tr>
						</table>
				  	</div>

				  <div class="box">
					  <h2 style=" text-transform:none"><script>show_words(TEXT072);</script></h2>
						  <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1>
						  	<tr>
								<td><b><script>show_words(IPV6_TEXT0);</script></b></td>
								<td><b><script>show_words(YM187);</script></b></td>
						 	</tr>
							<script>set_mac_info();</script>
						  </table>
				  </div>
				  <!-- === END MAINCONTENT === -->
      </div>
			</td></form>
		<form id="form3" name="form3" method="post" action="ipv6_pppoe_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="back.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
		<form id="form2" name="form2" method="post" action="ipv6_pppoe_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_ipv6.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"></form>
		<form id="form4" name="form4" method="post" action="pppoe_00_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="back.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
		<form id="form5" name="form5" method="post" action="pppoe_00_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_ipv6.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"></form>
		<form id="form6" name="form6" method="post" action="ipv6_dhcp_renew.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_ipv6.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
		<form id="form7" name="form7" method="post" action="ipv6_dhcp_release.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_ipv6.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
	
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table cellpadding="2" cellspacing="0" border="0" bgcolor="#FFFFFF">
					<tr>
					  <td id="help_text">
					  	<strong><script>show_words(_hints);</script>&hellip;</strong>
					  	<p><script>show_words(hhsd_intro);</script></p>
						<p><a href="support_status.asp#Device_Info"><script>show_words(_more);</script>&hellip;</a></p>
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
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
<br>
<script>
	get_wan_time();
	WTime();
	get_File();
</script>
</body>
</html>
