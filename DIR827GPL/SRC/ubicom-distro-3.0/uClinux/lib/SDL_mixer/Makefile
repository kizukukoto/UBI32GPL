VER = SDL_mixer-1.2.8
VER-SDL = SDL-1.2.13

BUILD_DIR:=build-ubicom
LIBSDL_PREFIX:=$(shell pkg-config --variable=prefix sdl)

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/SDL_mixer/$(BUILD_DIR)/install \
	--host=$(CONFIGURE_HOST) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--disable-libtool-lock \
	--enable-music-cmd=yes \
	--enable-music-wave=yes \
	--enable-music-libmikmod=no \
	--enable-music-mod=yes \
	--enable-music-midi=yes \
	--enable-music-timidity-midi=yes \
	--enable-music-native-midi=yes \
	--enable-music-native-midi-gpl=no \
	--enable-music-ogg=yes \
	--enable-music-ogg-tremor=yes \
	--enable-music-ogg-shared=no \
	--enable-music-mp3=no \
	--disable-smpegtest \
	--enable-music-mp3-shared=no \
	--enable-music-mp3-mad-gpl=yes \
	--with-gnu-ld \
	--with-pic \
	--with-sdl-prefix=$(LIBSDL_PREFIX) \
	--without-smpeg-prefix \
	LIBS="-lm"

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) install > install.log

$(BUILD_DIR)/Makefile: Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	../$(VER)/configure $(CONF_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/SDL_mixer/$(BUILD_DIR)/install/lib/libSDL_mixer-1.2.so.0.2.6 /usr/lib/libSDL_mixer-1.2.so.0

.PHONY: all clean romfs
