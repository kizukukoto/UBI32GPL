#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<HTML>
 <HEAD>
 <TITLE>Set DHCP server settings</TITLE>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </HEAD>
 <BODY>"

# Stop dnsmasq before config file update
/etc/init.d/do_dnsmasq stop > /dev/null

conf_file=/etc/config/dnsmasq.conf
sysconf_file=/etc/rgw_config
helper_subnet=/etc/init.d/helper/subnet.awk

#eval $(echo "$QUERY_STRING" | awk -F'&' '{for(i=1;i<=NF;i++){print $i}}')
#echo "REQUEST_URI=$REQUEST_URI"
#all_params=`echo "$REQUEST_URI" | awk -F'?' '{print $2}`
all_params=$QUERY_STRING

#echo "all_params=$all_params"

# Learn if DHCP enabled param is on, its format is like 'cbx_enable=on'
cbx_enable_param=`echo $all_params | grep -o "cbx_enable=[a-z]*"`
#echo "cbx_enable_param=$cbx_enable_param"

cbx_enable=`echo "$cbx_enable_param" | awk -F'=' '{print $2}'`

#echo "all_params=$all_params"

if [ $cbx_enable = on ]; then

#   echo "Enabling DHCP server"

   # Get param=value pairs
   tf_range_start_param=`echo $all_params | grep -o "tf_range_start=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
   tf_range_end_param=`echo $all_params | grep -o "tf_range_end=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`

   # Get value of the params
   tf_range_start=`echo "$tf_range_start_param" | awk -F'=' '{print $2}'`
   tf_range_end=`echo "$tf_range_end_param" | awk -F'=' '{print $2}'`

   # Check address integrity
   start_check=`echo $tf_range_start | egrep '^0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5])$'`
   if [ $start_check =  ]; then
     echo "<font style='color: red'>Invalid range start</font>"
     exit
   fi
   end_check=`echo $tf_range_end | egrep '^0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5])$'`
   if [ $end_check =  ]; then
     echo "<font style='color: red'>Invalid range end</font>"
     exit
   fi

   # Check range validity
   lan_ip=`grep "^BRIDGEIP" $sysconf_file | awk -F'=' '{print $2}'`
   lan_mask=`grep "^BRIDGEMASK" $sysconf_file | awk -F'=' '{print $2}'`
   range_check=`awk -v args=rc -v ip=$lan_ip -v mask=$lan_mask -v sta_ip=$tf_range_start -v end_ip=$tf_range_end -f $helper_subnet`
   if [ $range_check = 1 ]; then
     echo "<font style='color: red'>Range start and end must be in the same subnet with device</font>"
     exit
   fi
   if [ $range_check = 2 ]; then
     echo "<font style='color: red'>Range start must be smaller than range end</font>"
     exit
   fi
   if [ $range_check = 3 ]; then
     echo "<font style='color: red'>Range cannot include device ip</font>"
     exit
   fi

   # Get netmask from config file
   conf_netmask=`grep "^dhcp-range" $conf_file | awk -F',' '{print $3}'`

   sed -i 's/^#dhcp-range/dhcp-range/' $conf_file

   # Update dhcp server ip range : caution - don't indent the following 3rd line
   sed -i '
   /^dhcp-range=/ c\
dhcp-range='$tf_range_start','$tf_range_end','$conf_netmask',24h
   ' $conf_file

else

#   echo "Disabling DHCP server"
   sed -i 's/^dhcp-range/#&/' $conf_file

fi

# Debug outputs
# echo "all_params=$all_params"
# echo "cbx_enable=$cbx_enable"
# echo "tf_start_range=$tf_range_start"
# echo "tf_end_range=$tf_range_end"
# echo "conf_netmask=$conf_netmask"

/etc/init.d/do_dnsmasq start > /dev/null

echo "Done

 </BODY></HTML>"
