<html lang=en-US xml:lang="en-US" xmlns="http://www.w3.org/1999/xhtml">
<html><head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<meta http-equiv="Content-Type" content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<LINK href="form_css.css" type=text/css rel=stylesheet>
</head>
<body>
<form id="form1" name="form1" method="post" action="apply.cgi" target="_parent">
<input type="hidden" id="html_response_page" name="html_response_page" value="please_wait.asp">
<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp">
<input type="hidden" id="reboot_type" name="reboot_type" value="none">
</form>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
	function chk_result(){
		if("<% CmoGetStatus("if_measuring_uplink_now"); %>" == "1"){
		//	parent.document.location.href ="please_wait.asp";
			document.getElementById("form1").submit();
			
		}
	}
	chk_result();
</script>
</body>
</html>
