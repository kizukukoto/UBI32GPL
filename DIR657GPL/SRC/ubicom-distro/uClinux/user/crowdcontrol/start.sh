#!/bin/sh
killall crowdcontrol
#./crowdcontrol  --deny-domains blocked-domains --deny-urls blocked-urls -p 3128 -a 192.168.0.1 -s 255.255.255.0

./crowdcontrol  \
#--implicitly-deny --permit-domains permit-domains -p 3128 -a 192.168.0.1 -s 255.255.255.0

#./crowdcontrol  --test yahoo -p 3128 -a 192.168.0.1 -s 255.255.255.0

echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -F
iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 3128
iptables -t nat -A PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 3128
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE


crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3128 --deny-domains /var/tmp/blocked-domains 
crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3128 --permit-domains /var/tmp/permit-domains
crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3128 --deny-urls /var/tmp/blocked-domains  
crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3128 --permit-urls /var/tmp/permit-domains
crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3128 --deny-expressions /var/tmp/blocked-domains  
crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3128 --permit-expressions /var/tmp/permit-domains

#iptables -I INPUT -i br0 -p tcp --dport 3128 -j ACCEPT
#iptables -D INPUT -i br0 -p tcp --dport 3128 -j ACCEPT

# for trust ip
iptables -t nat -I PREROUTING -i br0 -s 192.168.10.101 -p tcp --dport 80 -j ACCEPT
iptables -t nat -D PREROUTING -i br0 -s 192.168.10.101 -p tcp --dport 80 -j ACCEPT

iptables -t nat -I PREROUTING -i br0 -d ! 192.168.10.1 -p tcp --dport 80 -j DNAT --to-destination 192.168.10.1:3128
iptables -t nat -D PREROUTING -i br0 -d ! 192.168.10.1 -p tcp --dport 80 -j DNAT --to-destination 192.168.10.1:3128
	
# premit and deny both
crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3128 --premit-domains /var/tmp/premit-domains
crowdcontrol -a 192.168.10.1 -s 255.255.255.0 -p 3129 --deny-domains /var/tmp/premit-domain
iptables -t nat -I PREROUTING -i br0 -s 192.168.10.101 -d ! 192.168.10.1 -p tcp --dport 80 -j DNAT --to-destination 192.168.10.1:3128
iptables -t nat -D PREROUTING -i br0 -s 192.168.10.101 -d ! 192.168.10.1 -p tcp --dport 80 -j DNAT --to-destination 192.168.10.1:3128
iptables -t nat -I PREROUTING -i br0 -s 192.168.10.102 -d ! 192.168.10.1 -p tcp --dport 80 -j DNAT --to-destination 192.168.10.1:3129
iptables -t nat -D PREROUTING -i br0 -s 192.168.10.102 -d ! 192.168.10.1 -p tcp --dport 80 -j DNAT --to-destination 192.168.10.1:3129