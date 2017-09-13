VER = SDL_ttf-2.0.9
VER-SDL = SDL-1.2.13

BUILD_DIR:=build-ubicom
LIBSDL_PREFIX:=$(shell pkg-config --variable=prefix sdl)
FREETYPE_PREFIX:=$(shell pkg-config --variable=prefix freetype2)

CONF_OPTS = \
	--prefix=$(ROOTDIR)/lib/SDL_ttf/$(BUILD_DIR)/install \
	--host=$(CONFIGURE_HOST) \
	$(LIB_ENABLE_SHARED) \
	$(LIB_ENABLE_STATIC) \
	--disable-fast-install \
	--disable-dependency-tracking \
	--with-gnu-ld \
	--with-pic \
	--without-x \
	--with-freetype-prefix=$(FREETYPE_PREFIX) \
	--with-sdl-prefix=$(LIBSDL_PREFIX) \
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
	$(ROMFSINST) -d -e CONFIG_BINFMT_ELF_FDPIC $(ROOTDIR)/lib/SDL_ttf/$(BUILD_DIR)/install/lib/libSDL_ttf-2.0.so.0.6.3 /usr/lib/libSDL_ttf-2.0.so.0

.PHONY: all clean romfs
