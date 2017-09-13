#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>Dhcp server configuration</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 <script type='text/javascript'>
 function cbxClicked(is_checked)
 {
  document.getElementById('tf_range_start').disabled = !is_checked;
  document.getElementById('tf_range_end').disabled = !is_checked;
 }
 </script>
 </head>"

conf_file=/etc/config/dnsmasq.conf

   # Get range info from config file
   range_info=`grep "^dhcp-range" $conf_file | awk -F'=' '{print $2}'`
if [ $range_info != '' ]; then
   dhcp_enabled="checked='checked'"
else
   range_info=`grep "^#dhcp-range" $conf_file | awk -F'=' '{print $2}'`
   tf_x_disabled="disabled='true'"
fi

   tf_range_start=`echo "$range_info" | awk -F',' '{print $1}'`
   tf_range_end=`echo "$range_info" | awk -F',' '{print $2}'`
#    lb_netmask=`echo "$range_info" | awk -F',' '{print $3}'`
#    lb_lease=`echo "$range_info" | awk -F',' '{print $4}'`

# Debug outputs
# echo "range_info=$range_info"
# echo "tf_range_start=$tf_range_start"
# echo "tf_range_end=$tf_range_end
# echo "dhcp_enabled=$dhcp_enabled"

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/dhcp_server_config.cgi'>
 <div class='section'>

 <div class='section_head'>
 <h2>DHCP server</h2>
 <p>Enable or disable your DHCP server and define a new IP range.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 </div>

 <div class='box'>
 <h3>Enable/disable</h3>
 <fieldset>
  <p>
   <label class='duple'>Enable:</label>
   <input name='cbx_enable' type='checkbox' $dhcp_enabled onClick='cbxClicked(this.checked);' />
  </p>
 </fieldset>
 </div>

 <div class='box'>
 <h3>Dynamic IP range</h3>
 <fieldset>
  <p>
   <label class='duple'>Range start:</label>
   <input name='tf_range_start' type='text' id='tf_range_start' value='$tf_range_start' $tf_x_disabled>
  </p>
  <p>
   <label class='duple'>Range end:</label>
   <input name='tf_range_end' type='text' id='tf_range_end' value='$tf_range_end' $tf_x_disabled>
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"
