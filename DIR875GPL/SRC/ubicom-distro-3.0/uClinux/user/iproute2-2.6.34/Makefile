
KERNEL_INCLUDE = $(ROOTDIR)/$(LINUXDIR)/include
export KERNEL_INCLUDE

DESTDIR=
ROOTDIR=$(DESTDIR)
LIBDIR=/usr/lib/
SBINDIR=/usr/sbin
CONFDIR=/etc/iproute2
DOCDIR=/usr/share/doc/iproute2
MANDIR=/usr/share/man
ARPDDIR=/var/lib/arpd

# Path to db_185.h include
DBM_INCLUDE:=$(ROOTDIR)/usr/include

SHARED_LIBS = y

DEFINES= -DRESOLVE_HOSTNAMES -DLIBDIR=\"$(LIBDIR)\"
ifneq ($(SHARED_LIBS),y)
DEFINES+= -DNO_SHARED_LIBS
endif

#options if you have a bind>=4.9.4 libresolv (or, maybe, glibc)
LDLIBS := -lresolv $(LDLIBS)
ADDLIB=

#options for decnet
ADDLIB+=dnet_ntop.o dnet_pton.o

#options for ipx
ADDLIB+=ipx_ntop.o ipx_pton.o

#CC = gcc
#HOSTCC = gcc
CCOPTS = -D_GNU_SOURCE -O2 -Wstrict-prototypes -Wall
ifdef not_defined
CFLAGS = $(CCOPTS) -I../include $(DEFINES)
else
CFLAGS += $(CCOPTS) -I../include -I$(ROOTDIR)/include $(DEFINES)
endif
YACCFLAGS = -d -t -v

LDLIBS += -L../lib -lnetlink -lutil
#LDLIBS := -L../lib -lnetlink -lip2util $(LDLIBS)

#SUBDIRS=lib ip tc
SUBDIRS=lib ip 

#LIBNETLINK=../lib/libnetlink.a ../lib/libip2util.a
LIBNETLINK=../lib/libnetlink.a ../lib/libutil.a

all: Config
	@set -e; \
	for i in $(SUBDIRS); \
	do $(MAKE) $(MFLAGS) -C $$i; done

Config:
	sh configure $(KERNEL_INCLUDE)

install: all
	install -m 0755 -d $(DESTDIR)$(SBINDIR)
	install -m 0755 -d $(DESTDIR)$(CONFDIR)
	install -m 0755 -d $(DESTDIR)$(ARPDDIR)
	install -m 0755 -d $(DESTDIR)$(DOCDIR)/examples
	install -m 0755 -d $(DESTDIR)$(DOCDIR)/examples/diffserv
	install -m 0644 README.iproute2+tc $(shell find examples -maxdepth 1 -type f) \
		$(DESTDIR)$(DOCDIR)/examples
	install -m 0644 $(shell find examples/diffserv -maxdepth 1 -type f) \
		$(DESTDIR)$(DOCDIR)/examples/diffserv
	@for i in $(SUBDIRS) doc; do $(MAKE) -C $$i install; done
	install -m 0644 $(shell find etc/iproute2 -maxdepth 1 -type f) $(DESTDIR)$(CONFDIR)
	install -m 0755 -d $(DESTDIR)$(MANDIR)/man8
	install -m 0644 $(shell find man/man8 -maxdepth 1 -type f) $(DESTDIR)$(MANDIR)/man8
	ln -sf tc-bfifo.8  $(DESTDIR)$(MANDIR)/man8/tc-pfifo.8
	ln -sf lnstat.8  $(DESTDIR)$(MANDIR)/man8/rtstat.8
	ln -sf lnstat.8  $(DESTDIR)$(MANDIR)/man8/ctstat.8
	ln -sf rtacct.8  $(DESTDIR)$(MANDIR)/man8/nstat.8
	ln -sf routel.8  $(DESTDIR)$(MANDIR)/man8/routef.8
	install -m 0755 -d $(DESTDIR)$(MANDIR)/man3
	install -m 0644 $(shell find man/man3 -maxdepth 1 -type f) $(DESTDIR)$(MANDIR)/man3

snapshot:
	echo "static const char SNAPSHOT[] = \""`date +%y%m%d`"\";" \
		> include/SNAPSHOT.h

clean:
	rm -f cscope.*
	@for i in $(SUBDIRS) doc; \
	do $(MAKE) $(MFLAGS) -C $$i clean; done
	rm -f Config

clobber: clean
	rm -f Config

distclean: clobber

cscope:
	cscope -b -q -R -Iinclude -sip -slib -smisc -snetem -stc

romfs: 
	$(ROMFSINST) etc /etc
	for i in $(SUBDIRS); \
	do $(MAKE) -C $$i romfs; done

.EXPORT_ALL_VARIABLES:
