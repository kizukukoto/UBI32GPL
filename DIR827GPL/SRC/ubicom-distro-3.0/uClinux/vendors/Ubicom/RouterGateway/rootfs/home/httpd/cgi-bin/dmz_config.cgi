#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>dmz settings</TITLE></HEAD>
 <BODY>
 <pre>"
#env

sys_conf_file=/etc/rgw_config

# Learn if dmz host enabled param is on, its format is like 'cbx_enable=on'
dmz_host=`echo $QUERY_STRING | grep -o "dmz_host=[-._:%a-zA-Z0-9]*"`
dmz_host_value=`echo "$dmz_host" | awk -F'=' '{print $2}'`

# Check address integrity
dmz_valid=`echo $dmz_host_value | egrep '^0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5]).\0*([0-9]{1,2}|1[0-9]{2}|2[0-4][0-9]|25[0-5])$'`
if [ $dmz_valid =  ]; then
  echo "Invalid IP"
  exit
fi

# Get dmz_enabled from config file
config_dmz_enabled=`grep "^DMZENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
config_dmz_host=`grep "^DMZHOST" $sys_conf_file | awk -F'=' '{print $2}'`

cbx_enable=`echo $QUERY_STRING | grep -o "cbx_enable=[a-z]*"`
cbx_enable_value=`echo "$cbx_enable" | awk -F'=' '{print $2}'`

# Get ntp server from config file
wan_interface=`grep "^WANINTERFACE" $sys_conf_file | awk -F'=' '{print $2}'`

#remove rules
iptables -t nat -D PREROUTING -i $wan_interface -j DNAT --to $dmz_host_value > /dev/null 2>/dev/null
iptables -t nat -D PREROUTING -i $wan_interface -j DROP > /dev/null 2>/dev/null
iptables -D INPUT -i $wan_interface -j ACCEPT > /dev/null 2>/dev/null
if [ $cbx_enable_value = "on" ]; then
	sed -i 's/^DMZHOST=\(.*\)/DMZHOST='$dmz_host_value/'' $sys_conf_file
	sed -i -e 's/^DMZENABLED=\(.*\)/DMZENABLED=YES/' $sys_conf_file

	iptables -t nat -A PREROUTING -i $wan_interface -j DNAT --to $dmz_host_value
	iptables -A INPUT -i $wan_interface -j ACCEPT
else
	sed -i -e 's/^DMZENABLED=\(.*\)/DMZENABLED=NO/' $sys_conf_file

	iptables -t nat -A PREROUTING -i $wan_interface -j DROP
fi

echo "Done

 </pre>
</BODY></HTML>"
