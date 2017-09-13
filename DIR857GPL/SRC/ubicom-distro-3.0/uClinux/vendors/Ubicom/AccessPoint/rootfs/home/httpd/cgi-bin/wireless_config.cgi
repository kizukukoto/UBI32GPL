#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>Set Wireless Settings</TITLE></HEAD>
 <BODY>
 <pre>"
#env

conf_file=/etc/ath/apcfg
sys_conf_file=/etc/ap_config

# Get current wireless status from the config file
config_wireless_enabled=`grep "^WIRELESSENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
# Get current SSID and security settings from the config file
current_ssid=`grep "export AP_SSID" $conf_file | awk -F'=' '{print $2}'`
current_security=`grep "export AP_SECMODE" $conf_file | awk -F'=' '{print $2}'`

# Learn if Wireless enabled param is on, its format is like 'cbx_enable=on'
all_params=$QUERY_STRING
cbx_enable_param=`echo $all_params | grep -o "cbx_enable=[a-z]*"`
cbx_enable=`echo "$cbx_enable_param" | awk -F'=' '{print $2}'`

# Get param=value pairs
ssid_param=`echo $all_params | grep -o "ssid=[0-9a-zA-Z_]*"`
security_param=`echo $all_params | grep -o "security=[0-9a-zA-Z]*"`

# Get value of the params
ssid=`echo "$ssid_param" | awk -F'=' '{print $2}'`
security=`echo "$security_param" | awk -F'=' '{print $2}'`

#echo "all_params=$all_params"
#echo "New SSID: $ssid"
#echo "Current SSID: $current_ssid"
#echo "New security: $security"
#echo "Current security: $current_security"
#echo "Wireless already enabled=$config_wireless_enabled"
#echo "New wireless status=$cbx_enable"

if [ $config_wireless_enabled = 'YES' ]; then
	if [ $cbx_enable = on ]; then
		if [ "$ssid" != "$current_ssid" ] || [ "$security" != "$current_security" ]; then
			sed -i '/^    export AP_SSID=/ c\    export AP_SSID='$ssid'' $conf_file
			sed -i '/^    export AP_SECMODE=/ c\    export AP_SECMODE='$security'' $conf_file
			echo "Wireless settings changed. Stopping Wireless..."
			/etc/ath/apdown
			echo "Restarting wireless with new settings..."
			/etc/ath/apup
		fi
	else
		echo "Stopping Wireless"
		sed -i '/^WIRELESSENABLED=/ c\WIRELESSENABLED='NO'' $sys_conf_file
		/etc/ath/apdown
	fi
else
	if [ $cbx_enable = on ]; then
		if [ -f /lib/modules/2.6.28/net/ath_hal.ko ] || [ -f /lib/modules/ath_hal.ko ]; then
			if [ -f /etc/ath/apup ]; then
				echo "Starting wireless"
				sed -i '/^WIRELESSENABLED=/ c\WIRELESSENABLED='YES'' $sys_conf_file
				if [ "$ssid" != "$current_ssid" ] || [ "$security" != "$current_security" ]; then
					sed -i '/^    export AP_SSID=/ c\    export AP_SSID='$ssid'' $conf_file
					sed -i '/^    export AP_SECMODE=/ c\    export AP_SECMODE='$security'' $conf_file
				fi
				/etc/ath/apup
				sleep 1
			else
				# Although wireless is enabled, we don`t have the necessary scripts to run wireless.
				echo "Wireless start script could not be found!"
				echo "Cannot enable wireless!"
			fi
		else
			# Although wireless is enabled, we don`t have the necessary moduleso run wireless.
			echo "Wireless modules could not be found!"
			echo "Cannot enable wireless!"
		fi
	else
		echo "Wireless already disabled!"
	fi
fi

echo "Done"

echo "</pre>"
echo "</BODY></HTML>"
