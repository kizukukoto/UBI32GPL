#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>Static WAN Configuration</TITLE></HEAD>"
echo "<BODY>"
echo "<pre>"

sysconf_file=/etc/rgw_config

# Stop the current WAN connection before starting in Static WAN mode
/etc/init.d/wan_stop

all_params=$QUERY_STRING

# Get param=value pairs
wan_ip_param=`echo $all_params | grep -o "wan_ip=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
wan_subnet_param=`echo $all_params | grep -o "wan_subnet=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
wan_gateway_param=`echo $all_params | grep -o "wan_gateway=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
dns1_param=`echo $all_params | grep -o "dns1=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
dns2_param=`echo $all_params | grep -o "dns2=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`

# Get value of the param
wan_ip=`echo "$wan_ip_param" | awk -F'=' '{print $2}'`
wan_subnet=`echo "$wan_subnet_param" | awk -F'=' '{print $2}'`
wan_gateway=`echo "$wan_gateway_param" | awk -F'=' '{print $2}'`
dns1=`echo "$dns1_param" | awk -F'=' '{print $2}'`
dns2=`echo "$dns2_param" | awk -F'=' '{print $2}'`

# Write the Static WAN IP specific entries to system config file.
sed -i '/^STATICWANIP=/ c\STATICWANIP='$wan_ip'' $sysconf_file
sed -i '/^STATICWANMASK=/ c\STATICWANMASK='$wan_subnet'' $sysconf_file
sed -i '/^STATICGATEWAY=/ c\STATICGATEWAY='$wan_gateway'' $sysconf_file
sed -i '/^STATICDNS1=/ c\STATICDNS1='$dns1'' $sysconf_file
sed -i '/^STATICDNS2=/ c\STATICDNS2='$dns2'' $sysconf_file
sed -i '/^WANCONNECTIONTYPE=/ c\WANCONNECTIONTYPE='"static"'' $sysconf_file

# Debug outputs
#echo "all_params=$all_params"
#echo "wan_ip=$wan_ip"
#echo "wan_subnet=$wan_subnet"
#echo "wan_gateway=$wan_gateway"
#echo "dns1=$dns1"
#echo "dns2=$dns2"

/etc/init.d/do_static start

echo "Done"

echo "</pre>"
echo "</BODY></HTML>"
