#! /bin/sh
# $Id: iptables_flush.sh,v 1.1 2009/04/21 11:14:48 jimmy_huang Exp $
IPTABLES=iptables

#flush all rules owned by miniupnpd
$IPTABLES -t nat -F MINIUPNPD
$IPTABLES -t filter -F MINIUPNPD

