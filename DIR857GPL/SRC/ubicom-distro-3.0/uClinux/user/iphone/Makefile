# DEBUG
#CC = ubicom32-linux-uclibc-gcc
#CFLAGS = -march=ubicom32v4 -Os                     -g -fomit-frame-pointer -fno-common -fno-builtin -Wall  -Dlinux -D__linux__ -Dunix -D__uClinux__ -D__linux -DEMBED -DNO_FORK -I/home/ubicom-distro/uClinux/include -I/home/ubicom-distro/uClinux/lib/zlib -I/home/ubicom-distro/uClinux/uClibc/include -I/home/ubicom-distro/uClinux/lib/libssl/include -DAR9100 -DCONFIG_VLAN_ROUTER -DATHEROS_11N_DRIVER -DPCI_MODULE -DADVANCE_SPI -DUSE_SILEX -DSET_GET_FROM_ARTBLOCK -DCONFIG_NVRAM_SIZE="1088" -DCONFIG_UPGRADE_TIME="180" -DNAT_ENABLE -DPORT_FORWARD_BOTH -DINBOUND_FILTER -DACCESS_CONTROL -DDLINK_ROUTER_LED_DEFINE -DUDHCPD_NETBIOS -DUDHCPD_ALWAYS_BROADCAST -DUDHCPD_REVOKE -DUDHCPC_UNICAST -DUDHCPC_STATIC_ROUTE -DUDHCPC_CLASSLESS_STATIC_ROUTE -DUDHCPC_RFC_CLASSLESS_STATIC_ROUTE -DCONFIG_USER_IP_LOOKUP -DCONFIG_USER_NMBD -DDLINK_FW_QUERY -DUPNP_ATH_MINIUPNPD_RC -DCONFIG_USER_WCN -DUPNP_ATH_MINIUPNPD_1_3 -DCONFIG_USER_WISH -DCONFIG_USER_SHOW_CHECKSUM -DWPS_WPATALK -DCONFIG_USER_WL_RALINK -DUSER_WL_RALINK_5GHZ -DCONFIG_USER_WIRELESS_SCHEDULE -DCONFIG_UBICOM_SWITCH_AR8316 -DCONFIG_USER_LCDSHOWD -DDIR865 -I./ -I/home/ubicom-distro/uClinux/lib/libusb/libusb-0.1.12/ -I/home/ubicom-distro/uClinux/lib/libssl/build/include/ -fno-tree-scev-cprop
##########



#CROSS_COMPILER=/opt/ap71_tools/mips32k/bin/mips-linux-
APPS_PATH=$(ROOTDIR)/user
#TARGET_PATH=/home/AthSDK/rootfs/target/lib
MODULES_PATH = $(ROOTDIR)/lib
#GLIBC_MODULES_PATH = $(ROOTDIR)/../uClibc/lib
PROG        = iphone
VERS        = 1.0.4
#STRIP	    = $(CROSS_COMPILER)strip
#CC          = $(CROSS_COMPILER)gcc
IPHONE_CFLAGS     = $(CFLAGS) -I./ -I$(MODULES_PATH)/libusb/libusb-0.1.12/ -I$(MODULES_PATH)/libssl/build/include/
#LDFLAGS     = -L$(MODULES_PATH)/libusb/build-ubicom/lib/ -lusb -L$(MODULES_PATH)/uClibc/lib/ -lrt
#LDFLAGS     += -L$(MODULES_PATH)/uClibc/lib/ -lpthread -L$(MODULES_PATH)/ -lssl -L$(MODULES_PATH)/ -lcrypto
IPHONE_LDFLAGS = $(LDFLAGS) -lpthread -lssl -lcrypto -lrt -L$(MODULES_PATH)/libusb/build-ubicom/lib/ -lusb

RM          = /bin/rm -f
#OBJS        = *.c
SRCS = $(wildcard *.c)
OBJS = $(subst .c,.o,$(SRCS))

##PREFIX	    = $(DESTDIR)/usr
#ETCDIR	    = $(DESTDIR)/etc
#SBINDIR	    = $(PREFIX)/sbin

.PHONY: clean $(PROG)

all: $(PROG)

$(PROG): $(OBJS)
	echo "!!!!!!!!!!!"
	echo "CFLAGS = $(CFLAGS)"
	echo "!!!!!!!!!!!"
	$(CC) $(IPHONE_CFLAGS) $(OBJS) $(IPHONE_LDFLAGS) -o $@
	$(STRIP) $(PROG)

clean:
	$(RM) -rf *~ .sgbuilt_user *.o
	$(RM) ./iphone


romfs:
	$(ROMFSINST) -d $(ROOTDIR)/user/iphone/$(PROG) /bin
	$(ROMFSINST) -d $(ROOTDIR)/user/iphone/pem/HostCertificate.pem /etc
	$(ROMFSINST) -d $(ROOTDIR)/user/iphone/pem/RootCertificate.pem /etc
	$(ROMFSINST) -d $(ROOTDIR)/user/iphone/pem/RootPrivateKey.pem /etc

install: all
	mkdir -p $(SBINDIR)
	install ./iphone $(SBINDIR)
	mkdir -p $(ETCDIR)
	install --mode=644 ./iphone.conf $(ETCDIR)

uninstall: clean
	$(RM) /usr/sbin/iphone
	$(RM) /etc/usb_modeswitch.conf

.PHONY:     clean install uninstall

integrated:
	make -f Makefile.integrated

integrated_install:
	make -f Makefile.integrated install


%.o : %.c
	$(CC) $(IPHONE_CFLAGS) -c $*.c -o $*.o
