#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML><HEAD><TITLE>dmz settings</TITLE></HEAD>
 <BODY>
 <pre>"
#env

sys_conf_file=/etc/rgw_config

# get inadyn user 
inadyn_user=`echo $QUERY_STRING | grep -o "inadyn_user=[-._:%a-zA-Z0-9]*"`
inadyn_user_value=`echo "$inadyn_user" | awk -F'=' '{print $2}'`

# get inadyn pass
inadyn_pass=`echo $QUERY_STRING | grep -o "inadyn_pass=[-._:%a-zA-Z0-9]*"`
inadyn_pass_value=`echo "$inadyn_pass" | awk -F'=' '{print $2}'`

# get inadyn host
inadyn_host=`echo $QUERY_STRING | grep -o "inadyn_host=[-._:%a-zA-Z0-9]*"`
inadyn_host_value=`echo "$inadyn_host" | awk -F'=' '{print $2}'`


# Check host alias integrity
inadyn_host_valid=`echo $inadyn_host_value | egrep '[a-zA-Z0-9\-\.]+\.[a-zA-Z]{2,3}(/\S*)?$'`
if [ $inadyn_host_valid =  ]; then
  echo "Invalid host"
  exit
fi

cbx_enable=`echo $QUERY_STRING | grep -o "cbx_enable=[a-z]*"`
cbx_enable_value=`echo "$cbx_enable" | awk -F'=' '{print $2}'`

if [ $cbx_enable_value = "on" ]; then
	sed -i 's/^INADYNUSERNAME=\(.*\)/INADYNUSERNAME='$inadyn_user_value/'' $sys_conf_file
	sed -i 's/^INADYNPASSWD=\(.*\)/INADYNPASSWD='$inadyn_pass_value/'' $sys_conf_file
	sed -i 's/^INADYNALIAS=\(.*\)/INADYNALIAS='$inadyn_host_value/'' $sys_conf_file
	sed -i -e 's/^INADYNENABLED=\(.*\)/INADYNENABLED=YES/' $sys_conf_file
else
	sed -i -e 's/^INADYNENABLED=\(.*\)/INADYNENABLED=NO/' $sys_conf_file
	#check if rules already added and remove them.
fi

echo "Done

 </pre>
</BODY></HTML>"
