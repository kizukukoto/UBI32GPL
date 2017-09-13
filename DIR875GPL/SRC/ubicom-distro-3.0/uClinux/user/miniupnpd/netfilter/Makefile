# $Id: Makefile,v 1.1 2009/04/21 11:14:48 jimmy_huang Exp $
CFLAGS=-Wall -g

all:	iptcrdr.o testiptcrdr

clean:
	$(RM) *.o testiptcrdr

testiptcrdr:	testiptcrdr.o iptcrdr.o -liptc

iptcrdr.o:	iptcrdr.c iptcrdr.h

