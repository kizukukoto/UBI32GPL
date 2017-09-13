#PPPD := $(shell /usr/sbin/pppd --version 2>&1 | grep version)
#PPPD := $(patsubst pppd,,$(PPPD))
#PPPD := $(patsubst version,,$(PPPD))
#PPPD := $(strip $(PPPD))

# Ubicom: Modified 20091113

CONFOPTS_pppd_plugin := --disable-nls --disable-libtool-lock --enable-shared=yes
	
export KDIR=$(ROOTDIR)/linux-2.6.x

GCCLIBDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))

ACCEL_PPTP_LDFLAGS=-g -L$(ROOTDIR)/uClibc/lib -rpath-link $(ROOTDIR)/uClibc/lib -L$(GCCLIBDIR) -L$(ROOTDIR)/lib -rpath-link /$(ROOTDIR)/lib

default: module plugin pptpd
client: module plugin
server: default

module:
	echo "Building kernel module ldflags=$(LDFLAGS) TOOLC=$(GCCLIBDIR)"
	(cd kernel/driver; CFLAGS="$(CFLAGS)" LDFLAGS="$(ACCEL_PPTP_LDFLAGS)" LIBS="$(LDLIBS)" make )

plugin: pppd_plugin/Makefile
	echo Building pppd plugin
	(cd pppd_plugin; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" make)

pptpd: pptpd-1.3.3/Makefile
	echo Building pptpd
	(cd pptpd-1.3.3; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" make)

pppd_plugin/Makefile: pptpd-1.3.3/Makefile
	(cd pppd_plugin; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" pppd=$(ROOTDIR)/user/pppd/pppd/pppd pppd_ver=2.4.3 ./configure $(CONFIGURE_OPTS) $(CONFOPTS_pppd_plugin))

pptpd-1.3.3/Makefile:
	(cd pptpd-1.3.3; CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="${LDLIBS}" pppd=$(ROOTDIR)/user/pppd/pppd/pppd pppd_ver=2.4.3 ac_cv_header_libintl_h=no ./configure $(CONFIGURE_OPTS))


module_install: module
	(cd kernel/driver; make install)

plugin_install: plugin
	install -m 0644 pppd_plugin/src/.libs/pptp.so.0.0.0 /usr/lib/pppd/$(PPPD)/pptp.so

	install -m 0777 pppd_plugin/src/pptp_callmgr /bin/pptp_callmgr
client_install: module_install plugin_install

server_install: server module_install plugin_install
	(cd pptpd-1.3.3; make install)

clean:
	(cd kernel/driver; make clean)
#	(cd pppd_plugin; make clean)
#	(cd pptpd-1.3.3; make clean)
	(cd pppd_plugin; make distclean)
	(cd pptpd-1.3.3; make distclean)

distclean:
	(cd kernel/driver; make clean)
	(cd pppd_plugin; make distclean)
	(cd pptpd-1.3.3; make distclean)
	
romfs:
	$(ROMFSINST) pppd_plugin/src/.libs/pptp.so.0.0.0 /lib/pptp.so
	$(ROMFSINST) pppd_plugin/src/pptp_callmgr /bin/pptp_callmgr
	$(ROMFSINST) -d -e CONFIG_USER_ACCEL_PPTP_ACCEL_PPTPD pptpd-1.3.3/pptpctrl /usr/local/sbin/pptpctrl
	$(ROMFSINST) -e CONFIG_USER_ACCEL_PPTP_ACCEL_PPTPD pptpd-1.3.3/pptpd /bin/pptpd
	mkdir -p $(ROMFSDIR)/lib/modules
	cp kernel/driver/pptp.ko $(ROMFSDIR)/lib/modules
