VER = SDL_image-1.2.7
VER-SDL = SDL-1.2.13

BUILD_DIR:=build-ubicom
LIBSDL_PREFIX:=$(shell pkg-config --variable=prefix sdl)

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/SDL_image/$(BUILD_DIR)/install \
	--host=$(CONFIGURE_HOST) \
	--with-sdl-prefix=$(LIBSDL_PREFIX) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--enable-bmp \
	--enable-gif \
	--enable-jpg \
	--enable-jpg-shared \
	--enable-lbm \
	--enable-pcx \
	--enable-png \
	--enable-png-shared \
	--enable-pnm \
	--enable-tga \
	--enable-tif \
	--enable-tif-shared \
	--enable-xcf \
	--enable-xpm \
	--enable-xv \
	LIBS="-lm -lz"

all: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR)
	$(MAKE) -C $(BUILD_DIR) install > install.log

$(BUILD_DIR)/Makefile: Makefile
	find $(VER) -type f -print0 | xargs -0 touch -r $(VER)/configure
	set -e ; \
	rm -rf $(BUILD_DIR) ; \
	mkdir $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	../$(VER)/configure $(CONFIGURE_OPTS) $(CONF_OPTS)

clean:
	rm -rf build*

romfs:
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/SDL_image/$(BUILD_DIR)/install/lib/libSDL_image-1.2.so.0.1.6 /usr/lib/libSDL_image-1.2.so.0

.PHONY: all clean romfs
