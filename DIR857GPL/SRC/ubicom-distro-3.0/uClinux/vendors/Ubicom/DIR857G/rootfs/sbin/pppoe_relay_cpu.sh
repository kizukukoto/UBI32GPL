#!/bin/sh

rm -rf /var/tmp/pppoe_relay_cpu
GET_TOP_PPPOERELAY=`top -n1 | sort -k 7 -r | head -10 | grep pppoe-relay`
if [ "$GET_TOP_PPPOERELAY" != "" ]; then
       echo 1 > /var/tmp/pppoe_relay_cpu
fi
