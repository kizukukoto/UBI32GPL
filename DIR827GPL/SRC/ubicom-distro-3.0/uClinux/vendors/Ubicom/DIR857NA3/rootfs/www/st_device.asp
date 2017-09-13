<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>_tmp.js"></script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script>
    function onPageLoad()
    {
        get_time();
        STime();
        WTime();
        get_File();

        if ("<% CmoGetCfg("wlan0_enable","none"); %>" == "0") {
            get_by_id("show_wlan0_dot11_mode").style.display = "none";
            get_by_id("show_wlan0_11n_protection").style.display = "none";
            get_by_id("show_wlan_channel").style.display = "none";
            get_by_id("show_wish_engine").style.display = "none";
            get_by_id("show_wps_enable").style.display = "none";
            get_by_id("show_ssid_lst").style.display = "none";
            get_by_id("show_ssid_table").style.display = "none";
        }
        else {
            get_by_id("show_wlan0_dot11_mode").style.display = "";
            get_by_id("show_wlan0_11n_protection").style.display = "";
            get_by_id("show_wlan_channel").style.display = "";
            get_by_id("show_wish_engine").style.display = "";
            get_by_id("show_wps_enable").style.display = "";
            get_by_id("show_ssid_lst").style.display = "";
            get_by_id("show_ssid_table").style.display = "";
        }

        if ("<% CmoGetCfg("wlan1_enable","none"); %>" == "0") {
            get_by_id("show_wlan1_dot11_mode").style.display = "none";
            get_by_id("show_wlan1_11n_protection").style.display = "none";
            get_by_id("show_wlan1_channel").style.display = "none";
            get_by_id("show_wish_engine1").style.display = "none";
            get_by_id("show_wps_enable1").style.display = "none";
            get_by_id("show_ssid_lst1").style.display = "none";
            get_by_id("show_ssid_table1").style.display = "none";
        }
        else {
            get_by_id("show_wlan1_dot11_mode").style.display = "";
            get_by_id("show_wlan1_11n_protection").style.display = "";
            get_by_id("show_wlan1_channel").style.display = "";
            get_by_id("show_wish_engine1").style.display = "";
            get_by_id("show_wps_enable1").style.display = "";
            get_by_id("show_ssid_lst1").style.display = "";
            get_by_id("show_ssid_table1").style.display = "";
        }

        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1,true);
            get_by_id("show_button").disabled = true;
        }

        if ("<% CmoGetCfg("wlan0_11n_protection", "none"); %>" == "auto")
        	get_by_id("wlan0_11n_prot").innerHTML = "20/40";
        else
        	get_by_id("wlan0_11n_prot").innerHTML = "<% CmoGetCfg("wlan0_11n_protection", "none"); %>";
        
        if ("<% CmoGetCfg("wlan0_vap1_enable", "none"); %>" == "1")
        	get_by_id("guest_zone_ssid").style.display = "";
        else
        	get_by_id("guest_zone_ssid").style.display = "none";
        	
        if ("<% CmoGetCfg("wlan1_11n_protection", "none"); %>" == "auto")
        	get_by_id("wlan1_11n_prot").innerHTML = "20/40 ";
        else
        	get_by_id("wlan1_11n_prot").innerHTML = "20 ";
        
        if ("<% CmoGetCfg("wlan1_vap1_enable", "none"); %>" == "1")
        	get_by_id("guest_zone_ssid1").style.display = "";
        else
        	get_by_id("guest_zone_ssid1").style.display = "none";        	
    }

    function set_mac_info(){
        var temp_dhcp_list = get_by_id("dhcp_list").value.split(",");

        for (var i = 0; i < temp_dhcp_list.length; i++){
            var temp_data = temp_dhcp_list[i].split("/");
            if(temp_data.length != 0){
                if(temp_data.length > 1){
                    document.write("<tr><td>" + temp_data[0] + "</td><td>" + temp_data[1] + "</td><td>" + temp_data[2] + "</td></tr>")
                }
            }
        }
    }

    function set_igmp_info(){
        var temp_igmp = get_by_id("igmp_list").value.split(",");

        for (var i = 0; i < temp_igmp.length; i++){
            if(temp_igmp.length > 1){
                document.write("<tr><td>" + temp_igmp[i] + "</td></tr>")
            }
        }
    }

    function show_status(){
        get_by_id("net_status").style.display = "none";
        if(get_by_id("wan_proto").value != ""){
            get_by_id("net_status").style.display = "";
            if(get_by_id("wan_proto").value == "dhcpc"){
				get_by_id("show_button").innerHTML = '<input type=\"button\" value=\"'+sd_Release+'\" name=\"release\" id=\"release\" onClick=\"dhcp_release()\">&nbsp;<input type=\"button\" value=\"'+sd_Renew+'\" name=\"renew\" id=\"renew\" onClick=\"dhcp_renew()\">';
            }else if(get_by_id("wan_proto").value == "pppoe"){
                ifname = "PPPoE";
                if(get_by_id("wan_pppoe_dynamic_00").value == "0"){
					get_by_id("show_button").innerHTML = '<input type=button id=\"connect\" name=\"connect\" value=\"'+_connect+'\" onClick=\"pppoe_connect()\">&nbsp;<input type=button id=\"disconnect\" name=\"disconnect\" value=\"'+sd_Disconnect+'\" onClick=\"pppoe_disconnect()\">';
                }else{
					get_by_id("show_button").innerHTML = '<input type=button id=\"connect\" name=\"connect\" value=\"'+_connect+'\" onClick=\"pppoe_connect()\">&nbsp;<input type=button id=\"disconnect\" name=\"disconnect\" value=\"'+sd_Disconnect+'\" onClick=\"pppoe_disconnect()\">';
                }
            }else if(get_by_id("wan_proto").value == "pptp"){
                if(get_by_id("wan_pptp_dynamic").value == "0"){
					get_by_id("show_button").innerHTML = '<input type=button id=\"pptpconnect\" name=\"pptpconnect\" value=\"'+_connect+'\" onClick=\"pptp_connect()\">&nbsp;<input type=button id=\"pptpdisconnect\" name=\"pptpdisconnect\" value=\"'+sd_Disconnect+'\" onClick=\"pptp_disconnect()\">';
                }else{
					get_by_id("show_button").innerHTML = '<input type=button id=\"pptpconnect\" name=\"pptpconnect\" value=\"'+_connect+'\" onClick=\"pptp_connect()\">&nbsp;<input type=button id=\"pptpdisconnect\" name=\"pptpdisconnect\" value=\"'+sd_Disconnect+'\" onClick=\"pptp_disconnect()\">';
                }
            }else if(get_by_id("wan_proto").value == "l2tp"){
                if(get_by_id("wan_l2tp_dynamic").value == "0"){
					get_by_id("show_button").innerHTML = '<input type=button id=\"l2tpconnect\" name=\"l2tpconnect\" value=\"'+_connect+'\" onClick=\"l2tp_connect()\">&nbsp;<input type=button id=\"l2tpdisconnect\" name=\"l2tpdisconnect\" value=\"'+sd_Disconnect+'\" onClick=\"l2tp_disconnect()\">';
                }else{
					get_by_id("show_button").innerHTML = '<input type=button id=\"l2tpconnect\" name=\"l2tpconnect\" value=\"'+_connect+'\" onClick=\"l2tp_connect()\">&nbsp;<input type=button id=\"l2tpdisconnect\" name=\"l2tpdisconnect\" value=\"'+sd_Disconnect+'\" onClick=\"l2tp_disconnect()\">';
                }
           }else if(get_by_id("wan_proto").value == "usb3g"){
				get_by_id("show_button").innerHTML = '<input type=button id=\"usb3gconnect\" name=\"usb3gconnect\" value=\"'+_connect+'\" onClick=\"usb3g_connect()\">&nbsp;<input type=button id=\"usb3gdisconnect\" name=\"usb3gdisconnect\" value=\"'+sd_Disconnect+'\" onClick=\"usb3g_disconnect()\">';
            }
        }
    }

    function dhcp_renew(){
        if(get_by_id("cable_status").innerHTML == "Disconnected"){
         return;
        }
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form2");
        }
    }

    function dhcp_release(){
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form3");
        }
    }

    function pppoe_connect(){
        if(get_by_id("cable_status").innerHTML == "Disconnected")
         return;

        get_by_id("connect").disabled = "true"
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form4");
        }
    }

    function pppoe_disconnect(){
        get_by_id("disconnect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form5");
        }
    }

    function usb3g_connect(){
        get_by_id("usb3gconnect").disabled = "true"
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            window.location.href ="back.asp";
        }else{
            send_submit("form12");
        }
    }

    function usb3g_disconnect(){
        get_by_id("usb3gdisconnect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            window.location.href ="back.asp";
        }else{
            send_submit("form13");
        }
    }

    function pptp_connect(){
        if(get_by_id("cable_status").innerHTML == "Disconnected")
         return;

        get_by_id("pptpconnect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form6");
        }
    }

    function pptp_disconnect(){
        get_by_id("pptpdisconnect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form7");
        }
    }

    function l2tp_connect(){
        if(get_by_id("cable_status").innerHTML == "Disconnected")
         return;

        get_by_id("l2tpconnect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form8");
        }
    }

    function l2tp_disconnect(){
        get_by_id("l2tpdisconnect").disabled = "true";
        var login_who="<% CmoGetStatus("get_current_user"); %>";
        if(login_who== "user"){
            //window.location.href ="back.asp";
        }else{
            send_submit("form9");
        }
    }

    var nNow;
    function get_time(){
        if (gTime){
            return;
        }
        var gTime = "<% CmoGetStatus("system_time"); %>";
        var time = gTime.split("/");
        gTime = month[time[1]-1] + " " + time[2] + ", " + time[0] + " " + time[3] + ":" + time[4] + ":" + time[5];
        nNow = new Date(gTime);
    }

    function STime(){
        nNow.setTime(nNow.getTime() + 1000);
        get_by_id("show_time").innerHTML = nNow.toLocaleString();
        setTimeout('STime()', 1000);
    }
    function padout(number)
        {
            return (number < 10) ? '0' + number : number;
        }


    var wTimesec = 0, wan_time = _NA;
    var temp, days = 0, hours = 0, mins = 0, secs = 0;
    function caculate_time(){

        var wTime = wTimesec;
        var days = Math.floor(wTime / 86400);
            wTime %= 86400;
            var hours = Math.floor(wTime / 3600);
            wTime %= 3600;
            var mins = Math.floor(wTime / 60);
            wTime %= 60;

            wan_time = days + " " +
                ((days <= 1)
                    ? "Day"
                    : "Days")
                + ", ";
            wan_time += hours + ":" + padout(mins) + ":" + padout(wTime);

    }

    function get_wan_time(){
        wTimesec = <% CmoGetStatus("wan_uptime"); %>;
        if(wTimesec == 0){
            wan_time = _NA;
        }else{
            caculate_time();
        }
    }

