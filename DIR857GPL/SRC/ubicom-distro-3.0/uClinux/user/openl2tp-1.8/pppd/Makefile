# BEGIN CONFIGURABLE SETTINGS

ifdef PPPD_INCDIR
CFLAGS.pppd=	-I$(PPPD_INCDIR)
endif
ifndef PPPD_LIBDIR
#PPPD_LIBDIR=	$(SYS_LIBDIR)/pppd/$(PPPD_VERSION)
PPPD_LIBDIR= $(ROMFSDIR)/lib/pppd/2.4.3
endif

# END CONFIGURABLE SETTINGS

CFLAGS	+= $(CFLAGS.optimize) -I../include -I.. -I$(ROOTDIR)/user/pppd \
	$(CFLAGS.pppd) -DHAVE_MULTILINK -fPIC -I$(ROOTDIR)/config
LDFLAGS	+= -shared

PLUGINS=	pppol2tp.so
PLUGINS+=	openl2tp.so

all:	$(PLUGINS)

pppol2tp.so: pppol2tp.c
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) -D_linux_=1 $^

openl2tp.so: openl2tp.c
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) -D_linux_=1 $^

install: pppol2tp.so
	$(INSTALL) -d $(DESTDIR)$(PPPD_LIBDIR)
	$(STRIP) pppol2tp.so
	$(STRIP) openl2tp.so
	$(INSTALL) pppol2tp.so $(DESTDIR)$(PPPD_LIBDIR)
	$(INSTALL) openl2tp.so $(DESTDIR)$(PPPD_LIBDIR)

clean:
	-rm -rf *.o *.so *.d
