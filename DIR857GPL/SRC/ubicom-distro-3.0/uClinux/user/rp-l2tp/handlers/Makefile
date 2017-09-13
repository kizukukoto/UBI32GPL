# Generated automatically from Makefile.in by configure.
# $Id: Makefile,v 1.1.1.1 2007/08/30 10:28:17 roy Exp $
# Makefile for LNS handlers
# LIC: GPL

prefix=$(TARGET)
exec_prefix=${prefix}
install=/usr/bin/install -c
install_dir=/usr/bin/install -c -d
sbindir=${exec_prefix}/sbin

HANDLERS=sync-pppd.so cmd.so

OBJS=pty.o sync-pppd.o dstring.o
SRCS=$(OBJS:.o=.c)
CFLAGS+=-g -O2 -I.. -I../libevent -I../../libevent -Wall

all: $(HANDLERS) l2tp-control

l2tp-control: cmd-control.o
	$(CC) -o l2tp-control cmd-control.o

cmd-control.o: cmd-control.c
	$(CC) -c -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

sync-pppd.so: pty.o sync-pppd.o
	$(CC) -shared -o $@ $^

cmd.so: cmd.o dstring.o
	$(CC) -shared -o $@ $^

clean:
	rm -f *.so *.o *~
	rm -f l2tp-control

install: all
	-mkdir -p $(sbindir)
	-mkdir -p $(prefix)/lib/l2tp/plugins
	$(install) -D l2tp-control $(sbindir)/l2tp-control
	$(STRIP)	$(sbindir)/l2tp-control
	cp $(HANDLERS) $(prefix)/lib/l2tp/plugins/
distclean: clean
