
include ${ROOTDIR}/../.config

VER = iptables-1.4.8

ifeq ($(CONFIG_LPJ_ultra7k), y)
CONFIG_FLAG=--disable-ipv4
endif

iptables-1.4.8:iptables-1.4.8/Makefile
	$(MAKE) -C $@

$(VER)/Makefile:
	cd $(VER); \
	./configure \
		--host=ubicom32-uclinux		\
		--prefix=/ \
		${CONFIG_FLAG}
romfs:
	cp $(VER)/ip6tables-multi $(ROMFSDIR)/bin
	$(STRIP) $(ROMFSDIR)/bin/ip6tables-multi
	cd $(ROMFSDIR)/bin && ln -sf ip6tables-multi ip6tables
	cd $(ROMFSDIR)/bin && ln -sf ip6tables-multi ip6tables-save
	cd $(ROMFSDIR)/bin && ln -sf ip6tables-multi ip6tables-restore
ifeq ($(CONFIG_LPJ_ultra8k), y) 	
	cp $(VER)/iptables-multi $(ROMFSDIR)/bin
	$(STRIP) $(ROMFSDIR)/bin/iptables-multi
	cd $(ROMFSDIR)/bin && ln -sf iptables-multi iptables
	cd $(ROMFSDIR)/bin && ln -sf iptables-multi iptables-save
	cd $(ROMFSDIR)/bin && ln -sf iptables-multi iptables-restore
endif

clean:
	$(MAKE) -C $(VER) distclean


#VER = iptables-1.4.2

##for debugging purposes##
#CFLAGS += -O0
#
#IPTABLES-CFLAGS = -I$(ROOTDIR)/user/iptables-new/$(VER)/include -I$(ROOTDIR)/user/iptables-new/build-$(VER)/include 
#
#all: build-$(VER)/Makefile
#	$(MAKE) -C build-$(VER)
#
#build-$(VER)/Makefile:
#	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
#	set -e ; \
#	rm -rf build-$(VER) ; \
#	mkdir build-$(VER) ; \
#	cd build-$(VER) ; \
#	CC="$(CC) $(IPTABLES-CFLAGS) $(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBS="$(LDLIBS)" ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes ../$(VER)/configure --enable-static --disable-shared $(CONFIGURE_OPTS)
#
#clean:
#	rm -rf build*
#

#romfs:
#	$(ROMFSINST) build-$(VER)/iptables-static /bin/
#	$(ROMFSINST) -s iptables-static /bin/iptables
#	$(ROMFSINST) -s iptables-static /bin/iptables-save
#	$(ROMFSINST) -s iptables-static /bin/iptables-restore
#	$(ROMFSINST) build-$(VER)/ip6tables-static /bin/
#	$(ROMFSINST) -s ip6tables-static /bin/ip6tables
#	$(ROMFSINST) -s ip6tables-static /bin/ip6tables-save
#	$(ROMFSINST) -s ip6tables-static /bin/ip6tables-restore
#	$(ROMFSINST) build-$(VER)/iptables-xml /bin/
#
#.PHONY: all clean romfs
#
