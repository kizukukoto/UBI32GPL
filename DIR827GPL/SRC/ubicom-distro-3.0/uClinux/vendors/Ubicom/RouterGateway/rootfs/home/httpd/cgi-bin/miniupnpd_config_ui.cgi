#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>UPnP configuration</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

conf_file=/etc/miniupnpd/miniupnpd.conf
sys_conf_file=/etc/rgw_config

# Get upnp_enabled from config file
   config_upnp_enabled=`grep "^UPNPENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
if [ $config_upnp_enabled = 'YES' ]; then
   upnp_enabled="checked='checked'"
fi

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/miniupnpd_config.cgi' target='debug_frame'>
 <div class='section'>

 <div class='section_head'>
 <h2>UPnP Configuration</h2>
 <p>Enable or disable UPnP.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 </div>

 <div class='box'>
 <h3>Enable/Disable</h3>
 <fieldset>
  <p>
   <label class='duple'>Enable</label>
   <input name='cbx_enable' type='checkbox' $upnp_enabled />
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </form>
 </div>

 </body>
 </html>"
