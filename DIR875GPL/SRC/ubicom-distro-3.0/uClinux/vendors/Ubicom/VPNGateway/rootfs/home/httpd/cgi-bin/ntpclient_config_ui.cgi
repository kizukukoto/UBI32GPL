#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>Time Setting</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

#conf_file=/etc/ntp/ntpclient.conf
sys_conf_file=/etc/rgw_config
time_zones_file=/etc/time_zones

# Get ntp_enabled from config file
config_ntp_enabled=`grep "^NTPENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
if [ $config_ntp_enabled = 'YES' ]; then
   ntp_enabled="checked='checked'"
fi

# Get ntp server from config file
config_ntp_server=`grep "^NTPSERVER" $sys_conf_file | awk -F'=' '{print $2}'`
#Get time zone index
config_tz_index=`grep "^TIMEZONE_INX" $sys_conf_file | awk -F'=' '{print $2}'`
#Get time zone friendly name
config_tz_friendly=`grep "^$config_tz_index" $time_zones_file | awk -F'=' '{print $2}'`

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/ntpclient_config.cgi'>
 <div class='section'>"

echo \
 "<div class='section_head'>
	 <h2>Time Configuration</h2>
 </div>"

echo \
 "<div class='section_head'>
	 <p>Enable/Disable time synchronization from Internet during boot</p>
	 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 </div>"

echo \
 "<div class='box'>
	 <h3>Enable/Disable</h3>
	<fieldset>
		<p>
			<label class='duple'>Enable</label>
			<input name='cbx_enable' type='checkbox' $ntp_enabled' />
		</p>
	</fieldset>
 </div>"

echo \
 "<div class='box'>
 <h3>Settings</h3>
  <fieldset>"



echo \
   "<label class='duple'>Current Time Zone:</label>
    <label>$config_tz_friendly</label>
    <br>"

echo \
   "<label class='duple'>Select Time Zone:</label>
    <select name="time_zone">"

echo \
	"<option selected="selected" value="ZONE00">Keep Current Time Zone</option>
	<option value="ZONE01">(GMT-12:00) International Date Line West</option>
	<option value="ZONE02">(GMT-11:00) Midway Island, Samoa</option>
	<option value="ZONE03">(GMT-10:00) Hawaii</option>
	<option value="ZONE04">(GMT-09:00) Alaska</option>
	<option value="ZONE05">(GMT-08:00) Pacific Time (US &amp; Canada); Tijuana</option>"
echo \
	"<option value="ZONE06">(GMT-07:00) Arizona</option>
	<option value="ZONE07">(GMT-07:00) Chihuahua, La Paz, Mazatlan</option>
	<option value="ZONE08">(GMT-07:00) Mountain Time (US &amp; Canada)</option>
	<option value="ZONE09">(GMT-06:00) Central America</option>
	<option value="ZONE10">(GMT-06:00) Central Time (US &amp; Canada)</option>"

echo \
	"<option value="ZONE11">(GMT-06:00) Guadalajara, Mexico City, Monterrey</option>
	<option value="ZONE12">(GMT-06:00) Saskatchewan</option>
	<option value="ZONE13">(GMT-05:00) Bogota, Lima, Quito</option>
	<option value="ZONE14">(GMT-05:00) Eastern Time (US &amp; Canada)</option>
	<option value="ZONE15">(GMT-05:00) Indiana (East)</option>"

echo \
	"<option value="ZONE16">(GMT-04:00) Atlantic Time (Canada)</option>
	<option value="ZONE17">(GMT-04:00) Caracas, La Paz</option>
	<option value="ZONE18">(GMT-04:00) Santiago</option>
	<option value="ZONE19">(GMT-03:00) Newfoundland</option>
	<option value="ZONE20">(GMT-03:00) Brasilia</option>
	<option value="ZONE21">(GMT-03:00) Buenos Aires, Georgetown</option>"

echo \
	"<option value="ZONE22">(GMT-03:00) Greenland</option>
	<option value="ZONE23">(GMT-02:00) Mid-Atlantic</option>
	<option value="ZONE24">(GMT-01:00) Azores</option>
	<option value="ZONE25">(GMT-01:00) Cape Verde Is.</option>
	<option value="ZONE26">(GMT) Casablanca, Monrovia</option>
	<option value="ZONE27">(GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London</option>"

echo \
	"<option value="ZONE28">(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna</option>
	<option value="ZONE29">(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague</option>
	<option value="ZONE30">(GMT+01:00) Brussels, Copenhagen, Madrid, Paris</option>
	<option value="ZONE31">(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb</option>
	<option value="ZONE32">(GMT+01:00) West Central Africa</option>
	<option value="ZONE33">(GMT+02:00) Athens, Beirut, Istanbul, Minsk</option>"

echo \
	"<option value="ZONE34">(GMT+02:00) Bucharest</option>
	<option value="ZONE35">(GMT+02:00) Cairo</option>
	<option value="ZONE36">(GMT+02:00) Harare, Pretoria</option>
	<option value="ZONE37">(GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius</option>
	<option value="ZONE38">(GMT+02:00) Jerusalem</option>
	<option value="ZONE39">(GMT+03:00) Baghdad</option>"

echo \
	"<option value="ZONE40">(GMT+03:00) Kuwait, Riyadh</option>
	<option value="ZONE41">(GMT+03:00) Moscow, St. Petersburg, Volgograd</option>
	<option value="ZONE42">(GMT+03:00) Nairobi</option>
	<option value="ZONE43">(GMT+03:00) Tehran</option>
	<option value="ZONE44">(GMT+04:00) Abu Dhabi, Muscat</option>
	<option value="ZONE45">(GMT+04:00) Baku, Tbilisi, Yerevan</option>"

echo \
	"<option value="ZONE46">(GMT+04:00) Kabul</option>
	<option value="ZONE47">(GMT+05:00) Ekaterinburg</option>
	<option value="ZONE48">(GMT+05:00) Islamabad, Karachi, Tashkent</option>
	<option value="ZONE49">(GMT+05:00) Chennai, Kolkata, Mumbai, New Delhi</option>
	<option value="ZONE50">(GMT+05:00) Kathmandu</option>
	<option value="ZONE51">(GMT+06:00) Almaty, Novosibirsk</option>"

echo \
	"<option value="ZONE52">(GMT+06:00) Astana, Dhaka</option>
	<option value="ZONE53">(GMT+06:00) Sri Jayawardenepura</option>
	<option value="ZONE54">(GMT+06:00) Rangoon</option>
	<option value="ZONE55">(GMT+07:00) Bangkok, Hanoi, Jakarta</option>
	<option value="ZONE56">(GMT+07:00) Krasnoyarsk</option>
	<option value="ZONE57">(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi</option>"

echo \
	"<option value="ZONE58">(GMT+08:00) Irkutsk, Ulaan Bataar</option>
	<option value="ZONE59">(GMT+08:00) Kuala Lumpur, Singapore</option>
	<option value="ZONE60">(GMT+08:00) Perth</option>
	<option value="ZONE61">(GMT+08:00) Taipei</option>
	<option value="ZONE62">(GMT+09:00) Osaka, Sapporo, Tokyo</option>
	<option value="ZONE63">(GMT+09:00) Seoul</option>"

echo \
	"<option value="ZONE64">(GMT+09:00) Yakutsk</option>
	<option value="ZONE65">(GMT+09:00) Adelaide</option>
	<option value="ZONE66">(GMT+09:00) Darwin</option>
	<option value="ZONE67">(GMT+10:00) Brisbane</option>
	<option value="ZONE68">(GMT+10:00) Canberra, Melbourne, Sydney</option>
	<option value="ZONE69">(GMT+10:00) Guam, Port Moresby</option>"

echo \
	"<option value="ZONE70">(GMT+10:00) Hobart</option>
	<option value="ZONE71">(GMT+10:00) Vladivostok</option>
	<option value="ZONE72">(GMT+11:00) Magadan, Solomon Ls., New Caledonia</option>
	<option value="ZONE73">(GMT+12:00) Auckland, Wellington</option>
	<option value="ZONE74">(GMT+12:00) Fiji, Kamchatka, Marshall ls.</option>
	<option value="ZONE75">(GMT+13:00) Nuku'alofa</option>
    </select>

    <p>
	    <label class='duple'>Time Server:</label>
	    <input type='text' name='time_server' value='$config_ntp_server'>
    </p>

  </fieldset>
 </div>"

echo \
 "<p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"

