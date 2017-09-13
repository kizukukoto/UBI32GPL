<html>
<head>
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript"> 
var submit_button_flag = 0;
var radius_button_flag = 0;
var radius_button_flag_1 = 0;
var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

	function onPageLoad(){
		set_checked(get_by_id("wlan0_enable").value, get_by_id("w_enable"));
		set_checked(get_by_id("wlan1_enable").value, get_by_id("w_enable_1"));
		get_by_id("show_ssid").value = get_by_id("wlan0_ssid").value;
		get_by_id("show_ssid_1").value = get_by_id("wlan1_ssid").value;
		
		set_checked(get_by_id("wlan0_auto_channel_enable").value, get_by_id("auto_channel"));
		set_checked(get_by_id("wlan1_auto_channel_enable").value, get_by_id("auto_channel_1"));
		set_checked("<% CmoGetCfg("wlan0_ssid_broadcast","none"); %>", get_by_name("wlan0_ssid_broadcast"));
		set_checked("<% CmoGetCfg("wlan1_ssid_broadcast","none"); %>", get_by_name("wlan1_ssid_broadcast"));
		
		get_by_id("sel_wlan0_channel").disabled = true;
		get_by_id("sel_wlan1_channel").disabled = true;
		
		set_selectIndex("<% CmoGetCfg("wlan0_dot11_mode","none"); %>", get_by_id("dot11_mode"));
		set_selectIndex("<% CmoGetCfg("wlan1_dot11_mode","none"); %>", get_by_id("dot11_mode_1"));
		set_selectIndex("<% CmoGetCfg("wlan0_11n_protection","none"); %>", get_by_id("11n_protection"));
		set_selectIndex("<% CmoGetCfg("wlan1_11n_protection","none"); %>", get_by_id("11a_protection"));
		
	    var temp_sch_0 = get_by_id("wlan0_schedule").value;
	    var temp_sch_1 = get_by_id("wlan1_schedule").value;
		var temp_data_0 = temp_sch_0.split("/");	
		var temp_data_1 = temp_sch_1.split("/");
		
	  	//2009/4/17 Tina modify:Fixed schedule can't selected.
		//set_selectIndex(temp_data_0[0], get_by_id("wlan0_schedule_select"));
		//set_selectIndex(temp_data_1[0], get_by_id("wlan1_schedule_select"));
		
	  	if((temp_sch_0 == "Always") || (temp_sch_0 == "Never") || (temp_sch_0 == "")){
	   		set_selectIndex(temp_data_0[0], get_by_id("wlan0_schedule_select"));
	    }else{
	  	var temp_index_0 = get_schedule_index(temp_data_0[0]);
	   		get_by_id("wlan0_schedule_select").selectedIndex=temp_index_0+2;
	     }
  
		  if((temp_sch_1 == "Always") || (temp_sch_1 == "Never")|| (temp_sch_1 == "")){
		   	set_selectIndex(temp_data_1[0], get_by_id("wlan1_schedule_select"));
		  }else{
		     var temp_index_1 = get_schedule_index(temp_data_1[0]);
	   		 get_by_id("wlan1_schedule_select").selectedIndex=temp_index_1+2;
		  }
  
		
		var wlan0_psk_cipher_type= "<% CmoGetCfg("wlan0_psk_cipher_type","none"); %>";
		var wlan1_psk_cipher_type= "<% CmoGetCfg("wlan1_psk_cipher_type","none"); %>";
		var temp_r0 = get_by_id("wlan0_eap_radius_server_0").value;
		var temp_r00 = get_by_id("wlan1_eap_radius_server_0").value;
		var Dr0 = temp_r0.split("/");
		var Dr00 = temp_r00.split("/");
		
		if(Dr0.length > 1){
			get_by_id("radius_ip1").value=Dr0[0];
			get_by_id("radius_port1").value=Dr0[1];
			get_by_id("radius_pass1").value=Dr0[2];
		}
		if(Dr00.length > 1){
			get_by_id("radius_ip1_1").value=Dr00[0];
			get_by_id("radius_port1_1").value=Dr00[1];
			get_by_id("radius_pass1_1").value=Dr00[2];
		}
		
		var temp_r1 = get_by_id("wlan0_eap_radius_server_1").value;
		var temp_r11 = get_by_id("wlan1_eap_radius_server_1").value;
		var Dr1 = temp_r1.split("/");
		var Dr11 = temp_r11.split("/");
		if(Dr1.length > 1){
			get_by_id("radius_ip2").value=Dr1[0];
			get_by_id("radius_port2").value=Dr1[1];
			get_by_id("radius_pass2").value=Dr1[2];
		}
		if(Dr11.length > 1){
			get_by_id("radius_ip2_1").value=Dr11[0];
			get_by_id("radius_port2_1").value=Dr11[1];
			get_by_id("radius_pass2_1").value=Dr11[2];
		}
		
		var wlan0_eap_mac_auth = get_by_id("wlan0_eap_mac_auth").value;
		var wlan1_eap_mac_auth = get_by_id("wlan1_eap_mac_auth").value;
		if(wlan0_eap_mac_auth == 0){
			get_by_id("radius_auth_mac1").checked = false;
			get_by_id("radius_auth_mac2").checked = false;
		}else if(wlan0_eap_mac_auth == 1){
			get_by_id("radius_auth_mac1").checked = true;
			get_by_id("radius_auth_mac2").checked = false;
		}else if(wlan0_eap_mac_auth == 2){
			get_by_id("radius_auth_mac1").checked = false;
			get_by_id("radius_auth_mac2").checked = true;
		}else{
			get_by_id("radius_auth_mac1").checked = true;
			get_by_id("radius_auth_mac2").checked = true;
		}
		
		if(wlan1_eap_mac_auth == 0){
			get_by_id("radius_auth_mac1_1").checked = false;
			get_by_id("radius_auth_mac2_1").checked = false;
		}else if(wlan1_eap_mac_auth == 1){
			get_by_id("radius_auth_mac1_1").checked = true;
			get_by_id("radius_auth_mac2_1").checked = false;
		}else if(wlan1_eap_mac_auth == 2){
			get_by_id("radius_auth_mac1_1").checked = false;
			get_by_id("radius_auth_mac2_1").checked = true;
		}else{
			get_by_id("radius_auth_mac1_1").checked = true;
			get_by_id("radius_auth_mac2_1").checked = true;
		}
		
		set_selectIndex(get_by_id("wlan0_wep_default_key").value, get_by_id("wep_def_key"));
		set_selectIndex(get_by_id("wlan1_wep_default_key").value, get_by_id("wep_def_key_1"));

		var wlan0_security= "<% CmoGetCfg("wlan0_security","none"); %>";
		var wlan1_security= "<% CmoGetCfg("wlan1_security","none"); %>";
		var security = wlan0_security.split("_");
		var security1 = wlan1_security.split("_");
		
		//2.4G
		if(wlan0_security == "disable"){				//Disabled
			set_selectIndex(0, get_by_id("wep_type"));
		}else if(security[0] == "wep"){					//WEP
			get_by_id("show_wep").style.display = "";
			set_selectIndex(1, get_by_id("wep_type"));
			//check auth_type is open
			if (security[1] == "open"||security[1] =="auto"){
				security[1] = "auto";	
			set_selectIndex(security[1], get_by_id("auth_type"));
			}else{
			        set_selectIndex(security[1], get_by_id("auth_type"));
			}
			if(security[2] == "64"){
				set_selectIndex(5, get_by_id("wep_key_len"));
			}else{
				set_selectIndex(13, get_by_id("wep_key_len"));
			}
		}else{
			if(security[1] == "psk"){					//PSK
			    get_by_id("show_wpa_psk").style.display = "";
				set_selectIndex(2, get_by_id("wep_type"));
			}else if(security[1] == "eap"){				//EAP
			    get_by_id("show_wpa_eap").style.display = "";
				set_selectIndex(3, get_by_id("wep_type"));
			}
			//set wpa_mode
			if(security[0] == "wpa2auto"){
				get_by_id("show_wpa").style.display = "";	
				set_selectIndex(2, get_by_id("wpa_mode"));
			}else{
				get_by_id("wpa_mode").value = security[0];
			}
		}
		//5G
		if(wlan1_security == "disable"){				//Disabled
			set_selectIndex(0, get_by_id("wep_type_1"));
		}else if(security1[0] == "wep"){					//WEP
			get_by_id("show_wep_1").style.display = "";
			set_selectIndex(1, get_by_id("wep_type_1"));
                        //check auth_type is open
			if (security1[1] == "open"||security1[1] =="auto"){
				security[1] = "auto";	
				set_selectIndex(security1[1], get_by_id("auth_type_1"));
			}else{
			set_selectIndex(security1[1], get_by_id("auth_type_1"));
			}
			
			if(security1[2] == "64"){
				set_selectIndex(5, get_by_id("wep_key_len_1"));
			}else{
				set_selectIndex(13, get_by_id("wep_key_len_1"));
			}
		}else{
			if(security1[1] == "psk"){					//PSK
				get_by_id("show_wpa_psk_1").style.display = "";
				set_selectIndex(2, get_by_id("wep_type_1"));
			}else if(security1[1] == "eap"){				//EAP
				get_by_id("show_wpa_eap_1").style.display = "";
				set_selectIndex(3, get_by_id("wep_type_1"));
			}
			//set wpa_mode
			if(security1[0] == "wpa2auto"){
				get_by_id("show_wpa_1").style.display = "";	
				set_selectIndex(2, get_by_id("wpa_mode_1"));
			}else{
				get_by_id("wpa_mode_1").value = security1[0];
			}
		}

		change_wep_key_fun();
		change_wep_key_fun_1();
		set_selectIndex("<% CmoGetCfg("wlan0_psk_cipher_type","none"); %>",get_by_id("c_type"));
		set_selectIndex("<% CmoGetCfg("wlan1_psk_cipher_type","none"); %>",get_by_id("c_type_1"));

        
        if (get_by_id("wlan0_11n_protection").value == "40"){
			set_selectIndex("auto", get_by_id("11n_protection"));
		}
		else
			set_selectIndex(get_by_id("wlan0_11n_protection").value, get_by_id("11n_protection"));        

        if (get_by_id("wlan1_11n_protection").value == "40"){
			set_selectIndex("auto", get_by_id("11a_protection"));
		}
		else
			set_selectIndex(get_by_id("wlan1_11n_protection").value, get_by_id("11a_protection")); 
    
		var login_who= "<% CmoGetStatus("get_current_user"); %>";
		if(login_who== "user"){
			DisableEnableForm(document.form1,true);	
		}else{
			disable_wireless();
			disable_wireless_1();
			disable_channel();
			disable_channel_1();
			show_chan_width();
			show_chan_width_1();
		}
		    set_form_default_values("form1");
	}//onPageLoad() 

	function change_wep_key_fun(){
		var length = parseInt(get_by_id("wep_key_len").value) * 2;
		var key_length = get_by_id("wep_key_len").selectedIndex;
		var key_type = get_by_id("wlan0_wep_display").value;
		var key1 = get_by_id("wlan0_wep" + key_num_array[key_length] + "_key_1").value;
		var key2 = get_by_id("wlan0_wep" + key_num_array[key_length] + "_key_1").value;
		var key3 = get_by_id("wlan0_wep" + key_num_array[key_length] + "_key_1").value;
		var key4 = get_by_id("wlan0_wep" + key_num_array[key_length] + "_key_1").value;
		
        var key_value = "<% CmoGetStatus("wlan0_wep_key1"); %>"
        var key_tmp = key_value.split("/");
        var key64_ps, key128_ps;
        key64_ps = decode(key_tmp[0], (key_tmp[0].length)-1);
       get_by_id("wlan0_wep64_key_1").value = key64_ps.substring(0, 10);
        var keyx = key64_ps.substring(0, 10);
        key128_ps = decode(key_tmp[1], (key_tmp[1].length)-1);
        var keyy = key128_ps.substring(0, 26);
        get_by_id("wlan0_wep128_key_1").value = key128_ps.substring(0, 26);

        if (key_length == 0)
            key1 = get_by_id("wlan0_wep64_key_1").value;
        else
            key1 = get_by_id("wlan0_wep128_key_1").value
		
		get_by_id("show_resert1").innerHTML = "<input type=\"password\" id=\"key1\" name=\"key1\" maxlength=" + length + " size=\"31\" value=" + key1 + " >"
		get_by_id("show_resert2").innerHTML = "<input type=\"hidden\" id=\"key2\" name=\"key2\" maxlength=" + length + " size=\"31\" value=" + key2 + " >"
		get_by_id("show_resert3").innerHTML = "<input type=\"hidden\" id=\"key3\" name=\"key3\" maxlength=" + length + " size=\"31\" value=" + key3 + " >"
		get_by_id("show_resert4").innerHTML = "<input type=\"hidden\" id=\"key4\" name=\"key4\" maxlength=" + length + " size=\"31\" value=" + key4 + " >"		
	}
	function change_wep_key_fun_1(){
		var length_1 = parseInt(get_by_id("wep_key_len_1").value) * 2;
		var key_length_1 = get_by_id("wep_key_len_1").selectedIndex;
		var key_type_1 = get_by_id("wlan1_wep_display").value;
		var key5 = get_by_id("wlan1_wep" + key_num_array[key_length_1] + "_key_1").value;
		var key6 = get_by_id("wlan1_wep" + key_num_array[key_length_1] + "_key_1").value;
		var key7 = get_by_id("wlan1_wep" + key_num_array[key_length_1] + "_key_1").value;
		var key8 = get_by_id("wlan1_wep" + key_num_array[key_length_1] + "_key_1").value;

        var key_value = "<% CmoGetStatus("wlan1_wep_key1"); %>"
        var key_tmp = key_value.split("/");
        var key64_ps, key128_ps;
        key64_ps = decode(key_tmp[0], key_tmp[0].length)
        get_by_id("wlan1_wep64_key_1").value = key64_ps.substring(0, 10);
        key128_ps = decode(key_tmp[1], key_tmp[1].length);
        get_by_id("wlan1_wep128_key_1").value = key128_ps.substring(0, 26);

        if (key_length_1 == 0)
            key5 = get_by_id("wlan1_wep64_key_1").value;
        else
            key5 = get_by_id("wlan1_wep128_key_1").value
		
		get_by_id("show_resert5").innerHTML = "<input type=\"password\" id=\"key5\" name=\"key5\" maxlength=" + length_1 + " size=\"31\" value=" + key5 + " >"
		get_by_id("show_resert6").innerHTML = "<input type=\"hidden\" id=\"key6\" name=\"key6\" maxlength=" + length_1 + " size=\"31\" value=" + key6 + " >"
		get_by_id("show_resert7").innerHTML = "<input type=\"hidden\" id=\"key7\" name=\"key7\" maxlength=" + length_1 + " size=\"31\" value=" + key7 + " >"
		get_by_id("show_resert8").innerHTML = "<input type=\"hidden\" id=\"key8\" name=\"key8\" maxlength=" + length_1 + " size=\"31\" value=" + key8 + " >"		
	}

    function check_8021x(){
    	var ip1 = get_by_id("radius_ip1").value;
    	var ip2 = get_by_id("radius_ip2").value;
        
        var radius1_msg = replace_msg(all_ip_addr_msg,"Radius Server 1");
    	var radius2_msg = replace_msg(all_ip_addr_msg,"Radius Server 2");
        
        var temp_ip1 = new addr_obj(ip1.split("."), radius1_msg, false, false);
        var temp_ip2 = new addr_obj(ip2.split("."), radius2_msg, true, false);
        var temp_radius1 = new raidus_obj(temp_ip1, get_by_id("radius_port1").value, get_by_id("radius_pass1").value);
        var temp_radius2 = new raidus_obj(temp_ip2, get_by_id("radius_port2").value, get_by_id("radius_pass2").value);
        
        if (!check_radius(temp_radius1)){
    		return false;        	               
    	}else if (ip2 != "" && ip2 != "0.0.0.0"){
    		if (!check_radius(temp_radius2)){
    			return false;        	               
    		}
    	}	
    	return true;	    
    }
    function check_8021x_1(){
    	var ip1 = get_by_id("radius_ip1_1").value;
    	var ip2 = get_by_id("radius_ip2_1").value;
        
        var radius1_msg = replace_msg(all_ip_addr_msg,"Radius Server 1");
    	var radius2_msg = replace_msg(all_ip_addr_msg,"Radius Server 2");
        
        var temp_ip1 = new addr_obj(ip1.split("."), radius1_msg, false, false);
        var temp_ip2 = new addr_obj(ip2.split("."), radius2_msg, true, false);
        var temp_radius1 = new raidus_obj(temp_ip1, get_by_id("radius_port1_1").value, get_by_id("radius_pass1_1").value);
        var temp_radius2 = new raidus_obj(temp_ip2, get_by_id("radius_port2_1").value, get_by_id("radius_pass2_1").value);
         
        if (!check_radius(temp_radius1)){
    		return false;        	               
    	}else if (ip2 != "" && ip2 != "0.0.0.0"){
    		if (!check_radius(temp_radius2)){
    			return false;        	               
    		}
    	}	
    	return true;	    
    }
	    
    function check_psk(){
		var psk_value = get_by_id("wlan0_psk_pass_phrase").value;
		if (psk_value.length < 8){                   
			alert(YM116);
				return false;
		}else if (psk_value.length > 63){
			if(!isHex(psk_value)){
				alert(GW_WLAN_WPA_PSK_HEX_STRING_INVALID);
				return false;
			}
        }
        return true;         
    }
    function check_psk_1(){
		var psk_value = get_by_id("wlan1_psk_pass_phrase").value;
		if (psk_value.length < 8){                   
			alert(YM116);
				return false;
		}else if (psk_value.length > 63){
			if(!isHex(psk_value)){
				alert(GW_WLAN_WPA_PSK_HEX_STRING_INVALID);
				return false;
			}
        }
        return true;         
    }
   
	function show_wpa_wep(){			
		var wep_type = get_by_id("wep_type").value;	
		
		get_by_id("show_wep").style.display = "none";
		get_by_id("show_wpa").style.display = "none";	
	    get_by_id("show_wpa_psk").style.display = "none";
	    get_by_id("show_wpa_eap").style.display = "none";
			
		if (wep_type == 1){			//WEP
			get_by_id("show_wep").style.display = "";
		}else if(wep_type == 2){	//WPA-Personal
			check_wps_psk_eap();
			get_by_id("show_wpa").style.display = "";	
			get_by_id("show_wpa_psk").style.display = "";	
		}else if(wep_type == 3){	//WPA-Enterprise
			if(check_wps_psk_eap()){
			get_by_id("show_wpa").style.display = "";	
			get_by_id("show_wpa_eap").style.display = "";
		}
    }
    }
    function show_wpa_wep_1(){			
		var wep_type = get_by_id("wep_type_1").value;	
		
		get_by_id("show_wep_1").style.display = "none";
		get_by_id("show_wpa_1").style.display = "none";	
	    get_by_id("show_wpa_psk_1").style.display = "none";
	    get_by_id("show_wpa_eap_1").style.display = "none";
			
		if (wep_type == 1){			//WEP
			get_by_id("show_wep_1").style.display = "";
		}else if(wep_type == 2){	//WPA-Personal
			check_wps_psk_eap_1();
			get_by_id("show_wpa_1").style.display = "";	
			get_by_id("show_wpa_psk_1").style.display = "";	
		}else if(wep_type == 3){	//WPA-Enterprise
			if(check_wps_psk_eap_1()){
			get_by_id("show_wpa_1").style.display = "";	
			get_by_id("show_wpa_eap_1").style.display = "";
		}
    }
    }
       function show_chan_width(){
 		var mode = get_by_id("dot11_mode").selectedIndex;	
		
		switch(mode){
		case 0:
		case 1:
		case 3:
			get_by_id("show_channel_width").style.display = "none";
			get_by_id("11n_protection").value ="20";
			break;			
		default:
			get_by_id("show_channel_width").style.display = "";
			break;	
		}  	  
       }
       
       function show_chan_width_1(){
 		var mode = get_by_id("dot11_mode_1").selectedIndex;	
		
		switch(mode){
		case 1:
			get_by_id("show_channel_width_1").style.display = "none";
			get_by_id("11a_protection").value ="20";
			break;
		default:
			get_by_id("show_channel_width_1").style.display = "";
			break;	
		}  	  
       }

	function disable_channel(){		
		if(get_by_id("w_enable").checked)
		get_by_id("sel_wlan0_channel").disabled = get_by_id("auto_channel").checked;
	}
	function disable_channel_1(){		
		if(get_by_id("w_enable_1").checked)
		get_by_id("sel_wlan1_channel").disabled = get_by_id("auto_channel_1").checked;
	}

	function disable_wireless(){
		var is_display = "";
		get_by_id("auto_channel").disabled = !get_by_id("w_enable").checked;
		get_by_id("show_ssid").disabled = !get_by_id("w_enable").checked;
		get_by_id("dot11_mode").disabled = !get_by_id("w_enable").checked;
		get_by_id("sel_wlan0_channel").disabled = !get_by_id("w_enable").checked;
		get_by_id("11n_protection").disabled = !get_by_id("w_enable").checked;
		get_by_name("wlan0_ssid_broadcast")[0].disabled = !get_by_id("w_enable").checked;
		get_by_name("wlan0_ssid_broadcast")[1].disabled = !get_by_id("w_enable").checked;
		get_by_id("add_new_schedule").disabled = !get_by_id("w_enable").checked;
		get_by_id("wlan0_schedule_select").disabled = !get_by_id("w_enable").checked;
		disable_channel();
		if(!get_by_id("w_enable").checked){
			get_by_id("show_security").style.display = "none";
			get_by_id("show_wep").style.display = "none";;
			get_by_id("show_wpa").style.display = "none";;	
			get_by_id("show_wpa_psk").style.display = "none";;
			get_by_id("show_wpa_eap").style.display = "none";;		
		}else{
			get_by_id("show_security").style.display = "";
			show_wpa_wep();
		}
	}
	
	function disable_wireless_1(){
		var is_display = "";
		get_by_id("auto_channel_1").disabled = !get_by_id("w_enable_1").checked;
		get_by_id("show_ssid_1").disabled = !get_by_id("w_enable_1").checked;
		get_by_id("dot11_mode_1").disabled = !get_by_id("w_enable_1").checked;
		get_by_id("sel_wlan1_channel").disabled = !get_by_id("w_enable_1").checked;
		get_by_id("11a_protection").disabled = !get_by_id("w_enable_1").checked;
		get_by_name("wlan1_ssid_broadcast")[0].disabled = !get_by_id("w_enable_1").checked;
		get_by_name("wlan1_ssid_broadcast")[1].disabled = !get_by_id("w_enable_1").checked;
		get_by_id("add_new_schedule2").disabled = !get_by_id("w_enable_1").checked;
		get_by_id("wlan1_schedule_select").disabled = !get_by_id("w_enable_1").checked;
		disable_channel_1();
		if(!get_by_id("w_enable_1").checked){
			get_by_id("show_security_1").style.display = "none";
			get_by_id("show_wep_1").style.display = "none";;
			get_by_id("show_wpa_1").style.display = "none";;	
			get_by_id("show_wpa_psk_1").style.display = "none";;
			get_by_id("show_wpa_eap_1").style.display = "none";;		
		}else{
			get_by_id("show_security_1").style.display = "";
			show_wpa_wep_1();
		}
	}

	function show_radius(){
		get_by_id("radius2").style.display = "none";
		get_by_id("none_radius2").style.display = "none";
		get_by_id("show_radius2").style.display = "none";
		if(radius_button_flag){
			get_by_id("radius2").style.display = "";
			radius_button_flag = 0;
		}else{
			get_by_id("none_radius2").style.display = "";
			get_by_id("show_radius2").style.display = "";
			radius_button_flag = 1;
		}
	}

	function show_radius_1(){
		get_by_id("radius2_1").style.display = "none";
		get_by_id("none_radius2_1").style.display = "none";
		get_by_id("show_radius2_1").style.display = "none";
		if(radius_button_flag_1){
			get_by_id("radius2_1").style.display = "";
			radius_button_flag_1 = 0;
		}else{
			get_by_id("none_radius2_1").style.display = "";
			get_by_id("show_radius2_1").style.display = "";
			radius_button_flag_1 = 1;
		}
	}

	function send_key_value(key_length){
		var key_type = get_by_id("wlan0_wep_display").value;

		for(var i = 1; i < 2; i++){
			get_by_id("wlan0_wep" + key_length + "_key_" + i).value = get_by_id("key" + i).value;
		}
	}

	function send_key_value_1(key_length_1){
		var key_type_1 = get_by_id("wlan1_wep_display").value;

		for(var i = 1; i < 2; i++){
				get_by_id("wlan1_wep" + key_length_1 + "_key_" + i).value = get_by_id("key" + (i+4)).value;
		}
	}
	
	function send_cipher_value(){
		if(get_by_id("c_type").selectedIndex == 0){
			get_by_id("wlan0_psk_cipher_type").value = "tkip";
		}else if(get_by_id("c_type").selectedIndex == 1){
			get_by_id("wlan0_psk_cipher_type").value = "aes";
		}else{
			get_by_id("wlan0_psk_cipher_type").value = "both";
		}
	}

	function send_cipher_value_1(){
		if(get_by_id("c_type_1").selectedIndex == 0){
			get_by_id("wlan1_psk_cipher_type").value = "tkip";
		}else if(get_by_id("c_type_1").selectedIndex == 1){
			get_by_id("wlan1_psk_cipher_type").value = "aes";
		}else{
			get_by_id("wlan1_psk_cipher_type").value = "both";
		}
	}
        
	function send_request(){	
		if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
			return false;
		}
				
		//2.4G
		var wlan_ssid = get_by_id("show_ssid").value
		var wep_type_value = get_by_id("wep_type").value;
		var key_length =get_by_id("wep_key_len").selectedIndex;
		var wlan0_dot11_mode = get_by_id("dot11_mode").value;
		var c_type_value = get_by_id("c_type").value;

		var rekey_msg = replace_msg(check_num_msg, bws_GKUI, 30, 65535);
		var temp_rekey = new varible_obj(get_by_id("wlan0_gkey_rekey_time").value, rekey_msg, 30, 65535, false);
		
		//5G
		var wlan_ssid_1 = get_by_id("show_ssid_1").value
		var wep_type_value_1 = get_by_id("wep_type_1").value;
		var key_length_1 =get_by_id("wep_key_len_1").selectedIndex;		
		var rekey_msg_1 = replace_msg(check_num_msg, bws_GKUI, 30, 65535);
		var temp_rekey_1 = new varible_obj(get_by_id("wlan1_gkey_rekey_time").value, rekey_msg, 30, 65535, false);
		var wlan1_dot11_mode = get_by_id("dot11_mode_1").value;
		var c_type_1_value = get_by_id("c_type_1").value;
		
		if(!(check_ssid("show_ssid"))){
				return false;
		}
		if(!(check_ssid("show_ssid_1"))){
				return false;
		}

		if(!(ischeck_wps("auto"))){
				return false;
		}
		//if(!(ischeck_wps_1("auto"))){
		//		return false;
		//}

        //2.4G 
		if(wep_type_value == 1){		//WEP
				if (wlan0_dot11_mode == "11n"){
				alert(MSG044);
				return false;
			}else{
				if(!check_key()){
					return false;
				}
			}
		}else if(wep_type_value == 2){	//PSK
			if ((wlan0_dot11_mode == "11n") && (c_type_value != "aes") && (get_by_id("w_enable").checked)) {
				alert(MSG045);
				return false;
			}else{
				if (!check_varible(temp_rekey)){
					return false;
				}
				if(!check_psk()){
					return false;
				}
			}
		}else if(wep_type_value == 3){	//EAP
		    if ((wlan0_dot11_mode == "11n") && (c_type_value != "aes") && (get_by_id("w_enable").checked)) {
				alert(MSG045);
				return false;
			}
			if (!check_varible(temp_rekey)){
				return false;
			}
			if(!check_8021x()){
				return false;
			}
		}
		//5G
		if(wep_type_value_1 == 1){		//WEP
			if (wlan1_dot11_mode == "11n"){
				alert(MSG044);
				return false;
			}else{
				if(!check_key_1()){
					return false;
				}
			}
		}else if(wep_type_value_1 == 2){	//PSK
			if ((wlan1_dot11_mode == "11n") && (c_type_1_value != "aes") && (get_by_id("w_enable_1").checked)) {
				alert(MSG045);
				return false;
			}else{
				if (!check_varible(temp_rekey_1)){
					return false;
				}
				if(!check_psk_1()){
					return false;
				}
			}
		}else if(wep_type_value_1 == 3){	//EAP
			if ((wlan1_dot11_mode == "11n") && (c_type_1_value != "aes") && (get_by_id("w_enable_1").checked)) {
				alert(MSG045);
				return false;
			}
			if (!check_varible(temp_rekey_1)){
				return false;
			}
			
			if(!check_8021x_1()){
				return false;
			}
		}
		<!--save wireless network setting-2.4G-->
		get_by_id("wlan0_enable").value = get_checked_value(get_by_id("w_enable"));
		get_by_id("wlan0_auto_channel_enable").value = get_checked_value(get_by_id("auto_channel"));
		get_by_id("wlan0_channel").value = get_by_id("sel_wlan0_channel").value;
		get_by_id("wlan0_dot11_mode").value = get_by_id("dot11_mode").value;
		//get_by_id("wlan0_11n_protection").value = get_by_id("11n_protection").value;
		
		get_by_id("wlan0_wep_default_key").value = get_by_id("wep_def_key").value;
		var wpa_mode = get_by_id("wpa_mode").value;
				
		<!--save wireless network setting-5G-->
		get_by_id("wlan1_enable").value = get_checked_value(get_by_id("w_enable_1"));
		get_by_id("wlan1_auto_channel_enable").value = get_checked_value(get_by_id("auto_channel_1"));
		get_by_id("wlan1_channel").value = get_by_id("sel_wlan1_channel").value;
		get_by_id("wlan1_dot11_mode").value = get_by_id("dot11_mode_1").value;
		//get_by_id("wlan1_11n_protection").value = get_by_id("11a_protection").value;
		
		get_by_id("wlan1_wep_default_key").value = get_by_id("wep_def_key_1").value;
		var wpa_mode_1 = get_by_id("wpa_mode_1").value;
		
		<!--save security -2.4G-->
		if(wep_type_value == 1){			//WEP
			get_by_id("wlan0_security").value = "wep_"+ get_by_id("auth_type").value +"_"+ key_num_array[key_length];
			//save wep key
			send_key_value(key_num_array[key_length]);
		}else if(wep_type_value == 2){		//PSK
			if(wpa_mode == "auto"){
				get_by_id("wlan0_security").value = "wpa2auto_psk";
			}else{
				get_by_id("wlan0_security").value = wpa_mode + "_psk";
			}
			send_cipher_value();
		}else if(wep_type_value == 3){		//EAP
			if(wpa_mode == "auto"){
				get_by_id("wlan0_security").value = "wpa2auto_eap";
			}else{
				get_by_id("wlan0_security").value = wpa_mode + "_eap";
			}
			get_by_id("wps_enable").value = "0";//wps enable=0 if EAP mode
			send_cipher_value();
			//save radius server
			var r_ip_0 = get_by_id("radius_ip1").value;
			var r_port_0 = get_by_id("radius_port1").value;
			var r_pass_0 = get_by_id("radius_pass1").value;
			var dat0 =r_ip_0 +"/"+ r_port_0 +"/"+ r_pass_0;
			get_by_id("wlan0_eap_radius_server_0").value = dat0;
			
			if(radius_button_flag){
				var r_ip_1 = get_by_id("radius_ip2").value;
				var r_port_1 = get_by_id("radius_port2").value;
				var r_pass_1 = get_by_id("radius_pass2").value;
				var dat1 =r_ip_1 +"/"+ r_port_1 +"/"+ r_pass_1;
				get_by_id("wlan0_eap_radius_server_1").value = dat1;
			}

			if((get_by_id("radius_auth_mac1").checked == false) && (get_by_id("radius_auth_mac2").checked = false))
				get_by_id("wlan0_eap_mac_auth").value = 0;
			else if((get_by_id("radius_auth_mac1").checked == true) && (get_by_id("radius_auth_mac2").checked = false))
				get_by_id("wlan0_eap_mac_auth").value = 1;
			else if((get_by_id("radius_auth_mac1").checked == false) && (get_by_id("radius_auth_mac2").checked = true))
				get_by_id("wlan0_eap_mac_auth").value = 2;
			else
				get_by_id("wlan0_eap_mac_auth").value = 3;

		}else{								//DISABLED
			get_by_id("wlan0_security").value = "disable";
		}
		
		<!--save security -5G-->
		if(wep_type_value_1 == 1){			//WEP
			get_by_id("wlan1_security").value = "wep_"+ get_by_id("auth_type_1").value +"_"+ key_num_array[key_length_1];
			//save wep key
			send_key_value_1(key_num_array[key_length_1]);
		}else if(wep_type_value_1 == 2){		//PSK
			if(wpa_mode_1 == "auto"){
				get_by_id("wlan1_security").value = "wpa2auto_psk";
			}else{
				get_by_id("wlan1_security").value = wpa_mode_1 + "_psk";
			}
			send_cipher_value_1();
		}else if(wep_type_value_1 == 3){		//EAP
			if(wpa_mode_1 == "auto"){
				get_by_id("wlan1_security").value = "wpa2auto_eap";
			}else{
				get_by_id("wlan1_security").value = wpa_mode_1 + "_eap";
			}
			get_by_id("wps_enable").value = "0";//wps enable=0 if EAP mode
			send_cipher_value_1();
			//save radius server
			var r_ip_00 = get_by_id("radius_ip1_1").value;
			var r_port_00 = get_by_id("radius_port1_1").value;
			var r_pass_00 = get_by_id("radius_pass1_1").value;
			var dat00 =r_ip_00 +"/"+ r_port_00 +"/"+ r_pass_00;
			get_by_id("wlan1_eap_radius_server_0").value = dat00;
			
			if(radius_button_flag_1){
				var r_ip_11 = get_by_id("radius_ip2_1").value;
				var r_port_11 = get_by_id("radius_port2_1").value;
				var r_pass_11 = get_by_id("radius_pass2_1").value;
				var dat11 =r_ip_11 +"/"+ r_port_11 +"/"+ r_pass_11;
				get_by_id("wlan1_eap_radius_server_1").value = dat11;
			}

			if((get_by_id("radius_auth_mac1_1").checked == false) && (get_by_id("radius_auth_mac2_1").checked = false))
				get_by_id("wlan1_eap_mac_auth").value = 0;
			else if((get_by_id("radius_auth_mac1_1").checked == true) && (get_by_id("radius_auth_mac2_1").checked = false))
				get_by_id("wlan1_eap_mac_auth").value = 1;
			else if((get_by_id("radius_auth_mac1_1").checked == false) && (get_by_id("radius_auth_mac2_1").checked = true))
				get_by_id("wlan1_eap_mac_auth").value = 2;
			else
				get_by_id("wlan1_eap_mac_auth").value = 3;

		}else{								//DISABLED
			get_by_id("wlan1_security").value = "disable";
		}
