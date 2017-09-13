VER = libxml2-2.7.6

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/libxml2/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--enable-rebuild-docs=no \
	--with-gnu-ld \
	--with-pic \
	--without-debug \
	--without-ftp \
	--without-html \
	--with-http \
	--with-iconv=$(ROOTDIR)/lib/libiconv/build-ubicom/install \
	--without-legacy \
	--without-mem-debug \
	--with-minimum \
	--with-output \
	--without-pattern \
	--with-push=yes \
	--with-python=no \
	--without-reader \
	--without-run-debug \
	--with-sax1 \
	--with-schemas \
	--without-schematron \
	--with-tree \
	--without-valid \
	--without-writer \
	--without-xinclude \
	--with-xpath \
	--without-xptr \
	--without-modules \
	--with-zlib=$(ROOTDIR)/lib \
	CFLAGS="$(CFLAGS) -D_FILE_OFFSET_BITS=32"

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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/libxml2/build-ubicom/install/lib/libxml2.so.2.7.6 /usr/lib/libxml2.so.2

.PHONY: all clean romfs
