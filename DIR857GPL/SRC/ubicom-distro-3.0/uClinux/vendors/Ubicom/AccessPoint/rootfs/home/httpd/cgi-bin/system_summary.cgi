#!/bin/sh

. /etc/ap_config

echo "Content-type: text/html"
echo ""

echo "<HTML>
 <HEAD>
 <TITLE>System logs</TITLE>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </HEAD>
 <BODY>

 <p>&nbsp;</p>

 <div class='section'>

 <div class='section_head'>
 <h2>System summary</h2>
 <p>See your system settings.</p>
 </div>

 <div class='box'>
 <h3>Product</h3>
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
 </fieldset>
 </div>

 <div class='box'>
 <h3>System Information</h3>
 <fieldset>
  <p>
   <label class='duple'>Hostname:</label>
   $HOSTNAME
  </p>
  <p>
   <label class='duple'>Domain name:</label>
   $DOMAINNAME
  </p>
  <p>
   <label class='duple'>LAN IP:</label>
   $BRIDGEIP
  </p>
  <p>
   <label class='duple'>LAN netmask:</label>
   $BRIDGEMASK
  </p>
  <p>
   <label class='duple'>LAN MAC:</label>
   $LANMAC
  </p>
  <p>
   <label class='duple'>LAN interface:</label>
   $LANINTERFACE
  </p>
  <p>
   <label class='duple'>Bridge interface:</label>
   $BRINTERFACE
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
   <label class='duple'>DST active:</label>
   $DAYLIGHTSAVING
  </p>
  <p>
   <label class='duple'>DST start date:</label>
   $DAYLIGHTSTARTDATE
  </p>
  <p>
   <label class='duple'>DST end date:</label>
   $DAYLIGHTENDDATE
  </p>
  <p>
   <label class='duple'>NTP server:</label>
   $NTPSERVER
  </p>
 </fieldset>
 </div>"

echo " <div class='box'>
 <h3>Wireless settings</h3>
 <fieldset>
  <p>
   <label class='duple'>Wireless enabled:</label>
   $WIRELESSENABLED
  </p>
 </fieldset>
 </div>

 <div class='box'>
 <h3>Web settings</h3>
 <fieldset>
  <p>
   <label class='duple'>Web server:</label>
   $WEBSERVER
  </p>
 </fieldset>
 </div>

 <p>&nbsp;</p>

 </div>

 </BODY></HTML>"
