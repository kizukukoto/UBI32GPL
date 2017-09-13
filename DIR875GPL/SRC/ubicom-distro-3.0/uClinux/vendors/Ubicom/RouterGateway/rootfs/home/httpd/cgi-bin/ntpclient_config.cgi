#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>Set ntp settings</TITLE></HEAD>
 <BODY>
 <pre>"
#env

sys_conf_file=/etc/rgw_config
time_zones_file=/etc/time_zones

# Get ntp_enabled from config file
config_ntp_enabled=`grep "^NTPENABLED" $sys_conf_file | awk -F'=' '{print $2}'`

# Learn if ntp enabled param is on, its format is like 'cbx_enable=on'
time_zone=`echo $QUERY_STRING | grep -o "time_zone=[-._:%a-zA-Z0-9]*"`
time_zone_inx=`echo "$time_zone" | awk -F'=' '{print $2}'`
time_zone_code=`grep "^$time_zone_inx" $time_zones_file | awk -F'=' '{print $3}'`
#time_zone_friendly=`grep "^$time_zone_inx" $time_zones_file | awk -F'=' '{print $2}'`

time_server=`echo $QUERY_STRING | grep -o "time_server=[-._:%a-zA-Z0-9]*"`
time_server_value=`echo "$time_server" | awk -F'=' '{print $2}'`

# time server validity
time_server_valid=`echo $time_server_value | egrep '[a-zA-Z0-9\-\.]+\.[a-zA-Z]{2,3}(/\S*)?$'`
if [ $time_server_valid =  ]; then
  echo "Invalid server addres"
  exit
fi

cbx_enable=`echo $QUERY_STRING | grep -o "cbx_enable=[a-z]*"`
cbx_enable_value=`echo "$cbx_enable" | awk -F'=' '{print $2}'`

if [ $cbx_enable_value = "on" ]; then
	sed -i 's/^NTPSERVER=\(.*\)/NTPSERVER='$time_server_value/'' $sys_conf_file
	sed -i -e 's/^NTPENABLED=\(.*\)/NTPENABLED=YES/' $sys_conf_file
	if [ ! $time_zone_inx = "ZONE00" ]; then
		sed -i 's/^TIMEZONE=\(.*\)/TIMEZONE='$time_zone_code/'' $sys_conf_file
		sed -i 's/^TIMEZONE_INX=\(.*\)/TIMEZONE_INX='$time_zone_inx/'' $sys_conf_file
		echo $time_zone_code > /etc/TZ
		echo "Getting time"
		ntpclient -s -h $NTPSERVER
	fi
else
	sed -i -e 's/^NTPENABLED=\(.*\)/NTPENABLED=NO/' $sys_conf_file
fi

echo "Done

 </pre>
</BODY></HTML>"
