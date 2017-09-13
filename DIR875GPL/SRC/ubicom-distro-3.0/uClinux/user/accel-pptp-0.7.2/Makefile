PPPD := $(shell /usr/sbin/pppd --version 2>&1 | grep version)
PPPD := $(patsubst pppd,,$(PPPD))
PPPD := $(patsubst version,,$(PPPD))
PPPD := $(strip $(PPPD))

CONFOPTS_pppd_plugin := \
	--disable-nls \
	--disable-libtool-lock \
	--enable-shared=yes \
	--with-included-gettext \

# jimmy added
#export KDIR=/home/AthSDK/platform/AR9100/kernels/mips-linux-2.6.15
# below is used for mips environment
#include ../../.config
#export CROSS_COMPILE := $(APPS_CROSS)
#export APPS_PATH := $(ENV_PATH)/apps
#export FS_PATH := $(ENV_PATH)/rootfs/target
#export TARGET := $(FS_PATH)
#export CC := $(CROSS_COMPILE)gcc
#export AS := $(CROSS_COMPILE)as
#export AR := $(CROSS_COMPILE)ar
#export LD := $(CROSS_COMPILE)ld
#export CCPP := $(CROSS_COMPILE)g++
#export COMPILER := $(CROSS_COMPILE)g++
#export STRIP := $(CROSS_COMPILE)strip
#export SIZE := $(CROSS_COMPILE)size
#export RANLIB := $(CROSS_COMPILE)ranlib

all: plugin

install:
	cp pppd_plugin/src/pptp.so $(TARGET)/lib/
#	cp /home/AthSDK/apps/accel-pptp-0.7.12/pppd_plugin/src/.libs/pptp.so.0.0.0 /home/AthSDK/rootfs/target/lib/pptp.so
# ----------------------------------------------------

default: module plugin pptpd
client: module plugin
server: default


module:
	echo Building kernel module
	(cd kernel/driver; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" make )

plugin: pppd_plugin/Makefile
	echo Building pppd plugin
	(cd pppd_plugin; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" make)

pptpd: pptpd-1.3.3/Makefile
	echo Building pptpd
	(cd pptpd-1.3.3; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" make)

pppd_plugin/Makefile: pptpd-1.3.3/Makefile
	(cd pppd_plugin; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" ./configure $(CONFIGURE_OPTS) $(CONFOPTS_pppd_plugin))

pptpd-1.3.3/Makefile:
	(cd pptpd-1.3.3; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" ./configure $(CONFIGURE_OPTS))


module_install: module
	(cd kernel/driver; make install)

plugin_install: plugin
	install -m 0644 pppd_plugin/src/.libs/pptp.so.0.0.0 /usr/lib/pppd/$(PPPD)/pptp.so

client_install: module_install plugin_install

server_install: server module_install plugin_install
	(cd pptpd-1.3.3; make install)

clean:
	rm -rf *~
	(cd kernel/driver; make clean)
	(cd pppd_plugin; make clean)
	(cd pptpd-1.3.3; make clean)
distclean:
	(cd kernel/driver; make clean)
	(cd pppd_plugin; make distclean)
	(cd pptpd-1.3.3; make distclean)

romfs:
	$(ROMFSINST) pppd_plugin/src/.libs/pptp.so.0.0.0 /lib/pptp.so

