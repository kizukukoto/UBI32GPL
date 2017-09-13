# Makefile.  Generated from Makefile.in by configure.
#***********************************************************************
#
# Makefile
#
# Makefile for Roaring Penguin's Linux user-space PPPoE client.
#
# Copyright (C) 2000 Roaring Penguin Software Inc.
#
# This program may be distributed according to the terms of the GNU
# General Public License, version 2 or (at your option) any later version.
#
# LIC: GPL
#
# $Id: Makefile,v 1.3 2009/07/13 03:03:43 ken_chiang Exp $
#***********************************************************************

# Version is set ONLY IN THE MAKEFILE!  Don't delete this!
VERSION=3.7

DEFINES=
prefix=
exec_prefix=${prefix}
mandir=${prefix}/man
docdir=/usr/share/doc/rp-pppoe-$(VERSION)
install=/usr/bin/install -c
install_dir=/usr/bin/install -c -d
sbindir = $(TARGET)/sbin

# Plugin for pppd on Linux
LINUX_KERNELMODE_PLUGIN=
PPPD_INCDIR=

# Licensed version
LIC_INCDIR=
LIC_LIBDIR=
LIC_LIB=
LIC_DEFINE=

# PPPoE relay -- currently only supported on Linux
PPPOE_RELAY=pppoe-relay

# Program paths
PPPOE_PATH=$(sbindir)/pppoe
PPPD_PATH=$(TARGET)/sbin/pppd

# Include ServPoET version if we're building for ServPoET


# Kernel-mode plugin gets installed here.
PLUGIN_DIR = $(TARGET)/lib/pppd/2.4.3/
PLUGIN_PATH=$(PLUGIN_DIR)/rp-pppoe.so

# Configuration file paths
PPPOESERVER_PPPD_OPTIONS=/etc/ppp/pppoe-server-options

PATHS='-DPPPOE_PATH="$(PPPOE_PATH)"' '-DPPPD_PATH="$(PPPD_PATH)"' \
	'-DPLUGIN_PATH="$(PLUGIN_PATH)"' \
	'-DPPPOE_SERVER_OPTIONS="$(PPPOESERVER_PPPD_OPTIONS)"'

CFLAGS= -g -O2 -Wall -Wstrict-prototypes -ansi $(LIC_INCDIR) $(DEFINES) $(LIC_DEFINE) $(PATHS) -Ilibevent
# 	Date: 2009-7-14
# 	Name: Ken_Chiang
# 	Reason: modify for support pppoe relay feature.
# 	Notice :	
ifdef CONFIG_USER_PPPOE_RELAY
TARGETS=pppoe pppoe-relay
else
TARGETS=pppoe 
endif
PPPOE_SERVER_LIBS=$(LIC_LIBDIR) $(LIC_LIB)

all: $(TARGETS)
	@echo "$(TARGETS)"
	@echo "Type 'make install' as root to install the software."

pppoe-sniff: pppoe-sniff.o if.o common.o debug.o
	$(CC) -o pppoe-sniff pppoe-sniff.o if.o common.o debug.o

pppoe-server: pppoe-server.o if.o debug.o common.o md5.o libevent/libevent.a 
	$(CC) -o pppoe-server  pppoe-server.o if.o debug.o common.o md5.o $(PPPOE_SERVER_LIBS) -Llibevent -levent

pppoe: pppoe.o if.o debug.o common.o ppp.o discovery.o
	$(CC) -o pppoe pppoe.o if.o debug.o common.o ppp.o discovery.o

pppoe-relay: relay.o if.o debug.o common.o
	$(CC) -o pppoe-relay relay.o if.o debug.o common.o

pppoe.o: pppoe.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o pppoe.o pppoe.c

discovery.o: discovery.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o discovery.o discovery.c

ppp.o: ppp.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o ppp.o ppp.c

md5.o: md5.c md5.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o md5.o md5.c

pppoe-server.o: pppoe-server.c pppoe.h 
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o pppoe-server.o pppoe-server.c

pppoe-sniff.o: pppoe-sniff.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o pppoe-sniff.o pppoe-sniff.c

if.o: if.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o if.o if.c

libevent/libevent.a:
	cd libevent && $(MAKE)

common.o: common.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o common.o common.c

debug.o: debug.c pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o debug.o debug.c

relay.o: relay.c relay.h pppoe.h
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o relay.o relay.c

# Linux-specific plugin
rp-pppoe.so: plugin/libplugin.a plugin/plugin.o
	$(CC) -o rp-pppoe.so -shared plugin/plugin.o plugin/libplugin.a

