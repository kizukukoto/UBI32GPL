VER = atk-1.26.0

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/atk/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--disable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--disable-rebuilds \
	--disable-glibtest \
	--disable-gtk-doc \
	--with-gnu-ld \
	--with-pic \
	--without-html-dir \
	DEP_CFLAGS="$(pkg-config --cflags glib-2.0 gobject-2.0)" \
	DEP_LIBS="$(pkg-config --libs glib-2.0 gobject-2.0)"

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

	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/atk/build-ubicom/install/lib/libatk-1.0.so.0.2609.1 /usr/lib/libatk-1.0.so.0

.PHONY: all clean romfs