function WTime(){
	get_by_id("show_uptime").innerHTML = wan_time;
	if(wTimesec != 0){
    		wTimesec++;
   	 	caculate_time();
	}	
		setTimeout('WTime()', 1000);
}

function set_dslite(net_status, aftr_address, aftr_enable)
{
	get_by_id("connection_type").innerHTML = "DS-Lite";
	get_by_id("tr_aftr_address").style.display="";
	get_by_id("tr_dslite_dhcp").style.display="";
	get_by_id("show_button").innerHTML ="";
	get_by_id("net_status").style.display="none";
	
	for(var i = 0 ; i <= 6 ; i++) 
		get_by_id("dslite_unuse"+i).style.display="none";
	
	if(net_status == 1)
	{
		if(aftr_enable == "1")
		{
			get_by_id("aftr_address").innerHTML = aftr_address;
		}else{
			get_by_id("aftr_address").innerHTML = "<% CmoGetCfg("wan_dslite_aftr", "None"); %>";
		}
	}else{
		get_by_id("aftr_address").innerHTML = _na;
	}

	if(net_status == "1"){
		get_by_id("dslite_dhcp").innerHTML = _enabled;
	}else{
		get_by_id("dslite_dhcp").innerHTML = _disabled;
	}
}

function xmldoc(){
    var d;
    var dns1,dns2;
    var aftr_address = "", aftr_enable = "";
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
        //show wan status
        var fixed_ip ="";
        d = document.getElementById("connection_type");
        aftr_address = xmlhttp.responseXML.getElementsByTagName("aftr_address")[0].firstChild.nodeValue;
	aftr_enable = xmlhttp.responseXML.getElementsByTagName("aftr_enable")[0].firstChild.nodeValue;
	if(aftr_address !== "None" && aftr_address !== "NULL" && aftr_address !== "(null)" && "<% CmoGetCfg("ipv6_wan_proto","none"); %>" === "ipv6_autodetect")
	{
		get_by_id("wan_proto").value = "dslite";

	} else {
		get_by_id("wan_proto").value = "<% CmoGetCfg("wan_proto","none"); %>";
	}

	if (get_by_id("wan_proto").value !== "dslite")
	{
		for(var i = 0 ; i <= 6 ; i++) 
			get_by_id("dslite_unuse"+i).style.display="";
               		get_by_id("tr_aftr_address").style.display="none";
	                get_by_id("tr_dslite_dhcp").style.display="none";
        }else{
		for(var i = 0 ; i <= 6 ; i++) 
			get_by_id("dslite_unuse"+i).style.display="none";
			get_by_id("tr_aftr_address").style.display="";
	                get_by_id("tr_dslite_dhcp").style.display="";	        
                        get_by_id("aftr_address").innerHTML = "N/A";
               		get_by_id("dslite_dhcp").innerHTML = "N/A";
        }

        if (get_by_id("wan_proto").value == "static")
            d.innerHTML = "Static IP";
        else if (get_by_id("wan_proto").value == "dhcpc")
            d.innerHTML = "DHCP Client";
        else if (get_by_id("wan_proto").value == "pppoe")
            d.innerHTML = "PPPoE";
        else if (get_by_id("wan_proto").value == "pptp")
            d.innerHTML = "PPTP";
        else if (get_by_id("wan_proto").value == "l2tp")
            d.innerHTML = "L2TP";
        else if(get_by_id("wan_proto").value == "dslite")
	    d.innerHTML = "DS-Lite";    
        else if(get_by_id("wan_proto").value == "usb3g")
            d.innerHTML = "3G USB Adapter";
        else
            alert("connect type none");

        show_status();
        //show cable status ;show network status
        wan_port_status = xmlhttp.responseXML.getElementsByTagName("wan_cable_status")[0].firstChild.nodeValue;
        wan_port_check = xmlhttp.responseXML.getElementsByTagName("wan_network_status")[0].firstChild.nodeValue;
        if(wan_port_status == "connect"){
		cable_status.innerHTML = CONNECTED;
		if(wan_port_check == "1"){
                        network_status.innerHTML = _sdi_s4b;
			if ("<% CmoGetCfg("traffic_shaping","none"); %>" == "1") {
				streamengine_status.innerHTML = _enabled;
			} else {
				streamengine_status.innerHTML = _disabled;
			}

                        if(get_by_id("wan_proto").value == "dhcpc"){
                            get_by_id("renew").disabled = true;
                        }else if(get_by_id("wan_proto").value == "pppoe"){
                            get_by_id("connect").disabled = true;
                        }else if(get_by_id("wan_proto").value == "pptp"){
                            get_by_id("pptpconnect").disabled = true;
                        }else if(get_by_id("wan_proto").value == "l2tp"){
                            get_by_id("l2tpconnect").disabled = true;
                        }else if(get_by_id("wan_proto").value == "usb3g"){
                            get_by_id("usb3gconnect").disabled = true;
                        }
                }else{
                    network_status.innerHTML = _sdi_s2;
                    streamengine_status.innerHTML = "N/A";
                    if(get_by_id("wan_proto").value == "dhcpc"){
                        get_by_id("release").disabled = true;
                    }else if(get_by_id("wan_proto").value == "pppoe"){
                        get_by_id("disconnect").disabled = true;
                    }else if(get_by_id("wan_proto").value == "pptp"){
                        get_by_id("pptpdisconnect").disabled = true;
                    }else if(get_by_id("wan_proto").value == "l2tp"){
                        get_by_id("l2tpdisconnect").disabled = true;
                    }else if(get_by_id("wan_proto").value == "usb3g"){
                        get_by_id("usb3gdisconnect").disabled = true;
                    }
                }
        }else{
		cable_status.innerHTML = _sdi_s2;
		network_status.innerHTML = _sdi_s2;
		streamengine_status.innerHTML = "N/A";
		if (get_by_id("wan_proto").value == "dhcpc") {
			get_by_id("release").disabled = true;
		} else if (get_by_id("wan_proto").value == "pppoe") {
			get_by_id("disconnect").disabled = true;
		} else if (get_by_id("wan_proto").value == "pptp") {
			get_by_id("pptpdisconnect").disabled = true;
		} else if (get_by_id("wan_proto").value == "l2tp") {
			get_by_id("l2tpdisconnect").disabled = true;
		} else if (get_by_id("wan_proto").value == "usb3g") {
			get_by_id("usb3gdisconnect").disabled = true;
		}
        }

 		
     	if ("<% CmoGetCfg("ipv6_wan_proto","none"); %>" === "ipv6_autodetect" && get_by_id("wan_proto").value === "dslite") {
			try {
				aftr_address = xmlhttp.responseXML.getElementsByTagName("aftr_address")[0].firstChild.nodeValue;
				aftr_enable = xmlhttp.responseXML.getElementsByTagName("aftr_enable")[0].firstChild.nodeValue;
			} catch(e){
				aftr_address = "None"; aftr_enable = "0";
			}
			set_dslite(wan_port_check, aftr_address, aftr_enable);
		}

	if (get_by_id("wan_proto").value === "dslite")
	{
			set_dslite(wan_port_check, aftr_address, aftr_enable);     
	}

        d = get_by_id("wan_ip");
        d.innerHTML = xmlhttp.responseXML.getElementsByTagName("wan_ip")[0].firstChild.nodeValue;
        d = get_by_id("wan_netmask");
        d.innerHTML = xmlhttp.responseXML.getElementsByTagName("wan_netmask")[0].firstChild.nodeValue;
        d = get_by_id("wan_gateway");
        d.innerHTML = xmlhttp.responseXML.getElementsByTagName("wan_default_gateway")[0].firstChild.nodeValue;
        //show dns
        dns1 = get_by_id("wan_dns1");
        dns1_value =xmlhttp.responseXML.getElementsByTagName("wan_primary_dns")[0].firstChild.nodeValue;
        dns2 = get_by_id("wan_dns2");
        dns2_value =xmlhttp.responseXML.getElementsByTagName("wan_secondary_dns")[0].firstChild.nodeValue;

        dns1.innerHTML = dns1_value;
        dns2.innerHTML = dns2_value;

        if(dns1_value !="(null)" ){
            dns1.innerHTML = dns1_value;
        }else
          dns1.innerHTML = "0.0.0.0";

        if(dns2_value !="(null)" ){
            dns2.innerHTML = dns2_value;
        }else{
            dns2.innerHTML = "0.0.0.0";
        }
        //wlan status
        d = get_by_id("wlan_channel");
		/*2009.10.28 Tina Tsao added
		* Fixed when wireless schedule is due but wireless status show enable.
		*/
		var d1 = get_by_id("show_wlan0");
		var show_wlan0_string="";
		var wlan0_enable = "<% CmoGetCfg("wlan0_enable","none"); %>";
		var lan_mac = "<% CmoGetCfg("lan_mac","none"); %>";
		if(wlan0_enable == "0"){
			show_wlan0_string= _off;
		}else if(wlan0_enable == "1" && lan_mac == "00:00:00:00:00:00"){
			show_wlan0_string= _init_fail;										
		}else if(wlan0_enable == "1"){
			show_wlan0_string= _enabled;																					
		}
        channel = xmlhttp.responseXML.getElementsByTagName("wlan_channel")[0].firstChild.nodeValue;

        if(channel !="(null)" ) {
            d.innerHTML = channel;
            show_wlan0_string= _enabled;
			get_by_id("show_wlan0_dot11_mode").style.display = "";
			get_by_id("show_wlan0_11n_protection").style.display = "";
		    get_by_id("show_wlan_channel").style.display = "";
	        get_by_id("show_wish_engine").style.display = "";
	        get_by_id("show_wps_enable").style.display = "";
		    get_by_id("show_ssid_lst").style.display = "";
		    get_by_id("show_ssid_table").style.display = "";
        }
        else {
            show_wlan0_string= _off;
			get_by_id("show_wlan0_dot11_mode").style.display = "none";
			get_by_id("show_wlan0_11n_protection").style.display = "none";
		    get_by_id("show_wlan_channel").style.display = "none";
	        get_by_id("show_wish_engine").style.display = "none";
	        get_by_id("show_wps_enable").style.display = "none";
		    get_by_id("show_ssid_lst").style.display = "none";
		    get_by_id("show_ssid_table").style.display = "none";
        }
        d1.innerHTML = show_wlan0_string;

        //wlan1 status
        d = get_by_id("wlan1_channel");
		/*2009.10.28 Tina Tsao added
		* Fixed when wireless schedule is due but wireless status show enable.
		*/
		var d2 = get_by_id("show_wlan1");
		var show_wlan1_string="";
		var wlan1_enable = "<% CmoGetCfg("wlan1_enable","none"); %>";

		if(wlan1_enable == "0"){
			show_wlan1_string= _off;
		}else if(wlan1_enable == "1" && lan_mac == "00:00:00:00:00:00"){
			show_wlan1_string= _init_fail;										
		}else if(wlan1_enable == "1"){
			show_wlan1_string= _enabled;																					
		}

        channel = xmlhttp.responseXML.getElementsByTagName("wlan1_channel")[0].firstChild.nodeValue;
        if (channel !="(null)" ) {
            d.innerHTML = channel;
            show_wlan1_string= _enabled;
			get_by_id("show_wlan1_dot11_mode").style.display = "";
			get_by_id("show_wlan1_11n_protection").style.display = "";
		    get_by_id("show_wlan1_channel").style.display = "";
	        get_by_id("show_wish_engine1").style.display = "";
	        get_by_id("show_wps_enable1").style.display = "";
		    get_by_id("show_ssid_lst1").style.display = "";
		    get_by_id("show_ssid_table1").style.display = "";
        }
        else {
            show_wlan1_string= _off;
			get_by_id("show_wlan1_dot11_mode").style.display = "none";
			get_by_id("show_wlan1_11n_protection").style.display = "none";
		    get_by_id("show_wlan1_channel").style.display = "none";
	        get_by_id("show_wish_engine1").style.display = "none";
	        get_by_id("show_wps_enable1").style.display = "none";
		    get_by_id("show_ssid_lst1").style.display = "none";
		    get_by_id("show_ssid_table1").style.display = "none";
        }
        d2.innerHTML = show_wlan1_string;

        //show wan_up time
        wTimesec = xmlhttp.responseXML.getElementsByTagName("wan_uptime")[0].firstChild.nodeValue;
        if(wTimesec == 0){
            wan_time = _NA;
        }else{
            caculate_time();
        }

    }
    else if(get_by_id("connection_type").innerHTML == "")
    {
    	get_by_id("connection_type").innerHTML = "N/A";
	cable_status.innerHTML = "Disconnected";
	network_status.innerHTML = "Disconnected";
	streamengine_status.innerHTML = "N/A";
    	get_by_id("wan_ip").innerHTML = "0.0.0.0";
    	get_by_id("wan_netmask").innerHTML = "0.0.0.0";
    	get_by_id("wan_gateway").innerHTML = "0.0.0.0";
    	get_by_id("wan_dns1").innerHTML = "0.0.0.0";
    	get_by_id("wan_dns2").innerHTML = "0.0.0.0";
    }
}

    function get_File() {
        xmlhttp = createRequest();
        if(xmlhttp){
        var url;
            var lan_ip="<% CmoGetCfg("lan_ipaddr","none"); %>";
            var temp_cURL = document.URL.split("/");
            var mURL = temp_cURL[2];
			/* Date:   2009-05-11
			 * Name:   Yufa Huang
			 * Reason: add HTTPS function.
			 */
			var hURL = temp_cURL[0];
            if(mURL == lan_ip){
				if (hURL == "https:")
					url="https://"+lan_ip+"/device.xml=device_status";
				else
                    url="http://"+lan_ip+"/device.xml=device_status";
            }else{
				if (hURL == "https:")
					url="https://"+mURL+"/device.xml=device_status";
				else
                    url="http://"+mURL+"/device.xml=device_status";
            }

        xmlhttp.onreadystatechange = xmldoc;
        xmlhttp.open("GET", url, true);
        xmlhttp.send(null);
        }
        setTimeout("get_File()",3000);
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
<input type="hidden" id="dhcp_list" name="dhcp_list" value="<% CmoGetList("local_lan_ip"); %>">
<input type="hidden" id="igmp_list" name="igmp_list" value="<% CmoGetList("igmp_table"); %>">
<form id="form1" name="form1" method="post" action="st_device.cgi">
<input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp">

<input type="hidden" id="wan_current_ipaddr" name="wan_current_ipaddr" value="<% CmoGetStatus("wan_current_ipaddr_00"); %>">
<input type="hidden" id="wan_proto" name="wan_proto" value="<% CmoGetCfg("wan_proto","none"); %>">
<input type="hidden" id="wan_pppoe_dynamic_00" name="wan_pppoe_dynamic_00" value="<% CmoGetCfg("wan_pppoe_dynamic_00","none"); %>">
<input type="hidden" id="wan_pptp_dynamic" name="wan_pptp_dynamic" value="<% CmoGetCfg("wan_pptp_dynamic","none"); %>">
<input type="hidden" id="wan_l2tp_dynamic" name="wan_l2tp_dynamic" value="<% CmoGetCfg("wan_l2tp_dynamic","none"); %>">
<input type="hidden" id="dhcpd_enable" name="dhcpd_enable" value="<% CmoGetCfg("dhcpd_enable","none"); %>">

<input type="hidden" id="dhcpc_connection_status" name="dhcpc_connection_status" value="<% CmoGetStatus("dhcpc_connection_status"); %>">
<input type="hidden" id="pppoe_00_connection_status" name="pppoe_00_connection_status" value="<% CmoGetStatus("pppoe_00_connection_status"); %>">
<input type="hidden" id="pptp_connection_status" name="pptp_connection_status" value="<% CmoGetStatus("pptp_connection_status"); %>">
<input type="hidden" id="l2tp_connection_status" name="l2tp_connection_status" value="<% CmoGetStatus("l2tp_connection_status"); %>">
<!--input type="hidden" id="bigpond_connection_status" name="bigpond_connection_status" value="<% CmoGetStatus("bigpond_connection_status"); %>"-->

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
									<li><div id="sidenavoff"><script>show_words(_devinfo)</script></div></li>
									<li><div><a href="st_log.asp"><script>show_words(_logs)</script></a></div></li>
									<li><div><a href="st_stats.asp"><script>show_words(_stats)</script></a></div></li>
									<li><div><a href="internet_sessions.asp"><script>show_words(YM157)</script></a></div></li>
									<li><div><a href="st_routing.asp"><script>show_words(_routing)</script></a></div></li>									
									<li><div><a href="st_wireless.asp"><script>show_words(_wireless)</script></a></div></li>
                                    <li><div ><a href="st_ipv6.asp">IPv6</a></div></li>
									<li><div><a href="st_routing6.asp"><script>show_words(IPV6_routing)</script></a></div></li>
                                </ul>
                                <!-- === END SIDENAV === -->
                            </div>
                        </td>
                    </tr>
                </table>
            </td>
            <td valign="top" id="maincontent_container">
                <div id="maincontent">
                    <!-- === BEGIN MAINCONTENT === -->
                    <div id="box_header">
						<h1><script>show_words(sd_title_Dev_Info)</script></h1>
						<script>show_words(sd_intro)</script>
                    </div>
                    <br>
                    <div class="box">
						<h2><script>show_words(sd_General)</script></h2>
                        <table cellpadding="1" cellspacing="1" border="0" width="525">
                            <tr>
								<td class="duple"><script>show_words(_time)</script> :</td>
                                <td width="340">&nbsp;&nbsp;<span id="show_time"></span></td>
                            </tr>
                            <tr>
								<td class="duple"><script>show_words(sd_FWV)</script> :</td>
                                <td width="340">&nbsp;&nbsp;<strong><% CmoGetStatus("version"); %> , <% CmoGetStatus("version_date"); %></strong></td>
                            </tr>
                        </table>
                    </div>
                    <div class="box">
						<h2><script>show_words(_WAN)</script></h2>
                            <table cellpadding="1" cellspacing="1" border="0" width="525">
                                <tr>
                                  <td class="duple"><script>show_words(_contype)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="connection_type"></span>
                                  </td>
                                </tr>
                                <tr>
                                  <td class="duple"><script>show_words(_cablestate)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="cable_status"></span>
                                   </td>
                                </tr>
                                <tr>
                                  <td class="duple"><script>show_words(_networkstate)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="network_status"></span>
                                   </td>
                                </tr>
                                <tr id="dslite_unuse0">
                                  <td class="duple"><script>show_words(KR71)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="streamengine_status"></span>
                                   </td>
                                </tr>
                                <tr>
                              <td class="duple"><script>show_words(_conuptime)</script> :</td>
                                  <td width="340">&nbsp;&nbsp;<span id="show_uptime"></span></td>
                                </tr>
                                <tr id="net_status" style="display:none">
                                  <td class="duple">&nbsp;</td>
                                  <td width="340">&nbsp;&nbsp;<span id="show_button"></span></td>
                                </tr>
                                <tr>
                                  <td class="duple"><script>show_words(_macaddr)</script> :</td>
                                  <td width="340">&nbsp;
                                  <script>
                                       var wan_mac = "<% CmoGetCfg("wan_mac","none"); %>";
                                       wan_mac = wan_mac.toUpperCase();
                                       document.write(wan_mac);
                                  </script>
                                   </td>
                                </tr>
                                <tr id="dslite_unuse1">
                                  <td class="duple"><script>show_words(_ipaddr)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="wan_ip"></span>
                                  </td>
                                </tr>
                                <tr id="dslite_unuse2">
                                  <td class="duple"><script>show_words(_subnet)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="wan_netmask"></span>
                                  </td>
                                </tr>
                                <tr id="dslite_unuse3">
                                  <td class="duple"><script>show_words(_defgw)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="wan_gateway"></span>
                                  </td>
                                </tr>
                                <tr id="dslite_unuse4">
                                  <td class="duple"><script>show_words(_dns1)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="wan_dns1"></span>
                                  </td>
                                </tr>
                                <tr id="dslite_unuse5">
                                  <td class="duple"><script>show_words(_dns2)</script> :</td>
                                  <td width="340">&nbsp;
                                  <span id="wan_dns2"></span>
                                  </td>
                                </tr>
                               <tr id="dslite_unuse6">
								  <td class="duple"><script>show_words(_st_AdvDns);</script> :</td>
                                  <td width="340">&nbsp;
                <span id="opendns_enable"></span>
								</tr>
                 <script>
                    var opendns="<% CmoGetCfg("opendns_enable","none"); %>";
                    get_by_id("opendns_enable").innerHTML = (opendns=="1")? _enabled:_disabled;
                                </script>

								<tr id="tr_aftr_address" style="display:none">
									<td class="duple">AFTR Address :</td>
									<td width="340">&nbsp;&nbsp;<span id="aftr_address"></span></td>
								</tr>
								<tr id="tr_dslite_dhcp" style="display:none">
									<td class="duple">DS-Lite DHCPv6 option :</td>
									<td width="340">&nbsp;&nbsp;<span id="dslite_dhcp"></span></td>
								</tr>
                </td>
                                </tr>
                            </table>
                    </div>
                    <div class="box">
						<h2><script>show_words(_LAN)</script></h2>
                            <table cellpadding="1" cellspacing="1" border="0" width="525">
                                <tr>
                                  <td class="duple"><script>show_words(_macaddr)</script> :</td>
                                  <td width="340">&nbsp;
                                  <script>
                                       var lan_mac = "<% CmoGetCfg("lan_mac","none"); %>";
                                       lan_mac = lan_mac.toUpperCase();
                                       document.write(lan_mac);
                                  </script>
                                  </td>
                              </tr>
                                <tr>
                                  <td class="duple"><script>show_words(_ipaddr)</script> :</td>
                                  <td width="340">&nbsp;
                                  <% CmoGetCfg("lan_ipaddr","none"); %>
                                  </td>
                              </tr>
                                <tr>
                                  <td class="duple"><script>show_words(_subnet)</script> :</td>
                                  <td width="340">&nbsp;
                                  <% CmoGetCfg("lan_netmask","none"); %>
                                  </td>
                              </tr>
                                <tr>
                                  <td class="duple"><script>show_words(_dhcpsrv)</script> :</td>
                                  <td width="340">&nbsp;
                                  <script>
                                        if(get_by_id("dhcpd_enable").value == "1"){
                                            //document.write("Enabled");
                                            document.write(_enabled);
                                        }else{
                                            //document.write("Disabled");
                                            document.write(_disabled);
                                        }

                                  </script>
                                  <!-- insert name=dhcp_enable -->
                                  </td>
                              </tr>
                            </table>
                    </div>
                    <div class="box">
                        <h2><script>show_words(sd_WLAN)</script></h2>
                            <table cellpadding="1" cellspacing="1" border="0" width="525">
                    <tr>
                      <td class="duple"><script>show_words(wwl_band)</script> :</td>
                      <td>&nbsp; <script>show_words(GW_WLAN_RADIO_0_NAME)</script></td>
                    </tr>
            <tr>
              <td class="duple"><script>show_words(sd_WRadio)</script> :</td>
                                    <td width="340">&nbsp; <span id="show_wlan0"></span></td>
                                </tr>
            <tr id="show_wlan0_dot11_mode">
              <td class="duple"><script>show_words(bwl_Mode)</script> :</td>
              <td>&nbsp; <% CmoGetCfg("wlan0_dot11_mode","none"); %></td>
            </tr>
            <tr id="show_wlan0_11n_protection">
              <td class="duple"><script>show_words(bwl_CWM)</script> :</td>
              <td>&nbsp;
                <span id="wlan0_11n_prot"></span>&nbsp;MHz
              </td>
            </tr>
            <tr id="show_wlan_channel">
              <td class="duple"><script>show_words(_channel)</script> :</td>
              <td>&nbsp; <span id="wlan_channel"></span> </td>
                                </tr>
            <tr id="show_wish_engine">
              <td class="duple"><script>show_words(YM63)</script> :</td>
              <td>&nbsp;
                                    <script>
                                    var wish_engine_enabled = <% CmoGetCfg("wish_engine_enabled","none"); %>;
                                        if(wish_engine_enabled == "0"){
                                        //document.write("Disabled");
                                        document.write(YM165);
                                        }else{
                                        //document.write("Enabled");
                                        document.write(YM164);
                                        }
                                  </script>
                                    </td>
            </tr>
            <!--tr>
              <td class="duple">Security Mode :</td>
              <td width="340">&nbsp; <script>
                                        var security_w = "<% CmoGetCfg("wlan0_security","none"); %>";
                                        var show_security = security_w;
                                        if(security_w == "wpa2auto_eap"){
                                            show_security = "AUTO (WPA or WPA2) - EAP";
                                        }else if(security_w == "wpa2auto_psk"){
                                            show_security = "AUTO (WPA or WPA2) - PSK";
                                        }else if(security_w == "wep_open_64"){
                                            show_security = "wep_64";
                                        }else if(security_w == "wep_open_128"){
                                            show_security = "wep_128";
                                        }
                                        document.write(show_security);
                                      </script> </td>
            </tr-->
            <tr id="show_wps_enable">
              <td class="duple"><script>show_words(LY2)</script> :</td>
              <td>&nbsp;
               <script>
                                    var wps_enable = "<% CmoGetCfg("wps_enable","none"); %>";
                                    var wps_config="<% CmoGetCfg("wps_configured_mode","none"); %>";
                                        if(wps_enable == "0"){
                                        //document.write("Disabled");
                                        document.write(_disabled);
                                        }else{
                                                if(wps_config != 1) {
                                                        document.write(LW66);
                                                }else {
                                                        document.write(LW67);
                                                }
                                        //document.write("Enabled");
                                        }
                                         </script></td>
                                </tr>
            <!--tr id="show_guest">
              <td class="duple"><script>show_words(guest)</script> <script>show_words(LY2)</script> :</td>
              <td>&nbsp;</td>
            </tr-->
            <tr id="show_ssid_lst">
              <td><b><script>show_words(ssid_lst)</script> :</b></td>
              <td width="340">&nbsp;</td>
            </tr>
            <tr id="show_ssid_table">
              <td colspan="2" class="duple">
                <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1 style="word-break:break-all;">
                <tr>
                    <td width="185"><b><script>show_words(sd_NNSSID)</script></b></td>
                    <td width="50"><b><script>show_words(guest)</script></b></td>
                    <td width="130"><b><script>show_words(_macaddr)</script></b></td>
                    <td width="160"><b><script>show_words(bws_SM)</script></b></td>
                </tr>
                <td width="123"><% CmoGetCfg("wlan0_ssid","none"); %></td>
                <td width="47">&nbsp;<script>show_words(_no)</script></td>
                <td width="128">
                	<% CmoGetCfg("lan_mac","none"); %>
                </td>
                <td width="196">
                  <script>
                var security_w = "<% CmoGetCfg("wlan0_security","none"); %>";
                var show_security = security_w;
                if(security_w == "wpa2auto_eap"){
                  show_security = "AUTO (WPA or WPA2) - EAP";
                }else if(security_w == "wpa2auto_psk"){
                  show_security = "AUTO (WPA or WPA2) - PSK";
                }else if(security_w == "wep_open_64"){
                  show_security = "wep_64";
                }else if(security_w == "wep_open_128"){
                  show_security = "wep_128";
                  }
                  document.write(show_security);
                  </script></td>
                <script>
                var tmp_guest_enable = <% CmoGetCfg("wlan0_vap1_enable","none"); %>;
                if (tmp_guest_enable == "1") {
                  document.write("<tr><td width=123>");
                }
                </script>
                <div id="guest_zone_ssid" style="display:none"><% CmoGetCfg("wlan0_vap1_ssid","none"); %></div>
                <script>
                var tmp_guest_enable = <% CmoGetCfg("wlan0_vap1_enable","none"); %>;
                if (tmp_guest_enable == "1") {                	
                  document.write("</td><td width=47> " + _yes + "</td>");
                  document.write("<td width=128> <% CmoGetStatus("guestzone_mac", "0"); %></td>");
                  document.write("<td width=196>");
                  var security_w = "<% CmoGetCfg("wlan0_vap1_security","none"); %>";
                  var show_security = security_w;
                  if(security_w == "wpa2auto_eap"){
                    show_security = "AUTO (WPA or WPA2) - EAP";
                  }else if(security_w == "wpa2auto_psk"){
                  show_security = "AUTO (WPA or WPA2) - PSK";
                  }
                  document.write(show_security);
                }
                document.write("</td></tr>");
                </script>
                </table></td>
              </tr>
              </table>
                  </div>

                    <div class="box">
                        <h2><script>show_words(sd_WLAN)</script>2</h2>
                            <table cellpadding="1" cellspacing="1" border="0" width="525">
                    <tr>
                      <td class="duple"><script>show_words(wwl_band)</script> :</td>
                      <td>&nbsp; <script>show_words(GW_WLAN_RADIO_1_NAME)</script></td>
                    </tr>
            <tr>
              <td class="duple"><script>show_words(sd_WRadio)</script> :</td>
                                    <td width="340">&nbsp; <span id="show_wlan1"></span></td>
                                </tr>
            <tr id="show_wlan1_dot11_mode">
              <td class="duple"><script>show_words(bwl_Mode)</script> :</td>
              <td>&nbsp; <% CmoGetCfg("wlan1_dot11_mode","none"); %></td>
            </tr>
            <tr id="show_wlan1_11n_protection">
              <td class="duple"><script>show_words(bwl_CWM)</script> :</td>
              <td>&nbsp;
                <span id="wlan1_11n_prot"></span>&nbsp;MHz
              </td>
            </tr>
            <tr id="show_wlan1_channel">
              <td class="duple"><script>show_words(_channel)</script> :</td>
              <td>&nbsp; <span id="wlan1_channel"></span> </td>
                                </tr>
            <tr id="show_wish_engine1">
              <td class="duple"><script>show_words(YM63)</script> :</td>
              <td>&nbsp;
                                    <script>
                                    var wish_engine_enabled = <% CmoGetCfg("wish_engine_enabled","none"); %>;
                                        if(wish_engine_enabled == "0"){
                                        //document.write("Disabled");
                                        document.write(YM165);
                                        }else{
                                        //document.write("Enabled");
                                        document.write(YM164);
                                        }
                                  </script>
                                    </td>
            </tr>
            <!--tr>
              <td class="duple">Security Mode :</td>
              <td width="340">&nbsp; <script>
                                        var security_w = "<% CmoGetCfg("wlan0_security","none"); %>";
                                        var show_security = security_w;
                                        if(security_w == "wpa2auto_eap"){
                                            show_security = "AUTO (WPA or WPA2) - EAP";
                                        }else if(security_w == "wpa2auto_psk"){
                                            show_security = "AUTO (WPA or WPA2) - PSK";
                                        }else if(security_w == "wep_open_64"){
                                            show_security = "wep_64";
                                        }else if(security_w == "wep_open_128"){
                                            show_security = "wep_128";
                                        }
                                        document.write(show_security);
                                      </script> </td>
            </tr-->
            <tr id="show_wps_enable1">
              <td class="duple"><script>show_words(LY2)</script> :</td>
              <td>&nbsp;
               <script>
                                    var wps_enable = "<% CmoGetCfg("wps_enable","none"); %>";
                                    var wps_config="<% CmoGetCfg("wps_configured_mode","none"); %>";
                                        if(wps_enable == "0"){
                                        //document.write("Disabled");
                                        document.write(_disabled);
                                        }else{
                                                if(wps_config != 1) {
                                                        document.write(LW66);
                                                }else {
                                                        document.write(LW67);
                                                }
                                        //document.write("Enabled");
                                        }
                                         </script></td>
                                </tr>
            <!--tr id="show_guest">
              <td class="duple"><script>show_words(guest)</script> <script>show_words(LY2)</script> :</td>
              <td>&nbsp;</td>
            </tr-->
            <tr id="show_ssid_lst1">
              <td><b><script>show_words(ssid_lst)</script> :</b></td>
              <td width="340">&nbsp;</td>
            </tr>
            <tr id="show_ssid_table1">
              <td colspan="2" class="duple">
                <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1 style="word-break:break-all;">
                <tr>
                    <td width="185"><b><script>show_words(sd_NNSSID)</script></b></td>
                    <td width="50"><b><script>show_words(guest)</script></b></td>
                    <td width="130"><b><script>show_words(_macaddr)</script></b></td>
                    <td width="160"><b><script>show_words(bws_SM)</script></b></td>
                </tr>
                <td width="123"><% CmoGetCfg("wlan1_ssid","none"); %></td>
                <td width="47">&nbsp;<script>show_words(_no)</script></td>
                <td width="128">
                	<% CmoGetStatus("wlan1_mac_addr","none"); %>
                </td>
                <td width="196">
                  <script>
                var security_w = "<% CmoGetCfg("wlan1_security","none"); %>";
                var show_security = security_w;
                if(security_w == "wpa2auto_eap"){
                  show_security = "AUTO (WPA or WPA2) - EAP";
                }else if(security_w == "wpa2auto_psk"){
                  show_security = "AUTO (WPA or WPA2) - PSK";
                }else if(security_w == "wep_open_64"){
                  show_security = "wep_64";
                }else if(security_w == "wep_open_128"){
                  show_security = "wep_128";
                  }
                  document.write(show_security);
                  </script></td>
                <script>
                var tmp_guest_enable = <% CmoGetCfg("wlan1_vap1_enable","none"); %>;
                if (tmp_guest_enable == "1") {
                  document.write("<tr><td width=123>");
                }
                </script>
                <div id="guest_zone_ssid1" style="display:none"><% CmoGetCfg("wlan1_vap1_ssid","none"); %></div>
                <script>
                var tmp_guest_enable = <% CmoGetCfg("wlan1_vap1_enable","none"); %>;
                if (tmp_guest_enable == "1") {                	
                  document.write("</td><td width=47> " + _yes + "</td>");
                  document.write("<td width=128> <% CmoGetStatus("guestzone_mac", "1"); %></td>");
                  document.write("<td width=196>");
                  var security_w = "<% CmoGetCfg("wlan1_vap1_security","none"); %>";
                  var show_security = security_w;
                  if(security_w == "wpa2auto_eap"){
                    show_security = "AUTO (WPA or WPA2) - EAP";
                  }else if(security_w == "wpa2auto_psk"){
                  show_security = "AUTO (WPA or WPA2) - PSK";
                  }
                  document.write(show_security);
                }
                document.write("</td></tr>");
                </script>
                </table></td>
              </tr>
              </table>
                  </div>
 
                  <div class="box">
                      <h2><script>show_words(_LANComputers)</script></h2>
                          <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1>
                            <tr>
                                <td><script>show_words(_ipaddr)</script></td>
                                <td><script>show_words(YM187)</script></td>
                                <td><script>show_words(aa_AT_1)</script></td>
                            </tr>
                            <script>set_mac_info();</script>
                          </table>
                  </div>
                  <div class="box">
					  <h2><script>show_words(_bln_title_IGMPMemberships)</script></h2>
                          <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1>
                            <tr>
								<td><script>show_words(YM186)</script></td>
                            </tr>
                            <script>set_igmp_info();</script>
                          </table>
                  </div>

                  <div id="renew_result" name="renew_result" style="display:none">
                          <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#ffffff border=1>
                            <tr><td>
                                    <IFRAME id="iframe_result" name="iframe_result" align=middle border="0" frameborder="0" marginWidth=0 marginHeight=0 src="" width="100%" height=0 scrolling="no"></IFRAME>
                                </td></tr>
                          </table>
                  </div>
                    <!-- === END MAINCONTENT === -->
                </div>
            </td>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table cellpadding="2" cellspacing="0" border="0" bgcolor="#FFFFFF">
                    <tr>
                      <td id="help_text">
					  	<strong><script>show_words(_hints)</script>&hellip;</strong>
					  	<p><script>show_words(hhsd_intro)</script></p>
						<p><a href="support_status.asp#Device_Info"><script>show_words(_more)</script>&hellip;</a></p>
                      </td>
                    </tr>
                </table>
            </td></form>
        </tr>
    </table>
    <table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
        <tr>
            <td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
            <td width="10">&nbsp;</td><td>&nbsp;</td>
        </tr>
    </table>
<form id="form2" name="form2" method="post" action="dhcp_renew.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_device.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
<form id="form3" name="form3" method="post" action="dhcp_release.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_device.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"></form>
<form id="form4" name="form4" method="post" action="pppoe_00_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_device.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
<form id="form5" name="form5" method="post" action="pppoe_00_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_device.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"></form>
<form id="form6" name="form6" method="post" action="pptp_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="back.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
<form id="form7" name="form7" method="post" action="pptp_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_device.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"></form>
<form id="form8" name="form8" method="post" action="l2tp_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="back.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"><input type="hidden" id="html_response_message" name="html_response_message" value="WAN is connecting. "></form>
<form id="form9" name="form9" method="post" action="l2tp_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_device.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"></form>
<form id="form12" name="form12" method="post" action="usb3g_init_connect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="back.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"><input type="hidden" id="html_response_message" name="html_response_message" value=""><script>get_by_id("html_response_message").value = ddns_connecting;</script></form>
<form id="form13" name="form13" method="post" action="usb3g_disconnect.cgi"><input type="hidden" id="html_response_page" name="html_response_page" value="st_device.asp"><input type="hidden" id="html_response_return_page" name="html_response_return_page" value="st_device.asp"></form>
<br>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
<br>
</body>
</html>
