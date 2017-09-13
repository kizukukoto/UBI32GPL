#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML>
 <HEAD>
 <TITLE>System logs</TITLE>
 <link rel='stylesheet' rev='stylesheet' href='../ubicom_style.css' type='text/css' />
 </HEAD>"

syslog_file=/var/log/messages

echo "<BODY><pre>"

cat $syslog_file

echo "</pre></BODY>
 </HTML>"
