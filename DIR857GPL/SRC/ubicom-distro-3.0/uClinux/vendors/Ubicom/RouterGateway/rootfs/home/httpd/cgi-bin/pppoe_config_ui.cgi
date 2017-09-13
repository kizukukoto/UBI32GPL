#!/bin/sh

sysconf_file=/etc/rgw_config

echo "Content-type: text/html"
echo ""

echo "<html>
 <head>
 <title>PPPoE Configuration</title>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </head>"

username=`grep "^PPPOEUSERNAME" $sysconf_file | awk -F'=' '{print $2}'`
password=`grep "^PPPOEPASSWORD" $sysconf_file | awk -F'=' '{print $2}'`

echo "<body>

 <p>&nbsp;</p>
 <form name='form1' method='get' action='/cgi-bin/pppoe_config.cgi' target='result'>

 <div class='section'>
 <div class='section_head'>
 <h2>PPPoE Configuration</h2>
 <p>Save PPPoE Settings.</p>
 <input class='button_submit' type='submit' name='Save_Apply' value='Save/Apply'>
 <iframe name='result' width='50%' height='30' frameborder='0' scrolling='no'></iframe>
 </div>

 <div class='box'>
 <h3>Username/Password</h3>
  <fieldset>
   <p>
    <label class='duple'>User Name</label>
    <input name='username' type='text' id='username' value='$username'>
   </p>
 
   <p>
    <label class='duple'>Password</label>
    <input name='password' type='password' id='password' value='$password'>
   </p>
 
  </fieldset>
</div>

 <p>&nbsp;</p>

 </div>
 </form>

 </body>
 </html>"