/*		
		//save wps_configured_mode  -- >only check SSID / Security
        if (get_by_id("show_ssid").value != "dlink" || get_by_id("show_ssid_1").value != "dlink_media" || 
        	get_by_id("wep_type").value != "0" || get_by_id("wep_type_1").value != "0" ||
        	get_by_id("wlan0_dot11_mode").value != "11bgn" || get_by_id("wlan1_dot11_mode").value != "11na" ||
        	get_by_id("wlan0_auto_channel_enable").value != "1" || get_by_id("wlan1_auto_channel_enable").value != "1" ||
        	get_by_id("wlan0_11n_protection").value != "20" || get_by_id("wlan1_11n_protection").value != "20" ||
		    get_by_name("wlan0_ssid_broadcast") != "1" || get_by_name("wlan1_ssid_broadcast") != "1"){
		        	if(get_by_id("wps_configured_mode").value == 1)
		        		get_by_id("wps_lock").value = 1;
		            get_by_id("wps_configured_mode").value = 5;
                }
*/
			if(
				get_by_id("show_ssid").value != "<% CmoGetCfg("wlan0_ssid","none"); %>" ||
				get_by_id("wlan0_security").value != "<% CmoGetCfg("wlan0_security","none"); %>" ||
				get_by_id("wlan0_wep64_key_1").value != "<% CmoGetCfg("wlan0_wep64_key_1","none"); %>" ||
				get_by_id("wlan0_wep128_key_1").value != "<% CmoGetCfg("wlan0_wep128_key_1","none"); %>" ||
				get_by_id("wlan0_psk_cipher_type").value != "<% CmoGetCfg("wlan0_psk_cipher_type","none"); %>" ||
				get_by_id("wlan0_psk_pass_phrase").value != "<% CmoGetCfg("wlan0_psk_pass_phrase","none"); %>" ||
				get_by_id("wlan0_eap_radius_server_0").value != "<% CmoGetCfg("wlan0_eap_radius_server_0","none"); %>" ||
				get_by_id("wlan0_eap_radius_server_1").value != "<% CmoGetCfg("wlan0_eap_radius_server_1","none"); %>" ||
				get_by_id("wlan0_gkey_rekey_time").value != "<% CmoGetCfg("wlan0_gkey_rekey_time","none"); %>" ||
				get_by_id("wlan0_eap_mac_auth").value != "<% CmoGetCfg("wlan0_eap_mac_auth","none"); %>" ||
				get_by_id("wlan0_eap_reauth_period").value != "<% CmoGetCfg("wlan0_eap_reauth_period","none"); %>"
			){
	        	get_by_id("wps_configured_mode").value = "5";
	        	get_by_id("disable_wps_pin").value = "1";
	        }

			if(
				get_by_id("show_ssid_1").value != "<% CmoGetCfg("wlan1_ssid","none"); %>" ||
				get_by_id("wlan1_security").value != "<% CmoGetCfg("wlan1_security","none"); %>" ||
				get_by_id("wlan1_wep64_key_1").value != "<% CmoGetCfg("wlan1_wep64_key_1","none"); %>" ||
				get_by_id("wlan1_wep128_key_1").value != "<% CmoGetCfg("wlan1_wep128_key_1","none"); %>" ||
				get_by_id("wlan1_psk_cipher_type").value != "<% CmoGetCfg("wlan1_psk_cipher_type","none"); %>" ||
				get_by_id("wlan1_psk_pass_phrase").value != "<% CmoGetCfg("wlan1_psk_pass_phrase","none"); %>" ||
				get_by_id("wlan1_eap_radius_server_0").value != "<% CmoGetCfg("wlan1_eap_radius_server_0","none"); %>" ||
				get_by_id("wlan1_eap_radius_server_1").value != "<% CmoGetCfg("wlan1_eap_radius_server_1","none"); %>" ||
				get_by_id("wlan1_gkey_rekey_time").value != "<% CmoGetCfg("wlan1_gkey_rekey_time","none"); %>" ||
				get_by_id("wlan1_eap_mac_auth").value != "<% CmoGetCfg("wlan1_eap_mac_auth","none"); %>" ||
				get_by_id("wlan1_eap_reauth_period").value != "<% CmoGetCfg("wlan1_eap_reauth_period","none"); %>"
			){
	        	get_by_id("wps_configured_mode").value = "5";
	        	get_by_id("disable_wps_pin").value = "1";
	        }

		if (check_schedule() == -1) {
            alert("The schedule of Guest Zone must be within the schedule of main WLAN.");
            return false;
        }

		if (check_schedule_1() == -1) {
            alert("The schedule of Guest Zone must be within the schedule of main WLAN.");
            return false;
        }

        if (get_by_id("11n_protection").value == "20") {
        	get_by_id("wlan0_11n_protection").value = "20";
        }
        else if ("<% CmoGetCfg("wlan0_11n_protection","none"); %>" == "20") {
            get_by_id("wlan0_11n_protection").value = "auto";
        }

        if (get_by_id("11a_protection").value == "20") {
        	get_by_id("wlan1_11n_protection").value = "20";
        }
        else if ("<% CmoGetCfg("wlan1_11n_protection","none"); %>" == "20") {
            get_by_id("wlan1_11n_protection").value = "auto";
        }

		if(submit_button_flag == 0){
			submit_button_flag = 1;
			get_by_id("wlan0_ssid").value = get_by_id("show_ssid").value;
			get_by_id("wlan1_ssid").value = get_by_id("show_ssid_1").value;
			return true;
		}else{
			return false;
		}
		
	}

	
	<!--2.4G-->
	function set_channel(){
		var channel_list = "<% CmoGetStatus("wlan0_channel_list"); %>";
		var current_channel = "<% CmoGetCfg("wlan0_channel","none"); %>";
		var ch = channel_list.split(", ");
		var ch_text = new Array("2.412","2.417","2.422","2.427","2.432","2.437","2.442","2.447","2.452","2.457","2.462","2.467","2.472");
		var obj = get_by_id("sel_wlan0_channel");
		var count = 0;
		
		for (var i = 0; i < ch.length; i++){			
			var ooption = document.createElement("option");						
			ooption.text = ch_text[i] + " GHz - CH " + ch[i];
			ooption.value = ch[i];				
			obj.options[count++] = ooption;
								
			if (ch[i] == current_channel){
				ooption.selected = true;
			}
		}
	}
	<!--5G-->
	function set_channel_1(){
		var channel_list = "<% CmoGetStatus("wlan1_channel_list"); %>";
		var current_channel = "<% CmoGetCfg("wlan1_channel","none"); %>";
		var ch = channel_list.split(", ");
		var obj = get_by_id("sel_wlan1_channel");
		var count = 0;

		for (var i = 0; i < ch.length; i++){			
			var ooption = document.createElement("option");						
			ooption.text = ch[i];
			ooption.value = ch[i];	
			obj.options[count++] = ooption;
								
			if (ch[i] == current_channel){
				ooption.selected = true;
			}
		}
	}

	function check_wps_psk_eap(){
		if(get_by_id("wps_enable").value =="1"){
			if((get_by_id("wep_type").value == "1") && (get_by_id("wep_def_key").value != "1")){
				alert(TEXT024);//Can't choose WEP key 2, 3, 4 when WPS is enable
				return false;
			}
			/*if((get_by_id("wep_type").value == "2") && (get_by_id("c_type").value == "both")){
				alert(TEXT025);//Cannot choose WPA-Personal/TKIP and AES when WPS is enabled
				set_selectIndex("tkip", get_by_id("c_type"));
				return false;
			}*/
			if(get_by_id("wep_type").value == "3" &&  !confirm(WPS_WARR_WPAEAP)){
				return false;
				//alert(TEXT026);//Cannot choose WPA-Enterprise when WPS is enabled
				//set_selectIndex("0", get_by_id("wep_type"));
				//return false;
				//get_by_id("wps_enable").value = "0";
			}
		}
		return true;
	}
	function check_wps_psk_eap_1(){
		if(get_by_id("wps_enable").value =="1"){
			if((get_by_id("wep_type_1").value == "1") && (get_by_id("wep_def_key_1").value != "1")){
				alert(TEXT024);
                return false;
			}
			/*if((get_by_id("wep_type_1").value == "2") && (get_by_id("c_type_1").value == "both")){
				alert(TEXT025);
				set_selectIndex("tkip", get_by_id("c_type_1"));
				return false;
			}*/
			if(get_by_id("wep_type_1").value == "3" &&  !confirm(WPS_WARR_WPAEAP)){
				return false;				
				//alert(TEXT026);
				//set_selectIndex("0", get_by_id("wep_type_1"));
				//return false;
				//get_by_id("wps_enable").value = "0";
			}
			}
		return true;
	}


    /*
     * Reason: Check host and guest schedule time.
     * Modified: Yufa Huang
     * Date: 2010.10.05
     */
    function check_schedule()
    {
        var host_sched;
        var host_sched_name = get_by_id("wlan0_schedule").value;
        var host_start_time = 0;
        var host_start_hour =0;
        var host_start_min = 0;
        var host_end_time = 0;
        var host_end_hour = 0;
        var host_end_min = 0;
        var host_weekdays = 0;
        var guest_sched;
        var guest_sched_name = get_by_id("wlan0_vap1_schedule").value;
        var guest_start_time = 0;
        var guest_start_hour =0;
        var guest_start_min = 0;        
        var guest_end_time = 0;
        var guest_end_hour =0;
        var guest_end_min = 0;        
        var guest_weekdays = 0;
        var tmp_sched;

        /* host or guest is disabled */
        if ((get_by_id("wlan0_enable").value == "0") ||
            ("<% CmoGetCfg("wlan0_vap1_enable","none"); %>" == "0")) {
            return 0;
        }

        /* host is null (default value) */
        if (host_sched_name == "") {
            host_sched_name = "Always";
        }

        /* guest is null (default value) */
        if (guest_sched_name == "") {
            guest_sched_name = "Always";
        }

        /* host is always enabled or guest is always disabled */
        if ((host_sched_name == "Always") || (guest_sched_name == "Never")) {
            return 0;
        }

        /* host is always disabled, but guest is not always disabled */
        if ((host_sched_name == "Never") && (guest_sched_name != "Never")) {
            return -1;
        }

        /* host is not always enabled, but guest is always enabled */
        if ((host_sched_name != "Always") && (guest_sched_name == "Always")) {
            return -1;
        }

        host_sched  = get_by_id("wlan0_schedule").value.split("/");
        host_sched_name = host_sched[0];
        host_weekdays = parseInt(host_sched[1], 10);
        host_start_time = host_sched[2].split(":");
        host_start_hour = parseInt(host_start_time[0], 10);
        host_start_min = parseInt(host_start_time[1], 10);
        host_end_time = host_sched[3].split(":");
        host_end_hour = parseInt(host_end_time[0], 10);
        host_end_min = parseInt(host_end_time[1], 10);

        guest_sched = get_by_id("wlan0_vap1_schedule").value.split("/");
        guest_sched_name = guest_sched[0];
        guest_weekdays = parseInt(guest_sched[1], 10);
        guest_start_time = guest_sched[2].split(":");
        guest_start_hour = parseInt(guest_start_time[0], 10);
        guest_start_min = parseInt(guest_start_time[1], 10);
        guest_end_time = guest_sched[3].split(":");
        guest_end_hour = parseInt(guest_end_time[0], 10);
        guest_end_min = parseInt(guest_end_time[1], 10);

        /* check schedule start and end time */
        if ((host_start_time > guest_start_time) ||
            (host_end_time < guest_end_time)) {
            return -1;
        }

        /* check schdule days */
        for (var i=0; i < 7; i++) {
            if (((host_weekdays & 1) == 0) && ((guest_weekdays & 1) == 1)) {
                return -1;
            }

            host_weekdays  = host_weekdays >> 1;
            guest_weekdays = guest_weekdays >> 1;
        }

        return 0;
    }

    /*
     * Reason: Check host and guest schedule time.
     * Modified: Yufa Huang
     * Date: 2010.10.05
     */
    function check_schedule_1()
    {
        var host_sched;
        var host_sched_name = get_by_id("wlan1_schedule").value;
        var host_start_time = 0;
        var host_start_hour =0;
        var host_start_min = 0;
        var host_end_time = 0;
        var host_end_hour = 0;
        var host_end_min = 0;
        var host_weekdays = 0;
        var guest_sched;
        var guest_sched_name = get_by_id("wlan1_vap1_schedule").value;
        var guest_start_time = 0;
        var guest_start_hour =0;
        var guest_start_min = 0;        
        var guest_end_time = 0;
        var guest_end_hour =0;
        var guest_end_min = 0;        
        var guest_weekdays = 0;
        var tmp_sched;

        /* host or guest is disabled */
        if ((get_by_id("wlan1_enable").value == "0") ||
            ("<% CmoGetCfg("wlan1_vap1_enable","none"); %>" == "0")) {
            return 0;
        }

        /* host is null (default value) */
        if (host_sched_name == "") {
            host_sched_name = "Always";
        }

        /* guest is null (default value) */
        if (guest_sched_name == "") {
            guest_sched_name = "Always";
        }

        /* host is always enabled or guest is always disabled */
        if ((host_sched_name == "Always") || (guest_sched_name == "Never")) {
            return 0;
        }

        /* host is always disabled, but guest is not always disabled */
        if ((host_sched_name == "Never") && (guest_sched_name != "Never")) {
            return -1;
        }

        /* host is not always enabled, but guest is always enabled */
        if ((host_sched_name != "Always") && (guest_sched_name == "Always")) {
            return -1;
        }

        host_sched  = get_by_id("wlan1_schedule").value.split("/");
        host_sched_name = host_sched[0];
        host_weekdays = parseInt(host_sched[1], 10);
        host_start_time = host_sched[2].split(":");
        host_start_hour = parseInt(host_start_time[0], 10);
        host_start_min = parseInt(host_start_time[1], 10);
        host_end_time = host_sched[3].split(":");
        host_end_hour = parseInt(host_end_time[0], 10);
        host_end_min = parseInt(host_end_time[1], 10);

        guest_sched = get_by_id("wlan1_vap1_schedule").value.split("/");
        guest_sched_name = guest_sched[0];
        guest_weekdays = parseInt(guest_sched[1], 10);
        guest_start_time = guest_sched[2].split(":");
        guest_start_hour = parseInt(guest_start_time[0], 10);
        guest_start_min = parseInt(guest_start_time[1], 10);
        guest_end_time = guest_sched[3].split(":");
        guest_end_hour = parseInt(guest_end_time[0], 10);
        guest_end_min = parseInt(guest_end_time[1], 10);

        /* check schedule start and end time */
        if ((host_start_time > guest_start_time) ||
            (host_end_time < guest_end_time)) {
            return -1;
        }

        /* check schdule days */
        for (var i=0; i < 7; i++) {
            if (((host_weekdays & 1) == 0) && ((guest_weekdays & 1) == 1)) {
                return -1;
            }

            host_weekdays  = host_weekdays >> 1;
            guest_weekdays = guest_weekdays >> 1;
        }

        return 0;
    }
	
	function ischeck_wps(obj){
		var is_true = false;
		if(get_by_id("wps_enable").value =="1"){
			//if(!get_by_id("w_enable").checked){
				//alert("Please Enable Wireless first.");
				//is_true = true;
			//}else 
			if(!check_wps_psk_eap()){
				is_true = true;
			}else if(get_by_id("auth_type").value == "share"){
				if(get_by_id("w_enable").checked){
					alert(TEXT027);	//Can't choose shared key when WPS is enable
					is_true = true;
					if(obj == "auto"){
						set_selectIndex("auto", get_by_id("auth_type"));
					}
				}
			}
			if(!check_wps_psk_eap_1()){
				is_true = true;
			}else if(get_by_id("auth_type_1").value == "share"){
				if(get_by_id("w_enable_1").checked){
					alert(TEXT027);	//Can't choose shared key when WPS is enable
					is_true = true;
					if(obj == "auto"){
						set_selectIndex("open", get_by_id("auth_type_1"));
					}
				}
			}
		}
		if(is_true){
			if(obj == "wps"){
				get_by_id("wps_enable").value =="0";
			}
			return false;
		}
		return true;
		
	}
	
	 function do_add_new_schedule(){
            window.location.href = "tools_schedules.asp";
     }
     
    function get_wlan0_schedule(obj){
	  	var tmp_schedule = obj.options[obj.selectedIndex].value;
	  	var schedule_val;
  	 	var tmp_schedule_index = obj.selectedIndex;
	  	
	  	//2009/4/10 Tina modify:Fixed can't save space char.
/*	  	for (var i = 0; i < 32; i++){
		var temp_sch = get_by_id("schedule_rule_" + i).value;
		var temp_data = temp_sch.split("/");
		
		  
		  if(tmp_schedule == temp_data[0]){
			  schedule_val = get_by_id("schedule_rule_" + i).value;
	}
	   }	
*/	       
	     if(tmp_schedule == "Always"){
		  		schedule_val = "Always";
		 }else if(tmp_schedule == "Never"){
			  	schedule_val = "Never";
		 }else{
			  schedule_val = get_by_id("schedule_rule_" + (tmp_schedule_index-2)).value;
		 }
		 
		 get_by_id("wlan0_schedule").value = schedule_val;
	}
	
	function get_wlan1_schedule(obj){
	  	var tmp_schedule = obj.options[obj.selectedIndex].value;
	  	var schedule_val;
  	 	var tmp_schedule_index = obj.selectedIndex;
	  	
	  	//2009/4/10 Tina modify:Fixed can't save space char.
/*	  	for (var i = 0; i < 32; i++){
		var temp_sch = get_by_id("schedule_rule_" + i).value;
		var temp_data = temp_sch.split("/");
		
		  
		  if(tmp_schedule == temp_data[0]){
			  schedule_val = get_by_id("schedule_rule_" + i).value;
			}		  		
	   }	
*/	      
	     if(tmp_schedule == "Always"){
		  		schedule_val = "Always";
		 }else if(tmp_schedule == "Never"){
			  	schedule_val = "Never";
		 }else{
			  schedule_val = get_by_id("schedule_rule_" + (tmp_schedule_index-2)).value;
		 }
		get_by_id("wlan1_schedule").value = schedule_val;
	}
	
