#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>PPTP Configuration</TITLE></HEAD>"
echo "<BODY>"
echo "<pre>"

sysconf_file=/etc/rgw_config

# Stop the current WAN connection before starting in PPTP WAN mode
/etc/init.d/wan_stop

all_params=$QUERY_STRING

# Get param=value pairs
username_param=`echo $all_params | awk -F'&' '{print $2}' | grep -r -o "username=[0-9a-zA-Z].*"`
password_param=`echo $all_params | awk -F'&' '{print $3}' | grep -r -o "password=[0-9a-zA-Z].*"`
pptp_server_ip_param=`echo $all_params | awk -F'&' '{print $4}' | grep -r -o "pptp_server_ip=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
wan_ip_param=`echo $all_params | awk -F'&' '{print $5}' | grep -r -o "wan_ip=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
wan_subnet_param=`echo $all_params | awk -F'&' '{print $6}' | grep -r -o "wan_subnet=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
wan_gateway_param=`echo $all_params | awk -F'&' '{print $7}' | grep -r -o "wan_gateway=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`

# Get value of the param
username=`echo "$username_param" | awk -F'=' '{print $2}'`
password=`echo "$password_param" | awk -F'=' '{print $2}'`
pptp_server_ip=`echo "$pptp_server_ip_param" | awk -F'=' '{print $2}'`
wan_ip=`echo "$wan_ip_param" | awk -F'=' '{print $2}'`
wan_subnet=`echo "$wan_subnet_param" | awk -F'=' '{print $2}'`
wan_gateway=`echo "$wan_gateway_param" | awk -F'=' '{print $2}'`

# Write the PPTP specific entries to system config file.
sed -i '/^PPTPUSERNAME=/ c\PPTPUSERNAME='$username'' $sysconf_file
sed -i '/^PPTPPASSWORD=/ c\PPTPPASSWORD='$password'' $sysconf_file
sed -i '/^PPTPSERVERIP=/ c\PPTPSERVERIP='$pptp_server_ip'' $sysconf_file
sed -i '/^PPTPWANIP=/ c\PPTPWANIP='$wan_ip'' $sysconf_file
sed -i '/^PPTPSUBNETMASK=/ c\PPTPSUBNETMASK='$wan_subnet'' $sysconf_file
sed -i '/^PPTPGATEWAYIP=/ c\PPTPGATEWAYIP='$wan_gateway'' $sysconf_file
sed -i '/^WANCONNECTIONTYPE=/ c\WANCONNECTIONTYPE='"pptp"'' $sysconf_file

# Debug outputs
#echo "all_params=$all_params"
#echo "username=$username"
#echo "password=$password"
#echo "pptp_server_ip=$pptp_server_ip"
#echo "wan_ip=$wan_ip"
#echo "wan_subnet=$wan_subnet"
#echo "wan_gateway=$wan_gateway"

/etc/init.d/do_pptp start

echo "Done"

echo "</pre>"
echo "</BODY></HTML>"
