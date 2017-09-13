<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<script language="JavaScript">
    var submit_button_flag = 0;
    var rule_max_num = 10;
    var DataArray = new Array();
    var DataArray_detail = new Array(10);
    var rule_vs_num = 24;
    var reboot_needed = "<% CmoGetStatus("reboot_needed"); %>";

    function onPageLoad()
    {
        if ("<% CmoGetStatus("get_current_user"); %>" == "user") {
            DisableEnableForm(document.form1,true);
        }

        set_form_default_values("form1");
    }

    //name/action/used(vs/port/wan/remote)
    function Data(name, action, used, onList)
    {
        this.Name = name;
        this.Action = action;
        this.Used = used;
        this.Show_W = "";
        this.OnList = onList;
        var sActionW = "Allow"
        if (action =="deny") {
            sActionW = "Deny";
        }
        this.sAction = sActionW;
    }

    function Detail_Data(enable, ip_start, ip_end)
    {
        this.Enable = enable;
        this.Ip_start = ip_start;
        this.Ip_end = ip_end;
    }

    function set_Inbound()
    {
        var index = 0;
        for (var i = 0; i < rule_max_num; i++) {
            var temp_st;
            var temp_A;
            var temp_B;
            var k=i;
            if (parseInt(i,10) < 10) {
                k = "0" + i;
            }

            temp_st = (get_by_id("inbound_filter_name_" + k).value).split("/");
            if (temp_st.length > 1) {
                if (temp_st[0] != "") {
                    DataArray[DataArray.length++] = new Data(temp_st[0], temp_st[1], temp_st[2], index);
                    temp_A = get_by_id("inbound_filter_ip_"+ k +"_A").value.split(",");
                    DataArray_detail[index] = new Array();

                    var temp_ip_rang = 0;
                    for (j=0;j<temp_A.length;j++) {
                        var temp_A_e = temp_A[j].split("/");
                        var temp_A_ip = temp_A_e[1].split("-");
                        DataArray_detail[index][temp_ip_rang] = new Detail_Data(temp_A_e[0], temp_A_ip[0], temp_A_ip[1]);
                        temp_ip_rang++;

                        if (temp_A_e[0] == "1") {
                            var T_IP_R = temp_A_e[1];
                            if (temp_A_e[1] == "0.0.0.0-255.255.255.255") {
                                T_IP_R = "*";
                            }
                            if (DataArray[index].Show_W != "") {
                                DataArray[index].Show_W = DataArray[index].Show_W + ",";
                            }
                            DataArray[index].Show_W = DataArray[index].Show_W + T_IP_R;
                        }
                    }

                    temp_B = get_by_id("inbound_filter_ip_"+ k +"_B").value.split(",");
                    for (j=0;j<temp_B.length;j++) {
                        var temp_B_e = temp_B[j].split("/");
                        var temp_B_ip = temp_B_e[1].split("-");
                        DataArray_detail[index][temp_ip_rang] = new Detail_Data(temp_B_e[0], temp_B_ip[0], temp_B_ip[1]);
                        temp_ip_rang++;

                        if (temp_B_e[0] == "1") {
                            var T_IP_R = temp_B_e[1];
                            if (temp_B_e[1] == "0.0.0.0-255.255.255.255") {
                                T_IP_R = "*";
                            }
                            if (DataArray[index].Show_W != "") {
                                DataArray[index].Show_W = DataArray[index].Show_W + ",";
                            }
                            DataArray[index].Show_W = DataArray[index].Show_W + T_IP_R;
                        }
                    }
                    index++;
                }
            }
        }
    }

    function edit_row(obj)
    {
        get_by_id("edit").value = obj;
        get_by_id("ingress_filter_name").value = DataArray[obj].Name;
        set_selectIndex(DataArray[obj].Action, get_by_id("action_select"));

        for (j=0;j<DataArray_detail[obj].length;j++) {
            set_checked(DataArray_detail[obj][j].Enable, get_by_id("entry_enable_"+j));
            get_by_id("ip_start_"+j).value = DataArray_detail[obj][j].Ip_start;
            get_by_id("ip_end_"+j).value = DataArray_detail[obj][j].Ip_end;
        }
		get_by_id("button1").value = YM34;
    }

    function del_row(obj)
    {
        if (!is_data_used(obj)){
			delete_data(obj);
			return;
        }
        else{
        	alert(DataArray[obj].Name + " " + GW_SCHEDULES_IN_USE_INVALID_s2);
        	return;
        }
    }
    
    function is_data_used(obj)
    {
    	  var vs_inbound_filter = "", pf_inbound_filter = "";
    	  var http_inbound_filter = "<% CmoGetCfg("remote_http_management_inbound_filter"); %>";
    	  var wan_port_inbound_filter = "<% CmoGetCfg("wan_port_ping_response_inbound_filter"); %>";
    	  var flag_d=0;

    	  for (var i = 0; i < rule_vs_num; i++) {
            var temp_vs, temp_pf;

            if (i > 9){
                temp_vs = (get_by_id("vs_rule_" + i).value).split("/");
                temp_pf = (get_by_id("port_forward_both_" + i).value).split("/");
              }
            else{
                temp_vs = (get_by_id("vs_rule_0" + i).value).split("/");
                temp_pf = (get_by_id("port_forward_both_0" + i).value).split("/");
              }

            if ((temp_vs[0] == 1) && (temp_vs[7] != "Allow_All")) {
                vs_inbound_filter = temp_vs[7];
              }

            if ((temp_pf[0] == 1) && (temp_pf.length > 1)) {
                pf_inbound_filter = temp_pf[6];
              }

            if ((DataArray[obj].Name == http_inbound_filter) || (DataArray[obj].Name == wan_port_inbound_filter) || (DataArray[obj].Name == vs_inbound_filter) || (DataArray[obj].Name == pf_inbound_filter))
        	  	flag_d = 1;
        }

        if (flag_d == 1){
        	  DataArray[obj].Used = "1111";
        	  return true;
        }
        return false;
    }

    function delete_data(num)
    {
        for (i=num ; i<DataArray.length-1 ;i++) {
            DataArray[i].Name = DataArray[i+1].Name;
            DataArray[i].Action = DataArray[i+1].Action;
            DataArray[i].Used = DataArray[i+1].Used;
            DataArray[i].Show_W = DataArray[i+1].Show_W;
            DataArray[i].sAction = DataArray[i+1].sAction;
            DataArray[i].OnList = DataArray[i+1].OnList;

            for (j=0;j<DataArray_detail[i].length;j++) {
                DataArray_detail[i][j].Enable = DataArray_detail[i+1][j].Enable;
                DataArray_detail[i][j].Ip_start = DataArray_detail[i+1][j].Ip_start;
                DataArray_detail[i][j].Ip_end = DataArray_detail[i+1][j].Ip_end;
            }
        }

        --DataArray.length;
        --DataArray_detail[DataArray.length].length;
        save_date();
        send_submit("form1");
    }

    function send_request()
    {
        if (get_by_id("ingress_filter_name").value.length > 0) {
            if (!is_form_modified("form1") && !confirm(_ask_nochange)) {
                return false;
            }

            var index = parseInt(get_by_id("edit").value, 10);

            if (index > -1) {
                if (!confirm("Are you sure you want to update : " + get_by_id("ingress_filter_name").value)) {
                    return false;
                }
                else {
                    if (is_data_used(index)) {
							if(get_by_id("ingress_filter_name").value != DataArray[index].Name){
									alert(DataArray[index].Name + " " + GW_SCHEDULES_IN_USE_INVALID_s2);
                        return false;
                    }
                }
                }

                if ((index < 0) && (DataArray.length >= rule_max_num)) {
					alert(TEXT013+ rule_max_num);
                    return false;
                }
            }

            if (get_by_id("ingress_filter_name").value == "Allow All" || get_by_id("ingress_filter_name").value == "Deny All") {
				alert(TEXT014);
                return false;
            }

            for (var i = 0; i < DataArray.length; i++) {
                if (DataArray[i].Name == get_by_id("ingress_filter_name").value) {
                    if (DataArray[i].Name != "" && index == (DataArray[i].OnList)) {
                        continue;
                    }
                    else {
                        alert('Name "'+ get_by_id("ingress_filter_name").value +'" is already used!');
                        return false;
                    }
                }
            }

            var is_checked = false;
            for (i=0;i<8;i++) {
            	get_by_id("ip_start_" + i).value = trim_string(get_by_id("ip_start_" + i).value);
            	if (get_by_id("ip_start_" + i).value == "")
            		get_by_id("ip_start_" + i).value = "0.0.0.0";
            	
            	get_by_id("ip_end_" + i).value = trim_string(get_by_id("ip_end_" + i).value);
            	if (get_by_id("ip_end_" + i).value == "")
            		get_by_id("ip_end_" + i).value = "255.255.255.255";            	
            	
                var start_ip = get_by_id("ip_start_"+i).value;
                var end_ip = get_by_id("ip_end_"+i).value;

                var start_ip_addr_msg = replace_msg(all_ip_addr_msg,"Start IP address");
                var end_ip_addr_msg = replace_msg(all_ip_addr_msg,"End IP address");
                var start_obj = new addr_obj(start_ip.split("."), start_ip_addr_msg, false, false);
                var end_obj = new addr_obj(end_ip.split("."), end_ip_addr_msg, false, false);

                if (!check_ip_order(start_obj, end_obj)) {
					alert(TEXT039);
                    return false;
                }

                if (!is_ipv4_valid(start_ip)) {
					alert(KR2 +" : " + start_ip + ".");
                    get_by_id("ip_start_"+i).select();
                    get_by_id("ip_start_"+i).focus();
                    return false;
                }

                if (!is_ipv4_valid(end_ip)) {
					alert(KR2 +" : " + end_ip + ".");
                    get_by_id("ip_end_"+i).select();
                    get_by_id("ip_end_"+i).focus();
                    return false;
                }

                if (get_by_id("entry_enable_"+i).checked) {
                    for (j=i+1;j<8;j++) {
                        if (get_by_id("entry_enable_"+j).checked && (start_ip == get_by_id("ip_start_"+j).value && end_ip == get_by_id("ip_end_"+j).value)) {
							//alert("This IP Rang '"+ start_ip +"-"+ end_ip +"' is duplicated.");
							alert(ai_alert_7a+" '"+ start_ip +"-"+ end_ip +"' "+ai_alert_7b); 
							
                            return false;
                        }
                    }
                    is_checked = true;
                }
            }

            if (!is_checked) {
				alert(ai_alert_5a+" '"+ get_by_id("ingress_filter_name").value +"'.");	//ai_alert_5
                return false;
            }

            if (index > -1) {
                DataArray[index].Name = get_by_id("ingress_filter_name").value;
                DataArray[index].Action = get_by_id("action_select").value;
                for (j=0;j<DataArray_detail[index].length;j++) {
                    DataArray_detail[index][j].Enable = get_checked_value(get_by_id("entry_enable_"+j));
                    DataArray_detail[index][j].Ip_start = get_by_id("ip_start_"+j).value;
                    DataArray_detail[index][j].Ip_end = get_by_id("ip_end_"+j).value;
                }
            }
            else {
                var T_num = DataArray.length;
                DataArray[DataArray.length++] = new Data(get_by_id("ingress_filter_name").value, get_by_id("action_select").value, "0000", T_num);
                DataArray_detail[T_num] = new Array();

                for (i=0;i<8;i++) {
                    DataArray_detail[T_num][i] = new Detail_Data(get_checked_value(get_by_id("entry_enable_"+i)), get_by_id("ip_start_"+i).value, get_by_id("ip_end_"+i).value);
                }
            }

            return save_date();
        }
        else {
            alert(GW_FIREWALL_RULE_NAME_INVALID);
            return false;
        }
    }

    function save_date()
    {
        for (var i=0; i<rule_max_num; i++) {
            var k=i;
            if (parseInt(i,10) < 10) {
                k = "0" + i;
            }
            get_by_id("inbound_filter_name_" + k).value = "";
            get_by_id("inbound_filter_ip_"+ k +"_A").value = "";
            get_by_id("inbound_filter_ip_"+ k +"_B").value = "";

            if (i < DataArray.length) {
                var temp_st = DataArray[i].Name +"/"+ DataArray[i].Action +"/"+ DataArray[i].Used;
                var temp_A = "";
                var temp_B = "";

                for (j=0;j<5;j++) {
                    if (temp_A != "") {
                        temp_A += ",";
                    }
                    temp_A += DataArray_detail[i][j].Enable + "/" + DataArray_detail[i][j].Ip_start + "-" + DataArray_detail[i][j].Ip_end;
                }

                for (j=5;j<DataArray_detail[i].length;j++) {
                    if (temp_B != "") {
                        temp_B += ",";
                    }
                    temp_B += DataArray_detail[i][j].Enable + "/" + DataArray_detail[i][j].Ip_start + "-" + DataArray_detail[i][j].Ip_end;
                }

                get_by_id("inbound_filter_name_" + k).value = temp_st;
                get_by_id("inbound_filter_ip_"+ k +"_A").value = temp_A;
                get_by_id("inbound_filter_ip_"+ k +"_B").value = temp_B;
            }
        }

        if (submit_button_flag == 0) {
            submit_button_flag = 1;
            return true;
        }

        return false;
    }
