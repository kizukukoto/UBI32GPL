<html lang=en-US xml:lang="en-US" xmlns="http://www.w3.org/1999/xhtml">
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="Javascript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var count = 70;
    var login_who = "<% CmoGetStatus("get_current_user"); %>";

    function onPageLoad()
    {
        var temp_count = "<% CmoGetCfg("countdown_time","none"); %>";
        if (temp_count != "") {
            count = parseInt(temp_count);
        }

//        get_by_id("html_response_page").value = get_by_id("html_response_return_page").value;
        do_count_down();
    }

function sleep(milliseconds) {
  var start = new Date().getTime();
  for (var i = 0; i < 1e7; i++) {
    if ((new Date().getTime() - start) > milliseconds){
      break;
    }
  }
}
    function confirm_reboot()
    {
//                send_submit("form6");
//document.form6.submit();
                send_submit("form6");
//		return true;
//		sleep(2000);
                return;

        if (login_who == "user") {
            window.location.href ="back.asp";
        }
        else {
            if (confirm(msg[REBOOT_ROUTER])) {
                send_submit("form6");
            }
        }
    }

    function do_count_down()
    {
        get_by_id("show_sec").innerHTML = count;

        if (count == 0) {
            back();
            return;
        }

        if (count > 0) {
            count--;
            setTimeout('do_count_down()', 1000);
        }
    }

    function back()
    {
        if ("<% CmoGetStatus("get_current_user"); %>" == "user")
            window.location.href ="index.asp";
        else {
        	var new_url;
					var deviceid = "<% CmoGetCfg("opendns_deviceid","none"); %>";
        	new_url = "http://www.opendns.com/device/welcome/?device_id=" +  deviceid;
//            window.location.href = get_by_id("html_response_page").value;
					new_url = "opendns_Wait.asp";
            window.location.href = new_url;
            
        }
    }
</script>
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<style type="text/css">
</style>
</head>

<body onload="onPageLoad();" >
	<center>
									<br>
                    <div class="centered">You have successfully configured your router to use OpenDNS&reg; Parental Controls. </div>
                  <br ><div class="centered">Please wait <span id="show_sec"></span>&nbsp;seconds.</div><br>
  					<br>
 	</center>				
  					

<script src=https://www-files.opendns.com/js/router.js></script>

</BODY>

</html>
