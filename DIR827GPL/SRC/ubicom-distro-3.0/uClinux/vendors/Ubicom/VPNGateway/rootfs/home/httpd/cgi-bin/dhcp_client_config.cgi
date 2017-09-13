#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>DHCP WAN Configuration</TITLE></HEAD>"
echo "<BODY>"
echo "<pre>"

sysconf_file=/etc/rgw_config

# Stop the current WAN connection before starting in DHCP WAN mode
/etc/init.d/wan_stop

all_params=$QUERY_STRING

# Learn if DHCP enabled param is on, its format is like 'cbx_enable=on'
cbx_enable_param=`echo $all_params | grep -o "cbx_enable=[a-z]*"`
cbx_enable=`echo "$cbx_enable_param" | awk -F'=' '{print $2}'`

if [ $cbx_enable ]; then
	echo "DNS addresses are obtained statically" >> /var/log/messages
	
	sed -i '/^DHCPUSERSPECIALDNS=/ c\DHCPUSERSPECIALDNS='"YES"'' $sysconf_file
	
	# Get param=value pairs
	dns1_param=`echo $all_params | awk -F'&' '{print $3}' | grep -r -o "dns1=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
	dns2_param=`echo $all_params | awk -F'&' '{print $4}' | grep -r -o "dns2=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`

	# Get value of the params
	dns1=`echo "$dns1_param" | awk -F'=' '{print $2}'`
	dns2=`echo "$dns2_param" | awk -F'=' '{print $2}'`
   
   	# Check the addresses   	
	dns1_check=`echo $dns1 | egrep '^([0-9]){1,3}\.([0-9]){1,3}\.([0-9]){1,3}\.([0-9]){1,3}$'`
	if [ $dns1_check =  ]; then
	  echo "<font style='color: red'>Invalid DNS 1 IP</font>"
	  exit
	fi
   
   	dns2_check=`echo $dns2 | egrep '^([0-9]){1,3}\.([0-9]){1,3}\.([0-9]){1,3}\.([0-9]){1,3}$'`
	if [ $dns2_check =  ]; then
	  echo "<font style='color: red'>Invalid DNS 2 IP</font>"
	  exit
	fi
	
	sed -i '/^DHCPDNS1=/ c\DHCPDNS1='$dns1'' $sysconf_file
	sed -i '/^DHCPDNS2=/ c\DHCPDNS2='$dns2'' $sysconf_file
	sed -i '/^DHCPDNS3=/ c\DHCPDNS3=''' $sysconf_file

else
	echo "DNS addresses will be obtained dynamically" >> /var/log/messages
	
	sed -i '/^DHCPUSERSPECIALDNS=/ c\DHCPUSERSPECIALDNS='"NO"'' $sysconf_file  
fi

sed -i '/^WANCONNECTIONTYPE=/ c\WANCONNECTIONTYPE='"dhcp"'' $sysconf_file

# Debug outputs
#echo "all_params=$all_params"
#echo "cbx_enable=$cbx_enable"
#echo "dns1=$dns1"
#echo "dns2=$dns2"

/etc/init.d/do_dhcp start

echo "Done"

echo "</pre>"
echo "</BODY></HTML>"
