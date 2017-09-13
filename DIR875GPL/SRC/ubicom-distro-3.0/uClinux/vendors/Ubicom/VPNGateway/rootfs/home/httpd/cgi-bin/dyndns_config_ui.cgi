#!/bin/sh

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>Time Setting</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

sys_conf_file=/etc/rgw_config

# Get inadyn_enabled from config file
config_inadyn_enabled=`grep "^INADYNENABLED" $sys_conf_file | awk -F'=' '{print $2}'`
if [ $config_inadyn_enabled = 'YES' ]; then
   inadyn_enabled="checked='checked'"
fi

# Get inadyn parameter from config file
config_inadyn_user=`grep "^INADYNUSERNAME" $sys_conf_file | awk -F'=' '{print $2}'`
config_inadyn_pass=`grep "^INADYNPASSWD" $sys_conf_file | awk -F'=' '{print $2}'`
config_inadyn_alias=`grep "^INADYNALIAS" $sys_conf_file | awk -F'=' '{print $2}'`

echo "<body>

 <p>&nbsp;</p>

 <form name='form1' method='get' action='/cgi-bin/dyndns_config.cgi'>
 <div class='section'>"

echo \
 "<div class='section_head'>
	 <h2>Inadyn Dynamic DNS Setting</h2>
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
			<input name='cbx_enable' type='checkbox' $inadyn_enabled;' />
		</p>
	</fieldset>
 </div>"

echo \
"<div class='box'>
	<h3>Settings</h3>
	<fieldset>
		<p>
			<label class='duple'>Username:</label>
			<input type='text' name='inadyn_user' value='$config_inadyn_user'>
		</p>
		<p>
			<label class='duple'>Password:</label>
			<input type='password' name='inadyn_pass' value='$config_inadyn_pass'>
		</p>

		<p>
			<label class='duple'>Host alias:</label>
			<input type='text' name='inadyn_host' value='$config_inadyn_alias'>
		</p>
	</fieldset>
</div>"


echo \
 "<p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"

