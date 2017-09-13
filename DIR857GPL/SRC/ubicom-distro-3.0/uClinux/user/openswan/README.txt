

1. How to build

The following options should be turned on.

** Kernel Options **
Networking support
	Network options
		Transformation user configuration interface
		PF_KEY sockets
		IP: AH transformation
		IP: ESP transformation
		IP: IPComp transformation
		IP: IPsec transport mode
		IP: IPsec tunnel mode
		IP: IPsec BEET mode (?)

Device Drivers
	Character devices
		Hardware Random Number Generator Core support

Cryptographic API
	CBC (? may not necessary if HW crypto is turned on)
	ECB (? may not necessary if HW crypto is turned on)
	HMAC
	MD5 & SHA1 (? may not necessary)
	AES & DES  (? may not necessary if HW crypto is turned on)
	Hardware crypto devices
		Ubicom32 Security Module
			AES
			DES
			SHA1
			AES

** uClibc Options **
	Make sure MALLOC_GLIBC_COMPAT is set to y. It is set for Ubicom32/RouterGateway product by default.

** User Options **
	Network Applications
		openswan apps
			pluto
			whack
			ranbits 
			rsasigkey
			eroute
			..
			tncfg
			*** turn off lwdnsq
		iproute2
			ip

2. How to test

2.1 *-to-host

	This covers host-to-host, net-to-host and nat-to-host. 

2.1.1 Remote host setup

The following are for file /etc/ipsec.conf. You can find sample conf files at /etc/ipsec.d/examples.

The remote host can have the same config setup section:
	
	protostack=netkey
	nat_traversal=yes
	interfaces="eth0"

	# depends on how verbose is expected
	plutodebug=all		

Better disable OE:

	include /etc/ipsed.d/examples/no_oe.conf

Connection section is a little different when the local side is 'host' instead of 'net' or 'nat'
2.1.1.1 local side is host
The sample setup will look like
conn my_test
	# the WAN_IF of the local node
	left=192.168.2.77
	# the IP address of remote host
	right=192.168.x.y
	authby=secret
	auto=add
	
2.1.1.2 local side is net or nat
conn my_test
        # the WAN_IF of the local node
        left=192.168.2.77
	# the LAN subnet of the local node
	leftsubnet=192.168.0.0/24

        # the IP address of remote host
        right=192.168.x.y
        authby=secret
        auto=add


	# the LAN subnet of the local node
	leftsubnet=192.168.0.0/24

	# the LAN subnet of the local node
	leftsubnet=192.168.0.0/24

2.1.1.3 Remote secrets
The secrets are saved in /etc/ipsec.secrets. The way to setup secrets is the same as regular openswan on both sides.

PSK: It is our default setting, please copy romfs/etc/ipsec.secrets out, and just change the psk.

RSA: run rsasigkey to generate the key and copy the output to /etc/ipsec.secrets

2.1.1.4 Remote host commands

# restart ipsec
/etc/init.d/ipsec --restart

# start the connection
ipsec auto --up my_test

Whenever ipsec.conf is updated, we need to restart ipsec for pluto to pick up the latest change.

2.1.2 Local node (DUT) setups

2.1.2.1 Local secrets
The secrets file is also /etc/ipsec.secrets. Please follow 2.1.1.3 for how to set it up.

2.1.2.3 Local node commands

uClinux shell is not powerful enough to allow use to use the shell scripts come with openswan. So we created our own script /bin/ipsec_uc_start and /bin/ipsec_uc_stop

Here are some sample usages:
ipsec_uc_start host-to-host 192.168.2.77 192.168.x.y
ipsec_uc_start net-to-host 192.168.2.77 192.168.x.y 192.168.x.0/24 192.168.x.1
ipsec_uc_start nat-to-host 192.168.2.77 192.168.x.y 192.168.x.0/24 192.168.x.1

The 3rd option needs a special iptable rule to work.

