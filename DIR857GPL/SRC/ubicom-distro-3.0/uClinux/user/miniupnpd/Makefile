# $Id: Makefile,v 1.6 2009/07/20 09:16:28 jimmy_huang Exp $
# MiniUPnP project
# http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
# Author : Thomas Bernard
# for use with GNU Make
#
# To install use :
# $ PREFIX=/dummyinstalldir make -f Makefile.linux install
# or :
# $ INSTALLPREFIX=/usr/local make -f Makefile.linux install
# or :
# $ make -f Makefile.linux install
#
# if your system hasn't iptables libiptc headers and binary correctly
# installed, you need to get iptables sources from http://netfilter.org/
# ./configure them and build them then miniupnpd will build using :
# $ IPTABLESPATH=/path/to/iptables-1.4.1 make -f Makefile.linux
#
#CFLAGS = -Wall -O -D_GNU_SOURCE -g -DDEBUG

include ${ROOTDIR}/../.config

########		jimmy added		########

# below is used for mips environment
#include ../../.config

# for test, if u want to directly make within this directory
# unmark the line below
#include $(ENV_PATH)/platform/$(PLATFORM)/env.mk

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
export APPS_PATH := $(ROOTDIR)/user
export KERNEL_DIR := $(ROOTDIR)/linux-2.6.x
                            
ifeq ($(CONFIG_USER_MINIUPNPD_DEBUG),y)
CFLAGS += -DMINIUPNPD_DEBUG
CFLAGS += -DDEBUG
endif

ifeq ($(CONFIG_USER_MINIUPNPD_DEBUG_SSDP),y)
CFLAGS += -DMINIUPNPD_DEBUG_SSDP
endif

ifeq ($(CONFIG_USER_MINIUPNPD_DEBUG_SOAP),y)
CFLAGS += -DMINIUPNPD_DEBUG_SOAP
endif

ifeq ($(CONFIG_USER_MINIUPNPD_DEBUG_EVENTS),y)
CFLAGS += -DMINIUPNPD_DEBUG_EVENTS
endif

ifeq ($(CONFIG_USER_MINIUPNPD_DEBUG_DESCRIPTION),y)
CFLAGS += -DMINIUPNPD_DEBUG_DESCRIPTION
endif

ifeq ($(CONFIG_USER_MINIUPNPD_DEBUG_HTTP),y)
CFLAGS += -DMINIUPNPD_DEBUG_HTTP
endif

ifeq ($(CONFIG_USER_MINIUPNPD_DEBUG_NVRAM_FUNS),y)
CFLAGS += -DMINIUPNPD_DEBUG_NVRAM_FUNS
endif

ifeq ($(PLATFORM),AthSDK_AP61) 
	IPTABLESPATH=../iptables
else
#	IPTABLESPATH=../iptables_2_6
	IPTABLESPATH=../iptables
endif

ifeq ($(CONFIG_LPJ_ultra8k), y)
	IPTABLESPATH=${ROOTDIR}/user/iptables-new/iptables-1.4.8
endif
                    
INSTALLPREFIX=$(TARGET)
PREFIX=$(TARGET)

# To get CONFIG_USER_UPNP_ATH_MINIUPNPD_1_3_LEASE_FILES and
#       CONFIG_USER_UPNP_ATH_MINIUPNPD_1_3_CHECK_VS_NVRAM
#include ./../.config
ifeq ($(CONFIG_USER_WL_ATH),y)
	CFLAGS += -DWPS_WITH_WSCCMD
endif

ifeq ($(CONFIG_USER_UPNP_ATH_MINIUPNPD_1_3_LEASE_FILES),y)
	CFLAGS += -DENABLE_LEASEFILE
endif

ifeq ($(CONFIG_USER_UPNP_ATH_MINIUPNPD_1_3_CHECK_VS_NVRAM),y)
	CFLAGS += -DCHECK_VS_NVRAM
endif

ifeq ($(CONFIG_USER_UPNP_ATH_MINIUPNPD_1_3_HAIRPIN),y)
	CFLAGS += -DHAIRPIN_SUPPORT
