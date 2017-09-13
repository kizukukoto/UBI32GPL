#! /bin/sh

IPTABLES=iptables

. /etc/rgw_config

if [ "$WANINTERFACE" = "" ]; then
	echo "Warning: No WAN interface! Terminated."
	exit 1
fi
EXTIP=`ifconfig $WANINTERFACE | grep 'inet addr' | cut -d':' -f2 | cut -d' ' -f1`
if [ "$EXTIP" = "" ]; then
	echo "Warning: No WAN IP! Terminated."
	exit 1
fi

#removing the MINIUPNPD chain for nat
$IPTABLES -t nat -F MINIUPNPD
#rmeoving the rule to MINIUPNPD
$IPTABLES -t nat -D PREROUTING -d $EXTIP -i $WANINTERFACE -j MINIUPNPD
$IPTABLES -t nat -X MINIUPNPD

#removing the MINIUPNPD chain for filter
$IPTABLES -t filter -F MINIUPNPD
#adding the rule to MINIUPNPD
$IPTABLES -t filter -D FORWARD -i $WANINTERFACE -o ! $WANINTERFACE -j MINIUPNPD
$IPTABLES -t filter -X MINIUPNPD

exit 0

