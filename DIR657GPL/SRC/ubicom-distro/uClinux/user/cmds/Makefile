# CFLAGS="-I`pwd`/../nvram/ -DCONFIG_BCM_IPSEC" \
# LDFLAGS="-L`pwd`/../nvram/" 
# make CC=arm-linux-gcc LD=arm-linux-ld
#

#SUBDIR=sys main net services lib utility security security/vpn misc
SUBDIR=main lib

ifdef SRCBASE
SUBDIR+=security/vpn security
endif

ifeq ($(CONFIG_PRODUCT),DIR652V)
SUBDIR+=security/vpn security utility services
endif

ifndef SRCBASE
SUBDIR+=ipv6
ifeq ($(CONFIG_PRODUCT),DIR652V)
SUBDIR+=sys
endif
endif

BUILTIN_OBJS=$(addsuffix /built-in.o, $(SUBDIR))
TOPDIR=$(shell pwd)
export LDFLAGS += -Wall -lnvram

ifeq ($(CONFIG_USER_WL_RALINK), y)
        @echo =================== CONFIG_USER_WL_RALINK ===================
SUBDIR += wlan/Ralink
CFLAGS += -I$(TOPDIR)/../sutil -I$(shell pwd)/../../include/modules/
LDFLAGS += -L$(TOPDIR)/../sutil -L$(TOPDIR)/../artblock -lsutil -lartblock
else
ifeq ($(CONFIG_USER_WL_ATH), y)
        @echo =================== CONFIG_USER_WL_ATH ===================
SUBDIR += wlan/Atheros
CFLAGS += -I$(TOPDIR)/../sutil -I$(shell pwd)/../../include/modules/
LDFLAGS += -L$(TOPDIR)/../sutil -L$(TOPDIR)/../artblock -lsutil 
ifeq ($(CONFIG_PRODUCT),DIR652V)
LDFLAGS += -lartblock
endif
else
ifeq ($(CONFIG_USER_WL_MVL), y)
        @echo =================== CONFIG_USER_WL_MARVEL ===================
SUBDIR += wlan/Marvel
CFLAGS += -I$(TOPDIR)/../sutil -I$(shell pwd)/../../include/modules/
LDFLAGS += -L$(TOPDIR)/../sutil -L$(TOPDIR)/../artblock -lsutil -lartblock
else
SUBDIR += wlan/Default
endif
endif
endif

#BCM base
ifdef SRCBASE
CFLAGS += -I$(SRCBASE)/include -DCONFIG_BCM_IPSEC
else
CFLAGS += -DCONFIG_UBICOM_ARCH
endif

CFLAGS += -I$(TOPDIR)/include -I$(TOPDIR)/../nvram
#export CFLAGS += -I$(TOPDIR)/include -I$(TOPDIR)/../nvram -I$(SRCBASE)/include
export CFLAGS

#all: cli plugin
all: indep cli

plugin:
	@$(MAKE) -C $@
plugin-install:
	@$(MAKE) -C plugin install 
cli:$(SUBDIR)
	$(CC) -L../nvram $(LDFLAGS) $(BUILTIN_OBJS)  -o $@ -ldl
$(SUBDIR):
	@$(MAKE) -C $@

clean: uindep
	@for i in $(SUBDIR); do make -C $$i $@; done
	@rm -f cli
install:
	[ -d $(INSTALLDIR)/bin ] || mkdir -p $(INSTALLDIR)/bin
	cp cli $(INSTALLDIR)/bin
	-$(STRIP) $(INSTALLDIR)/bin/cli

romfs:
	$(ROMFSINST) cli  /bin/cli

ifeq ($(CONFIG_VENDOR), Ubicom)
CFLAGS += -DLINUX_NVRAM -DNOMMU 
endif
.PHONY: indep uindep $(SUBDIR) plugin