</script>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
<style type="text/css">
<!--
.style1 {font-size: 11px}
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
				<table border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td id="sidenav_container">
							<div id="sidenav">
								<ul>
																	<script>
																		show_side_bar(1);
																	</script>
								</ul>
							</div>
						</td>			
					</tr>
				</table>			
		  </td>
			<form id="form1" name="form1" method="post" action="apply.cgi">
				<input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
				<input type="hidden" id="html_response_message" name="html_response_message" value="">
				<script>get_by_id("html_response_message").value = sc_intro_sv;</script>
				<input type="hidden" id="html_response_return_page" name="html_response_return_page" value="wireless.asp">
				<input type="hidden" id="reboot_type" name="reboot_type" value="wireless">
				<input type="hidden" id="wlan0_ssid" name="wlan0_ssid" value="<% CmoGetCfg("wlan0_ssid","none"); %>">
				<input type="hidden" id="wlan1_ssid" name="wlan1_ssid" value="<% CmoGetCfg("wlan1_ssid","none"); %>">
				<input type="hidden" id="wps_pin" name="wps_pin" value="<% CmoGetCfg("wps_pin","none"); %>">
				<input type="hidden" id="wps_configured_mode" name="wps_configured_mode" value="<% CmoGetCfg("wps_configured_mode","none"); %>">
				<input type="hidden" id="wlan0_wep_display" name="wlan0_wep_display" value="<% CmoGetCfg("wlan0_wep_display","none"); %>">
				<input type="hidden" id="wlan1_wep_display" name="wlan1_wep_display" value="<% CmoGetCfg("wlan1_wep_display","none"); %>">
				<input type="hidden" id="wps_enable" name="wps_enable" value="<% CmoGetCfg("wps_enable","none"); %>">
				<input type="hidden" id="wlan0_schedule" name="wlan0_schedule" value="<% CmoGetCfg("wlan0_schedule","none"); %>">
				<input type="hidden" id="wlan1_schedule" name="wlan1_schedule" value="<% CmoGetCfg("wlan1_schedule","none"); %>">
				<input type="hidden" id="wlan0_vap1_schedule" name="wlan0_vap1_schedule" value="<% CmoGetCfg("wlan0_vap1_schedule","none"); %>">
				<input type="hidden" id="wlan1_vap1_schedule" name="wlan1_vap1_schedule" value="<% CmoGetCfg("wlan1_vap1_schedule","none"); %>">
				<input type="hidden" id="wps_lock" name="wps_lock" value="<% CmoGetCfg("wps_lock","none"); %>">
				<input type="hidden" id="disable_wps_pin" name="disable_wps_pin" value="<% CmoGetCfg("disable_wps_pin","none"); %>">
				
				<input type="hidden" id="schedule_rule_0" name="schedule_rule_0" value="<% CmoGetCfg("schedule_rule_00","none"); %>">
				<input type="hidden" id="schedule_rule_1" name="schedule_rule_1" value="<% CmoGetCfg("schedule_rule_01","none"); %>">
                <input type="hidden" id="schedule_rule_2" name="schedule_rule_2" value="<% CmoGetCfg("schedule_rule_02","none"); %>">
                <input type="hidden" id="schedule_rule_3" name="schedule_rule_3" value="<% CmoGetCfg("schedule_rule_03","none"); %>">
                <input type="hidden" id="schedule_rule_4" name="schedule_rule_4" value="<% CmoGetCfg("schedule_rule_04","none"); %>">
                <input type="hidden" id="schedule_rule_5" name="schedule_rule_5" value="<% CmoGetCfg("schedule_rule_05","none"); %>">
                <input type="hidden" id="schedule_rule_6" name="schedule_rule_6" value="<% CmoGetCfg("schedule_rule_06","none"); %>">
                <input type="hidden" id="schedule_rule_7" name="schedule_rule_7" value="<% CmoGetCfg("schedule_rule_07","none"); %>">
                <input type="hidden" id="schedule_rule_8" name="schedule_rule_8" value="<% CmoGetCfg("schedule_rule_08","none"); %>">
                <input type="hidden" id="schedule_rule_9" name="schedule_rule_9" value="<% CmoGetCfg("schedule_rule_09","none"); %>">
                <input type="hidden" id="schedule_rule_10" name="schedule_rule_10" value="<% CmoGetCfg("schedule_rule_10","none"); %>">
                <input type="hidden" id="schedule_rule_11" name="schedule_rule_11" value="<% CmoGetCfg("schedule_rule_11","none"); %>">
                <input type="hidden" id="schedule_rule_12" name="schedule_rule_12" value="<% CmoGetCfg("schedule_rule_12","none"); %>">
                <input type="hidden" id="schedule_rule_13" name="schedule_rule_13" value="<% CmoGetCfg("schedule_rule_13","none"); %>">
                <input type="hidden" id="schedule_rule_14" name="schedule_rule_14" value="<% CmoGetCfg("schedule_rule_14","none"); %>">
                <input type="hidden" id="schedule_rule_15" name="schedule_rule_15" value="<% CmoGetCfg("schedule_rule_15","none"); %>">
                <input type="hidden" id="schedule_rule_16" name="schedule_rule_16" value="<% CmoGetCfg("schedule_rule_16","none"); %>">
                <input type="hidden" id="schedule_rule_17" name="schedule_rule_17" value="<% CmoGetCfg("schedule_rule_17","none"); %>">
                <input type="hidden" id="schedule_rule_18" name="schedule_rule_18" value="<% CmoGetCfg("schedule_rule_18","none"); %>">
                <input type="hidden" id="schedule_rule_19" name="schedule_rule_19" value="<% CmoGetCfg("schedule_rule_19","none"); %>">
                <input type="hidden" id="schedule_rule_20" name="schedule_rule_20" value="<% CmoGetCfg("schedule_rule_20","none"); %>">
                <input type="hidden" id="schedule_rule_21" name="schedule_rule_21" value="<% CmoGetCfg("schedule_rule_21","none"); %>">
                <input type="hidden" id="schedule_rule_22" name="schedule_rule_22" value="<% CmoGetCfg("schedule_rule_22","none"); %>">
                <input type="hidden" id="schedule_rule_23" name="schedule_rule_23" value="<% CmoGetCfg("schedule_rule_23","none"); %>">
                <input type="hidden" id="schedule_rule_24" name="schedule_rule_24" value="<% CmoGetCfg("schedule_rule_24","none"); %>">
                <input type="hidden" id="schedule_rule_25" name="schedule_rule_25" value="<% CmoGetCfg("schedule_rule_25","none"); %>">
                <input type="hidden" id="schedule_rule_26" name="schedule_rule_26" value="<% CmoGetCfg("schedule_rule_26","none"); %>">
                <input type="hidden" id="schedule_rule_27" name="schedule_rule_27" value="<% CmoGetCfg("schedule_rule_27","none"); %>">
                <input type="hidden" id="schedule_rule_28" name="schedule_rule_28" value="<% CmoGetCfg("schedule_rule_28","none"); %>">
                <input type="hidden" id="schedule_rule_29" name="schedule_rule_29" value="<% CmoGetCfg("schedule_rule_29","none"); %>">
                <input type="hidden" id="schedule_rule_30" name="schedule_rule_30" value="<% CmoGetCfg("schedule_rule_30","none"); %>">
                <input type="hidden" id="schedule_rule_31" name="schedule_rule_31" value="<% CmoGetCfg("schedule_rule_31","none"); %>">
		                                
              
              <td valign="top" id="maincontent_container">
				  <div id="box_header"> 
						<h1><script>show_words(_wireless)</script> : </h1>
						<p><script>show_words(bwl_Intro_1)</script></p>
						<input name="button" id="button" type="submit" class=button_submit value="" onClick="return send_request()">
						<input name="button2" id="button2" type="button" class=button_submit value="" onclick="page_cancel('form1', 'wireless.asp');">
						<script>check_reboot();</script>
						<script>get_by_id("button").value = _savesettings;</script>
						<script>get_by_id("button2").value = _dontsavesettings;</script>
					</div>
					
					<div class="box"> 
						<h2><script>show_words(bwl_title_1)</script></h2>
							
          <table cellpadding="1" cellspacing="1" border="0" width="525">
            <tr> 
              <td class="duple"><script>show_words(wwl_band)</script> :</td>
              <td>&nbsp;&nbsp;<strong><script>show_words(GW_WLAN_RADIO_0_NAME)</script></strong></td>
            </tr>
            <tr> 
              <td class="duple"><script>show_words(bwl_EW)</script> :</td>
              <td width="340">&nbsp; <input id="w_enable" name="w_enable" type="checkbox" value="1" onClick="disable_wireless();" checked> 
                <input type="hidden" id="wlan0_enable" name="wlan0_enable" value="<% CmoGetCfg("wlan0_enable","none"); %>"> 
                <select id="wlan0_schedule_select" name="wlan0_schedule_select" onChange="get_wlan0_schedule(this);">
                 <option value="Always" selected><script>show_words(_always)</script></option>
				 <option value="Never"><script>show_words(_never)</script></option>
					<script>document.write(set_schedule_option());</script>
               </select>
                <input name="add_new_schedule" type="button" class="button_submit" id="add_new_schedule" onclick="do_add_new_schedule(true)" value=""> 
				<script>get_by_id("add_new_schedule").value = dlink_1_add_new;</script>
              </td>
            </tr>
            <tr> 
              <td class="duple"><script>show_words(bwl_NN)</script> :</td>
              <td width="340">&nbsp;&nbsp;&nbsp; <input name="show_ssid" type="text" id="show_ssid" size="20" maxlength="32" value="">
                <script>show_words(bwl_AS)</script> </td>
            </tr>
            <tr> 
              <td class="duple"><script>show_words(bwl_Mode)</script> :</td>
              <td width="340">&nbsp;&nbsp; 
			  <select id="dot11_mode" name="dot11_mode" onClick="show_chan_width();">
                <option value="11b"><script>show_words(bwl_Mode_1)</script></option>
				<option value="11g"><script>show_words(bwl_Mode_2)</script></option>
				<option value="11n"><script>show_words(bwl_Mode_n)</script></option>
				<option value="11bg"><script>show_words(bwl_Mode_3)</script></option>
				<option value="11gn"><script>show_words(bwl_Mode_10)</script></option>
				<option value="11bgn"><script>show_words(bwl_Mode_11)</script></option>
               </select> <input type="hidden" id="wlan0_dot11_mode" name="wlan0_dot11_mode" value="<% CmoGetCfg("wlan0_dot11_mode","none"); %>"> 
              </td>
            </tr>
            <tr> 
              <td class="duple"><script>show_words(ebwl_AChan)</script> :</td>
              <td width="340">&nbsp; <input type="checkbox" id="auto_channel" name="auto_channel" value="1" onClick="disable_channel();"> 
                <input type="hidden" id="wlan0_auto_channel_enable" name="wlan0_auto_channel_enable" value="<% CmoGetCfg("wlan0_auto_channel_enable","none"); %>"> 
              </td>
            </tr>
            <tr> 
              <td class="duple"><script>show_words(_wchannel)</script> :</td>
              <td width="340">&nbsp;&nbsp; <select name="sel_wlan0_channel" id="sel_wlan0_channel">
                  <script>
					set_channel();
				</script>
                </select> <input type="hidden" id="wlan0_channel" name="wlan0_channel" value="<% CmoGetCfg("wlan0_channel","none"); %>"> 
              </td>
            </tr>
            <tr id="show_channel_width"> 
              <td class="duple"><script>show_words(bwl_CWM)</script> :</td>
              <td width="340">&nbsp;&nbsp; <select id="11n_protection" name="11n_protection">
                  <option value="20"><script>show_words(bwl_ht20)</script></option>
                  <option value="auto"><script>show_words(bwl_ht2040)</script></option>
                </select> <input type="hidden" id="wlan0_11n_protection" name="wlan0_11n_protection" value="<% CmoGetCfg("wlan0_11n_protection","none"); %>"> 
              </td>
            </tr>
            <tr> 
              <td class="duple"><script>show_words(bwl_VS)</script> :</td>
              <td width="340">&nbsp; <input type="radio" name="wlan0_ssid_broadcast" value="1">
                <script>show_words(bwl_VS_0)</script> 
                <input type="radio" name="wlan0_ssid_broadcast" value="0">
                <script>show_words(bwl_VS_1)</script> </td>
            </tr>
          </table>
					</div>
							<input type="hidden" id="wlan0_security" name="wlan0_security" value="<% CmoGetCfg("wlan0_security","none"); %>">
					<div class="box" id="show_security"> 
						<h2><script>show_words(bws_WSMode)</script></h2>
						
          <script>show_words(bws_intro_WlsSec)</script>
          <br><br>
				            <table cellpadding="1" cellspacing="1" border="0" width="525">
                              <tr>
                                <td class="duple"><script>show_words(bws_SM)</script> :</td>
                                <td width="340">&nbsp;
                                    <select id="wep_type" name="wep_type" onChange="show_wpa_wep()">
                                     <option value="0" selected><script>show_words(_none)</script></option>
                                      <option value="1"><script>show_words(_WEP)</script></option>
                                      <option value="2"><script>show_words(_WPApersonal)</script></option>
                                      <option value="3"><script>show_words(_WPAenterprise)</script></option>
                                    </select>
                                </td>
                              </tr>
                            </table>
				</div>
					
        <div class="box" id="show_wep" style="display:none"> 
          <h2>
            <script>show_words(_WEP)</script>
          </h2>
						
          <p> 
            <script>show_words(bws_msg_WEP_1)</script>
          </p>
          <p> 
            <script>show_words(bws_msg_WEP_2)</script>
          </p>
          <p> 
            <script>show_words(bws_msg_WEP_3)</script>
          </p>
                  			
          <table cellpadding="1" cellspacing="1" border="0" width="525">
            <input type="hidden" id="wlan0_wep64_key_1" name="wlan0_wep64_key_1" >
            <input type="hidden" id="wlan0_wep128_key_1" name="wlan0_wep128_key_1" >
            <input type="hidden" id="wlan0_wep64_key_2" name="wlan0_wep64_key_2" value="<% CmoGetCfg("wlan0_wep64_key_2","hex"); %>">
            <input type="hidden" id="wlan0_wep128_key_2" name="wlan0_wep128_key_2" value="<% CmoGetCfg("wlan0_wep128_key_2","hex"); %>">
            <input type="hidden" id="wlan0_wep64_key_3" name="wlan0_wep64_key_3" value="<% CmoGetCfg("wlan0_wep64_key_3","hex"); %>">
            <input type="hidden" id="wlan0_wep128_key_3" name="wlan0_wep128_key_3" value="<% CmoGetCfg("wlan0_wep128_key_3","hex"); %>">
            <input type="hidden" id="wlan0_wep64_key_4" name="wlan0_wep64_key_4" value="<% CmoGetCfg("wlan0_wep64_key_4","hex"); %>">
            <input type="hidden" id="wlan0_wep128_key_4" name="wlan0_wep128_key_4" value="<% CmoGetCfg("wlan0_wep128_key_4","hex"); %>">

            <tr>
              <td class="duple"> <script>show_words(bws_WKL)</script> :</td>
              <td width="340">&nbsp; <select id="wep_key_len" name="wep_key_len" size=1 onChange="change_wep_key_fun();">
                  <option value="5"><script>show_words(bws_WKL_0)</script></option>
				  <option value="13"><script>show_words(bws_WKL_1)</script></option>
                </select>
                 <script>show_words(bws_length)</script> </td>
            </tr>
            <tr id=show_wlan0_wep style="display:none">
              <td class="duple"><script>show_words(bws_DFWK)</script>
                :</td>
              <td width="340">&nbsp; <select id="wep_def_key" name="wep_def_key" onChange="ischeck_wps('wep');">
                <option value="1"><script>show_words(wepkey1)</script></option>
				<!--option value="2"><script>show_words(wepkey2)</script></option>
				<option value="3"><script>show_words(wepkey3)</script></option>
				<option value="4"><script>show_words(wepkey4)</script></option-->
			</select>
                  <input type="hidden" id="wlan0_wep_default_key" name="wlan0_wep_default_key" value="<% CmoGetCfg("wlan0_wep_default_key","none"); %>"> 
              </td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(auth)</script>
                :</td>
              <td width="340">&nbsp; <select name="auth_type" id="auth_type" onChange="ischeck_wps('auto');">
				<option value="auto"><script>show_words(_both)</script></option>
				<option value="share"><script>show_words(bws_Auth_2)</script></option>
		      </select> </td>
            </tr>
            <tr>
              <td class="duple"><script>show_words(_wepkey1)</script> :</td>
              <td width="340">&nbsp; <span id="show_resert1"></span> </td>
            </tr>
            <span id="show_resert2"></span>
			<span id="show_resert3"></span>
			<span id="show_resert4"></span>
            <!--tr>
              <td class="duple"><script>show_words(_wepkey2)</script> :</td>
              <td width="340">&nbsp; <span id="show_resert2"></span> </td>
            </tr>
            <tr>
              <td class="duple"><script>show_words(_wepkey3)</script> :</td>
              <td width="340">&nbsp; <span id="show_resert3"></span> </td>
            </tr>
            <tr>
              <td class="duple"><script>show_words(_wepkey4)</script> :</td>
              <td width="340">&nbsp; <span id="show_resert4"></span> </td>
            </tr-->
          </table>
					</div>
					
        <div class="box" id="show_wpa"  style="display:none"> 
          <h2><script>show_words(_WPA)</script></h2>
          <p><script>show_words(bws_msg_WPA)</script></p>
          <p><script>show_words(bws_msg_WPA_2)</script></p>
			<input type="hidden" id="wlan0_psk_cipher_type" name="wlan0_psk_cipher_type" value="<% CmoGetCfg("wlan0_psk_cipher_type","none"); %>">
                  			
          <table cellpadding="1" cellspacing="1" border="0" width="525">
            <tr>
              <td class="duple"> <script>show_words(bws_WPAM)</script> :</td>
              <td width="340">&nbsp; <select id="wpa_mode" name="wpa_mode">
                <option value="auto"><script>show_words(bws_WPAM_2)</script></option>
				<option value="wpa2"><script>show_words(bws_WPAM_3)</script></option>
				<option value="wpa"><script>show_words(bws_WPAM_1)</script></option>
				</select></td>

            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_CT)</script> :</td>
              <td width="340">&nbsp; <select id="c_type" name="c_type" onChange="check_wps_psk_eap()">
				<option value="tkip"><script>show_words(bws_CT_1)</script></option>
				<option value="aes"><script>show_words(bws_CT_2)</script></option>
				<option value="both"><script>show_words(bws_CT_3)</script></option>
				</select> </td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_GKUI)</script> :</td>
              <td width="340">&nbsp; <input type="text" id="wlan0_gkey_rekey_time" name="wlan0_gkey_rekey_time" size="8" maxlength="5" value="<% CmoGetCfg("wlan0_gkey_rekey_time","none"); %>">
                <script>show_words(bws_secs)</script></td>
            </tr>
          </table>
					</div>
					<div class="box" id="show_wpa_psk" style="display:none"> 
						 <h2><script>show_words(_psk)</script></h2>
						
          <p class="box_msg"> 
            <script>show_words(KR18)</script>
            <script>show_words(KR19)</script>
          </p>
                  			<table cellpadding="1" cellspacing="1" border="0" width="525">
								<tr>
									<td class="duple"><script>show_words(_psk)</script> :</td>
									<td width="340">&nbsp;<input type="password" id="wlan0_psk_pass_phrase" name="wlan0_psk_pass_phrase" size="40" maxlength="64" value="<% CmoGetCfg("wlan0_psk_pass_phrase","none"); %>"></td>
					  			</tr>
							</table>
					</div>
					<div class="box" id="show_wpa_eap" style="display:none"> 
						<h2><script>show_words(bws_EAPx)</script></h2>
						
          <p class="box_msg">
            <script>show_words(bws_msg_EAP)</script>
            <script>show_words(bws_RMAA)</script>
          </p>
                  			
          <table cellpadding="1" cellspacing="1" border="0" width="525">
            <tr>
              <td class="duple"> <script>show_words(bwsAT_)</script> :</td>
              <input type="hidden" id="wlan0_eap_radius_server_0" name="wlan0_eap_radius_server_0" value="<% CmoGetCfg("wlan0_eap_radius_server_0","none"); %>">
              <input type="hidden" id="wlan0_eap_mac_auth" name="wlan0_eap_mac_auth" value="<% CmoGetCfg("wlan0_eap_mac_auth","none"); %>">
              <td width="340">&nbsp;
                <input id="wlan0_eap_reauth_period" name="wlan0_eap_reauth_period" size=10 value="<% CmoGetCfg("wlan0_eap_reauth_period","none"); %>">
                <script>show_words(_minutes)</script></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RIPA)</script> :</td>
              <td width="340">&nbsp;
                <input id="radius_ip1" name="radius_ip1" maxlength=15 size=15></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RSP)</script> :</td>
              <td width="340">&nbsp;
                <input type="text" id="radius_port1" name="radius_port1" size="8" maxlength="5" value="1812"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RSSs)</script> :</td>
              <td width="340">&nbsp;
                <input type="password" id="radius_pass1" name="radius_pass1" size="32" maxlength="64"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RMAA)</script> :</td>
              <td width="340">&nbsp;
                <input type="checkbox" id="radius_auth_mac1" name="radius_auth_mac1"></td>
            </tr>
            <tr id="radius2"> 
              <td colspan="2"><input type="button" id="advanced" name="advanced" value="" onClick="show_radius();">
                <script>get_by_id("advanced").value = _advanced+">>";</script> 
              </td>
            </tr>
            <tr id="none_radius2" style="display:none"> 
              <td colspan="2"><input type="button" id="advanced0" name="advanced0" value="" onClick="show_radius();">
			  <script>get_by_id("advanced0").value = "<<"+_advanced;</script></td>
            </tr>
          </table>
                  			
          <table id="show_radius2" cellpadding="1" cellspacing="1" border="0" width="525" style="display:none">
            <tr> 
              <input type="hidden" id="wlan0_eap_radius_server_1" name="wlan0_eap_radius_server_1" value="<% CmoGetCfg("wlan0_eap_radius_server_1","none"); %>">
              <td class="box_msg" colspan="2"><script>show_words(bws_ORAD)</script>
                :</td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RIPA)</script> :</td>
              <td width="340">&nbsp;
                <input id="radius_ip2" name="radius_ip2" maxlength=15 size=15></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RSP)</script> :</td>
              <td width="340">&nbsp;
                <input type="text" id="radius_port2" name="radius_port2" size="8" maxlength="5" value="1812"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RSSS)</script> :</td>
              <td width="340">&nbsp;
                <input type="password" id="radius_pass2" name="radius_pass2" size="32" maxlength="64"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RMAA)</script> :</td>
              <td width="340">&nbsp;
                <input type="checkbox" id="radius_auth_mac2" name="radius_auth_mac2"></td>
            </tr>
          </table>         
			</div>
			
			 <div class="box">
            <h2><script>show_words(bwl_title_1)</script></h2>
            <table cellpadding="1" cellspacing="1" border="0" width="525">
              <tr> 
                <td class="duple"><script>show_words(wwl_band)</script> :</td>
                <td><strong>&nbsp;&nbsp;<script>show_words(GW_WLAN_RADIO_1_NAME)</script></strong></td>
              </tr>
              <tr> 
                <td class="duple"><script>show_words(bwl_EW)</script> :</td>
                <td width="340">&nbsp; <input id="w_enable_1" name="w_enable_1" type="checkbox" value="1" onClick="disable_wireless_1();" checked> 
                  <input type="hidden" id="wlan1_enable" name="wlan1_enable" value="<% CmoGetCfg("wlan1_enable","none"); %>"> 
                <select id="wlan1_schedule_select" name="wlan1_schedule_select" onChange="get_wlan1_schedule(this);">
                  <option value="Always" selected><script>show_words(_always)</script></option>
                  <option value="Never"><script>show_words(_never)</script></option>
				   <script>document.write(set_schedule_option());</script>
                </select> 
				<input name="add_new_schedule2" type="button" class="button_submit" id="add_new_schedule2" onclick="do_add_new_schedule(true)" value=""> 
                <script>get_by_id("add_new_schedule2").value = dlink_1_add_new;</script>
				</td>
              </tr>
              <tr> 
                <td class="duple"><script>show_words(bwl_NN)</script> :</td>
                <td width="340">&nbsp;&nbsp;&nbsp; <input name="show_ssid_1" type="text" id="show_ssid_1" size="20" maxlength="32" value="">
                  <script>show_words(bwl_AS)</script> </td>
              </tr>
              <tr> 
                <td class="duple"><script>show_words(bwl_Mode)</script> :</td>
                <td width="340">&nbsp;&nbsp; <select id="dot11_mode_1" name="dot11_mode_1" onClick="show_chan_width_1();">
                    <option value="11n"><script>show_words(bwl_Mode_n)</script></option>
                    <option value="11a"><script>show_words(bwl_Mode_a)</script></option>
                    <option value="11na"><script>show_words(bwl_Mode_5)</script></option>
                  </select> <input type="hidden" id="wlan1_dot11_mode" name="wlan1_dot11_mode" value="<% CmoGetCfg("wlan1_dot11_mode","none"); %>"> 
                </td>
              </tr>
              <tr> 
                <td class="duple"><script>show_words(ebwl_AChan)</script> :</td>
                <td width="340">&nbsp; <input type="checkbox" id="auto_channel_1" name="auto_channel_1" value="1" onClick="disable_channel_1();"> 
                  <input type="hidden" id="wlan1_auto_channel_enable" name="wlan1_auto_channel_enable" value="<% CmoGetCfg("wlan1_auto_channel_enable","none"); %>"> 
                </td>
              </tr>
              <tr> 
                <td class="duple"><script>show_words(_wchannel)</script> :</td>
                <td width="340">&nbsp;&nbsp; <select name="sel_wlan1_channel" id="sel_wlan1_channel">
                    <script>
										set_channel_1();
									</script>
                  </select> <input type="hidden" id="wlan1_channel" name="wlan1_channel" value="<% CmoGetCfg("wlan1_channel","none"); %>"> 
                </td>
              </tr>
              <tr id="show_channel_width_1"> 
                <td class="duple"><script>show_words(bwl_CWM)</script> :</td>
                <td width="340">&nbsp;&nbsp; <select id="11a_protection" name="11a_protection">
                    <option value="20"><script>show_words(bwl_ht20)</script></option>
                    <option value="auto"><script>show_words(bwl_ht2040)</script></option>
                  </select> <input type="hidden" id="wlan1_11n_protection" name="wlan1_11n_protection" value="<% CmoGetCfg("wlan1_11n_protection","none"); %>"> 
                </td>
              </tr>
              <tr> 
                <td class="duple"><script>show_words(bwl_VS)</script> :</td>
                <td width="340">&nbsp; <input type="radio" name="wlan1_ssid_broadcast" value="1">
                  <script>show_words(bwl_VS_0)</script> 
                  <input type="radio" name="wlan1_ssid_broadcast" value="0">
                  <script>show_words(bwl_VS_1)</script> </td>
              </tr>
            </table>
          </div>
          <input type="hidden" id="wlan1_security" name="wlan1_security" value="<% CmoGetCfg("wlan1_security","none"); %>">
          <div class="box" id="show_security_1"> 
            <h2><script>show_words(bws_WSMode)</script></h2>
            
          <script>show_words(bws_intro_WlsSec)</script>
          <br>
            <br>
            <table cellpadding="1" cellspacing="1" border="0" width="525">
              <tr> 
                <td class="duple"><script>show_words(bws_SM)</script> :</td>
                <td width="340">&nbsp; <select id="wep_type_1" name="wep_type_1" onChange="show_wpa_wep_1()">
                    <option value="0" selected><script>show_words(_none)</script></option>
				  <option value="1"><script>show_words(_WEP)</script></option>
				  <option value="2"><script>show_words(_WPApersonal)</script></option>
				  <option value="3"><script>show_words(_WPAenterprise)</script></option>
                  </select> </td>
              </tr>
            </table>
          </div>
          
        <div class="box" id="show_wep_1" style="display:none"> 
          <h2>
            <script>show_words(_WEP)</script>
          </h2>
            
          <p> 
            <script>show_words(bws_msg_WEP_1)</script>
          </p>
          <p> 
            <script>show_words(bws_msg_WEP_2)</script>
          </p>
          <p> 
            <script>show_words(bws_msg_WEP_3)</script>
          </p>
                  			
          <table cellpadding="1" cellspacing="1" border="0" width="525">
            <input type="hidden" id="wlan1_wep64_key_1" name="wlan1_wep64_key_1" >
            <input type="hidden" id="wlan1_wep128_key_1" name="wlan1_wep128_key_1" >
            <input type="hidden" id="wlan1_wep64_key_2" name="wlan1_wep64_key_2" value="<% CmoGetCfg("wlan1_wep64_key_2","hex"); %>">
            <input type="hidden" id="wlan1_wep128_key_2" name="wlan1_wep128_key_2" value="<% CmoGetCfg("wlan1_wep128_key_2","hex"); %>">
            <input type="hidden" id="wlan1_wep64_key_3" name="wlan1_wep64_key_3" value="<% CmoGetCfg("wlan1_wep64_key_3","hex"); %>">
            <input type="hidden" id="wlan1_wep128_key_3" name="wlan1_wep128_key_3" value="<% CmoGetCfg("wlan1_wep128_key_3","hex"); %>">
            <input type="hidden" id="wlan1_wep64_key_4" name="wlan1_wep64_key_4" value="<% CmoGetCfg("wlan1_wep64_key_4","hex"); %>">
            <input type="hidden" id="wlan1_wep128_key_4" name="wlan1_wep128_key_4" value="<% CmoGetCfg("wlan1_wep128_key_4","hex"); %>">
            <tr>
              <td class="duple"> <script>show_words(bws_WKL)</script> :</td>
              <td width="340">&nbsp; <select id="wep_key_len_1" name="wep_key_len_1" size=1 onChange="change_wep_key_fun_1();">
                  <option value="5"><script>show_words(bws_WKL_0)</script></option>
				  <option value="13"><script>show_words(bws_WKL_1)</script></option>
                </select>
                 <script>show_words(bws_length)</script> </td>
            </tr>
            <tr id=show_wlan1_wep style="display:none">
              <td class="duple"> <script>show_words(bws_DFWK)</script>
                :</td>
              <td width="340">&nbsp; <select id="wep_def_key_1" name="wep_def_key_1" onChange="ischeck_wps('wep');">
                  <option value="1"><script>show_words(wepkey1)</script></option>
				  <!--option value="2"><script>show_words(wepkey2)</script></option>
				  <option value="3"><script>show_words(wepkey3)</script></option>
				  <option value="4"><script>show_words(wepkey4)</script></option-->
			</select> <input type="hidden" id="wlan1_wep_default_key" name="wlan1_wep_default_key" value="<% CmoGetCfg("wlan1_wep_default_key","none"); %>"> 
              </td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(auth)</script>
                :</td>
              <td width="340">&nbsp; <select name="auth_type_1" id="auth_type_1" onChange="ischeck_wps('auto');">
                  <option value="auto"><script>show_words(_both)</script></option>
				  <option value="share"><script>show_words(bws_Auth_2)</script></option>
		      </select> </td>
            </tr>
            <tr>
              <td class="duple"><script>show_words(_wepkey1)</script>
                :</td>
              <td width="340">&nbsp; <span id="show_resert5"></span> </td>
            </tr>
            <span id="show_resert6"></span>
			<span id="show_resert7"></span>
			<span id="show_resert8"></span>
            <!--tr>
              <td class="duple"><script>show_words(_wepkey2)</script>
                :</td>
              <td width="340">&nbsp; <span id="show_resert6"></span> </td>
            </tr>
            <tr>
              <td class="duple"><script>show_words(_wepkey3)</script>
                :</td>
              <td width="340">&nbsp; <span id="show_resert7"></span> </td>
            </tr>
            <tr>
              <td class="duple"><script>show_words(_wepkey4)</script>
                :</td>
              <td width="340">&nbsp; <span id="show_resert8"></span> </td>
            </tr-->
          </table>
          </div>
          
        <div class="box" id="show_wpa_1"  style="display:none"> 
          <h2>
            <script>show_words(_WPA)</script>
          </h2>
            
          <p> 
            <script>show_words(bws_msg_WPA)</script>
          </p>
          <p> 
            <script>show_words(bws_msg_WPA_2)</script>
          </p>
							<input type="hidden" id="wlan1_psk_cipher_type" name="wlan1_psk_cipher_type" value="<% CmoGetCfg("wlan1_psk_cipher_type","none"); %>">
            
          <table cellpadding="1" cellspacing="1" border="0" width="525">
            <tr>
              <td class="duple"> <script>show_words(bws_WPAM)</script>
                :</td>
              <td width="340">&nbsp; <select id="wpa_mode_1" name="wpa_mode_1">
				<option value="auto"><script>show_words(bws_WPAM_2)</script></option>
				<option value="wpa2"><script>show_words(bws_WPAM_3)</script></option>
				<option value="wpa"><script>show_words(bws_WPAM_1)</script></option>
				</select></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_CT)</script>
                :</td>
              <td width="340">&nbsp; <select id="c_type_1" name="c_type_1" onChange="check_wps_psk_eap_1()">
                  <option value="tkip"><script>show_words(bws_CT_1)</script></option>
				<option value="aes"><script>show_words(bws_CT_2)</script></option>
				<option value="both"><script>show_words(bws_CT_3)</script></option>
                </select> </td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_GKUI)</script>
                :</td>
              <td width="340">&nbsp; <input type="text" id="wlan1_gkey_rekey_time" name="wlan1_gkey_rekey_time" size="8" maxlength="5" value="<% CmoGetCfg("wlan1_gkey_rekey_time","none"); %>">
                <script>show_words(bws_secs)</script></td>
            </tr>
          </table>
          </div>
          
        <div class="box" id="show_wpa_psk_1" style="display:none"> 
          <h2>
            <script>show_words(_psk)</script>
          </h2>
						
          <p class="box_msg"> 
            <script>show_words(KR18)</script>
            <script>show_words(KR19)</script>
          </p>
            <table cellpadding="1" cellspacing="1" border="0" width="525">
              <tr> 
                <td class="duple"><script>show_words(_psk)</script> :</td>
                <td width="340">&nbsp;
                  <input type="password" id="wlan1_psk_pass_phrase" name="wlan1_psk_pass_phrase" size="40" maxlength="64" value="<% CmoGetCfg("wlan1_psk_pass_phrase","none"); %>"></td>
              </tr>
            </table>
          </div>
          <div class="box" id="show_wpa_eap_1" style="display:none"> 
            <h2><script>show_words(bws_EAPx)</script></h2>
            <p class="box_msg"><script>show_words(bws_msg_EAP)</script></p>
            
          <table cellpadding="1" cellspacing="1" border="0" width="525">
            <tr>
              <td class="duple"> <script>show_words(bwsAT_)</script>
                :</td>
              <input type="hidden" id="wlan1_eap_radius_server_0" name="wlan1_eap_radius_server_0" value="<% CmoGetCfg("wlan1_eap_radius_server_0","none"); %>">
              <input type="hidden" id="wlan1_eap_mac_auth" name="wlan1_eap_mac_auth" value="<% CmoGetCfg("wlan1_eap_mac_auth","none"); %>">
              <td width="340">&nbsp; <input id="wlan1_eap_reauth_period" name="wlan1_eap_reauth_period" size=10 value="<% CmoGetCfg("wlan1_eap_reauth_period","none"); %>">
                <script>show_words(_minutes)</script></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RIPA)</script>
                :</td>
              <td width="340">&nbsp; <input id="radius_ip1_1" name="radius_ip1_1" maxlength=15 size=15></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RSP)</script>
                :</td>
              <td width="340">&nbsp; <input type="text" id="radius_port1_1" name="radius_port1_1" size="8" maxlength="5" value="1812"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RSSs)</script>
                :</td>
              <td width="340">&nbsp; <input type="password" id="radius_pass1_1" name="radius_pass1_1" size="32" maxlength="64"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_RMAA)</script>
                :</td>
              <td width="340">&nbsp; <input type="checkbox" id="radius_auth_mac1_1" name="radius_auth_mac1_1"></td>
            </tr>
            <tr id="radius2_1"> 
              <td colspan="2"><input type="button" id="advanced_1" name="advanced_1" value="" onClick="show_radius_1();">
			  <script>get_by_id("advanced_1").value = _advanced+">>";</script></td>
            </tr>
            <tr id="none_radius2_1" style="display:none"> 
              <td colspan="2"><input type="button" id="advanced_2" name="advanced_2" value="" onClick="show_radius_1();">
			  <script>get_by_id("advanced_2").value = "<<"+_advanced;</script></td>
            </tr>
          </table>
            
          <table id="show_radius2_1" cellpadding="1" cellspacing="1" border="0" width="525" style="display:none">
            <tr> 
              <input type="hidden" id="wlan1_eap_radius_server_1" name="wlan1_eap_radius_server_1" value="<% CmoGetCfg("wlan1_eap_radius_server_1","none"); %>">
              <td class="box_msg" colspan="2"><script>show_words(bws_ORAD)</script>
                :</td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RIPA)</script>
                :</td>
              <td width="340">&nbsp; <input id="radius_ip2_1" name="radius_ip2_1" maxlength=15 size=15></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RSP)</script>
                :</td>
              <td width="340">&nbsp; <input type="text" id="radius_port2_1" name="radius_port2_1" size="8" maxlength="5" value="1812"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RSSS)</script>
                :</td>
              <td width="340">&nbsp; <input type="password" id="radius_pass2_1" name="radius_pass2_1" size="32" maxlength="64"></td>
            </tr>
            <tr>
              <td class="duple"> <script>show_words(bws_2RMAA)</script>
                :</td>
              <td width="340">&nbsp; <input type="checkbox" id="radius_auth_mac2_1" name="radius_auth_mac2_1"></td>
            </tr>
          </table>
          </div>
			</form>
            
    <td valign="top" width="150" id="sidehelp_container" align="left"> <table cellSpacing=0 cellPadding=2 bgColor=#ffffff border=0>
        <tbody>
          <tr> 
            <td id=help_text><strong> 
              <script>show_words(_hints)</script>
              &hellip;</strong> <p> 
                <script>show_words(YM123)</script>
              </p>
              <p> 
                <script>show_words(YM124)</script>
              </p>
              <p> 
                <script>show_words(YM125)</script>
              </p>
              <p> 
                <script>show_words(YM126)</script>
              </p>
              <p><a href="support_internet.asp#Wireless" onclick="return jump_if();">
                <script>show_words(_more)</script>
                &hellip;</a></p></td>
          </tr>
        </tbody>
      </table></td>
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
	reboot_form();
	onPageLoad();
</script>
</html>
