#!/bin/sh

. /etc/rgw_config

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>Static WAN Configuration</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

 wan_ip=$STATICWANIP
 wan_subnet=$STATICWANMASK
 wan_gateway=$STATICGATEWAY
 dns1=$STATICDNS1
 dns2=$STATICDNS2

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/static_config.cgi' target='result'>
 <div class='section'>

 <div class='section_head'>
 <h2>Static WAN Configuration</h2>
 <p>Decide your WAN IP, netmask, default gateway, and DNS addresses.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 <iframe name='result' width='50%' height='30' frameborder='0' scrolling='no'></iframe>
 </div>

 <div class='box'>
 <h3>IP addresses</h3>
 <fieldset>
  <p>
   <label class='duple'>WAN IP:</label>
   <input name='wan_ip' type='text' id='wan_ip' value='$wan_ip'>
  </p>
  <p>
   <label class='duple'>Netmask:</label>
   <input name='wan_subnet' type='text' id='wan_subnet' value='$wan_subnet'>
  </p>
  <p>
   <label class='duple'>Default gateway:</label>
   <input name='wan_gateway' type='text' id='wan_gateway' value='$wan_gateway'>
  </p>
 </fieldset>
 </div>

 <div class='box'>
 <h3>DNS addresses</h3>
 <fieldset>
  <p>
   <label class='duple'>DNS1:</label>
   <input name='dns1' type='text' id='dns1' value='$dns1'>
  </p>
  <p>
   <label class='duple'>DNS2:</label>
   <input name='dns2' type='text' id='dns2' value='$dns2'>
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </form>
 </div>

 </body>
 </html>"
