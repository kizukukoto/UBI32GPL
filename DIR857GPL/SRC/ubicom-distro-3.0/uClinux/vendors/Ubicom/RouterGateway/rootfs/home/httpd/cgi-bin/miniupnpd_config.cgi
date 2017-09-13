#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>Set UPnP settings</TITLE></HEAD>
 <BODY>
 <pre>"
#env

conf_file=/etc/miniupnpd/miniupnpd.conf
sys_conf_file=/etc/rgw_config

# Get upnp_enabled from config file
config_upnp_enabled=`grep "^UPNPENABLED" $sys_conf_file | awk -F'=' '{print $2}'`

# Learn if UPnP enabled param is on, its format is like 'cbx_enable=on'
all_params=$QUERY_STRING
cbx_enable_param=`echo $all_params | grep -o "cbx_enable=[a-z]*"`
cbx_enable=`echo "$cbx_enable_param" | awk -F'=' '{print $2}'`

if [ $config_upnp_enabled = 'YES' ]; then
	if [ $cbx_enable = on ]; then
		echo "UPnP already enabled" >> /var/log/messages
	else
		echo "Stopping UPnP" >> /var/log/messages
		/etc/init.d/do_miniupnpd stop > /dev/null 2>&1
		sed -i -e 's/^UPNPENABLED=[a-zA-Z]*/UPNPENABLED=NO/' $sys_conf_file
		echo "UPnP stopped" >> /var/log/messages
	fi
else
	if [ $cbx_enable = on ]; then
		echo "Starting UPnP" >> /var/log/messages
		/etc/init.d/do_miniupnpd start > /dev/null 2>&1
		if [ -f /var/run/miniupnpd.pid ]; then
			sed -i -e 's/^UPNPENABLED=[a-zA-Z]*/UPNPENABLED=YES/' $sys_conf_file
			echo "UPnP started" >> /var/log/messages
		else
			echo "Error: Could not start UPnP!" >> /var/log/messages
		fi
	else
		echo "UPnP already not enabled" >> /var/log/messages
	fi
fi

echo "Done

</pre>
<script type='text/javascript'>
parent.active_config.location='/cgi-bin/miniupnpd_config_ui.cgi';
</script>
</BODY></HTML>"
