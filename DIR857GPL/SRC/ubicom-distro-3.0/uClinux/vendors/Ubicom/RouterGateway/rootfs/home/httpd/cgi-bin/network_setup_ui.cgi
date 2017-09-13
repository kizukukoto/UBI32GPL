#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>Network setup</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

conf_file=/etc/config/dnsmasq.conf
sysconf_file=/etc/rgw_config

. /etc/rgw_config

   # Get range info from config file
   lan_ip=`grep "^BRIDGEIP" $sysconf_file | awk -F'=' '{print $2}'`
   lan_mask=`grep "^BRIDGEMASK" $sysconf_file | awk -F'=' '{print $2}'`
#   lan_mac=`grep "^LANMAC" $sysconf_file | awk -F'=' '{print $2}'`
   lan_mac=`ifconfig $BRINTERFACE | grep -o "HWaddr .*" | awk '{ print $2 }'`

# Debug outputs
# echo "range_info=$range_info"
# echo "tf_range_start=$tf_range_start"
# echo "tf_range_end=$tf_range_end
# echo "dhcp_enabled=$dhcp_enabled"

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/network_setup.cgi' target='result'> 
 <div class='section'>

 <div class='section_head'>
 <h2>Network Setup</h2>
 <p>Change your LAN settings.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 <iframe name='result' width='50%' height='20' frameborder='0' scrolling='no'></iframe>
 </div>

 <div class='box'>
 <h3>LAN settings</h3>
 <fieldset>
  <p>
   <label class='duple'>MAC address:</label>
    $lan_mac
  </p>
  <p>
   <label class='duple'>Device IP:</label>
   <input name='tf_lan_ip' type='text' id='tf_lan_ip' value='$lan_ip'>
  </p>
  <p>
   <label class='duple'>Subnet mask:</label>
   <input name='tf_lan_mask' type='text' id='tf_lan_mask' value='$lan_mask'>
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"
