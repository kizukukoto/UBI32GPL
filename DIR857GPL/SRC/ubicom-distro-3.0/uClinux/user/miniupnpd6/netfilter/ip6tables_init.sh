#! /bin/sh
# $Id: iptables_init.sh,v 1.4 2008/04/25 18:15:08 nanard Exp $
IPTABLES=ip6tables

#change this parameters :
EXTIF=eth2
#EXTIP="`LC_ALL=C /sbin/ifconfig $EXTIF | grep 'inet6 addr' | awk '{print $2}' | sed -e 's/.*://'`"
EXTIP="`LC_ALL=C /sbin/ifconfig $EXTIF | grep 'inet6 addr' | awk '{print $3}' | cut -d "/" -f1`"

echo "External IP = $EXTIP"

#adding the MINIUPNPD chain for nat
$IPTABLES -t mangle -N MINIUPNPD
#adding the rule to MINIUPNPD
#$IPTABLES -t nat -A PREROUTING -d $EXTIP -i $EXTIF -j MINIUPNPD
$IPTABLES -t mangle -A PREROUTING -i $EXTIF -j MINIUPNPD

#adding the MINIUPNPD chain for filter
$IPTABLES -t filter -N MINIUPNPD
#adding the rule to MINIUPNPD
$IPTABLES -t filter -I FORWARD -p icmpv6 -j ACCEPT
$IPTABLES -t filter -A FORWARD -i $EXTIF ! -o $EXTIF -j MINIUPNPD
$IPTABLES -A MINIUPNPD -i $EXTIF ! -o $EXTIF -m state --state ESTABLISHED,RELATED -j ACCEPT
$IPTABLES -A FORWARD ! -i $EXTIF -o $EXTIF -j ACCEPT
$IPTABLES -P FORWARD DROP