plugin/plugin.o: plugin.c
	$(CC) '-DRP_VERSION="$(VERSION)"' $(CFLAGS) -I$(PPPD_INCDIR) -c -o plugin/plugin.o -fPIC plugin.c

plugin/libplugin.a: plugin/discovery.o plugin/if.o plugin/common.o plugin/debug.o
	ar -rc $@ $^

plugin/discovery.o: discovery.c
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o plugin/discovery.o -fPIC discovery.c

plugin/if.o: if.c
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o plugin/if.o -fPIC if.c

plugin/debug.o: debug.c
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o plugin/debug.o -fPIC debug.c

plugin/common.o: common.c
	$(CC) $(CFLAGS) '-DVERSION="$(VERSION)"' -c -o plugin/common.o -fPIC common.c

install: all
ifdef CONFIG_USER_PPPOE_RELAY
	$(install) -m 755 pppoe-relay $(sbindir)
endif
	#$(install) -m 755 pppoe $(sbindir)
	#$(STRIP) $(sbindir)/pppoe
	-mkdir -p $(PLUGIN_DIR)
	-echo "# Directory created by rp-pppoe for kernel-mode plugin" > $(PLUGIN_DIR)/README
	@if test -r rp-pppoe.so; then $(install) -m 755 rp-pppoe.so $(PLUGIN_DIR); fi

	# L2TP
	@if [ -f l2tp/handlers/sync-pppd.so ] ; then \
		mkdir -p $(RPM_INSTALL_ROOT)/usr/lib/l2tp/plugins ; \
		$(install) -m 755 l2tp/handlers/sync-pppd.so $(RPM_INSTALL_ROOT)/usr/lib/l2tp/plugins ; \
		mkdir -p $(RPM_INSTALL_ROOT)/etc/l2tp ; \
		$(install) -m 600 l2tp/l2tp.conf $(RPM_INSTALL_ROOT)/etc/l2tp/l2tp.conf.example ; \
	fi

servpoet-tgz: distro-servpoet
	cd .. && tar cvf servpoet-$(VERSION)$(BETA).tar servpoet-$(VERSION)$(BETA)
	gzip -f -v -9 ../servpoet-$(VERSION)$(BETA).tar
	rm -rf ../servpoet-$(VERSION)$(BETA)

tgz: distro
	cd .. && tar cvf rp-pppoe-$(VERSION)$(BETA).tar rp-pppoe-$(VERSION)$(BETA)
	gzip -f -v -9 ../rp-pppoe-$(VERSION)$(BETA).tar
	rm -rf ../rp-pppoe-$(VERSION)$(BETA)

distro-servpoet: distro
	cp ../servpoet.spec ../rp-pppoe-$(VERSION)$(BETA)
	$(MAKE) -C licensed-only distro VERSION=$(VERSION) BETA=$(BETA)
	mv ../rp-pppoe-$(VERSION)$(BETA) ../servpoet-$(VERSION)$(BETA)

distro:
	rm -rf ../rp-pppoe-$(VERSION)$(BETA)
	mkdir ../rp-pppoe-$(VERSION)$(BETA)
	for i in README SERVPOET go go-gui rp-pppoe.spec ; do \
		cp ../$$i ../rp-pppoe-$(VERSION)$(BETA) || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/gui
	for i in en.msg ja.msg Makefile.in tkpppoe.in wrapper.c tkpppoe.1 pppoe-wrapper.1 ; do \
		cp ../gui/$$i ../rp-pppoe-$(VERSION)$(BETA)/gui || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/gui/html
	for i in mainwin-busy.png mainwin-nonroot.png mainwin.png props-advanced.png props-basic.png props-nic.png props-options.png tkpppoe.html ; do \
		cp ../gui/html/$$i ../rp-pppoe-$(VERSION)$(BETA)/gui/html || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/configs
	for i in firewall-masq firewall-standalone pap-secrets pppoe-server-options pppoe.conf ; do \
		cp ../configs/$$i ../rp-pppoe-$(VERSION)$(BETA)/configs || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/doc
	for i in CHANGES KERNEL-MODE-PPPOE HOW-TO-CONNECT LICENSE PROBLEMS ; do \
		cp ../doc/$$i ../rp-pppoe-$(VERSION)$(BETA)/doc || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/man
	for i in pppoe-connect.8 pppoe-setup.8 pppoe-start.8 pppoe-status.8 pppoe-stop.8 pppoe-server.8 pppoe-sniff.8 pppoe.8 pppoe-relay.8 pppoe.conf.5 ; do \
		cp ../man/$$i ../rp-pppoe-$(VERSION)$(BETA)/man || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/scripts
	for i in pppoe-connect.in pppoe-init-suse.in pppoe-init-turbolinux.in pppoe-init.in pppoe-setup.in pppoe-start.in pppoe-stop.in pppoe-status ; do \
		cp ../scripts/$$i ../rp-pppoe-$(VERSION)$(BETA)/scripts || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/src
	for i in Makefile.in install-sh common.c config.h.in configure configure.in debug.c discovery.c if.c md5.c md5.h ppp.c pppoe-server.c pppoe-sniff.c pppoe.c pppoe.h pppoe-server.h plugin.c relay.c relay.h ; do \
		cp ../src/$$i ../rp-pppoe-$(VERSION)$(BETA)/src || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/src/libevent
	for i in Makefile.in event.c event.h event_tcp.c event_tcp.h eventpriv.h hash.c hash.h event_sig.c ; do \
		cp ../src/libevent/$$i ../rp-pppoe-$(VERSION)$(BETA)/src/libevent || exit 1; \
	done
	mkdir ../rp-pppoe-$(VERSION)$(BETA)/src/plugin

