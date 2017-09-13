#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>Set Wireless Settings</TITLE></HEAD>
 <BODY>
 <pre>"
#env

conf_file=/etc/ath/apcfg
sys_conf_file=/etc/rgw_config
psk_config_file=/etc/wpa2/config_psk.ap_bss

# Get current wireless status from the config file
config_wireless_enabled=`grep "^WIRELESSENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
# Get current SSID and security settings from the config file
current_ssid=`grep "export AP_SSID" $conf_file | awk -F'=' '{print $2}'`
current_security=`grep "export AP_SECMODE" $conf_file | awk -F'=' '{print $2}'`
current_wpa_passphrase=`grep "wpa_passphrase" $psk_config_file | awk -F'=' '{print $2}'`

# Learn if Wireless enabled param is on, its format is like 'cbx_enable=on'
all_params=$QUERY_STRING
cbx_enable_param=`echo $all_params | grep -o "cbx_enable=[a-z]*"`
cbx_enable=`echo "$cbx_enable_param" | awk -F'=' '{print $2}'`

# Get param=value pairs
ssid_param=`echo $all_params | grep -o "ssid=[0-9a-zA-Z_]*"`
security_param=`echo $all_params | grep -o "security=[0-9a-zA-Z]*"`
wpa_passphrase_param=`echo $all_params | grep -o "wpa_passphrase=[0-9a-zA-Z]*"`

# Get value of the params
ssid=`echo "$ssid_param" | awk -F'=' '{print $2}'`
security=`echo "$security_param" | awk -F'=' '{print $2}'`
wpa_passphrase=`echo "$wpa_passphrase_param" | awk -F'=' '{print $2}'`

if [ $config_wireless_enabled = 'YES' ]; then
	if [ $cbx_enable = on ]; then
		if [ "$ssid" != "$current_ssid" ] || [ "$security" != "$current_security" ] || [ "$wpa_passphrase" != "$current_wpa_passphrase" ]; then
			sed -i '/^    export AP_SSID=/ c\    export AP_SSID='$ssid'' $conf_file
			sed -i '/^    export AP_SECMODE=/ c\    export AP_SECMODE='$security'' $conf_file
			sed -i '/^wpa_passphrase=/ c\wpa_passphrase='$wpa_passphrase'' $psk_config_file
			echo "Wireless settings changed. Stopping Wireless..." >> /var/log/messages
			/etc/ath/apdown > /dev/null 2>&1
			echo "Restarting wireless with new settings..." >> /var/log/messages
			/etc/ath/apup > /dev/null 2>&1
		fi
	else
		echo "Stopping Wireless" >> /var/log/messages
		sed -i '/^WIRELESSENABLED=/ c\WIRELESSENABLED='NO'' $sys_conf_file
		/etc/ath/apdown > /dev/null 2>&1
	fi
else
	if [ $cbx_enable = on ]; then
		if [ -f /lib/modules/2.6.28/net/ath_hal.ko ] || [ -f /lib/modules/ath_hal.ko ]; then
			if [ -f /etc/ath/apup ]; then
				echo "Starting wireless" >> /var/log/messages
				sed -i '/^WIRELESSENABLED=/ c\WIRELESSENABLED='YES'' $sys_conf_file
				if [ "$ssid" != "$current_ssid" ] || [ "$security" != "$current_security" ] || [ "$wpa_passphrase" != 															"$current_wpa_passphrase" ]; then
					sed -i '/^    export AP_SSID=/ c\    export AP_SSID='$ssid'' $conf_file
					sed -i '/^    export AP_SECMODE=/ c\    export AP_SECMODE='$security'' $conf_file
					sed -i '/^wpa_passphrase=/ c\wpa_passphrase='$wpa_passphrase'' $psk_config_file
				fi
				/etc/ath/apup > /dev/null 2>&1
				sleep 1
			else
				# Although wireless is enabled, we don`t have the necessary scripts to run wireless.
				echo "Wireless start script could not be found!" >> /var/log/messages
				echo "Cannot enable wireless!" >> /var/log/messages
			fi
		else
			# Although wireless is enabled, we don`t have the necessary moduleso run wireless.
			echo "Wireless modules could not be found!" >> /var/log/messages
			echo "Cannot enable wireless!" >> /var/log/messages
		fi
	else
		echo "Wireless already disabled!" >> /var/log/messages
	fi
fi

echo "Done"

echo "</pre>"
echo "</BODY></HTML>"
