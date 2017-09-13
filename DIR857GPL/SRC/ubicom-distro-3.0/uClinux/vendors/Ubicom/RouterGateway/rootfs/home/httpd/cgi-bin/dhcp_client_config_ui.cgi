#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>DHCP WAN Configuration</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 <script type='text/javascript'>
 function cbxClicked(is_checked)
 {
  document.getElementById('dns1').disabled = !is_checked;
  document.getElementById('dns2').disabled = !is_checked;
 }
 </script>
 </head>"
 
sys_conf_file=/etc/rgw_config

# Get user special DNS setting status from config file
config_user_special_dns=`grep "^DHCPUSERSPECIALDNS" $sys_conf_file | awk -F'=' '{print $2}'`

if [ $config_user_special_dns = "YES" ]; then
   dns_enabled="checked='checked'"
else
   dns_disabled="disabled='true'"
fi

dns1=`grep "^DHCPDNS1" $sys_conf_file | awk -F'=' '{print $2}'`
dns2=`grep "^DHCPDNS2" $sys_conf_file | awk -F'=' '{print $2}'`

echo "<body>

 <p>&nbsp;</p>
 
 <form name='form1' method='get' action='/cgi-bin/dhcp_client_config.cgi' target='result'>
 <div class='section'>

 <div class='section_head'>
  <h2>DHCP WAN Configuration</h2>
  <p>Save DHCP WAN Configuration Settings</p>
  <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
  <iframe name='result' width='50%' height='30' frameborder='0' scrolling='no'></iframe>
 </div>

 <div class='box'>
  <h3>Enable/Disable</h3>
  <fieldset>
   <p>
    <label class='duple'>Use the Following DNS Server Addresses:</label>
    <input name='cbx_enable' type='checkbox' $dns_enabled onClick='cbxClicked(this.checked);'>
   </p>
   </fieldset>
 </div>

 <div class='box'>
   <fieldset>
   <p>
    <label class='duple'>DNS 1:</label>
    <input name='dns1' type='text' id='dns1' value='$dns1' $dns_disabled>
   </p>
  
   <p>
    <label class='duple'>DNS 2:</label>
    <input name='dns2' type='text' id='dns2' value='$dns2' $dns_disabled>
   </p>
 
  </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"
