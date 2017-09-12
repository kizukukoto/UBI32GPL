status=`curl -k https://www.wd2go.com/api/1.0/rest/relay_server?format=openvpn -o /tmp/txt1 >/dev/null 2>/dev/null`
SRV=`cat /tmp/txt1`
sed -e "s/remote host port/$SRV/g" client.ovpn.tpl > /internalhd/etc/orion/client.ovpn
rm /tmp/txt1
