#!/bin/sh 
service ORION stop;
while [ "1" = "1" ]; do
	cc=`ps | grep crawler | grep -v grep`;
	if [ "$cc" = "" ]; then break;
	else echo "Waiting"; sleep 1;fi 
done
/usr/local/mediacrawler/mediacrawlerd reset;
sleep 2;
/usr/local/orion/miocrawler/miocrawlerd reset;
sleep 2;
INTETC=/internalhd/etc;
ORIONCONFS="orion.db oriondb_version.txt dynamicconfig.ini dynamicconfig.ini_safe orion_cm_enabled";
for i in $ORIONCONFS; do
	if [ -f $INTETC/orion/$i ]; then
   		rm -f $INTETC/orion/$i;
		echo "[Orion]Remove $i" > /dev/console;
	fi
done
