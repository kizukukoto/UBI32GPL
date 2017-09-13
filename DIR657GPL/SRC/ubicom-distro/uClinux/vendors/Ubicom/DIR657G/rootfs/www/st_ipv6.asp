<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script>
var ipv6_pppoe_share = "<% CmoGetCfg("ipv6_pppoe_share","none"); %>"; 
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
                                                        document.write("<tr><td>" +temp_ipv6_address[j] + "</td><td>" +  temp_data[1]+ "</td></tr>")
						}
					}
					else
                                                document.write("<tr><td>" + temp_data[2] + "</td><td>" + temp_data[1] + "</td></tr>")
				}
			}
		}
	}

function replace_null_to_none(item){
	if(item=="(null)")
		item=_none;
	return item;
}

function xmldoc(){
	var ipv6_wan_type;
	var ipv6_network_status;
	var ipv6_network_status_flag;
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
	
	

	if ( ! (xmlhttp.readyState == 4 && xmlhttp.status == 200) )
		return;

	ipv6_wan_type = get_by_id("connection_ipv6_type");
	ipv6_network_status = get_by_id("network_ipv6_status");
	ipv6_wan_ip = get_by_id("wan_ipv6_addr");
	ipv6_wan_ip_local = get_by_id("wan_ipv6_addr_local");
	ipv6_wan_gw = get_by_id("wan_ipv6_gw");
	ipv6_lan_ip = get_by_id("lan_ipv6_addr");
	ipv6_primary_dns = get_by_id("primary_ipv6_dns");
	ipv6_secondary_dns = get_by_id("secondary_ipv6_dns");
	link_local = get_by_id("lan_link_local_ip");
	dhcp_pd = get_by_id("dhcp_pd");
	dhcp_pd_networkassined = get_by_id("dhcp_pd_networkassined");

	ipv6_sel_wan = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_wan_proto")[0].firstChild.nodeValue); 

	ipv6_network_status_flag = xmlhttp.responseXML.getElementsByTagName("ipv6_wan_network_status")[0].firstChild.nodeValue;

ipv6_wan_ip_local.innerHTML = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_wan_ip_local")[0].firstChild.nodeValue);

	if (ipv6_network_status_flag == "connect") {
		ipv6_network_status.innerHTML = CONNECTED;
		ipv6_wan_ip_list = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_wan_ip")[0].firstChild.nodeValue).split(",");
	} else {
		ipv6_network_status.innerHTML= DISCONNECTED;
		ipv6_wan_ip_list = "none".split(",");
	}

	show_status(ipv6_sel_wan);
	ipv6_lan_ip.innerHTML = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_lan_ip_global")[0].firstChild.nodeValue);
	link_local.innerHTML = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_lan_ip_local")[0].firstChild.nodeValue);
	ipv6_wan_gw.innerHTML = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_wan_default_gateway")[0].firstChild.nodeValue);

        if(ipv6_network_status_flag == "connect"){
		ipv6_primary_dns.innerHTML = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_wan_primary_dns")[0].firstChild.nodeValue);
		ipv6_secondary_dns.innerHTML = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_wan_secondary_dns")[0].firstChild.nodeValue);
		ipv6_network_status.innerHTML = CONNECTED;
		if(ipv6_sel_wan =="ipv6_pppoe"){
                        get_by_id("connect").disabled = true;
                }
        }else{
		ipv6_primary_dns.innerHTML = _none;
		ipv6_secondary_dns.innerHTML = _none;
		ipv6_network_status.innerHTML = DISCONNECTED;
		if(ipv6_sel_wan =="ipv6_pppoe"){
                    get_by_id("disconnect").disabled = true;
                }
             }
	dhcp_pd_status=xmlhttp.responseXML.getElementsByTagName("ipv6_dhcp_pd_enable")[0].firstChild.nodeValue;
	dhcp_pd_networkassined_status=xmlhttp.responseXML.getElementsByTagName("ipv6_dhcp_pd")[0].firstChild.nodeValue;

	if(dhcp_pd_status =="Enabled"){
		dhcp_pd.innerHTML=_enabled;
	}else if(dhcp_pd_status =="Disabled"){
		dhcp_pd.innerHTML=_disabled;
	}else{
	dhcp_pd.innerHTML = dhcp_pd_status;
	}

	if(dhcp_pd_networkassined_status =="(null)"){
		dhcp_pd_networkassined.innerHTML=_none;
	}else{
	dhcp_pd_networkassined.innerHTML = dhcp_pd_networkassined_status;
	}

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

	if(ipv6_sel_wan =="ipv6_static"){
		var use_link_local = "<% CmoGetCfg("ipv6_use_link_local","none"); %>";
		if ( use_link_local == 1)
			ipv6_wan_ip.innerHTML = replace_null_to_none(xmlhttp.responseXML.getElementsByTagName("ipv6_wan_ip_local")[0].firstChild.nodeValue);
		      ipv6_wan_type.innerHTML = IPV6_TEXT32;
		      tr_wan_ipv6_addr_local.style.display="none";
          tr_dhcp_pd.style.display="none";
          tr_dhcp_pd_networkassined.style.display="none";
 	}else if(ipv6_sel_wan =="ipv6_autoconfig"){
		ipv6_wan_type.innerHTML = IPV6_TEXT32a;
		tr_wan_ipv6_addr_local.style.display="none";
	/*	         
	}else if(ipv6_sel_wan =="ipv6_dhcp"){
                        ipv6_wan_type.innerHTML = "DHCPv6";
*/                        
	}else if(ipv6_sel_wan =="ipv6_autodetect"){ 
     					ipv6_wan_type.innerHTML = IPV6_TEXT32b ; 
							 tr_wan_ipv6_addr_local.style.display="none";              


	}else if(ipv6_sel_wan =="ipv6_pppoe"){
                ipv6_wan_type.innerHTML = IPV6_TEXT34;
		tr_wan_ipv6_addr_local.style.display="none";		  
			if(ipv6_pppoe_share == "0"){
		               // ipv6_wan_type.innerHTML += "<input type=button id=connect name=connect value=\"Connect\" onclick=wan_connection_type(\"ipv6_pppoe_connect\")>";
		               // ipv6_wan_type.innerHTML += "<input type=button id=disconnect name=disconnect value=\"Disonnect\" onclick=wan_connection_type(\"ipv6_pppoe_disconnect\")>";
		                
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
		 							 tr_wan_ipv6_gw.style.display="none";
	}else if(ipv6_sel_wan =="ipv6_6to4"){
		  ipv6_wan_type.innerHTML = IPV6_TEXT36;
		  		tr_wan_ipv6_addr_local.style.display="none";		  
          tr_dhcp_pd.style.display="none";
          tr_dhcp_pd_networkassined.style.display="none";
          
	}else if(ipv6_sel_wan =="ipv6_6rd"){
		      ipv6_wan_type.innerHTML = IPV6_TEXT36a;
		  		tr_wan_ipv6_addr.style.display="none";		  
          tr_dhcp_pd.style.display="none";
          tr_dhcp_pd_networkassined.style.display="none";
	}else if(ipv6_sel_wan =="link_local"){
		  tr_network_ipv6_status.style.display="none";
		  tr_wan_ipv6_addr.style.display="none";
		  tr_lan_ipv6_addr.style.display="none";
		  tr_primary_ipv6_dns.style.display="none";
		  tr_secondary_ipv6_dns.style.display="none";
		  tr_wan_ipv6_addr_local.style.display="none";
                  ipv6_wan_type.innerHTML = "Link Local";
 /*    
	}else if(ipv6_sel_wan =="ipv6_stateless"){
                  ipv6_wan_type.innerHTML = "Stateless Autoconfiguration";
*/
}
			}
function show_status(ipv6_wan_proto){
	
//	var ipv6_pppoe_share = "<% CmoGetCfg("ipv6_pppoe_share","none"); %>"; 
	
	get_by_id("tr_ipv6_net_status").style.display = "none";

	if(ipv6_wan_proto != ""){

		if( (ipv6_wan_proto == "ipv6_pppoe")){

			get_by_id("tr_ipv6_net_status").style.display = "";
			get_by_id("show_button").innerHTML = '<input type=button id=connect name=connect value='+_connect+' onClick=ipv6_pppoe_connect()>&nbsp;<input type=button id=disconnect name=disconnect value='+sd_Disconnect+' onClick=ipv6_pppoe_disconnect()>';
	    	}
	}
    }

    function ipv6_pppoe_connect(){
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

function get_File() {
	xmlhttp = createRequest();
	if(xmlhttp){
	var url;
		var lan_ip="<% CmoGetCfg("lan_ipaddr","none"); %>";
		var temp_cURL = document.URL.split("/");
		var mURL = temp_cURL[2];
		var hURL = temp_cURL[0];
		if(mURL == lan_ip){
				if (hURL == "https:")
								url="https://"+lan_ip+"/device.xml=ipv6_status";
		else
				url="http://"+lan_ip+"/device.xml=ipv6_status";
		}else{
				if (hURL == "https:")
								url="https://"+mURL+"/device.xml=ipv6_status";
				else
				url="http://"+mURL+"/device.xml=ipv6_status";
		}

	xmlhttp.onreadystatechange = xmldoc;
	xmlhttp.open("GET", url, true);
	xmlhttp.send(null);
	}
	setTimeout("get_File()",5000);
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
					<div id="box_header">
						<h1 style=" text-transform:none"><script>show_words(TEXT068)</script></h1>
						<script>show_words(TEXT069)</script>
						<br>
						<br>
				  	</div>
				  	<div class="box">
						<h2 style=" text-transform:none"><script>show_words(TEXT070)</script></h2>
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
                                                        <tr id="tr_ipv6_net_status" style="display:none">
                                  				<td class="duple">&nbsp;</td>
                                  				<td width="340">&nbsp;&nbsp;<span id="show_button"></span></td>
                                			</tr>							
<tr id="tr_wan_ipv6_addr">
								<td align=right class="duple"  nowrap><script>show_words(TEXT071)</script> :</td>
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
								<td align=right class="duple"  nowrap><script>show_words(wwa_sdns)</script> :</td>
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
							        <td><script>show_words(IPV6_TEXT0)</script></td>
								<td><script>show_words(YM187)</script></td>
							</tr>
							<script>set_mac_info();</script>
						  </table>
				  </div>

      </div>
			</td></form>
		<form id="form3" name="form3" method="post" action="ipv6_pppoe_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="back.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
		<form id="form2" name="form2" method="post" action="ipv6_pppoe_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_ipv6.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"></form>
		<form id="form4" name="form4" method="post" action="pppoe_00_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="back.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
		<form id="form5" name="form5" method="post" action="pppoe_00_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_ipv6.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_ipv6.asp"></form>
			<td valign="top" width="150" id="sidehelp_container" align="left">
				<table cellpadding="2" cellspacing="0" border="0" bgcolor="#FFFFFF">
					<tr>

          <td id="help_text"> <strong>
            <script>show_words(_hints)</script>
            &hellip;</strong>
            <p><script>show_words(hhsd_intro)</script></p>
						<p><a href="support_status.asp#Device_Info"><script>show_words(_more)</script>&hellip;</a></p>
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
	get_File();
</script>
</body>
</html>
