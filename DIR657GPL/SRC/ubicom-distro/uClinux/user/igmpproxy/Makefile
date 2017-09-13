#include ../.config

#SBINDIR=$(TARGET)/sbin
#ETCDIR=$(TARGET)/etc

#CFLAGS=-O

default : build.h igmpproxy

all	: igmpproxy

clean	:
	rm -f *~ *.o *.asm build.h igmpproxy  

romfs:
	$(ROMFSINST) igmpproxy /sbin/igmpproxy
	$(ROMFSINST) -d igmpproxy.conf /etc/igmpproxy.conf

#install : 
#	cp igmpproxy ${SBINDIR}
#	if [ ! -e ${ETCDIR}/igmpproxy.conf ]; then cp igmpproxy.conf ${ETCDIR}; fi

igmpproxy : igmpproxy.o config.o confread.o request.o udpsock.o mcgroup.o rttable.o \
			igmp.o ifvc.o callout.o kern.o syslog.o lib.o mroute-api.o

build.h	:  
	echo '#define BUILD "' `date +%y%m%d` '"' >build.h


