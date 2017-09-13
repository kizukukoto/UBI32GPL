<html>
<head>
<title>D-LINK CORPORATION, INC | WIRELESS ROUTER | HOME</title>
<meta http-equiv=Content-Type content="text/html; charset=UTF-8">
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script lang="javasddcript" src="jquery-1.4.2.min.js"></script>
<style type="text/css">
#loading{
	position: relative;
	margin:0px auto;
	*margin:0px auto;
	_margin:0px auto;
	width:500px;
	height:20px;
	background:#ffffff;
}
#loading div{
	width:4px;
	height:20px;
	background:#ff6600;
}

</style>
<script language="Javascript">

var submit_button_flag = 0;
var percentage=0;
var count = 0;

	function onPageLoad(){
		if("<% CmoGetCfg("html_response_page","none"); %>" == "wizard_ipv6_2.asp") {
			get_by_id("button_2").style.display="none";
			get_by_id("info_2").style.display="none";
			autodetect_start();
		} else {
			get_by_id("button_1").style.display="none";
			get_by_id("info_1").style.display="none";
			get_by_id("loading").style.display="none";
		}
		get_by_id("connect_b1").disabled = true;
		var i = 0
		if("<% CmoGetCfg("wan_proto","none"); %>" == "pppoe" ){
			
		for(i = 1 ; i <= 99 ; i ++){ 
				$('#loading div').animate({width:i*5+"px"});
	  }
	  }else{
	  			for(i = 1 ; i <= 24 ; i ++){ 
				$('#loading div').animate({width:i*20+"px"});
	  }
	  }

	}
	/*
	function get_bar(){
				var i = 0
				for(i = 0 ; i < 40 ; i ++){ 
						$('#loading div').animate({width:i*12+"px"});
		}

		}
		*/

	function replay_page() {
		get_by_id("html_response_page").value="wizard_ipv6_2.asp";
		get_by_id("html_response_return_page").value="wizard_ipv6_2.asp";
		get_by_id("asp_temp_42").value="wizard_ipv6_1.asp";
                send_submit('form1');
	}

	 function get_File() {
                xmlhttp = new createRequest();
                if(xmlhttp){
                        var url = "";
                        var temp_cURL = document.URL.split("/");
                        for (var i = 0; i < temp_cURL.length-1; i++) {
                                if (i == 1) continue;
                                if ( i == 0)
                                        url += temp_cURL[i] + "\x2F\x2F";
                                else
                                        url += temp_cURL[i] + "/";
                        }
                        url += "device.xml=ipv6_wizard_status";
                        xmlhttp.onreadystatechange = xmldoc; 
                        xmlhttp.open("GET", url, true); 
                        xmlhttp.send(null); 
                }
                setTimeout("get_File()", 1000);
        }
 function sleep( seconds ) {
	var timer = new Date();
	var time = timer.getTime();
	do
		timer = new Date();
	while( (timer.getTime() - time) < (seconds * 1000) );
}
       

	function xmldoc(){
		var _width;
		var _text;
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			try {
				percentage = xmlhttp.responseXML.getElementsByTagName("ipv6_wan_connect_percentage")[0].firstChild.nodeValue;
				if (percentage == 100) {
					$("#loading div").animate({width:"500px"});
					$("#loading").hide(500);
					get_by_id("connect_b1").disabled = false;
					get_by_id("html_response_page").value="wizard_ipv6_4.asp";
					window.location.href="wizard_ipv6_4.asp";
				}else if(percentage == -1){
					$("#loading div").animate({width:"500px"});
					$("#loading").hide(500);
		                        get_by_id("button_2").style.display="";
                		        get_by_id("info_2").style.display="";
		                        get_by_id("button_1").style.display="none";
		                        get_by_id("info_1").style.display="none";
                		        get_by_id("loading").style.display="none";
					get_by_id("html_response_page").value="wizard_ipv6_3.asp";
//				}else if(percentage != 0){
//					}else{
				}
//					_width = percentage*5+"px";
//					_text = percentage+"%";
//					$('#loading div').animate({width:_width});
					
//					$('#loading div').animate({width:"400px"});
					
					


					
					
//				}
//				$("#loading div").animate({width:"200px"});
//				$("#loading div").animate({width:"400px"});
/*
				else if(precentage == 1)
					$("#loading div").animate({width:"5px"});
									else if(precentage == 2)
					$("#loading div").animate({width:"10px"});
									else if(precentage == 3)
					$("#loading div").animate({width:"15px"});
									else if(precentage == 4)
					$("#loading div").animate({width:"20px"});
									else if(precentage == 5)
					$("#loading div").animate({width:"25px"});
									else if(precentage == 6)
					$("#loading div").animate({width:"30px"});
									else if(precentage == 7)
					$("#loading div").animate({width:"35px"});
									else if(precentage == 8)
					$("#loading div").animate({width:"40px"});
									else if(precentage == 9)
					$("#loading div").animate({width:"45px"});
									else if(precentage == 10)
					$("#loading div").animate({width:"50px"});
									else if(precentage == 11)
					$("#loading div").animate({width:"55px"});
									else if(precentage == 12)
					$("#loading div").animate({width:"60px"});
									else if(precentage == 13)
					$("#loading div").animate({width:"65px"});
									else if(precentage == 14)
					$("#loading div").animate({width:"70px"});
									else if(precentage == 15)
					$("#loading div").animate({width:"75px"});
									else if(precentage == 16)
					$("#loading div").animate({width:"80px"});
									else if(precentage == 17)
					$("#loading div").animate({width:"85px"});
									else if(precentage == 18)
					$("#loading div").animate({width:"90px"});
									else if(precentage == 19)
					$("#loading div").animate({width:"95px"});
									else if(precentage == 20)
					$("#loading div").animate({width:"100px"});
									else if(precentage == 21)
					$("#loading div").animate({width:"105px"});
									else if(precentage == 22)
					$("#loading div").animate({width:"110px"});
									else if(precentage == 23)
					$("#loading div").animate({width:"115px"});
									else if(precentage == 24)
					$("#loading div").animate({width:"120px"});
									else if(precentage == 25)
					$("#loading div").animate({width:"125px"});
									else if(precentage == 26)
					$("#loading div").animate({width:"130px"});
									else if(precentage == 27)
					$("#loading div").animate({width:"135px"});
									else if(precentage == 28)
					$("#loading div").animate({width:"140px"});
									else if(precentage == 29)
					$("#loading div").animate({width:"145px"});
									else if(precentage == 30)
					$("#loading div").animate({width:"150px"});
									else if(precentage == 31)
					$("#loading div").animate({width:"155px"});
									else if(precentage == 32)
					$("#loading div").animate({width:"160px"});
*/					
			}catch(e){
				percentage ="0";
			}
		}
	}
	
	function autodetect_start(){
		wan_con = new createRequest();
		if(wan_con){
                        var lan_ip="<% CmoGetCfg("lan_ipaddr","none"); %>";
                        var url;
                        var temp_cURL = document.URL.split("/");
                        var mURL = temp_cURL[2];
                        if(mURL == lan_ip){
                                url="http://"+lan_ip+"/wizard_autodetect_start.cgi";
                        }else{
                                url="http://"+mURL+"/wizard_autodetect_start.cgi";
                        }

                        wan_con.open("GET", url, true);
                        wan_con.send(null);
                }
	}

	function autodetect_stop(page){
		wan_con = new createRequest();
		if(wan_con){
                        var lan_ip="<% CmoGetCfg("lan_ipaddr","none"); %>";
                        var url;
                        var temp_cURL = document.URL.split("/");
                        var mURL = temp_cURL[2];
                        if(mURL == lan_ip){
                                url="http://"+lan_ip+"/wizard_autodetect_stop.cgi";
                        }else{
                                url="http://"+mURL+"/wizard_autodetect_stop.cgi";
                        }
			wan_con.onreadystatechange = function() {
				if (wan_con.readyState == 4 && wan_con.status == 200) {
					window.location.href=page;
				}
			}
                        wan_con.open("GET", url, true);
                        wan_con.send(null);
               }
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
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575" onload="onPageLoad();">
<table border=0 cellspacing=0 cellpadding=0 align=center width=838>
<tr>
	<td bgcolor="#FFFFFF">
	<table width=838 border=0 cellspacing=0 cellpadding=0 align=center height=100>
	<tr>
		<td bgcolor="#FFFFFF">
			<div align=center>
	<table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
      <tr>
        <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
        <td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
	    	<td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
		<td>&nbsp;</td>
      </tr>
    </table>
			<img src="wlan_masthead.gif" width="836" height="92" align="middle"> 
			</div>
		</td>
	</tr>
	</table>
	</td>
</tr>
<tr>
	<td bgcolor="#FFFFFF">
	<p>&nbsp;</p>
	<table width="650" border="0" align="center">
	<tr>
		<td>
			<div id=box_header>
			<h1 align="left"><script>show_words(IPV6_wizard_step_1);</script></h1>
			<div align="left">
			<p id="info_1" class="box_msg"><script>show_words(IPv6_wizard_4);</script></p>
			<p id="info_2" class="box_msg"><script>show_words(IPv6_wizard_5);</script></p>
			<div>
			<div id=w1>
			<form id="form1" name="form1" method="post" action="apply.cgi">
				<input type="hidden" name="html_response_page" id="html_response_page" value="wizard_ipv6_3.asp">
				<input type="hidden" name="html_response_message" value="">
				<input type="hidden" name="html_response_return_page" id="html_response_return_page" value="wizard_ipv6_3.asp">
				<input type="hidden" name="reboot_type" value="none">
				<input type="hidden" name="asp_temp_42" id="asp_temp_42" value="wizard_ipv6_2.asp">

			<table align="center" class=formarea>
			<tbody>
			<tr align="center">
				<div id="loading"><div></div></div>
			</tr>
			<tr id="button_1"align="center">
				<td>&nbsp;</td>
				<td align="center">&nbsp;<BR>
					<input type="button" class="button_submit" id="prev_b1" name="prev_b1" value="Prev" onclick="autodetect_stop('wizard_ipv6_1.asp')">
					<input type="button" class="button_submit" id="next_b1" name="next_b1" value="Next" onclick="autodetect_stop('wizard_ipv6_3.asp')">
					<input type="button" class="button_submit" id="cancel_b1" name="cancel_b1" value="Cancel" onclick="autodetect_stop('ipv6.asp')">
					<input type="submit" class="button_submit" id="connect_b1" name="connect_b1" value="connect" onclick="window.location.href='wizard_ipv6_4.asp'">
					<script>
						get_by_id("prev_b1").value = _prev;
						get_by_id("next_b1").value = _next;
						get_by_id("cancel_b1").value = _cancel;
						get_by_id("connect_b1").value = _connect;
					</script>
				</td>
			</tr>
			<tr id="button_2" align="center">
                                <td>&nbsp;</td>
                                <td align="center">&nbsp;<BR>
                                        <input type="button" class="button_submit" id="cancel_b2" name="cancel_b2" value="Cancel" onclick="window.location.href='ipv6.asp'">
                                        <input type="button" class="button_submit" id="again_b2" name="again_b2" value="Try again" onclick="return replay_page()">
                                        <input type="button" class="button_submit" id="connect_b2" name="connect_b2" value="connect" onclick="window.location.href='wizard_ipv6_3.asp'">
                                        <script>
                                                get_by_id("cancel_b2").value = _cancel;
                                                get_by_id("again_b2").value = IPv6_wizard_button_0;
                                                get_by_id("connect_b2").value = IPv6_wizard_button_1;
                                        </script>
                                </td>
                        </tr>
			</tbody>
			</table>

			<br>&nbsp;
			</form>
			</div><!--w1-->
			</div>
			</div><!--left-->
			</div><!--header-->
		</td>
	</tr>
	</table>
	<p>&nbsp;</p>
	</td>
</tr>
<tr>
	<td bgcolor="#FFFFFF">
	<table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="836" align="center">
	<tr>
		<td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
		<td width="10">&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
	</tr>
	</table>
	</td>
</tr>
</table>
<div id="copyright"><script>show_words(_copyright);</script></div>
<script>
	get_File();
</script>
</body>
</html>
