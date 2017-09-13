<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var dDat = new Array();
    var rDat = new Array();
    var xmlhttp, temp_table, temp_sta_num;

    function get_File()
    {
        xmlhttp = createRequest();
        if (xmlhttp) {
            var url;
            var lan_ip="<% CmoGetCfg("lan_ipaddr","none"); %>";
            var temp_cURL = document.URL.split("/");
            var mURL = temp_cURL[2];
			var hURL = temp_cURL[0];
            if (mURL == lan_ip) {
				if (hURL == "https:")
					url="https://"+lan_ip+"/device.xml=wireless_list";
				else
                    url="http://"+lan_ip+"/device.xml=wireless_list";
            }
            else {
				if (hURL == "https:")
					url="https://"+mURL+"/device.xml=wireless_list";
				else
                    url="http://"+mURL+"/device.xml=wireless_list";
            }

            xmlhttp.onreadystatechange = xmldoc;
            xmlhttp.open("GET", url, true);
            xmlhttp.send(null);
        }
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

    function xmldoc()
    {
        var mac_address, ip_address, temp_type, temp_rate, temp_rsi;
        var newRow, newCell0, newCell1, newCell2, newCell3, newCell4, lenRow;
        var temp_lenRow;

        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
        	temp_table = get_by_id("station_table");
            temp_sta_num = xmlhttp.responseXML.getElementsByTagName("num_2")[0].firstChild.nodeValue;

            if (temp_sta_num > 0) {  //2.4G
                for (var i = 0; i < temp_sta_num; i++) {
                    lenRow = parseInt(temp_table.rows.length);
                    temp_lenRow = parseInt(lenRow) -1;
                    if (temp_lenRow < temp_sta_num) {
                        newRow = temp_table.insertRow(lenRow);
                        newCell0 = newRow.insertCell(0);
                        newCell1 = newRow.insertCell(1);
                        newCell2 = newRow.insertCell(2);
                        newCell3 = newRow.insertCell(3);
                        newCell4 = newRow.insertCell(4);
                    }
                    else if (temp_lenRow > temp_sta_num) {
                        lenRow = parseInt(temp_table.rows.length);
                        temp_lenRow = parseInt(lenRow) -1;
                        for ( var j = temp_lenRow; j > temp_sta_num; j--) {
                            newRow = temp_table.deleteRow(j);
                        }
                    }

                    mac_address = xmlhttp.responseXML.getElementsByTagName("mac_2")[i].firstChild.nodeValue;
                    ip_address = xmlhttp.responseXML.getElementsByTagName("ip_2")[i].firstChild.nodeValue;
                    temp_type = xmlhttp.responseXML.getElementsByTagName("type_2")[i].firstChild.nodeValue;
                    temp_rate = xmlhttp.responseXML.getElementsByTagName("rate_2")[i].firstChild.nodeValue;
                    temp_rsi = xmlhttp.responseXML.getElementsByTagName("rsi_2")[i].firstChild.nodeValue;

                    temp_table.rows[i+1].cells[0].innerHTML = "<center>" + mac_address +"</center>";
                    temp_table.rows[i+1].cells[1].innerHTML = "<center>" + ip_address +"</center>";
                    temp_table.rows[i+1].cells[2].innerHTML = "<center>" + temp_type +"</center>";
                    temp_table.rows[i+1].cells[3].innerHTML = "<center>" + temp_rate +"</center>";
                    temp_table.rows[i+1].cells[4].innerHTML = "<center>" + temp_rsi +"</center>";
                }
            }
            else if (temp_sta_num == 0) {
                lenRow = parseInt(temp_table.rows.length);
                temp_lenRow = parseInt(lenRow) -1;
                for (var j = temp_lenRow; j > temp_sta_num; j--) {
                    newRow = temp_table.deleteRow(j);
                }
            }

            get_by_id("show_resert").innerHTML = temp_sta_num;
            setTimeout("get_File()",10000);
        }
    }
</script>

<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style4 {
    font-size: 11px;
    font-weight: bold;
}
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
			<td id="topnavoff"><a href="index.asp"><script>show_words(_setup)</script></a></td>
			<td id="topnavoff"><a href="adv_virtual.asp"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp"><script>show_words(_tools)</script></a></td>
			<td id="topnavon"><a href="st_device.asp"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp"><script>show_words(_support)</script></a></td>
        </tr>
    </table>
    <table border="1" cellpadding="2" cellspacing="0" width="838" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
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
									<li><div id="sidenavoff"><script>show_words(_wireless)</script></div></li>
                                    <li><div><a href="st_ipv6.asp">IPv6</a></div></li>
									<li><div><a href="st_routing6.asp"><script>show_words(IPV6_routing)</script></a></div></li>
                                </ul>
                                <!-- === END SIDENAV === -->
                            </div>
                        </td>
                    </tr>
                <td>

                </td>
                </table>
            </td>

            <td valign="top" id="maincontent_container">
                <div id="maincontent">

                    <div id="box_header">
						<h1><script>show_words(_wireless)</script></h1>
						<p><script>show_words(sw_intro)</script></p>
                    </div>
                    <div class="box">
						<h2><script>show_words(sw_title_list)</script> - <script>show_words(GW_WLAN_RADIO_0_NAME)</script>:&nbsp;<span id="show_resert"></span></h2>
                            <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1 id="station_table">
                              <tr id="box_header">
							    <TD><b><script>show_words(sd_macaddr)</script></b></TD>
							    <TD><b><script>show_words(_ipaddr)</script></b></TD>
							    <TD><b><script>show_words(_mode)</script></b></TD>
							    <TD><b><script>show_words(_rate)</script></b></TD>
							    <TD><b><script>show_words(_rssi)</script></b></TD>
                              </tr>
                            </table>
                    </div>
                </div></td>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table cellpadding="2" cellspacing="0" border="0" bgcolor="#FFFFFF">
                    <tr>
					  <td id=help_text><strong><script>show_words(_hints)</script>&hellip;</strong>
                      	<p><script>show_words(hhsw_intro)</script></p>
                      	<p class="more"><a href="support_status.asp#Wireless"><script>show_words(_more)</script>&hellip;</a></p>
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
	get_File();
</script>
</html>
