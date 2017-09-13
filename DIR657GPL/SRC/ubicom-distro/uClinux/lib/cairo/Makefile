VER = cairo-1.8.6
VER-FREETYPE = freetype-2.3.8

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/cairo/build-ubicom/install \
	--host=$(CONFIGURE_HOST) \
	--build=$(HOSTARCH) \
	--target=$(TARGETARCH) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--disable-libtool-lock \
	--disable-gtk-doc \
	--disable-gcov \
	--enable-xlib=no \
	--enable-xlib-render=no \
	--enable-xcb=no \
	--enable-quartz=no \
	--enable-quartz-fonts=no \
	--enable-quartz-image=no \
	--enable-win32=no \
	--enable-beos=no \
	--enable-png \
	--enable-glitz=no \
	--enable-directfb=yes \
	--disable-pdf \
	--disable-ps \
	--disable-svg \
	--enable-test-surfaces=no \
	--disable-some-floating-point \
	--with-pic \
	--with-gnu-ld \
	--without-x \
	CFLAGS="$(CFLAGS) -DCAIRO_NO_MUTEX=1 -DCAIRO_HAS_DIRECTFB_SURFACE -DBOOST_SP_USE_PTHREADS" \
	FREETYPE_CFLAGS="$(pkg-config --cflags freetype2)" \
	FREETYPE_LIBS="$(pkg-config --libs freetype2)" \
	FONTCONFIG_CFLAGS="$(pkg-config --cflags fontconfig)" \
	FONTCONFIG_LIBS="$(pkg-config --libs fontconfig)" \
	pixman_CFLAGS="$(pkg-config --cflags pixman-1)" \
	pixman_LIBS="$(pkg-config --libs pixman-1)" \
	png_CFLAGS="$(pkg-config --cflags libpng12)" \
	png_LIBS="$(pkg-config --libs libpng12)" \
	directfb_CFLAGS="$(pkg-config --cflags direct fusion directfb)" \
	directfb_LIBS="$(pkg-config --libs direct fusion directfb)"

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

	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/cairo/build-ubicom/install/lib/libcairo.so.2.10800.6 /usr/lib/libcairo.so.2

.PHONY: all clean romfs