</script>

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
			<td id="topnavoff"><a href="index.asp" onclick="return jump_if();"><script>show_words(_setup)</script></a></td>
			<td id="topnavon"><a href="adv_virtual.asp" onclick="return jump_if();"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp" onclick="return jump_if();"><script>show_words(_tools)</script></a></td>
			<td id="topnavoff"><a href="st_device.asp" onclick="return jump_if();"><script>show_words(_status)</script></a></td>
			<td id="topnavoff"><a href="support_men.asp" onclick="return jump_if();"><script>show_words(_support)</script></a></td>
        </tr>
    </table>
    <table border="1" cellpadding="2" cellspacing="0" width="838" height="100%" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
        <tr>
            <td id="sidenav_container" valign="top" width="125" align="right">
                <table cellSpacing=0 cellPadding=0 border=0>
                    <tr>
                      <td id=sidenav_container>
                        <div id=sidenav>
                            <ul>
							<LI><div><a href="adv_virtual.asp" onClick="return jump_if();"><script>show_words(_virtserv)</script></a></div></LI>
                            <LI><div><a href="adv_portforward.asp" onclick="return jump_if();"><script>show_words(_pf)</script></a></div></LI>
                            <LI><div><a href="adv_appl.asp" onclick="return jump_if();"><script>show_words(_specappsr)</script></a></div></LI>
                            <LI><div><a href="adv_qos.asp" onclick="return jump_if();"><script>show_words(YM48)</script></a></div></LI>
                            <LI><div><a href="adv_filters_mac.asp" onclick="return jump_if();"><script>show_words(_netfilt)</script></a></div></LI>
                            <LI><div><a href="adv_access_control.asp" onclick="return jump_if();"><script>show_words(_acccon)</script></a></div></LI>
							<LI><div><a href="adv_filters_url.asp" onclick="return jump_if();"><script>show_words(_websfilter)</script></a></div></LI>
							<LI><div id=sidenavoff><script>show_words(_inboundfilter)</script></div></LI>
							<LI><div><a href="adv_dmz.asp" onclick="return jump_if();"><script>show_words(_firewalls)</script></a></div></LI>
                            <LI><div><a href="adv_routing.asp" onclick="return jump_if();"><script>show_words(_routing)</script></a></div></LI>
							<LI><div><a href="adv_wlan_perform.asp" onclick="return jump_if();"><script>show_words(_adwwls)</script></a></div></LI>
                            <LI><div><a href="adv_wish.asp" onclick="return jump_if();">WISH</a></div></LI>
							<LI><div><a href="adv_wps_setting.asp" onclick="return jump_if();"><script>show_words(LY2)</script></a></div></LI>
                            <LI><div><a href="adv_network.asp" onclick="return jump_if();"><script>show_words(_advnetwork)</script></a></div></LI>
                            <LI><div><a href="guest_zone.asp" onclick="return jump_if();"><script>show_words(_guestzone)</script></a></div></LI>
                            <LI><div><a href="adv_ipv6_firewall.asp" onclick="return jump_if();"><script>show_words(Tag05762)</script></a></div></LI>
                            <LI><div><a href="adv_ipv6_routing.asp" onclick="return jump_if();"><script>show_words(IPV6_routing)</script></a></div></LI>
                            </ul>
                        </div>
		                  </td>
                    </tr>
                </table></td>

                <input type="hidden" id="vs_rule_00" name="vs_rule_00" value="<% CmoGetCfg("vs_rule_00","none"); %>">
                <input type="hidden" id="vs_rule_01" name="vs_rule_01" value="<% CmoGetCfg("vs_rule_01","none"); %>">
                <input type="hidden" id="vs_rule_02" name="vs_rule_02" value="<% CmoGetCfg("vs_rule_02","none"); %>">
                <input type="hidden" id="vs_rule_03" name="vs_rule_03" value="<% CmoGetCfg("vs_rule_03","none"); %>">
                <input type="hidden" id="vs_rule_04" name="vs_rule_04" value="<% CmoGetCfg("vs_rule_04","none"); %>">
                <input type="hidden" id="vs_rule_05" name="vs_rule_05" value="<% CmoGetCfg("vs_rule_05","none"); %>">
                <input type="hidden" id="vs_rule_06" name="vs_rule_06" value="<% CmoGetCfg("vs_rule_06","none"); %>">
                <input type="hidden" id="vs_rule_07" name="vs_rule_07" value="<% CmoGetCfg("vs_rule_07","none"); %>">
                <input type="hidden" id="vs_rule_08" name="vs_rule_08" value="<% CmoGetCfg("vs_rule_08","none"); %>">
                <input type="hidden" id="vs_rule_09" name="vs_rule_09" value="<% CmoGetCfg("vs_rule_09","none"); %>">
                <input type="hidden" id="vs_rule_10" name="vs_rule_10" value="<% CmoGetCfg("vs_rule_10","none"); %>">
                <input type="hidden" id="vs_rule_11" name="vs_rule_11" value="<% CmoGetCfg("vs_rule_11","none"); %>">
                <input type="hidden" id="vs_rule_12" name="vs_rule_12" value="<% CmoGetCfg("vs_rule_12","none"); %>">
                <input type="hidden" id="vs_rule_13" name="vs_rule_13" value="<% CmoGetCfg("vs_rule_13","none"); %>">
                <input type="hidden" id="vs_rule_14" name="vs_rule_14" value="<% CmoGetCfg("vs_rule_14","none"); %>">
                <input type="hidden" id="vs_rule_15" name="vs_rule_15" value="<% CmoGetCfg("vs_rule_15","none"); %>">
                <input type="hidden" id="vs_rule_16" name="vs_rule_16" value="<% CmoGetCfg("vs_rule_16","none"); %>">
                <input type="hidden" id="vs_rule_17" name="vs_rule_17" value="<% CmoGetCfg("vs_rule_17","none"); %>">
                <input type="hidden" id="vs_rule_18" name="vs_rule_18" value="<% CmoGetCfg("vs_rule_18","none"); %>">
                <input type="hidden" id="vs_rule_19" name="vs_rule_19" value="<% CmoGetCfg("vs_rule_19","none"); %>">
                <input type="hidden" id="vs_rule_20" name="vs_rule_20" value="<% CmoGetCfg("vs_rule_20","none"); %>">
                <input type="hidden" id="vs_rule_21" name="vs_rule_21" value="<% CmoGetCfg("vs_rule_21","none"); %>">
                <input type="hidden" id="vs_rule_22" name="vs_rule_22" value="<% CmoGetCfg("vs_rule_22","none"); %>">
                <input type="hidden" id="vs_rule_23" name="vs_rule_23" value="<% CmoGetCfg("vs_rule_23","none"); %>">

                <input type="hidden" id="port_forward_both_00" name="port_forward_both_00" value="<% CmoGetCfg("port_forward_both_00","none"); %>">
                <input type="hidden" id="port_forward_both_01" name="port_forward_both_01" value="<% CmoGetCfg("port_forward_both_01","none"); %>">
                <input type="hidden" id="port_forward_both_02" name="port_forward_both_02" value="<% CmoGetCfg("port_forward_both_02","none"); %>">
                <input type="hidden" id="port_forward_both_03" name="port_forward_both_03" value="<% CmoGetCfg("port_forward_both_03","none"); %>">
                <input type="hidden" id="port_forward_both_04" name="port_forward_both_04" value="<% CmoGetCfg("port_forward_both_04","none"); %>">
                <input type="hidden" id="port_forward_both_05" name="port_forward_both_05" value="<% CmoGetCfg("port_forward_both_05","none"); %>">
                <input type="hidden" id="port_forward_both_06" name="port_forward_both_06" value="<% CmoGetCfg("port_forward_both_06","none"); %>">
                <input type="hidden" id="port_forward_both_07" name="port_forward_both_07" value="<% CmoGetCfg("port_forward_both_07","none"); %>">
                <input type="hidden" id="port_forward_both_08" name="port_forward_both_08" value="<% CmoGetCfg("port_forward_both_08","none"); %>">
                <input type="hidden" id="port_forward_both_09" name="port_forward_both_09" value="<% CmoGetCfg("port_forward_both_09","none"); %>">
                <input type="hidden" id="port_forward_both_10" name="port_forward_both_10" value="<% CmoGetCfg("port_forward_both_10","none"); %>">
                <input type="hidden" id="port_forward_both_11" name="port_forward_both_11" value="<% CmoGetCfg("port_forward_both_11","none"); %>">
                <input type="hidden" id="port_forward_both_12" name="port_forward_both_12" value="<% CmoGetCfg("port_forward_both_12","none"); %>">
                <input type="hidden" id="port_forward_both_13" name="port_forward_both_13" value="<% CmoGetCfg("port_forward_both_13","none"); %>">
                <input type="hidden" id="port_forward_both_14" name="port_forward_both_14" value="<% CmoGetCfg("port_forward_both_14","none"); %>">
                <input type="hidden" id="port_forward_both_15" name="port_forward_both_15" value="<% CmoGetCfg("port_forward_both_15","none"); %>">
                <input type="hidden" id="port_forward_both_16" name="port_forward_both_16" value="<% CmoGetCfg("port_forward_both_16","none"); %>">
                <input type="hidden" id="port_forward_both_17" name="port_forward_both_17" value="<% CmoGetCfg("port_forward_both_17","none"); %>">
                <input type="hidden" id="port_forward_both_18" name="port_forward_both_18" value="<% CmoGetCfg("port_forward_both_18","none"); %>">
                <input type="hidden" id="port_forward_both_19" name="port_forward_both_19" value="<% CmoGetCfg("port_forward_both_19","none"); %>">
                <input type="hidden" id="port_forward_both_20" name="port_forward_both_20" value="<% CmoGetCfg("port_forward_both_20","none"); %>">
                <input type="hidden" id="port_forward_both_21" name="port_forward_both_21" value="<% CmoGetCfg("port_forward_both_21","none"); %>">
                <input type="hidden" id="port_forward_both_22" name="port_forward_both_22" value="<% CmoGetCfg("port_forward_both_22","none"); %>">
                <input type="hidden" id="port_forward_both_23" name="port_forward_both_23" value="<% CmoGetCfg("port_forward_both_23","none"); %>">

                <form id="form1" name="form1" method="post" action="apply.cgi">
                <input type="hidden" id="html_response_page" name="html_response_page" value="back.asp">
                <input type="hidden" id="html_response_message" name="html_response_message" value="">
                <script>get_by_id("html_response_message").value = sc_intro_sv;</script>
                <input type="hidden" id="html_response_return_page" name="html_response_return_page" value="Inbound_Filter.asp">
                <input type="hidden" id="reboot_type" name="reboot_type" value="filter">

                <input type="hidden" id="inbound_filter_name_00" name="inbound_filter_name_00" value="<% CmoGetCfg("inbound_filter_name_00","none"); %>">
                <input type="hidden" id="inbound_filter_name_01" name="inbound_filter_name_01" value="<% CmoGetCfg("inbound_filter_name_01","none"); %>">
                <input type="hidden" id="inbound_filter_name_02" name="inbound_filter_name_02" value="<% CmoGetCfg("inbound_filter_name_02","none"); %>">
                <input type="hidden" id="inbound_filter_name_03" name="inbound_filter_name_03" value="<% CmoGetCfg("inbound_filter_name_03","none"); %>">
                <input type="hidden" id="inbound_filter_name_04" name="inbound_filter_name_04" value="<% CmoGetCfg("inbound_filter_name_04","none"); %>">
                <input type="hidden" id="inbound_filter_name_05" name="inbound_filter_name_05" value="<% CmoGetCfg("inbound_filter_name_05","none"); %>">
                <input type="hidden" id="inbound_filter_name_06" name="inbound_filter_name_06" value="<% CmoGetCfg("inbound_filter_name_06","none"); %>">
                <input type="hidden" id="inbound_filter_name_07" name="inbound_filter_name_07" value="<% CmoGetCfg("inbound_filter_name_07","none"); %>">
                <input type="hidden" id="inbound_filter_name_08" name="inbound_filter_name_08" value="<% CmoGetCfg("inbound_filter_name_08","none"); %>">
                <input type="hidden" id="inbound_filter_name_09" name="inbound_filter_name_09" value="<% CmoGetCfg("inbound_filter_name_09","none"); %>">
                <input type="hidden" id="inbound_filter_name_10" name="inbound_filter_name_10" value="<% CmoGetCfg("inbound_filter_name_10","none"); %>">
                <input type="hidden" id="inbound_filter_name_11" name="inbound_filter_name_11" value="<% CmoGetCfg("inbound_filter_name_11","none"); %>">
                <input type="hidden" id="inbound_filter_name_12" name="inbound_filter_name_12" value="<% CmoGetCfg("inbound_filter_name_12","none"); %>">
                <input type="hidden" id="inbound_filter_name_13" name="inbound_filter_name_13" value="<% CmoGetCfg("inbound_filter_name_13","none"); %>">
                <input type="hidden" id="inbound_filter_name_14" name="inbound_filter_name_14" value="<% CmoGetCfg("inbound_filter_name_14","none"); %>">
                <input type="hidden" id="inbound_filter_name_15" name="inbound_filter_name_15" value="<% CmoGetCfg("inbound_filter_name_15","none"); %>">
                <input type="hidden" id="inbound_filter_name_16" name="inbound_filter_name_16" value="<% CmoGetCfg("inbound_filter_name_16","none"); %>">
                <input type="hidden" id="inbound_filter_name_17" name="inbound_filter_name_17" value="<% CmoGetCfg("inbound_filter_name_17","none"); %>">
                <input type="hidden" id="inbound_filter_name_18" name="inbound_filter_name_18" value="<% CmoGetCfg("inbound_filter_name_18","none"); %>">
                <input type="hidden" id="inbound_filter_name_19" name="inbound_filter_name_19" value="<% CmoGetCfg("inbound_filter_name_19","none"); %>">
                <input type="hidden" id="inbound_filter_name_20" name="inbound_filter_name_20" value="<% CmoGetCfg("inbound_filter_name_20","none"); %>">
                <input type="hidden" id="inbound_filter_name_21" name="inbound_filter_name_21" value="<% CmoGetCfg("inbound_filter_name_21","none"); %>">
                <input type="hidden" id="inbound_filter_name_22" name="inbound_filter_name_22" value="<% CmoGetCfg("inbound_filter_name_22","none"); %>">
                <input type="hidden" id="inbound_filter_name_23" name="inbound_filter_name_23" value="<% CmoGetCfg("inbound_filter_name_23","none"); %>">

                <input type="hidden" id="inbound_filter_ip_00_A" name="inbound_filter_ip_00_A" value="<% CmoGetCfg("inbound_filter_ip_00_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_01_A" name="inbound_filter_ip_01_A" value="<% CmoGetCfg("inbound_filter_ip_01_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_02_A" name="inbound_filter_ip_02_A" value="<% CmoGetCfg("inbound_filter_ip_02_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_03_A" name="inbound_filter_ip_03_A" value="<% CmoGetCfg("inbound_filter_ip_03_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_04_A" name="inbound_filter_ip_04_A" value="<% CmoGetCfg("inbound_filter_ip_04_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_05_A" name="inbound_filter_ip_05_A" value="<% CmoGetCfg("inbound_filter_ip_05_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_06_A" name="inbound_filter_ip_06_A" value="<% CmoGetCfg("inbound_filter_ip_06_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_07_A" name="inbound_filter_ip_07_A" value="<% CmoGetCfg("inbound_filter_ip_07_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_08_A" name="inbound_filter_ip_08_A" value="<% CmoGetCfg("inbound_filter_ip_08_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_09_A" name="inbound_filter_ip_09_A" value="<% CmoGetCfg("inbound_filter_ip_09_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_10_A" name="inbound_filter_ip_10_A" value="<% CmoGetCfg("inbound_filter_ip_10_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_11_A" name="inbound_filter_ip_11_A" value="<% CmoGetCfg("inbound_filter_ip_11_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_12_A" name="inbound_filter_ip_12_A" value="<% CmoGetCfg("inbound_filter_ip_12_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_13_A" name="inbound_filter_ip_13_A" value="<% CmoGetCfg("inbound_filter_ip_13_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_14_A" name="inbound_filter_ip_14_A" value="<% CmoGetCfg("inbound_filter_ip_14_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_15_A" name="inbound_filter_ip_15_A" value="<% CmoGetCfg("inbound_filter_ip_15_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_16_A" name="inbound_filter_ip_16_A" value="<% CmoGetCfg("inbound_filter_ip_16_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_17_A" name="inbound_filter_ip_17_A" value="<% CmoGetCfg("inbound_filter_ip_17_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_18_A" name="inbound_filter_ip_18_A" value="<% CmoGetCfg("inbound_filter_ip_18_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_19_A" name="inbound_filter_ip_19_A" value="<% CmoGetCfg("inbound_filter_ip_19_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_20_A" name="inbound_filter_ip_20_A" value="<% CmoGetCfg("inbound_filter_ip_20_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_21_A" name="inbound_filter_ip_21_A" value="<% CmoGetCfg("inbound_filter_ip_21_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_22_A" name="inbound_filter_ip_22_A" value="<% CmoGetCfg("inbound_filter_ip_22_A","none"); %>">
                <input type="hidden" id="inbound_filter_ip_23_A" name="inbound_filter_ip_23_A" value="<% CmoGetCfg("inbound_filter_ip_23_A","none"); %>">

                <input type="hidden" id="inbound_filter_ip_00_B" name="inbound_filter_ip_00_B" value="<% CmoGetCfg("inbound_filter_ip_00_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_01_B" name="inbound_filter_ip_01_B" value="<% CmoGetCfg("inbound_filter_ip_01_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_02_B" name="inbound_filter_ip_02_B" value="<% CmoGetCfg("inbound_filter_ip_02_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_03_B" name="inbound_filter_ip_03_B" value="<% CmoGetCfg("inbound_filter_ip_03_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_04_B" name="inbound_filter_ip_04_B" value="<% CmoGetCfg("inbound_filter_ip_04_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_05_B" name="inbound_filter_ip_05_B" value="<% CmoGetCfg("inbound_filter_ip_05_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_06_B" name="inbound_filter_ip_06_B" value="<% CmoGetCfg("inbound_filter_ip_06_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_07_B" name="inbound_filter_ip_07_B" value="<% CmoGetCfg("inbound_filter_ip_07_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_08_B" name="inbound_filter_ip_08_B" value="<% CmoGetCfg("inbound_filter_ip_08_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_09_B" name="inbound_filter_ip_09_B" value="<% CmoGetCfg("inbound_filter_ip_09_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_10_B" name="inbound_filter_ip_10_B" value="<% CmoGetCfg("inbound_filter_ip_10_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_11_B" name="inbound_filter_ip_11_B" value="<% CmoGetCfg("inbound_filter_ip_11_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_12_B" name="inbound_filter_ip_12_B" value="<% CmoGetCfg("inbound_filter_ip_12_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_13_B" name="inbound_filter_ip_13_B" value="<% CmoGetCfg("inbound_filter_ip_13_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_14_B" name="inbound_filter_ip_14_B" value="<% CmoGetCfg("inbound_filter_ip_14_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_15_B" name="inbound_filter_ip_15_B" value="<% CmoGetCfg("inbound_filter_ip_15_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_16_B" name="inbound_filter_ip_16_B" value="<% CmoGetCfg("inbound_filter_ip_16_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_17_B" name="inbound_filter_ip_17_B" value="<% CmoGetCfg("inbound_filter_ip_17_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_18_B" name="inbound_filter_ip_18_B" value="<% CmoGetCfg("inbound_filter_ip_18_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_19_B" name="inbound_filter_ip_19_B" value="<% CmoGetCfg("inbound_filter_ip_19_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_20_B" name="inbound_filter_ip_20_B" value="<% CmoGetCfg("inbound_filter_ip_20_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_21_B" name="inbound_filter_ip_21_B" value="<% CmoGetCfg("inbound_filter_ip_21_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_22_B" name="inbound_filter_ip_22_B" value="<% CmoGetCfg("inbound_filter_ip_22_B","none"); %>">
                <input type="hidden" id="inbound_filter_ip_23_B" name="inbound_filter_ip_23_B" value="<% CmoGetCfg("inbound_filter_ip_23_B","none"); %>">
                <input type="hidden" id="edit" name="edit" value="-1">

            <td valign="top" id="maincontent_container">
                <div id="maincontent">
                  <div id=box_header>
                    <h1><script>show_words(_inboundfilter)</script></h1>
                    <script>show_words(ai_intro_1)</script><br><br>
                    <script>show_words(ai_intro_2)</script>
                    <script>show_words(ai_intro_3)</script>
                    <br><br>
                    <script>check_reboot();</script>
                  </div>
                  <div class=box>
                    <h2><span id="inbound_filter_name_rule_title"><script>show_words(_add)</script></span> <script>show_words(ai_title_IFR)</script></h2>
                    <table cellSpacing=1 cellPadding=2 width=500 border=0>
                        <tr>

                <td align=right class="duple">
                  <script>show_words(_name)</script>
                  :</td>
                          <td>
                            <input type="text" id="ingress_filter_name" size="20" maxlength="15">
                          </td>
                        </tr>
                        <tr>

                <td align=right class="duple">
                  <script>show_words(ai_Action)</script>
                  :</td>
                          <td>
                            <select id="action_select">
						  		<option value="allow"><script>show_words(_allow)</script></option>
						  		<option value="deny"><script>show_words(_deny)</script></option>
                            </select>
                          </td>
                        </tr>
                        <tr>

                <td align=right valign="top" class="duple">
                  <script>show_words(at_ReIPR)</script>
                  :</td>
                          <td>
                            <table cellSpacing=1 cellPadding=2 width=300 border=0>
                                <tr>

                      <td><b><script>show_words(_enable)</script></b></td>
			<td><b><script>show_words(KR5)</script></b></td>
			<td><b><script>show_words(KR6)</script></b></td>
                                </tr>
                                <script>
                                    for(i=0;i<8;i++){
                                        document.write("<tr>")
                                        document.write("<td align=\"middle\"><INPUT type=\"checkbox\" id=\"entry_enable_"+ i +"\" id=\"entry_enable_"+ i +"\" value=\"1\"></td>")
                                        document.write("<td><input id=\"ip_start_" + i + "\" name=\"ip_start_" + i + "\" size=\"15\" maxlength=\"15\" value=\"0.0.0.0\"></td>")
                                        document.write("<td><input id=\"ip_end_" + i + "\" name=\"ip_end_" + i + "\" size=\"15\" maxlength=\"15\" value=\"255.255.255.255\"></td>")
                                        document.write("</tr>")
                                    }
                                </script>
                            </table>
                          </td>
                        </tr>
                        <tr>
                          <td></td>
                          <td>
                            <input type="submit" id="button1" name="button1" class=button_submit value="" onClick="return send_request();">
                            <input type="reset" id="button2" name="button2"class=button_submit value="">
                            <script>get_by_id("button1").value = _add;</script>
                            <script>get_by_id("button2").value = _clear;</script>
                          </td>
                        </tr>
                    </table>
                  </div>
                  <div class=box>
            <h2>
              <script>show_words(ai_title_IFRL)</script>
            </h2>
                    <table borderColor=#ffffff cellSpacing=1 cellPadding=2 width=525 bgColor=#dfdfdf border=1>
                        <tr align=center>

                <td align=middle width=20>
                  <b><script>show_words(_name)</script></b>
                </td>

                <td align=middle width=20>
                  <b><script>show_words(ai_Action)</script></b>
                </td>

                <td width="255">
                  <b><script>show_words(at_ReIPR)</script></b>
                </td>
                          <td align=middle width=20><b>&nbsp;</b></td>
                          <td align=middle width=20><b>&nbsp;</b></td>
                        </tr>
                        <script>
                        set_Inbound();
                        for(var i=0;i<DataArray.length;i++){
                            document.write("<tr>")
                            document.write("<td>"+ DataArray[i].Name +"</td>")
                            document.write("<td>"+ DataArray[i].sAction +"</td>")
                            document.write("<td>"+ DataArray[i].Show_W +"</td>")
                            document.write("<td><a href=\"javascript:edit_row("+ i +")\"><img src=\"edit.jpg\" border=\"0\" alt=\"edit\"></a></td>")
                            document.write("<td><a href=\"javascript:del_row(" + i +")\"><img src=\"delete.jpg\"  border=\"0\" alt=\"delete\"></a></td>")
                            document.write("</tr>")
                        }
                      </script>
                    </table>
                  </div>
              </div>
            </td>
        </form>
            <td valign="top" width="150" id="sidehelp_container" align="left">
                <table width="125" border=0
      cellPadding=2 cellSpacing=0 borderColor=#ffffff borderColorLight=#ffffff borderColorDark=#ffffff bgColor=#ffffff>
                    <tr>

          <td id=help_text><strong>
            <script>show_words(_hints)</script>
            &hellip;</strong> <p>
              <script>show_words(hhai_name)</script>
            </p>
            <p>
              <script>show_words(hhai_action)</script>
            </p>
            <p>
              <script>show_words(hhai_ipr)</script>
            </p>
            <p>
              <script>show_words(hhai_ip)</script>
            </p>
            <p>
              <script>show_words(hhai_save)</script>
            </p>
            <p>
              <script>show_words(hhai_edit)</script>
            </p>
            <p>
              <script>show_words(hhai_delete)</script>
            </p>
						<p class="more"><a href="support_adv.asp#Inbound_Filter" onclick="return jump_if();"><script>show_words(_more)</script>&hellip;</a></p>
                      </td>
                    </tr>
                </table></td>
        </tr>
    </table>
    <table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
        <tr>
            <td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
            <td width="10">&nbsp;</td><td>&nbsp;</td>
        </tr>
    </table>
<br>
<div id="copyright"><% CmoGetStatus("title"); %></div>
<br>
</body>
<script>
	reboot_form();
	onPageLoad();
</script>
</html>