romfs:
	$(ROMFSINST) /sbin/pppoe
ifdef CONFIG_USER_PPPOE_RELAY
	$(ROMFSINST) /sbin/pppoe-relay
endif
distro-beta: beta-check
	$(MAKE) distro BETA=-BETA-$(BETA)

beta-check:
	@if test "$(BETA)" = "" ; then \
		echo "Usage: make distro-beta BETA=<x>"; \
		exit 1; \
	fi

rpms: tgz
	cp ../rp-pppoe-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	gpg --detach-sign /usr/src/redhat/SOURCES/rp-pppoe-$(VERSION).tar.gz
	cd ..; \
	rpm -ba rp-pppoe.spec
	rpm --addsign /usr/src/redhat/SRPMS/rp-pppoe-$(VERSION)-1.src.rpm \
		/usr/src/redhat/RPMS/i386/rp-pppoe-$(VERSION)-1.i386.rpm \
		/usr/src/redhat/RPMS/i386/rp-pppoe-gui-$(VERSION)-1.i386.rpm

servpoet-rpms: servpoet-tgz
	cp ../servpoet-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	cd .. && rpm -ba servpoet.spec

clean:
	rm -f *.o pppoe-relay pppoe pppoe-sniff pppoe-server core rp-pppoe.so plugin/*.o plugin/libplugin.a *~
	test -f licensed-only/Makefile && $(MAKE) -C licensed-only clean || true
	test -f libevent/Makefile && $(MAKE) -C libevent clean || true
	test -f l2tp/Makefile && $(MAKE) -C l2tp clean || true

distclean: clean
	rm -f Makefile config.h config.cache config.log config.status
	rm -f libevent/Makefile
	rm -f ../scripts/pppoe-connect ../scripts/pppoe-start ../scripts/pppoe-stop ../scripts/pppoe-init ../scripts/pppoe-setup ../scripts/pppoe-init-suse ../scripts/pppoe-init-turbolinux

update-version:
	sed -e 's/^Version: .*$$/Version: $(VERSION)/' ../rp-pppoe.spec > ../rp-pppoe.spec.new && mv ../rp-pppoe.spec.new ../rp-pppoe.spec
	sed -e 's+^Source: .*$$+Source: http://www.roaringpenguin.com/pppoe/rp-pppoe-$(VERSION).tar.gz+' ../rp-pppoe.spec > ../rp-pppoe.spec.new && mv ../rp-pppoe.spec.new ../rp-pppoe.spec
	test -f ../servpoet.spec && sed -e 's/^Version: .*$$/Version: $(VERSION)/' ../servpoet.spec > ../servpoet.spec.new && mv ../servpoet.spec.new ../servpoet.spec || true
	test -f ../servpoet.spec && sed -e 's+^Source: .*$$+Source: http://www.roaringpenguin.com/pppoe/servpoet-$(VERSION).tar.gz+' ../servpoet.spec > ../servpoet.spec.new && mv ../servpoet.spec.new ../servpoet.spec || true

# Convenience target for David!  Don't try to use this one.
km:
	./configure --enable-plugin=/home/dfs/Archive/PPP/ppp-2.4.0.pppoe4-patched-dfs

licensed-only:
	$(MAKE) -C licensed-only all VERSION=$(VERSION) SERVPOET_VERSION=$(SERVPOET_VERSION)

l2tp: libevent/libevent.a
	$(MAKE) -C l2tp all

.PHONY: update-version

.PHONY: clean

.PHONY: distclean

.PHONY: rpms

.PHONY: licensed-only

.PHONY: distro

.PHONY: l2tp
