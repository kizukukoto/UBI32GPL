# Generated automatically from Makefile.in by configure.
# Generated automatically from Makefile.in by configure.
#***********************************************************************
#
# Makefile
#
# Makefile for L2TP code.
#
# Copyright (C) 2002 Roaring Penguin Software Inc.
#
# This software may be distributed under the terms of the GNU General
# Public License, Version 2, or (at your option) any later version.
#
# LIC: GPL
#
# $Id: Makefile,v 1.1.1.1 2007/08/30 10:28:17 roy Exp $
#***********************************************************************

VERSION=0.4

DEFINES=
prefix=$(TARGET)
exec_prefix=${prefix}
docdir=/usr/local/doc/l2tp-$(VERSION)
install=/usr/bin/install -c
install_dir=/usr/bin/install -c -d
sbindir=${exec_prefix}/sbin
etcdir=$(TARGET)/etc

OBJS=auth.o debug.o dgram.o main.o md5.o network.o options.o peer.o session.o tunnel.o utils.o
EXTRA_LIBS=libevent/*.o -ldl

SRCS=$(OBJS:.o=.c)
CFLAGS+=-g -I.. -Ilibevent -Wall -Wstrict-prototypes -ansi -pedantic -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" -DPREFIX=\"\"

TARGETS=l2tpd libl2tp.a handlers

all: $(TARGETS)

libl2tp.a: $(OBJS)
	rm -f $@
	ar -rc $@ $^
	ranlib $@ || true

l2tpd: libl2tp.a libevent/libevent.a
	$(MAKE) -C handlers
	$(CC) -o l2tpd -rdynamic $(OBJS) $(EXTRA_LIBS)

libevent/libevent.a:
	test -d libevent || ln -s ../libevent .
	$(MAKE) -C libevent

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<

depend: .depend

.depend: $(SRCS)
	$(CC) -M $(CFLAGS) $^ > .depend

handlers:
	$(MAKE) -C handlers

# Release requires GNU tar!!
release: FORCE
	rm -f rp-l2tp-$(VERSION).tar.gz
	make -C libevent/Doc
	./make-release.sh $(VERSION)
	tar cvhf rp-l2tp-$(VERSION).tar rp-l2tp-$(VERSION)
	gzip -v -9 rp-l2tp-$(VERSION).tar

clean:
	rm -f *.o *.a *~
	rm -f l2tpd
	rm -f .depend
	$(MAKE) -C handlers clean
	$(MAKE) -C libevent clean

distclean: clean
	rm -f Makefile config.cache
	$(MAKE) -C handlers distclean

-include .depend

install: all
	-mkdir -p $(sbindir)
	cp l2tpd $(sbindir)/l2tpd
	$(STRIP) $(sbindir)/l2tpd
	$(MAKE) -C handlers install
#	-mkdir -p $(etcdir)/l2tp
#	cp l2tp.conf $(etcdir)/l2tp/l2tp.conf

romfs:
	$(ROMFSINST) l2tpd /sbin
	$(ROMFSINST) -d handlers/sync-pppd.so /lib/l2tp/plugins/sync-pppd.so
	$(ROMFSINST) handlers/cmd.so /lib/l2tp/plugins/cmd.so
	$(ROMFSINST) handlers/l2tp-control /sbin

FORCE:

.PHONY: handlers

.PHONY: FORCE
