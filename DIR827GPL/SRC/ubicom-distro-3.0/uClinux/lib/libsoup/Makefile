# Libsoup makefile for Ubicom32
VER = libsoup-2.28.1

CONF_OPTS = \
		--prefix=$(ROOTDIR)/lib/libsoup/build-ubicom \
		--host=$(CONFIGURE_HOST) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--enable-debug=minimum \
		$(LIB_ENABLE_STATIC) \
		$(LIB_ENABLE_SHARED) \
		--with-gnu-ld \
		--with-pic \
		--disable-glibtest \
		--disable-ssl \
	--disable-gtk-doc \
	--without-gnome

all: build-ubicom/Makefile
	$(MAKE) -C build-ubicom
	$(MAKE) -C build-ubicom install > install.log

build-ubicom/Makefile: Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf build-ubicom ; \
	mkdir build-ubicom ; \
	cd build-ubicom ; \
	../$(VER)/configure $(CONF_OPTS)

clean:
#	-$(MAKE) -C $(VER) distclean
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libsoup/build-ubicom/lib/libsoup-2.4.so /usr/lib/libsoup-2.4.so
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libsoup/build-ubicom/lib/libsoup-2.4.so.1 /usr/lib/libsoup-2.4.so.1
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libsoup/build-ubicom/lib/libsoup-2.4.so.1.3.0 /usr/lib/libsoup-2.4.so.1.3.0

.PHONY: all clean romfs
