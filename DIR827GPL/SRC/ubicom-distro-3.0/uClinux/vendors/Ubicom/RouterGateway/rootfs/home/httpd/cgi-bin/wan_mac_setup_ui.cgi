#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>WAN MAC address setup</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

sysconf_file=/etc/rgw_config

# Get WAN MAC address
wan_mac1=`grep "^WANMAC" $sysconf_file | awk -F'=' '{print $2}' | awk -F':' '{print $1}'`
wan_mac2=`grep "^WANMAC" $sysconf_file | awk -F'=' '{print $2}' | awk -F':' '{print $2}'`
wan_mac3=`grep "^WANMAC" $sysconf_file | awk -F'=' '{print $2}' | awk -F':' '{print $3}'`
wan_mac4=`grep "^WANMAC" $sysconf_file | awk -F'=' '{print $2}' | awk -F':' '{print $4}'`
wan_mac5=`grep "^WANMAC" $sysconf_file | awk -F'=' '{print $2}' | awk -F':' '{print $5}'`
wan_mac6=`grep "^WANMAC" $sysconf_file | awk -F'=' '{print $2}' | awk -F':' '{print $6}'`

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/wan_mac_setup.cgi'> 
 <div class='section'>

 <div class='section_head'>
 <h2>WAN MAC Address Setup</h2>
 <p>Change your WAN MAC address.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 </div>

 <div class='box'>
 <h3>MAC Address</h3>
 <fieldset>
  <p>
   <label class='duple'>WAN MAC address:</label>
   <input size="2" maxlength="2" name='tf_wan_mac1' type='text' id='tf_wan_mac1' value='$wan_mac1'>:
   <input size="2" maxlength="2" name='tf_wan_mac2' type='text' id='tf_wan_mac2' value='$wan_mac2'>:
   <input size="2" maxlength="2" name='tf_wan_mac3' type='text' id='tf_wan_mac3' value='$wan_mac3'>:
   <input size="2" maxlength="2" name='tf_wan_mac4' type='text' id='tf_wan_mac4' value='$wan_mac4'>:
   <input size="2" maxlength="2" name='tf_wan_mac5' type='text' id='tf_wan_mac5' value='$wan_mac5'>:
   <input size="2" maxlength="2" name='tf_wan_mac6' type='text' id='tf_wan_mac6' value='$wan_mac6'>
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"
