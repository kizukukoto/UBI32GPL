VER = gtk+-2.16.0
VER-FREETYPE = freetype-2.3.8

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/gtk/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--build=$(HOSTARCH) \
	--target=$(TARGETARCH) \
	--disable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--disable-largefile \
	--enable-debug=minimum \
	--enable-shm=no \
	--enable-xkb=no \
	--enable-xinerama=no \
	--disable-rebuilds \
	--disable-visibility \
	--enable-explicit-deps=auto \
	--disable-glibtest \
	--disable-modules \
	--disable-cups \
	--with-pic \
	--with-gnu-ld \
	--with-xinput=no \
	--with-gdktarget=directfb \
	--with-libpng \
	--with-libjpeg \
	--without-libtiff \
	--without-libjasper \
	--with-included-loaders=png,jpeg \
	--without-x \
	CFLAGS="$(CFLAGS) -DBOOST_SP_USE_PTHREADS" \
	LIBS="-lz" \
	BASE_DEPENDENCIES_CFLAGS="$(pkg-config --cflags freetype2 fontconfig glib-2.0 gmodule-2.0 gobject-2.0 gthread-2.0 atk libpng direct fusion directfb  pixman-1 cairo pango pangocairo)" \
	BASE_DEPENDENCIES_LIBS="$(pkg-config --libs freetype2 fontconfig glib-2.0 gmodule-2.0 gobject-2.0 gthread-2.0 atk libpng direct fusion directfb cairo pango pangocairo)"

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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gtk/build-ubicom/install/lib/libgailutil.so.18.0.1 /usr/lib/libgailutil.so.18
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gtk/build-ubicom/install/lib/libgdk-directfb-2.0.so.0.1600.0 /usr/lib/libgdk-directfb-2.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gtk/build-ubicom/install/lib/libgdk_pixbuf-2.0.so.0.1600.0 /usr/lib/libgdk_pixbuf-2.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gtk/build-ubicom/install/lib/libgtk-directfb-2.0.so.0.1600.0 /usr/lib/libgtk-directfb-2.0.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/gtk/build-ubicom/install/lib/gtk-2.0 /usr/lib/gtk-2.0
	find $(ROMFSDIR)/usr/lib/gtk-2.0 -type f -name '*.la' -print0 | xargs -0 rm -f
	find $(ROMFSDIR)/usr/lib/gtk-2.0 -type f -name '*.h' -print0 | xargs -0 rm -f

.PHONY: all clean romfs
