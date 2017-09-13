VER = pango-1.20.5

FREETYPE_PREFIX:=$(shell pkg-config --variable=prefix freetype2)

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/pango/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--enable-debug=minimum \
	--disable-rebuilds \
	--disable-gtk-doc \
	--disable-man \
	--disable-doc-cross-references \
	--enable-explicit-deps=auto \
	--with-gnu-ld \
	--with-pic \
	--without-x  \
	--with-included-modules=yes \
	--with-dynamic-modules=no \
	FREETYPE_CFLAGS="$(pkg-config --cflags freetype2)" \
	FREETYPE_LIBS="$(pkg-config --libs freetype2)" \
	FREETYPE_CONFIG="$(FREETYPE_PREFIX)/bin/freetype-config" \
	FONTCONFIG_CFLAGS="$(pkg-config --cflags fontconfig)" \
	FONTCONFIG_LIBS="$(pkg-config --libs fontconfig)" \
	GLIB_CFLAGS="$(pkg-config --cflags glib-2.0 gmodule-2.0 gobject-2.0 gmodule-no-export-2.0)" \
	GLIB_LIBS="$(pkg-config --libs glib-2.0 gmodule-2.0 gobject-2.0 gmodule-no-export-2.0)" \
	CAIRO_CFLAGS="$(pkg-config --cflags cairo pixman-1)" \
	CAIRO_LIBS="$(pkg-config --libs cairo)"

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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/pango/build-ubicom/install/lib/libpango-1.0.so.0.2002.3 /usr/lib/libpango-1.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/pango/build-ubicom/install/lib/libpangocairo-1.0.so.0.2002.3 /usr/lib/libpangocairo-1.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/pango/build-ubicom/install/lib/libpangoft2-1.0.so.0.2002.3 /usr/lib/libpangoft2-1.0.so.0

.PHONY: all clean romfs
