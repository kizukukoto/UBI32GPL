#!/bin/sh

. /etc/rgw_config
. /etc/ath/wpa2-psk.conf
. /etc/ath/apcfg

echo "Content-type: text/html"
echo ""

# Read from other data sources in addition to rgw_config
lan_mac=`ifconfig $BRINTERFACE | grep -o "HWaddr .*" | awk '{ print $2 }'`
date=`date`

echo "<HTML>
 <HEAD>
 <TITLE>System Logs</TITLE>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </HEAD>
 <BODY>

 <p>&nbsp;</p>

 <div class='section'>

 <div class='section_head'>
 <h2>System Summary</h2>
 </div>

 <div class='box'>
 <h3>System Information</h3>
 <fieldset>
  <p>
   <label class='duple'>Version:</label>
   $VERSION
  </p>
  <p>
   <label class='duple'>Model:</label>
   $MODEL
  </p>
  <p>
   <label class='duple'>Serial no:</label>
   $SERIALNO
  </p>
  <p>
   <label class='duple'>Host Name:</label>
   $HOSTNAME
  </p>
  <p>
   <label class='duple'>Domain Name:</label>
   $DOMAINNAME
  </p>
   <p>
   <label class='duple'>Current Date and Time:</label>
   $date
  </p>
 </fieldset>
 </div>

 <div class='box'>
 <h3>LAN Settings</h3>
 <fieldset>
  <p>
   <label class='duple'>LAN IP:</label>
   $BRIDGEIP
  </p>
  <p>
   <label class='duple'>LAN Netmask:</label>
   $BRIDGEMASK
  </p>
  <p>
   <label class='duple'>LAN MAC:</label>
   $lan_mac
  </p>
 </fieldset>
 </div>"

echo "<div class='box'>
 <h3>WAN Settings</h3>
 <fieldset>
  <p>
   <label class='duple'>Connection Type:</label>
   $WANCONNECTIONTYPE
  </p>
  
  <p>
   <label class='duple'>WAN Interface:</label>
   $WANINTERFACE
  </p>"
  
if [ "$WANCONNECTIONTYPE" = "pppoe" ] || [ "$WANCONNECTIONTYPE" = "pptp" ]; then

echo  "<p>
   <label class='duple'>PPP Interface:</label>
   $PPPINTERFACE
  </p>

  <p>
   <label class='duple'>PPP Local IP:</label>
   $PPPLOCALIP
  </p>
	
  <p>
   <label class='duple'>PPP Remote IP:</label>
   $PPPREMOTEIP
  </p>"
  
  	if [ "$WANCONNECTIONTYPE" = "pptp" ]; then
  	
	echo  	"<p>
		  <label class='duple'>PPTP Server IP:</label>
		   $PPTPSERVERIP
		  </p>
		  <p>
		  <label class='duple'>PPTP WAN IP:</label>
		   $PPTPWANIP
		  </p>

		  <p>
		   <label class='duple'>PPTP WAN Subnet Mask:</label>
		   $PPTPSUBNETMASK
		  </p>
	
		  <p>
		   <label class='duple'>PPTP Gateway IP:</label>
		   $PPTPGATEWAYIP
		  </p>"		  	
  	fi
echo  "<p>
   <label class='duple'>User Special DNS:</label>
   $PPPUSERSPECIALDNS
  </p>
  <p>
   <label class='duple'>DNS1:</label>
   $PPPDNS1
  </p>
  <p>
   <label class='duple'>DNS2:</label>
   $PPPDNS2
  </p>
  <p>
   <label class='duple'>DNS3:</label>
   $PPPDNS3
  </p>"

elif [ "$WANCONNECTIONTYPE" = "dhcp" ]; then

echo  "<p>
   <label class='duple'>WAN IP:</label>
   $DHCPWANIP
  </p>
  <p>
   <label class='duple'>WAN Netmask:</label>
   $DHCPWANMASK
  </p>
  <p>
   <label class='duple'>Gateway:</label>
   $DHCPGATEWAY
  </p>
  <p>
   <label class='duple'>User Special DNS:</label>
   $DHCPUSERSPECIALDNS
  </p>
  <p>
   <label class='duple'>DNS1:</label>
   $DHCPDNS1
  </p>
  <p>
   <label class='duple'>DNS2:</label>
   $DHCPDNS2
  </p>
  <p>
   <label class='duple'>DNS3:</label>
   $DHCPDNS3
  </p>"
elif [ "$WANCONNECTIONTYPE" = "static" ]; then

echo  "<p>
   <label class='duple'>WAN IP:</label>
   $STATICWANIP
  </p>
  <p>
   <label class='duple'>WAN Netmask:</label>
   $STATICWANMASK
  </p>
  <p>
   <label class='duple'>Gateway:</label>
   $STATICGATEWAY
  </p>
  <p>
   <label class='duple'>User Special DNS:</label>
   $STATICUSERSPECIALDNS
  </p>
  <p>
   <label class='duple'>DNS1:</label>
   $STATICDNS1
  </p>
  <p>
   <label class='duple'>DNS2:</label>
   $STATICDNS2
  </p>"
fi

echo  "<p>
   <label class='duple'>WAN MAC:</label>
   $WANMAC
  </p>
  <p>
   <label class='duple'>Auto MTU:</label>
   $AUTOMTU
  </p>
  <p>
   <label class='duple'>MTU:</label>
   $MTU
  </p>
<!--  
  <p>
   <label class='duple'>WAN Connection:</label>
   $WANCONNECTION
  </p>
  <p>
   <label class='duple'>DMZ host:</label>
   $DMZHOST
  </p>
-->
 </fieldset>
 </div>"

echo " <div class='box'>
 <h3>Wireless settings</h3>
 <fieldset>
  <p>
   <label class='duple'>Wireless Enabled:</label>
   $WIRELESSENABLED
  </p>
   <p>
   <label class='duple'>SSID:</label>
   $AP_SSID
  </p>
  <p>
   <label class='duple'>Security:</label>
   $AP_SECMODE
  </p>
 </fieldset>
 </div>

  <div class='box'>
 <h3>NTP settings</h3>
 <fieldset>
  <p>
   <label class='duple'>Enabled:</label>
   $NTPENABLED
  </p>
  <p>
   <label class='duple'>Timezone:</label>
   $TIMEZONE
  </p>
  <p>
   <label class='duple'>DST Active:</label>
   $DAYLIGHTSAVING
  </p>
  <p>
   <label class='duple'>DST Start Date:</label>
   $DAYLIGHTSTARTDATE
  </p>
  <p>
   <label class='duple'>DST End Date:</label>
   $DAYLIGHTENDDATE
  </p>
  <p>
   <label class='duple'>NTP Server:</label>
   $NTPSERVER
  </p>
 </fieldset>
 </div>

 <div class='box'>
 <h3>UPnP settings</h3>
 <fieldset>
  <p>
   <label class='duple'>UPnP Enabled:</label>
   $UPNPENABLED
  </p>
 </fieldset>
 </div>

 <div class='box'>
 <h3>Web settings</h3>
 <fieldset>
  <p>
   <label class='duple'>Web Server:</label>
   $WEBSERVER
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>

 </BODY></HTML>"
