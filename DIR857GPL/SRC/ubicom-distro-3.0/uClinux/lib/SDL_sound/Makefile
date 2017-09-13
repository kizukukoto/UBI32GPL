VER = SDL_sound-1.0.3
VER-SDL = SDL-1.2.13

BUILD_DIR:=build-ubicom
LIBSDL_PREFIX:=$(shell pkg-config --variable=prefix sdl)

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/SDL_sound/$(BUILD_DIR)/install \
	--host=$(CONFIGURE_HOST) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--disable-libtool-lock \
	--disable-sdltest \
	--enable-voc \
	--enable-wav \
	--enable-raw \
	--enable-aiff \
	--enable-au \
	--enable-shn \
	--enable-midi \
	--enable-smpeg \
	--enable-mpglib \
	--enable-mikmod \
	--enable-modplug \
	--enable-ogg \
	--enable-flac \
	--enable-speex \
	--enable-physfs \
	--with-gnu-ld \
	--with-pic \
	--with-sdl-prefix=$(LIBSDL_PREFIX) \
	LIBS="-lc -lgcc -lm"

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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/SDL_sound/$(BUILD_DIR)/install/lib/libSDL_sound-1.0.so.1.0.2 /usr/lib/libSDL_sound-1.0.so.1

.PHONY: all clean romfs
