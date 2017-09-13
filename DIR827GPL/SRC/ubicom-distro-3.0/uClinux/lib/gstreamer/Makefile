VER = gstreamer-0.10.22

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/gstreamer/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-maintainer-mode \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--disable-libtool-lock \
	--disable-largefile \
	--disable-docbook \
	--disable-gtk-doc \
	--with-pic \
	--with-gnu-ld \
	--without-html-dir \
	LIBS="-liconv" \
	GLIB_CFLAGS="$(pkg-config --cflags glib-2.0 gmodule-2.0 gobject-2.0 gthread-2.0)" \
	GLIB_LIBS="$(pkg-config --libs glib-2.0 gmodule-2.0 gobject-2.0 gthread-2.0)" \
	XML_CFLAGS="$(pkg-config --cflags libxml-2.0)" \
	XML_LIBS="$(pkg-config --libs libxml-2.0)" 

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
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gstreamer/build-ubicom/install/lib/libgstbase-0.10.so.0.19.0 /usr/lib/libgstbase-0.10.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gstreamer/build-ubicom/install/lib/libgstcontroller-0.10.so.0.19.0 /usr/lib/libgstcontroller-0.10.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gstreamer/build-ubicom/install/lib/libgstdataprotocol-0.10.so.0.19.0 /usr/lib/libgstdataprotocol-0.10.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gstreamer/build-ubicom/install/lib/libgstnet-0.10.so.0.19.0 /usr/lib/libgstnet-0.10.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gstreamer/build-ubicom/install/lib/libgstreamer-0.10.so.0.19.0 /usr/lib/libgstreamer-0.10.so.0


.PHONY: all clean romfs
