#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>PPPoE Configuration</TITLE></HEAD>"
echo "<BODY>"
echo "<pre>"

sysconf_file=/etc/rgw_config

# Stop the current WAN connection before starting in PPPoE WAN mode
/etc/init.d/wan_stop

all_params=$QUERY_STRING


# Get param=value pairs
username_param=`echo $all_params | awk -F'&' '{print $2}' | grep -r -o "username=[0-9a-zA-Z].*"`
password_param=`echo $all_params | awk -F'&' '{print $3}' | grep -r -o "password=[0-9a-zA-Z].*"`

# Get value of the param
username=`echo "$username_param" | awk -F'=' '{print $2}'`
password=`echo "$password_param" | awk -F'=' '{print $2}'`

# Write username, password and WAN Connection type to system config file.
sed -i '/^PPPOEUSERNAME=/ c\PPPOEUSERNAME='$username'' $sysconf_file
sed -i '/^PPPOEPASSWORD=/ c\PPPOEPASSWORD='$password'' $sysconf_file
sed -i '/^WANCONNECTIONTYPE=/ c\WANCONNECTIONTYPE='"pppoe"'' $sysconf_file

# Debug outputs
#echo "all_params=$all_params"
#echo "username=$username"
#echo "password=$password"

/etc/init.d/do_pppoe start

echo "Done"

echo "</pre>"
echo "</BODY></HTML>"
