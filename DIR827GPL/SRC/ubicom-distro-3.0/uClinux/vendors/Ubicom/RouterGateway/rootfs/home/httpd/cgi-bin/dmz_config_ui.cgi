#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>Time Setting</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

sys_conf_file=/etc/rgw_config

# Get dmz_enabled from config file
config_dmz_enabled=`grep "^DMZENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
if [ $config_dmz_enabled = 'YES' ]; then
   dmz_enabled="checked='checked'"
fi

# Get dmz host server from config file
config_dmz_host=`grep "^DMZHOST" $sys_conf_file | awk -F'=' '{print $2}'`

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/dmz_config.cgi'>
 <div class='section'>"

echo \
 "<div class='section_head'>
	 <h2>DMZ Host Setting</h2>
 </div>"

echo \
 "<div class='section_head'>
	 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 </div>"

echo \
 "<div class='box'>
	 <h3>Enable/Disable</h3>
	<fieldset>
		<p>
			<label class='duple'>Enable</label>
			<input name='cbx_enable' type='checkbox' $dmz_enabled ' />
		</p>
	</fieldset>
 </div>"

echo \
"<div class='box'>
	<h3>Settings</h3>
	<fieldset>
		<p>
			<label class='duple'>DMZ Host:</label>
			<input type='text' name='dmz_host' value='$config_dmz_host'>
		</p>
	</fieldset>
</div>"

echo \
 "<p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"

