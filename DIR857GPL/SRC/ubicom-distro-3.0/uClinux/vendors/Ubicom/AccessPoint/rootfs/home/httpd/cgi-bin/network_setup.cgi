#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<HTML><HEAD><TITLE>Network setup</TITLE></HEAD>
 <BODY>
 <pre>"

sysconf_file=/etc/ap_config
conf_file=/etc/config/dnsmasq.conf

all_params=$QUERY_STRING
echo "all_params=$all_params"

# Get param=value pairs
lan_ip_param=`echo $all_params | grep -o "tf_lan_ip=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`
lan_mask_param=`echo $all_params | grep -o "tf_lan_mask=[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*"`

# Get value of the param
lan_ip=`echo "$lan_ip_param" | awk -F'=' '{print $2}'`
lan_mask=`echo "$lan_mask_param" | awk -F'=' '{print $2}'`

# Get previous lan ip
prev_lan_ip=`grep '^BRIDGEIP=' $sysconf_file | awk -F'=' '{print $2}'`
lan_interface=`grep '^LANINTERFACE=' $sysconf_file | awk -F'=' '{print $2}'`

sed -i '
/^BRIDGEIP=/ c\
BRIDGEIP='$lan_ip'
' $sysconf_file

sed -i '
/^BRIDGEMASK=/ c\
BRIDGEMASK='$lan_mask'
' $sysconf_file

# Update netmask and listen-address in dnsmasq.conf file

   dhcp_enabled=`grep "^dhcp-range" $conf_file`
if [ $dhcp_enabled != '' ]; then
   /etc/init.d/do_dnsmasq stop > /dev/null
   ifconfig $lan_interface $lan_ip netmask $lan_mask up
   sed -i 's/\(dhcp-range=\)\([0-9\.]*,\)\([0-9\.]*,\)\([0-9\.]*\)\(.*\)/\1\2\3'$lan_mask'\5/' $conf_file
   /etc/init.d/do_dnsmasq start > /dev/null
   echo "DHCP server restarted"
else
   ifconfig $lan_interface $lan_ip netmask $lan_mask up
   sed -i 's/\(#dhcp-range=\)\([0-9\.]*,\)\([0-9\.]*,\)\([0-9\.]*\)\(.*\)/\1\2\3'$lan_mask'\5/' $conf_file
fi

sed -i '
/^listen-address='$prev_lan_ip'/ c\
listen-address='$lan_ip'
' $conf_file

# Debug outputs
# cat $conf_file
# echo "lan_ip=$lan_ip"
# echo "lan_mask=$lan_mask"
# echo "prev_lan_ip=$prev_lan_ip"

echo "Done

 </pre>
 </BODY></HTML>"