endif

############# ----------------------- #############

#CFLAGS += -Wall -Os -D_GNU_SOURCE
#CC = gcc
#RM = rm -f
#INSTALL = install
#STRIP = strip

INSTALLPREFIX ?= $(PREFIX)/usr
SBININSTALLDIR = $(INSTALLPREFIX)/sbin
ETCINSTALLDIR = $(PREFIX)/etc/miniupnpd

BASEOBJS = miniupnpd.o upnphttp.o upnpdescgen.o upnpsoap.o \
           upnpreplyparse.o minixml.o \
		   upnpredirect.o getifaddr.o daemonize.o upnpglobalvars.o \
           options.o upnppermissions.o minissdp.o natpmp.o \
           upnpevents.o nvram_funs.o

LNXOBJS = linux/getifstats.o
NETFILTEROBJS = netfilter/iptcrdr.o

ALLOBJS = $(BASEOBJS) $(LNXOBJS) $(NETFILTEROBJS)

LIBS = -liptc

ifdef IPTABLESPATH
CFLAGS := $(CFLAGS) -I$(IPTABLESPATH)/include/ -I$(KERNEL_DIR)/include/
ifeq ($(CONFIG_LPJ_ultra8k), y)
LDFLAGS += $(LDFLAFGS) -L$(IPTABLESPATH)/libiptc/.libs/
LIBS = $(IPTABLESPATH)/libiptc/.libs/libip4tc.a
CFLAGS += -DIPTABLES_143
else
LDFLAGS := $(LDFLAFGS) -L$(IPTABLESPATH)/libiptc/
LIBS = $(IPTABLESPATH)/libiptc/libiptc.a
endif
endif

#	Date:	2009-05-05
#	Name:	jimmy huang
#	Reason:	For the PhysicalLinkStatus Support 
#			(UPnP_UGD_WANCommonInterfaceConfig 1.0.pdf, page 8)
#
LDFLAGS += -L$(APPS_PATH)/libplatform -lvct -L$(APPS_PATH)/libplatform -lversion
CFLAGS += -I$(APPS_PATH)/libplatform/ -I./ -I$(APPS_PATH)/sutil/

#
# Gareth Williams (gwilliams@ubicom.com) 14/dec/2009 - performance work
#
LDLIBS += -lsutil
LDFLAGS += -L$(ROOTDIR)/user/sutil

TESTUPNPDESCGENOBJS = testupnpdescgen.o upnpdescgen.o

#EXECUTABLES = miniupnpd testupnpdescgen testgetifstats \
#              testupnppermissions miniupnpdctl testgetifaddr
EXECUTABLES = miniupnpd miniupnpdctl

.PHONY:	all clean install depend genuuid

all:	$(EXECUTABLES)

clean:
	$(RM) $(ALLOBJS)
	$(RM) $(EXECUTABLES)
	$(RM) testupnpdescgen.o testgetifstats.o
	$(RM) testupnppermissions.o
	$(RM) miniupnpdctl.o
	$(RM) -rf *~

install:	miniupnpd genuuid
	$(STRIP) miniupnpd
	$(INSTALL) -d $(SBININSTALLDIR)
	$(INSTALL) miniupnpd $(SBININSTALLDIR)
	$(INSTALL) -d $(ETCINSTALLDIR)
	$(INSTALL) netfilter/iptables_init.sh $(ETCINSTALLDIR)
	$(INSTALL) netfilter/iptables_removeall.sh $(ETCINSTALLDIR)
#	$(INSTALL) --mode=0644 -b miniupnpd.conf $(ETCINSTALLDIR)
#	$(INSTALL) -d $(PREFIX)/etc/init.d
#	$(INSTALL) linux/miniupnpd.init.d.script $(PREFIX)/etc/init.d/miniupnpd

romfs:
	$(ROMFSINST) miniupnpd /sbin/miniupnpd
	$(ROMFSINST) -d miniupnpd.conf /var/etc/miniupnpd.conf

