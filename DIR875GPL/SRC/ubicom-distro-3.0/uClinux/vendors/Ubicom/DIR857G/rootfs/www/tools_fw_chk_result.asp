<html lang=en-US xml:lang="en-US" xmlns="http://www.w3.org/1999/xhtml">
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<meta http-equiv="Content-Type" content="text/html; charset=UTF8">
<LINK href="form_css.css" type=text/css rel=stylesheet>
</head>
<body>
<script language="JavaScript">
function add_downloadsite_info(cur_ver, result, type){
	var rtn=1;
	if(result =="LATEST"){
		rtn=0;
	}else if(result.length > 0 && result !="ERROR"){
		var fm_info = result.split("+");
		if(parseFloat(fm_info[0]) <= parseFloat(cur_ver))
			rtn=0;
		else{
			parent.document.getElementById( type + "downloadsite_info").style.display="";
			parent.document.getElementById( type + "latest_version").innerHTML = fm_info[0] //new firmware
			parent.document.getElementById( type + "latest_date").innerHTML = fm_info[1] //new date

		var site = fm_info[2].split(",");
		var site_url = fm_info[3].split(",");
			var obj = parent.document.getElementById( type + "sel_site");
		var count = 0;

		for (var i = 0; i < site.length; i++){
			var ooption = parent.document.createElement("option");
			ooption.text = site[i];
			ooption.value = site_url[i];
			obj.options[count++] = ooption;
		}
		}
	}else{
		rtn=0;
	}
	parent.document.getElementById("check_now_b").disabled = false;
	return rtn;
}

function show_online_check_result(){
	var fw_version ="<% CmoGetStatus("version"); %>";
	var online_firmware_check_result = "<% CmoGetStatus("online_firmware_check"); %>";
	if( add_downloadsite_info(fw_version,online_firmware_check_result,"FW") ==1)
		parent.document.getElementById("downloadsite_info").style.display ="";
	else
		parent.document.getElementById("check_result").innerHTML = TEXT045;
/*IFDEF CONFIG_LP*/
	var lp_version = "<% CmoGetStatus("lp_version"); %>";
	var online_lp_check_result = "<% CmoGetStatus("online_lp_check"); %>";
	if(add_downloadsite_info(lp_version, online_lp_check_result,"LP") ==1)
		parent.document.getElementById("downloadsite_info").style.display ="";
/*ENDIF CONFIG_LP*/
}

</script>
<script>
show_online_check_result();
</script>
</body>
</html>
