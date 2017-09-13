#!/bin/sh

sysconf_file=/etc/rgw_config

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>PPTP Configuration</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

username=`grep "^PPTPUSERNAME" $sysconf_file | awk -F'=' '{print $2}'`
password=`grep "^PPTPPASSWORD" $sysconf_file | awk -F'=' '{print $2}'`
pptp_server_ip=`grep "^PPTPSERVERIP" $sysconf_file | awk -F'=' '{print $2}'`
wan_ip=`grep "^PPTPWANIP" $sysconf_file | awk -F'=' '{print $2}'`
wan_subnet=`grep "^PPTPSUBNETMASK" $sysconf_file | awk -F'=' '{print $2}'`
wan_gateway=`grep "^PPTPGATEWAYIP" $sysconf_file | awk -F'=' '{print $2}'`

echo "<body>

 <p>&nbsp;</p>
 
 <form name='form1' method='get' action='/cgi-bin/pptp_config.cgi' target='result'>

 <div class='section'>
 <div class='section_head'>
 <h2>PPTP Configuration</h2>
 <p>Save PPTP Settings.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 <iframe name='result' width='50%' height='30' frameborder='0' scrolling='no'></iframe>
 </div>

 <div class='box'>
 <h3>Username/Password</h3>
  <fieldset>
   <p>
    <label class='duple'>User Name</label>
    <input name='username' type='text' id='username' value='$username'>
   </p>
 
   <p>
    <label class='duple'>Password</label>
    <input name='password' type='password' id='password' value='$password'>
   </p>
  </fieldset>
 </div>
 
 <div class='box'>
 <h3>IP Addresses</h3>
  <fieldset>
   <p>
    <label class='duple'>PPTP Server IP</label>
    <input name='pptp_server_ip' type='text' id='pptp_server_ip' value='$pptp_server_ip'>
   </p>
 
   <p>
    <label class='duple'>Specify WAN IP Address</label>
    <input name='wan_ip' type='text' id='wan_ip' value='$wan_ip'>
   </p>
   
   <p>
    <label class='duple'>Subnet Mask</label>
    <input name='wan_subnet' type='text' id='wan_subnet' value='$wan_subnet'>
   </p>
   
   <p>
    <label class='duple'>Default Gateway Address</label>
    <input name='wan_gateway' type='text' id='wan_gateway' value='$wan_gateway'>
   </p>
 
  </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"
