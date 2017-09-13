#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>Wireless Configuration</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 <script type='text/javascript'>
 function cbxClicked(is_checked)
 {
  document.getElementById('ssid').disabled = !is_checked;
  document.getElementById('security').disabled = !is_checked;
 }
 </script>
 </head>"

conf_file=/etc/ath/apcfg
sys_conf_file=/etc/rgw_config
psk_config_file=/etc/wpa2/config_psk.ap_bss

# Get wireless status from config file
config_wireless_enabled=`grep "^WIRELESSENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
# Get current SSID
ssid=`grep "export AP_SSID" $conf_file | awk -F'=' '{print $2}'`
# Get current security settings.
security=`grep "export AP_SECMODE" $conf_file | awk -F'=' '{print $2}'`
wpa_passphrase=`grep "wpa_passphrase" $psk_config_file | awk -F'=' '{print $2}'`

if [ $config_wireless_enabled = 'YES' ]; then
   wireless_enabled="checked='checked'"
else
   wireless_disabled="disabled='true'"
fi

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/wireless_config.cgi' target='result'>
 <div class='section'>

 <div class='section_head'>
 <h2>Wireless Configuration</h2>
 <p>Enable or disable wireless connection and configure wireless settings.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 <iframe name='result' width='50%' height='20' frameborder='0' scrolling='no'></iframe>
 </div>

 <div class='box'>
 <h3>Enable/Disable</h3>
 <fieldset>
  <p>
   <label class='duple'>Enable:</label>
   <input name='cbx_enable' type='checkbox' $wireless_enabled onClick='cbxClicked(this.checked);' />
  </p>
 </fieldset>
 </div>

 <div class='box'>
 <h3>Wireless Settings</h3>
 <fieldset>
  <p>
   <label class='duple'>SSID:</label>
   <input name='ssid' type='text' id='ssid' value='$ssid' $wireless_disabled>
  </p>

 <label class='duple'>Security:</label>
    <select name="security">
	<option selected="selected" value="NONE">No Security</option>
	<option value="WPA">WPA-2</option>
    </select>

  <p>
   <label class='duple'>WPA Passphrase:</label>
   <input name='wpa_passphrase' type='text' id='wpa_passphrase' value='$wpa_passphrase' $wireless_disabled>
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"
