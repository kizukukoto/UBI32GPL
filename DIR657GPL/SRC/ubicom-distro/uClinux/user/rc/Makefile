# Router rc Makefile
#
#include ./../.config

# This already done in ubicom-distro/uClinux/vendors/Ubicom/DIR655/config.arch
#ifeq ($(CONFIG_USER_WCN),y)
#CFLAGS += -DCONFIG_USER_WCN
#endif

# tr069 support openagent 2010.01.28
ifeq ($(CONFIG_USER_OPENAGENT),y)
CFLAGS += -DCONFIG_USER_OPENAGENT
endif

ifeq ($(CONFIG_USER_RC_DEBUG),y)
CFLAGS += -DRC_DEBUG
endif

ifeq ($(CONFIG_USER_RC_DEBUG_WANTIMER),y)
CFLAGS += -DRC_DEBUG_WANTIMER
endif

ifeq ($(CONFIG_USER_RC_DEBUG_WLAN),y)
CFLAGS += -DRC_DEBUG_WLAN
endif

ifeq ($(CONFIG_USER_CROWDCONTROL),y)
CFLAGS += -DCONFIG_USER_CROWDCONTROL
endif

ifeq ($(CONFIG_USER_WAKE_ON_LAN),y)
CFLAGS += -DWAKE_ON_LAN
endif

ifeq ($(CONFIG_USER_OPENDNS),y)
CFLAGS += -DOPENDNS
endif

ifeq ($(CONFIG_USER_WL_ATH_WDS),y)
CFLAGS += -DATH_WDS
endif

ifeq ($(CONFIG_USER_WL_ATH_WDS_AP),y)
CFLAGS += -DATH_WDS_AP
endif

ifeq ($(CONFIG_USER_WL_ATH_WDS_BR),y)
CFLAGS += -DATH_WDS_BR
endif

ifeq ($(CONFIG_USER_WL_ATH_WDS_RPT),y)
CFLAGS += -DATH_WDS_RPT
endif

ifeq ($(CONFIG_USER_DLINK_ROUTER_URL),y)
CFLAGS += -DCONFIG_USER_DLINK_ROUTER_URL
endif

ifeq ($(CONFIG_USER_WL_ATH_GUEST_ZONE),y)
CFLAGS += -DCONFIG_USER_WL_ATH_GUEST_ZONE
endif

# Support HTTPS
ifeq ($(CONFIG_USER_HTTPS),y)
CFLAGS += -DHAVE_HTTPS
endif

ifeq ($(CONFIG_USER_WAN_8021X),y)
CFLAGS += -DCONFIG_USER_WAN_8021X
endif

ifeq ($(CONFIG_USER_LINUX_NVRAM),y)
CFLAGS += -I$(shell pwd)/../artblock -DCONFIG_USER_LINUX_NVRAM
LDFLAGS += -L$(shell pwd)/../artblock -lartblock
else
endif

CFLAGS += -I./ -I$(ROOTDIR)/user/sutil/ -I$(ROOTDIR)/user/nvram -I$(ROOTDIR)/user/libplatform/ -I$(ROOTDIR)/user/mtd-utils/mtd/ #-DWLAN_FUNC
CFLAGS += -I$(ROOTDIR)/config
LDFLAGS += -L$(ROOTDIR)/user/nvram -L$(ROOTDIR)/user/sutil -L$(ROOTDIR)/user/libplatform
LDLIBS += -lnvram -lsutil -lversion -lvct -lproject -lipv6

ifeq ($(CONFIG_USER_FASTNAT_APPS), y)
CFLAGS += -DCONFIG_USER_FASTNAT_APPS
endif

OBJS := rc.o lan.o app.o platform.o process.o psmon.o network.o wan.o firewall.o ppp.o route.o wantimer.o mtd.o wlan.a

ifeq ($(CONFIG_USER_WEB_FILE_ACCESS), y)
OBJS += lighttpd.o
endif

ifeq ($(CONFIG_USER_WISH), y)
OBJS += ubicom_wish.o
endif

ifeq ($(CONFIG_USER_FIREWALL2), y)
CFLAGS += -DCONFIG_USER_FIREWALL2
OBJS += firewall2.o
endif

ifeq ($(CONFIG_USER_IPTABLES_IP6TABLES), y)
CFLAGS += -DCONFIG_IPV6_FIREWALL
endif

# Support 6rd or not
ifeq ($(CONFIG_USER_IPV6_6RD),y)
CFLAGS += -DIPV6_6RD 
endif

# Support disable lan rdnss function or not
ifeq ($(CONFIG_USER_IPV6_DISABLE_LAN_RDNSS ),y)
CFLAGS += -DIPV6_DISABLE_LAN_RDNSS 
endif

ifeq ($(CONFIG_USER_TC),y)
CFLAGS +=-DCONFIG_USER_TC
OBJS += qos.o
endif

ifeq ($(CONFIG_USER_STREAMENGINE),y)
CFLAGS += -DCONFIG_USER_STREAMENGINE
endif

ifeq ($(CONFIG_USER_STREAMENGINE2),y)
CFLAGS += -DCONFIG_USER_STREAMENGINE2
endif

ifeq ($(CONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K),y)
CFLAGS += -DCONFIG_MODULE_UBICOM_STREAMENGINE2_IP8K
endif

ifeq ($(CONFIG_USER_PPPOE_RELAY),y)
CFLAGS += -DPPPOE_RELAY
endif

ifeq ($(CONFIG_USER_RADVD_RADVD),y)
CFLAGS += -DIPv6_SUPPORT -DIPv6_STATIC -DIPV6_STATELESS_WAN -DDHCPv6 -DIPV6_CLIENT_LIST -DRADVD -DCONFIG_IPV6_LLMNR -DIPV6_TUNNEL -DSIX_TO4_TUNNEL -DSIX_IN4_TUNNEL -DIPV6_PPPoE
OBJS += tunnel.o
obj-$(CONFIG_IPV6_LLMNR) += llmnr
LDLIBS += -lipv6
endif

all: builddate wlan rc

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o rc .*.depend *~ build.h wlan/*/*.o *.a

install: all
	install -d $(TARGET)/sbin
	install rc $(TARGET)/sbin
	$(STRIP) $(TARGET)/sbin/rc
#	cd $(INSTALLDIR)/sbin && ln -sf rc init

.PHONY: wlan
wlan:
	$(MAKE) -C wlan

rc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS) -a wlan.a

romfs:
	$(ROMFSINST) rc /sbin/rc

# dnsmasq resolv.conf setting
ifeq ($(CONFIG_USER_IPV6_SUPPORT),y)
	cd $(ROMFSDIR)/etc ; ln -sf ../var/etc/resolv_dual.conf resolv.conf
endif

ifeq ($(CONFIG_USER_WL_RALINK), y)
	cd $(ROMFSDIR)/sbin/ ; ln -sf rc cmds
	cd $(ROMFSDIR)/sbin/ ; ln -sf rc wlan-config
endif

builddate:
	echo  '#define __BUILD_DATE__ "' `date +%m%d%H%M%Y` '"' > build.h

