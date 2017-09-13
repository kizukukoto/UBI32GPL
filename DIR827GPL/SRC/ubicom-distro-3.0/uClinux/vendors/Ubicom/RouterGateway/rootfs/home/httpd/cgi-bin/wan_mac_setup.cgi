#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<HTML><HEAD><TITLE>WAN MAC setup</TITLE></HEAD>
 <BODY>
 <pre>"

sysconf_file=/etc/rgw_config

. /etc/rgw_config

all_params=$QUERY_STRING

# Get param=value pairs
# FIXME: find a better way instead of using cut -c13-14
wan_mac1=`echo $all_params | grep -o "tf_wan_mac1=[[:xdigit:]]*" | awk -F'=' '{print $2}'`
wan_mac2=`echo $all_params | grep -o "tf_wan_mac2=[[:xdigit:]]*" | awk -F'=' '{print $2}'`
wan_mac3=`echo $all_params | grep -o "tf_wan_mac3=[[:xdigit:]]*" | awk -F'=' '{print $2}'`
wan_mac4=`echo $all_params | grep -o "tf_wan_mac4=[[:xdigit:]]*" | awk -F'=' '{print $2}'`
wan_mac5=`echo $all_params | grep -o "tf_wan_mac5=[[:xdigit:]]*" | awk -F'=' '{print $2}'`
wan_mac6=`echo $all_params | grep -o "tf_wan_mac6=[[:xdigit:]]*" | awk -F'=' '{print $2}'`

wan_mac=$wan_mac1:$wan_mac2:$wan_mac3:$wan_mac4:$wan_mac5:$wan_mac6
wan_mac_ok=`echo $wan_mac | egrep '^([[:xdigit:]]{2}:[[:xdigit:]]{2}:[[:xdigit:]]{2}:[[:xdigit:]]{2}:[[:xdigit:]]{2}:[[:xdigit:]]{2})$'`

if [ "$wan_mac_ok" == "" ]; then
	echo "Error! Invalid MAC address: $wan_mac"
	echo "MAC address not changed!"
else
	# Check if it is a unicast addres (first octet must be an even number)
	# The args param is set to 'ec', which means 'even check' (returns 1 if the number is even, otherwise 0.)
	wan_mac1_ok=`awk -v args=ec -v number=$wan_mac1 -f /etc/init.d/helper/utils.awk`
	if [ "$wan_mac1_ok" != "1" ]; then
		echo "Error! Cannot set to a multicast MAC address: $wan_mac. First octet must be an even number."
		echo "MAC address not changed!"
	else
		echo "Your WAN IP address might change if you use dhcp. Please use your new IP address to reconnect to the web page."

		# Stop the current WAN connection
		$SCRIPT_PATH/wan_stop > /dev/null 2>&1

		# Pull down the WAN interface
		wan=`ifconfig | grep $WANINTERFACE`
		if [ "$wan" != "" ]; then
			ifconfig $WANINTERFACE 0.0.0.0 down
		fi

		# Try to set the new MAC address
		ifconfig $WANINTERFACE hw ether $wan_mac
		wan_mac_set=`ifconfig $WANINTERFACE | grep "HWaddr" | awk -F'HWaddr ' '{print $2}'`
		if [ "$wan_mac_set" == "" ]; then
			echo "Error!"
			echo "MAC address not changed!"
		else
			# Write the new WAN MAC address to the system config file
			sed -i '/^WANMAC=/ c\WANMAC='$wan_mac'' $sysconf_file
			echo "WAN MAC address set to $wan_mac"
		fi
	
		if [ -f /etc/config/dhcpcd-$WANINTERFACE.cache ]; then
			rm /etc/config/dhcpcd-$WANINTERFACE.cache
		fi
		
		# Start WAN connection
		$SCRIPT_PATH/wan_start > /dev/null 2>&1
	fi
fi

echo "Done

 </pre>
 </BODY></HTML>"
