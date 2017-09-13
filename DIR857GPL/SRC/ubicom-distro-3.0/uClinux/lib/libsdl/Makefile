VER = SDL-1.2.13

BUILD_DIR=build-ubicom
LIBSDL_PREFIX:=$(ROOTDIR)/lib/libsdl/$(BUILD_DIR)/install

CONF_OPTS = \
	--prefix=$(LIBSDL_PREFIX) \
	--host=$(CONFIGURE_HOST) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-libtool-lock \
	--enable-libc=yes \
	--enable-audio=yes \
	--enable-video=yes \
	--enable-events=yes \
	--enable-joystick=no \
	--enable-cdrom=no \
	--enable-threads=yes \
	--enable-timers=yes \
	--enable-file=yes \
	--enable-loadso \
	--enable-cpuinfo=yes \
	--enable-assembly=yes \
	--enable-oss=no \
	--enable-alsa=yes \
	--enable-alsatest \
	--enable-alsa-shared=yes \
	--with-alsa-prefix=$(ROOTDIR)/lib \
	--with-alsa-inc-prefix=$(ROOTDIR)/include \
	--enable-esd=no \
	--disable-esdtest \
	--enable-esd-shared=no \
	--enable-pulseaudio=no \
	--enable-pulseaudio-shared=no \
	--enable-arts=no \
	--enable-arts-shared=no \
	--enable-nas=no \
	--enable-diskaudio=no \
	--enable-dummyaudio=no \
	--enable-mintaudio=no \
	--enable-nasm=no \
	--enable-altivec=no \
	--enable-ipod=no \
	--enable-video-nanox=no \
	--enable-nanox-debug=no \
	--enable-nanox-share-memory=no \
	--enable-nanox-direct-fb=no \
	--enable-video-x11=no \
	--enable-x11-shared=no \
	--enable-dga=no \
	--enable-video-dga=no \
	--enable-video-x11-dgamouse=no \
	--enable-video-x11-vm=no \
	--enable-video-x11-xv=no \
	--enable-video-x11-xinerama=no \
	--enable-video-x11-xme=no \
	--enable-video-x11-xrandr=no \
	--enable-video-x11-dpms=no \
	--enable-video-photon=no \
	--enable-video-carbon=no \
	--enable-video-cocoa=no \
	--enable-video-fbcon=yes \
	--enable-video-directfb=no \
	--enable-video-ps2gs=no \
	--enable-video-ggi=no \
	--enable-video-svga=no \
	--enable-video-vgl=no \
	--enable-video-wscons=no \
	--enable-video-aalib=no \
	--enable-video-qtopia=no \
	--enable-video-picogui=no \
	--enable-video-xbios=no \
	--enable-video-gem=no \
	--enable-video-dummy=no \
	--enable-video-opengl=no \
	--enable-osmesa-shared=no \
	--enable-input-events=yes \
	--enable-input-tslib=no \
	--enable-pth=no \
	--enable-pthreads=yes \
	--enable-pthread-sem=yes \
	--enable-stdio-redirect=no \
	--enable-directx=no \
	--enable-sdl-dlopen=no \
	--enable-atari-ldg=no \
	--enable-clock_gettime=no \
	--disable-rpath \
	--with-gnu-ld \
	--with-pic \
	--without-x \
	LIBS="-lc -lgcc"

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) install > install.log

$(BUILD_DIR)/Makefile: Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf $(LIBSDL_PREFIX); \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	../$(VER)/configure $(CONFIGURE_OPTS) $(CONF_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(LIBSDL_PREFIX)/lib/libSDL-1.2.so.0.11.2 /usr/lib/libSDL-1.2.so.0

.PHONY: all clean romfs
