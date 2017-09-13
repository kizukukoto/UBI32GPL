#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<HTML>
 <HEAD>
 <TITLE>Set DHCP server settings</TITLE>
 </HEAD>
 <BODY>
 <pre>"

# Stop dnsmasq before config file update
/etc/init.d/do_dnsmasq stop > /dev/null

conf_file=/etc/config/dnsmasq.conf

#eval $(echo "$QUERY_STRING" | awk -F'&' '{for(i=1;i<=NF;i++){print $i}}')
#echo "REQUEST_URI=$REQUEST_URI"
#all_params=`echo "$REQUEST_URI" | awk -F'?' '{print $2}`
all_params=$QUERY_STRING

echo "all_params=$all_params"

# Learn if DHCP enabled param is on, its format is like 'cbx_enable=on'
cbx_enable_param=`echo $all_params | grep -o "cbx_enable=[a-z]*"`
echo "cbx_enable_param=$cbx_enable_param"

cbx_enable=`echo "$cbx_enable_param" | awk -F'=' '{print $2}'`

echo "all_params=$all_params"

if [ $cbx_enable = on ]; then

   echo "Enabling DHCP server"

   sed -i 's/^#dhcp-range/dhcp-range/' $conf_file

   # Get param=value pairs
   tf_range_start_param=`echo $all_params | grep -o "tf_range_start=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
   tf_range_end_param=`echo $all_params | grep -o "tf_range_end=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`

   # Get value of the params
   tf_range_start=`echo "$tf_range_start_param" | awk -F'=' '{print $2}'`
   tf_range_end=`echo "$tf_range_end_param" | awk -F'=' '{print $2}'`

   # Get netmask from config file
   conf_netmask=`grep "^dhcp-range" $conf_file | awk -F',' '{print $3}'`

   # Update dhcp server ip range : caution - don't indent the following 3rd line
   sed -i '
   /^dhcp-range=/ c\
dhcp-range='$tf_range_start','$tf_range_end','$conf_netmask',24h
   ' $conf_file

else

   echo "Disabling DHCP server"
   sed -i 's/^dhcp-range/#&/' $conf_file

fi

# Debug outputs
# echo "all_params=$all_params"
# echo "cbx_enable=$cbx_enable"
# echo "tf_start_range=$tf_range_start"
# echo "tf_end_range=$tf_range_end"
# echo "conf_netmask=$conf_netmask"

/etc/init.d/do_dnsmasq start > /dev/null

echo "Done"

echo "</pre>"
echo "</BODY></HTML>"