# genuuid is using the uuidgen CLI tool which is part of libuuid
# from the e2fsprogs
genuuid:
	sed -i -e "s/^uuid=[-0-9a-f]*/uuid=`(genuuid||uuidgen) 2>/dev/null`/" miniupnpd.conf

miniupnpd:	$(BASEOBJS) $(LNXOBJS) $(NETFILTEROBJS) $(LIBS)

testupnpdescgen:	$(TESTUPNPDESCGENOBJS)

testgetifstats:	testgetifstats.o linux/getifstats.o

testupnppermissions:	testupnppermissions.o upnppermissions.o

testgetifaddr:	testgetifaddr.o getifaddr.o

miniupnpdctl:	miniupnpdctl.o

config.h:	genconfig.sh
#	./genconfig.sh

depend:	config.h
	makedepend -f$(MAKEFILE_LIST) -Y \
	$(ALLOBJS:.o=.c) $(TESTUPNPDESCGENOBJS:.o=.c) \
	testgetifstats.c 2>/dev/null

# DO NOT DELETE

miniupnpd.o: config.h upnpglobalvars.h upnppermissions.h miniupnpdtypes.h
miniupnpd.o: upnphttp.h upnpdescgen.h miniupnpdpath.h getifaddr.h upnpsoap.h
miniupnpd.o: options.h minissdp.h upnpredirect.h daemonize.h upnpevents.h
miniupnpd.o: natpmp.h commonrdr.h
upnphttp.o: config.h upnphttp.h upnpdescgen.h miniupnpdpath.h upnpsoap.h
upnphttp.o: upnpevents.h
upnpdescgen.o: config.h upnpdescgen.h miniupnpdpath.h upnpglobalvars.h
upnpdescgen.o: upnppermissions.h miniupnpdtypes.h upnpdescstrings.h
upnpsoap.o: config.h upnpglobalvars.h upnppermissions.h miniupnpdtypes.h
upnpsoap.o: upnphttp.h upnpsoap.h upnpreplyparse.h upnpredirect.h getifaddr.h
upnpsoap.o: getifstats.h
upnpreplyparse.o: upnpreplyparse.h minixml.h
minixml.o: minixml.h
upnpredirect.o: config.h upnpredirect.h upnpglobalvars.h upnppermissions.h
upnpredirect.o: miniupnpdtypes.h upnpevents.h netfilter/iptcrdr.h commonrdr.h
getifaddr.o: getifaddr.h
daemonize.o: daemonize.h config.h
upnpglobalvars.o: config.h upnpglobalvars.h upnppermissions.h
upnpglobalvars.o: miniupnpdtypes.h
options.o: options.h config.h upnppermissions.h upnpglobalvars.h
options.o: miniupnpdtypes.h
upnppermissions.o: config.h upnppermissions.h
minissdp.o: config.h upnpdescstrings.h miniupnpdpath.h upnphttp.h
minissdp.o: upnpglobalvars.h upnppermissions.h miniupnpdtypes.h minissdp.h
minissdp.o: codelength.h
natpmp.o: config.h natpmp.h upnpglobalvars.h upnppermissions.h
natpmp.o: miniupnpdtypes.h getifaddr.h upnpredirect.h commonrdr.h
upnpevents.o: config.h upnpevents.h miniupnpdpath.h upnpglobalvars.h
upnpevents.o: upnppermissions.h miniupnpdtypes.h upnpdescgen.h
linux/getifstats.o: getifstats.h config.h
netfilter/iptcrdr.o: netfilter/iptcrdr.h commonrdr.h config.h
netfilter/iptcrdr.o: upnpglobalvars.h upnppermissions.h miniupnpdtypes.h
testupnpdescgen.o: config.h upnpdescgen.h
upnpdescgen.o: config.h upnpdescgen.h miniupnpdpath.h upnpglobalvars.h
upnpdescgen.o: upnppermissions.h miniupnpdtypes.h upnpdescstrings.h
testgetifstats.o: getifstats.h
# jimmy added
nvram_funs.o: config.h nvram_funs.h
# -----------

