#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<HTML>
 <HEAD>
 <TITLE>Network setup</TITLE>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </HEAD>
 <BODY>"


sysconf_file=/etc/rgw_config
conf_file=/etc/config/dnsmasq.conf
helper_subnet=/etc/init.d/helper/subnet.awk

all_params=$QUERY_STRING
# echo "all_params=$all_params"

# Get param=value pairs
lan_ip_param=`echo $all_params | awk -F'&' '{print $2}'`
lan_mask_param=`echo $all_params | awk -F'&' '{print $3}'`

# Get value of the param
lan_ip=`echo "$lan_ip_param" | awk -F'=' '{print $2}'`
lan_mask=`echo "$lan_mask_param" | awk -F'=' '{print $2}'`

# Check address integrity
lan_ip_check=`echo $lan_ip | egrep '^0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([1-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-4])$'`
if [ $lan_ip_check =  ]; then
  echo "<font style='color: red'>Invalid device IP</font>"
  exit
fi
lan_mask_check=`echo $lan_mask | egrep '^(255|254|252|248|240|224|192|128|0)\.(255|254|252|248|240|224|192|128|0)\.(255|254|252|248|240|224|192|128|0)\.(255|254|252|248|240|224|192|128|0)$'`
if [ $lan_mask_check =  ]; then
  echo "<font style='color: red'>Invalid subnet mask</font>"
  exit
fi

# Get previous lan ip
prev_lan_ip=`grep '^BRIDGEIP=' $sysconf_file | awk -F'=' '{print $2}'`
prev_lan_mask=`grep '^BRIDGEMASK=' $sysconf_file | awk -F'=' '{print $2}'`
lan_interface=`grep '^BRINTERFACE=' $sysconf_file | awk -F'=' '{print $2}'`

# Check address validity
range_info=`grep "^dhcp-range" $conf_file | awk -F'=' '{print $2}'`
range_start=`echo "$range_info" | awk -F',' '{print $1}'`
range_end=`echo "$range_info" | awk -F',' '{print $2}'`
tuned_range_start=`awk -v args=ti -v ip=$lan_ip -v mask=$lan_mask -v prev_mask=$prev_lan_mask -v is_start=y -v tuned_ip=$range_start -f $helper_subnet`
tuned_range_end=`awk -v args=ti -v ip=$lan_ip -v mask=$lan_mask -v prev_mask=$prev_lan_mask -v is_start=n -v tuned_ip=$range_end -f $helper_subnet`

sed -i '
/^BRIDGEIP=/ c\
BRIDGEIP='$lan_ip'
' $sysconf_file

sed -i '
/^BRIDGEMASK=/ c\
BRIDGEMASK='$lan_mask'
' $sysconf_file

# Update listen-address and netmask in dnsmasq.conf file

   dhcp_enabled=`grep "^dhcp-range" $conf_file`
if [ $dhcp_enabled != '' ]; then
   /etc/init.d/do_dnsmasq stop > /dev/null
#   echo "LAN IP changed<br/>"
   sed -i '
/^listen-address='$prev_lan_ip'/ c\
listen-address='$lan_ip'
' $conf_file
   sed -i 's/\(dhcp-range=\)\([0-9\.]*,\)\([0-9\.]*,\)\([0-9\.]*\)\(.*\)/\1'$tuned_range_start','$tuned_range_end','$lan_mask'\5/' $conf_file
   /etc/init.d/do_dnsmasq start >> /dev/null
#   echo "DHCP server restarted<br/>"
else
#   ifconfig $lan_interface $lan_ip netmask $lan_mask up
#   echo "LAN IP changed<br/>"
   sed -i '/
^listen-address='$prev_lan_ip'/ c\
listen-address='$lan_ip'
' $conf_file
   sed -i 's/\(#dhcp-range=\)\([0-9\.]*,\)\([0-9\.]*,\)\([0-9\.]*\)\(.*\)/\1'$tuned_range_start','$tuned_range_end','$lan_mask'\5/' $conf_file
fi

# Restart miniupnpd (UPnP Internet Gateway Device)
upnp_enabled=`grep "^UPNPENABLED" $sysconf_file | awk -F'=' '{print $2}'`
if [ "$upnp_enabled" = "YES" ]; then
	if [ -f /etc/init.d/do_miniupnpd ]; then
	       /etc/init.d/do_miniupnpd restart > /dev/null &
	fi
fi

# Debug outputs
# cat $conf_file
# echo "lan_ip=$lan_ip"
# echo "lan_mask=$lan_mask"
# echo "prev_lan_ip=$prev_lan_ip"

echo "Done - <a href='http://$lan_ip' target='_top'>http://$lan_ip</a>

 </BODY></HTML>"

ifconfig $lan_interface $lan_ip netmask $lan_mask up
