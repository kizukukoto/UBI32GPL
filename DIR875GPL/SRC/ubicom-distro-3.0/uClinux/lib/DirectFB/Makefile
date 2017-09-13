# DirectFB makefile for Ubicom32
VER = DirectFB-1.3.0

BUILD_DIR:=build-ubicom
DIRECTFB_PREFIX:=$(ROOTDIR)/lib/DirectFB/$(BUILD_DIR)/install
LIBPNG_PREFIX:=$(shell pkg-config --variable=prefix libpng)

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) install > install.log

$(BUILD_DIR)/Makefile: $(VER)/configure Makefile
	set -e ; \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	LIBPNG_CONFIG="$(LIBPNG_PREFIX)/bin/libpng-config" \
	CFLAGS="$(CFLAGS) -I$(ROOTDIR)/linux-2.6.x/include -I$$PWD/include" \
	LDFLAGS="$(LDFLAGS) -L$$PWD/install/lib" \
	LIBS="-lc -lgcc" \
	../$(VER)/configure  \
	--host=$(CONFIGURE_HOST) \
	--disable-maintainer-mode \
	--disable-dependency-tracking \
	$(LIB_ENABLE_STATIC) \
	$(LIB_ENABLE_SHARED) \
	--disable-fast-install \
	--disable-libtool-lock \
	--disable-osx \
	--disable-x11 \
	--enable-extra-warnings \
	--disable-profiling \
	--enable-debug \
	--enable-debug-support \
	--disable-trace \
	--enable-text \
	--disable-gettid \
	--enable-network \
	--disable-multi \
	--disable-voodoo \
	--disable-unique \
	--disable-mmx \
	--disable-sse \
	--disable-devmem \
	--enable-fbdev \
	--disable-sdl \
	--disable-vnc \
	--disable-sysfs \
	--enable-jpeg \
	--enable-zlib \
	--enable-png \
	--disable-gif \
	--enable-freetype \
	--disable-video4linux \
	--disable-video4linux2 \
	--with-gnu-ld \
	--with-pic \
	--with-gfxdrivers=ubicom32vfb \
	--with-inputdrivers=keyboard,ps2mouse,linuxinput,tslib \
	--without-tools \
	--without-smooth-scaling \
	--without-sysroot \
	--prefix=$(DIRECTFB_PREFIX)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(DIRECTFB_PREFIX)/lib/libdirect-1.3.so.0.0.0   /usr/lib/libdirect-1.3.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(DIRECTFB_PREFIX)/lib/libdirectfb-1.3.so.0.0.0 /usr/lib/libdirectfb-1.3.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(DIRECTFB_PREFIX)/lib/libfusion-1.3.so.0.0.0   /usr/lib/libfusion-1.3.so.0
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(DIRECTFB_PREFIX)/lib/directfb-1.3-0           /usr/lib/directfb-1.3-0
	find $(ROMFSDIR)/usr/lib/directfb-1.3-0 -type f -name '*.la' -print0 | xargs -0 rm -f
	find $(ROMFSDIR)/usr/lib/directfb-1.3-0 -type f -name '*.o' -print0 | xargs -0 rm -f
	find $(ROMFSDIR)/usr/lib/directfb-1.3-0 -type f -name '*.a' -print0 | xargs -0 rm -f

.PHONY: all clean romfs
