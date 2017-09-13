VER = fontconfig-2.6.0
FREETYPE_PREFIX:=$(shell pkg-config --variable=prefix freetype2)

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/fontconfig/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-maintainer-mode \
	--disable-dependency-tracking \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-libtool-lock \
	--enable-libxml2 \
	--disable-docs \
	--with-gnu-ld \
	--with-pic \
	--with-arch=ubicom32 \
	--with-freetype-config=$(FREETYPE_PREFIX)/bin/freetype-config \
	--without-expat \
	--without-expat-includes \
	--without-expat-lib \
	--with-default-fonts=/usr/fonts \
	--with-cache-dir=$(ROOTDIR)/lib/fontconfig/build-ubicom/install/cache \
	--with-confdir=$(ROOTDIR)/lib/fontconfig/build-ubicom/install/conf \
	--with-docdir=$(ROOTDIR)/lib/fontconfig/build-ubicom/install/doc \
	LIBXML2_CFLAGS="$(pkg-config --cflags libxml-2.0)" \
	LIBXML2_LIBS="$(pkg-config --libs libxml-2.0)" 

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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/fontconfig/build-ubicom/install/lib/libfontconfig.so.1.3.0 /usr/lib/libfontconfig.so.1

.PHONY: all clean romfs
