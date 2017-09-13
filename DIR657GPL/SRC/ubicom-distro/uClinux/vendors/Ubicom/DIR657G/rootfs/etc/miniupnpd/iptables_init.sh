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

#adding the MINIUPNPD chain for nat
$IPTABLES -t nat -N MINIUPNPD
#adding the rule to MINIUPNPD
$IPTABLES -t nat -A PREROUTING -d $EXTIP -i $WANINTERFACE -j MINIUPNPD

#adding the MINIUPNPD chain for filter
$IPTABLES -t filter -N MINIUPNPD
#adding the rule to MINIUPNPD
$IPTABLES -t filter -A FORWARD -i $WANINTERFACE -o ! $WANINTERFACE -j MINIUPNPD

exit 0

