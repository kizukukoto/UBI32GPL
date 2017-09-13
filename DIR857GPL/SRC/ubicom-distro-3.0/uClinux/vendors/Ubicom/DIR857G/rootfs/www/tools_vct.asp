<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language=JavaScript>

    var ping_url;
    var ping_tx_count;
    var ping_rx_count;

    var rtt_min;
    var rtt_max;
    var rtt_avg;
    var rtt_tot;

    var host_name;
    var stop_now;
    var progress;

    function onPageLoad()
    {
    	progress = get_by_id("ping_check_progress_field");
        progress.innerHTML = tsc_pingt_msg1;
        get_by_id("ping_host").focus();

        rtt_min = 10000;
        rtt_max = 0;
        rtt_avg = 0;
        rtt_tot = 0;

	get_by_id("stop").disabled = true;
	get_by_id("stop6").disabled = true;
    }

    function ping_ready()
    {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
    	    var reply_time;
    	    var ttl;
    	    var refresh_time;

        	reply_time = xmlhttp.responseXML.getElementsByTagName("reply_millis")[0].firstChild.nodeValue;
        	ttl = xmlhttp.responseXML.getElementsByTagName("ttl")[0].firstChild.nodeValue;

        	if (reply_time == "-1") {
                progress.innerHTML += "No response from host, retrying..." + "<br>";
                refresh_time = 2000;
            }
            else {
            	refresh_time = 1000;
                update_stats(reply_time);
                progress.innerHTML += "Response from " + host_name + " received in " + reply_time + " milliseconds.  " +
                                      "TTL = " + ttl + "<br>";
            }

            if (stop_now == 0) {
            	window.setTimeout("ping_repeat();", refresh_time);
            }
            else {
                display_stats();
            }
        }
    }

    /*
     * update_stats()
     */
    function update_stats(reply_time)
    {
        ping_rx_count++;
        var rtt = reply_time * 1;

        if (rtt > rtt_max) {
            rtt_max = rtt;
        }
        if (rtt < rtt_min) {
            rtt_min = rtt;
        }

        rtt_tot += rtt;
        rtt_avg = parseInt(rtt_tot / ping_rx_count, 10);
    }

    /*
     * display_stats()
     */
    function display_stats()
    {
        progress.innerHTML += "User stopped<br/>";
        progress.innerHTML += "Pings sent: " + ping_tx_count + "<br/>";
        progress.innerHTML += "Pings received: " + ping_rx_count + "<br/>";
        progress.innerHTML += "Pings lost: " + (ping_tx_count - ping_rx_count)
                            + " (" + parseInt((100 - ((ping_rx_count / ping_tx_count) * 100)) + "", 10)
                            + "% loss)<br/>";
        if (ping_rx_count != 0) {
            progress.innerHTML += "Shortest ping time (in milliseconds): " + rtt_min + "<br/>";
            progress.innerHTML += "Longest ping time (in milliseconds): " + rtt_max + "<br/>";
            progress.innerHTML += "Average ping time (in milliseconds): " + rtt_avg + "<br/>";
        }

        ping_tx_count = 0;
        ping_rx_count = 0;

        rtt_min = 10000;
        rtt_max = 0;
        rtt_avg = 0;
        rtt_tot = 0;

        get_by_id("ping").disabled = false;
        get_by_id("stop").disabled = true;
        get_by_id("ping6").disabled = false;
        get_by_id("stop6").disabled = true;
    }

    function process_start(ipv6_type)
    {
        stop_now = 0;

        if (ipv6_type == 0)
            host_name = trim_string(get_by_id("ping_host").value);
        else
            host_name = trim_string(get_by_id("ping6_host").value);

        if (host_name == "" || (ipv6_type == 0 && host_name == "0.0.0.0")) {
            alert(msg[PING_IP_ERROR]);
            return;
        }

        process_clean();

        get_by_id("ping").disabled = true;
        get_by_id("ping6").disabled = true;

        if (ipv6_type == 0) {
            get_by_id("stop").disabled = false;
            get_by_id("stop6").disabled = true;
        }
        else {
            get_by_id("stop").disabled = true;
            get_by_id("stop6").disabled = false;
        }

        ping_tx_count = 1;
        ping_rx_count = 0;

        xmlhttp = createRequest();
        if (xmlhttp) {
            var lan_ip = "<% CmoGetCfg("lan_ipaddr", "none"); %>";
            var temp_cURL = document.URL.split("/");
            var mURL = temp_cURL[2];

            if (mURL == lan_ip) {
                ping_url = "http://" + lan_ip + "/ping_test.xml=" + ipv6_type + "_" + host_name;
            }
            else {
                ping_url = "http://" + mURL + "/ping_test.xml=" + ipv6_type + "_" + host_name;
            }

            xmlhttp.onreadystatechange = ping_ready;
            xmlhttp.open("GET", ping_url, true);
            xmlhttp.send(null);
        }
    }

    function ping_repeat()
    {
    	ping_tx_count++;

        xmlhttp = createRequest();
        xmlhttp.onreadystatechange = ping_ready;
        xmlhttp.open("GET", ping_url, true);
        xmlhttp.send(null);
    }

    function process_stop()
    {
    	stop_now = 1;
        get_by_id("stop").disabled = true;
	get_by_id("stop6").disabled = true;
    }

    /*
     * process_clean()
     */
    function process_clean()
    {
        document.getElementById("ping_list").innerHTML = "";
        progress.innerHTML = "";
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

    function check_ipv6_ip()
    {
		if (get_by_id("ping6_ipaddr").value == ""){
			alert(tsc_pingt_msg2);
			return false;
		}else{
			return true;
			//send_submit("form5");
		}
	}
</script>
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
    <table border="1" cellpadding="2" cellspacing="0" width="838" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
        <tr>
            <td id="sidenav_container" valign="top" width="125" align="right">
                <table border="0" cellpadding="0" cellspacing="0">
                    <tr>
                        <td id="sidenav_container">
                            <div id="sidenav">
                            <!-- === BEGIN SIDENAV === -->
                                <ul>
									<li><div><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_admin)</script></a></div></li>
									<li><div><a href="tools_time.asp" onclick="return jump_if();"><script>show_words(_time)</script></a></div></li>
									<li><div><a href="tools_syslog.asp" onclick="return jump_if();"><script>show_words(_syslog)</script></a></div></li>
									<li><div><a href="tools_email.asp" onclick="return jump_if();"><script>show_words(_email)</script></a></div></li>
									<li><div><a href="tools_system.asp" onclick="return jump_if();"><script>show_words(_system)</script></a></div></li>
									<li><div><a href="tools_firmw.asp" onclick="return jump_if();"><script>show_words(_firmware)</script></a></div></li>
									<li><div><a href="tools_ddns.asp" onclick="return jump_if();"><script>show_words(_dyndns)</script></a></div></li>
								  	<li><div id="sidenavoff"><script>show_words(_syscheck)</script></div></li>
									<li><div><a href="tools_schedules.asp" onclick="return jump_if();"><script>show_words(_scheds)</script></a></div></li>
                                </ul>
                                <!-- === END SIDENAV === -->
                            </div>
                        </td>
                    </tr>
                </table>
            </td>
			<td valign="top" id="maincontent_container">			  <div id="maincontent">
                    <!-- === BEGIN MAINCONTENT === -->
                  <div id="box_header">
          <h1>
            <script>show_words(tsc_pingt)</script>
          </h1>

          <p>
            <script>show_words(tsc_pingt_mesg)</script>
          </p>
                  </div>

                  <div class=box>
          <h2>
            <script>show_words(tsc_pingt)</script>
          </h2>
                    <!--P>Ping Test is used to send &quot;Ping&quot; packets to test if a computer is on the Internet.</P-->
                    <table cellSpacing=1 cellPadding=1 width=525 border=0>
                        <form id="form5" name="form5" method="post" action="ping_response.cgi">
                        <input type="hidden" id="html_response_page" name="html_response_page" value="tools_vct.asp">
                        <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="tools_vct.asp">
                          <tr>
                            <td><div align="right"><strong><script>show_words(tsc_pingt_h)</script>&nbsp;:&nbsp;</strong></div></td>
                            <td height="20" valign="top">
                              <div id="ping_list" style="display:none"></div>
                              <div id="ping_ip" style="display:none"></div>
                              <input type="text" id="ping_host" name="ping_host" size=30 maxlength=63>
                              &nbsp;&nbsp;&nbsp;&nbsp;
                              <input type="button" name="ping" id="ping" value="Ping" onClick="process_start(0);">
                              <input type="button" name="stop" id="stop" value="Stop" onClick="process_stop();"></td>
			<script>get_by_id("stop").value=_stop</script>
                          </tr>
                        </form>
                    </table>

                  </div>
                  <div class=box>
         <h2 style=" text-transform:none">IPv6 <script>show_words(tsc_pingt_msg102)</script></h2>
          <table cellSpacing=1 cellPadding=1 width=525 border=0>
          <form id="form6" name="form6" method="post" action="ping6_response.cgi">
           <input type="hidden" id="html_response_page" name="html_response_page" value="tools_vct.asp">
	    <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="tools_vct.asp">
                <tr><td> <div align="right"><strong><script>show_words(TEXT075)</script>&nbsp;:&nbsp;</strong></div></td>
							<td height="20" valign="top">
                             <div id="ping_list" style="display:none"></div>
                             <div id="ping_ip" style="display:none"></div>
							 <input type="text" id="ping6_host" name="ping6_host" size=30 maxlength=63>
							 &nbsp;&nbsp;&nbsp;&nbsp;
                             <input type="button" name="ping6" id="ping6" value="Ping" onClick="process_start(1);">
                             <input type="button" name="stop6" id="stop6" value="Stop" onClick="process_stop();">
			<script>get_by_id("stop6").value=_stop</script>
                             </td>
                </tr>
            </form>
           </table>
 </div>
                  <div class=box>
                    <h2><script>show_words(tsc_pingr)</script></h2>
                    <span id="ping_check_progress_field"></span>
                  </div>
                    <!-- === END MAINCONTENT === -->
                </div></td>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table cellpadding="2" cellspacing="0" border="0" bgcolor="#FFFFFF">
                    <tr>

          <td id=help_text><strong>
            <script>show_words(_hints)</script>
            &hellip;</strong> <p>
              <script>show_words(hhtsc_pingt_intro)</script>
              <br>
              <script>show_words(htsc_pingt_h)</script>
            </p>
					    <p class="more"><a href="support_tools.asp#System_Check" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
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
</html>
